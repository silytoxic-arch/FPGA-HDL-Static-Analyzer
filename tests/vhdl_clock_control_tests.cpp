#include "analysis/vhdl_clock_control.h"
#include <iostream>

int main() {
    using namespace fpga;
    int failures=0, tests=0;
    const std::string ctx="library ieee; use ieee.std_logic_1164.all; ";
    const auto wrap=[&](const std::string& body,const std::string& decl="",const std::string& ending="end architecture;") {
        return ctx+"entity dut is port(clk,rst,din: in std_logic; dout:out std_logic); end entity; "+ctx+
            "architecture rtl of dut is "+decl+" begin "+body+" "+ending;
    };
    const auto check=[&](const std::string& text,std::size_t models,std::size_t overlaps,bool incomplete,const char* label) {
        ++tests;
        core::SourceFile src("probe.vhd",core::HdlLanguage::Vhdl,text);
        const auto tokens=parser::tokenize(src);
        const auto out=analysis::scanVhdlClockControl(src,tokens);
        if (out.models.size()!=models||out.overlaps.size()!=overlaps||out.incomplete!=incomplete||!out.partial) {
            ++failures; std::cerr<<"FAIL "<<label<<" models="<<out.models.size()<<" overlaps="<<out.overlaps.size()<<" incomplete="<<out.incomplete<<'\n';
        }
        for(const auto& overlap:out.overlaps)
            if(overlap.clockToken>=tokens.size()||overlap.equalityIdentifierToken>=tokens.size()) ++failures;
    };
    const std::string pure="process(clk) begin if rising_edge(clk) then dout<='0'; end if; end process;";
    const std::string same="process(clk) begin if rising_edge(clk) then if clk='0' then dout<=din; end if; end if; end process;";
    const std::string other="process(clk) begin if rising_edge(clk) then if rst='0' then dout<='0'; end if; end if; end process;";
    check(wrap(pure),1,0,false,"pure constant edge isn't control reuse");
    check(wrap(same),1,1,false,"class2 nested same clock EQ");
    check(wrap(other),1,0,false,"ordinary reset different clock");
    check(wrap("process(clk) begin if clk'event and clk='1' then if clk='0' then dout<='0'; end if; end if; end process;"),1,1,false,"legacy edge own EQ not automatically counted");
    check(wrap("process(clk,rst) begin if clk='0' then dout<='0'; elsif rising_edge(clk) then dout<=din; end if; end process;"),1,1,false,"class1 reset-control clock");
    check(wrap("process(clk,rst) begin if rst='0' then dout<='0'; elsif rising_edge(clk) then dout<=din; end if; end process;"),1,0,false,"class1 distinct reset");
    check(wrap("process(clk,rst) begin if rst='0' then dout<='0'; elsif rising_edge(clk) then if clk='0' then dout<='0'; end if; end if; end process;"),1,1,false,"class1 appended constant inner condition");
    check(wrap("process(clk,rst) begin if rst='0' then dout<='0'; elsif rising_edge(clk) then if clk='0' then dout<=din; end if; end if; end process;"),0,0,true,"class1 nonconstant inner unsupported not compliant");
    const std::string otherClock="process(rst) begin if rising_edge(rst) then if clk='0' then dout<=din; end if; end if; end process;";
    check(wrap(pure+otherClock),2,1,false,"cross admitted process intersection");
    check(wrap(pure)+wrap(otherClock),2,0,false,"architectures never cross intersect");
    check(wrap(same,"","end rtl;"),1,1,false,"named architecture end");
    check(wrap(same,"","end;"),1,1,false,"bare architecture end balanced process");
    check(wrap(same,"signal enable : std_logic;"),1,1,false,"scalar signal declaration");
    check(wrap("process(clk) begin if rising_edge(clk) then if enable='0' then dout<=din; end if; end if; end process;","signal enable : std_logic;"),1,0,false,"bound scalar control");
    check(wrap("process(clk) begin if rising_edge(clk) then if missing='0' then dout<=din; end if; end if; end process;"),0,0,true,"undeclared signal not guessed");
    check(wrap("process(clk) begin if rising_edge(clk) then if rst='0' then dout<=din; end if; if clk='0' then dout<=din; end if; end if; end process;"),0,0,true,"multiple edge body IF incomplete");
    check(wrap(pure.substr(0,pure.find("end process"))+"if clk='0' then dout<=din; end if; end process;"),0,0,true,"multiple top-level IF not class2");
    check(wrap("process(clk) variable clk:std_logic; begin if rising_edge(clk) then dout<='0'; end if; end process;"),0,0,true,"local process declarations rejected");
    check(wrap(same,"function rising_edge(x:std_logic) return boolean is begin return true; end function;"),0,0,true,"local edge function shadow rejected");
    check(wrap(same,"subtype std_logic is integer;"),0,0,true,"type shadow rejected");
    check("entity dut is port(clk:in std_logic; dout:out std_logic); end entity; architecture rtl of dut is begin "+pure+" end architecture;",0,0,true,"missing IEEE context");
    check(wrap("process(clk) begin if rising_edge(clk(0)) then dout<='0'; end if; end process;"),0,0,true,"selected clock incomplete");
    check(wrap("process(clk) begin if clk='1' and clk'event then dout<='0'; end if; end process;"),1,0,false,"swapped legacy AND admitted");
    check(wrap("process(clk) begin if rising_edge(clk) then if clk='0' and rst='0' then dout<=din; end if; end if; end process;"),0,0,true,"descendant EQ not direct condition");
    check(wrap("process(clk) begin if rising_edge(clk) then dout<=din; if clk='0' then dout<=din; end if; end if; end process;"),0,0,true,"non-first inner IF cannot widen native scope");
    check(wrap("g:if true generate "+same+" end generate;"),0,0,true,"generate scope unsupported");
    check(wrap("process(clk) begin if rising_edge(clk) then if '0'=clk then dout<=din; end if; end if; end process;"),1,1,false,"reversed EQ operand");
    check(wrap("process(clk) begin if rising_edge(clk) then if rst=clk then dout<=din; end if; end if; end process;"),1,0,false,"first identifier EQ selected not any operand");
    check(wrap("process(clk) begin if rising_edge(clk) then if CLK=RST then dout<=din; end if; end if; end process;"),1,1,false,"first identifier EQ case insensitive");
    check(wrap("process(clk) begin if rising_edge(clk) then if clk='0' then dout<=din; elsif rst='0' then dout<='0'; else dout<=din; end if; end if; end process;"),1,1,false,"class2 direct IF elsif conditions");
    check(wrap(same).replace(wrap(same).find("end entity;"),11,"end dut;"),1,1,false,"named entity end");
    check(wrap(same).replace(wrap(same).find("end entity;"),11,"end;"),1,1,false,"bare entity end");
    check(wrap(same,"use work.user_pkg.all;"),0,0,true,"architecture extra imported functions uncertain");
    check(wrap(same).replace(wrap(same).find("architecture"),0,"use work.user_pkg.all; "),0,0,true,"context imported function ambiguity");
    check(wrap(same).replace(wrap(same).find("end entity;"),0,"function rising_edge(x:std_logic) return boolean; "),0,0,true,"entity function shadow unknown");
    check(wrap("postponed "+same),0,0,true,"postponed process not widened");
    check("library ieee; use ieee.std_logic_1164.all; use ieee.std_logic_arith.all; "
          "use ieee.std_logic_unsigned.all; entity dut is port(clk,rst,din: in std_logic; "
          "dout:out std_logic); end entity; architecture rtl of dut is begin "+pure+
          " end architecture;",1,0,false,
          "packaged additional IEEE imports and adjacent context inheritance");
    {
        ++tests;
        core::SourceFile src("probe.vhd", core::HdlLanguage::Vhdl, wrap(pure));
        const auto out=analysis::scanVhdlClockControl(src,parser::tokenize(src));
        if(out.models.size()!=1||!out.models.front().clockDrivesOnlyConstants) {
            ++failures; std::cerr<<"FAIL packaged pure-edge constant action\n";
        }
    }
    {
        ++tests;
        core::SourceFile src("probe.vhd", core::HdlLanguage::Vhdl,
            wrap("process(clk) begin if rising_edge(clk) then dout<=din; end if; end process;"));
        const auto out=analysis::scanVhdlClockControl(src,parser::tokenize(src));
        if(out.models.size()!=1||out.models.front().clockDrivesOnlyConstants) {
            ++failures; std::cerr<<"FAIL edge-triggered data assignment boundary\n";
        }
    }
    std::cout<<tests<<" cases, "<<failures<<" failures\n";
    return failures?1:0;
}
