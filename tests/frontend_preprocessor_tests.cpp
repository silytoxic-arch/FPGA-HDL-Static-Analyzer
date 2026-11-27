#include "parser/frontend_preprocessor.h"
#include "analysis/analyzer.h"
#include "parser/design_parser.h"
#include "project/file_list.h"
#include <algorithm>
#include <chrono>
#include <fstream>
#include <future>
#include <iostream>

int main() {
    using namespace fpga;
    int failures=0,checks=0;
    const auto check=[&](bool okay,const char* label){++checks;if(!okay){++failures;std::cerr<<"FAIL "<<label<<'\n';}};
    const auto base=std::filesystem::temp_directory_path()/
        ("fpga-frontend-unit-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directories(base);
    struct Cleanup { std::filesystem::path path;~Cleanup(){std::error_code ec;std::filesystem::remove_all(path,ec);} } cleanup{base};
    const auto root=base/"top.sv";
    const auto run=[&](const std::string& text,const parser::PreprocessorOptions& options=parser::PreprocessorOptions{},
                       const parser::FrontendPreprocessorLimits& limits=parser::FrontendPreprocessorLimits{}) {
        return parser::expandFrontendSource(core::SourceFile(root,core::HdlLanguage::SystemVerilog,text),options,limits);
    };
    const auto names=[](const parser::FrontendPreprocessorResult& out) {
        std::vector<std::string> result;
        for(const auto& token:parser::tokenize(out.source)) result.push_back(token.text);
        return result;
    };
    const auto has=[&](const auto& out,std::string_view value) {
        const auto all=names(out);return std::find(all.begin(),all.end(),value)!=all.end();
    };
    const auto origin=[](const auto& out,std::string_view value) {
        for(const auto& token:parser::tokenize(out.source)) if(token.text==value) return out.origin(token.line);
        return core::SourceLocation{};
    };
    auto out=run("`define DECL wire bad__;\nmodule m;\n`DECL\nendmodule\n");
    check(out.issues.empty()&&has(out,"bad__"),"a macro invocation at the start of a physical line expands");
    check(origin(out,"bad__").file==root&&origin(out,"bad__").line==3,"macro body uses frontend invocation line, not define line");
    out=run("`define A bad__\n`define B `A\nmodule m;wire `B;endmodule\n");
    check(out.issues.empty()&&has(out,"bad__"),"nested object macros expand transitively");
    out=run("`define ID(x) x\nmodule m;wire `ID(bad__);endmodule\n");
    check(out.issues.empty()&&has(out,"bad__"),"function-like formal substitution");
    out=run("`define ID(x=bad__) x\nmodule m;wire `ID();endmodule\n");
    check(out.issues.empty()&&has(out,"bad__"),"empty supplied argument uses a default");
    out=run("`define ID(x=) x\nmodule m;`ID() wire good;endmodule\n");
    check(out.issues.empty()&&has(out,"good"),"empty defaults are not treated as missing required arguments");
    out=run("`define JOIN(a,b) a``b\nmodule m;wire `JOIN(bad,__);endmodule\n");
    check(out.issues.empty()&&has(out,"bad__"),"token pasting produces a new identifier");
    out=run("`define TEXT(x) `\"x`\"\nmodule m;initial $display(`TEXT(hello));endmodule\n");
    check(out.issues.empty()&&has(out,"\"hello\""),"macro string quotation substitutes formals");
    out=run("`define ID(x) x\nmodule m;initial $display(`ID({1,2,(3+4)}));endmodule\n");
    check(out.issues.empty()&&has(out,"display")==false&&has(out,"$display"),"nested braces and parentheses retain commas in one argument");
    out=run("`define NAME first__\n`undef NAME\n`define NAME second__\nmodule m;wire `NAME;endmodule\n");
    check(out.issues.empty()&&has(out,"second__")&&!has(out,"first__"),"undef and later definitions are ordered");
    out=run("`ifdef ENABLED\nmodule yes;wire bad__;endmodule\n`else\nmodule no;endmodule\n`endif\n",{{},{"ENABLED=1"}});
    check(out.issues.empty()&&has(out,"yes")&&!has(out,"no"),"configured definitions select branches");
    out=run("/*\n`define HIDDEN 1\n*/\n`ifdef HIDDEN\nmodule no;endmodule\n`else\nmodule yes;endmodule\n`endif\n");
    check(out.issues.empty()&&has(out,"yes")&&!has(out,"no"),"directives inside multiline comments have no effect");
    out=run("module m;initial $display(\"`UNKNOWN `file_push file\");endmodule\n");
    check(out.issues.empty(),"macro and file-stack spellings inside ordinary strings remain literal");
    out=run("`define LONG \\\n wire first__; \\\n wire second_;\nmodule m;\n`LONG\nendmodule\n");
    check(out.issues.empty()&&origin(out,"first__").line==5&&origin(out,"second_").line==5,
          "continued definitions retain line counts and expand on one invocation line");
    out=run("`define ID(x) wire x;\nmodule m;\n`ID(\nbad__\n)\nendmodule\n");
    check(out.issues.empty()&&origin(out,"bad__").line==5,"multiline actuals preserve native post-argument line counter");
    {std::ofstream f(base/"body.vh");f<<"wire included__;\n";}
    out=run("module m;\n`include \"body.vh\"\nwire after_;\nendmodule\n");
    check(out.issues.empty()&&origin(out,"included__").file==base/"body.vh"&&origin(out,"included__").line==1,
          "include declarations use their physical file and line");
    check(origin(out,"after_").file==root&&origin(out,"after_").line==3,"file_pop restores the parent physical line");
    {std::ofstream f(base/"inner.vh");f<<"wire nested__;\n";}
    {std::ofstream f(base/"outer.vh");f<<"`include \"inner.vh\"\nwire outer_;\n";}
    out=run("module m;`include \"outer.vh\"\nwire final_;endmodule\n");
    check(out.issues.empty()&&origin(out,"nested__").file==base/"inner.vh"&&origin(out,"outer_").line==2&&origin(out,"final_").line==2,
          "nested inline include stacks restore both inner and outer sources");
    {std::ofstream f(base/"defs.vh");f<<"`define ACTIVE 1\n`define NAME from_header__\n";}
    out=run("`include \"defs.vh\"\n`ifdef ACTIVE\nmodule m;wire `NAME;endmodule\n`endif\n");
    check(out.issues.empty()&&has(out,"from_header__")&&origin(out,"from_header__").file==root&&origin(out,"from_header__").line==3,
          "header macro definitions affect parent branches and expansion, with caller location");
    out=run("`define HEADER \"body.vh\"\nmodule m;`include `HEADER\nendmodule\n");
    check(out.issues.empty()&&has(out,"included__"),"include filenames can be macro-expanded");
    std::filesystem::create_directories(base/"includes");
    {std::ofstream f(base/"includes"/"only.vh");f<<"wire from_path__;\n";}
    out=run("module m;`include \"only.vh\"\nendmodule\n",{{base/"includes"},{}});
    check(out.issues.empty()&&has(out,"from_path__"),"configured include directories are searched");
    const auto unicode=std::filesystem::path(u8"中文目录");std::filesystem::create_directories(base/unicode);
    {std::ofstream f(base/unicode/"body.vh");f<<"wire unicode__;\n";}
    out=run("module m;`include \"中文目录/body.vh\"\nendmodule\n");
    check(out.issues.empty()&&origin(out,"unicode__").file==base/unicode/"body.vh","UTF-8 include paths retain their physical source on all hosts");
    out=run("`line 40 \"virtual.sv\" 0\nmodule m;wire virtual__;endmodule\n");
    check(out.issues.empty()&&origin(out,"virtual__").file=="virtual.sv"&&origin(out,"virtual__").line==40,"line directive follows the retained lexer counter");
    out=run("`default_nettype none module m;wire same_line__;endmodule\n");
    check(out.issues.empty()&&has(out,"same_line__"),"default_nettype consumes only its argument, not later declarations");
    out=run("module m;wire `MISSING;endmodule\n");check(!out.issues.empty(),"undefined macros do not become successful frontend input");
    out=run("`include \"missing_file_132.vh\"\n");check(!out.issues.empty(),"missing include markers become diagnostics");
    out=run("`define ID(x,y) x\nmodule m;wire `ID(a);endmodule\n");check(!out.issues.empty(),"missing required macro arguments fail closed");
    out=run("`define ID(x,x) x\n");check(!out.issues.empty(),"duplicate macro formals fail closed");
    out=run("`ifdef OPEN\nmodule m;endmodule\n");check(!out.issues.empty(),"unterminated conditionals fail closed");
    out=run("`define LOOP `LOOP\n`LOOP\n",{}, {2000,1024*1024,16,32});check(!out.issues.empty(),"recursive macros are bounded by expansion operations");
    {std::ofstream f(base/"cycle.vh");f<<"`include \"cycle.vh\"\n";}
    out=run("`include \"cycle.vh\"\n",{}, {100000,1024*1024,8,32});check(!out.issues.empty(),"recursive include depth is bounded");
    out=run("`define BIG abcdefghijklmnopqrstuvwxyz\n`BIG\n",{}, {100000,8,8,32});check(!out.issues.empty(),"input/output byte budget is enforced");
    out=run("`define BIG "+std::string(6000,'a')+"\n`BIG\n",{}, {100000,4096,8,32});
    check(!out.issues.empty()&&out.issues.front().message.find("展开结果超过大小限制")!=std::string::npos,
          "expanded output budget is distinct from permitted physical input bytes");
    parser::FrontendPreprocessorLimits inputLimit;inputLimit.maxInputBytes=8;
    out=run("module exceeds_input;endmodule\n",{},inputLimit);
    check(!out.issues.empty()&&out.issues.front().message.find("输入大小限制")!=std::string::npos,
          "physical input byte limits fail visibly without guessing a source line");
    out=run("`define F(x) `F(x)\nmodule m;`F(value) endmodule\n",{}, {100000,1024*1024,16,8});
    check(!out.issues.empty()&&out.issues.front().message.find("递归展开或嵌套过深")!=std::string::npos&&
          out.issues.front().location.line==0,"recursive function macro state is bounded and unknown diagnostic lines remain zero");
    {std::ofstream f(base/"guarded.vh");f<<"`ifndef GUARDED_132\n`define GUARDED_132\n`include \"guarded.vh\"\nwire guarded__;\n`endif\n";}
    out=run("module m;`include \"guarded.vh\"\nendmodule\n");
    const auto guardedNames=names(out);
    check(out.issues.empty()&&std::count(guardedNames.begin(),guardedNames.end(),"guarded__")==1,
          "legal include guards stop recursive inclusion without discarding the active declaration");
    out=run("module recovered;wire clean;endmodule\n");check(out.issues.empty()&&has(out,"clean"),"failed expansions leave no token state in the next call");
    std::vector<std::future<bool>> jobs;
    for(int i=0;i<8;++i) jobs.push_back(std::async(std::launch::async,[&,i]{
        const auto name="private_"+std::to_string(i);
        const auto value=run("`define NAME "+name+"\nmodule m;wire `NAME;endmodule\n");
        return value.issues.empty()&&has(value,name);
    }));
    for(auto& job:jobs) check(job.get(),"parallel analyzer calls do not share macro/token state");
    auto projected=parser::preprocess({root,core::HdlLanguage::SystemVerilog,
        "/*\n`include \"does_not_exist.vh\"\n`define COMMENT_ONLY\n*/\n`ifdef COMMENT_ONLY\nmodule no;endmodule\n`else\nmodule yes;endmodule\n`endif\n"});
    check(projected.issues.empty()&&projected.source.content().find("module no;")==std::string::npos&&
        projected.source.content().find("module yes;")!=std::string::npos,
        "position-preserving input also ignores directives hidden in comments");
    projected=parser::preprocess({root,core::HdlLanguage::SystemVerilog,
        "`define HEADER \"body.vh\"\nmodule m;\n`include `HEADER\nendmodule\n"});
    check(projected.issues.empty(),"legacy include validation does not reject a legal macro filename");
    std::filesystem::create_directories(base/"cwd");
    {std::ofstream f(base/"cwd"/"cwd_only.vh");f<<"wire from_cwd__;\n";}
    {
        const auto before=std::filesystem::current_path();
        struct Restore {std::filesystem::path path;~Restore(){std::error_code ec;std::filesystem::current_path(path,ec);}} restore{before};
        std::filesystem::current_path(base/"cwd");
        out=run("module m;`include \"cwd_only.vh\"\nendmodule\n");
        projected=parser::preprocess({root,core::HdlLanguage::SystemVerilog,"`include \"cwd_only.vh\"\n"});
        check(out.issues.empty()&&projected.issues.empty()&&has(out,"from_cwd__"),
              "both source paths honor the native initial working-directory include lookup");
    }
    const auto lint=[&](std::string text) {
        analysis::AnalysisOptions options;options.standardId="GJB10157-2021";
        return analysis::Analyzer(options).analyzeSource({root,core::HdlLanguage::SystemVerilog,std::move(text)});
    };
    const auto count=[](const auto& result,std::string_view rule) {
        return std::count_if(result.violations.begin(),result.violations.end(),[&](const auto& item){return item.ruleId==rule;});
    };
    auto linted=lint("`define NAME bad__\nmodule m;wire `NAME;endmodule\n");
    check(count(linted,"GJB10157.5.2.2")==1&&std::any_of(linted.violations.begin(),linted.violations.end(),[&](const auto& item){
        return item.ruleId=="GJB10157.5.2.2"&&item.file==root&&item.line==2&&item.column==1&&
            item.message=="bad__:禁止命名中使用连续的\"__\"";
    }),"R-2-2 macro-generated declaration retains native text, source line and absent-column fallback");
    linted=lint("`define ID(x) x\nmodule m;wire `ID(_bad_);endmodule\n");
    check(count(linted,"GJB10157.5.2.1")==1&&count(linted,"GJB10157.5.2.2")==1,
          "macro-generated names independently reach first-byte and trailing-underscore predicates");
    linted=lint("module m;\n`include \"body.vh\"\nendmodule\n");
    check(count(linted,"GJB10157.5.2.2")==1&&std::any_of(linted.violations.begin(),linted.violations.end(),[&](const auto& item){
        return item.ruleId=="GJB10157.5.2.2"&&item.file==base/"body.vh"&&item.line==1&&item.column==1;
    }),"include-generated declarations report the actual included file rather than the root source");
    linted=lint("`include \"defs.vh\"\n`ifdef ACTIVE\nmodule m;wire `NAME;endmodule\n`endif\n");
    check(count(linted,"GJB10157.5.2.2")==1,"macros defined in headers select and name root declarations end to end");
    linted=lint("`define DECL wire a__;wire b_;\nmodule m;\n`DECL\nendmodule\n");
    check(count(linted,"GJB10157.5.2.2")==2&&std::count_if(linted.violations.begin(),linted.violations.end(),[](const auto& item){
        return item.ruleId=="GJB10157.5.2.2"&&item.line==3;
    })==2,"multiple macro-generated declarations preserve per-node report multiplicity on one source line");
    linted=lint("module m;wire `UNKNOWN;endmodule\n");
    check(count(linted,"PP-FRONTEND-DIRECTIVE")>0&&count(linted,"GJB10157.5.2.2")==0,
          "failed expansion is visible and does not fabricate a target-rule match");
    check(std::any_of(linted.standardAssessments.begin(),linted.standardAssessments.end(),[](const auto& item){
        return item.clauseId=="GJB10157.5.2.2"&&item.state==analysis::StandardRuleState::ManualReviewRequired;
    }),"failed or alarm-free frontend input never becomes automatic compliance");
    linted=lint("`define HEADER \"body.vh\"\nmodule m;\n`include `HEADER\nendmodule\n");
    check(count(linted,"GJB10157.5.2.2")==1&&count(linted,"PP-INVALID-DIRECTIVE")==0,
        "macro include diagnostics agree with the expanded naming source");
    linted=lint("module first;`include \"body.vh\"\nendmodule\nmodule second;`include \"body.vh\"\nendmodule\n");
    check(count(linted,"GJB10157.5.2.2")==2,"identical included declarations in distinct module owners keep both native node reports");
    linted=lint("`define MAKE module same;wire duplicate__;endmodule\n`MAKE\n`MAKE\n");
    check(count(linted,"GJB10157.5.2.2")==0,"macro-generated duplicate module groups retain the recovered unique-module gate");
    linted=lint("`define NAME VDD\nmodule valid_module;wire `NAME;endmodule\n");
    check(count(linted,"GJB10157.5.2.4")==1,"macro-generated power names retain their independent native predicate");
    linted=lint("`define NAME x\nmodule valid_module;wire `NAME;endmodule\n");
    check(count(linted,"GJB10157.5.2.12")==1,"macro-generated short names retain fixed name-length limits");
    linted=lint("`define NAME lower_param\nmodule valid_module;parameter `NAME=1;endmodule\n");
    check(count(linted,"GJB10157.5.2.15")==1,"macro-generated parameters use the native constant naming-case selector");
    linted=lint("`define UPPER Data\n`define LOWER data\nmodule valid_module;wire `UPPER, `LOWER;endmodule\n");
    check(count(linted,"GJB10157.5.2.6")==1,"macro-generated case-only collisions retain module-local name comparison");
    out=run("`ifdef SYNTHESIS\nmodule active;wire synth_name;endmodule\n`else\nmodule inactive;endmodule\n`endif\n");
    check(out.issues.empty()&&has(out,"active")&&!has(out,"inactive"),
          "default native read_hdl selects SYNTHESIS, not simulation-only branches");
    out=run("`ifdef YOSYS\nmodule active;endmodule\n`endif\n`ifdef FORMAL\nmodule formal_only;endmodule\n`endif\n");
    check(out.issues.empty()&&has(out,"active")&&!has(out,"formal_only"),"default tool macro exists without inventing FORMAL mode");
    out=run("module m;localparam VALUE=`SYNTHESIS;endmodule\n",{{},{"SYNTHESIS=0"}});
    check(out.issues.empty()&&has(out,"1")&&!has(out,"0"),"implicit SYNTHESIS is applied after configured defines as in native execute");
    for(const auto directive:{"`undef SYNTHESIS\n","`resetall\n"}) {
        const auto text=std::string(directive)+"`ifdef SYNTHESIS\nmodule inactive;endmodule\n`else\nmodule active;endmodule\n`endif\n";
        out=run(text);projected=parser::preprocess({root,core::HdlLanguage::SystemVerilog,text});
        check(out.issues.empty()&&has(out,"active")&&!has(out,"inactive")&&
              projected.source.content().find("module active;")!=std::string::npos&&
              projected.source.content().find("module inactive;")==std::string::npos,
              "source undef/resetall removes default macros in both input paths");
    }
    linted=lint("`ifdef SYNTHESIS\nmodule valid_module;wire bad__;endmodule\n`else\nmodule valid_module;wire good_name;endmodule\n`endif\n");
    check(count(linted,"GJB10157.5.2.2")==1,"default synthesis branch reaches naming checks end to end");
    linted=lint("`define NAME SIGNAL\nmodule valid_module;wire `NAME;endmodule\n");
    check(count(linted,"GJB10157.5.2.3")==1&&std::any_of(linted.violations.begin(),linted.violations.end(),[&](const auto& item){
        return item.ruleId=="GJB10157.5.2.3"&&item.file==root&&item.line==2&&item.column==1&&
            item.message=="禁止使用VHDL关键字同名的SIGNAL"&&item.severity==analysis::Severity::Error;
    }),"R-2-3 macro names retain exact category text, severity and invocation line");
    {std::ofstream f(base/"keyword.vh");f<<"wire SIGNAL;\n";}
    linted=lint("module valid_module;\n`include \"keyword.vh\"\nendmodule\n");
    check(count(linted,"GJB10157.5.2.3")==1&&std::any_of(linted.violations.begin(),linted.violations.end(),[&](const auto& item){
        return item.ruleId=="GJB10157.5.2.3"&&item.file==base/"keyword.vh"&&item.line==1;
    }),"R-2-3 included names use the established lexer origin policy");
    linted=lint("`define MAKE module valid_module;wire SIGNAL;endmodule\n`MAKE\n");
    check(count(linted,"GJB10157.5.2.3")==1,"macro-created module roots enter the expanded owner index");
    {std::ofstream f(base/"keyword_defs.vh");f<<"`define ACTIVE_KEYWORD\n`define NAME SIGNAL\n";}
    linted=lint("`include \"keyword_defs.vh\"\n`ifdef ACTIVE_KEYWORD\nmodule valid_module;wire `NAME;endmodule\n`endif\n");
    check(count(linted,"GJB10157.5.2.3")==1,"header-defined conditionals no longer depend on the unexpanded structure index");
    const auto multi=[&](std::string a,std::string b) {
        analysis::AnalysisOptions options;options.standardId="GJB10157-2021";
        return analysis::Analyzer(options).analyzeSources({
            {root,core::HdlLanguage::SystemVerilog,std::move(a)},
            {base/"other.sv",core::HdlLanguage::SystemVerilog,std::move(b)}});
    };
    const std::string namesBody="wire _bad__,SIGNAL,VDD,x,Data,data;";
    linted=multi("module shared;"+namesBody+"endmodule\n","module shared;"+namesBody+"endmodule\n");
    for(const auto rule:{"GJB10157.5.2.1","GJB10157.5.2.2","GJB10157.5.2.3",
                         "GJB10157.5.2.4","GJB10157.5.2.6","GJB10157.5.2.12"})
        check(count(linted,rule)==0,"duplicate owner groups across files suppress only rules with the recovered uniqueness gate");
    linted=multi("`define MAKE module shared;wire SIGNAL;endmodule\n`MAKE\n",
                 "module shared;wire SIGNAL;endmodule\nmodule unique_owner;wire SIGNAL;endmodule\n");
    check(count(linted,"GJB10157.5.2.3")==1&&std::any_of(linted.violations.begin(),linted.violations.end(),[&](const auto& item){
        return item.ruleId=="GJB10157.5.2.3"&&item.file==base/"other.sv";
    }),"macro-created duplicate groups do not suppress a different unique owner");
    {std::ofstream f(base/"module.vh");f<<"module shared;wire SIGNAL;endmodule\n";}
    linted=multi("`include \"module.vh\"\n","module shared;wire SIGNAL;endmodule\n");
    check(count(linted,"GJB10157.5.2.3")==0,"included module roots participate in cross-file duplicate groups");
    linted=multi("module unique_owner;wire SIGNAL;endmodule\n","`ifdef NEVER_DEFINED\nmodule unique_owner;endmodule\n`endif\n");
    check(count(linted,"GJB10157.5.2.3")==1,"inactive roots do not manufacture duplicate owners");
    linted=multi("module unique_owner;wire SIGNAL;endmodule\n","`UNKNOWN\n");
    check(count(linted,"GJB10157.5.2.3")==0&&count(linted,"PP-FRONTEND-DIRECTIVE")>0,
        "an unexpanded source leaves the global owner index unproved, not uniquely bound by omission");
    check(std::any_of(linted.standardAssessments.begin(),linted.standardAssessments.end(),[](const auto& item){
        return item.clauseId=="GJB10157.5.2.3"&&item.state==analysis::StandardRuleState::ManualReviewRequired;
    }),"an unknown project owner index never certifies keyword compliance");
    linted=lint("module valid_module;wire SIGNAL;endmodule\n");
    check(count(linted,"GJB10157.5.2.3")==1,"a failed prior project does not poison the next analysis owner index");
    linted=multi("module shared;parameter lower_name=1;endmodule\n",
                 "module shared;parameter other_lower=1;endmodule\n");
    check(count(linted,"GJB10157.5.2.15")==0,
        "A-2-5 native entry also requires a unique AST module, across physical roots");
    linted=multi("module shared;parameter lower_name=1;endmodule\n",
                 "module shared;endmodule\nmodule unique_owner;parameter other_lower=1;endmodule\n");
    check(count(linted,"GJB10157.5.2.15")==1,
        "A-2-5 duplicate suppression preserves a different unique module and its own declaration selector");
    parser::FrontendPreprocessorSession session;
    const auto step=[&](std::string text,const parser::PreprocessorOptions& options=parser::PreprocessorOptions{}) {
        return session.expand({root,core::HdlLanguage::SystemVerilog,std::move(text)},options);
    };
    out=step("`define SHARED_NAME from_first__\n`define SHARED_ACTIVE\n");
    check(out.issues.empty(),"definition-only roots are valid session inputs");
    out=step("`ifdef SHARED_ACTIVE\nmodule active;wire `SHARED_NAME;endmodule\n`else\nmodule inactive;endmodule\n`endif\n");
    check(out.issues.empty()&&has(out,"from_first__")&&has(out,"active")&&!has(out,"inactive"),
        "a later root consumes definitions and conditional state from the previous root");
    check(std::find(out.initialMacroNames.begin(),out.initialMacroNames.end(),"SHARED_ACTIVE")!=out.initialMacroNames.end(),
        "initial macro snapshot records the merged native defined-name set");
    projected=parser::preprocess({root,core::HdlLanguage::SystemVerilog,
        "`ifdef SHARED_ACTIVE\nmodule active;wire `SHARED_NAME;endmodule\n`else\nmodule inactive;endmodule\n`endif\n"},{},&out);
    check(projected.issues.empty()&&projected.source.content().find("module active;")!=std::string::npos&&
        projected.source.content().find("module inactive;")==std::string::npos,
        "physical-position projection uses the same inherited branch configuration");
    out=step("`define ID(x=default__) wire x;\n");
    out=step("module m;`ID() endmodule\n");
    check(out.issues.empty()&&has(out,"default__"),"function bodies and formal defaults survive into a later root");
    out=step("`define JOIN(a,b) a``b\n");
    out=step("module m;wire `JOIN(joined,__);endmodule\n");
    check(out.issues.empty()&&has(out,"joined__"),"cached function macros preserve argument substitution and pasting");
    out=step("`undef SHARED_NAME\n`undef SHARED_ACTIVE\n");
    out=step("`ifdef SHARED_ACTIVE\nmodule inactive;endmodule\n`else\nmodule active;endmodule\n`endif\n");
    check(out.issues.empty()&&has(out,"active")&&!has(out,"inactive"),"undef in one root updates the next root's cache");
    out=step("module m;wire `SHARED_NAME;endmodule\n");
    check(!out.issues.empty(),"an undefined cached body is not resurrected in the next root");
    session.reset();
    out=step("`ifdef SHARED_ACTIVE\nmodule inactive;endmodule\n`else\nmodule active;endmodule\n`endif\n");
    check(out.issues.empty()&&has(out,"active")&&!has(out,"inactive"),"explicit reset clears session source definitions");
    out=step("`define NAME from_cache__\n");
    out=step("module m;wire `NAME;endmodule\n",{{},{"NAME=from_config"}});
    check(out.issues.empty()&&has(out,"from_cache__")&&!has(out,"from_config"),"cached source definitions take precedence over configured pre-defines");
    session.reset();
    out=step("module first;endmodule\n",{{},{"CONFIG_ONLY"}});
    out=step("`ifdef CONFIG_ONLY\nmodule inactive;endmodule\n`else\nmodule active;endmodule\n`endif\n");
    check(out.issues.empty()&&has(out,"active")&&!has(out,"inactive"),"configured definitions are not silently persisted as source cache entries");
    out=step("`undef CONFIG_ONLY\n",{{},{"CONFIG_ONLY"}});
    out=step("`ifdef CONFIG_ONLY\nmodule active;endmodule\n`endif\n",{{},{"CONFIG_ONLY"}});
    check(out.issues.empty()&&has(out,"active"),"the next read command reapplies its configured definitions after a prior source undef");
    out=step("`define OLD_NAME old__\n`resetall\n");
    out=step("`ifdef OLD_NAME\nmodule inactive;endmodule\n`else\nmodule active;endmodule\n`endif\n");
    check(out.issues.empty()&&has(out,"active")&&!has(out,"inactive"),"resetall clears the shared cache for later roots");
    out=step("`ifdef SYNTHESIS\nmodule active;endmodule\n`endif\n`ifdef YOSYS\nmodule tool_default;endmodule\n`endif\n");
    check(out.issues.empty()&&has(out,"active")&&has(out,"tool_default"),"new read commands restore implicit defaults after resetall");
    out=step("`define SYNTHESIS 0\n");
    out=step("module m;localparam VALUE=`SYNTHESIS;endmodule\n");
    check(out.issues.empty()&&has(out,"0")&&!has(out,"1"),"source-cached SYNTHESIS overrides the next command's implicit definition at cache merge");
    session.reset();
    out=step("module m;localparam VALUE=`YOSYS;endmodule\n",{{},{"YOSYS=0"}});
    check(out.issues.empty()&&has(out,"0"),"a clear boundary does not inject an extra cache entry over an explicit configured YOSYS value");
    out=step("`include \"defs.vh\"\n");
    out=step("`ifdef ACTIVE\nmodule m;wire `NAME;endmodule\n`endif\n");
    check(out.issues.empty()&&has(out,"from_header__"),"header definitions are retained across root boundaries");
    session.reset();
    {std::ofstream f(base/"once.vh");f<<"`ifndef ONCE_SESSION\n`define ONCE_SESSION\nmodule once_owner;wire SIGNAL;endmodule\n`endif\n";}
    out=step("`include \"once.vh\"\n");
    check(out.issues.empty()&&has(out,"once_owner"),"first session include enters a guarded header");
    out=step("`include \"once.vh\"\n");
    check(out.issues.empty()&&!has(out,"once_owner"),"a later root observes the previous include guard");
    session.reset();
    out=step("`include \"once.vh\"\n");
    check(out.issues.empty()&&has(out,"once_owner"),"reset reopens include guards for a new import session");
    out=step("`UNKNOWN\n");
    check(!out.issues.empty(),"failed session expansion is visible");
    session.reset();
    out=step("module recovered;endmodule\n");
    check(out.issues.empty()&&has(out,"recovered"),"reset after a failure leaves no stale token or macro state");
    linted=multi("`define NAME SIGNAL\n","module second_owner;\nwire `NAME;\nendmodule\n");
    check(count(linted,"GJB10157.5.2.3")==1&&std::any_of(linted.violations.begin(),linted.violations.end(),[&](const auto& item){
        return item.ruleId=="GJB10157.5.2.3"&&item.file==base/"other.sv"&&item.line==2&&item.column==1;
    }),"cross-root cached macro declarations report the consuming source and line");
    linted=multi("`define ACTIVE_PROJECT\n",
        "`ifdef ACTIVE_PROJECT\nmodule active_owner;wire bad__;endmodule\n`else\nmodule inactive_owner;wire good_name;endmodule\n`endif\n");
    check(count(linted,"GJB10157.5.2.2")==1&&std::any_of(linted.structure.begin(),linted.structure.end(),[](const auto& unit){return unit.name=="active_owner";})&&
        std::none_of(linted.structure.begin(),linted.structure.end(),[](const auto& unit){return unit.name=="inactive_owner";}),
        "generic structure and naming use the same inherited root-entry conditional state");
    linted=multi("`define HEADER \"keyword.vh\"\n","module owner;\n`include `HEADER\nendmodule\n");
    check(count(linted,"GJB10157.5.2.3")==1&&std::none_of(linted.violations.begin(),linted.violations.end(),[](const auto& item){return item.ruleId.starts_with("PP-");}),
        "macro include filenames from an earlier root are not revalidated in an isolated cache");
    linted=multi("`include \"once.vh\"\n","`include \"once.vh\"\n");
    check(count(linted,"GJB10157.5.2.3")==1,"project include guards prevent a fabricated duplicate-module group");
    linted=multi("`define MAKE module shared;wire SIGNAL;endmodule\n",
                 "`MAKE\n`MAKE\nmodule unique_owner;wire SIGNAL;endmodule\n");
    check(count(linted,"GJB10157.5.2.3")==1,"cached function-free module macros preserve duplicate exclusion and a unique sibling");
    linted=multi("`define SHARED_NAME SIGNAL\n`undef SHARED_NAME\n","module second_owner;wire `SHARED_NAME;endmodule\n");
    check(count(linted,"PP-FRONTEND-DIRECTIVE")>0&&count(linted,"GJB10157.5.2.3")==0,
        "an undefined cross-root macro remains an unresolved project input");
    linted=multi("module first_owner;wire `NAME;endmodule\n","`define NAME SIGNAL\n");
    check(count(linted,"PP-FRONTEND-DIRECTIVE")>0&&count(linted,"GJB10157.5.2.3")==0,
        "processing order is explicit; later definitions are not invented before use");
    linted=lint("module standalone;wire `NAME;endmodule\n");
    check(count(linted,"PP-FRONTEND-DIRECTIVE")>0,"a new Analyzer invocation cannot see the prior project's cache");
    jobs.clear();
    for(int i=0;i<8;++i) jobs.push_back(std::async(std::launch::async,[&,i] {
        parser::FrontendPreprocessorSession privateSession;
        const auto name="session_private_"+std::to_string(i);
        auto first=privateSession.expand({root,core::HdlLanguage::SystemVerilog,"`define NAME "+name+"\n"});
        const auto second=privateSession.expand({root,core::HdlLanguage::SystemVerilog,"module m;wire `NAME;endmodule\n"});
        return first.issues.empty()&&second.issues.empty()&&has(second,name);
    }));
    for(auto& job:jobs) check(job.get(),"parallel project sessions preserve private caches across multiple calls");
    const auto orderDir=base/"ordered_inputs";std::filesystem::create_directories(orderDir);
    {std::ofstream f(orderDir/"z_defs.sv");f<<"`define ORDERED_NAME SIGNAL\n";}
    {std::ofstream f(orderDir/"a_use.sv");f<<"module ordered_owner;wire `ORDERED_NAME;endmodule\n";}
    analysis::AnalysisOptions orderedOptions;orderedOptions.standardId="GJB10157-2021";
    linted=analysis::Analyzer(orderedOptions).analyzePaths({orderDir/"z_defs.sv",orderDir/"a_use.sv",orderDir/"z_defs.sv"});
    check(count(linted,"GJB10157.5.2.3")==1&&linted.filesAnalyzed==2&&
        std::none_of(linted.violations.begin(),linted.violations.end(),[](const auto& item){return item.ruleId.starts_with("PP-");}),
        "path imports preserve explicit definition-before-use order and first occurrence deduplication");
    linted=analysis::Analyzer(orderedOptions).analyzePaths({orderDir});
    check(count(linted,"PP-FRONTEND-DIRECTIVE")>0&&count(linted,"GJB10157.5.2.3")==0,
        "directory input remains deterministic path order, without inventing macro dependencies");
    {std::ofstream f(orderDir/"ordered.f");f<<"z_defs.sv\na_use.sv\n";}
    const auto listed=project::loadFileList(orderDir/"ordered.f");
    linted=analysis::Analyzer(orderedOptions).analyzePaths(listed.inputs);
    check(listed.errors.empty()&&count(linted,"GJB10157.5.2.3")==1,
        "simulator file-list import retains definition-before-use order through the analyzer");
    const analysis::Analyzer reusedAnalyzer(orderedOptions);
    linted=reusedAnalyzer.analyzeSources({{root,core::HdlLanguage::SystemVerilog,"`define SHOULD_NOT_LEAK SIGNAL\n"}});
    linted=reusedAnalyzer.analyzeSource({root,core::HdlLanguage::SystemVerilog,"module standalone;wire `SHOULD_NOT_LEAK;endmodule\n"});
    check(count(linted,"PP-FRONTEND-DIRECTIVE")>0&&count(linted,"GJB10157.5.2.3")==0,
        "even reuse of the same Analyzer object starts a fresh import session per call");
    std::cout<<checks<<" checks, "<<failures<<" failures\n";
    return failures?1:0;
}
