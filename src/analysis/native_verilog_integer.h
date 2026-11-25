#pragma once

#include "analysis/verilog_constant_expression.h"

#include <algorithm>
#include <bit>
#include <charconv>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace fpga::analysis::detail {

// Recovered two-state integer folding domain, not a general HDL elaborator.
// Native: simplify constant-operand dispatch -> bit-vector result -> +0xbc
// low32 cache. Context width/signedness is propagated before arithmetic.
// Unsupported widths, four-state operations and overridable values stay unknown.
class NativeVerilogInteger {
public:
    struct Value {
        std::uint64_t bits{};
        unsigned width{};
        bool signedValue{};
        std::size_t source{};
        [[nodiscard]] std::uint32_t cache() const {
            auto result = static_cast<std::uint32_t>(bits);
            if (signedValue && width < 32 && negative())
                result |= ~static_cast<std::uint32_t>(mask(width));
            return result;
        }
        [[nodiscard]] bool negative() const {
            return signedValue && width > 0 && ((bits >> (width - 1)) & 1U) != 0;
        }
    };
    NativeVerilogInteger(const std::vector<parser::HdlToken>& tokens,
                         const VerilogConstantExpressions& symbols)
        : tokens_(tokens), symbols_(symbols) {}

    [[nodiscard]] std::optional<Value> evaluate(std::size_t begin, std::size_t end) {
        const auto node = parse(begin, end, 0);
        return node ? fold(node, 0, std::nullopt, 0) : std::nullopt;
    }

private:
    static std::uint64_t mask(unsigned width) {
        return width == 64 ? ~std::uint64_t{0} : (std::uint64_t{1} << width) - 1;
    }
    struct Node;
    using Ptr = std::shared_ptr<Node>;
    struct Node {
        std::string op;
        Value value;
        Ptr left, right;
        unsigned width{};
        bool sign{};
        std::size_t source{};
    };
    struct Operator { std::string text; std::size_t count{1}; };
    const std::vector<parser::HdlToken>& tokens_;
    const VerilogConstantExpressions& symbols_;
    std::vector<std::pair<std::size_t, std::size_t>> active_;

    Operator operation(std::size_t at, std::size_t end) const {
        auto text = tokens_[at].text;
        if (text.size() != 1) return {text, 1};
        auto joined = text;
        for (auto next = at + 1; next < end && next < at + 3; ++next) {
            if (tokens_[next].text.size() != 1 ||
                tokens_[next - 1].offset + tokens_[next - 1].text.size() != tokens_[next].offset) break;
            joined += tokens_[next].text;
            if (joined == "===" || joined == "!==" || joined == "<<<" || joined == ">>>")
                return {joined, next - at + 1};
        }
        if (joined.size() >= 2) {
            const auto pair = joined.substr(0, 2);
            if (pair == "&&" || pair == "||" || pair == "==" || pair == "!=" ||
                pair == "<=" || pair == ">=" || pair == "<<" || pair == ">>" ||
                pair == "**" || pair == "~&" || pair == "~|" || pair == "~^" || pair == "^~")
                return {pair, 2};
        }
        return {text, 1};
    }
    static int precedence(std::string_view op) {
        if (op == "||") return 1;
        if (op == "&&") return 2;
        if (op == "|") return 3;
        if (op == "^" || op == "^~" || op == "~^") return 4;
        if (op == "&") return 5;
        if (op == "==" || op == "!=" || op == "===" || op == "!==") return 6;
        if (op == "<" || op == "<=" || op == ">" || op == ">=") return 7;
        if (op == "<<" || op == ">>" || op == "<<<" || op == ">>>") return 8;
        if (op == "+" || op == "-") return 9;
        if (op == "*" || op == "/" || op == "%") return 10;
        if (op == "**") return 12;
        return 0;
    }
    static bool booleanBinary(std::string_view op) {
        const auto p = precedence(op);
        return (p >= 1 && p <= 2) || (p >= 6 && p <= 7);
    }
    static bool shift(std::string_view op) {
        return op == "<<" || op == ">>" || op == "<<<" || op == ">>>";
    }
    static bool booleanUnary(std::string_view op) {
        return op != "+" && op != "-" && op != "~";
    }
    static Ptr literal(Value value) {
        return std::make_shared<Node>(Node{{}, value, {}, {}, value.width, value.signedValue, value.source});
    }
    static std::optional<Value> number(std::string text, std::size_t source) {
        text.erase(std::remove(text.begin(), text.end(), '_'), text.end());
        const auto quote = text.find('\'');
        unsigned width = 0, base = 10;
        bool sign = true;
        std::size_t at = 0;
        if (quote != std::string::npos) {
            if (quote == 0) return std::nullopt;
            const auto [last, error] = std::from_chars(text.data(), text.data() + quote, width);
            if (error != std::errc{} || last != text.data() + quote || width == 0 || width > 64)
                return std::nullopt;
            sign = false;
            at = quote + 1;
            if (at < text.size() && text[at] == 's') { sign = true; ++at; }
            if (at == text.size()) return std::nullopt;
            switch (text[at++]) {
            case 'b': case 'B': base = 2; break;
            case 'o': case 'O': base = 8; break;
            case 'd': case 'D': base = 10; break;
            case 'h': case 'H': base = 16; break;
            default: return std::nullopt;
            }
        }
        if (at == text.size()) return std::nullopt;
        std::uint64_t bits = 0;
        for (; at < text.size(); ++at) {
            const auto c = text[at];
            const unsigned digit = c >= '0' && c <= '9' ? c - '0' :
                c >= 'a' && c <= 'f' ? c - 'a' + 10 : c >= 'A' && c <= 'F' ? c - 'A' + 10 : 99;
            if (digit >= base) return std::nullopt;
            if (quote == std::string::npos && bits > (~std::uint64_t{0} - digit) / base)
                return std::nullopt;
            bits = bits * base + digit;
        }
        if (quote == std::string::npos) {
            // Native decimal conversion appends a zero sign bit when needed.
            width = std::max(32U, static_cast<unsigned>(std::bit_width(bits)) + 1U);
            if (width > 64) return std::nullopt;
        }
        return Value{bits & mask(width), width, sign, source};
    }
    Ptr parameter(std::size_t index, unsigned depth) {
        const auto binding = symbols_.localParameter(index);
        if (!binding || depth >= 256) return {};
        const auto key = std::pair{binding->begin, binding->end};
        if (std::ranges::find(active_, key) != active_.end()) return {};
        std::optional<unsigned> width;
        std::optional<bool> sign;
        auto at = binding->declaration + 1;
        bool integerType = false;
        for (; at < binding->begin;) {
            const auto& word = tokens_[at].text;
            if (word == "signed" || word == "unsigned") { sign = word == "signed"; ++at; continue; }
            if (word == "integer" || word == "int" || word == "shortint" ||
                word == "longint" || word == "byte" || word == "time") {
                if (width) return {};
                width = word == "shortint" ? 16 : word == "byte" ? 8 :
                        word == "longint" || word == "time" ? 64 : 32;
                sign = word != "time";
                integerType = true; ++at; continue;
            }
            if (word == "logic" || word == "reg" || word == "bit") {
                if (width) return {};
                width = 1; if (!sign) sign = false; ++at; continue;
            }
            if (word == "[") {
                if (integerType || (width && *width != 1)) return {};
                const auto bound = [&](std::size_t& cursor) -> std::optional<std::int64_t> {
                    bool negative = false;
                    if (cursor < binding->begin && tokens_[cursor].text == "-") { negative = true; ++cursor; }
                    if (cursor >= binding->begin) return std::nullopt;
                    const auto& token = tokens_[cursor++].text;
                    std::uint64_t result = 0;
                    const auto [last, error] = std::from_chars(token.data(), token.data() + token.size(), result);
                    if (error != std::errc{} || last != token.data() + token.size() || result > 0x7fffffffU)
                        return std::nullopt;
                    return negative ? -static_cast<std::int64_t>(result) : static_cast<std::int64_t>(result);
                };
                ++at;
                const auto left = bound(at);
                if (!left || at >= binding->begin || tokens_[at++].text != ":") return {};
                const auto right = bound(at);
                if (!right || at >= binding->begin || tokens_[at++].text != "]") return {};
                const auto size = (*left >= *right ? *left - *right : *right - *left) + 1;
                if (size > 64) return {};
                width = static_cast<unsigned>(size); if (!sign) sign = false;
                continue;
            }
            break;
        }
        if (at + 1 >= tokens_.size() || tokens_[at].kind != parser::HdlTokenKind::Identifier ||
            tokens_[at + 1].text != "=") return {};
        active_.push_back(key);
        const auto root = parse(binding->begin, binding->end, depth + 1);
        auto value = root ? fold(root, width.value_or(0), std::nullopt, depth + 1) : std::nullopt;
        active_.pop_back();
        if (!value) return {};
        if (width) { value->bits &= mask(*width); value->width = *width; }
        if (sign) value->signedValue = *sign;
        value->source = index;
        return literal(*value);
    }
    Ptr parse(std::size_t begin, std::size_t end, unsigned depth) {
        if (begin >= end || end > tokens_.size() || depth >= 256) return {};
        struct Parser {
            NativeVerilogInteger& owner;
            std::size_t at, end;
            Ptr expression(int minimum, unsigned depth) {
                if (depth >= 256 || at >= end) return {};
                auto op = owner.operation(at, end);
                const auto source = at;
                Ptr lhs;
                if (op.text == "+" || op.text == "-" || op.text == "!" || op.text == "~" ||
                    op.text == "&" || op.text == "|" || op.text == "^" || op.text == "~&" ||
                    op.text == "~|" || op.text == "~^" || op.text == "^~") {
                    at += op.count;
                    const auto operand = expression(12, depth + 1);
                    if (!operand) return {};
                    const bool boolean = booleanUnary(op.text);
                    lhs = std::make_shared<Node>(Node{op.text, {}, operand, {},
                        boolean ? 1U : operand->width, !boolean && operand->sign, source});
                } else if (op.text == "(") {
                    ++at; lhs = expression(0, depth + 1);
                    if (!lhs || at >= end || owner.tokens_[at++].text != ")") return {};
                } else if (owner.tokens_[at].kind == parser::HdlTokenKind::Number) {
                    const auto value = number(owner.tokens_[at].text, at);
                    if (!value) return {};
                    lhs = literal(*value); ++at;
                } else if (owner.tokens_[at].kind == parser::HdlTokenKind::Identifier) {
                    lhs = owner.parameter(at++, depth + 1);
                    if (!lhs) return {};
                } else return {};
                while (at < end) {
                    op = owner.operation(at, end);
                    const auto p = precedence(op.text);
                    if (p == 0 || p < minimum) break;
                    at += op.count;
                    const auto rhs = expression(p + (op.text == "**" ? 0 : 1), depth + 1);
                    if (!rhs) return {};
                    const bool boolean = booleanBinary(op.text);
                    const bool leftWidth = shift(op.text) || op.text == "**";
                    lhs = std::make_shared<Node>(Node{op.text, {}, lhs, rhs,
                        boolean ? 1U : leftWidth ? lhs->width : std::max(lhs->width, rhs->width),
                        !boolean && (leftWidth ? lhs->sign : lhs->sign && rhs->sign), lhs->source});
                }
                return lhs;
            }
        } parser{*this, begin, end};
        auto root = parser.expression(0, depth + 1);
        return parser.at == end ? root : Ptr{};
    }
    static Value resized(Value value, unsigned width, bool sign) {
        if (width > value.width && sign && value.negative()) value.bits |= ~mask(value.width);
        value.bits &= mask(width); value.width = width; value.signedValue = sign;
        return value;
    }
    std::optional<Value> fold(const Ptr& node, unsigned hint, std::optional<bool> forcedSign, unsigned depth) {
        if (!node || depth >= 256) return std::nullopt;
        const auto width = std::max(node->width, hint);
        if (width == 0 || width > 64) return std::nullopt;
        const bool sign = forcedSign.value_or(node->sign);
        const auto& op = node->op;
        if (op.empty()) return resized(node->value, width, sign);
        const bool boolean = node->right ? booleanBinary(op) : booleanUnary(op);
        const bool logical = op == "&&" || op == "||";
        const auto operandWidth = boolean && node->right && !logical
            ? std::max(node->left->width, node->right->width) : boolean ? 0U : width;
        const auto operandSign = boolean ? std::optional<bool>{} : std::optional<bool>{sign};
        const bool comparisonSign = node->right && node->left->sign && node->right->sign;
        auto lhs = fold(node->left, operandWidth,
            boolean && node->right && !logical ? std::optional<bool>{comparisonSign} : operandSign, depth + 1);
        if (!lhs) return std::nullopt;
        std::uint64_t bits = 0;
        if (!node->right) {
            if (op == "+") bits = lhs->bits;
            else if (op == "-") bits = std::uint64_t{0} - lhs->bits;
            else if (op == "~") bits = ~lhs->bits;
            else if (op == "!") bits = lhs->bits == 0;
            else if (op == "&" || op == "~&") bits = lhs->bits == mask(lhs->width);
            else if (op == "|" || op == "~|") bits = lhs->bits != 0;
            else bits = std::popcount(lhs->bits) % 2;
            if (op == "~&" || op == "~|" || op == "~^" || op == "^~") bits ^= 1;
        } else {
            auto rhs = fold(node->right, shift(op) || op == "**" ? 0 : operandWidth,
                shift(op) || op == "**" ? std::optional<bool>{} :
                boolean && !logical ? std::optional<bool>{comparisonSign} : operandSign, depth + 1);
            if (!rhs) return std::nullopt;
            const auto a = lhs->bits, b = rhs->bits;
            if (op == "+") bits = a + b;
            else if (op == "-") bits = a - b;
            else if (op == "*") bits = a * b;
            else if (op == "/" || op == "%") {
                if (b == 0) return std::nullopt;
                const bool an = lhs->negative(), bn = rhs->negative();
                const auto am = an ? (std::uint64_t{0} - a) & mask(width) : a;
                const auto bm = bn ? (std::uint64_t{0} - b) & mask(width) : b;
                bits = op == "/" ? am / bm : am % bm;
                if (op == "/" ? an != bn : an) bits = std::uint64_t{0} - bits;
            } else if (shift(op)) {
                if (rhs->negative()) return std::nullopt;
                if (op == "<<" || op == "<<<") bits = b >= width ? 0 : a << b;
                else {
                    const bool fill = op == ">>>" && lhs->negative();
                    bits = b >= width ? (fill ? mask(width) : 0) : a >> b;
                    if (fill && b != 0 && b < width) bits |= mask(width) ^ mask(width - static_cast<unsigned>(b));
                }
            } else if (op == "&") bits = a & b;
            else if (op == "|") bits = a | b;
            else if (op == "^" || op == "^~" || op == "~^") {
                bits = a ^ b; if (op != "^") bits = ~bits;
            } else if (op == "&&") bits = a != 0 && b != 0;
            else if (op == "||") bits = a != 0 || b != 0;
            else if (op == "==" || op == "===") bits = a == b;
            else if (op == "!=" || op == "!==") bits = a != b;
            else if (op == "<" || op == "<=" || op == ">" || op == ">=") {
                const bool less = lhs->negative() != rhs->negative() ? lhs->negative() : a < b;
                bits = op == "<" ? less : op == "<=" ? less || a == b :
                       op == ">" ? !less && a != b : !less;
            } else return std::nullopt;
        }
        return Value{bits & mask(width), width, boolean ? false : sign, node->source};
    }
};

} // namespace fpga::analysis::detail
