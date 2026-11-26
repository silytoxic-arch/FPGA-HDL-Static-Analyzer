#include "analysis/native_verilog_integer.h"

#include <iostream>
#include <string>
#include <vector>

// Known-value expectations cover context propagation and constant binding;
// verify_integer_folding_iverilog.py independently checks bits/width.
// Diagnostic cache has separate fixed assertions below.
// The oracle is not an execution of the proprietary target.
int main(int argc, char** argv) {
    using fpga::analysis::detail::NativeVerilogInteger;
    struct Case {
        std::string declaration, expression;
        std::uint64_t bits;
        unsigned width;
        bool sign;
    };
    const std::vector<Case> cases{
        {"", "$clog2(0)", 0, 32, true},
        {"", "$clog2(1)", 0, 32, true},
        {"", "$clog2(2)", 1, 32, true},
        {"", "$clog2(3)", 2, 32, true},
        {"", "$clog2(4)", 2, 32, true},
        {"", "$clog2(5)", 3, 32, true},
        {"", "$clog2(1'sb1)", 0, 32, true},
        {"", "$clog2(4'sh8)", 3, 32, true},
        {"", "$clog2(8'shff)", 8, 32, true},
        {"", "$clog2(-1)", 32, 32, true},
        {"", "$clog2(64'hffffffffffffffff)", 64, 32, true},
        {"", "$clog2(64'h8000000000000000)", 63, 32, true},
        {"", "$clog2(64'sh8000000000000000)", 63, 32, true},
        {"", "$clog2(4'hf+4'h1)", 0, 32, true},
        {"", "$clog2(4'hf+4'h1)+64'h0", 0, 64, false},
        {"", "$clog2((4'hf+4'h1)+8'h0)", 4, 32, true},
        {"", "$clog2($clog2(17))", 3, 32, true},
        {"", "$clog2('0)", 0, 32, true},
        {"", "$clog2('1)", 0, 32, true},
        {"", "$clog2({8'hff,8'hff})", 16, 32, true},
        {"", "$clog2(1 ? 4'hf : 8'h0)", 4, 32, true},
        {"localparam N=17;", "$clog2(N)", 5, 32, true},
        {"localparam signed [0:0] P=1'b1;", "$clog2(P)", 0, 32, true},
        {"", "'0", 0, 1, false},
        {"", "'1", 1, 1, false},
        {"", "'0+8'h0", 0, 8, false},
        {"", "'1+8'h0", 1, 8, false},
        {"", "'1+0", 1, 32, false},
        {"", "'1+1", 2, 32, false},
        {"", "'1+'1", 0, 1, false},
        {"", "~'1", 0, 1, false},
        {"", "(~'1)+8'h0", 254, 8, false},
        {"", "'1==8'h1", 1, 1, false},
        {"", "'1==8'hff", 0, 1, false},
        {"", "'1<8'h2", 1, 1, false},
        {"", "'1>>>1", 0, 1, false},
        {"", "{'1,1'b0}", 2, 2, false},
        {"", "{4{'1}}", 15, 4, false},
        {"", "1 ? '1 : 8'h0", 1, 8, false},
        {"", "0 ? 8'h0 : '1", 1, 8, false},
        {"localparam [7:0] P='1;", "P", 1, 8, false},
        {"localparam [7:0] P='0;", "P", 0, 8, false},
        {"localparam signed [7:0] P='1;", "P", 1, 8, true},
        {"localparam signed [0:0] P='1;", "P", 1, 1, true},
        {"localparam P='1;", "P", 1, 1, false},
        {"localparam P='1;", "P+8'h0", 1, 8, false},
        {"localparam [7:0] P='1+1;", "P", 2, 8, false},
        {"", "1+2*3", 7, 32, true},
        {"", "4'hf+4'h1", 0, 4, false},
        {"", "(4'hf+4'h1)+8'h0", 16, 8, false},
        {"", "(4'shf+4'sh0)+8'h0", 15, 8, false},
        {"", "4'shf+4'sh0", 15, 4, true},
        {"", "-4'sd1", 15, 4, true},
        {"", "~4'h0", 15, 4, false},
        {"", "!0", 1, 1, false},
        {"", "&4'hf", 1, 1, false},
        {"", "~&4'hf", 0, 1, false},
        {"", "|4'h0", 0, 1, false},
        {"", "^4'h7", 1, 1, false},
        {"", "~^4'h7", 0, 1, false},
        {"", "4'sh8>>>1", 12, 4, true},
        {"", "4'h8>>>1", 4, 4, false},
        {"", "4'sh8>>1", 4, 4, true},
        {"", "(4'sh8>>>1)+8'h0", 4, 8, false},
        {"", "4'shf<8'h1", 0, 1, false},
        {"", "4'shf<4'sh1", 1, 1, false},
        {"", "64'hffffffffffffffff+64'h1", 0, 64, false},
        {"", "64'sh8000000000000000 / -64'sd1", 0x8000000000000000ULL, 64, true},
        {"", "8'shff/8'sd2", 0, 8, true},
        {"", "8'shfb%8'sd2", 255, 8, true},
        {"", "8'h80<<8", 0, 8, false},
        {"", "8'sh80>>>8", 255, 8, true},
        {"", "8'h1<<64'hffffffffffffffff", 0, 8, false},
        {"", "2147483648+0", 0x80000000ULL, 33, true},
        {"", "4294967295+1", 0x100000000ULL, 33, true},
        {"", "4'hf==4'hf", 1, 1, false},
        {"", "4'hf!=4'hf", 0, 1, false},
        {"", "4'shf==8'shff", 1, 1, false},
        {"", "4'shf==8'hff", 0, 1, false},
        {"", "4'hf ^~ 4'h0", 0, 4, false},
        {"", "0||8'hff", 1, 1, false},
        {"", "0&&8'hff", 0, 1, false},
        {"", "8'h0 + -1", 0xffffffffULL, 32, false},
        {"", "2**3", 8, 32, true},
        {"", "2**3**2", 64, 32, true},
        {"", "2**(3**2)", 512, 32, true},
        {"", "-2**2", 4, 32, true},
        {"", "-(2**2)", 0xfffffffcULL, 32, true},
        {"", "4'h2**4'h4", 0, 4, false},
        {"", "(4'h2**4'h4)+8'h0", 16, 8, false},
        {"", "4'sh8**1'h1", 8, 4, true},
        {"", "(4'sh8**1'h1)>>>1", 12, 4, true},
        {"", "4'sh8**1'sh1", 0, 4, true},
        {"", "8'shff**-2", 1, 8, true},
        {"", "8'shff**-3", 255, 8, true},
        {"", "8'shfe**-2", 0, 8, true},
        {"", "8'hff**-3", 0, 8, false},
        {"", "0**0", 1, 32, true},
        {"", "1**-1", 1, 32, true},
        {"", "64'h3**64'hffffffffffffffff", 0xaaaaaaaaaaaaaaabULL, 64, false},
        {"localparam [7:0] P=4'h2**4'h4;", "P", 16, 8, false},
        {"localparam P=4'h2**4'h4;", "P", 0, 4, false},
        {"", "{4'h1,4'h2}", 0x12, 8, false},
        {"", "{2{4'ha}}", 0xaa, 8, false},
        {"", "{{4'h1,4'h2},4'h3}", 0x123, 12, false},
        {"", "{4'shf,4'sh8}", 0xf8, 8, false},
        {"", "{4'hf+4'h1,4'h1}", 1, 8, false},
        {"", "({4'hf+4'h1,4'h1})+16'h0", 1, 16, false},
        {"", "{2{4'hf+4'h1}}", 0, 8, false},
        {"", "{1{4'shf}}", 15, 4, false},
        {"", "{2{{2{4'h1}}}}", 0x1111, 16, false},
        {"", "{64{1'b1}}", 0xffffffffffffffffULL, 64, false},
        {"", "{1{64'h8000000000000000}}", 0x8000000000000000ULL, 64, false},
        {"", "{1'b1}", 1, 1, false},
        {"", "{2{4'ha,4'hb}}", 0xabab, 16, false},
        {"", "{(1+1){4'h1}}", 0x11, 8, false},
        {"", "{4'h1 << 1,4'h2}", 0x22, 8, false},
        {"", "{8'shff}+16'sh0", 255, 16, false},
        {"localparam N=2;", "{N{4'h1}}", 0x11, 8, false},
        {"localparam signed [7:0] P={4'hf,4'hf};", "P", 255, 8, true},
        {"localparam [3:0] P={4'h1,4'h2};", "P", 2, 4, false},
        {"localparam [15:0] P={4'hf+4'h1,4'h1};", "P", 1, 16, false},
        {"", "1 ? 4'hf : 8'h0", 15, 8, false},
        {"", "1 ? 4'shf : 8'sh0", 255, 8, true},
        {"", "1 ? 4'shf : 8'h0", 15, 8, false},
        {"", "0 ? 4'h0 : 8'shff", 255, 8, false},
        {"", "0 ? 8'sh0 : 4'shf", 255, 8, true},
        {"", "(1 ? 4'hf+4'h1 : 4'h0)+8'h0", 16, 8, false},
        {"", "1 ? 4'hf+4'h1 : 8'h0", 16, 8, false},
        {"", "1 ? 0 ? 1 : 2 : 3", 2, 32, true},
        {"", "0 ? 1 : 0 ? 2 : 3", 3, 32, true},
        {"", "64'h100000000 ? 0 : 1", 0, 32, true},
        {"", "0 ? 8'h0 : 4'h8<<1", 16, 8, false},
        {"", "1 ? 4'sh8>>>1 : 8'h0", 4, 8, false},
        {"", "1 ? 4'sh8>>>1 : 8'sh0", 252, 8, true},
        {"", "(1 ? 4'shf : 4'sh0)<8'h1", 0, 1, false},
        {"", "{1 ? 4'hf+4'h1 : 4'h0,4'h1}", 1, 8, false},
        {"", "1 ? 8'h1 : 4'h0/4'h0", 1, 8, false},
        {"", "1 ? 1 : 0**-1", 1, 32, true},
        {"localparam signed [7:0] P=1 ? 4'shf : 4'sh0;", "P", 255, 8, true},
        {"localparam [7:0] P=1 ? 4'hf+4'h1 : 4'h0;", "P", 16, 8, false},
        {"localparam K=64'h100000000;", "K ? 1 : 0", 1, 32, true},
        {"", "'hff+1", 256, 32, false},
        {"", "'b10+1", 3, 32, false},
        {"", "'o17+1", 16, 32, false},
        {"", "'d255+1", 256, 32, false},
        {"", "'shff+1", 256, 32, true},
        {"", "-'h1", 0xffffffffULL, 32, false},
        {"", "~'h0", 0xffffffffULL, 32, false},
        {"", "'h1<<31", 0x80000000ULL, 32, false},
        {"", "'h1<<32", 0, 32, false},
        {"", "'sh1+4'shf", 0, 32, true},
        {"", "'h1+4'shf", 16, 32, false},
        {"", "'sh80000000+0", 0x80000000ULL, 33, true},
        {"", "'sh80000000>>>1", 0x40000000ULL, 33, true},
        {"", "'h0000000001+0", 1, 40, false},
        {"", "'o17777777777+0", 0x7fffffffULL, 33, false},
        {"", "'hffffffffffffffff+64'h1", 0, 64, false},
        {"", "'shffffffff>>>31", 1, 33, true},
        {"", "'sh80000000<0", 0, 1, false},
        {"localparam [63:0] P='h80000000;", "P", 0x80000000ULL, 64, false},
        {"localparam P='shffffffff;", "P", 0xffffffffULL, 33, true},
        {"localparam [7:0] P='h1;", "P", 1, 8, false},
        {"", "'sd255+1", 256, 32, true},
        {"localparam signed P=4'hf;", "P", 15, 4, false},
        {"localparam signed P=4'hf;", "+P", 15, 4, true},
        {"localparam signed P=4'hf;", "P+4'sh0", 15, 4, true},
        {"localparam signed P=4'hf;", "P<4'sh0", 0, 1, false},
        {"localparam signed P=4'hf;", "(P+4'sh0)<4'sh0", 1, 1, false},
        {"localparam signed P=4'hf;", "P/4'sh2", 0, 4, true},
        {"localparam signed P=4'hf;", "P>>>1", 15, 4, true},
        {"localparam signed P=4'hf;", "1 ? P : 4'sh0", 15, 4, true},
        {"localparam signed P=4'hf;", "{P,4'h0}", 240, 8, false},
        {"localparam P=4'shf;", "P", 15, 4, true},
        {"localparam P=4'shf;", "P/4'sh2", 7, 4, false},
        {"localparam P=4'shf;", "P>>>1", 7, 4, false},
        {"localparam P=4'shf;", "P<4'sh0", 1, 1, false},
        {"localparam P=4'shf;", "(P+4'sh0)<4'sh0", 0, 1, false},
        {"localparam P=4'shf;", "1 ? P : 4'sh0", 15, 4, false},
        {"localparam signed P=4'hf;localparam Q=P;", "Q", 15, 4, false},
        {"localparam signed P=4'hf;localparam Q=P;", "Q/4'sh2", 7, 4, false},
        {"localparam signed P=1'b1;", "P", 1, 1, false},
        {"localparam signed P=1'b1;", "P<1'sb0", 0, 1, false},
        {"localparam P=1'sb1;", "P+1'sb0", 1, 1, false},
        {"localparam [7:0] P=4'shf;", "P", 255, 8, false},
        {"localparam signed [3:0] P=4'hf;", "P", 15, 4, true},
        {"localparam [7:0] P=4'hf+4'h1;", "P", 16, 8, false},
        {"localparam P=4'hf+4'h1;", "P", 0, 4, false},
        {"localparam P=4'hf+4'h1;", "P+8'h0", 0, 8, false},
        {"localparam P=0,Q=P+1;", "Q", 1, 32, false},
        {"localparam integer P=-1;", "P", 0xffffffffULL, 32, true},
        {"localparam bit P=2;", "P", 0, 1, false},
        {"localparam unsigned P=4'hf;", "P", 15, 4, false},
        {"localparam [0:7] P=-1;", "P", 255, 8, false},
        {"localparam [-1:-8] P=-1;", "P", 255, 8, false},
    };
    const bool dump = argc > 1 && std::string(argv[1]) == "--dump";
    int failures = 0;
    const auto evaluate = [](const std::string& declaration, const std::string& expression) {
        const fpga::core::SourceFile source("fold.sv", fpga::core::HdlLanguage::SystemVerilog,
            "module m;" + declaration + " wire [63:0] result; assign result=" + expression + "; endmodule");
        const auto tokens = fpga::parser::tokenize(source);
        const fpga::analysis::VerilogConstantExpressions symbols(tokens);
        NativeVerilogInteger model(tokens, symbols);
        for (std::size_t at = 0; at + 3 < tokens.size(); ++at) {
            if (tokens[at].text != "assign") continue;
            auto end = at + 3;
            while (end < tokens.size() && tokens[end].text != ";") ++end;
            return model.evaluate(at + 3, end);
        }
        return std::optional<NativeVerilogInteger::Value>{};
    };
    for (const auto& test : cases) {
        const auto value = evaluate(test.declaration, test.expression);
        if (!value || value->bits != test.bits || value->width != test.width || value->signedValue != test.sign) {
            ++failures;
            std::cerr << "FAIL integer fold: " << test.declaration << ' ' << test.expression;
            if (value) std::cerr << " got " << value->width << ':' << value->bits << ':' << value->signedValue;
            std::cerr << '\n';
        }
        if (dump && value) std::cout << test.declaration << '\t' << test.expression << '\t'
            << value->width << '\t' << value->bits << '\t' << value->cache() << '\t'
            << value->signedValue << '\n';
    }
    for (const auto& expression : {"1/0", "1%0", "0**-1", "4'h0**4'shf", "1<<-1", "65'h1+1", "1'bx+1", "'x+1", "'z+1", "'1x", "' 1", "'h10000000000000000+1", "'shffffffffffffffff+0", "'d18446744073709551616+0", "1?0", "1?:0", "1?0:", "1'bx ? 0:1", "MISSING ? 0:1", "0 ? 0:1/0", "{}", "{1'b0,}", "{,1'b0}", "{1'bx,1'b0}", "{65{1'b1}}", "{64'h0,1'b0}", "{0{1'b1}}", "{-1{1'b1}}", "{1{1'b1}", "{1'b0", "1+", "0 1", "$clog2()", "$clog2(1,2)", "$clog2(2", "$clog2(1/0)", "$clog2(1'bx)", "$clog2(65'h1)", "$clog2(MISSING)", "$clog2($signed(2))", "$signed(2)", "$unsigned(2)", "$CLOG2(2)"}) {
        if (evaluate("", expression)) { ++failures; std::cerr << "FAIL unknown boundary: " << expression << '\n'; }
    }
    for (const auto& declaration : {"parameter P=0;", "parameter Q=0;localparam P=Q;",
             "localparam P=Q,Q=P;", "localparam P=MISSING;", "localparam real P=0;",
             "localparam [64:0] P=0;", "localparam [W-1:0] P=0;", "localparam P=1'bx;",
             "localparam unsigned P=4'shf;",
             "localparam P=$clog2(17);", "localparam [7:0] P=$clog2(4'hf+4'h1);",
             "localparam P=$clog2(2);localparam Q=$clog2(P+1);",
             "parameter N=2;localparam P={N{1'b1}};"}) {
        if (evaluate(declaration, "P")) { ++failures; std::cerr << "FAIL binding boundary: " << declaration << '\n'; }
    }
    struct CacheCase { const char* declaration; const char* expression; std::uint32_t cache; };
    for (const auto& test : std::vector<CacheCase>{
             {"localparam [0:0] P=1'sb1;", "$clog2(P)", 0},
             {"localparam [32:0] P=33'h100000000;", "$clog2(P)", 32},
             {"localparam signed [0:0] P='1;", "P", 1},
             {"localparam signed [0:0] P='1;", "+P", 0xffffffffU},
             {"localparam [7:0] P='1;", "P", 1},
             {"localparam signed [7:0] P='1;", "P", 1},
             {"localparam [0:0] P=1'sb1;", "P", 0xffffffffU},
             {"localparam signed [0:0] P=1'b1;", "P", 1},
             {"localparam [0:0] P=1'sb1;", "P+1'b0", 1},
             {"localparam signed [0:0] P=1'b1;", "+P", 0xffffffffU},
             {"localparam signed [0:0] P=1'b1;", "1 ? P : 1'sb0", 0xffffffffU},
             {"localparam signed [3:0] P=4'hf;", "P", 15},
             {"localparam [7:0] P=4'shf;", "P", 255},
             {"localparam [7:0] P=4'shf+4'sh0;", "P", 0xffffffffU},
             {"localparam [0:0] P=1'sb1;localparam [0:0] Q=P;", "Q", 0xffffffffU},
             {"localparam [0:0] P=1'sb1;localparam [1:0] Q=P;", "Q", 1},
             {"localparam signed P=1'b1;", "P", 1},
             {"localparam signed P=1'b1;", "P+1'sb0", 0xffffffffU},
             {"localparam signed P=1'b1;", "P<1'sb0", 0},
             {"localparam P=1'sb1;", "P", 0xffffffffU},
             {"localparam P=1'sb1;", "P+1'sb0", 1},
             {"localparam P=1'sb1;", "P<1'sb0", 1}}) {
        const auto value = evaluate(test.declaration, test.expression);
        if (!value || value->cache() != test.cache) {
            ++failures;
            std::cerr << "FAIL native stored cache: " << test.declaration << ' ' << test.expression << '\n';
        }
    }
    return failures == 0 ? 0 : 1;
}
