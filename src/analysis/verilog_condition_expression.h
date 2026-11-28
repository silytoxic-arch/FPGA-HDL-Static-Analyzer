#pragma once
#include "parser/design_parser.h"
#include <algorithm>
#include <functional>
#include <optional>
#include <string_view>
#include <utility>

namespace fpga::analysis::detail {
// Bounded expression-tree width adapter for the native flag-zero condition
// helper. Input operators are normalized (as supplied by standardTokens).
// The parser must consume the entire range; unsupported constructs
// retain an unknown width. This is not HDL elaboration or type resolution.
class VerilogConditionExpression {
public:
    using Width = std::optional<std::size_t>;
    using Resolver = std::function<Width(std::size_t,std::size_t)>;
    VerilogConditionExpression(const std::vector<parser::HdlToken>& tokens,
        std::size_t begin,std::size_t end,Resolver leaf,Resolver count)
        : tokens_(tokens),at_(begin),end_(std::min(end,tokens.size())),
          leaf_(std::move(leaf)),count_(std::move(count)) {}
    [[nodiscard]] Width width() {
        const auto value=expression(0,0);
        return value.valid && at_==end_ ? value.width : Width{};
    }
private:
    struct Value { bool valid{};Width width; };
    const std::vector<parser::HdlToken>& tokens_;
    std::size_t at_,end_;
    Resolver leaf_,count_;
    bool is(std::string_view s) const {return at_<end_&&tokens_[at_].text==s;}
    bool take(std::string_view s) {if(!is(s))return false;++at_;return true;}
    // Native grammar table replay corroborates unary > power > arithmetic >
    // relation > equality > bitwise > logical; power is left associative.
    static int precedence(std::string_view op) {
        if(op=="?")return 1;
        if(op=="||")return 2;
        if(op=="&&")return 3;
        if(op=="|")return 4;
        if(op=="^"||op=="^~"||op=="~^")return 5;
        if(op=="&")return 6;
        if(op=="=="||op=="!="||op=="==="||op=="!==")return 7;
        if(op=="<"||op=="<="||op==">"||op==">=")return 8;
        if(op=="<<"||op==">>"||op=="<<<"||op==">>>")return 9;
        if(op=="+"||op=="-")return 10;
        if(op=="*"||op=="/"||op=="%")return 11;
        if(op=="**")return 12;
        return -1;
    }
    Value expression(int minimum,unsigned depth) {
        if(depth>128)return {};
        auto left=primary(depth+1);
        if(!left.valid)return {};
        while(at_<end_) {
            const auto op=tokens_[at_].text;
            const auto rank=precedence(op);
            if(rank<minimum)break;
            ++at_;
            if(op=="?") {
                const auto yes=expression(0,depth+1);
                if(!yes.valid||!take(":"))return {};
                const auto no=expression(rank,depth+1);
                if(!no.valid)return {};
                // Full native ternary-result width semantics remain unproved.
                left.width.reset();
                continue;
            }
            const auto right=expression(rank+1,depth+1);
            if(!right.valid)return {};
            if(rank==2||rank==3||rank==7||rank==8)left.width=1;
            else if(rank==9)left.width.reset(); // shift width helper not admitted
            else if(left.width&&right.width) {
                if(op!="**")left.width=std::max(*left.width,*right.width);
            } else left.width.reset();
        }
        return left;
    }
    Value primary(unsigned depth) {
        if(at_>=end_||depth>128)return {};
        const auto start=at_;
        const auto op=tokens_[at_].text;
        if(op=="!"||op=="~"||op=="+"||op=="-"||op=="&"||op=="|"||
           op=="^"||op=="~&"||op=="~|"||op=="~^"||op=="^~") {
            ++at_;
            auto child=expression(13,depth+1);
            if(!child.valid)return {};
            if(op=="!")child.width=1;
            else if(op!="~"&&op!="+"&&op!="-")child.width.reset();
            return child;
        }
        if(take("(")) {
            auto value=expression(0,depth+1);
            if(!value.valid||!take(")"))return {};
            return value;
        }
        if(take("{")) {
            const auto countBegin=at_;
            auto first=expression(0,depth+1);
            if(!first.valid)return {};
            if(is("{")) {
                const auto repetitions=count_(countBegin,at_);
                const auto repeated=primary(depth+1);
                if(!repeated.valid||!take("}"))return {};
                Width width;
                if(repetitions&&*repetitions>0&&repeated.width&&*repeated.width>0&&
                   *repetitions<=65536 / *repeated.width)width=*repetitions * *repeated.width;
                return {true,width};
            }
            Width total=first.width;
            while(take(",")) {
                const auto item=expression(0,depth+1);
                if(!item.valid)return {};
                if(total&&item.width&&*item.width<=65536&&*total<=65536-*item.width)*total+=*item.width;
                else total.reset();
            }
            if(!take("}"))return {};
            return {true,total};
        }
        if(tokens_[at_].kind!=parser::HdlTokenKind::Identifier&&
           tokens_[at_].kind!=parser::HdlTokenKind::Number)return {};
        ++at_;
        bool unsupported=false;
        for(;;) {
            if(take("(")) {
                unsupported=true;
                if(!take(")")) {
                    do {if(!expression(0,depth+1).valid)return {};}while(take(","));
                    if(!take(")"))return {};
                }
            } else if(take("[")) {
                if(!expression(0,depth+1).valid)return {};
                if(take(":")||take("+:")||take("-:"))
                    if(!expression(0,depth+1).valid)return {};
                if(!take("]"))return {};
            } else if(take(".")||take("::")) {
                unsupported=true;
                if(at_>=end_||tokens_[at_].kind!=parser::HdlTokenKind::Identifier)return {};
                ++at_;
            } else break;
        }
        return {true,unsupported?Width{}:leaf_(start,at_)};
    }
};
} // namespace fpga::analysis::detail
