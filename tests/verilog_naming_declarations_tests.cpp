#include "analysis/verilog_naming_declarations.h"

#include <iostream>
#include <string>
#include <vector>

int main() {
    using namespace fpga;
    using namespace analysis;
    int failures = 0;
    const auto check = [&](const std::string& text, const std::vector<std::string>& expected,
                           const std::vector<std::string>& bad, const char* label) {
        core::SourceFile source("probe.sv", core::HdlLanguage::SystemVerilog, text);
        const auto tokens = parser::tokenize(source);
        const auto declarations = VerilogNamingDeclarations(source, tokens).collect();
        std::vector<std::string> names, actualBad;
        for (const auto& decl : declarations) {
            names.push_back(decl.normalizedName);
            if (checkVerilogNamingStyle(decl.normalizedName, decl.subject) == true)
                actualBad.push_back(decl.normalizedName);
            if (decl.tokenIndex >= tokens.size()) ++failures;
        }
        if (names != expected || actualBad != bad) {
            ++failures; std::cerr << "FAIL: " << label << " names:";
            for (const auto& name : names) std::cerr << ' ' << name;
            std::cerr << " bad:";
            for (const auto& name : actualBad) std::cerr << ' ' << name;
            std::cerr << '\n';
        }
    };
    check("module m(input clk,input RST,output reg OUT); parameter data=32;"
          "always @(posedge clk) OUT<=data; endmodule", {"clk","RST","OUT","data"},
          {"RST","OUT","data"}, "attachment negative names only once at declaration");
    check("module m(input clk,input rst,output reg out); parameter Data=32; endmodule",
          {"clk","rst","out","Data"}, {"Data"}, "HTML positive Data conflicts with native default");
    check("module m #(parameter integer WIDTH=8, Data=3, parameter logic [2:0] CODE=1) "
          "(input wire [WIDTH-1:0] DATA, clk, output logic Done); localparam L=1, lower=2; endmodule",
          {"WIDTH","Data","CODE","DATA","clk","Done","L","lower"},
          {"Data","DATA","lower"}, "parameter ports, repeated kind and inherited port qualifiers");
    check("module m(a,b); input a,b; output reg BAD; wire okay=func(1,2), Out; logic dataOut; endmodule",
          {"a","b","BAD","okay","Out","dataOut"}, {"BAD","dataOut"},
          "non-ANSI ports and initializer comma nesting");
    check("module m; wire [7:0] GOOD[0:7], BAD, memory[2]; reg MEM[2]; logic okay; endmodule",
          {"BAD","okay"}, {"BAD"}, "packed vector accepted, each unpacked array excluded");
    check("module m; parameter P[2]='{1,2}, Q=2; localparam type T=int; "
          "parameter custom_type BAD=1, ALSO=2; wire okay; endmodule", {"Q","okay"}, {},
          "parameter arrays, type parameter and unknown parameter type excluded");
    check("module m; function int F(input logic BAD); localparam bad=1; logic OUT; "
          "endfunction task T(input BAD); wire BAD; endtask class C; logic BAD; "
          "function void foo; parameter bad=1; endfunction endclass logic okay; endmodule",
          {"okay"}, {}, "function task class subtrees excluded");
    check("module m; typedef logic [7:0] TypeName; typedef struct packed {logic BAD;} T; "
          "T BAD; struct packed {logic BAD;} STRUCT; union packed {logic BAD;} UNION;"
          "custom_type ALSO; bit BITS; integer INT; genvar GEN; wire good; endmodule",
          {"good"}, {}, "typedef bodies, opaque types and unsupported categories excluded");
    check("package p; parameter lower=1; logic BAD; endpackage "
          "interface i; logic BAD; endinterface module m; wire okay; endmodule "
          "module n; localparam lower=1; endmodule", {"okay","lower"}, {"lower"},
          "only explicit module bodies collected");
    check("module m; wire \\wire , \\function , \\module , \\Data ; parameter \\DATA =1; "
          "\\wire NotADeclaration; logic good; endmodule",
          {"wire","function","module","Data","DATA","good"}, {},
          "escaped keyword identifiers cannot change declaration/scope category");
    check("module m;\n`define DECL wire BAD;\n`define CONT \\\n logic BAD;\n"
          "wire okay; wire `TYPE BAD, OTHER; logic good; endmodule",
          {"okay","good"}, {}, "macro definition/continuation and macro declarations excluded");
    check("module m;\n`ifdef ENABLE\nlogic BAD;\n`ifndef NESTED\nparameter lower=1;\n"
          "`endif\n`else\nwire OTHER;\n`endif\nwire good; endmodule", {"good"}, {},
          "unresolved preprocessor conditional declarations excluded in both branches");
    check("module m(input custom_type BAD, input logic okay, output struct packed {logic BAD;} OUT); "
          "logic good; endmodule", {"okay","good"}, {}, "unknown port types do not leak nested members");
    check("module m; generate if(1) begin : b logic BAD; localparam lower=1; end endgenerate "
          "always_comb begin logic Inner; end endmodule", {"BAD","lower","Inner"}, {"BAD","lower"},
          "explicit generated and procedural block declarations outside functions");
    check("module m; initial $display(\"wire BAD; parameter lower=1\"); "
          "/* wire BAD; */ // parameter lower=1\nlogic good; endmodule", {"good"}, {},
          "comments strings and references cannot create declarations");
    check("module m; wire \\\\DATA ; wire \\\\Data ; endmodule", {"DATA","Data"}, {"DATA"},
          "remaining literal leading slashes normalize as observed native predicate input");
    return failures == 0 ? 0 : 1;
}
