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
    check(wrap(same+same),2,4,false,"two clock and two control nodes preserve four ordered matches");
    const std::string repeatedControl="process(clk) begin if rising_edge(clk) then "
        "if clk='0' then dout<=din; elsif clk='1' then dout<=din; end if; end if; end process;";
    check(wrap(repeatedControl),1,2,false,"two EQ controls in one admitted process remain distinct");
    check(wrap(pure+repeatedControl),2,4,false,"an edge-only clock combines with both other control nodes");
    check(wrap("process(clk,rst) begin if clk='0' then dout<='0'; elsif rising_edge(clk) then "
        "if clk='1' then dout<='0'; end if; end if; end process;"),1,2,false,
        "outer class1 reset and direct constant inner reset both contribute");
    check(wrap("process(clk) begin if rising_edge(clk) then if clk=clk then dout<=din; "
        "end if; end if; end process;"),1,1,false,"two identifier operands still select only the first");
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
    check(wrap("g:if true generate "+same+" end generate;"),1,1,false,"generate descendants reach the native process collector");
    check(wrap("process(clk) begin if rising_edge(clk) then if '0'=clk then dout<=din; end if; end if; end process;"),1,1,false,"reversed EQ operand");
    check(wrap("process(clk) begin if rising_edge(clk) then if rst=clk then dout<=din; end if; end if; end process;"),1,0,false,"first identifier EQ selected not any operand");
    check(wrap("process(clk) begin if rising_edge(clk) then if CLK=RST then dout<=din; end if; end if; end process;"),1,1,false,"first identifier EQ case insensitive");
    check(wrap("process(clk) begin if rising_edge(clk) then if clk='0' then dout<=din; elsif rst='0' then dout<='0'; else dout<=din; end if; end if; end process;"),1,1,false,"class2 direct IF elsif conditions");
    check(wrap(same).replace(wrap(same).find("end entity;"),11,"end dut;"),1,1,false,"named entity end");
    check(wrap(same).replace(wrap(same).find("end entity;"),11,"end;"),1,1,false,"bare entity end");
    check(wrap(same,"use work.user_pkg.all;"),0,0,true,"architecture extra imported functions uncertain");
    check(wrap(same).replace(wrap(same).find("architecture"),0,"use work.user_pkg.all; "),0,0,true,"context imported function ambiguity");
    check(wrap(same).replace(wrap(same).find("end entity;"),0,"function rising_edge(x:std_logic) return boolean; "),0,0,true,"entity function shadow unknown");
    check(wrap("postponed "+same),1,1,false,"postponed preserves the admitted process shape");
    check("library ieee; use ieee.std_logic_1164.all; use ieee.std_logic_arith.all; "
          "use ieee.std_logic_unsigned.all; entity dut is port(clk,rst,din: in std_logic; "
          "dout:out std_logic); end entity; architecture rtl of dut is begin "+pure+
          " end architecture;",1,0,false,
          "packaged additional IEEE imports and adjacent context inheritance");
    {
        ++tests;
        core::SourceFile src("probe.vhd", core::HdlLanguage::Vhdl, wrap(pure));
        const auto out=analysis::scanVhdlClockControl(src,parser::tokenize(src));
        if(out.models.size()!=1||!out.overlaps.empty()) {
            ++failures; std::cerr<<"FAIL pure-edge constant action must not invent a control node\n";
        }
    }
    {
        ++tests;
        core::SourceFile src("probe.vhd", core::HdlLanguage::Vhdl,
            wrap("process(clk) begin if rising_edge(clk) then dout<=din; end if; end process;"));
        const auto out=analysis::scanVhdlClockControl(src,parser::tokenize(src));
        if(out.models.size()!=1||!out.overlaps.empty()) {
            ++failures; std::cerr<<"FAIL edge-triggered data assignment boundary\n";
        }
    }
    // Type nodes must not suppress unrelated clock/control relations. Alias
    // normalization is deliberately not supplied by a source-spelling guess.
    check(wrap(same,"type state_t is (IDLE,RUN); signal state:state_t;"),1,1,false,
        "ordinary architecture enumeration no longer rejects the complete unit");
    check(wrap(same,"TYPE state_t IS (Idle,Run);"),1,1,false,"enumeration keywords are case insensitive");
    check(wrap(other,"type state_t is (IDLE,RUN);"),1,0,false,"an enum cannot invent a clock/control pair");
    check(wrap(same,"subtype data_t is std_logic;"),1,1,false,"unused subtype leaves proven clocks visible");
    check(wrap(same,"subtype data_t is work.pkg.std_logic;"),1,1,false,
        "unused selected subtype marks do not change known signal bindings");
    check(wrap(same,"subtype clock_t is std_logic; signal clk:clock_t;"),1,1,false,
        "a previously bound unqualified IEEE subtype preserves identifier nodes");
    check(wrap(same,"subtype clock_t is work.pkg.std_logic; signal clk:clock_t;"),0,0,true,
        "a selected type ending in std_logic is not IEEE proof");
    check(wrap(same,"subtype first_t is std_logic; subtype second_t is first_t; signal clk:second_t;"),1,1,false,
        "ordered scalar subtype chains preserve the bound clock");
    check(wrap(same,"subtype clock_t is unknown_t; signal clk:clock_t;"),0,0,true,
        "a missing subtype base cannot inherit an outer object binding");
    check(wrap(same,"subtype clk is std_logic;"),0,0,true,"a subtype name hides an outer object");
    check(wrap(same,"type clk is (IDLE,RUN);"),0,0,true,"a type name hides an outer object");
    check(wrap(same,"type state_t is (clk,RUN);"),0,0,true,"an enum literal cannot reuse an outer signal binding");
    check(wrap(same,"type state_t is (IDLE,rst);"),1,1,false,"an unrelated hidden control name does not hide clk");
    check(wrap(same,"type state_t is (rising_edge,RUN);"),0,0,true,"overloaded enum edge names remain unknown");
    check(wrap(same,"type state_t is (std_logic,RUN);"),0,0,true,"enum literals cannot silently shadow IEEE types");
    check(wrap(same,"type std_logic is (IDLE,RUN);"),0,0,true,"local IEEE type replacement remains unknown");
    check(wrap(same,"type state_t is (IDLE,);"),0,0,true,"a trailing enum comma is unsupported");
    check(wrap(same,"type state_t is ();"),0,0,true,"empty enumerations cannot establish a declaration boundary");
    check(wrap(same,"type state_t is (IDLE,IDLE);"),0,0,true,"duplicate enum literals remain unknown");
    check(wrap(same,"type state_t is (IDLE,RUN)"),0,0,true,"missing enum semicolon does not swallow BEGIN");
    check(wrap(same,"subtype clock_t is ;"),0,0,true,"missing subtype mark remains unknown");
    check(wrap(same,"subtype clock_t is work..std_logic;"),0,0,true,"empty selected-name segments remain unknown");
    check(wrap(same,"subtype clock_t is std_logic range '0' to '1';"),0,0,true,
        "constraints are not mistaken for a supported bare subtype mark");
    check(wrap(same,"type state_t is record clk:std_logic; end record;"),0,0,true,
        "record declarations are not skipped using the first inner semicolon");
    check(wrap(same,"type state_t is ('0','1');"),0,0,true,"character enumeration lowering remains unknown");
    check(wrap(same,"type state_t is (IDLE,RUN); signal clk:state_t;"),0,0,true,
        "enum-typed clocks cannot inherit an outer scalar binding");
    check(wrap("b:block is signal clk:std_logic; begin "+same+" end block;","subtype clk is std_logic;"),1,1,false,
        "ordered declarations bind a later explicit signal independently");
    check(wrap(same,"signal clk:std_logic; subtype clk is std_logic;"),0,0,true,
        "a later subtype must hide an earlier signal");
    check(wrap("b:block is type state_t is (IDLE,RUN); begin "+same+" end block;"),1,1,false,
        "block enums preserve inherited clock bindings");
    check(wrap("g:if true generate subtype data_t is std_logic; begin "+same+" end generate;"),1,1,false,
        "generate subtypes preserve unrelated inherited clocks");
    check(wrap("b:block is type state_t is (clk,RUN); begin "+same+" end block;"+same),1,1,true,
        "local enum hiding is restored after the block");
    check(wrap("b:block is subtype clk is std_logic; begin "+same+" end block;"+same),1,1,true,
        "local subtype hiding is restored after the block");
    check(wrap("b:block is type state_t is record x:integer; end record; begin "+same+" end block;"+same),1,1,true,
        "unsupported type bodies do not erase proven outer diagnostics");
    check(wrap(same,"type state_t is (IDLE,RUN); signal clk:state_t;")+wrap(same),1,1,true,
        "type declarations and their unknown uses do not leak across architectures");
    check(wrap(same,"subtype clock_t is std_ulogic; signal clk:clock_t;"),1,1,false,
        "std_ulogic subtypes bind to the same source scalar subset");
    check(wrap(same,"SUBTYPE Clock_T IS STD_LOGIC; SIGNAL CLK:clock_t;"),1,1,false,
        "subtype visibility and uses are case insensitive");
    check(wrap(same,"subtype clock_t is std_logic; signal clk:clock_t := '0';"),1,1,false,
        "scalar subtype signals retain default initializers");
    check(wrap(same,"subtype clock_t is std_logic; signal clk:clock_t register;"),1,1,false,
        "scalar subtype signals retain signal kinds");
    check(wrap(other,"subtype clock_t is std_logic; signal clk:clock_t;"),1,0,false,
        "a subtype clock does not invent equality reuse with another reset");
    check(wrap(same,"subtype clock_t is bit; signal clk:clock_t;"),0,0,true,
        "a bit subtype cannot inherit the hidden IEEE port type");
    check(wrap(same,"subtype clock_t is std_logic_vector; signal clk:clock_t;"),0,0,true,
        "a vector subtype is not guessed to be scalar");
    check(wrap(same,"subtype clock_t is ieee.std_logic_1164.std_logic; signal clk:clock_t;"),0,0,true,
        "selected type visibility remains outside this unqualified source adapter");
    check(wrap(same,"subtype first_t is second_t; subtype second_t is std_logic; signal clk:first_t;"),0,0,true,
        "a later alias cannot retroactively bind an earlier forward mark");
    check(wrap(same,"subtype first_t is second_t; subtype second_t is first_t; signal clk:second_t;"),0,0,true,
        "cyclic aliases cannot manufacture a scalar base");
    check(wrap(same,"subtype clock_t is clock_t; signal clk:clock_t;"),0,0,true,
        "a self alias without a proven distinct base remains unknown");
    check(wrap(same,"signal clk:clock_t; subtype clock_t is std_logic;"),0,0,true,
        "object types are bound at the declaration point rather than after a full-file scan");
    check(wrap(same,"subtype clock_t is std_logic; subtype clock_t is std_ulogic; signal clk:clock_t;"),0,0,true,
        "duplicate same-frame subtype declarations cannot silently overwrite type bindings");
    check(wrap("b:block is signal clk:clock_t; begin "+same+" end block;",
        "subtype clock_t is std_logic;"),1,1,false,"blocks inherit an outer subtype binding");
    check(wrap("g:for i in 0 to 1 generate signal clk:clock_t; begin "+same+" end generate;",
        "subtype clock_t is std_logic;"),1,1,false,"generate bodies inherit an outer subtype binding once");
    check(wrap("b:block is subtype local_t is clock_t; signal clk:local_t; begin "+same+" end block;",
        "subtype clock_t is std_logic;"),1,1,false,"child subtype chains resolve a previously visible outer base");
    check(wrap("b:block is subtype clock_t is bit; signal clk:clock_t; begin "+same+" end block;"
        " c:block is signal clk:clock_t; begin "+same+" end block;",
        "subtype clock_t is std_logic;"),1,1,true,
        "a child non-scalar subtype cannot leak into its sibling frame");
    check(wrap("b:block is constant clock_t:integer:=0; signal clk:clock_t; begin "+same+" end block;"
        " c:block is signal clk:clock_t; begin "+same+" end block;",
        "subtype clock_t is std_logic;"),1,1,true,
        "an object hides a subtype locally and the sibling restores the alias");
    check(wrap("g:for clock_t in 0 to 1 generate signal clk:clock_t; begin "+same+" end generate;"+same,
        "subtype clock_t is std_logic;"),1,1,true,
        "a generate iterator hides an outer type binding only inside its own frame");
    check(wrap("b:block is type state_t is (clock_t,RUN); signal clk:clock_t; begin "+same+" end block;"+same,
        "subtype clock_t is std_logic;"),1,1,true,
        "an enum literal cannot retain an outer same-spelling subtype binding");
    check(wrap("b:block is type clock_t is (IDLE,RUN); signal clk:clock_t; begin "+same+" end block;"+same,
        "subtype clock_t is std_logic;"),1,1,true,
        "a derived type replacement hides the outer scalar subtype");
    check(wrap("b:block is signal clk:clock_t; begin "+same+" end block;"
        " c:block is signal clk:clock_t; begin "+same+" end block;",
        "subtype clock_t is std_logic;"),2,2,true,
        "equal subtype bases do not identify distinct local clock objects");
    check(wrap("b:block is signal clk:clock_t; begin "+same+same+" end block;",
        "subtype clock_t is std_logic;"),2,4,false,
        "same-owner subtype clocks preserve the complete ordered pair multiplicity");
    check(wrap(same,"subtype clock_t is std_logic; signal clk:clock_t;")+wrap(same,"signal clk:clock_t;"),1,1,true,
        "an architecture scalar alias never leaks into a later design unit");
    const auto postponed=std::string("p: postponed ")+same.substr(0,same.find("end process;"))+"end postponed process p;";
    check(wrap(postponed),1,1,false,"named END POSTPONED PROCESS closes exactly once");
    check(wrap(postponed+same),2,4,false,"adjacent postponed and ordinary processes preserve cross pairs");
    check(wrap(same+postponed),2,4,false,"ordinary process before postponed has independent bounds");
    check(wrap(postponed)+wrap(same),2,2,false,"postponed closure cannot leak into a second architecture");
    check(wrap("g: if false generate "+same+" end generate g;"),1,1,false,
        "retained native AST traversal does not evaluate a generate guard");
    check(wrap("g: for i in 0 to 7 generate "+same+" end generate g;"),1,1,false,
        "retained generate body is visited once rather than elaborated eight times");
    check(wrap("g: for clk in 0 to 1 generate "+same+" end generate;"),0,0,true,
        "integer iterator hides the outer IEEE scalar clock binding");
    check(wrap("g: for rst in 0 to 1 generate "+same+" end generate;"),1,1,false,
        "an unused iterator shadow does not hide an unrelated clock");
    check(wrap("g: for rising_edge in 0 to 1 generate "+same+" end generate;"),0,0,true,
        "an iterator cannot inherit the IEEE edge-function identity");
    check(wrap("g: if true generate begin "+same+" end generate;"),1,1,false,
        "an optional BEGIN with no declarations preserves inherited names");
    check(wrap("g: if true generate constant N: integer := 1; begin "+same+" end generate;"),1,1,false,
        "unreferenced generate constants do not destroy visible scalar bindings");
    check(wrap("g: if true generate signal enable: std_logic; begin "+same+" end generate;"),1,1,false,
        "explicit scalar declarations are bound inside a generate body");
    check(wrap("g: if true generate signal clk: std_logic := '0'; begin "+same+" end generate;"),1,1,false,
        "an initialized local scalar owns its clock/control references");
    check(wrap("b: block begin "+same+" end block b;"),1,1,false,
        "named block descendants are reached");
    check(wrap("b: block (clk='1') is begin "+same+" end block;"),1,1,false,
        "a block guard does not prune native descendant traversal");
    check(wrap("b: block is signal clk: std_logic; begin "+same+" end block;"),1,1,false,
        "local IEEE scalar binding is restored inside a block");
    check(wrap("b: block is signal clk: bit; begin "+same+" end block;"),0,0,true,
        "a local non-IEEE clock never inherits the outer IEEE clock type");
    check(wrap("b: block is constant clk: integer := 0; begin "+same+" end block;"),0,0,true,
        "a constant shadow cannot be treated as the outer scalar signal");
    check(wrap("b: block is signal clk: bit; begin "+same+" end block;"+same),1,1,true,
        "an unsupported local binding does not suppress a valid outer process");
    check(wrap("b: block is signal clk: std_logic; begin "+same+" end block;"+same),2,2,true,
        "equal-spelling distinct local objects do not acquire unverified cross-scope pairs");
    check(wrap("b: block is type std_logic is range 0 to 1; begin "+same+" end block;"+same),1,1,true,
        "an unknown local type scope is isolated from outer valid processes");
    check(wrap("b: block is function rising_edge(x:std_logic) return boolean is begin "
        "return true; end; begin "+same+" end block;"+same,"","end;"),1,1,true,
        "a bare function end in an unsupported block cannot close the architecture");
    check(wrap("b: block is signal rising_edge: integer; begin "+same+" end block;"+same),1,1,true,
        "an object shadowing the edge function invalidates only its local frame");
    check(wrap("g: if true generate b: block begin h: for i in 0 to 1 generate "+
        postponed+" end generate h; end block b; end generate g;"),1,1,false,
        "nested generate/block/postponed scopes retain one source process");
    check(wrap("g: if true generate "+same+" end generate; b: block begin "+same+" end block;"),2,4,false,
        "inherited clock identities still combine across containers");
    check(wrap("g: if true generate "+same+" elsif false generate "+same+" end generate;"+same),1,1,true,
        "unestablished alternative-generate grammar is isolated from outer valid processes");
    check(wrap("g: case 0 generate when others => "+same+" end generate;"+same),1,1,true,
        "unestablished case-generate grammar does not inherit IF/FOR admission");
    check(wrap("g: if true generate "+same),0,0,true,
        "an unterminated generate cannot prove an owning architecture boundary");
    check(wrap("b: block begin "+same),0,0,true,
        "an unterminated block does not expose a false root scope");
    check(wrap("b: block is generic(N:integer:=1); begin "+same+" end block;"+same),1,1,true,
        "unsupported block interface declarations cannot reuse outer bindings");
    check(wrap("b: block is signal clk: std_logic; begin assert true report \"end block; process\"; "+
        same+" end block;"),1,1,false,"quoted container words do not affect balanced boundaries");
    auto localClock=same;
    for (std::size_t pos=0;(pos=localClock.find("clk",pos))!=std::string::npos;pos+=9)
        localClock.replace(pos,3,"local_clk");
    check(wrap("a: block is signal local_clk: std_logic; begin "+localClock+" end block;"
        "b: block is signal local_clk: std_logic; begin "+localClock+" end block;"),2,2,true,
        "same-spelling sibling local objects keep their proven per-owner matches");
    check(wrap(same,"signal clk: bit;"),0,0,true,
        "architecture declarations cannot inherit a hidden entity scalar type");
    check(wrap(same,"constant clk: integer := 0;"),0,0,true,
        "architecture constants do not retain a hidden signal binding");
    check(wrap(same,"signal enable: std_logic := '0';"),1,1,false,
        "initialized unrelated architecture scalars do not suppress a known process");
    auto vectorFirst=wrap(same);
    vectorFirst.insert(vectorFirst.find("port(")+5,"bus_data: in std_logic_vector(7 downto 0); ");
    check(vectorFirst,1,1,false,"a constrained bus port cannot truncate later scalar port bindings");
    auto defaultPort=wrap(same);
    const auto portPosition=defaultPort.find("clk,rst,din: in std_logic;");
    defaultPort.replace(portPosition,std::string("clk,rst,din: in std_logic;").size(),
        "clk,rst,din: in std_logic := '0';");
    check(defaultPort,1,1,false,"defaulted explicit scalar ports retain their IEEE bindings");
    check(wrap("g: if generate "+same+" end generate;"+same),1,1,true,
        "an empty generate condition cannot establish an admitted child frame");
    check(wrap("g: for i in generate "+same+" end generate;"+same),1,1,true,
        "a missing discrete range cannot establish a generated process source");
    std::cout<<tests<<" cases, "<<failures<<" failures\n";
    return failures?1:0;
}
