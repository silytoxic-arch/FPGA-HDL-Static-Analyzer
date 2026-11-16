#pragma once

#include "analysis/verilog_constant_expression.h"

#include <cstddef>
#include <vector>

namespace fpga::analysis {

// Partial lexical counterpart of GJB10157.9.2.6. Native run 0x180583ca0
// (ev_95102f809deb39475a14e001a1aa069a17fbeafdfff5d1337a06af1641c9cb40)
// accepts CONSTANT/REALVALUE/DEFAULT/BLOCK as a branch's first child. The vendor
// frontend's constant folding remains unknown. Here only proven variable
// dependencies are reported; unresolved labels require manual review.
class VerilogCaseLabels {
public:
    VerilogCaseLabels(const std::vector<parser::HdlToken>& tokens,
                      const VerilogConstantExpressions& constants)
        : tokens_(tokens), constants_(constants) {}

    // One item-start token index per violating native branch node.  The native
    // consumer inspects only the first child of each COND/CONDX/CONDZ, so later
    // comma-separated choices cannot create a diagnostic for this rule.
    [[nodiscard]] std::vector<std::size_t> runtimeLabelIndices() const {
        std::vector<std::size_t> result;
        for (std::size_t i = 0; i + 1 < tokens_.size(); ++i) {
            if (!isCase(tokens_[i].text) || tokens_[i + 1].text != "(") continue;
            auto cursor = matching(i + 1, "(", ")");
            if (cursor == tokens_.size()) continue;
            ++cursor;
            // SystemVerilog case inside/matches labels need separate grammar.
            if (cursor < tokens_.size() && (tokens_[cursor].text == "inside" ||
                                           tokens_[cursor].text == "matches")) continue;
            while (cursor < tokens_.size() && tokens_[cursor].text != "endcase") {
                const auto label = cursor;
                const auto colon = labelColon(cursor);
                if (colon == tokens_.size()) break;
                if (tokens_[label].text != "default") {
                    auto firstEnd = colon;
                    int parens = 0, brackets = 0, braces = 0;
                    for (auto at = label; at < colon; ++at) {
                        const auto& text = tokens_[at].text;
                        if (text == "(") ++parens;
                        else if (text == ")") --parens;
                        else if (text == "[") ++brackets;
                        else if (text == "]") --brackets;
                        else if (text == "{") ++braces;
                        else if (text == "}") --braces;
                        else if (text == "," && parens == 0 && brackets == 0 && braces == 0) {
                            firstEnd = at;
                            break;
                        }
                    }
                    if (constants_.hasRuntimeDependency(label, firstEnd))
                        result.push_back(label);
                }
                const auto next = statementEnd(colon + 1);
                if (next <= cursor || next >= tokens_.size()) break;
                cursor = next;
            }
        }
        return result;
    }

private:
    const std::vector<parser::HdlToken>& tokens_;
    const VerilogConstantExpressions& constants_;
    static bool isCase(const std::string& word) {
        return word == "case" || word == "casex" || word == "casez";
    }
    std::size_t matching(std::size_t open, const char* left, const char* right) const {
        int depth = 0;
        for (auto i = open; i < tokens_.size(); ++i) {
            if (tokens_[i].text == left) ++depth;
            else if (tokens_[i].text == right && --depth == 0) return i;
        }
        return tokens_.size();
    }
    std::size_t labelColon(std::size_t start) const {
        int parens = 0, brackets = 0, braces = 0, ternary = 0;
        for (auto i = start; i < tokens_.size(); ++i) {
            const auto& text = tokens_[i].text;
            const bool flat = parens == 0 && brackets == 0 && braces == 0;
            if (flat && (text == "endcase" || text == ";" || text == "begin" || text == "end"))
                return tokens_.size();
            if (flat && text == "?") ++ternary;
            else if (flat && text == ":") {
                // Raw lexer uses two ':' symbols for package/class scope.
                if (i + 1 < tokens_.size() && tokens_[i + 1].text == ":") { ++i; continue; }
                if (ternary != 0) --ternary;
                else return i;
            }
            if (text == "(") ++parens;
            else if (text == ")") --parens;
            else if (text == "[") ++brackets;
            else if (text == "]") --brackets;
            else if (text == "{") ++braces;
            else if (text == "}") --braces;
            if (parens < 0 || brackets < 0 || braces < 0) return tokens_.size();
        }
        return tokens_.size();
    }
    std::size_t statementEnd(std::size_t start) const {
        if (start >= tokens_.size()) return tokens_.size();
        const auto& word = tokens_[start].text;
        if (word == "unique" || word == "unique0" || word == "priority")
            return statementEnd(start + 1);
        if (word == "begin" || word == "fork") {
            const bool fork = word == "fork";
            int depth = 1;
            for (auto i = start + 1; i < tokens_.size(); ++i) {
                if (tokens_[i].text == word) ++depth;
                const auto& text = tokens_[i].text;
                if ((!fork && text == "end") ||
                    (fork && (text == "join" || text == "join_any" || text == "join_none"))) {
                    if (--depth != 0) continue;
                    auto next = i + 1;
                    if (next + 1 < tokens_.size() && tokens_[next].text == ":") next += 2;
                    return next;
                }
            }
            return tokens_.size();
        }
        if (isCase(word)) {
            int depth = 1;
            for (auto i = start + 1; i < tokens_.size(); ++i) {
                if (isCase(tokens_[i].text)) ++depth;
                else if (tokens_[i].text == "endcase" && --depth == 0) return i + 1;
            }
            return tokens_.size();
        }
        if (word == "if" || word == "for" || word == "while" || word == "repeat" ||
            word == "foreach") {
            if (start + 1 >= tokens_.size() || tokens_[start + 1].text != "(") return tokens_.size();
            const auto close = matching(start + 1, "(", ")");
            if (close == tokens_.size()) return close;
            auto next = statementEnd(close + 1);
            if (word == "if" && next < tokens_.size() && tokens_[next].text == "else")
                next = statementEnd(next + 1);
            return next;
        }
        if (word == "forever") return statementEnd(start + 1);
        // Event/delay control bodies and do/while are not guessed as labels.
        if (word == "@" || word == "#" || word == "do") return tokens_.size();
        int parens = 0, braces = 0, brackets = 0;
        for (auto i = start; i < tokens_.size(); ++i) {
            const auto& text = tokens_[i].text;
            if (parens == 0 && braces == 0 && brackets == 0) {
                if (text == ";") return i + 1;
                if (text == "endcase" || text == "end") return tokens_.size();
            }
            if (text == "(") ++parens;
            else if (text == ")") --parens;
            else if (text == "{") ++braces;
            else if (text == "}") --braces;
            else if (text == "[") ++brackets;
            else if (text == "]") --brackets;
        }
        return tokens_.size();
    }
};

} // namespace fpga::analysis
