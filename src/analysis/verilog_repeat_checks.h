#pragma once

#include "analysis/verilog_constant_expression.h"

#include <algorithm>
#include <cstddef>
#include <set>
#include <string>
#include <vector>

namespace fpga::analysis {

// GJB10157.9.3.6 bounded, independent source check. The vendor collector
// selects AST_REPEAT/AST_PARAMETER/AST_LOCALPARAM, and its run walks every
// AST_IDENTIFIER below the repeat count, accepting parameter names.
// Collector: ev_f42e94198ef2de79623d4e0f9d84f788367e30bc183e571e4317b2fa12167eed
// Run: ev_cc1cc2326f3fb89f862a679ec1a028a505bd07da523173fd2d6fa1f2caa9d538
// Recursive walk: ev_64f1059ebe9591553a1ca8ce04e68a11ec13f174085f30430d2355624bf9696d
//
// Return every identifier token below a repeat-count expression whose exact
// name is absent from the owning module group's flat parameter/localparam table.
// Native run reports inside its collected AST_IDENTIFIER loop, so repeated or
// distinct names in one repeat are independent diagnostics. Its table has no
// lexical-scope key: a same-name parameter anywhere in the group exempts the
// occurrence even when a runtime declaration shadows it. Unknown direct names
// are reported because the native predicate is absence from the name table,
// not proof of a runtime binding. Qualified names/macros and unsupported
// elaboration remain unproved;
// an empty result is NOT a conformance result. The caller must retain partial
// or manual-review coverage. tokens and constants must share the same vector.
[[nodiscard]] inline std::vector<std::size_t> verilogRepeatRuntimeDependencies(
    const core::SourceFile& source, const std::vector<parser::HdlToken>& tokens,
    const VerilogConstantExpressions& constants) {
    (void)constants;
    std::vector<std::size_t> result;
    struct ModuleGroup {
        std::size_t begin;
        std::size_t end;
        std::set<std::string> parameters;
    };
    std::vector<ModuleGroup> groups;
    const auto keyword = [&](std::size_t index) {
        return index < tokens.size() && tokens[index].offset < source.content().size() &&
            source.content()[tokens[index].offset] != '\\';
    };
    const auto typeWord = [](const std::string& word) {
        return word == "parameter" || word == "localparam" || word == "signed" ||
            word == "unsigned" || word == "wire" || word == "reg" ||
            word == "logic" || word == "bit" || word == "byte" ||
            word == "shortint" || word == "int" || word == "integer" ||
            word == "longint" || word == "time" || word == "real" ||
            word == "shortreal" || word == "realtime" || word == "string" ||
            word == "type";
    };
    for (std::size_t module = 0; module < tokens.size(); ++module) {
        if (tokens[module].text != "module" || !keyword(module)) continue;
        auto moduleEnd = module + 1;
        while (moduleEnd < tokens.size() &&
               (tokens[moduleEnd].text != "endmodule" || !keyword(moduleEnd))) ++moduleEnd;
        if (moduleEnd == tokens.size()) break;
        ModuleGroup group{module, moduleEnd, {}};
        for (auto declaration = module + 1; declaration < moduleEnd; ++declaration) {
            if ((tokens[declaration].text != "parameter" &&
                 tokens[declaration].text != "localparam") || !keyword(declaration)) continue;
            auto segment = declaration + 1;
            int parens = 0, brackets = 0, braces = 0;
            for (auto index = segment; index <= moduleEnd; ++index) {
                const bool atEnd = index == moduleEnd;
                const auto text = atEnd ? std::string{} : tokens[index].text;
                const bool flat = parens == 0 && brackets == 0 && braces == 0;
                const bool boundary = atEnd || (flat &&
                    (text == "," || text == ";" || text == ")"));
                if (boundary) {
                    auto equals = index;
                    auto name = index;
                    for (auto candidate = segment; candidate < index; ++candidate) {
                        if (tokens[candidate].text == "=") { equals = candidate; break; }
                        if (tokens[candidate].kind == parser::HdlTokenKind::Identifier &&
                            !typeWord(tokens[candidate].text)) name = candidate;
                    }
                    if (name < equals) group.parameters.insert(tokens[name].text);
                    if (atEnd || text == ";" || text == ")") {
                        declaration = index;
                        break;
                    }
                    segment = index + 1;
                }
                if (atEnd) break;
                if (text == "(") ++parens;
                else if (text == ")" && parens > 0) --parens;
                else if (text == "[") ++brackets;
                else if (text == "]" && brackets > 0) --brackets;
                else if (text == "{") ++braces;
                else if (text == "}" && braces > 0) --braces;
            }
        }
        groups.push_back(std::move(group));
        module = moduleEnd;
    }
    for (std::size_t i = 0; i + 1 < tokens.size(); ++i) {
        const auto& token = tokens[i];
        if (token.kind != parser::HdlTokenKind::Identifier || token.text != "repeat" ||
            tokens[i + 1].text != "(" || token.offset >= source.content().size() ||
            source.content()[token.offset] == '\\' ||
            (i > 0 && (tokens[i - 1].text == "`" || tokens[i - 1].text == "." ||
                       tokens[i - 1].text == "::"))) continue;
        int depth = 1;
        auto end = i + 2;
        for (; end < tokens.size(); ++end) {
            if (tokens[end].text == "(") ++depth;
            else if (tokens[end].text == ")" && --depth == 0) break;
        }
        if (end == tokens.size()) continue; // malformed syntax is not a proof
        const auto group = std::ranges::find_if(groups, [&](const auto& candidate) {
            return candidate.begin < i && i < candidate.end;
        });
        if (group == groups.end()) continue;
        bool macro = false;
        for (auto j = i + 2; j < end; ++j) if (tokens[j].text == "`") {
            macro = true;
            break;
        }
        if (macro) continue;
        for (auto j = i + 2; j < end; ++j) {
            if (tokens[j].kind != parser::HdlTokenKind::Identifier) continue;
            // Function names lower to call nodes rather than AST_IDENTIFIER;
            // their ordinary argument identifiers are still visited.
            if (j + 1 < end && tokens[j + 1].text == "(") {
                // Elaboration/system-query arguments are deliberately unknown,
                // except $clog2 whose ordinary expression argument is retained.
                if (!tokens[j].text.empty() && tokens[j].text.front() == '$' &&
                    tokens[j].text != "$clog2") {
                    int callDepth = 0;
                    for (++j; j < end; ++j) {
                        if (tokens[j].text == "(") ++callDepth;
                        else if (tokens[j].text == ")" && --callDepth == 0) break;
                    }
                }
                continue;
            }
            // Package, class/member and hierarchical binding needs elaboration.
            if ((j > i + 2 && (tokens[j - 1].text == "." || tokens[j - 1].text == "::")) ||
                (j + 1 < end && (tokens[j + 1].text == "." || tokens[j + 1].text == "::")) ||
                (j > i + 3 && tokens[j - 1].text == ":" && tokens[j - 2].text == ":") ||
                (j + 2 < end && tokens[j + 1].text == ":" && tokens[j + 2].text == ":")) continue;
            if (!group->parameters.contains(tokens[j].text)) result.push_back(j);
        }
    }
    return result;
}

} // namespace fpga::analysis
