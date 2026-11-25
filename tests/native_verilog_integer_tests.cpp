#include "analysis/native_verilog_integer.h"

#include <iostream>
#include <string>
#include <vector>

// Known-value expectations cover context propagation and constant binding;
// verify_integer_folding_iverilog.py independently checks bits/width/cache.
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
        {"localparam [7:0] P=4'shf;", "P", 255, 8, false},
        {"localparam signed [3:0] P=4'hf;", "P", 15, 4, true},
        {"localparam [7:0] P=4'hf+4'h1;", "P", 16, 8, false},
        {"localparam P=4'hf+4'h1;", "P", 0, 4, false},
        {"localparam P=4'hf+4'h1;", "P+8'h0", 0, 8, false},
        {"localparam P=0,Q=P+1;", "Q", 1, 32, true},
        {"localparam integer P=-1;", "P", 0xffffffffULL, 32, true},
        {"localparam bit P=2;", "P", 0, 1, false},
        {"localparam signed P=4'hf;", "P", 15, 4, true},
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
    for (const auto& expression : {"1/0", "1%0", "2**3", "1<<-1", "65'h1+1", "1'bx+1", "'1+1", "'h1+1", "1?0:1", "{1'b0,1'b1}", "1+", "0 1", "$clog2(2)"}) {
        if (evaluate("", expression)) { ++failures; std::cerr << "FAIL unknown boundary: " << expression << '\n'; }
    }
    for (const auto& declaration : {"parameter P=0;", "parameter Q=0;localparam P=Q;",
             "localparam P=Q,Q=P;", "localparam P=MISSING;", "localparam real P=0;",
             "localparam [64:0] P=0;", "localparam [W-1:0] P=0;", "localparam P=1'bx;"}) {
        if (evaluate(declaration, "P")) { ++failures; std::cerr << "FAIL binding boundary: " << declaration << '\n'; }
    }
    return failures == 0 ? 0 : 1;
}
