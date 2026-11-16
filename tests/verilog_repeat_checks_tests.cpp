#include "analysis/verilog_repeat_checks.h"

#include <iostream>
#include <string>
#include <vector>

// Independent tests for partial repeat-count variable detection. These do not
// execute the vendor tool or claim equivalence to its elaboration phases.
int main() {
    int failures = 0;
    const auto check = [&](const std::string& text, std::size_t expected,
                           const char* description) {
        fpga::core::SourceFile source("repeat_probe.sv", fpga::core::HdlLanguage::SystemVerilog, text);
        const auto tokens = fpga::parser::tokenize(source);
        const fpga::analysis::VerilogConstantExpressions constants(tokens);
        const auto actual = fpga::analysis::verilogRepeatRuntimeDependencies(source, tokens, constants);
        if (actual.size() != expected) {
            ++failures;
            std::cerr << "FAIL: " << description << " expected " << expected
                      << " actual " << actual.size() << '\n';
        }
        for (const auto index : actual) {
            if (tokens[index].kind != fpga::parser::HdlTokenKind::Identifier ||
                tokens[index].text == "repeat") {
                ++failures;
                std::cerr << "FAIL: diagnostic anchor is not a runtime identifier\n";
            }
        }
    };
    check("module m(input [1:0] B1, input A, B, output reg C);"
          "always @(posedge A) repeat(B1) C <= B || A; endmodule", 1,
          "vendor HTML B1 variable count");
    check("module m(input A, B, output reg C); repeat(5) C <= B || A; endmodule", 0,
          "vendor HTML literal count");
    check("module m(input data_in, output reg data_out);"
          "always @(*) repeat(data_in) data_out=1; endmodule", 1,
          "vendor HTML data_in variable count");
    check("module m(input data_in, output reg data_out); parameter XX=10;"
          "always @(*) repeat(XX) data_out=1; endmodule", 0,
          "vendor HTML parameter count");
    check("module m(input logic count); parameter P=4; localparam Q=P+2;"
          "repeat(Q*2) ; repeat(2+3) ; repeat(Q+count) ; endmodule", 1,
          "parameter arithmetic retained and full expression variable detected");
    check("module m(input int signal); localparam P=signal+1; repeat(P) ; endmodule", 0,
          "native rule accepts a declared localparam name without rechecking its initializer");
    check("module m(input logic index); parameter P=12; repeat(P[index]) ; endmodule", 1,
          "nested parameter selection variable is scanned");
    check("module m(input int count); repeat(user_function((count+1))) ; endmodule", 1,
          "unknown function argument variable remains detectable");
    check("module m; repeat(user_function(5)) ; repeat(pkg::P) ; repeat(unknown) ;"
          "repeat(`COUNT) ; endmodule", 1,
          "an unknown direct identifier is absent from the native parameter-name table");
    check("module m; parameter P=3; repeat(P) ; endmodule "
          "module n(input int P); repeat(P) ; endmodule", 1,
          "no cross-module parameter name whitelist leakage");
    check("module m; parameter P=3; begin : b integer P; repeat(P) ; end "
          "repeat(P) ; endmodule", 0, "native flat parameter table ignores block shadowing");
    check("module m; parameter P=3; function int f(input int P); repeat(P) ;"
          "endfunction repeat(P) ; endmodule", 0, "native flat parameter table ignores argument shadowing");
    check("module m(input int P); begin : b localparam P=2; repeat(P) ; end "
          "repeat(P) ; endmodule", 0,
          "a nested localparam name enters the owning module group's flat table");
    check("module m; localparam P=Q, Q=P; repeat(P) ; endmodule", 0,
          "cyclic unknown parameters are not runtime proof");
    check("module m(input int sig); repeat((2+(sig*3))) ; repeat(2) repeat(sig) ;"
          "endmodule", 2, "nested parentheses and nested repeats");
    check("module m(input int a,b); repeat(a+b+a) ; endmodule", 3,
          "native per-identifier multiplicity is preserved within one repeat count");
    check("module m(input int sig); // repeat(sig)\n"
          "initial $display(\"repeat(sig)\"); /* repeat(sig) */ endmodule", 0,
          "comments and strings are excluded");
    check("module m(input int sig); wire \\repeat ; assign \\repeat =sig; endmodule", 0,
          "escaped repeat identifier is not a keyword");
    check("module m(input int sig); repeat(sig; endmodule", 0,
          "unbalanced repeat syntax is not a proven count");
    check("module m(input int P); repeat(`P) ; endmodule", 0,
          "macro name is not a variable even if a declaration matches");
    check("module m; function int f(); repeat(f()) ; endfunction endmodule", 0,
          "arbitrary call name is not its implicit function return variable");
    return failures == 0 ? 0 : 1;
}
