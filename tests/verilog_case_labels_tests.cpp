#include "analysis/verilog_case_labels.h"

#include <iostream>
#include <string>
#include <vector>

int main() {
    int failures = 0;
    const auto check = [&](const std::string& body, const std::vector<std::string>& expected,
                           const char* description) {
        fpga::core::SourceFile source("probe.sv", fpga::core::HdlLanguage::SystemVerilog,
            "module m(input logic [7:0] sel, dsf, flag, output logic [7:0] q); "
            "parameter P=3; localparam Q=P+1; " + body + " endmodule");
        const auto tokens = fpga::parser::tokenize(source);
        const fpga::analysis::VerilogConstantExpressions constants(tokens);
        const auto positions = fpga::analysis::VerilogCaseLabels(tokens, constants).runtimeLabelIndices();
        std::vector<std::string> actual;
        for (const auto index : positions) actual.push_back(tokens[index].text);
        if (actual != expected) {
            ++failures; std::cerr << "FAIL: " << description << " actual:";
            for (const auto& name : actual) std::cerr << ' ' << name;
            std::cerr << '\n';
        }
    };
    check("case(sel) 2'b00:q=0; 2'b01:q=1; dsf:q=2; default:q=0; endcase", {"dsf"},
          "attachment negative case label, not control or body");
    check("case(sel) 2'b00:q=dsf+1; 2'b01:q=sel; default:q=flag; endcase", {},
          "attachment-style numeric labels allow variable/operation bodies");
    check("case(sel) P:q=0; Q:q=1; 1+2:q=3; default:q=0; endcase", {},
          "constant-folding ambiguity is left for manual review");
    check("case(sel) dsf+1:q=0; (flag && dsf):q=1; endcase", {"dsf", "("},
          "known variable arithmetic and logic labels");
    check("case(sel) 0,1,dsf:q=0; 2,3:q=1; endcase", {},
          "native consumer ignores runtime choices after a constant first child");
    check("case(sel) {1,2},dsf:q=0; 3:q=1; endcase", {},
          "concatenation first child remains constant and later choice is ignored");
    check("case(sel) 0,flag,dsf:q=0; endcase", {},
          "later runtime choices cannot create a native branch diagnostic");
    check("case(sel) flag,0,dsf:q=0; endcase", {"flag"},
          "runtime first child creates exactly one diagnostic for the item");
    check("case(sel) flag ? 1 : 2:q=0; 3:q=flag ? dsf : sel; dsf:q=1; endcase",
          {"flag", "dsf"}, "ternary colon in label/body is not label terminator");
    check("case(sel) (flag ? (dsf ? 1 : 2) : 3):q=0; dsf[3:0]:q=1; endcase",
          {"(", "dsf"}, "nested ternary and part-select label colons");
    check("case(sel) 0: begin case(dsf) 1:q=sel; flag:q=0; endcase q=dsf; end "
          "dsf: q=1; endcase", {"dsf", "flag"},
          "nested case analyzed separately, variable body is skipped");
    check("case(sel) 0: if(flag) begin q=dsf; end else q=sel; dsf:q=1; endcase", {"dsf"},
          "if/else body doesn't hide following label");
    check("case(sel) 0: for(int i=0;i<3;i=i+1) q=dsf; flag:q=1; endcase", {"flag"},
          "for semicolons are not item boundaries");
    check("case(sel) 0: repeat(2) begin q=dsf; end flag:q=1; endcase", {"flag"},
          "repeat block body is skipped");
    check("casez(sel) dsf:q=0; endcase casex(sel) flag:q=1; endcase", {"dsf", "flag"},
          "wildcard case branch node variants");
    check("case(sel) unknown:q=0; pkg::UNKNOWN:q=1; top.dsf:q=2; `MACRO:q=3; endcase", {},
          "unresolved symbols are not falsely classified as known variables");
    check("case(sel) pkg::dsf:q=0; `dsf:q=1; $bits(dsf):q=2; dsf:q=3; endcase", {"dsf"},
          "qualified names/macros/elaboration query cannot bind to local signal");
    check("begin : block_name logic P; case(sel) P:q=0; endcase end "
          "case(sel) P:q=0; endcase", {"P"}, "lexical signal shadowing versus outer parameter");
    check("localparam R=dsf+1; case(sel) R:q=0; endcase", {"R"},
          "invalid parameter initializer has a proven variable dependency");
    check("function int f(); case(sel) f():q=0; f(dsf):q=1; endcase endfunction", {"f"},
          "function call name is not the implicit return signal but variable argument is runtime");
    check("case(sel) default:q=dsf; endcase", {}, "default is not an expression label");
    check("case(sel) 0:; dsf:; endcase", {"dsf"}, "empty statement body");
    check("case(sel) 0: unique case(dsf) flag:q=sel; endcase dsf:q=1; endcase", {"dsf","flag"},
          "unique nested case body");
    check("case(sel) 0: begin : branch q=dsf; end : branch flag:q=0; endcase", {"flag"},
          "named begin/end body");
    check("case(sel) 0:@(posedge flag) q=dsf; dsf:q=1; endcase", {},
          "unsupported event-controlled body ends conservative scan");
    return failures == 0 ? 0 : 1;
}
