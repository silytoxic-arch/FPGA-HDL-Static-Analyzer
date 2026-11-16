#include "analysis/verilog_multiple_reset.h"

#include <iostream>
#include <string>

int main() {
    int failures = 0;
    const auto check = [&](const std::string& body, std::size_t expected, const char* why) {
        const fpga::core::SourceFile source("probe.v", fpga::core::HdlLanguage::Verilog,
            "module m(input CLK, ARST1, ARST2, D, output reg Q); " + body + " endmodule");
        const auto tokens = fpga::parser::tokenize(source);
        const auto actual = fpga::analysis::VerilogMultipleReset(tokens).violatingAlwaysIndices().size();
        if (actual != expected) {
            ++failures;
            std::cerr << "FAIL: " << why << " expected " << expected << " got " << actual << '\n';
        }
    };
    check("always @(posedge ARST1 or posedge ARST2 or posedge CLK) begin "
          "if (ARST1 || ARST2) Q <= 1'b1; else Q <= D; end", 1,
          "attachment combined reset guard");
    check("always @(posedge ARST1 or posedge ARST2 or posedge CLK) begin "
          "if (ARST1) Q <= 1'b1; else if (ARST2) Q <= 1'b1; else Q <= D; end", 1,
          "attachment chained reset guards");
    check("always @(posedge ARST1 or posedge CLK) begin "
          "if (ARST1) Q <= 1'b1; else Q <= D; end", 0,
          "one reset event remains clear");
    check("always @(posedge ARST1 or posedge ARST2 or posedge CLK) begin "
          "if (ARST1) Q <= 1'b1; else Q <= D; end", 0,
          "unclassified third edge is not guessed as a second reset");
    check("always @(posedge ARST1 or posedge ARST2 or posedge CLK) begin "
          "if (ARST1 || ARST2 || CLK) Q <= D; end", 0,
          "no distinct unguarded clock event remains");
    check("always @(posedge ARST1 or posedge ARST2 or posedge CLK) begin "
          "if (ARST1 || ARST2) $display(\"x\"); end", 0,
          "non-assignment body is not an inferred reset register");
    check("always @(posedge ARST1 or posedge CLK) begin if (ARST1) Q <= 0; end "
          "always @(posedge ARST2 or posedge CLK) begin if (ARST2) Q <= 0; end", 0,
          "separate processes do not combine reset counts");
    return failures == 0 ? 0 : 1;
}
