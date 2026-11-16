#pragma once

#include "parser/design_parser.h"

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <set>
#include <string>
#include <vector>

namespace fpga::analysis::detail {

// A bounded source adapter for the vendor's internal `is_override_assign`
// model. The original producer performs process/bit-level lowering; this
// deliberately handles only exact scalar or fixed numeric-bit targets in
// straight-line begin/end and complete if/else paths. Unknown control-flow
// forms stop the local proof rather than certifying compliance.
// ev_082bb18f097147f33d9c4ea8bfbc4fd286a2532293060392b59ec6f1102d1292
// ev_245deea633a8fe6d7385d7d67223ce5a36e4d0fe99017b1b482795e0f8a662ad
class VerilogOverrideAssignments {
public:
    explicit VerilogOverrideAssignments(const std::vector<parser::HdlToken>& tokens)
        : tokens_(tokens) {}

    [[nodiscard]] std::vector<std::size_t> find() {
        for (std::size_t i = 0; i + 2 < tokens_.size(); ++i) {
            if (tokens_[i].text != "always" || tokens_[i + 1].text != "@") continue;
            std::size_t body = i + 2;
            if (tokens_[body].text == "(") {
                body = afterBalanced(body);
            } else if (tokens_[body].text == "*") {
                ++body;
            } else {
                continue;
            }
            if (body >= tokens_.size()) continue;
            std::set<std::string> written;
            const auto after = statement(body, written, 0);
            if (after > body && after <= tokens_.size()) i = after - 1;
        }
        return found_;
    }

private:
    const std::vector<parser::HdlToken>& tokens_;
    std::vector<std::size_t> found_;

    [[nodiscard]] std::size_t afterBalanced(std::size_t open) const {
        if (open >= tokens_.size() || tokens_[open].text != "(") return tokens_.size();
        int depth = 0;
        for (auto i = open; i < tokens_.size(); ++i) {
            if (tokens_[i].text == "(") ++depth;
            else if (tokens_[i].text == ")" && --depth == 0) return i + 1;
        }
        return tokens_.size();
    }

    [[nodiscard]] std::string target(std::size_t begin, std::size_t end) const {
        using parser::HdlTokenKind;
        if (begin >= end || tokens_[begin].kind != HdlTokenKind::Identifier) return {};
        std::string key = tokens_[begin].text;
        if (begin + 1 == end) return key;
        if (begin + 4 == end && tokens_[begin + 1].text == "[" &&
            tokens_[begin + 2].kind == HdlTokenKind::Number &&
            tokens_[begin + 3].text == "]") {
            key += "[" + tokens_[begin + 2].text + "]";
            return key;
        }
        return {};
    }

    [[nodiscard]] std::size_t statement(std::size_t begin, std::set<std::string>& written,
                                        int depth) {
        if (begin >= tokens_.size() || depth > 32) return tokens_.size();
        const auto& word = tokens_[begin].text;
        if (word == "begin") {
            auto i = begin + 1;
            if (i + 1 < tokens_.size() && tokens_[i].text == ":") i += 2;
            while (i < tokens_.size() && tokens_[i].text != "end") {
                const auto next = statement(i, written, depth + 1);
                if (next <= i || next == tokens_.size()) return tokens_.size();
                i = next;
            }
            if (i == tokens_.size()) return i;
            ++i;
            if (i + 1 < tokens_.size() && tokens_[i].text == ":") i += 2;
            return i;
        }
        if (word == "if") {
            if (begin + 1 >= tokens_.size()) return tokens_.size();
            const auto afterCondition = afterBalanced(begin + 1);
            if (afterCondition == tokens_.size()) return tokens_.size();
            auto thenWritten = written;
            const auto afterThen = statement(afterCondition, thenWritten, depth + 1);
            if (afterThen == tokens_.size()) return afterThen;
            if (afterThen < tokens_.size() && tokens_[afterThen].text == "else") {
                auto elseWritten = written;
                const auto afterElse = statement(afterThen + 1, elseWritten, depth + 1);
                if (afterElse == tokens_.size()) return afterElse;
                std::set<std::string> common;
                std::set_intersection(thenWritten.begin(), thenWritten.end(),
                    elseWritten.begin(), elseWritten.end(),
                    std::inserter(common, common.end()));
                written = std::move(common);
                return afterElse;
            }
            // Without an else, only writes known before the if are definite.
            return afterThen;
        }
        if (word == "case" || word == "casex" || word == "casez" ||
            word == "for" || word == "while" || word == "repeat" ||
            word == "forever" || word == "fork" || word == "@" || word == "#")
            return tokens_.size();
        std::size_t assignment = tokens_.size();
        int parens = 0, brackets = 0, braces = 0;
        for (auto i = begin; i < tokens_.size(); ++i) {
            const auto& value = tokens_[i].text;
            if (value == "(") ++parens;
            else if (value == ")") --parens;
            else if (value == "[") ++brackets;
            else if (value == "]") --brackets;
            else if (value == "{") ++braces;
            else if (value == "}") --braces;
            if (parens < 0 || brackets < 0 || braces < 0) return tokens_.size();
            if (parens || brackets || braces) continue;
            if (value == "=" || value == "<=") assignment = i;
            if (value == ";") {
                if (assignment < i) {
                    const auto key = target(begin, assignment);
                    if (!key.empty()) {
                        if (written.contains(key)) found_.push_back(begin);
                        written.insert(key);
                    }
                }
                return i + 1;
            }
            if (value == "end" || value == "else" || value == "endcase")
                return tokens_.size();
        }
        return tokens_.size();
    }
};

} // namespace fpga::analysis::detail
