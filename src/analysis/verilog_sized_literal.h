#pragma once

#include "parser/design_parser.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>
#include <vector>

namespace fpga::analysis {

// Bounded source counterpart of native GJB10157.8.9 run 0x18057a7b0.
// ev_221f2aa099ba55fb91ffd978e478d153fc136a4425219ec7c416d0059e9e31ee:
// strtol parses a leading width; a zero/missing width or missing b/o/d/h
// suffix takes the warning path. FUN_18057afa0 recursively collects nonempty
// AST_CONSTANT text outside AST_RANGE/MULTIRANGE/IDENTIFIER subtrees, with a
// special AST_FOR/GENFOR traversal that omits initialization and increment.
class VerilogSizedLiteral {
public:
    explicit VerilogSizedLiteral(const std::vector<parser::HdlToken>& tokens) : tokens_(tokens) {}

    [[nodiscard]] std::vector<std::size_t> violatingLiteralIndices() const {
        return scanNativeConstantSubset(
            [](std::string_view value) { return missingWidthOrBase(value); });
    }

    // GJB10157.8.6: offline decompilation of native FUN_180576130 shows the
    // binary path retains leading zero digits when comparing encoded width,
    // while nonbinary paths trim leading zero bits. The native run accepts an
    // optional s/S before the same base dispatch, and the helper recognizes
    // X/Z/? digits. This adapter admits signed numeric literals and binary
    // unknown digits; other unknown-radix details remain under review.
    [[nodiscard]] std::vector<std::size_t> overflowingLiteralIndices() const {
        return scanAssignmentRhs([](std::string_view value) { return literalOverflow(value); });
    }

private:
    const std::vector<parser::HdlToken>& tokens_;
    template <typename Predicate>
    [[nodiscard]] std::vector<std::size_t> scanNativeConstantSubset(Predicate predicate) const {
        std::vector<bool> excluded(tokens_.size(), false);

        // FUN_18057afa0 stops recursion at AST_RANGE/AST_MULTIRANGE and at
        // AST_IDENTIFIER, so declaration ranges and identifier selects are not
        // candidate constants. Source brackets are the bounded counterpart.
        int brackets = 0;
        for (std::size_t i = 0; i < tokens_.size(); ++i) {
            if (tokens_[i].text == "[") ++brackets;
            if (brackets > 0) excluded[i] = true;
            if (tokens_[i].text == "]" && brackets > 0) --brackets;
        }

        // The collector has a four-child AST_FOR/AST_GENFOR branch that walks
        // the condition and body while omitting initialization and increment.
        for (std::size_t i = 0; i + 1 < tokens_.size(); ++i) {
            if ((tokens_[i].text != "for" && tokens_[i].text != "genfor") ||
                tokens_[i + 1].text != "(") continue;
            const auto close = matching(i + 1, tokens_.size());
            if (close == tokens_.size()) continue;
            std::size_t first = close, second = close;
            int depth = 0;
            for (auto at = i + 2; at < close; ++at) {
                if (tokens_[at].text == "(") ++depth;
                else if (tokens_[at].text == ")") --depth;
                else if (tokens_[at].text == ";" && depth == 0) {
                    if (first == close) first = at;
                    else { second = at; break; }
                }
            }
            if (first == close || second == close) continue;
            for (auto at = i + 2; at < first; ++at) excluded[at] = true;
            for (auto at = second + 1; at < close; ++at) excluded[at] = true;
        }

        // Unexpanded directives do not establish an AST_CONSTANT node. The
        // preprocessor normally removes them; retain this guard for malformed
        // or unsupported directives.
        for (std::size_t i = 0; i < tokens_.size(); ++i) {
            if (tokens_[i].text != "`") continue;
            const auto line = tokens_[i].line;
            while (i < tokens_.size() && tokens_[i].line == line) excluded[i++] = true;
            if (i != 0) --i;
        }

        std::vector<std::size_t> result;
        for (std::size_t i = 0; i < tokens_.size(); ++i) {
            if (excluded[i] || tokens_[i].kind != parser::HdlTokenKind::Number) continue;
            if (const auto violation = predicate(tokens_[i].text); violation == true)
                result.push_back(i);
        }
        return result;
    }

    template <typename Predicate>
    [[nodiscard]] std::vector<std::size_t> scanAssignmentRhs(Predicate predicate) const {
        std::vector<std::size_t> result;
        for (std::size_t i = 0; i < tokens_.size(); ++i) {
            const auto& word = tokens_[i].text;
            if ((word == "if" || word == "while" || word == "repeat" || word == "for" ||
                 word == "case" || word == "casex" || word == "casez") &&
                i + 1 < tokens_.size() && tokens_[i + 1].text == "(") {
                const auto close = matching(i + 1, tokens_.size());
                if (close != tokens_.size()) { i = close; continue; }
            }
            if (word != "=" && word != "<=") continue;
            const auto begin = i + 1;
            int parens = 0, brackets = 0, braces = 0;
            std::size_t end = begin;
            for (; end < tokens_.size(); ++end) {
                const auto& token = tokens_[end].text;
                if (token == "(") ++parens;
                else if (token == ")") {
                    if (parens == 0) break;
                    --parens;
                } else if (token == "[") ++brackets;
                else if (token == "]") {
                    if (brackets == 0) break;
                    --brackets;
                } else if (token == "{") ++braces;
                else if (token == "}") {
                    if (braces == 0) break;
                    --braces;
                }
                if (!parens && !brackets && !braces &&
                    (token == ";" || token == "," || token == "end" || token == "endmodule")) break;
            }
            if (end == tokens_.size() || end == begin) continue;
            bool macro = false;
            for (auto at = begin; at < end; ++at) macro |= tokens_[at].text == "`";
            if (macro) { i = end; continue; }
            brackets = 0;
            for (auto at = begin; at < end; ++at) {
                const auto& token = tokens_[at];
                if (token.text == "[") { ++brackets; continue; }
                if (token.text == "]") { --brackets; continue; }
                if (brackets != 0) continue; // selects and ranges are not recovered
                if (token.kind != parser::HdlTokenKind::Number) continue;
                if (const auto violation = predicate(token.text); violation == true)
                    result.push_back(at);
            }
            i = end == 0 ? i : end - 1;
        }
        return result;
    }

    [[nodiscard]] static std::optional<bool> literalOverflow(std::string_view value) {
        const auto quote = value.find('\'');
        if (quote == std::string_view::npos || quote == 0) return std::nullopt;
        std::size_t width = 0;
        for (std::size_t i = 0; i < quote; ++i) {
            const char ch = value[i];
            if (ch == '_') continue;
            if (ch < '0' || ch > '9' || width > 1000000) return std::nullopt;
            width = width * 10 + static_cast<std::size_t>(ch - '0');
        }
        if (width == 0 || width > 1000000) return std::nullopt;
        std::size_t at = quote + 1;
        if (at < value.size() && (value[at] == 's' || value[at] == 'S')) ++at;
        if (at == value.size()) return std::nullopt;
        unsigned base = 0;
        switch (value[at++]) {
        case 'b': case 'B': base = 2; break;
        case 'o': case 'O': base = 8; break;
        case 'd': case 'D': base = 10; break;
        case 'h': case 'H': base = 16; break;
        default: return std::nullopt;
        }
        if (at == value.size()) return std::nullopt;
        // Incremental binary magnitude; cap above the declared width because
        // only the overflow predicate matters. This also handles long values.
        std::vector<unsigned char> bits(1, 0);
        bool digitSeen = false;
        bool overflow = false;
        std::size_t binaryDigits = 0;
        for (; at < value.size(); ++at) {
            const char ch = value[at];
            if (ch == '_') continue;
            unsigned digit = 0;
            if (ch >= '0' && ch <= '9') digit = static_cast<unsigned>(ch - '0');
            else if (ch >= 'a' && ch <= 'f') digit = static_cast<unsigned>(ch - 'a' + 10);
            else if (ch >= 'A' && ch <= 'F') digit = static_cast<unsigned>(ch - 'A' + 10);
            else if (ch == 'x' || ch == 'X' || ch == 'z' || ch == 'Z' || ch == '?') {
                // FUN_180576130 explicitly accepts all three unknown spellings.
                // Its binary branch preserves one encoded bit per source digit.
                if (base != 2) return std::nullopt;
                digitSeen = true;
                ++binaryDigits;
                overflow = binaryDigits > width;
                continue;
            } else return std::nullopt;
            if (digit >= base) return std::nullopt;
            digitSeen = true;
            if (base == 2) {
                ++binaryDigits;
                overflow = binaryDigits > width;
                continue;
            }
            if (overflow) continue;
            unsigned carry = digit;
            for (auto& bit : bits) {
                const unsigned next = static_cast<unsigned>(bit) * base + carry;
                bit = static_cast<unsigned char>(next & 1U);
                carry = next >> 1U;
            }
            while (carry != 0) {
                bits.push_back(static_cast<unsigned char>(carry & 1U));
                carry >>= 1U;
            }
            overflow = bits.size() > width;
        }
        return digitSeen ? std::optional<bool>{overflow} : std::nullopt;
    }
    [[nodiscard]] std::size_t matching(std::size_t open, std::size_t limit) const {
        int depth = 0;
        for (auto j = open; j < limit; ++j) {
            if (tokens_[j].text == "(") ++depth;
            else if (tokens_[j].text == ")" && --depth == 0) return j;
        }
        return limit;
    }
    [[nodiscard]] static std::optional<bool> missingWidthOrBase(std::string_view value) {
        const auto quote = value.find('\'');
        if (quote == std::string_view::npos) {
            bool digits = !value.empty();
            for (const auto ch : value) digits &= (ch >= '0' && ch <= '9') || ch == '_';
            return digits ? std::optional<bool>{true} : std::nullopt;
        }
        bool widthSyntax = true, nonzero = false;
        for (std::size_t j = 0; j < quote; ++j) {
            const auto ch = value[j];
            widthSyntax &= (ch >= '0' && ch <= '9') || ch == '_';
            nonzero |= ch >= '1' && ch <= '9';
        }
        if (!widthSyntax) return std::nullopt;
        auto base = quote + 1;
        if (base < value.size() && (value[base] == 's' || value[base] == 'S')) ++base;
        if (base >= value.size()) return std::nullopt;
        const auto ch = value[base];
        const bool hasBase = ch == 'b' || ch == 'B' || ch == 'o' || ch == 'O' ||
            ch == 'd' || ch == 'D' || ch == 'h' || ch == 'H';
        if (!hasBase) return quote == 0 ? std::optional<bool>{true} : std::nullopt;
        if (base + 1 >= value.size()) return std::nullopt;
        return !nonzero;
    }
};

} // namespace fpga::analysis
