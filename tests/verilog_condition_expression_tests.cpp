#include "analysis/verilog_condition_expression.h"
#include <iostream>
#include <map>
int main() {
    using namespace fpga;
    int checks=0,failures=0;
    const auto check=[&](bool okay,const char* label){++checks;if(!okay){++failures;std::cerr<<"FAIL "<<label<<'\n';}};
    const auto width=[](const std::string& text) {
        const auto raw=parser::tokenize({"expression.sv",core::HdlLanguage::SystemVerilog,text});
        // The production adapter supplies normalized, comment-free operators.
        std::vector<parser::HdlToken> tokens;
        for(std::size_t i=0;i<raw.size();++i) {
            auto token=raw[i];
            if(i+1<raw.size()&&raw[i+1].offset==token.offset+token.text.size()) {
                const auto pair=token.text+raw[i+1].text;
                if(pair=="=="||pair=="&&"||pair=="**"||pair=="<<") {
                    token.text=pair;++i;
                }
            }
            tokens.push_back(std::move(token));
        }
        return analysis::detail::VerilogConditionExpression(tokens,0,tokens.size(),
            [&](auto first,auto last)->std::optional<std::size_t>{
                if(last!=first+1)return {};
                const std::map<std::string,std::size_t> widths{{"a",1},{"b",4},{"c",8}};
                const auto found=widths.find(tokens[first].text);
                return found==widths.end()?std::nullopt:std::optional<std::size_t>{found->second};
            },[&](auto first,auto last)->std::optional<std::size_t>{
                return last==first+1&&tokens[first].text=="2"?std::optional<std::size_t>{2}:std::nullopt;
            }).width();
    };
    check(width("!a+b")==4,"unary logical child does not turn arithmetic root into boolean");
    check(width("!(a+b)")==1,"outer logical root is boolean");
    check(width("(!a)+b")==4,"explicit unary child agrees with native precedence");
    check(width("a==b|c")==8,"bitwise OR owns equality child");
    check(width("a==(b|c)")==1,"explicit equality root is boolean");
    check(width("a|b==c")==1,"equality binds above bitwise OR");
    check(width("b|a==c")==4,"bitwise parent preserves wider child");
    check(width("a+b==c")==1,"comparison owns arithmetic child");
    check(width("a&&(b+c)")==1,"logical AND root is boolean");
    check(width("a+(b==c)")==1,"known comparison can be arithmetic child");
    check(width("!a*b")==4,"unary binds above multiplication");
    check(width("~a+b")==4,"bitwise complement applies to one unary operand");
    check(width("a**b+c")==8,"power propagates its base before outer addition");
    check(width("a+b**c")==4,"power base width enters outer addition");
    check(width("{!a,b}")==5,"concatenation sums inferred child widths");
    check(width("{2{!a,b}}")==10,"replication count multiplies concatenation width");
    check(!width("{unknown{a,b}}"),"unresolved replication count is unknown");
    check(width("unknown==a")==1,"comparison root width does not require leaf width");
    check(!width("unknown+a"),"arithmetic does require child widths");
    check(!width("f(a)+b"),"unresolved call result is unknown");
    check(width("!f(a)")==1,"logical result remains boolean for valid call subtree");
    check(!width("a<<b"),"unrecovered shift helper branch stays unknown");
    check(!width("a?b:c"),"unrecovered ternary-result branch stays unknown");
    check(!width("!a+"),"incomplete binary expression cannot inherit unary width");
    check(!width("!a garbage"),"trailing tokens cannot be silently ignored");
    check(!width("(a+b"),"unbalanced expression is unknown");
    check(!width(std::string(300,'!')+"a"),"nesting budget prevents unbounded recursion");
    std::cout<<checks<<" checks, "<<failures<<" failures\n";
    return failures?1:0;
}
