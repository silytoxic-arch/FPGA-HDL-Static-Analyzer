#include "analysis/verilog_function_checks.h"
#include <iostream>
#include <string>

int main() {
    int failures = 0;
    const auto check = [&](const std::string& text, std::size_t expected, const char* name) {
        auto tokens = fpga::parser::tokenize({"probe.sv", fpga::core::HdlLanguage::SystemVerilog, text});
        // Match the standard evaluator's operator normalization.
        for (std::size_t i = 0; i + 1 < tokens.size(); ++i) {
            if ((tokens[i].text == "<" || tokens[i].text == "=") && tokens[i + 1].text == "=") {
                tokens[i].text += '=';
                tokens.erase(tokens.begin() + i + 1);
            }
        }
        const auto actual = fpga::analysis::functionNonblockingAssignments(tokens).size();
        if (actual != expected) { ++failures; std::cerr << name << ": " << actual << " != " << expected << '\n'; }
    };
    check("module m; function f; input a; f<=a+1; endfunction endmodule", 1, "published violation");
    check("module m; function f; input a; f=a+1; endfunction endmodule", 0, "published compliance");
    check("function int f(input int a); if(a<=2) f=a<=1; else f=0; endfunction", 0, "conditions and blocking RHS");
    check("function int f(input int a); return a<=1; endfunction", 0, "return comparison");
    check("function f; for(int i=0; i<=2; i++) begin f<=i; end endfunction", 1, "for condition exclusion");
    check("function f; if(a<=2) f<=a; else f<=0; endfunction", 2, "both assignment branches");
    check("function f; f[a<=2]<=x; endfunction", 1, "selected target comparison");
    check("function f; begin if(a) begin q<=d; r<=e; end else s<=t; end endfunction", 3,
          "recursive collector reports every nested assignment node");
    check("function f; q <= (a <= b); endfunction", 1,
          "assignment root is reported while its RHS relation is not duplicated");
    check("class c; function void f(); q<=d; endfunction endclass", 1,
          "class method function remains an admitted FUNCTION root");
    check("package p; function int f(input int a); f<=a; endfunction endpackage", 1,
          "package function remains an admitted FUNCTION root");
    check("task t; q<=a; endtask always @* q<=a;", 0, "task and always exclusion");
    check("function f; /*f<=1;*/ string s=\"f<=2\"; f=0; endfunction", 0, "comment/string exclusion");
    check("class c; extern function f(); function g(); g=0; endfunction endclass", 0, "extern prototype");
    check("class c; pure virtual function f(); endclass module m; always @* q<=1; endmodule", 0, "pure virtual prototype");
    check("module m; function f; f<=1; endmodule", 0, "unclosed function remains unproved");
    return failures ? 1 : 0;
}
