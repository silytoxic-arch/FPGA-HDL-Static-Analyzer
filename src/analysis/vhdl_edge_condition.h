#pragma once

#include "parser/design_parser.h"

#include <cstddef>
#include <functional>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace fpga::analysis {

enum class VhdlEdgeDirection { Unknown, Rising, Falling };
enum class VhdlEdgeConditionStatus { Matched, Incomplete, Unsupported };

struct VhdlEdgeCondition {
    std::string clockName; // ASCII lower-case basic identifier
    std::size_t clockToken{std::numeric_limits<std::size_t>::max()};
    VhdlEdgeDirection direction{VhdlEdgeDirection::Unknown};
    VhdlEdgeConditionStatus status{VhdlEdgeConditionStatus::Unsupported};
};

// Two bounded syntactic candidates recovered from retained native evidence:
// 68bee0 simple edge call: ev_05d49a38e893ea5b4591cee350df7b6abfca2915ed94f59f3d583e38cc803914
// 68c5e0 event/EQ: ev_fce412e465519df4bf6baafd56270a64e826e87d9f68ca491115b442590d7e4e
// caller out-pointer: ev_d008f17ed69c4f68f9a04a3d4506c61625c548eaf4892b141f885dd553a7a9b1
// literal bytes: ev_9f35780b79592a6b25403a3619597621054607d8b8512370b55988eac83b7ab5
// name normalization: ev_6fb4664a90434afc0de6d58ae87fb64ed8e97e5fc4158182429a88f72e6bba73
// This is NOT a full 4.10.h rule/process classifier. A matched shape does not
// establish type binding, standard-library function identity or process class.
[[nodiscard]] inline VhdlEdgeCondition
parseVhdlEdgeCondition(const core::SourceFile& source,
                       const std::vector<parser::HdlToken>& tokens,
                       std::size_t begin, std::size_t end) {
    VhdlEdgeCondition result;
    if (source.language() != core::HdlLanguage::Vhdl) return result;
    if (begin >= end || end > tokens.size()) {
        result.status = VhdlEdgeConditionStatus::Incomplete;
        return result;
    }
    const auto lower = [](std::string text) {
        for (auto& c : text) if (c >= 'A' && c <= 'Z') c = static_cast<char>(c - 'A' + 'a');
        return text;
    };
    const auto word = [&](std::size_t i, std::string_view name) {
        return i < tokens.size() && tokens[i].kind == parser::HdlTokenKind::Identifier &&
            lower(tokens[i].text) == name;
    };
    const auto basicIdentifier = [&](std::size_t i) {
        if (i >= tokens.size() || tokens[i].kind != parser::HdlTokenKind::Identifier) return false;
        const auto& name = tokens[i].text;
        if (name.empty() || !((name[0] >= 'A' && name[0] <= 'Z') ||
                             (name[0] >= 'a' && name[0] <= 'z'))) return false;
        bool underscore = false;
        for (const auto c : name) {
            if (!((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
                  (c >= '0' && c <= '9') || c == '_')) return false;
            if (c == '_' && underscore) return false;
            underscore = c == '_';
        }
        if (underscore) return false;
        constexpr std::string_view reserved =
            " abs access after alias all and architecture array assert assume assume_guarantee "
            " attribute begin block body buffer bus case component configuration constant cover "
            " default disconnect downto else elsif end entity exit fairness file for force "
            " function generate generic group guarded if impure in inertial inout is label "
            " library linkage literal loop map mod nand new next nor not null of on open or "
            " others out package parameter port postponed procedure process property protected "
            " pure range record register reject release rem report restrict restrict_guarantee "
            " return rol ror select sequence severity shared signal sla sll sra srl strong "
            " subtype then to transport type unaffected units until use variable vmode vprop "
            " vunit wait when while with xnor xor ";
        return reserved.find(" " + lower(name) + " ") == std::string_view::npos;
    };
    bool edgeHint = false;
    for (auto i = begin; i < end; ++i)
        edgeHint |= word(i, "rising_edge") || word(i, "falling_edge") || word(i, "event");
    result.status = edgeHint ? VhdlEdgeConditionStatus::Incomplete : VhdlEdgeConditionStatus::Unsupported;

    const auto stripOuterParentheses = [&](std::size_t first, std::size_t last) {
        while (first < last && tokens[first].text == "(") {
            int depth = 0;
            auto close = first;
            for (; close < last; ++close) {
                if (tokens[close].text == "(") ++depth;
                else if (tokens[close].text == ")" && --depth == 0) break;
            }
            if (close + 1 != last) break;
            ++first;
            --last;
        }
        return std::pair{first, last};
    };
    const auto [rootBegin, rootEnd] = stripOuterParentheses(begin, end);

    if (rootEnd - rootBegin == 4 &&
        (word(rootBegin, "rising_edge") || word(rootBegin, "falling_edge")) &&
        tokens[rootBegin + 1].text == "(" && basicIdentifier(rootBegin + 2) &&
        tokens[rootBegin + 3].text == ")") {
        result.clockName = lower(tokens[rootBegin + 2].text);
        result.clockToken = rootBegin + 2;
        result.direction = word(rootBegin, "rising_edge")
            ? VhdlEdgeDirection::Rising : VhdlEdgeDirection::Falling;
        result.status = VhdlEdgeConditionStatus::Matched;
        return result;
    }

    const auto binaryLiteral = [&](std::size_t start, std::size_t stop) -> VhdlEdgeDirection {
        std::string text;
        if (stop - start == 1 && tokens[start].kind != parser::HdlTokenKind::Identifier &&
            tokens[start].kind != parser::HdlTokenKind::String) text = tokens[start].text;
        // The current public lexer can retain the closing apostrophe in its
        // digit-led Number token, yielding Symbol("'") + Number("1'").
        else if (stop - start == 2 && tokens[start].text == "'" &&
                 tokens[start + 1].kind == parser::HdlTokenKind::Number &&
                 tokens[start + 1].offset == tokens[start].offset + 1)
            text = "'" + tokens[start + 1].text;
        else if (stop - start == 3 && tokens[start].text == "'" && tokens[start + 2].text == "'" &&
                 tokens[start + 1].kind == parser::HdlTokenKind::Number &&
                 tokens[start + 1].offset == tokens[start].offset + 1 &&
                 tokens[start + 2].offset == tokens[start + 1].offset + 1)
            text = "'" + tokens[start + 1].text + "'";
        if (text != "'0'" && text != "'1'") return VhdlEdgeDirection::Unknown;
        // Joining whitespace-separated quote/number/quote isn't a literal.
        if (tokens[start].offset + 3 > source.content().size() ||
            source.content().compare(tokens[start].offset, 3, text) != 0) return VhdlEdgeDirection::Unknown;
        return text == "'1'" ? VhdlEdgeDirection::Rising : VhdlEdgeDirection::Falling;
    };

    // FUN_1804d4cc0 receives one AST_BIT_AND/AST_LOGIC_AND node and scans its
    // direct children without imposing source order. VHDL binary operators are
    // left associative, so the last top-level AND separates the root's two
    // direct operands. Source parentheses disappear before this helper runs.
    std::size_t andToken = rootEnd;
    int depth = 0;
    for (auto token = rootBegin; token < rootEnd; ++token) {
        if (tokens[token].text == "(") ++depth;
        else if (tokens[token].text == ")" && depth > 0) --depth;
        else if (depth == 0 && word(token, "and")) andToken = token;
    }
    if (andToken == rootEnd) return result;
    const auto [leftBegin, leftEnd] = stripOuterParentheses(rootBegin, andToken);
    const auto [rightBegin, rightEnd] = stripOuterParentheses(andToken + 1, rootEnd);
    const auto eventIdentifier = [&](std::size_t first, std::size_t last)
        -> std::optional<std::size_t> {
        if (last - first == 3 && basicIdentifier(first) &&
            tokens[first + 1].text == "'" && word(first + 2, "event")) return first;
        return std::nullopt;
    };
    const auto equality = [&](std::size_t first, std::size_t last)
        -> std::optional<std::pair<std::size_t, VhdlEdgeDirection>> {
        std::size_t equals = last;
        int equalityDepth = 0;
        for (auto token = first; token < last; ++token) {
            if (tokens[token].text == "(") ++equalityDepth;
            else if (tokens[token].text == ")" && equalityDepth > 0) --equalityDepth;
            else if (equalityDepth == 0 && tokens[token].text == "=") {
                if (equals != last) return std::nullopt;
                equals = token;
            }
        }
        if (equals == last) return std::nullopt;
        if (equals == first + 1 && basicIdentifier(first)) {
            const auto direction = binaryLiteral(equals + 1, last);
            if (direction != VhdlEdgeDirection::Unknown) return std::pair{first, direction};
        }
        if (equals + 2 == last && basicIdentifier(equals + 1)) {
            const auto direction = binaryLiteral(first, equals);
            if (direction != VhdlEdgeDirection::Unknown)
                return std::pair{equals + 1, direction};
        }
        return std::nullopt;
    };
    const auto leftEvent = eventIdentifier(leftBegin, leftEnd);
    const auto rightEvent = eventIdentifier(rightBegin, rightEnd);
    const auto leftEquality = equality(leftBegin, leftEnd);
    const auto rightEquality = equality(rightBegin, rightEnd);
    const auto event = leftEvent ? leftEvent : rightEvent;
    const auto compared = leftEvent ? rightEquality : rightEvent ? leftEquality : std::nullopt;
    if (!event || !compared || lower(tokens[*event].text) != lower(tokens[compared->first].text))
        return result;
    const auto clock = compared->first;
    const auto direction = compared->second;
    result.clockName = lower(tokens[clock].text);
    result.clockToken = clock; // native caller's out-pointer selects the EQ identifier
    result.direction = direction;
    result.status = VhdlEdgeConditionStatus::Matched;
    return result;
}

// Reconstruct FUN_1804c4430's bounded traversal contract for callers that need
// the first edge anywhere below a condition root. The native filter table is
// {AST_POSEDGE, AST_NEGEDGE, AST_LOGIC_AND, AST_BIT_AND}; FUN_1804f3870 records
// a matching current node before visiting children from left to right. This
// helper preserves that preorder for logical expression trees and source-order
// child lists while leaving unsupported name binding incomplete.
[[nodiscard]] inline VhdlEdgeCondition
findFirstVhdlEdgeCondition(const core::SourceFile& source,
                           const std::vector<parser::HdlToken>& tokens,
                           std::size_t begin, std::size_t end) {
    VhdlEdgeCondition unsupported;
    if (source.language() != core::HdlLanguage::Vhdl) return unsupported;
    if (begin >= end || end > tokens.size()) {
        unsupported.status = VhdlEdgeConditionStatus::Incomplete;
        return unsupported;
    }
    const auto lower = [](std::string text) {
        for (auto& c : text) if (c >= 'A' && c <= 'Z') c = static_cast<char>(c - 'A' + 'a');
        return text;
    };
    const auto closeParen = [&](std::size_t open, std::size_t limit) {
        int depth = 0;
        for (auto token = open; token < limit; ++token) {
            if (tokens[token].text == "(") ++depth;
            else if (tokens[token].text == ")" && --depth == 0) return token;
        }
        return limit;
    };
    const auto stripOuterParentheses = [&](std::size_t first, std::size_t last) {
        while (first < last && tokens[first].text == "(") {
            const auto close = closeParen(first, last);
            if (close + 1 != last) break;
            ++first;
            --last;
        }
        return std::pair{first, last};
    };
    std::function<VhdlEdgeCondition(std::size_t, std::size_t)> visit;
    visit = [&](std::size_t first, std::size_t last) -> VhdlEdgeCondition {
        const auto [rootBegin, rootEnd] = stripOuterParentheses(first, last);
        if (rootBegin >= rootEnd) {
            VhdlEdgeCondition incomplete;
            incomplete.status = VhdlEdgeConditionStatus::Incomplete;
            return incomplete;
        }
        auto current = parseVhdlEdgeCondition(source, tokens, rootBegin, rootEnd);
        if (current.status == VhdlEdgeConditionStatus::Matched) return current;

        // Argument/aggregate children are visited in source order.
        std::vector<std::size_t> commas;
        int depth = 0;
        for (auto token = rootBegin; token < rootEnd; ++token) {
            if (tokens[token].text == "(") ++depth;
            else if (tokens[token].text == ")" && depth > 0) --depth;
            else if (depth == 0 && tokens[token].text == ",") commas.push_back(token);
        }
        if (!commas.empty()) {
            auto segment = rootBegin;
            commas.push_back(rootEnd);
            for (const auto comma : commas) {
                if (segment < comma) {
                    auto child = visit(segment, comma);
                    if (child.status == VhdlEdgeConditionStatus::Matched) return child;
                }
                segment = comma + 1;
            }
            return current;
        }

        // VHDL logical operators are left associative: the last top-level
        // operator is the root, whose left subtree is visited before its right.
        std::size_t logical = rootEnd;
        depth = 0;
        for (auto token = rootBegin; token < rootEnd; ++token) {
            if (tokens[token].text == "(") ++depth;
            else if (tokens[token].text == ")" && depth > 0) --depth;
            else if (depth == 0 && tokens[token].kind == parser::HdlTokenKind::Identifier) {
                const auto word = lower(tokens[token].text);
                if (word == "and" || word == "or" || word == "xor" ||
                    word == "nand" || word == "nor" || word == "xnor") logical = token;
            }
        }
        if (logical != rootEnd) {
            auto left = visit(rootBegin, logical);
            if (left.status == VhdlEdgeConditionStatus::Matched) return left;
            auto right = visit(logical + 1, rootEnd);
            if (right.status == VhdlEdgeConditionStatus::Matched) return right;
            return current;
        }
        if (lower(tokens[rootBegin].text) == "not" && rootBegin + 1 < rootEnd)
            return visit(rootBegin + 1, rootEnd);

        // For other AST node kinds, descend into source parentheses in order;
        // direct edge calls are attempted at their own node before arguments.
        for (auto token = rootBegin; token < rootEnd; ++token) {
            if (tokens[token].kind == parser::HdlTokenKind::Identifier && token + 1 < rootEnd &&
                (lower(tokens[token].text) == "rising_edge" ||
                 lower(tokens[token].text) == "falling_edge") &&
                tokens[token + 1].text == "(") {
                const auto close = closeParen(token + 1, rootEnd);
                if (close < rootEnd) {
                    auto edge = parseVhdlEdgeCondition(source, tokens, token, close + 1);
                    if (edge.status == VhdlEdgeConditionStatus::Matched) return edge;
                }
            }
            if (tokens[token].text != "(") continue;
            const auto close = closeParen(token, rootEnd);
            if (close >= rootEnd) break;
            auto child = visit(token + 1, close);
            if (child.status == VhdlEdgeConditionStatus::Matched) return child;
            token = close;
        }
        return current;
    };
    return visit(begin, end);
}

} // namespace fpga::analysis
