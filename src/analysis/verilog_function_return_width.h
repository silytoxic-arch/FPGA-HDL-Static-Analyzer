#pragma once

#include "parser/design_parser.h"

#include <algorithm>
#include <charconv>
#include <cstddef>
#include <functional>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace fpga::analysis::detail {

// Native 9.4.5 selects ASSIGN_EQ/LE nodes whose first-child internal name
// contains "$func$" and ends in "$result", then compares computed widths
// of its first two children. This independent source adapter handles only
// explicit non-ANSI function returns with fixed decimal ranges and bounded
// input identifier/select/arithmetic RHS expressions.
// ev_0a5c5f8e4a601c41857969d2485a32e4f7eea3c4f2ffea1f5443da08b26e39c0
// ev_4d0d4622ea9605db64e877e7f04f99a9219f4f5bcb28ac39acd4134deaa27e10
// ev_9e78a36c6bd5d2da1500b37922980ffe87fb0858db5ab22fd036a378923f6eab
struct FunctionReturnWidthMismatch {
    std::size_t token{};
    std::size_t returnWidth{};
    std::size_t inputWidth{};
};

[[nodiscard]] inline std::vector<FunctionReturnWidthMismatch> functionReturnWidthMismatches(
    const std::vector<parser::HdlToken>& tokens) {
    using parser::HdlTokenKind;
    std::vector<FunctionReturnWidthMismatch> found;
    const auto decimal = [&](std::size_t i) -> std::optional<std::size_t> {
        if (i >= tokens.size() || tokens[i].kind != HdlTokenKind::Number) return {};
        std::size_t value = 0;
        const auto& text = tokens[i].text;
        const auto [last, error] = std::from_chars(text.data(), text.data() + text.size(), value);
        if (error != std::errc{} || last != text.data() + text.size() || value > 65535) return {};
        return value;
    };
    const auto rangeWidth = [&](std::size_t open, std::size_t limit) -> std::optional<std::size_t> {
        if (open + 4 >= limit || tokens[open].text != "[" ||
            tokens[open + 2].text != ":" || tokens[open + 4].text != "]") return {};
        const auto left = decimal(open + 1), right = decimal(open + 3);
        if (!left || !right) return {};
        return *left > *right ? *left - *right + 1 : *right - *left + 1;
    };
    for (auto function = std::size_t{0}; function < tokens.size(); ++function) {
        if (tokens[function].text != "function" ||
            tokens[function].kind != HdlTokenKind::Identifier) continue;
        auto headerEnd = function + 1;
        while (headerEnd < tokens.size() && tokens[headerEnd].text != ";" &&
               tokens[headerEnd].text != "endfunction" &&
               tokens[headerEnd].text != "endmodule") ++headerEnd;
        if (headerEnd == tokens.size() || tokens[headerEnd].text != ";") continue;
        auto end = headerEnd + 1;
        while (end < tokens.size() && tokens[end].text != "endfunction" &&
               tokens[end].text != "endmodule") ++end;
        if (end == tokens.size() || tokens[end].text != "endfunction") continue;
        bool unknownHeader = false;
        std::size_t returnWidth = 1;
        int returnRanges = 0;
        std::size_t name = headerEnd;
        for (auto i = function + 1; i < headerEnd; ++i) {
            if (tokens[i].text == "(") { unknownHeader = true; break; }
            if (tokens[i].text == "[") {
                const auto width = rangeWidth(i, headerEnd);
                if (!width || ++returnRanges != 1) { unknownHeader = true; break; }
                returnWidth = *width;
                i += 4;
            } else if (tokens[i].kind == HdlTokenKind::Identifier &&
                       tokens[i].text != "automatic" && tokens[i].text != "static" &&
                       tokens[i].text != "reg" && tokens[i].text != "logic" &&
                       tokens[i].text != "signed" && tokens[i].text != "unsigned") {
                name = i;
            }
        }
        for (auto i = function + 1; i < name; ++i) {
            if (tokens[i].kind != HdlTokenKind::Identifier) continue;
            const auto& text = tokens[i].text;
            if (text != "automatic" && text != "static" && text != "reg" &&
                text != "logic" && text != "signed" && text != "unsigned")
                unknownHeader = true;
        }
        if (unknownHeader || name == headerEnd) { function = end; continue; }
        const auto& functionName = tokens[name].text;
        std::map<std::string, std::size_t> inputWidths;
        bool macro = false;
        for (auto i = headerEnd + 1; i < end; ++i) {
            if (tokens[i].text == "`") { macro = true; break; }
            if (tokens[i].text != "input" || tokens[i].kind != HdlTokenKind::Identifier) continue;
            auto statementEnd = i + 1;
            while (statementEnd < end && tokens[statementEnd].text != ";") ++statementEnd;
            if (statementEnd == end) break;
            auto cursor = i + 1;
            if (cursor < statementEnd && (tokens[cursor].text == "reg" ||
                tokens[cursor].text == "logic" || tokens[cursor].text == "signed")) ++cursor;
            std::size_t width = 1;
            if (cursor < statementEnd && tokens[cursor].text == "[") {
                const auto parsed = rangeWidth(cursor, statementEnd);
                if (!parsed) { i = statementEnd; continue; }
                width = *parsed;
                cursor += 5;
            }
            for (; cursor < statementEnd; ++cursor) {
                if (tokens[cursor].kind == HdlTokenKind::Identifier &&
                    (cursor + 1 == statementEnd || tokens[cursor + 1].text == ","))
                    inputWidths[tokens[cursor].text] = width;
            }
            i = statementEnd;
        }
        if (macro) { function = end; continue; }
        std::function<std::optional<std::size_t>(std::size_t, std::size_t)> expressionWidth;
        expressionWidth = [&](std::size_t begin, std::size_t finish)
            -> std::optional<std::size_t> {
            while (finish > begin + 1 && tokens[begin].text == "(" &&
                   tokens[finish - 1].text == ")") {
                int depth = 0;
                bool enclosing = true;
                for (auto at = begin; at < finish; ++at) {
                    if (tokens[at].text == "(") ++depth;
                    else if (tokens[at].text == ")") --depth;
                    if (depth == 0 && at + 1 != finish) { enclosing = false; break; }
                    if (depth < 0) return {};
                }
                if (!enclosing || depth != 0) break;
                ++begin;
                --finish;
            }
            if (begin >= finish) return {};
            if (finish == begin + 1 && tokens[begin].kind == HdlTokenKind::Identifier) {
                const auto known = inputWidths.find(tokens[begin].text);
                return known == inputWidths.end() ? std::nullopt :
                    std::optional<std::size_t>{known->second};
            }
            if (tokens[begin].kind == HdlTokenKind::Identifier && begin + 3 < finish &&
                tokens[begin + 1].text == "[" && tokens[finish - 1].text == "]") {
                if (finish == begin + 4 && decimal(begin + 2)) return 1;
                if (finish == begin + 6 && tokens[begin + 3].text == ":") {
                    const auto left = decimal(begin + 2), right = decimal(begin + 4);
                    if (left && right)
                        return *left > *right ? *left - *right + 1 : *right - *left + 1;
                }
                return {};
            }
            int parens = 0, brackets = 0, braces = 0;
            for (auto at = finish; at > begin;) {
                --at;
                const auto& text = tokens[at].text;
                if (text == ")") ++parens;
                else if (text == "(") --parens;
                else if (text == "]") ++brackets;
                else if (text == "[") --brackets;
                else if (text == "}") ++braces;
                else if (text == "{") --braces;
                else if (parens == 0 && brackets == 0 && braces == 0 &&
                         (text == "+" || text == "-" || text == "*" ||
                          text == "/" || text == "%")) {
                    if (at == begin) break; // unary sign
                    const auto left = expressionWidth(begin, at);
                    const auto right = expressionWidth(at + 1, finish);
                    if (left && right) return std::max(*left, *right);
                    return {};
                }
                if (parens < 0 || brackets < 0 || braces < 0) return {};
            }
            if (finish > begin + 1 &&
                (tokens[begin].text == "+" || tokens[begin].text == "-" ||
                 tokens[begin].text == "~"))
                return expressionWidth(begin + 1, finish);
            return {};
        };
        for (auto i = headerEnd + 1; i + 2 < end; ++i) {
            if (tokens[i].text != functionName ||
                (tokens[i + 1].text != "=" && tokens[i + 1].text != "<=")) continue;
            auto rhsEnd = i + 2;
            while (rhsEnd < end && tokens[rhsEnd].text != ";") ++rhsEnd;
            if (rhsEnd == end) break;
            const auto rhsWidth = expressionWidth(i + 2, rhsEnd);
            // The native run compares the two independently inferred widths
            // with !=. Keep both mismatch directions for these known shapes.
            if (rhsWidth && *rhsWidth != returnWidth)
                found.push_back({i, returnWidth, *rhsWidth});
            i = rhsEnd;
        }
        function = end;
    }
    return found;
}

} // namespace fpga::analysis::detail
