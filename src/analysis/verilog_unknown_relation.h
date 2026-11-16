#pragma once

#include "parser/design_parser.h"

#include <algorithm>
#include <cstddef>
#include <functional>
#include <iterator>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

namespace fpga::analysis::detail {

struct UnknownRelationOccurrence {
    std::size_t operatorIndex{};
    std::size_t literalIndex{};
    char state{}; // 'x' or 'z'; native emits the two classes separately.
};

// Source-level expression tree adapter for GJB10157.8.4. Native slot 1 admits
// AST_LT/LE/GE/GT (0x34,0x35,0x3a,0x3b). Slot 0 recursively gathers constants
// under each admitted relation and reports encoded X (0x03) and Z (0x02)
// classes independently. This parser preserves that subtree ownership rather
// than borrowing an unknown literal from the surrounding statement.
class UnknownRelationParser {
public:
    using LiteralMask = std::function<unsigned(const parser::HdlToken&)>;
    using RelationFilter = std::function<bool(std::size_t)>;

    UnknownRelationParser(const std::vector<parser::HdlToken>& tokens,
                          std::size_t begin, std::size_t end,
                          LiteralMask literalMask, RelationFilter relationFilter)
        : tokens_(tokens), position_(std::min(begin, tokens.size())),
          end_(std::min(end, tokens.size())), literalMask_(std::move(literalMask)),
          relationFilter_(std::move(relationFilter)) {}

    [[nodiscard]] std::vector<UnknownRelationOccurrence> collect() {
        while (position_ < end_) {
            const auto start = position_;
            const auto checkpoint = occurrences_.size();
            const auto value = expression(0, 0);
            if (!value.valid) occurrences_.resize(checkpoint);
            if (position_ == start) ++position_;
        }
        std::sort(occurrences_.begin(), occurrences_.end(), [](const auto& a, const auto& b) {
            return std::tie(a.operatorIndex, a.literalIndex, a.state) <
                   std::tie(b.operatorIndex, b.literalIndex, b.state);
        });
        occurrences_.erase(std::unique(occurrences_.begin(), occurrences_.end(),
            [](const auto& a, const auto& b) {
                return a.operatorIndex == b.operatorIndex &&
                       a.literalIndex == b.literalIndex && a.state == b.state;
            }), occurrences_.end());
        return occurrences_;
    }

private:
    struct Literal { std::size_t index{}; unsigned mask{}; };
    struct Value { std::vector<Literal> unknowns; bool valid{false}; };

    const std::vector<parser::HdlToken>& tokens_;
    std::size_t position_;
    std::size_t end_;
    LiteralMask literalMask_;
    RelationFilter relationFilter_;
    std::vector<UnknownRelationOccurrence> occurrences_;

    bool at(std::string_view text) const {
        return position_ < end_ && tokens_[position_].text == text;
    }
    bool accept(std::string_view text) {
        if (!at(text)) return false;
        ++position_;
        return true;
    }
    static int precedence(std::string_view op) {
        if (op == "?") return 1;
        if (op == "||") return 2;
        if (op == "&&") return 3;
        if (op == "|") return 4;
        if (op == "^" || op == "^~" || op == "~^") return 5;
        if (op == "&") return 6;
        if (op == "==" || op == "!=" || op == "===" || op == "!==" ||
            op == "==?" || op == "!=?") return 7;
        if (relation(op)) return 8;
        if (op == "<<" || op == ">>" || op == "<<<" || op == ">>>") return 9;
        if (op == "+" || op == "-") return 10;
        if (op == "*" || op == "/" || op == "%") return 11;
        if (op == "**") return 12;
        return -1;
    }
    static bool relation(std::string_view op) {
        return op == "<" || op == "<=" || op == ">" || op == ">=";
    }
    static bool keyword(std::string_view text) {
        return text == "begin" || text == "end" || text == "if" || text == "else" ||
            text == "case" || text == "casex" || text == "casez" || text == "endcase" ||
            text == "default" || text == "for" || text == "while" || text == "repeat" ||
            text == "always" || text == "always_comb" || text == "always_ff" ||
            text == "always_latch" || text == "assign" || text == "posedge" ||
            text == "negedge" || text == "or";
    }
    static void append(Value& target, Value&& source) {
        target.unknowns.insert(target.unknowns.end(),
            std::make_move_iterator(source.unknowns.begin()),
            std::make_move_iterator(source.unknowns.end()));
    }

    Value expression(int minimum, unsigned depth) {
        if (depth > 128) return {};
        auto lhs = primary(depth + 1);
        if (!lhs.valid) return lhs;
        while (position_ < end_) {
            const auto index = position_;
            const auto op = tokens_[position_].text;
            const auto power = precedence(op);
            if (power < minimum || (relation(op) && !relationFilter_(index))) break;
            ++position_;
            if (op == "?") {
                auto yes = expression(0, depth + 1);
                if (!yes.valid || !accept(":")) return {};
                auto no = expression(power, depth + 1);
                if (!no.valid) return {};
                append(lhs, std::move(yes));
                append(lhs, std::move(no));
                continue;
            }
            auto rhs = expression(power + (op == "**" ? 0 : 1), depth + 1);
            if (!rhs.valid) return {};
            append(lhs, std::move(rhs));
            if (relation(op)) {
                for (const auto& literal : lhs.unknowns) {
                    if ((literal.mask & 1U) != 0)
                        occurrences_.push_back({index, literal.index, 'x'});
                    if ((literal.mask & 2U) != 0)
                        occurrences_.push_back({index, literal.index, 'z'});
                }
            }
        }
        return lhs;
    }

    Value list(std::string_view closer, unsigned depth) {
        Value result{{}, true};
        if (accept(closer)) return result;
        for (;;) {
            auto item = expression(0, depth + 1);
            if (!item.valid) return {};
            append(result, std::move(item));
            if (at("{")) {
                auto repeated = primary(depth + 1);
                if (!repeated.valid) return {};
                append(result, std::move(repeated));
            }
            if (accept(closer)) return result;
            if (!accept(",")) return {};
        }
    }

    Value primary(unsigned depth) {
        if (position_ >= end_ || depth > 128) return {};
        if (at("+") || at("-") || at("!") || at("~") || at("&") || at("|") ||
            at("^") || at("~&") || at("~|") || at("~^") || at("^~")) {
            ++position_;
            return expression(12, depth + 1);
        }
        Value value{{}, true};
        if (accept("(")) {
            value = expression(0, depth + 1);
            if (!value.valid || !accept(")")) return {};
        } else if (accept("{")) {
            value = list("}", depth + 1);
        } else {
            const auto index = position_;
            const auto& token = tokens_[position_];
            if (token.kind == parser::HdlTokenKind::Symbol || keyword(token.text)) return {};
            const auto mask = literalMask_(token);
            if (mask != 0) value.unknowns.push_back({index, mask});
            ++position_;
        }
        if (!value.valid) return value;
        for (;;) {
            if (accept("(")) {
                auto arguments = list(")", depth + 1);
                if (!arguments.valid) return {};
                append(value, std::move(arguments));
            } else if (accept("[")) {
                auto index = expression(0, depth + 1);
                if (!index.valid) return {};
                append(value, std::move(index));
                if (accept(":") || accept("+:") || accept("-:")) {
                    auto right = expression(0, depth + 1);
                    if (!right.valid) return {};
                    append(value, std::move(right));
                }
                if (!accept("]")) return {};
            } else if (accept(".") || accept("::")) {
                if (position_ >= end_ ||
                    tokens_[position_].kind != parser::HdlTokenKind::Identifier) return {};
                ++position_;
            } else break;
        }
        return value;
    }
};

[[nodiscard]] inline std::vector<UnknownRelationOccurrence> unknownRelationOccurrences(
    const std::vector<parser::HdlToken>& tokens, std::size_t begin, std::size_t end,
    UnknownRelationParser::LiteralMask predicate,
    UnknownRelationParser::RelationFilter relationFilter = [](std::size_t) { return true; }) {
    return UnknownRelationParser(tokens, begin, end, std::move(predicate),
                                 std::move(relationFilter)).collect();
}

} // namespace fpga::analysis::detail
