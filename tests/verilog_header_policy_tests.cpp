#include "analysis/verilog_header_policy.h"

#include <iostream>
#include <string>
#include <vector>

int main() {
    using namespace fpga;
    using namespace analysis;
    int failures = 0;
    const auto check = [&](std::string text, std::size_t moduleLine, VerilogHeaderPolicy policy,
                           VerilogHeaderPolicyStatus status, bool missingHeader,
                           std::vector<std::string> missing, const char* label) {
        core::SourceFile source("probe.sv",core::HdlLanguage::SystemVerilog,std::move(text));
        const auto result = evaluateVerilogHeaderPolicy(source,moduleLine,policy);
        if (result.status != status || result.missingHeader != missingHeader || result.missingKeywords != missing) {
            ++failures; std::cerr << "FAIL: " << label << '\n';
        }
    };
    constexpr auto complete = VerilogHeaderPolicyStatus::Complete;
    constexpr auto incomplete = VerilogHeaderPolicyStatus::Incomplete;
    const VerilogHeaderPolicy author{true,{{"Author",false}}};
    check("module m;",1,{},VerilogHeaderPolicyStatus::UnknownConfiguration,false,{},"no fabricated native defaults");
    check("module m;",1,{true,{}},VerilogHeaderPolicyStatus::Disabled,false,{},"known empty map disables check");
    check("module m;",1,author,complete,true,{},"module on first line lacks prefix data");
    check("\n\nmodule m;",3,author,complete,true,{},"empty physical prefix lines ignored");
    check(" \nmodule m;",2,author,complete,false,{"Author"},"whitespace is nonempty prefix data, not trim");
    check("\r\nmodule m;",2,author,complete,false,{"Author"},"carriage return retained as nonempty line");
    check("// author: \nmodule m;",2,author,complete,false,{},"ASCII case-insensitive keyword; empty value accepted");
    check("// author: name\nmodule m;",2,{true,{{"Author",true}}},complete,false,{"Author"},"case-sensitive entry preserved");
    check("// Author: name\nmodule m;",2,{true,{{"Author",true}}},complete,false,{},"case-sensitive entry match");
    check("// coAuthorSuffix\nmodule m;",2,author,complete,false,{},"substring rather than whole word/field");
    check("`define TEXT \"Author\"\nmodule m;",2,author,complete,false,{},"native prefix is not comment-only");
    check("// filename: probe.sv\n// Description:\nmodule m;",3,
          {true,{{"Filename",false},{"Description",true},{"History",false}}},complete,false,{"History"},
          "explicit map requirement controls missing list");
    check("// no configured fields\nmodule m;",2,
          {true,{{"Zulu",false},{"alpha",false},{"Author",false}}},complete,false,
          {"Author","Zulu","alpha"},"native ordered map determines missing-key order");
    check("// Author\nmodule m;\n// History\n",2,{true,{{"History",false}}},complete,false,{"History"},
          "module line and later body not searched");
    check("// Au\n// thor\nmodule m;",3,author,complete,false,{"Author"},"keyword cannot span physical lines");
    check("// Author\nmodule m;\nendmodule\nmodule n;",4,author,complete,false,{},
          "caller-selected later module uses all preceding physical lines");
    check("// Author\nmodule m;",2,{true,{{"",false}}},complete,false,{},"empty keyword permitted as explicit map key");
    check("// Author\nmodule m;",2,{true,{{"Author",false},{"Author",true}}},incomplete,false,{},"duplicate map keys ambiguous");
    check("// \xc3\xa9 Author\nmodule m;",2,author,incomplete,false,{},"non-ASCII line requires Windows ACP context");
    check("// Author\nmodule m;",2,{true,{{"\xc3\xa9",false}}},incomplete,false,{},"non-ASCII key unsupported subset");
    check(std::string("// Author\0other\nmodule m;",26),2,author,incomplete,false,{},"embedded NUL not guessed");
    check("// Author\nmodule m;",0,author,incomplete,false,{},"invalid one-based module line");
    check("// Author\nmodule m;",3,author,incomplete,false,{},"module line beyond input");
    check("",1,author,incomplete,false,{},"empty source cannot establish module identity");

    const auto checkVhdl = [&](std::string text, std::size_t declarationLine,
                               VerilogHeaderPolicy policy,
                               VerilogHeaderPolicyStatus status, bool missingHeader,
                               std::vector<std::string> missing, const char* label) {
        core::SourceFile source("probe.vhd", core::HdlLanguage::Vhdl, std::move(text));
        const auto result = evaluateVhdlHeaderPolicy(source, declarationLine, policy);
        if (result.status != status || result.missingHeader != missingHeader ||
            result.missingKeywords != missing) {
            ++failures; std::cerr << "FAIL: " << label << '\n';
        }
    };
    checkVhdl("entity m is end;", 1, {}, VerilogHeaderPolicyStatus::UnknownConfiguration,
              false, {}, "VHDL unknown map does not fabricate defaults");
    checkVhdl("entity m is end;", 1, {true, {}}, VerilogHeaderPolicyStatus::Disabled,
              false, {}, "VHDL known empty map disables check");
    checkVhdl("-- author: name\nentity m is end;", 2, author, complete, false, {},
              "VHDL insensitive configured keyword matches prefix line");
    checkVhdl("-- Author\nentity m is end;", 2, {true, {{"History", false}}},
              complete, false, {"History"}, "VHDL missing configured keyword reports");
    checkVhdl("-- Au\n-- thor\nentity m is end;", 3, author, complete, false,
              {"Author"}, "VHDL keyword cannot span physical lines");
    checkVhdl("entity m is end;", 1, author, complete, true, {},
              "VHDL first-line design unit lacks prefix data");
    checkVhdl("-- Author\nentity m is end;", 2, {true, {{"Author", true}}},
              complete, false, {}, "VHDL case-sensitive configured keyword matches");
    return failures == 0 ? 0 : 1;
}
