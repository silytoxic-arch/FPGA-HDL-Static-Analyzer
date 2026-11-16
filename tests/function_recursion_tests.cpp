#include "analysis/function_recursion.h"
#include "core/source.h"

#include <iostream>
#include <string>

int main() {
    using namespace fpga;
    int failures = 0;
    const auto check = [&](const char* code, std::size_t expected, const char* label,
                           bool incomplete = false) {
        const core::SourceFile source("recursion.sv", core::HdlLanguage::SystemVerilog, code);
        const auto result = analysis::detail::functionRecursion(source, parser::tokenize(source));
        if (result.cycles.size() != expected || result.incomplete != incomplete) {
            ++failures;
            std::cerr << "FAIL " << label << " cycles=" << result.cycles.size()
                      << " incomplete=" << result.incomplete << '\n';
        }
    };
    check("module m; function integer f(input integer n); f=f(n-1); endfunction endmodule",
          1, "ANSI direct recursion");
    check("module m; function [1:0] f; input n; f=f(n); endfunction endmodule", 1, "non-ANSI recursion");
    check("module m; function integer f; f=g(0); endfunction function integer g; g=f(0); endfunction endmodule",
          1, "mutual recursion is one member-set cycle");
    check("module m; function integer f; f=g(0); endfunction function integer g; g=h(0); endfunction "
          "function integer h; h=f(0); endfunction endmodule", 1, "three-node member-set cycle");
    check("module m; function integer f; f=g(0); endfunction function integer g; g=0; endfunction endmodule",
          0, "acyclic chain");
    check("module m; function integer f; f=0; endfunction endmodule module n; "
          "function integer f; f=f(0); endfunction endmodule", 1, "same names isolated");
    check("module a; function integer f; f=g(0); endfunction endmodule "
          "module b; function integer g; g=f(0); endfunction endmodule",
          1, "native global name map joins a cross-module cycle");
    check("module a; function integer f; f=f(0); endfunction endmodule "
          "module b; function integer f; f=f(1); endfunction endmodule",
          1, "duplicate recursive names share one native member-set record");
    check("module m; function integer f; f=0; endfunction endmodule", 0, "return assignment not a call");
    check("module m; function integer f; /* f=f(0); */ f=0; endfunction endmodule", 0, "comment excluded");
    check("module m; function string f; f=\"f(0)\"; endfunction endmodule", 0, "string excluded");
    check("module m; function integer f; f=external(0); endfunction endmodule", 0, "unresolved call", true);
    check("module m; function integer f; f=obj.f(0); endfunction endmodule", 0, "hierarchical unresolved", true);
    check("module m; function integer f; f=pkg::f(0); endfunction endmodule", 0, "package unresolved", true);
    check("module m; function integer f; f=$clog2(8); endfunction endmodule", 0, "system call excluded");
    check("module m; function automatic integer f(input integer n); if(n) f=f(n-1); else f=0; endfunction endmodule",
          1, "automatic conditional recursion");
    check("module m; function integer f; f=f(0); endfunction function integer f; f=0; endfunction endmodule",
          1, "call-free duplicate is absent from native candidate map");
    check("module m; function integer f; f=f(0)+f(1); endfunction endmodule",
          1, "repeated self-call deduplicates by member set");
    check("module m; function integer f; f=g(0)+h(0); endfunction "
          "function integer g; g=f(0); endfunction function integer h; h=f(0); endfunction endmodule",
          2, "overlapping cycles with different member sets remain distinct");
    check("module m; function integer f; f=f(0); endmodule", 0, "malformed function unresolved", true);
    check("module m; function integer f(input integer f); f=f(0); endfunction endmodule",
          0, "parameter shadow unresolved", true);
    check("module m; function integer f; integer g; f=g(0); endfunction "
          "function integer g; g=f(0); endfunction endmodule", 0, "local shadow excludes graph edge", true);
    check("module m; extern function integer f(); function integer g; g=f(0); endfunction endmodule",
          0, "extern prototype does not swallow next definition", true);
    check("module m; function integer f; f=`f(0); endfunction endmodule",
          0, "macro name does not create a recursive edge", true);
    check("module m; wire \\function ; function integer f; f=0; endfunction endmodule",
          0, "escaped function keyword does not start a definition");
    return failures ? 1 : 0;
}
