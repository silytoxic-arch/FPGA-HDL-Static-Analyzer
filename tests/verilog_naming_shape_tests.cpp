#include "analysis/verilog_naming_shape.h"
#include <iostream>

int main() {
    using namespace fpga;
    using namespace analysis;
    int failures=0,tests=0;
    const auto predicate=[&](std::string_view name,VerilogNamingShapeSubject subject,
                             std::optional<bool> first,std::optional<bool> underscore,const char* label) {
        ++tests; const auto out=checkVerilogNamingShape(name,subject);
        if(out.firstByteViolation!=first||out.underscoreViolation!=underscore) {++failures;std::cerr<<"FAIL "<<label<<'\n';}
    };
    const auto net=VerilogNamingShapeSubject::Net;
    predicate("_bad",net,true,false,"leading underscore only first rule");
    predicate("9bad",net,true,false,"leading digit exact predicate");
    predicate("Good",net,false,false,"uppercase allowed by shape rules");
    predicate("$not_letter",net,false,false,"not full ASCII alphabet ban");
    predicate("!odd",net,false,false,"punctuation not first-byte violation");
    predicate("éname",net,false,false,"nonASCII byte not expanded to alphabet ban");
    predicate("a__b",net,false,true,"double underscore substring");
    predicate("a_",net,false,true,"trailing underscore");
    predicate("__a_",net,true,true,"both conditions one underscore result");
    predicate("",net,false,false,"empty native name skipped");
    predicate("a__",VerilogNamingShapeSubject::Function,false,std::nullopt,"function excluded from second native type set");
    predicate("_fn",VerilogNamingShapeSubject::Function,true,std::nullopt,"function included first type set");
    predicate("_bad",VerilogNamingShapeSubject::Unsupported,std::nullopt,std::nullopt,"unknown not compliant");
    const auto sourceCheck=[&](std::string text,std::vector<std::pair<std::string,VerilogNamingShapeSubject>> expected,
                               bool incomplete,const char* label) {
        ++tests; core::SourceFile source("probe.sv",core::HdlLanguage::SystemVerilog,std::move(text));
        const auto tokens=parser::tokenize(source);
        const auto out=scanVerilogNamingShape(source,tokens);
        std::vector<std::pair<std::string,VerilogNamingShapeSubject>> got;
        for(const auto& d:out.declarations) {
            got.emplace_back(d.normalizedName,d.subject);
            if(d.tokenIndex>=tokens.size()||tokens[d.tokenIndex].text!=d.normalizedName) ++failures;
        }
        if(got!=expected||out.incomplete!=incomplete||!out.partial) {
            ++failures; std::cerr<<"FAIL "<<label<<" incomplete="<<out.incomplete<<" names:";
            for(const auto& d:got) std::cerr<<' '<<d.first;
            std::cerr<<'\n';
        }
    };
    const auto module=VerilogNamingShapeSubject::Module, param=VerilogNamingShapeSubject::Parameter,
        local=VerilogNamingShapeSubject::Localparam, task=VerilogNamingShapeSubject::Task,
        function=VerilogNamingShapeSubject::Function, cell=VerilogNamingShapeSubject::Cell;
    sourceCheck("module _m(input wire _clk, output reg data_); parameter _P=1, P__Q=2; localparam L_=3; wire good,_bad; endmodule",
        {{"_m",module},{"_clk",net},{"data_",net},{"_P",param},{"P__Q",param},{"L_",local},{"good",net},{"_bad",net}},false,"module wire and parameters declarations");
    sourceCheck("module m; task automatic _task(input wire _arg); reg t__reg; parameter T_=1; endtask function automatic logic [7:0] _func(input wire f__arg); reg f__reg; localparam F_=2; _func=f__arg; endfunction endmodule",
        {{"m",module},{"_task",task},{"_arg",net},{"t__reg",net},{"T_",param},{"_func",function},{"f__arg",net},{"f__reg",net},{"F_",local}},false,"recursive task function descendant names");
    sourceCheck("module m; wire good; assign good = _undeclared; initial begin _task(); $display(\"wire __fake\"); end endmodule",
        {{"m",module},{"good",net}},false,"uses calls strings not declarations");
    sourceCheck("module m; function integer good_fn; input wire _in; reg x_; good_fn=_in; endfunction endmodule",
        {{"m",module},{"good_fn",function},{"_in",net},{"x_",net}},false,"nonANSI builtin function");
    sourceCheck("module m; task t_; input wire data; endtask endmodule",{{"m",module},{"t_",task},{"data",net}},false,"nonANSI task");
    sourceCheck("module child; endmodule module m; child _cell(.a(_use)); endmodule",{{"child",module},{"m",module},{"_cell",cell}},false,"known local module cell name not port uses");
    sourceCheck("module child; endmodule module m; child #(.N(1)) __cell(); endmodule",{{"child",module},{"m",module}},true,"parameterized cell unproved");
    sourceCheck("module m; class C; task __hidden; wire _hidden; endtask endclass wire visible; endmodule",{{"m",module},{"visible",net}},true,"class subtree excluded");
    sourceCheck("module m; `ifdef X\n wire __hidden; task _hidden; endtask\n `endif\n wire visible; endmodule",{{"m",module},{"visible",net}},true,"conditional macro declarations unproved");
    sourceCheck("module m;\n `define X \\\n task __fake; wire _fake; endtask\n wire visible; endmodule",{{"m",module},{"visible",net}},true,"continued macro body not a declaration");
    sourceCheck("module m; wire \\__escaped ; task \\_escaped ; endtask endmodule",{{"m",module}},true,"escaped names unknown source binding");
    sourceCheck("module m; function user_t __unknown(input wire a); endfunction endmodule",{{"m",module},{"a",net}},true,"unknown return type does not invent function name");
    // Existing adapter's default remains module-level for 5.2.15.
    ++tests;
    core::SourceFile original("probe.sv",core::HdlLanguage::SystemVerilog,"module m; wire a; function logic f(input wire inner); reg local_; endfunction endmodule");
    const auto tokens=parser::tokenize(original);
    const auto old=VerilogNamingDeclarations(original,tokens).collect();
    if(old.size()!=1||old.front().normalizedName!="a") ++failures;
    std::cout<<tests<<" cases, "<<failures<<" failures\n";
    return failures?1:0;
}
