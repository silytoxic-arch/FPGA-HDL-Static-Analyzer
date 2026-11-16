#pragma once
#include "parser/design_parser.h"
#include <string>
#include <vector>

namespace fpga::analysis {

// Independent, bounded token implementation of the native FUNCTION ->
// ASSIGN_LE core predicate. This is not complete HDL grammar/elaboration.
// Unclosed functions and extern/pure-virtual prototypes are not traversed.
inline std::vector<std::size_t> functionNonblockingAssignments(
    const std::vector<parser::HdlToken>& tokens) {
    std::vector<std::size_t> found;
    for (std::size_t i = 0; i < tokens.size(); ++i) {
        if (tokens[i].text != "function") continue;
        bool prototype = false;
        for (auto j = i; j > 0;) {
            const auto& text = tokens[--j].text;
            if (text == ";" || text == "endfunction" || text == "end" || text == "endmodule") break;
            if (text == "extern" || text == "pure") { prototype = true; break; }
        }
        if (prototype) continue;
        auto end = i + 1;
        for (; end < tokens.size(); ++end) {
            const auto& text = tokens[end].text;
            if (text == "endfunction" || text == "function" || text == "endmodule" || text == "endclass") break;
        }
        if (end == tokens.size() || tokens[end].text != "endfunction") continue;
        int parens = 0, brackets = 0, braces = 0;
        bool assigned = false, returning = false;
        for (auto j = i + 1; j < end; ++j) {
            const auto& text = tokens[j].text;
            if (text == "(") ++parens;
            else if (text == ")" && parens > 0) --parens;
            else if (text == "[") ++brackets;
            else if (text == "]" && brackets > 0) --brackets;
            else if (text == "{") ++braces;
            else if (text == "}" && braces > 0) --braces;
            if (parens || brackets || braces) continue;
            if (text == ";" || text == "begin" || text == "end" || text == "else") {
                assigned = returning = false;
            } else if (text == "return") returning = true;
            else if (text == "=") assigned = true;
            else if (text == "<=" && !assigned && !returning && j > i + 1) {
                const auto& lhsEnd = tokens[j - 1];
                if (lhsEnd.kind == parser::HdlTokenKind::Identifier || lhsEnd.text == "]" || lhsEnd.text == "}")
                    found.push_back(j);
                assigned = true;
            }
        }
        i = end;
    }
    return found;
}
} // namespace fpga::analysis
