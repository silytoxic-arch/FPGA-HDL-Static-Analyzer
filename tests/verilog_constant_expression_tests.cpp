#include "analysis/verilog_constant_expression.h"

#include <iostream>
#include <string>
#include <vector>

// Contract tests for conservative scope-aware parameter classification. These
// do not assert equivalence to the vendor's complete elaboration algorithm.
int main() {
    int failures = 0;
    const auto check = [&](const std::string& text, const std::vector<bool>& expected,
                           const char* description) {
        fpga::core::SourceFile source("probe.sv", fpga::core::HdlLanguage::SystemVerilog, text);
        const auto tokens = fpga::parser::tokenize(source);
        const fpga::analysis::VerilogConstantExpressions constants(tokens);
        std::vector<bool> actual;
        for (std::size_t i = 0; i + 1 < tokens.size(); ++i) {
            if (tokens[i].text != "case" || tokens[i + 1].text != "(") continue;
            int depth = 1;
            auto j = i + 2;
            for (; j < tokens.size(); ++j) {
                if (tokens[j].text == "(") ++depth;
                if (tokens[j].text == ")" && --depth == 0) break;
            }
            actual.push_back(constants.isConstant(i + 2, j));
        }
        if (actual != expected) {
            ++failures;
            std::cerr << "FAIL: " << description << " actual:";
            for (const auto value : actual) std::cerr << ' ' << value;
            std::cerr << '\n';
        }
    };
    check("module m; parameter P=2; localparam Q=P+1; case(Q) endcase endmodule",
          {true}, "transitive localparam reference");
    check("module m #(parameter integer P=2, Q=P+1, parameter R=Q*2) ();"
          "case(P) endcase case(Q) endcase case(R) endcase endmodule", {true,true,true},
          "parameter port list and typed/repeated declarations");
    check("module m; localparam P=Q+1, Q=3; case(P) endcase endmodule", {true},
          "forward parameter references");
    check("module m; localparam P=Q, Q=P; case(P) endcase endmodule", {false},
          "cyclic parameter dependency is unproved");
    check("module m; localparam P=unknown+1; case(P) endcase endmodule", {false},
          "unresolved initializer is unproved");
    check("module m(input logic signal); parameter P=1;"
          "case(P+signal) endcase case(signal) endcase endmodule", {false,false},
          "signals do not become constants");
    check("module m; parameter P=1; case(P) endcase endmodule "
          "module n(input P); case(P) endcase endmodule", {true,false},
          "same-named module signal does not inherit parameter");
    check("module m; parameter P=1; function int f(input int P);"
          "case(P) endcase endfunction case(P) endcase endmodule", {false,true},
          "function argument shadows only its scope");
    check("module m; parameter P=1; begin : local_block integer P; case(P) endcase end "
          "case(P) endcase endmodule", {false,true}, "named block variable shadows parameter");
    check("module m; parameter P=1; begin localparam P=unknown; case(P) endcase end "
          "case(P) endcase endmodule", {false,true}, "unresolved inner parameter blocks outer lookup");
    check("module m; parameter P=1; begin localparam P=3; case(P+1) endcase end "
          "case(P) endcase endmodule", {true,true}, "inner numeric parameter");
    check("module m; parameter P=1; case(other.P) endcase case(user_call(P)) endcase "
          "case($random) endcase endmodule", {false,false,false},
          "hierarchy, arbitrary calls, impure system calls remain unproved");
    check("module m; parameter N=8; localparam P=$clog2(N); case(P) endcase "
          "case({N, 2}) endcase case(N[2:0]) endcase endmodule", {true,true,true},
          "bounded pure builtin, constant concatenation and selection");
    check("module m; localparam type T=int; case(T) endcase endmodule", {false},
          "type parameter is not numeric");
    check("module m; parameter P=1; logic P; case(P) endcase endmodule", {false},
          "conflicting declarations do not prove a constant");
    check("module m; typedef logic [7:0] data_t; parameter P=1; begin : block_name "
          "data_t P; case(P) endcase end case(P) endcase endmodule", {false,true},
          "typedef variable shadows only its block");
    check("package constants; parameter P=1; endpackage module m; case(P) endcase endmodule",
          {false}, "package names do not leak into modules without explicit resolution");
    check("module m; parameter P=1; function int P(); case(P) endcase endfunction "
          "case(P) endcase endmodule", {false,true}, "implicit function result shadow");
    check("module m; localparam P; case(P) endcase case(1+2) endcase "
          "case(\"literal\") endcase endmodule", {false,true,false},
          "missing initializer, literal expression and string boundary");
    return failures == 0 ? 0 : 1;
}
