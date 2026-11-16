#pragma once

#include "parser/design_parser.h"

#include <algorithm>
#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace fpga::analysis {

// Partial independent GJB10157.9.4.10 source counterpart; not an elaborator.
// Native collector: ev_b8626bfc2750926780bcaa2a281cd2e99f4662b6069f347b81a555de2957b066
// Native run: ev_37cde213a465a52d14f34788a0fc365ae4bce55faa59813c880f4b61855a3227
// Run accepts CONSTANT/IDENTIFIER roots in MEMRD/MEMWR and identifier ranges.
// Source constant folding and vendor memory lowering are not reproduced.
class VerilogIndexChecks {
public:
    VerilogIndexChecks(const core::SourceFile& source,
                       const std::vector<parser::HdlToken>& tokens)
        : source_(source), tokens_(tokens), declarationRange_(tokens.size(), false) {
        indexDeclarations();
    }

    // '[' indices for high-confidence expression-position complex selections.
    // No result means unproved, not compliant. Macro, qualified-name and indexed
    // part-select handling deliberately remain outside this partial check.
    [[nodiscard]] std::vector<std::size_t> complexIndexIndices() const {
        std::vector<std::size_t> result;
        for (std::size_t i = 1; i < tokens_.size(); ++i) {
            if (tokens_[i].text != "[" || declarationRange_[i]) continue;
            const auto& previous = tokens_[i - 1];
            if (previous.kind != parser::HdlTokenKind::Identifier &&
                previous.text != "]" && previous.text != ")") continue;
            if (previous.text == "new") continue;
            const auto end = matching(i, "[", "]");
            if (end == tokens_.size()) continue;
            if (complex(i + 1, end)) result.push_back(i);
        }
        return result;
    }

private:
    const core::SourceFile& source_;
    const std::vector<parser::HdlToken>& tokens_;
    std::vector<bool> declarationRange_;

    bool keyword(std::size_t i) const {
        return tokens_[i].offset < source_.content().size() &&
            source_.content()[tokens_[i].offset] != '\\';
    }
    static bool declarationWord(std::string_view word) {
        return word == "wire" || word == "reg" || word == "logic" || word == "bit" ||
            word == "byte" || word == "shortint" || word == "int" || word == "integer" ||
            word == "longint" || word == "real" || word == "shortreal" ||
            word == "realtime" || word == "time" || word == "input" || word == "output" ||
            word == "inout" || word == "ref" || word == "parameter" || word == "localparam" ||
            word == "tri" || word == "tri0" || word == "tri1" || word == "trireg" ||
            word == "supply0" || word == "supply1" || word == "wand" || word == "wor" ||
            word == "uwire" || word == "genvar";
    }
    std::size_t matching(std::size_t open, std::string_view left, std::string_view right) const {
        int depth = 0;
        for (auto i = open; i < tokens_.size(); ++i) {
            if (tokens_[i].text == left) ++depth;
            else if (tokens_[i].text == right && --depth == 0) return i;
        }
        return tokens_.size();
    }
    void mask(std::size_t open, std::size_t end) {
        std::fill(declarationRange_.begin() + open,
                  declarationRange_.begin() + std::min(end + 1, tokens_.size()), true);
    }
    void indexDeclarations() {
        std::vector<std::string> types;
        // typedef/struct/enum declarations contain type dimensions, not indexes.
        for (std::size_t i = 0; i < tokens_.size(); ++i) {
            if (!keyword(i) || tokens_[i].text != "typedef") continue;
            int braces = 0;
            std::string name;
            for (auto j = i + 1; j < tokens_.size(); ++j) {
                if (tokens_[j].text == "{") ++braces;
                else if (tokens_[j].text == "}") --braces;
                else if (tokens_[j].text == "[" ) {
                    const auto end = matching(j, "[", "]");
                    if (end == tokens_.size()) break;
                    mask(j, end); j = end;
                } else if (braces == 0 && tokens_[j].text == ";") {
                    if (!name.empty()) types.push_back(name);
                    break;
                } else if (braces == 0 && tokens_[j].kind == parser::HdlTokenKind::Identifier)
                    name = tokens_[j].text;
            }
        }
        for (std::size_t i = 0; i < tokens_.size(); ++i) {
            if (!keyword(i)) continue;
            const auto& word = tokens_[i].text;
            const bool header = word == "function" || word == "task";
            if (!header && !declarationWord(word) &&
                std::find(types.begin(), types.end(), word) == types.end()) continue;
            // Prevent a return-type keyword / nested declaration keyword from
            // overriding a previously identified initializer expression.
            if (i > 0 && declarationRange_[i]) continue;
            bool initializer = false;
            int parens = 0, braces = 0;
            for (auto j = i + 1; j < tokens_.size(); ++j) {
                const auto& text = tokens_[j].text;
                const bool flat = parens == 0 && braces == 0;
                if (flat && (text == ";" || text == ")" || text == "begin" ||
                             text == "end" || text == "endfunction" || text == "endtask")) break;
                if (flat && text == "=") initializer = true;
                else if (flat && text == ",") initializer = false;
                else if (text == "[") {
                    const auto end = matching(j, "[", "]");
                    if (end == tokens_.size()) break;
                    if (header || !initializer) mask(j, end);
                    j = end;
                } else if (text == "(") ++parens;
                else if (text == ")") --parens;
                else if (text == "{") ++braces;
                else if (text == "}") --braces;
            }
        }
    }
    bool complex(std::size_t begin, std::size_t end) const {
        if (begin >= end) return false;
        for (auto i = begin; i < end; ++i) {
            const auto& text = tokens_[i].text;
            if (text == "`" || text == "." || text == "::" ||
                (text == ":" && i + 1 < end && tokens_[i + 1].text == ":") ||
                ((text == "+" || text == "-") && i + 1 < end && tokens_[i + 1].text == ":") ||
                text == "+:" || text == "-:") return false;
        }
        while (begin + 1 < end && tokens_[begin].text == "(" &&
               matching(begin, "(", ")") == end - 1) { ++begin; --end; }
        if (begin + 1 == end && (tokens_[begin].kind == parser::HdlTokenKind::Number ||
                                tokens_[begin].kind == parser::HdlTokenKind::Identifier)) return false;
        for (auto i = begin; i < end; ++i) {
            const auto& token = tokens_[i];
            // A selected identifier remains an IDENTIFIER root in the native
            // check. Its nested range is visited independently; do not borrow
            // that range's operator to report the outer index a[b[i+1]].
            if (token.text == "[") {
                const auto close = matching(i, "[", "]");
                if (close >= end) return false;
                i = close;
                continue;
            }
            if (token.text == "{" || (token.kind == parser::HdlTokenKind::Identifier &&
                i + 1 < end && tokens_[i + 1].text == "(")) return true;
            if (token.kind == parser::HdlTokenKind::Symbol &&
                token.text != "(" && token.text != ")" && token.text != ":" &&
                token.text != "[" && token.text != "]") return true;
        }
        return false;
    }
};

} // namespace fpga::analysis
