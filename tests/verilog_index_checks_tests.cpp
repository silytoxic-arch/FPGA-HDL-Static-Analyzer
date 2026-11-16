#include "analysis/verilog_index_checks.h"

#include <iostream>
#include <string>

int main() {
    int failures = 0;
    const auto check = [&](const std::string& text, std::size_t expected, const char* label) {
        fpga::core::SourceFile source("index_probe.sv", fpga::core::HdlLanguage::SystemVerilog, text);
        const auto tokens = fpga::parser::tokenize(source);
        const auto actual = fpga::analysis::VerilogIndexChecks(source, tokens).complexIndexIndices();
        if (actual.size() != expected) {
            ++failures;
            std::cerr << "FAIL: " << label << " expected " << expected
                      << " actual " << actual.size() << '\n';
        }
        for (const auto index : actual) if (tokens[index].text != "[") {
            ++failures; std::cerr << "FAIL: index anchor\n";
        }
    };
    const std::string prefix = "module m #(parameter P_ARR_SIZE=5, P_BUS1_SIZE=4)"
        "(input CLK, RST, input [P_BUS1_SIZE-1:0] A1, output reg [P_BUS1_SIZE-1:0] C1);"
        "wire [P_BUS1_SIZE-1:0] B1 [P_ARR_SIZE-1:0];";
    check(prefix + "always @(posedge CLK) C1 <= B1[A1*2-1]; endmodule", 1,
          "vendor HTML violation and declaration ranges excluded");
    check(prefix + "always @(posedge CLK) C1 <= B1[A1]; endmodule", 0,
          "vendor HTML compliant simple signal index");
    check("module m; wire [W-1:0] a[N-1:0], b[M+1:0]; reg [W*2:0] q[K/2:0];"
          "logic [Q+4:0] r[L-1:0]; endmodule", 0, "packed and unpacked variable declarations");
    check("module m(input logic [W-1:0] a[N+1:0], output reg [W*2:0] b);"
          "input [Q-1:0] c; output [R+1:0] d; inout [S*2:0] e; endmodule", 0,
          "ANSI and legacy port declarations");
    check("module m #(parameter [W-1:0] P=0, parameter int Q[N-1:0]='{default:0});"
          "localparam [R+1:0] K=0; endmodule", 0, "parameter packed and unpacked dimensions");
    check("module m; typedef logic [W-1:0] word_t; word_t a[N-1:0];"
          "typedef struct packed { logic [W-1:0] q; } record_t; record_t b[M*2:0];"
          "typedef enum logic [E+1:0] { A,B } state_t; endmodule", 0,
          "typedef aliases structs enums and typed declarations");
    check("module m; function automatic logic [W-1:0] f(input logic [W-1:0] p[N+1:0]);"
          "f=p[i+1]; endfunction task t(input logic [W-1:0] p[N+1:0]);"
          "q=p[j*2]; endtask endmodule", 2, "function return dimensions and formal dimensions excluded");
    check("module m; assign q=a[1]; assign q=a[i]; assign q=a[(i)]; assign q=a[3:0];"
          "assign q=a[HI:LO]; endmodule", 0, "simple number identifier parentheses and slice roots");
    check("module m; assign q=a[i+1]; assign q=a[-1]; assign q=a[2*3];"
          "assign q=a[f(i)]; assign q=a[{i,j}]; assign q=a[i?j:k]; endmodule", 6,
          "arithmetic unary call concatenate ternary complexity");
    check("module m; wire [W-1:0] q=a[i+1], r=a[j*2]; localparam P=a[k+1];"
          "endmodule", 3, "declaration initializers are expression positions");
    check("module m; assign a[i+1]=b[j+2]; assign q=a[i+1][j];"
          "assign q=a[i][j+1]; endmodule", 4, "read write and multidimensional selections");
    check("module m; assign q=a[b[i+1]]; assign q=a[b[i]+1]; endmodule", 2,
          "nested selected identifier keeps simple outer root without duplicate");
    check("module m; assign q=a[hi+1:lo]; assign q=a[hi:lo-1]; endmodule", 2,
          "complex ordinary slice bounds");
    check("module m; assign q=a[i+:4]; assign q=a[i-:4]; assign q=a[`INDEX+1];"
          "assign q=a[pkg::P+1]; assign q=a[obj.field+1]; endmodule", 0,
          "indexed part-select macro and qualified expressions remain unproved");
    check("module m; initial $display(\"a[i+1]\"); // a[i+1]\n"
          "/* a[i+1] */ endmodule", 0, "comments and string contents excluded");
    check("module m; assign q=a[i+1; endmodule", 0, "unclosed index is not a proof");
    check("module m; initial q=new[N+1]; endmodule", 0, "dynamic allocation size is not selection");
    return failures == 0 ? 0 : 1;
}
