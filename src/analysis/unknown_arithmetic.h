#pragma once

#include "parser/design_parser.h"

#include <algorithm>
#include <cstddef>
#include <functional>
#include <string_view>
#include <vector>

namespace fpga::analysis::detail {

// Independently implemented expression-tree walk. The caller supplies an
// admitted ALWAYS/continuous-ASSIGN root interval and the normalized-literal
// predicate; this helper does not decide module/process scope or fold names.
// Native evidence: 8.5 selects binary ADD/SUB/MUL/DIV/MOD/POW descendants,
// then recursively checks each selected subtree for X/Z constants.
// ev_869231e3e00357939a5cde478f93d051e876601f110406d8c9083277445d45eb
// ev_64f1059ebe9591553a1ca8ce04e68a11ec13f174085f30430d2355624bf9696d
// ev_fa14e04dbded349cef291eeeab29d025885bb826dbae5af594950decf08da31d
struct UnknownArithmeticOccurrence {
    std::size_t operatorIndex{};
    char state{};
};

class UnknownArithmeticParser {
public:
    using LiteralMask = std::function<unsigned(const parser::HdlToken&)>;
    UnknownArithmeticParser(const std::vector<parser::HdlToken>& tokens,
                            std::size_t begin, std::size_t end,
                            LiteralMask unknownLiteral)
        : tokens_(tokens), position_(std::min(begin, tokens.size())),
          end_(std::min(end, tokens.size())), unknownLiteral_(std::move(unknownLiteral)) {}

    [[nodiscard]] std::vector<UnknownArithmeticOccurrence> collect() {
        while (position_ < end_) {
            const auto start = position_;
            const auto checkpoint = operators_.size();
            const auto value = expression(0, 0);
            if (!value.valid) operators_.resize(checkpoint);
            if (position_ == start) ++position_;
        }
        std::sort(operators_.begin(), operators_.end(), [](const auto& left, const auto& right) {
            return left.operatorIndex < right.operatorIndex ||
                (left.operatorIndex == right.operatorIndex && left.state < right.state);
        });
        operators_.erase(std::unique(operators_.begin(), operators_.end(),
            [](const auto& left, const auto& right) {
                return left.operatorIndex == right.operatorIndex && left.state == right.state;
            }), operators_.end());
        return operators_;
    }

private:
    struct Value { unsigned unknownMask{0}; bool valid{false}; };
    const std::vector<parser::HdlToken>& tokens_;
    std::size_t position_;
    std::size_t end_;
    LiteralMask unknownLiteral_;
    std::vector<UnknownArithmeticOccurrence> operators_;

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
        if (op == "<" || op == "<=" || op == ">" || op == ">=") return 8;
        if (op == "<<" || op == ">>" || op == "<<<" || op == ">>>") return 9;
        if (op == "+" || op == "-") return 10;
        if (op == "*" || op == "/" || op == "%") return 11;
        if (op == "**") return 12;
        return -1;
    }
    static bool arithmetic(std::string_view op) {
        return op == "+" || op == "-" || op == "*" || op == "/" || op == "%" || op == "**";
    }
    static bool keyword(std::string_view text) {
        return text == "begin" || text == "end" || text == "if" || text == "else" ||
            text == "case" || text == "casex" || text == "casez" || text == "endcase" ||
            text == "default" || text == "for" || text == "while" || text == "repeat" ||
            text == "always" || text == "always_comb" || text == "always_ff" ||
            text == "always_latch" || text == "assign" || text == "posedge" ||
            text == "negedge" || text == "or";
    }

    Value expression(int minimum, unsigned depth) {
        if (depth > 128) return {};
        auto lhs = primary(depth + 1);
        if (!lhs.valid) return lhs;
        while (position_ < end_) {
            const auto index = position_;
            const auto op = tokens_[position_].text;
            const auto power = precedence(op);
            if (power < minimum) break;
            ++position_;
            if (op == "?") {
                const auto yes = expression(0, depth + 1);
                if (!yes.valid || !accept(":")) return {};
                const auto no = expression(power, depth + 1);
                if (!no.valid) return {};
                lhs.unknownMask |= yes.unknownMask | no.unknownMask;
                continue;
            }
            const auto rhs = expression(power + (op == "**" ? 0 : 1), depth + 1);
            if (!rhs.valid) return {};
            lhs.unknownMask |= rhs.unknownMask;
            if (arithmetic(op)) {
                // Native slot 0 calls the recursive helper once with 'z' and
                // once with 'x', and emits independently for both results.
                if ((lhs.unknownMask & 1U) != 0) operators_.push_back({index, 'x'});
                if ((lhs.unknownMask & 2U) != 0) operators_.push_back({index, 'z'});
            }
        }
        return lhs;
    }

    Value list(std::string_view closer, unsigned depth) {
        Value result{false, true};
        if (accept(closer)) return result;
        for (;;) {
            const auto item = expression(0, depth + 1);
            if (!item.valid) return {};
            result.unknownMask |= item.unknownMask;
            // Replication: {count{items}}. Inner braces are one operand;
            // unlike comma siblings, its unknown propagates to the container.
            if (at("{")) {
                const auto repeated = primary(depth + 1);
                if (!repeated.valid) return {};
                result.unknownMask |= repeated.unknownMask;
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
            // Unary POS/NEG are intentionally not selected as binary arithmetic.
            return expression(12, depth + 1);
        }
        Value value;
        if (accept("(")) {
            value = expression(0, depth + 1);
            if (!value.valid || !accept(")")) return {};
        } else if (accept("{")) {
            value = list("}", depth + 1);
        } else {
            const auto& token = tokens_[position_];
            if (token.kind == parser::HdlTokenKind::Symbol || keyword(token.text)) return {};
            value = {unknownLiteral_(token), true};
            ++position_;
        }
        if (!value.valid) return value;
        for (;;) {
            if (accept("(")) {
                const auto arguments = list(")", depth + 1);
                if (!arguments.valid) return {};
                value.unknownMask |= arguments.unknownMask;
            } else if (accept("[")) {
                const auto index = expression(0, depth + 1);
                if (!index.valid) return {};
                value.unknownMask |= index.unknownMask;
                if (accept(":") || accept("+:") || accept("-:")) {
                    const auto right = expression(0, depth + 1);
                    if (!right.valid) return {};
                    value.unknownMask |= right.unknownMask;
                }
                if (!accept("]")) return {};
            } else if (accept("." ) || accept("::")) {
                if (position_ >= end_ || tokens_[position_].kind != parser::HdlTokenKind::Identifier)
                    return {};
                ++position_;
            } else break;
        }
        return value;
    }
};

[[nodiscard]] inline std::vector<UnknownArithmeticOccurrence> unknownArithmeticOccurrences(
    const std::vector<parser::HdlToken>& tokens, std::size_t begin, std::size_t end,
    UnknownArithmeticParser::LiteralMask predicate) {
    return UnknownArithmeticParser(tokens, begin, end, std::move(predicate)).collect();
}

} // namespace fpga::analysis::detail
