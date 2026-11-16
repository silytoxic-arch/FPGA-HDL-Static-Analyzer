#include "analysis/verilog_keyword_names.h"
#include <iostream>

int main() {
    using namespace fpga::analysis;
    int failures=0, assertions=0;
    const auto check=[&](bool condition, const char* label) {
        ++assertions;
        if (!condition) { ++failures; std::cerr << "FAIL: " << label << '\n'; }
    };
    const std::array subjects{VerilogNamingShapeSubject::Module, VerilogNamingShapeSubject::Net,
        VerilogNamingShapeSubject::Parameter, VerilogNamingShapeSubject::Localparam,
        VerilogNamingShapeSubject::Cell, VerilogNamingShapeSubject::Task, VerilogNamingShapeSubject::Function};
    const auto tableTests=[&](const auto& table, std::size_t index) {
        VerilogKeywordPolicy policy{true,{}};
        policy.enabled[index]=true;
        for (const auto key:table) {
            std::string upper(key);
            for (auto& c:upper) if (c>='a'&&c<='z') c=static_cast<char>(c-'a'+'A');
            for (const auto subject:subjects) {
                const auto match=checkVerilogKeywordName(upper,subject,policy);
                check(match.status==VerilogKeywordStatus::Matched && match.category==static_cast<VerilogKeywordCategory>(index),
                      "each retained table initializer matches all seven subject types after casefold");
            }
            check(checkVerilogKeywordName(std::string(key)+"_suffix", subjects[0], policy).status==VerilogKeywordStatus::NoMatch,
                  "exact table membership, never keyword prefix ban");
        }
    };
    tableTests(keyword_tables::Vhdl,0); tableTests(keyword_tables::Verilog,1);
    tableTests(keyword_tables::SystemVerilog,2); tableTests(keyword_tables::SdfEdif,3);
    tableTests(keyword_tables::Psl,4); tableTests(keyword_tables::Windows,5);
    check(keyword_tables::Vhdl.size()==115 && keyword_tables::Verilog.size()==126 &&
          keyword_tables::SystemVerilog.size()==117 && keyword_tables::SdfEdif.size()==1 &&
          keyword_tables::Psl.size()==67 && keyword_tables::Windows.size()==22, "all six native initializer counts retained");
    check(checkVerilogKeywordName("signal", subjects[0], {}).status==VerilogKeywordStatus::UnknownConfiguration,
          "vendor defaults unknown, no fabricated enabled categories");
    VerilogKeywordPolicy policy{true,{true,true,true,true,true,true}};
    check(checkVerilogKeywordName("alias",subjects[6],policy).category==VerilogKeywordCategory::Vhdl,
          "first enabled MATCH wins across overlapping categories");
    check(checkVerilogKeywordName("module",subjects[0],policy).category==VerilogKeywordCategory::Verilog,
          "enabled but nonmatching VHDL continues into Verilog");
    check(checkVerilogKeywordName("always_ff",subjects[0],policy).category==VerilogKeywordCategory::SystemVerilog,
          "multiple earlier nonmatching categories continue into SystemVerilog");
    check(checkVerilogKeywordName("clock",subjects[0],policy).category==VerilogKeywordCategory::Psl,
          "PSL reached after earlier enabled categories fail to match");
    check(checkVerilogKeywordName("CON",subjects[0],policy).category==VerilogKeywordCategory::Windows,
          "Windows reached after five earlier categories");
    policy.enabled[0]=false;
    check(checkVerilogKeywordName("alias",subjects[6],policy).category==VerilogKeywordCategory::SystemVerilog,
          "disabled overlapping category does not win");
    check(checkVerilogKeywordName("com10",subjects[0],policy).status==VerilogKeywordStatus::NoMatch &&
          checkVerilogKeywordName("con.txt",subjects[0],policy).status==VerilogKeywordStatus::NoMatch,
          "not an inferred Windows filename/device policy");
    VerilogKeywordPolicy sdf{true,{false,false,false,true,false,false}};
    for (const auto name:{"delayfile","edif","net","view"})
        check(checkVerilogKeywordName(name,subjects[0],sdf).status==VerilogKeywordStatus::NoMatch,
              "SDF/EDIF single comma string not silently split into words");
    check(checkVerilogKeywordName("signal",subjects[0],{true,{}}).status==VerilogKeywordStatus::NoMatch,
          "explicit all-off disables core matching");
    check(checkVerilogKeywordName("",subjects[0],policy).status==VerilogKeywordStatus::NoMatch,
          "empty normalized name skips native category chain");
    check(checkVerilogKeywordName("\xc3\xa9",subjects[0],policy).status==VerilogKeywordStatus::Unsupported,
          "non-ASCII locale-dependent names unproved");
    check(checkVerilogKeywordName(std::string("con\0",4),subjects[0],policy).status==VerilogKeywordStatus::Unsupported,
          "embedded NUL unproved");
    check(checkVerilogKeywordName("signal",VerilogNamingShapeSubject::Unsupported,policy).status==VerilogKeywordStatus::Unsupported,
          "unsupported declaration kinds excluded");
    for (std::size_t i=0;i<6;++i)
        check(parseVerilogKeywordCategory(VerilogKeywordCategoryNames[i])==static_cast<VerilogKeywordCategory>(i),
              "strict category parser maps all six keys");
    check(!parseVerilogKeywordCategory("VHDL") && !parseVerilogKeywordCategory("all"), "no implicit aliases/default all");
    std::cout << assertions << " keyword predicate assertions, " << failures << " failures\n";
    return failures ? 1 : 0;
}
