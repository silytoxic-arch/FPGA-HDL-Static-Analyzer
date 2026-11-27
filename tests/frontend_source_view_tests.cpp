#include "parser/frontend_source_view.h"
#include "analysis/analyzer.h"
#include <algorithm>
#include <chrono>
#include <fstream>
#include <iostream>

int main() {
    using namespace fpga;
    int checks=0,failures=0;
    const auto check=[&](bool okay,const char* label) {++checks;if(!okay){++failures;std::cerr<<"FAIL "<<label<<'\n';}};
    const auto base=std::filesystem::temp_directory_path()/
        ("fpga-source-view-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directories(base);
    struct Cleanup {std::filesystem::path path;~Cleanup(){std::error_code ec;std::filesystem::remove_all(path,ec);}} cleanup{base};
    const auto count=[](const auto& result,std::string_view rule) {
        return std::count_if(result.violations.begin(),result.violations.end(),[&](const auto& item){return item.ruleId==rule;});
    };
    const core::SourceFile literal(base/"literal.sv",core::HdlLanguage::SystemVerilog,
        "module literal_owner(input a,output y);\r\nassign y=a;\r\nendmodule\r\n");
    const auto expanded=parser::expandFrontendSource(literal);
    const parser::FrontendSourceView view(literal,expanded);
    const auto parsed=parser::parseDesignUnits(view.input());
    check(view.admitted()&&parsed.size()==1,"a literal root is admitted through the typed expanded view");
    if(parsed.size()!=1) return 1;
    const auto reported=view.report(parsed.front());
    check(reported.location.file==literal.path()&&reported.location.line==1&&reported.location.column==8&&
          reported.location.byteOffset==literal.content().find("literal_owner"),"literal module coordinates retain the original precise column and byte offset");
    check(parsed.front().location.byteOffset!=reported.location.byteOffset&&parsed.front().location.line!=reported.location.line,
          "report mapping does not overwrite the retained virtual parse coordinates");
    check(reported.ports.size()==2&&reported.ports[0].location.byteOffset==literal.content().find("a,output"),
          "nested literal port locations use original text offsets");
    for(const auto& issue:parser::analyzeSemantics(view.input())) {
        const auto at=view.report(issue.location);
        check(at.file==literal.path()&&at.line>0,"semantic issue locations resolve through the typed source view");
    }
    const core::SourceFile macro(base/"macro.sv",core::HdlLanguage::SystemVerilog,
        "`define MAKE module macro_owner(input clk,input d,output reg q);always @(posedge clk) q<=d;endmodule\n`MAKE\n");
    auto result=analysis::Analyzer().analyzeSource(macro);
    check(result.designUnits==1&&result.structure.size()==1,"generic analysis reports a macro-generated module instead of an empty design");
    if(result.structure.size()!=1) return 1;
    const auto& unit=result.structure.front();
    check(unit.name=="macro_owner"&&unit.ports.size()==3&&unit.clocks.size()==1&&unit.sequentialProcesses.size()==1,
          "macro-generated ports, clocks and sequential processes enter the actual analysis result");
    if(unit.ports.size()!=3 || unit.clocks.size()!=1 || unit.sequentialProcesses.size()!=1) return 1;
    check(unit.location.file==macro.path()&&unit.location.line==2&&unit.location.column==1&&unit.location.byteOffset==0,
          "macro-generated locations retain invocation line and explicitly unknown physical offset");
    check(unit.ports[0].location.line==2&&unit.clocks[0].location.line==2&&unit.sequentialProcesses[0].location.line==2,
          "nested model locations map independently to the invocation");
    check(unit.sequentialProcesses[0].clock=="clk"&&unit.sequentialProcesses[0].writes==std::vector<std::string>{"q"},
          "timing relationships are computed before report location conversion");
    const core::SourceFile twoProcesses(base/"two.sv",core::HdlLanguage::SystemVerilog,
        "`define MAKE module two_owner(input clk,d0,d1,output reg q0,q1);always @(posedge clk) q0<=d0;always @(posedge clk) q1<=d1;endmodule\n`MAKE\n");
    result=analysis::Analyzer().analyzeSource(twoProcesses);
    check(result.structure.size()==1&&result.structure.front().sequentialProcesses.size()==2,
          "distinct expanded processes are not collapsed when physical invocation coordinates coincide");
    if(result.structure.size()==1&&result.structure.front().sequentialProcesses.size()==2) {
        const auto& processes=result.structure.front().sequentialProcesses;
        check(processes[0].location==processes[1].location&&processes[0].writes!=processes[1].writes,
              "equal report coordinates preserve distinct process data");
    }
    {std::ofstream file(base/"leaf.vh");file<<"module leaf #(parameter P=1)(input a,output y);\nassign y=a;\nendmodule\n";}
    const core::SourceFile parent(base/"parent.sv",core::HdlLanguage::SystemVerilog,
        "`include \"leaf.vh\"\nmodule parent(input a,output y);\nleaf #(.P(2)) child(.a(a),.y(y));\nendmodule\n");
    result=analysis::Analyzer().analyzeSources({parent});
    check(result.structure.size()==2&&result.designUnits==2&&result.filesAnalyzed==1,
          "included design units enter structure without pretending the selected root count changed");
    const auto leaf=std::find_if(result.structure.begin(),result.structure.end(),[](const auto& item){return item.name=="leaf";});
    const auto top=std::find_if(result.structure.begin(),result.structure.end(),[](const auto& item){return item.name=="parent";});
    check(leaf!=result.structure.end()&&top!=result.structure.end(),"both included child and parent owners are retained");
    if(leaf!=result.structure.end()&&top!=result.structure.end()) {
        check(leaf->location.file==base/"leaf.vh"&&leaf->location.line==1&&leaf->location.column==1&&leaf->location.byteOffset==0,
              "included module locations name the actual header without a guessed byte offset");
        check(leaf->ports.size()==2&&leaf->parameters.size()==1&&leaf->ports[0].location.file==base/"leaf.vh"&&
              leaf->parameters[0].location.file==base/"leaf.vh","included parameter and port origins are mapped with their owner");
        check(!leaf->topLevel&&top->topLevel&&top->instances.size()==1&&top->instances[0].resolved,
              "hierarchy resolution consumes expanded units and preserves child binding");
        if(top->instances.size()==1) {
            const auto& instance=top->instances.front();
            check(instance.location.file==parent.path()&&instance.location.line==3&&instance.location.byteOffset>0,
                  "literal parent instance coordinates retain a physical text offset");
            check(instance.namedConnections.size()==2&&instance.namedParameterOverrides.size()==1,
                  "expanded instances retain named connection and parameter metadata");
            if(instance.namedConnections.size()==2&&instance.namedParameterOverrides.size()==1) {
                check(instance.namedConnections[0].location.file==parent.path()&&instance.namedConnections[0].location.line==3&&
                      instance.namedConnections[0].actualLocation.file==parent.path()&&instance.namedConnections[0].actualLocation.line==3,
                      "formal and actual association positions are mapped separately");
                check(instance.namedParameterOverrides[0].location.file==parent.path()&&instance.namedParameterOverrides[0].location.line==3,
                      "named parameter override positions remain attached to the parent source");
            }
        }
    }
    const core::SourceFile undriven(base/"undriven.sv",core::HdlLanguage::SystemVerilog,
        "`define MAKE module undriven_owner(output y);endmodule\n`MAKE\n");
    result=analysis::Analyzer().analyzeSource(undriven);
    check(count(result,"RTL-UNDRIVEN-OUTPUT")==1&&std::any_of(result.violations.begin(),result.violations.end(),[&](const auto& item){
        return item.ruleId=="RTL-UNDRIVEN-OUTPUT"&&item.file==undriven.path()&&item.line==2&&item.column==1;
    }),"semantic analysis of macro-generated declarations reports the original invocation");
    {std::ofstream file(base/"broken.vh");file<<"module broken;\n";}
    result=analysis::Analyzer().analyzeSource({base/"broken_root.sv",core::HdlLanguage::SystemVerilog,"`include \"broken.vh\"\n"});
    check(std::any_of(result.violations.begin(),result.violations.end(),[&](const auto& item){
        return item.ruleId=="SYNTAX-UNCLOSED-CONSTRUCT"&&item.file==base/"broken.vh"&&item.line==1;
    }),"structural syntax issues in included HDL retain the included file origin");
    const core::SourceFile aliased(base/"aliased.sv",core::HdlLanguage::SystemVerilog,
        "`line 90 \"generated_original.sv\" 0\nmodule aliased_owner(input clk,d,output reg q);always @(posedge clk) q<=d;endmodule\n");
    result=analysis::Analyzer().analyzeSource(aliased);
    check(result.structure.size()==1&&result.structure.front().location.file=="generated_original.sv"&&
        result.structure.front().location.line==90&&result.structure.front().location.byteOffset==0,
        "logical line origins are report coordinates, not byte offsets into the selected root");
    check(result.structure.size()==1&&result.structure.front().clocks.size()==1,"aliased timing remains populated");
    if(result.structure.size()==1&&result.structure.front().clocks.size()==1)
        check(result.structure.front().clocks[0].location.file=="generated_original.sv"&&result.structure.front().clocks[0].location.line==90,
              "timing origins follow logical line mapping with their module");
    result=analysis::Analyzer().analyzeSource({base/"unknown.sv",core::HdlLanguage::SystemVerilog,"module unknown_owner;wire `UNKNOWN;endmodule\n"});
    check(result.structure.empty()&&count(result,"PP-FRONTEND-DIRECTIVE")>0,
          "failed expansion never falls back to a fabricated partial core model");
    analysis::AnalysisOptions options;options.standardId="GJB10157-2021";options.topModule="macro_owner";
    result=analysis::Analyzer(options).analyzeSource(macro);
    check(result.errors.empty()&&result.structure.size()==1&&result.structure.front().clocks.size()==1,
          "standard evaluation retains the actual expanded structure and timing result");
    check(!result.standardAssessments.empty()&&std::none_of(result.standardAssessments.begin(),result.standardAssessments.end(),[](const auto& row){
        return row.state==analysis::StandardRuleState::Passed;
    }),"unmigrated standard adapters and clean core results do not become automatic rule compliance");
    const analysis::Analyzer generic;
    const core::SourceFile sharedDefinitions(base/"definitions.sv",core::HdlLanguage::SystemVerilog,
        "`define SHARED module shared_owner(input a,output y);assign y=a;endmodule\n");
    const core::SourceFile sharedUse(base/"use.sv",core::HdlLanguage::SystemVerilog,"`SHARED\n");
    result=generic.analyzeSources({sharedDefinitions,sharedUse});
    check(result.structure.size()==1&&result.structure.front().name=="shared_owner"&&result.structure.front().ports.size()==2,
          "generic project mode shares source macros in root order");
    result=generic.analyzeSource(sharedUse);
    check(result.structure.empty()&&count(result,"PP-FRONTEND-DIRECTIVE")>0,
          "reusing a generic Analyzer does not leak a prior project macro cache");
    {std::ofstream file(base/"waived.vh");file<<"module waived_owner(output y); // fpga-lint-disable-line RTL-UNDRIVEN-OUTPUT\nendmodule\n";}
    const core::SourceFile waivedRoot(base/"waived_root.sv",core::HdlLanguage::SystemVerilog,"`include \"waived.vh\"\n");
    const auto waiverFrame=parser::expandFrontendSource(waivedRoot);
    check(waiverFrame.includedSources.size()==1&&waiverFrame.includedSources[0].content().find("fpga-lint-disable-line")!=std::string::npos,
          "include snapshots preserve the actual consumed comments before expansion strips them");
    result=generic.analyzeSource(waivedRoot);
    check(count(result,"RTL-UNDRIVEN-OUTPUT")==0&&result.suppressedViolations>0,
          "generic include diagnostics honor waivers from their consumed source snapshot");
    {std::ofstream file(base/"waived.vh");file<<"module waived_owner(output y);\nendmodule\n";}
    result=generic.analyzeSource(waivedRoot);
    check(count(result,"RTL-UNDRIVEN-OUTPUT")==1,"a later independent analysis reads the new include text instead of stale waiver state");
    {std::ofstream file(base/"self.sv");file<<"`ifndef SEEN\nmodule disk_owner;endmodule\n`endif\n";}
    const core::SourceFile changing(base/"self.sv",core::HdlLanguage::SystemVerilog,
        "`define SEEN\n`include \"./self.sv\"\n// fpga-lint-disable-next-line RTL-UNDRIVEN-OUTPUT\nmodule memory_owner(output y);\nendmodule\n");
    const auto changingFrame=parser::expandFrontendSource(changing);
    check(changingFrame.issues.empty()&&changingFrame.ambiguousInputFiles.size()==1,
          "different consumed texts for one pathname are explicitly marked ambiguous");
    const parser::FrontendSourceView changingView(changing,changingFrame);
    const auto changingUnits=parser::parseDesignUnits(changingView.input());
    if(changingUnits.size()==1) {
        const auto mapped=changingView.report(changingUnits[0]);
        check(mapped.location.column==1&&mapped.location.byteOffset==0,
              "ambiguous pathname snapshots do not acquire a guessed exact offset");
    } else check(false,"guarded self-include leaves one memory owner");
    result=generic.analyzeSource(changing);
    check(count(result,"RTL-UNDRIVEN-OUTPUT")==1,"ambiguous same-path text cannot prove a comment-based suppression");
    const std::string crlfText="`ifndef SAME_GUARD\r\n`define SAME_GUARD\r\n`include \"same.sv\"\r\nmodule same_owner(output y); // fpga-lint-disable-line RTL-UNDRIVEN-OUTPUT\r\nendmodule\r\n`endif\r\n";
    {std::ofstream file(base/"same.sv",std::ios::binary);file<<crlfText;}
    const core::SourceFile sameRoot(base/"same.sv",core::HdlLanguage::SystemVerilog,crlfText);
    const auto sameFrame=parser::expandFrontendSource(sameRoot);
    check(sameFrame.issues.empty()&&sameFrame.ambiguousInputFiles.empty(),
          "host CRLF normalization alone does not fabricate conflicting same-path snapshots");
    result=generic.analyzeSource(sameRoot);
    check(result.structure.size()==1&&count(result,"RTL-UNDRIVEN-OUTPUT")==0&&result.suppressedViolations>0,
          "a guarded CRLF self-include preserves an original-source waiver on all hosts");
    analysis::AnalysisOptions mappedOptions;mappedOptions.standardId="GJB10157-2021";
    result=analysis::Analyzer(mappedOptions).analyzeSource({base/"mapped_seq.sv",core::HdlLanguage::SystemVerilog,
        "`define MAKE module mapped_seq(input clk,d,output reg q);always @(posedge clk) q=d;endmodule\n`MAKE\n"});
    check(result.structure.size()==1&&count(result,"GJB10157.9.1.4")==1&&std::any_of(result.violations.begin(),result.violations.end(),[](const auto& item){
        return item.ruleId=="GJB10157.9.1.4"&&item.line==2&&item.column==1;
    }),"expanded sequential blocking evidence survives standard translation and source-bound adapter evaluation");
    result=analysis::Analyzer(mappedOptions).analyzeSource({base/"mapped_comb.sv",core::HdlLanguage::SystemVerilog,
        "`define MAKE module mapped_comb(input d,output reg q);always @* q<=d;endmodule\n`MAKE\n"});
    check(count(result,"GJB10157.9.1.3")==1,"expanded combinational nonblocking assignments reach their official standard clause");
    result=analysis::Analyzer(mappedOptions).analyzeSource({base/"mapped_defparam.sv",core::HdlLanguage::SystemVerilog,
        "`define MAKE module leaf #(parameter P=1);endmodule module parent;leaf child();defparam child.P=2;endmodule\n`MAKE\n"});
    check(count(result,"GJB10157.5.2.16")==1&&std::any_of(result.violations.begin(),result.violations.end(),[](const auto& item){
        return item.ruleId=="GJB10157.5.2.16"&&item.message=="禁止使用defparam语句"&&item.line==2&&item.column==1;
    }),"macro-generated defparam preserves the recovered fixed standard diagnostic and invocation origin");
    std::cout<<checks<<" source-view checks, "<<failures<<" failures\n";
    return failures?1:0;
}
