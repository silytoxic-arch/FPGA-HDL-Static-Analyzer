#include "parser/preprocessor.h"
#include "parser/frontend_preprocessor.h"
#include "parser/design_parser.h"
#include <unordered_set>

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <optional>
#include <sstream>
#include <unordered_map>

namespace fpga::parser {
namespace {

std::string trim(std::string text) {
    const auto notSpace = [](unsigned char character) { return !std::isspace(character); };
    const auto begin = std::ranges::find_if(text, notSpace);
    if (begin == text.end()) return {};
    const auto end = std::find_if(text.rbegin(), text.rend(), notSpace).base();
    return {begin, end};
}

std::pair<std::string, std::string> firstWord(std::string text) {
    text = trim(std::move(text));
    const auto split = std::ranges::find_if(text, [](unsigned char character) {
        return std::isspace(character);
    });
    if (split == text.end()) return {text, {}};
    return {std::string(text.begin(), split), trim(std::string(split, text.end()))};
}

std::string macroName(std::string definition) {
    auto [name, ignored] = firstWord(std::move(definition));
    static_cast<void>(ignored);
    const auto parameterList = name.find('(');
    if (parameterList != std::string::npos) name.erase(parameterList);
    return name;
}

std::optional<std::filesystem::path> resolveInclude(
    const std::filesystem::path& sourceFile, const std::string& requested,
    const PreprocessorOptions& options) {
    std::vector<std::filesystem::path> candidates;
    candidates.emplace_back(requested); // Retained frontend tries CWD before source parent.
    candidates.push_back(sourceFile.parent_path() / requested);
    for (const auto& directory : options.includeDirectories) {
        candidates.push_back(directory / requested);
    }
    for (const auto& candidate : candidates) {
        std::error_code error;
        if (std::filesystem::is_regular_file(candidate, error)) return candidate.lexically_normal();
    }
    return std::nullopt;
}

std::optional<std::string> includeName(const std::string& arguments) {
    const auto begin = arguments.find_first_of("\"<");
    if (begin == std::string::npos) return std::nullopt;
    const char close = arguments[begin] == '"' ? '"' : '>';
    const auto end = arguments.find(close, begin + 1);
    if (end == std::string::npos || end == begin + 1) return std::nullopt;
    return arguments.substr(begin + 1, end - begin - 1);
}

std::string blank(std::string line) {
    for (char& character : line) {
        if (character != '\r' && character != '\n') character = ' ';
    }
    return line;
}

struct ConditionalFrame {
    bool parentActive;
    bool branchTaken;
    core::SourceLocation location;
};

} // namespace

PreprocessorResult preprocess(const core::SourceFile& source,
                              const PreprocessorOptions& options,
                              const FrontendPreprocessorResult* validatedFrontend) {
    if (source.language() == core::HdlLanguage::Vhdl) {
        return {source, {}};
    }
    // Match the recovered default read_hdl/read_verilog branch. These are
    // ordinary definitions: source `undef and `resetall can remove them.
    std::unordered_map<std::string, std::string> macros{{"YOSYS","1"},{"SYNTHESIS","1"}};
    for (const auto& definition : options.defines) {
        const auto equals = definition.find('=');
        const auto name = macroName(definition.substr(0, equals));
        if (!name.empty()) {
            macros[name] = equals == std::string::npos ? "1" : definition.substr(equals + 1);
        }
    }
    if(validatedFrontend) {
        macros.clear();
        for(const auto& name:validatedFrontend->initialMacroNames) macros[name]="1";
    }

    std::unordered_set<std::size_t> directiveOffsets;
    for (const auto& token:tokenize(source))
        if (token.text=="`") directiveOffsets.insert(token.offset);
    bool frontendIncludeValidation=false;
    std::vector<PreprocessorIssue> issues;
    std::vector<ConditionalFrame> conditionals;
    bool active = true;
    std::string output;
    const auto& content = source.content();
    std::size_t offset = 0;
    std::size_t lineNumber = 1;
    while (offset < content.size()) {
        const auto newline = content.find('\n', offset);
        const auto length = newline == std::string::npos ? content.size() - offset : newline - offset + 1;
        auto line = content.substr(offset, length);
        const auto first = line.find_first_not_of(" \t\r");
        const bool directiveLine = first != std::string::npos && line[first] == '`' &&
            directiveOffsets.contains(offset+first);
        if (!directiveLine) {
            output += active ? line : blank(std::move(line));
            offset += length;
            ++lineNumber;
            continue;
        }

        auto [directive, arguments] = firstWord(line.substr(first + 1));
        const auto location = core::SourceLocation{source.path(), lineNumber, first + 1, offset + first};
        if (directive == "ifdef" || directive == "ifndef") {
            const auto name = macroName(arguments);
            const bool defined = macros.contains(name);
            const bool condition = directive == "ifdef" ? defined : !defined;
            conditionals.push_back({active, condition, location});
            active = active && condition;
        } else if (directive == "elsif") {
            if (conditionals.empty()) {
                issues.push_back({"PP-UNEXPECTED-DIRECTIVE",
                                  "`elsif has no matching `ifdef or `ifndef", location});
            } else {
                auto& frame = conditionals.back();
                const bool condition = macros.contains(macroName(arguments));
                active = frame.parentActive && !frame.branchTaken && condition;
                frame.branchTaken = frame.branchTaken || condition;
            }
        } else if (directive == "else") {
            if (conditionals.empty()) {
                issues.push_back({"PP-UNEXPECTED-DIRECTIVE",
                                  "`else has no matching `ifdef or `ifndef", location});
            } else {
                auto& frame = conditionals.back();
                active = frame.parentActive && !frame.branchTaken;
                frame.branchTaken = true;
            }
        } else if (directive == "endif") {
            if (conditionals.empty()) {
                issues.push_back({"PP-UNEXPECTED-DIRECTIVE",
                                  "`endif has no matching `ifdef or `ifndef", location});
            } else {
                active = conditionals.back().parentActive;
                conditionals.pop_back();
            }
        } else if (active && directive == "define") {
            const auto name = macroName(arguments);
            if (name.empty()) {
                issues.push_back({"PP-INVALID-DIRECTIVE", "`define requires a macro name", location});
            } else {
                macros[name] = arguments;
            }
        } else if (active && directive == "undef") {
            macros.erase(macroName(arguments));
        } else if (active && directive == "resetall") {
            macros.clear();
        } else if (active && directive == "include" && arguments.find('`')!=std::string::npos) {
            // Function/default/quoted include macros require the real token
            // expansion engine. Do not mislabel a valid macro as a bad path.
            frontendIncludeValidation=true;
        } else if (active && directive == "include") {
            const auto requested = includeName(arguments);
            if (!requested) {
                issues.push_back({"PP-INVALID-DIRECTIVE",
                                  "`include requires a quoted or angle-bracket file name", location});
            } else if (!resolveInclude(source.path(), *requested, options)) {
                issues.push_back({"PP-INCLUDE-NOT-FOUND",
                                  "Included file was not found: " + *requested, location});
            }
        }
        output += blank(std::move(line));
        offset += length;
        ++lineNumber;
    }
    for (const auto& frame : conditionals) {
        issues.push_back({"PP-UNCLOSED-CONDITIONAL",
                          "Conditional compilation block has no matching `endif", frame.location});
    }
    if (validatedFrontend) {
        // Native expansion is authoritative for directive/include validity;
        // do not run an isolated second engine with a different macro cache.
        issues=validatedFrontend->issues;
    } else if (frontendIncludeValidation) {
        const auto expanded=expandFrontendSource(source,options);
        issues.insert(issues.end(),expanded.issues.begin(),expanded.issues.end());
    }
    return {core::SourceFile(source.path(), source.language(), std::move(output)),
            std::move(issues)};
}

} // namespace fpga::parser
