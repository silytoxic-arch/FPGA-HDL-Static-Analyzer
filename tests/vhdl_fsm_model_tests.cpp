#include "analysis/vhdl_fsm_model.h"

#include <iostream>
#include <string>

int main() {
    int failures = 0;
    const auto extraction = [](const std::string& text) {
        fpga::core::SourceFile source("fsm_probe.vhd", fpga::core::HdlLanguage::Vhdl, text);
        const auto tokens = fpga::parser::tokenize(source);
        return fpga::analysis::extractBoundedVhdlFsmModels(source, tokens);
    };
    const auto check = [&](const std::string& text, std::size_t models, const char* label,
                           bool unresolved = false) {
        const auto result = extraction(text);
        if (result.models.size() != models || (unresolved && result.unresolvedReasons.empty())) {
            ++failures;
            std::cerr << "FAIL " << label << " models " << result.models.size()
                      << " unresolved " << result.unresolvedReasons.size() << '\n';
        }
        for (const auto& model : result.models) if (!model.partial) {
            ++failures; std::cerr << "FAIL model must retain partial coverage\n";
        }
    };
    const std::string declarations =
        "library ieee; use ieee.std_logic_1164.all;"
        "entity unit is port(clk,rst:in std_logic); end entity;"
        "library ieee; use ieee.std_logic_1164.all;"
        "architecture rtl of unit is type state_t is (idle, active, done);"
        "signal state : state_t := idle; begin ";
    const std::string head = declarations +
        "process(clk,rst) begin if rst='1' then state<=idle;"
        "elsif rising_edge(clk) then case state is ";
    const std::string tail = "end case; end if; end process; end architecture;";
    const std::string body =
        "when idle => state<=active; when active => state<=done; when done => state<=idle; ";
    check(head + body + tail, 1, "clocked enum signal with genuine state transitions");
    const auto model = extraction(head + body + tail).models.at(0);
    if (model.transitions.size() != 3 || model.sourceStates.size() != 3 ||
        model.destinationStates.size() != 3 || model.declarationInitialStates != std::vector<std::string>{"idle"} ||
        model.outsideCaseAssignments != std::vector<std::string>{"idle"}) {
        ++failures; std::cerr << "FAIL graph/initialization provenance\n";
    }
    const std::string twoProcess =
        "library ieee; use ieee.std_logic_1164.all; entity unit is "
        "port(clk,rst:in std_logic); end entity; "
        "library ieee; use ieee.std_logic_1164.all; use ieee.numeric_std.all; "
        "architecture rtl of unit is type state_t is (idle,active,done); "
        "signal state:state_t; signal next_state:state_t; begin "
        "process(state) begin case state is "
        "when idle=>next_state<=active; when active=>next_state<=done; "
        "when done=>next_state<=idle; "
        "end case; end process; "
        "process(clk,rst) begin if rst='1' then state<=idle; "
        "elsif rising_edge(clk) then state<=next_state; end if; end process; "
        "end architecture;";
    check(twoProcess, 1, "bounded two-process enum FSM with explicit next-state transfer");
    const auto twoProcessModel = extraction(twoProcess).models;
    if (twoProcessModel.size() != 1 || twoProcessModel[0].sourceStates.size() != 3 ||
        twoProcessModel[0].transitions.size() != 3 || twoProcessModel[0].clock != "clk" ||
        twoProcessModel[0].outsideCaseAssignments != std::vector<std::string>{"idle"}) {
        ++failures; std::cerr << "FAIL two-process graph and reset provenance\n";
    }
    const auto replaceTwoProcess = [](std::string text, const std::string& from,
                                      const std::string& to) {
        text.replace(text.find(from), from.size(), to); return text;
    };
    check(replaceTwoProcess(twoProcess, "state<=next_state;", "state<=done;"), 0,
          "two-process model requires explicit next-state transfer", true);
    check(replaceTwoProcess(twoProcess, "end architecture;",
          "next_state<=idle; end architecture;"), 0,
          "two-process next-state multiple driver rejected", true);
    check(replaceTwoProcess(twoProcess, "elsif rising_edge(clk) then state<=next_state;",
          "else state<=next_state;"), 0,
          "unguarded next-state transfer rejected", true);
    check(declarations + "end architecture;", 0, "enum and signal alone are not FSM");
    check(head + "when idle=>output<=active; when active=>output<=done;"
          "when done=>output<=idle;" + tail, 0, "case decoder does not update state");
    check(head + "when idle=>state<=state; when active=>state<=done;"
          "when done=>state<=idle;" + tail, 1, "explicit self loop");
    check(head + "when idle=>state<=active; when active=>state<=done;"
          "when others=>state<=idle;" + tail, 1, "default transition retained separately");
    const auto withDefault = extraction(head + "when idle=>state<=active; when others=>state<=done;" + tail);
    if (withDefault.models.size() != 1 || withDefault.models[0].sourceStates != std::vector<std::string>{"idle"} ||
        withDefault.models[0].destinationStates != std::vector<std::string>{"active","done"}) {
        ++failures; std::cerr << "FAIL source vs destination-only state distinction\n";
    }
    check(head + "when idle=>state<=active; when active=>state<=state_t'val(n);"
          "when done=>state<=idle;" + tail, 0, "dynamic target unsupported", true);
    check(head + "when idle=>state<=other_state; when active=>state<=done;"
          "when done=>state<=idle;" + tail, 0, "unknown/runtime target unsupported", true);
    check(head + "when idle=>null; when active=>state<=done; when done=>state<=idle;" + tail,
          0, "implicit hold remains unresolved", true);
    check(head + "when idle=>state<=active; when active=>state<=done;" + tail,
          0, "incomplete case without others", true);
    check(head + "when idle|active=>state<=done; when done=>state<=idle;" + tail,
          0, "multiple labels conservative exclusion", true);
    check(head + body + tail.substr(0, tail.find("end architecture;")) +
          "process(clk) begin if rising_edge(clk) then state<=idle; end if; end process;"
          "end architecture;", 0, "multiple state drivers rejected", true);
    check(head + body + tail.substr(0, tail.find("end architecture;")) +
          "state<=idle; end architecture;", 0, "concurrent state driver rejected", true);
    check(declarations + "process(clk) variable state:state_t; begin if rising_edge(clk) then "
          "case state is " + body + tail, 0, "process local shadowing", true);
    check(declarations + "process(state) begin case state is " + body +
          "end case; end process; end architecture;", 0, "combinational enum feedback not clocked FSM", true);
    check(head + "when idle=>case mode is when others=>state<=active; end case;"
          "when active=>state<=done; when done=>state<=idle;" + tail, 0,
          "nested case unresolved", true);
    check(head + "when idle=>if go='1' then state<=active; else state<=idle; end if;"
          "when active=>state<=done; when done=>state<=idle;" + tail, 1,
          "conditional may-transitions remain partial");
    check("LIBRARY IEEE; USE IEEE.STD_LOGIC_1164.ALL; ENTITY UNIT IS PORT(CLK:IN STD_ULOGIC); END ENTITY;"
          "LIBRARY IEEE; USE IEEE.STD_LOGIC_1164.ALL;"
          "ARCHITECTURE RTL OF UNIT IS TYPE STATE_T IS (IDLE, ACTIVE, DONE);"
          "SIGNAL STATE:STATE_T; BEGIN PROCESS(CLK) BEGIN IF RISING_EDGE(CLK) THEN"
          " CASE STATE IS WHEN IDLE=>STATE<=ACTIVE; WHEN ACTIVE=>STATE<=DONE;"
          " WHEN DONE=>STATE<=IDLE; END CASE; END IF; END PROCESS; END RTL;", 1,
          "case insensitive VHDL and named architecture end");
    check("architecture rtl of unit is type state_t is (idle,active); signal state:state_t;"
          "begin end rtl; architecture alternate of unit is begin process(clk) begin"
          "if rising_edge(clk) then case state is when idle=>state<=active;"
          "when active=>state<=idle; end case; end if; end process; end alternate;", 0,
          "enum and signal declarations do not leak between architectures");
    check(head + "//not VHDL comment" + body + tail, 0, "unsupported syntax not accepted", true);
    check(head + "when idle=>state<=active; -- state<=mystery;\n"
          "when active=>state<=done; when done=>state<=idle;" + tail, 1,
          "VHDL comments excluded");
    check(head + body + "end case; end if; end process; end;", 1,
          "bare architecture end supported");
    check(declarations + "process(clk) begin report boolean'image(rising_edge(clk)); case state is " +
          body + "end case; end process; end architecture;", 0,
          "non-guard edge call cannot admit model", true);
    check(declarations + "process(clk) begin if rising_edge(clk) then null; end if; case state is " +
          body + "end case; end process; end architecture;", 0,
          "edge guard elsewhere cannot admit case", true);
    check(declarations + "process(clk) function rising_edge(x:bit) return boolean is "
          "begin return true; end function; begin if rising_edge(clk) then case state is " +
          body + tail, 0, "locally overloaded edge function unresolved", true);
    check(head + body + "end case; end if; end; end architecture;", 0,
          "bare process end not confused with architecture scope");
    const auto replace = [](std::string text, const std::string& from, const std::string& to) {
        text.replace(text.find(from), from.size(), to); return text;
    };
    check(replace(head, "clk,rst:in std_logic", "clk,rst:in bit") + body + tail,
          0, "non IEEE scalar clock type rejected", true);
    check(replace(head, "clk,rst:in std_logic", "clk,rst:in std_logic_vector(1 downto 0)") + body + tail,
          0, "clock vector not scalar binding", true);
    check(replace(head, "use ieee.std_logic_1164.all;", "") + body + tail,
          0, "missing explicit IEEE use unresolved", true);
    check("use work.custom.all;" + head + body + tail,
          0, "extra package import may overload clock unresolved", true);
    check(replace(head, "process(clk,rst) begin", "process(clk,rst) variable clk:std_logic; begin") +
          body + tail, 0, "clock local shadow rejected", true);
    check(replace(head, "architecture rtl of unit is", "architecture rtl of unit is subtype std_logic is bit;") +
          body + tail, 0, "IEEE scalar type shadow rejected", true);
    check(replace(head, "port(clk,rst:in std_logic)", "port(rst:in std_logic)") + body + tail,
          0, "undeclared clock name unresolved", true);
    check(replace(replace(head, "port(clk,rst:in std_logic)", "port(rst:in std_logic)"),
          "signal state : state_t := idle;", "signal clk:std_ulogic; signal state : state_t := idle;") +
          body + tail, 1, "architecture scalar signal clock explicitly bound");
    check(replace(head, "entity unit is", "entity unit is generic(clk:std_logic);") + body + tail,
          0, "generic shadow/binding requires elaboration", true);
    check(replace(head, "library ieee; use ieee.std_logic_1164.all;entity", "entity") + body + tail,
          0, "architecture use does not authorize preceding entity", true);
    check(replace(head, "end entity;library ieee; use ieee.std_logic_1164.all;", "end entity;") +
          body + tail + "library ieee; use ieee.std_logic_1164.all;", 1,
          "same-file matching entity context follows packaged T-Linter FSM recovery");
    check("library ieee; use ieee.std_logic_1164.all;entity unrelated is end entity;" +
          replace(head, "library ieee; use ieee.std_logic_1164.all;entity", "entity") + body + tail,
          0, "other design unit context cannot authorize entity", true);
    const auto attrHead = [&](const std::string& value, const std::string& target = "state") {
        return replace(head, "signal state : state_t := idle;", "signal state : state_t := idle;"
            " attribute fsm_encoding:string; attribute fsm_encoding of " + target + ":signal is \"" + value + "\";");
    };
    for (const auto& [value, encoding, safe] : std::vector<std::tuple<std::string,int,bool>>{
            {"one-hot",1,false}, {"prefix_ONEHOT_suffix",1,false}, {"gray",2,false},
            {"sequential",0,false}, {"safe",0,true}, {"GRAY safe",2,false},
            {"OneHot",0,false}, {"unknown",0,false}}) {
        const auto r = extraction(attrHead(value) + body + tail);
        if (r.models.size() != 1 || r.models[0].encoding != encoding ||
            r.models[0].safeEncoding != safe || !r.models[0].encodingExplicit ||
            !r.models[0].encodingResolved || !r.models[0].clockBindingComplete || r.models[0].clock != "clk") {
            ++failures; std::cerr << "FAIL bound encoding " << value << '\n';
        }
    }
    const auto unrelated = extraction(attrHead("onehot", "other") + body + tail);
    if (unrelated.models.size() != 1 || unrelated.models[0].encodingExplicit || unrelated.models[0].encoding != 0) {
        ++failures; std::cerr << "FAIL unrelated attribute must not bind to control\n";
    }
    const auto ambiguousAttr = extraction(replace(attrHead("gray"),
        "attribute fsm_encoding:string;", "attribute fsm_encoding:integer;") + body + tail);
    if (ambiguousAttr.models.size() != 1 || ambiguousAttr.models[0].encodingResolved) {
        ++failures; std::cerr << "FAIL unknown attribute declaration unresolved\n";
    }
    const auto synAttrHead = [&](const std::string& value, bool declare = false) {
        return replace(head, "signal state : state_t := idle;", "signal state : state_t := idle;" +
            std::string(declare ? " attribute syn_encoding:string;" : "") +
            " attribute syn_encoding of state:signal is \"" + value + "\";");
    };
    for (const auto& [value, encoding] : std::vector<std::pair<std::string, int>>{
            {"safe, onehot", 1}, {"safe, sequential", 0}}) {
        for (const bool declare : {false, true}) {
            const auto r = extraction(synAttrHead(value, declare) + body + tail);
            if (r.models.size() != 1 || r.models[0].encoding != encoding ||
                !r.models[0].encodingExplicit || !r.models[0].encodingResolved) {
                ++failures; std::cerr << "FAIL syn_encoding alias " << value << '\n';
            }
        }
    }
    const auto invalidSynDeclaration = extraction(replace(synAttrHead("onehot", true),
        "attribute syn_encoding:string;", "attribute syn_encoding:integer;") + body + tail);
    if (invalidSynDeclaration.models.size() != 1 || invalidSynDeclaration.models[0].encodingResolved) {
        ++failures; std::cerr << "FAIL invalid syn_encoding declaration unresolved\n";
    }
    const auto conflictingAliases = extraction(replace(attrHead("onehot"),
        "attribute fsm_encoding:string;",
        "attribute fsm_encoding:string; attribute syn_encoding of state:signal is \"sequential\";") +
        body + tail);
    if (conflictingAliases.models.size() != 1 || conflictingAliases.models[0].encodingResolved) {
        ++failures; std::cerr << "FAIL conflicting encoding aliases unresolved\n";
    }
    const std::string packagedTwoProcess =
        "library ieee; use ieee.std_logic_1164.all; use ieee.numeric_std.all;"
        "entity pkg is port(clk,rst,sel:in std_logic; q:out std_logic); end entity;"
        "architecture rtl of pkg is type state_t is(idle,s1,s2);"
        "signal state:state_t; signal next_state:state_t;"
        "attribute syn_encoding of state:signal is \"safe, onehot\"; begin "
        "process(state,sel) begin case state is "
        "when idle=>q<='0';if sel='1' then next_state<=s1;else next_state<=s2;end if;"
        "when s1=>q<='0';next_state<=s2; when s2=>q<='1';next_state<=idle;"
        "when others=>q<='0';next_state<=idle; end case; end process;"
        "process(rst,clk) begin if rst='1' then state<=idle;"
        "elsif rising_edge(clk) then state<=next_state;end if;end process;end architecture;";
    const auto packagedModel = extraction(packagedTwoProcess);
    if (packagedModel.models.size() != 1 || packagedModel.models[0].encoding != 1 ||
        !packagedModel.models[0].encodingResolved) {
        ++failures; std::cerr << "FAIL packaged syn_encoding two-process shape";
        for (const auto& reason : packagedModel.unresolvedReasons) std::cerr << " | " << reason;
        std::cerr << '\n';
    }
    std::string oversizedMalformed =
        "library ieee; use ieee.std_logic_1164.all; entity unit is port(clk:in std_logic); end entity;"
        "architecture rtl of unit is type state_t is (idle,s1,s1";
    for (int i = 2; i <= 41; ++i) oversizedMalformed += ",s" + std::to_string(i);
    oversizedMalformed += "); signal state:state_t; signal next_state:state_t; begin "
        "process(state) begin case state is when idle=>next_state<=s1;";
    for (int i = 1; i <= 41; ++i)
        oversizedMalformed += "when x" + std::to_string(i) + "=>next_state<=idle;";
    oversizedMalformed += "when others=>next_state<=idle; end case; end process;"
        "process(clk) begin if rising_edge(clk) then state<=next_state; end if; end process; end architecture;";
    const auto oversizedMalformedResult = extraction(oversizedMalformed);
    if (!oversizedMalformedResult.models.empty() ||
        oversizedMalformedResult.oversizedStateCandidates.size() != 1 ||
        oversizedMalformedResult.oversizedStateCandidates[0].sourceStates.size() != 42) {
        ++failures; std::cerr << "FAIL oversized source-count candidate must survive enum/label defects\n";
    }
    return failures == 0 ? 0 : 1;
}
