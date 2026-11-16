#pragma once

#include "analysis/verilog_naming_style.h"
#include "parser/design_parser.h"

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace fpga::analysis {

struct VerilogNamingDeclaration {
    std::size_t tokenIndex;
    VerilogNamingSubject subject;
    std::string normalizedName;
    // Owning module as established by the same bounded source traversal.  It
    // is needed by native rules whose diagnostic surface and traversal order
    // include the AST_MODULE map key (notably GJB10157.5.2.17).
    std::string moduleName;
};

// Conservative source-to-subject adapter, not a reconstruction of the complete
// vendor frontend. Supports explicit module parameters and wire/reg/logic/ports
// with optional packed ranges. Unpacked arrays/memories, unknown types, macros,
// class/function/task subtrees and implicit net declarations are left unproved.
// Native 5.2.15 predicates and task/function collection boundary are documented
// in naming-5-2-15-findings.md. Missing declarations DO NOT establish compliance.
class VerilogNamingDeclarations {
public:
    VerilogNamingDeclarations(const core::SourceFile& source,
                              const std::vector<parser::HdlToken>& tokens,
                              bool includeSubprogramBodies = false)
        : source_(source), tokens_(tokens), includeSubprogramBodies_(includeSubprogramBodies) {}

    [[nodiscard]] std::vector<VerilogNamingDeclaration> collect() const {
        std::vector<VerilogNamingDeclaration> result;
        bool module = false;
        std::string moduleName;
        std::size_t conditionalDepth = 0;
        std::vector<std::string> excluded;
        for (std::size_t i = 0; i < tokens_.size();) {
            if (tokens_[i].text == "`") {
                if (keyword(i + 1, "ifdef") || keyword(i + 1, "ifndef")) ++conditionalDepth;
                else if (keyword(i + 1, "endif") && conditionalDepth != 0) --conditionalDepth;
                i = skipDirectiveLine(i); continue;
            }
            if (conditionalDepth != 0) { ++i; continue; }
            if (keyword(i, "module")) {
                module = true;
                moduleName.clear();
                excluded.clear();
                auto name = i + 1;
                if (keyword(name, "automatic") || keyword(name, "static")) ++name;
                if (name < tokens_.size() &&
                    tokens_[name].kind == parser::HdlTokenKind::Identifier) {
                    moduleName = tokens_[name].text;
                    const auto first = moduleName.find_first_not_of('\\');
                    if (first == std::string::npos) moduleName.clear();
                    else moduleName.erase(0, first);
                }
                ++i;
                continue;
            }
            if (keyword(i, "endmodule")) {
                module = false;
                moduleName.clear();
                excluded.clear();
                ++i;
                continue;
            }
            if (!module) { ++i; continue; }
            // Opt-in only for rules whose native recursive collector includes
            // task/function descendants. Existing 5.2.15 callers retain the
            // original exclusion. Skip the header; a second adapter handles
            // the subprogram name itself, not arbitrary use identifiers.
            if (includeSubprogramBodies_ && excluded.empty() &&
                (keyword(i,"function") || keyword(i,"task"))) {
                // Do not reinterpret a function's return type (e.g. reg) as
                // a net declaration and consume its ANSI argument list.
                ++i;
                while(i<tokens_.size() && tokens_[i].text!="(" && tokens_[i].text!=";") ++i;
                if(i<tokens_.size()) ++i;
                continue;
            }
            if (keyword(i, "function") || keyword(i, "task") || keyword(i, "class") ||
                keyword(i, "interface") || keyword(i, "package") || keyword(i, "program")) {
                excluded.push_back(keyword(i, "function") ? "endfunction" :
                    keyword(i, "task") ? "endtask" : keyword(i, "class") ? "endclass" :
                    keyword(i, "interface") ? "endinterface" : keyword(i, "package") ? "endpackage" :
                    "endprogram");
                ++i; continue;
            }
            if (!excluded.empty()) {
                if (keyword(i, excluded.back())) excluded.pop_back();
                ++i; continue;
            }
            if (keyword(i, "typedef") || keyword(i, "struct") || keyword(i, "union") ||
                keyword(i, "enum")) { i = skipStatement(i); continue; }
            const auto subject = declarationKind(i);
            if (subject == VerilogNamingSubject::Unsupported) { ++i; continue; }
            i = declaration(i, subject, moduleName, result);
        }
        return result;
    }

private:
    const core::SourceFile& source_;
    const std::vector<parser::HdlToken>& tokens_;
    bool includeSubprogramBodies_{false};

    bool escaped(std::size_t i) const {
        return i < tokens_.size() && tokens_[i].offset < source_.content().size() &&
            source_.content()[tokens_[i].offset] == '\\';
    }
    bool keyword(std::size_t i, std::string_view text) const {
        return i < tokens_.size() && tokens_[i].kind == parser::HdlTokenKind::Identifier &&
            !escaped(i) && tokens_[i].text == text;
    }
    VerilogNamingSubject declarationKind(std::size_t i) const {
        if (keyword(i, "parameter")) return VerilogNamingSubject::Parameter;
        if (keyword(i, "localparam")) return VerilogNamingSubject::Localparam;
        if (keyword(i, "wire") || keyword(i, "reg") || keyword(i, "logic") ||
            keyword(i, "input") || keyword(i, "output") || keyword(i, "inout"))
            return VerilogNamingSubject::WireLike;
        return VerilogNamingSubject::Unsupported;
    }
    static bool qualifier(std::string_view text) {
        return text == "signed" || text == "unsigned" || text == "wire" ||
            text == "reg" || text == "logic" || text == "var";
    }
    static bool parameterType(std::string_view text) {
        return qualifier(text) || text == "integer" || text == "int" || text == "bit" ||
            text == "byte" || text == "shortint" || text == "longint" || text == "time" ||
            text == "real" || text == "realtime" || text == "shortreal";
    }
    std::size_t skipDirectiveLine(std::size_t start) const {
        auto line = tokens_[start].line;
        auto i = start + 1;
        while (i < tokens_.size()) {
            if (tokens_[i].line > line) {
                // A continued preprocessor line cannot expose declaration
                // keywords in the continuation as actual HDL declarations.
                const auto offset = tokens_[i - 1].offset;
                const auto newline = source_.content().find('\n', offset);
                auto end = newline == std::string::npos ? source_.content().size() : newline;
                while (end > offset && (source_.content()[end - 1] == ' ' ||
                       source_.content()[end - 1] == '\t' || source_.content()[end - 1] == '\r')) --end;
                if (end == 0 || source_.content()[end - 1] != '\\') break;
                line = tokens_[i].line;
            }
            ++i;
        }
        return i;
    }
    std::size_t skipStatement(std::size_t start) const {
        int parens = 0, brackets = 0, braces = 0;
        for (auto i = start; i < tokens_.size(); ++i) {
            const auto& text = tokens_[i].text;
            if (text == ";" && parens == 0 && brackets == 0 && braces == 0) return i + 1;
            if (keyword(i, "endmodule")) return i;
            if (text == "(") ++parens;
            else if (text == ")") --parens;
            else if (text == "[") ++brackets;
            else if (text == "]") --brackets;
            else if (text == "{") ++braces;
            else if (text == "}") --braces;
        }
        return tokens_.size();
    }
    void segment(std::size_t start, std::size_t end, VerilogNamingSubject& subject,
                 bool& supported, std::string_view moduleName,
                 std::vector<VerilogNamingDeclaration>& output) const {
        if (start >= end) return;
        if (const auto explicitKind = declarationKind(start);
            explicitKind != VerilogNamingSubject::Unsupported) {
            subject = explicitKind; supported = true; ++start;
        }
        if (!supported) return;
        for (auto j = start; j < end; ++j) if (tokens_[j].text == "`") { supported = false; return; }
        const bool parameter = subject == VerilogNamingSubject::Parameter ||
                               subject == VerilogNamingSubject::Localparam;
        auto name = start;
        while (name < end) {
            if (!escaped(name) && tokens_[name].kind == parser::HdlTokenKind::Identifier &&
                (parameter ? parameterType(tokens_[name].text) : qualifier(tokens_[name].text))) {
                ++name; continue;
            }
            if (tokens_[name].text == "[") {
                int depth = 1;
                for (++name; name < end && depth != 0; ++name) {
                    if (tokens_[name].text == "[") ++depth;
                    else if (tokens_[name].text == "]") --depth;
                }
                if (depth != 0) return;
                continue;
            }
            break;
        }
        if (name >= end || tokens_[name].kind != parser::HdlTokenKind::Identifier) return;
        // More than one pre-initializer identifier is an unknown datatype or
        // unsupported qualifier, not an invitation to guess the last name.
        if (name + 1 < end && tokens_[name + 1].text != "=") {
            if (tokens_[name + 1].text != "[") supported = false;
            return; // post-name dimensions are unpacked arrays/memories
        }
        if (!escaped(name) && (tokens_[name].text == "type" || tokens_[name].text == "struct" ||
            tokens_[name].text == "union" || tokens_[name].text == "enum")) {
            supported = false; return;
        }
        auto normalized = tokens_[name].text;
        const auto first = normalized.find_first_not_of('\\');
        if (first == std::string::npos) return; // native substring(-1) boundary is unknown
        normalized.erase(0, first);
        output.push_back({name, subject, std::move(normalized), std::string(moduleName)});
    }
    std::size_t declaration(std::size_t begin, VerilogNamingSubject subject,
                            std::string_view moduleName,
                            std::vector<VerilogNamingDeclaration>& output) const {
        std::size_t start = begin + 1;
        int parens = 0, brackets = 0, braces = 0;
        bool supported = true;
        for (auto i = start; i < tokens_.size(); ++i) {
            const auto& text = tokens_[i].text;
            const bool flat = parens == 0 && brackets == 0 && braces == 0;
            if (keyword(i, "endmodule") || keyword(i, "endfunction") || keyword(i, "endtask")) return i;
            if (flat && (text == ";" || text == ")")) {
                segment(start, i, subject, supported, moduleName, output);
                return i + 1;
            }
            if (flat && text == ",") {
                segment(start, i, subject, supported, moduleName, output);
                start = i + 1; continue;
            }
            if (text == "(") ++parens;
            else if (text == ")") --parens;
            else if (text == "[") ++brackets;
            else if (text == "]") --brackets;
            else if (text == "{") ++braces;
            else if (text == "}") --braces;
        }
        return tokens_.size();
    }
};

} // namespace fpga::analysis
