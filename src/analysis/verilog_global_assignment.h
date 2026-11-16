#pragma once

#include "parser/design_parser.h"

#include <cstddef>
#include <set>
#include <string>
#include <string_view>
#include <vector>

namespace fpga::analysis::detail {

// Bounded source adapter for native GJB10157.9.4.4. Native run gathers
// MODULE descendants FUNCTION/TASK/WIRE, admits WIRE only if +0xa2 != 0,
// removes every direct child name from one shared function candidate set and
// a separate shared task candidate set, and reports ASSIGN_EQ (not ASSIGN_LE)
// whose first child name equals a remaining candidate. Native parser error
// paths identify +0xa2 as the Verilog register-declaration flag, so this
// adapter admits explicit module `reg` declarations, including initialized
// declarations and ANSI output-reg ports.
// ev_aaa493aac258e6c8e9f4d04fcd2ebeb575f37e5d55e363a21c32a358a3f3965b
// ev_e9b91922eba7ad4818588c41bea1f1caf91c07a8afbea44c38d4d821c54b1117
struct GlobalRegBlockingWrite {
    std::size_t token{};
    bool function{};
    std::string signal;
};

[[nodiscard]] inline std::vector<GlobalRegBlockingWrite> globalRegBlockingWrites(
    const std::vector<parser::HdlToken>& tokens) {
    using parser::HdlTokenKind;
    std::vector<GlobalRegBlockingWrite> found;
    const auto word = [&](std::size_t i, std::string_view value) {
        return i < tokens.size() && tokens[i].kind == HdlTokenKind::Identifier &&
            tokens[i].text == value;
    };
    const auto findWord = [&](std::size_t from, std::size_t limit, std::string_view value) {
        while (from < limit && (value == ";" ? tokens[from].text != ";" : !word(from, value))) ++from;
        return from;
    };
    const auto namesInSimpleDeclaration = [&](std::size_t begin, std::size_t limit,
                                               std::set<std::string>& names,
                                               bool allowInitializer) {
        auto end = begin + 1;
        int brackets = 0, parens = 0, braces = 0;
        for (; end < limit; ++end) {
            if (tokens[end].text == "[") ++brackets;
            else if (tokens[end].text == "]") --brackets;
            else if (tokens[end].text == "(") ++parens;
            else if (tokens[end].text == ")") --parens;
            else if (tokens[end].text == "{") ++braces;
            else if (tokens[end].text == "}") --braces;
            else if (tokens[end].text == ";" && brackets == 0 && parens == 0 && braces == 0) break;
        }
        if (end == limit || brackets != 0 || parens != 0 || braces != 0) return begin + 1;
        auto segment = begin + 1;
        brackets = parens = braces = 0;
        for (auto i = segment; i <= end; ++i) {
            if (i != end) {
                if (tokens[i].text == "[") ++brackets;
                else if (tokens[i].text == "]") --brackets;
                else if (tokens[i].text == "(") ++parens;
                else if (tokens[i].text == ")") --parens;
                else if (tokens[i].text == "{") ++braces;
                else if (tokens[i].text == "}") --braces;
                if (tokens[i].text != "," || brackets || parens || braces) continue;
            }
            auto name = segment;
            while (name < i) {
                const auto& value = tokens[name].text;
                if (value == "signed" || value == "unsigned" || value == "reg" ||
                    value == "wire" || value == "logic") { ++name; continue; }
                if (value == "[") {
                    int depth = 1;
                    for (++name; name < i && depth; ++name) {
                        if (tokens[name].text == "[") ++depth;
                        else if (tokens[name].text == "]") --depth;
                    }
                    continue;
                }
                break;
            }
            if (name < i && tokens[name].kind == HdlTokenKind::Identifier &&
                (name + 1 == i || (allowInitializer && tokens[name + 1].text == "=")))
                names.insert(tokens[name].text);
            segment = i + 1;
        }
        return end + 1;
    };
    const auto headerRegNames = [&](std::size_t begin, std::size_t end,
                                    std::set<std::string>& names) {
        for (auto reg = begin; reg < end; ++reg) {
            if (!word(reg, "reg")) continue;
            bool portDeclaration = false;
            int nesting = 0;
            for (auto cursor = reg; cursor > begin;) {
                --cursor;
                if (tokens[cursor].text == ")") ++nesting;
                else if (tokens[cursor].text == "(" && nesting > 0) --nesting;
                if (nesting == 0 && (tokens[cursor].text == "," ||
                                    tokens[cursor].text == "(" ||
                                    tokens[cursor].text == "#")) break;
                if (nesting == 0 && (word(cursor, "input") || word(cursor, "output") ||
                                    word(cursor, "inout"))) {
                    portDeclaration = true;
                    break;
                }
            }
            if (!portDeclaration) continue;
            auto segment = reg + 1;
            int brackets = 0, braces = 0, parens = 0;
            for (auto cursor = reg + 1; cursor <= end; ++cursor) {
                const bool terminal = cursor == end;
                if (!terminal) {
                    if (tokens[cursor].text == "[") ++brackets;
                    else if (tokens[cursor].text == "]") --brackets;
                    else if (tokens[cursor].text == "{") ++braces;
                    else if (tokens[cursor].text == "}") --braces;
                    else if (tokens[cursor].text == "(") ++parens;
                    else if (tokens[cursor].text == ")" && parens > 0) --parens;
                    if (brackets == 0 && braces == 0 && parens == 0 &&
                        (word(cursor, "input") || word(cursor, "output") ||
                         word(cursor, "inout"))) break;
                    if (tokens[cursor].text != "," || brackets || braces || parens) continue;
                }
                int itemBrackets = 0;
                for (auto item = segment; item < cursor; ++item) {
                    if (tokens[item].text == "[") { ++itemBrackets; continue; }
                    if (tokens[item].text == "]") { --itemBrackets; continue; }
                    if (itemBrackets != 0 || tokens[item].kind != HdlTokenKind::Identifier ||
                        word(item, "signed") || word(item, "unsigned") ||
                        word(item, "reg")) continue;
                    names.insert(tokens[item].text);
                    break;
                }
                segment = cursor + 1;
            }
        }
    };
    const auto blockingAssignmentAfter = [&](std::size_t name, std::size_t end) {
        auto cursor = name + 1;
        while (cursor < end && tokens[cursor].text == "[") {
            int depth = 1;
            for (++cursor; cursor < end && depth != 0; ++cursor) {
                if (tokens[cursor].text == "[") ++depth;
                else if (tokens[cursor].text == "]") --depth;
            }
            if (depth != 0) return false;
        }
        return cursor < end && tokens[cursor].text == "=";
    };
    struct Subprogram {
        std::size_t begin;
        std::size_t end;
        std::size_t headerEnd;
        std::string name;
        bool function;
        bool hasMacro;
    };
    for (auto module = std::size_t{0}; module < tokens.size(); ++module) {
        if (!word(module, "module")) continue;
        const auto moduleEnd = findWord(module + 1, tokens.size(), "endmodule");
        if (moduleEnd == tokens.size()) break;
        auto headerEnd = findWord(module + 1, moduleEnd, ";");
        if (headerEnd == moduleEnd) { module = moduleEnd; continue; }
        std::set<std::string> globals;
        headerRegNames(module + 1, headerEnd, globals);
        std::vector<Subprogram> subprograms;
        int blockDepth = 0;
        for (auto i = headerEnd + 1; i < moduleEnd;) {
            if (word(i, "begin")) { ++blockDepth; ++i; continue; }
            if (word(i, "end")) { if (blockDepth > 0) --blockDepth; ++i; continue; }
            const bool function = word(i, "function");
            if (function || word(i, "task")) {
                const auto end = findWord(i + 1, moduleEnd,
                                          function ? "endfunction" : "endtask");
                if (end == moduleEnd) break;
                const auto declarationEnd = findWord(i + 1, end, ";");
                if (declarationEnd != end) {
                    auto name = std::string{};
                    for (auto j = i + 1; j < declarationEnd; ++j) {
                        if (tokens[j].text == "(") break;
                        if (tokens[j].kind == HdlTokenKind::Identifier)
                            name = tokens[j].text;
                    }
                    subprograms.push_back({i, end, declarationEnd, name, function, false});
                }
                i = end + 1;
            } else if (blockDepth == 0 && word(i, "reg")) {
                i = namesInSimpleDeclaration(i, moduleEnd, globals, true);
            } else {
                ++i;
            }
        }
        std::set<std::string> functionChildren;
        std::set<std::string> taskChildren;
        for (auto& scope : subprograms) {
            std::set<std::string> locals;
            if (!scope.name.empty()) locals.insert(scope.name);
            // Header arguments can shadow module names; exclude every
            // identifier in the parameter list rather than guessing its type.
            bool inArguments = false;
            for (auto i = scope.begin + 1; i < scope.headerEnd; ++i) {
                if (tokens[i].text == "(") inArguments = true;
                else if (tokens[i].text == ")") inArguments = false;
                else if (inArguments && tokens[i].kind == HdlTokenKind::Identifier)
                    locals.insert(tokens[i].text);
            }
            for (auto i = scope.headerEnd + 1; i < scope.end; ++i) {
                if (tokens[i].text == "`") { scope.hasMacro = true; continue; }
                if (word(i, "input") || word(i, "output") || word(i, "inout") ||
                    word(i, "reg") || word(i, "wire") || word(i, "logic") ||
                    word(i, "bit") || word(i, "integer") || word(i, "int") ||
                    word(i, "byte") || word(i, "shortint") || word(i, "longint") ||
                    word(i, "parameter") || word(i, "localparam"))
                    i = namesInSimpleDeclaration(i, scope.end, locals, true) - 1;
            }
            auto& children = scope.function ? functionChildren : taskChildren;
            children.insert(locals.begin(), locals.end());
        }
        std::set<std::string> functionGlobals = globals;
        std::set<std::string> taskGlobals = globals;
        for (const auto& name : functionChildren) functionGlobals.erase(name);
        for (const auto& name : taskChildren) taskGlobals.erase(name);
        for (const auto& scope : subprograms) {
            if (scope.hasMacro) continue;
            const auto& candidates = scope.function ? functionGlobals : taskGlobals;
            for (auto i = scope.headerEnd + 1; i + 1 < scope.end; ++i) {
                if (tokens[i].kind != HdlTokenKind::Identifier ||
                    !candidates.contains(tokens[i].text) || !blockingAssignmentAfter(i, scope.end) ||
                    (i > 0 && (tokens[i - 1].text == "." || tokens[i - 1].text == "::")))
                    continue;
                found.push_back({i, scope.function, tokens[i].text});
            }
        }
        module = moduleEnd;
    }
    return found;
}

} // namespace fpga::analysis::detail
