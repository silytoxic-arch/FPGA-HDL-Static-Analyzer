#include "analysis/analyzer.h"
#include "analysis/verilog_keyword_names.h"

#include <algorithm>
#include <array>
#include <iostream>
#include <ranges>
#include <string>
#include <tuple>
#include <vector>

// Regressions derived from the attachment's published HTML positive/negative
// examples, plus metamorphic comment, line-break and assignment controls.
// These verify our implementation, not the proprietary binary's algorithm.
int main() {
    using namespace fpga;
    int failures = 0;
    const auto check = [&](bool ok, const char* message) {
        if (!ok) { ++failures; std::cerr << "FAIL: " << message << '\n'; }
    };
    const auto analyze = [&](std::string text, bool vhdl = false,
                             analysis::FsmOptimizationPriority priority = analysis::FsmOptimizationPriority::Unspecified) {
        analysis::AnalysisOptions options;
        options.standardId = vhdl ? "GJB9765-2020" : "GJB10157-2021";
        options.fsmOptimizationPriority = priority;
        return analysis::Analyzer(options).analyzeSource({vhdl ? "probe.vhd" : "probe.sv",
            vhdl ? core::HdlLanguage::Vhdl : core::HdlLanguage::SystemVerilog, std::move(text)});
    };
    const auto count = [](const analysis::AnalysisResult& result, const char* rule) {
        return std::ranges::count_if(result.violations, [&](const auto& violation) {
            return violation.ruleId == rule;
        });
    };
    const auto analyzeVerilog = [&](std::string text) {
        analysis::AnalysisOptions options;
        options.standardId = "GJB10157-2021";
        return analysis::Analyzer(options).analyzeSource(
            {"probe.v", core::HdlLanguage::Verilog, std::move(text)});
    };
    const auto analyzeVerilogSources = [&](std::string first, std::string second) {
        analysis::AnalysisOptions options;
        options.standardId = "GJB10157-2021";
        return analysis::Analyzer(options).analyzeSources({
            {"first.v", core::HdlLanguage::Verilog, std::move(first)},
            {"second.v", core::HdlLanguage::Verilog, std::move(second)}});
    };
    const auto analyzeMixedSources = [&](std::string verilog, std::string vhdl) {
        analysis::AnalysisOptions options;
        options.standardId = "GJB10157-2021";
        return analysis::Analyzer(options).analyzeSources({
            {"main.v", core::HdlLanguage::Verilog, std::move(verilog)},
            {"aux.vhd", core::HdlLanguage::Vhdl, std::move(vhdl)}});
    };
    const auto analyzeVhdlAt = [&](std::string path, std::string text) {
        analysis::AnalysisOptions options;
        options.standardId = "GJB9765-2020";
        return analysis::Analyzer(options).analyzeSource(
            {std::move(path), core::HdlLanguage::Vhdl, std::move(text)});
    };
    const auto analyzeVhdlSources = [&](std::string child, std::string parent) {
        analysis::AnalysisOptions options;
        options.standardId = "GJB9765-2020";
        return analysis::Analyzer(options).analyzeSources({
            {"child.vhd", core::HdlLanguage::Vhdl, std::move(child)},
            {"parent.vhd", core::HdlLanguage::Vhdl, std::move(parent)}});
    };
    const auto analyzeThreeVhdlSources = [&](std::string leaf, std::string middle,
                                              std::string parent) {
        analysis::AnalysisOptions options;
        options.standardId = "GJB9765-2020";
        return analysis::Analyzer(options).analyzeSources({
            {"leaf.vhd", core::HdlLanguage::Vhdl, std::move(leaf)},
            {"middle.vhd", core::HdlLanguage::Vhdl, std::move(middle)},
            {"parent.vhd", core::HdlLanguage::Vhdl, std::move(parent)}});
    };
    const auto portWidthExample = [&](std::string childPorts, std::string connections) {
        return analyzeVerilog(
            "module child(" + childPorts + "); endmodule\n"
            "module parent(input [2:0] A, input [1:0] B, input [2:0] C, input [1:0] D);\n"
            "child u0(" + connections + ");\nendmodule\n");
    };
    {
        const auto outputRead = analyze(
            "entity probe is port(a,b,clk: in bit; q,r: out bit; io: inout bit); end entity;\n"
            "architecture rtl of probe is begin\n"
            "q <= a;\n"
            "r <= b and Q;\n"
            "io <= q;\n"
            "end architecture;\n", true);
        check(count(outputRead, "GJB9765.4.2.b") == 2,
              "4.2.b detects case-insensitive output-port reads on assignment RHS");
        const auto noOutputRead = analyze(
            "entity probe is port(a,clk: in bit; q: out bit; io: inout bit); end entity;\n"
            "architecture rtl of probe is signal intermediate: bit; begin\n"
            "intermediate <= a; q <= intermediate; io <= a;\n"
            "end architecture;\n", true);
        check(count(noOutputRead, "GJB9765.4.2.b") == 0,
              "4.2.b excludes output targets, inout ports and intermediate-signal use");
        const auto attributeRead = analyze(
            "entity probe is port(a: in bit; q: out bit); end entity;\n"
            "architecture rtl of probe is signal internal: bit; begin\n"
            "internal <= q'event; q <= a;\n"
            "end architecture;\n", true);
        check(count(attributeRead, "GJB9765.4.2.b") == 0,
              "4.2.b preserves the native attribute-node exclusion");
        const auto relationalOnly = analyze(
            "entity probe is port(q: out bit); end entity;\n"
            "architecture rtl of probe is begin\n"
            "p: process is variable v: boolean; begin\n"
            "if q <= q then null; end if;\n"
            "while q <= q loop exit; end loop;\n"
            "assert q <= q report \"condition\";\n"
            "wait until q <= q;\n"
            "report boolean'image(q <= q);\n"
            "v := q <= q;\n"
            "end process; end architecture;\n", true);
        check(count(relationalOnly, "GJB9765.4.2.b") == 0,
              "4.2.b does not interpret relational <= as an assignment");
        const auto guardedAssignment = analyze(
            "entity probe is port(a: in bit; q,r: out bit); end entity;\n"
            "architecture rtl of probe is begin\n"
            "p: process begin\n"
            "if a = '1' then r <= q; end if;\n"
            "end process; end architecture;\n", true);
        check(count(guardedAssignment, "GJB9765.4.2.b") == 1,
              "4.2.b retains a genuine assignment after then");
    }
    {
        const auto multipleSignals = analyze(
            "entity m is port(arst, clk1, clk2, clk3, d : in bit; q : out bit); end entity;\n"
            "architecture rtl of m is begin\n"
            "p: process(arst, clk1, clk2, clk3) begin\n"
            "if arst = '1' then q <= '0';\n"
            "elsif rising_edge(clk1) then q <= d;\n"
            "elsif rising_edge(clk2) then q <= not d;\n"
            "elsif falling_edge(clk3) then q <= d; end if;\n"
            "end process; end architecture;\n", true);
        check(count(multipleSignals, "GJB9765.4.9.d") == 2,
              "4.9.d reports each additional clock signal after the process baseline");
        const auto oppositeEdges = analyze(
            "entity m is port(arst, clk, d : in bit; q : out bit); end entity;\n"
            "architecture rtl of m is begin p: process(arst, clk) begin\n"
            "if arst = '1' then q <= '0';\n"
            "elsif rising_edge(clk) then q <= d;\n"
            "elsif falling_edge(CLK) then q <= not d; end if;\n"
            "end process; end architecture;\n", true);
        check(count(oppositeEdges, "GJB9765.4.9.d") == 1,
              "4.9.d treats opposite edges of one case-insensitive VHDL clock as a conflict");
        const auto duplicateSameEdge = analyze(
            "entity m is port(arst, clk, d : in bit; q : out bit); end entity;\n"
            "architecture rtl of m is begin p: process(arst, clk) begin\n"
            "if arst = '1' then q <= '0';\n"
            "elsif rising_edge(clk) then q <= d;\n"
            "elsif rising_edge(CLK) then q <= not d; end if;\n"
            "end process; end architecture;\n", true);
        check(count(duplicateSameEdge, "GJB9765.4.9.d") == 0,
              "4.9.d deduplicates repeated same-name same-edge relations");
        const auto separateProcesses = analyze(
            "entity m is port(clk1, clk2, d : in bit; q1, q2 : out bit); end entity;\n"
            "architecture rtl of m is begin\n"
            "p1: process(clk1) begin if rising_edge(clk1) then q1 <= d; end if; end process;\n"
            "p2: process(clk2) begin if rising_edge(clk2) then q2 <= d; end if; end process;\n"
            "end architecture;\n", true);
        check(count(separateProcesses, "GJB9765.4.9.d") == 0,
              "4.9.d keeps independent clocks in separate processes compliant");
        const auto unprovenAsyncAction = analyze(
            "entity m is port(arst, clk1, clk2, d : in bit; q : out bit); end entity;\n"
            "architecture rtl of m is begin p: process(arst, clk1, clk2) begin\n"
            "if arst = '1' then q <= d;\n"
            "elsif rising_edge(clk1) then q <= '0';\n"
            "elsif rising_edge(clk2) then q <= '1'; end if;\n"
            "end process; end architecture;\n", true);
        check(count(unprovenAsyncAction, "GJB9765.4.9.d") == 0,
              "4.9.d excludes native relation class 4 with an unproven asynchronous action");
        const auto genericAsyncAction = analyze(
            "entity m is generic(reset_value : bit := '0'); "
            "port(arst, clk1, clk2, d : in bit; q : out bit); end entity;\n"
            "architecture rtl of m is begin p: process(arst, clk1, clk2) begin\n"
            "if arst = '1' then q <= reset_value;\n"
            "elsif rising_edge(clk1) then q <= d;\n"
            "elsif rising_edge(clk2) then q <= not d; end if;\n"
            "end process; end architecture;\n", true);
        check(count(genericAsyncAction, "GJB9765.4.9.d") == 1,
              "4.9.d admits an owner-entity generic in the native static action set");
        const auto architectureConstantAction = analyze(
            "entity m is port(arst, clk1, clk2, d : in bit; q : out bit); end entity;\n"
            "architecture rtl of m is constant reset_value : bit := '0'; begin\n"
            "p: process(arst, clk1, clk2) begin\n"
            "if arst = '1' then q <= reset_value;\n"
            "elsif rising_edge(clk1) then q <= d;\n"
            "elsif rising_edge(clk2) then q <= not d; end if;\n"
            "end process; end architecture;\n", true);
        check(count(architectureConstantAction, "GJB9765.4.9.d") == 1,
              "4.9.d admits an owning-architecture constant in the native static action set");
        const auto enumLiteralAction = analyze(
            "entity m is port(arst, clk1, clk2 : in bit); end entity;\n"
            "architecture rtl of m is type state_t is (idle, run); signal state : state_t; begin\n"
            "p: process(arst, clk1, clk2) begin\n"
            "if arst = '1' then state <= idle;\n"
            "elsif rising_edge(clk1) then state <= run;\n"
            "elsif rising_edge(clk2) then state <= idle; end if;\n"
            "end process; end architecture;\n", true);
        check(count(enumLiteralAction, "GJB9765.4.9.d") == 1,
              "4.9.d admits an owning-architecture enum literal in the native static action set");
        const auto processLocalConstantAction = analyze(
            "entity m is port(arst, clk1, clk2, d : in bit; q : out bit); end entity;\n"
            "architecture rtl of m is begin p: process(arst, clk1, clk2)\n"
            "constant reset_value : bit := '0'; begin\n"
            "if arst = '1' then q <= reset_value;\n"
            "elsif rising_edge(clk1) then q <= d;\n"
            "elsif rising_edge(clk2) then q <= not d; end if;\n"
            "end process; end architecture;\n", true);
        check(count(processLocalConstantAction, "GJB9765.4.9.d") == 0,
              "4.9.d keeps process-local constants outside the per-root native value tree");
        const auto localPackageConstantAction = analyze(
            "entity m is port(arst, clk1, clk2, d : in bit; q : out bit); end entity;\n"
            "architecture rtl of m is package p is constant reset_value : bit := '0'; "
            "end package; use p.all; begin\n"
            "p0: process(arst, clk1, clk2) begin\n"
            "if arst = '1' then q <= reset_value;\n"
            "elsif rising_edge(clk1) then q <= d;\n"
            "elsif rising_edge(clk2) then q <= not d; end if;\n"
            "end process; end architecture;\n", true);
        check(count(localPackageConstantAction, "GJB9765.4.9.d") == 0,
              "4.9.d excludes constants nested below a local package declaration");
        const auto literalConcatAction = analyze(
            "entity m is port(arst, clk1, clk2 : in bit; q : out bit_vector(1 downto 0)); "
            "end entity;\narchitecture rtl of m is begin p: process(arst, clk1, clk2) begin\n"
            "if arst = '1' then q <= '0' & '1';\n"
            "elsif rising_edge(clk1) then q <= \"00\";\n"
            "elsif rising_edge(clk2) then q <= \"11\"; end if;\n"
            "end process; end architecture;\n", true);
        check(count(literalConcatAction, "GJB9765.4.9.d") == 1,
              "4.9.d admits a native AST_CONCAT whose direct children are all literals");
        const auto othersAggregateAction = analyze(
            "entity m is port(arst, clk1, clk2, d : in bit; "
            "q : out bit_vector(1 downto 0)); end entity;\n"
            "architecture rtl of m is begin p: process(arst, clk1, clk2) begin\n"
            "if arst = '1' then q <= (others => d);\n"
            "elsif rising_edge(clk1) then q <= \"00\";\n"
            "elsif rising_edge(clk2) then q <= \"11\"; end if;\n"
            "end process; end architecture;\n", true);
        check(count(othersAggregateAction, "GJB9765.4.9.d") == 1,
              "4.9.d mirrors the native first-child AST_OTHERS acceptance branch");
        const auto rhsEdgeCall = analyze(
            "entity m is port(clk1, clk2 : in bit; q : out boolean); end entity;\n"
            "architecture rtl of m is begin p: process(clk1) begin\n"
            "if rising_edge(clk1) then q <= rising_edge(clk2); end if;\n"
            "end process; end architecture;\n", true);
        check(count(rhsEdgeCall, "GJB9765.4.9.d") == 0,
              "4.9.d ignores edge-function calls outside AST condition relations");
        const auto category2ElsifIgnored = analyze(
            "entity m is port(clk1, clk2, d : in bit; q : out bit); end entity;\n"
            "architecture rtl of m is begin p: process(clk1, clk2) begin\n"
            "if rising_edge(clk1) then q <= d;\n"
            "elsif rising_edge(clk2) then q <= not d; end if;\n"
            "end process; end architecture;\n", true);
        check(count(category2ElsifIgnored, "GJB9765.4.9.d") == 0,
              "4.9.d category 2 keeps only the outer IF main-condition clock");
        const auto category2NestedIgnored = analyze(
            "entity m is port(clk1, clk2, d : in bit; q : out bit); end entity;\n"
            "architecture rtl of m is begin p: process(clk1, clk2) begin\n"
            "if rising_edge(clk1) then\n"
            "if rising_edge(clk2) then q <= d; end if;\n"
            "end if; end process; end architecture;\n", true);
        check(count(category2NestedIgnored, "GJB9765.4.9.d") == 0,
              "4.9.d does not promote a linked nested IF clock into category 2");
        const auto separateTopLevelIfs = analyze(
            "entity m is port(clk1, clk2, d : in bit; q : out bit); end entity;\n"
            "architecture rtl of m is begin p: process(clk1, clk2) begin\n"
            "q <= d;\n"
            "if rising_edge(clk1) then q <= d; end if;\n"
            "if rising_edge(clk2) then q <= not d; end if;\n"
            "end process; end architecture;\n", true);
        check(count(separateTopLevelIfs, "GJB9765.4.9.d") == 1,
              "4.9.d compares separate top-level source IF relations even after another statement");
        const auto loopNestedIfIgnored = analyze(
            "entity m is port(clk1, clk2, d : in bit; q : out bit); end entity;\n"
            "architecture rtl of m is begin p: process(clk1, clk2) begin\n"
            "if rising_edge(clk1) then q <= d; end if;\n"
            "for i in 0 to 0 loop\n"
            "if rising_edge(clk2) then q <= not d; end if;\n"
            "end loop; end process; end architecture;\n", true);
        check(count(loopNestedIfIgnored, "GJB9765.4.9.d") == 0,
              "4.9.d excludes IF statements nested below a top-level loop");
        const auto elseLinkedClock = analyze(
            "entity m is port(arst, clk1, clk2, d : in bit; q : out bit); end entity;\n"
            "architecture rtl of m is begin p: process(arst, clk1, clk2) begin\n"
            "if arst='1' then q<='0'; else q<=d;\n"
            "if rising_edge(clk1) then q<=d; end if; end if;\n"
            "if rising_edge(clk2) then q<=not d; end if;\n"
            "end process; end architecture;\n", true);
        check(count(elseLinkedClock, "GJB9765.4.9.d") == 1,
              "4.9.d category 1 uses a direct source IF linked below ELSE");
        const auto staticElsifBeforeEdges = analyze(
            "entity m is port(mode, arst, clk1, clk2, d : in bit; q : out bit); end entity;\n"
            "architecture rtl of m is begin p: process(mode, arst, clk1, clk2) begin\n"
            "if mode='1' then q<=d;\n"
            "elsif arst='1' then q<='0';\n"
            "elsif rising_edge(clk1) then q<=d;\n"
            "elsif rising_edge(clk2) then q<=not d; end if;\n"
            "end process; end architecture;\n", true);
        check(count(staticElsifBeforeEdges, "GJB9765.4.9.d") == 1,
              "4.9.d accumulates a static non-edge ELSIF before the first edge");
        const auto staticElsifBetweenEdges = analyze(
            "entity m is port(mode, arst, clk1, clk2, d : in bit; q : out bit); end entity;\n"
            "architecture rtl of m is begin p: process(mode, arst, clk1, clk2) begin\n"
            "if mode='1' then q<=d;\n"
            "elsif rising_edge(clk1) then q<=d;\n"
            "elsif arst='1' then q<='0';\n"
            "elsif rising_edge(clk2) then q<=not d; end if;\n"
            "end process; end architecture;\n", true);
        check(count(staticElsifBetweenEdges, "GJB9765.4.9.d") == 1,
              "4.9.d accepts a static ELSIF after the primary edge but before an extra edge");
        const auto staticElsifAfterTwoEdges = analyze(
            "entity m is port(mode, arst, clk1, clk2, d : in bit; q : out bit); end entity;\n"
            "architecture rtl of m is begin p: process(mode, arst, clk1, clk2) begin\n"
            "if mode='1' then q<=d;\n"
            "elsif rising_edge(clk1) then q<=d;\n"
            "elsif rising_edge(clk2) then q<=not d;\n"
            "elsif arst='1' then q<='0'; end if;\n"
            "end process; end architecture;\n", true);
        check(count(staticElsifAfterTwoEdges, "GJB9765.4.9.d") == 0,
              "4.9.d does not promote key 4 using a static branch after an extra edge record");
        const auto lastElseLinkedIfOnly = analyze(
            "entity m is port(arst, clk1, clk2, d : in bit; q : out bit); end entity;\n"
            "architecture rtl of m is begin p: process(arst, clk1, clk2) begin\n"
            "if arst='1' then q<='0'; else\n"
            "if rising_edge(clk1) then q<=d; end if;\n"
            "if rising_edge(clk2) then q<=not d; end if; end if;\n"
            "if rising_edge(clk2) then q<=d; end if;\n"
            "end process; end architecture;\n", true);
        check(count(lastElseLinkedIfOnly, "GJB9765.4.9.d") == 0,
              "4.9.d category 1 classifies only the last direct source IF linked below ELSE");
        const auto linkedElseElsifIgnored = analyze(
            "entity m is port(arst, clk1, clk2, d : in bit; q : out bit); end entity;\n"
            "architecture rtl of m is begin p: process(arst, clk1, clk2) begin\n"
            "if arst='1' then q<='0'; else\n"
            "if rising_edge(clk1) then q<=d;\n"
            "elsif rising_edge(clk2) then q<=not d; end if; end if;\n"
            "if rising_edge(clk1) then q<=d; end if;\n"
            "end process; end architecture;\n", true);
        check(count(linkedElseElsifIgnored, "GJB9765.4.9.d") == 0,
              "4.9.d uses only the linked source IF direct main condition, not its ELSIF");
        const auto swappedLegacyOperands = analyze(
            "entity m is port(clk1, clk2, d : in bit; q : out bit); end entity;\n"
            "architecture rtl of m is begin p: process(clk1, clk2) begin\n"
            "if clk1='1' and clk1'event then q<=d; end if;\n"
            "if rising_edge(clk2) then q<=not d; end if;\n"
            "end process; end architecture;\n", true);
        check(count(swappedLegacyOperands, "GJB9765.4.9.d") == 1,
              "4.9.d accepts equality/event operands in either direct AND order");
        const auto legacyPrecedesLaterEdge = analyze(
            "entity m is port(clk1, clk2, d : in bit; q : out bit); end entity;\n"
            "architecture rtl of m is begin p: process(clk1, clk2) begin\n"
            "if clk1'event and clk1='1' and falling_edge(clk2) then q<=d; end if;\n"
            "if falling_edge(clk2) then q<=not d; end if;\n"
            "end process; end architecture;\n", true);
        check(count(legacyPrecedesLaterEdge, "GJB9765.4.9.d") == 1,
              "4.9.d native preorder selects a left legacy subtree before a later edge call");
        const auto noTrailingLegacySubtree = analyze(
            "entity m is port(enable, clk1, clk2, d : in bit; q : out bit); end entity;\n"
            "architecture rtl of m is begin p: process(enable, clk1, clk2) begin\n"
            "if enable='1' and clk1'event and clk1='1' then q<=d; end if;\n"
            "if rising_edge(clk2) then q<=not d; end if;\n"
            "end process; end architecture;\n", true);
        check(count(noTrailingLegacySubtree, "GJB9765.4.9.d") == 0,
              "4.9.d does not invent a legacy edge from a non-subtree trailing token pair");
        const auto parenthesizedLegacySubtree = analyze(
            "entity m is port(enable, clk1, clk2, d : in bit; q : out bit); end entity;\n"
            "architecture rtl of m is begin p: process(enable, clk1, clk2) begin\n"
            "if enable='1' and (clk1'event and clk1='1') then q<=d; end if;\n"
            "if rising_edge(clk2) then q<=not d; end if;\n"
            "end process; end architecture;\n", true);
        check(count(parenthesizedLegacySubtree, "GJB9765.4.9.d") == 1,
              "4.9.d descends into a parenthesized right legacy AND subtree");
        const auto generateNestedProcess = analyze(
            "entity m is port(clk1, clk2, d : in bit; q : out bit); end entity;\n"
            "architecture rtl of m is begin\n"
            "g: if true generate\n"
            "p: process(clk1, clk2) begin\n"
            "if rising_edge(clk1) then q <= d; end if;\n"
            "if rising_edge(clk2) then q <= not d; end if;\n"
            "end process; end generate; end architecture;\n", true);
        check(count(generateNestedProcess, "GJB9765.4.9.d") == 0,
              "4.9.d excludes processes nested below generate nodes");
        const auto blockNestedProcess = analyze(
            "entity m is port(clk1, clk2, d : in bit; q : out bit); end entity;\n"
            "architecture rtl of m is begin\n"
            "b: block is begin\n"
            "p: process(clk1, clk2) begin\n"
            "if rising_edge(clk1) then q <= d; end if;\n"
            "if rising_edge(clk2) then q <= not d; end if;\n"
            "end process; end block; end architecture;\n", true);
        check(count(blockNestedProcess, "GJB9765.4.9.d") == 0,
              "4.9.d excludes processes nested below block nodes");
    }
    {
        const auto mixedEdges = analyze(
            "entity m is port(clk, clk2, clk3, d : in bit; q1,q2,q3,q4,q5,q6 : out bit); end entity;\n"
            "architecture rtl of m is signal clk3_inv : bit; begin\n"
            "p1: process(clk) begin if rising_edge(clk) then q1<=d; end if; end process;\n"
            "p2: process(clk) begin if falling_edge(CLK) then q2<=d; end if; end process;\n"
            "p3: process(clk2) begin if clk2'event and clk2='1' then q3<=d; end if; end process;\n"
            "p4: process(clk2) begin if CLK2'event and CLK2='0' then q4<=d; end if; end process;\n"
            "p5: process(clk3) begin if rising_edge(clk3) then q5<=d; end if; end process;\n"
            "clk3_inv <= not CLK3;\n"
            "p6: process(clk3_inv) begin if rising_edge(clk3_inv) then q6<=d; end if; end process;\n"
            "end architecture;\n", true);
        check(count(mixedEdges, "GJB9765.4.9.i") == 3,
              "4.9.i covers direct, event-form and inverted-alias mixed clock edges");
        const auto effectiveSameEdge = analyze(
            "entity m is port(clk,d : in bit; q1,q2 : out bit); end entity;\n"
            "architecture rtl of m is signal inv : bit; begin inv <= not clk;\n"
            "p1: process(clk) begin if rising_edge(clk) then q1<=d; end if; end process;\n"
            "p2: process(inv) begin if falling_edge(inv) then q2<=d; end if; end process;\n"
            "end architecture;\n", true);
        check(count(effectiveSameEdge, "GJB9765.4.9.i") == 0,
              "4.9.i accepts opposite physical edges separated by one inverter");
        const auto directAlias = analyze(
            "entity m is port(clk,d : in bit; q1,q2 : out bit); end entity;\n"
            "architecture rtl of m is signal alias_clk : bit; begin alias_clk <= clk;\n"
            "p1: process(clk) begin if rising_edge(clk) then q1<=d; end if; end process;\n"
            "p2: process(alias_clk) begin if rising_edge(alias_clk) then q2<=d; end if; end process;\n"
            "end architecture;\n", true);
        check(count(directAlias, "GJB9765.4.9.i") == 0,
              "4.9.i does not treat a same-edge direct alias as an inversion conflict");
        const auto generatedProcessEdgeBoundary = analyze(
            "entity m is generic(use_nested : boolean := true); "
            "port(clk,d : in bit; q1,q2 : out bit); end entity;\n"
            "architecture rtl of m is begin\n"
            "p1: process(clk) begin if rising_edge(clk) then q1<=d; end if; end process;\n"
            "g: if use_nested generate p2: process(clk) begin "
            "if falling_edge(clk) then q2<=d; end if; end process; end generate;\n"
            "end architecture;\n", true);
        check(count(generatedProcessEdgeBoundary, "GJB9765.4.9.i") == 0,
              "4.9.i excludes clock relations from generate-nested processes");
        const auto blockProcessEdgeBoundary = analyze(
            "entity m is port(clk,d : in bit; q1,q2 : out bit); end entity;\n"
            "architecture rtl of m is begin\n"
            "p1: process(clk) begin if rising_edge(clk) then q1<=d; end if; end process;\n"
            "b: block begin p2: process(clk) begin if falling_edge(clk) then q2<=d; "
            "end if; end process; end block;\n"
            "end architecture;\n", true);
        check(count(blockProcessEdgeBoundary, "GJB9765.4.9.i") == 0,
              "4.9.i excludes clock relations from block-nested processes");
    }
    {
        const auto attachmentGatedClock = analyze(
            "entity m is port(clk,en,d : in bit; q : out bit); end entity;\n"
            "architecture rtl of m is signal clk_g1 : bit; begin\n"
            "clk_g1 <= clk and en;\n"
            "p: process(clk_g1) begin if rising_edge(clk_g1) then q<=d; end if; end process;\n"
            "end architecture;\n", true);
        check(count(attachmentGatedClock, "GJB9765.4.9.c") == 1,
              "attachment 4.9.c reports a binary AND gated VHDL clock");
        check(count(attachmentGatedClock, "GJB9765.4.9.e") == 1,
              "attachment 4.9.e reports an AND-generated VHDL clock");
        check(count(attachmentGatedClock, "GJB9765.4.9.j") == 0,
              "4.9.j does not misclassify a logical gate as a unary clock inverter");
        check(count(attachmentGatedClock, "GJB9765.4.9.g") == 0,
              "a gated clock is not mislabeled as the distinct ripple-clock clause");

        const auto reconvergentClock = analyze(
            "entity m is port(clk,en1,en2,d : in bit; q0,q1 : out bit); end entity;\n"
            "architecture rtl of m is signal path1,path2,gclk : bit; begin\n"
            "path1 <= clk and en1; path2 <= clk or en2; gclk <= path1 xor path2;\n"
            "p0: process(clk) begin if rising_edge(clk) then q0<=d; end if; end process;\n"
            "p1: process(gclk) begin if rising_edge(gclk) then q1<=d; end if; end process;\n"
            "end architecture;\n", true);
        check(count(reconvergentClock, "GJB9765.4.9.a") == 1,
              "attachment 4.9.a direct subset reports one clock origin reconverging through two paths");

        const auto reconvergenceThroughAlias = analyze(
            "entity m is port(clk,en1,en2,d : in bit; q0,q1 : out bit); end entity;\n"
            "architecture rtl of m is signal path1,path2,merged,alias_clk : bit; begin\n"
            "path1 <= clk and en1; path2 <= clk or en2; merged <= path1 nand path2; "
            "alias_clk <= merged;\n"
            "p0: process(clk) begin if rising_edge(clk) then q0<=d; end if; end process;\n"
            "p1: process(alias_clk) begin if rising_edge(alias_clk) then q1<=d; end if; end process;\n"
            "end architecture;\n", true);
        check(count(reconvergenceThroughAlias, "GJB9765.4.9.a") == 0,
              "4.9.a does not promote a post-convergence alias into a native candidate");

        const auto oneHopPropagationBoundary = analyze(
            "entity m is port(clk,en1,en2,en3,en4,d : in bit; q0,q1 : out bit); "
            "end entity;\n"
            "architecture rtl of m is signal base1,base2,path1,path2,gclk : bit; begin\n"
            "base1 <= clk and en1; base2 <= clk or en2; "
            "path1 <= base1 and en3; path2 <= base2 or en4; "
            "gclk <= path1 xor path2;\n"
            "p0: process(clk) begin if rising_edge(clk) then q0<=d; end if; end process;\n"
            "p1: process(gclk) begin if rising_edge(gclk) then q1<=d; end if; end process;\n"
            "end architecture;\n", true);
        check(count(oneHopPropagationBoundary, "GJB9765.4.9.a") == 0,
              "4.9.a uses the recovered one-hop relation-name construction");

        const auto distinctClockRelationsReconverge = analyze(
            "entity m is port(clk_a,clk_b,en1,en2,d : in bit; q0,q1,q2 : out bit); "
            "end entity;\n"
            "architecture rtl of m is signal path1,path2,gclk : bit; begin\n"
            "path1 <= clk_a and en1; path2 <= clk_b or en2; gclk <= path1 xor path2;\n"
            "p0: process(clk_a) begin if rising_edge(clk_a) then q0<=d; end if; end process;\n"
            "p1: process(clk_b) begin if rising_edge(clk_b) then q1<=d; end if; end process;\n"
            "p2: process(gclk) begin if rising_edge(gclk) then q2<=d; end if; end process;\n"
            "end architecture;\n", true);
        check(count(distinctClockRelationsReconverge, "GJB9765.4.9.a") == 1,
              "4.9.a counts matching relation leaves without requiring one shared origin");

        const auto classOneReconvergenceClock = analyze(
            "entity m is port(clk,rst,en1,en2,d : in bit; q0,q1 : out bit); end entity;\n"
            "architecture rtl of m is signal path1,path2,gclk : bit; begin\n"
            "path1 <= clk and en1; path2 <= clk or en2; gclk <= path1 xor path2;\n"
            "p0: process(clk,rst) begin if rst='0' then q0<='0'; "
            "elsif rising_edge(clk) then q0<=d; end if; end process;\n"
            "p1: process(gclk) begin if rising_edge(gclk) then q1<=d; end if; end process;\n"
            "end architecture;\n", true);
        check(count(classOneReconvergenceClock, "GJB9765.4.9.a") == 1,
              "4.9.a consumes recovered clock-relation class 1 names");

        const auto classFourReconvergenceBoundary = analyze(
            "entity m is port(clk,rst,en1,en2,d : in bit; q0,q1 : out bit); end entity;\n"
            "architecture rtl of m is signal path1,path2,gclk : bit; begin\n"
            "path1 <= clk and en1; path2 <= clk or en2; gclk <= path1 xor path2;\n"
            "p0: process(clk,rst,d) begin if rst='0' then q0<=d; "
            "elsif rising_edge(clk) then q0<=d; end if; end process;\n"
            "p1: process(gclk) begin if rising_edge(gclk) then q1<=d; end if; end process;\n"
            "end architecture;\n", true);
        check(count(classFourReconvergenceBoundary, "GJB9765.4.9.a") == 0,
              "4.9.a excludes recovered clock-relation class 4 names");

        const auto generatedProcessRelationBoundary = analyze(
            "entity m is generic(use_nested : boolean := true); "
            "port(clk,en1,en2,d : in bit; q0,q1 : out bit); end entity;\n"
            "architecture rtl of m is signal path1,path2,gclk : bit; begin\n"
            "path1 <= clk and en1; path2 <= clk or en2; gclk <= path1 xor path2;\n"
            "g: if use_nested generate p0: process(clk) begin "
            "if rising_edge(clk) then q0<=d; end if; end process; end generate;\n"
            "p1: process(gclk) begin if rising_edge(gclk) then q1<=d; "
            "end if; end process; end architecture;\n", true);
        check(count(generatedProcessRelationBoundary, "GJB9765.4.9.a") == 0,
              "4.9.a excludes clock relations from processes nested below generate roots");

        const auto shiftRootIsNotReconvergence = analyze(
            "entity m is port(clk,en1,en2,d : in bit; q : out bit); end entity;\n"
            "architecture rtl of m is signal path1,path2,gclk : bit; begin\n"
            "path1 <= clk and en1; path2 <= clk or en2; gclk <= path1 sll path2;\n"
            "p: process(gclk) begin if rising_edge(gclk) then q<=d; end if; end process;\n"
            "end architecture;\n", true);
        check(count(shiftRootIsNotReconvergence, "GJB9765.4.9.a") == 0,
              "4.9.a excludes shift/rotate roots outside mask 0xe0000019f");

        const auto reconvergenceStopsAtOutput = analyze(
            "entity m is port(clk,en1,en2,d : in bit; q0,gout : out bit); end entity;\n"
            "architecture rtl of m is signal path1,path2,gclk : bit; begin\n"
            "path1 <= clk and en1; path2 <= clk or en2; gclk <= path1 xor path2; gout<=gclk;\n"
            "p0: process(clk) begin if rising_edge(clk) then q0<=d; end if; end process;\n"
            "end architecture;\n", true);
        check(count(reconvergenceStopsAtOutput, "GJB9765.4.9.a") == 0,
              "4.9.a accepts the packaged output-port stop condition when the convergence is not a clock");

        const auto singleClockPath = analyze(
            "entity m is port(clk,en,d : in bit; q0,q1 : out bit); end entity;\n"
            "architecture rtl of m is signal path1,gclk : bit; begin\n"
            "path1 <= clk and en; gclk <= path1 xor en;\n"
            "p0: process(clk) begin if rising_edge(clk) then q0<=d; end if; end process;\n"
            "p1: process(gclk) begin if rising_edge(gclk) then q1<=d; end if; end process;\n"
            "end architecture;\n", true);
        const auto reconvergenceReview = std::ranges::find_if(
            singleClockPath.standardAssessments,
            [](const auto& assessment) { return assessment.clauseId == "GJB9765.4.9.a"; });
        check(count(singleClockPath, "GJB9765.4.9.a") == 0 &&
              reconvergenceReview != singleClockPath.standardAssessments.end() &&
              reconvergenceReview->state == analysis::StandardRuleState::ManualReviewRequired,
              "4.9.a requires at least two proven paths and keeps unsupported clean paths under review");

        const auto aliasesAndBoundaries = analyze(
            "entity m is port(clk,en,d : in bit; q1,q2,q3 : out bit); end entity;\n"
            "architecture rtl of m is signal gate0,alias0,inv0,arith0 : bit; begin\n"
            "gate0 <= clk xor en; alias0 <= gate0; inv0 <= not clk; arith0 <= clk + en;\n"
            "p1: process(alias0) begin if rising_edge(alias0) then q1<=d; end if; end process;\n"
            "p2: process(inv0) begin if rising_edge(inv0) then q2<=d; end if; end process;\n"
            "p3: process(arith0) begin if rising_edge(arith0) then q3<=d; end if; end process;\n"
            "end architecture;\n", true);
        check(count(aliasesAndBoundaries, "GJB9765.4.9.c") == 0,
              "4.9.c excludes XOR, unary inversion and arithmetic from the native AND/OR candidate set");
        check(count(aliasesAndBoundaries, "GJB9765.4.9.e") == 3,
              "4.9.e follows aliases and includes logic, NOT and arithmetic clock drivers");
        check(count(aliasesAndBoundaries, "GJB9765.4.9.j") == 1,
              "4.9.j reports the direct unary NOT clock driver only");

        const auto rootOrClock = analyze(
            "entity m is port(clk,en,d : in bit; q : out bit); end entity;\n"
            "architecture rtl of m is signal derived_clk : bit; begin\n"
            "derived_clk <= clk or en;\n"
            "p: process(derived_clk) begin if rising_edge(derived_clk) then q<=d; end if; "
            "end process; end architecture;\n", true);
        check(count(rootOrClock, "GJB9765.4.9.c") == 1,
              "4.9.c admits root OR (native AST 0x26/0x46 family)");

        const auto repeatedGateTarget = analyze(
            "entity m is port(clk,en1,en2,d : in bit; q : out bit); end entity;\n"
            "architecture rtl of m is signal derived_clk : bit; begin\n"
            "derived_clk <= clk and en1;\n"
            "derived_clk <= clk or en2;\n"
            "p: process(derived_clk) begin if rising_edge(derived_clk) then q<=d; end if; "
            "end process; end architecture;\n", true);
        const auto repeatedGate = std::ranges::find_if(
            repeatedGateTarget.violations, [](const auto& violation) {
                return violation.ruleId == "GJB9765.4.9.c";
            });
        check(count(repeatedGateTarget, "GJB9765.4.9.c") == 1 &&
                  repeatedGate != repeatedGateTarget.violations.end() &&
                  repeatedGate->line == 4,
              "4.9.c name-keyed candidate map keeps the last admitted assignment location");

        const auto rootXorClock = analyze(
            "entity m is port(clk,en,d : in bit; q : out bit); end entity;\n"
            "architecture rtl of m is signal derived_clk : bit; begin\n"
            "derived_clk <= clk xor en;\n"
            "p: process(derived_clk) begin if rising_edge(derived_clk) then q<=d; end if; "
            "end process; end architecture;\n", true);
        check(count(rootXorClock, "GJB9765.4.9.c") == 0,
              "4.9.c excludes root XOR outside native candidate types 0x25/0x26/0x45/0x46");

        const auto nestedAndClock = analyze(
            "entity m is port(clk,en,d : in bit; q : out bit); end entity;\n"
            "architecture rtl of m is\n"
            "function wrap(x: bit) return bit is begin return x; end;\n"
            "signal derived_clk : bit; begin derived_clk <= wrap(clk and en);\n"
            "p: process(derived_clk) begin if rising_edge(derived_clk) then q<=d; end if; "
            "end process; end architecture;\n", true);
        check(count(nestedAndClock, "GJB9765.4.9.c") == 0,
              "4.9.c excludes AND nested below an unsupported function-call RHS root");

        const auto conditionalClock = analyze(
            "entity m is port(clk,en,d : in bit; q : out bit); end entity;\n"
            "architecture rtl of m is signal derived_clk : bit; begin\n"
            "derived_clk <= clk when en='1' else '0';\n"
            "p: process(derived_clk) begin if rising_edge(derived_clk) then q<=d; end if; "
            "end process; end architecture;\n", true);
        check(count(conditionalClock, "GJB9765.4.9.c") == 0,
              "4.9.c excludes AST_WHEN from the native direct gate-candidate table");

        const auto bareAliasAfterGate = analyze(
            "entity m is port(clk,en,d : in bit; q : out bit); end entity;\n"
            "architecture rtl of m is signal gate0,alias0 : bit; begin\n"
            "gate0 <= clk and en; alias0 <= gate0;\n"
            "p: process(alias0) begin if rising_edge(alias0) then q<=d; end if; "
            "end process; end architecture;\n", true);
        check(count(bareAliasAfterGate, "GJB9765.4.9.c") == 0,
              "4.9.c does not propagate its native relation through a bare alias root");

        const auto xorAfterGate = analyze(
            "entity m is port(clk,en,d : in bit; q : out bit); end entity;\n"
            "architecture rtl of m is signal gate0,derived_clk : bit; begin\n"
            "gate0 <= clk and en; derived_clk <= gate0 xor en;\n"
            "p: process(derived_clk) begin if rising_edge(derived_clk) then q<=d; end if; "
            "end process; end architecture;\n", true);
        check(count(xorAfterGate, "GJB9765.4.9.c") == 1,
              "4.9.c propagates through an admitted direct XOR relation root");

        const auto functionAfterGate = analyze(
            "entity m is port(clk,en,d : in bit; q : out bit); end entity;\n"
            "architecture rtl of m is\n"
            "function wrap(x: bit) return bit is begin return x; end;\n"
            "signal gate0,derived_clk : bit; begin\n"
            "gate0 <= clk and en; derived_clk <= wrap(gate0);\n"
            "p: process(derived_clk) begin if rising_edge(derived_clk) then q<=d; end if; "
            "end process; end architecture;\n", true);
        check(count(functionAfterGate, "GJB9765.4.9.c") == 0,
              "4.9.c does not propagate through an unsupported call RHS root");

        const auto whenAfterGate = analyze(
            "entity m is port(clk,en,d : in bit; q : out bit); end entity;\n"
            "architecture rtl of m is signal gate0,derived_clk : bit; begin\n"
            "gate0 <= clk and en; derived_clk <= gate0 when en='1' else clk;\n"
            "p: process(derived_clk) begin if rising_edge(derived_clk) then q<=d; end if; "
            "end process; end architecture;\n", true);
        check(count(whenAfterGate, "GJB9765.4.9.c") == 1,
              "4.9.c propagates through the admitted AST_WHEN relation root");

        const auto unaryArithmeticClock = analyze(
            "library ieee; use ieee.std_logic_1164.all;\n"
            "entity m is port(clk,d : in std_logic; q : out std_logic); end entity;\n"
            "architecture rtl of m is\n"
            "function \"-\"(x: std_logic) return std_logic is begin return x; end;\n"
            "signal derived_clk : std_logic; begin derived_clk <= -clk;\n"
            "p: process(derived_clk) begin if rising_edge(derived_clk) then q<=d; end if; "
            "end process; end architecture;\n", true);
        check(count(unaryArithmeticClock, "GJB9765.4.9.e") == 0,
              "4.9.e excludes root unary NEG (AST 0x44) even when the overloaded VHDL is valid");

        const auto binaryArithmeticClock = analyze(
            "library ieee; use ieee.std_logic_1164.all;\n"
            "entity m is port(clk,en,d : in std_logic; q : out std_logic); end entity;\n"
            "architecture rtl of m is\n"
            "function \"+\"(l,r: std_logic) return std_logic is begin return l; end;\n"
            "signal derived_clk : std_logic; begin derived_clk <= clk + en;\n"
            "p: process(derived_clk) begin if rising_edge(derived_clk) then q<=d; end if; "
            "end process; end architecture;\n", true);
        check(count(binaryArithmeticClock, "GJB9765.4.9.e") == 1,
              "4.9.e admits root binary ADD (AST 0x3c) for a parser-confirmed clock");

        const auto nestedArithmeticClock = analyze(
            "library ieee; use ieee.std_logic_1164.all;\n"
            "entity m is port(clk,en,d : in std_logic; q : out std_logic); end entity;\n"
            "architecture rtl of m is\n"
            "function \"+\"(l,r: std_logic) return std_logic is begin return l; end;\n"
            "function wrap(x: std_logic) return std_logic is begin return x; end;\n"
            "signal derived_clk : std_logic; begin derived_clk <= wrap(clk + en);\n"
            "p: process(derived_clk) begin if rising_edge(derived_clk) then q<=d; end if; "
            "end process; end architecture;\n", true);
        check(count(nestedArithmeticClock, "GJB9765.4.9.e") == 0,
              "4.9.e excludes an ADD nested below an unsupported function-call RHS root");

        const auto primaryClock = analyze(
            "entity m is port(clk,d : in bit; q : out bit); end entity;\n"
            "architecture rtl of m is begin p: process(clk) begin\n"
            "if rising_edge(clk) then q<=d; end if; end process; end architecture;\n", true);
        check(count(primaryClock, "GJB9765.4.9.c") == 0,
              "4.9.c accepts the attachment's external primary-clock control");
        check(count(primaryClock, "GJB9765.4.9.e") == 0,
              "4.9.e accepts the attachment's external primary-clock control");

        const auto bareClockAlias = analyze(
            "entity m is port(clk,d : in bit; q : out bit); end entity;\n"
            "architecture rtl of m is signal alias_clk : bit; begin alias_clk<=clk;\n"
            "p: process(alias_clk) begin if rising_edge(alias_clk) then q<=d; end if; "
            "end process; end architecture;\n", true);
        check(count(bareClockAlias, "GJB9765.4.9.e") == 0,
              "4.9.e excludes a bare identifier RHS clock alias");
        check(count(bareClockAlias, "GJB9765.4.9.j") == 0,
              "4.9.j excludes a bare identifier clock alias");

        const auto parenthesizedInverter = analyze(
            "entity m is port(clk,d : in bit; q : out bit); end entity;\n"
            "architecture rtl of m is signal inv_clk : bit; begin inv_clk<=not (clk);\n"
            "p: process(inv_clk) begin if rising_edge(inv_clk) then q<=d; end if; "
            "end process; end architecture;\n", true);
        check(count(parenthesizedInverter, "GJB9765.4.9.j") == 1,
              "4.9.j accepts source parentheses around the unary identifier operand");

        const auto stdLogicInverter = analyze(
            "library ieee; use ieee.std_logic_1164.all;\n"
            "entity m is port(clk,d : in std_logic; q : out std_logic); end entity;\n"
            "architecture rtl of m is signal inv_clk : std_logic; begin inv_clk<=not clk;\n"
            "p: process(inv_clk) begin if rising_edge(inv_clk) then q<=d; end if; "
            "end process; end architecture;\n", true);
        check(count(stdLogicInverter, "GJB9765.4.9.j") == 1,
              "4.9.j accepts the predefined std_logic NOT spelling represented by a unary identifier root");

        const auto selectedNotOperand = analyze(
            "entity m is port(bus_value : in bit_vector(1 downto 0); d : in bit; q : out bit); "
            "end entity;\n"
            "architecture rtl of m is signal inv_clk : bit; begin inv_clk<=not bus_value(0);\n"
            "p: process(inv_clk) begin if rising_edge(inv_clk) then q<=d; end if; "
            "end process; end architecture;\n", true);
        check(count(selectedNotOperand, "GJB9765.4.9.j") == 0,
              "4.9.j excludes unary NOT whose direct AST child is an indexed selection");

        const auto calledNotOperand = analyze(
            "entity m is port(clk,d : in bit; q : out bit); end entity;\n"
            "architecture rtl of m is function pass(x : bit) return bit is begin return x; end; "
            "signal inv_clk : bit; begin inv_clk<=not pass(clk);\n"
            "p: process(inv_clk) begin if rising_edge(inv_clk) then q<=d; end if; "
            "end process; end architecture;\n", true);
        check(count(calledNotOperand, "GJB9765.4.9.j") == 0,
              "4.9.j excludes unary NOT whose direct AST child is a function call");

        const auto doubleNotOperand = analyze(
            "entity m is port(clk,d : in bit; q : out bit); end entity;\n"
            "architecture rtl of m is signal inv_clk : bit; begin inv_clk<=not not clk;\n"
            "p: process(inv_clk) begin if rising_edge(inv_clk) then q<=d; end if; "
            "end process; end architecture;\n", true);
        const auto inverterReview = std::ranges::find_if(
            doubleNotOperand.standardAssessments,
            [](const auto& assessment) { return assessment.clauseId == "GJB9765.4.9.j"; });
        check(count(doubleNotOperand, "GJB9765.4.9.j") == 0 &&
                  inverterReview != doubleNotOperand.standardAssessments.end() &&
                  inverterReview->state == analysis::StandardRuleState::ManualReviewRequired,
              "4.9.j excludes a nested unary NOT child and leaves the clean unsupported path under review");

        const auto arithmeticUnaryOperand = analyze(
            "entity m is port(clk,d : in integer; q : out integer); end entity;\n"
            "architecture rtl of m is signal derived_clk : integer; begin derived_clk<=-clk;\n"
            "p: process(derived_clk) begin if derived_clk'event then q<=d; end if; "
            "end process; end architecture;\n", true);
        check(count(arithmeticUnaryOperand, "GJB9765.4.9.j") == 0,
              "4.9.j does not confuse unary NEG with AST_BIT_NOT or AST_LOGIC_NOT");

        const auto generatedInvertedClock = analyze(
            "entity m is generic(use_nested : boolean := true); "
            "port(clk,d : in bit; q : out bit); end entity;\n"
            "architecture rtl of m is signal inv_clk : bit; begin inv_clk<=not clk;\n"
            "g: if use_nested generate p: process(inv_clk) begin "
            "if rising_edge(inv_clk) then q<=d; end if; end process; end generate;\n"
            "end architecture;\n", true);
        check(count(generatedInvertedClock, "GJB9765.4.9.j") == 1,
              "4.9.j includes clock paths used by generate-nested processes");
        const auto blockInvertedClock = analyze(
            "entity m is port(clk,d : in bit; q : out bit); end entity;\n"
            "architecture rtl of m is signal inv_clk : bit; begin inv_clk<=not clk;\n"
            "b: block begin p: process(inv_clk) begin if rising_edge(inv_clk) then q<=d; "
            "end if; end process; end block;\n"
            "end architecture;\n", true);
        check(count(blockInvertedClock, "GJB9765.4.9.j") == 1,
              "4.9.j includes clock paths used by block-nested processes");

        const auto componentClockBuffer = analyzeVhdlSources(
            "entity clock_buffer is port(i : in bit; o : out bit); end entity;\n"
            "architecture rtl of clock_buffer is begin o <= i; end architecture;\n"
            ,
            "entity m is port(clk,d : in bit; q : out bit); end entity;\n"
            "architecture rtl of m is signal buffered_clk : bit; begin\n"
            "u_buf: entity work.clock_buffer port map(i => clk, o => buffered_clk);\n"
            "p: process(buffered_clk) begin if rising_edge(buffered_clk) then q<=d; "
            "end if; end process; end architecture;\n");
        const auto bufferChild = std::ranges::find_if(
            componentClockBuffer.structure, [](const auto& unit) {
                return unit.name == "clock_buffer";
            });
        const auto bufferParent = std::ranges::find_if(
            componentClockBuffer.structure, [](const auto& unit) {
                return unit.name == "m";
            });
        check(bufferChild != componentClockBuffer.structure.end() &&
                  bufferChild->simpleSignalTransfers.size() == 1,
              "parser records one direct child component buffer transfer");
        check(bufferParent != componentClockBuffer.structure.end() &&
                  bufferParent->instances.size() == 1 &&
                  bufferParent->instances.front().resolved,
              "component clock path fixture resolves its child entity");
        check(count(componentClockBuffer, "GJB9765.4.9.j") == 1,
              "4.9.j reports a resolved user component that buffers a clock path");
        const auto componentClockInverter = analyzeVhdlSources(
            "entity clock_inverter is port(i : in bit; o : out bit); end entity;\n"
            "architecture rtl of clock_inverter is begin o <= not i; end architecture;\n"
            ,
            "entity m is port(clk,d : in bit; q : out bit); end entity;\n"
            "architecture rtl of m is signal inverted_clk : bit; begin\n"
            "u_inv: entity work.clock_inverter port map(clk, inverted_clk);\n"
            "p: process(inverted_clk) begin if rising_edge(inverted_clk) then q<=d; "
            "end if; end process; end architecture;\n");
        check(count(componentClockInverter, "GJB9765.4.9.j") == 1,
              "4.9.j reports a resolved positional component clock inversion");
        const auto dedicatedClockPrimitive = analyzeVhdlSources(
            "entity BUFG is port(i : in bit; o : out bit); end entity;\n"
            "architecture model of BUFG is begin o <= i; end architecture;\n",
            "entity m is port(clk,d : in bit; q : out bit); end entity;\n"
            "architecture rtl of m is signal global_clk : bit; begin\n"
            "u_bufg: entity work.BUFG port map(i => clk, o => global_clk);\n"
            "p: process(global_clk) begin if rising_edge(global_clk) then q<=d; "
            "end if; end process; end architecture;\n");
        check(count(dedicatedClockPrimitive, "GJB9765.4.9.j") == 0,
              "4.9.j exempts the recovered dedicated-clock primitive allow-list");
        const auto sequentialComponentBoundary = analyzeVhdlSources(
            "entity registered_clock is port(i : in bit; o : out bit); end entity;\n"
            "architecture rtl of registered_clock is begin p0: process(i) begin "
            "if rising_edge(i) then o<=i; end if; end process; end architecture;\n"
            ,
            "entity m is port(clk,d : in bit; q : out bit); end entity;\n"
            "architecture rtl of m is signal registered_clk : bit; begin\n"
            "u_reg: entity work.registered_clock port map(i => clk, o => registered_clk);\n"
            "p: process(registered_clk) begin if rising_edge(registered_clk) then q<=d; "
            "end if; end process; end architecture;\n");
        check(count(sequentialComponentBoundary, "GJB9765.4.9.j") == 0,
              "4.9.j component buffer subset excludes a sequential child transfer");

        const auto repeatedInvertersOneClock = analyze(
            "entity m is port(clk,d : in bit; q : out bit); end entity;\n"
            "architecture rtl of m is signal inv1,inv2 : bit; begin\n"
            "inv1 <= not clk; inv2 <= not inv1;\n"
            "p: process(inv2) begin if rising_edge(inv2) then q<=d; end if; "
            "end process; end architecture;\n", true);
        check(count(repeatedInvertersOneClock, "GJB9765.4.9.j") == 1,
              "4.9.j aggregates repeated inverter hits into one diagnostic per clock");

        const auto combinedBufferAndInverter = analyzeVhdlSources(
            "entity clock_buffer is port(i : in bit; o : out bit); end entity;\n"
            "architecture rtl of clock_buffer is begin o <= i; end architecture;\n",
            "entity m is port(clk,d : in bit; q : out bit); end entity;\n"
            "architecture rtl of m is signal buffered_clk,inverted_clk : bit; begin\n"
            "u_buf: entity work.clock_buffer port map(i => clk, o => buffered_clk);\n"
            "inverted_clk <= not buffered_clk;\n"
            "p: process(inverted_clk) begin if rising_edge(inverted_clk) then q<=d; "
            "end if; end process; end architecture;\n");
        const auto combinedDiagnostic = std::ranges::find_if(
            combinedBufferAndInverter.violations, [](const auto& violation) {
                return violation.ruleId == "GJB9765.4.9.j";
            });
        check(count(combinedBufferAndInverter, "GJB9765.4.9.j") == 1 &&
                  combinedDiagnostic != combinedBufferAndInverter.violations.end() &&
                  combinedDiagnostic->message.find("反相器和缓冲器") != std::string::npos,
              "4.9.j ORs native inverter/buffer bits and emits the combined message once");

        const auto recursiveComponentInverter = analyzeThreeVhdlSources(
            "entity leaf_inverter is port(i : in bit; o : out bit); end entity;\n"
            "architecture rtl of leaf_inverter is begin o <= not i; end architecture;\n",
            "entity clock_wrapper is port(i : in bit; o : out bit); end entity;\n"
            "architecture rtl of clock_wrapper is begin\n"
            "u_leaf: entity work.leaf_inverter port map(i => i, o => o);\n"
            "end architecture;\n",
            "entity m is port(clk,d : in bit; q : out bit); end entity;\n"
            "architecture rtl of m is signal nested_clk : bit; begin\n"
            "u_mid: entity work.clock_wrapper port map(clk, nested_clk);\n"
            "p: process(nested_clk) begin if rising_edge(nested_clk) then q<=d; "
            "end if; end process; end architecture;\n");
        const auto wrapper = std::ranges::find_if(
            recursiveComponentInverter.structure, [](const auto& candidate) {
                return candidate.name == "clock_wrapper";
            });
        check(wrapper != recursiveComponentInverter.structure.end() &&
                  wrapper->instances.size() == 1 &&
                  wrapper->instances.front().namedConnections.size() == 2 &&
                  wrapper->instances.front().namedConnections.front().actualIdentifier == "i",
              "parser retains direct identifier actuals for recursive component binding");
        const auto recursiveInverterDiagnostic = std::ranges::find_if(
            recursiveComponentInverter.violations, [](const auto& violation) {
                return violation.ruleId == "GJB9765.4.9.j";
            });
        check(count(recursiveComponentInverter, "GJB9765.4.9.j") == 1 &&
                  recursiveInverterDiagnostic != recursiveComponentInverter.violations.end() &&
                  recursiveInverterDiagnostic->message.find("反相器") != std::string::npos &&
                  recursiveInverterDiagnostic->message.find("缓冲器") == std::string::npos,
              "4.9.j follows a proven two-level user-component inverter path");

        const auto recursiveComponentBuffer = analyzeThreeVhdlSources(
            "entity leaf_buffer is port(i : in bit; o : out bit); end entity;\n"
            "architecture rtl of leaf_buffer is begin o <= i; end architecture;\n",
            "entity clock_wrapper is port(i : in bit; o : out bit); end entity;\n"
            "architecture rtl of clock_wrapper is signal internal_clk : bit; begin\n"
            "u_leaf: entity work.leaf_buffer port map(i, internal_clk);\n"
            "o <= internal_clk; end architecture;\n",
            "entity m is port(clk,d : in bit; q : out bit); end entity;\n"
            "architecture rtl of m is signal nested_clk : bit; begin\n"
            "u_mid: entity work.clock_wrapper port map(i => clk, o => nested_clk);\n"
            "p: process(nested_clk) begin if rising_edge(nested_clk) then q<=d; "
            "end if; end process; end architecture;\n");
        check(count(recursiveComponentBuffer, "GJB9765.4.9.j") == 1,
              "4.9.j follows a two-level component buffer path through an internal alias");

        const auto complexRecursiveActual = analyzeThreeVhdlSources(
            "entity leaf_inverter is port(i : in bit; o : out bit); end entity;\n"
            "architecture rtl of leaf_inverter is begin o <= not i; end architecture;\n",
            "entity clock_wrapper is port(i : in bit; o : out bit); end entity;\n"
            "architecture rtl of clock_wrapper is begin\n"
            "u_leaf: entity work.leaf_inverter port map(i => not i, o => o);\n"
            "end architecture;\n",
            "entity m is port(clk,d : in bit; q : out bit); end entity;\n"
            "architecture rtl of m is signal nested_clk : bit; begin\n"
            "u_mid: entity work.clock_wrapper port map(i => clk, o => nested_clk);\n"
            "p: process(nested_clk) begin if rising_edge(nested_clk) then q<=d; "
            "end if; end process; end architecture;\n");
        const auto complexWrapper = std::ranges::find_if(
            complexRecursiveActual.structure, [](const auto& candidate) {
                return candidate.name == "clock_wrapper";
            });
        check(complexWrapper != complexRecursiveActual.structure.end() &&
                  !complexWrapper->instances.empty() &&
                  !complexWrapper->instances.front().namedConnections.empty() &&
                  complexWrapper->instances.front().namedConnections.front().actualIdentifier.empty() &&
                  count(complexRecursiveActual, "GJB9765.4.9.j") == 0,
              "4.9.j leaves expression actuals outside the proven recursive hierarchy subset");

        const auto cyclicHierarchy = analyzeThreeVhdlSources(
            "entity a is port(i : in bit; o : out bit); end entity;\n"
            "architecture rtl of a is begin u_b: entity work.b port map(i, o); "
            "end architecture;\n",
            "entity b is port(i : in bit; o : out bit); end entity;\n"
            "architecture rtl of b is begin u_a: entity work.a port map(i, o); "
            "end architecture;\n",
            "entity m is port(clk,d : in bit; q : out bit); end entity;\n"
            "architecture rtl of m is signal cyclic_clk : bit; begin\n"
            "u_a: entity work.a port map(clk, cyclic_clk);\n"
            "p: process(cyclic_clk) begin if rising_edge(cyclic_clk) then q<=d; "
            "end if; end process; end architecture;\n");
        check(count(cyclicHierarchy, "GJB9765.4.9.j") == 0,
              "4.9.j terminates cyclic entity graphs without claiming a proven path");

        const auto twoIndependentClockPaths = analyze(
            "entity m is port(clk1,clk2,d : in bit; q1,q2 : out bit); end entity;\n"
            "architecture rtl of m is signal inv1,inv2 : bit; begin\n"
            "inv1 <= not clk1; inv2 <= not clk2;\n"
            "p1: process(inv1) begin if rising_edge(inv1) then q1<=d; end if; end process;\n"
            "p2: process(inv2) begin if rising_edge(inv2) then q2<=d; end if; end process;\n"
            "end architecture;\n", true);
        check(count(twoIndependentClockPaths, "GJB9765.4.9.j") == 2,
              "4.9.j preserves one diagnostic for each independent clock relation");

        const auto ordinaryDataLogic = analyze(
            "entity m is port(clk,a,b : in bit; q : out bit); end entity;\n"
            "architecture rtl of m is signal data_gate : bit; begin data_gate<=a and b;\n"
            "p: process(clk) begin if rising_edge(clk) then q<=data_gate; end if; "
            "end process; end architecture;\n", true);
        check(count(ordinaryDataLogic, "GJB9765.4.9.e") == 0,
              "4.9.e excludes combinational data not used as a clock");
        check(count(ordinaryDataLogic, "GJB9765.4.9.j") == 0,
              "4.9.j excludes a logical data expression not used as a clock");

        const auto rippleClock = analyze(
            "entity m is port(clk,d : in bit; q : out bit); end entity;\n"
            "architecture rtl of m is signal clk_div2 : bit; begin\n"
            "p1: process(clk) begin if rising_edge(clk) then clk_div2<=not clk_div2; end if; end process;\n"
            "p2: process(clk_div2) begin if rising_edge(clk_div2) then q<=d; end if; end process;\n"
            "end architecture;\n", true);
        check(count(rippleClock, "GJB9765.4.9.c") == 0,
              "4.9.c does not classify the separate ripple-clock example as gating");
        check(count(rippleClock, "GJB9765.4.9.g") == 1,
              "attachment 4.9.g reports a flip-flop output reused as a clock");

        const auto registeredClock = analyze(
            "entity m is port(clk,d : in bit; q : out bit); end entity;\n"
            "architecture rtl of m is signal registered_clk : bit; begin\n"
            "p1: process(clk) begin if rising_edge(clk) then registered_clk<=d; end if; end process;\n"
            "p2: process(registered_clk) begin if rising_edge(registered_clk) then q<=d; end if; end process;\n"
            "end architecture;\n", true);
        check(count(registeredClock, "GJB9765.4.9.g") == 1,
              "native 4.9.g intersection does not require a self-inverting divider RHS");

        const auto nestedClockConsumer = analyze(
            "entity m is port(clk,d : in bit; q : out bit); end entity;\n"
            "architecture rtl of m is signal derived_clk : bit; begin\n"
            "p1: process(clk) begin if rising_edge(clk) then derived_clk<=d; end if; end process;\n"
            "g: if true generate\n"
            "p2: process(derived_clk) begin if rising_edge(derived_clk) then q<=d; end if; end process;\n"
            "end generate; end architecture;\n", true);
        check(count(nestedClockConsumer, "GJB9765.4.9.g") == 0,
              "4.9.g excludes clock relations from generate-nested processes");

        const auto nestedRegisterProducer = analyze(
            "entity m is port(clk,d : in bit; q : out bit); end entity;\n"
            "architecture rtl of m is signal derived_clk : bit; begin\n"
            "g: if true generate\n"
            "p1: process(clk) begin if rising_edge(clk) then derived_clk<=d; end if; end process;\n"
            "end generate;\n"
            "p2: process(derived_clk) begin if rising_edge(derived_clk) then q<=d; end if; end process;\n"
            "end architecture;\n", true);
        check(count(nestedRegisterProducer, "GJB9765.4.9.g") == 0,
              "4.9.g excludes register assignments from generate-nested processes");

        const auto combinationalClock = analyze(
            "entity m is port(clk,en,d : in bit; q : out bit); end entity;\n"
            "architecture rtl of m is signal comb_clk : bit; begin\n"
            "p1: process(clk,en) begin comb_clk<=clk and en; end process;\n"
            "p2: process(comb_clk) begin if rising_edge(comb_clk) then q<=d; end if; end process;\n"
            "end architecture;\n", true);
        check(count(combinationalClock, "GJB9765.4.9.c") == 0 &&
                  count(combinationalClock, "GJB9765.4.9.g") == 0,
              "4.9.c excludes process-local assignments from direct architecture candidates");

        const auto generatedGateClock = analyze(
            "entity m is generic(use_gate : boolean := true); "
            "port(clk,en,d : in bit; q : out bit); end entity;\n"
            "architecture rtl of m is signal generated_clk : bit; begin\n"
            "g: if use_gate generate generated_clk <= clk and en; end generate;\n"
            "p: process(generated_clk) begin if rising_edge(generated_clk) then q<=d; "
            "end if; end process; end architecture;\n", true);
        check(count(generatedGateClock, "GJB9765.4.9.c") == 0,
              "4.9.c excludes assignments nested below a generate AST root");

        const auto ordinaryRegister = analyze(
            "entity m is port(clk,d : in bit; q : out bit); end entity;\n"
            "architecture rtl of m is signal stored : bit; begin\n"
            "p1: process(clk) begin if rising_edge(clk) then stored<=d; q<=stored; end if; end process;\n"
            "end architecture;\n", true);
        check(count(ordinaryRegister, "GJB9765.4.9.g") == 0,
              "4.9.g requires the sequential assignment target to be used as a clock");

        const auto multipleClockDomains = analyze(
            "entity m is port(clk1,clk2,d : in bit; q1,q2 : out bit); end entity;\n"
            "architecture rtl of m is begin\n"
            "p1: process(clk1) begin if rising_edge(clk1) then q1<=d; end if; end process;\n"
            "p2: process(clk2) begin if clk2'event and clk2='1' then q2<=d; end if; end process;\n"
            "end architecture;\n", true);
        check(count(multipleClockDomains, "GJB9765.4.9.h") == 1,
              "attachment 4.9.h reports one design unit with two clock domains");

        const auto generatedClockDomain = analyze(
            "entity m is port(clk1,clk2,d : in bit; q1,q2 : out bit); end entity;\n"
            "architecture rtl of m is begin\n"
            "p1: process(clk1) begin if rising_edge(clk1) then q1<=d; end if; end process;\n"
            "g: if true generate\n"
            "p2: process(clk2) begin if rising_edge(clk2) then q2<=d; end if; end process;\n"
            "end generate; end architecture;\n", true);
        check(count(generatedClockDomain, "GJB9765.4.9.h") == 1,
              "4.9.h recursively includes clock domains in generate-nested processes");

        const auto blockClockDomain = analyze(
            "entity m is port(clk1,clk2,d : in bit; q1,q2 : out bit); end entity;\n"
            "architecture rtl of m is begin\n"
            "p1: process(clk1) begin if rising_edge(clk1) then q1<=d; end if; end process;\n"
            "b: block is begin\n"
            "p2: process(clk2) begin if rising_edge(clk2) then q2<=d; end if; end process;\n"
            "end block; end architecture;\n", true);
        check(count(blockClockDomain, "GJB9765.4.9.h") == 1,
              "4.9.h recursively includes clock domains in block-nested processes");

        const auto oneClockDomain = analyze(
            "entity m is port(clk,d : in bit; q1,q2 : out bit); end entity;\n"
            "architecture rtl of m is begin\n"
            "p1: process(clk) begin if rising_edge(clk) then q1<=d; end if; end process;\n"
            "p2: process(clk) begin if falling_edge(CLK) then q2<=d; end if; end process;\n"
            "end architecture;\n", true);
        check(count(oneClockDomain, "GJB9765.4.9.h") == 0,
              "4.9.h deduplicates one case-insensitive clock name across edge polarities");
        const auto aliasedClockDomain = analyze(
            "entity m is port(clk,d : in bit; q1,q2 : out bit); end entity;\n"
            "architecture rtl of m is signal clk_alias : bit; begin\n"
            "clk_alias <= not CLK;\n"
            "p1: process(clk) begin if rising_edge(clk) then q1<=d; end if; end process;\n"
            "p2: process(clk_alias) begin if rising_edge(clk_alias) then q2<=d; end if; end process;\n"
            "end architecture;\n", true);
        check(count(aliasedClockDomain, "GJB9765.4.9.h") == 0,
              "4.9.h relation adapter folds concurrent direct and inverted clock aliases");
        check(count(registeredClock, "GJB9765.4.9.h") == 1,
              "4.9.h does not fold a process-assigned registered clock into its source domain");
        const auto clockDomainAssessment = std::ranges::find_if(
            oneClockDomain.standardAssessments,
            [](const auto& entry) { return entry.clauseId == "GJB9765.4.9.h"; });
        check(clockDomainAssessment != oneClockDomain.standardAssessments.end() &&
                  clockDomainAssessment->state ==
                      analysis::StandardRuleState::ProjectContextRequired,
              "bounded 4.9.h no finding still requires hierarchy project context");

        {
            analysis::AnalysisOptions selectedClockTop;
            selectedClockTop.standardId = "GJB9765-2020";
            selectedClockTop.topModule = "TOP_A";
            const std::vector<core::SourceFile> roots{
                {"vhdl-clock-roots.vhd", core::HdlLanguage::Vhdl,
                 "entity top_a is port(clk,d: in bit; q: out bit); end entity;\n"
                 "architecture rtl of top_a is begin p: process(clk) begin "
                 "if rising_edge(clk) then q<=d; end if; end process; end architecture;\n"
                 "entity top_b is port(clk1,clk2,d: in bit; q1,q2: out bit); end entity;\n"
                 "architecture rtl of top_b is begin "
                 "p1: process(clk1) begin if rising_edge(clk1) then q1<=d; end if; end process; "
                 "p2: process(clk2) begin if rising_edge(clk2) then q2<=d; end if; end process; "
                 "end architecture;\n"}
            };
            auto selected = analysis::Analyzer(selectedClockTop).analyzeSources(roots);
            check(selected.errors.empty() && count(selected, "GJB9765.4.9.h") == 0,
                  "explicit case-insensitive VHDL top excludes multiple clocks in another root");
            selectedClockTop.topModule = "top_b";
            selected = analysis::Analyzer(selectedClockTop).analyzeSources(roots);
            check(selected.errors.empty() && count(selected, "GJB9765.4.9.h") == 1,
                  "explicit VHDL top admits its own multiple-clock-domain finding");
        }

        const auto clockControlsMux = analyze(
            "entity m is port(clk,d : in bit; q1,q2 : out bit); end entity;\n"
            "architecture rtl of m is signal ordinary : bit; begin\n"
            "p1: process(clk) begin if rising_edge(clk) then ordinary<=d; end if; end process;\n"
            "p2: process(clk,d) begin if CLK then q1<=d; else q1<='0'; end if; end process;\n"
            "p3: process(clk,d) begin case clk is when '1' => q2<=d;\n"
            "when others => q2<='0'; end case; end process;\n"
            "end architecture;\n", true);
        check(count(clockControlsMux, "GJB9765.4.9.b") == 2,
              "4.9.b reports direct clock IF and CASE selectors, not rising_edge guards");
        const auto recoveredClockControls = analyze(
            "entity m is port(clk,en,d : in bit; q1,q2,q3 : out bit); end entity;\n"
            "architecture rtl of m is begin\n"
            "p0: process(clk) begin if rising_edge(clk) then q1<=d; end if; end process;\n"
            "p1: process(clk,d) begin if not clk then q1<=d; end if; end process;\n"
            "p2: process(clk,en,d) begin if clk and en then q2<=d; end if; end process;\n"
            "p3: process(clk,d) begin if clk = '1' then q3<=d; end if; end process;\n"
            "end architecture;\n", true);
        check(count(recoveredClockControls, "GJB9765.4.9.b") == 3,
              "4.9.b admits native NOT, AND/OR and binary equality control forms");
        const auto nativeEdgeControls = analyze(
            "entity m is port(clk,d : in bit; q : out bit); end entity;\n"
            "architecture rtl of m is begin\n"
            "p1: process(clk) begin if clk'event and clk='1' then q<=d; end if; end process;\n"
            "end architecture;\n", true);
        check(count(nativeEdgeControls, "GJB9765.4.9.b") == 0,
              "4.9.b excludes native event-plus-level edge idiom");
        const auto unsupportedClockControls = analyze(
            "entity m is port(clk,d : in bit; q : out bit); end entity;\n"
            "architecture rtl of m is begin\n"
            "p1: process(clk,d) begin if wrap(clk) then q<=d; end if; end process;\n"
            "p2: process(clk,d) begin if clk > d then q<=d; end if; end process;\n"
            "end architecture;\n", true);
        check(count(unsupportedClockControls, "GJB9765.4.9.b") == 0,
              "4.9.b does not widen recovered candidates to calls or relations");
        const auto classOneClockControl = analyze(
            "entity m is port(clk,rst,d : in bit; q1,q2 : out bit); end entity;\n"
            "architecture rtl of m is begin\n"
            "p1: process(clk,rst) begin if rst='0' then q1<='0';\n"
            "elsif rising_edge(clk) then q1<=d; end if; end process;\n"
            "p2: process(clk,d) begin if clk then q2<=d; end if; end process;\n"
            "end architecture;\n", true);
        check(count(classOneClockControl, "GJB9765.4.9.b") == 1,
              "4.9.b consumes recovered relation class 1 clock names");
        const auto classOneGenericClockControl = analyze(
            "entity m is generic(reset_value : bit := '0'); "
            "port(clk,rst,d : in bit; q1,q2 : out bit); end entity;\n"
            "architecture rtl of m is begin\n"
            "p1: process(clk,rst) begin if rst='0' then q1<=reset_value; "
            "elsif rising_edge(clk) then q1<=d; end if; end process;\n"
            "p2: process(clk,d) begin if clk then q2<=d; end if; end process;\n"
            "end architecture;\n", true);
        check(count(classOneGenericClockControl, "GJB9765.4.9.b") == 1,
              "4.9.b class 1 admits an owner-entity generic static action");
        const auto classOneArchitectureConstantClockControl = analyze(
            "entity m is port(clk,rst,d : in bit; q1,q2 : out bit); end entity;\n"
            "architecture rtl of m is constant reset_value : bit := '0'; begin\n"
            "p1: process(clk,rst) begin if rst='0' then q1<=reset_value; "
            "elsif rising_edge(clk) then q1<=d; end if; end process;\n"
            "p2: process(clk,d) begin if clk then q2<=d; end if; end process;\n"
            "end architecture;\n", true);
        check(count(classOneArchitectureConstantClockControl, "GJB9765.4.9.b") == 1,
              "4.9.b class 1 admits a direct architecture constant static action");
        const auto classOneEnumClockControl = analyze(
            "entity m is port(clk,rst,d : in bit; q2 : out bit); end entity;\n"
            "architecture rtl of m is type state_t is (idle, run); "
            "signal state : state_t; begin\n"
            "p1: process(clk,rst) begin if rst='0' then state<=idle; "
            "elsif rising_edge(clk) then state<=run; end if; end process;\n"
            "p2: process(clk,d) begin if clk then q2<=d; end if; end process;\n"
            "end architecture;\n", true);
        check(count(classOneEnumClockControl, "GJB9765.4.9.b") == 1,
              "4.9.b class 1 admits a direct architecture enum literal static action");
        const auto classOneLiteralConcatClockControl = analyze(
            "entity m is port(clk,rst,d : in bit; q1 : out bit_vector(1 downto 0); "
            "q2 : out bit); end entity;\narchitecture rtl of m is begin\n"
            "p1: process(clk,rst) begin if rst='0' then q1<='0' & '1'; "
            "elsif rising_edge(clk) then q1<=\"00\"; end if; end process;\n"
            "p2: process(clk,d) begin if clk then q2<=d; end if; end process;\n"
            "end architecture;\n", true);
        check(count(classOneLiteralConcatClockControl, "GJB9765.4.9.b") == 1,
              "4.9.b class 1 admits an all-literal AST_CONCAT static action");
        const auto classOneOthersClockControl = analyze(
            "entity m is port(clk,rst,d : in bit; q1 : out bit_vector(1 downto 0); "
            "q2 : out bit); end entity;\narchitecture rtl of m is begin\n"
            "p1: process(clk,rst) begin if rst='0' then q1<=(others=>d); "
            "elsif rising_edge(clk) then q1<=\"00\"; end if; end process;\n"
            "p2: process(clk,d) begin if clk then q2<=d; end if; end process;\n"
            "end architecture;\n", true);
        check(count(classOneOthersClockControl, "GJB9765.4.9.b") == 1,
              "4.9.b class 1 preserves the native first AST_OTHERS shortcut");
        const auto classFourProcessConstantClockControl = analyze(
            "entity m is port(clk,rst,d : in bit; q1,q2 : out bit); end entity;\n"
            "architecture rtl of m is begin\n"
            "p1: process(clk,rst) constant reset_value : bit := '0'; begin "
            "if rst='0' then q1<=reset_value; elsif rising_edge(clk) then q1<=d; "
            "end if; end process;\n"
            "p2: process(clk,d) begin if clk then q2<=d; end if; end process;\n"
            "end architecture;\n", true);
        check(count(classFourProcessConstantClockControl, "GJB9765.4.9.b") == 0,
              "4.9.b keeps process-local constants outside the per-root static set");
        const auto classFourPackageConstantClockControl = analyze(
            "entity m is port(clk,rst,d : in bit; q1,q2 : out bit); end entity;\n"
            "architecture rtl of m is package p is constant reset_value : bit := '0'; "
            "end package; use p.all; begin\n"
            "p1: process(clk,rst) begin if rst='0' then q1<=reset_value; "
            "elsif rising_edge(clk) then q1<=d; end if; end process;\n"
            "p2: process(clk,d) begin if clk then q2<=d; end if; end process;\n"
            "end architecture;\n", true);
        check(count(classFourPackageConstantClockControl, "GJB9765.4.9.b") == 0,
              "4.9.b keeps local-package constants outside the per-root static set");
        const auto classFourClockControl = analyze(
            "entity m is port(clk,rst,d : in bit; q1,q2 : out bit); end entity;\n"
            "architecture rtl of m is begin\n"
            "p1: process(clk,rst,d) begin if rst='0' then q1<=d;\n"
            "elsif rising_edge(clk) then q1<=d; end if; end process;\n"
            "p2: process(clk,d) begin if clk then q2<=d; end if; end process;\n"
            "end architecture;\n", true);
        check(count(classFourClockControl, "GJB9765.4.9.b") == 0,
              "4.9.b excludes recovered relation class 4 clocks");
        const auto classOneElseClockControl = analyze(
            "entity m is port(clk,rst,d : in bit; q1,q2 : out bit); end entity;\n"
            "architecture rtl of m is begin\n"
            "p1: process(clk,rst) begin if rst='0' then q1<='0'; else\n"
            "if rising_edge(clk) then q1<=d; end if; end if; end process;\n"
            "p2: process(clk,d) begin if clk then q2<=d; end if; end process;\n"
            "end architecture;\n", true);
        check(count(classOneElseClockControl, "GJB9765.4.9.b") == 1,
              "4.9.b follows recovered class 1 ELSE-linked source IF");
        const auto classOneGenericElseClockControl = analyze(
            "entity m is generic(reset_value : bit := '0'); "
            "port(clk,rst,d : in bit; q1,q2 : out bit); end entity;\n"
            "architecture rtl of m is begin\n"
            "p1: process(clk,rst) begin if rst='0' then q1<=reset_value; else "
            "if rising_edge(clk) then q1<=d; end if; end if; end process;\n"
            "p2: process(clk,d) begin if clk then q2<=d; end if; end process;\n"
            "end architecture;\n", true);
        check(count(classOneGenericElseClockControl, "GJB9765.4.9.b") == 1,
              "4.9.b ELSE-linked class 1 shares the recovered named static-action set");
        const auto descendantEdgeClockControl = analyze(
            "entity m is port(clk,en,d : in bit; q1,q2 : out bit); end entity;\n"
            "architecture rtl of m is begin\n"
            "p1: process(clk,en) begin if (clk='1' and clk'event) and en then "
            "q1<=d; end if; end process;\n"
            "p2: process(clk,d) begin if clk then q2<=d; end if; end process;\n"
            "end architecture;\n", true);
        check(count(descendantEdgeClockControl, "GJB9765.4.9.b") == 1,
              "4.9.b relation builder admits a legacy edge in the first preorder subtree");
        const auto classOneDescendantEdgeClockControl = analyze(
            "entity m is port(clk,rst,en,d : in bit; q1,q2 : out bit); end entity;\n"
            "architecture rtl of m is begin\n"
            "p1: process(clk,rst,en) begin if rst='0' then q1<='0'; "
            "elsif en and (clk='1' and clk'event) then q1<=d; end if; end process;\n"
            "p2: process(clk,d) begin if clk then q2<=d; end if; end process;\n"
            "end architecture;\n", true);
        check(count(classOneDescendantEdgeClockControl, "GJB9765.4.9.b") == 1,
              "4.9.b class-1 ELSIF uses the shared preorder edge collector");
        const auto classOneElseDescendantEdgeClockControl = analyze(
            "entity m is port(clk,rst,en,d : in bit; q1,q2 : out bit); end entity;\n"
            "architecture rtl of m is begin\n"
            "p1: process(clk,rst,en) begin if rst='0' then q1<='0'; else "
            "if en and (clk'event and clk='1') then q1<=d; end if; end if; end process;\n"
            "p2: process(clk,d) begin if clk then q2<=d; end if; end process;\n"
            "end architecture;\n", true);
        check(count(classOneElseDescendantEdgeClockControl, "GJB9765.4.9.b") == 1,
              "4.9.b ELSE-linked class-1 IF uses the shared preorder edge collector");
        const auto firstPreorderEdgeClockControl = analyze(
            "entity m is port(clk_a,clk_b,d : in bit; q1,q2,q3 : out bit); end entity;\n"
            "architecture rtl of m is begin\n"
            "p1: process(clk_a,clk_b) begin if rising_edge(clk_a) or "
            "rising_edge(clk_b) then q1<=d; end if; end process;\n"
            "p2: process(clk_a,d) begin if clk_a then q2<=d; end if; end process;\n"
            "p3: process(clk_b,d) begin if clk_b then q3<=d; end if; end process;\n"
            "end architecture;\n", true);
        const auto preorderClockControls = std::ranges::count_if(
            firstPreorderEdgeClockControl.violations, [](const auto& violation) {
                return violation.ruleId == "GJB9765.4.9.b" &&
                    violation.message.find("clk_a") != std::string::npos;
            });
        check(count(firstPreorderEdgeClockControl, "GJB9765.4.9.b") == 1 &&
                  preorderClockControls == 1,
              "4.9.b relation builder keeps only the first successful preorder edge");
        const auto laterTopLevelSourceIfClockControl = analyze(
            "entity m is port(clk,d : in bit; q1,q2 : out bit); end entity;\n"
            "architecture rtl of m is begin\n"
            "p1: process(clk,d) begin q1<=d; if rising_edge(clk) then "
            "q1<=not d; end if; end process;\n"
            "p2: process(clk,d) begin if clk then q2<=d; end if; end process;\n"
            "end architecture;\n", true);
        check(count(laterTopLevelSourceIfClockControl, "GJB9765.4.9.b") == 1,
              "4.9.b enumerates a direct source IF after an ordinary process statement");
        const auto separateTopLevelSourceIfClockControls = analyze(
            "entity m is port(clk1,clk2,d : in bit; q1,q2,q3,q4 : out bit); end entity;\n"
            "architecture rtl of m is begin\n"
            "p1: process(clk1,clk2,d) begin q1<=d; "
            "if rising_edge(clk1) then q1<=not d; end if; "
            "if rising_edge(clk2) then q2<=not d; end if; end process;\n"
            "p2: process(clk1,d) begin if clk1 then q3<=d; end if; end process;\n"
            "p3: process(clk2,d) begin if clk2 then q4<=d; end if; end process;\n"
            "end architecture;\n", true);
        check(count(separateTopLevelSourceIfClockControls, "GJB9765.4.9.b") == 2,
              "4.9.b builds relations for every independent top-level source IF");
        const auto staticBetweenEdgesClockControl = analyze(
            "entity m is port(clk1,clk2,rst,en,d : in bit; q1,q2 : out bit); end entity;\n"
            "architecture rtl of m is begin\n"
            "p1: process(clk1,clk2,rst,en,d) begin if rst='0' then q1<=d; "
            "elsif rising_edge(clk1) then q1<=d; "
            "elsif en='1' then q1<='0'; "
            "elsif rising_edge(clk2) then q1<=not d; end if; end process;\n"
            "p2: process(clk1,d) begin if clk1 then q2<=d; end if; end process;\n"
            "end architecture;\n", true);
        check(count(staticBetweenEdgesClockControl, "GJB9765.4.9.b") == 1,
              "4.9.b category 1 accepts a static ELSIF between the first two edges");
        const auto staticAfterSecondEdgeClockControl = analyze(
            "entity m is port(clk1,clk2,rst,en,d : in bit; q1,q2 : out bit); end entity;\n"
            "architecture rtl of m is begin\n"
            "p1: process(clk1,clk2,rst,en,d) begin if rst='0' then q1<=d; "
            "elsif rising_edge(clk1) then q1<=d; "
            "elsif rising_edge(clk2) then q1<=not d; "
            "elsif en='1' then q1<='0'; end if; end process;\n"
            "p2: process(clk1,d) begin if clk1 then q2<=d; end if; end process;\n"
            "end architecture;\n", true);
        check(count(staticAfterSecondEdgeClockControl, "GJB9765.4.9.b") == 0,
              "4.9.b category 4 is not promoted by a static ELSIF after the second edge");
        const auto lastElseLinkedIfClockControl = analyze(
            "entity m is port(clk1,clk2,rst,d : in bit; q1,q2,q3 : out bit); end entity;\n"
            "architecture rtl of m is begin\n"
            "p1: process(clk1,clk2,rst) begin if rst='0' then q1<='0'; else "
            "if rising_edge(clk1) then q1<=d; end if; "
            "if rising_edge(clk2) then q1<=not d; end if; end if; end process;\n"
            "p2: process(clk1,d) begin if clk1 then q2<=d; end if; end process;\n"
            "p3: process(clk2,d) begin if clk2 then q3<=d; end if; end process;\n"
            "end architecture;\n", true);
        const auto lastElseClockHits = std::ranges::count_if(
            lastElseLinkedIfClockControl.violations, [](const auto& violation) {
                return violation.ruleId == "GJB9765.4.9.b" &&
                    violation.message.find("clk2") != std::string::npos;
            });
        check(count(lastElseLinkedIfClockControl, "GJB9765.4.9.b") == 1 &&
                  lastElseClockHits == 1,
              "4.9.b category 1 selects only the last direct ELSE-linked source IF");
        const auto linkedIfElsifExcludedClockControl = analyze(
            "entity m is port(clk1,clk2,rst,d : in bit; q1,q2,q3 : out bit); end entity;\n"
            "architecture rtl of m is begin\n"
            "p1: process(clk1,clk2,rst) begin if rst='0' then q1<='0'; else "
            "if rising_edge(clk1) then q1<=d; elsif rising_edge(clk2) then "
            "q1<=not d; end if; end if; end process;\n"
            "p2: process(clk1,d) begin if clk1 then q2<=d; end if; end process;\n"
            "p3: process(clk2,d) begin if clk2 then q3<=d; end if; end process;\n"
            "end architecture;\n", true);
        const auto linkedMainClockHits = std::ranges::count_if(
            linkedIfElsifExcludedClockControl.violations, [](const auto& violation) {
                return violation.ruleId == "GJB9765.4.9.b" &&
                    violation.message.find("clk1") != std::string::npos;
            });
        check(count(linkedIfElsifExcludedClockControl, "GJB9765.4.9.b") == 1 &&
                  linkedMainClockHits == 1,
              "4.9.b uses an ELSE-linked IF main condition but not its ELSIF edge");
        const auto classFourElseClockControl = analyze(
            "entity m is port(clk,rst,d : in bit; q1,q2 : out bit); end entity;\n"
            "architecture rtl of m is begin\n"
            "p1: process(clk,rst,d) begin if rst='0' then q1<=d; else\n"
            "if rising_edge(clk) then q1<=d; end if; end if; end process;\n"
            "p2: process(clk,d) begin if clk then q2<=d; end if; end process;\n"
            "end architecture;\n", true);
        check(count(classFourElseClockControl, "GJB9765.4.9.b") == 0,
              "4.9.b ELSE-linked dynamic action remains excluded class 4");
        const auto conditionalAssignmentClockControls = analyze(
            "entity m is port(clk,en,d : in bit; q1,q2,q3,q4,q5 : out bit); end entity;\n"
            "architecture rtl of m is begin\n"
            "p1: process(clk) begin if rising_edge(clk) then q1<=d; end if; end process;\n"
            "q2 <= d when clk else '0';\n"
            "q3 <= d when clk='1' else '0';\n"
            "q4 <= d when en=clk else '0';\n"
            "q5 <= d when not clk else '0';\n"
            "end architecture;\n", true);
        check(count(conditionalAssignmentClockControls, "GJB9765.4.9.b") == 2,
              "4.9.b admits WHEN direct identifier and EQ-left identifier only");
        const auto relationInternalClockControls = analyze(
            "entity m is port(clk,rst,d : in bit; q1,q2 : out bit); end entity;\n"
            "architecture rtl of m is begin\n"
            "p1: process(clk,rst) begin if clk='0' then q1<='0';\n"
            "elsif rising_edge(clk) then q1<=d; end if; end process;\n"
            "p2: process(clk) begin if rising_edge(clk) then\n"
            "if clk='0' then q2<=d; end if; end if; end process;\n"
            "end architecture;\n", true);
        check(count(relationInternalClockControls, "GJB9765.4.9.b") == 0,
              "4.9.b excludes class relation source lines from candidate reports");
        const auto duplicateClockControl = analyze(
            "entity m is port(clk,d : in bit; q1,q2 : out bit); end entity;\n"
            "architecture rtl of m is begin\n"
            "p1: process(clk) begin if rising_edge(clk) then q1<=d; end if; end process;\n"
            "p2: process(clk,d) begin if clk and clk then q2<=d; end if; end process;\n"
            "end architecture;\n", true);
        check(count(duplicateClockControl, "GJB9765.4.9.b") == 1,
              "4.9.b de-duplicates one control name within one native condition map");
        const auto repeatedClockAcrossConditions = analyze(
            "entity m is port(clk,d : in bit; q1,q2,q3 : out bit); end entity;\n"
            "architecture rtl of m is begin\n"
            "p1: process(clk) begin if rising_edge(clk) then q1<=d; end if; end process;\n"
            "p2: process(clk,d) begin\n"
            "if clk then q2<=d; end if;\n"
            "if clk then q3<=d; end if;\n"
            "end process; end architecture;\n", true);
        check(count(repeatedClockAcrossConditions, "GJB9765.4.9.b") == 2,
              "4.9.b preserves repeated clock candidates across distinct conditions");
        const auto repeatedConditionalAssignmentClock = analyze(
            "entity m is port(clk,d : in bit; q1,q2 : out bit); end entity;\n"
            "architecture rtl of m is begin\n"
            "p1: process(clk) begin if rising_edge(clk) then q1<=d; end if; end process;\n"
            "q2 <= d when clk else not d when clk else '0';\n"
            "end architecture;\n", true);
        check(count(repeatedConditionalAssignmentClock, "GJB9765.4.9.b") == 2,
              "4.9.b preserves one candidate for each AST_WHEN condition child");
        const auto generateRelationDoesNotEscape = analyze(
            "entity m is port(clk,d : in bit; q1,q2 : out bit); end entity;\n"
            "architecture rtl of m is begin\n"
            "g: if true generate\n"
            "p1: process(clk) begin if rising_edge(clk) then q1<=d; end if; end process;\n"
            "end generate;\n"
            "p2: process(clk,d) begin if clk then q2<=d; end if; end process;\n"
            "end architecture;\n", true);
        check(count(generateRelationDoesNotEscape, "GJB9765.4.9.b") == 0,
              "4.9.b excludes clock relations produced by generate-nested processes");
        const auto generateCandidateDoesNotReport = analyze(
            "entity m is port(clk,d : in bit; q1,q2 : out bit); end entity;\n"
            "architecture rtl of m is begin\n"
            "p1: process(clk) begin if rising_edge(clk) then q1<=d; end if; end process;\n"
            "g: if true generate\n"
            "p2: process(clk,d) begin if clk then q2<=d; end if; end process;\n"
            "end generate;\n"
            "end architecture;\n", true);
        check(count(generateCandidateDoesNotReport, "GJB9765.4.9.b") == 0,
              "4.9.b excludes control candidates in generate-nested processes");
        const auto dataControlsMux = analyze(
            "entity m is port(clk,d : in bit; q : out bit); end entity;\n"
            "architecture rtl of m is signal ordinary : bit; begin\n"
            "p1: process(clk) begin if rising_edge(clk) then ordinary<=d; end if; end process;\n"
            "p2: process(ordinary,d) begin if ordinary then q<=d; else q<='0'; end if;\n"
            "end process; end architecture;\n", true);
        check(count(dataControlsMux, "GJB9765.4.9.b") == 0,
              "attachment 4.9.b accepts a nonclock signal as direct MUX control");
        const auto clockMuxAssessment = std::ranges::find_if(
            dataControlsMux.standardAssessments,
            [](const auto& entry) { return entry.clauseId == "GJB9765.4.9.b"; });
        check(clockMuxAssessment != dataControlsMux.standardAssessments.end() &&
                  clockMuxAssessment->state ==
                      analysis::StandardRuleState::ManualReviewRequired,
              "bounded 4.9.b clean result still needs manual review");
    }
    {
        const auto vectorLogical = analyzeVerilog(
            "module m #(P_BUS_SIZE_TOP = 12)(input CLK,\n"
            "input [P_BUS_SIZE_TOP-1:0] IN1, IN2,\n"
            "output reg [P_BUS_SIZE_TOP-1:0] OUT1, OUT2, OUT3);\n"
            "always @(posedge CLK) begin\n"
            "OUT1 <= IN1 && IN2; OUT2 <= IN1 || IN2; OUT3 <= !IN1;\n"
            "end endmodule\n");
        check(count(vectorLogical, "GJB10157.8.8") == 3,
              "attachment 8.8 vector logical AND, OR and NOT report");
        const auto bitwise = analyzeVerilog(
            "module m #(P_BUS_SIZE_TOP = 12)(input CLK,\n"
            "input [P_BUS_SIZE_TOP-1:0] IN1, IN2,\n"
            "output reg [P_BUS_SIZE_TOP-1:0] OUT1, OUT2, OUT3);\n"
            "always @(posedge CLK) begin\n"
            "OUT1 <= IN1 & IN2; OUT2 <= IN1 | IN2; OUT3 <= ~IN1;\n"
            "end endmodule\n");
        check(count(bitwise, "GJB10157.8.8") == 0,
              "attachment 8.8 bitwise equivalents remain clear");
        const auto boundary = analyzeVerilog(
            "module m(input a,b, input [3:0] v, output reg q);\n"
            "always @* begin q = a && b; q = !v[0]; q = v[1:0] || b;\n"
            "q = v[0] && b; end endmodule\n");
        check(count(boundary, "GJB10157.8.8") == 1,
              "8.8 scalar and bit-select operands excluded, two-bit slice reported");
        const auto unknown = analyzeVerilog(
            "module m #(P = 4)(input [P-1:0] v, output reg q);\n"
            "always @* q = v && missing; endmodule\n");
        check(count(unknown, "GJB10157.8.8") == 1,
              "one definitely vector operand suffices even if its peer is unresolved");
        const auto compoundOperands = analyzeVerilog(
            "module m(input a,b,q, input [3:0] v, output o);\n"
            "assign o = {a,b} && q; assign o = {v[0]} && q;\n"
            "assign o = (a | v) && q; assign o = q && a | v;\n"
            "assign o = !(a & v); assign o = 1'b1 && q; assign o = 8'h1 && q;\n"
            "endmodule\n");
        check(count(compoundOperands, "GJB10157.8.8") == 5,
              "8.8 resolves concatenations, parenthesized bitwise expressions and sized literals");
        const auto arithmeticOperands = analyzeVerilog(
            "module m(input a,b, input [2:0] v,w, output o);\n"
            "assign o=(v+w)&&a; assign o=a||(v*w); assign o=!(v**w);\n"
            "assign o={2{a}}&&b; assign o={2{v[0]}}||b;\nendmodule\n");
        check(count(arithmeticOperands, "GJB10157.8.8") == 5,
              "8.8 applies recovered flag-zero arithmetic and replication widths to logical children");
        const auto nestedLogical = analyzeVerilog(
            "module m(input a,b, input [3:0] v, output o);\n"
            "assign o=a||(v&&b); assign o=(v&&b)||a; assign o=!(v&&b);\n"
            "assign o=(a||b)&&v;\nendmodule\n");
        check(count(nestedLogical, "GJB10157.8.8") == 4,
              "8.8 reports only logical nodes with direct vector children, not their one-bit parents");
        const auto arithmeticScalar = analyzeVerilog(
            "module m(input a,b, output o);\n"
            "assign o=(a+b)&&b; assign o=a||(a*b); assign o=!(a**b);\nendmodule\n");
        check(count(arithmeticScalar, "GJB10157.8.8") == 0,
              "8.8 keeps flag-zero arithmetic trees of scalar operands clear");
        const auto overridden = analyzeVerilog(
            "module child #(P = 4)(input [P-1:0] v, input b, output q);\n"
            "assign q = v && b; endmodule\n"
            "module top(input v,b, output q); child #(.P(1)) u(v,b,q); endmodule\n");
        check(count(overridden, "GJB10157.8.8") == 0,
              "parameterized module with explicit project override stays unresolved");
        const std::string conditional =
            "module m(input [3:0] v, input b, output q);\n"
            "`ifdef VECTOR_LOGIC\nassign q = v && b;\n"
            "`else\nassign q = v[0] && b;\n`endif\nendmodule\n";
        check(count(analyzeVerilog(conditional), "GJB10157.8.8") == 0,
              "inactive vector-logical branch cannot report");
        analysis::AnalysisOptions enabled;
        enabled.standardId = "GJB10157-2021";
        enabled.defines.push_back("VECTOR_LOGIC");
        check(count(analysis::Analyzer(enabled).analyzeSource(
                  {"probe.v", core::HdlLanguage::Verilog, conditional}),
              "GJB10157.8.8") == 1,
              "active vector-logical branch reports");
        const auto review = std::ranges::find_if(bitwise.standardAssessments,
            [](const auto& entry) { return entry.clauseId == "GJB10157.8.8"; });
        check(review != bitwise.standardAssessments.end() &&
              review->state == analysis::StandardRuleState::ManualReviewRequired,
              "8.8 no finding remains manual review rather than certified compliance");
    }
    {
        const auto violating = analyzeVerilog(
            "module m; wire [7:0] a,o; wire [5:0] b;\n"
            "assign o=a|b; assign o=a&b; assign o=a^b; "
            "assign o=a^~b; assign o=a~^b; assign o=~a; endmodule\n");
        check(count(violating, "GJB10157.8.2") == 5,
              "attachment 8.2 binary AND, OR, XOR and both XNOR spellings report");
        const auto compliant = analyzeVerilog(
            "module m; wire [7:0] a,b,o; assign o=a|b; assign o=a&b; "
            "assign o=a^b; assign o=a^~b; assign o=a~^b; assign o=~a; endmodule\n");
        check(count(compliant, "GJB10157.8.2") == 0,
              "attachment 8.2 equal-width and unary bitwise operations stay clear");
        const auto boundary = analyzeVerilog(
            "module m(input [7:0] a, input [5:0] b, output [7:0] o, output q);\n"
            "assign o=a[5:0]&b; assign o=a[3:0]|4'h0; assign o=a&6'h0;\n"
            "always @* if (a|b) ;\nendmodule\n");
        check(count(boundary, "GJB10157.8.2") == 2,
              "8.2 resolves slices and sized literals and scans bitwise conditions inside always");
        const auto nested = analyzeVerilog(
            "module m(input [2:0] a,b, input [4:0] c, output [4:0] o);\n"
            "assign o=(a+b)&c; assign o=(a*b)|c; assign o=(a**b)^c;\n"
            "assign o=(a-b)^a; assign o={a,b}&6'h0;\nendmodule\n");
        check(count(nested, "GJB10157.8.2") == 3,
              "8.2 uses recovered flag-zero nested arithmetic widths on both operands");
        const auto nestedRight = analyzeVerilog(
            "module m(input [2:0] a,b, input [4:0] c, output [4:0] o);\n"
            "always @* begin o=c&(a+b); o=c|(a*b); o=c^(a**b); end\nendmodule\n");
        check(count(nestedRight, "GJB10157.8.2") == 3,
              "8.2 applies flag-zero width inference to the right child in process roots");
        const auto review = std::ranges::find_if(compliant.standardAssessments,
            [](const auto& entry) { return entry.clauseId == "GJB10157.8.2"; });
        check(review != compliant.standardAssessments.end() &&
              review->state == analysis::StandardRuleState::ManualReviewRequired,
              "8.2 no finding remains manual review rather than certified compliance");
    }
    {
        const auto violating = analyzeVerilog(
            "module m #(P=3)(input CLK, input [P-1:0] MY_IN1, MY_IN2, "
            "output reg [P-1:0] MY_OUT1, MY_OUT2, MY_OUT3);\n"
            "reg signed [P-1:0] a; reg [P-1:0] b;\n"
            "always @(posedge CLK) begin a<=MY_IN1; b<=MY_IN2; "
            "MY_OUT1<=a*b; MY_OUT2<=a+b; MY_OUT3<=a-b; end endmodule\n");
        check(count(violating, "GJB10157.8.3") == 3,
              "attachment 8.3 mixed signed/unsigned multiply, add and subtract report");
        const auto compliant = analyzeVerilog(
            "module m #(P=3)(input CLK, input [P-1:0] MY_IN1, MY_IN2, "
            "output reg signed [P-1:0] MY_OUT1, output reg [P-1:0] MY_OUT2, MY_OUT3);\n"
            "reg signed [P-1:0] a; reg unsigned [P-1:0] b;\n"
            "always @(posedge CLK) begin a<=MY_IN1; b<=MY_IN2; "
            "MY_OUT1<=a*a; MY_OUT2<=b+b; MY_OUT3<=b-MY_IN2; end endmodule\n");
        check(count(compliant, "GJB10157.8.3") == 0,
              "attachment 8.3 same-signedness arithmetic stays clear");
        const auto boundary = analyzeVerilog(
            "module m(input signed [3:0] s, input [3:0] u, output [7:0] q);\n"
            "assign q=(s)*(u); assign q=s/u; assign q=s%u; assign q=s**u;\n"
            "assign q=s&u; assign q=-s; assign q=u+4'd1; endmodule\n");
        check(count(boundary, "GJB10157.8.3") == 4,
              "8.3 covers parenthesized multiply, divide, modulo and power while excluding bitwise, unary and literal-only peers");
        const auto nested = analyzeVerilog(
            "module m(input signed [3:0] s, input [3:0] u, output [7:0] q);\n"
            "assign q=s*4'd2+u; assign q=(s+u)*s;\n"
            "assign q=u+(u*2); assign q=s**(u+u);\nendmodule\n");
        check(count(nested, "GJB10157.8.3") == 4,
              "8.3 recursively classifies every arithmetic AST subtree, including unparenthesized chains");
        const auto nestedCompliant = analyzeVerilog(
            "module m(input signed [3:0] s, input [3:0] u, output [7:0] q);\n"
            "assign q=(s+s)*(s+s); assign q=(u+u)+(u*u); "
            "assign q=s+4'sd1;\nendmodule\n");
        check(count(nestedCompliant, "GJB10157.8.3") == 0,
              "8.3 ignores literals and keeps recursively uniform arithmetic subtrees clear");
        const auto review = std::ranges::find_if(compliant.standardAssessments,
            [](const auto& entry) { return entry.clauseId == "GJB10157.8.3"; });
        check(review != compliant.standardAssessments.end() &&
              review->state == analysis::StandardRuleState::ManualReviewRequired,
              "8.3 no finding remains manual review rather than certified compliance");
    }
    {
        const auto violating = analyzeVerilog(
            "module m(\n"
            "input CLK,\n"
            "input INPUT1, B1,\n"
            "output reg C1,\n"
            "output C2, output C3,\n"
            "output C4\n"
            "); endmodule\n");
        check(count(violating, "GJB10157.5.1.7") == 2,
              "attachment 5.1.7 inherited and repeated direction on one line report");
        const auto compliant = analyzeVerilog(
            "module m(\ninput CLK,\ninput INPUT1,\ninput B1,\n"
            "output reg C1,\noutput C2,\noutput C3\n); endmodule\n");
        check(count(compliant, "GJB10157.5.1.7") == 0,
              "attachment 5.1.7 one port per physical line stays clear");
        const auto boundary = analyzeVerilog(
            "module first #(parameter A=1, B=2)(\n"
            "input [3:0] x,\ninput [3:0] y\n); endmodule\n"
            "module second(input a, input b); endmodule\n");
        check(count(boundary, "GJB10157.5.1.7") == 1,
              "parameter commas and separate modules do not count as port pairs");
        const auto threeOnOneLine = analyzeVerilog(
            "module triple(input first, input second, output third); endmodule\n");
        check(count(threeOnOneLine, "GJB10157.5.1.7") == 3,
              "native 5.1.7 emits one finding for every same-line port-record pair");
        const auto exactDiagnostic = std::ranges::find_if(
            threeOnOneLine.violations, [](const auto& item) {
                return item.ruleId == "GJB10157.5.1.7";
            });
        check(exactDiagnostic != threeOnOneLine.violations.end() &&
                  exactDiagnostic->severity == analysis::Severity::Warning &&
                  exactDiagnostic->message == "一行中存在多个端口声明",
              "5.1.7 preserves the native fixed message and Warning severity");
        const auto allDirections = analyzeVerilog(
            "module directions(input first, inout second, output third); endmodule\n");
        check(count(allDirections, "GJB10157.5.1.7") == 3,
              "native inputport, inoutport and outputport producers share one pair set");
        const auto nonAnsi = analyzeVerilog(
            "module legacy(a, b); input a; output b; endmodule\n");
        check(count(nonAnsi, "GJB10157.5.1.7") == 1,
              "native producer-backed non-ANSI input and output declarations report");
        const auto sameFileCrossModule = analyzeVerilog(
            "module first(input a); endmodule module second(input b); endmodule\n");
        check(count(sameFileCrossModule, "GJB10157.5.1.7") == 1,
              "native 5.1.7 groups port records by file and physical line, not module");
        const std::string conditional =
            "module m(\n`ifdef EXTRA\ninput a, input b,\n`else\n"
            "input a,\ninput b,\n`endif\noutput c\n); endmodule\n";
        check(count(analyzeVerilog(conditional), "GJB10157.5.1.7") == 0,
              "inactive same-line ports do not report");
        analysis::AnalysisOptions enabled;
        enabled.standardId = "GJB10157-2021";
        enabled.defines.push_back("EXTRA");
        check(count(analysis::Analyzer(enabled).analyzeSource(
                  {"probe.v", core::HdlLanguage::Verilog, conditional}),
              "GJB10157.5.1.7") == 1,
              "active same-line ports report");
        const auto review = std::ranges::find_if(compliant.standardAssessments,
            [](const auto& entry) { return entry.clauseId == "GJB10157.5.1.7"; });
        check(review != compliant.standardAssessments.end() &&
              review->state == analysis::StandardRuleState::ManualReviewRequired,
              "5.1.7 no finding remains manual review");
    }
    {
        const auto multiple = analyzeVerilog(
            "module m(input CLK, ARST1, ARST2, D, output reg Q);\n"
            "always @(posedge ARST1 or posedge ARST2 or posedge CLK) begin\n"
            "if (ARST1) Q <= 1'b1; else if (ARST2) Q <= 1'b1; else Q <= D;\n"
            "end endmodule\n");
        check(count(multiple, "GJB10157.6.2.10") == 1,
              "attachment two asynchronous resets in one always report");
        const auto activeLow = analyzeVerilog(
            "module m(input CLK, ARST1_N, ARST2_N, D, output reg Q);\n"
            "always_ff @(negedge ARST1_N or negedge ARST2_N or posedge CLK) begin\n"
            "if (!ARST1_N) Q <= 1'b0; else if (!ARST2_N) Q <= 1'b0; else Q <= D;\n"
            "end endmodule\n");
        check(count(activeLow, "GJB10157.6.2.10") == 1,
              "active-low SystemVerilog always_ff with two reset groups reports");
        const auto threeResets = analyzeVerilog(
            "module m(input CLK, R1, R2, R3, D, output reg Q);\n"
            "always @(posedge CLK or posedge R1 or posedge R2 or posedge R3) begin\n"
            "if (R1) Q <= 1'b0; else if (R2) Q <= 1'b0; "
            "else if (R3) Q <= 1'b0; else Q <= D; end endmodule\n");
        check(count(threeResets, "GJB10157.6.2.10") == 1,
              "native rule emits once after the second of three qualifying reset groups");
        const auto repeatedReset = analyzeVerilog(
            "module m(input CLK, RST, D, output reg Q);\n"
            "always @(posedge CLK or posedge RST or negedge RST) begin\n"
            "if (RST) Q <= 1'b0; else Q <= D; end endmodule\n");
        check(count(repeatedReset, "GJB10157.6.2.10") == 0,
              "repeated edges of one reset name do not create two reset groups");
        const auto single = analyzeVerilog(
            "module m(input CLK, ARST1, D, output reg Q);\n"
            "always @(posedge ARST1 or posedge CLK) begin\n"
            "if (ARST1) Q <= 1'b1; else Q <= D;\n"
            "end endmodule\n");
        check(count(single, "GJB10157.6.2.10") == 0,
              "attachment single asynchronous reset remains clear");
        const std::string conditionalSource =
            "module m(input CLK, ARST1, ARST2, D, output reg Q);\n"
            "`ifdef EXTRA_RESET\n"
            "always @(posedge ARST1 or posedge ARST2 or posedge CLK) begin\n"
            "if (ARST1 || ARST2) Q <= 1'b1; else Q <= D; end\n"
            "`else\n"
            "always @(posedge ARST1 or posedge CLK) begin\n"
            "if (ARST1) Q <= 1'b1; else Q <= D; end\n"
            "`endif\nendmodule\n";
        const auto inactive = analyzeVerilog(conditionalSource);
        check(count(inactive, "GJB10157.6.2.10") == 0,
              "inactive conditional-compilation branch cannot report multiple resets");
        analysis::AnalysisOptions enabledOptions;
        enabledOptions.standardId = "GJB10157-2021";
        enabledOptions.defines.push_back("EXTRA_RESET");
        const auto active = analysis::Analyzer(enabledOptions).analyzeSource(
            {"probe.v", core::HdlLanguage::Verilog, conditionalSource});
        check(count(active, "GJB10157.6.2.10") == 1,
              "enabled conditional-compilation branch reports multiple resets");
        const auto assessment = std::ranges::find_if(single.standardAssessments,
            [](const auto& entry) { return entry.clauseId == "GJB10157.6.2.10"; });
        check(assessment != single.standardAssessments.end() &&
              assessment->state == analysis::StandardRuleState::ManualReviewRequired,
              "partial multiple-reset check does not certify vendor-equivalent compliance");
    }
    {
        const auto overflowing = analyzeVerilog(
            "module m(input clk, output reg [15:0] a,b,c);\n"
            "always @(posedge clk) begin\n"
            "a <= 2'd10; b <= 2'b101; c <= 2'hFFF;\n"
            "end endmodule\n");
        check(count(overflowing, "GJB10157.8.6") == 3,
              "attachment 8.6 decimal, binary and hex overflow examples report");
        const auto fitting = analyzeVerilog(
            "module m(input clk, output reg [15:0] a,b,c);\n"
            "always @(posedge clk) begin\n"
            "a <= 4'd10; b <= 3'b101; c <= 12'hFFF;\n"
            "end endmodule\n");
        check(count(fitting, "GJB10157.8.6") == 0,
              "attachment 8.6 fitting decimal, binary and hex examples stay clear");
        const auto boundary = analyzeVerilog(
            "module m(output reg [127:0] a,b,c,d,e); always @* begin\n"
            "a = 4'd15; b = 4'd16; c = 64'h1_FFFF_FFFF_FFFF_FFFF;\n"
            "d = 2'bx11; e = 2'sd3; end endmodule\n");
        check(count(boundary, "GJB10157.8.6") == 3,
              "8.6 exact fit and signed fit stay clear; binary unknown digits and 1-bit overflows report");
        const auto binaryLeadingZeros = analyzeVerilog(
            "module m(output reg [7:0] a,b,c,d,e); always @* begin\n"
            "a = 2'b001; b = 1'b00; c = 3'b001; d = 2'b0_1; e = 2'h01;\n"
            "end endmodule\n");
        check(count(binaryLeadingZeros, "GJB10157.8.6") == 2,
              "native binary-width branch retains leading zeros; hex branch trims them");
        const auto signedAndUnknown = analyzeVerilog(
            "module m(output reg [15:0] a,b,c,d,e,f); always @* begin\n"
            "a=2'sd3; b=2'sd4; c=3'sb1000; d=3'sh8; e=2'bz01; f=1'b?1;\n"
            "end endmodule\n");
        check(count(signedAndUnknown, "GJB10157.8.6") == 5,
              "native signed base dispatch and binary X/Z/? encoded width are integrated");
        const auto review = std::ranges::find_if(fitting.standardAssessments,
            [](const auto& entry) { return entry.clauseId == "GJB10157.8.6"; });
        check(review != fitting.standardAssessments.end() &&
              review->state == analysis::StandardRuleState::ManualReviewRequired,
              "8.6 bounded overflow detector does not claim vendor-equivalent compliance");
    }
    {
        const auto missing = analyzeVerilog(
            "module m #(P = 'd4)(input clk, output reg [3:0] a,b,c,d);\n"
            "always @(posedge clk) begin\n"
            "a <= 'b0100; b <= 'o5; c <= 4; d <= 'hf;\n"
            "end endmodule\n");
        check(count(missing, "GJB10157.8.9") == 5,
              "attachment unsized parameter and four unsized assignment literals report");
        const auto explicitSizes = analyzeVerilog(
            "module m #(P = 4'd4)(input clk, output reg [3:0] a,b,c,d);\n"
            "always @(posedge clk) begin\n"
            "a <= 4'b0100; b <= 4'o5; c <= 4'd4; d <= 4'hf;\n"
            "end endmodule\n");
        check(count(explicitSizes, "GJB10157.8.9") == 0,
              "attachment explicitly sized and based values stay clear");
        const auto boundary = analyzeVerilog(
            "module m(input [7:0] a, output reg [7:0] b);\n"
            "always @* b = a[3:0] + 8'd1;\n"
            "endmodule\n");
        check(count(boundary, "GJB10157.8.9") == 0,
              "unrecovered bit-select numbers are not treated as assigned literals");
        const auto zeroWidth = analyzeVerilog(
            "module m(output reg [3:0] q); always @* q = 0'd4; endmodule\n");
        check(count(zeroWidth, "GJB10157.8.9") == 1,
              "native zero-width branch is reported");
        const auto structuralContexts = analyzeVerilog(
            "module m(input s, output reg q);\n"
            "function f; input x; begin f=x; end endfunction\n"
            "always @* begin\n"
            "if (4) q=f(5); case(s) 6: q=1'b0; default: q=1'b1; endcase\n"
            "repeat(7) q=q; for(integer i=0; i<8; i=i+1) q=9;\n"
            "end endmodule\n");
        check(count(structuralContexts, "GJB10157.8.9") == 6,
              "8.9 follows AST constants in conditions, calls, case labels, repeat and for condition/body");
        const auto excludedRangesAndForClauses = analyzeVerilog(
            "module m(input [7:0] a, output reg q); integer i;\n"
            "always @* for(i=0; i<8'd8; i=i+1) q=a[4]; endmodule\n");
        check(count(excludedRangesAndForClauses, "GJB10157.8.9") == 0,
              "8.9 bounded adapter excludes AST range/select and omitted for init/increment constants");
        const std::string conditionalLiteral =
            "module m(output reg [3:0] q); always @* begin\n"
            "`ifdef UNSIZED_VALUE\n"
            "q = 4;\n"
            "`else\n"
            "q = 4'd4;\n"
            "`endif\nend endmodule\n";
        check(count(analyzeVerilog(conditionalLiteral), "GJB10157.8.9") == 0,
              "inactive unsized literal is not reported");
        analysis::AnalysisOptions unsizedOptions;
        unsizedOptions.standardId = "GJB10157-2021";
        unsizedOptions.defines.push_back("UNSIZED_VALUE");
        check(count(analysis::Analyzer(unsizedOptions).analyzeSource(
                  {"probe.v", core::HdlLanguage::Verilog, conditionalLiteral}),
              "GJB10157.8.9") == 1,
              "active unsized literal reports after preprocessing");
        const auto assessment = std::ranges::find_if(explicitSizes.standardAssessments,
            [](const auto& entry) { return entry.clauseId == "GJB10157.8.9"; });
        check(assessment != explicitSizes.standardAssessments.end() &&
              assessment->state == analysis::StandardRuleState::ManualReviewRequired,
              "bounded literal scan does not certify all native constant contexts");
    }
    {
    auto result = analyzeVerilog(
        "module m(input rst, clk, a, b, output reg c);\n"
        "always @(rst, clk, a, b) begin\n"
        "  if (rst) c = a; else if (clk) c = b;\n"
        "end\nendmodule\n");
    check(count(result, "GJB10157.7.2.1") == 1,
          "documented incomplete combinational if/else reports latch subpredicate");
    result = analyzeVerilog(
        "module m(input rst, clk, a, b, output reg c);\n"
        "always @(rst, clk, a, b) begin\n"
        "  if (rst) c = a; else if (clk) c = b; else c = a;\n"
        "end\nendmodule\n");
    check(count(result, "GJB10157.7.2.1") == 0,
          "documented complete combinational branches do not infer latch");
    const auto loopAssessment = std::ranges::find_if(result.standardAssessments,
        [](const auto& entry) { return entry.clauseId == "GJB10157.7.2.1"; });
    check(loopAssessment != result.standardAssessments.end() &&
          loopAssessment->state == analysis::StandardRuleState::ManualReviewRequired,
          "7.2.1 partial check does not certify absence of native netlist loops");
    result = analyzeVerilog(
        "module m(input clk, a, output reg c);\n"
        "always @(posedge clk) if (a) c <= 1'b1;\nendmodule\n");
    check(count(result, "GJB10157.7.2.1") == 0,
          "clocked enable describes a flip-flop and is not a combinational latch");
    }
    check(count(portWidthExample(
              "input [1:0] A, input [2:0] B, input [1:0] C, input [2:0] D",
              ".A(A), .B(B), .C(C), .D(D)"), "GJB10157.9.1.1") == 4,
          "published four named port-width mismatches report");
    check(count(portWidthExample(
              "input [2:0] A, input [1:0] B, input [2:0] C, input [1:0] D",
              ".A(A), .B(B), .C(C), .D(D)"), "GJB10157.9.1.1") == 0,
          "published matching named port widths stay clear");
    check(count(portWidthExample(
              "input [1:0] A, input [2:0] B, input [1:0] C, input [2:0] D",
              "A, B, C, D"), "GJB10157.9.1.1") == 4,
          "positional port-width mismatches report");
    check(count(portWidthExample(
              "input [2:0] A, input [1:0] B, input [2:0] C, input [1:0] D",
              ".A(A[1:0]), .B(B), .C(C), .D(D)"), "GJB10157.9.1.1") == 1,
          "constant slice width is compared with the formal port");
    check(count(analyzeVerilog(
              "module child(input [3:0] A); endmodule\n"
              "module parent(input [1:0] x,y); child u0(.A(x+y)); endmodule\n"),
              "GJB10157.9.1.1") == 1,
          "flag-zero arithmetic actual width is compared with the formal port");
    check(count(analyzeVerilog(
              "module child(input [1:0] A); endmodule\n"
              "module parent(input [1:0] x,y); child u0(.A(x+y)); endmodule\n"),
              "GJB10157.9.1.1") == 0,
          "matching flag-zero arithmetic actual stays clear");
    check(count(analyzeVerilog(
              "module child(input [7:0] A); endmodule\n"
              "module parent(input [3:0] x,y); child u0(.A(x*y)); endmodule\n"),
              "GJB10157.9.1.1") == 1,
          "port rule uses flag-zero multiplication maximum rather than assignment flag-one sum");
    check(count(analyzeVerilog(
              "module child(input [3:0] A); endmodule\n"
              "module parent(input [1:0] x,y); child u0(.A({x,y})); endmodule\n"),
              "GJB10157.9.1.1") == 0,
          "matching concatenation actual width stays clear");
    check(count(analyzeVerilog(
              "module child(input [2:0] A); endmodule\n"
              "module parent(input [1:0] x,y); child u0({x,y}); endmodule\n"),
              "GJB10157.9.1.1") == 1,
          "positional concatenation actual width mismatch reports");
    check(count(analyzeVerilog(
              "module child(input [1:0] A); endmodule\n"
              "module parent(input [2:0] A); child u0(.A); endmodule\n"),
              "GJB10157.9.1.1") == 1,
          "SystemVerilog shorthand named connection resolves the same-name actual");
    check(count(analyzeVerilog(
              "module child(input [1:0] A); endmodule\n"
              "module parent(input [2:0] A);\n`ifdef UNUSED\nchild bad(.A(A));\n`endif\n"
              "child good(.A(A[1:0]));\nendmodule\n"),
              "GJB10157.9.1.1") == 0,
          "inactive port connections are excluded after configured preprocessing");
    check(count(analyzeVerilog(
              "module child #(parameter W=2)(input [W-1:0] A); endmodule\n"
              "module parent(input [2:0] A); child u0(.A(A)); endmodule\n"),
              "GJB10157.9.1.1") == 0,
          "parameterized formal width remains unknown");
    check(count(analyzeVerilog(
              "module parent(input [2:0] A); blackbox u0(.A(A)); endmodule\n"),
              "GJB10157.9.1.1") == 0,
          "unresolved target does not imply a port mismatch");
    analysis::AnalysisOptions portWidthOptions;
    portWidthOptions.standardId = "GJB10157-2021";
    const auto splitPortWidths = analysis::Analyzer(portWidthOptions).analyzeSources({
        {"child.v", core::HdlLanguage::Verilog,
         "module child(input [1:0] A); endmodule\n"},
        {"parent.v", core::HdlLanguage::Verilog,
         "module parent(input [2:0] A); child u0(.A(A)); endmodule\n"}});
    check(count(splitPortWidths, "GJB10157.9.1.1") == 1,
          "port-width mismatch across source files reports");
    const auto duplicateTargets = analysis::Analyzer(portWidthOptions).analyzeSources({
        {"child_a.v", core::HdlLanguage::Verilog,
         "module child(input [1:0] A); endmodule\n"},
        {"child_b.v", core::HdlLanguage::Verilog,
         "module child(input [2:0] A); endmodule\n"},
        {"parent.v", core::HdlLanguage::Verilog,
         "module parent(input [2:0] A); child u0(.A(A)); endmodule\n"}});
    check(count(duplicateTargets, "GJB10157.9.1.1") == 0,
          "duplicate target definitions remain unresolved");
    check(count(analyzeVerilog(
              "module probe(input [7:0] A, output [8:0] Z); assign Z = A; endmodule\n"),
              "GJB10157.9.1.2") == 1,
          "assignment widening is still a documented width mismatch");
    check(count(analyzeVerilog(
              "module probe(input clk, input [7:0] A, output reg [8:0] Z); "
              "always @(posedge clk) Z <= A; endmodule\n"),
              "GJB10157.9.1.2") == 1,
          "nonblocking assignment widening reports");
    check(count(analyzeVerilog(
              "module probe(input [7:0] A, output [8:0] Z); assign Z = 8'd0; endmodule\n"),
              "GJB10157.9.1.2") == 1,
          "explicitly sized literal widening reports");
    check(count(analyzeVerilog(
              "module probe(input [7:0] A, output [8:0] Z); assign Z = 9'd0; endmodule\n"),
              "GJB10157.9.1.2") == 0,
          "equal-width explicit literal assignment stays clear");
    check(count(analyzeVerilog(
              "module probe(input [7:0] A, output [8:0] Z); assign Z = A + 1'b1; endmodule\n"),
              "GJB10157.9.1.2") == 1,
          "flag-zero and flag-one addition widths both expose widening");
    check(count(analyzeVerilog(
              "module probe(input [3:0] A,B, output [3:0] Z); assign Z = A * B; endmodule\n"),
              "GJB10157.9.1.2") == 0,
          "multiplication stays clear when flag-zero width matches the target");
    check(count(analyzeVerilog(
              "module probe(input [3:0] A,B, output [7:0] Z); assign Z = A * B; endmodule\n"),
              "GJB10157.9.1.2") == 0,
          "flag-one multiplication sum can clear an initial flag-zero mismatch");
    check(count(analyzeVerilog(
              "module probe(input [3:0] A,B, output [8:0] Z); assign Z = A * B; endmodule\n"),
              "GJB10157.9.1.2") == 1,
          "multiplication reports when neither recovered width matches the target");
    check(count(analyzeVerilog(
              "module probe(input [3:0] A, output [4:0] Z); assign Z = A * 4'd2; endmodule\n"),
              "GJB10157.9.1.2") == 0,
          "flag-one multiplication preserves the decoded-constant-two one-bit special case");
    check(count(analyzeVerilog(
              "module probe(input s, input [7:0] A, input [3:0] B, output [7:0] Z); "
              "assign Z = s ? A : B; endmodule\n"),
              "GJB10157.9.1.2") == 1,
          "ternary true and false branches are compared independently");
    check(count(analyzeVerilog(
              "module probe(input s, input [3:0] A,B, output [7:0] Z); "
              "assign Z = s ? A : B; endmodule\n"),
              "GJB10157.9.1.2") == 2,
          "two mismatching ternary branches produce two native comparisons");
    check(count(analyzeVerilog(
              "module probe #(parameter W=8)(input [W-1:0] A, output [8:0] Z); "
              "assign Z = A; endmodule\n"),
              "GJB10157.9.1.2") == 0,
          "parameterized assignment width remains unknown rather than guessed");
    check(count(analyzeVerilog(
              "module probe(input [7:0] A, output [8:0] Z);\n"
              "`ifdef UNUSED\nassign Z = A;\n`endif\n"
              "assign Z = 9'd0;\nendmodule\n"),
              "GJB10157.9.1.2") == 0,
          "inactive conditional-compilation assignments are excluded");
    check(count(analyzeVerilog(
              "module probe(input clk, input d, output reg q); "
              "always @(posedge clk) q <= #2 d; endmodule\n"),
              "GJB10157.9.1.6") == 1,
          "clocked assignment delay reports");
    check(count(analyzeVerilog(
              "module probe(input a, output reg q); always @* begin #2 q = a; end endmodule\n"),
              "GJB10157.9.1.6") == 1,
          "combinational statement delay reports");
    check(count(analyzeVerilog(
              "module probe(input a, output q); assign #2 q = a; endmodule\n"),
              "GJB10157.9.1.6") == 1,
          "continuous assignment delay reports");
    check(count(analyzeVerilog(
              "module probe(input a, output reg q); task drive; begin #3 q = a; end endtask "
              "always @* drive(); endmodule\n"),
              "GJB10157.9.1.6") == 1,
          "recovered behavioral statement delay also reports inside a task body");
    check(count(analyzeVerilog(
              "`define DLY 4\nmodule probe(input a, output reg q); "
              "always @* begin #(`DLY) q = a; end endmodule\n"),
              "GJB10157.9.1.6") == 1,
          "parenthesized macro statement delay reports");
    check(count(analyzeVerilog(
              "module probe(input a, output reg q); "
              "always @* begin @(posedge a); q = a; end endmodule\n"),
              "GJB10157.9.1.6") == 0,
          "event control without a hash delay stays clear");
    check(count(analyzeVerilog(
              "module child #(parameter W=1)(input [W-1:0] a, output [W-1:0] q); "
              "assign q = a; endmodule\n"
              "module parent(input [1:0] a, output [1:0] q); "
              "child #(.W(2)) u0(.a(a), .q(q)); endmodule\n"),
              "GJB10157.9.1.6") == 0,
          "parameter declarations and overrides are not delay controls");
    const auto vectorCondition = [&](std::string declaration, std::string condition) {
        return analyzeVerilog("module probe(input clk, " + declaration +
            ", output q, output reg r); assign q = " + condition +
            " ? 1'b1 : 1'b0; always @(posedge clk) "
            "if (" + condition + ") r <= 1'b1; else r <= 1'b0; endmodule\n");
    };
    check(count(vectorCondition("input [7:0] sel", "sel"), "GJB10157.9.2.10") == 2,
          "published vector if and ternary conditions both report");
    check(count(vectorCondition("input sel", "sel"), "GJB10157.9.2.10") == 0,
          "published scalar if and ternary conditions stay clear");
    check(count(vectorCondition("input [7:0] sel", "sel[0]"), "GJB10157.9.2.10") == 0,
          "constant bit selection is a one-bit condition");
    check(count(vectorCondition("input [7:0] sel", "sel[3:2]"), "GJB10157.9.2.10") == 2,
          "two-bit part selection is a vector condition");
    check(count(vectorCondition("input [7:0] sel", "sel == 8'd0"),
              "GJB10157.9.2.10") == 0,
          "one-bit comparison result is not confused with vector operand");
    check(count(analyzeVerilog(
              "module probe(input clk, input [3:0] a,b, output q, output reg r); "
              "assign q = (a+b) ? 1'b1 : 1'b0; always @(posedge clk) "
              "if ((a*b)) r <= 1'b1; else r <= 1'b0; endmodule\n"),
              "GJB10157.9.2.10") == 2,
          "flag-zero arithmetic ternary and balanced nested if conditions report");
    check(count(analyzeVerilog(
              "module probe(input clk, input a,b, output q, output reg r); "
              "assign q = {2{a}} ? 1'b1 : 1'b0; always @(posedge clk) "
              "if ({a,b}) r <= 1'b1; else r <= 1'b0; endmodule\n"),
              "GJB10157.9.2.10") == 2,
          "static replication and concatenation conditions report their recovered widths");
    check(count(analyzeVerilog(
              "module probe(input clk, input a,b, output q, output reg r); "
              "assign q = (a+b) ? 1'b1 : 1'b0; always @(posedge clk) "
              "if ((a*b)) r <= 1'b1; else r <= 1'b0; endmodule\n"),
              "GJB10157.9.2.10") == 0,
          "flag-zero arithmetic trees made only from scalar operands stay clear");
    check(count(analyzeVerilog(
              "module probe(input clk, input [3:0] a,b, input s, output q, output reg r); "
              "assign q = (a == b) ? 1'b1 : 1'b0; always @(posedge clk) "
              "if (a && s) r <= 1'b1; else r <= 1'b0; endmodule\n"),
              "GJB10157.9.2.10") == 0,
          "one-bit comparison and logical results stay clear for condition-width rule");
    const auto ifElseExample = [&](std::string process) {
        return analyzeVerilog("module probe(input a, input b, input clk, output reg q); " +
            process + " endmodule\n");
    };
    check(count(ifElseExample(
              "always @(a or b) begin if (a) q=0; if (b) begin if (a) q=1; end else q=0; end"),
              "GJB10157.9.2.11") == 2,
          "two documented missing combinational else branches report separately");
    check(count(ifElseExample(
              "always @* begin if (a) q=0; else q=1; end"),
              "GJB10157.9.2.11") == 0,
          "combinational if with else stays clear");
    check(count(ifElseExample(
              "always @* begin q=0; if (a) q=1; end"),
              "GJB10157.9.2.11") == 1,
          "prior default assignment does not invent an else branch");
    check(count(ifElseExample(
              "always @(posedge clk) begin if (a) q<=1; end"),
              "GJB10157.9.2.11") == 0,
          "the native admission helper excludes a clocked if without else");
    check(count(ifElseExample(
              "always begin if (a) q=1; end"),
              "GJB10157.9.2.11") == 1,
          "an edge-free bare always is admitted by the native helper");
    check(count(analyze(
              "module probe(input logic a, output logic q); "
              "always_latch begin if (a) q=1'b1; end endmodule\n"),
              "GJB10157.9.2.11") == 1,
          "always_latch lowers to an edge-free AST_ALWAYS and reports");
    check(count(analyze(
              "module probe(input logic clk, input logic a, output logic q); "
              "always_ff @(posedge clk) begin if (a) q<=1'b1; end endmodule\n"),
              "GJB10157.9.2.11") == 0,
          "always_ff with an immediate posedge child is rejected");
    check(count(analyzeVerilog(
              "module probe(input a, output reg q); event tick; "
              "always @tick begin if (a) q=1'b1; end endmodule\n"),
              "GJB10157.9.2.11") == 1,
          "a named AST_EDGE event is not one of the rejected edge types");
    check(count(ifElseExample(
              "always @* begin case (a) 1'b0: q=0; endcase end"),
              "GJB10157.9.2.11") == 0,
          "case without default is not mislabelled as missing if else");
    check(count(analyzeVerilog(
              "module probe(input a, output reg q); "
              "task drive; input v; begin if (v) q=1'b1; end endtask "
              "always @* drive(a); endmodule\n"),
              "GJB10157.9.2.11") == 1,
          "missing else in a task directly called by an always block reports");
    check(count(analyzeVerilog(
              "module probe(input a, output reg q); "
              "function f; input v; begin if (v) f=1'b1; end endfunction "
              "always @* q=f(a); endmodule\n"),
              "GJB10157.9.2.11") == 1,
          "missing else in a function directly called by an always block reports");
    check(count(analyzeVerilog(
              "module probe(input a, output reg q); "
              "task unused; input v; begin if (v) q=1'b1; end endtask "
              "always @* q=a; endmodule\n"),
              "GJB10157.9.2.11") == 0,
          "an uncalled task definition is not scanned by the native call expansion");
    check(count(analyzeVerilog(
              "module probe(input a, output reg q); "
              "task drive; input v; begin if (v) q=1'b1; end endtask "
              "always @* begin drive(a); drive(a); end endmodule\n"),
              "GJB10157.9.2.11") == 1,
          "repeated calls expand one task definition only once");
    check(count(analyzeVerilog(
              "module probe(input a, output reg q); "
              "task inner; input v; begin if (v) q=1'b1; end endtask "
              "task outer; begin inner(a); end endtask "
              "always @* outer; endmodule\n"),
              "GJB10157.9.2.11") == 0,
          "native expansion remains one level and does not infer nested task calls");
    check(count(analyzeVerilog(
              "module probe(input a, output reg q); "
              "task drive; input v; begin if (v) q=1'b1; end endtask "
              "initial drive(a); endmodule\n"),
              "GJB10157.9.2.11") == 0,
          "a task called only from initial is outside the AST_ALWAYS walk");
    check(count(analyzeVerilog(
              "module probe(input clk, input a, output reg q); "
              "task drive; input v; begin if (v) q=1'b1; end endtask "
              "always @(posedge clk) drive(a); endmodule\n"),
              "GJB10157.9.2.11") == 0,
          "a task called only from a clocked always remains outside admission");
    const auto forLoop = [&](std::string header) {
        return analyzeVerilog("module probe(input d, output reg q); integer i; "
            "always @* begin for (" + header + ") q = d; end endmodule\n");
    };
    check(count(forLoop("i=10; i<5; i=i+1"), "GJB10157.9.3.4") == 1,
          "published literal zero-iteration for loop reports");
    check(count(forLoop("i=0; i<5; i=i+1"), "GJB10157.9.3.4") == 0,
          "positive-iteration for loop stays clear");
    check(count(analyzeVerilog(
              "module probe(input d, output reg q);\ninteger i;\n"
              "always @* for (i=0; i<4; i=i+1) q=d;\nendmodule\n"),
              "GJB10157.5.1.6") == 0,
          "for header separators are not multiple assignments on a line");
    check(count(analyzeVerilog(
              "module probe(input d, output reg q, output reg r);\ninteger i;\n"
              "always @* for (i=0; i<4; i=i+1) begin q=d; r=d; end\nendmodule\n"),
              "GJB10157.5.1.6") == 1,
          "two loop-body assignments on one line still report");
    check(count(analyzeVerilog(
              "module probe(input d, output reg q, output reg r);\n"
              "always @* begin q=d; r=d; end\nendmodule\n"),
              "GJB10157.5.1.6") == 1,
          "two ordinary assignments on one line report");
    check(count(analyzeVerilog(
              "module probe(input d, output reg q, output reg r);\n"
              "always @* begin\nq=d;\nr=d;\nend\nendmodule\n"),
              "GJB10157.5.1.6") == 0,
          "assignments on separate physical lines stay clear");
    check(count(analyzeVerilog(
              "module probe(input d, output reg q);\n"
              "wire a; wire b;\nparameter P=1; localparam Q=2;\n"
              "always @* q=d;\nendmodule\n"),
              "GJB10157.5.1.6") == 0,
          "multiple declarations on one line are not assignments");
    check(count(analyzeVerilog(
              "module probe(input [1:0] d, output reg q);\n"
              "always @* if (d <= 2) q=d[0];\nendmodule\n"),
              "GJB10157.5.1.6") == 0,
          "comparison inside an if condition is not a second assignment");
    check(count(analyzeVerilog(
              "module probe(input d, output q, output r);\n"
              "assign q=d; assign r=d;\nendmodule\n"),
              "GJB10157.5.1.6") == 1,
          "two continuous assignments on one line report");
    auto assignmentLine = analyzeVerilog(
        "module probe(input d, output reg q, output reg r);\n"
        "always @* begin q=d; r=d; end\nendmodule\n");
    check(count(assignmentLine, "GJB10157.5.1.6") == 1 &&
          std::ranges::any_of(assignmentLine.violations, [](const auto& item) {
              return item.ruleId == "GJB10157.5.1.6" && item.line == 2 &&
                     item.column == 1 && item.severity == analysis::Severity::Warning &&
                     item.message == "一行中不止一个信号赋值语句";
          }), "native assignment-line warning uses fixed text and line-only location");
    check(count(analyzeVerilog(
              "module probe(input d, output reg q, output reg r); integer i;\n"
              "always @* begin for(i=0;i<1;i=i+1) q=d; r=d; end\nendmodule\n"),
              "GJB10157.5.1.6") == 0,
          "a bare FOR-body assignment is removed as a direct AST_FOR child");
    check(count(analyzeVerilog(
              "module probe(output reg q, output reg r);\n"
              "always @* begin {q,r}=2'b00; q=1'b1; end\nendmodule\n"),
              "GJB10157.5.1.6") == 0,
          "compound first LHS is not an AST_IDENTIFIER pair source");
    check(count(analyzeVerilog(
              "module probe(output reg q, output reg r, output reg s);\n"
              "always @* begin {q,r}=2'b00; q=1'b1; s=1'b0; end\nendmodule\n"),
              "GJB10157.5.1.6") == 1,
          "a later direct-identifier pair still yields one de-duplicated line warning");
    check(count(forLoop("i=5; i<=5; i=i+1"), "GJB10157.9.3.4") == 0,
          "inclusive bound permits the first iteration");
    check(count(forLoop("i=5; i<5; i=i+1"), "GJB10157.9.3.4") == 1,
          "strict equal bound proves zero iterations");
    check(count(forLoop("i=0; i>5; i=i-1"), "GJB10157.9.3.4") == 1,
          "reverse-direction false first test reports");
    check(count(analyzeVerilog("module probe #(parameter N=5)(input d, output reg q); "
              "integer i; always @* for (i=10; i<N; i=i+1) q=d; endmodule\n"),
              "GJB10157.9.3.4") == 1,
          "parser-time parameter default participates in the recovered first-test fold");
    check(count(analyzeVerilog("module probe #(parameter [3:0] A=4'd10, B=4'd5)"
              "(input d, output reg q); integer i; "
              "always @* for (i=A; i<B; i=i+1) q=d; endmodule\n"),
              "GJB10157.9.3.4") == 1,
          "sized parameter initializer and bound reproduce the packaged violation");
    check(count(analyzeVerilog("`define LIMIT 3\nmodule probe(input d, output reg q); genvar i; "
              "generate for (i=4; i<`LIMIT; i=i+1) begin assign q=d; end endgenerate endmodule\n"),
              "GJB10157.9.3.4") == 1,
          "object-like numeric macro bound reproduces the packaged generate violation");
    check(count(analyzeVerilog("module probe(input d, output reg q); localparam N=5; integer i; "
              "always @* for (i=N; N>i; i=i+1) q=d; endmodule\n"),
              "GJB10157.9.3.4") == 1,
          "reversed constant comparison is evaluated at the first test");
    check(count(forLoop("i=2+3; (i*2)<(3+7); i=i+1"), "GJB10157.9.3.4") == 1,
          "the complete arithmetic comparison is folded before the first iteration");
    check(count(forLoop("i=5; (i<5) || (2>3); i=i+1"), "GJB10157.9.3.4") == 1,
          "a completely false compound condition reports once");
    check(count(forLoop("i=5; (i<5) || (2<3); i=i+1"), "GJB10157.9.3.4") == 0,
          "a true constant arm permits the first iteration");
    check(count(forLoop("i=0; !(i==0); i=i+1"), "GJB10157.9.3.4") == 1,
          "unary logical negation is included in the frontend-style fold");
    check(count(analyzeVerilog(
              "module probe #(parameter A=2, B=A+3)(input d, output reg q); "
              "integer i; always @* for (i=B; i<(A*2); i=i+1) q=d; endmodule\n"),
              "GJB10157.9.3.4") == 1,
          "source-ordered parameter expressions participate in first-test folding");
    check(count(forLoop("i=5; (i<5) && d; i=i+1"), "GJB10157.9.3.4") == 0,
          "a runtime-dependent compound condition remains unproved rather than misreported");
    check(count(forLoop("i=5; (i<5) && (1/0); i=i+1"), "GJB10157.9.3.4") == 0,
          "undefined constant arithmetic remains unproved rather than misreported");
    check(count(analyzeVerilog("module probe(input d, output reg q); integer i;\n"
              "`ifdef ENABLE_EMPTY_LOOP\n"
              "always @* for (i=10; i<5; i=i+1) q=d;\n"
              "`endif\nendmodule\n"),
              "GJB10157.9.3.4") == 0,
          "inactive conditional-compilation loop must not report");
    analysis::AnalysisOptions enabledLoopOptions;
    enabledLoopOptions.standardId = "GJB10157-2021";
    enabledLoopOptions.defines.push_back("ENABLE_EMPTY_LOOP");
    const auto enabledLoop = analysis::Analyzer(enabledLoopOptions).analyzeSource(
        {"probe.v", core::HdlLanguage::Verilog,
         "module probe(input d, output reg q); integer i;\n"
         "`ifdef ENABLE_EMPTY_LOOP\n"
         "always @* for (i=10; i<5; i=i+1) q=d;\n"
         "`endif\nendmodule\n"});
    check(count(enabledLoop, "GJB10157.9.3.4") == 1,
          "configured active conditional-compilation loop reports");
    const auto inactiveCombIf = analyzeVerilog(
        "module probe(input d, output reg q);\n"
        "`ifdef ENABLE_COMB_IF\n"
        "always @* if (d) q=d;\n"
        "`endif\nendmodule\n");
    check(count(inactiveCombIf, "GJB10157.9.2.11") == 0,
          "inactive conditional-compilation if must not report");
    const auto forArithmetic = [&](std::string body) {
        return analyzeVerilog("module probe(input [3:0] d, output reg [3:0] q); "
            "parameter P=4; integer i; wire [3:0] w; "
            "always @* for(i=0; i<P; i=i+1) begin " + body + " end endmodule\n");
    };
    check(count(forArithmetic("q=d+1; q=w*2; q=d%3; q=d**2;"),
              "GJB10157.9.3.2") == 4,
          "documented runtime arithmetic forms inside for report");
    check(count(forArithmetic("q=d; q=i+1; q=P+1;"),
              "GJB10157.9.3.2") == 0,
          "plain assignment, counter arithmetic and parameter arithmetic stay clear");
    check(count(analyzeVerilog(
              "module probe(input [3:0] d, output reg [3:0] q); integer i; "
              "wire [3:0] t=d+1; always @* for(i=0;i<4;i=i+1) q=t; endmodule\n"),
              "GJB10157.9.3.2") == 0,
          "arithmetic moved outside loop body does not report");
    check(count(forArithmetic("if (i>d-1) q=i+1; else q=0;"),
              "GJB10157.9.3.2") == 1,
          "documented runtime arithmetic in loop condition reports but counter increment does not");
    check(count(forArithmetic("q=(d+1)*w;"),
              "GJB10157.9.3.2") == 2,
          "nested arithmetic reports each distinct runtime identifier occurrence once");
    check(count(forArithmetic("q=d+1; q=d*2;"),
              "GJB10157.9.3.2") == 2,
          "two source occurrences of the same runtime identifier report separately");
    check(count(forArithmetic("q=d+w;"),
              "GJB10157.9.3.2") == 2,
          "both runtime identifier children of one arithmetic node report");
    check(count(analyzeVerilog(
              "module probe(input [3:0] d, input [1:0] k, output reg q); "
              "parameter P=4; integer i; always @* for(i=0;i<P;i=i+1) "
              "q=d[k]+1; endmodule\n"),
              "GJB10157.9.3.2") == 2,
          "runtime identifiers nested in an indexed arithmetic operand report");
    check(count(analyzeVerilog(
              "module probe(input [3:0] d, output reg q); parameter P=4; integer i,j; "
              "always @* for(i=0;i<P;i=i+1) begin "
              "for(j=0;j<d+1;j=j+1) q=d[j]; q=d+1; end endmodule\n"),
              "GJB10157.9.3.2") == 1,
          "a nested for header is not part of the enclosing loop body arithmetic predicate");
    check(count(analyzeVerilog(
              "module probe(input [3:0] d, output reg q); parameter P=4; "
              "always @* for(integer i=0;i<P;i=i+1) q=d+1; endmodule\n"),
              "GJB10157.9.3.2") == 1,
          "inline procedural loop-counter declarations retain the recovered body predicate");
    check(count(analyzeVerilog(
              "module probe(input d, output reg q); integer i;\n"
              "`ifdef ENABLE_LOOP\n"
              "always @* for(i=0;i<4;i=i+1) q=d+1;\n"
              "`endif\nendmodule\n"),
              "GJB10157.9.3.2") == 0,
          "inactive conditional-compilation loop arithmetic stays clear");
    const auto forHeader = [&](std::string header) {
        return analyzeVerilog("module probe(input [3:0] d, output reg q); "
            "parameter P=4; integer i,j; always @* for(" + header +
            ") q=d[0]; endmodule\n");
    };
    check(count(forHeader("i=d; i<P; i=i+1"), "GJB10157.9.3.1") == 1,
          "runtime for initializer reports");
    check(count(forHeader("i=i+1; i<P; i=i+1"), "GJB10157.9.3.1") == 1,
          "self-dependent for initializer is not a constant");
    check(count(forHeader("i=0; i<d; i=i+1"), "GJB10157.9.3.1") == 1,
          "runtime for condition reports");
    check(count(forHeader("i=0; i<P; j=i+1"), "GJB10157.9.3.1") == 1,
          "different step target reports");
    check(count(forHeader("i=d; i<d; j=i+1"), "GJB10157.9.3.1") == 3,
          "three distinct documented header faults report");
    check(count(forHeader("i=0; i<P; i=i+1"), "GJB10157.9.3.1") == 0,
          "parameter-bound well-formed for header stays clear");
    check(count(analyzeVerilog(
              "module probe(input [3:0] d, output reg q); parameter P=4; "
              "localparam L=P+1; integer i; always @* "
              "for(i=(P-4); i<(L+1); i=i+1) q=d[0]; endmodule\n"),
              "GJB10157.9.3.1") == 0,
          "frontend-foldable literal, parameter and localparam expressions stay clear");
    check(count(analyzeVerilog(
              "module probe(input [3:0] d, output reg q); integer i; always @* "
              "for(i=(d+1); i<($bits(d)+4); i=i+1) q=d[0]; endmodule\n"),
              "GJB10157.9.3.1") == 1,
          "nested runtime initializer reports while constant system-width bound stays clear");
    check(count(analyzeVerilog(
              "module probe(input [3:0] d, output reg q); integer i,j; always @* "
              "for(i=0; i<4; j[0]=i+1) q=d[0]; endmodule\n"),
              "GJB10157.9.3.1") == 1,
          "step target identity follows a selected declared object rather than raw lhs shape");
    check(count(analyzeVerilog(
              "module probe(input [3:0] d, output reg q); always @* "
              "for(integer i=0; i<4; i=i+1) q=d[0]; endmodule\n"),
              "GJB10157.9.3.1") == 0,
          "inline procedural loop-counter declaration follows the same bound-object check");
    check(count(analyzeVerilog(
              "module probe; generate for(genvar g=0; g<4; g=g+1) begin : G "
              "wire w; end endgenerate endmodule\n"),
              "GJB10157.9.3.1") == 0,
          "inline genvar loop-counter declaration follows the recovered generate branch");
    check(count(forHeader("i=UNKNOWN; i<UNKNOWN; i=i+1"), "GJB10157.9.3.1") == 0,
          "unresolved header names stay unknown rather than guessed runtime");
    const auto resetFor = [&](std::string body) {
        return analyzeVerilog("module probe(input clk, input rst_n, input [3:0] d, "
            "output reg [3:0] q); integer i; "
            "always @(posedge clk or negedge rst_n) " + body + " endmodule\n");
    };
    check(count(resetFor(
              "for(i=0;i<4;i=i+1) begin if(!rst_n) q[i]<=0; else q[i]<=d[i]; end"),
              "GJB10157.9.3.3") == 1,
          "documented reset and normal assignment in one for reports");
    check(count(resetFor(
              "if(!rst_n) for(i=0;i<4;i=i+1) q[i]<=0; "
              "else for(i=0;i<4;i=i+1) q[i]<=d[i];"),
              "GJB10157.9.3.3") == 0,
          "separate reset and normal for loops stay clear");
    check(count(analyzeVerilog(
              "module probe(input clk, input rst_n, input [3:0] d, output reg [3:0] q); "
              "integer i; always @* for(i=0;i<4;i=i+1) begin "
              "if(!rst_n) q[i]=0; else q[i]=d[i]; end endmodule\n"),
              "GJB10157.9.3.3") == 0,
          "combinational loop is not an asynchronous-reset process");
    check(count(resetFor(
              "for(i=0;i<4;i=i+1) begin if(!rst_n) q[i]<=0; else q[0]<=d[i]; end"),
              "GJB10157.9.3.3") == 1,
          "native predicate does not compare reset and logic assignment targets");
    check(count(analyzeVerilog(
              "module probe(input clk, input rst, input d, output reg q, output reg r); "
              "integer i; always @(posedge clk or posedge rst) "
              "for(i=0;i<4;i=i+1) begin if(rst) q<=0; else r<=d; end endmodule\n"),
              "GJB10157.9.3.3") == 1,
          "active-high event identifier inside a complete for-body if reports");
    check(count(analyzeVerilog(
              "module probe(input clk, input d, output reg q); integer i; "
              "always @(posedge clk) for(i=0;i<4;i=i+1) "
              "if(clk) q<=0; else q<=d; endmodule\n"),
              "GJB10157.9.3.3") == 1,
          "native event-name set does not distinguish clock names from reset names");
    check(count(resetFor(
              "for(i=0;i<4;i=i+1) begin if(!rst_n) q[i]<=0; end"),
              "GJB10157.9.3.3") == 0,
          "if without the helper-required default branch stays clear");
    check(count(resetFor(
              "for(i=0;i<4;i=i+1) begin if(!rst_n && d[i]) q[i]<=0; else q[i]<=d[i]; end"),
              "GJB10157.9.3.3") == 0,
          "compound condition is not a direct identifier or one logic-not wrapper");
    check(count(resetFor(
              "begin q=0; for(i=0;i<4;i=i+1) if(!rst_n) q[i]<=0; else q[i]<=d[i]; "
              "for(i=0;i<4;i=i+1) if(rst_n) q[i]<=0; else q[i]<=d[i]; end"),
              "GJB10157.9.3.3") == 2,
          "all direct for statements and both direct identifier polarities are checked");
    check(count(resetFor(
              "for(i=0;i<4;i=i+1) begin begin if(!rst_n) q[i]<=0; else q[i]<=d[i]; end end"),
              "GJB10157.9.3.3") == 1,
          "one nested block layer matches the native AST_BLOCK unwrap");
    check(count(resetFor(
              "for(i=0;i<4;i=i+1) begin if(!rst_n) q[i]<=0; "
              "else if(d[i]) q[i]<=1; end"),
              "GJB10157.9.3.3") == 0,
          "else-if chain without a final default branch stays clear");
    check(count(resetFor(
              "for(i=0;i<4;i=i+1) begin if(!rst_n) q[i]<=0; "
              "else if(d[i]) q[i]<=1; else q[i]<=d[i]; end"),
              "GJB10157.9.3.3") == 1,
          "else-if chain ending in a default branch passes the recursive helper gate");
    check(count(analyzeVerilog(
              "module probe(input clk, input rst, input d, output reg q); integer i; "
              "always @(posedge (clk) or negedge (rst)) for(i=0;i<4;i=i+1) "
              "if(!rst) q<=0; else q<=d; endmodule\n"),
              "GJB10157.9.3.3") == 1,
          "parenthesized single-identifier edge children retain native AST identity");
    check(count(analyzeVerilog(
              "module first(input clk, input rst, input d, output reg q); integer i; "
              "always @(posedge clk or negedge rst) for(i=0;i<4;i=i+1) "
              "if(!rst) q<=0; else q<=d; endmodule "
              "module second; endmodule\n"),
              "GJB10157.9.3.3") == 0,
          "native entry skips a source record that does not contain exactly one module root");
    check(count(analyzeVerilog(
              "module probe #(parameter W=8)(input [W-1:0] sel, output q); "
              "assign q = sel ? 1'b1 : 1'b0; endmodule\n"),
              "GJB10157.9.2.10") == 0,
          "parameterized condition width remains unknown");
    const auto caseDefaultExample = [&](std::string body) {
        return analyzeVerilog("module probe(input [1:0] sel, output reg q); "
            "always @* begin " + body + " end endmodule\n");
    };
    check(count(caseDefaultExample(
              "case (sel) 2'b00: q=0; 2'b01: q=1; endcase"),
              "GJB10157.9.2.1") == 1,
          "case without default reports once");
    check(count(caseDefaultExample(
              "case (sel) 2'b00: q=0; default: q=1; endcase"),
              "GJB10157.9.2.1") == 0,
          "case with default stays clear");
    check(count(caseDefaultExample(
              "casez (sel) 2'b0?: q=0; default: q=1; endcase"),
              "GJB10157.9.2.1") == 0,
          "casez default is recognized");
    check(count(caseDefaultExample(
              "case (sel) 2'b00:q=0; 2'b01:q=0; 2'b10:q=1; 2'b11:q=1; endcase"),
              "GJB10157.9.2.1") == 1,
          "exhaustive numeric labels still require explicit default");
    check(count(caseDefaultExample(
              "case (sel) 2'b00: begin case (sel) default:q=0; endcase end "
              "2'b01:q=1; endcase"),
              "GJB10157.9.2.1") == 1,
          "inner default does not satisfy an outer case");
    check(count(caseDefaultExample(
              "case (sel) 2'b00:q=0; default q=1; endcase"),
              "GJB10157.9.2.1") == 0,
          "default without a colon maps to the native singleton BLOCK fallback");
    check(count(caseDefaultExample("case (sel) endcase"),
              "GJB10157.9.2.1") == 1,
          "an empty case has no fallback branch");
    check(count(analyzeVerilog(
              "module probe(input a, output reg q); always @* if(a) q=1; endmodule\n"),
              "GJB10157.9.2.1") == 0,
          "if-lowered CASE nodes are excluded by the native collector flag");
    check(count(caseDefaultExample(
              "case (sel) 2'b00:q=0; endcase "
              "casex (sel) 2'b??:q=1; endcase"),
              "GJB10157.9.2.1") == 2,
          "each ordinary or wildcard case node without fallback reports once");
    analysis::AnalysisOptions inactiveDefaultOptions;
    inactiveDefaultOptions.standardId = "GJB10157-2021";
    const auto inactiveDefault = analysis::Analyzer(inactiveDefaultOptions).analyzeSource(
        {"probe.v", core::HdlLanguage::Verilog,
         "module probe(input [1:0] sel, output reg q); always @* case(sel)\n"
         "2'b00:q=0;\n`ifdef HIDDEN_DEFAULT\ndefault:q=1;\n`endif\n"
         "2'b01:q=1; endcase endmodule\n"});
    check(count(inactiveDefault, "GJB10157.9.2.1") == 1,
          "an inactive preprocessor default does not satisfy the native fallback test");
    const auto caseChoiceWidth = [&](std::string kind, std::string labels) {
        return analyzeVerilog("module probe(input [2:0] sel, output reg q); "
            "always @* begin " + kind + " (sel) " + labels +
            " default:q=0; endcase end endmodule\n");
    };
    check(count(caseChoiceWidth("case", "2'b01:q=1; 3'b101:q=0;"),
              "GJB10157.9.2.3") == 1,
          "one fixed-width case item differs from selector");
    check(count(caseChoiceWidth("casez", "4'b1???:q=1; 3'b101:q=0;"),
              "GJB10157.9.2.3") == 0,
          "native casez items are outside the admitted case-width branch");
    check(count(caseChoiceWidth("case", "3'b001:q=1; 3'b101:q=0;"),
              "GJB10157.9.2.3") == 0,
          "matching case item widths stay clear");
    check(count(caseChoiceWidth("case", "1:q=1;"),
              "GJB10157.9.2.3") == 0,
          "unsized case item width is not guessed");
    check(count(analyzeVerilog(
              "module probe(input [2:0] sel, input [1:0] item, output reg q); "
              "always @* case(sel) item:q=1; default:q=0; endcase endmodule\n"),
              "GJB10157.9.2.3") == 1,
          "native case width checks a direct named item expression");
    check(count(analyzeVerilog(
              "module probe(input [2:0] sel, input [1:0] a, input b, output reg q); "
              "always @* case(sel) {a,b}:q=1; default:q=0; endcase endmodule\n"),
              "GJB10157.9.2.3") == 0,
          "matching concatenated case item width stays clear");
    check(count(analyzeVerilog(
              "module probe(input [3:0] sel, input [1:0] a, input b, output reg q); "
              "always @* case(sel) {a,b}:q=1; default:q=0; endcase endmodule\n"),
              "GJB10157.9.2.3") == 1,
          "mismatching concatenated case item width reports");
    check(count(analyzeVerilog(
              "module probe(input [3:0] sel, input [3:0] a,b, output reg q); "
              "always @* case(sel) a*b:q=1; default:q=0; endcase endmodule\n"),
              "GJB10157.9.2.3") == 0,
          "case item multiplication uses native flag-zero maximum width");
    check(count(analyzeVerilog(
              "module probe #(parameter W=4'd3)(input [W-1:0] sel, output reg q); "
              "always @* case(sel) 3'b001:q=1; default:q=0; endcase endmodule\n"),
              "GJB10157.9.2.3") == 0,
          "fixed parameter defaults resolve a matching selector width");
    check(count(analyzeVerilog(
              "module probe(output reg q); "
              "always @* case(f()) 3'b001:q=1; default:q=0; endcase endmodule\n"),
              "GJB10157.9.2.3") == 1,
          "known case item warns when the native selector width remains minus one");
    check(count(analyzeVerilog(
              "module probe(input [2:0] sel, output reg q); "
              "always @* case(sel) f():q=1; default:q=0; endcase endmodule\n"),
              "GJB10157.9.2.3") == 0,
          "unknown case item width is skipped by the native item-side gate");
    check(count(analyzeVerilog(
              "module probe(input [2:0] sel, output reg q); "
              "always @* case(sel) 2'b00,1'b1:q=1; default:q=0; endcase endmodule\n"),
              "GJB10157.9.2.3") == 2,
          "each inferable mismatching label in one case group reports");
    check(count(analyzeVerilog(
              "module probe(input [2:0] sel, input [1:0] sub, output reg q); "
              "always @* case(sel) 3'b000: begin case(sub) 1'b0:q=1; "
              "default:q=0; endcase end default:q=0; endcase endmodule\n"),
              "GJB10157.9.2.3") == 1,
          "nested ordinary case nodes are checked independently");
    analysis::AnalysisOptions inactiveCaseWidthOptions;
    inactiveCaseWidthOptions.standardId = "GJB10157-2021";
    const auto inactiveCaseWidth = analysis::Analyzer(inactiveCaseWidthOptions).analyzeSource(
        {"probe.v", core::HdlLanguage::Verilog,
         "module probe(input [2:0] sel, output reg q); always @* case(sel)\n"
         "`ifdef BAD_CASE_WIDTH\n2'b00:q=1;\n`endif\n"
         "3'b001:q=1; default:q=0; endcase endmodule\n"});
    check(count(inactiveCaseWidth, "GJB10157.9.2.3") == 0,
          "inactive preprocessor case items do not produce width findings");
    const auto unknownCondition = [&](std::string kind, std::string value) {
        return analyzeVerilog("module probe(input a, output reg q); "
            "always @* begin " + kind + " (a == " + value +
            ") 1'b0:q=0; default:q=1; endcase end endmodule\n");
    };
    check(count(unknownCondition("case", "1'bx"), "GJB10157.9.2.4") == 1,
          "ordinary case selector containing X reports");
    check(count(unknownCondition("case", "1'bz"), "GJB10157.9.2.4") == 1,
          "ordinary case selector containing Z reports");
    check(count(unknownCondition("casez", "1'bx"), "GJB10157.9.2.4") == 0,
          "native casez branch is excluded from X/Z condition check");
    check(count(unknownCondition("casex", "1'bz"), "GJB10157.9.2.4") == 0,
          "native casex branch is excluded from X/Z condition check");
    check(count(analyzeVerilog(
              "module probe(input a, output reg q); always @* if (a == 1'bx) "
              "q=1; else q=0; endmodule\n"), "GJB10157.9.2.4") == 1,
          "if condition containing X still reports");
    check(count(caseDefaultExample(
              "case (sel) 2'b00:q=0; 2'b01:q=1; 2'b00:q=1; default:q=0; endcase"),
              "GJB10157.9.2.7") == 1,
          "exact duplicate ordinary case choice maps to overlap clause");
    check(count(caseDefaultExample(
              "case (sel) 2'b00:q=0; 2'b01:q=1; default:q=0; endcase"),
              "GJB10157.9.2.7") == 0,
          "distinct ordinary case choices stay clear");
    check(count(caseDefaultExample(
              "casex (sel) 2'b00:q=0; 2'b0?:q=1; default:q=0; endcase"),
              "GJB10157.9.2.7") == 1,
          "wildcard overlap remains reported");
    check(count(caseDefaultExample(
              "casex (sel) 2'b00:q=0; 2'b1?:q=1; default:q=0; endcase"),
              "GJB10157.9.2.7") == 0,
          "nonoverlapping wildcard choices stay clear");
    check(count(caseDefaultExample(
              "case (sel) 2'b00: begin case (sel) 2'b00:q=0; default:q=1; endcase end "
              "2'b01:q=1; default:q=0; endcase"),
              "GJB10157.9.2.7") == 0,
          "same literal in separate nested cases is not overlap");
    check(count(caseDefaultExample(
              "case (sel) 2'b01:q=0; 3'b001:q=1; default:q=0; endcase"),
              "GJB10157.9.2.7") == 1,
          "native overlap comparison zero-extends a shorter constant");
    check(count(caseDefaultExample(
              "case (sel) 2'b01:q=0; 3'b101:q=1; default:q=0; endcase"),
              "GJB10157.9.2.7") == 0,
          "an extra one in the longer constant prevents overlap");
    check(count(caseDefaultExample(
              "casez (sel) 2'b0z:q=0; 3'b001:q=1; default:q=0; endcase"),
              "GJB10157.9.2.7") == 1,
          "casez converts Z state to the native wildcard state");
    check(count(caseDefaultExample(
              "casez (sel) 2'b0x:q=0; 2'b00:q=1; default:q=0; endcase"),
              "GJB10157.9.2.7") == 0,
          "casez preserves X as an exact state rather than a wildcard");
    check(count(caseDefaultExample(
              "case (sel) READY:q=0; READY:q=1; default:q=0; endcase"),
              "GJB10157.9.2.7") == 1,
          "equal nonconstant label names follow the native name branch");
    check(count(caseDefaultExample(
              "case (sel) 2'b00:q=0; 2'b00:q=1; 2'b00:q=0; default:q=1; endcase"),
              "GJB10157.9.2.7") == 3,
          "every overlapping label pair emits at the later label");
    analysis::AnalysisOptions inactiveOverlapOptions;
    inactiveOverlapOptions.standardId = "GJB10157-2021";
    const auto inactiveOverlap = analysis::Analyzer(inactiveOverlapOptions).analyzeSource(
        {"probe.v", core::HdlLanguage::Verilog,
         "module probe(input [1:0] sel, output reg q); always @* case(sel)\n"
         "2'b00:q=0;\n`ifdef HIDDEN_DUP\n2'b00:q=1;\n`endif\n"
         "2'b01:q=1; default:q=0; endcase endmodule\n"});
    check(count(inactiveOverlap, "GJB10157.9.2.7") == 0,
          "inactive preprocessor choices do not enter the native overlap set");
    const auto wrap = [](const std::string& body) {
        return "// Filename: probe.sv\n// Author: tester\n// Description: regression\n"
               "module probe(input clk, input [7:0] aa, output logic [7:0] qq);\n" +
            body + "\nendmodule\n";
    };

    const auto vhdlUnreachableRange = [&](std::string packageUse,
                                          std::string vectorType,
                                          std::string guard) {
        return analyze("library ieee; use ieee.std_logic_1164.all; " +
            packageUse + "\nentity probe is port(clk: in std_logic; q: out std_logic); end;\n" +
            "architecture rtl of probe is signal count: " + vectorType +
            "(3 downto 0); begin process(clk) begin if rising_edge(clk) then " +
            "if " + guard + " then q <= '1'; else q <= '0'; end if; " +
            "end if; end process; end architecture;\n", true);
    };
    check(count(vhdlUnreachableRange("use ieee.std_logic_unsigned.all;",
              "std_logic_vector", "count = 16"), "GJB9765.4.2.e") == 1,
          "4-bit unsigned comparison with 16 is an unreachable branch");
    check(count(vhdlUnreachableRange("use ieee.std_logic_unsigned.all;",
              "std_logic_vector", "count = 10"), "GJB9765.4.2.e") == 0,
          "4-bit unsigned comparison with representable 10 stays unknown");
    check(count(vhdlUnreachableRange("use ieee.std_logic_unsigned.all;",
              "std_logic_vector", "count = 15"), "GJB9765.4.2.e") == 0,
          "largest 4-bit unsigned value remains reachable");
    check(count(vhdlUnreachableRange("use ieee.numeric_std.all;",
              "unsigned", "16 > count"), "GJB9765.4.2.e") == 0,
          "native collector does not normalize a constant left operand");
    for (const auto& guard : {"count >= 16", "count > 15", "count > 16"})
        check(count(vhdlUnreachableRange("use ieee.numeric_std.all;",
                  "unsigned", guard), "GJB9765.4.2.e") == 1,
              "native upper-bound comparison reports an unreachable true arm");
    for (const auto& guard : {"count >= 15", "count > 14", "count < 16", "count <= 16",
                             "count /= 16", "true", "1 = 2", "count = 16 + 1",
                             "count = 4294967296"})
        check(count(vhdlUnreachableRange("use ieee.numeric_std.all;",
                  "unsigned", guard), "GJB9765.4.2.e") == 0,
              "non-native shape or reachable upper-bound comparison stays unreported");
    check(count(vhdlUnreachableRange("use ieee.numeric_std.all;",
              "signed", "count = 16"), "GJB9765.4.2.e") == 0,
          "unsupported signed comparison remains unknown");
    check(count(vhdlUnreachableRange("", "std_logic_vector", "count = 16"),
              "GJB9765.4.2.e") == 0,
          "std_logic_vector without numeric overload remains unknown");
    check(count(vhdlUnreachableRange("use ieee.std_logic_unsigned.all;",
              "std_logic_vector", "count + 1 = 16"), "GJB9765.4.2.e") == 0,
          "arithmetic expression is not mistaken for a direct range comparison");
    check(count(analyze("library ieee; use ieee.numeric_std.all; "
              "entity probe is end; architecture rtl of probe is "
              "signal count: unsigned(3 downto 0); signal flag: boolean; begin "
              "flag <= count > 15; end architecture;", true), "GJB9765.4.2.e") == 1,
          "native recursive collector also sees comparison values outside control guards");
    check(count(analyze("library ieee; use ieee.numeric_std.all; "
              "entity probe is end; architecture rtl of probe is "
              "signal count: unsigned(3 downto 0); begin "
              "process variable count: unsigned(7 downto 0); begin "
              "if count = 16 then null; end if; wait; end process; end architecture;", true),
              "GJB9765.4.2.e") == 0,
          "local shadowing is deferred rather than borrowing architecture width");

    auto clockNameResult = analyze("library ieee; use ieee.std_logic_1164.all;\n"
        "entity probe is port(CLK: in std_logic; A1: in std_logic; C1_clk: out std_logic); end;\n"
        "architecture rtl of probe is begin\n"
        "process(CLK) begin if rising_edge(CLK) then C1_clk <= A1; end if; end process;\n"
        "end architecture;\n", true);
    check(count(clockNameResult, "GJB9765.4.5.e") >= 1,
          "published VHDL CLK/C1_clk naming counterexample reports");
    clockNameResult = analyze("library ieee; use ieee.std_logic_1164.all;\n"
        "entity probe is port(clk_50: in std_logic; A1: in std_logic; C1: out std_logic); end;\n"
        "architecture rtl of probe is begin\n"
        "process(clk_50) begin if rising_edge(clk_50) then C1 <= A1; end if; end process;\n"
        "end architecture;\n", true);
    check(count(clockNameResult, "GJB9765.4.5.e") == 0,
          "published VHDL clk_50 naming example stays clear");
    const auto vhdlClockName = [&](std::string clock, std::string output) {
        return analyze("library ieee; use ieee.std_logic_1164.all;\n"
            "entity probe is port(" + clock + ": in std_logic; data_in: in std_logic; " +
            output + ": out std_logic); end;\n"
            "architecture rtl of probe is begin\n"
            "process(" + clock + ") begin if rising_edge(" + clock +
            ") then " + output + " <= data_in; end if; end process;\n"
            "end architecture;\n", true);
    };
    check(count(vhdlClockName("clk", "data_out"), "GJB9765.4.5.e") == 1,
          "VHDL clock prefix requires underscore");
    check(count(vhdlClockName("CLK_50", "data_out"), "GJB9765.4.5.e") == 0,
          "VHDL native prefix comparison is case-insensitive");
    check(count(vhdlClockName("clk_50", "data_clk"), "GJB9765.4.5.e") == 1,
          "clock-marked data output is not accepted as a clock-named port");

    auto identifierFoundation = analyze(
        "library ieee; use ieee.std_logic_1164.all;\n"
        "entity duplicate_name is port(duplicate_name: in std_logic; q: out std_logic); end;\n"
        "architecture duplicate_arc of duplicate_name is\n"
        "constant duplicate_arc: integer := 1; signal data_ok: std_logic;\n"
        "begin q <= duplicate_name and data_ok; end architecture;\n", true);
    check(count(identifierFoundation, "GJB9765.4.5.b") == 2,
          "native 4.5.b category intersections catch port/entity and constant/architecture names");
    identifierFoundation = analyze(
        "entity distinct_name is port(input_a: in bit; q: out bit); end;\n"
        "architecture distinct_arc of distinct_name is constant c_value: integer := 1; "
        "signal data_ok: bit; begin q <= input_a and data_ok; end architecture;\n", true);
    check(count(identifierFoundation, "GJB9765.4.5.b") == 0,
          "distinct names across native 4.5.b categories stay clear");

    identifierFoundation = analyze(
        "entity keyword_ent is generic(vmode: integer := 1); "
        "port(vprop: in bit; q: out bit); end;\n"
        "architecture keyword_arc of keyword_ent is type literal is (idle, run); "
        "subtype vunit is integer; constant access: integer := 1; "
        "signal LABEL: bit; begin process variable alias: integer; begin "
        "q <= vprop; end process; end architecture;\n", true);
    check(count(identifierFoundation, "GJB9765.4.5.a") == 7,
          "native 4.5.a VHDL keyword dictionary covers all seven recovered object categories case-insensitively");
    identifierFoundation = analyze(
        "entity keyword_ent is generic(wire: integer := 1); "
        "port(always_ff: in bit; q: out bit); end;\n"
        "architecture keyword_arc of keyword_ent is type delayfile is (idle, run); "
        "subtype abort is integer; constant con: integer := 1; "
        "signal data_ok: bit; begin q <= always_ff; end architecture;\n", true);
    check(count(identifierFoundation, "GJB9765.4.5.a") == 0,
          "optional non-VHDL native dictionaries are not silently enabled for the IEEE 1076 rule");

    identifierFoundation = analyze(
        "entity valid_ent is port(port_ok: in bit; q: out bit); end;\n"
        "architecture valid_arc of valid_ent is type type_ok is (idle, run); "
        "constant c_ok: integer := 1; signal my$sig_d: bit; signal 1_sig_d: bit; "
        "signal mysig_: bit; signal my__sig: bit; begin q <= port_ok; end architecture;\n", true);
    check(count(identifierFoundation, "GJB9765.4.5.c") == 4,
          "native 4.5.c independently catches illegal byte, non-letter start, trailing and repeated underscore");
    identifierFoundation = analyze(
        "entity valid_ent is generic(g_width: integer := 1); "
        "port(port_ok: in bit; q: out bit); end;\n"
        "architecture valid_arc of valid_ent is type type_ok is (idle, run); "
        "constant c_ok: integer := 1; signal my_sig: bit; begin q <= port_ok; end architecture;\n", true);
    check(count(identifierFoundation, "GJB9765.4.5.c") == 0,
          "valid native short-identifier categories stay clear for 4.5.c");

    identifierFoundation = analyze(
        "entity case_ent is port(input_a: in bit; q: out bit); end;\n"
        "architecture case_arc of case_ent is constant c_my_const: integer := 1; "
        "signal C_my_const: bit; begin q <= input_a; end architecture;\n", true);
    check(count(identifierFoundation, "GJB9765.4.5.d") == 1,
          "native 4.5.d folded declaration comparison catches case-only duplicate names");
    identifierFoundation = analyze(
        "entity case_ent is port(input_a: in bit; q: out bit); end;\n"
        "architecture case_arc of case_ent is signal DataValue: bit; "
        "begin datavalue <= input_a; q <= DataValue; end architecture;\n", true);
    check(count(identifierFoundation, "GJB9765.4.5.d") == 1,
          "native 4.5.d declaration/use comparison catches inconsistent identifier spelling");
    identifierFoundation = analyze(
        "entity case_ent is port(input_a: in bit; q: out bit); end;\n"
        "architecture case_arc of case_ent is signal DataValue: bit; "
        "begin DataValue <= input_a; q <= DataValue; end architecture;\n", true);
    check(count(identifierFoundation, "GJB9765.4.5.d") == 0,
          "consistent exact spelling stays clear for 4.5.d");

    auto semanticNaming = analyze(
        "library ieee; use ieee.std_logic_1164.all;\n"
        "entity probe is port(MY_CLK, ARST, ARST_B, ARST_N: in std_logic; "
        "A1: in std_logic; C1, C2: out std_logic); end;\n"
        "architecture probe_arc of probe is signal int_rst: std_logic; "
        "signal signal1, signal2: std_logic; begin\n"
        "p1: process(MY_CLK, ARST) begin if ARST='0' then C1<='1'; "
        "elsif rising_edge(MY_CLK) then C1<=A1; end if; end process;\n"
        "int_rst <= ARST_N;\n"
        "p2: process(MY_CLK, int_rst) begin if int_rst='0' then C2<='1'; "
        "elsif rising_edge(MY_CLK) then C2<=A1; end if; end process;\n"
        "p3: process(MY_CLK, ARST_B) begin if ARST_B='0' then C2<='1'; "
        "elsif rising_edge(MY_CLK) then C2<=A1; end if; end process;\n"
        "signal1 <= A1; signal2 <= not signal1; C2 <= signal2;\n"
        "end architecture;\n", true);
    check(count(semanticNaming, "GJB9765.4.5.f.26") == 1,
          "native f.26 checks the actual clock port with case-insensitive clk_ prefix mode");
    check(count(semanticNaming, "GJB9765.4.5.f.28") == 3,
          "published low-active ARST, ARST_B and propagated internal reset names report");

    semanticNaming = analyze(
        "entity probe is port(INPUT1: in bit; C1: out bit); end;\n"
        "architecture probe_arc of probe is signal signal1, signal2: bit; begin\n"
        "signal1 <= INPUT1; signal2 <= not signal1; C1 <= signal2;\n"
        "end architecture;\n", true);
    check(count(semanticNaming, "GJB9765.4.5.f.29") == 3,
          "published three simple buffer/inverter assignments with changed bases report");

    semanticNaming = analyze(
        "library ieee; use ieee.std_logic_1164.all;\n"
        "entity probe is port(CLK_50, ARST_n: in std_logic; INPUT1: in std_logic; "
        "INPUT1_OUT: out std_logic); end;\n"
        "architecture probe_arc of probe is signal input1_d1, input1_inv: std_logic; begin\n"
        "p: process(CLK_50, ARST_n) begin if ARST_n='0' then INPUT1_OUT<='0'; "
        "elsif rising_edge(CLK_50) then INPUT1_OUT<=input1_inv; end if; end process;\n"
        "input1_d1 <= INPUT1; input1_inv <= not input1_d1; INPUT1_OUT <= input1_inv;\n"
        "end architecture;\n", true);
    check(count(semanticNaming, "GJB9765.4.5.f.26") == 0 &&
          count(semanticNaming, "GJB9765.4.5.f.28") == 0 &&
          count(semanticNaming, "GJB9765.4.5.f.29") == 0,
          "published clk_, _n and unchanged-first-segment naming examples stay clear");

    semanticNaming = analyze(
        "entity probe is port(clk_50, enable, rst_p: in bit; q: out bit); end;\n"
        "architecture probe_arc of probe is signal data_a, data_b: bit; begin\n"
        "p: process(clk_50) begin if rising_edge(clk_50) then "
        "if enable='0' then q<=data_b; end if; end if; end process;\n"
        "data_a <= data_b and enable;\n"
        "end architecture;\n", true);
    check(count(semanticNaming, "GJB9765.4.5.f.28") == 0,
          "ordinary data compared with zero is not guessed to be a low-active control");
    check(count(semanticNaming, "GJB9765.4.5.f.29") == 0,
          "compound and documented direct-buffer sequential assignments stay outside f.29");

    semanticNaming = analyze(
        "entity probe is port(clk_fall: in bit; q: out bit); end;\n"
        "architecture probe_arc of probe is begin\n"
        "p: process(clk_fall) begin if falling_edge(clk_fall) then q<='0'; "
        "end if; end process; end architecture;\n", true);
    check(count(semanticNaming, "GJB9765.4.5.f.28") == 0,
          "a bare falling-edge IF is neither category one nor a flag-one category-two relation");

    semanticNaming = analyze(
        "entity probe is port(clk_data, d: in bit; q: out bit); end;\n"
        "architecture probe_arc of probe is begin\n"
        "p: process(clk_data) begin if falling_edge(clk_data) then q<=d; "
        "end if; end process; end architecture;\n", true);
    check(count(semanticNaming, "GJB9765.4.5.f.28") == 0,
          "falling_edge data transfers remain outside the recovered category-one admission boundary");

    semanticNaming = analyze(
        "entity probe is port(clk_branch, d: in bit; q, spare: out bit); end;\n"
        "architecture probe_arc of probe is begin\n"
        "p: process(clk_branch) begin spare<='0'; "
        "if falling_edge(clk_branch) then q<=d; end if; "
        "end process; end architecture;\n", true);
    check(count(semanticNaming, "GJB9765.4.5.f.28") == 0,
          "a literal assignment outside the falling-edge branch cannot admit the native condition relation");

    semanticNaming = analyze(
        "entity probe is port(clk_nested, enable: in bit; q: out bit); end;\n"
        "architecture probe_arc of probe is begin\n"
        "p: process(clk_nested) begin if falling_edge(clk_nested) then "
        "if enable='1' then q<='0'; end if; end if; "
        "end process; end architecture;\n", true);
    check(count(semanticNaming, "GJB9765.4.5.f.28") == 1,
          "a linked nested IF with a literal assignment remains admitted inside the falling-edge branch");

    semanticNaming = analyze(
        "entity probe is port(clk_late, enable: in bit; q, spare: out bit); end;\n"
        "architecture probe_arc of probe is begin\n"
        "p: process(clk_late) begin if falling_edge(clk_late) then "
        "spare<=q; if enable='1' then q<='0'; end if; end if; "
        "end process; end architecture;\n", true);
    check(count(semanticNaming, "GJB9765.4.5.f.28") == 0,
          "a nested IF that is not the first branch node stays outside native category two");

    semanticNaming = analyze(
        "entity probe is port(clk_elsif, enable, hold, d: in bit; q: out bit); end;\n"
        "architecture probe_arc of probe is begin\n"
        "p: process(clk_elsif) begin if falling_edge(clk_elsif) then "
        "if enable='1' then q<=d; elsif hold='1' then q<='0'; end if; end if; "
        "end process; end architecture;\n", true);
    check(count(semanticNaming, "GJB9765.4.5.f.28") == 0,
          "category two reads direct AST_COND records and does not borrow an ELSIF-body literal");

    semanticNaming = analyze(
        "entity probe is generic(INIT_LEVEL: bit := '0'); "
        "port(clk_generic, enable: in bit; q: out bit); end;\n"
        "architecture probe_arc of probe is begin\n"
        "p: process(clk_generic) begin if falling_edge(clk_generic) then "
        "if enable='1' then q<=INIT_LEVEL; end if; end if; "
        "end process; end architecture;\n", true);
    check(count(semanticNaming, "GJB9765.4.5.f.28") == 1,
          "a resolved entity generic RHS is admitted by the native named-value set");

    semanticNaming = analyze(
        "entity probe is port(clk_constant, enable: in bit; q: out bit); end;\n"
        "architecture probe_arc of probe is constant C_INIT: bit := '0'; begin\n"
        "p: process(clk_constant) begin if falling_edge(clk_constant) then "
        "if enable='1' then q<=C_INIT; end if; end if; "
        "end process; end architecture;\n", true);
    check(count(semanticNaming, "GJB9765.4.5.f.28") == 1,
          "an architecture constant RHS is admitted by the native named-value set");

    semanticNaming = analyze(
        "entity probe is port(clk_enum, enable: in bit; q: out bit); end;\n"
        "architecture probe_arc of probe is type state_t is (IDLE, RUN); "
        "signal state_q: state_t; begin\n"
        "p: process(clk_enum) begin if falling_edge(clk_enum) then "
        "if enable='1' then state_q<=IDLE; end if; end if; q<=enable; "
        "end process; end architecture;\n", true);
    check(count(semanticNaming, "GJB9765.4.5.f.28") == 1,
          "an architecture enum literal RHS is admitted by the native named-value set");

    semanticNaming = analyze(
        "entity probe is port(clk_signal, enable, data_value: in bit; q: out bit); end;\n"
        "architecture probe_arc of probe is begin\n"
        "p: process(clk_signal) begin if falling_edge(clk_signal) then "
        "if enable='1' then q<=data_value; end if; end if; "
        "end process; end architecture;\n", true);
    check(count(semanticNaming, "GJB9765.4.5.f.28") == 0,
          "an ordinary signal RHS is not guessed to be a constant named value");

    semanticNaming = analyze(
        "entity probe is port(clk_local, enable: in bit; q: out bit); end;\n"
        "architecture probe_arc of probe is begin\n"
        "p: process(clk_local) constant C_LOCAL: bit := '0'; begin "
        "if falling_edge(clk_local) then if enable='1' then q<=C_LOCAL; "
        "end if; end if; end process; end architecture;\n", true);
    check(count(semanticNaming, "GJB9765.4.5.f.28") == 0,
          "a process-local constant is outside the native design-root named-value collector");

    semanticNaming = analyze(
        "entity first is port(a: in bit; y: out bit); end;\n"
        "architecture first_arc of first is constant C_FIRST: bit := '0'; "
        "begin y<=C_FIRST; end architecture;\n"
        "entity probe is port(clk_scope, enable: in bit; q: out bit); end;\n"
        "architecture probe_arc of probe is begin\n"
        "p: process(clk_scope) begin if falling_edge(clk_scope) then "
        "if enable='1' then q<=C_FIRST; end if; end if; "
        "end process; end architecture;\n", true);
    check(count(semanticNaming, "GJB9765.4.5.f.28") == 0,
          "named constants do not leak across separate architecture design roots");

    semanticNaming = analyze(
        "entity probe is port(clk_50, data_b: in bit; q: out bit); end;\n"
        "architecture probe_arc of probe is signal other_a: bit; begin\n"
        "p: process(clk_50) begin if rising_edge(clk_50) then "
        "other_a <= not data_b; end if; end process; q <= other_a;\n"
        "end architecture;\n", true);
    check(count(semanticNaming, "GJB9765.4.5.f.29") == 2,
          "native f.29 recursive assignment collector includes sequential unary NOT while the documented concurrent buffer extension remains active");

    auto localInput = analyze("library ieee; use ieee.std_logic_1164.all;\n"
        "entity top is port(a,b: in std_logic; y: out std_logic); end;\n"
        "architecture rtl of top is\n"
        "component child port(a,b: in std_logic; y: out std_logic); end component;\n"
        "begin u: child port map(a => a, y => y); end architecture;\n", true);
    check(count(localInput, "GJB9765.4.1.b") == 1,
          "local component omitted required input is reported");
    localInput = analyze("library ieee; use ieee.std_logic_1164.all;\n"
        "entity top is port(a,b: in std_logic; y: out std_logic); end;\n"
        "architecture rtl of top is\n"
        "component child port(a,b: in std_logic; y: out std_logic); end component;\n"
        "begin u: child port map(a => a, b => b, y => y); end architecture;\n", true);
    check(count(localInput, "GJB9765.4.1.b") == 0,
          "local component connected input is not reported");
    localInput = analyze("library ieee; use ieee.std_logic_1164.all;\n"
        "entity top is port(a: in std_logic; y: out std_logic); end;\n"
        "architecture rtl of top is\n"
        "component child port(a,b: in std_logic; y: out std_logic); end component;\n"
        "begin u: child port map(a => a, b => open, y => y); end architecture;\n", true);
    check(count(localInput, "GJB9765.4.1.b") == 1,
          "explicit OPEN on required component input is reported");
    localInput = analyze("library ieee; use ieee.std_logic_1164.all;\n"
        "entity top is port(a: in std_logic; y: out std_logic); end;\n"
        "architecture rtl of top is\n"
        "component child port(a: in std_logic; b: in std_logic := '0'; y: out std_logic); end component;\n"
        "begin u: child port map(a => a, y => y); end architecture;\n", true);
    check(count(localInput, "GJB9765.4.1.b") == 0,
          "defaulted component input is not floating when omitted");
    localInput = analyze("library ieee; use ieee.std_logic_1164.all;\n"
        "entity top is port(a: in std_logic; y: out std_logic); end;\n"
        "architecture first of top is\n"
        "component child port(a,b: in std_logic; y: out std_logic); end component;\n"
        "begin y <= a; end architecture;\n"
        "architecture second of top is begin\n"
        "u: child port map(a => a, y => y); end architecture;\n", true);
    check(count(localInput, "GJB9765.4.1.b") == 0,
          "component declaration from another architecture is not borrowed");
    localInput = analyze("library ieee; use ieee.std_logic_1164.all;\n"
        "entity top is port(a: in std_logic; y: out std_logic); end;\n"
        "architecture rtl of top is\n"
        "component child port(a: in std_logic; y: out std_logic); end component;\n"
        "begin u: child port map(a => a); end architecture;\n", true);
    check(count(localInput, "GJB9765.4.1.c") == 1,
          "omitted local component output is reported");
    localInput = analyze("library ieee; use ieee.std_logic_1164.all;\n"
        "entity top is port(a: in std_logic; y: out std_logic); end;\n"
        "architecture rtl of top is\n"
        "component child port(a: in std_logic; y: out std_logic); end component;\n"
        "begin u: child port map(a => a, y => open); end architecture;\n", true);
    check(count(localInput, "GJB9765.4.1.c") == 0,
          "explicit OPEN on unused local component output is accepted");
    analysis::AnalysisOptions knownEntityOptions;
    knownEntityOptions.standardId = "GJB9765-2020";
    const auto knownEntity = [&](std::string association) {
        return analysis::Analyzer(knownEntityOptions).analyzeSources({
            {"child.vhd", core::HdlLanguage::Vhdl,
             "entity child is port(a: in bit; y: out bit); end;\n"},
            {"top.vhd", core::HdlLanguage::Vhdl,
             "entity top is port(a: in bit; y: out bit); end;\n"
             "architecture rtl of top is begin u: child port map(" + association +
             "); end architecture;\n"}
        });
    };
    localInput = knownEntity("a => a, y => open");
    check(count(localInput, "GJB9765.4.1.c") == 0,
          "explicit OPEN is accepted for a resolved entity output too");
    localInput = knownEntity("a => a");
    check(count(localInput, "GJB9765.4.1.c") == 1,
          "omitted resolved entity output still reports");

    auto resetPort = analyze("library ieee; use ieee.std_logic_1164.all;\n"
        "entity top is port(clk,rst_n,data: in std_logic; y: out std_logic); end;\n"
        "architecture rtl of top is\n"
        "component child port(clk,rst_n,data: in std_logic; y: out std_logic); end component;\n"
        "begin u: child port map(clk => clk, rst_n => open, data => data, y => y); "
        "end architecture;\n", true);
    check(count(resetPort, "GJB9765.4.10.d") == 1 &&
          std::ranges::any_of(resetPort.violations, [](const auto& item) {
              return item.ruleId == "GJB9765.4.10.d" &&
                  item.message.find("u.rst_n") != std::string::npos;
          }), "attachment 4.10.d reports a local component reset formal mapped to OPEN");
    resetPort = analyze("library ieee; use ieee.std_logic_1164.all;\n"
        "entity top is port(clk,rst_n,data: in std_logic; y: out std_logic); end;\n"
        "architecture rtl of top is\n"
        "component child port(clk,rst_n,data: in std_logic; y: out std_logic); end component;\n"
        "begin u: child port map(clk => clk, rst_n => rst_n, data => data, y => y); "
        "end architecture;\n", true);
    check(count(resetPort, "GJB9765.4.10.d") == 0,
          "attachment 4.10.d accepts a driven local component reset formal");
    resetPort = analyze("library ieee; use ieee.std_logic_1164.all;\n"
        "entity top is port(clk,rst_n: in std_logic; y: out std_logic); end;\n"
        "architecture rtl of top is\n"
        "component child port(clk,rst_n,data: in std_logic; y: out std_logic); end component;\n"
        "begin u: child port map(clk => clk, rst_n => rst_n, data => open, y => y); "
        "end architecture;\n", true);
    check(count(resetPort, "GJB9765.4.10.d") == 0,
          "4.10.d does not relabel an ordinary open input as a reset port");
    resetPort = analyze("library ieee; use ieee.std_logic_1164.all;\n"
        "entity top is port(clk,rst_n,data: in std_logic; y: out std_logic); end;\n"
        "architecture rtl of top is\n"
        "component child port(clk,rst_n,data: in std_logic; y: out std_logic); end component;\n"
        "begin u: child port map(clk, open, data, y); end architecture;\n", true);
    check(count(resetPort, "GJB9765.4.10.d") == 1,
          "4.10.d resolves an explicit positional OPEN against the reset formal");
    resetPort = analyze("library ieee; use ieee.std_logic_1164.all;\n"
        "entity top is port(clk,data: in std_logic; y: out std_logic); end;\n"
        "architecture rtl of top is\n"
        "component child port(clk,rst_n,data: in std_logic; y: out std_logic); end component;\n"
        "begin -- rst_n => open\n"
        "u: child port map(clk => clk, data => data, y => y); end architecture;\n", true);
    check(count(resetPort, "GJB9765.4.10.d") == 0,
          "4.10.d excludes comments and omitted associations from its proven OPEN predicate");

    const auto resolvedResetPort = analysis::Analyzer(knownEntityOptions).analyzeSources({
        {"child.vhd", core::HdlLanguage::Vhdl,
         "entity child is port(clk,reset_n,data: in bit; y: out bit); end;\n"},
        {"top.vhd", core::HdlLanguage::Vhdl,
         "entity top is port(clk,data: in bit; y: out bit); end;\n"
         "architecture rtl of top is begin u: child port map("
         "clk => clk, reset_n => open, data => data, y => y); end architecture;\n"}
    });
    check(count(resolvedResetPort, "GJB9765.4.10.d") == 1,
          "4.10.d applies the same OPEN predicate to a resolved entity reset formal");

    auto resetDomain = analyze(
        "entity top is port(clk1,arst1,arst2,global_rst1: in bit); end entity;\n"
        "architecture rtl of top is\n"
        "component submod1 port(clk,arst1,arst2: in bit); end component;\n"
        "begin u: submod1 port map(clk => clk1, arst1 => arst1, "
        "arst2 => global_rst1); end architecture;\n", true);
    check(count(resetDomain, "GJB9765.4.10.f") == 1 &&
          std::ranges::any_of(resetDomain.violations, [](const auto& item) {
              return item.ruleId == "GJB9765.4.10.f" &&
                  item.message.find("u") != std::string::npos &&
                  item.message.find("arst1") != std::string::npos &&
                  item.message.find("global_rst1") != std::string::npos;
          }), "attachment 4.10.f reports two reset actuals on a one-clock component");
    resetDomain = analyze(
        "entity top is port(clk1,arst1: in bit); end entity;\n"
        "architecture rtl of top is\n"
        "component submod1 port(clk,arst1,arst2: in bit); end component;\n"
        "begin u: submod1 port map(clk => clk1, arst1 => arst1, "
        "arst2 => ARST1); end architecture;\n", true);
    check(count(resetDomain, "GJB9765.4.10.f") == 0,
          "attachment 4.10.f accepts case-insensitive equal reset actuals");
    resetDomain = analyze(
        "entity top is port(clk1,clk2,arst1,arst2: in bit); end entity;\n"
        "architecture rtl of top is\n"
        "component submod2 port(clk1,clk2,arst1,arst2: in bit); end component;\n"
        "begin u: submod2 port map(clk1 => clk1, clk2 => clk2, "
        "arst1 => arst1, arst2 => arst2); end architecture;\n", true);
    check(count(resetDomain, "GJB9765.4.10.f") == 0,
          "4.10.f preserves the native exclusion for a multi-clock target");
    resetDomain = analyze(
        "entity top is port(clk1,arst1,arst2: in bit); end entity;\n"
        "architecture rtl of top is\n"
        "component submod1 port(clk,arst1,arst2: in bit); end component;\n"
        "begin u: submod1 port map(clk => clk1, arst1 => arst1, "
        "arst2 => not arst2); end architecture;\n", true);
    check(count(resetDomain, "GJB9765.4.10.f") == 0,
          "4.10.f leaves expression reset actuals review-required instead of guessing");

    const auto resolvedResetDomain = analysis::Analyzer(knownEntityOptions).analyzeSources({
        {"child.vhd", core::HdlLanguage::Vhdl,
         "entity child is port(clock,reset_a,reset_b: in bit); end;\n"},
        {"top.vhd", core::HdlLanguage::Vhdl,
         "entity top is port(clk,rst_a,rst_b: in bit); end;\n"
         "architecture rtl of top is begin u: child port map("
         "clock => clk, reset_a => rst_a, reset_b => rst_b); end architecture;\n"}
    });
    check(count(resolvedResetDomain, "GJB9765.4.10.f") == 1,
          "4.10.f applies the recovered predicate to a resolved entity target");

    auto result = analyze(
        "entity child is port(a,b,c: in bit); end entity;\n"
        "entity top is port(x: in bit); end entity;\n"
        "architecture rtl of top is begin\n"
        "u0: child port map(x, x, x);\n"
        "u1: child port map(x, x, x);\n"
        "end architecture;\n", true);
    check(count(result, "GJB9765.4.1.a") == 2,
          "4.1.a reports each positional association container independently");
    result = analyze(
        "entity child is port(a,b,c: in bit); end entity;\n"
        "entity top is port(x: in bit); end entity;\n"
        "architecture rtl of top is begin\n"
        "u0: child port map(x, x, x);\n"
        "end architecture;\n", true);
    check(count(result, "GJB9765.4.1.a") == 1,
          "4.1.a stops after the first positional association in one instance");
    result = analyze(
        "entity child is port(a,b,c: in bit); end entity;\n"
        "entity top is port(x: in bit); end entity;\n"
        "architecture rtl of top is begin\n"
        "u0: child port map(x, b => x, c => x);\n"
        "end architecture;\n", true);
    check(count(result, "GJB9765.4.1.a") == 1,
          "4.1.a treats a mixed positional and named association list as one violation");
    result = analyze(
        "entity child is port(a,b,c: in bit); end entity;\n"
        "entity top is port(x: in bit); end entity;\n"
        "architecture rtl of top is begin\n"
        "u0: child port map(a => x, b => open, c => x);\n"
        "end architecture;\n", true);
    check(count(result, "GJB9765.4.1.a") == 0,
          "4.1.a accepts named associations including a named OPEN actual");

    result = analyze("library ieee; use ieee.std_logic_1164.all;\n"
        "entity probe is port(a,b,c,d: in std_logic; z: out std_logic); end;\n"
        "architecture rtl of probe is begin\n"
        "z <= b when a='0' else 'Z';\n"
        "z <= c when a='1' else 'Z';\n"
        "end architecture;\n", true);
    check(count(result, "GJB9765.4.6.f") == 0,
          "native VHDL rule permits explicit conditional tri-state drivers");
    result = analyze("library ieee; use ieee.std_logic_1164.all;\n"
        "entity probe is port(a,d,b,c: in std_logic; z: out std_logic); end;\n"
        "architecture rtl of probe is begin\n"
        "z <= b when a='1' else 'Z';\n"
        "z <= c when d='1' else 'Z';\n"
        "end architecture;\n", true);
    check(count(result, "GJB9765.4.6.f") == 1,
          "4.6.f does not exempt tri-state drivers resolved to different control sources");
    result = analyze("library ieee; use ieee.std_logic_1164.all;\n"
        "entity probe is port(a,b,c,d: in std_logic; z: out std_logic); end;\n"
        "architecture rtl of probe is begin\n"
        "z <= b xor c when a='1' else 'Z';\n"
        "z <= d when a='0' else 'Z';\n"
        "end architecture;\n", true);
    check(count(result, "GJB9765.4.6.f") == 1,
          "4.6.f keeps complex tri-state data expressions outside the native direct-name exemption");
    result = analyze("library ieee; use ieee.std_logic_1164.all;\n"
        "entity probe is port(a,b,c: in std_logic; z: out std_logic); end;\n"
        "architecture rtl of probe is begin\n"
        "z <= b when '1'=a else 'Z';\n"
        "z <= c when a='0' else 'Z';\n"
        "end architecture;\n", true);
    check(count(result, "GJB9765.4.6.f") == 0,
          "4.6.f resolves the shared control identifier from either relation operand");
    result = analyze("library ieee; use ieee.std_logic_1164.all;\n"
        "entity probe is port(a,b,c: in std_logic; z: out std_logic); end;\n"
        "architecture rtl of probe is begin\n"
        "z <= a;\n"
        "z <= b when c='1' else 'Z';\n"
        "end architecture;\n", true);
    check(count(result, "GJB9765.4.6.f") == 1,
          "ordinary and tri-state drivers together still report");
    result = analyze("library ieee; use ieee.std_logic_1164.all;\n"
        "entity probe is port(a,b: in std_logic; z: out std_logic); end;\n"
        "architecture rtl of probe is begin z <= a; z <= b; end architecture;\n", true);
    check(count(result, "GJB9765.4.6.f") == 1,
          "two ordinary VHDL drivers still report");
    result = analyze("library ieee; use ieee.std_logic_1164.all;\n"
        "entity probe is port(a: in std_logic; z: out std_logic); end;\n"
        "architecture rtl of probe is begin z <= a; end architecture;\n", true);
    check(count(result, "GJB9765.4.6.f") == 0,
          "single VHDL driver stays unreported");
    result = analyze("library ieee; use ieee.std_logic_1164.all;\n"
        "entity probe is port(a: in std_logic; z: out std_logic); end;\n"
        "architecture rtl of probe is signal n: std_logic;\n"
        "component child is port(p_in: in std_logic; p_out: out std_logic); end component;\n"
        "begin u: child port map(p_in => a, p_out => n);\n"
        "process(a) begin n <= a; end process; z <= n; end architecture;\n", true);
    check(count(result, "GJB9765.4.6.f") == 1,
          "declared component output plus local assignment is a second driver");

    result = analyze("entity child is port(a,b,c: in bit); end entity;\n"
        "entity top is port(x: in bit); end entity;\n"
        "architecture rtl of top is begin\n"
        "u0: child port map(\n"
        "  a => x, b => x, c => x\n"
        "); end architecture;\n", true);
    check(count(result, "GJB9765.4.14.d.3") == 2 &&
          std::ranges::all_of(result.violations | std::views::filter([](const auto& item) {
              return item.ruleId == "GJB9765.4.14.d.3";
          }), [](const auto& item) { return item.line == 5 && item.severity == analysis::Severity::Warning; }),
          "native VHDL adjacent same-line port associations report each extra mapping");
    result = analyze("entity child is port(a,b: in bit); end entity;\n"
        "entity top is port(x: in bit); end entity;\n"
        "architecture rtl of top is begin\n"
        "u0: child port map(\n"
        "  a => x, -- a comment does not merge source lines\n"
        "  b => x\n"
        "); end architecture;\n", true);
    check(count(result, "GJB9765.4.14.d.3") == 0 &&
          std::ranges::any_of(result.standardAssessments, [](const auto& item) {
              return item.clauseId == "GJB9765.4.14.d.3" &&
                     item.state == analysis::StandardRuleState::ManualReviewRequired;
          }), "separate mapping lines stay unreported and unproved cases require review");

    result = analyze("entity child is port(a,b,c: in bit); end entity;\n"
        "entity top is port(x: in bit); end entity;\n"
        "architecture rtl of top is begin\n"
        "u0: child port map(\n"
        "\ta => x,\n"
        "\t  b => x,\n"
        "\t  c => x\n"
        "); end architecture;\n", true);
    check(count(result, "GJB9765.4.14.b.6") == 1 &&
          std::ranges::any_of(result.violations, [](const auto& item) {
              return item.ruleId == "GJB9765.4.14.b.6" && item.line == 6 &&
                     item.severity == analysis::Severity::Warning;
          }), "native VHDL port-map alignment compares adjacent tab-expanded columns");
    result = analyze("entity child is port(a,b: in bit); end entity;\n"
        "entity top is port(x: in bit); end entity;\n"
        "architecture rtl of top is begin\n"
        "u0: child port map(\n"
        "\ta => x,\n"
        "    b => x\n"
        "); end architecture;\n", true);
    check(count(result, "GJB9765.4.14.b.6") == 0 &&
          std::ranges::any_of(result.standardAssessments, [](const auto& item) {
              return item.clauseId == "GJB9765.4.14.b.6" &&
                     item.state == analysis::StandardRuleState::ManualReviewRequired;
          }), "tab and spaces reaching the same display column stay unreported and unproved");

    result = analyze("entity probe is port(clk,a: in bit); end entity;\n"
        "architecture rtl of probe is begin\n"
        "p: process(clk) begin\n"
        "if rising_edge(clk) then\n"
        "if a = '1' then\n"
        "null;\nend if;\nend if;\n"
        "end process;\nend architecture;\n", true);
    check(count(result, "GJB9765.4.14.b.4") == 1 &&
          std::ranges::any_of(result.violations, [](const auto& item) {
              return item.ruleId == "GJB9765.4.14.b.4" && item.line == 5 &&
                     item.severity == analysis::Severity::Warning;
          }), "native VHDL nested IF must begin to the right of its parent branch");
    result = analyze("entity probe is port(a,b: in bit); end entity;\n"
        "architecture rtl of probe is begin\n"
        "p: process(a,b) begin\n"
        "if a = '1' then\n"
        " null;\n"
        "elsif b = '1' then\n"
        "if a = '0' then null; end if;\n"
        "else\n"
        "if b = '0' then null; end if;\n"
        "end if;\nend process;\nend architecture;\n", true);
    check(count(result, "GJB9765.4.14.b.4") == 2,
          "elsif and else branches independently link their first nested IF");
    result = analyze("entity probe is port(a,b: in bit); end entity;\n"
        "architecture rtl of probe is begin\n"
        "p: process(a,b) begin\n"
        "if a = '1' then\n"
        " if b = '1' then null; end if;\n"
        "if b = '0' then null; end if;\n"
        "end if;\nend process;\nend architecture;\n", true);
    check(count(result, "GJB9765.4.14.b.4") == 0 &&
          std::ranges::any_of(result.standardAssessments, [](const auto& item) {
              return item.clauseId == "GJB9765.4.14.b.4" &&
                     item.state == analysis::StandardRuleState::ManualReviewRequired;
          }), "native branch collector links only the first nested IF and absence stays unproved");
    result = analyze("entity probe is port(a: in bit); end entity;\n"
        "architecture rtl of probe is begin\n"
        "g: if a = '1' generate\n"
        "x: block begin end block;\n"
        "end generate;\nend architecture;\n", true);
    check(count(result, "GJB9765.4.14.b.4") == 0,
          "if-generate is not conflated with the recovered sequential IF tree");

    result = analyze("entity probe is end entity; \n"
        "architecture rtl of probe is begin end architecture;\n", true);
    check(count(result, "GJB9765.4.14.b.2") == 1 &&
          std::ranges::any_of(result.violations, [](const auto& item) {
              return item.ruleId == "GJB9765.4.14.b.2" && item.line == 1 &&
                     item.column == 28;
          }), "native VHDL b.2 reports the ASCII space immediately before LF");
    result = analyze("entity probe is end entity;\t\n"
        "architecture rtl of probe is begin end architecture;", true);
    check(count(result, "GJB9765.4.14.b.2") == 0 &&
          std::ranges::any_of(result.standardAssessments, [](const auto& item) {
              return item.clauseId == "GJB9765.4.14.b.2" &&
                     item.state == analysis::StandardRuleState::ManualReviewRequired;
          }), "native b.2 does not map generic STYLE-001 trailing Tab or unterminated final line");
    result = analyze("entity probe is end entity;\r\n"
        "-- comment  \r\n"
        " \r\n"
        "architecture rtl of probe is begin end architecture; ", true);
    check(count(result, "GJB9765.4.14.b.2") == 2 &&
          std::ranges::count_if(result.violations, [](const auto& item) {
              return item.ruleId == "GJB9765.4.14.b.2" &&
                     (item.line == 2 || item.line == 3);
          }) == 2, "native b.2 includes comment and whitespace-only CRLF lines but not EOF without LF");
    result = analyze("entity probe is end entity;\n" + std::string(509, 'x') + " \n", true);
    check(count(result, "GJB9765.4.14.b.2") == 1 &&
          std::ranges::any_of(result.violations, [](const auto& item) {
              return item.ruleId == "GJB9765.4.14.b.2" && item.line == 2 &&
                     item.column == 510;
          }), "native b.2 reports a space before LF when both fit in one 511-byte fgets payload");
    result = analyze("entity probe is end entity;\n" + std::string(510, 'x') + " \n", true);
    check(count(result, "GJB9765.4.14.b.2") == 0,
          "native b.2 does not join a trailing space in one 511-byte chunk with LF in the next chunk");

    result = analyze("entity probe is port(clk: in bit); end entity;\n"
        "architecture rtl of probe is begin\n"
        "\n"
        "p1: process(clk) begin wait; end process;\n"
        "p2: process(clk) begin wait; end process;\n"
        "end architecture;\n", true);
    check(count(result, "GJB9765.4.14.a.1") == 1 &&
          std::ranges::any_of(result.violations, [](const auto& item) {
              return item.ruleId == "GJB9765.4.14.a.1" && item.line == 5;
          }), "native VHDL a.1 reports a second PROCESS following code without a separator");
    result = analyze("entity probe is port(clk: in bit); end entity;\n"
        "architecture rtl of probe is begin\n"
        "\n"
        "p1: process(clk) begin wait; end process;\n"
        "\n"
        "p2: process(clk) begin wait; end process;\n"
        "end architecture;\n", true);
    check(count(result, "GJB9765.4.14.a.1") == 0 &&
          std::ranges::any_of(result.standardAssessments, [](const auto& item) {
              return item.clauseId == "GJB9765.4.14.a.1" &&
                     item.state == analysis::StandardRuleState::ManualReviewRequired;
          }), "one blank line separates sequential VHDL processes without proving all section boundaries");
    result = analyze("entity probe is port(clk: in bit); end entity;\n"
        "architecture rtl of probe is begin\n"
        "\n"
        "p1: process(clk) begin wait; end process;\n"
        "  -- separator comment\n"
        "p2: postponed process(clk) begin wait; end process;\n"
        "end architecture;\n", true);
    check(count(result, "GJB9765.4.14.a.1") == 0,
          "a comment-only predecessor is accepted by the recovered native byte scan");
    result = analyze("entity probe is port(clk: in bit); end entity;\n"
        "architecture rtl of probe is begin\n"
        "\n"
        "p1: process(clk) begin wait; end process; -- trailing comment\n"
        "p2: postponed process(clk) begin wait; end process;\n"
        "end architecture;\n", true);
    check(count(result, "GJB9765.4.14.a.1") == 1,
          "code before an inline comment remains a predecessor violation");

    result = analyze("entity probe is\n"
        "port(\n"
        "   clk : in bit;\n"
        "\ta : in bit\n"
        ");\nend entity;\n", true);
    check(count(result, "GJB9765.4.14.b.1") == 1 &&
          std::ranges::any_of(result.violations, [](const auto& item) {
              return item.ruleId == "GJB9765.4.14.b.1" && item.line == 3;
          }), "native VHDL b.1 reports a source line indented with spaces");
    result = analyze("entity probe is\n"
        "port(\n"
        "\tclk : in bit;\n"
        "\ta : in bit\n"
        ");\nend entity;\n", true);
    check(count(result, "GJB9765.4.14.b.1") == 0 &&
          std::ranges::any_of(result.standardAssessments, [](const auto& item) {
              return item.clauseId == "GJB9765.4.14.b.1" &&
                     item.state == analysis::StandardRuleState::ManualReviewRequired;
          }), "native VHDL b.1 accepts leading Tabs without proving the full style rule");
    result = analyze("entity probe is end entity;\n"
        "   \n"
        "   -- comment\n"
        "\t -- tab before code byte\n", true);
    check(count(result, "GJB9765.4.14.b.1") == 2 &&
          std::ranges::any_of(result.violations, [](const auto& item) {
              return item.ruleId == "GJB9765.4.14.b.1" && item.line == 2;
          }) && std::ranges::any_of(result.violations, [](const auto& item) {
              return item.ruleId == "GJB9765.4.14.b.1" && item.line == 3;
          }), "native byte scan includes whitespace-only and comment lines but accepts an intervening Tab");
    result = analyze("entity probe is end entity;\n" + std::string(511, ' ') + "a\n", true);
    check(count(result, "GJB9765.4.14.b.1") == 1 &&
          std::ranges::any_of(result.violations, [](const auto& item) {
              return item.ruleId == "GJB9765.4.14.b.1" && item.line == 2;
          }), "native b.1 scans a long physical line as independent 511-byte fgets chunks");
    result = analyze("entity probe is end entity;\n\t" + std::string(510, ' ') + " a\n", true);
    check(count(result, "GJB9765.4.14.b.1") == 1 &&
          std::ranges::any_of(result.violations, [](const auto& item) {
              return item.ruleId == "GJB9765.4.14.b.1" && item.line == 3;
          }), "native b.1 accepts the Tab in the first long-line chunk and reports the next chunk ordinal");

    result = analyze("entity probe is port(a: in bit; q: out bit); end entity;\n"
        "architecture rtl of probe is constant CTMP : bit := '1'; begin\n"
        "p: process(a) begin case (ctmp) is\n"
        "when '0' => q <= a; when others => q <= '1';\n"
        "end case; end process; end architecture;\n", true);
    check(count(result, "GJB9765.4.8.a") == 1 &&
          std::ranges::any_of(result.violations, [](const auto& item) {
              return item.ruleId == "GJB9765.4.8.a" && item.line == 3;
          }), "native VHDL 4.8.a resolves a case selector declared as an architecture constant");
    result = analyze("entity probe is port(q: out bit); end entity;\n"
        "architecture rtl of probe is constant CTMP : bit := '1'; begin\n"
        "p: process begin case\n"
        "CTMP is when others => q <= '1'; end case; wait; end process;\n"
        "end architecture;\n", true);
    check(count(result, "GJB9765.4.8.a") == 1 &&
          std::ranges::any_of(result.violations, [](const auto& item) {
              return item.ruleId == "GJB9765.4.8.a" && item.line == 4;
          }), "native VHDL 4.8.a locates a multiline violation on the selector");
    result = analyze("entity probe is generic(MODE: integer := 0); port(q: out bit); end entity;\n"
        "architecture rtl of probe is begin\n"
        "p: process begin case MODE is when others => q <= '1'; end case; wait; end process;\n"
        "end architecture;\n", true);
    check(count(result, "GJB9765.4.8.a") == 1,
          "native VHDL 4.8.a constant-name set includes direct entity generic identifiers");
    result = analyze("entity probe is port(q: out bit); end entity;\n"
        "architecture rtl of probe is begin\n"
        "p: process constant MODE: integer := 0; begin\n"
        "case MODE is when others => q <= '1'; end case; wait; end process;\n"
        "end architecture;\n", true);
    check(count(result, "GJB9765.4.8.a") == 1,
          "native VHDL 4.8.a recursive flagged-constant collection includes process declarations");
    result = analyze("entity generic_owner is generic(MODE: integer := 0); end entity;\n"
        "architecture rtl of generic_owner is begin end architecture;\n"
        "entity signal_owner is port(MODE: in integer; q: out bit); end entity;\n"
        "architecture rtl of signal_owner is begin\n"
        "p: process(MODE) begin case MODE is when others => q <= '1'; end case; end process;\n"
        "end architecture;\n", true);
    check(count(result, "GJB9765.4.8.a") == 0,
          "native VHDL 4.8.a generic names remain isolated to their owning entity root");
    result = analyze("entity probe is port(a: in bit; q: out bit); end entity;\n"
        "architecture rtl of probe is constant CTMP : bit := '1'; begin\n"
        "p: process(a) begin case a is\n"
        "when '0' => q <= CTMP; when others => q <= '1';\n"
        "end case; end process; end architecture;\n", true);
    check(count(result, "GJB9765.4.8.a") == 0 &&
          std::ranges::any_of(result.standardAssessments, [](const auto& item) {
              return item.clauseId == "GJB9765.4.8.a" &&
                     item.state == analysis::StandardRuleState::ManualReviewRequired;
          }), "a signal selector stays unreported even when a constant appears in a case arm");
    result = analyze("entity probe is port(a: in bit; q: out bit); end entity;\n"
        "architecture rtl of probe is constant CTMP : bit := '1'; begin\n"
        "p: process(a) begin case '1' is when others => q <= a; end case;\n"
        "case CTMP & '0' is when others => q <= a; end case;\n"
        "case CTMP & a is when others => q <= a; end case;\n"
        "end process; end architecture;\n", true);
    check(count(result, "GJB9765.4.8.a") == 2,
          "native selector categories include literals and compounds with only constant identifiers");
    result = analyze("entity probe is port(a: in bit; q: out bit); end entity;\n"
        "architecture rtl of probe is constant CTMP : bit := '1'; begin\n"
        "g: case CTMP generate when others => x: block begin end block; end generate;\n"
        "p: process(a) begin case a is when others => q <= a; end case;\n"
        "end process; end architecture;\n", true);
    check(count(result, "GJB9765.4.8.a") == 0,
          "case-generate is outside the recovered sequential CASE selector predicate");
    result = analyze("entity probe is port(a: in bit; q: out bit); end entity;\n"
        "architecture rtl of probe is begin\n"
        "p: process(a) begin\n"
        "if a = '1' then\n"
        "end if;\n"
        "if a = '0' then q <= a; else null; end if;\n"
        "end process; end architecture;\n", true);
    check(count(result, "GJB9765.4.8.e") == 2 &&
          std::ranges::any_of(result.violations, [](const auto& item) {
              return item.ruleId == "GJB9765.4.8.e" && item.line == 4;
          }) && std::ranges::any_of(result.violations, [](const auto& item) {
              return item.ruleId == "GJB9765.4.8.e" && item.line == 6;
          }), "native VHDL 4.8.e detects an empty then arm and an explicit null statement");
    result = analyze("entity probe is port(a: in bit; q: out bit); end entity;\n"
        "architecture rtl of probe is begin\n"
        "p: process(a) begin if a = '1' then q <= a; else q <= '0'; end if; end process;\n"
        "end architecture;\n", true);
    check(count(result, "GJB9765.4.8.e") == 0 &&
          std::ranges::any_of(result.standardAssessments, [](const auto& item) {
              return item.clauseId == "GJB9765.4.8.e" &&
                     item.state == analysis::StandardRuleState::ManualReviewRequired;
          }), "populated VHDL branches stay unreported and are not auto-cleared");
    result = analyze("entity probe is port(q: out bit); end entity;\n"
        "architecture rtl of probe is begin\n"
        "p: process begin null; wait; end process;\n"
        "end architecture;\n", true);
    check(count(result, "GJB9765.4.8.e") == 1,
          "explicit null followed by a real statement is still an empty-statement violation");
    result = analyze("entity probe is end entity;\n"
        "architecture rtl of probe is begin\n"
        "p: process begin end process;\n"
        "end architecture;\n", true);
    check(count(result, "GJB9765.4.8.e") == 1,
          "an empty sequential process body is an empty-statement violation");
    result = analyze("entity probe is port(a: in bit; q: out bit); end entity;\n"
        "architecture rtl of probe is begin\n"
        "p: process(a) begin case a is\n"
        "when '0' => when others => q <= a; end case;\n"
        "end process; end architecture;\n", true);
    check(count(result, "GJB9765.4.8.e") == 0 &&
          std::ranges::any_of(result.standardAssessments, [](const auto& item) {
              return item.clauseId == "GJB9765.4.8.e" &&
                     item.state == analysis::StandardRuleState::ManualReviewRequired;
          }), "empty CASE alternatives are COND nodes, not recovered empty BLOCK nodes");
    result = analyze("entity probe is port(a: in bit); end entity;\n"
        "architecture rtl of probe is begin\n"
        "p: process(a) begin while a = '1' loop end loop; wait; end process;\n"
        "end architecture;\n", true);
    check(count(result, "GJB9765.4.8.e") == 0,
          "empty LOOP bodies retain a LOOP child and are not empty sequential BLOCKs");
    // Entry-level report fields are independently recovered, even though the
    // source-to-AST adapter and every lowered-node location remain partial.
    for (const std::string body : {
             "null; wait;", "end process;", "if true then end if; wait;",
             "case 0 is when others => end case; wait;",
             "while false loop end loop; wait;", "null; null; wait;"}) {
        const bool closesProcess = body == "end process;";
        const auto sample = analyze(
            "entity probe is end entity;\narchitecture rtl of probe is begin\n"
            "p: process begin\n  " + body +
            (closesProcess ? "\n" : "\nend process;\n") + "end architecture;\n", true);
        const auto expected = body == "null; null; wait;" ? 2 :
            (body.starts_with("case") || body.starts_with("while")) ? 0 : 1;
        check(count(sample, "GJB9765.4.8.e") == expected &&
              std::ranges::all_of(sample.violations, [](const auto& item) {
                  return item.ruleId != "GJB9765.4.8.e" ||
                         (item.message == "避免在进程中出现空语句" &&
                          item.column == 1 && item.severity == analysis::Severity::Warning);
              }), "VHDL empty-statement reports preserve multiplicity and fixed native fields");
    }
    const auto checkVhdlEmptyLines = [&](const std::string& statements,
                                          std::vector<std::size_t> expected,
                                          const char* description) {
        const auto sample = analyze("entity probe is end entity;\n"
            "architecture rtl of probe is begin\n" + statements + "\nend architecture;\n", true);
        std::vector<std::size_t> actual;
        for (const auto& item : sample.violations) {
            if (item.ruleId != "GJB9765.4.8.e") continue;
            actual.push_back(item.line);
            check(item.message == "避免在进程中出现空语句" && item.column == 1 &&
                  item.severity == analysis::Severity::Warning,
                  "VHDL NULL/source-block variants retain the recovered report surface");
        }
        std::ranges::sort(actual);
        std::ranges::sort(expected);
        check(actual == expected, description);
    };
    checkVhdlEmptyLines("process\nbegin\nend process;", {3},
        "empty process location belongs to the PROCESS context, not BEGIN");
    checkVhdlEmptyLines("p:\nprocess\nbegin\nend process p;", {3},
        "a multiline process label owns the empty root location");
    checkVhdlEmptyLines("postponed\nprocess\nbegin\nend postponed process;", {3},
        "POSTPONED starts an unlabeled process context");
    checkVhdlEmptyLines("p:\npostponed\nprocess is\nbegin\nend postponed process p;", {3},
        "a label precedes POSTPONED in the native process context");
    checkVhdlEmptyLines("p: process(all) is\nbegin\nend process;", {3},
        "an ALL-sensitive empty process has one empty root report");
    checkVhdlEmptyLines("p: process is\nconstant C : integer := 0;\nbegin\nend process;", {},
        "a process declaration occupies the native BLOCK even without sequential statements");
    checkVhdlEmptyLines("p: process begin\nn:\nnull;\nwait; end process;", {4},
        "labeled NULL statements use the sequential context label line");
    checkVhdlEmptyLines("p: process begin\nn1: null; n2: null;\nwait; end process;", {4,4},
        "different NULL labels on one line preserve separate empty-node reports");
    checkVhdlEmptyLines("p: process\ntype ptr is access integer;\nvariable v: ptr :=\nnull;\n"
        "begin wait; end process;", {6},
        "a NULL literal in a process declaration is admitted before BEGIN");
    checkVhdlEmptyLines("p: process\ntype ptr is access integer;\n"
        "type ptrs is array(0 to 1) of ptr;\nvariable v: ptrs;\n"
        "begin v := (null, null); wait; end process;", {7,7},
        "NULL literals in aggregates are separate native nodes without a following semicolon");
    checkVhdlEmptyLines("p: process\ntype ptr is access integer; variable v: ptr;\n"
        "begin if v = null then v := null; end if; wait; end process;", {5,5},
        "condition and assignment NULL literals each report once");
    checkVhdlEmptyLines("p: process\ntype ptr is access integer;\n"
        "function f return ptr is begin return\nnull; end function;\n"
        "begin wait; end process;", {6},
        "NULL literals in a local subprogram remain under the owning process tree");
    checkVhdlEmptyLines("p: postponed process begin wait; end postponed process;\n"
        "b: block is type ptr is access integer; signal v: ptr := null; begin end block;", {},
        "END POSTPONED PROCESS closes admission before subsequent concurrent declarations");
    checkVhdlEmptyLines("p: process begin null; wait; end process;\n"
        "q:\npostponed process\nbegin\nend postponed process q;", {3,4},
        "separate normal and postponed processes retain independent ownership");
    checkVhdlEmptyLines("p: process begin report \"null; end process;\";\n"
        "-- null;\nwait; end process;", {},
        "strings and comments do not create NULL or process boundary tokens");
    checkVhdlEmptyLines("p: process begin\nif\n true\nthen\nend if;\nwait; end process;", {5},
        "the initial empty IF arm inherits the condition start, not THEN or IF");
    checkVhdlEmptyLines("p: process begin\nif true then wait for 1 ns; else end if;\n"
        "wait; end process;", {},
        "a plain empty ELSE node is not an empty native BLOCK");
    checkVhdlEmptyLines("p: process begin\nif true then wait for 1 ns; elsif\n"
        "false then\nend if; wait; end process;", {0},
        "an empty ELSIF true BLOCK preserves the native constructor's unknown line zero");
    checkVhdlEmptyLines("p: process begin loop null; end loop; wait; end process;", {3},
        "a real NULL statement inside a loop still creates an empty sequential BLOCK");
    checkVhdlEmptyLines("p: process begin null;", {},
        "an unterminated process does not establish an admitted native root");
    result = analyze("entity very_long_entity_name_for_scope is\n"
        "port(port_01234567890123456789: out bit; short: in bit); end entity;\n"
        "architecture rtl of very_long_entity_name_for_scope is\n"
        "signal sig_01234567890123456789 : bit;\n"
        "signal short_a, sig_abcdefghijklmnopqrstuvwxyz : bit;\n"
        "begin port_01234567890123456789 <= sig_01234567890123456789;\n"
        "end architecture;\n", true);
    check(count(result, "GJB9765.4.5.f.1") == 4 &&
          std::ranges::any_of(result.violations, [](const auto& item) {
              return item.ruleId == "GJB9765.4.5.f.1" && item.line == 1;
          }), "native VHDL name-length collector includes entity, port and internal signal nodes");
    result = analyze("entity entity_name_longer_than_twenty is port(a: in bit); end entity;\n"
        "architecture rtl of entity_name_longer_than_twenty is begin end architecture;\n", true);
    check(count(result, "GJB9765.4.5.f.1") == 1,
          "native A-5-2 reports a long entity declaration name");
    result = analyze(
        "entity short_entity is\n"
        "generic(generic_name_0123456789: integer := 1); port(a: in bit);\n"
        "end entity;\n"
        "architecture architecture_name_0123456789 of short_entity is\n"
        "type type_name_012345678901 is (idle, run);\n"
        "subtype subtype_name_0123456789 is integer;\n"
        "constant constant_name_0123456789: integer := 1;\n"
        "signal signal_name_012345678901: bit;\n"
        "shared variable variable_name_0123456789: integer;\n"
        "function function_name_0123456789 return integer is begin return 0; end function;\n"
        "procedure procedure_name_0123456789 is begin null; end procedure;\n"
        "begin\n"
        "process_label_0123456789: process variable local_variable_0123456789: integer;\n"
        "begin wait; end process;\n"
        "block_label_012345678901: block is begin end block;\n"
        "end architecture;\n", true);
    check(count(result, "GJB9765.4.5.f.1") == 12,
          "native A-5-2 covers generic, architecture, type, subtype, constant, signal, "
          "variable, function, procedure, process and block names");
    result = analyze(
        "entity probe is port(a: in bit); end entity;\n"
        "architecture rtl of probe is begin\n"
        "b_0123456789012345678: block is begin end block;\n"
        "b_01234567890123456789: block is begin end block;\n"
        "b_012345678901234567890: block is begin end block;\n"
        "end architecture;\n", true);
    check(count(result, "GJB9765.4.5.f.1") == 1 &&
          std::ranges::any_of(result.violations, [](const auto& item) {
              return item.ruleId == "GJB9765.4.5.f.1" && item.line == 5;
          }), "native A-5-2 AST_BLOCK path excludes 21/22-byte labels and reports 23 bytes");
    result = analyze("entity probe is port(a,sel: in bit; c: inout bit); end entity;\n"
        "architecture rtl of probe is signal internal: bit; begin\n"
        "internal <= a when sel='1' else 'Z';\n"
        "c <= a when sel='1' else 'Z';\n"
        "end architecture;\n", true);
    check(count(result, "GJB9765.4.2.c") == 1 &&
          std::ranges::any_of(result.violations, [](const auto& item) {
              return item.ruleId == "GJB9765.4.2.c" && item.line == 3;
          }), "native VHDL tri-state rule distinguishes an internal signal from an I/O port");
    result = analyze("entity probe is port(a,sel: in bit); end entity;\n"
        "architecture rtl of probe is signal internal: bit; begin\n"
        "internal <= a when sel/='1' else 'Z';\n"
        "end architecture;\n", true);
    check(count(result, "GJB9765.4.2.c") == 0,
          "native 4.2.c comparison mask excludes AST_NE and VHDL /= candidates");
    result = analyze("entity probe is port(a,sel: in bit); end entity;\n"
        "architecture rtl of probe is signal internal: bit; begin\n"
        "internal <= a when sel?='1' else 'Z';\n"
        "end architecture;\n", true);
    check(count(result, "GJB9765.4.2.c") == 1,
          "native 4.2.c comparison mask includes AST_EQX and VHDL matching equality");
    result = analyze("entity probe is port(a,sel,guard: in bit); end entity;\n"
        "architecture rtl of probe is signal internal: bit; begin\n"
        "internal <= a when sel='1' and guard='1' else 'Z';\n"
        "end architecture;\n", true);
    check(count(result, "GJB9765.4.2.c") == 0,
          "native direct 4.2.c candidate requires a relation root, not a compound AND condition");
    result = analyze("entity probe is port(a,sel,guard: in bit); end entity;\n"
        "architecture rtl of probe is signal internal: bit; begin\n"
        "p: process(a,sel,guard) begin if sel='1' and guard='1' then\n"
        "internal <= a; else internal <= 'Z'; end if; end process;\n"
        "end architecture;\n", true);
    check(count(result, "GJB9765.4.2.c") == 0,
          "native IF 4.2.c candidate requires a relation root, not a compound AND condition");
    result = analyze("entity probe is port(a: in bit); end entity;\n"
        "architecture rtl of probe is signal sel: integer; signal internal: bit; begin\n"
        "internal <= a when sel + 1 = 2 else 'Z';\n"
        "end architecture;\n", true);
    check(count(result, "GJB9765.4.2.c") == 0,
          "native direct 4.2.c relation lookup does not promote a name inside an arithmetic operand");
    result = analyze("entity probe is port(a: in bit); end entity;\n"
        "architecture rtl of probe is signal sel: integer; signal internal: bit; begin\n"
        "p: process(a,sel) begin if sel + 1 = 2 then\n"
        "internal <= a; else internal <= 'Z'; end if; end process;\n"
        "end architecture;\n", true);
    check(count(result, "GJB9765.4.2.c") == 0,
          "native IF 4.2.c relation lookup does not promote a name inside an arithmetic operand");
    result = analyze("entity probe is port(a: in bit); end entity;\n"
        "architecture rtl of probe is signal sel: integer; signal internal: bit; begin\n"
        "internal <= a when 2 = sel else 'Z';\n"
        "end architecture;\n", true);
    check(count(result, "GJB9765.4.2.c") == 1,
          "native direct 4.2.c relation lookup accepts a declared direct right operand");
    result = analyze("entity probe is generic(enable: bit := '1'); port(a: in bit); end entity;\n"
        "architecture rtl of probe is signal internal: bit; begin\n"
        "internal <= a when enable='1' else 'Z';\n"
        "end architecture;\n", true);
    check(count(result, "GJB9765.4.2.c") == 1,
          "native 4.2.c control-name table includes direct AST_GENERIC identifiers");
    result = analyze("entity probe is port(a: in bit); end entity;\n"
        "architecture rtl of probe is constant enable: bit := '1'; signal internal: bit; begin\n"
        "internal <= a when enable='1' else 'Z';\n"
        "end architecture;\n", true);
    check(count(result, "GJB9765.4.2.c") == 0,
          "native 4.2.c control-name table excludes AST_CONSTANT declarations");
    result = analyze("entity probe is port(a: in bit); end entity;\n"
        "architecture rtl of probe is signal internal: bit; begin\n"
        "p: process(a) variable enable: bit; begin enable := a;\n"
        "if enable='1' then internal <= a; else internal <= 'Z'; end if; end process;\n"
        "end architecture;\n", true);
    check(count(result, "GJB9765.4.2.c") == 0,
          "native 4.2.c control-name table excludes AST_VARIABLE declarations");
    result = analyze("entity probe is port(a,sel: in bit); end entity;\n"
        "architecture rtl of probe is alias alias_sel is sel; signal internal: bit; begin\n"
        "internal <= a when alias_sel='1' else 'Z';\n"
        "end architecture;\n", true);
    check(count(result, "GJB9765.4.2.c") == 0,
          "native 4.2.c control-name table excludes AST_ALIAS declarations");
    result = analyze("entity probe is port(a: in bit; c: inout bit); end entity;\n"
        "architecture rtl of probe is signal internal: bit_vector(1 downto 0); begin\n"
        "p: process(a) begin if a='1' then internal(0) <= 'Z'; end if; end process;\n"
        "c <= 'Z'; end architecture;\n", true);
    check(count(result, "GJB9765.4.2.c") == 0,
          "one-armed process and bare boundary Z assignments are not native 4.2.c candidates");
    result = analyze("entity probe is port(a,sel: in bit; c: inout bit); end entity;\n"
        "architecture rtl of probe is signal internal: bit_vector(1 downto 0); begin\n"
        "p: process(a,sel) begin if sel='1' then internal(0) <= a; else internal(0) <= 'Z'; end if; end process;\n"
        "end architecture;\n", true);
    check(count(result, "GJB9765.4.2.c") == 1,
          "native 0xaa/0x58/0xac branch reports paired THEN data and ELSE Z on one internal target");
    result = analyze("entity probe is port(a,sel,guard: in bit); end entity;\n"
        "architecture rtl of probe is signal internal: bit; begin\n"
        "p: process(a,sel,guard) begin if sel='1' then\n"
        "if guard='1' then internal <= a; end if;\n"
        "else internal <= 'Z'; end if; end process;\n"
        "end architecture;\n", true);
    check(count(result, "GJB9765.4.2.c") == 0,
          "native 4.2.c does not promote a nested IF assignment to the outer direct THEN map");
    result = analyze("entity probe is port(a,sel,guard: in bit); end entity;\n"
        "architecture rtl of probe is signal internal: bit; begin\n"
        "p: process(a,sel,guard) begin if sel='1' then internal <= a;\n"
        "elsif guard='1' then internal <= a; else internal <= 'Z'; end if; end process;\n"
        "end architecture;\n", true);
    check(count(result, "GJB9765.4.2.c") == 0,
          "native 4.2.c two-arm IF candidate excludes an AST_ELSIF branch");
    result = analyze("entity probe is port(a,sel,guard: in bit); end entity;\n"
        "architecture rtl of probe is signal internal: bit; begin\n"
        "p: process(a,sel,guard) begin if sel='1' then\n"
        "case guard is when '1' => internal <= a; when others => null; end case;\n"
        "else internal <= 'Z'; end if; end process;\n"
        "end architecture;\n", true);
    check(count(result, "GJB9765.4.2.c") == 0,
          "native 4.2.c does not promote a nested CASE assignment to a direct THEN block");
    result = analyze("entity probe is port(a,sel,guard: in bit); end entity;\n"
        "architecture rtl of probe is signal internal: bit; begin\n"
        "p: process(a,sel,guard) begin if sel='1' then\n"
        "while guard='1' loop internal <= a; exit; end loop;\n"
        "else internal <= 'Z'; end if; end process;\n"
        "end architecture;\n", true);
    check(count(result, "GJB9765.4.2.c") == 0,
          "native 4.2.c does not promote a nested LOOP assignment to a direct THEN block");
    result = analyze("entity probe is port(a,sel: in bit); end entity;\n"
        "architecture rtl of probe is signal internal: bit; begin\n"
        "p: process(a,sel) begin if sel='1' then internal <= 'Z'; else internal <= a; end if; end process;\n"
        "end architecture;\n", true);
    check(count(result, "GJB9765.4.2.c") == 0,
          "reversed process arms do not match the recovered ELSE-Z native branch");
    result = analyze("entity probe is port(a: in bit; c: inout bit); end entity;\n"
        "architecture rtl of probe is begin c <= a when a='1' else 'Z'; end architecture;\n", true);
    check(count(result, "GJB9765.4.2.c") == 0 &&
          std::ranges::any_of(result.standardAssessments, [](const auto& item) {
              return item.clauseId == "GJB9765.4.2.c" &&
                     item.state == analysis::StandardRuleState::ManualReviewRequired;
          }), "I/O-only tri-state produces no 4.2.c diagnostic but is not auto-cleared");
    {
        analysis::AnalysisOptions selectedTriStateTop;
        selectedTriStateTop.standardId = "GJB9765-2020";
        selectedTriStateTop.topModule = "TOP_A";
        const std::vector<core::SourceFile> roots{
            {"tri-state-roots.vhd", core::HdlLanguage::Vhdl,
             "entity top_a is port(a: in bit; io_a: inout bit); end entity;\n"
             "architecture rtl of top_a is begin io_a <= a when a='1' else 'Z'; end architecture;\n"
             "entity top_b is port(a: in bit; io_b: inout bit); end entity;\n"
             "architecture rtl of top_b is begin io_b <= a when a='1' else 'Z'; end architecture;\n"}
        };
        auto selected = analysis::Analyzer(selectedTriStateTop).analyzeSources(roots);
        check(selected.errors.empty() && count(selected, "GJB9765.4.2.c") == 1 &&
                  std::ranges::any_of(selected.violations, [](const auto& item) {
                      return item.ruleId == "GJB9765.4.2.c" && item.line == 4;
                  }), "4.2.c permits only the explicitly selected top INOUT boundary");
        selectedTriStateTop.topModule = "top_b";
        selected = analysis::Analyzer(selectedTriStateTop).analyzeSources(roots);
        check(selected.errors.empty() && count(selected, "GJB9765.4.2.c") == 1 &&
                  std::ranges::any_of(selected.violations, [](const auto& item) {
                      return item.ruleId == "GJB9765.4.2.c" && item.line == 2;
                  }), "4.2.c treats an INOUT on every non-selected root as internal hierarchy tri-state");
    }
    {
        analysis::AnalysisOptions selectedTriStateTop;
        selectedTriStateTop.standardId = "GJB9765-2020";
        selectedTriStateTop.topModule = "top";
        const auto selected = analysis::Analyzer(selectedTriStateTop).analyzeSources({
            {"tri-state-output.vhd", core::HdlLanguage::Vhdl,
             "entity top is port(a: in bit; q: out bit); end entity;\n"
             "architecture rtl of top is begin q <= a when a='1' else 'Z'; end architecture;\n"}
        });
        check(selected.errors.empty() && count(selected, "GJB9765.4.2.c") == 1,
              "4.2.c top-boundary exemption is limited to INOUT and does not admit OUT tri-state");
    }
    result = analyze("entity probe is port(a: in bit; q: out bit); end entity;\n"
        "architecture rtl of probe is signal internal: bit; begin\n"
        "internal <= a when a='Z' else '0'; q <= internal;\n"
        "end architecture;\n", true);
    check(count(result, "GJB9765.4.2.c") == 0,
          "Z in a conditional assignment predicate is not itself a tri-state drive");

    result = analyze("entity probe is port(clk,a,b: in bit; sel: in bit_vector(1 downto 0)); end entity;\n"
        "architecture rtl of probe is begin\n"
        "p: process(clk) begin\n"
        "if rising_edge(clk) then\n"
        "if a = '1' then\n"
        "\tnull;\n"
        "\telsif b = '1' then\n"
        "\tnull;\n"
        "\telse\n"
        "\tnull;\n"
        "\tend if;\n"
        "\tcase sel is\n"
        "\t\twhen \"00\" => null;\n"
        "\t\twhen \"01\" => null;\n"
        "\t  when others => null;\n"
        "\tend case;\n"
        "end if; end process; end architecture;\n", true);
    check(count(result, "GJB9765.4.14.b.3") == 3 &&
          std::ranges::count_if(result.violations, [](const auto& item) {
              return item.ruleId == "GJB9765.4.14.b.3" && item.line == 15;
          }) == 1,
          "native b.3 attachment pattern reports misaligned elsif, else, and final WHEN");
    result = analyze("entity probe is port(a,b: in bit; sel: in bit_vector(1 downto 0)); end entity;\n"
        "architecture rtl of probe is begin p: process(a,b,sel) begin\n"
        "\tif a = '1' then\n"
        "\t\tnull;\n"
        "    elsif b = '1' then\n"
        "\t\tnull;\n"
        "    else\n"
        "\t\tnull;\n"
        "\tend if;\n"
        "\tcase sel is\n"
        "\t\twhen \"00\" => null;\n"
        "        when others => null;\n"
        "\tend case;\n"
        "end process; end architecture;\n", true);
    check(count(result, "GJB9765.4.14.b.3") == 0 &&
          std::ranges::any_of(result.standardAssessments, [](const auto& item) {
              return item.clauseId == "GJB9765.4.14.b.3" &&
                     item.state == analysis::StandardRuleState::ManualReviewRequired;
          }), "tabs and spaces at equal display columns satisfy the bounded predicate but remain unproved");
    result = analyze("entity probe is port(sel: in bit_vector(1 downto 0)); end entity;\n"
        "architecture rtl of probe is begin p: process(sel) begin\n"
        "\tcase sel is\n"
        "\twhen \"00\" => null;\n"
        "\twhen others => null;\n"
        "\tend case; end process; end architecture;\n", true);
    check(count(result, "GJB9765.4.14.b.3") == 2,
          "each aligned WHEN at the CASE column independently violates the native strict-depth predicate");
    result = analyze("entity probe is port(sel: in bit_vector(1 downto 0)); end entity;\n"
        "architecture rtl of probe is begin p: process(sel) begin\n"
        "\tcase sel is\n"
        "\t\twhen \"00\" => null;\n"
        "when others => null;\n"
        "\tend case; end process; end architecture;\n", true);
    check(count(result, "GJB9765.4.14.b.3") == 2 &&
          std::ranges::count_if(result.violations, [](const auto& item) {
              return item.ruleId == "GJB9765.4.14.b.3" && item.line == 5;
          }) == 2,
          "one shallow and off-baseline later WHEN emits both recovered native messages");
    result = analyze("entity probe is port(sel: in bit_vector(1 downto 0)); end entity;\n"
        "architecture rtl of probe is begin p: process(sel) begin\n"
        "\tcase sel is\n"
        "\t\twhen \"00\" => null;\n"
        "\t\t when others => null;\n"
        "\tend case; end process; end architecture;\n", true);
    check(count(result, "GJB9765.4.14.b.3") == 1 &&
          std::ranges::any_of(result.violations, [](const auto& item) {
              return item.ruleId == "GJB9765.4.14.b.3" &&
                     item.message.find("首个分支") != std::string::npos;
          }), "a deeper but differently aligned later WHEN triggers only the same-level predicate");
    result = analyze("entity probe is port(a: in bit); end entity;\n"
        "architecture rtl of probe is begin\n"
        "g: if a = '1' generate\n"
        "else generate\n"
        "end generate;\n"
        "q <= '1' when a = '1' else '0';\n"
        "end architecture;\n", true);
    check(count(result, "GJB9765.4.14.b.3") == 0,
          "IF-generate and conditional assignment keywords are outside the recovered sequential groups");
    result = analyze("entity probe is port(sel: in bit_vector(1 downto 0)); end entity;\n"
        "architecture rtl of probe is begin p: process(sel) begin\n"
        "\tcase sel is\n"
        "\t\twhen \"00\" =>\n"
        "\t\t\tcase sel is\n"
        "\t\t\t\twhen \"01\" => null;\n"
        "\t\t\t\twhen others => null;\n"
        "\t\t\tend case;\n"
        "\t\twhen others => null;\n"
        "\tend case; end process; end architecture;\n", true);
    check(count(result, "GJB9765.4.14.b.3") == 0,
          "nested CASE statements retain independent WHEN baselines and restore the outer group");

    result = analyze("entity probe is\n"
        "port(\n"
        "\tCLK    : in bit;\n"
        "   INPUT1 : in bit;\n"
        "\tINPUT2 : in bit_vector(1 downto 0);\n"
        "\tB1     : in bit\n"
        ");\nend entity;\n", true);
    check(count(result, "GJB9765.4.14.b.5") == 2 &&
          std::ranges::all_of(result.violations | std::views::filter([](const auto& item) {
              return item.ruleId == "GJB9765.4.14.b.5";
          }), [](const auto& item) { return (item.line == 4 || item.line == 5) &&
                                             item.severity == analysis::Severity::Warning; }),
          "native VHDL declaration alignment reports both adjacent column changes");
    result = analyze("entity probe is\n"
        "port(\n"
        "\tCLK : in bit;\n"
        "    DATA : in bit\n"
        ");\nend entity;\n", true);
    check(count(result, "GJB9765.4.14.b.5") == 0 &&
          std::ranges::any_of(result.standardAssessments, [](const auto& item) {
              return item.clauseId == "GJB9765.4.14.b.5" &&
                     item.state == analysis::StandardRuleState::ManualReviewRequired;
          }), "tab and spaces reaching the same declaration column remain unproved");
    result = analyze("entity probe is end entity;\n"
        "architecture rtl of probe is\n"
        "signal a : bit;\n"
        "  signal b : bit;\n"
        "begin\n"
        "p: process\n"
        "variable v1 : bit;\n"
        " variable v2 : bit;\n"
        "begin wait; end process;\n"
        "end architecture;\n", true);
    check(count(result, "GJB9765.4.14.b.5") == 2,
          "consecutive VHDL signal and variable declaration groups are checked independently");
    result = analyze("entity probe is end entity;\n"
        "architecture rtl of probe is\n"
        "signal a : bit;\n"
        "constant separator : bit := '0';\n"
        "  signal b : bit;\n"
        "begin end architecture;\n", true);
    check(count(result, "GJB9765.4.14.b.5") == 0,
          "unrelated declarations split bounded signal alignment groups");
    result = analyze("entity probe is port(a : in bit;       b : in bit); end entity;\n", true);
    check(count(result, "GJB9765.4.14.b.5") == 0,
          "same-line declaration children are excluded by the recovered native collector");

    result = analyze("entity probe is end entity; architecture rtl of probe is\n"
        "type integer_file is file of integer; file f: integer_file;\n"
        "begin\nFILE_OPEN(f, \"G:/test.dat\");\n"
        "file_open(f, \"C:\\\\data.dat\");\n"
        "FILE_OPEN(f, \"https://example.invalid/data.dat\");\n"
        "end architecture;\n", true);
    check(count(result, "GJB9765.4.14.d.4") == 3 &&
          std::ranges::all_of(result.violations | std::views::filter([](const auto& item) {
              return item.ruleId == "GJB9765.4.14.d.4";
          }), [](const auto& item) { return item.severity == analysis::Severity::Warning; }),
          "native FILE_OPEN colon-slash and colon-backslash literals report warnings per direct literal");
    result = analyze("entity probe is end entity; architecture rtl of probe is\n"
        "type integer_file is file of integer; file f: integer_file;\n"
        "begin\n-- FILE_OPEN(f, \"G:/comment.dat\");\n"
        "FILE_OPEN(f, \"../test.dat\");\n"
        "FILE_OPEN(f, \"/tmp/test.dat\");\n"
        "FILE_OPEN(f, \"logical:name.dat\");\n"
        "report \"G:/not_a_file_open.dat\";\n"
        "FILE_OPEN(f, helper(\"G:/nested.dat\"));\n"
        "end architecture;\n", true);
    check(count(result, "GJB9765.4.14.d.4") == 0 &&
          std::ranges::any_of(result.standardAssessments, [](const auto& item) {
              return item.clauseId == "GJB9765.4.14.d.4" &&
                     item.state == analysis::StandardRuleState::ManualReviewRequired;
          }), "relative, Unix-root, colon-only, unrelated and nested literals stay clear without certifying compliance");

    result = analyze("entity probe is end entity; architecture rtl of probe is\n"
        "signal q, d: bit; begin\n"
        "q <=d;\nq<= d;\nq<=d;\n"
        "process begin variable v: integer; begin v:=1; wait; end process;\n"
        "end architecture;\n", true);
    check(count(result, "GJB9765.4.14.c.2") == 4 &&
          std::ranges::all_of(result.violations | std::views::filter([](const auto& item) {
              return item.ruleId == "GJB9765.4.14.c.2";
          }), [](const auto& item) { return item.severity == analysis::Severity::Warning; }),
          "native VHDL assignment spacing detects missing left or right separation");
    result = analyze("entity probe is end entity; architecture rtl of probe is\n"
        "signal q, d: bit; begin\n"
        "q <= d;\nq\t<=\td;\n"
        "-- q<=d;\nreport \"q<=d\";\n"
        "process begin variable v: integer; begin v := 1; wait; end process;\n"
        "end architecture;\n", true);
    check(count(result, "GJB9765.4.14.c.2") == 1 &&
          std::ranges::any_of(result.violations, [](const auto& item) {
              return item.ruleId == "GJB9765.4.14.c.2" && item.line == 6;
          }), "native raw scan ignores comment text but checks quoted <= text");
    result = analyze("entity probe is end entity; architecture rtl of probe is\n"
        "begin\nreport \"prefix--q<=d\";\nend architecture;\n", true);
    check(count(result, "GJB9765.4.14.c.2") == 0,
          "native raw scan stops at the first literal double-hyphen even inside a string");
    result = analyze("entity probe is end entity; architecture rtl of probe is\n"
        "signal q, d: bit; begin\nq\f<=\fd;\nend architecture;\n", true);
    check(count(result, "GJB9765.4.14.c.2") == 1,
          "native assignment spacing accepts only ASCII space and Tab, not form-feed");
    result = analyze("entity probe is end entity;\n" + std::string(253, 'x') + "<= x\n", true);
    check(count(result, "GJB9765.4.14.c.2") == 0,
          "native c.2 does not join an assignment operator split across 254-byte fgets chunks");
    result = analyze("entity probe is end entity;\n" + std::string(252, 'x') + "<= x\n", true);
    check(count(result, "GJB9765.4.14.c.2") == 0,
          "native c.2 boundary guard skips an assignment operator ending at the chunk boundary");
    result = analyze("entity probe is end entity; architecture rtl of probe is\n"
        "begin\nreport \"x<=--tail\";\nend architecture;\n", true);
    check(count(result, "GJB9765.4.14.c.2") == 1,
          "native c.2 reads the comment opener as the operator right neighbor before stopping the scan");

    result = analyze("entity probe is end entity; architecture rtl of probe is\n"
        "signal q, a, b: integer; begin\n"
        "process begin variable v: integer; begin\n"
        "if (a= 0) then q <= b/2; elsif (b/=1) then q <= a*2; end if;\n"
        "v := a+1; v := b-1; wait; end process;\n"
        "end architecture;\n", true);
    check(count(result, "GJB9765.4.14.c.3") == 6,
          "native VHDL operator spacing detects comparison, arithmetic and inequality cases");
    result = analyze("entity probe is end entity; architecture rtl of probe is\n"
        "signal q, a, b: integer; begin\n"
        "process begin variable v: integer; begin\n"
        "if ( a = 0 ) then q <= b / 2; elsif ( b /= 1 ) then q <= a * 2; end if;\n"
        "v := a + 1; v := b - 1; v := a ** 2;\n"
        "-- q<=a/2;\nreport \"a/2 b+1\"; wait; end process;\n"
        "end architecture;\n", true);
    check(count(result, "GJB9765.4.14.c.3") == 2 &&
          std::ranges::all_of(result.violations | std::views::filter([](const auto& item) {
              return item.ruleId == "GJB9765.4.14.c.3";
          }), [](const auto& item) { return item.line == 7; }),
          "native raw scan checks quoted operators while excluding spaced code, exponentiation and comments");
    result = analyze("entity probe is end entity; architecture rtl of probe is\n"
        "begin\nreport \"<= := => ** /=>\";\nend architecture;\n", true);
    check(count(result, "GJB9765.4.14.c.3") == 0,
          "native raw operator scan excludes assignment, arrow, exponentiation and anomalous slash-arrow compounds");
    result = analyze("entity probe is end entity; architecture rtl of probe is\n"
        "begin\nreport \"prefix--a/2 b+1\";\nend architecture;\n", true);
    check(count(result, "GJB9765.4.14.c.3") == 0,
          "native raw operator scan stops at the first literal double-hyphen inside a string");
    result = analyze("entity probe is end entity; architecture rtl of probe is\n"
        "signal a, b: integer; begin\na\f+\fb;\nend architecture;\n", true);
    check(count(result, "GJB9765.4.14.c.3") == 1,
          "native operator spacing accepts only ASCII space and Tab, not form-feed");
    result = analyze("entity probe is end entity;\n" + std::string(253, 'x') + "+ x\n", true);
    check(count(result, "GJB9765.4.14.c.3") == 0,
          "native c.3 boundary guard skips a single operator at the end of a 254-byte chunk");
    result = analyze("entity probe is end entity; architecture rtl of probe is\n"
        "begin\nreport \"x+--tail\";\nend architecture;\n", true);
    check(count(result, "GJB9765.4.14.c.3") == 1,
          "native c.3 reads the comment opener as the operator right neighbor before stopping the scan");

    result = analyze("entity probe is end entity; architecture rtl of probe is\n"
        "signal q, a: bit; begin\n"
        "process begin\n"
        "if (a = '0') then q <= a; end if;\n"
        "if ( a = '0' ) then q <= a; end if;\n"
        "report \"x(y)\"; -- if (comment)\n"
        "wait; end process; end architecture;\n", true);
    check(count(result, "GJB9765.4.14.c.1") == 2 &&
          std::ranges::any_of(result.violations, [](const auto& item) {
              return item.ruleId == "GJB9765.4.14.c.1" && item.line == 4 &&
                     item.severity == analysis::Severity::Warning;
          }) && std::ranges::any_of(result.violations, [](const auto& item) {
              return item.ruleId == "GJB9765.4.14.c.1" && item.line == 6;
          }), "native raw parenthesis rule reports unspaced names in code and quoted text once per line");
    result = analyze("entity probe is end entity; architecture rtl of probe is\n"
        "signal v: bit_vector(7 downto 0); begin\n"
        "process begin\n"
        "if ( '0' = v(1) ) then report \"ok\"; end if;\n"
        "-- call(name)\nreport \"call(name)\"; wait; end process;\n"
        "end architecture;\n", true);
    check(count(result, "GJB9765.4.14.c.1") == 1 &&
          std::ranges::any_of(result.violations, [](const auto& item) {
              return item.ruleId == "GJB9765.4.14.c.1" && item.line == 6;
          }), "numeric parenthesis interiors stay clear while quoted raw text is checked");
    result = analyze("entity probe is end entity; architecture rtl of probe is\n"
        "signal bad: bit; signal q: bit_vector(1 downto 0); begin\n"
        "process begin\n"
        "if (bad = '0') then q <= (others => '0'); end if;\n"
        "q <= (others => '0'); if (bad = '0') then null; end if;\n"
        "wait; end process; end architecture;\n", true);
    check(count(result, "GJB9765.4.14.c.1") == 1 &&
          std::ranges::any_of(result.violations, [](const auto& item) {
              return item.ruleId == "GJB9765.4.14.c.1" && item.line == 5;
          }), "native later others aggregate clears an earlier line flag but a later violation sets it again");
    result = analyze("entity probe is end entity; architecture rtl of probe is\n"
        "signal a: bit; begin\nif (\fa = '0' ) then null; end if;\n"
        "report \"prefix--call(name)\";\nend architecture;\n", true);
    check(count(result, "GJB9765.4.14.c.1") == 1 &&
          std::ranges::any_of(result.violations, [](const auto& item) {
              return item.ruleId == "GJB9765.4.14.c.1" && item.line == 3;
          }), "native parenthesis scan rejects form-feed and stops at a literal double-hyphen in a string");
    result = analyze("entity probe is end entity; architecture rtl of probe is\n"
        "begin\nreport \"((1))\"; report \"()\";\nend architecture;\n", true);
    check(count(result, "GJB9765.4.14.c.1") == 1,
          "native parenthesis bitmaps admit nested same-side parentheses and digits but reject an empty pair once per line");
    result = analyze("entity probe is end entity;\n" + std::string(253, 'x') + "(name\n", true);
    check(count(result, "GJB9765.4.14.c.1") == 0,
          "native c.1 does not inspect the byte after an opening parenthesis split at a 254-byte chunk boundary");
    result = analyze("entity probe is end entity;\n" + std::string(249, 'x') + "(others\n", true);
    check(count(result, "GJB9765.4.14.c.1") == 1 &&
          std::ranges::any_of(result.violations, [](const auto& item) {
              return item.ruleId == "GJB9765.4.14.c.1" && item.line == 2;
          }), "native c.1 does not join an others exception split across chunks and uses chunk ordinal");

    result = analyzeVerilog("module m(input clk,a,b,output reg [1:0] q);\n"
        "always @(a or b) begin\n"
        "  if (a) q[0] = b; else q[0] = a;\n"
        "  q[0] = a;\n"
        "end\n"
        "always @(posedge clk) begin\n"
        "  if (a) q[1] <= b; else begin q[1] <= a; q[1] <= b; end\n"
        "end\nendmodule\n");
    check(count(result, "GJB10157.9.1.5") == 2 &&
          std::ranges::any_of(result.violations, [](const auto& item) {
              return item.ruleId == "GJB10157.9.1.5" && item.line == 4 &&
                     item.severity == analysis::Severity::Warning;
          }), "override check catches complete if/else paths and repeated same-branch writes");
    result = analyzeVerilog("module m(input clk,a,b,output reg [1:0] q);\n"
        "always @(a or b) begin\n"
        "  if (a) q[0] = b; else q[0] = a;\n"
        "  q[1] = a;\n"
        "end\n"
        "always @(posedge clk) begin if (a) q[1] <= b; else q[1] <= a; end\n"
        "endmodule\n");
    check(count(result, "GJB10157.9.1.5") == 0,
          "mutually exclusive branches and distinct bits are not overrides");
    result = analyzeVerilog("module m(input a,b,output reg q);\n"
        "always @(a or b) begin if (a) q = b; q = a; end\nendmodule\n");
    check(count(result, "GJB10157.9.1.5") == 0,
          "write in one optional branch is not a definite override");
    result = analyzeVerilog("module m(input a,b,output reg [1:0] q);\n"
        "always @(a or b) begin q[a] = b; q[a] = a; end\nendmodule\n");
    check(count(result, "GJB10157.9.1.5") == 0 &&
          std::ranges::any_of(result.standardAssessments, [](const auto& item) {
              return item.clauseId == "GJB10157.9.1.5" &&
                     item.state == analysis::StandardRuleState::ManualReviewRequired;
          }), "dynamic bit targets remain unresolved rather than falsely compliant");

    result = analyzeVerilog("module m(input a,b,clk,rst,output reg q);\n"
                            "always @(posedge rst or posedge (clk | a)) q <= 0;\n"
                            "always @(a || b) q = a & b;\nendmodule\n");
    check(count(result, "GJB10157.9.2.5") == 2 &&
          std::ranges::any_of(result.violations, [](const auto& item) {
              return item.ruleId == "GJB10157.9.2.5" && item.line == 2 &&
                     item.severity == analysis::Severity::Warning;
          }), "native sensitivity OR check reports each affected always once");
    result = analyzeVerilog("module m(input a,b,clk,rst,output reg q);\n"
                            "always @(posedge rst or posedge clk) q <= 0;\n"
                            "always @(a, b) q = a & b;\nendmodule\n");
    check(count(result, "GJB10157.9.2.5") == 0,
          "sensitivity separators and body bitwise AND are not OR violations");
    result = analyzeVerilog("module m(input a,b,output reg q);\n"
                            "// always @(a | b)\n"
                            "always @(*) q = a | b;\n"
                            "always @(a | b || a) q = a;\nendmodule\n");
    check(count(result, "GJB10157.9.2.5") == 1 &&
          std::ranges::any_of(result.standardAssessments, [](const auto& item) {
              return item.clauseId == "GJB10157.9.2.5" &&
                     item.state == analysis::StandardRuleState::Violated;
          }), "comments and body operators are ignored, two ORs in one list yield one alarm");
    result = analyze("module m(input logic a,b,clk,rst,output logic q);\n"
                     "always_ff @(posedge (clk | a) or negedge rst) q <= b;\n"
                     "always_ff @(posedge clk or negedge rst) q <= a | b;\n"
                     "endmodule\n");
    check(count(result, "GJB10157.9.2.5") == 1 &&
          std::ranges::any_of(result.violations, [](const auto& item) {
              return item.ruleId == "GJB10157.9.2.5" && item.line == 2;
          }), "always_ff is an AST_ALWAYS; event OR reports while body OR stays clear");
    result = analyzeVerilog("module m(input a,b,c,output reg q);\n"
                            "always @((a ? (b || c) : b)) q = a;\n"
                            "always @(a or b or c) q = b;\nendmodule\n");
    check(count(result, "GJB10157.9.2.5") == 1,
          "recursive event descendants report but event separators do not");
    result = analyzeVerilog("module m(input a,b,output reg q);\n"
                            "always @(a, b) q = a;\nendmodule\n");
    check(std::ranges::any_of(result.standardAssessments, [](const auto& item) {
              return item.clauseId == "GJB10157.9.2.5" &&
                     item.state == analysis::StandardRuleState::ManualReviewRequired;
          }), "absence of bounded sensitivity OR alarm does not prove native equivalence");

    result = analyze("module probe; endmodule\n");
    check(std::ranges::none_of(result.standardAssessments, [](const auto& item) {
        return item.state == analysis::StandardRuleState::Passed;
    }), "unverified native algorithms cannot produce a clean-pass assessment");
    result = analyze("entity probe is end entity; architecture rtl of probe is begin end architecture;\n", true);
    check(std::ranges::none_of(result.standardAssessments, [](const auto& item) {
        return item.state == analysis::StandardRuleState::Passed;
    }), "VHDL rules also need completed recovery before clean-pass assessment");
    result = analyze("module probe;\n//" + std::string(77, 'x') + "\nendmodule\n");
    check(count(result, "GJB10157.5.1.4") == 0,
          "native Verilog line length permits 79 physical bytes including comment markers");
    result = analyze("module probe;\n//" + std::string(78, 'x') + "\nendmodule\n");
    check(count(result, "GJB10157.5.1.4") == 1 &&
          std::ranges::any_of(result.violations, [](const auto& item) {
              return item.ruleId == "GJB10157.5.1.4" && item.line == 2 &&
                     item.column == 1 && item.severity == analysis::Severity::Error &&
                     item.message == "建议每行字符数小于80个";
          }), "native Verilog line length uses its line-only fixed error diagnostic");
    result = analyze("module probe;" + std::string(67, ' ') + "\nendmodule\n");
    check(count(result, "GJB10157.5.1.4") == 1,
          "native Verilog line length includes trailing whitespace");
    result = analyze("module probe;\n//" + std::string(79, 'x') + "\nendmodule\n");
    check(count(result, "GJB10157.5.1.4") == 1,
          "native Verilog line length flags 81 physical bytes");
    result = analyze("module alpha;\n//" + std::string(78, 'x') +
                     "\nendmodule\nmodule beta; endmodule\n");
    check(count(result, "GJB10157.5.1.4") == 2,
          "native Verilog line length rescans a shared file for each unique module key");
    result = analyze("module same; endmodule\nmodule same; endmodule\n//" +
                     std::string(78, 'x') + "\n");
    check(count(result, "GJB10157.5.1.4") == 0,
          "native Verilog line length skips a duplicate same-name module vector");
    result = analyze("//" + std::string(78, 'x') + "\n");
    check(count(result, "GJB10157.5.1.4") == 0,
          "native Verilog line length does not scan a module-free source");
    result = analyze("module probe;\n//" + std::string(78, 'x') + "\nendmodule\n", true);
    check(count(result, "GJB10157.5.1.4") == 0,
          "Verilog physical-line rule is not applied to VHDL");
    result = analyze("module probe;\n// note\nwire a;\nassign a = 1'b0;\nendmodule\n");
    check(count(result, "GJB10157.5.1.8") == 0,
          "native Verilog comment ratio permits exactly one of five physical lines");
    result = analyze("module probe;\n// note\nwire a;\nassign a = 1'b0;\nendmodule\n\n");
    check(count(result, "GJB10157.5.1.8") == 1 &&
          std::ranges::any_of(result.violations, [](const auto& item) {
              return item.ruleId == "GJB10157.5.1.8" &&
                     item.severity == analysis::Severity::Error;
          }), "native Verilog comment ratio counts blank physical lines in denominator");
    result = analyze("module probe;\nwire a; // inline note\nassign a = 1'b0;\nendmodule\n");
    check(count(result, "GJB10157.5.1.8") == 0,
          "native Verilog comment ratio counts inline comments as comment lines");
    result = analyze("module probe;\n/* start\n middle\nend */\nendmodule\n");
    check(count(result, "GJB10157.5.1.8") == 0,
          "native Verilog comment ratio counts every block-comment line");
    result = analyze("module probe;\nwire a;\nassign a = 1'b0;\nendmodule\n");
    check(count(result, "GJB10157.5.1.8") == 1 &&
          std::ranges::any_of(result.violations, [](const auto& item) {
              return item.ruleId == "GJB10157.5.1.8" &&
                     item.severity == analysis::Severity::Error &&
                     item.message == "该工程代码注释率小于20%" &&
                     item.file.empty() && item.line == 1;
          }), "native Verilog comment ratio uses the project-level fixed diagnostic");
    result = analyzeVerilogSources(
        "module first;\nendmodule\n",
        "// note\nmodule second;\nendmodule\n");
    check(count(result, "GJB10157.5.1.8") == 0,
          "native Verilog comment ratio aggregates all admitted project sources");
    result = analyzeVerilogSources(
        "module first; endmodule\nmodule extra; endmodule\nwire x;\nwire y;\n",
        "// one\n// two\nmodule second;\nendmodule\n");
    check(count(result, "GJB10157.5.1.8") == 1,
          "native Verilog comment ratio weights a source once per unique module record");
    result = analyze("module probe;\nwire a = \"// raw marker\";\nendmodule\n");
    check(count(result, "GJB10157.5.1.8") == 0,
          "native Verilog comment ratio searches raw marker bytes inside strings");
    result = analyzeMixedSources(
        "module main;\nendmodule\n",
        "-- VHDL note\nentity aux is end entity;\n");
    check(count(result, "GJB10157.5.1.8") == 0,
          "native project comment ratio includes alternate-frontend VHDL roots");
    result = analyze("module child(input a,b,c); endmodule\n"
                     "module top(input a,b,output c);\n"
                     "child u(.a(a | b), .b(a ^ b), .c(c));\nendmodule\n");
    check(count(result, "GJB10157.5.3.1") == 2 &&
          std::ranges::all_of(result.violations | std::views::filter([](const auto& item) {
              return item.ruleId == "GJB10157.5.3.1";
          }), [](const auto& item) {
              return item.line == 3 && item.column == 1 &&
                     item.severity == analysis::Severity::Warning &&
                     item.message == "端口例化时禁止使用表达式";
          }), "native port-actual rule preserves per-argument multiplicity and fixed diagnostic");
    result = analyze("module child(input a,b,c); endmodule\n"
                     "module top(input a,b,output c);\n"
                     "child u(a, ~b, c);\nendmodule\n");
    check(count(result, "GJB10157.5.3.1") == 1 &&
          std::ranges::any_of(result.violations, [](const auto& item) {
              return item.ruleId == "GJB10157.5.3.1" && item.line == 3 &&
                     item.column == 1 && item.message == "端口例化时禁止使用表达式";
          }), "native port-actual rule locates a positional unary expression by line only");
    result = analyze("module child(input a,b,c); endmodule\n"
                     "module top(input [3:0] a, output c);\n"
                     "child u(.a(a[1]), .b(1'b0), .c(c));\nendmodule\n");
    check(count(result, "GJB10157.5.3.1") == 0,
          "native port-actual rule admits identifier selections and constants");
    result = analyze("module child(input a); endmodule\n"
                     "module top(input a);\n"
                     "child u(.a(top.a));\nendmodule\n");
    check(count(result, "GJB10157.5.3.1") == 0,
          "native AST_PREFIX admission keeps a hierarchical identifier actual clear");
    result = analyze("module child(input a,b,c); endmodule\n"
                     "module top(input a,b,output c);\n"
                     "child u(.a((a)), .b(), .c(c));\nendmodule\n");
    check(count(result, "GJB10157.5.3.1") == 0,
          "native port-actual rule does not classify grouped identifiers or open ports as expressions");
    result = analyze("module child(input a,b,c); endmodule\n"
                     "module top(input a,b,output c);\n"
                     "child u(.a({a,b}), .b(a ? b : c), .c(c));\nendmodule\n");
    check(count(result, "GJB10157.5.3.1") == 2,
          "native port-actual rule diagnoses concatenation and ternary expression roots");
    result = analyze("module probe(input data_in); reg b;\n"
                     "task t2;\n#10 b = data_in + 1;\nendtask\nendmodule\n");
    check(count(result, "GJB10157.9.4.1") == 1 &&
          std::ranges::any_of(result.violations, [](const auto& item) {
              return item.ruleId == "GJB10157.9.4.1" && item.line == 3 &&
                     item.severity == analysis::Severity::Warning;
          }), "documented task statement delay reports at the delay token as Warning");
    result = analyze("module probe(input data_in); reg b;\n"
                     "task t1; b = data_in + 1; endtask\n"
                     "task t2; b = data_in + 1; endtask\nendmodule\n");
    check(count(result, "GJB10157.9.4.1") == 0 &&
          std::ranges::any_of(result.standardAssessments, [](const auto& item) {
              return item.clauseId == "GJB10157.9.4.1" &&
                     item.state == analysis::StandardRuleState::ManualReviewRequired;
          }), "documented tasks without delay remain alarm-free but not proven compliant");
    result = analyze("module probe(input data_in); reg b;\n"
                     "task t2; // #10 b = data_in;\n"
                     "b = data_in; #5 b = ~data_in;\n"
                     "endtask\ninitial #10 b = 0;\nendmodule\n");
    check(count(result, "GJB10157.9.4.1") == 1,
          "only statement delays within tasks trigger the bounded rule");
    result = analyze("module probe(input clk, input data_in); reg b;\n"
                     "task t2; @(posedge clk) b = data_in;\n"
                     "b = #2 data_in; endtask\nendmodule\n");
    check(count(result, "GJB10157.9.4.1") == 0,
          "unrecovered event controls and separately parsed intra-assignment delays are not guessed");
    result = analyze("module probe(input data_in); reg b;\n"
                     "task broken; always @* b = data_in; endtask\n"
                     "endmodule\n");
    check(count(result, "GJB10157.9.4.1") == 0,
          "illegal nested always text is not mistaken for a reachable native task descendant");
    result = analyze("`define TASK_DELAY 3\n"
                     "module probe(input data_in); reg b;\n"
                     "task t2; begin #(`TASK_DELAY) b = data_in; end endtask\n"
                     "endmodule\n");
    check(count(result, "GJB10157.9.4.1") == 1 &&
          std::ranges::any_of(result.violations, [](const auto& item) {
              return item.ruleId == "GJB10157.9.4.1" && item.line == 3;
          }), "recovered task prefix-delay predicate accepts parenthesized macro values");
    result = analyze("module probe(input data_in); reg b;\n"
                     "task t2; b = #2 data_in; b <= #3 data_in; endtask\n"
                     "endmodule\n");
    check(count(result, "GJB10157.9.4.1") == 0,
          "native prefix-delay flag is not generalized to assignment-delay productions");
    result = analyze("module probe(input a); reg global_sig;\n"
                     "function [1:0] f; input a; reg local_sig; begin\n"
                     "f = 0; local_sig = 0; global_sig = a; global_sig = 1;\n"
                     "end endfunction endmodule\n");
    check(count(result, "GJB10157.9.4.4") == 2 &&
          std::ranges::all_of(result.violations | std::views::filter([](const auto& item) {
              return item.ruleId == "GJB10157.9.4.4";
          }), [](const auto& item) { return item.line == 3 &&
              item.severity == analysis::Severity::Warning &&
              item.message == "在function语句中，禁止对全局信号global_sig进行赋值"; }),
          "module reg blocking writes in a function use the native per-assignment diagnostic");
    result = analyze("module probe; reg global_sig;\n"
                     "task t; global_sig = 1; endtask\nendmodule\n");
    check(count(result, "GJB10157.9.4.4") == 1 &&
          std::ranges::any_of(result.violations, [](const auto& item) {
              return item.ruleId == "GJB10157.9.4.4" &&
                     item.severity == analysis::Severity::Warning &&
                     item.message == "在task语句中，禁止对全局信号global_sig进行赋值";
          }), "module reg blocking write in a task uses the native task diagnostic");
    result = analyze("module probe; reg global_sig;\n"
                     "function f; reg global_sig; global_sig = 1; f = 0; endfunction\n"
                     "task t; reg global_sig; global_sig = 1; endtask\n"
                     "global_sig = 0; endmodule\n");
    check(count(result, "GJB10157.9.4.4") == 0 &&
          std::ranges::any_of(result.standardAssessments, [](const auto& item) {
              return item.clauseId == "GJB10157.9.4.4" &&
                     item.state == analysis::StandardRuleState::ManualReviewRequired;
          }), "local shadows, return assignment and module writes are excluded; no alarm remains review");
    result = analyze("module first; reg global_sig; endmodule\n"
                     "module second; function f; global_sig = 1; endfunction endmodule\n");
    check(count(result, "GJB10157.9.4.4") == 0,
          "global reg candidates do not cross module scopes");
    result = analyze("module probe; reg [1:0] global_sig;\n"
                     "function f; global_sig <= 1; global_sig[0] = 0; endfunction\nendmodule\n");
    check(count(result, "GJB10157.9.4.4") == 1,
          "nonblocking is excluded while a selected blocking target retains its first-child name");
    result = analyze("module probe; reg [1:0] a = {1'b0,1'b1};\n"
                     "function f; a = 1; f = 0; endfunction endmodule\n");
    check(count(result, "GJB10157.9.4.4") == 1,
          "initialized module reg declarations retain the native register candidate flag");
    result = analyze("module probe; reg global_sig;\n"
                     "function f; reg global_sig = 0; global_sig = 1; f = 0; endfunction\nendmodule\n");
    check(count(result, "GJB10157.9.4.4") == 0,
          "initialized function-local reg shadows the module candidate");
    result = analyze("module probe; reg global_sig;\n"
                     "function f1; reg global_sig; global_sig = 0; f1 = 0; endfunction\n"
                     "function f2; global_sig = 1; f2 = 0; endfunction\n"
                     "task t; global_sig = 1; endtask\nendmodule\n");
    check(count(result, "GJB10157.9.4.4") == 1 &&
          std::ranges::any_of(result.violations, [](const auto& item) {
              return item.ruleId == "GJB10157.9.4.4" && item.line == 4 &&
                     item.message.starts_with("在task语句中");
          }), "function child-name removal is cumulative across functions but separate from tasks");
    result = analyze("module probe(output reg global_sig);\n"
                     "task t; global_sig = 1; endtask\nendmodule\n");
    check(count(result, "GJB10157.9.4.4") == 1,
          "ANSI output reg is a native module WIRE candidate with the register flag");
    result = analyze("module probe(input [3:0] a,b);\n"
                     "function sum; input [3:0] a,b; begin sum = a + b; end endfunction\n"
                     "endmodule\n");
    check(count(result, "GJB10157.9.4.5") == 1 &&
          std::ranges::any_of(result.violations, [](const auto& item) {
              return item.ruleId == "GJB10157.9.4.5" && item.line == 2 &&
                     item.severity == analysis::Severity::Warning &&
                     item.message ==
                         "在function语句中，返回值的位宽1与输入信号的位宽4不一致";
          }), "documented function return narrowing uses the exact native Warning text");
    result = analyze("module probe(input [3:0] a,b);\n"
                     "function [3:0] sum; input [3:0] a,b; begin sum = a + b; end endfunction\n"
                     "endmodule\n");
    check(count(result, "GJB10157.9.4.5") == 0 &&
          std::ranges::any_of(result.standardAssessments, [](const auto& item) {
              return item.clauseId == "GJB10157.9.4.5" &&
                     item.state == analysis::StandardRuleState::ManualReviewRequired;
          }), "matching function return/input widths are alarm-free but not fully proven");
    result = analyze("module probe;\n"
                     "function f; input [7:0] a; begin f = a; end endfunction\n"
                     "function [7:0] g; input [7:0] a; begin g = a; end endfunction\n"
                     "endmodule\n");
    check(count(result, "GJB10157.9.4.5") == 1,
          "single known input width detects only the narrowed return");
    result = analyze("module probe;\n"
                     "function [3:0] f; input [3:0] a; input [7:0] b; begin\n"
                     "f = (a + b) - a; f <= b / a; end endfunction\nendmodule\n");
    check(count(result, "GJB10157.9.4.5") == 2 &&
          std::ranges::all_of(result.violations | std::views::filter([](const auto& item) {
              return item.ruleId == "GJB10157.9.4.5";
          }), [](const auto& item) {
              return item.line == 3 && item.severity == analysis::Severity::Warning &&
                     item.message ==
                         "在function语句中，返回值的位宽4与输入信号的位宽8不一致";
          }), "9.4.5 preserves per-assignment multiplicity for nested arithmetic and nonblocking returns");
    result = analyze("module probe;\n"
                     "function f; input [7:0] a; begin f = a[3]; end endfunction\n"
                     "function [3:0] g; input [7:0] a; begin g = a[5:2]; end endfunction\n"
                     "endmodule\n");
    check(count(result, "GJB10157.9.4.5") == 0,
          "9.4.5 computes bounded bit-select and constant part-select widths");
    result = analyze("module probe;\n"
                     "function [7:0] f; input [3:0] a; begin f = a; end endfunction\n"
                     "function g; input [W-1:0] a; begin g = a; end endfunction\n"
                     "endmodule\n");
    check(count(result, "GJB10157.9.4.5") == 1 &&
          std::ranges::any_of(result.violations, [](const auto& item) {
              return item.ruleId == "GJB10157.9.4.5" &&
                     item.message ==
                         "在function语句中，返回值的位宽8与输入信号的位宽4不一致";
          }), "wider return uses both native widths; parameterized unknown input is not guessed");
    result = analyze("module probe;\n"
                     "function integer f; input [3:0] a; begin f = a; end endfunction\nendmodule\n");
    check(count(result, "GJB10157.9.4.5") == 0,
          "unsupported typed return is not mistaken for a one-bit function");
    result = analyze("module probe(input clk, input [4:0] a,b, output reg [4:0] c);\n"
                     "task t(input [2:0] x, input [2:0] y, output [2:0] z);\n"
                     "begin z <= x + y; end endtask\n"
                     "always @(posedge clk) begin t(a[3:0], b[2:0], c[3:0]); end\n"
                     "endmodule\n");
    check(count(result, "GJB10157.9.4.6") == 2 &&
          std::ranges::all_of(result.violations | std::views::filter([](const auto& item) {
              return item.ruleId == "GJB10157.9.4.6";
          }), [](const auto& item) { return item.line == 4 &&
              item.severity == analysis::Severity::Warning; }),
          "task positional bindings report each known slice-width mismatch");
    check(std::ranges::any_of(result.violations, [](const auto& item) {
              return item.ruleId == "GJB10157.9.4.6" &&
                     item.message ==
                         "task语句，输入信号的位宽3与输出信号的位宽4不一致，任务输入：x";
          }) && std::ranges::any_of(result.violations, [](const auto& item) {
              return item.ruleId == "GJB10157.9.4.6" &&
                     item.message ==
                         "task语句，输入信号的位宽4与输出信号的位宽3不一致，任务输出：z";
          }), "task input and output bindings preserve native child-width order and formal suffix");
    result = analyze("module probe(input clk, input [4:0] a,b, output reg [4:0] c);\n"
                     "task t(input [2:0] x, input [2:0] y, output [2:0] z);\n"
                     "begin z <= x + y; end endtask\n"
                     "always @(posedge clk) begin t(a[2:0], b[2:0], c[2:0]); end\n"
                     "endmodule\n");
    check(count(result, "GJB10157.9.4.6") == 0 &&
          std::ranges::any_of(result.standardAssessments, [](const auto& item) {
              return item.clauseId == "GJB10157.9.4.6" &&
                     item.state == analysis::StandardRuleState::ManualReviewRequired;
          }), "matching task slices are alarm-free but not fully proven");
    result = analyze("module probe(input clk, input [4:0] a);\n"
                     "task t(input [2:0] x); endtask\n"
                     "always @(posedge clk) begin t(a); end\nendmodule\n");
    check(count(result, "GJB10157.9.4.6") == 1 &&
          std::ranges::any_of(result.violations, [](const auto& item) {
              return item.ruleId == "GJB10157.9.4.6" &&
                     item.message ==
                         "task语句，输入信号的位宽3与输出信号的位宽5不一致，任务输入：x";
          }), "fixed-width bare module-port actuals use their declaration width");
    result = analyze("module probe(input clk, input [4:0] a);\n"
                     "task t(input [1:0] x); endtask\n"
                     "always @(posedge clk) begin t(a[0]); end\nendmodule\n");
    check(count(result, "GJB10157.9.4.6") == 1 &&
          std::ranges::any_of(result.violations, [](const auto& item) {
              return item.ruleId == "GJB10157.9.4.6" &&
                     item.message ==
                         "task语句，输入信号的位宽2与输出信号的位宽1不一致，任务输入：x";
          }), "numeric bit-select task actuals have width one");
    result = analyze("module probe(input clk, inout [4:0] a);\n"
                     "task t(inout [2:0] x); endtask\n"
                     "always @(posedge clk) begin t(a[3:0]); end\nendmodule\n");
    check(count(result, "GJB10157.9.4.6") == 0,
          "inout lowering direction is not guessed without recovered native producer evidence");
    result = analyze("module probe(input clk);\n"
                     "task t(input [2:0] x); endtask\n"
                     "always @(posedge clk) begin t(unknown_signal[3:0]); end\nendmodule\n");
    check(count(result, "GJB10157.9.4.6") == 0,
          "unresolved actual base is not treated as a known task binding width");
    result = analyze("module first(input clk, input [4:0] a);\n"
                     "task t(input [2:0] x); endtask\n"
                     "always @(posedge clk) begin t(a[3:0]); end endmodule\n"
                     "module second(input clk, input [4:0] a);\n"
                     "task t(input [3:0] x); endtask\n"
                     "always @(posedge clk) begin t(a[3:0]); end endmodule\n");
    check(count(result, "GJB10157.9.4.6") == 1,
          "same-name task formals are resolved within each module");
    result = analyze("module probe(input clk, input [3:0] a, input [7:0] b);\n"
                     "reg [7:0] local_bus;\n"
                     "task t;\n"
                     "input [3:0] x;\n"
                     "input [7:0] y;\n"
                     "output [3:0] z;\n"
                     "endtask\n"
                     "always @(posedge clk) t(a + b, {a,a}, local_bus);\n"
                     "endmodule\n");
    check(count(result, "GJB10157.9.4.6") == 2 &&
          std::ranges::any_of(result.violations, [](const auto& item) {
              return item.ruleId == "GJB10157.9.4.6" && item.line == 8 &&
                     item.message ==
                         "task语句，输入信号的位宽4与输出信号的位宽8不一致，任务输入：x";
          }) && std::ranges::any_of(result.violations, [](const auto& item) {
              return item.ruleId == "GJB10157.9.4.6" && item.line == 8 &&
                     item.message ==
                         "task语句，输入信号的位宽8与输出信号的位宽4不一致，任务输出：z";
          }), "9.4.6 legacy task bindings use arithmetic, concat and internal-signal widths");
    result = analyze("module probe(input clk, input [3:0] a);\n"
                     "task t(input [3:0] x, input [7:0] y); endtask\n"
                     "always @(posedge clk) t(8'hff, {2{a}});\n"
                     "endmodule\n");
    check(count(result, "GJB10157.9.4.6") == 1 &&
          std::ranges::any_of(result.violations, [](const auto& item) {
              return item.ruleId == "GJB10157.9.4.6" &&
                     item.message ==
                         "task语句，输入信号的位宽4与输出信号的位宽8不一致，任务输入：x";
          }), "9.4.6 sized literals and static replication retain recovered widths");
    result = analyze("module probe(clk, a);\n"
                     "input clk; input [7:0] a;\n"
                     "task t; input [3:0] x; endtask\n"
                     "always @(posedge clk) t(a);\n"
                     "endmodule\n");
    check(count(result, "GJB10157.9.4.6") == 1 &&
          std::ranges::any_of(result.violations, [](const auto& item) {
              return item.ruleId == "GJB10157.9.4.6" &&
                     item.message ==
                         "task语句，输入信号的位宽4与输出信号的位宽8不一致，任务输入：x";
          }), "9.4.6 legacy module declarations provide fixed bare-actual widths");
    result = analyze("module probe(input [2:0] data, input sel, output wire [2:0] temp1);\n"
                     "assign temp1[0] = data[0] & temp1[2];\n"
                     "assign temp1[1] = temp1[0] ^ data[1];\n"
                     "assign temp1[2] = sel ? data[0] : temp1[1];\nendmodule\n");
    check(count(result, "GJB10157.9.4.7") == 1 &&
          std::ranges::any_of(result.violations, [](const auto& item) {
              return item.ruleId == "GJB10157.9.4.7" && item.signal == "temp1" &&
                     item.severity == analysis::Severity::Warning &&
                     item.message == "检测到组合回路temp1";
          }), "three constant-selected bits forming a cycle reproduce the native warning surface");
    result = analyze("module probe(input [2:0] data, input sel, output wire [2:0] temp1);\n"
                     "assign temp1[0] = data[0];\n"
                     "assign temp1[1] = temp1[0] ^ data[1];\n"
                     "assign temp1[2] = sel ? data[0] : temp1[1];\nendmodule\n");
    check(count(result, "GJB10157.9.4.7") == 0 &&
          std::ranges::any_of(result.standardAssessments, [](const auto& item) {
              return item.clauseId == "GJB10157.9.4.7" &&
                     item.state == analysis::StandardRuleState::ManualReviewRequired;
          }), "acyclic bit chain has no false feedback alarm but still requires review");
    result = analyze("module probe(output wire [2:0] temp1);\n"
                     "assign temp1[0] = temp1[2];\nendmodule\n");
    check(count(result, "GJB10157.9.4.7") == 0,
          "one-way transfer between two constant bit selects is not a feedback loop");
    result = analyze("module probe(input [3:1] data, output wire a,b);\n"
                     "assign a = data[-1];\n"
                     "assign b = data[4];\nendmodule\n");
    check(count(result, "GJB10157.9.4.8") == 2,
          "array bounds rule reports both negative and high constant indices");
    check(std::ranges::all_of(result.violations, [](const auto& item) {
              return item.ruleId != "GJB10157.9.4.8" ||
                     (item.severity == analysis::Severity::Warning &&
                      item.message == "信号data索引超出指定的数组范围");
          }), "array bounds findings reproduce the native warning surface");
    result = analyze("module probe(input [3:1] data, output wire a,b);\n"
                     "assign a = data[1];\n"
                     "assign b = data[3];\nendmodule\n");
    check(count(result, "GJB10157.9.4.8") == 0 &&
          std::ranges::any_of(result.standardAssessments, [](const auto& item) {
              return item.clauseId == "GJB10157.9.4.8" &&
                     item.state == analysis::StandardRuleState::ManualReviewRequired;
          }), "array bounds endpoints are permitted but no alarm is not native-equivalent proof");
    result = analyze("module probe(input [3:1] data, output wire bad, good);\n"
                     "localparam integer BAD_INDEX = 4;\n"
                     "localparam integer GOOD_INDEX = 3;\n"
                     "assign bad = data[BAD_INDEX];\n"
                     "assign good = data[GOOD_INDEX];\nendmodule\n");
    check(count(result, "GJB10157.9.4.8") == 1 &&
          std::ranges::any_of(result.violations, [](const auto& item) {
              return item.ruleId == "GJB10157.9.4.8" && item.line == 4;
          }), "known module localparam value is checked against array bounds");
    result = analyze("module probe #(parameter IDX = 4)(input [3:1] data, output wire q);\n"
                     "assign q = data[IDX];\nendmodule\n");
    check(count(result, "GJB10157.9.4.8") == 0 &&
          std::ranges::any_of(result.standardAssessments, [](const auto& item) {
              return item.clauseId == "GJB10157.9.4.8" &&
                     item.state == analysis::StandardRuleState::ManualReviewRequired;
          }), "overridable parameter default cannot prove an elaborated index violation");
    result = analyze("module probe(output wire [7:0] q);\n"
                     "reg [7:0] mem [1:3][4:6];\n"
                     "assign q = mem[0][4][7];\nendmodule\n");
    check(count(result, "GJB10157.9.4.8") == 1 &&
          std::ranges::any_of(result.violations, [](const auto& item) {
              return item.ruleId == "GJB10157.9.4.8" && item.signal == "mem" &&
                     item.line == 3 && item.severity == analysis::Severity::Warning;
          }), "first unpacked dimension is checked without confusing later legal dimensions");
    result = analyze("module probe(output wire [7:0] q);\n"
                     "reg [7:0] mem [1:3][4:6];\n"
                     "assign q = mem[1][7][8];\nendmodule\n");
    check(count(result, "GJB10157.9.4.8") == 2,
          "second unpacked and final packed dimensions are checked independently");
    result = analyze("module probe(output wire [7:0] q);\n"
                     "reg [7:0] mem [1:3][4:6];\n"
                     "assign q = mem[3][6][0];\nendmodule\n");
    check(count(result, "GJB10157.9.4.8") == 0,
          "all inclusive endpoints of unpacked and packed dimensions remain legal");
    result = analyze("module probe(input [1:0] idx, input d, output reg [2:0] data);\n"
                     "always @* data[idx] = d;\nendmodule\n");
    check(count(result, "GJB10157.9.4.8") == 1 &&
          std::ranges::any_of(result.violations, [](const auto& item) {
              return item.ruleId == "GJB10157.9.4.8" && item.signal == "data" &&
                     item.line == 2 && item.message ==
                         "信号data索引超出指定的数组范围";
          }), "dynamic direct-write index warns when 2^index-width exceeds signal width");
    result = analyze("module probe(input [1:0] idx, input d, output reg [3:0] data);\n"
                     "always @* data[idx] <= d;\nendmodule\n");
    check(count(result, "GJB10157.9.4.8") == 0,
          "dynamic direct-write index capacity equal to signal width remains legal");
    result = analyze("module probe(input [1:0] idx, input [2:0] data, output wire q);\n"
                     "assign q = data[idx];\nendmodule\n");
    check(count(result, "GJB10157.9.4.8") == 0 &&
          std::ranges::any_of(result.standardAssessments, [](const auto& item) {
              return item.clauseId == "GJB10157.9.4.8" &&
                     item.state == analysis::StandardRuleState::ManualReviewRequired;
          }), "recovered is_over_array_index producer is not generalized from writes to reads");
    result = analyzeVerilog("module probe(input A, B, output reg C, output reg Q);\n"
                            "always @(A, CLK) C = A;\n"
                            "always @(A) if (B_COND) C = A & B;\n"
                            "always @(B_COND) C = A;\n"
                            "always @(posedge CLK) Q <= A;\nendmodule\n");
    check(count(result, "GJB10157.9.4.9") == 5 &&
          std::ranges::any_of(result.violations, [](const auto& item) {
              return item.ruleId == "GJB10157.9.4.9" && item.line == 2 &&
                     item.signal == "CLK" &&
                     item.message == "组合always敏感列表中信号CLK是冗余的";
          }) && std::ranges::any_of(result.violations, [](const auto& item) {
              return item.ruleId == "GJB10157.9.4.9" && item.line == 3 &&
                     item.signal == "B_COND" &&
                     item.message == "组合always块的敏感列表不完整，缺少信号B_COND";
          }) && std::ranges::count_if(result.violations, [](const auto& item) {
              return item.ruleId == "GJB10157.9.4.9" && item.line == 3;
          }) == 2, "plain Verilog relation analysis retains implicit nets and reports each missing name independently");
    result = analyzeVerilog("module probe(input A, B, output reg C, output reg Q);\n"
                            "always @(*) C = A & B;\n"
                            "always @(posedge A) Q <= B;\nendmodule\n");
    check(count(result, "GJB10157.9.4.9") == 0 &&
          std::ranges::any_of(result.standardAssessments, [](const auto& item) {
              return item.clauseId == "GJB10157.9.4.9" &&
                     item.state == analysis::StandardRuleState::ManualReviewRequired;
          }), "automatic and edge sensitivity do not emit this bounded combinational rule");
    result = analyze("module probe; reg q;\ninitial q = 1'b0;\nendmodule\n");
    check(count(result, "GJB10157.9.4.2") == 1 &&
          std::ranges::any_of(result.violations, [](const auto& item) {
              return item.ruleId == "GJB10157.9.4.2" && item.line == 2 &&
                     item.severity == analysis::Severity::Warning &&
                     item.message == "禁止使用initial语句";
          }), "native AST_INITIAL rule reports an explicit initial statement as Warning");
    result = analyze("module probe; reg q;\ninitial q = 1'b0;\n"
                     "initial q = 1'b1;\nendmodule\n");
    check(count(result, "GJB10157.9.4.2") == 2,
          "native AST_INITIAL rule reports each explicit initial statement");
    result = analyze("module probe; reg q;\n"
                     "(* keep = \"true\" *) initial begin : boot q = 1'b0; end\n"
                     "initial fork q = 1'b1; join\nendmodule\n");
    check(count(result, "GJB10157.9.4.2") == 2 &&
          std::ranges::any_of(result.violations, [](const auto& item) {
              return item.ruleId == "GJB10157.9.4.2" && item.line == 2;
          }) && std::ranges::any_of(result.violations, [](const auto& item) {
              return item.ruleId == "GJB10157.9.4.2" && item.line == 3;
          }), "attribute, named block and fork forms retain one finding per explicit initial root");
    result = analyze("module probe; reg q = 1'b0;\n"
                     "// initial q = 1'b1;\n"
                     "initial $display(\"initial\");\nendmodule\n");
    check(count(result, "GJB10157.9.4.2") == 1,
          "declaration initialization, comments and strings are not AST_INITIAL statements");
    result = analyze("initial orphan = 1'b0;\n"
                     "module probe; reg q;\n"
                     "task broken; initial q = 1'b0; endtask\n"
                     "function automatic f; initial q = 1'b1; endfunction\n"
                     "endmodule\n");
    check(count(result, "GJB10157.9.4.2") == 0,
          "tokens outside a design unit or inside task/function syntax do not form native AST_INITIAL roots");
    result = analyze("module probe; reg q;\n"
                     "generate if (1) begin : g initial q = 1'b0; end endgenerate\n"
                     "endmodule\n");
    check(count(result, "GJB10157.9.4.2") == 1,
          "module-item initial below generate remains reachable by the native recursive selector");
    result = analyze("module probe; reg q = 1'b0; endmodule\n");
    check(count(result, "GJB10157.9.4.2") == 0 &&
          std::ranges::any_of(result.standardAssessments, [](const auto& item) {
              return item.clauseId == "GJB10157.9.4.2" &&
                     item.state == analysis::StandardRuleState::ManualReviewRequired;
          }), "no explicit initial is not proof of native algorithm equivalence");
    analysis::AnalysisOptions conditionalSubprogramOptions;
    conditionalSubprogramOptions.standardId = "GJB10157-2021";
    conditionalSubprogramOptions.defines.push_back("USE_BAD_SUBPROGRAMS");
    const std::string conditionalSubprograms =
        "module probe(input [4:0] a,b,input data_in); reg global_sig;\n"
        "`ifdef USE_BAD_SUBPROGRAMS\n"
        "task delayed; begin #10; end endtask\n"
        "initial global_sig=0;\n"
        "task writes_global; global_sig=1; endtask\n"
        "function narrow; input [3:0] a; begin narrow=a; end endfunction\n"
        "task width_task(input [2:0] x); endtask\n"
        "always @* width_task(a[3:0]);\n"
        "`else\nwire safe_signal;\n`endif\nendmodule\n";
    result = analysis::Analyzer(conditionalSubprogramOptions).analyzeSource(
        {"conditional-subprograms.sv", core::HdlLanguage::SystemVerilog,
         conditionalSubprograms});
    for (const auto* rule : {"GJB10157.9.4.1", "GJB10157.9.4.2",
                             "GJB10157.9.4.4", "GJB10157.9.4.5",
                             "GJB10157.9.4.6"})
        check(count(result, rule) == 1,
              "configured active branch reaches each recovered subprogram predicate");
    check(std::ranges::any_of(result.violations, [](const auto& item) {
        return item.ruleId == "GJB10157.9.4.1" && item.line == 3;
    }) && std::ranges::any_of(result.violations, [](const auto& item) {
        return item.ruleId == "GJB10157.9.4.2" && item.line == 4;
    }) && std::ranges::any_of(result.violations, [](const auto& item) {
        return item.ruleId == "GJB10157.9.4.4" && item.line == 5;
    }) && std::ranges::any_of(result.violations, [](const auto& item) {
        return item.ruleId == "GJB10157.9.4.5" && item.line == 6;
    }) && std::ranges::any_of(result.violations, [](const auto& item) {
        return item.ruleId == "GJB10157.9.4.6" && item.line == 8;
    }), "active subprogram findings retain their original physical source lines");
    conditionalSubprogramOptions.defines.clear();
    result = analysis::Analyzer(conditionalSubprogramOptions).analyzeSource(
        {"conditional-subprograms.sv", core::HdlLanguage::SystemVerilog,
         conditionalSubprograms});
    for (const auto* rule : {"GJB10157.9.4.1", "GJB10157.9.4.2",
                             "GJB10157.9.4.4", "GJB10157.9.4.5",
                             "GJB10157.9.4.6"})
        check(count(result, rule) == 0,
              "inactive branch cannot trigger a recovered subprogram predicate");
    result = analyze("module probe; endmodule\nmodule probe; endmodule\n");
    check(count(result, "GJB10157.5.2.5") == 1 &&
          std::ranges::any_of(result.violations, [](const auto& item) {
              return item.ruleId == "GJB10157.5.2.5" &&
                     item.severity == analysis::Severity::Warning &&
                     item.line == 1 && item.column == 1 &&
                     item.message == "在源文件中找到多个模块:probe,probe.";
          }), "same-name module entries preserve multiplicity and the exact native warning surface");
    result = analyze("module first; endmodule\nmodule second; endmodule\n");
    check(count(result, "GJB10157.5.2.5") == 1 &&
          std::ranges::any_of(result.violations, [](const auto& item) {
              return item.ruleId == "GJB10157.5.2.5" &&
                     item.message == "在源文件中找到多个模块:first,second.";
          }), "different-name module entries produce one aggregate source-group diagnostic");
    result = analyze("module first; endmodule\nmodule second; endmodule\nmodule third; endmodule\n");
    check(count(result, "GJB10157.5.2.5") == 1 &&
          std::ranges::any_of(result.violations, [](const auto& item) {
              return item.ruleId == "GJB10157.5.2.5" &&
                     item.message == "在源文件中找到多个模块:first,second,third.";
          }), "three source modules remain one diagnostic and retain every entry name");
    result = analyze("// source note\n\n  module later; endmodule\n\nmodule second; endmodule\n");
    check(std::ranges::any_of(result.violations, [](const auto& item) {
        return item.ruleId == "GJB10157.5.2.5" && item.line == 3 && item.column == 1;
    }), "multi-module violation uses the earliest physical module line and no native column");
    result = analyzeVerilogSources("module first; endmodule\n",
                                   "module second; endmodule\n");
    check(count(result, "GJB10157.5.2.5") == 0,
          "one module in each of two files does not form a multi-module source group");
    result = analyze("module probe; endmodule\n");
    check(count(result, "GJB10157.5.2.5") == 0 &&
          std::ranges::any_of(result.standardAssessments, [](const auto& item) {
              return item.clauseId == "GJB10157.5.2.5" &&
                     item.state == analysis::StandardRuleState::ManualReviewRequired;
          }), "one module has no alarm but unknown native exclusions remain under review");

    const auto namedFileAnalyze = [](std::string filename, std::string text) {
        analysis::AnalysisOptions options;
        options.standardId = "gjb10157";
        return analysis::Analyzer(options).analyzeSource({std::move(filename),
            core::HdlLanguage::Verilog, std::move(text)});
    };
    result = namedFileAnalyze("rtl/utest_140002_ecode1.v", "module dummy; endmodule\n");
    check(count(result, "GJB10157.5.2.11") == 1,
          "native module-file rule diagnoses a mismatched basename");
    check(std::ranges::any_of(result.violations, [](const auto& item) {
        return item.ruleId == "GJB10157.5.2.11" &&
               item.severity == analysis::Severity::Warning &&
               item.line == 1 && item.column == 1 &&
               item.message == "模块名dummy与文件名utest_140002_ecode1不匹配.";
    }), "native module-file diagnostic preserves exact message, Warning and line-only location");
    result = namedFileAnalyze("rtl/utest_GJB_A_2_1_ccode1.v",
                              "module utest_GJB_A_2_1_ccode1; endmodule\n");
    check(count(result, "GJB10157.5.2.11") == 0,
          "packaged correct example matches its parser-comment basename");
    check(std::ranges::any_of(result.standardAssessments, [](const auto& item) {
        return item.clauseId == "GJB10157.5.2.11" &&
               item.state == analysis::StandardRuleState::ManualReviewRequired;
    }), "matching basename does not certify the unrecovered full rule");
    result = namedFileAnalyze("rtl/probe.v", "module Probe; endmodule\n");
    check(count(result, "GJB10157.5.2.11") == 1,
          "native bytewise name comparison is case-sensitive");
    result = namedFileAnalyze("rtl/probe.v", "module probe; endmodule\nmodule other; endmodule\n");
    check(count(result, "GJB10157.5.2.11") == 1,
          "each source module is compared with the same file basename");
    result = namedFileAnalyze("rtl\\probe.v", "module probe; endmodule\n");
    check(count(result, "GJB10157.5.2.11") == 0,
          "native module-file rule normalizes Windows separators before extracting the basename");
    result = namedFileAnalyze("rtl/probe.generated.sv",
                              "module \\probe.generated ; endmodule\n");
    check(count(result, "GJB10157.5.2.11") == 0,
          "native module-file rule uses the final dot for a multi-dot filename");
    result = namedFileAnalyze("rtl/probe", "module probe; endmodule\n");
    check(count(result, "GJB10157.5.2.11") == 0,
          "native module-file rule treats an extensionless filename as its full basename");
    result = namedFileAnalyze("rtl/.probe", "module probe; endmodule\n");
    check(count(result, "GJB10157.5.2.11") == 1 &&
          std::ranges::any_of(result.violations, [](const auto& item) {
              return item.ruleId == "GJB10157.5.2.11" &&
                     item.message == "模块名probe与文件名不匹配.";
          }), "native last-dot extraction gives a leading-dot filename an empty stem");
    result = namedFileAnalyze("rtl/later.v", "// note\n\n  module wrong; endmodule\n");
    check(std::ranges::any_of(result.violations, [](const auto& item) {
        return item.ruleId == "GJB10157.5.2.11" && item.line == 3 && item.column == 1;
    }), "native module-file location uses the AST physical line and no source column");

    const auto checkClockName = [&](const char* name, bool expectViolation) {
        const auto source = std::string("module probe(input ") + name +
            ", output reg q); always @(posedge " + name + ") q <= 1'b1; endmodule\n";
        const auto clockResult = analyze(source);
        check((count(clockResult, "GJB10157.5.2.13") != 0) == expectViolation,
              "native clock-name regex boundary");
        if (expectViolation)
            check(std::ranges::any_of(clockResult.violations, [](const auto& item) {
                return item.ruleId == "GJB10157.5.2.13" &&
                       item.severity == analysis::Severity::Error;
            }), "native clock-name diagnostic uses error severity");
    };
    for (const auto* name : {"clk_100m", "clk_0", "clk_foo_", "Clk_100M", "Clk_X"})
        checkClockName(name, false);
    for (const auto* name : {"clk", "clk1", "clk_", "clk_A", "Clk_foo", "CLK_100M", "my_clk_100m"})
        checkClockName(name, true);
    result = analyze(
        "module probe(input bad_clk, output reg q);\n"
        "always @(posedge bad_clk) q <= 1'b1;\n"
        "always @(negedge bad_clk) q <= 1'b0;\n"
        "endmodule\n");
    check(count(result, "GJB10157.5.2.13") == 1,
          "native clock-name rule deduplicates one normalized name across edge polarities");
    check(std::ranges::any_of(result.violations, [](const auto& item) {
        return item.ruleId == "GJB10157.5.2.13" &&
               item.severity == analysis::Severity::Error &&
               item.line == 1 && item.column == 1 &&
               item.message ==
                   "时钟信号bad_clk命名应采用“clk_x”或者“Clk_X”,"
                   "其中“x”代表特定含义的时钟频率";
    }), "native clock-name diagnostic preserves exact text, Error and declaration line");
    result = analyze(
        "module probe(input data, output reg q);\n"
        "reg bad_internal_clock;\n"
        "always @(posedge bad_internal_clock) q <= data;\n"
        "endmodule\n");
    check(count(result, "GJB10157.5.2.13") == 1 &&
          std::ranges::any_of(result.violations, [](const auto& item) {
              return item.ruleId == "GJB10157.5.2.13" &&
                     item.line == 2 && item.column == 1 &&
                     item.message.starts_with("时钟信号bad_internal_clock命名应采用");
          }), "native clock-name lookup reports an internal wire-like declaration line");
    result = analyze(
        "module probe(output reg q);\n"
        "always @(posedge implicit_bad_clock) q <= 1'b1;\n"
        "endmodule\n");
    check(count(result, "GJB10157.5.2.13") == 0,
          "clock-name adapter does not invent an AST_WIRE lookup for an implicit net");
    result = analyze(
        "module first(input bad_clock, output reg q);\n"
        "always @(posedge bad_clock) q <= 1'b1;\n"
        "endmodule\n"
        "module second;\n"
        "reg bad_clock;\n"
        "always @(negedge bad_clock) bad_clock <= 1'b0;\n"
        "endmodule\n");
    check(count(result, "GJB10157.5.2.13") == 2 &&
          std::ranges::any_of(result.violations, [](const auto& item) {
              return item.ruleId == "GJB10157.5.2.13" && item.line == 1;
          }) &&
          std::ranges::any_of(result.violations, [](const auto& item) {
              return item.ruleId == "GJB10157.5.2.13" && item.line == 5;
          }), "native clock-name deduplication is scoped independently to each module");
    result = analyze(
        "module probe(input \\clk_100m , output reg q);\n"
        "always @(posedge \\clk_100m ) q <= 1'b1;\n"
        "endmodule\n");
    check(count(result, "GJB10157.5.2.13") == 0,
          "Verilog escaped clock identifier is normalized before native regex matching");
    result = analyze(
        "module probe(input \\bad_clk , output reg q);\n"
        "always @(posedge \\bad_clk ) q <= 1'b1;\n"
        "endmodule\n");
    check(count(result, "GJB10157.5.2.13") == 1 &&
          std::ranges::any_of(result.violations, [](const auto& item) {
              return item.ruleId == "GJB10157.5.2.13" &&
                     item.message.starts_with("时钟信号bad_clk命名应采用");
          }), "escaped invalid clock reports the normalized name without a backslash");

    const auto checkResetName = [&](const char* name, bool expectViolation) {
        const auto source = std::string("module probe(input clk_1, input ") + name +
            ", output reg q); always @(posedge clk_1 or negedge " + name +
            ") begin if (!" + name + ") q <= 1'b0; else q <= 1'b1; end endmodule\n";
        const auto resetResult = analyze(source);
        check((count(resetResult, "GJB10157.5.2.14") != 0) == expectViolation,
              "native reset-name prefix and polarity-marker boundary");
        if (expectViolation)
            check(std::ranges::any_of(resetResult.violations, [](const auto& item) {
                return item.ruleId == "GJB10157.5.2.14" &&
                       item.severity == analysis::Severity::Error &&
                       item.column == 1 && item.message.starts_with("复位信号") &&
                       item.message.ends_with(
                           "命名应采用“RST_X”、“Reset_X”、“rst_x”或者“reset_x”，"
                           "其中“x”为“p”时代表高电平复位，“x”为“n”时代表低电平复位");
            }), "native reset-name diagnostic preserves Error, fixed message and line-only location");
    };
    for (const auto* name : {"rst_n", "rst_p", "reset_n", "reset_p", "RST_N", "Reset_P",
                             "rst_n_async", "reset_p_ctrl"})
        checkResetName(name, false);
    for (const auto* name : {"rst", "reset", "rst2", "my_rst_n", "Rst_n", "rst_N",
                             "RST_n", "rst_nx", "reset_a", "rst_nx_n", "RST_NX_N"})
        checkResetName(name, true);
    result = analyze("module probe(input clk_1, input bad_reset, output reg q); "
                     "always @(posedge clk_1 or negedge bad_reset) begin "
                     "if (!bad_reset) q <= 1'b0; else q <= 1'b1; end endmodule\n");
    check(std::ranges::any_of(result.violations, [](const auto& item) {
        return item.ruleId == "GJB10157.5.2.14" &&
               item.message ==
                   "复位信号bad_reset命名应采用“RST_X”、“Reset_X”、“rst_x”或者“reset_x”，"
                   "其中“x”为“p”时代表高电平复位，“x”为“n”时代表低电平复位";
    }), "reset-name report embeds the normalized candidate between native prefix and suffix");

    result = analyze("module probe; child u(); defparam u.W = 4; endmodule\n");
    check(count(result, "GJB10157.5.2.16") == 1,
          "native AST_DEFPARAM core predicate catches an instance override");
    check(std::ranges::any_of(result.violations, [](const auto& item) {
        return item.ruleId == "GJB10157.5.2.16" &&
               item.severity == analysis::Severity::Error && item.line == 1 &&
               item.column == 1 && item.message == "禁止使用defparam语句";
    }), "native defparam report uses exact message, error severity and line-only location");
    result = analyze("module probe; child a(); child b(); defparam a.W = 4; defparam b.W = 8; endmodule\n");
    check(count(result, "GJB10157.5.2.16") == 2,
          "separate AST_DEFPARAM nodes yield separate reports");
    result = analyze("module probe; child #(.W(4)) u(); // defparam u.W = 8;\n"
                     "initial $display(\"defparam\"); endmodule\n");
    check(count(result, "GJB10157.5.2.16") == 0,
          "named overrides, comments and string literals do not create AST_DEFPARAM nodes");
    analysis::AnalysisOptions defparamMacroOptions;
    defparamMacroOptions.standardId = "gjb10157";
    defparamMacroOptions.defines.push_back("USE_DEFPARAM");
    const std::string conditionalDefparam =
        "module probe; child u();\n`ifdef USE_DEFPARAM\n"
        "defparam u.W = 4;\n`endif\nendmodule\n";
    result = analysis::Analyzer(defparamMacroOptions).analyzeSource(
        {"conditional-defparam.sv", core::HdlLanguage::SystemVerilog,
         conditionalDefparam});
    check(count(result, "GJB10157.5.2.16") == 1 &&
          std::ranges::any_of(result.violations, [](const auto& item) {
              return item.ruleId == "GJB10157.5.2.16" && item.line == 3;
          }), "active conditional branch produces one AST_DEFPARAM candidate");
    defparamMacroOptions.defines.clear();
    result = analysis::Analyzer(defparamMacroOptions).analyzeSource(
        {"conditional-defparam.sv", core::HdlLanguage::SystemVerilog,
         conditionalDefparam});
    check(count(result, "GJB10157.5.2.16") == 0,
          "inactive conditional branch does not produce AST_DEFPARAM");
    check(std::ranges::any_of(result.standardAssessments, [](const auto& item) {
        return item.clauseId == "GJB10157.5.2.16" &&
               item.state == analysis::StandardRuleState::ManualReviewRequired;
    }), "no defparam finding does not certify full native equivalence");

    analysis::AnalysisOptions parameterReuseOptions;
    parameterReuseOptions.standardId = "gjb10157";
    const auto checkParameterReuse = [&](std::vector<core::SourceFile> sources) {
        return analysis::Analyzer(parameterReuseOptions).analyzeSources(sources);
    };
    result = checkParameterReuse({
        {"first.sv", core::HdlLanguage::SystemVerilog,
         "module first #(parameter WIDTH = 8)(); endmodule\n"},
        {"second.sv", core::HdlLanguage::SystemVerilog,
         "module second #(parameter WIDTH = 16)(); endmodule\n"}});
    check(count(result, "GJB10157.5.2.17") == 1,
          "native global AST_PARAMETER name map detects cross-file reuse");
    check(std::ranges::any_of(result.violations, [](const auto& item) {
        return item.ruleId == "GJB10157.5.2.17" &&
               item.file.filename() == "second.sv" &&
               item.line == 1 && item.column == 1 &&
               item.message == "参数WIDTH有first.WIDTH同名" &&
               item.severity == analysis::Severity::Error;
    }), "later parameter occurrence has native message, line-only location and error severity");
    result = checkParameterReuse({
        {"zed.sv", core::HdlLanguage::SystemVerilog,
         "module zed #(parameter WIDTH = 8)(); endmodule\n"},
        {"alpha.sv", core::HdlLanguage::SystemVerilog,
         "module alpha #(parameter WIDTH = 16)(); endmodule\n"}});
    check(count(result, "GJB10157.5.2.17") == 1 &&
          std::ranges::any_of(result.violations, [](const auto& item) {
              return item.ruleId == "GJB10157.5.2.17" &&
                     item.file.filename() == "zed.sv" &&
                     item.message == "参数WIDTH有alpha.WIDTH同名";
          }), "ordered native module map determines the first parameter independent of source order");
    result = analyze("module repeated #(parameter WIDTH = 1, WIDTH = 2, WIDTH = 3)(); endmodule\n");
    check(count(result, "GJB10157.5.2.17") == 2 &&
          std::ranges::count_if(result.violations, [](const auto& item) {
              return item.ruleId == "GJB10157.5.2.17" && item.line == 1 &&
                     item.column == 1 && item.message == "参数WIDTH有repeated.WIDTH同名";
          }) == 2, "same-line later AST_PARAMETER nodes retain native multiplicity");
    result = checkParameterReuse({
        {"first.sv", core::HdlLanguage::SystemVerilog,
         "module first; parameter WIDTH = 8; endmodule\n"},
        {"second.sv", core::HdlLanguage::SystemVerilog,
         "module second; parameter WIDTH = 16; endmodule\n"},
        {"third.sv", core::HdlLanguage::SystemVerilog,
         "module third; parameter WIDTH = 32; endmodule\n"}});
    check(count(result, "GJB10157.5.2.17") == 2,
          "every parameter occurrence after the first is reported");
    result = analyze("module top; parameter DATA1 = 32; endmodule\n"
                     "module mod1; parameter DATA1 = 32; endmodule\n"
                     "module mod2; parameter DATA1 = 8; endmodule\n");
    check(count(result, "GJB10157.5.2.17") == 2,
          "packaged same-file three-module pattern reports later duplicate parameters");
    result = checkParameterReuse({
        {"first.sv", core::HdlLanguage::SystemVerilog,
         "module first; parameter WIDTH = 8; localparam DEPTH = 2; endmodule\n"},
        {"second.sv", core::HdlLanguage::SystemVerilog,
         "module second; parameter width = 16; localparam DEPTH = 4; endmodule\n"}});
    check(count(result, "GJB10157.5.2.17") == 0,
          "bytewise case distinction and AST_LOCALPARAM exclusion");
    analysis::AnalysisOptions definedParameterReuseOptions = parameterReuseOptions;
    definedParameterReuseOptions.defines.push_back("USE_SHARED_WIDTH");
    result = analysis::Analyzer(definedParameterReuseOptions).analyzeSources({
        {"first.sv", core::HdlLanguage::SystemVerilog,
         "module first;\n`ifdef USE_SHARED_WIDTH\nparameter WIDTH = 8;\n`else\nparameter FIRST_WIDTH = 8;\n`endif\nendmodule\n"},
        {"second.sv", core::HdlLanguage::SystemVerilog,
         "module second;\n`ifdef USE_SHARED_WIDTH\nparameter WIDTH = 16;\n`else\nparameter SECOND_WIDTH = 16;\n`endif\nendmodule\n"}});
    check(count(result, "GJB10157.5.2.17") == 1,
          "configured active conditional branches participate in the native global parameter map");
    check(std::ranges::any_of(result.violations, [](const auto& item) {
        return item.ruleId == "GJB10157.5.2.17" && item.file.filename() == "second.sv" &&
               item.line == 3;
    }), "conditional parameter reuse retains the active branch source location");
    result = checkParameterReuse({
        {"first.sv", core::HdlLanguage::SystemVerilog,
         "module first;\n`ifdef USE_SHARED_WIDTH\nparameter WIDTH = 8;\n`else\nparameter FIRST_WIDTH = 8;\n`endif\nendmodule\n"},
        {"second.sv", core::HdlLanguage::SystemVerilog,
         "module second;\n`ifdef USE_SHARED_WIDTH\nparameter WIDTH = 16;\n`else\nparameter SECOND_WIDTH = 16;\n`endif\nendmodule\n"}});
    check(count(result, "GJB10157.5.2.17") == 0,
          "inactive duplicate parameter branches do not create false positives");
    check(std::ranges::any_of(result.standardAssessments, [](const auto& item) {
        return item.clauseId == "GJB10157.5.2.17" &&
               item.state == analysis::StandardRuleState::ManualReviewRequired;
    }), "no repeated parameter finding is not certified compliant");

    const auto headerAnalyze = [](std::string text, analysis::VerilogHeaderPolicy policy) {
        analysis::AnalysisOptions options;
        options.standardId = "gjb10157";
        options.verilogHeaderPolicy = std::move(policy);
        return analysis::Analyzer(options).analyzeSource({"probe.sv", core::HdlLanguage::SystemVerilog, std::move(text)});
    };
    const auto headerManual = [](const analysis::AnalysisResult& result) {
        return std::ranges::any_of(result.standardAssessments, [](const auto& item) {
            return item.clauseId == "GJB10157.5.1.1" && item.state == analysis::StandardRuleState::ManualReviewRequired;
        });
    };
    const analysis::VerilogHeaderPolicy authorPolicy{true, {{"Author", false}}};
    auto headerResult = headerAnalyze("// author:\nmodule probe; endmodule\n", authorPolicy);
    check(count(headerResult, "GJB10157.5.1.1") == 0 && headerManual(headerResult),
          "explicit casefold substring accepts empty field value but never certifies whole rule");
    headerResult = headerAnalyze("// Author\nmodule probe; endmodule\n", {true, {{"Author", true}, {"History", false}}});
    check(count(headerResult, "GJB10157.5.1.1") == 1, "explicit missing keyword diagnosed once at module");
    check(std::ranges::any_of(headerResult.violations, [](const auto& item) {
        return item.ruleId == "GJB10157.5.1.1" && item.line == 2 && item.column == 1 &&
               item.message == "文件头部注释中缺少\"History\"的注释声明";
    }), "configured header preserves native line-only location and exact missing-key message");
    headerResult = headerAnalyze("module probe; endmodule\n", authorPolicy);
    check(std::ranges::any_of(headerResult.violations, [](const auto& item) {
        return item.ruleId == "GJB10157.5.1.1" && item.line == 1 && item.column == 1 &&
               item.message == "文件缺少头部注释";
    }), "first-line module uses exact native missing-header diagnostic");
    headerResult = headerAnalyze("// none\nmodule probe; endmodule\n",
                                 {true, {{"Zulu", false}, {"alpha", false}, {"Author", false}}});
    check(std::ranges::any_of(headerResult.violations, [](const auto& item) {
        return item.ruleId == "GJB10157.5.1.1" &&
               item.message == "文件头部注释中缺少\"Author、Zulu、alpha\"的注释声明";
    }), "multiple absent keys follow native ordered-map traversal and punctuation");
    for (const auto& text : {"module probe; endmodule\n", "\n\nmodule probe; endmodule\n",
                            " \nmodule probe; endmodule\n", "// author\nmodule probe; endmodule\n"}) {
        headerResult = headerAnalyze(text, {true, {{"Author", true}}});
        check(count(headerResult, "GJB10157.5.1.1") == 1, "missing prefix/keyword/case mismatch emits diagnostic");
    }
    headerResult = headerAnalyze("module probe; endmodule\n", {true, {}});
    check(count(headerResult, "GJB10157.5.1.1") == 0 && headerManual(headerResult), "empty configured map disables without compliance");
    headerResult = headerAnalyze("// Author\nmodule\nprobe; endmodule\n", authorPolicy);
    check(count(headerResult, "GJB10157.5.1.1") == 0, "multiline module uses module keyword anchor");
    headerResult = headerAnalyze("// module fake\nmodule\nAuthor; endmodule\n", authorPolicy);
    check(count(headerResult, "GJB10157.5.1.1") == 1, "comment fake module and module name cannot satisfy prefix key");
    headerResult = headerAnalyze("// Author\nmodule aa; endmodule\nmodule bb; endmodule\n", authorPolicy);
    check(count(headerResult, "GJB10157.5.1.1") == 0 && headerManual(headerResult), "later module searches all physical prefix lines");
    headerResult = headerAnalyze("// no key\nmodule aa; endmodule\n// Author\nmodule bb; endmodule\n", authorPolicy);
    check(count(headerResult, "GJB10157.5.1.1") == 1, "per-module prefixes are independently checked");
    for (const auto& text : {"// 中文 Author\nmodule probe; endmodule\n",
                            "`define X Author\nmodule probe; endmodule\n",
                            "// missing\nmodule probe;\n",
                            "// missing\nmodule probe; module nested; endmodule endmodule\n",
                            "// missing\nmodule probe( ; endmodule\n",
                            "// missing\nmodule probe; endmodule module probe; endmodule\n"}) {
        headerResult = headerAnalyze(text, authorPolicy);
        check(count(headerResult, "GJB10157.5.1.1") == 0 && headerManual(headerResult),
              "non-ASCII/preprocessor/malformed/duplicate contexts remain review-required");
    }
    analysis::AnalysisOptions headerProjectOptions;
    headerProjectOptions.standardId = "gjb10157";
    headerProjectOptions.verilogHeaderPolicy = authorPolicy;
    headerResult = analysis::Analyzer(headerProjectOptions).analyzeSources({
        {"aa.sv", core::HdlLanguage::SystemVerilog, "// missing\nmodule probe; endmodule\n"},
        {"bb.sv", core::HdlLanguage::SystemVerilog, "// missing\nmodule probe; endmodule\n"}});
    check(count(headerResult, "GJB10157.5.1.1") == 0 && headerManual(headerResult),
          "cross-file duplicate module roots are not treated as unique native groups");
    check(headerResult.verilogHeaderPolicy.configurationKnown &&
          headerResult.verilogHeaderPolicy.requirements.size() == 1 &&
          headerResult.verilogHeaderPolicy.requirements[0].keyword == "Author",
          "aggregate result preserves effective header configuration for report");

    analysis::AnalysisOptions keywordOptions;
    keywordOptions.standardId = "gjb10157";
    keywordOptions.verilogKeywordPolicy = {true, {true,true,true,true,true,true}};
    const auto keywordAnalyze = [&](std::string text) {
        return analysis::Analyzer(keywordOptions).analyzeSource({"probe.sv", core::HdlLanguage::SystemVerilog, std::move(text)});
    };
    auto keywordResult = keywordAnalyze(wrap("wire SIGNAL; localparam MODULE=1; function integer TYPE; TYPE=1; endfunction task CON; endtask"));
    check(count(keywordResult, "GJB10157.5.2.3") == 4, "native keyword tables apply to net/parameter/function/task declarations");
    keywordResult = keywordAnalyze("module child; endmodule\nmodule CON #(parameter TYPE=1)(input SIGNAL); child MODULE(); endmodule\n");
    check(count(keywordResult, "GJB10157.5.2.3") == 4, "module/ANSI port/parameter/same-source cell subjects checked");
    keywordResult = keywordAnalyze(wrap("wire SIGNAL_suffix; wire COM10; wire CON_txt; wire DELAYFILE;"));
    check(count(keywordResult, "GJB10157.5.2.3") == 0, "not substring/Windows filename/SDF comma splitting");
    const auto keywordState = [](const analysis::AnalysisResult& r, analysis::StandardRuleState state) {
        return std::ranges::any_of(r.standardAssessments, [&](const auto& row) {
            return row.clauseId == "GJB10157.5.2.3" && row.state == state;
        });
    };
    check(keywordState(keywordResult, analysis::StandardRuleState::ManualReviewRequired), "explicit no match never certifies full keyword rule");
    keywordOptions.verilogKeywordPolicy = {};
    keywordResult = keywordAnalyze(wrap("wire SIGNAL;"));
    check(count(keywordResult, "GJB10157.5.2.3") == 1 && keywordState(keywordResult, analysis::StandardRuleState::Violated),
          "unspecified project policy uses the four keyword families mandated by the rule text");
    keywordResult = keywordAnalyze(wrap("wire CLOCK; task CON; endtask"));
    check(count(keywordResult, "GJB10157.5.2.3") == 0 && keywordState(keywordResult, analysis::StandardRuleState::ManualReviewRequired),
          "documented fallback does not silently extend the rule to optional PSL or Windows categories");
    keywordOptions.verilogKeywordPolicy = {true,{}};
    keywordResult = keywordAnalyze(wrap("wire SIGNAL;"));
    check(count(keywordResult, "GJB10157.5.2.3") == 0 && keywordState(keywordResult, analysis::StandardRuleState::ManualReviewRequired),
          "all-off disables without compliance");
    keywordOptions.verilogKeywordPolicy = {true,{true,true,false,false,false,false}};
    keywordResult = keywordAnalyze(wrap("wire MODULE;"));
    check(count(keywordResult, "GJB10157.5.2.3") == 1 && std::ranges::any_of(keywordResult.violations, [](const auto& item) {
        return item.ruleId == "GJB10157.5.2.3" &&
               item.message == "禁止使用Verilog关键字同名的MODULE";
    }), "earlier enabled nonmatch continues into later matching category in evaluator");
    keywordOptions.verilogKeywordPolicy = {true,{true,true,true,true,true,true}};
    keywordResult = keywordAnalyze(
        "module probe;\nwire SIGNAL;\nwire MODULE;\nwire ALWAYS_COMB;\n"
        "wire CLOCK;\nwire CON;\nendmodule\n");
    const std::array<std::string, 6> exactKeywordMessages{
        "禁止使用VHDL关键字同名的SIGNAL",
        "禁止使用Verilog关键字同名的MODULE",
        "禁止使用SystemVerilog关键字同名的ALWAYS_COMB",
        "禁止使用SDF/EDIF关键字同名的WHOLE_SDF_LITERAL",
        "禁止使用PSL关键字同名的CLOCK",
        "禁止使用Windows关键字同名的CON",
    };
    const std::array<std::string_view, 6> diagnosticProbeNames{
        "SIGNAL", "MODULE", "ALWAYS_COMB", "WHOLE_SDF_LITERAL", "CLOCK", "CON"};
    check(std::ranges::all_of(exactKeywordMessages, [&](const auto& message) {
              const auto index = static_cast<std::size_t>(&message - exactKeywordMessages.data());
              return analysis::nativeVerilogKeywordMessage(
                         static_cast<analysis::VerilogKeywordCategory>(index),
                         diagnosticProbeNames[index]) == message;
          }) && count(keywordResult, "GJB10157.5.2.3") == 5 &&
          std::ranges::all_of(exactKeywordMessages | std::views::filter([](const auto& message) {
              return message.find("SDF/EDIF") == std::string::npos;
          }), [&](const auto& message) {
              return std::ranges::any_of(keywordResult.violations, [&](const auto& item) {
                  return item.ruleId == "GJB10157.5.2.3" &&
                         item.severity == analysis::Severity::Error &&
                         item.column == 1 && item.message == message;
              });
          }), "R-2-3 preserves all six native category diagnostics, Error and no-column mapping");
    keywordResult = keywordAnalyze(
        "module duplicate; wire SIGNAL; endmodule\n"
        "module duplicate; wire MODULE; endmodule\n"
        "module safeunit;\nwire ALWAYS_COMB;\nendmodule\n");
    check(count(keywordResult, "GJB10157.5.2.3") == 1 &&
          std::ranges::any_of(keywordResult.violations, [](const auto& item) {
              return item.ruleId == "GJB10157.5.2.3" && item.line == 4 &&
                     item.column == 1 &&
                     item.message == "禁止使用SystemVerilog关键字同名的ALWAYS_COMB";
          }), "R-2-3 skips only the duplicate module-name group and retains a unique group in the same source");
    keywordResult = keywordAnalyze("`define X 1\nmodule probe; wire SIGNAL; endmodule\n");
    check(count(keywordResult,"GJB10157.5.2.3") == 1 &&
          keywordState(keywordResult, analysis::StandardRuleState::Violated),
          "a successfully processed macro directive no longer disables the AST-backed keyword rule");
    keywordOptions.defines.push_back("USE_RESERVED_NAME");
    keywordResult = keywordAnalyze(
        "module probe;\n`ifdef USE_RESERVED_NAME\nwire SIGNAL;\n`else\nwire safe_name;\n`endif\nendmodule\n");
    check(count(keywordResult, "GJB10157.5.2.3") == 1 &&
          std::ranges::any_of(keywordResult.violations, [](const auto& item) {
              return item.ruleId == "GJB10157.5.2.3" && item.line == 3;
          }), "5.2.3 checks reserved names in the configured active branch with source location");
    keywordOptions.defines.clear();
    keywordResult = keywordAnalyze(
        "module probe;\n`ifdef USE_RESERVED_NAME\nwire SIGNAL;\n`else\nwire safe_name;\n`endif\nendmodule\n");
    check(count(keywordResult, "GJB10157.5.2.3") == 0 &&
          keywordState(keywordResult, analysis::StandardRuleState::ManualReviewRequired),
          "5.2.3 excludes reserved names that exist only in an inactive branch");
    for (const auto& text : {"module probe; wire SIGNAL;\n",
                            "module probe; wire SIGNAL; endmodule module probe; wire SIGNAL; endmodule\n"}) {
        keywordResult = keywordAnalyze(text);
        check(count(keywordResult,"GJB10157.5.2.3") == 0 && keywordState(keywordResult, analysis::StandardRuleState::ManualReviewRequired),
              "malformed or duplicate source admission remains unproved without fabricated compliance");
    }

    result = analyze(wrap("/* casez (1'bx); default: qq = 1'bx; */\n"
        "string text = \"if (aa == 1'bx); casez default\";\n"
        "always @* qq = 8'hzz;"));
    check(count(result, "GJB10157.9.2.8") == 0, "comments/strings cannot create wildcard-case diagnostics");
    check(count(result, "GJB10157.9.2.4") == 0, "Z assignment is not an if/case condition");
    check(count(result, "GJB10157.8.4") == 0, "plain Z assignment is not a comparison");
    check(count(result, "GJB10157.9.2.9") == 0, "default text in comments/strings is ignored");

    result = analyze(wrap("always @* begin\ncasez (aa)\n0: qq = 0;\ndefault: qq = 0;\nendcase\n"
        "casex (aa)\n0: qq = 0;\ndefault: qq = 0;\nendcase\nend"));
    check(count(result, "GJB10157.9.2.8") == 2, "both casex and casez are diagnosed exactly once");

    result = analyze(wrap("always @* begin\ncasex (aa)\nendcase\n"
        "casez (aa)\nendcase\nend"));
    check(count(result, "GJB10157.9.2.8") == 0,
          "native wildcard-case rule ignores empty cases without CONDX/CONDZ item nodes");

    result = analyze(wrap("always @* begin\ncasex (aa)\n"
        "default: qq = 0;\nendcase\ncasez (aa)\n"
        "(aa ? 1'b0 : 1'b1): qq = 1;\nendcase\nend"));
    check(count(result, "GJB10157.9.2.8") == 2,
          "default-only and ternary-selector wildcard cases each retain one native diagnostic");

    result = analyze(wrap("always @* casex (aa) 1'b0: qq = 0;"));
    check(count(result, "GJB10157.9.2.8") == 0 &&
          std::ranges::any_of(result.standardAssessments, [](const auto& evaluation) {
              return evaluation.clauseId == "GJB10157.9.2.8" &&
                  evaluation.state == analysis::StandardRuleState::ManualReviewRequired;
          }), "incomplete wildcard case is not guessed violated or compliant");

    result = analyze(wrap("always @* begin\ncasex (aa)\n"
        "1'b0: casez (aa) 1'b0: qq = 0; default: qq = 1; endcase\n"
        "default: qq = 0;\nendcase\nend"));
    check(count(result, "GJB10157.9.2.8") == 2,
          "nested casex/casez nodes report once per native AST_CASE root");

    result = analyze(wrap("always @* begin\nif (\naa ==\n8'hXZ\n) qq = 0;\n"
        "else qq = 1;\nend"));
    check(count(result, "GJB10157.9.2.4") == 1, "multiline based-literal if condition is diagnosed");
    check(count(result, "GJB10157.8.4") == 0, "native 8.4 does not include equality comparisons");
    const auto parameterWidthProbe = [&](const std::string& secondWidth,
                                         const std::string& comparison) {
        return analyze("module probe #(parameter P_BUS1_SIZE = 32, parameter P_BUS2_SIZE = " +
            secondWidth + ") (input CLK, input [P_BUS2_SIZE-1:0] MY_IN1, "
            "input [P_BUS1_SIZE-1:0] MY_IN2, output reg Q);\n"
            "always @(posedge CLK) if (" + comparison + ") Q <= 1'b1;\nendmodule\n");
    };
    result = parameterWidthProbe("16", "MY_IN1 < MY_IN2");
    check(count(result, "GJB10157.8.1") == 1,
          "native two-operand comparison catches attachment's 16 versus 32 default width");
    result = parameterWidthProbe("16", "MY_IN2 >= MY_IN1");
    check(count(result, "GJB10157.8.1") == 1,
          "reversed relational comparison also catches parameterized mismatch");
    result = parameterWidthProbe("32", "MY_IN1 == MY_IN2");
    check(count(result, "GJB10157.8.1") == 0,
          "equal parameterized widths do not trigger equality comparison rule");
    result = parameterWidthProbe("31", "MY_IN1 === MY_IN2");
    check(count(result, "GJB10157.8.1") == 1,
          "one-bit default width difference is caught for case equality");
    result = parameterWidthProbe("UNKNOWN", "MY_IN1 < MY_IN2");
    check(count(result, "GJB10157.8.1") == 0 &&
          std::ranges::any_of(result.standardAssessments, [](const auto& evaluation) {
              return evaluation.clauseId == "GJB10157.8.1" &&
                  evaluation.state == analysis::StandardRuleState::ManualReviewRequired;
          }), "unresolved parameter width is not guessed compliant");
    const auto nestedComparisonWidthProbe = [&](const std::string& comparison) {
        return analyze("module probe(input [4:0] A, C, input [2:0] B, "
            "output reg Q);\nalways @* if (" + comparison + ") Q = 1'b1;\nendmodule\n");
    };
    result = nestedComparisonWidthProbe("(A + B) == C");
    check(count(result, "GJB10157.8.1") == 0,
          "flag-zero nested addition takes max child width and matches the five-bit operand");
    result = nestedComparisonWidthProbe("(B * B) == A");
    check(count(result, "GJB10157.8.1") == 1,
          "comparison width helper uses flag-zero max width for multiply rather than product width");
    result = nestedComparisonWidthProbe("-(B + B) != A");
    check(count(result, "GJB10157.8.1") == 1,
          "unary sign propagates the nested arithmetic width before comparison");
    result = nestedComparisonWidthProbe("(B ** B) < A");
    check(count(result, "GJB10157.8.1") == 1,
          "power comparison operand propagates its first child width in the native helper");
    result = nestedComparisonWidthProbe("({B, B}) == 6'b0");
    check(count(result, "GJB10157.8.1") == 0,
          "concatenation width remains the sum of its children inside a comparison operand");
    result = analyze("module probe(input [4:0] A, input [2:0] B, output Q);\n"
        "assign Q = A < B;\nendmodule\n");
    check(count(result, "GJB10157.8.1") == 1,
          "native continuous AST_ASSIGN root exposes a comparison on its RHS");
    result = analyze("module probe(input [4:0] A, input [2:0] B, output reg Q);\n"
        "always @* Q = (A == B);\nendmodule\n");
    check(count(result, "GJB10157.8.1") == 1,
          "native AST_ALWAYS traversal exposes a comparison used as an assignment value");
    result = analyze("module probe(input [4:0] A, input [2:0] B, output Q = (A < B));\n"
        "endmodule\n");
    check(count(result, "GJB10157.8.1") == 0,
          "declaration initializer is not promoted to the native AST_ASSIGN root set");
    const auto arithmeticWidthProbe = [&](const std::string& outputRange,
                                          const std::string& expression) {
        return analyze("module probe #(parameter P_BUS1_SIZE = 3) "
            "(input CLK, input [P_BUS1_SIZE-1:0] A, B, output reg " + outputRange + " Q);\n"
            "always @(posedge CLK) Q <= " + expression + ";\nendmodule\n");
    };
    result = arithmeticWidthProbe("[P_BUS1_SIZE-1:0]", "A + B");
    check(count(result, "GJB10157.8.7") == 1,
          "native 8.7 attachment addition needs a carry bit beyond the three-bit target");
    result = arithmeticWidthProbe("[P_BUS1_SIZE-1:0]", "A - B");
    check(count(result, "GJB10157.8.7") == 1,
          "native 8.7 attachment subtraction needs a borrow bit beyond the three-bit target");
    result = arithmeticWidthProbe("[P_BUS1_SIZE:0]", "A + B");
    check(count(result, "GJB10157.8.7") == 0,
          "four-bit target carries the three-bit addition result");
    result = arithmeticWidthProbe("[P_BUS1_SIZE:0]", "A - B");
    check(count(result, "GJB10157.8.7") == 0,
          "four-bit target carries the three-bit subtraction result");
    result = arithmeticWidthProbe("[P_BUS1_SIZE-1:0]", "A + 3'b0");
    check(count(result, "GJB10157.8.7") == 1,
          "native add branch treats an equally wide sized literal as an ordinary operand");
    result = arithmeticWidthProbe("[P_BUS1_SIZE-1:0]", "A + A");
    check(count(result, "GJB10157.8.7") == 1,
          "doubling one signal can require a carry bit");
    result = arithmeticWidthProbe("[P_BUS1_SIZE-1:0]", "A - A");
    check(count(result, "GJB10157.8.7") == 1,
          "native subtraction width predicate has no same-identifier value exemption");
    result = arithmeticWidthProbe("[P_BUS1_SIZE:0]", "{1'b0, A} + {1'b0, B}");
    check(count(result, "GJB10157.8.7") == 0,
          "native add branch accepts equally wide operands explicitly prefixed by zero");
    result = arithmeticWidthProbe("[P_BUS1_SIZE:0]", "{1'b1, A} + {1'b0, B}");
    check(count(result, "GJB10157.8.7") == 1,
          "nonzero leftmost concat bits do not receive the native carry exemption");
    result = arithmeticWidthProbe("[P_BUS1_SIZE:0]", "{1'bx, A} - {1'b0, B}");
    check(count(result, "GJB10157.8.7") == 1,
          "unknown leftmost concat bits are not mistaken for native all-S0 bits");
    result = arithmeticWidthProbe("[P_BUS1_SIZE:0]", "{A[2], A} + {1'b0, B}");
    check(count(result, "GJB10157.8.7") == 0,
          "native empty bits-vector behavior also exempts an identifier concat prefix");
    result = arithmeticWidthProbe("[P_BUS1_SIZE-1:0]", "{A} + B");
    check(count(result, "GJB10157.8.7") == 1,
          "single-item concat does not satisfy the native more-than-one-child gate");
    result = arithmeticWidthProbe("[P_BUS1_SIZE+1:0]", "{1'b1, A} + {1'b1, B}");
    check(count(result, "GJB10157.8.7") == 0,
          "an add target wider than both concat operands remains clear");
    result = arithmeticWidthProbe("[P_BUS1_SIZE:0]", "(A + B) + A");
    check(count(result, "GJB10157.8.7") == 0,
          "nested add width uses native helper max-width inference before the outer add check");
    result = arithmeticWidthProbe("[4:0]", "(A + B) * A");
    check(count(result, "GJB10157.8.7") == 1,
          "outer multiply sums a nested add inferred width and its other child width");
    result = arithmeticWidthProbe("[5:0]", "(A + B) * A");
    check(count(result, "GJB10157.8.7") == 0,
          "six-bit target accepts native three-plus-three nested multiply threshold");
    result = arithmeticWidthProbe("[5:0]", "A * B * A");
    check(count(result, "GJB10157.8.7") == 0,
          "left-associated nested multiply is max-inferred as a child before outer width summation");
    result = analyze("module probe(input [4:0] A, input [2:0] B, input C, "
        "output reg [2:0] Q);\nalways @* Q = (A + B) / C;\nendmodule\n");
    check(count(result, "GJB10157.8.7") == 1,
          "nested add contributes its max child width to native division inference");
    result = analyze("module probe(input [4:0] A, input [2:0] B, input C, "
        "output reg [3:0] Q);\nalways @* Q = (A + B) / C;\nendmodule\n");
    check(count(result, "GJB10157.8.7") == 0,
          "nested division exact inferred-width-minus-one threshold remains clear");
    result = arithmeticWidthProbe("[4:0]", "A * B");
    check(count(result, "GJB10157.8.7") == 1,
          "native multiply branch requires the sum of the two three-bit operand widths");
    result = arithmeticWidthProbe("[5:0]", "A * B");
    check(count(result, "GJB10157.8.7") == 0,
          "six-bit target carries the complete three-by-three-bit product");
    result = analyze("module probe(input [4:0] A, input [2:0] B, output reg Q);\n"
        "always @* Q = A / B;\nendmodule\n");
    check(count(result, "GJB10157.8.7") == 1,
          "native division branch compares one-bit target against max operand width minus one");
    result = analyze("module probe(input [4:0] A, input [2:0] B, output reg [3:0] Q);\n"
        "always @* Q = A / B;\nendmodule\n");
    check(count(result, "GJB10157.8.7") == 0,
          "native division threshold accepts max operand width minus one");
    result = analyze("module probe(input [4:0] A, input [2:0] B, output reg [2:0] Q);\n"
        "always @* Q = A % B;\nendmodule\n");
    check(count(result, "GJB10157.8.7") == 1,
          "native modulo branch uses the same inferred-width-minus-one threshold");
    result = analyze("module probe(input A, B, output reg Q);\n"
        "always @* Q = A % B;\nendmodule\n");
    check(count(result, "GJB10157.8.7") == 0,
          "one-bit modulo cannot underflow the native inferred-width-minus-one threshold");
    result = analyze("module probe(input [1:0] A, B, output reg [3:0] Q);\n"
        "always @* Q = A ** B;\nendmodule\n");
    check(count(result, "GJB10157.8.7") == 1,
          "native power branch uses (2^exponent-width-1)*base-width-1 threshold");
    result = analyze("module probe(input [1:0] A, B, output reg [4:0] Q);\n"
        "always @* Q = A ** B;\nendmodule\n");
    check(count(result, "GJB10157.8.7") == 0,
          "power target exactly at the recovered native threshold stays clear");
    result = analyze("module probe(input A, B, output reg Q);\n"
        "always @* Q = A ** B;\nendmodule\n");
    check(count(result, "GJB10157.8.7") == 0,
          "one-bit power operands produce the native zero-bit threshold without underflow");
    result = analyze("module probe(input [1:0] B, output reg Q);\n"
        "always @* Q = 2'b10 ** B;\nendmodule\n");
    check(count(result, "GJB10157.8.7") == 0,
          "literal power base remains outside native AST_IDENTIFIER admission");
    result = analyze("module probe(input [3:0] A, input [1:0] B, C, output reg [3:0] Q);\n"
        "always @* Q = A[1:0] ** (B + C);\nendmodule\n");
    check(count(result, "GJB10157.8.7") == 1,
          "selected identifier base and nested exponent use recovered native power widths");
    result = analyze("module probe(input [1:0] A, B, C, output reg Q);\n"
        "always @* Q = (A + B) ** C;\nendmodule\n");
    check(count(result, "GJB10157.8.7") == 0,
          "nested expression power base remains excluded by native AST_IDENTIFIER gate");
    for (const auto* op : {"<", "<=", ">", ">="}) {
        result = analyze(wrap(std::string("always @* if (aa ") + op + " 8'hXZ) qq = 0;"));
        check(count(result, "GJB10157.8.4") == 2,
              "native relational operators diagnose X and Z classes independently");
    }
    for (const auto* op : {"==", "!=", "===", "!=="}) {
        result = analyze(wrap(std::string("always @* if (aa ") + op + " 8'hXZ) qq = 0;"));
        check(count(result, "GJB10157.8.4") == 0, "equality operators are outside native 8.4 predicate");
        check(count(result, "GJB10157.9.2.4") == 1, "equality condition still has its independent X/Z diagnostic");
    }
    result = analyze(wrap("always @* if (aa == 8'hxz) qq = aa + 1; else qq = 0;"));
    check(count(result, "GJB10157.8.5") == 0, "arithmetic in if body does not consume X predicate");

    result = analyze(wrap("always @(posedge clk) begin\nqq <= 8'hzz;\nend"));
    check(count(result, "GJB10157.8.4") == 0, "nonblocking assignment is not a <= comparison");
    check(count(result, "GJB10157.9.2.4") == 0, "nonblocking assignment is not a condition");
    result = analyze(wrap("always @* qq = aa <= 8'hxz;"));
    check(count(result, "GJB10157.8.4") == 2,
          "<= operator on blocking-assignment RHS remains a comparison with separate X/Z diagnostics");

    result = analyze(wrap("always @* qq = (aa < bb) + 8'hx;"));
    check(count(result, "GJB10157.8.4") == 0,
          "unknown literal outside the relational AST subtree is not borrowed into 8.4");
    result = analyze(wrap("always @* qq = aa < (bb + 8'hx);"));
    check(count(result, "GJB10157.8.4") == 1,
          "unknown literal nested inside a relational operand remains owned by the relation");
    result = analyze(wrap("initial if (aa > 8'hz) qq = 0;"));
    check(count(result, "GJB10157.8.4") == 1,
          "native relation-node admission is not narrowed to always or continuous assign roots");

    result = analyze(wrap("always @(posedge clk) begin\nqq <= aa +\n5'dx;\nend"));
    check(count(result, "GJB10157.8.5") == 1, "multiline arithmetic detects decimal-based X");
    check(count(result, "GJB10157.8.4") == 0, "arithmetic nonblocking assignment is not a comparison");
    result = analyze(wrap("always @* qq = (aa + 1) ? 'x : 0;"));
    check(count(result, "GJB10157.8.5") == 0, "arithmetic in ternary predicate does not consume X result branch");
    result = analyze(wrap("always @* qq = {1'bx, aa} + 1;"));
    check(count(result, "GJB10157.8.5") == 1, "arithmetic recursively consumes concat X child");
    result = analyze(wrap("always @* qq = fn('x, aa+1);"));
    check(count(result, "GJB10157.8.5") == 0, "function argument siblings do not leak unknown state");
    result = analyze(wrap("always @* qq = ('x ? aa : 0) + 1;"));
    check(count(result, "GJB10157.8.5") == 1, "arithmetic consumes unknown ternary descendant");
    result = analyze(wrap("always @* qq = ('x << 1) + 1;"));
    check(count(result, "GJB10157.8.5") == 1, "normalized shift subtree preserves unknown ancestry");
    result = analyze(wrap("always @* qq = aa['x +: 1] + 1;"));
    check(count(result, "GJB10157.8.5") == 1, "part-select subtree preserves unknown ancestry");
    result = analyze(wrap("localparam P = 'x + 1; initial qq = 'x+1;\n"
                          "function f; f='x+1; endfunction"));
    check(count(result, "GJB10157.8.5") == 0, "native ALWAYS/ASSIGN roots exclude unrelated initializers and function bodies");
    result = analyze(wrap("always @* for(integer j=0; j<2; j=j+1) begin qq=aa+'x; end"));
    check(count(result, "GJB10157.8.5") == 1, "for header semicolons do not truncate always root body");
    result = analyze(wrap("always @* qq = (aa + 8'hxz) * 2;"));
    check(count(result, "GJB10157.8.5") == 4,
          "native arithmetic walk reports X and Z separately at each selected ancestor node");

    result = analyze(wrap("always @* case (1 + aa)\n0: qq = 0;\nendcase"));
    check(count(result, "GJB10157.9.2.2") == 0, "expression starting with numeric operand is not necessarily fixed");
    check(count(result, "GJB10157.9.2.9") == 0, "missing default is not default-containing-X rule");
    result = analyze(wrap("always @* case (\n1 + 2\n)\ndefault: qq = 0;\nendcase"));
    check(count(result, "GJB10157.9.2.2") == 1, "multiline fixed case control is diagnosed");
    check(std::ranges::any_of(result.violations, [](const auto& item) {
        return item.ruleId == "GJB10157.9.2.2" && item.severity == analysis::Severity::Error &&
               item.message == "禁止在case语句的控制表达式中使用固定值.";
    }), "9.2.2 preserves the native message while the mandatory profile elevates severity to Error");
    result = analyze(wrap("always @* case(1) endcase"));
    check(count(result, "GJB10157.9.2.2") == 1,
          "complete empty case retains its constant selector child and native diagnostic");
    result = analyze(wrap("always @* case(1) 0:qq=0;"));
    check(count(result, "GJB10157.9.2.2") == 0 &&
          std::ranges::any_of(result.standardAssessments, [](const auto& evaluation) {
              return evaluation.clauseId == "GJB10157.9.2.2" &&
                  evaluation.state == analysis::StandardRuleState::ManualReviewRequired;
          }), "incomplete constant-selector case is not guessed violated or compliant");
    result = analyze(wrap("always @* begin\ncasex(1) default:qq=0; endcase\n"
                          "casez(2) default:qq=0; endcase\nend"));
    check(count(result, "GJB10157.9.2.2") == 2,
          "constant selectors on casex and casez each produce one CASE-root diagnostic");
    result = analyze(wrap("always @* case(1) 0:case(2) default:qq=0; endcase "
                          "default:qq=0; endcase"));
    check(count(result, "GJB10157.9.2.2") == 2,
          "nested constant-selector cases report once per native AST_CASE root");
    result = analyze(wrap("parameter P_VALUE = 32; localparam lp1 = 1;\n"
        "always @* begin case(P_VALUE) default: qq=0; endcase\n"
        "case(lp1) default: qq=0; endcase\n"
        "case(lp1-P_VALUE) default: qq=0; endcase end"));
    check(count(result, "GJB10157.9.2.2") == 3, "published parameter/localparam case examples are diagnosed");
    result = analyze(wrap("parameter P=1; always @* begin : nested\n"
        "integer P; case(P) default: qq=0; endcase end\n"
        "always @* case(P+aa) default: qq=0; endcase"));
    check(count(result, "GJB10157.9.2.2") == 0, "shadowing and signal-dependent expressions stay nonconstant");

    result = analyze(wrap("always @* case (aa)\ndefault: begin\nqq =\n8'hxF;\nend\n"
        "1: qq = 0;\nendcase"));
    check(count(result, "GJB10157.9.2.9") == 1, "default begin/end captures multiline X assignment");
    check(std::ranges::any_of(result.violations, [](const auto& item) {
        return item.ruleId == "GJB10157.9.2.9" &&
               item.severity == analysis::Severity::Warning &&
               item.message == "避免在default分支中出现包含'x'的值.";
    }), "9.2.9 preserves the recovered native Warning and exact message");
    result = analyze(wrap("always @* case (aa)\ndefault: qq = 8'hzz;\n"
        "1: qq = 8'hxx;\nendcase"));
    check(count(result, "GJB10157.9.2.9") == 0, "Z default and X in another case item do not violate X-default rule");
    result = analyze(wrap("always @* case (aa)\ndefault: begin\n"
        "qq = 8'hxx;\nqq = 8'hx0 | 8'h0x;\nend\nendcase"));
    check(count(result, "GJB10157.9.2.9") == 2,
          "native 9.2.9 emits once per violating assignment, not once per default branch or X constant");
    result = analyze(wrap("always @* begin\n"
        "case (8'h01) default: qq = 8'hxx; endcase\n"
        "case (aa + 1) default: qq = 8'hxx; endcase\n"
        "case (aa[0]) default: qq = 8'hxx; endcase\nend"));
    check(count(result, "GJB10157.9.2.9") == 1,
          "native 9.2.9 admits an AST_IDENTIFIER selector and rejects constant or operator roots");
    result = analyze(wrap("logic q$generated; always @* case (aa)\n"
        "default: begin q$generated = 1'bx; qq[1'bx] = 0; "
        "{q$generated, qq} = {1'bx, 1'b0}; end\nendcase"));
    check(count(result, "GJB10157.9.2.9") == 2,
          "native 9.2.9 excludes a dollar-named identifier root, scans LHS X constants, and does not borrow names below a concat root");
    result = analyze(wrap("always @* case (aa)\n"
        "1: case (qq) default: qq = 1'bx; endcase\n"
        "default: if (aa) begin qq = 1'bx; qq = 1'bx; end\nendcase"));
    check(count(result, "GJB10157.9.2.9") == 3,
          "nested CASE ownership and recursive assignments retain native per-assignment multiplicity");
    result = analyze(wrap("always @* case (aa) default: qq = 1'bx;"));
    check(count(result, "GJB10157.9.2.9") == 0,
          "incomplete identifier-selector case does not fabricate a native default-branch node");
    result = analyzeVerilog(
        "module probe(input aa, output reg qq); always @* case(aa) default:qq=1'bx; endcase endmodule\n");
    check(count(result, "GJB10157.9.2.9") == 0,
          "native case collector excludes CASE nodes sharing the module declaration line");
    result = analyze(wrap("default disable iff (aa == 'x);\n"
                          "always @* case (aa) default: qq = 0; endcase\n"
                          "default disable iff (aa == 'x);"));
    check(count(result, "GJB10157.9.2.9") == 0, "non-case default constructs do not create X-default diagnostics");
    result = analyze(wrap("function func; input a; func<=a+1; endfunction"));
    check(count(result, "GJB10157.9.4.3") == 1 &&
          std::ranges::any_of(result.violations, [](const auto& item) {
              return item.ruleId == "GJB10157.9.4.3" &&
                     item.severity == analysis::Severity::Warning &&
                     item.message == "禁止在function语句中使用非阻塞赋值.";
          }), "function nonblocking assignment native core predicate and diagnostic are integrated");
    result = analyze(wrap("function func; input a; if(a<=1) func=aa<=1; else func=0; endfunction"));
    check(count(result, "GJB10157.9.4.3") == 0, "function comparisons do not create assignment violations");
    const auto functionAssessment = std::ranges::find_if(result.standardAssessments,
        [](const auto& entry) { return entry.clauseId == "GJB10157.9.4.3"; });
    check(functionAssessment != result.standardAssessments.end() &&
          functionAssessment->state == analysis::StandardRuleState::ManualReviewRequired,
          "partial function check does not falsely certify compliance");

    result = analyzeVerilog(
        "module probe(\n"
        "   input wire CLK,\n"
        "   input wire A,\n"
        "    output reg Y,\n"
        "   output reg Z\n"
        "); endmodule\n");
    check(count(result, "GJB10157.5.1.3") == 1,
          "attachment 5.1.3 reports the one port direction shifted right");
    check(std::ranges::any_of(result.violations, [](const auto& item) {
              return item.ruleId == "GJB10157.5.1.3" && item.line == 4 &&
                     item.column == 1 && item.severity == analysis::Severity::Error &&
                     item.message == "Y定义左端未对齐";
          }), "native declaration alignment uses the declaration name, fixed Error and line-only location");
    result = analyzeVerilog(
        "module probe(\n"
        "   input wire CLK,\n"
        "   input wire A,\n"
        "   output reg Y,\n"
        "   output reg Z\n"
        "); endmodule\n");
    check(count(result, "GJB10157.5.1.3") == 0,
          "attachment 5.1.3 aligned port declarations remain clear");
    const auto alignmentAssessment = std::ranges::find_if(result.standardAssessments,
        [](const auto& entry) { return entry.clauseId == "GJB10157.5.1.3"; });
    check(alignmentAssessment != result.standardAssessments.end() &&
          alignmentAssessment->state == analysis::StandardRuleState::ManualReviewRequired,
          "bounded declaration alignment no finding is not certified compliance");

    result = analyzeVerilog(
        "module probe(input a, input b, output reg y);\n"
        "always @* begin\n"
        "  if (a) y = 0;\n"
        "  else if (b) y = 1;\n"
        "   else y = 0;\n"
        "  case (a)\n"
        "  1'b0: y = 0;\n"
        "   1'b1: y = 1;\n"
        "  endcase\n"
        "end endmodule\n");
    check(count(result, "GJB10157.5.1.2") == 2,
          "attachment 5.1.2 reports shifted final else and shifted case label");
    check(std::ranges::all_of(result.violations, [](const auto& item) {
              return item.ruleId != "GJB10157.5.1.2" ||
                     (item.severity == analysis::Severity::Error && item.column == 1 &&
                      item.message == "建议同一层次的所有语句左端对齐");
          }) && std::ranges::any_of(result.violations, [](const auto& item) {
              return item.ruleId == "GJB10157.5.1.2" && item.line == 5;
          }) && std::ranges::any_of(result.violations, [](const auto& item) {
              return item.ruleId == "GJB10157.5.1.2" && item.line == 8;
          }), "native statement-alignment diagnostics use fixed Error text and line-only locations");
    result = analyzeVerilog(
        "module probe(input a, input b, output reg y);\n"
        "always @* begin\n"
        "  if (a) y = 0;\n"
        "  else if (b) y = 1;\n"
        "  else y = 0;\n"
        "  case (a)\n"
        "  1'b0: y = 0;\n"
        "  1'b1: y = 1;\n"
        "  endcase\n"
        "end endmodule\n");
    check(count(result, "GJB10157.5.1.2") == 0,
          "attachment 5.1.2 aligned if/else and case items remain clear");
    const auto statementAlignmentAssessment = std::ranges::find_if(result.standardAssessments,
        [](const auto& entry) { return entry.clauseId == "GJB10157.5.1.2"; });
    check(statementAlignmentAssessment != result.standardAssessments.end() &&
          statementAlignmentAssessment->state == analysis::StandardRuleState::ManualReviewRequired,
          "bounded statement alignment no finding is not certified compliance");
    result = analyzeVerilog(
        "module probe(input a, input b, output reg y); always @* begin\n"
        " if (a) begin\n"
        "    if (b) y = 1;\n"
        "    else y = 0;\n"
        " end\n"
        " else y = 0;\n"
        "end endmodule\n");
    check(count(result, "GJB10157.5.1.2") == 0,
          "nested if chains establish independent indentation baselines");
    result = analyzeVerilog(
        "module probe(input [1:0] a, input b, output reg y); always @* begin\n"
        " case (a)\n"
        " 2'b00: case (b)\n"
        "        1'b0: y = 0;\n"
        "        default: y = 1;\n"
        "        endcase\n"
        " default: y = 0;\n"
        " endcase\n"
        "end endmodule\n");
    check(count(result, "GJB10157.5.1.2") == 0,
          "nested case statements compare only their own direct item labels");
    result = analyzeVerilog(
        "module probe(input a, output reg y); always @* begin\n"
        " // if else case labels in comments are ignored\n"
        " if (a) y = 1; else y = 0;\n"
        " $display(\"case else\");\n"
        "end endmodule\n");
    check(count(result, "GJB10157.5.1.2") == 1,
          "same-line else retains its native source column while comments and strings stay excluded");
    result = analyzeVerilog(
        "module probe(input [1:0] a, output reg y); always @* begin\n"
        "  case (a)\n"
        "  2'b00,  2'b01: y = 1;\n"
        "  default: y = 0;\n"
        "  endcase\n"
        "end endmodule\n");
    check(count(result, "GJB10157.5.1.2") == 1,
          "each direct case-item expression participates in native fixed-baseline comparison");

    result = analyzeVerilog(
        "module probe(\n"
        " input CLK,\n"
        " output C2,\n"
        " input A,\n"
        " input B,\n"
        " inout D,\n"
        " input B2,\n"
        " output reg C\n"
        "); always @(posedge CLK) C <= A; endmodule\n");
    check(count(result, "GJB10157.5.1.5") == 1 &&
          std::ranges::any_of(result.violations, [](const auto& item) {
              return item.ruleId == "GJB10157.5.1.5" && item.line == 3 &&
                     item.column == 1 && item.severity == analysis::Severity::Error &&
                     item.message ==
                         "端口列表应按照“时钟、复位、输入、输出、双向端口”的顺序描述";
          }), "native 5.1.5 reports the first mismatch with fixed line-only Error text");
    result = analyzeVerilog(
        "module probe(\n"
        " input CLK,\n"
        " input A,\n"
        " input B,\n"
        " input B2,\n"
        " output C2,\n"
        " output reg C,\n"
        " inout D\n"
        "); always @(posedge CLK) C <= A; endmodule\n");
    check(count(result, "GJB10157.5.1.5") == 0,
          "attachment 5.1.5 clock, inputs, outputs and inout order remains clear");
    const auto portOrderAssessment = std::ranges::find_if(result.standardAssessments,
        [](const auto& entry) { return entry.clauseId == "GJB10157.5.1.5"; });
    check(portOrderAssessment != result.standardAssessments.end() &&
          portOrderAssessment->state == analysis::StandardRuleState::ManualReviewRequired,
          "bounded port order no finding is not certified compliance");
    result = analyzeVerilog(
        "module probe(\n"
        " input DATA,\n"
        " input RESET_N,\n"
        " input CLK_MAIN,\n"
        " output reg Q,\n"
        " inout PAD\n"
        "); always @(posedge CLK_MAIN or negedge RESET_N) begin\n"
        " if (!RESET_N) Q <= 1'b0; else Q <= DATA; end endmodule\n");
    check(count(result, "GJB10157.5.1.5") == 1,
          "recognised clock and reset inputs are promoted ahead of ordinary inputs");
    result = analyzeVerilog(
        "module probe(\n"
        " input CLK_MAIN,\n"
        " input RESET_N,\n"
        " input DATA,\n"
        " output reg Q,\n"
        " inout PAD\n"
        "); always @(posedge CLK_MAIN or negedge RESET_N) begin\n"
        " if (!RESET_N) Q <= 1'b0; else Q <= DATA; end endmodule\n");
    check(count(result, "GJB10157.5.1.5") == 0,
          "semantic clock, reset, input, output and inout order remains clear");
    result = analyzeVerilog(
        "module probe(input DATA, input CLK_UNUSED, output Q); assign Q = DATA; endmodule\n");
    check(count(result, "GJB10157.5.1.5") == 0,
          "clock-like spelling alone does not create a native relation-order requirement");
    result = analyzeVerilog(
        "module probe(input CLK, output X, input X);\n"
        "always @(posedge CLK) begin end\nendmodule\n");
    check(count(result, "GJB10157.5.1.5") == 0,
          "native port-order comparison skips duplicate names after vector-size mismatch");
    result = analyzeVerilog(
        "module probe(input A, output Q, input B); endmodule\n");
    check(count(result, "GJB10157.5.1.5") == 1 &&
          std::ranges::any_of(result.violations, [](const auto& item) {
              return item.ruleId == "GJB10157.5.1.5" && item.line == 1 &&
                     item.column == 1;
          }), "same-line native port-order location remains line-only column one");

    result = analyzeVerilog(
        "module probe(input wire CLK, input wand A, input wand [3:0] A_ARR,\n"
        "input wor B, input tri B2, input tri B3, B4, input [1:0] COND,\n"
        "output reg C, C1, output integer D, input E1, E2, output time F);\n"
        "always @(posedge CLK) C <= A; endmodule\n");
    check(count(result, "GJB10157.5.3.2") == 8,
          "attachment 5.3.2 reports every top-level wand, wor, tri, integer and time port");
    check(std::ranges::all_of(result.violations | std::views::filter([](const auto& item) {
              return item.ruleId == "GJB10157.5.3.2";
          }), [](const auto& item) {
              return item.column == 1 && item.severity == analysis::Severity::Warning &&
                     item.message == "顶层模块的输入输出端口必须只使用reg型和wire型数据";
          }), "native top-port type rule preserves fixed Warning text and line-only location");
    result = analyzeVerilog(
        "module probe(input tri A, B, C, output wire Y); assign Y=A; endmodule\n");
    check(count(result, "GJB10157.5.3.2") == 3 &&
          std::ranges::all_of(result.violations | std::views::filter([](const auto& item) {
              return item.ruleId == "GJB10157.5.3.2";
          }), [](const auto& item) {
              return item.line == 1 && item.column == 1 &&
                     item.message == "顶层模块的输入输出端口必须只使用reg型和wire型数据";
          }), "native top-port type rule retains identical same-line per-port multiplicity");
    result = analyzeVerilog(
        "module probe(input CLK, input wire RST, input [3:0] A, input B,\n"
        "output reg C, output wire D, inout wire PAD); endmodule\n");
    check(count(result, "GJB10157.5.3.2") == 0,
          "attachment 5.3.2 implicit wire plus explicit wire/reg ports remain clear");
    const auto portTypeAssessment = std::ranges::find_if(result.standardAssessments,
        [](const auto& entry) { return entry.clauseId == "GJB10157.5.3.2"; });
    check(portTypeAssessment != result.standardAssessments.end() &&
          portTypeAssessment->state == analysis::StandardRuleState::ManualReviewRequired,
          "bounded top-level port type no finding is not certified compliance");
    result = analyze(
        "module probe(input logic A, output logic Y); assign Y=A; endmodule\n");
    check(count(result, "GJB10157.5.3.2") == 0,
          "SystemVerilog logic does not match any of the native twelve rejected flags");
    result = analyzeVerilog(
        "module probe(CLK,A,B,C,D,F); input CLK; input wand A; input tri B;\n"
        "output reg C; output integer D; output time F; endmodule\n");
    check(count(result, "GJB10157.5.3.2") == 4,
          "attachment 5.3.2 old-style port declarations use the same type predicate");
    {
        analysis::AnalysisOptions topOptions;
        topOptions.standardId = "GJB10157-2021";
        const std::vector<core::SourceFile> hierarchy{
            {"leaf.v", core::HdlLanguage::Verilog,
             "module leaf(input tri A, output integer Y); endmodule\n"},
            {"top.v", core::HdlLanguage::Verilog,
             "module top(input wire A, output wire Y); leaf u0(A,Y); endmodule\n"}
        };
        const auto hierarchyResult = analysis::Analyzer(topOptions).analyzeSources(hierarchy);
        check(count(hierarchyResult, "GJB10157.5.3.2") == 0,
              "5.3.2 excludes instantiated child modules and checks only project top levels");
    }
    {
        analysis::AnalysisOptions activeOptions;
        activeOptions.standardId = "GJB10157-2021";
        const std::string conditional =
            "module probe(\n`ifdef BAD_TYPE\ninput tri A,\n`else\ninput wire A,\n`endif\n"
            "output wire Y); endmodule\n";
        check(count(analysis::Analyzer(activeOptions).analyzeSource(
                  {"probe.v", core::HdlLanguage::Verilog, conditional}),
              "GJB10157.5.3.2") == 0,
              "inactive forbidden top-level port type cannot report");
        activeOptions.defines.push_back("BAD_TYPE");
        check(count(analysis::Analyzer(activeOptions).analyzeSource(
                  {"probe.v", core::HdlLanguage::Verilog, conditional}),
              "GJB10157.5.3.2") == 1,
              "active forbidden top-level port type reports");
    }
    result = analyzeVerilog(
        "module probe(input CLK, input IN1, IN2, output OUT1, OUT2, output reg OUT3);\n"
        "child u0(.CLK(CLK), .IN1(IN1), .OUT1(OUT1));\n"
        "always @(posedge CLK) OUT3 <= IN1;\n"
        "assign OUT2 = IN1 & IN2; endmodule\n");
    check(count(result, "GJB10157.5.3.3") == 2,
          "attachment 5.3.3 reports top-level procedural assignment and compound continuous RHS");
    check(std::ranges::any_of(result.violations, [](const auto& item) {
              return item.ruleId == "GJB10157.5.3.3" && item.line == 3 && item.column == 1 &&
                     item.severity == analysis::Severity::Warning &&
                     item.message == "顶层probe包含胶连逻辑OUT3";
          }) && std::ranges::any_of(result.violations, [](const auto& item) {
              return item.ruleId == "GJB10157.5.3.3" && item.line == 4 && item.column == 1 &&
                     item.severity == analysis::Severity::Warning &&
                     item.message == "顶层probe包含胶连逻辑OUT2";
          }), "native top-glue rule preserves dynamic message, Warning and line-only location");
    result = analyzeVerilog(
        "module probe(input A, B, output reg Y); always @* begin Y = A; Y = B; end endmodule\n");
    check(count(result, "GJB10157.5.3.3") == 2 &&
          std::ranges::all_of(result.violations | std::views::filter([](const auto& item) {
              return item.ruleId == "GJB10157.5.3.3";
          }), [](const auto& item) {
              return item.line == 1 && item.column == 1 &&
                     item.message == "顶层probe包含胶连逻辑Y";
          }), "native top-glue rule retains identical same-line per-assignment multiplicity");
    result = analyzeVerilog(
        "module probe(input A,B, output Y,Z); assign Y=A&B, Z=A|B; endmodule\n");
    check(count(result, "GJB10157.5.3.3") == 2 &&
          std::ranges::any_of(result.violations, [](const auto& item) {
              return item.ruleId == "GJB10157.5.3.3" &&
                     item.message == "顶层probe包含胶连逻辑Y";
          }) && std::ranges::any_of(result.violations, [](const auto& item) {
              return item.ruleId == "GJB10157.5.3.3" &&
                     item.message == "顶层probe包含胶连逻辑Z";
          }), "native top-glue rule preserves one AST_ASSIGN per comma-separated net assignment");
    result = analyzeVerilog(
        "module probe(input A,B, output Y,Z); assign Y=A, Z=B; endmodule\n");
    check(count(result, "GJB10157.5.3.3") == 0,
          "comma-separated direct identifier wire-through assignments remain clear");
    result = analyzeVerilog(
        "module probe(input CLK, input IN1, output OUT1, OUT2);\n"
        "child u0(.CLK(CLK), .IN1(IN1), .OUT1(OUT1));\n"
        "assign OUT2 = IN1; endmodule\n");
    check(count(result, "GJB10157.5.3.3") == 0,
          "attachment 5.3.3 permits top-level instances and direct identifier wire-through");
    const auto glueAssessment = std::ranges::find_if(result.standardAssessments,
        [](const auto& entry) { return entry.clauseId == "GJB10157.5.3.3"; });
    check(glueAssessment != result.standardAssessments.end() &&
          glueAssessment->state == analysis::StandardRuleState::ManualReviewRequired,
          "bounded top-level glue no finding is not certified compliance");
    result = analyzeVerilog(
        "module probe(input [3:0] IN1, output A, B, C);\n"
        "assign A = (IN1[0]); assign B = 1'b0; assign C = {IN1[1],IN1[0]}; endmodule\n");
    check(count(result, "GJB10157.5.3.3") == 2,
          "5.3.3 admits parenthesized identifier selects but reports constants and concatenations");
    {
        analysis::AnalysisOptions topOptions;
        topOptions.standardId = "GJB10157-2021";
        const std::vector<core::SourceFile> hierarchy{
            {"leaf.v", core::HdlLanguage::Verilog,
             "module leaf(input CLK,D, output reg Q); always @(posedge CLK) Q <= D; endmodule\n"},
            {"top.v", core::HdlLanguage::Verilog,
             "module top(input CLK,D, output Q); leaf u0(CLK,D,Q); endmodule\n"}
        };
        const auto hierarchyResult = analysis::Analyzer(topOptions).analyzeSources(hierarchy);
        check(count(hierarchyResult, "GJB10157.5.3.3") == 0,
              "5.3.3 excludes procedural logic inside an instantiated leaf module");
    }
    {
        analysis::AnalysisOptions hierarchyOptions;
        hierarchyOptions.standardId = "GJB10157-2021";
        const auto hierarchyResult = analysis::Analyzer(hierarchyOptions).analyzeSources({
            {"hierarchy.v", core::HdlLanguage::Verilog,
             "module top(input A, output Y); middle u0(A,Y); endmodule\n"
             "module middle(input A, output Y); wire T; assign T = A; leaf u1(T,Y); endmodule\n"
             "module leaf(input A, output reg Y); always @* Y = A; endmodule\n"}
        });
        check(count(hierarchyResult, "GJB10157.5.3.4") == 1,
              "attachment 5.3.4 reports one non-top intermediate module mixing an instance and logic");
        const auto violation = std::ranges::find_if(hierarchyResult.violations,
            [](const auto& entry) { return entry.ruleId == "GJB10157.5.3.4"; });
        check(violation != hierarchyResult.violations.end() &&
              violation->message == "除顶层模块外，上层模块建议仅包含子模块的例化." &&
              violation->severity == analysis::Severity::Warning &&
              violation->line == 2 && violation->column == 1,
              "5.3.4 preserves the native fixed message, Warning severity and module line-only location");
    }
    {
        analysis::AnalysisOptions hierarchyOptions;
        hierarchyOptions.standardId = "GJB10157-2021";
        const auto hierarchyResult = analysis::Analyzer(hierarchyOptions).analyzeSources({
            {"hierarchy.v", core::HdlLanguage::Verilog,
             "module top(input A, output Y); middle u0(A,Y); endmodule\n"
             "module middle(input A, output Y); leaf u1(A,Y); endmodule\n"
             "module leaf(input A, output reg Y); always @* Y = A; endmodule\n"}
        });
        check(count(hierarchyResult, "GJB10157.5.3.4") == 0,
              "attachment 5.3.4 permits an instance-only intermediate and logic-only leaf");
        const auto assessment = std::ranges::find_if(hierarchyResult.standardAssessments,
            [](const auto& entry) { return entry.clauseId == "GJB10157.5.3.4"; });
        check(assessment != hierarchyResult.standardAssessments.end() &&
              assessment->state == analysis::StandardRuleState::ManualReviewRequired,
              "5.3.4 no finding remains manual review because top selection and elaboration can be incomplete");
    }
    {
        analysis::AnalysisOptions hierarchyOptions;
        hierarchyOptions.standardId = "GJB10157-2021";
        const auto hierarchyResult = analysis::Analyzer(hierarchyOptions).analyzeSources({
            {"hierarchy.v", core::HdlLanguage::Verilog,
             "module top(input A, output Y); middle u0(A,Y); assign Y = A; endmodule\n"
             "module middle(input A, output Y); wire T = A; leaf u1(T,Y); endmodule\n"
             "module leaf(input A, output Y); assign Y = A; endmodule\n"}
        });
        check(count(hierarchyResult, "GJB10157.5.3.4") == 0,
              "5.3.4 skips selected top logic, declaration initialization and a logic-only leaf");
    }
    {
        analysis::AnalysisOptions hierarchyOptions;
        hierarchyOptions.standardId = "GJB10157-2021";
        const auto hierarchyResult = analysis::Analyzer(hierarchyOptions).analyzeSources({
            {"hierarchy-initial.sv", core::HdlLanguage::SystemVerilog,
             "module top(input A, output Y); middle u0(A,Y); endmodule\n"
             "module middle(input A, output Y); logic T; leaf u1(A,Y); "
             "(* keep = \"true\" *) initial begin : boot T = 1'b0; end endmodule\n"
             "module leaf(input A, output Y); assign Y = A; endmodule\n"}
        });
        check(count(hierarchyResult, "GJB10157.5.3.4") == 1,
              "5.3.4 treats a marked explicit initial_construct in an intermediate module as logic");
    }
    {
        analysis::AnalysisOptions hierarchyOptions;
        hierarchyOptions.standardId = "GJB10157-2021";
        const auto hierarchyResult = analysis::Analyzer(hierarchyOptions).analyzeSources({
            {"hierarchy-malformed.sv", core::HdlLanguage::SystemVerilog,
             "module top(input A, output Y); middle u0(A,Y); endmodule\n"
             "module middle(input A, output Y); leaf u1(A,Y); "
             "task broken; initial A = 1'b0; endtask endmodule\n"
             "module leaf(input A, output Y); assign Y = A; endmodule\n"}
        });
        check(count(hierarchyResult, "GJB10157.5.3.4") == 0,
              "5.3.4 does not promote an invalid task-local initial token to an AST_INITIAL root");
    }
    {
        analysis::AnalysisOptions configuredTopOptions;
        configuredTopOptions.standardId = "GJB10157-2021";
        configuredTopOptions.topModule = "top_a";
        const std::vector<core::SourceFile> multipleRoots{
            {"roots.sv", core::HdlLanguage::SystemVerilog,
             "module child(input A, output Y); assign Y=A; endmodule\n"
             "module top_a(input tri A, input B, output Y); assign Y=A&B; endmodule\n"
             "module top_b(input A, output Y); child u0(A,Y); assign Y=A; endmodule\n"}
        };
        auto configured = analysis::Analyzer(configuredTopOptions).analyzeSources(multipleRoots);
        check(configured.errors.empty() && count(configured, "GJB10157.5.3.2") == 1 &&
                  count(configured, "GJB10157.5.3.3") == 1 &&
                  count(configured, "GJB10157.5.3.4") == 1,
              "configured native top selects one root and treats every other module as non-top");
        check(std::ranges::any_of(configured.violations, [](const auto& item) {
                  return item.ruleId == "GJB10157.5.3.4" && item.line == 3 &&
                         item.message == "除顶层模块外，上层模块建议仅包含子模块的例化.";
              }), "configured top makes a second graph root eligible for non-top hierarchy checking");

        configuredTopOptions.topModule = "\\top_a";
        configured = analysis::Analyzer(configuredTopOptions).analyzeSources(multipleRoots);
        check(configured.errors.empty() && count(configured, "GJB10157.5.3.2") == 1,
              "native canonical leading backslash resolves to the lexer module name");

        configuredTopOptions.topModule = "missing";
        configured = analysis::Analyzer(configuredTopOptions).analyzeSources(multipleRoots);
        check(configured.errors.size() == 1 &&
                  configured.errors[0].find("指定的顶层设计不存在") != std::string::npos &&
                  count(configured, "GJB10157.5.3.2") == 0 &&
                  count(configured, "GJB10157.5.3.3") == 0 &&
                  count(configured, "GJB10157.5.3.4") == 0,
              "missing explicit top fails closed instead of guessing a graph root");
    }
    {
        analysis::AnalysisOptions hierarchyOptions;
        hierarchyOptions.standardId = "GJB10157-2021";
        hierarchyOptions.topModule = "top";
        const auto hierarchyResult = analysis::Analyzer(hierarchyOptions).analyzeSources({
            {"same-line-hierarchy.v", core::HdlLanguage::Verilog,
             "module leaf(input A,output Y);assign Y=A;endmodule module middle1(input A,output Y);leaf u1(A,Y);assign Y=A;endmodule module middle2(input A,output Y);leaf u2(A,Y);assign Y=A;endmodule module top(input A,output Y1,Y2);middle1 u1(A,Y1);middle2 u2(A,Y2);endmodule\n"}
        });
        check(count(hierarchyResult, "GJB10157.5.3.4") == 2,
              "5.3.4 retains one identical same-line diagnostic for every violating module AST");
    }
    result = analyzeVerilog(
        "module probe(input CLK, input [7:0] A1,A2,B, output C1,C2,C3,C5);\n"
        "reg int1; assign C1=A1[0]; assign C2=A1[0]&A2[0];\n"
        "always @(posedge CLK) int1 <= A1[2]; assign C3=int1&B[0];\n"
        "my_bb u0(.OUT1(C5)); endmodule\n");
    check(count(result, "GJB10157.5.3.5") == 3,
          "attachment 5.3.5 reports continuous output drivers and exempts a cell-driven output");
    check(std::ranges::all_of(result.violations, [](const auto& item) {
              if (item.ruleId != "GJB10157.5.3.5") return true;
              return item.severity == analysis::Severity::Warning && item.column == 1 &&
                     (item.message == "未寄存的模块输出端口C1" ||
                      item.message == "未寄存的模块输出端口C2" ||
                      item.message == "未寄存的模块输出端口C3");
          }), "5.3.5 uses the native fixed prefix, output name, Warning and line-only location");
    result = analyzeVerilog(
        "module probe(input CLK,D, output reg Q1,Q2, output Q3);\n"
        "always @(posedge CLK) begin Q1<=D; Q2<=Q1; end\n"
        "my_bb u0(.OUT1(Q3)); endmodule\n");
    check(count(result, "GJB10157.5.3.5") == 0,
          "attachment 5.3.5 permits edge-registered and cell-driven module outputs");
    const auto registeredOutputAssessment = std::ranges::find_if(result.standardAssessments,
        [](const auto& entry) { return entry.clauseId == "GJB10157.5.3.5"; });
    check(registeredOutputAssessment != result.standardAssessments.end() &&
          registeredOutputAssessment->state == analysis::StandardRuleState::ManualReviewRequired,
          "bounded registered-output no finding is not certified compliance");
    result = analyzeVerilog(
        "module probe(input D, output reg Q1, output logic [1:0] Q2);\n"
        "always @* Q1 = D; always_comb begin Q2[0] = D; Q2[1] <= Q1; end endmodule\n");
    check(count(result, "GJB10157.5.3.5") == 2,
          "5.3.5 reports each combinationally driven output, coalescing multiple selected bits on one line");
    check(std::ranges::any_of(result.violations, [](const auto& item) {
              return item.ruleId == "GJB10157.5.3.5" &&
                     item.message == "未寄存的模块输出端口Q2[0]";
          }), "5.3.5 excludes a selected bit driven only by a registered source");
    result = analyzeVerilog(
        "module probe(input CLK,D, output Q1,Q2); reg stage;\n"
        "always @(posedge CLK) stage <= D; assign Q1 = stage; assign Q2 = stage & D; endmodule\n");
    check(count(result, "GJB10157.5.3.5") == 1 &&
          std::ranges::any_of(result.violations, [](const auto& item) {
              return item.ruleId == "GJB10157.5.3.5" &&
                     item.message == "未寄存的模块输出端口Q2";
          }), "5.3.5 permits a direct registered source but reports a mixed registered/input expression");
    result = analyzeVerilog(
        "module probe(input D, output [3:0] Q); assign Q[2:1] = {D,D}; endmodule\n");
    check(count(result, "GJB10157.5.3.5") == 1 &&
          std::ranges::any_of(result.violations, [](const auto& item) {
              return item.ruleId == "GJB10157.5.3.5" &&
                     item.message == "未寄存的模块输出端口Q[2:1]";
          }), "5.3.5 formats one partial contiguous output range in native high-to-low order");
    result = analyzeVerilog(
        "module probe(input D, output [3:0] Q); "
        "assign Q[0]=D, Q[1]=D, Q[2]=D, Q[3]=D; endmodule\n");
    check(count(result, "GJB10157.5.3.5") == 1 &&
          std::ranges::any_of(result.violations, [](const auto& item) {
              return item.ruleId == "GJB10157.5.3.5" &&
                     item.message == "未寄存的模块输出端口Q";
          }), "5.3.5 merges same-line adjacent selected bits and omits a range for full-width coverage");
    result = analyzeVerilog(
        "module first(input D,output Q);assign Q=D;endmodule "
        "module second(input D,output Q);assign Q=D;endmodule\n");
    check(count(result, "GJB10157.5.3.5") == 2,
          "5.3.5 retains identical same-line diagnostics produced by distinct module runs");
    result = analyzeVerilog(
        "module probe(input CLK,ARST,D, output reg Q); reg clk_div2;\n"
        "always @(posedge ARST, posedge CLK) begin if(ARST) clk_div2<=0; else clk_div2<=~clk_div2; end\n"
        "always @(posedge clk_div2, posedge ARST) begin if(ARST) Q<=0; else Q<=D; end\n"
        "endmodule\n");
    check(count(result, "GJB10157.6.1.1") == 1,
          "attachment 6.1.1 reports a register output used as another process clock");
    const auto registerClockViolation = std::ranges::find_if(result.violations,
        [](const auto& entry) { return entry.ruleId == "GJB10157.6.1.1"; });
    check(registerClockViolation != result.violations.end() &&
          registerClockViolation->line == 2 &&
          registerClockViolation->column == 1 &&
          registerClockViolation->message ==
              "禁止将寄存器的输出clk_div2直接连接到其他寄存器的时钟管脚",
          "6.1.1 uses the native source-register line, line-only location and exact message");
    result = analyzeVerilog(
        "module probe(input CLK,D, output reg Q1,Q2); reg divided;\n"
        "always @(posedge CLK) divided<=~divided;\n"
        "always @(posedge divided) Q1<=D; always @(negedge divided) Q2<=D; endmodule\n");
    check(count(result, "GJB10157.6.1.1") == 1,
          "6.1.1 reports one diagnostic per source register output across multiple consumers");
    result = analyzeVerilog(
        "module probe(input CLK,ARST,D, output reg Q1,Q2); wire pll_clk;\n"
        "pll u0(.refclk(CLK),.outclk(pll_clk));\n"
        "always @(posedge CLK) Q1<=D; always @(posedge pll_clk) Q2<=Q1; endmodule\n");
    check(count(result, "GJB10157.6.1.1") == 0,
          "attachment 6.1.1 permits input clocks and instance-produced PLL clocks");
    const auto registerClockAssessment = std::ranges::find_if(result.standardAssessments,
        [](const auto& entry) { return entry.clauseId == "GJB10157.6.1.1"; });
    check(registerClockAssessment != result.standardAssessments.end() &&
          registerClockAssessment->state == analysis::StandardRuleState::ManualReviewRequired,
          "bounded register-clock no finding is not certified compliance");
    result = analyzeVerilog(
        "module probe(input CLK,D, output reg Q1,Q2); reg stage;\n"
        "always @(posedge CLK) stage<=D; always @(posedge CLK) begin Q1<=stage; Q2<=Q1; end\n"
        "endmodule\n");
    check(count(result, "GJB10157.6.1.1") == 0,
          "6.1.1 does not confuse ordinary register data fanout with a clock connection");
    result = analyzeVerilog(
        "module probe(input CLK,ARST,D, output reg [1:0] C1);\n"
        "always @(posedge CLK or posedge ARST) begin if(ARST) C1<=0; else begin C1[0]<=CLK; C1[1]<=D; end end\n"
        "endmodule\n");
    check(count(result, "GJB10157.6.1.2") == 1,
          "attachment 6.1.2 reports a clock identifier used in sequential assignment data");
    const auto clockDataViolation = std::ranges::find_if(result.violations,
        [](const auto& entry) { return entry.ruleId == "GJB10157.6.1.2"; });
    check(clockDataViolation != result.violations.end() &&
          clockDataViolation->line == 2 && clockDataViolation->column == 1 &&
          clockDataViolation->message == "时钟CLK驱动非时钟引脚.",
          "6.1.2 restores the native direct-RHS line-only diagnostic surface");
    result = analyzeVerilog(
        "module probe(input CLK,ARST,D, output reg [1:0] C1);\n"
        "always @(posedge CLK or posedge ARST) begin if(ARST) C1<=0; else begin C1[0]<=D; C1[1]<=C1[0]; end end\n"
        "endmodule\n");
    check(count(result, "GJB10157.6.1.2") == 0,
          "attachment 6.1.2 permits a clock used only as an edge event");
    const auto clockDataAssessment = std::ranges::find_if(result.standardAssessments,
        [](const auto& entry) { return entry.clauseId == "GJB10157.6.1.2"; });
    check(clockDataAssessment != result.standardAssessments.end() &&
          clockDataAssessment->state == analysis::StandardRuleState::ManualReviewRequired,
          "bounded clock-as-data no finding is not certified compliance");
    result = analyzeVerilog(
        "module probe(input CLK,D, output reg Q);\n"
        "always @(posedge CLK) begin if (CLK) Q<=D; else Q<=0; end endmodule\n");
    check(count(result, "GJB10157.6.1.2") == 0,
          "native 6.1.2 does not recursively classify a condition read as assignment data");
    result = analyzeVerilog(
        "module probe(input CLK,D, output reg Q);\n"
        "always @(posedge CLK) Q <= CLK & D; endmodule\n");
    check(count(result, "GJB10157.6.1.2") == 0,
          "native 6.1.2 compares only the assignment RHS root name, not compound descendants");
    result = analyzeVerilog(
        "module probe(input CLK, output Q); my_bb u(.clk(CLK),.datain(CLK),.q(Q)); endmodule\n");
    check(count(result, "GJB10157.6.1.2") == 0,
          "native 6.1.2 does not implement the packaged black-box data-port prose case");
    result = analyzeVerilog(
        "module probe(input CLK, output reg Q1,Q2);\n"
        "always @(posedge CLK) begin Q1<=CLK; Q2<=CLK; end endmodule\n");
    check(count(result, "GJB10157.6.1.2") == 2,
          "native 6.1.2 preserves one report per matching direct-RHS assignment");
    result = analyzeVerilog(
        "module probe(input CLK,EN,D, output reg Q); wire gated_clk;\n"
        "assign gated_clk = CLK & EN; always @(posedge gated_clk) Q<=D; endmodule\n");
    check(count(result, "GJB10157.6.1.3") == 1,
          "attachment 6.1.3 reports a combinational expression output used as a clock");
    const auto combinationalClockViolation = std::ranges::find_if(result.violations,
        [](const auto& entry) { return entry.ruleId == "GJB10157.6.1.3"; });
    check(combinationalClockViolation != result.violations.end() &&
          combinationalClockViolation->line == 2 &&
          combinationalClockViolation->column == 1 &&
          combinationalClockViolation->message ==
              "禁止将组合逻辑的输出gated_clk作为时钟信号",
          "6.1.3 restores native text, first candidate line and line-only location");
    result = analyzeVerilog(
        "module probe(input CLK,D, output reg Q); wire clock_alias;\n"
        "assign clock_alias = CLK; always @(posedge clock_alias) Q<=D; endmodule\n");
    check(count(result, "GJB10157.6.1.3") == 0,
          "native 6.1.3 excludes a direct identifier clock alias from combinational logic");
    const auto combinationalClockAssessment = std::ranges::find_if(result.standardAssessments,
        [](const auto& entry) { return entry.clauseId == "GJB10157.6.1.3"; });
    check(combinationalClockAssessment != result.standardAssessments.end() &&
          combinationalClockAssessment->state == analysis::StandardRuleState::ManualReviewRequired,
          "bounded combinational-clock no finding is not certified compliance");
    result = analyzeVerilog(
        "module probe(input CLK,EN,D, output reg Q); reg generated;\n"
        "always @* generated = CLK & EN; always @(posedge generated) Q<=D; endmodule\n");
    check(count(result, "GJB10157.6.1.3") == 1,
          "6.1.3 reports a combinational-process target used as a clock");
    result = analyzeVerilog(
        "module probe(input CLK,EN,D, output reg Q); wire generated;\n"
        "assign generated=CLK&EN; assign generated=CLK|EN;\n"
        "always @(posedge generated) Q<=D; endmodule\n");
    check(count(result, "GJB10157.6.1.3") == 1,
          "native 6.1.3 string-keyed candidates deduplicate multiple drivers of one clock name");
    result = analyzeVerilog(
        "module probe(input CLK,D, output reg Q1,Q2); wire inv_clk,arith_clk;\n"
        "assign inv_clk=~CLK; assign arith_clk=CLK+1'b0;\n"
        "always @(posedge inv_clk) Q1<=D; always @(posedge arith_clk) Q2<=D; endmodule\n");
    check(count(result, "GJB10157.6.1.3") == 2,
          "native 6.1.3 admits unary inversion and arithmetic expression clock drivers");
    result = analyzeVerilog(
        "module probe(input CLK,D, output reg Q); reg divided;\n"
        "always @(posedge CLK) divided<=~divided; always @(posedge divided) Q<=D; endmodule\n");
    check(count(result, "GJB10157.6.1.3") == 0,
          "6.1.3 excludes edge-triggered register outputs from its combinational predicate");
    result = analyzeVerilog(
        "module probe(input CLK,CLK2,D, output reg Q1,Q2,Q3,Q4,Q5,Q6); wire generated,inv;\n"
        "clock_bb u0(.CLKIN(CLK),.CLKOUT(generated)); assign inv=~CLK2;\n"
        "always @(posedge CLK) Q1<=D;\n"
        "always @(negedge CLK) Q2<=D;\n"
        "always @(posedge generated) Q3<=D;\n"
        "always @(negedge generated) Q4<=D;\n"
        "always @(posedge CLK2) Q5<=D;\n"
        "always @(posedge inv) Q6<=D; endmodule\n");
    check(count(result, "GJB10157.6.1.4") == 6,
          "native 6.1.4 reports both endpoints of direct, primitive-output and inverted-alias mixed edges");
    result = analyzeVerilog(
        "module probe(input CLK,D, output reg Q1,Q2);\n"
        "always @(posedge CLK) Q1<=D;\n"
        "always @(negedge CLK) Q2<=D; endmodule\n");
    std::vector<std::size_t> mixedEdgeLines;
    for (const auto& entry : result.violations) {
        if (entry.ruleId != "GJB10157.6.1.4") continue;
        mixedEdgeLines.push_back(entry.line);
        check(entry.column == 1 &&
              entry.message == "时钟CLK不能同时使用上升沿和下降沿",
              "6.1.4 restores native text and line-only locations");
    }
    std::ranges::sort(mixedEdgeLines);
    check(mixedEdgeLines == std::vector<std::size_t>{2, 3},
          "native 6.1.4 reports the two conflicting clock endpoints");
    result = analyzeVerilog(
        "module probe(input CLK,D, output reg Q1,Q2); "
        "always @(posedge CLK) Q1<=D; always @(negedge CLK) Q2<=D; endmodule\n");
    check(count(result, "GJB10157.6.1.4") == 2,
          "native 6.1.4 preserves two endpoint reports on one physical line");
    result = analyzeVerilog(
        "module probe(input CLK,D, output reg Q1,Q2,Q3);\n"
        "always @(posedge CLK) Q1<=D; always @(posedge CLK) Q2<=D;\n"
        "always @(negedge CLK) Q3<=D; endmodule\n");
    check(count(result, "GJB10157.6.1.4") == 2,
          "6.1.4 emits one retained endpoint per effective polarity");
    result = analyzeVerilog(
        "module probe(input CLK,D, output reg Q1,Q2); wire alias_clk; assign alias_clk=CLK;\n"
        "always @(posedge CLK) Q1<=D; always @(posedge alias_clk) Q2<=D; endmodule\n");
    check(count(result, "GJB10157.6.1.4") == 0,
          "6.1.4 permits one effective edge through a direct clock alias");
    const auto mixedClockAssessment = std::ranges::find_if(result.standardAssessments,
        [](const auto& entry) { return entry.clauseId == "GJB10157.6.1.4"; });
    check(mixedClockAssessment != result.standardAssessments.end() &&
          mixedClockAssessment->state == analysis::StandardRuleState::ManualReviewRequired,
          "bounded mixed-clock-edge no finding is not certified compliance");
    result = analyzeVerilog(
        "module probe(input CLK,EN,D, output reg Q); wire gated_clk;\n"
        "assign gated_clk=CLK & EN; always @(posedge gated_clk) Q<=D; endmodule\n");
    check(count(result, "GJB10157.6.1.6") == 1,
          "attachment 6.1.6 reports a binary AND gated clock");
    const auto gatedClockFinding = std::ranges::find_if(result.violations,
        [](const auto& entry) { return entry.ruleId == "GJB10157.6.1.6"; });
    check(gatedClockFinding != result.violations.end() &&
          gatedClockFinding->line == 2 && gatedClockFinding->column == 1 &&
          gatedClockFinding->message == "避免使用时钟门控.",
          "6.1.6 restores native fixed text and line-only location");
    result = analyzeVerilog(
        "module probe(input CLK,EN,D, output reg Q1,Q2); wire g1,g2; "
        "assign g1=CLK&EN; assign g2=CLK|EN; "
        "always @(posedge g1) Q1<=D; always @(posedge g2) Q2<=D; endmodule\n");
    check(count(result, "GJB10157.6.1.6") == 2,
          "native 6.1.6 preserves per-assignment multiplicity on one physical line");
    result = analyzeVerilog(
        "module probe(input CLK1,CLK2,SEL,D, output reg Q1,Q2); wire xor_clk,mux_clk;\n"
        "assign xor_clk=CLK1 ^ SEL; assign mux_clk=SEL ? CLK1 : CLK2;\n"
        "always @(posedge xor_clk) Q1<=D; always @(posedge mux_clk) Q2<=D; endmodule\n");
    check(count(result, "GJB10157.6.1.6") == 2,
          "native 6.1.6 binary-bitwise and conditional operator families are reported");
    result = analyzeVerilog(
        "module probe(input CLK,D, output reg Q1,Q2,Q3); wire alias_clk,inv_clk,arith_clk;\n"
        "assign alias_clk=CLK; assign inv_clk=~CLK; assign arith_clk=CLK+1'b0;\n"
        "always @(posedge alias_clk) Q1<=D; always @(posedge inv_clk) Q2<=D;\n"
        "always @(posedge arith_clk) Q3<=D; endmodule\n");
    check(count(result, "GJB10157.6.1.6") == 0,
          "6.1.6 excludes direct aliases, unary inversion and non-gating arithmetic");
    const auto gatedClockAssessment = std::ranges::find_if(result.standardAssessments,
        [](const auto& entry) { return entry.clauseId == "GJB10157.6.1.6"; });
    check(gatedClockAssessment != result.standardAssessments.end() &&
          gatedClockAssessment->state == analysis::StandardRuleState::ManualReviewRequired,
          "bounded gated-clock no finding is not certified compliance");
    result = analyzeVerilog(
        "module probe(input CLK,EN,D, output reg Q); reg gated_clk;\n"
        "always @* gated_clk=CLK && EN; always @(posedge gated_clk) Q<=D; endmodule\n");
    check(count(result, "GJB10157.6.1.6") == 1,
          "6.1.6 reports a procedural logical-operator gated clock");
    result = analyzeVerilog(
        "module probe(input CLK,CLK3,D, output reg Q1,Q2);\n"
        "always @(posedge CLK) Q1<=D; always @(negedge CLK3) Q2<=D; endmodule\n");
    check(count(result, "GJB10157.6.1.7") == 1,
          "attachment 6.1.7 reports a second distinct clock domain");
    const auto multipleClockFinding = std::ranges::find_if(result.violations,
        [](const auto& entry) { return entry.ruleId == "GJB10157.6.1.7"; });
    check(multipleClockFinding != result.violations.end() &&
          multipleClockFinding->line == 1 && multipleClockFinding->column == 1 &&
          multipleClockFinding->message == "建议仅使用一个时钟域",
          "6.1.7 restores native mode-zero text and module line-only location");
    result = analyzeVerilog(
        "\n\nmodule probe(input CLK1,CLK2,CLK3,D, output reg Q1,Q2,Q3);\n"
        "always @(posedge CLK1) Q1<=D; always @(posedge CLK2) Q2<=D;\n"
        "always @(posedge CLK3) Q3<=D; endmodule\n");
    check(count(result, "GJB10157.6.1.7") == 1 &&
          std::ranges::find_if(result.violations, [](const auto& entry) {
              return entry.ruleId == "GJB10157.6.1.7";
          })->line == 3,
          "native 6.1.7 emits once at the module for more than two direct domains");
    result = analyzeVerilog(
        "module probe(input CLK,D, output reg Q1,Q2,Q3); wire alias_clk,inv_clk;\n"
        "assign alias_clk=CLK; assign inv_clk=~CLK;\n"
        "always @(posedge CLK) Q1<=D; always @(negedge alias_clk) Q2<=D;\n"
        "always @(posedge inv_clk) Q3<=D; endmodule\n");
    check(count(result, "GJB10157.6.1.7") == 0,
          "6.1.7 treats opposite edges and bounded aliases of one signal as one domain");
    const auto singleClockAssessment = std::ranges::find_if(result.standardAssessments,
        [](const auto& entry) { return entry.clauseId == "GJB10157.6.1.7"; });
    check(singleClockAssessment != result.standardAssessments.end() &&
          singleClockAssessment->state == analysis::StandardRuleState::ProjectContextRequired,
          "bounded 6.1.7 no finding still requires project context");
    {
        analysis::AnalysisOptions selectedClockTop;
        selectedClockTop.standardId = "GJB10157-2021";
        selectedClockTop.topModule = "top_a";
        const std::vector<core::SourceFile> roots{
            {"clock-roots.sv", core::HdlLanguage::SystemVerilog,
             "module top_a(input CLK,D, output reg Q); "
             "always @(posedge CLK) Q<=D; endmodule\n"
             "module top_b(input CLK1,CLK2,D, output reg Q1,Q2); logic dead; "
             "always @(posedge CLK1) Q1<=D; always @(posedge CLK2) Q2<=D; "
             "always_comb dead=D; endmodule\n"}
        };
        auto selected = analysis::Analyzer(selectedClockTop).analyzeSources(roots);
        check(selected.errors.empty() && count(selected, "GJB10157.6.1.7") == 0 &&
                  count(selected, "GJB10157.5.2.10") == 0,
              "explicit Verilog top excludes clock-domain and write-only findings in another root");
        selectedClockTop.topModule = "top_b";
        selected = analysis::Analyzer(selectedClockTop).analyzeSources(roots);
        check(selected.errors.empty() && count(selected, "GJB10157.6.1.7") == 1 &&
                  count(selected, "GJB10157.5.2.10") == 1,
              "explicit Verilog top admits its own clock-domain and write-only findings");
        check(count(selected, "GJB10157.5.2.18") >= 1,
              "top-specific 5.2.10 filtering does not remove the broader redundancy clause mapping");
    }
    result = analyzeVerilog(
        "module probe(input CLK,RST,D, output reg Q1,Q2);\n"
        "always @(posedge RST or posedge CLK) if(RST) Q1<=0; else Q1<=D;\n"
        "always @(posedge CLK) if(RST) Q2<=0; else Q2<=D; endmodule\n");
    check(count(result, "GJB10157.6.2.2") == 1,
          "attachment 6.2.2 reports one reset used asynchronously and synchronously");
    {
        const auto finding = std::ranges::find_if(result.violations, [](const auto& violation) {
            return violation.ruleId == "GJB10157.6.2.2";
        });
        check(finding != result.violations.end() &&
                  finding->message == "RST同时作为了异步复位和同步复位" &&
                  finding->line == 3 && finding->column == 1,
              "6.2.2 restores native text and synchronous-condition line-only location");
    }
    result = analyzeVerilog(
        "module probe(input CLK,RST,D, output reg Q,Q2,Q3);\n"
        "always @(posedge RST or posedge CLK) if(RST) Q<=0; else Q<=D;\n"
        "always @(posedge CLK) if(RST) Q2<=0; else Q2<=D;\n"
        "always @(negedge CLK) if(RST) Q3<=0; else Q3<=D; endmodule\n");
    check(count(result, "GJB10157.6.2.2") == 2,
          "6.2.2 preserves one native finding per synchronous condition identifier");
    result = analyzeVerilog(
        "module probe(input CLK,RST,D, output reg Q,Q2);\n"
        "always @(posedge RST or posedge CLK) if(RST) Q<=0; else Q<=D;\n"
        "always @(posedge CLK) if(RST&&RST) Q2<=0; else Q2<=D; endmodule\n");
    check(count(result, "GJB10157.6.2.2") == 2,
          "6.2.2 retains duplicate identifier AST findings on one physical line");
    result = analyzeVerilog(
        "module probe(input CLK,RST,A,D, output reg Q,Q2);\n"
        "always @(posedge RST or posedge CLK) if(RST) Q<=0; else Q<=D;\n"
        "always @(posedge CLK) if(A) begin if(RST) Q2<=0; else Q2<=D; end endmodule\n");
    check(count(result, "GJB10157.6.2.2") == 0,
          "6.2.2 examines only the first native reduce-bool condition in a process");
    result = analyzeVerilog(
        "module probe(input CLK,RST,A,D, output reg Q,Q2);\n"
        "always @(posedge RST or posedge CLK) if(RST) Q<=0; else Q<=D;\n"
        "always @(posedge CLK) begin while(A) Q2<=D; if(RST) Q2<=0; end endmodule\n");
    check(count(result, "GJB10157.6.2.2") == 0,
          "6.2.2 does not substitute a later if for an earlier loop condition");
    result = analyzeVerilog(
        "module probe(input CLK,RST,RST2,D, output reg Q1,Q2,Q3);\n"
        "always @(posedge RST or posedge CLK) if(RST) Q1<=0; else Q1<=D;\n"
        "always @(posedge RST or posedge CLK) if(RST) Q2<=0; else Q2<=D;\n"
        "always @(posedge CLK) if(RST2) Q3<=0; else Q3<=D; endmodule\n");
    check(count(result, "GJB10157.6.2.2") == 0,
          "6.2.2 permits repeated asynchronous use and a different synchronous reset");
    const auto mixedResetAssessment = std::ranges::find_if(result.standardAssessments,
        [](const auto& entry) { return entry.clauseId == "GJB10157.6.2.2"; });
    check(mixedResetAssessment != result.standardAssessments.end() &&
          mixedResetAssessment->state == analysis::StandardRuleState::ManualReviewRequired,
          "bounded 6.2.2 no finding is not certified compliance");
    result = analyzeVerilog(
        "module probe(input CLK,RST,B, input [1:0] COND, output reg C,D); wire my_logic;\n"
        "always @(posedge RST or posedge CLK) if(RST) C<=0; else C<=B;\n"
        "always @(posedge CLK) D<=RST;\n"
        "my_bb u0(.CLK(CLK),.RST(RST),.DI(RST),.DO());\n"
        "assign my_logic=RST&COND[0]; endmodule\n");
    check(count(result, "GJB10157.6.2.1") == 3,
          "attachment 6.2.1 reports reset use as register data, BB data and logic input");
    {
        const auto finding = std::ranges::find_if(result.violations, [](const auto& violation) {
            return violation.ruleId == "GJB10157.6.2.1";
        });
        check(finding != result.violations.end() &&
                  finding->message == "异步复位/置位信号RST连接到非复位/置位端口" &&
                  finding->column == 1,
              "6.2.1 restores exact native text and line-only location");
    }
    result = analyzeVerilog(
        "module probe(input CLK,RST,D, output reg Q); wire X,Y;\n"
        "always @(posedge RST or posedge CLK) if(RST) Q<=0; else Q<=D;\n"
        "assign X=RST; assign Y=RST; endmodule\n");
    check(count(result, "GJB10157.6.2.1") == 2,
          "6.2.1 preserves one native finding per reset identifier AST on one line");
    result = analyzeVerilog(
        "module probe(input CLK,RST,RST2,D, output reg Q,Q2); wire X;\n"
        "always @(posedge RST or posedge CLK) if(RST) Q<=0; else Q<=D;\n"
        "always @(posedge RST2 or posedge CLK) if(RST2) Q2<=0; else Q2<=D;\n"
        "assign X=RST|RST2; endmodule\n");
    check(count(result, "GJB10157.6.2.1") == 2,
          "6.2.1 reports every asynchronous control identifier in one data expression");
    result = analyzeVerilog(
        "module probe(input CLK,RST,A,B, input [1:0] COND, output reg C,D); wire my_logic;\n"
        "always @(posedge RST or posedge CLK) if(RST) C<=0; else C<=B;\n"
        "always @(posedge CLK) D<=my_logic;\n"
        "my_bb u0(.CLK(CLK),.RST(RST),.DI(A),.DO());\n"
        "assign my_logic=B&COND[0]; endmodule\n");
    check(count(result, "GJB10157.6.2.1") == 0,
          "attachment 6.2.1 permits reset only on an asynchronous reset formal and guard");
    const auto resetEndpointAssessment = std::ranges::find_if(result.standardAssessments,
        [](const auto& entry) { return entry.clauseId == "GJB10157.6.2.1"; });
    check(resetEndpointAssessment != result.standardAssessments.end() &&
          resetEndpointAssessment->state == analysis::StandardRuleState::ManualReviewRequired,
          "bounded 6.2.1 no finding is not certified compliance");
    result = analyzeVerilog(
        "module probe(input CLK,RST,D, output reg Q);\n"
        "always @(posedge RST or posedge CLK) if(RST) Q<=0; else Q<=D;\n"
        "my_bb u0(CLK,RST,D); endmodule\n");
    check(count(result, "GJB10157.6.2.1") == 0,
          "6.2.1 leaves positional black-box port semantics unresolved");
    result = analyzeVerilog(
        "module probe(input CLK,A,B,D, output reg Q); wire my_comb_rst;\n"
        "assign my_comb_rst=A&B;\n"
        "always @(posedge my_comb_rst or posedge CLK) if(my_comb_rst) Q<=0; else Q<=D;\n"
        "endmodule\n");
    check(count(result, "GJB10157.6.2.3") == 1 &&
          count(result, "GJB10157.6.2.7") == 1,
          "attachment 6.2.3 and 6.2.7 report a combinational asynchronous reset");
    for (const auto clause : {std::string_view{"GJB10157.6.2.3"},
                              std::string_view{"GJB10157.6.2.7"}}) {
        const auto violation = std::ranges::find_if(result.violations,
            [&](const auto& entry) { return entry.ruleId == clause; });
        check(violation != result.violations.end() &&
              violation->message == "异步复位my_comb_rst用组合逻辑驱动" &&
              violation->severity == analysis::Severity::Warning &&
              violation->line == 2 && violation->column == 1,
              "R-5-3/A-5-2 preserve the shared native message, Warning and assignment line-only location");
    }
    result = analyzeVerilog(
        "module probe(input CLK,A,B,D, output reg Q); reg ff_rst;\n"
        "always @(posedge CLK) ff_rst<=A&B;\n"
        "always @(posedge ff_rst or posedge CLK) if(ff_rst) Q<=0; else Q<=D;\n"
        "endmodule\n");
    check(count(result, "GJB10157.6.2.3") == 0 &&
          count(result, "GJB10157.6.2.7") == 0,
          "attachment 6.2.3 and 6.2.7 permit an edge-triggered reset generator");
    result = analyzeVerilog(
        "module probe(input CLK,ARST,D, output reg Q1,Q2); wire alias_rst,inv_rst;\n"
        "assign alias_rst=ARST; assign inv_rst=~(ARST);\n"
        "always @(posedge alias_rst or posedge CLK) if(alias_rst) Q1<=0; else Q1<=D;\n"
        "always @(negedge inv_rst or posedge CLK) if(!inv_rst) Q2<=0; else Q2<=D;\n"
        "endmodule\n");
    check(count(result, "GJB10157.6.2.3") == 0 &&
          count(result, "GJB10157.6.2.7") == 0,
          "6.2.3 and 6.2.7 propagate direct and unary reset aliases without reporting");
    const auto resetLogicAssessment = std::ranges::find_if(result.standardAssessments,
        [](const auto& entry) { return entry.clauseId == "GJB10157.6.2.3"; });
    check(resetLogicAssessment != result.standardAssessments.end() &&
          resetLogicAssessment->state == analysis::StandardRuleState::ManualReviewRequired,
          "bounded 6.2.3 no finding is not certified compliance");
    result = analyzeVerilog(
        "module probe(input CLK,ARST,D, input [1:0] RST_BUS, output reg Q0,Q1,Q2); "
        "wire logical_rst,bit_rst,selected_rst;\n"
        "assign logical_rst=!(ARST); assign bit_rst=~ARST; "
        "assign selected_rst=~RST_BUS[0];\n"
        "always @(negedge logical_rst or posedge CLK) if(!logical_rst) Q0<=0; else Q0<=D;\n"
        "always @(negedge bit_rst or posedge CLK) if(!bit_rst) Q1<=0; else Q1<=D;\n"
        "always @(negedge selected_rst or posedge CLK) if(!selected_rst) Q2<=0; else Q2<=D;\n"
        "endmodule\n");
    check(count(result, "GJB10157.6.2.3") == 0 &&
          count(result, "GJB10157.6.2.7") == 0,
          "R-5-3/A-5-2 admit exact logical NOT, bitwise NOT and identifier-select alias roots");
    result = analyzeVerilog(
        "module probe(input CLK,ARST,D, input [1:0] RST_BUS, output reg Q0,Q1,Q2); "
        "wire nested_rst,called_rst,reduced_rst;\n"
        "function pass; input value; begin pass=value; end endfunction\n"
        "assign nested_rst=!!ARST; assign called_rst=!pass(ARST); "
        "assign reduced_rst=&RST_BUS;\n"
        "always @(posedge nested_rst or posedge CLK) if(nested_rst) Q0<=0; else Q0<=D;\n"
        "always @(posedge called_rst or posedge CLK) if(called_rst) Q1<=0; else Q1<=D;\n"
        "always @(posedge reduced_rst or posedge CLK) if(reduced_rst) Q2<=0; else Q2<=D;\n"
        "endmodule\n");
    check(count(result, "GJB10157.6.2.3") == 3 &&
          count(result, "GJB10157.6.2.7") == 3,
          "R-5-3/A-5-2 reject nested NOT, function-call and reduction roots as reset aliases");
    result = analyzeVerilog(
        "module probe(input CLK,A,B,D, output reg Q); wire logic_rst,alias_rst1,alias_rst2;\n"
        "assign logic_rst=A&B; assign alias_rst1=logic_rst; assign alias_rst2=~alias_rst1;\n"
        "always @(negedge alias_rst2 or posedge CLK) if(!alias_rst2) Q<=0; else Q<=D;\n"
        "endmodule\n");
    const auto transitiveResetFinding = std::ranges::find_if(
        result.violations, [](const auto& violation) {
            return violation.ruleId == "GJB10157.6.2.3";
        });
    check(count(result, "GJB10157.6.2.3") == 1 &&
          count(result, "GJB10157.6.2.7") == 1 &&
          transitiveResetFinding != result.violations.end() &&
          transitiveResetFinding->message.find("logic_rst") != std::string::npos,
          "R-5-3/A-5-2 follow mixed direct/inverted aliases to the upstream combinational driver");
    result = analyzeVerilog(
        "module probe(input CLK,A,B,D, output reg Q); reg logic_rst; wire alias_rst;\n"
        "always @* logic_rst=A&B; assign alias_rst=logic_rst;\n"
        "always @(posedge alias_rst or posedge CLK) if(alias_rst) Q<=0; else Q<=D;\n"
        "endmodule\n");
    check(count(result, "GJB10157.6.2.3") == 1 &&
          count(result, "GJB10157.6.2.7") == 1,
          "R-5-3/A-5-2 follow a continuous alias to an upstream combinational process driver");
    result = analyzeVerilog(
        "module probe(input CLK,D, output reg Q); wire alias_rst1,alias_rst2;\n"
        "assign alias_rst1=alias_rst2; assign alias_rst2=~alias_rst1;\n"
        "always @(posedge alias_rst1 or posedge CLK) if(alias_rst1) Q<=0; else Q<=D;\n"
        "endmodule\n");
    const auto cyclicResetAssessment = std::ranges::find_if(
        result.standardAssessments,
        [](const auto& entry) { return entry.clauseId == "GJB10157.6.2.3"; });
    check(count(result, "GJB10157.6.2.3") == 0 &&
          count(result, "GJB10157.6.2.7") == 0 &&
          cyclicResetAssessment != result.standardAssessments.end() &&
          cyclicResetAssessment->state == analysis::StandardRuleState::ManualReviewRequired,
          "R-5-3/A-5-2 terminate an alias cycle without certifying the unsupported clean path");
    result = analyzeVerilog(
        "module probe(input CLK,A,B,C,D, output reg Q); wire comb_rst;\n"
        "assign comb_rst=A&B; assign comb_rst=C&D;\n"
        "always @(posedge comb_rst or posedge CLK) if(comb_rst) Q<=0; else Q<=D;\n"
        "endmodule\n");
    check(count(result, "GJB10157.6.2.3") == 2 &&
          count(result, "GJB10157.6.2.7") == 2,
          "R-5-3/A-5-2 report every distinct same-line non-alias assignment node");
    result = analyzeVerilog(
        "module probe(input CLK,A,B,D, output reg Q0,Q1); "
        "wire logic_rst,alias_rst1,alias_rst2;\n"
        "assign logic_rst=A&B; assign alias_rst1=logic_rst; assign alias_rst2=~logic_rst;\n"
        "always @(posedge alias_rst1 or posedge CLK) if(alias_rst1) Q0<=0; else Q0<=D;\n"
        "always @(negedge alias_rst2 or posedge CLK) if(!alias_rst2) Q1<=0; else Q1<=D;\n"
        "endmodule\n");
    check(count(result, "GJB10157.6.2.3") == 1 &&
          count(result, "GJB10157.6.2.7") == 1,
          "R-5-3/A-5-2 visit one upstream driver once when two alias paths converge");
    result = analyzeVerilog(
        "module probe(input CLK,A,B,D, output reg Q); reg comb_rst;\n"
        "always @* comb_rst=A&B;\n"
        "always @(posedge comb_rst or posedge CLK) if(comb_rst) Q<=0; else Q<=D;\n"
        "endmodule\n");
    check(count(result, "GJB10157.6.2.3") == 1 &&
          count(result, "GJB10157.6.2.7") == 1,
          "6.2.3 and 6.2.7 report a combinational procedural reset driver");
    result = analyzeVerilog(
        "module probe(input CLK,A,D, output reg Q); reg my_ff_rst;\n"
        "always @(posedge CLK) begin\n"
        "  my_ff_rst<=A;\n"
        "end\n"
        "always @(posedge my_ff_rst or posedge CLK) if(my_ff_rst) Q<=0; else Q<=D;\n"
        "endmodule\n");
    const auto sequentialResetFinding = std::ranges::find_if(
        result.violations, [](const auto& violation) {
            return violation.ruleId == "GJB10157.6.2.8";
        });
    check(count(result, "GJB10157.6.2.8") == 1 &&
          sequentialResetFinding != result.violations.end() &&
          sequentialResetFinding->message == "异步复位my_ff_rst由时序逻辑驱动" &&
          sequentialResetFinding->severity == analysis::Severity::Warning &&
          sequentialResetFinding->line == 3 && sequentialResetFinding->column == 1 &&
          sequentialResetFinding->signal == std::optional<std::string>{"my_ff_rst"},
          "A-5-3 reproduces the native message, Warning and assignment line-only location");
    result = analyzeVerilog(
        "module probe(input CLK,A,B,D, output reg Q); reg my_ff_rst;\n"
        "always @(posedge CLK) begin\n"
        "  my_ff_rst<=A;\n"
        "  my_ff_rst=B;\n"
        "end\n"
        "always @(posedge my_ff_rst or posedge CLK) if(my_ff_rst) Q<=0; else Q<=D;\n"
        "endmodule\n");
    check(count(result, "GJB10157.6.2.8") == 2 &&
          std::ranges::count_if(result.violations, [](const auto& violation) {
              return violation.ruleId == "GJB10157.6.2.8" &&
                     (violation.line == 3 || violation.line == 4) && violation.column == 1;
          }) == 2,
          "A-5-3 preserves one native report per matching blocking/nonblocking assignment node");
    result = analyzeVerilog(
        "module probe(input CLK,A,D, output reg Q); reg my_ff_rst;\n"
        "always @(posedge CLK) if(my_ff_rst<=A) Q<=D;\n"
        "always @(posedge my_ff_rst or posedge CLK) if(my_ff_rst) Q<=0; else Q<=D;\n"
        "endmodule\n");
    check(count(result, "GJB10157.6.2.8") == 0,
          "A-5-3 does not mistake a less-than-or-equal condition for AST_ASSIGN_LE");
    result = analyzeVerilog(
        "module probe(input CLK,A,B,D, output reg Q); wire my_comb_rst;\n"
        "assign my_comb_rst=A&B;\n"
        "always @(posedge my_comb_rst or posedge CLK) if(my_comb_rst) Q<=0; else Q<=D;\n"
        "endmodule\n");
    check(count(result, "GJB10157.6.2.8") == 0,
          "attachment 6.2.8 combinational reset control stays outside the sequential predicate");
    result = analyzeVerilog(
        "module probe(input CLK,RST,D, output reg Q);\n"
        "always @(posedge RST or posedge CLK) if(RST) Q<=0; else Q<=D;\n"
        "endmodule\n");
    check(count(result, "GJB10157.6.2.8") == 0,
          "6.2.8 permits an external asynchronous reset without a local sequential writer");
    const auto internalResetAssessment = std::ranges::find_if(result.standardAssessments,
        [](const auto& entry) { return entry.clauseId == "GJB10157.6.2.8"; });
    check(internalResetAssessment != result.standardAssessments.end() &&
          internalResetAssessment->state == analysis::StandardRuleState::ManualReviewRequired,
          "bounded 6.2.8 no finding is not certified compliance");
    result = analyzeVerilog(
        "module probe(input CLK,RST,D,E, output reg C,F);\n"
        "always @(posedge RST or posedge CLK) begin if(RST==1'b0) C<=0; else C<=D; end\n"
        "always @(posedge CLK or negedge RST) begin if(RST==1'b1) F<=0; else F<=E; end\n"
        "endmodule\n");
    check(count(result, "GJB10157.7.1.3") == 2,
          "attachment 7.1.3 reports opposite reset edge and condition polarities");
    result = analyzeVerilog(
        "module probe(input CLK,RST,D,E, output reg C,F);\n"
        "always @(posedge RST or posedge CLK) begin if(1'b1==RST) C<=0; else C<=D; end\n"
        "always @(posedge CLK or negedge RST) begin if(!RST) F<=0; else F<=E; end\n"
        "endmodule\n");
    check(count(result, "GJB10157.7.1.3") == 0,
          "7.1.3 accepts matching reversed equality and negated active-low conditions");
    result = analyzeVerilog(
        "module probe(input CLK,RST,D, output reg Q1,Q2);\n"
        "always @(posedge RST or posedge CLK) begin if(!RST) Q1<=0; else Q1<=D; end\n"
        "always @(negedge RST or posedge CLK) begin if(RST) Q2<=0; else Q2<=D; end\n"
        "endmodule\n");
    check(count(result, "GJB10157.7.1.3") == 2,
          "7.1.3 compares plain and unary-negated conditions with event polarity");
    check(std::ranges::all_of(result.violations, [](const auto& item) {
              return item.ruleId != "GJB10157.7.1.3" ||
                  (item.severity == analysis::Severity::Warning &&
                   item.message == "复位信号的触发条件与进入条件必须一致.");
          }), "R-6-3 retains the native fixed Warning text");
    result = analyzeVerilog(
        "module probe(input CLK,RST,D, output reg Q1,Q2);\n"
        "always @(posedge RST or posedge CLK) begin if(!RST) Q1<=0; if(!RST) Q2<=0; end\n"
        "endmodule\n");
    check(count(result, "GJB10157.7.1.3") == 2,
          "R-6-3 retains independent contradictory IFs on the same physical line");
    check(std::ranges::all_of(result.violations, [](const auto& item) {
              return item.ruleId != "GJB10157.7.1.3" || item.column == 1;
          }), "R-6-3 maps the native line-only report to product column one");
    result = analyzeVerilog(
        "module probe(input CLK,RST,D, output reg Q1,Q2,Q3,Q4);\n"
        "always @(posedge RST or posedge CLK) begin\n"
        "  if (\n"
        "    RST ==\n"
        "    1'b0) Q1<=0;\n"
        "end\n"
        "always @(posedge RST or posedge CLK) begin\n"
        "  if (\n"
        "    1'b0 ==\n"
        "    RST) Q2<=0;\n"
        "end\n"
        "always @(negedge RST or posedge CLK) begin\n"
        "  if (\n"
        "    RST) Q3<=0;\n"
        "end\n"
        "always @(posedge RST or posedge CLK) begin\n"
        "  if (\n"
        "    !\n"
        "    RST) Q4<=0;\n"
        "end\n"
        "endmodule\n");
    std::vector<std::size_t> resetConditionLines;
    for (const auto& item : result.violations) {
        if (item.ruleId == "GJB10157.7.1.3") {
            resetConditionLines.push_back(item.line);
            check(item.column == 1, "R-6-3 multiline report has no invented native column");
        }
    }
    check(resetConditionLines == std::vector<std::size_t>{5, 9, 14, 18},
          "R-6-3 locates equality constants, plain identifier and logical NOT on their own lines");
    const std::pair<const char*, unsigned> resetLiteralValues[] = {
        {"0", 0}, {"1", 1}, {"2", 2}, {"255", 255},
        {"'b10", 2}, {"'d2", 2}, {"'h2", 2}, {"'o2", 2},
        {"8'b00000001", 1}, {"8'd0", 0}, {"1'b10", 0}, {"1'b11", 1},
        {"2'sb01", 1}, {"2'sb11", 0xffffffffU}, {"1'sb1", 0xffffffffU},
        {"8'shFF", 0xffffffffU}, {"8'HFF", 255}, {"33'h100000001", 1},
        {"33'h100000000", 0}, {"4294967297", 1}, {"4294967296", 0},
        {"4'd17", 1}, {"'0", 0}, {"'1", 1},
        {"'x", 0}, {"'z", 0}, {"'X", 0}, {"'Z", 0},
        {"4'bx001", 1}, {"4'b1x00", 8}, {"4'sb1x00", 0xfffffff8U},
        {"4'sbx001", 1}, {"8'hx1", 1}, {"8'h1x", 16}, {"8'hX?", 0},
        {"8'shFx", 0xfffffff0U}, {"8'shxf", 15}, {"12'shxf", 15},
        {"4'hxF", 15}, {"4'shxF", 0xffffffffU}, {"8'o?1", 1}, {"8'o1?", 8},
        {"8'bZZZZ0001", 1}, {"8'b0001ZZZZ", 16}, {"8'dx", 0}, {"8'dZ", 0},
        {"'bx1", 1}, {"33'hx00000001", 1}
    };
    for (const auto& [literal, value] : resetLiteralValues) {
        for (const auto edgeValue : {0U, 1U}) {
            for (const auto reversed : {false, true}) {
                const std::string condition = reversed
                    ? std::string(literal) + " == RST" : std::string("RST == ") + literal;
                const auto literalResult = analyzeVerilog(
                    std::string("module probe(input CLK,RST,D, output reg Q);\n") +
                    "always @(" + (edgeValue ? "posedge" : "negedge") +
                    " RST or posedge CLK) if(" + condition + ") Q<=0; else Q<=D;\n"
                    "endmodule\n");
                const auto context =
                    "R-6-3 compares native low32 integer, width truncation and sign extension: " +
                    condition + (edgeValue ? " posedge" : " negedge");
                check(count(literalResult, "GJB10157.7.1.3") == (value != edgeValue ? 1 : 0),
                      context.c_str());
            }
        }
    }
    result = analyzeVerilog(
        "module probe(input CLK,RST,D, output reg Q1,Q2);\n"
        "always @(posedge RST or posedge CLK) begin if(RST==4'bx001) Q1<=0; if(RST==-1) Q2<=0; end\n"
        "endmodule\n");
    check(count(result, "GJB10157.7.1.3") == 1,
          "R-6-3 preserves raw unknown literal cache and folds negative integer operands");
    struct ResetFoldCase { const char* declaration; const char* expression; unsigned cache; };
    const ResetFoldCase resetFoldCases[] = {
        {"localparam P=$clog2(2);", "P", 1},
        {"localparam P=$clog2(2);localparam K=$clog2(P+1);", "K", 1},
        {"", "$signed(1'b1)", 0xffffffffU}, {"", "$unsigned(4'shf)", 15},
        {"", "$signed(4'hf)+8'sh0", 0xffffffffU},
        {"", "$signed(4'hf)+8'h0", 15},
        {"localparam P=$signed(1'b1);", "P", 0xffffffffU},
        {"localparam P=$unsigned(1'sb1);", "P", 1},
        {"", "signed'(1'b1)", 0xffffffffU}, {"", "unsigned'(1'sb1)", 1},
        {"", "$clog2(0)", 0}, {"", "$clog2(1)", 0},
        {"", "$clog2(2)", 1}, {"", "$clog2(3)", 2},
        {"", "$clog2(1'sb1)", 0}, {"", "$clog2(8'shff)", 8},
        {"", "$clog2(64'hffffffffffffffff)", 64},
        {"", "$clog2(4'hf+4'h1)+64'h0", 0},
        {"", "$clog2($clog2(17))", 3},
        {"localparam signed [0:0] P=1'b1;", "$clog2(P)", 0},
        {"", "'0+8'h0", 0}, {"", "'1+8'h0", 1},
        {"", "'1+0", 1}, {"", "'1+1", 2},
        {"", "'1+'1", 0}, {"", "(~'1)+8'h0", 254},
        {"", "'1==8'h1", 1}, {"", "'1==8'hff", 0},
        {"", "'1<8'h2", 1}, {"", "{'1,1'b0}", 2},
        {"", "1 ? '1 : 8'h0", 1},
        {"localparam [7:0] P='1;", "P", 1},
        {"localparam P='1;", "P+8'h0", 1},
        {"localparam signed [0:0] P='1;", "P", 1},
        {"", "1-1", 0}, {"", "4'hf+4'h1", 0},
        {"", "(4'hf+4'h1)+8'h0", 16}, {"", "-4'sd1", 0xffffffffU},
        {"", "4'shf<4'sh1", 1}, {"", "4'shf<8'h1", 0},
        {"", "8'sh80>>>8", 0xffffffffU}, {"", "4294967295+1", 0},
        {"localparam [7:0] P=4'hf+4'h1;", "P", 16},
        {"localparam P=4'hf+4'h1;", "P", 0},
        {"localparam signed [3:0] P=4'hf;", "P", 15},
        {"localparam P=0,K=P+1;", "K", 1},
        {"", "2**0", 1}, {"", "4'h2**4'h4", 0},
        {"", "(4'h2**4'h4)+8'h0", 16}, {"", "8'shff**-3", 0xffffffffU},
        {"", "8'shfe**-2", 0}, {"", "2**3**2", 64},
        {"localparam [7:0] P=4'h2**4'h4;", "P", 16},
        {"", "{1'b0,1'b1}", 1}, {"", "{1'b1,1'b0}", 2},
        {"", "{2{1'b1}}", 3}, {"", "{2{1'b0}}", 0},
        {"", "{1{4'shf}}", 15}, {"", "{4'hf+4'h1,4'h1}", 1},
        {"localparam signed [7:0] P={4'hf,4'hf};", "P", 255},
        {"localparam N=2;", "{N{1'b1}}", 3},
        {"", "1 ? 0 : 1", 0}, {"", "0 ? 0 : 1", 1},
        {"", "64'h100000000 ? 0 : 1", 0},
        {"", "1 ? 4'shf : 8'sh0", 0xffffffffU},
        {"", "1 ? 4'shf : 8'h0", 15},
        {"", "1 ? 8'h1 : 4'h0/4'h0", 1},
        {"", "(1 ? 4'hf+4'h1 : 4'h0)+8'h0", 16},
        {"localparam P=0 ? 1 : 0 ? 2 : 3;", "P", 3},
        {"", "&8'hff", 1}, {"", "|8'h0", 0},
        {"", "'h1-1", 0}, {"", "'b10-1", 1},
        {"", "'h1+4'shf", 16}, {"", "'sh1+4'shf", 0},
        {"", "'shffffffff>>>31", 1}, {"", "'sh80000000<0", 0},
        {"", "'hffffffffffffffff+64'h1", 0},
        {"localparam P='shffffffff;", "P>>>31", 1},
        {"localparam [0:0] P=1'sb1;", "P", 0xffffffffU},
        {"localparam signed [0:0] P=1'b1;", "P", 1},
        {"localparam [0:0] P=1'sb1;", "P+1'b0", 1},
        {"localparam signed [0:0] P=1'b1;", "+P", 0xffffffffU},
        {"localparam signed P=1'b1;", "P", 1},
        {"localparam signed P=1'b1;", "P+1'sb0", 0xffffffffU},
        {"localparam signed P=1'b1;", "P<1'sb0", 0},
        {"localparam P=1'sb1;", "P", 0xffffffffU},
        {"localparam P=1'sb1;", "P+1'sb0", 1},
        {"localparam P=1'sb1;", "P<1'sb0", 1},
        {"localparam P=4'shf;", "P/4'sh2", 7},
        {"localparam P=4'shf;", "(P+4'sh0)<4'sh0", 0}
    };
    for (const auto& test : resetFoldCases) {
        for (const auto edge : {0U, 1U}) {
            for (const bool reversed : {false, true}) {
                const std::string condition = reversed ? std::string("(") + test.expression + ") == RST"
                    : std::string("RST == (") + test.expression + ")";
                const auto foldedResult = analyzeVerilog(
                    std::string("module probe(input CLK,RST,D,output reg PRIME,Q);\n") +
                    test.declaration + "\n" +
                    "always @(posedge CLK or posedge RST) if(RST) PRIME<=0; else PRIME<=D;\n" +
                    "always @(" + (edge ? "posedge" : "negedge") + " RST) if(" + condition +
                    ") Q<=0; else Q<=D;\nendmodule\n");
                const auto context = std::string("R-6-3 simplified integer cache: ") + condition;
                check(count(foldedResult, "GJB10157.7.1.3") == (test.cache != edge ? 1 : 0),
                      context.c_str());
            }
        }
    }
    result = analyzeVerilog(
        "module probe(input CLK,RST,D,output reg PRIME,Q1,Q2);\n"
        "always @(posedge CLK or posedge RST) if(RST) PRIME<=0; else PRIME<=D;\n"
        "always @(posedge RST) begin\n"
        "if(RST==\n"
        "$clog2(\n"
        "1)) Q1<=0;\n"
        "if(\n"
        "$clog2(1)\n"
        "==RST) Q2<=0;\n"
        "end\nendmodule\n");
    resetConditionLines.clear();
    for (const auto& item : result.violations) {
        if (item.ruleId != "GJB10157.7.1.3") continue;
        resetConditionLines.push_back(item.line);
        check(item.column == 1, "R-6-3 folded builtin keeps native no-column reporting");
    }
    check(resetConditionLines == std::vector<std::size_t>{5, 8},
          "R-6-3 folded builtin retains call line instead of argument or enclosing IF line");
    result = analyzeVerilog(
        "module probe(input CLK,RST,D,output reg PRIME,Q1,Q2);\n"
        "always @(posedge CLK or posedge RST) if(RST) PRIME<=0; else PRIME<=D;\n"
        "always @(posedge RST) begin\n"
        "if(RST==\n"
        "$clog2\n"
        "(\n"
        "1)) Q1<=0;\n"
        "if(RST==\n"
        "$unsigned(\n"
        "1'b0\n"
        ")) Q2<=0;\n"
        "end\nendmodule\n");
    resetConditionLines.clear();
    for (const auto& item : result.violations)
        if (item.ruleId == "GJB10157.7.1.3") resetConditionLines.push_back(item.line);
    check(resetConditionLines == std::vector<std::size_t>{6, 11},
          "R-6-3 default grammar uses FCALL opening and sign-conversion closing token lines");
    for (const auto* declaration : {"parameter P=0;", "parameter K=0;localparam P=K;",
             "localparam P=1'bx+1;", "localparam P=1/0;", "localparam P=65'h1+0;",
             "localparam P=0**-1;", "parameter N=2;localparam P={N{1'b0}};",
             "localparam P={65{1'b0}};", "localparam P={1'bx,1'b0};",
             "parameter K=1;localparam P=K ? 0 : 1;", "localparam P=1'bx ? 0 : 1;",
             "localparam P=$clog2(1'bx);", "localparam P=$clog2(1/0);",
             "parameter N=2;localparam P=$clog2(N);", "localparam P=$signed(MISSING);",
             "localparam P='shffffffffffffffff+0;", "localparam P='x+1;", "localparam P='z+1;"}) {
        result = analyzeVerilog(std::string("module probe(input CLK,RST,D,output reg PRIME,Q);\n") +
            declaration + "\nalways @(posedge CLK or posedge RST) if(RST) PRIME<=0; else PRIME<=D;\n"
            "always @(posedge RST) if(RST==P) Q<=0; else Q<=D;\nendmodule\n");
        check(count(result, "GJB10157.7.1.3") == 0,
              "R-6-3 does not invent a cache for overridable or unsupported expressions");
    }
    result = analyzeVerilog(
        "module probe(input CLK,RST,D,output reg PRIME,Q1,Q2);\n"
        "localparam P=0;\n"
        "always @(posedge CLK or posedge RST) if(RST) PRIME<=0; else PRIME<=D;\n"
        "always @(posedge RST) if(RST==\nP) Q1<=0;\n"
        "always @(posedge RST) if(RST==\n-\n1) Q2<=0;\nendmodule\n");
    resetConditionLines.clear();
    for (const auto& item : result.violations)
        if (item.ruleId == "GJB10157.7.1.3") resetConditionLines.push_back(item.line);
    check(resetConditionLines == std::vector<std::size_t>{5, 7},
          "R-6-3 locates folded localparam use and unary source, not the declaration");
    result = analyzeVerilog(
        "module probe(input CLK,RST,D,output reg PRIME,Q);\n"
        "always @(posedge CLK or posedge RST) if(RST) PRIME<=0; else PRIME<=D;\n"
        "always @(posedge RST) if(RST==\n{\n1'b0,1'b0}) Q<=0;\nendmodule\n");
    resetConditionLines.clear();
    for (const auto& item : result.violations)
        if (item.ruleId == "GJB10157.7.1.3") resetConditionLines.push_back(item.line);
    check(resetConditionLines == std::vector<std::size_t>{4},
          "R-6-3 locates a folded concatenation at its opening brace");
    result = analyzeVerilog(
        "module probe(input CLK,RST,D,output reg PRIME,Q);\n"
        "always @(posedge CLK or posedge RST) if(RST) PRIME<=0; else PRIME<=D;\n"
        "always @(posedge RST) if(RST==(\n1\n? 0\n: 1)) Q<=0;\nendmodule\n");
    resetConditionLines.clear();
    for (const auto& item : result.violations)
        if (item.ruleId == "GJB10157.7.1.3") resetConditionLines.push_back(item.line);
    check(resetConditionLines == std::vector<std::size_t>{4},
          "R-6-3 locates the folded conditional node rather than the selected arm");
    for (const auto* condition : {"RST == 1 ? 0 : 1", "RST == 1 && 0", "RST == 1 || 0",
             "RST == 1 | 0", "RST == 1 ^ 0", "RST == 1 & 0", "RST == 1 === 0"}) {
        result = analyzeVerilog(std::string("module probe(input CLK,RST,D,output reg PRIME,Q);\n") +
            "always @(posedge CLK or posedge RST) if(RST) PRIME<=0; else PRIME<=D;\n" +
            "always @(posedge RST) if(" + condition + ") Q<=0;\nendmodule\n");
        check(count(result, "GJB10157.7.1.3") == 0,
              "R-6-3 requires an equality AST root rather than an embedded equality token");
    }
    result = analyzeVerilog(
        "module probe(input CLK,RST,D,output reg PRIME,Q);\n"
        "always @(posedge CLK or posedge RST) if(RST) PRIME<=0; else PRIME<=D;\n"
        "always @(posedge RST) if(1 == 0 == RST) Q<=0;\nendmodule\n");
    check(count(result, "GJB10157.7.1.3") == 1,
          "R-6-3 simplifies a constant comparison below the final equality root");
    result = analyzeVerilog(
        "module probe(input CLK,RST,D,output reg Q,OTHER);\n"
        "always @(posedge CLK or posedge RST) if(RST) Q<={1'b0}; else Q<=D;\n"
        "always @(posedge CLK) if(!CLK) OTHER<=0; else OTHER<=D;\nendmodule\n");
    check(count(result, "GJB10157.7.1.3") == 0,
          "R-6-3 recovered concatenation data closes the simple reset catalog and excludes CLK");
    // Establish an admitted reset name in a separate multi-edge process.
    // The condition under test is in a single-edge process, so its shape
    // does not have to establish its own asynchronous-reset catalog entry.
    const std::pair<const char*, bool> resetPrimaryConditions[] = {
        {"!(RST)", true}, {"!(((RST)))", true},
        {"(RST) == (1'b0)", true}, {"((1'b0)) == (((RST)))", true},
        {"((0 : RST : 1)) == (0)", true}, {"!((0 : RST : 1))", true},
        {"(RST)==(1:0:1)", true}, {"((0 : (RST == 0) : 1))", true},
        {"((1 : (RST == 1) : 0))", false}, {"((0 : RST : 1))", false},
        {"(RST)==(0:1:0)", false}, {"!((1:0:0))", false},
        {"!(RST == 1)", false}, {"RST == (EN ? 0 : 1)", false},
        {"(RST && EN) == 0", false}, {"((1?0:1 : RST :0)) == 0", true},
        {"((0 : (EN ? RST : EN) : 1))", false}, {"((RST ==0) && EN)", false},
        {"((1 : !((RST)) : 0))", true}, {"((1:~RST:0))", false}
    };
    for (const auto& [condition, expected] : resetPrimaryConditions) {
        const auto primaryResult = analyzeVerilog(
            std::string("module probe(input CLK,EN,D,RST, output reg Q,PRIME);\n") +
            "always @(posedge CLK or posedge RST) if(RST) PRIME<=0; else PRIME<=D;\n" +
            "always @(posedge RST) if(" + condition + ") Q<=0; else Q<=D;\n"
            "endmodule\n");
        const auto context = std::string("R-6-3 preserves native primary/typical expression: ") + condition;
        check(count(primaryResult, "GJB10157.7.1.3") == (expected ? 1 : 0), context.c_str());
    }
    const std::pair<const char*, bool> resetEventExpressions[] = {
        {"RST", true}, {"(RST)", true}, {"((RST))", true}, {"RST[0]", true},
        {"(RST[0])", true}, {"(0:RST:1)", true}, {"RST & EN", false},
        {"(RST & EN)", false}, {"RST | EN", false}, {"(RST ? EN : CLK)", false},
        {"!RST", false}, {"{RST}", false}, {"RST[0] & EN", false}
    };
    for (const auto& [event, expected] : resetEventExpressions) {
        const auto eventResult = analyzeVerilog(
            std::string("module probe(input CLK,EN,D, input [0:0] RST, output reg Q,PRIME);\n") +
            "always @(posedge CLK or posedge RST) if(RST) PRIME<=0; else PRIME<=D;\n" +
            "always @(posedge " + event + ") if(!RST) Q<=0; else Q<=D;\n"
            "endmodule\n");
        const auto context = std::string("R-6-3 admits only native identifier event nodes: ") + event;
        check(count(eventResult, "GJB10157.7.1.3") == (expected ? 1 : 0), context.c_str());
    }
    for (const auto& event : {"RST", "RST[0]", "RST[1]", "RST[1:0]"}) {
        for (const auto& condition : {"!RST", "!RST[0]", "RST[0]==0", "0==RST[1]"}) {
            result = analyzeVerilog(
                std::string("module probe(input CLK,D, input [1:0] RST, output reg Q,PRIME);\n") +
                "always @(posedge CLK or posedge RST[0]) if(RST[0]) PRIME<=0; else PRIME<=D;\n" +
                "always @(posedge " + event + ") if(" + condition + ") Q<=0; else Q<=D;\n" +
                "endmodule\n");
            check(count(result, "GJB10157.7.1.3") == 0 &&
                  std::ranges::any_of(result.standardAssessments, [](const auto& item) {
                      return item.clauseId == "GJB10157.7.1.3" &&
                             item.state == analysis::StandardRuleState::ManualReviewRequired;
                  }),
                  "R-6-3 does not confuse formatted vector slice reset names with AST base names");
        }
    }
    result = analyzeVerilog(
        "module probe(input CLK,D,RST, input [1:0] BUS_RST, output reg Q,P,X);\n"
        "always @(posedge CLK or posedge RST) if(RST) P<=0; else P<=D;\n"
        "always @(posedge CLK or posedge BUS_RST[0]) if(BUS_RST[0]) Q<=0; else Q<=D;\n"
        "always @(posedge RST) if(!RST) X<=0; else X<=D;\n"
        "endmodule\n");
    check(count(result, "GJB10157.7.1.3") == 1 &&
          std::ranges::any_of(result.violations, [](const auto& item) {
              return item.ruleId == "GJB10157.7.1.3" && item.line == 4;
          }), "R-6-3 vector-name filtering preserves an independent scalar reset diagnostic");
    result = analyzeVerilog(
        "module probe(input CLK,RST,D, output reg Q);\n"
        "always @(posedge (RST) or posedge CLK) begin\n"
        "if(\n"
        " (RST) == (\n"
        " 1'b0)) Q<=0;\n"
        "if(\n"
        " !\n"
        " (RST)) Q<=0;\n"
        "if((1:\n"
        " !RST\n"
        " :0)) Q<=0;\n"
        "end\n"
        "endmodule\n");
    resetConditionLines.clear();
    for (const auto& item : result.violations) {
        if (item.ruleId == "GJB10157.7.1.3") resetConditionLines.push_back(item.line);
    }
    check(resetConditionLines == std::vector<std::size_t>{5, 7, 10},
          "R-6-3 keeps underlying constant/unary source lines through primary wrappers");
    result = analyzeVerilog(
        "module probe(input CLK,D, output reg Q1,Q2);\n"
        "always @(posedge CLK) if(!CLK) Q1<=0; else Q1<=D;\n"
        "always @(negedge CLK) if(CLK==1) Q2<=0; else Q2<=D;\n"
        "endmodule\n");
    check(count(result, "GJB10157.7.1.3") == 0,
          "R-6-3 does not invent a module reset catalog from separate single-edge processes");
    result = analyzeVerilog(
        "module plain(input CLK,D, output reg Q);\n"
        "always @(posedge CLK) if(!CLK) Q<=0; else Q<=D; endmodule\n"
        "module reset_design(input CLK,RST,D, output reg Q);\n"
        "always @(posedge CLK or posedge RST) if(!RST) Q<=0; else Q<=D; endmodule\n");
    check(count(result, "GJB10157.7.1.3") == 1 &&
          std::ranges::any_of(result.violations, [](const auto& item) {
              return item.ruleId == "GJB10157.7.1.3" && item.line == 4;
          }), "R-6-3 keeps module catalog admission local in source order");
    result = analyzeVerilog(
        "module reset_design(input CLK,RST,D, output reg Q);\n"
        "always @(posedge CLK or posedge RST) if(!RST) Q<=0; else Q<=D; endmodule\n"
        "module plain(input CLK,D, output reg Q);\n"
        "always @(posedge CLK) if(!CLK) Q<=0; else Q<=D; endmodule\n");
    check(count(result, "GJB10157.7.1.3") == 1 &&
          std::ranges::any_of(result.violations, [](const auto& item) {
              return item.ruleId == "GJB10157.7.1.3" && item.line == 2;
          }), "R-6-3 does not leak a preceding module reset catalog into the next module");
    result = analyzeVerilog(
        "module probe(input CLK,RST,D, output reg Q1,Q2);\n"
        "always @(posedge CLK or posedge RST) if(RST) Q1<=0; else Q1<=D;\n"
        "always @(posedge RST) if(!RST) Q2<=0; else Q2<=D;\n"
        "endmodule\n");
    check(count(result, "GJB10157.7.1.3") == 1,
          "R-6-3 allows a known reset name in another single-edge process of the same module");
    result = analyzeVerilog(
        "module probe(input CLK,RST,D, output reg Q1,Q2);\n"
        "always @(posedge CLK or posedge RST) if(RST) Q1<=0; else Q1<=D;\n"
        "always @(posedge CLK) if(!CLK) Q2<=0; else Q2<=D;\n"
        "endmodule\n");
    check(count(result, "GJB10157.7.1.3") == 0,
          "R-6-3 filters a clock name absent from the recovered module reset catalog");
    result = analyzeVerilog(
        "module probe(input CLK,RST,D, output reg Q1,Q2);\n"
        "always @(posedge CLK) if(!CLK) Q2<=0; else Q2<=D;\n"
        "always @(posedge CLK or posedge RST) begin : reset_logic\n"
        " if(!(RST)) begin Q1<=0; end else begin Q1<=D; end\n"
        "end endmodule\n");
    check(count(result, "GJB10157.7.1.3") == 1 &&
          std::ranges::any_of(result.violations, [](const auto& item) {
              return item.ruleId == "GJB10157.7.1.3" && item.line == 4;
          }), "R-6-3 resolves the complete simple module before scanning source-order conditions");
    check(result.structure.size() == 1 && result.structure.front().instances.empty(),
          "a named procedural block followed by IF does not create a fake module instance");
    const auto escapedInstanceUnits = parser::parseDesignUnits(
        {"escaped_instance.v", core::HdlLanguage::Verilog,
         "module leaf(input A); endmodule\n"
         "module top(input A); leaf \\if (.A(A)); endmodule\n"});
    check(escapedInstanceUnits.size() == 2 && escapedInstanceUnits[1].instances.size() == 1 &&
          escapedInstanceUnits[1].instances.front().target == "leaf",
          "an escaped keyword remains a legal instance identifier");
    result = analyzeVerilog(
        "module probe(input CLK,R1,R2,D, output reg Q1,Q2,Q3,Q4);\n"
        "always @(posedge CLK or posedge R1) if(R1) Q1<=0; else Q1<=D;\n"
        "always @(posedge CLK or posedge R2) if(R2) Q2<=0; else Q2<=D;\n"
        "always @(posedge CLK) if(!CLK) Q3<=0; else Q3<=D;\n"
        "always @(posedge R2) if(!R2) Q4<=0; else Q4<=D;\n"
        "endmodule\n");
    check(count(result, "GJB10157.7.1.3") == 1 &&
          std::ranges::any_of(result.violations, [](const auto& item) {
              return item.ruleId == "GJB10157.7.1.3" && item.line == 5;
          }), "R-6-3 unions reset names from independent processes without admitting the clock");
    result = analyzeVerilog(
        "module probe(input CLK,RST,EN,D, output reg Q1,Q2);\n"
        "always @(posedge CLK or posedge RST) if(EN) Q1<=0; else Q1<=D;\n"
        "always @(posedge RST) if(!RST) Q2<=0; else Q2<=D;\n"
        "endmodule\n");
    check(count(result, "GJB10157.7.1.3") == 0,
          "R-6-3 does not create a reset name from an unrelated scalar input root condition");
    result = analyzeVerilog(
        "module probe(input CLK,RST,D1,D2, output reg Q1,Q2,Q3);\n"
        "always @(posedge CLK or posedge RST) begin\n"
        " if(RST) begin Q1<=0; Q2<=0; end\n"
        " else begin Q2<=D2; Q1<=D1; end\n"
        "end\n"
        "always @(posedge CLK) if(!CLK) Q3<=0; else Q3<=D1;\n"
        "endmodule\n");
    check(count(result, "GJB10157.7.1.3") == 0,
          "R-6-3 closes multi-register reset branches by target name independently of assignment order");
    result = analyzeVerilog(
        "module probe(input CLK,RST,D1,D2, output reg Q1,Q2,Q3,Q4);\n"
        "always @(posedge CLK or negedge RST) begin\n"
        " if(!RST) begin Q1<=0; begin Q2<=0; end end\n"
        " else begin ; Q2<=D2; Q1<=D1; end\n"
        "end\n"
        "always @(posedge CLK) if(!CLK) Q3<=0; else Q3<=D1;\n"
        "always @(posedge RST) if(!RST) Q4<=0; else Q4<=D2;\n"
        "endmodule\n");
    check(count(result, "GJB10157.7.1.3") == 1 &&
          std::ranges::any_of(result.violations, [](const auto& item) {
              return item.ruleId == "GJB10157.7.1.3" && item.line == 7;
          }), "R-6-3 retains reset names through nested assignment blocks and empty statements");
    result = analyzeVerilog(
        "module probe(input CLK,R1,R2,D, output reg A,B,C,E,F,G);\n"
        "always @(posedge CLK or posedge R1) if(R1) begin A<=0;B<=0;end else begin A<=D;B<=D;end\n"
        "always @(posedge CLK or negedge R2) if(~R2) begin C<=0;E<=0;end else begin E<=D;C<=D;end\n"
        "always @(posedge CLK) if(!CLK) F<=0; else F<=D;\n"
        "always @(posedge R2) if(!R2) G<=0; else G<=D;\n"
        "endmodule\n");
    check(count(result, "GJB10157.7.1.3") == 1 &&
          std::ranges::any_of(result.violations, [](const auto& item) {
              return item.ruleId == "GJB10157.7.1.3" && item.line == 5;
          }), "R-6-3 unions multi-register high/low reset roots while excluding their shared clock");
    result = analyzeVerilog(
        "module probe(input CLK,RST, input [7:0] D, output reg [7:0] Q, output reg X);\n"
        "always @(posedge CLK or posedge RST) if(RST) Q<=8'h00; else Q<=D;\n"
        "always @(posedge CLK) if(!CLK) X<=0; else X<=1;\n"
        "endmodule\n");
    check(count(result, "GJB10157.7.1.3") == 0,
          "R-6-3 admits a fixed-width vector reset target without treating the clock as reset");
    result = analyzeVerilog(
        "module probe(input CLK,RST, input [15:0] D, output reg X);\n"
        "reg [15:0] Q; reg [0:7] Q2; reg [0:7] D2;\n"
        "always @(posedge CLK or negedge RST) begin\n"
        " if(!RST) begin Q<=16'h0000;Q2<=8'hff;end else begin Q2<=D2;Q<=D;end end\n"
        "always @(posedge CLK) if(!CLK) X<=0; else X<=1;\n"
        "always @(posedge RST) if(!RST) Q<=0; else Q<=D;\n"
        "endmodule\n");
    check(count(result, "GJB10157.7.1.3") == 1 &&
          std::ranges::any_of(result.violations, [](const auto& item) {
              return item.ruleId == "GJB10157.7.1.3" && item.line == 6;
          }), "R-6-3 retains the scalar reset name across mixed-width vector target pairs");
    for (const auto& [condition, edge, findings] :
         std::vector<std::tuple<std::string, std::string, int>>{
             {"RST == 1'b1", "posedge", 1},
             {"RST == 1'b0", "negedge", 1},
             {"1'b1 == RST", "posedge", 1},
             {"1'b0 == RST", "negedge", 1},
             {"RST != 1'b0", "posedge", 1},
             {"1'b1 != RST", "negedge", 1},
             {"RST === 1'b1", "posedge", 1},
             {"1'b0 === RST", "negedge", 1},
             {"RST !== 1'b0", "posedge", 1},
             {"1'b1 !== RST", "negedge", 1},
             {"!(RST == 1'b0)", "posedge", 1},
             {"~(1'b0 !== RST)", "posedge", 1},
             {"(RST == 1'b1) == 1'b1", "posedge", 1},
             {"!!RST", "posedge", 1},
             {"!~RST", "posedge", 1},
             {"RST == 1'h1", "posedge", 1},
             {"RST == 1'o1", "posedge", 1},
             {"RST == 1'd1", "posedge", 1},
             {"RST == 1'sb1", "posedge", 2},
             {"RST == 1'bx", "negedge", 1},
             {"RST === 1'bz", "negedge", 1},
             {"RST !== 1'b?", "posedge", 1},
             {"RST == 1", "posedge", 1},
             {"RST == 2'b01", "posedge", 1},
             {"RST == 32'b0", "negedge", 1},
             {"RST == 8'h80", "posedge", 2},
             {"RST == 33'h100000000", "negedge", 1},
             {"RST == '1", "posedge", 1},
             {"RST == '0", "negedge", 1},
             {"RST == 'x", "negedge", 1},
             {"RST == K", "posedge", 1},
             {"RST == (1+0)", "posedge", 1},
             {"RST == (3-3)", "negedge", 1},
             {"RST != (1-1)", "posedge", 1},
             {"1+0 == RST", "posedge", 1},
             {"K === RST", "posedge", 1}}) {
        result = analyzeVerilog(
            std::string("module probe(input CLK,RST,D, output reg Q,X,Y); localparam K=1'b1;\n") +
            "always @(posedge CLK or " + edge + " RST) if(" + condition +
            ") Q<=0; else Q<=D;\n" +
            "always @(posedge CLK) if(!CLK) X<=0; else X<=D;\n" +
            "always @(posedge RST) if(!RST) Y<=0; else Y<=D;\nendmodule\n");
        check(count(result, "GJB10157.7.1.3") == findings &&
              std::ranges::none_of(result.violations, [](const auto& item) {
                  return item.ruleId == "GJB10157.7.1.3" && item.line == 3;
              }) && std::ranges::any_of(result.violations, [](const auto& item) {
                  return item.ruleId == "GJB10157.7.1.3" && item.line == 4;
              }), "R-6-3 admits constant comparison ports while preserving scalar origins and diagnostic shapes");
    }
    for (const auto& condition : {"RST == D", "RST == (D+1)", "RST == {D,1'b0}", "RST == K[0]",
                                  "!RST == 32'b1", "~RST == 2'b01", "(RST==1'b1) == 32'b1"}) {
        result = analyzeVerilog(
            std::string("module probe(input CLK,RST,D, output reg Q,X); localparam K=1'b1;\n") +
            "always @(posedge CLK or posedge RST) if(" + condition +
            ") Q<=0; else Q<=D;\n" +
            "always @(posedge CLK) if(!CLK) X<=0; else X<=D;\nendmodule\n");
        check(std::ranges::any_of(result.violations, [](const auto& item) {
                  return item.ruleId == "GJB10157.7.1.3" && item.line == 3;
              }), "R-6-3 keeps runtime and unrecovered comparison operands outside the closed catalog");
    }
    for (const auto& resetValue : {
             "RESET", "ALIAS", "NEXT", "(RESET+1)&8'hff", "~RESET", "-1",
             "$clog2(2)", "$signed(2)", "unsigned'(2)", "1+2*3", "8/2", "7%4", "2**3", "1<<2", "8>>1", "8>>>1",
             "1<2", "1>=2", "1==0", "1!=0", "1===0", "1!==0", "1&&0", "1||0",
             "8'hf0|8'h0f", "8'hf0^8'h0f", "8'hf0~^8'h0f", "&8'hff", "!0",
             "'0", "8'hxz", "1/0", "{8{1'b0}}", "{4'h0,4'h0}", "1 ? 0 : 1"}) {
        result = analyzeVerilog(
            std::string("module probe #(parameter RESET=8'h00)(input CLK,RST,input [7:0] D,output reg [7:0] Q,output reg X,Y); localparam ALIAS=(RESET),NEXT=ALIAS+1;\n") +
            "always @(posedge CLK or posedge RST) if(RST) Q<=" + resetValue + "; else Q<=D;\n" +
            "always @(posedge CLK) if(!CLK) X<=0; else X<=1;\n" +
            "always @(posedge RST) if(!RST) Y<=0; else Y<=1;\nendmodule\n");
        const auto context = std::string("R-6-3 recovers constant reset data without treating its clock as reset: ") + resetValue;
        check(count(result, "GJB10157.7.1.3") == 1 &&
              std::ranges::any_of(result.violations, [](const auto& item) {
                  return item.ruleId == "GJB10157.7.1.3" && item.line == 4;
              }), context.c_str());
    }
    for (const auto& resetValue : {"RV", "CYCLE", "CONST_CALL", "MISSING", "$clog2(RV)", "$signed(RV)", "{8{RV}}", "{65{1'b0}}", "RST ? 0 : 1", "1+", "0 1"}) {
        result = analyzeVerilog(
            std::string("module probe(input CLK,RST,input [7:0] D,RV,output reg [7:0] Q,output reg X); localparam CYCLE=OTHER,OTHER=CYCLE; localparam CONST_CALL=$clog2(RV);\n") +
            "always @(posedge CLK or posedge RST) if(RST) Q<=" + resetValue + "; else Q<=D;\n" +
            "always @(posedge CLK) if(!CLK) X<=0; else X<=1;\nendmodule\n");
        check(std::ranges::any_of(result.violations, [](const auto& item) {
                  return item.ruleId == "GJB10157.7.1.3" && item.line == 3;
              }), "R-6-3 does not close the catalog from runtime, cyclic, unresolved or unsupported reset values");
    }
    result = analyzeVerilog(
        "module a(input CLK,RST,input [7:0] D,output reg [7:0] Q,output reg X); localparam VALUE=0;\n"
        "always @(posedge CLK or posedge RST) if(RST) Q<=VALUE; else Q<=D;\n"
        "always @(posedge CLK) if(!CLK) X<=0; else X<=1; endmodule\n"
        "module b(input CLK,RST,input [7:0] D,VALUE,output reg [7:0] Q,output reg X);\n"
        "always @(posedge CLK or posedge RST) if(RST) Q<=VALUE; else Q<=D;\n"
        "always @(posedge CLK) if(!CLK) X<=0; else X<=1; endmodule\n");
    check(count(result, "GJB10157.7.1.3") == 1 &&
          std::ranges::any_of(result.violations, [](const auto& item) {
              return item.ruleId == "GJB10157.7.1.3" && item.line == 6;
          }), "R-6-3 parameter binding does not leak a reset constant into a sibling module variable");
    for (const auto& dataWidth : {"[W-1:0]", "[3:0]"}) {
        result = analyzeVerilog(
            std::string("module probe #(parameter W=8)(input CLK,RST, input ") + dataWidth +
            " D, output reg [7:0] Q, output reg X);\n" +
            "always @(posedge CLK or posedge RST) if(RST) Q<=0; else Q<=D;\n" +
            "always @(posedge CLK) if(!CLK) X<=0; else X<=1;\nendmodule\n");
        check(count(result, "GJB10157.7.1.3") == 1,
              "R-6-3 keeps unknown or resized runtime vector data outside the closed catalog");
    }
    for (const auto& resetBranch : {
             "if(RST) begin Q1<=0;Q1<=0;end else begin Q1<=D;Q2<=D;end",
             "if(RST) begin Q1<=0;Q2<=0;end else begin Q1<=D;end"}) {
        result = analyzeVerilog(
            std::string("module probe(input CLK,RST,D, output reg Q1,Q2,Q3);\n") +
            "always @(posedge CLK or posedge RST) " + resetBranch + "\n" +
            "always @(posedge CLK) if(!CLK) Q3<=0; else Q3<=D;\nendmodule\n");
        check(count(result, "GJB10157.7.1.3") == 1,
              "R-6-3 does not publish a partial reset catalog with duplicate or missing targets");
    }
    result = analyzeVerilog(
        "module probe(input CLK,RST,D, output reg Q1,Q2);\n"
        "always @(posedge RST or posedge CLK) begin if(~RST) Q1<=0; if(RST != 1'b1) Q2<=0; end\n"
        "endmodule\n");
    check(count(result, "GJB10157.7.1.3") == 0,
          "R-6-3 does not conflate bitwise NOT or inequality with native logical NOT");
    result = analyzeVerilog(
        "module probe(input CLK,RST,EN,D, output reg Q);\n"
        "always @(posedge RST or posedge CLK) begin if(RST && EN) Q<=0; else Q<=D; end\n"
        "endmodule\n");
    check(count(result, "GJB10157.7.1.3") == 0,
          "7.1.3 leaves compound reset conditions outside the bounded predicate");
    const auto resetPolarityAssessment = std::ranges::find_if(result.standardAssessments,
        [](const auto& entry) { return entry.clauseId == "GJB10157.7.1.3"; });
    check(resetPolarityAssessment != result.standardAssessments.end() &&
          resetPolarityAssessment->state == analysis::StandardRuleState::ManualReviewRequired,
          "bounded 7.1.3 no finding remains manual review rather than certified compliance");
    result = analyzeVerilog(
        "module probe(input CLK,D, output reg Q1,Q2,Q3);\n"
        "always @(posedge CLK) begin Q1<=0; Q1<=D; end\n"
        "always @(posedge CLK) begin Q2<=D; Q2<=1; end\n"
        "always @(posedge CLK) Q3<=D; endmodule\n");
    check(count(result, "GJB10157.7.1.2") == 1 &&
          std::ranges::any_of(result.violations, [](const auto& violation) {
              return violation.ruleId == "GJB10157.7.1.2" &&
                     violation.message == "触发器Q3无异步复位或置位" &&
                     violation.severity == analysis::Severity::Error;
          }),
          "R-6-2 literal RHS coverage is sticky in either assignment order and process-local");
    result = analyzeVerilog(
        "module probe #(parameter P=1)(input CLK,D, output reg Q1,Q2,Q3,Q4);\n"
        "localparam L=0;\n"
        "always @(posedge CLK) begin Q1<=P+D; Q2<=D+L; Q3<=1+2; Q4<=D; end\n"
        "endmodule\n");
    check(count(result, "GJB10157.7.1.2") == 1 &&
          std::ranges::any_of(result.violations, [](const auto& violation) {
              return violation.ruleId == "GJB10157.7.1.2" &&
                     violation.message.find("Q4") != std::string::npos;
          }),
          "R-6-2 follows native any-parameter RHS and identifier-free expression state updates");
    result = analyzeVerilog(
        "module first #(parameter P=1)(input CLK,D, output reg Q);\n"
        "always @(posedge CLK) Q<=P; endmodule\n"
        "module second(input CLK,P, output reg Q);\n"
        "always @(posedge CLK) Q<=P; endmodule\n");
    check(count(result, "GJB10157.7.1.2") == 1,
          "R-6-2 parameter names remain local to their owning module");
    result = analyzeVerilog(
        "module probe #(parameter P=1)(input CLK,D, output reg Q);\n"
        "always @(posedge CLK) if(Q<=P) Q<=D; endmodule\n");
    check(count(result, "GJB10157.7.1.2") == 1,
          "R-6-2 comparison conditions do not initialize assignment targets");
    result = analyzeVerilog(
        "module probe(input CLK,A,B, output reg Q1,Q2,Q3,Q4,Q5,Q6);\n"
        "always @(posedge CLK) {Q1,Q2}<=A;\n"
        "always @(posedge CLK) {Q3,Q4}<=A+B;\n"
        "always @(posedge CLK) {Q5,Q6}<=0; endmodule\n");
    check(count(result, "GJB10157.7.1.2") == 3 &&
          std::ranges::any_of(result.violations, [](const auto& violation) {
              return violation.ruleId == "GJB10157.7.1.2" &&
                     violation.message.find("Q2") != std::string::npos;
          }) && !std::ranges::any_of(result.violations, [](const auto& violation) {
              return violation.ruleId == "GJB10157.7.1.2" &&
                     violation.message.find("Q1") != std::string::npos;
          }),
          "R-6-2 concat targets use reverse order and one absent insertion per RHS identifier");
    result = analyzeVerilog(
        "module probe(input CLK,A, output reg Q1,Q2);\n"
        "always @(posedge CLK) begin {Q1,Q2}<=A; {Q1,Q2}<=A; end endmodule\n");
    check(count(result, "GJB10157.7.1.2") == 2,
          "R-6-2 repeated dynamic concat assignments progressively insert missing target keys");
    result = analyzeVerilog(
        "module probe(input CLK,A,B, output reg Q1,Q2,Q3);\n"
        "always @(posedge CLK) {Q1,{Q2,Q3}}<=A+B; endmodule\n");
    check(count(result, "GJB10157.7.1.2") == 2 &&
          !std::ranges::any_of(result.violations, [](const auto& violation) {
              return violation.ruleId == "GJB10157.7.1.2" &&
                     violation.message.find("Q1") != std::string::npos;
          }),
          "R-6-2 recursive concat collection preserves right-to-left leaf order");
    result = analyzeVerilog(
        "module probe(input CLK,A, output reg Q1,Q2,Q3);\n"
        "always @(posedge CLK) begin {Q1,{Q2,Q3}}<=0; {Q1,{Q2,Q3}}<=A; end\n"
        "endmodule\n");
    check(count(result, "GJB10157.7.1.2") == 0,
          "R-6-2 constant coverage applies to all nested concat leaves and stays set");
    result = analyzeVerilog(
        "module probe(input CLK,D, output reg Q);\n"
        "  always @(posedge CLK) begin\n"
        "\n"
        "    Q<=D;\n"
        "  end endmodule\n");
    check(std::ranges::any_of(result.violations, [](const auto& violation) {
              return violation.ruleId == "GJB10157.7.1.2" &&
                     violation.message == "触发器Q无异步复位或置位" &&
                     violation.line == 2 && violation.column == 1 &&
                     violation.severity == analysis::Severity::Error;
          }),
          "R-6-2 single-edge diagnostics use the always start line, not the assignment line");
    result = analyzeVerilog(
        "module probe(input CLK,RST,D, output reg Q1,Q2,Q3);\n"
        "always @(posedge CLK or posedge RST) begin\n"
        "if(RST) Q1<=0;\n"
        "else begin Q1<=D; Q2<=1; Q3<=D; end end endmodule\n");
    check(count(result, "GJB10157.7.1.2") == 2 &&
          std::ranges::any_of(result.violations, [](const auto& violation) {
              return violation.ruleId == "GJB10157.7.1.2" &&
                     violation.message == "触发器Q3无初值" &&
                     violation.line == 3 && violation.column == 1 &&
                     violation.severity == analysis::Severity::Error;
          }),
          "R-6-2 multi-edge state includes constant data assignments and reports at candidate IF");
    result = analyzeVerilog(
        "module probe #(parameter P=1)(input CLK,RST,D, output reg Q1,Q2);\n"
        "always @(posedge CLK or posedge RST) begin\n"
        "if(RST) Q1<=0; else begin Q1<=D; Q2<=P+D; end end\n"
        "endmodule\n");
    check(count(result, "GJB10157.7.1.2") == 0,
          "R-6-2 multi-edge candidates share native any-parameter RHS state semantics");
    result = analyzeVerilog(
        "module probe(input CLK,RST,EN,D, output reg Q1,Q2,Q3,Q4);\n"
        "always @(posedge CLK or posedge RST) begin\n"
        "if(RST) Q1<=0; else Q2<=D;\n"
        "if(EN) Q2<=0;\n"
        "if(EN) Q3<=D;\n"
        "begin : extra if(EN) Q4<=D; end\n"
        "end endmodule\n");
    check(count(result, "GJB10157.7.1.2") == 5 &&
          std::ranges::any_of(result.violations, [](const auto& violation) {
              return violation.ruleId == "GJB10157.7.1.2" &&
                     violation.message == "触发器Q2无初值" && violation.line == 3;
          }),
          "R-6-2 direct candidates occur twice and an extra-block candidate once with independent maps");
    result = analyzeVerilog(
        "module probe(input CLK,RST,EN,D, output reg Q1,Q2);\n"
        "always @(posedge CLK or posedge RST) begin\n"
        "if(RST) Q1<=0; else Q1<=D;\n"
        "begin begin if(EN) Q2<=D; end end\n"
        "end endmodule\n");
    check(count(result, "GJB10157.7.1.2") == 0,
          "R-6-2 candidate collector stops beyond two block ancestors");
    result = analyzeVerilog(
        "module probe(input CLK,ARST,D, output reg Q1,Q2,Q3);\n"
        "always @(posedge ARST or posedge CLK) begin : mixed\n"
        "if(ARST) Q1<=1'b0;\n"
        "else begin\n"
        "Q1<=D; Q2<=Q1; end end\n"
        "always @(posedge CLK) Q3<=Q2; endmodule\n");
    const auto mixedCoverageFinding = std::ranges::find_if(
        result.violations, [](const auto& violation) {
            return violation.ruleId == "GJB10157.7.1.1";
        });
    check(count(result, "GJB10157.7.1.1") == 1 &&
          mixedCoverageFinding != result.violations.end() &&
          mixedCoverageFinding->message ==
              "always语句中混合使用有复位和无复位的触发器：触发器“Q2”未进行复位" &&
          mixedCoverageFinding->severity == analysis::Severity::Error &&
          mixedCoverageFinding->line == 5 && mixedCoverageFinding->column == 1,
          "R-6-1 reproduces the native per-target text, Error and assignment line-only location");
    check(count(result, "GJB10157.7.1.2") == 3,
          "attachment 7.1.2 reports a missing reset-branch target and a no-reset process target");
    result = analyzeVerilog(
        "module probe(input CLK,ARST,D, output reg Q1,Q2,Q3);\n"
        "always @(posedge ARST or posedge CLK) begin\n"
        "if(1'b1==ARST) begin Q1<=0; Q2<=0; Q3<=0; end\n"
        "else begin Q1<=D; Q2<=Q1; Q3<=Q2; end end endmodule\n");
    check(count(result, "GJB10157.7.1.1") == 0 &&
          count(result, "GJB10157.7.1.2") == 0,
          "7.1.1 and 7.1.2 accept complete reset coverage with reversed equality");
    const auto mixedFlipFlopAssessment = std::ranges::find_if(result.standardAssessments,
        [](const auto& entry) { return entry.clauseId == "GJB10157.7.1.1"; });
    const auto initializedAssessment = std::ranges::find_if(result.standardAssessments,
        [](const auto& entry) { return entry.clauseId == "GJB10157.7.1.2"; });
    check(mixedFlipFlopAssessment != result.standardAssessments.end() &&
          mixedFlipFlopAssessment->state == analysis::StandardRuleState::ManualReviewRequired &&
          initializedAssessment != result.standardAssessments.end() &&
          initializedAssessment->state == analysis::StandardRuleState::ManualReviewRequired,
          "bounded 7.1.1 and 7.1.2 no findings remain manual review");
    result = analyzeVerilog(
        "module probe(input CLK,ARST,EN,D, output reg Q);\n"
        "always @(posedge ARST or posedge CLK) begin\n"
        "if(ARST && EN) Q<=0; else Q<=D; end endmodule\n");
    check(count(result, "GJB10157.7.1.1") == 0 &&
          count(result, "GJB10157.7.1.2") == 0,
          "7.1.1 and 7.1.2 leave compound reset guards outside the bounded predicate");
    result = analyzeVerilog(
        "module probe(input CLK,ARST,ASET,D, output reg Q1,Q2,Q3);\n"
        "always @(posedge ARST or posedge ASET or posedge CLK) begin\n"
        "if(ARST) Q1<=0; else if(ASET) Q2<=1;\n"
        "else begin\n"
        "Q1<=D; Q2<=Q1; Q3<=Q2; end end endmodule\n");
    check(count(result, "GJB10157.7.1.1") == 1 &&
          std::ranges::any_of(result.violations, [](const auto& violation) {
              return violation.ruleId == "GJB10157.7.1.1" && violation.line == 5 &&
                     violation.column == 1 &&
                     violation.message.ends_with("触发器“Q3”未进行复位");
          }),
          "R-6-1 compares the final default branch with every preceding else-if branch");
    result = analyzeVerilog(
        "module probe(input CLK,ARST,D, output reg Q1,Q2);\n"
        "always @(posedge ARST or posedge CLK) begin\n"
        "if(ARST) begin end\n"
        "else begin\n"
        "Q1<=D; Q2<=Q1; end end endmodule\n");
    check(count(result, "GJB10157.7.1.1") == 2,
          "R-6-1 reports every final-default target when preceding reset branches are empty");
    result = analyzeVerilog(
        "module probe(input CLK,ARST,D, output reg Q1,Q2);\n"
        "always @(posedge ARST or posedge CLK) begin\n"
        "if(ARST) Q1<=0; else Q2<=D; end endmodule\n");
    check(count(result, "GJB10157.7.1.1") == 0,
          "R-6-1 preserves the native <= default-line cutoff for same-line else assignments");
    result = analyzeVerilog(
        "module probe(input CLK,ARST,COND,D, output reg E,G);\n"
        "always @(posedge ARST or posedge CLK) begin\n"
        "if(ARST) begin E<=0; G<=0; end\n"
        "else if(COND) E<=D; else begin E<=~D; G<=D; end end endmodule\n");
    check(count(result, "GJB10157.7.1.1") == 0,
          "R-6-1 retains complete reset coverage across a nested synchronous data condition");
    result = analyzeVerilog(
        "module probe(input A,B,X, output reg C,D); always @(A,B,X) begin\n"
        "if(X) C=A; else C=C; D=B|A; end endmodule\n");
    check(count(result, "GJB10157.7.2.3") == 1,
          "attachment 7.2.3 reports other combinational logic mixed with an explicit self-hold latch");
    result = analyzeVerilog(
        "module probe(input A,X, output reg C); always @* begin\n"
        "if(X) C=A; else C=C; end endmodule\n");
    check(count(result, "GJB10157.7.2.3") == 0,
          "7.2.3 permits a combinational process containing only the latch target");
    result = analyzeVerilog(
        "module probe(input A,B,X, output reg C,D); always @* begin\n"
        "if(X) C=A; D=A&B; end endmodule\n");
    check(count(result, "GJB10157.7.2.3") == 1,
          "7.2.3 uses proven incomplete-path latch evidence with another definite target");
    result = analyzeVerilog(
        "module probe(input A,B,X, output reg C,D); always @* begin\n"
        "if(A<=B) C=A; else C=B; D=A&B; end endmodule\n");
    check(count(result, "GJB10157.7.2.3") == 0,
          "7.2.3 does not confuse a relational less-equal condition with an assignment");
    result = analyzeVerilog(
        "module probe(input CLK,A, output reg C,D); always @(posedge CLK) begin\n"
        "C<=C; D<=A; end endmodule\n");
    check(count(result, "GJB10157.7.2.3") == 0,
          "7.2.3 follows the native edge-event exclusion for sequential processes");
    const auto latchSeparationAssessment = std::ranges::find_if(result.standardAssessments,
        [](const auto& entry) { return entry.clauseId == "GJB10157.7.2.3"; });
    check(latchSeparationAssessment != result.standardAssessments.end() &&
          latchSeparationAssessment->state == analysis::StandardRuleState::ManualReviewRequired,
          "bounded 7.2.3 no finding remains manual review rather than certified compliance");
    result = analyzeVerilog(
        "module probe(input CLK, input [1:0] A,A2, inout C1,C2);\n"
        "reg [1:0] r1,r2; wire [1:0] comb;\n"
        "always @(posedge CLK) begin r1<=A; r2<=A2; end\n"
        "assign comb = r1 & r2;\n"
        "assign C1 = (comb[0]==1'b1) ? r1[0] : 1'bz;\n"
        "assign C2 = (comb[1]==1'b0) ? r1[1] : 1'bz; endmodule\n");
    check(count(result, "GJB10157.7.3.1") == 1,
          "attachment 7.3.1 reports one combinational driver used by two tri-state enables");
    result = analyzeVerilog(
        "module probe(input CLK,A,EN, inout C1,C2,C3); reg q; wire bb_ctrl;\n"
        "always @(posedge CLK) q<=A; blackbox u0(.ctrl(bb_ctrl));\n"
        "assign C1=EN ? A : 1'bz; assign C2=!q ? A : 1'bz;\n"
        "assign C3=bb_ctrl ? A : 1'bz; endmodule\n");
    check(count(result, "GJB10157.7.3.1") == 0,
          "7.3.1 permits input, register and unresolved cell-output tri-state enables");
    result = analyzeVerilog(
        "module probe(input A,B, inout C); wire logic_en,alias_en;\n"
        "assign logic_en=A|B; assign alias_en=logic_en;\n"
        "assign C=alias_en ? A : 1'bz; endmodule\n");
    check(count(result, "GJB10157.7.3.1") == 1,
          "7.3.1 follows a bounded direct alias back to its combinational origin");
    result = analyzeVerilog(
        "module probe(input A,B, output Y, inout C); wire logic_en;\n"
        "assign logic_en=A&B; assign Y=logic_en; assign C=A ? B : 1'bx; endmodule\n");
    check(count(result, "GJB10157.7.3.1") == 0,
          "7.3.1 does not report unused combinational outputs or X-only conditional drives");
    const auto tristateEnableAssessment = std::ranges::find_if(result.standardAssessments,
        [](const auto& entry) { return entry.clauseId == "GJB10157.7.3.1"; });
    check(tristateEnableAssessment != result.standardAssessments.end() &&
          tristateEnableAssessment->state == analysis::StandardRuleState::ManualReviewRequired,
          "bounded 7.3.1 no finding remains manual review rather than certified compliance");
    result = analyzeVerilog(
        "module probe(input [3:0] A,A2, input EN, output [3:0] Y, inout [3:0] C);\n"
        "wire [3:0] int_vec2; wire int_sig;\n"
        "assign int_vec2 = EN ? A2 : 4'bzzzz;\n"
        "assign int_sig = A2[2] ? A[0] : 1'bz;\n"
        "assign Y = A; assign C[0] = EN ? A[0] : 1'bz; endmodule\n");
    check(count(result, "GJB10157.7.3.2") == 2,
          "attachment 7.3.2 reports Z drivers on internal continuous-assignment targets only");
    result = analyzeVerilog(
        "module probe(input A,EN, output Y, inout C); wire internal;\n"
        "assign Y = EN ? A : 1'bz; assign C = EN ? A : 1'bz;\n"
        "assign internal = 1'bz; endmodule\n");
    check(count(result, "GJB10157.7.3.2") == 1,
          "7.3.2 permits boundary tri-state targets and reports a definite direct internal Z drive");
    result = analyzeVerilog(
        "module probe(input A,EN, output reg Q); wire internal;\n"
        "assign internal = EN ? A : 1'bx;\n"
        "always @* Q = EN ? A : 1'bz; endmodule\n");
    check(count(result, "GJB10157.7.3.2") == 0,
          "7.3.2 distinguishes X from Z and follows the native continuous-assignment filter");
    const auto internalTristateAssessment = std::ranges::find_if(result.standardAssessments,
        [](const auto& entry) { return entry.clauseId == "GJB10157.7.3.2"; });
    check(internalTristateAssessment != result.standardAssessments.end() &&
          internalTristateAssessment->state == analysis::StandardRuleState::ManualReviewRequired,
          "bounded 7.3.2 no finding remains manual review rather than certified compliance");
    result = analyzeVerilog(
        "module probe(input CLK,ARST1,ARST2,D, output reg Q);\n"
        "always @(posedge ARST1 or posedge ARST2 or posedge CLK) begin\n"
        "if(ARST1) Q<=1'b0; else if(ARST2) Q<=1'b1; else Q<=D; end endmodule\n");
    const auto mixedSetResetFinding = std::ranges::find_if(
        result.violations, [](const auto& violation) {
            return violation.ruleId == "GJB10157.6.2.9";
        });
    check(count(result, "GJB10157.6.2.9") == 1 &&
          mixedSetResetFinding != result.violations.end() &&
          mixedSetResetFinding->message ==
              "always中同时使用了异步复位信号和异步置位信号" &&
          mixedSetResetFinding->severity == analysis::Severity::Error &&
          mixedSetResetFinding->line == 3 && mixedSetResetFinding->column == 1,
          "A-5-4 reproduces the native message, Error and qualifying IF line-only location");
    result = analyzeVerilog(
        "module probe(input CLK,ARST1,ARST2,D, output reg Q1,Q2);\n"
        "always @(posedge ARST1 or posedge ARST2 or posedge CLK) begin\n"
        "if(ARST1||ARST2) Q1<=1'b0; else Q1<=D; end\n"
        "always @(posedge ARST1 or posedge ARST2 or posedge CLK) begin\n"
        "if(ARST1||ARST2) Q2<=1'b1; else Q2<=D; end endmodule\n");
    check(count(result, "GJB10157.6.2.9") == 0,
          "6.2.9 permits multiple asynchronous controls performing only one action type");
    result = analyzeVerilog(
        "module probe(input CLK,ARST1,ARST2,D, output reg Q1,Q2);\n"
        "always @(posedge ARST1 or posedge ARST2 or posedge CLK) begin\n"
        "if(ARST1) Q1<=1'b0;\n"
        "if(ARST2) Q2<=1'b1;\n"
        "end endmodule\n");
    check(count(result, "GJB10157.6.2.9") == 0,
          "A-5-4 does not merge constants from independent sibling IF trees");
    result = analyzeVerilog(
        "module probe(input CLK,ARST1,ARST2,D, output reg [1:0] Q);\n"
        "always @(posedge ARST1 or posedge ARST2 or posedge CLK) begin\n"
        "if(ARST1||ARST2) Q<=2'b10; else Q<=D; end endmodule\n");
    check(count(result, "GJB10157.6.2.9") == 1 &&
          std::ranges::any_of(result.violations, [](const auto& violation) {
              return violation.ruleId == "GJB10157.6.2.9" && violation.line == 3 &&
                     violation.column == 1 && violation.severity == analysis::Severity::Error;
          }),
          "A-5-4 mirrors the native integer >=2 branch by contributing both 0 and 1");
    const auto mixedSetResetAssessment = std::ranges::find_if(result.standardAssessments,
        [](const auto& entry) { return entry.clauseId == "GJB10157.6.2.9"; });
    check(mixedSetResetAssessment != result.standardAssessments.end() &&
          mixedSetResetAssessment->state == analysis::StandardRuleState::Violated,
          "A-5-4 reports the recovered wide-constant violation state");
    result = analyzeVerilog(
        "module probe(input CLK,ARST1,ARST2,D, output reg Q);\n"
        "always @(posedge ARST1 or posedge ARST2 or posedge CLK) begin\n"
        "if(ARST1||ARST2) Q<=1'b0; else Q<=D; end endmodule\n");
    const auto cleanMixedSetResetAssessment = std::ranges::find_if(result.standardAssessments,
        [](const auto& entry) { return entry.clauseId == "GJB10157.6.2.9"; });
    check(cleanMixedSetResetAssessment != result.standardAssessments.end() &&
          cleanMixedSetResetAssessment->state == analysis::StandardRuleState::ManualReviewRequired,
          "bounded 6.2.9 no finding is not certified compliance");
    result = analyzeVerilog(
        "module probe(\n"
        "\tinput wire A,\n"
        "    output wire Y\n"
        ");\n"
        " wire first;\n"
        "  logic shifted;\n"
        "endmodule\n");
    check(count(result, "GJB10157.5.1.3") == 1,
          "port and internal declaration groups use independent baselines and literal four-space tabs");
    check(std::ranges::any_of(result.violations, [](const auto& item) {
              return item.ruleId == "GJB10157.5.1.3" && item.line == 6 &&
                     item.column == 1 && item.severity == analysis::Severity::Error &&
                     item.message == "shifted定义左端未对齐";
          }), "internal declaration diagnostic preserves the recovered name-prefixed message");
    result = analyzeVerilog(
        "module probe(\n"
        " input a,\n"
        "  b,\n"
        " output y\n"
        "); endmodule\n");
    check(count(result, "GJB10157.5.1.3") == 0,
          "continued port lines preserve native uint32 failed-search wraparound column");
    result = analyzeVerilog(
        "module first(\n input a,\n output y\n); wire x; endmodule\n"
        "module probe(\n    input a,\n    output y\n);    wire x; endmodule\n");
    check(count(result, "GJB10157.5.1.3") == 0,
          "5.1.3 resets both alignment baselines for every module");
    result = analyzeVerilog(
        "module probe(\n input a,\n output y\n);\n"
        "// input output inout are ignored without declaration AST nodes\n"
        "initial $display(\"    input\");\n"
        " wire first,\n"
        " second;\n"
        "endmodule\n");
    check(count(result, "GJB10157.5.1.3") == 0,
          "comments and strings do not create declarations and continued internal names retain their line indent");
    result = analyze(wrap("always @* case(aa) default: if(aa=='x) qq=0; endcase"));
    check(count(result, "GJB10157.9.2.9") == 0, "X in default control condition is not an assignment subtree");
    result = analyze(wrap("always @* case(aa) default: $display('x); endcase"));
    check(count(result, "GJB10157.9.2.9") == 0, "standalone task argument X is not a default assignment");
    result = analyze(wrap("always @* case(aa) aa:qq=0; default:qq=1; endcase\n"
                          "always @* repeat(aa+1) qq=0;"));
    check(count(result, "GJB10157.9.2.6") == 1, "case label variable check is integrated");
    check(count(result, "GJB10157.9.3.6") == 1, "repeat nested variable check is integrated");
    check(std::ranges::any_of(result.violations, [](const auto& item) {
        return item.ruleId == "GJB10157.9.3.6" &&
               item.severity == analysis::Severity::Warning &&
               item.message == "避免在repeat语句表达式中使用变量.";
    }), "9.3.6 preserves the recovered native Warning severity and exact message");
    result = analyze(wrap("always @* case(aa)\n"
        "0, aa: qq=0;\n"
        "aa, 0: qq=1;\n"
        "default: qq=0;\nendcase"));
    check(count(result, "GJB10157.9.2.6") == 1 &&
          std::ranges::any_of(result.violations, [](const auto& item) {
              return item.ruleId == "GJB10157.9.2.6" && item.line == 7;
          }), "native case-label rule checks only each branch node's first child");
    result = analyze(wrap("always @* case(aa)\n"
        "aa, 0: qq=0;\n"
        "qq, 1: qq=1;\n"
        "default: qq=0;\nendcase"));
    check(count(result, "GJB10157.9.2.6") == 2,
          "two branches with runtime first children produce two native-grain diagnostics");
    result = analyze(wrap("always @* repeat(aa+qq+aa) qq=0;"));
    check(count(result, "GJB10157.9.3.6") == 3,
          "9.3.6 preserves native per-identifier multiplicity in one repeat count");
    result = analyze(wrap("always @* repeat(unknown_count) qq=0;"));
    check(count(result, "GJB10157.9.3.6") == 1,
          "A-12-1 reports a direct identifier absent from the flat parameter table");
    result = analyze(wrap("parameter N=2; begin : b integer N; repeat(N) qq=0; end"));
    check(count(result, "GJB10157.9.3.6") == 0,
          "A-12-1 preserves native flat-name behavior despite runtime shadowing");
    result = analyze(wrap("parameter N=2; always @* repeat(N+1) qq=0;"));
    check(count(result, "GJB10157.9.3.6") == 0, "repeat parameter expression is not falsely classified as variable");
    const auto unknownDefault = std::ranges::find_if(result.standardAssessments,
        [](const auto& entry) { return entry.clauseId == "GJB10157.9.2.9"; });
    check(unknownDefault != result.standardAssessments.end() &&
          unknownDefault->state == analysis::StandardRuleState::ManualReviewRequired,
          "unrecovered default exclusions remain review-required, not passed");

    result = analyze(wrap("function automatic integer rec(input integer n); rec=rec(n-1); endfunction"));
    check(count(result, "GJB10157.9.3.5") == 1, "recursive function graph is integrated");
    check(std::ranges::any_of(result.violations, [](const auto& item) {
        return item.ruleId == "GJB10157.9.3.5" &&
               item.severity == analysis::Severity::Error &&
               item.message == "禁止调用递归函数.函数probe/rec存在递归调用";
    }), "R-12-5 preserves native direct-cycle text and official Error severity");
    result = analyze(wrap("function integer f; f=g(); endfunction\n"
                          "function integer g; g=f(); endfunction"));
    check(count(result, "GJB10157.9.3.5") == 1 &&
          std::ranges::any_of(result.violations, [](const auto& item) {
              return item.ruleId == "GJB10157.9.3.5" &&
                     item.message == "禁止调用递归函数.函数probe/f、probe/g组成递归调用";
          }), "R-12-5 deduplicates a mutual-recursion member set and lists its path");
    result = analyzeVerilog(
        "module a; function integer f; f=g(); endfunction endmodule\n"
        "module b; function integer g; g=f(); endfunction endmodule\n");
    check(count(result, "GJB10157.9.3.5") == 1,
          "R-12-5 uses the recovered global function-name graph across module groups");
    result = analyzeVerilog(
        "module a; function integer f; f=f(0); endfunction endmodule\n"
        "module b; function integer f; f=f(1); endfunction endmodule\n");
    check(count(result, "GJB10157.9.3.5") == 1 &&
          std::ranges::any_of(result.violations, [](const auto& item) {
              return item.ruleId == "GJB10157.9.3.5" && item.line == 2 &&
                     item.message == "禁止调用递归函数.函数b/f存在递归调用";
          }), "R-12-5 duplicate names overwrite the report node while edges merge");
    result = analyze(wrap("parameter Data=1; wire DATA, data, Data;"));
    check(count(result, "GJB10157.5.2.15") == 2,
          "native default parameter/wire predicates are integrated despite HTML Data conflict");
    check(std::ranges::any_of(result.violations, [](const auto& item) {
        return item.ruleId == "GJB10157.5.2.15" && item.column == 1 &&
               item.severity == analysis::Severity::Error &&
               item.message == "Data:建议常量的命名中字母全部采用大写";
    }), "parameter branch preserves native constant-name message, Error and line-only location");
    check(std::ranges::any_of(result.violations, [](const auto& item) {
        return item.ruleId == "GJB10157.5.2.15" && item.column == 1 &&
               item.severity == analysis::Severity::Error &&
               item.message == "DATA:建议变量的命名中字母全部采用小写或者仅首字母大写";
    }), "wire branch preserves native variable-name message, Error and line-only location");
    result = analyze(wrap("wire DATA, DATA;"));
    check(count(result, "GJB10157.5.2.15") == 2 &&
          std::ranges::count_if(result.violations, [](const auto& item) {
              return item.ruleId == "GJB10157.5.2.15" && item.column == 1 &&
                     item.message ==
                         "DATA:建议变量的命名中字母全部采用小写或者仅首字母大写";
          }) == 2, "same-line AST_WIRE declarations retain native per-node multiplicity");
    result = analyze(wrap("parameter DATA=1; wire data; function integer f; reg DATA; f=0; endfunction"));
    check(count(result, "GJB10157.5.2.15") == 0, "function-local declaration excluded from naming rule");
    const auto namingAssessment = std::ranges::find_if(result.standardAssessments,
        [](const auto& entry) { return entry.clauseId == "GJB10157.5.2.15"; });
    check(namingAssessment != result.standardAssessments.end() &&
          namingAssessment->state == analysis::StandardRuleState::ManualReviewRequired,
          "partial naming declarations do not certify whole-rule compliance");
    analysis::AnalysisOptions conditionalNamingOptions;
    conditionalNamingOptions.standardId = "GJB10157-2021";
    conditionalNamingOptions.defines.push_back("USE_BAD_NAMES");
    result = analysis::Analyzer(conditionalNamingOptions).analyzeSource(
        {"conditional-names.sv", core::HdlLanguage::SystemVerilog,
         "module probe;\n`ifdef USE_BAD_NAMES\nparameter BadName = 1; wire badName;\n"
         "`else\nparameter GOOD_NAME = 1; wire good_name;\n`endif\nendmodule\n"});
    check(count(result, "GJB10157.5.2.15") == 2,
          "5.2.15 checks parameter and wire declarations in the configured active branch");
    check(std::ranges::all_of(result.violations | std::views::filter([](const auto& item) {
        return item.ruleId == "GJB10157.5.2.15";
    }), [](const auto& item) { return item.line == 3 && item.column == 1; }),
          "5.2.15 active conditional findings retain native physical line-only location");
    conditionalNamingOptions.defines.clear();
    result = analysis::Analyzer(conditionalNamingOptions).analyzeSource(
        {"conditional-names.sv", core::HdlLanguage::SystemVerilog,
         "module probe;\n`ifdef USE_BAD_NAMES\nparameter BadName = 1; wire badName;\n"
         "`else\nparameter GOOD_NAME = 1; wire good_name;\n`endif\nendmodule\n"});
    check(count(result, "GJB10157.5.2.15") == 0,
          "5.2.15 excludes violations that exist only in an inactive conditional branch");
    result = analyze(wrap("function integer leaf; leaf=1; endfunction\n"
                          "function integer caller; caller=leaf(); endfunction"));
    check(count(result, "GJB10157.9.3.5") == 0, "acyclic function graph does not trigger recursion");
    result = analyze(wrap("always @* qq=aa[qq+1];"));
    check(count(result, "GJB10157.9.4.10") == 1, "complex selection predicate is integrated");
    check(std::ranges::any_of(result.violations, [](const auto& item) {
        return item.ruleId == "GJB10157.9.4.10" &&
               item.severity == analysis::Severity::Warning &&
               item.message == "建议数组索引为简单的信号，避免使用表达式";
    }), "9.4.10 preserves the recovered native severity and diagnostic text");
    result = analyze(wrap("always @* begin qq=aa[qq+1]; rr=bb[rr*2]; end"));
    check(count(result, "GJB10157.9.4.10") == 2,
          "9.4.10 reports each complex selection candidate independently");
    check(std::ranges::all_of(result.violations | std::views::filter([](const auto& item) {
        return item.ruleId == "GJB10157.9.4.10";
    }), [](const auto& item) { return item.line == 5; }),
          "9.4.10 preserves source line for each complex selection candidate");
    result = analyze(wrap("wire [7+1:0] vector_data; always @* qq=vector_data[aa];"));
    check(count(result, "GJB10157.9.4.10") == 0, "declaration ranges and simple indices are not complex selection");
    for (const auto* clause : {"GJB10157.9.3.5", "GJB10157.9.4.10"}) {
        const auto assessment = std::ranges::find_if(result.standardAssessments,
            [&](const auto& entry) { return entry.clauseId == clause; });
        check(assessment != result.standardAssessments.end() &&
              assessment->state == analysis::StandardRuleState::ManualReviewRequired,
              "partial structural predicate does not certify compliance");
    }
    analysis::AnalysisOptions conditionalStructureOptions;
    conditionalStructureOptions.standardId = "GJB10157-2021";
    conditionalStructureOptions.defines.push_back("USE_BAD_STRUCTURES");
    const std::string conditionalStructures =
        "module probe(input a,b,sel,input [3:0] vec,output reg q);\n"
        "`ifdef USE_BAD_STRUCTURES\n"
        "always @* begin q=0; q=1; end\n"
        "always @(a | b) q=a;\n"
        "always @* case(sel) a:q=0; default:q=1; endcase\n"
        "always @* repeat(a+1) q=0;\n"
        "function integer rec; rec=rec(); endfunction\n"
        "always @* q=vec[a+1];\n"
        "function integer bad_fn; input a; bad_fn<=a; endfunction\n"
        "`else\nwire safe_signal;\n`endif\nendmodule\n";
    result = analysis::Analyzer(conditionalStructureOptions).analyzeSource(
        {"conditional-structures.sv", core::HdlLanguage::SystemVerilog,
         conditionalStructures});
    for (const auto* rule : {"GJB10157.9.1.5", "GJB10157.9.2.5",
                             "GJB10157.9.2.6", "GJB10157.9.3.5",
                             "GJB10157.9.3.6", "GJB10157.9.4.3",
                             "GJB10157.9.4.10"})
        check(count(result, rule) == 1,
              "configured active branch reaches each recovered structural predicate");
    check(std::ranges::any_of(result.violations, [](const auto& item) {
        return item.ruleId == "GJB10157.9.1.5" && item.line == 3;
    }) && std::ranges::any_of(result.violations, [](const auto& item) {
        return item.ruleId == "GJB10157.9.2.5" && item.line == 4;
    }) && std::ranges::any_of(result.violations, [](const auto& item) {
        return item.ruleId == "GJB10157.9.2.6" && item.line == 5;
    }) && std::ranges::any_of(result.violations, [](const auto& item) {
        return item.ruleId == "GJB10157.9.3.6" && item.line == 6;
    }) && std::ranges::any_of(result.violations, [](const auto& item) {
        return item.ruleId == "GJB10157.9.3.5" && item.line == 7;
    }) && std::ranges::any_of(result.violations, [](const auto& item) {
        return item.ruleId == "GJB10157.9.4.10" && item.line == 8;
    }) && std::ranges::any_of(result.violations, [](const auto& item) {
        return item.ruleId == "GJB10157.9.4.3" && item.line == 9;
    }), "active structural findings retain their original physical source lines");
    conditionalStructureOptions.defines.clear();
    result = analysis::Analyzer(conditionalStructureOptions).analyzeSource(
        {"conditional-structures.sv", core::HdlLanguage::SystemVerilog,
         conditionalStructures});
    for (const auto* rule : {"GJB10157.9.1.5", "GJB10157.9.2.5",
                             "GJB10157.9.2.6", "GJB10157.9.3.5",
                             "GJB10157.9.3.6", "GJB10157.9.4.3",
                             "GJB10157.9.4.10"})
        check(count(result, rule) == 0,
              "inactive branch cannot trigger a recovered structural predicate");

    analysis::AnalysisOptions conditionalExpressionOptions;
    conditionalExpressionOptions.standardId = "GJB10157-2021";
    conditionalExpressionOptions.defines.push_back("USE_BAD_EXPRESSIONS");
    const std::string conditionalExpressions =
        "module probe(input a,input [1:0] sel); reg q; wire w;\n"
        "`ifdef USE_BAD_EXPRESSIONS\n"
        "assign w = a + 1'bx;\n"
        "always @* q = (a < 1'bx);\n"
        "always @* if (1'bx) q=0; else q=1;\n"
        "always @* case(2'b00) 2'b00:q=0; default:q=1; endcase\n"
        "always @* casex(sel) 2'b0x:q=0; default:q=1; endcase\n"
        "always @* case(sel) default:q=1'bx; endcase\n"
        "`else\nwire safe_signal;\n`endif\nendmodule\n";
    result = analysis::Analyzer(conditionalExpressionOptions).analyzeSource(
        {"conditional-expressions.sv", core::HdlLanguage::SystemVerilog,
         conditionalExpressions});
    for (const auto* rule : {"GJB10157.8.4", "GJB10157.8.5",
                             "GJB10157.9.2.2", "GJB10157.9.2.4",
                             "GJB10157.9.2.8", "GJB10157.9.2.9"})
        check(count(result, rule) == 1,
              "configured active branch reaches each recovered expression predicate exactly once");
    check(std::ranges::any_of(result.violations, [](const auto& item) {
        return item.ruleId == "GJB10157.8.5" && item.line == 3;
    }) && std::ranges::any_of(result.violations, [](const auto& item) {
        return item.ruleId == "GJB10157.8.4" && item.line == 4;
    }) && std::ranges::any_of(result.violations, [](const auto& item) {
        return item.ruleId == "GJB10157.9.2.4" && item.line == 5;
    }) && std::ranges::any_of(result.violations, [](const auto& item) {
        return item.ruleId == "GJB10157.9.2.2" && item.line == 6;
    }) && std::ranges::any_of(result.violations, [](const auto& item) {
        return item.ruleId == "GJB10157.9.2.8" && item.line == 7;
    }) && std::ranges::any_of(result.violations, [](const auto& item) {
        return item.ruleId == "GJB10157.9.2.9" && item.line == 8;
    }), "active expression findings retain their original physical source lines");
    conditionalExpressionOptions.defines.clear();
    result = analysis::Analyzer(conditionalExpressionOptions).analyzeSource(
        {"conditional-expressions.sv", core::HdlLanguage::SystemVerilog,
         conditionalExpressions});
    for (const auto* rule : {"GJB10157.8.4", "GJB10157.8.5",
                             "GJB10157.9.2.2", "GJB10157.9.2.4",
                             "GJB10157.9.2.8", "GJB10157.9.2.9"})
        check(count(result, rule) == 0,
              "inactive branch cannot trigger a recovered expression predicate");

    analysis::AnalysisOptions conditionalAlignmentOptions;
    conditionalAlignmentOptions.standardId = "GJB10157-2021";
    conditionalAlignmentOptions.defines.push_back("USE_BAD_ALIGNMENT");
    const std::string conditionalAlignment =
        "module probe(input a, output reg q, output reg r);\n"
        "`ifdef USE_BAD_ALIGNMENT\n"
        "always @* begin\n"
        "if(a) q=1;\n"
        " else q=0;\n"
        "end\n"
        " wire bad_one;\n"
        "   wire bad_two;\n"
        "always @* begin q=a; r=a; end\n"
        "`else\nwire safe_signal;\n`endif\nendmodule\n";
    result = analysis::Analyzer(conditionalAlignmentOptions).analyzeSource(
        {"conditional-alignment.sv", core::HdlLanguage::SystemVerilog,
         conditionalAlignment});
    for (const auto* rule : {"GJB10157.5.1.2", "GJB10157.5.1.3",
                             "GJB10157.5.1.6"})
        check(count(result, rule) == 1,
              "configured active branch reaches each recovered alignment predicate once");
    check(std::ranges::any_of(result.violations, [](const auto& item) {
        return item.ruleId == "GJB10157.5.1.2" && item.line == 5;
    }) && std::ranges::any_of(result.violations, [](const auto& item) {
        return item.ruleId == "GJB10157.5.1.3" && item.line == 8;
    }) && std::ranges::any_of(result.violations, [](const auto& item) {
        return item.ruleId == "GJB10157.5.1.6" && item.line == 9;
    }), "active alignment findings retain their original physical source lines");
    conditionalAlignmentOptions.defines.clear();
    result = analysis::Analyzer(conditionalAlignmentOptions).analyzeSource(
        {"conditional-alignment.sv", core::HdlLanguage::SystemVerilog,
         conditionalAlignment});
    for (const auto* rule : {"GJB10157.5.1.2", "GJB10157.5.1.3",
                             "GJB10157.5.1.6"})
        check(count(result, rule) == 0,
              "inactive branch cannot trigger a recovered alignment predicate");

    for (const auto& declarations : {"wire Data, data;", "wire data, Data;"}) {
        result = analyze(wrap(declarations));
        check(count(result, "GJB10157.5.2.6") == 1, "case-only declaration collision is order independent");
    }
    result = analyze("module probe;\n  wire Data;\n    wire data;\nendmodule\n");
    check(count(result, "GJB10157.5.2.6") == 1 &&
          std::ranges::any_of(result.violations, [](const auto& item) {
              return item.ruleId == "GJB10157.5.2.6" &&
                     item.severity == analysis::Severity::Error &&
                     item.line == 3 && item.column == 1 &&
                     item.message == "禁止仅使用大小写区分命名data和Data";
          }), "native case-only diagnostic preserves later-first message order, Error and no column");
    result = analyze("module probe; wire data; wire Data; wire DATA; endmodule\n");
    check(count(result, "GJB10157.5.2.6") == 2 &&
          std::ranges::count_if(result.violations, [](const auto& item) {
              return item.ruleId == "GJB10157.5.2.6" &&
                     (item.message == "禁止仅使用大小写区分命名Data和data" ||
                      item.message == "禁止仅使用大小写区分命名DATA和data");
          }) == 2, "each later case variant reports once against the first differing spelling");
    result = analyze("module first; wire Data; endmodule\nmodule second; wire data; endmodule\n");
    check(count(result, "GJB10157.5.2.6") == 0,
          "case-only names in separate modules are not paired");
    result = analyze("module duplicate; wire Data; wire data; endmodule\n"
                     "module duplicate; wire More; wire more; endmodule\n");
    check(count(result, "GJB10157.5.2.6") == 0,
          "duplicate same-name module groups fail the native unique-module admission gate");
    result = analyze("module probe; wire Data; wire data; endmodule\n");
    check(count(result, "GJB10157.5.2.6") == 1,
          "wire declarations in one module are paired");
    result = analyze("module probe; function integer Data; Data=1; endfunction\n"
                     "wire data; endmodule\n");
    check(count(result, "GJB10157.5.2.6") == 0,
          "native 5.2.6 collector excludes AST_FUNCTION");
    result = analyze("module probe; function integer f; reg Data; f=Data; endfunction\n"
                     "wire data; endmodule\n");
    check(count(result, "GJB10157.5.2.6") == 1,
          "excluding the function node does not exclude selected declaration descendants");
    result = analyze("module probe; task Data; begin end endtask\nwire data; endmodule\n");
    check(count(result, "GJB10157.5.2.6") == 1,
          "native 5.2.6 collector includes AST_TASK");
    result = analyze("module probe; wire Data; wire Data; endmodule\n");
    check(count(result, "GJB10157.5.2.6") == 0,
          "exact duplicate spelling is not a case-only distinction");
    analysis::AnalysisOptions conditionalCaseCollisionOptions;
    conditionalCaseCollisionOptions.standardId = "GJB10157-2021";
    conditionalCaseCollisionOptions.defines.push_back("CASE_COLLISION");
    const std::string conditionalCaseCollision =
        "module probe;\nwire Data;\n`ifdef CASE_COLLISION\nwire data;\n"
        "`else\nwire other;\n`endif\nendmodule\n";
    result = analysis::Analyzer(conditionalCaseCollisionOptions).analyzeSource(
        {"conditional-collision.sv", core::HdlLanguage::SystemVerilog,
         conditionalCaseCollision});
    check(count(result, "GJB10157.5.2.6") == 1,
          "5.2.6 compares declarations in the configured active branch");
    check(std::ranges::any_of(result.violations, [](const auto& item) {
        return item.ruleId == "GJB10157.5.2.6" && item.line == 4;
    }), "5.2.6 active conditional collision retains the later declaration location");
    conditionalCaseCollisionOptions.defines.clear();
    result = analysis::Analyzer(conditionalCaseCollisionOptions).analyzeSource(
        {"conditional-collision.sv", core::HdlLanguage::SystemVerilog,
         conditionalCaseCollision});
    check(count(result, "GJB10157.5.2.6") == 0,
          "5.2.6 does not compare a declaration from an inactive conditional branch");
    result = analyze("`include \"C:/rtl/defs.vh\"\nmodule probe; endmodule\n");
    check(count(result, "GJB10157.5.2.7") == 1,
          "native include-line colon predicate catches Windows drive path");
    result = analyze("`include \"relative/name:variant.vh\"\nmodule probe; endmodule\n");
    check(count(result, "GJB10157.5.2.7") == 1,
          "native predicate checks colon, not just drive-letter syntax");
    result = analyze("`include \"relative/defs.vh\"\nmodule probe; endmodule\n");
    check(count(result, "GJB10157.5.2.7") == 0,
          "relative include path without colon is not flagged");
    result = analyze("`include \"/rtl/defs.vh\"\nmodule probe; endmodule\n");
    check(count(result, "GJB10157.5.2.7") == 0,
          "native colon predicate does not classify a Unix absolute path");
    result = analyze("// `include \"C:/rtl/defs.vh\"\nmodule probe; endmodule\n");
    check(count(result, "GJB10157.5.2.7") == 1,
          "native physical-line scan includes comment-only text");
    result = analyze("`include C:/rtl/defs.vh\nmodule probe; endmodule\n");
    check(count(result, "GJB10157.5.2.7") == 1,
          "native physical-line scan does not require a quoted include path");
    result = analyze("/* prefix:C */ `include \"relative/defs.vh\"\n"
                     "module probe; endmodule\n");
    check(count(result, "GJB10157.5.2.7") == 1,
          "native predicate accepts a colon anywhere on the include line");
    result = analyze("module probe;\n`ifdef NEVER\n`include \"C:/rtl/defs.vh\"\n"
                     "`endif\nendmodule\n");
    check(count(result, "GJB10157.5.2.7") == 1,
          "native raw-line predicate remains visible in an inactive conditional arm");
    result = analyze("module probe; initial $display(\"`include C:/rtl/defs.vh\"); endmodule\n");
    check(count(result, "GJB10157.5.2.7") == 1,
          "native byte scan does not apply string-literal lexical filtering");
    result = analyze("// `include \"C:/rtl/defs.vh\"\n");
    check(count(result, "GJB10157.5.2.7") == 0,
          "native design map has no AST_MODULE entry for a module-free source");
    result = analyze("`include \"C:/rtl/defs.vh\"\n"
                     "module first; endmodule\nmodule second; endmodule\n");
    check(count(result, "GJB10157.5.2.7") == 2,
          "native design map scans one shared source once per distinct unique module");
    result = analyze("`include \"C:/rtl/defs.vh\"\n"
                     "module duplicate; endmodule\nmodule duplicate; endmodule\n");
    check(count(result, "GJB10157.5.2.7") == 0,
          "native design map skips a module-name entry with more than one AST root");
    result = analyze("`include \"C:/rtl/defs.vh\"\n"
                     "module unique; endmodule\nmodule duplicate; endmodule\n"
                     "module duplicate; endmodule\n");
    check(count(result, "GJB10157.5.2.7") == 1,
          "only uniquely bound module-name entries contribute native file scans");
    result = analyze("module child(input aa); endmodule\n"
                     "module top(input aa); child first(aa); child second(aa); endmodule\n");
    check(count(result, "GJB10157.5.2.9") == 2 &&
          std::ranges::any_of(result.violations, [](const auto& item) {
              return item.ruleId == "GJB10157.5.2.9" &&
                     item.message == "实例first未使用命名关联端口映射连接" &&
                     item.line == 2 && item.column == 1 &&
                     item.severity == analysis::Severity::Error;
          }) && std::ranges::any_of(result.violations, [](const auto& item) {
              return item.ruleId == "GJB10157.5.2.9" &&
                     item.message == "实例second未使用命名关联端口映射连接" &&
                     item.line == 2 && item.column == 1;
          }),
          "native cell admission reports each positional instance with exact text and no-column mapping");
    result = analyze("module child(input aa); endmodule\n"
                     "module first(input aa); child left(aa); endmodule\n"
                     "module second(input aa); child right(aa); endmodule\n");
    check(count(result, "GJB10157.5.2.9") == 2,
          "separate design units each retain their positional-cell finding");
    result = analyze("module child(input aa, input bb); endmodule\n"
                     "module top(input aa, input bb); child both(aa, bb); endmodule\n");
    check(count(result, "GJB10157.5.2.9") == 1,
          "one cell with several unnamed AST_ARGUMENT children reports only once");
    result = analyze("module child(input aa); endmodule\n"
                     "module top(input aa); child first(.aa(aa)); child second(.aa(aa)); endmodule\n");
    check(count(result, "GJB10157.5.2.9") == 0,
          "named-only cell connections do not trigger the positional predicate");
    result = analyze(wrap("wire a;"));
    check(count(result, "GJB10157.5.2.12") >= 1, "internal wire names are checked for length");
    result = analyze(wrap("wire descriptive_signal;"));
    check(count(result, "GJB10157.5.2.12") == 0, "ordinary internal wire names pass length check");
    result = analyze("module probe #(real P_REAL=10.1, time P_TIME=0, "
                     "realtime P_RTIME=0, integer P_SIZE=2)(); endmodule\n");
    check(count(result, "GJB10157.5.2.8") == 3,
          "published real/time/realtime module parameters each report");
    result = analyze("module probe #(parameter P_SIZE=2, integer P_OTHER=3)(); endmodule\n");
    check(count(result, "GJB10157.5.2.8") == 0,
          "published ordinary and integer parameters stay clear");
    result = analyze("module probe; parameter real P_GAIN=1.5; "
                     "localparam time P_DELAY=0; endmodule\n");
    check(count(result, "GJB10157.5.2.8") == 2,
          "explicitly typed body parameter declarations are covered");
    result = analyze("module probe;\n`ifdef ENABLE_REAL\nparameter real P_GAIN=1.5;\n"
                     "`endif\nparameter integer P_COUNT=2;\nendmodule\n");
    check(count(result, "GJB10157.5.2.8") == 0,
          "inactive typed parameter declaration cannot report");
    analysis::AnalysisOptions typedParameterOptions;
    typedParameterOptions.standardId = "GJB10157-2021";
    typedParameterOptions.defines.push_back("ENABLE_REAL");
    result = analysis::Analyzer(typedParameterOptions).analyzeSource(
        {"probe.sv", core::HdlLanguage::SystemVerilog,
         "module probe;\n`ifdef ENABLE_REAL\nparameter real P_GAIN=1.5;\n"
         "`endif\nendmodule\n"});
    check(count(result, "GJB10157.5.2.8") == 1,
          "active typed parameter declaration reports after preprocessing");
    result = analyze("package settings; parameter real P_GAIN=1.5; endpackage\n"
                     "module probe; endmodule\n");
    check(count(result, "GJB10157.5.2.8") == 0,
          "module-parameter subpredicate does not infer package policy");
    result = analyze("module probe; // parameter real COMMENTED=1.5;\n"
                     "parameter integer P_COUNT=2; endmodule\n");
    check(count(result, "GJB10157.5.2.8") == 0,
          "comment text cannot create a typed parameter finding");
    result = analyze("module probe; time T0, T1; realtime RT; real R; "
                     "tri0 N0; tri1 N1; triand NA; trior NO; trireg NR; "
                     "supply0 S0; supply1 S1; uwire U; wand WA; wor WO; endmodule\n");
    const auto recoveredTypeCount = count(result, "GJB10157.5.2.8");
    check(recoveredTypeCount == 14,
          "all thirteen recovered AST_WIRE type branches report per declared object");
    result = analyze("module probe; localparam A=1, B=2; parameter C=3; endmodule\n");
    check(count(result, "GJB10157.5.2.8") == 2,
          "native AST_LOCALPARAM reports each object while AST_PARAMETER is skipped");
    result = analyze("module probe;\n  time T0;\n  localparam A=1;\nendmodule\n");
    check(std::ranges::any_of(result.violations, [](const auto& item) {
        return item.ruleId == "GJB10157.5.2.8" &&
               item.severity == analysis::Severity::Error && item.line == 2 && item.column == 1 &&
               item.message == "禁止使用除reg、wire、integer、tri和parameter之外的数据类型，信号T0是time类型违反.";
    }), "R-2-8 wire-type branch preserves native severity, location and exact diagnostic");
    check(std::ranges::any_of(result.violations, [](const auto& item) {
        return item.ruleId == "GJB10157.5.2.8" &&
               item.severity == analysis::Severity::Error && item.line == 3 && item.column == 1 &&
               item.message == "禁止使用除reg、wire、integer、tri和parameter之外的数据类型，A是localparam类型违反.";
    }), "R-2-8 localparam branch preserves its distinct exact native diagnostic");
    result = analyze("module probe #(parameter real A=1.0, B=2.0, parameter C=3, "
                     "localparam D=4) (input tri0 a, b, output wire c); endmodule\n");
    check(count(result, "GJB10157.5.2.8") == 5,
          "ANSI declarations inherit recovered type and reset it at a new declaration");
    result = analyze("module probe; function real F; real local_r; F=local_r; endfunction "
                     "task T; time local_t; begin end endtask "
                     "always @* begin real block_r; end endmodule\n");
    check(count(result, "GJB10157.5.2.8") == 0,
          "native direct-child scope excludes function task and procedural declarations");
    result = analyze("package settings; localparam A=1; real R; endpackage "
                     "module probe; wire ok; endmodule\n");
    check(count(result, "GJB10157.5.2.8") == 0,
          "native module traversal does not infer package declarations");
    result = analyze("module probe; typedef real sample_real_t; "
                     "parameter type value_t = real; endmodule\n");
    check(count(result, "GJB10157.5.2.8") == 0,
          "native direct-child predicate does not infer typedef or type-parameter policy");

    result = analyze("// Filename: probe.sv\n// Author: tester\n// Description:\nmodule probe;\nendmodule\n");
    check(count(result, "GJB10157.5.1.1") == 0, "compact published header template with present description passes");
    result = analyze("// Filename: probe.sv\n// Author: \n// Description:\nmodule probe;\nendmodule\n");
    check(count(result, "GJB10157.5.1.1") == 0, "compact published example permits empty author");
    result = analyze("// File name: probe.sv\n// Author: \n// Last modified Date: 2020\n"
                     "// History: 2021\n// Descriptions:\nmodule probe;\nendmodule\n");
    check(count(result, "GJB10157.5.1.1") == 1, "detailed published example rejects empty author");
    result = analyze("// Filename: probe.sv\n// Description:\nmodule probe;\nendmodule\n");
    check(count(result, "GJB10157.5.1.1") == 1, "compact template still requires author keyword");
    result = analyze("// Filename: probe.sv\n// Author: tester\n// Last modified Date: 2020\n"
                     "// Description:\nmodule probe;\nendmodule\n");
    check(count(result, "GJB10157.5.1.1") == 1, "detailed revision header requires history");

    const std::string vhdlHead = "--file:probe.vhd\n--Author: tester\n--Description: regression\n"
        "library ieee;\nuse ieee.std_logic_1164.all;\nentity probe is\n"
        "port(clk : in std_logic; aa : in std_logic; qq : out std_logic);\nend;\n"
        "architecture rtl of probe is begin\n";
    analysis::AnalysisOptions vhdlHeaderOptions;
    vhdlHeaderOptions.standardId = "gjb9765";
    vhdlHeaderOptions.verilogHeaderPolicy = {true, {{"Author", false}, {"Revision", true}}};
    result = analysis::Analyzer(vhdlHeaderOptions).analyzeSource(
        {"probe.vhd", core::HdlLanguage::Vhdl,
         "-- author: tester\nentity probe is end entity;\n"});
    check(count(result, "GJB9765.4.13.a") == 1,
          "configured VHDL header reports exactly when an explicit keyword is missing");
    check(std::ranges::any_of(result.violations, [](const auto& item) {
        return item.ruleId == "GJB9765.4.13.a" &&
               item.message.find("Revision") != std::string::npos;
    }), "configured VHDL header preserves missing keyword in diagnostic");
    result = analysis::Analyzer(vhdlHeaderOptions).analyzeSource(
        {"probe.vhd", core::HdlLanguage::Vhdl,
         "-- author: tester\n-- Revision 1\nentity probe is end entity;\n"});
    check(count(result, "GJB9765.4.13.a") == 0,
          "configured VHDL header accepts all explicit keyword matches");
    result = analysis::Analyzer(vhdlHeaderOptions).analyzeSource(
        {"probe.vhd", core::HdlLanguage::Vhdl,
         "library ieee;\n-- author: tester\n-- Revision 1\n"
         "entity probe is end entity;\n"});
    check(count(result, "GJB9765.4.13.a") == 1,
          "VHDL header after the first library clause is too late");
    result = analysis::Analyzer(vhdlHeaderOptions).analyzeSource(
        {"probe.vhd", core::HdlLanguage::Vhdl,
         "-- author: tester\n-- Revision 1\nlibrary ieee;\n"
         "use ieee.std_logic_1164.all;\nentity probe is end entity;\n"});
    check(count(result, "GJB9765.4.13.a") == 0,
          "VHDL header before context clauses satisfies explicit keyword map");
    analysis::AnalysisOptions unknownVhdlHeaderOptions;
    unknownVhdlHeaderOptions.standardId = "gjb9765";
    result = analysis::Analyzer(unknownVhdlHeaderOptions).analyzeSource(
        {"probe.vhd", core::HdlLanguage::Vhdl,
         "entity probe is end entity;\n"});
    check(count(result, "GJB9765.4.13.a") == 1 &&
          std::ranges::any_of(result.standardAssessments, [](const auto& row) {
              return row.clauseId == "GJB9765.4.13.a" &&
                     row.state == analysis::StandardRuleState::Violated;
          }), "unknown keyword map still applies the documented file-start comment requirement");
    result = analysis::Analyzer(unknownVhdlHeaderOptions).analyzeSource(
        {"probe.vhd", core::HdlLanguage::Vhdl,
         "-- English header\nlibrary ieee;\nentity probe is end entity;\n"});
    check(count(result, "GJB9765.4.13.a") == 0 &&
          std::ranges::any_of(result.standardAssessments, [](const auto& row) {
              return row.clauseId == "GJB9765.4.13.a" &&
                     row.state == analysis::StandardRuleState::ManualReviewRequired;
          }), "documented comment fallback accepts a prefix comment without certifying all configured fields");
    result = analysis::Analyzer(unknownVhdlHeaderOptions).analyzeSource(
        {"probe.vhd", core::HdlLanguage::Vhdl,
         "library ieee;\n-- late header\nentity probe is end entity;\n"});
    check(count(result, "GJB9765.4.13.a") == 1,
          "a VHDL comment after the first context clause is not a file-start header");
    vhdlHeaderOptions.verilogHeaderPolicy = {true, {}};
    result = analysis::Analyzer(vhdlHeaderOptions).analyzeSource(
        {"probe.vhd", core::HdlLanguage::Vhdl,
         "entity probe is end entity;\n"});
    check(count(result, "GJB9765.4.13.a") == 0,
          "known empty VHDL keyword map disables the native check");

    result = analyze(
        "entity probe is\nport(\n"
        "  -- adjacent comment does not annotate following ports\n"
        "  aa : in bit;\n"
        "  bb, cc : out bit;\n"
        "  dd : inout bit -- same-line comment\n"
        ");\nend entity;\n", true);
    check(count(result, "GJB9765.4.13.b") == 3,
          "4.13.b reports every unannotated port name and ignores preceding comment");
    check(std::ranges::any_of(result.violations, [](const auto& item) {
        return item.ruleId == "GJB9765.4.13.b" && item.line == 5 &&
               item.message.find("bb") != std::string::npos;
    }) && std::ranges::any_of(result.violations, [](const auto& item) {
        return item.ruleId == "GJB9765.4.13.b" && item.line == 5 &&
               item.message.find("cc") != std::string::npos;
    }), "4.13.b grouped declaration reports each native port candidate");
    result = analyze(
        "entity probe is\nport(\n"
        "  aa, bb : in bit; -- one raw marker covers names on this line\n"
        "  cc,\n"
        "  dd : out bit; -- marker only covers dd identifier line\n"
        "  ee : in string := \"literal--marker\";\n"
        "  ff : in bit\n"
        "  -- following comment is too late\n"
        ");\nend entity;\n"
        "architecture rtl of probe is signal internal : bit; begin end architecture;\n",
        true);
    check(count(result, "GJB9765.4.13.b") == 2,
          "4.13.b uses each name's exact raw line, including literal marker and late-comment boundary");
    check(std::ranges::any_of(result.violations, [](const auto& item) {
        return item.ruleId == "GJB9765.4.13.b" &&
               item.message.find("cc") != std::string::npos;
    }) && std::ranges::any_of(result.violations, [](const auto& item) {
        return item.ruleId == "GJB9765.4.13.b" &&
               item.message.find("ff") != std::string::npos;
    }), "4.13.b identifies multiline and following-line misses without checking internal signals");
    result = analyze(
        "entity probe is\ngeneric(size : integer := 8);\n"
        "port(aa : in bit -- annotated\n);\nend entity;\n", true);
    check(count(result, "GJB9765.4.13.b") == 0,
          "4.13.b excludes generics and accepts an annotated final port declaration");

    result = analyze(
        "entity probe is end entity;\n"
        "architecture rtl of probe is\n"
        "  -- preceding comment is not on a declaration line\n"
        "  signal aa : bit;\n"
        "  signal bb, cc : bit;\n"
        "  signal dd : bit; -- annotated\n"
        "  signal ee\n"
        "    : bit; -- comment on the type line is too late\n"
        "  variable vv : integer := 0;\n"
        "begin end architecture;\n", true);
    check(count(result, "GJB9765.4.13.c") == 5,
          "4.13.c checks every signal or variable name on its exact physical line");
    check(std::ranges::any_of(result.violations, [](const auto& item) {
        return item.ruleId == "GJB9765.4.13.c" &&
               item.message.find("cc") != std::string::npos;
    }) && std::ranges::any_of(result.violations, [](const auto& item) {
        return item.ruleId == "GJB9765.4.13.c" &&
               item.message.find("ee") != std::string::npos;
    }) && std::ranges::any_of(result.violations, [](const auto& item) {
        return item.ruleId == "GJB9765.4.13.c" &&
               item.message.find("vv") != std::string::npos;
    }), "4.13.c covers grouped, multiline and variable declaration candidates");
    result = analyze(
        "entity probe is port(signal pin : in bit); end entity;\n"
        "architecture rtl of probe is\n"
        "  signal aa, bb : bit; -- shared marker\n"
        "  signal cc : string := \"raw--marker\";\n"
        "begin\n"
        "  process variable local : integer := 0; -- annotated\n"
        "  begin null; end process;\n"
        "end architecture;\n", true);
    check(count(result, "GJB9765.4.13.c") == 0,
          "4.13.c accepts raw same-line markers and excludes explicit port-interface signal class");
    result = analyze(
        "entity first_unit is end entity;\n"
        "architecture rtl_first of first_unit is begin end architecture;\n"
        "entity second_unit is end entity;\n"
        "architecture rtl_second of second_unit is begin end architecture;\n", true);
    check(count(result, "GJB9765.4.2.g") == 2,
          "native one-entity-per-file predicate reports every entity when a file contains more than one");
    check(std::ranges::all_of(result.violations, [](const auto& item) {
        return item.ruleId != "GJB9765.4.2.g" || item.line == 1 || item.line == 3;
    }), "4.2.g diagnostics use the physical entity declaration lines");
    result = analyze(
        "-- entity commented is end entity;\n"
        "entity only_unit is end entity;\n"
        "architecture rtl_a of only_unit is begin end architecture;\n"
        "architecture rtl_b of only_unit is begin end architecture;\n", true);
    check(count(result, "GJB9765.4.2.g") == 0,
          "comments and multiple architectures do not increase the native entity count");
    analysis::AnalysisOptions oneEntityPerFileOptions;
    oneEntityPerFileOptions.standardId = "GJB9765-2020";
    const auto splitEntities = analysis::Analyzer(oneEntityPerFileOptions).analyzeSources({
        {"first.vhd", core::HdlLanguage::Vhdl,
         "entity first_unit is end entity; architecture rtl of first_unit is begin end;\n"},
        {"second.vhd", core::HdlLanguage::Vhdl,
         "entity second_unit is end entity; architecture rtl of second_unit is begin end;\n"}
    });
    check(count(splitEntities, "GJB9765.4.2.g") == 0,
          "one entity in each of two project files remains compliant");
    const std::string genericPrefix = "--file:probe.vhd\n--Author: tester\n--Description: regression\n"
        "library ieee; use ieee.std_logic_1164.all;\nentity probe is\n";
    const std::string genericSuffix = ");\nport(clk : in std_logic); end entity;\n"
        "architecture rtl of probe is begin end architecture;\n";
    result = analyze(genericPrefix + "generic(size : integer range 0 to 15 := 8; "
                     "param : natural := 3" + genericSuffix, true);
    check(count(result, "GJB9765.4.12.a") == 0,
          "integer and natural entity generics remain clear");
    result = analyze(genericPrefix + "generic(size : integer := 8; "
                     "name : string := \"x\"; bit_value : std_logic := '0'; "
                     "word : std_logic_vector(size-1 downto 0) := x\"FF\"" + genericSuffix, true);
    check(count(result, "GJB9765.4.12.a") == 3,
          "published noninteger generic types are each diagnosed");
    result = analyze(genericPrefix + "generic(flag : boolean := false; "
                     "delay : time := 1 ns; gain : real := 1.0; "
                     "custom_value : application_integer := 1" + genericSuffix, true);
    check(count(result, "GJB9765.4.12.a") == 3,
          "native noninteger generic branch covers known builtin types but leaves custom subtypes unresolved");
    result = analyze(genericPrefix + "-- generic(fake : string);\n"
                     "generic(size : integer := 8" + genericSuffix, true);
    check(count(result, "GJB9765.4.12.a") == 0,
          "commented generic is not admitted as a declaration");
    result = analyze(vhdlHead + "process begin\nwait until rising_edge(clk);\nqq <= aa;\n"
                     "end process;\nend;\n", true);
    check(count(result, "GJB9765.4.12.a") == 0, "clock-edge wait is not generically forbidden as unsynthesizable");
    result = analyze(vhdlHead + "process begin\nwait for 10 ns;\nqq <= aa;\n"
                     "end process;\nend;\n", true);
    check(count(result, "GJB9765.4.12.a") == 1,
          "native wait-for child form is diagnosed exactly once");
    result = analyze(vhdlHead + "process begin\nwait on aa;\nwait;\n"
                     "end process;\nend;\n", true);
    check(count(result, "GJB9765.4.12.a") == 0,
          "wait-on and bare wait do not satisfy the native wait-for predicate");
    result = analyze(vhdlHead + "process begin\nreport \"wait for 10 ns\"; -- wait for 20 ns\n"
                     "wait until rising_edge(clk);\nend process;\nend;\n", true);
    check(count(result, "GJB9765.4.12.a") == 0,
          "wait-for text in strings and comments is ignored");
    result = analyze(
        "entity probe is end entity;\n"
        "architecture rtl of probe is\n"
        "constant empty_name : string := \"\";\n"
        "constant present_name : string := \"x\";\n"
        "begin end architecture;\n", true);
    check(count(result, "GJB9765.4.12.a") == 1,
          "native exact empty-string constant is diagnosed but a nonempty constant is clear");
    result = analyze(
        "entity probe is end entity;\n"
        "architecture rtl of probe is\n"
        "constant composed : string := \"\" & \"x\";\n"
        "begin assert \"\" = \"\"; end architecture;\n", true);
    check(count(result, "GJB9765.4.12.a") == 0,
          "empty string must be the complete direct constant initializer");
    result = analyze(vhdlHead + "process begin\nwait for 1 ns;\nwait for 2 ns;\n"
                     "end process;\nend;\n", true);
    check(count(result, "GJB9765.4.12.a") == 2,
          "native wait-for traversal reports every admitted statement");
    for (const auto* literal : {"'X'", "'Z'", "'U'", "'H'", "'L'", "'W'", "'-'"}) {
        result = analyze(vhdlHead + "qq <= '1' when aa =\n" + literal + " else '0';\nend;\n", true);
        check(count(result, "GJB9765.4.4.c") == 1, "VHDL comparisons cover all forbidden meta states");
    }
    for (const auto* op : {"=", "/=", "<", "<=", ">", ">="}) {
        result = analyze(vhdlHead + "qq <= '1' when aa " + op +
                         " 'X' else '0';\nend;\n", true);
        check(count(result, "GJB9765.4.4.c") == 1,
              "4.4.c covers every native two-child relational AST kind");
    }
    result = analyze(vhdlHead +
                     "qq <= '1' when 'X' = 'Z' else '0';\nend;\n", true);
    check(count(result, "GJB9765.4.4.c") == 2,
          "4.4.c reports both forbidden direct literal operands independently");
    result = analyze(vhdlHead +
                     "qq <= '1' when (aa and 'X') = '1' else '0';\nend;\n", true);
    check(count(result, "GJB9765.4.4.c") == 0,
          "4.4.c does not promote a nested literal below a non-relational operand AST");
    result = analyze(vhdlHead +
                     "qq <= '1' when boolean'(aa = '1') = true else '0';\nend;\n", true);
    check(count(result, "GJB9765.4.4.c") == 0,
          "4.4.c does not borrow a literal from a nested qualified relation");
    result = analyze(
        "library ieee; use ieee.std_logic_1164.all;\n"
        "entity probe is port(vv: in std_logic_vector(1 downto 0); qq: out std_logic); end;\n"
        "architecture rtl of probe is begin\n"
        "qq <= '1' when vv = \"AZ\" else '0';\nend;\n", true);
    check(count(result, "GJB9765.4.4.c") == 1,
          "4.4.c follows the native direct string search without requiring every character to be a logic state");
    result = analyze(
        "library ieee; use ieee.std_logic_1164.all;\n"
        "entity probe is port(vv: in std_logic_vector(7 downto 0); qq: out std_logic); end;\n"
        "architecture rtl of probe is begin\n"
        "qq <= '1' when vv = X\"5A\" else '0';\nend;\n", true);
    check(count(result, "GJB9765.4.4.c") == 0,
          "4.4.c excludes an X-prefixed based literal before searching its payload");
    result = analyze(vhdlHead + "qq <= 'Z';\nend;\n", true);
    check(count(result, "GJB9765.4.4.c") == 0, "VHDL signal assignment of Z is not a comparison");
    result = analyze(vhdlHead + "process(aa) begin if aa='1' then qq<='1'; end if; end process; end;", true);
    check(count(result, "GJB9765.4.11.a") == 0,
          "ordinary latch risk is not proof of a recognized FSM invalid-state violation");
    result = analyze(vhdlHead + "qq <= aa;" + std::string(90, ' ') + "\n--" +
                     std::string(90, '-') + "\nend;\n", true);
    check(count(result, "GJB9765.4.14.d.1") == 0, "native width excludes trailing whitespace and stops at a literal comment delimiter");
    result = analyze(vhdlHead + std::string(100, ' ') + "qq <= aa;\nend;\n", true);
    check(count(result, "GJB9765.4.14.d.1") == 0, "native width excludes indentation, not just trailing blanks");
    result = analyze(vhdlHead + "qq" + std::string(100, ' ') + "<= aa;\nend;\n", true);
    check(count(result, "GJB9765.4.14.d.1") == 0, "native width excludes internal spacing");
    result = analyze(vhdlHead + "qq <= \"" + std::string(65, '0') + "\";\nend;\n", true);
    check(count(result, "GJB9765.4.14.d.1") == 0, "72 nonblank bytes is permitted");
    result = analyze(vhdlHead + "qq <= \"" + std::string(66, '0') + "\";\nend;\n", true);
    check(count(result, "GJB9765.4.14.d.1") == 1, "73 nonblank bytes is diagnosed");
    result = analyze(vhdlHead + "qq <= \"prefix--" + std::string(90, '0') + "\";\nend;\n", true);
    check(count(result, "GJB9765.4.14.d.1") == 0,
          "native width scan stops at a literal double-hyphen inside quoted text");
    result = analyze(vhdlHead + std::string(253, ' ') + "-" + "-" +
                     std::string(73, 'x') + "\nend;\n", true);
    check(count(result, "GJB9765.4.14.d.1") == 1,
          "native width scan does not join a comment delimiter split across fgets chunks");
    const auto declarationLineViolations = count(analyze(vhdlHead + "end;\n", true), "GJB9765.4.14.d.2");
    result = analyze(vhdlHead + "qq <= aa; -- ; ; ;\nend;\n", true);
    check(count(result, "GJB9765.4.14.d.2") == declarationLineViolations, "comment semicolons do not count as statements");
    result = analyze(vhdlHead + "qq <= \";;\";\nend;\n", true);
    check(count(result, "GJB9765.4.14.d.2") == declarationLineViolations + 1,
          "native raw loop counts semicolons inside quoted text");
    result = analyze(vhdlHead + "qq <= \"prefix--;;\"; qq <= aa;\nend;\n", true);
    check(count(result, "GJB9765.4.14.d.2") == declarationLineViolations,
          "native statement scan stops at a literal double-hyphen inside quoted text");
    result = analyze(vhdlHead + std::string(253, ' ') + "-" + "-;;\nend;\n", true);
    check(count(result, "GJB9765.4.14.d.2") == declarationLineViolations + 1,
          "native statement scan does not join a comment delimiter split across fgets chunks");

    const auto clockWrap = [](const std::string& body) {
        const std::string ctx = "library ieee; use ieee.std_logic_1164.all;\n";
        return ctx + "entity dut is port(clk,rst,din:in std_logic; dout:out std_logic); end entity;\n" +
            ctx + "architecture rtl of dut is begin\n" + body + "\nend architecture;\n";
    };
    result = analyze(clockWrap("process(clk) begin if rising_edge(clk) then if clk='0' then "
        "dout<=din; end if; end if; end process;"), true);
    check(count(result, "GJB9765.4.10.h") == 1, "native clock/control intersection reaches standard evaluator");
    result = analyze(clockWrap("process(clk) begin if rising_edge(clk) then dout<='0'; end if; end process;"), true);
    check(count(result, "GJB9765.4.10.h") == 1,
          "packaged pure-edge constant reset/set example reaches standard evaluator");
    result = analyze(clockWrap("process(clk) begin if rising_edge(clk) then dout<=din; end if; end process;"), true);
    check(count(result, "GJB9765.4.10.h") == 0,
          "ordinary edge-triggered data assignment is not reset/set misuse");
    check(std::ranges::any_of(result.standardAssessments, [](const auto& evaluation) {
        return evaluation.clauseId == "GJB9765.4.10.h" &&
            evaluation.state == analysis::StandardRuleState::ManualReviewRequired;
    }), "partial clock detector no alarm stays manual review");

    result = analyze(
        "entity probe is port(clk,a,b:in bit; d:out bit); end entity;\n"
        "architecture rtl of probe is signal my_temp1,my_temp2:bit; begin\n"
        "my_temp1 <= a and my_temp2;\n"
        "p: process(my_temp1,clk) begin if my_temp1='1' then my_temp2<='0'; "
        "elsif rising_edge(clk) then my_temp2<=b; end if; end process;\n"
        "d<=my_temp2; end architecture;\n", true);
    check(count(result, "GJB9765.4.10.a") == 1,
          "attachment 4.10.a reports register output fed through AND logic to its asynchronous reset");
    result = analyze(
        "entity probe is port(clk,rst,a:in bit; q:out bit); end entity;\n"
        "architecture rtl of probe is begin p: process(rst,clk) begin "
        "if rst='1' then q<='0'; elsif rising_edge(clk) then q<=a; end if; "
        "end process; end architecture;\n", true);
    check(count(result, "GJB9765.4.10.a") == 0,
          "4.10.a accepts an independent primary asynchronous reset");
    result = analyze(
        "entity probe is port(clk,a,b:in bit; q:out bit); end entity;\n"
        "architecture rtl of probe is signal feedback,reset_alias:bit; begin\n"
        "feedback<=q or a; reset_alias<=feedback;\n"
        "p: process(reset_alias,clk) begin if reset_alias='1' then q<='0'; "
        "elsif rising_edge(clk) then q<=b; end if; end process; end architecture;\n", true);
    check(count(result, "GJB9765.4.10.a") == 1,
          "4.10.a follows a unique direct alias after a proven combinational feedback driver");
    result = analyze(
        "entity probe is port(clk,a,b:in bit; q1,q2:out bit); end entity;\n"
        "architecture rtl of probe is signal reset_i:bit; begin reset_i<=q2 and a;\n"
        "p1: process(reset_i,clk) begin if reset_i='1' then q1<='0'; "
        "elsif rising_edge(clk) then q1<=b; end if; end process;\n"
        "p2: process(clk) begin if rising_edge(clk) then q2<=b; end if; end process;\n"
        "end architecture;\n", true);
    check(count(result, "GJB9765.4.10.a") == 0,
          "4.10.a does not report feedback from a different register output");
    check(std::ranges::any_of(result.standardAssessments, [](const auto& evaluation) {
        return evaluation.clauseId == "GJB9765.4.10.a" &&
            evaluation.state == analysis::StandardRuleState::ManualReviewRequired;
    }), "partial asynchronous-feedback detector keeps unsupported clean paths under review");

    result = analyze(
        "entity probe is port(clk,rst_a,rst_b,din:in bit; dout:out bit); end entity;\n"
        "architecture rtl of probe is signal rst_n:bit; begin rst_n<=rst_a and rst_b;\n"
        "p: process(clk,rst_n) begin if rst_n='0' then dout<='0'; "
        "elsif rising_edge(clk) then dout<=din; end if; end process; end architecture;\n", true);
    check(count(result, "GJB9765.4.10.k") == 1,
          "attachment 4.10.k reports an AND-generated asynchronous reset without synchronization");
    result = analyze(
        "entity probe is port(clk,rst_a,rst_b,din:in bit; dout:out bit); end entity;\n"
        "architecture rtl of probe is signal rst_n,sync0,sync1:bit; begin "
        "rst_n<=rst_a and rst_b;\n"
        "ps: process(clk) begin if rising_edge(clk) then sync0<=rst_n; sync1<=sync0; "
        "end if; end process;\n"
        "pd: process(clk,sync1) begin if sync1='0' then dout<='0'; "
        "elsif rising_edge(clk) then dout<=din; end if; end process; end architecture;\n", true);
    check(count(result, "GJB9765.4.10.k") == 0,
          "attachment 4.10.k accepts a combinational reset after two clocked synchronization stages");
    result = analyze(
        "entity probe is port(clk,rst_a,rst_b,din:in bit; dout:out bit); end entity;\n"
        "architecture rtl of probe is signal rst_n,rst_alias:bit; begin "
        "rst_n<=rst_a xor rst_b; rst_alias<=rst_n;\n"
        "p: process(clk,rst_alias) begin if rst_alias='0' then dout<='0'; "
        "elsif rising_edge(clk) then dout<=din; end if; end process; end architecture;\n", true);
    check(count(result, "GJB9765.4.10.k") == 1,
          "4.10.k follows a unique direct alias back to a combinational reset driver");
    result = analyze(
        "entity probe is port(clk,rst,din:in bit; dout:out bit); end entity;\n"
        "architecture rtl of probe is begin p: process(clk,rst) begin "
        "if rst='0' then dout<='0'; elsif rising_edge(clk) then dout<=din; "
        "end if; end process; end architecture;\n", true);
    check(count(result, "GJB9765.4.10.k") == 0,
          "4.10.k accepts a direct external asynchronous reset");
    check(std::ranges::any_of(result.standardAssessments, [](const auto& evaluation) {
        return evaluation.clauseId == "GJB9765.4.10.k" &&
            evaluation.state == analysis::StandardRuleState::ManualReviewRequired;
    }), "partial combinational-reset detector keeps unsupported clean paths under review");

    result = analyze(
        "entity probe is port(clk,rst_n,data:in bit; q:out bit); end entity;\n"
        "architecture rtl of probe is begin p: process(clk,rst_n) begin "
        "if rst_n='0' then q<='0'; elsif rising_edge(clk) then q<=data; "
        "end if; end process; end architecture;\n", true);
    check(count(result, "GJB9765.4.10.c") == 1,
          "attachment 4.10.c reports raw asynchronous reset release on a business register");
    result = analyze(
        "entity probe is port(clk,rst_n,data:in bit; q:out bit); end entity;\n"
        "architecture rtl of probe is signal rst_sync0,rst_sync1:bit; begin "
        "ps: process(clk,rst_n) begin if rst_n='0' then "
        "rst_sync0<='0'; rst_sync1<='0'; elsif rising_edge(clk) then "
        "rst_sync0<='1'; rst_sync1<=rst_sync0; end if; end process; "
        "pd: process(clk,rst_sync1) begin if rst_sync1='0' then q<='0'; "
        "elsif rising_edge(clk) then q<=data; end if; end process; "
        "end architecture;\n", true);
    check(count(result, "GJB9765.4.10.c") == 0,
          "attachment 4.10.c accepts asynchronous assertion with two-stage synchronous release");
    result = analyze(
        "entity probe is port(clk,rst_n,data:in bit; q:out bit); end entity;\n"
        "architecture rtl of probe is signal rst_sync0:bit; begin "
        "ps: process(clk,rst_n) begin if rst_n='0' then rst_sync0<='0'; "
        "elsif rising_edge(clk) then rst_sync0<='1'; end if; end process; "
        "pd: process(clk,rst_sync0) begin if rst_sync0='0' then q<='0'; "
        "elsif rising_edge(clk) then q<=data; end if; end process; "
        "end architecture;\n", true);
    check(count(result, "GJB9765.4.10.c") > 0,
          "4.10.c does not accept a single release stage as the proven two-stage chain");
    result = analyze(
        "entity probe is port(clk,rst_n,data:in bit; q:out bit); end entity;\n"
        "architecture rtl of probe is signal rst_sync0,rst_sync1:bit; begin "
        "ps: process(clk,rst_n) begin if rst_n='0' then "
        "rst_sync0<='0'; rst_sync1<='0'; elsif rising_edge(clk) then "
        "rst_sync0<='1'; rst_sync1<=rst_sync0; end if; end process; "
        "pd: process(clk,rst_n) begin if rst_n='0' then q<='0'; "
        "elsif rising_edge(clk) then q<=data; end if; end process; "
        "end architecture;\n", true);
    check(count(result, "GJB9765.4.10.c") == 1,
          "4.10.c still reports a raw-reset business process when an unused synchronizer exists");
    result = analyze(
        "entity probe is port(clk,rst_n,data:in bit; q:out bit); end entity;\n"
        "architecture rtl of probe is begin p: process(clk) begin "
        "if rising_edge(clk) then if rst_n='0' then q<='0'; else q<=data; "
        "end if; end if; end process; end architecture;\n", true);
    check(count(result, "GJB9765.4.10.c") == 0,
          "4.10.c excludes a synchronous reset that is absent from the sensitivity list");
    result = analyze(
        "entity probe is port(clk,rst,data:in bit; q:out bit); end entity;\n"
        "architecture rtl of probe is signal rst_sync0,rst_sync1:bit; begin "
        "ps: process(clk,rst) begin if rst='1' then "
        "rst_sync0<='1'; rst_sync1<='1'; elsif rising_edge(clk) then "
        "rst_sync0<='0'; rst_sync1<=rst_sync0; end if; end process; "
        "pd: process(clk,rst_sync1) begin if rst_sync1='1' then q<='0'; "
        "elsif rising_edge(clk) then q<=data; end if; end process; "
        "end architecture;\n", true);
    check(count(result, "GJB9765.4.10.c") == 0,
          "4.10.c recognizes the equivalent active-high two-stage release chain");

    result = analyze(
        "entity probe is port(clk,rst,din:in bit; dout:out bit); end entity;\n"
        "architecture rtl of probe is begin p: process(clk,rst) begin "
        "if rising_edge(clk) then if rst='0' then dout<='0'; else dout<=din; "
        "end if; end if; end process; end architecture;\n", true);
    check(count(result, "GJB9765.4.10.i") == 1,
          "attachment 4.10.i reports an external input used directly as synchronous reset");
    result = analyze(
        "entity probe is port(clk,rst,din:in bit; dout:out bit); end entity;\n"
        "architecture rtl of probe is signal rst_sync0,rst_sync1:bit:='0'; begin "
        "ps: process(clk) begin if rising_edge(clk) then rst_sync0<=rst; "
        "rst_sync1<=rst_sync0; end if; end process; "
        "pd: process(clk,rst_sync1) begin if rising_edge(clk) then "
        "if rst_sync1='0' then dout<='0'; else dout<=din; end if; end if; "
        "end process; end architecture;\n", true);
    check(count(result, "GJB9765.4.10.i") == 0,
          "attachment 4.10.i accepts the internal second-stage synchronous reset signal");
    result = analyze(
        "entity probe is port(clk,rst,din:in bit; dout:out bit); end entity;\n"
        "architecture rtl of probe is begin p: process(clk,rst) begin "
        "if rst='0' then dout<='0'; elsif rising_edge(clk) then dout<=din; "
        "end if; end process; end architecture;\n", true);
    check(count(result, "GJB9765.4.10.i") == 0,
          "4.10.i excludes direct asynchronous-reset relations handled by other clauses");
    result = analyze(
        "entity probe is port(clk,enable,din:in bit; dout:out bit); end entity;\n"
        "architecture rtl of probe is begin p: process(clk) begin "
        "if rising_edge(clk) then if enable='1' then dout<=din; end if; "
        "end if; end process; end architecture;\n", true);
    check(count(result, "GJB9765.4.10.i") == 0,
          "4.10.i does not relabel an ordinary synchronous enable as reset");

    result = analyze(
        "entity probe is port(clk,data:in bit; dout:out bit); end entity;\n"
        "architecture rtl of probe is signal state:bit; begin p: process(clk) begin "
        "if rising_edge(clk) then state<=data; end if; end process; "
        "dout<=state; end architecture;\n", true);
    check(count(result, "GJB9765.4.10.e") == 1,
          "attachment 4.10.e reports a clocked register target without reset initialization");
    result = analyze(
        "entity probe is port(clk,rst_n,data:in bit; dout:out bit); end entity;\n"
        "architecture rtl of probe is signal state:bit; begin p: process(clk,rst_n) begin "
        "if rst_n='0' then state<='0'; elsif rising_edge(clk) then state<=data; "
        "end if; end process; dout<=state; end architecture;\n", true);
    check(count(result, "GJB9765.4.10.e") == 0,
          "attachment 4.10.e accepts a register initialized in an asynchronous reset branch");
    result = analyze(
        "entity probe is port(clk,rst_n,data:in bit; dout:out bit); end entity;\n"
        "architecture rtl of probe is signal state:bit; begin p: process(clk) begin "
        "if rising_edge(clk) then if rst_n='0' then state<='0'; else state<=data; "
        "end if; end if; end process; dout<=state; end architecture;\n", true);
    check(count(result, "GJB9765.4.10.e") == 0,
          "4.10.e accepts a register initialized in a parser-recognized synchronous reset branch");
    result = analyze(
        "entity probe is port(clk,rst_n,a,b:in bit; q1,q2:out bit); end entity;\n"
        "architecture rtl of probe is begin p: process(clk,rst_n) begin "
        "if rst_n='0' then q1<='0'; elsif rising_edge(clk) then q1<=a; q2<=b; "
        "end if; end process; end architecture;\n", true);
    check(count(result, "GJB9765.4.10.e") == 1 &&
          std::ranges::any_of(result.violations, [](const auto& item) {
              return item.ruleId == "GJB9765.4.10.e" &&
                  item.message.find("q2") != std::string::npos;
          }), "4.10.e reports only the unreset target in a mixed-coverage process");
    result = analyze(
        "entity probe is port(a,b:in bit; q:out bit); end entity;\n"
        "architecture rtl of probe is begin p: process(a,b) begin q<=a and b; "
        "end process; end architecture;\n", true);
    check(count(result, "GJB9765.4.10.e") == 0,
          "4.10.e excludes a combinational process with no clock relation");

    result = analyze(
        "entity probe is port(clk, rst, set : in bit; q1, q2, q3, q4 : out bit); end;\n"
        "architecture rtl of probe is begin\n"
        "process(rst, clk) begin if rst = '1' then q1 <= '0'; "
        "elsif rising_edge(clk) then q1 <= '1'; end if; end process;\n"
        "process(rst, clk) begin if rst = '0' then q2 <= '0'; "
        "elsif rising_edge(clk) then q2 <= '1'; end if; end process;\n"
        "process(set, clk) begin if set = '1' then q3 <= '1'; "
        "elsif rising_edge(clk) then q3 <= '0'; end if; end process;\n"
        "process(set, clk) begin if set = '0' then q4 <= '1'; "
        "elsif rising_edge(clk) then q4 <= '0'; end if; end process;\n"
        "end architecture;\n", true);
    check(count(result, "GJB9765.4.10.j") == 2,
          "4.10.j reports independent reset and set active-level conflicts");
    check(std::ranges::any_of(result.violations, [](const auto& item) {
        return item.ruleId == "GJB9765.4.10.j" && item.signal == "rst";
    }) && std::ranges::any_of(result.violations, [](const auto& item) {
        return item.ruleId == "GJB9765.4.10.j" && item.signal == "set";
    }), "4.10.j retains both native asynchronous-control signal identities");
    result = analyze(
        "entity probe is port(clk, set : in bit; q1, q2 : out bit); end;\n"
        "architecture rtl of probe is begin\n"
        "process(set, clk) begin if set = '1' then q1 <= '1'; "
        "elsif rising_edge(clk) then q1 <= '0'; end if; end process;\n"
        "process(set, clk) begin if set = '1' then q2 <= '1'; "
        "elsif rising_edge(clk) then q2 <= '0'; end if; end process;\n"
        "end architecture;\n", true);
    check(count(result, "GJB9765.4.10.j") == 0,
          "4.10.j accepts repeated use of one set signal with a unique active level");
    result = analyze(
        "entity probe is port(clk, set : in bit; q : out bit); end;\n"
        "architecture rtl of probe is begin\n"
        "process(set, clk) begin if set = '0' then q <= '1'; "
        "elsif rising_edge(clk) then q <= '0'; end if; end process;\n"
        "end architecture;\n", true);
    check(count(result, "GJB9765.4.10.j") == 0,
          "4.10.j does not invent a name-based high level beside an explicit low level");

    result = analyze("module _probe; wire good__bad, trailing_; task _task; endtask "
                     "function integer _func; _func=0; endfunction endmodule");
    check(count(result, "GJB10157.5.2.1") == 3, "module/task/function first-byte checks integrated");
    check(std::ranges::all_of(result.violations, [](const auto& item) {
        return item.ruleId != "GJB10157.5.2.1" ||
            (item.severity == analysis::Severity::Error && item.column == 1 &&
             item.message.ends_with("命名必须以字母开始"));
    }), "native first-byte diagnostics preserve Error, line-only location and exact suffix");
    check(std::ranges::any_of(result.violations, [](const auto& item) {
        return item.ruleId == "GJB10157.5.2.1" &&
            item.message == "_probe命名必须以字母开始";
    }), "native first-byte diagnostic prefixes the normalized declaration name");
    check(count(result, "GJB10157.5.2.2") == 2, "double/trailing underscore net checks integrated");
    check(std::ranges::any_of(result.violations, [](const auto& item) {
        return item.ruleId == "GJB10157.5.2.2" &&
            item.severity == analysis::Severity::Error && item.column == 1 &&
            item.message == "good__bad:禁止命名中使用连续的\"__\"";
    }), "native double-underscore diagnostic preserves Error, line-only location and exact text");
    check(std::ranges::any_of(result.violations, [](const auto& item) {
        return item.ruleId == "GJB10157.5.2.2" &&
            item.severity == analysis::Severity::Error && item.column == 1 &&
            item.message == "trailing_:禁止命名末尾使用\"_\"";
    }), "native trailing-underscore diagnostic preserves exact text");
    result = analyze("module probe; wire both__; endmodule\n");
    check(count(result, "GJB10157.5.2.2") == 1 &&
          std::ranges::any_of(result.violations, [](const auto& item) {
              return item.ruleId == "GJB10157.5.2.2" &&
                  item.message == "both__:禁止命名中使用连续的\"__\"";
          }), "double underscore branch has native priority over trailing underscore branch");
    result = analyze("module probe; wire same_, same_; endmodule\n");
    check(count(result, "GJB10157.5.2.2") == 2,
          "two identical same-line underscore candidates retain native per-node multiplicity");
    result = analyze("module dup; wire bad_; endmodule module dup; wire bad_; endmodule\n");
    check(count(result, "GJB10157.5.2.2") == 0,
          "duplicate same-name module map group is excluded before underscore checks");
    result = analyze("module one; wire bad_; endmodule module two; wire bad_; endmodule\n");
    check(count(result, "GJB10157.5.2.2") == 2,
          "distinct unique module-map groups receive independent underscore checks");
    result = analyze("module probe; wire _same, _same; endmodule\n");
    check(count(result, "GJB10157.5.2.1") == 2,
          "two identical same-line AST_WIRE candidates retain native per-node multiplicity");
    result = analyze("module _dup; endmodule module _dup; endmodule\n");
    check(count(result, "GJB10157.5.2.1") == 0,
          "duplicate same-name module map group is excluded before first-byte checks");
    result = analyze("module _one; endmodule module _two; endmodule\n");
    check(count(result, "GJB10157.5.2.1") == 2,
          "distinct unique module-map groups are checked independently");
    result = analyze("module probe(input 1_B1, input B1); endmodule\n");
    check(count(result, "GJB10157.5.2.1") == 1,
          "malformed numeric-leading ANSI port name is checked in declaration position");
    result = analyze("module probe(input B1, input [7:0] data); wire [7:0] value = 8'h01; endmodule\n");
    check(count(result, "GJB10157.5.2.1") == 0,
          "ordinary ANSI ports, range numbers and based literals do not trigger first-byte rule");
    result = analyze("module probe(input B1); // input 1_B1\n wire [1:0] data; endmodule\n");
    check(count(result, "GJB10157.5.2.1") == 0,
          "numeric-looking names in comments and dimensions are not declarations");
    result = analyze("module probe(input 1'b0); endmodule\n");
    check(count(result, "GJB10157.5.2.1") == 0,
          "based literal in malformed port list is not recovered as an identifier");
    result = analyze("module probe; function integer fn__bad_; fn__bad_=0; endfunction endmodule");
    check(count(result, "GJB10157.5.2.2") == 0, "native second collector excludes function name");
    result = analyze("module probe; function integer fn; reg inner_; fn=0; endfunction endmodule");
    check(count(result, "GJB10157.5.2.2") == 1,
          "function declaration is excluded while its selected declaration descendants remain included");
    result = analyze("module probe; wire " + std::string(32, 'a') + "; endmodule");
    check(count(result, "GJB10157.5.2.12") == 0, "native inclusive 32-byte bound integrated");
    result = analyze("module probe; wire " + std::string(33, 'a') + "; endmodule");
    check(count(result, "GJB10157.5.2.12") == 1, "native 33-byte bound integrated");
    result = analyze("module probe; function integer " + std::string(33, 'a') +
                     "; endfunction endmodule");
    check(count(result, "GJB10157.5.2.12") == 0, "length collector excludes function definition name");
    for (const auto* rule : {"GJB10157.5.2.1", "GJB10157.5.2.2", "GJB10157.5.2.12"})
        check(std::ranges::any_of(result.standardAssessments, [&](const auto& evaluation) {
            return evaluation.clauseId == rule && evaluation.state == analysis::StandardRuleState::ManualReviewRequired;
        }), "partial name detector without alarm stays manual review");
    result = analyze("module a; wire b; endmodule module a; wire c; endmodule");
    check(count(result, "GJB10157.5.2.12") == 0,
          "native module-name map skips the complete duplicate-module group");
    result = analyze("module a; endmodule module b; endmodule");
    check(count(result, "GJB10157.5.2.12") == 2,
          "different one-byte module names remain independent admitted groups");
    check(std::ranges::all_of(result.violations, [](const auto& item) {
        return item.ruleId != "GJB10157.5.2.12" ||
            (item.severity == analysis::Severity::Error && item.column == 1);
    }), "native length diagnostics use error severity and line-only location");
    check(std::ranges::any_of(result.violations, [](const auto& item) {
        return item.ruleId == "GJB10157.5.2.12" &&
            item.message == "a:建议命名长度在2到32个字符之间";
    }), "native length diagnostic text is preserved");
    analysis::AnalysisOptions conditionalShapeOptions;
    conditionalShapeOptions.standardId = "GJB10157-2021";
    conditionalShapeOptions.defines.push_back("USE_BAD_SHAPES");
    const std::string conditionalShapes =
        "module probe;\n`ifdef USE_BAD_SHAPES\nwire _bad;\nwire bad__name;\n"
        "wire VDD;\nwire x;\n`else\nwire safe_name;\n`endif\nendmodule\n";
    result = analysis::Analyzer(conditionalShapeOptions).analyzeSource(
        {"conditional-shapes.sv", core::HdlLanguage::SystemVerilog,
         conditionalShapes});
    for (const auto* rule : {"GJB10157.5.2.1", "GJB10157.5.2.2",
                             "GJB10157.5.2.4", "GJB10157.5.2.12"})
        check(count(result, rule) == 1,
              "configured active declaration reaches each recovered naming-shape predicate");
    check(std::ranges::any_of(result.violations, [](const auto& item) {
        return item.ruleId == "GJB10157.5.2.1" && item.line == 3;
    }) && std::ranges::any_of(result.violations, [](const auto& item) {
        return item.ruleId == "GJB10157.5.2.2" && item.line == 4;
    }) && std::ranges::any_of(result.violations, [](const auto& item) {
        return item.ruleId == "GJB10157.5.2.4" && item.line == 5;
    }) && std::ranges::any_of(result.violations, [](const auto& item) {
        return item.ruleId == "GJB10157.5.2.12" && item.line == 6;
    }), "active naming-shape findings retain their physical source lines");
    conditionalShapeOptions.defines.clear();
    result = analysis::Analyzer(conditionalShapeOptions).analyzeSource(
        {"conditional-shapes.sv", core::HdlLanguage::SystemVerilog,
         conditionalShapes});
    for (const auto* rule : {"GJB10157.5.2.1", "GJB10157.5.2.2",
                             "GJB10157.5.2.4", "GJB10157.5.2.12"})
        check(count(result, rule) == 0,
              "inactive declaration cannot trigger a recovered naming-shape predicate");

    const std::string fsmContext =
        "library ieee; use ieee.std_logic_1164.all;\n"
        "entity probe is port(clk,sel:in std_logic; q:out std_logic); end entity;\n";
    result = analyze(fsmContext +
        "architecture rtl of probe is signal state:std_logic_vector(1 downto 0); begin\n"
        "process(clk) begin if rising_edge(clk) then case state is "
        "when \"00\"=>state<=\"01\"; when \"01\"=>state<=\"00\"; "
        "when others=>state<=\"00\"; end case; end if; end process;\n"
        "end architecture;\n", true);
    check(count(result, "GJB9765.4.11.e") == 1,
          "4.11.e detects a source-proved one-process FSM with a non-enumeration state type");
    result = analyze(fsmContext +
        "architecture rtl of probe is type state_t is(idle,active); signal state:state_t; begin\n"
        "process(clk) begin if rising_edge(clk) then case state is "
        "when idle=>state<=active; when active=>state<=idle; end case; end if; end process;\n"
        "end architecture;\n", true);
    check(count(result, "GJB9765.4.11.e") == 0,
          "4.11.e accepts an enumeration-declared one-process FSM");
    result = analyze(fsmContext +
        "architecture rtl of probe is signal state,next_state:integer range 0 to 2; begin\n"
        "process(state,sel) begin case state is when 0=>next_state<=1; "
        "when 1=>next_state<=0; when others=>next_state<=0; end case; end process;\n"
        "process(clk) begin if rising_edge(clk) then state<=next_state; end if; end process;\n"
        "end architecture;\n", true);
    check(count(result, "GJB9765.4.11.e") == 1,
          "4.11.e detects a source-proved two-process FSM with a non-enumeration state type");
    result = analyze(fsmContext +
        "architecture rtl of probe is signal sel2:std_logic_vector(1 downto 0); begin\n"
        "process(sel2) begin case sel2 is when \"00\"=>q<='0'; "
        "when \"01\"=>q<='1'; when others=>q<='0'; end case; end process;\n"
        "end architecture;\n", true);
    check(count(result, "GJB9765.4.11.e") == 0,
          "4.11.e excludes an ordinary combinational decoder case");
    check(std::ranges::any_of(result.standardAssessments, [](const auto& evaluation) {
        return evaluation.clauseId == "GJB9765.4.11.e" &&
            evaluation.state == analysis::StandardRuleState::ManualReviewRequired;
    }), "unproved non-enumeration FSM cases remain manual review");

    result = analyze(fsmContext +
        "architecture rtl of probe is type state_t is(idle,active); signal state:state_t; begin\n"
        "process(clk) begin if rising_edge(clk) then case state is "
        "when idle=>state<=active; when active=>state<=idle; end case; end if; end process;\n"
        "end architecture;\n", true);
    check(count(result, "GJB9765.4.11.c") == 1,
          "4.11.c detects a recognized FSM without reset or declaration initialization");
    result = analyze(fsmContext +
        "architecture rtl of probe is type state_t is(idle,active); signal state:state_t; begin\n"
        "process(clk) begin if sel='1' then state<=idle; elsif rising_edge(clk) then "
        "case state is when idle=>state<=active; when active=>state<=idle; end case; "
        "end if; end process; end architecture;\n", true);
    check(count(result, "GJB9765.4.11.c") == 0,
          "4.11.c accepts a reset assignment to a resolved legal state");
    result = analyze(fsmContext +
        "architecture rtl of probe is type state_t is(idle,active); signal state:state_t; begin\n"
        "process(clk) begin if sel='1' then state<=illegal; elsif rising_edge(clk) then "
        "case state is when idle=>state<=active; when active=>state<=idle; end case; "
        "end if; end process; end architecture;\n", true);
    check(count(result, "GJB9765.4.11.c") == 1,
          "4.11.c rejects a reset assignment outside the resolved legal-state set");
    result = analyze(fsmContext +
        "architecture rtl of probe is type state_t is(idle,active); "
        "signal state:state_t:=idle; begin process(clk) begin if rising_edge(clk) then "
        "case state is when idle=>state<=active; when active=>state<=idle; end case; "
        "end if; end process; end architecture;\n", true);
    check(count(result, "GJB9765.4.11.c") == 0,
          "4.11.c accepts a declaration initializer in the legal enumeration set");
    result = analyze(fsmContext +
        "architecture rtl of probe is signal state,next_state:integer range 0 to 2; begin\n"
        "process(state,sel) begin case state is when 0=>next_state<=1; "
        "when 1=>next_state<=0; when others=>next_state<=0; end case; end process;\n"
        "process(clk) begin if rising_edge(clk) then state<=next_state; end if; end process;\n"
        "end architecture;\n", true);
    check(count(result, "GJB9765.4.11.c") == 1,
          "4.11.c detects a two-process FSM whose transfer process has no initialization");

    const auto invalidStateFsm = [&](const std::string& branches) {
        return fsmContext +
            "architecture rtl of probe is signal state,next_state:std_logic_vector(1 downto 0); begin\n"
            "process(state,sel) begin case state is " + branches +
            " end case; end process;\n"
            "process(clk) begin if sel='1' then state<=\"00\"; "
            "elsif rising_edge(clk) then state<=next_state; end if; end process;\n"
            "end architecture;\n";
    };
    result = analyze(invalidStateFsm(
        "when \"00\"=>q<='0';next_state<=\"01\"; "
        "when \"01\"=>q<='1';next_state<=\"00\"; "
        "when others=>next_state<=\"00\";"), true);
    check(count(result, "GJB9765.4.11.a") == 1,
          "4.11.a detects an others branch missing an explicit-state assignment target");
    result = analyze(invalidStateFsm(
        "when \"00\"=>q<='0';next_state<=\"01\"; "
        "when \"01\"=>q<='1';next_state<=\"00\"; "
        "when others=>q<='0';next_state<=\"00\";"), true);
    check(count(result, "GJB9765.4.11.a") == 0,
          "4.11.a accepts an others branch covering every explicit-state assignment target");
    result = analyze(invalidStateFsm(
        "when \"00\"=>q<='0';next_state<=\"01\"; "
        "when \"01\"=>q<='1';next_state<=\"00\"; when others=>null;"), true);
    check(count(result, "GJB9765.4.11.a") == 1,
          "4.11.a detects an empty others branch");
    result = analyze(invalidStateFsm(
        "when \"00\"=>next_state<=\"01\"; when \"01\"=>next_state<=\"10\"; "
        "when \"10\"=>next_state<=\"00\";"), true);
    check(count(result, "GJB9765.4.11.a") == 1,
          "4.11.a requires invalid-state handling when explicit binary states do not fill the encoding space");
    result = analyze(invalidStateFsm(
        "when \"00\"=>next_state<=\"01\"; when \"01\"=>next_state<=\"10\"; "
        "when \"10\"=>next_state<=\"11\"; when \"11\"=>next_state<=\"00\";"), true);
    check(count(result, "GJB9765.4.11.a") == 0,
          "4.11.a permits a fully covered binary state encoding without others");

    result = analyze(invalidStateFsm(
        "when \"00\"=>q<='0';next_state<=\"01\"; "
        "when \"01\"=>q<='1';next_state<=\"00\"; "
        "when others=>q<='0';next_state<=\"00\";"), true);
    check(count(result, "GJB9765.4.11.d") == 1,
          "4.11.d detects output decoding mixed into a two-process next-state decoder");
    result = analyze(invalidStateFsm(
        "when \"00\"=>next_state<=\"01\"; when \"01\"=>next_state<=\"00\"; "
        "when others=>next_state<=\"00\";"), true);
    check(count(result, "GJB9765.4.11.d") == 1,
          "4.11.d requires a separate output decoder in addition to pure next-state logic");
    result = analyze(fsmContext +
        "architecture rtl of probe is signal state,next_state:std_logic_vector(1 downto 0); begin\n"
        "process(state,sel) begin case state is when \"00\"=>next_state<=\"01\"; "
        "when \"01\"=>next_state<=\"00\"; when others=>next_state<=\"00\"; end case; end process;\n"
        "process(state) begin case state is when \"00\"=>q<='0'; when \"01\"=>q<='1'; "
        "when others=>q<='0'; end case; end process;\n"
        "process(clk) begin if sel='1' then state<=\"00\"; elsif rising_edge(clk) then "
        "state<=next_state; end if; end process; end architecture;\n", true);
    check(count(result, "GJB9765.4.11.d") == 0,
          "4.11.d accepts a source-proved three-process FSM");
    result = analyze(fsmContext +
        "architecture rtl of probe is signal state:std_logic_vector(1 downto 0); begin\n"
        "process(clk) begin if sel='1' then state<=\"00\"; elsif rising_edge(clk) then "
        "case state is when \"00\"=>state<=\"01\"; when \"01\"=>state<=\"00\"; "
        "when others=>state<=\"00\"; end case; end if; end process; end architecture;\n", true);
    check(count(result, "GJB9765.4.11.d") == 1,
          "4.11.d detects a recognized one-process FSM");

    const auto fsmSource = [](int states, const std::string& encoding = "") {
        // Native-backed state model tests; no reachability certification.
        const std::string ctx = "library ieee; use ieee.std_logic_1164.all;\n";
        std::string text = ctx + "entity probe is port(clk:in std_logic); end entity;\n" + ctx +
            "architecture rtl of probe is type state_t is (";
        for (int i = 0; i < states; ++i) text += (i ? "," : "") + std::string("s") + std::to_string(i);
        text += "); signal state:state_t;\n";
        if (!encoding.empty()) text += "attribute fsm_encoding:string;\nattribute fsm_encoding of state:signal is \"" + encoding + "\";\n";
        text += "begin process(clk) begin if rising_edge(clk) then case state is\n";
        for (int i = 0; i < states; ++i) text += "when s" + std::to_string(i) + "=>state<=s" + std::to_string((i+1)%states) + ";\n";
        return text + "end case; end if; end process; end architecture;\n";
    };
    result = analyze(fsmSource(40), true);
    check(count(result, "GJB9765.4.11.f.a") == 0, "40 transition-source states permitted");
    result = analyze(fsmSource(41), true);
    check(count(result, "GJB9765.4.11.f.a") == 1, "41 transition-source states diagnosed");
    const auto fsmTwoProcessSource = [](int states) {
        const std::string entityContext = "library ieee; use ieee.std_logic_1164.all;\n";
        const std::string architectureContext = entityContext + "use ieee.numeric_std.all;\n";
        std::string text = entityContext +
            "entity probe is port(clk,rst:in std_logic); end entity;\n" +
            architectureContext + "architecture rtl of probe is type state_t is (";
        for (int i = 0; i < states; ++i)
            text += (i ? "," : "") + std::string("s") + std::to_string(i);
        text += "); signal state:state_t; signal next_state:state_t; begin\n"
            "process(state) begin case state is\n";
        for (int i = 0; i < states; ++i)
            text += "when s" + std::to_string(i) + "=>next_state<=s" +
                std::to_string((i + 1) % states) + ";\n";
        text += "end case; end process;\n"
            "process(clk,rst) begin if rst='1' then state<=s0; "
            "elsif rising_edge(clk) then state<=next_state; end if; end process;\n"
            "end architecture;\n";
        return text;
    };
    result = analyze(fsmTwoProcessSource(40), true);
    check(count(result, "GJB9765.4.11.f.a") == 0,
          "40 source states in two-process FSM remain within bound");
    result = analyze(fsmTwoProcessSource(41), true);
    check(count(result, "GJB9765.4.11.f.a") == 1,
          "41 source states in two-process FSM exceed native observed bound");
    result = analyze(fsmTwoProcessSource(3), true,
                     analysis::FsmOptimizationPriority::Area);
    check(count(result, "GJB9765.4.11.f.b") == 0 &&
          count(result, "GJB9765.4.11.f.c") == 1,
          "two-process default sequential encoding independently satisfies f.b and violates f.c");
    result = analyze(fsmTwoProcessSource(3), true,
                     analysis::FsmOptimizationPriority::Speed);
    check(count(result, "GJB9765.4.11.f.b") == 0 &&
          count(result, "GJB9765.4.11.f.c") == 1,
          "area/speed option does not alter either recovered native predicate");
    auto duplicateState = fsmTwoProcessSource(41);
    duplicateState.replace(duplicateState.find("s40);"), 5, "s39);");
    result = analyze(duplicateState, true);
    check(count(result, "GJB9765.4.11.f.a") == 1,
          "native transition-source count remains available despite duplicate enum declaration");
    check(count(result, "GJB9765.4.11.f.b") == 0 &&
          count(result, "GJB9765.4.11.f.c") == 0,
          "malformed enum source-count fallback cannot drive encoding rules");
    result = analyze(fsmSource(3, "onehot"), true);
    check(count(result, "GJB9765.4.11.f.b") == 1 && count(result, "GJB9765.4.11.f.c") == 0,
          "onehot independently violates f.b and satisfies f.c without an invented priority gate");
    check(std::ranges::any_of(result.standardAssessments, [](const auto& e) {
        return e.clauseId == "GJB9765.4.11.f.b" && e.state == analysis::StandardRuleState::Violated;
    }), "onehot produces concrete f.b violation evidence without project priority context");
    result = analyze(fsmSource(3, "onehot"), true, analysis::FsmOptimizationPriority::Area);
    check(count(result, "GJB9765.4.11.f.b") == 1 && count(result, "GJB9765.4.11.f.c") == 0,
          "area context does not change independent onehot predicates");
    result = analyze(fsmSource(3, "onehot"), true, analysis::FsmOptimizationPriority::Speed);
    check(count(result, "GJB9765.4.11.f.b") == 1 && count(result, "GJB9765.4.11.f.c") == 0,
          "speed context does not suppress independently enabled f.b");
    check(std::ranges::any_of(result.standardAssessments, [](const auto& e) {
        return e.clauseId == "GJB9765.4.11.f.c" && e.state == analysis::StandardRuleState::ManualReviewRequired;
    }), "partial encoding no alarm stays manual review");
    result = analyze(fsmSource(3), true, analysis::FsmOptimizationPriority::Speed);
    check(count(result, "GJB9765.4.11.f.c") == 1, "default sequential encoding violates speed requirement");
    result = analyze(fsmSource(3, "gray"), true, analysis::FsmOptimizationPriority::Area);
    check(count(result, "GJB9765.4.11.f.b") == 1 && count(result, "GJB9765.4.11.f.c") == 1,
          "gray encoding violates both independently enabled native requirements");

    const auto verilogFsmSource = [](int states, bool twoProcess = false) {
        std::string text = "module probe(input wire clk); reg [5:0] state,next_state;\n";
        for (int i = 0; i < states; ++i)
            text += "localparam S" + std::to_string(i) + "=" + std::to_string(i) + ";\n";
        if (twoProcess) text += "always @* begin case(state)\n";
        else text += "always @(posedge clk) begin case(state)\n";
        for (int i = 0; i < states; ++i) {
            const auto lhs = twoProcess ? "next_state = " : "state <= ";
            text += "S" + std::to_string(i) + ": " + lhs + "S" +
                std::to_string((i + 1) % states) + ";\n";
        }
        text += "endcase end\n";
        if (twoProcess) text += "always @(posedge clk) state <= next_state;\n";
        return text + "endmodule\n";
    };
    result = analyze(verilogFsmSource(40));
    check(count(result, "GJB10157.10.4") == 0,
          "GJB10157.10.4 permits exactly 40 source-proved Verilog FSM states");
    result = analyze(verilogFsmSource(41));
    check(count(result, "GJB10157.10.4") == 1 &&
          std::ranges::any_of(result.violations, [](const auto& item) {
              return item.ruleId == "GJB10157.10.4" &&
                     item.severity == analysis::Severity::Warning &&
                     item.message == "状态机状态数不应超过40个,当前41个";
          }), "A-14-1 reports 41 states with the exact native warning text");
    result = analyze(verilogFsmSource(41, true));
    check(count(result, "GJB10157.10.4") == 1,
          "GJB10157.10.4 reports 41 source-proved two-process Verilog FSM states");
    auto alwaysFfFsm = verilogFsmSource(41);
    alwaysFfFsm.replace(alwaysFfFsm.find("always @(posedge clk)"), 6, "always_ff");
    result = analyze(alwaysFfFsm);
    check(count(result, "GJB10157.10.4") == 1,
          "GJB10157.10.4 admits an always_ff state register process");
    auto duplicateVerilogState = verilogFsmSource(41);
    duplicateVerilogState.replace(duplicateVerilogState.find("S40: state"), 3, "S39");
    result = analyze(duplicateVerilogState);
    check(count(result, "GJB10157.10.4") == 0,
          "A-14-1 counts distinct source states rather than case branches");

    std::string equivalentNumericStates =
        "module probe(input wire clk); reg [5:0] state; always @(posedge clk) case(state)\n";
    for (int i = 0; i < 40; ++i)
        equivalentNumericStates += "6'd" + std::to_string(i) + ": state <= 6'd" +
            std::to_string((i + 1) % 40) + ";\n";
    equivalentNumericStates +=
        "6'b000001: state <= 6'd2; default: state <= 6'd0; endcase endmodule\n";
    result = analyze(equivalentNumericStates);
    check(count(result, "GJB10157.10.4") == 0,
          "A-14-1 collapses equivalent AST_CONSTANT encodings before state counting");
    std::string decoder = "module decoder(input [5:0] state, output reg q); always @* case(state)\n";
    for (int i = 0; i < 41; ++i)
        decoder += std::to_string(i) + ": q = 1'b0;\n";
    decoder += "endcase endmodule\n";
    result = analyze(decoder);
    check(count(result, "GJB10157.10.4") == 0,
          "a wide decoder case without a state update is not admitted as an FSM");

    const auto fsmWithoutInitialization =
        "module probe(input wire clk); reg state; localparam IDLE=1'b0, RUN=1'b1;\n"
        "always @(posedge clk) begin case(state)\n"
        "IDLE: state <= RUN; RUN: state <= IDLE; endcase end endmodule\n";
    result = analyze(fsmWithoutInitialization);
    check(count(result, "GJB10157.10.3") == 1 &&
          std::ranges::any_of(result.violations, [](const auto& item) {
              return item.ruleId == "GJB10157.10.3" &&
                     item.severity == analysis::Severity::Warning &&
                     item.message == "状态机变量state未设置初始状态";
          }), "R-14-3 reports the native warning text for a proved FSM without init");

    result = analyze(
        "module probe(input wire clk); reg state; localparam INIT=1'b0, RUN=1'b1;\n"
        "always @(posedge clk) begin case(state)\n"
        "INIT: state <= RUN; RUN: state <= INIT; endcase end endmodule\n");
    check(count(result, "GJB10157.10.3") == 1,
          "a source state literally named init is not the native synthetic init relation");

    result = analyze(
        "module probe(input wire clk, input wire rst_n); reg state; "
        "localparam IDLE=1'b0, RUN=1'b1;\n"
        "always @(posedge clk or negedge rst_n) begin\n"
        "if (!rst_n) state <= IDLE; else begin case(state)\n"
        "IDLE: state <= RUN; RUN: state <= IDLE; endcase end end endmodule\n");
    check(count(result, "GJB10157.10.3") == 0,
          "R-14-3 accepts a non-default if branch assignment in a one-process FSM");

    result = analyze(
        "module probe(input wire clk, input wire rst_n); reg state,next_state; "
        "localparam IDLE=1'b0, RUN=1'b1;\n"
        "always @* begin case(state) IDLE: next_state=RUN; "
        "RUN: next_state=IDLE; endcase end\n"
        "always @(posedge clk or negedge rst_n) begin "
        "if (!rst_n) state<=IDLE; else state<=next_state; end endmodule\n");
    check(count(result, "GJB10157.10.3") == 0,
          "R-14-3 accepts a non-default if branch assignment in a two-process FSM");

    result = analyze(
        "module probe(input wire clk, input wire en); reg state,next_state; "
        "localparam IDLE=1'b0, RUN=1'b1;\n"
        "always @* begin case(state) IDLE: next_state=RUN; "
        "RUN: next_state=IDLE; endcase end\n"
        "always @(posedge clk) begin if (en) state<=next_state; "
        "else state<=next_state; end endmodule\n");
    check(count(result, "GJB10157.10.3") == 0,
          "R-14-3 mirrors the native synthetic init producer even for normal then transfer");

    result = analyze(
        "module probe(input wire clk, input wire rst_n); reg state; "
        "localparam IDLE=1'b0, RUN=1'b1;\n"
        "always @(posedge clk or negedge rst_n) begin if (rst_n) begin case(state) "
        "IDLE: state<=RUN; RUN: state<=IDLE; endcase end else state<=IDLE; end endmodule\n");
    check(count(result, "GJB10157.10.3") == 1,
          "R-14-3 does not treat an else-only reset assignment as the native init key");

    result = analyze(
        "module probe(input wire clk, input wire en); reg state; "
        "localparam IDLE=1'b0, RUN=1'b1;\n"
        "always @(posedge clk) begin if (en) state<=state; case(state) "
        "IDLE: state<=RUN; RUN: state<=IDLE; endcase end endmodule\n");
    check(count(result, "GJB10157.10.3") == 0,
          "R-14-3 accepts a supported non-default if assignment without an else branch");

    result = analyze(
        "module probe(input wire clk, input wire enable); reg state; "
        "localparam IDLE=1'b0, RUN=1'b1;\n"
        "always @(posedge clk) begin case(enable) 1'b1: state<=IDLE; default: ; endcase "
        "case(state) IDLE: state<=RUN; RUN: state<=IDLE; endcase end endmodule\n");
    check(count(result, "GJB10157.10.3") == 0,
          "R-14-3 accepts a non-state case non-default branch assignment");

    result = analyze(
        "module probe(input wire clk, input wire enable); reg state; "
        "localparam IDLE=1'b0, RUN=1'b1;\n"
        "always @(posedge clk) begin case(enable) 1'b1: ; default: state<=IDLE; endcase "
        "case(state) IDLE: state<=RUN; RUN: state<=IDLE; endcase end endmodule\n");
    check(count(result, "GJB10157.10.3") == 1,
          "R-14-3 keeps a default-only non-state case assignment outside the bounded init producer");

    result = analyze(
        "module probe(input wire clk); reg state; localparam IDLE=1'b0, RUN=1'b1;\n"
        "initial state = IDLE;\n"
        "always @(posedge clk) begin case(state)\n"
        "IDLE: state <= RUN; RUN: state <= IDLE; endcase end endmodule\n");
    check(count(result, "GJB10157.10.3") == 0,
          "R-14-3 accepts the native AST_INITIAL assignment branch");

    result = analyze(
        "module probe(input wire clk, input wire select); reg state,next_state; "
        "localparam IDLE=1'b0, RUN=1'b1;\n"
        "initial next_state = select ? IDLE : RUN;\n"
        "always @* case(state) IDLE: next_state=RUN; RUN: next_state=IDLE; endcase\n"
        "always @(posedge clk) state<=next_state; endmodule\n");
    check(count(result, "GJB10157.10.3") == 0,
          "R-14-3 accepts a supported ternary initial assignment to the next-state relation");

    result = analyze(
        "module probe(input wire clk); reg state; localparam IDLE=1'b0, RUN=1'b1;\n"
        "always @(posedge clk) begin state<=IDLE; case(state) "
        "IDLE: state<=RUN; RUN: state<=IDLE; endcase end endmodule\n");
    check(count(result, "GJB10157.10.3") == 1,
          "R-14-3 does not promote an unconditional assignment to the synthetic init key");

    result = analyze(
        "module probe(input wire clk); reg state = 1'b0; "
        "localparam IDLE=1'b0, RUN=1'b1;\n"
        "always @(posedge clk) case(state) IDLE: state<=RUN; RUN: state<=IDLE; endcase "
        "endmodule\n");
    check(count(result, "GJB10157.10.3") == 1,
          "R-14-3 does not conflate a declaration initializer with AST_INITIAL");

    result = analyze(
        "module probe(input wire clk); reg state; localparam IDLE=1'b0, LOCK=1'b1;\n"
        "always @(posedge clk) begin case(state)\n"
        "IDLE: state <= LOCK; LOCK: state <= LOCK; endcase end endmodule\n");
    check(count(result, "GJB10157.10.1") == 1 &&
          std::ranges::any_of(result.violations, [](const auto& item) {
              return item.ruleId == "GJB10157.10.1" &&
                     item.severity == analysis::Severity::Warning &&
                     item.message == "禁止使用死循环状态机，state状态LOCK";
          }),
          "GJB10157.10.1 reports a source-proved absorbing deadlock state");

    result = analyze(
        "module probe(input wire clk); reg state; localparam IDLE=1'b0, RUN=1'b1;\n"
        "always @(posedge clk) begin case(state)\n"
        "IDLE: state <= RUN; RUN: state <= IDLE; endcase end endmodule\n");
    check(count(result, "GJB10157.10.1") == 0,
          "GJB10157.10.1 accepts a complete two-state cycle");

    result = analyze(
        "module probe(input wire clk); reg [1:0] state; "
        "localparam IDLE=2'd0, RUN=2'd1, ORPHAN_A=2'd2, ORPHAN_B=2'd3;\n"
        "initial state = IDLE;\n"
        "always @(posedge clk) begin case(state)\n"
        "IDLE: state <= RUN; RUN: state <= IDLE; "
        "ORPHAN_A: state <= ORPHAN_B; ORPHAN_B: state <= ORPHAN_A; "
        "endcase end endmodule\n");
    check(count(result, "GJB10157.10.1") == 2 &&
          std::ranges::all_of(result.violations | std::views::filter([](const auto& item) {
              return item.ruleId == "GJB10157.10.1";
          }), [](const auto& item) {
              return item.message.starts_with("状态机变量state状态ORPHAN_") &&
                     item.message.ends_with("不可达") &&
                     item.severity == analysis::Severity::Warning;
          }), "GJB10157.10.1 reports each node outside the native init reachability walk");

    result = analyze(
        "module probe(input wire clk); reg [1:0] state; "
        "localparam IDLE=2'd0, RUN=2'd1, DONE=2'd2;\n"
        "initial state = IDLE;\n"
        "always @(posedge clk) begin case(state)\n"
        "IDLE: state <= RUN; RUN: state <= DONE; DONE: state <= IDLE; "
        "endcase end endmodule\n");
    check(count(result, "GJB10157.10.1") == 0,
          "GJB10157.10.1 keeps every state reached from the explicit init node clear");

    result = analyze(
        "module probe(input wire clk); reg [1:0] state; "
        "localparam IDLE=2'd0, RUN=2'd1, ORPHAN=2'd2;\n"
        "initial state = UNKNOWN_STATE;\n"
        "always @(posedge clk) begin case(state)\n"
        "IDLE: state <= RUN; RUN: state <= IDLE; ORPHAN: state <= IDLE; "
        "endcase end endmodule\n");
    check(count(result, "GJB10157.10.1") == 0,
          "an unresolved init target cannot prove native unreachable-state membership");

    result = analyze(
        "module probe(input wire clk); reg [1:0] state; "
        "localparam IDLE=2'd0, RUN=2'd1, ORPHAN=2'd2;\n"
        "initial state = IDLE; initial state = RUN;\n"
        "always @(posedge clk) begin case(state)\n"
        "IDLE: state <= RUN; RUN: state <= IDLE; ORPHAN: state <= IDLE; "
        "endcase end endmodule\n");
    check(count(result, "GJB10157.10.1") == 0,
          "conflicting explicit init nodes do not select one reachability root by guess");

    result = analyze(
        "module probe(input wire clk, input wire start); reg [1:0] state; "
        "localparam IDLE=2'd0, RUN=2'd1, ORPHAN=2'd2;\n"
        "initial if (start) state = IDLE;\n"
        "always @(posedge clk) begin case(state)\n"
        "IDLE: state <= RUN; RUN: state <= IDLE; ORPHAN: state <= IDLE; "
        "endcase end endmodule\n");
    check(count(result, "GJB10157.10.1") == 0,
          "a conditionally executed initial assignment is not a proven reachability root");

    result = analyze(
        "module probe(input wire clk); reg state; "
        "localparam IDLE=1'b0, idle=1'b1;\n"
        "initial state = IDLE;\n"
        "always @(posedge clk) begin case(state)\n"
        "IDLE: state <= IDLE; idle: state <= IDLE; endcase end endmodule\n");
    check(count(result, "GJB10157.10.1") == 2 &&
          std::ranges::any_of(result.violations, [](const auto& item) {
              return item.ruleId == "GJB10157.10.1" &&
                     item.message == "禁止使用死循环状态机，state状态IDLE";
          }) && std::ranges::any_of(result.violations, [](const auto& item) {
              return item.ruleId == "GJB10157.10.1" &&
                     item.message == "状态机变量state状态idle不可达";
          }), "Verilog state identifiers remain case-sensitive in the native graph adapter");

    result = analyze(
        "module probe(input wire clk); reg state,next_state; "
        "localparam IDLE=1'b0, LOCK=1'b1;\n"
        "always @* begin case(state) IDLE: next_state=LOCK; "
        "LOCK: next_state=LOCK; endcase end\n"
        "always @(posedge clk) state<=next_state; endmodule\n");
    check(count(result, "GJB10157.10.1") == 1,
          "GJB10157.10.1 reports an absorbing state in a direct two-process FSM");

    result = analyze(
        "module probe(input wire clk); reg state; localparam IDLE=1'b0, LOCK=1'b1;\n"
        "always @(posedge clk) begin casex(state)\n"
        "IDLE: state <= LOCK; LOCK: state <= LOCK; endcase end endmodule\n");
    check(count(result, "GJB10157.10.1") == 0,
          "GJB10157.10.1 leaves wildcard case semantics for manual review");

    result = analyze(
        "module probe(input wire clk, input wire leave); reg state; "
        "localparam IDLE=1'b0, HOLD=1'b1;\n"
        "always @(posedge clk) begin case(state)\n"
        "IDLE: state <= HOLD; HOLD: begin if (leave) state <= IDLE; "
        "else state <= HOLD; end endcase end endmodule\n");
    check(count(result, "GJB10157.10.1") == 0,
          "GJB10157.10.1 keeps a state with an explicit escape transition");

    result = analyze(
        "module probe(input wire clk, input wire leave); reg state; "
        "localparam IDLE=1'b0, HOLD=1'b1;\n"
        "always @(posedge clk) begin case(state)\n"
        "IDLE: state <= HOLD; HOLD: if (leave) state <= IDLE; endcase end endmodule\n");
    check(count(result, "GJB10157.10.1") == 0,
          "GJB10157.10.1 does not decide an incomplete transition graph");

    result = analyze(
        "module probe(input wire clk); localparam IDLE=1'b0, RUN=1'b1; "
        "reg state = IDLE;\n"
        "always @(posedge clk) begin case(state)\n"
        "IDLE: state <= RUN; RUN: state <= IDLE; endcase end endmodule\n");
    check(count(result, "GJB10157.10.3") == 1,
          "declaration initialization remains outside the recovered AST_INITIAL predicate");

    result = analyze("module latch_gate(input ss,rr,output qq,qn); "
                     "assign qq=~(rr|qn); assign qn=~(ss|qq); endmodule");
    check(count(result, "GJB10157.7.2.2") == 1,
          "7.2.2 detects a continuous-assignment cross-coupled NOR latch");
    check(count(result, "GJB10157.7.2.1") == 1,
          "7.2.1 reuses an existing continuous-feedback report without duplication");
    check(count(result, "GJB10157.9.4.7") == 1,
          "9.4.7 reuses the semantic continuous-feedback report without duplication");
    result = analyze("module latch_gate(input ss,rr,output qq,qn); "
                     "nand n0(qq,ss,qn); nand n1(qn,rr,qq); endmodule");
    check(count(result, "GJB10157.7.2.2") == 1,
          "7.2.2 lowers cross-coupled NAND primitives to the native gate classes");
    check(count(result, "GJB10157.7.2.1") == 1,
          "7.2.1 now reports a primitive-only combinational feedback loop");
    check(count(result, "GJB10157.9.4.7") == 1 &&
          std::ranges::any_of(result.violations, [](const auto& item) {
              return item.ruleId == "GJB10157.9.4.7" &&
                     item.message == "检测到组合回路qn、qq" &&
                     item.severity == analysis::Severity::Warning;
          }), "9.4.7 reports a primitive-only SCC with the recovered message surface");
    result = analyze("module latch_gate(input ss,rr,output qq,qn); "
                     "assign qq=!(rr||qn); assign qn=!(ss||qq); endmodule");
    check(count(result, "GJB10157.7.2.2") == 1,
          "7.2.2 includes native logical OR cells");
    result = analyze("module latch_gate(input ss,rr,output qq,qn); "
                     "nor n0(qq,ss,qn), n1(qn,rr,qq); endmodule");
    check(count(result, "GJB10157.7.2.2") == 1,
          "7.2.2 handles multiple primitive instances in one declaration");
    result = analyze("module comb(input ss,rr,output qq,qn); "
                     "assign qq=~(rr|ss); assign qn=~(ss|qq); endmodule");
    check(count(result, "GJB10157.7.2.2") == 0,
          "7.2.2 excludes an acyclic two-gate chain");
    result = analyze("module ring(input aa,bb,cc,output qq,qn,qp); "
                     "assign qq=~(aa|qn); assign qn=~(bb|qp); "
                     "assign qp=~(cc|qq); endmodule");
    check(count(result, "GJB10157.7.2.2") == 0,
          "7.2.2 preserves native exclusion above two counted cells");
    check(count(result, "GJB10157.7.2.1") == 1,
          "7.2.1 retains a larger loop excluded by the separate gate-latch rule");
    result = analyze("module ring(input aa,bb,cc,output qq,qn,qp); "
                     "nor n0(qq,aa,qn),n1(qn,bb,qp),n2(qp,cc,qq); endmodule");
    check(count(result, "GJB10157.7.2.1") == 1 && count(result, "GJB10157.7.2.2") == 0,
          "7.2.1 reports a three-primitive ring independently of 7.2.2 count exclusion");
    check(count(result, "GJB10157.9.4.7") == 1,
          "9.4.7 reports a primitive SCC independently of the gate-latch count threshold");
    result = analyze("module chain(input aa,bb,output qq,qn); "
                     "nor n0(qq,aa,bb),n1(qn,qq,bb); endmodule");
    check(count(result, "GJB10157.7.2.1") == 0,
          "7.2.1 does not report an acyclic primitive chain");
    check(count(result, "GJB10157.9.4.7") == 0,
          "9.4.7 does not report an acyclic primitive chain");
    result = analyze("module ff(input clk,dd,output reg qq); "
                     "always @(posedge clk) qq<=dd; endmodule");
    check(count(result, "GJB10157.7.2.2") == 0,
          "7.2.2 excludes sequential register feedback");
    result = analyze("module scope(input ss,rr,output qq,qn); "
                     "assign qq=~(rr|qn); endmodule "
                     "module other(input ss,rr,output qq,qn); "
                     "assign qn=~(ss|qq); endmodule");
    check(count(result, "GJB10157.7.2.2") == 0,
          "7.2.2 keeps same-named nets in different modules separate");
    result = analyze("module folded(input rr,output qq,qn); "
                     "assign qq=qn&1'b0; assign qn=~(rr|qq); endmodule");
    check(count(result, "GJB10157.7.2.2") == 0,
          "7.2.2 defers constants that need folding to prove a surviving SCC");
    result = analyze("module aliases(output qq,qn); "
                     "assign qq=qn; assign qn=qq; endmodule");
    check(count(result, "GJB10157.7.2.2") == 0,
          "7.2.2 does not treat wire aliases as gate cells");
    result = analyze("module gated(input clk,ss,rr,output reg qq,qn); "
                     "always @(posedge clk) qq<=ss; "
                     "assign qq=~(rr|qn); assign qn=~(ss|qq); endmodule");
    check(count(result, "GJB10157.7.2.2") == 0,
          "7.2.2 defers ambiguous procedural and continuous multiple drivers");
    result = analyze("module two_latches(input ss,rr,output aa,ab,ba,bb); "
                     "nor n0(aa,ss,ab), n1(ab,rr,aa); "
                     "nand n2(ba,ss,bb), n3(bb,rr,ba); endmodule");
    check(count(result, "GJB10157.7.2.2") == 2,
          "7.2.2 emits once per independent feedback component");
    result = analyze("module fanin(input ss,rr,enable,output qq,qn); wire ctl; "
                     "assign ctl=enable&ss; "
                     "assign qq=~(rr|qn); assign qn=~(ctl|qq); endmodule");
    check(count(result, "GJB10157.7.2.2") == 1,
          "7.2.2 excludes acyclic fan-in gates from the component gate count");
    result = analyze("module inactive(input ss,rr,output qq,qn);\n"
                     "`ifdef NOT_DEFINED\n"
                     "assign qq=~(rr|qn); assign qn=~(ss|qq);\n"
                     "`endif\nendmodule");
    check(count(result, "GJB10157.7.2.2") == 0,
          "7.2.2 ignores preprocessor-inactive feedback");
    result = analyze("module spelling(input ss,rr,output qq,qn); "
                     "assign qq=~(rr|qn); assign qn=~(ss|QQ); endmodule");
    check(count(result, "GJB10157.7.2.2") == 0,
          "7.2.2 preserves case-sensitive Verilog net identity");
    check(std::ranges::any_of(result.standardAssessments, [](const auto& assessment) {
        return assessment.clauseId == "GJB10157.7.2.2" &&
            assessment.state == analysis::StandardRuleState::ManualReviewRequired;
    }), "7.2.2 does not promote a no-alarm partial check to automatic compliance");

    const auto powerOn = [&](std::string text, analysis::FpgaTechnology technology,
                              std::optional<bool> accept = std::nullopt) {
        analysis::AnalysisOptions options;
        options.standardId = "GJB10157-2021";
        options.powerOnPolicy = {technology, accept};
        return analysis::Analyzer(options).analyzeSource(
            {"power_on.v", core::HdlLanguage::Verilog, std::move(text)});
    };
    const std::string powerRegister = "module ff(input clk,dd,output reg qq); "
                                     "always @(posedge clk) qq<=dd; endmodule";
    result = powerOn(powerRegister, analysis::FpgaTechnology::NonSram);
    check(count(result, "GJB10157.6.2.5") == 1,
          "6.2.5 reports an uninitialized direct register on explicit non-SRAM hardware");
    const auto missingPowerOn = std::ranges::find_if(result.violations,
        [](const auto& violation) { return violation.ruleId == "GJB10157.6.2.5"; });
    check(missingPowerOn != result.violations.end() &&
          missingPowerOn->message == "必须对寄存器qq进行上电初始化,赋固定值" &&
          missingPowerOn->severity == analysis::Severity::Error &&
          missingPowerOn->line == 1 && missingPowerOn->column == 1,
          "R-5-5 preserves the native message, Error and assignment line-only location");
    result = powerOn(powerRegister, analysis::FpgaTechnology::Unspecified);
    check(count(result, "GJB10157.6.2.5") == 0,
          "6.2.5 does not invent the target FPGA technology");
    result = powerOn(powerRegister, analysis::FpgaTechnology::Sram);
    check(count(result, "GJB10157.6.2.5") == 0,
          "6.2.5 preserves the documented non-SRAM applicability boundary");
    const std::string initializedRegister = "module ff(input clk,dd,output reg qq); "
        "initial qq=1'b0; always @(posedge clk) qq<=dd; endmodule";
    result = powerOn(initializedRegister, analysis::FpgaTechnology::NonSram, true);
    check(count(result, "GJB10157.6.2.5") == 0,
          "6.2.5 accepts an initial target when its recovered acceptance switch is enabled");
    result = powerOn("module ff(input clk,dd,output reg qq); "
        "initial qq=dd; always @(posedge clk) qq<=dd; endmodule",
        analysis::FpgaTechnology::NonSram, true);
    check(count(result, "GJB10157.6.2.5") == 0,
          "R-5-5 mirrors the native target-only INITIAL collector without inventing RHS validation");
    result = powerOn(initializedRegister, analysis::FpgaTechnology::NonSram, false);
    check(count(result, "GJB10157.6.2.5") == 1,
          "6.2.5 does not count initial assignments when the acceptance switch is disabled");
    result = powerOn(initializedRegister, analysis::FpgaTechnology::NonSram);
    check(count(result, "GJB10157.6.2.5") == 0,
          "6.2.5 defers initial acceptance when the project policy is unspecified");
    result = powerOn("module ff(input clk,dd,output reg qq,rr); "
        "initial qq=1'b0; always @(posedge clk) begin qq<=dd; rr<=dd; end endmodule",
        analysis::FpgaTechnology::NonSram, true);
    check(count(result, "GJB10157.6.2.5") == 1,
          "6.2.5 compares initialization per register rather than per module");
    result = powerOn("module ff(input clk,dd,output reg qq);\n"
        "always @(posedge clk) begin\nqq<=dd;\nqq<=~dd;\nend endmodule\n",
        analysis::FpgaTechnology::NonSram, false);
    check(count(result, "GJB10157.6.2.5") == 2,
          "R-5-5 preserves one missing-initial report per target name and physical line");
    result = powerOn("module ff(input clk,dd,output reg qq); "
        "always @(posedge clk) begin qq<=dd; qq<=~dd; end endmodule",
        analysis::FpgaTechnology::NonSram, false);
    check(count(result, "GJB10157.6.2.5") == 1,
          "R-5-5 collapses repeated assignments sharing the same target name and physical line");
    result = powerOn("module ff(input c1,c2,dd,output reg qq); "
        "always @(posedge c1) qq<=dd; always @(posedge c2) qq<=~dd; endmodule",
        analysis::FpgaTechnology::NonSram, false);
    check(count(result, "GJB10157.6.2.5") == 0,
          "R-5-5 keeps cross-process multiple-driver ownership outside the bounded adapter");
    result = powerOn("module ff(input clk,rst,dd,output reg qq); "
        "always @(posedge clk) if(rst) qq<=1'b0; else qq<=dd; endmodule",
        analysis::FpgaTechnology::NonSram, false);
    check(count(result, "GJB10157.6.2.5") == 0,
          "6.2.5 keeps complex reset/default-case initialization paths in manual review");
    result = powerOn("module ff(input clk,dd,output reg qq=1'b0); "
        "always @(posedge clk) qq<=dd; endmodule", analysis::FpgaTechnology::NonSram);
    check(count(result, "GJB10157.6.2.5") == 0,
          "6.2.5 does not misreport unresolved declaration initialization");
    check(std::ranges::any_of(result.standardAssessments, [](const auto& assessment) {
        return assessment.clauseId == "GJB10157.6.2.5" &&
               assessment.state == analysis::StandardRuleState::ManualReviewRequired;
    }), "6.2.5 no-alarm partial path remains manual review");

    const auto defaultResetFsm = [](const std::string& defaultBody) {
        return "module probe(input wire clk); reg state,busy; "
               "localparam IDLE=1'b0, RUN=1'b1;\n"
               "always @(posedge clk) begin case(state)\n"
               "IDLE: begin state<=RUN; busy<=1'b1; end "
               "RUN: begin state<=IDLE; busy<=1'b0; end "
               "default: begin " + defaultBody + " end endcase end endmodule\n";
    };
    result = analyze(defaultResetFsm("state<=IDLE; busy<=1'b0;"));
    check(count(result, "GJB10157.10.2") == 0,
          "R-14-2 accepts a default branch with terminal reset values");
    result = analyze(defaultResetFsm("state<=IDLE;"));
    check(count(result, "GJB10157.10.2") == 0,
          "R-14-2 does not invent a native per-register omission comparison");
    result = analyze(defaultResetFsm("state<=1'bx; busy<=1'b0;"));
    check(count(result, "GJB10157.10.2") == 0,
          "R-14-2 preserves native X literals as terminal relation values");
    result = analyze(defaultResetFsm(";"));
    check(count(result, "GJB10157.10.2") == 1,
          "R-14-2 reports an existing default entry with no collected reset value");
    check(std::ranges::any_of(result.violations, [](const auto& item) {
              return item.ruleId == "GJB10157.10.2" &&
                     item.severity == analysis::Severity::Warning &&
                     item.message == "状态机信号state在default状态时，没有把状态机中所有寄存器重置为确定状态.";
          }), "R-14-2 uses the recovered warning severity and native diagnostic text");
    result = analyze(defaultResetFsm("state<=state;"));
    check(count(result, "GJB10157.10.2") == 1,
          "R-14-2 removes an RHS that remains an assigned dependency");
    result = analyze(defaultResetFsm("state<=busy ? IDLE : RUN;"));
    check(count(result, "GJB10157.10.2") == 0,
          "R-14-2 preserves terminal values from both simple ternary result arms");
    result = analyze(defaultResetFsm("state<=busy ? state : state;"));
    check(count(result, "GJB10157.10.2") == 1,
          "R-14-2 filters assigned dependencies from both simple ternary result arms");
    result = analyze(
        "module probe(input wire clk); reg state; localparam IDLE=0,RUN=1;\n"
        "always @(posedge clk) case(state) IDLE:state<=RUN; RUN:state<=IDLE; "
        "endcase endmodule\n");
    check(count(result, "GJB10157.10.2") == 0,
          "R-14-2 preserves the native no-default no-report boundary");
    result = analyze(defaultResetFsm("state<=reset_state; busy<=reset_value;"));
    check(count(result, "GJB10157.10.2") == 0,
          "R-14-2 keeps terminal external identifiers in the filtered set");
    result = analyze(defaultResetFsm("state<=reset_state & 1'b1;"));
    check(count(result, "GJB10157.10.2") == 0,
          "R-14-2 leaves unsupported compound RHS expressions for manual review");

    const auto safeModeFsm = [](const std::string& declaration, bool twoProcess = false) {
        std::string text = "module probe(input wire clk);\n" + declaration + "\n"
            "localparam IDLE=2'b00, RUN=2'b01, STOP=2'b10;\n";
        if (twoProcess) {
            text += "always @* begin case(state) IDLE: next_state=RUN; "
                    "RUN: next_state=STOP; STOP: next_state=IDLE; endcase end\n"
                    "always @(posedge clk) state<=next_state;\n";
        } else {
            text += "always @(posedge clk) begin case(state) IDLE: state<=RUN; "
                    "RUN: state<=STOP; STOP: state<=IDLE; endcase end\n";
        }
        return text + "endmodule\n";
    };
    result = analyze(safeModeFsm("reg [1:0] state;"));
    check(count(result, "GJB10157.12.2") == 1,
          "GJB10157.12.2 reports a recognized multi-bit FSM without safe mode");
    check(std::ranges::any_of(result.violations, [](const auto& item) {
              return item.ruleId == "GJB10157.12.2" &&
                     item.severity == analysis::Severity::Warning &&
                     item.message == "状态机未设置为safe模式";
          }), "R-16-2 uses the recovered warning severity and exact native diagnostic");
    result = analyze(safeModeFsm("reg [1:0] state,next_state;", true));
    check(count(result, "GJB10157.12.2") == 1,
          "GJB10157.12.2 reports a recognized two-process FSM without safe mode");
    result = analyze(safeModeFsm("(* fsm_encoding = \"safe\" *) reg [1:0] state;"));
    check(count(result, "GJB10157.12.2") == 0,
          "GJB10157.12.2 accepts the native safe fsm_encoding attribute");
    result = analyze(safeModeFsm(
        "reg [1:0] state; // synthesis syn_encoding = \"safe\""));
    check(count(result, "GJB10157.12.2") == 0,
          "GJB10157.12.2 accepts the native synthesis syn_encoding comment");
    result = analyze(safeModeFsm(
        "/* synthesis fsm_safe_state = \"state\" */\nreg [1:0] state;"));
    check(count(result, "GJB10157.12.2") == 0,
          "GJB10157.12.2 accepts the native fsm_safe_state comment form");
    result = analyze(safeModeFsm(
        "// SYNTHESIS SYN_ENCODING = \"SAFE\"\nreg [1:0] state;"));
    check(count(result, "GJB10157.12.2") == 0,
          "R-16-2 synthesis-comment regex preserves the native icase flag");
    result = analyze(safeModeFsm(
        "// SYNTHESIS FSM_SAFE_STATE = \"STATE\"\nreg [1:0] state;"));
    check(count(result, "GJB10157.12.2") == 0,
          "R-16-2 safe-state comment regex preserves the native icase flag");
    result = analyze(safeModeFsm(
        "// synthesis = \"safe\"\nreg [1:0] state;"));
    check(count(result, "GJB10157.12.2") == 0,
          "R-16-2 preserves the native optional syn_encoding group");
    result = analyze(safeModeFsm("(* fsm_encoding = \"SAFE\" *) reg [1:0] state;"));
    check(count(result, "GJB10157.12.2") == 1,
          "R-16-2 fsm_encoding attribute safe substring remains case-sensitive");
    result = analyze(safeModeFsm("(* fsm_encoding = \"onehot\" *) reg [1:0] state;"));
    check(count(result, "GJB10157.12.2") == 1,
          "a non-safe fsm_encoding value remains a GJB10157.12.2 violation");
    result = analyze(safeModeFsm("(* fsm_encoding = \"none\" *) reg [1:0] state;"));
    check(count(result, "GJB10157.12.2") == 0,
          "fsm_encoding none preserves the native FSM-detection exclusion");
    result = analyze(
        "module probe(input wire clk); // synthesis syn_encoding = \"safe\"\n\n"
        "reg [1:0] state; localparam IDLE=0,RUN=1,STOP=2;\n"
        "always @(posedge clk) case(state) IDLE:state<=RUN; RUN:state<=STOP; "
        "STOP:state<=IDLE; endcase endmodule\n");
    check(count(result, "GJB10157.12.2") == 1,
          "an unrelated safe directive more than one source line away cannot suppress 12.2");
    result = analyze(
        "module probe(input wire clk); reg state; localparam IDLE=0,RUN=1;\n"
        "always @(posedge clk) case(state) IDLE:state<=RUN; RUN:state<=IDLE; "
        "endcase endmodule\n");
    check(count(result, "GJB10157.12.2") == 0,
          "GJB10157.12.2 preserves the native one-bit state-register exclusion");

    result = analyze("module VDD; wire vss, vCc; parameter GND=1; localparam VREF=2; "
                     "task vdd; input wire VREF; endtask endmodule");
    check(count(result, "GJB10157.5.2.4") == 7, "power name check reaches selected declarations and descendants");
    check(std::ranges::all_of(result.violations, [](const auto& item) {
        return item.ruleId != "GJB10157.5.2.4" ||
            (item.severity == analysis::Severity::Error && item.column == 1);
    }), "native power-name diagnostics preserve Error and line-only location");
    check(std::ranges::any_of(result.violations, [](const auto& item) {
        return item.ruleId == "GJB10157.5.2.4" &&
            item.message == "禁止使用VDD命名";
    }) && std::ranges::any_of(result.violations, [](const auto& item) {
        return item.ruleId == "GJB10157.5.2.4" &&
            item.message == "禁止使用vCc命名";
    }), "native power-name message retains each original normalized name spelling");
    result = analyze("module probe; wire VDD, VDD; endmodule");
    check(count(result, "GJB10157.5.2.4") == 2,
          "two identical same-line power-name candidates retain native per-node multiplicity");
    result = analyze("module dup; wire VDD; endmodule module dup; wire VSS; endmodule");
    check(count(result, "GJB10157.5.2.4") == 0,
          "duplicate same-name module map group is excluded before power-name checks");
    result = analyze("module one; wire VDD; endmodule module two; wire VSS; endmodule");
    check(count(result, "GJB10157.5.2.4") == 2,
          "distinct unique module-map groups receive independent power-name checks");
    result = analyze("module probe; function integer VDD; VDD=0; endfunction "
                     "wire vdd_x,xvdd,vref1; endmodule");
    check(count(result, "GJB10157.5.2.4") == 0, "function name and prefixed/suffixed names excluded");
    check(std::ranges::any_of(result.standardAssessments, [](const auto& e) {
        return e.clauseId == "GJB10157.5.2.4" && e.state == analysis::StandardRuleState::ManualReviewRequired;
    }), "power name partial detector no alarm stays manual review");
    result = analyze("module probe; function integer VDD; reg GND; VDD=0; endfunction endmodule");
    check(count(result, "GJB10157.5.2.4") == 1 &&
          std::ranges::any_of(result.violations, [](const auto& item) {
              return item.ruleId == "GJB10157.5.2.4" &&
                  item.message == "禁止使用GND命名";
          }), "function definition is excluded while selected declaration descendants remain included");
    result = analyze(
        "library ieee; use ieee.std_logic_1164.all;\n"
        "entity probe is port(A: in std_logic; B: BUFFER std_logic; "
        "C: buffer std_logic);\n"
        "component nested is port(X: buffer std_logic); end component;\n"
        "end entity; architecture rtl of probe is begin end architecture;\n", true);
    check(count(result, "GJB9765.4.12.c") == 2,
          "4.12.c reports each separate entity buffer port case-insensitively");
    result = analyze(
        "library ieee; use ieee.std_logic_1164.all;\n"
        "entity probe is port(A: in std_logic; B: out std_logic);\n"
        "component nested is port(X: buffer std_logic); end component;\n"
        "end entity; architecture rtl of probe is begin end architecture;\n", true);
    check(count(result, "GJB9765.4.12.c") == 0,
          "4.12.c native entity collector excludes nested component buffer ports");
    check(std::ranges::any_of(result.standardAssessments, [](const auto& e) {
        return e.clauseId == "GJB9765.4.12.c" &&
               e.state == analysis::StandardRuleState::ManualReviewRequired;
    }), "4.12.c bounded no-alarm result stays manual review");
    result = analyze(
        "library ieee; use ieee.std_logic_1164.all;\n"
        "entity probe is port(CLK, datain: in std_logic; C1: out std_logic; "
        "BUS: inout std_logic; HOLD: buffer std_logic);\n"
        "end entity; architecture rtl of probe is begin end architecture;\n", true);
    check(count(result, "GJB9765.4.5.f.22") == 2,
          "4.5.f.22 requires literal underscore suffix and does not exempt CLK");
    check(count(result, "GJB9765.4.5.f.23") == 1,
          "4.5.f.23 reports OUT without _out while excluding BUFFER mode");
    check(count(result, "GJB9765.4.5.f.24") == 1,
          "4.5.f.24 reports an INOUT port without _inout");
    result = analyze(
        "library ieee; use ieee.std_logic_1164.all;\n"
        "entity probe is port(CLK_IN, data_in: in std_logic; C1_OUT: OUT std_logic; "
        "BUS_INOUT: InOuT std_logic; HOLD: BUFFER std_logic);\n"
        "end entity; architecture rtl of probe is begin end architecture;\n", true);
    check(count(result, "GJB9765.4.5.f.22") == 0 &&
          count(result, "GJB9765.4.5.f.23") == 0 &&
          count(result, "GJB9765.4.5.f.24") == 0,
          "4.5.f.22-.24 compare direction suffixes case-insensitively and ignore buffer");
    result = analyze(
        "library ieee; use ieee.std_logic_1164.all;\n"
        "entity probe is generic(constant N: integer := 2); "
        "port(signal A_in, B: std_logic; Y_out: out std_logic; "
        "IO_inout: inout std_logic); end entity;\n"
        "architecture rtl of probe is begin end architecture;\n", true);
    check(count(result, "GJB9765.4.5.f.22") == 1,
          "4.5.f.22 treats omitted VHDL port mode as IN and checks grouped names");
    check(count(result, "GJB9765.4.5.f.23") == 0 &&
          count(result, "GJB9765.4.5.f.24") == 0,
          "4.5.f.23-.24 keep valid explicit grouped directions clear");
    check(std::ranges::any_of(result.standardAssessments, [](const auto& e) {
        return e.clauseId == "GJB9765.4.5.f.23" &&
               e.state == analysis::StandardRuleState::ManualReviewRequired;
    }), "4.5.f.23 no finding remains manual review rather than certified compliance");
    result = analyze(
        "library ieee; use ieee.std_logic_1164.all;\n"
        "entity probe is port(A_in: in std_logic; Y_out: out std_logic); end entity;\n"
        "architecture rtl of probe is\n"
        "type my_type is array (0 to 1) of std_logic;\n"
        "subtype my_subtype is std_logic;\n"
        "function my_function(A: std_logic) return std_logic is begin return A; end function;\n"
        "begin p: process(A_in) variable a_var, second_var: std_logic; begin "
        "a_var := A_in; second_var := a_var; Y_out <= my_function(second_var); end process;\n"
        "end architecture;\n", true);
    check(count(result, "GJB9765.4.5.f.2") == 2,
          "4.5.f.2 reports every grouped VARIABLE name lacking V prefix");
    check(count(result, "GJB9765.4.5.f.14") == 1,
          "4.5.f.14 reports a TYPE name lacking T prefix");
    check(count(result, "GJB9765.4.5.f.15") == 1,
          "4.5.f.15 reports a SUBTYPE name lacking ST prefix");
    check(count(result, "GJB9765.4.5.f.16") == 1,
          "4.5.f.16 reports a FUNCTION declaration name lacking F prefix");
    result = analyze(
        "library ieee; use ieee.std_logic_1164.all;\n"
        "entity probe is port(A_in: in std_logic; Y_out: out std_logic); end entity;\n"
        "architecture rtl of probe is\n"
        "type T_kind is array (0 to 1) of std_logic; subtype ST_bit is std_logic;\n"
        "function Func_copy(A: std_logic) return std_logic is begin return A; end function;\n"
        "begin p: process(A_in) variable Var_a, VSecond: std_logic; begin "
        "Var_a := A_in; VSecond := Var_a; Y_out <= Func_copy(VSecond); end process;\n"
        "end architecture;\n", true);
    check(count(result, "GJB9765.4.5.f.2") == 0 &&
          count(result, "GJB9765.4.5.f.14") == 0 &&
          count(result, "GJB9765.4.5.f.15") == 0 &&
          count(result, "GJB9765.4.5.f.16") == 0,
          "4.5.f.2/.14/.15/.16 compare default prefixes case-insensitively");
    check(std::ranges::any_of(result.standardAssessments, [](const auto& e) {
        return e.clauseId == "GJB9765.4.5.f.16" &&
               e.state == analysis::StandardRuleState::ManualReviewRequired;
    }), "4.5.f.16 no finding remains manual review rather than certified compliance");
    result = analyze(
        "library ieee; use ieee.std_logic_1164.all;\n"
        "entity core is generic(constant WIDTH: integer := 2); end entity;\n"
        "architecture rtl of core is constant bad_value, C_good: integer := 1; "
        "begin end architecture;\n", true);
    check(count(result, "GJB9765.4.5.f.3") == 1,
          "4.5.f.3 checks grouped declarative constants but excludes generic interface constants");
    check(count(result, "GJB9765.4.5.f.4") == 1,
          "4.5.f.4 checks the entity design-unit name for _ent suffix");
    check(count(result, "GJB9765.4.5.f.6") == 1,
          "4.5.f.6 checks the architecture design-unit name for _arc suffix");
    result = analyze(
        "library ieee; use ieee.std_logic_1164.all;\n"
        "entity core_ENT is generic(constant WIDTH: integer := 2); end entity;\n"
        "architecture rtl_ARC of core_ENT is constant Count, CSecond: integer := 1; "
        "begin end architecture;\n", true);
    check(count(result, "GJB9765.4.5.f.3") == 0 &&
          count(result, "GJB9765.4.5.f.4") == 0 &&
          count(result, "GJB9765.4.5.f.6") == 0,
          "4.5.f.3/.4/.6 compare the recovered names case-insensitively");
    result = analyze(
        "library ieee; use ieee.std_logic_1164.all;\n"
        "package utility_pkg is end package; "
        "package body utility_pkg is end package body;\n"
        "library work; use work.remote_pkg.all; use work.remote_pkg.item;\n", true);
    check(count(result, "GJB9765.4.5.f.8") == 2,
          "4.5.f.8 checks body-bound and WORK-referenced package names once each");
    result = analyze(
        "library ieee; use ieee.std_logic_1164.all;\n"
        "package utility_PACK is end package; "
        "package body utility_PACK is end package body;\n"
        "library work; use work.remote_pack.all;\n", true);
    check(count(result, "GJB9765.4.5.f.8") == 0,
          "4.5.f.8 accepts case-insensitive _pack and excludes IEEE library packages");
    check(std::ranges::any_of(result.standardAssessments, [](const auto& e) {
        return e.clauseId == "GJB9765.4.5.f.8" &&
               e.state == analysis::StandardRuleState::ManualReviewRequired;
    }), "4.5.f.8 no finding remains manual review rather than certified compliance");
    result = analyzeVhdlAt("core_ent.vhd",
        "entity core_ent is end entity; architecture rtl_arc of core_ent is begin end architecture;");
    check(count(result, "GJB9765.4.5.f.5") == 0,
          "4.5.f.5 accepts the exact case-sensitive entity name plus .vhd");
    result = analyzeVhdlAt("CORE_ENT.vhd",
        "entity core_ent is end entity; architecture rtl_arc of core_ent is begin end architecture;");
    check(count(result, "GJB9765.4.5.f.5") == 1,
          "4.5.f.5 preserves the native case-sensitive full filename comparison");
    result = analyzeVhdlAt("core_ent.vhdl",
        "entity core_ent is end entity; architecture rtl_arc of core_ent is begin end architecture;");
    check(count(result, "GJB9765.4.5.f.5") == 1,
          "4.5.f.5 requires the recovered literal .vhd extension");
    result = analyzeVhdlAt("types_pack.vhdl",
        "package types_pack is end package; package body types_pack is end package body;");
    check(count(result, "GJB9765.4.5.f.9") == 0,
          "4.5.f.9 compares the package name with the basename after removing its last extension");
    result = analyzeVhdlAt("Types_pack.vhd",
        "package types_pack is end package; package body types_pack is end package body;");
    check(count(result, "GJB9765.4.5.f.9") == 1,
          "4.5.f.9 preserves the native case-sensitive stem comparison");
    result = analyzeVhdlAt("TYPES_PACK_BODY.VHD",
        "package body types_pack is end package body;");
    check(count(result, "GJB9765.4.5.f.10") == 0,
          "4.5.f.10 compares package-body filenames case-insensitively");
    result = analyzeVhdlAt("types_pack.vhd",
        "package body types_pack is end package body;");
    check(count(result, "GJB9765.4.5.f.10") == 1,
          "4.5.f.10 requires the recovered _body.vhd suffix for a separate body file");
    result = analyzeVhdlAt("types_pack.vhd",
        "package types_pack is end package; package body types_pack is end package body;");
    check(count(result, "GJB9765.4.5.f.9") == 0 &&
          count(result, "GJB9765.4.5.f.10") == 0,
          "4.5.f.10 excludes a package body sharing the declaration source path");
    result = analyze(
        "library ieee; use ieee.std_logic_1164.all;\n"
        "entity probe is generic(N: integer := 2); "
        "port(A_in: in std_logic; Y_out: out std_logic); end entity;\n"
        "architecture rtl of probe is begin\n"
        "bad_label: block begin Y_out <= A_in; end block bad_label;\n"
        "bad_gen: for i in 0 to N-1 generate Y_out <= A_in; end generate bad_gen;\n"
        "wrong: if N = 2 generate Y_out <= A_in; end generate wrong;\n"
        "bad_process: process(A_in) begin Y_out <= A_in; end process bad_process;\n"
        "process(A_in) begin Y_out <= A_in; end process;\n"
        "end architecture;\n", true);
    check(count(result, "GJB9765.4.5.f.18") == 1,
          "4.5.f.18 checks explicit BLOCK labels for _block suffix");
    check(count(result, "GJB9765.4.5.f.19") == 1,
          "4.5.f.19 checks FOR/IF GENERATE labels for _gen suffix");
    check(count(result, "GJB9765.4.5.f.20") == 1,
          "4.5.f.20 checks labeled processes and excludes unlabeled processes");
    result = analyze(
        "library ieee; use ieee.std_logic_1164.all;\n"
        "entity probe is generic(N: integer := 2); "
        "port(A_in: in std_logic; Y_out: out std_logic); end entity;\n"
        "architecture rtl of probe is begin\n"
        "logic_BLOCK: block begin Y_out <= A_in; end block logic_BLOCK;\n"
        "loop_GEN: for i in 0 to N-1 generate Y_out <= A_in; end generate loop_GEN;\n"
        "comb_PROC: process(A_in) begin Y_out <= A_in; end process comb_PROC;\n"
        "end architecture;\n", true);
    check(count(result, "GJB9765.4.5.f.18") == 0 &&
          count(result, "GJB9765.4.5.f.19") == 0 &&
          count(result, "GJB9765.4.5.f.20") == 0,
          "4.5.f.18-.20 compare label suffixes case-insensitively");
    check(std::ranges::any_of(result.standardAssessments, [](const auto& e) {
        return e.clauseId == "GJB9765.4.5.f.20" &&
               e.state == analysis::StandardRuleState::ManualReviewRequired;
    }), "4.5.f.20 no finding remains manual review rather than certified compliance");
    result = analyze(
        "library ieee; use ieee.std_logic_1164.all;\n"
        "entity probe is port(A: in std_logic); end entity;\n"
        "architecture rtl of probe is\n"
        "component empty_child is end component;\n"
        "component used_child is port(X: in std_logic); end component;\n"
        "begin end architecture;\n", true);
    check(count(result, "GJB9765.4.2.a") == 1,
          "4.2.a embedded VLR-010073 branch reports a portless component");
    result = analyze(
        "library ieee; use ieee.std_logic_1164.all;\n"
        "entity probe is end entity;\n"
        "architecture rtl of probe is component empty_child is end component;\n"
        "begin end architecture;\n", true);
    check(count(result, "GJB9765.4.2.a") == 2,
          "4.2.a independently reports portless entity and component");
    result = analyze("module child; endmodule module probe; child GND(); endmodule");
    check(count(result, "GJB10157.5.2.4") == 1, "same-source cell declaration power name checked");
    result = analyze("module probe; wire \\VDD ; assign normal=GND; "
                     "initial $display(\"VSS VDD VREF\"); endmodule");
    check(count(result, "GJB10157.5.2.4") == 0, "escaped unknowns, uses and strings are not guessed declarations");

    result = analyze(
        "module probe(input A, input COND, input CLK, output G);\n"
        "reg my_sig1; reg my_sig2;\n"
        "always @(posedge CLK) my_sig1 <= A;\n"
        "always @(*) if (COND) my_sig2 = COND; else my_sig2 = 1'b0;\n"
        "assign G = A; endmodule\n");
    check(count(result, "GJB10157.5.2.10") == 2,
          "5.2.10 attachment pattern reports each driven internal signal without a load");
    result = analyze(
        "module probe(input A, input COND, input CLK, output G);\n"
        "reg my_sig1; reg my_sig2;\n"
        "always @(posedge CLK) my_sig1 <= A;\n"
        "always @(*) if (COND) my_sig2 = COND; else my_sig2 = 1'b0;\n"
        "assign G = my_sig1 & my_sig2; endmodule\n");
    check(count(result, "GJB10157.5.2.10") == 0,
          "5.2.10 attachment compliant pattern accepts loaded driven signals");
    result = analyze(
        "module probe(input logic a, output logic q, output wire y);\n"
        "logic declared_only; logic self_read; logic bit_read; logic forwarded;\n"
        "always_comb begin q = a; self_read = self_read; bit_read = a; forwarded = a; end\n"
        "assign y = bit_read[0]; sink u_sink(.a(forwarded)); endmodule\n"
        "module sink(input logic a); endmodule\n");
    check(count(result, "GJB10157.5.2.10") == 0,
          "5.2.10 excludes undriven declarations, self-read targets, bit-select loads, ports, and instance actuals");
    result = analyze(
        "module probe(input logic a, input logic b, output logic q);\n"
        "logic compare_only; always_comb begin q = (a <= b); compare_only = a; end endmodule\n");
    check(count(result, "GJB10157.5.2.10") == 1,
          "5.2.10 treats relational <= as a read expression and only reports the true write-only signal");
    check(std::ranges::any_of(result.standardAssessments, [](const auto& e) {
        return e.clauseId == "GJB10157.5.2.10" &&
               e.state == analysis::StandardRuleState::Violated;
    }), "5.2.10 concrete write-only evidence marks the clause violated");
    result = analyze("module probe(input logic a, output logic q); assign q = a; endmodule");
    check(std::ranges::any_of(result.standardAssessments, [](const auto& e) {
        return e.clauseId == "GJB10157.5.2.10" &&
               e.state == analysis::StandardRuleState::ManualReviewRequired;
    }), "5.2.10 clean bounded result remains manual review rather than claiming full equivalence");
    {
        analysis::AnalysisOptions selectedWriteOnlyTop;
        selectedWriteOnlyTop.standardId = "GJB10157-2021";
        selectedWriteOnlyTop.topModule = "probe";
        const auto exact = analysis::Analyzer(selectedWriteOnlyTop).analyzeSource(
            {"probe.sv", core::HdlLanguage::SystemVerilog,
             "module probe(input logic a, output logic q);\n"
             "  logic dead_signal;\n"
             "  always_comb begin q = a; dead_signal = a; end\n"
             "endmodule\n"});
        check(std::ranges::any_of(exact.violations, [](const auto& item) {
            return item.ruleId == "GJB10157.5.2.10" &&
                   item.severity == analysis::Severity::Error &&
                   item.line == 2 && item.column == 1 &&
                   item.message == "禁止顶层的输入管脚dead_signal悬空";
        }), "R-2-10 preserves native severity, declaration line, no-column mapping and exact diagnostic");
    }

    result = analyze(
        "module probe(input logic used, input logic unused, output logic y);\n"
        "logic declared_only; logic write_only; logic loaded;\n"
        "always_comb begin write_only = used; loaded = used; end\n"
        "assign y = loaded; endmodule\n");
    check(count(result, "GJB10157.5.2.18") == 3,
          "5.2.18 distinguishes an unused input, unused internal declaration, and driven unloaded signal");
    check(std::ranges::count_if(result.violations, [](const auto& item) {
              return item.ruleId == "GJB10157.5.2.18" &&
                     item.message == "冗余的端口";
          }) == 1 &&
          std::ranges::count_if(result.violations, [](const auto& item) {
              return item.ruleId == "GJB10157.5.2.18" &&
                     item.message == "冗余的内部信号";
          }) == 2,
          "5.2.18 preserves the constructor-selected native diagnostic variants");
    result = analyze(
        "module child(input logic a, output logic z); assign z = a; endmodule\n"
        "module probe(input logic used, output logic y, inout wire io);\n"
        "logic forwarded; logic sink; assign forwarded = used; assign y = used ^ sink;\n"
        "child u0(.a(forwarded), .z(sink)); endmodule\n");
    check(count(result, "GJB10157.5.2.18") == 0,
          "5.2.18 accepts read inputs, loaded internals, instance actuals, outputs, and boundary inout ports");
    result = analyze("module probe(output wire undriven, inout wire boundary); endmodule\n");
    check(count(result, "GJB10157.5.2.18") == 0,
          "5.2.18 does not infer redundancy from an undriven output or unobserved inout boundary alone");
    result = analyze(
        "module child(input logic i, output logic o, inout wire io); assign o = i; endmodule\n"
        "module probe(input logic source, inout wire boundary, output logic y);\n"
        "logic to_input; logic from_output; logic through_inout;\n"
        "assign to_input = source; assign y = source;\n"
        "child u0(.i(to_input), .o(from_output), .io(through_inout)); endmodule\n");
    check(count(result, "GJB10157.5.2.18") == 1 &&
          std::ranges::any_of(result.violations, [](const auto& item) {
              return item.ruleId == "GJB10157.5.2.18" &&
                     item.signal == std::optional<std::string>{"from_output"};
          }),
          "5.2.18 resolved named output formal drives rather than loads its actual");
    result = analyze(
        "module child(input logic i, output logic o, inout wire io); assign o = i; endmodule\n"
        "module probe(input logic source, output logic y);\n"
        "logic to_input; logic from_output; wire through_inout;\n"
        "assign to_input = source; assign y = source;\n"
        "child u0(to_input, from_output, through_inout); endmodule\n");
    check(count(result, "GJB10157.5.2.18") == 1 &&
          std::ranges::any_of(result.violations, [](const auto& item) {
              return item.ruleId == "GJB10157.5.2.18" &&
                     item.signal == std::optional<std::string>{"from_output"};
          }),
          "5.2.18 resolved positional output formal preserves native direction semantics");
    result = analyze(
        "module probe(input logic source, output logic y);\n"
        "logic externally_connected; assign y = source;\n"
        "unknown_child u0(.z(externally_connected)); endmodule\n");
    check(count(result, "GJB10157.5.2.18") == 0,
          "5.2.18 unresolved cell directions remain conservative instead of guessing an output");
    check(std::ranges::any_of(result.standardAssessments, [](const auto& e) {
        return e.clauseId == "GJB10157.5.2.18" &&
               e.state == analysis::StandardRuleState::ManualReviewRequired;
    }), "5.2.18 clean bounded result remains manual review because native relation coverage is partial");

    result = analyzeVerilog(
        "module probe(input clk, input rst, input d, output reg q);\n"
        "//synthesis async_set_reset \"rst\"\n"
        "always @(posedge clk) q <= d;\n"
        "// synthesis sync_set_reset \"rst\"\n"
        "endmodule\n");
    check(count(result, "GJB10157.12.1") == 2,
          "12.1 attachment async and sync reset synthesis directives both report");
    check(std::ranges::all_of(result.violations | std::views::filter([](const auto& item) {
        return item.ruleId == "GJB10157.12.1";
    }), [](const auto& item) {
        return item.severity == analysis::Severity::Warning &&
               item.message == "禁止使用综合工具改变代码的复位属性.";
    }), "12.1 preserves the recovered native Warning severity and exact message");
    result = analyzeVerilog(
        "module probe;\n"
        "wire async_set_reset; /* sync_set_reset */\n"
        "// synthesis ASYNC_SET_RESET \"rst\"\n"
        "// sync_set_reset and async_set_reset on one physical line\n"
        "initial $display(\"// async_set_reset\");\n"
        "endmodule\n");
    check(count(result, "GJB10157.12.1") == 2,
          "12.1 preserves exact case, // suffix, raw-string and one-per-line native boundaries");
    check(std::ranges::any_of(result.violations, [](const auto& item) {
        return item.ruleId == "GJB10157.12.1" && item.line == 4 && item.column == 23;
    }), "12.1 tests async first and anchors the selected keyword on a mixed-keyword line");
    check(std::ranges::any_of(result.standardAssessments, [](const auto& e) {
        return e.clauseId == "GJB10157.12.1" &&
               e.state == analysis::StandardRuleState::Violated;
    }), "12.1 concrete reset-attribute directive marks the clause violated");
    result = analyzeVerilog(
        "module probe(input clk, input rst, input d, output reg q);\n"
        "always @(posedge clk or negedge rst) if (!rst) q <= 1'b0; else q <= d;\n"
        "endmodule\n");
    check(count(result, "GJB10157.12.1") == 0,
          "12.1 accepts ordinary reset logic without reset-attribute synthesis directives");
    check(std::ranges::any_of(result.standardAssessments, [](const auto& e) {
        return e.clauseId == "GJB10157.12.1" &&
               e.state == analysis::StandardRuleState::ManualReviewRequired;
    }), "12.1 clean detector result remains manual review because full native equivalence is not claimed");

    result = analyzeVerilog(
        "module probe(input [1:0] sel, output reg q);\n"
        "always @* begin case (sel) //synopsys full_case\n"
        "2'b00: q = 1'b0; default: q = 1'b1; endcase end\n"
        "endmodule\n");
    check(count(result, "GJB10157.12.3") == 1,
          "12.3 attachment-shaped synopsys full_case hot comment reports on its case");
    check(std::ranges::any_of(result.violations, [](const auto& item) {
        return item.ruleId == "GJB10157.12.3" &&
               item.severity == analysis::Severity::Warning &&
               item.message == "Synopsys full case用于模块probe第2行.";
    }), "A-16-1 preserves the recovered Warning severity and native message format");
    result = analyzeVerilog(
        "module probe(input [1:0] sel, output reg q);\n"
        "always @* begin (* full_case = 1 *) casez (sel)\n"
        "2'b0?: q = 1'b0; default: q = 1'b1; endcase end\n"
        "endmodule\n");
    check(count(result, "GJB10157.12.3") == 1,
          "12.3 exact explicit full_case attribute on a casez reports");
    result = analyzeVerilog(
        "module probe(input [1:0] sel, output reg q);\n"
        "// synopsys full_case is mentioned but is not attached to a case\n"
        "always @* begin case (sel)\n"
        "2'b00: q = 1'b0; default: q = 1'b1; endcase end\n"
        "initial $display(\"synopsys full_case\");\n"
        "endmodule\n");
    check(count(result, "GJB10157.12.3") == 0,
          "12.3 excludes unattached comments and string contents");
    result = analyzeVerilog(
        "module probe(input [1:0] sel, output reg q);\n"
        "always @* begin (* note = \"full_case\" *) case (sel)\n"
        "default: q = 1'b0; endcase end\nendmodule\n");
    check(count(result, "GJB10157.12.3") == 0,
          "A-16-1 does not confuse an attribute value with the exact attribute key");
    result = analyzeVerilog(
        "module probe(input [1:0] sel, output reg q);\n"
        "always @* begin (* full_case_extra = 1 *) case (sel)\n"
        "default: q = 1'b0; endcase end\nendmodule\n");
    check(count(result, "GJB10157.12.3") == 0,
          "A-16-1 keeps the explicit AST attribute key exact");
    result = analyzeVerilog(
        "module probe(input [1:0] sel, output reg q);\n"
        "always @* begin (* full_case = 0 *) (* other = 1 *) case (sel)\n"
        "default: q = 1'b0; endcase end\nendmodule\n");
    check(count(result, "GJB10157.12.3") == 1,
          "A-16-1 walks consecutive attached attribute blocks and ignores attribute value");
    result = analyzeVerilog(
        "module probe(input [1:0] sel, output reg q);\n"
        "always @* begin case (sel) // synopsys full_case_extra\n"
        "default: q = 1'b0; endcase end\nendmodule\n");
    check(count(result, "GJB10157.12.3") == 1,
          "A-16-1 preserves the recovered lexer prefix action inside a hot comment");
    result = analyzeVerilog(
        "module probe(input [1:0] sel, output reg q);\n"
        "always @* begin case (sel) /* synthesis full_case */\n"
        "default: q = 1'b0; endcase end\nendmodule\n");
    check(count(result, "GJB10157.12.3") == 1,
          "A-16-1 accepts the recovered Yosys synthesis hot-comment producer");
    result = analyzeVerilog(
        "module probe(input [1:0] sel, output reg q);\n"
        "always @* begin (* FULL_CASE = 1 *) case (sel) // SYNOPSYS full_case\n"
        "default: q = 1'b0; endcase end\nendmodule\n");
    check(count(result, "GJB10157.12.3") == 0,
          "A-16-1 keeps explicit keys and lexer producers case-sensitive");
    result = analyzeVerilog(
        "module probe(input [1:0] sel, output reg q);\n"
        "always @* begin case /* synopsys full_case */ (sel)\n"
        "default: q = 1'b0; endcase end\nendmodule\n");
    check(count(result, "GJB10157.12.3") == 0,
          "A-16-1 does not attach a hot comment before the case selector closes");
    result = analyzeVerilog(
        "module probe(input [1:0] sel, output reg q);\n"
        "always @* begin case (sel) // pragma full_case\n"
        "default: q = 1'b0; endcase end\nendmodule\n");
    check(count(result, "GJB10157.12.3") == 0,
          "A-16-1 excludes an unproved pragma producer from the AST hot-comment path");
    result = analyzeVerilog(
        "module probe(input [1:0] sel, output reg q);\n"
        "`ifdef UNUSED\n"
        "always @* begin case (sel) // synopsys full_case\n"
        "default: q = 1'b0; endcase end\n"
        "`endif\n"
        "always @* begin case (sel) default: q = 1'b0; endcase end\n"
        "endmodule\n");
    check(count(result, "GJB10157.12.3") == 0,
          "12.3 ignores a full_case attribute in an inactive conditional branch");
    check(std::ranges::any_of(result.standardAssessments, [](const auto& e) {
        return e.clauseId == "GJB10157.12.3" &&
               e.state == analysis::StandardRuleState::ManualReviewRequired;
    }), "12.3 clean bounded detector remains manual review rather than proven compliant");

    result = analyze(
        "library ieee; use ieee.std_logic_1164.all;\n"
        "entity probe is port(CLK, MY_RST1, A_RST2_A: in std_logic; Q: out std_logic); end;\n"
        "architecture rtl of probe is signal my_rst_int_2, reset, my_sig: std_logic; begin\n"
        "my_rst_int_2 <= A_RST2_A; reset <= not my_rst_int_2; my_sig <= not reset;\n"
        "process(MY_RST1, CLK)\nbegin\nif MY_RST1='1' then Q <= '0';\nelsif rising_edge(CLK) then Q <= '1';\nend if;\nend process;\n"
        "process(my_sig, CLK)\nbegin\nif my_sig='1' then Q <= '0';\nelsif rising_edge(CLK) then Q <= '1';\nend if;\nend process;\n"
        "end architecture;\n", true);
    check(count(result, "GJB9765.4.5.f.30") == 5,
          "4.5.f.30 attachment-shaped reset ports and complete alias chain require rst prefix");
    result = analyze(
        "library ieee; use ieee.std_logic_1164.all;\n"
        "entity probe is port(CLK, rst1, rst2: in std_logic; Q: out std_logic); end;\n"
        "architecture rtl of probe is signal rst_2, rst_3, rst_4: std_logic; begin\n"
        "rst_2 <= rst2; rst_3 <= not rst_2; rst_4 <= not rst_3;\n"
        "process(rst1, CLK)\nbegin\nif rst1='1' then Q <= '0';\nelsif rising_edge(CLK) then Q <= '1';\nend if;\nend process;\n"
        "process(rst_4, CLK)\nbegin\nif rst_4='1' then Q <= '0';\nelsif rising_edge(CLK) then Q <= '1';\nend if;\nend process;\n"
        "end architecture;\n", true);
    check(count(result, "GJB9765.4.5.f.30") == 0,
          "4.5.f.30 attachment compliant rst-prefixed alias chain remains clear");
    result = analyze(
        "entity probe is port(clk, reset_main: in bit; q: out bit); end;\n"
        "architecture rtl of probe is begin\n"
        "process(reset_main, clk) begin if reset_main='1' then q <= '0'; "
        "elsif rising_edge(clk) then q <= '1'; end if; end process;\n"
        "end architecture;\n", true);
    check(count(result, "GJB9765.4.5.f.30") == 1,
          "4.5.f.30 documented policy is exact rst prefix and does not broaden to reset");
    result = analyze(
        "entity probe is port(clk: in bit); end;\n"
        "architecture rtl of probe is signal data_rst_shadow, reset_data: bit; begin end;\n", true);
    check(count(result, "GJB9765.4.5.f.30") == 1,
          "4.5.f.30 reports rst substring in a non-reset signal but does not broaden it to reset");

    result = analyze(
        "entity probe is port(clk, rst_source: in bit; q: out bit); end;\n"
        "architecture rtl of probe is signal derived_bad: bit; begin\n"
        "derived_bad <= rst_source;\n"
        "process(rst_source, clk) begin if rst_source='1' then q <= '0'; "
        "elsif rising_edge(clk) then q <= derived_bad; end if; end process;\n"
        "end architecture;\n", true);
    check(count(result, "GJB9765.4.5.f.30") == 0,
          "4.5.f.30 native graph is directed and does not propagate a reset source downstream to an assignment target");

    result = analyze(
        "entity probe is port(clk, rst_source: in bit; q: out bit); end;\n"
        "architecture rtl of probe is signal bad_reset: bit; begin\n"
        "bad_reset <= not rst_source;\n"
        "process(bad_reset, clk) begin if bad_reset='1' then q <= '0'; "
        "elsif rising_edge(clk) then q <= '1'; end if; end process;\n"
        "end architecture;\n", true);
    check(count(result, "GJB9765.4.5.f.30") == 1,
          "4.5.f.30 follows a native unary-NOT assignment from reset target upstream to its source");

    result = analyze(
        "entity probe is port(clk: in bit; q: out bit); end;\n"
        "architecture rtl of probe is signal bad_reset, source_bad: bit; begin\n"
        "nested_block: block begin bad_reset <= source_bad; end block;\n"
        "process(bad_reset, clk) begin if bad_reset='1' then q <= '0'; "
        "elsif rising_edge(clk) then q <= '1'; end if; end process;\n"
        "end architecture;\n", true);
    check(count(result, "GJB9765.4.5.f.30") == 1,
          "4.5.f.30 excludes nested block assignments from the architecture-direct native relation graph");
    result = analyze(
        "entity alpha is port(clk, bad_source: in bit; q: out bit); end;\n"
        "architecture rtl_alpha of alpha is signal rst_alias: bit; begin\n"
        "rst_alias <= bad_source;\n"
        "process(rst_alias, clk) begin if rst_alias='1' then q <= '0'; "
        "elsif rising_edge(clk) then q <= '1'; end if; end process;\n"
        "end architecture;\n"
        "entity beta is port(clk, rst_payload: in bit; q: out bit); end;\n"
        "architecture rtl_beta of beta is begin q <= rst_payload; end architecture;\n", true);
    check(count(result, "GJB9765.4.5.f.30") == 2,
          "4.5.f.30 isolates multi-unit design roots while preserving upstream reset propagation and non-reset rst checks");
    check(std::ranges::any_of(result.violations, [](const auto& item) {
        return item.ruleId == "GJB9765.4.5.f.30" &&
               item.message.find("bad_source") != std::string::npos;
    }), "4.5.f.30 propagates a reset alias to its upstream source inside the owning architecture");
    check(std::ranges::any_of(result.violations, [](const auto& item) {
        return item.ruleId == "GJB9765.4.5.f.30" &&
               item.message.find("rst_payload") != std::string::npos;
    }), "4.5.f.30 checks a non-reset rst name in a second design root without merging reset identity");
    analysis::AnalysisOptions splitResetNameOptions;
    splitResetNameOptions.standardId = "GJB9765-2020";
    const auto splitResetNames = analysis::Analyzer(splitResetNameOptions).analyzeSources({
        {"split_entity.vhd", core::HdlLanguage::Vhdl,
         "entity split_probe is port(clk, bad_source, rst_payload: in bit; q: out bit); end;\n"},
        {"split_architecture.vhd", core::HdlLanguage::Vhdl,
         "architecture rtl of split_probe is signal rst_alias: bit; begin\n"
         "rst_alias <= bad_source;\n"
         "process(rst_alias, clk) begin if rst_alias='1' then q <= '0'; "
         "elsif rising_edge(clk) then q <= '1'; end if; end process;\n"
         "q <= rst_payload; end architecture;\n"}
    });
    check(count(splitResetNames, "GJB9765.4.5.f.30") == 2,
          "4.5.f.30 binds a unique entity file to its separate architecture file for reset naming");
    check(std::ranges::count_if(splitResetNames.violations, [](const auto& item) {
        return item.ruleId == "GJB9765.4.5.f.30" &&
               item.file.filename() == "split_entity.vhd";
    }) == 2, "4.5.f.30 preserves entity-port source locations for split-file diagnostics");
    const auto ambiguousSplitResetNames = analysis::Analyzer(splitResetNameOptions).analyzeSources({
        {"duplicate_a.vhd", core::HdlLanguage::Vhdl,
         "entity duplicate_probe is port(clk, bad_source: in bit; q: out bit); end;\n"},
        {"duplicate_b.vhd", core::HdlLanguage::Vhdl,
         "entity duplicate_probe is port(clk, other_source: in bit; q: out bit); end;\n"},
        {"duplicate_architecture.vhd", core::HdlLanguage::Vhdl,
         "architecture rtl of duplicate_probe is signal rst_alias: bit; begin\n"
         "rst_alias <= bad_source;\n"
         "process(rst_alias, clk) begin if rst_alias='1' then q <= '0'; "
         "elsif rising_edge(clk) then q <= '1'; end if; end process;\n"
         "end architecture;\n"}
    });
    check(count(ambiguousSplitResetNames, "GJB9765.4.5.f.30") == 0,
          "4.5.f.30 does not guess a split-file binding when duplicate entity names are ambiguous");
    check(std::ranges::any_of(ambiguousSplitResetNames.standardAssessments, [](const auto& e) {
        return e.clauseId == "GJB9765.4.5.f.30" &&
               e.state == analysis::StandardRuleState::ManualReviewRequired;
    }), "4.5.f.30 ambiguous split-file binding remains manual review rather than compliant");
    check(std::ranges::any_of(result.standardAssessments, [](const auto& e) {
        return e.clauseId == "GJB9765.4.5.f.30" &&
               e.state == analysis::StandardRuleState::Violated;
    }), "4.5.f.30 concrete naming evidence marks the clause violated");

    result = analyze(
        "entity probe is port(a: in bit; q: out bit); end;\n"
        "architecture rtl of probe is signal unused_sig: bit; begin q <= a; end;\n", true);
    check(count(result, "GJB9765.4.4.b") == 1,
          "4.4.b reports a declared signal absent from the native-style use set");
    result = analyze(
        "entity probe is port(a: in bit; q: out bit); end;\n"
        "architecture rtl of probe is signal lhs_only: bit; begin lhs_only <= a; q <= a; end;\n", true);
    check(count(result, "GJB9765.4.4.b") == 0,
          "4.4.b counts an assignment target as use like native AST_ASSIGN_LE child traversal");
    result = analyze(
        "entity probe is port(a: in bit; q: out bit); end;\n"
        "architecture rtl of probe is signal rhs_used: bit; begin rhs_used <= a; q <= rhs_used; end;\n", true);
    check(count(result, "GJB9765.4.4.b") == 0,
          "4.4.b keeps an ordinary RHS-used signal clear");
    check(std::ranges::any_of(result.standardAssessments, [](const auto& e) {
        return e.clauseId == "GJB9765.4.4.b" &&
               e.state == analysis::StandardRuleState::ManualReviewRequired;
    }), "4.4.b clean bounded detector remains manual review rather than proven compliant");

    result = analyze(
        "entity probe is port(a, b, c, d: in bit; q: out bit); end;\n"
        "architecture rtl of probe is begin\n"
        "process(a, c, d) begin\n"
        "q <= a and b;\n"
        "end process; end architecture;\n", true);
    check(count(result, "GJB9765.4.3.a") == 3,
          "4.3.a reports each missing or redundant sensitivity name separately");
    check(std::ranges::any_of(result.violations, [](const auto& e) {
        return e.ruleId == "GJB9765.4.3.a" && e.line == 4 &&
               e.signal && *e.signal == "b" &&
               e.message.find("missing") != std::string::npos;
    }), "4.3.a missing signal is located at its first process-body read");
    check(std::ranges::count_if(result.violations, [](const auto& e) {
        return e.ruleId == "GJB9765.4.3.a" && e.line == 3 && e.signal &&
               (*e.signal == "c" || *e.signal == "d") &&
               e.message.find("unread") != std::string::npos;
    }) == 2, "4.3.a redundant signals are located at their sensitivity-list entries");
    result = analyze(
        "entity probe is port(a: in bit; q: out bit); end;\n"
        "architecture rtl of probe is signal driven_only: bit; begin\n"
        "process(a, driven_only) begin driven_only <= a; q <= a; end process;\n"
        "end architecture;\n", true);
    check(count(result, "GJB9765.4.3.a") == 1 &&
          std::ranges::any_of(result.violations, [](const auto& e) {
              return e.ruleId == "GJB9765.4.3.a" && e.signal &&
                     *e.signal == "driven_only";
          }), "4.3.a treats a listed write-only signal as redundant, matching the native driven subset");
    result = analyze(
        "entity probe is port(a, b: in bit; q: out bit); end;\n"
        "architecture rtl of probe is begin\n"
        "process(A, B) begin q <= a and b; end process;\n"
        "end architecture;\n", true);
    check(count(result, "GJB9765.4.3.a") == 0 &&
          std::ranges::any_of(result.standardAssessments, [](const auto& e) {
              return e.clauseId == "GJB9765.4.3.a" &&
                     e.state == analysis::StandardRuleState::ManualReviewRequired;
          }), "4.3.a compares VHDL names case-insensitively and keeps clean bounded results under review");
    result = analyze(
        "entity probe is port(clk, d: in bit; q: out bit); end;\n"
        "architecture rtl of probe is begin process(clk) begin\n"
        "if rising_edge(clk) then q <= d; end if; end process; end architecture;\n", true);
    check(count(result, "GJB9765.4.3.a") == 0,
          "4.3.a excludes clocked processes from combinational set comparison");

    result = analyze(
        "entity probe is port(clk1, clk2, d: in bit; q: out bit); end;\n"
        "architecture rtl of probe is begin\n"
        "process(clk1, clk2) begin\n"
        "if clk1'event and clk1='1' then\n"
        "if clk2'event and clk2='1' then q <= d; end if;\n"
        "end if; end process; end architecture;\n", true);
    check(count(result, "GJB9765.4.4.d") == 1,
          "4.4.d attachment-shaped nested legacy event controls report the second event");
    check(std::ranges::any_of(result.violations, [](const auto& e) {
        return e.ruleId == "GJB9765.4.4.d" && e.line == 5;
    }), "4.4.d reports at the second event-control node");
    result = analyze(
        "entity probe is port(clk, d: in bit; q: out bit); end;\n"
        "architecture rtl of probe is begin\n"
        "process(clk) begin\n"
        "if rising_edge(clk) then q <= d;\n"
        "elsif falling_edge(clk) then q <= not d; end if;\n"
        "end process; end architecture;\n", true);
    check(count(result, "GJB9765.4.4.d") == 1,
          "4.4.d counts two edge controls even when they use the same signal");
    result = analyze(
        "entity probe is port(a, b, c, d: in bit; q: out bit); end;\n"
        "architecture rtl of probe is begin process(a,b,c) begin\n"
        "if rising_edge(a) then q <= d; elsif rising_edge(b) then q <= not d;\n"
        "elsif falling_edge(c) then q <= d; end if; end process; end;\n", true);
    check(count(result, "GJB9765.4.4.d") == 2,
          "4.4.d reports every event-control node after the first");
    result = analyze(
        "entity probe is port(a, b, d: in bit; q1, q2: out bit); end;\n"
        "architecture rtl of probe is begin\n"
        "process(a) begin if rising_edge(a) then q1 <= d; end if; end process;\n"
        "process(b) begin if falling_edge(b) then q2 <= d; end if; end process;\n"
        "end architecture;\n", true);
    check(count(result, "GJB9765.4.4.d") == 0,
          "4.4.d resets its event counter for each separate process");
    result = analyze(
        "entity probe is port(clk, d: in bit; q: out bit); end;\n"
        "architecture rtl of probe is begin process(clk) begin\n"
        "-- rising_edge(d) and d'event are comments\n"
        "if rising_edge(clk) then q <= d; end if; end process; end;\n", true);
    check(count(result, "GJB9765.4.4.d") == 0,
          "4.4.d excludes comment text and accepts one real event control");
    check(std::ranges::any_of(result.standardAssessments, [](const auto& e) {
        return e.clauseId == "GJB9765.4.4.d" &&
               e.state == analysis::StandardRuleState::ManualReviewRequired;
    }), "4.4.d clean bounded detector remains manual review rather than proven compliant");

    result = analyze(
        "entity probe is port(a: in bit; q: out bit); end;\n"
        "architecture rtl of probe is signal s_tmp: bit; begin\n"
        "q <= s_tmp;\nend architecture;\n", true);
    check(count(result, "GJB9765.4.6.a") == 1,
          "4.6.a reports an internal signal that is read without a recognized driver");
    check(std::ranges::any_of(result.violations, [](const auto& e) {
        return e.ruleId == "GJB9765.4.6.a" && e.line == 3 &&
               e.signal && *e.signal == "s_tmp";
    }), "4.6.a points to the first identifier read, matching the native helper");
    result = analyze(
        "entity probe is port(a: in bit; q: out bit); end;\n"
        "architecture rtl of probe is signal s_tmp: bit; begin\n"
        "s_tmp <= a; q <= s_tmp;\nend architecture;\n", true);
    check(count(result, "GJB9765.4.6.a") == 0,
          "4.6.a accepts a read signal after a recognized concurrent assignment");
    result = analyze(
        "entity probe is port(a: in bit; q: out bit); end;\n"
        "architecture rtl of probe is signal unused_sig: bit; begin q <= a; end;\n", true);
    check(count(result, "GJB9765.4.6.a") == 0,
          "4.6.a does not confuse an unread declaration with use before assignment");
    result = analyze(
        "entity probe is port(q1, q2: out bit); end;\n"
        "architecture rtl of probe is signal s_tmp: bit; begin\n"
        "q1 <= s_tmp; q2 <= s_tmp;\nend architecture;\n", true);
    check(count(result, "GJB9765.4.6.a") == 1,
          "4.6.a deduplicates repeated reads of the same undriven signal");
    check(std::ranges::any_of(result.standardAssessments, [](const auto& e) {
        return e.clauseId == "GJB9765.4.6.a" &&
               e.state == analysis::StandardRuleState::Violated;
    }), "4.6.a concrete undriven-read evidence marks the clause violated");

    result = analyze(
        "entity top is port(a: in bit; q: out bit); end;\n"
        "architecture rtl of top is\n"
        "component child port(d: in bit; y: out bit); end component;\n"
        "signal s_input: bit; signal s_output: bit; begin\n"
        "u0: child port map(d => s_input, y => s_output);\n"
        "q <= s_output; end architecture;\n", true);
    check(count(result, "GJB9765.4.6.a") == 1,
          "4.6.a treats a local component input actual as a read and its output actual as a driver");
    check(std::ranges::any_of(result.violations, [](const auto& e) {
        return e.ruleId == "GJB9765.4.6.a" && e.line == 5 &&
               e.signal && *e.signal == "s_input";
    }), "4.6.a locates the undriven component-input actual at the hierarchy read");

    analysis::AnalysisOptions hierarchyUndrivenOptions;
    hierarchyUndrivenOptions.standardId = "GJB9765-2020";
    const auto splitHierarchyUndriven =
        analysis::Analyzer(hierarchyUndrivenOptions).analyzeSources({
            {"child_direction.vhd", core::HdlLanguage::Vhdl,
             "entity child_direction is port(d: in bit; y: out bit); end;\n"},
            {"parent_direction.vhd", core::HdlLanguage::Vhdl,
             "entity parent_direction is port(a: in bit; q: out bit); end;\n"
             "architecture rtl of parent_direction is\n"
             "signal s_input: bit; signal s_output: bit; begin\n"
             "u0: entity work.child_direction port map(s_input, s_output);\n"
             "q <= s_output; end architecture;\n"}
        });
    check(count(splitHierarchyUndriven, "GJB9765.4.6.a") == 1,
          "4.6.a resolves positional input/output directions from a uniquely bound cross-file entity");
    check(std::ranges::any_of(splitHierarchyUndriven.violations, [](const auto& e) {
        return e.ruleId == "GJB9765.4.6.a" &&
               e.file.filename() == "parent_direction.vhd" && e.line == 4 &&
               e.signal && *e.signal == "s_input";
    }), "4.6.a reports the cross-file input read but accepts the entity-output driver");

    const auto ambiguousHierarchyUndriven =
        analysis::Analyzer(hierarchyUndrivenOptions).analyzeSources({
            {"ambiguous_input_a.vhd", core::HdlLanguage::Vhdl,
             "entity ambiguous_direction is port(d: in bit); end;\n"},
            {"ambiguous_input_b.vhd", core::HdlLanguage::Vhdl,
             "entity ambiguous_direction is port(y: out bit); end;\n"},
            {"ambiguous_direction_parent.vhd", core::HdlLanguage::Vhdl,
             "entity ambiguous_direction_parent is port(q: out bit); end;\n"
             "architecture rtl of ambiguous_direction_parent is signal s: bit; begin\n"
             "u0: entity work.ambiguous_direction port map(s); q <= '0';\n"
             "end architecture;\n"}
        });
    check(count(ambiguousHierarchyUndriven, "GJB9765.4.6.a") == 0,
          "4.6.a does not guess whether an actual is read or driven when entity binding is ambiguous");
    check(std::ranges::any_of(ambiguousHierarchyUndriven.standardAssessments,
        [](const auto& e) {
            return e.clauseId == "GJB9765.4.6.a" &&
                   e.state == analysis::StandardRuleState::ManualReviewRequired;
        }), "4.6.a ambiguous direction binding remains manual review rather than compliant");

    result = analyze(
        "entity probe is port(a, b: in std_logic_vector(7 downto 0); q: out bit); end;\n"
        "architecture rtl of probe is\n"
        "signal wide: std_logic_vector(8 downto 0);\n"
        "signal narrow: std_logic_vector(6 downto 0);\n"
        "signal same: std_logic_vector(7 downto 0);\n"
        "begin wide <= a; narrow <= a; same <= a;\n"
        "process(all) begin wide <= b; if a = narrow then q <= '1'; end if;\n"
        "same <= a and narrow; end process; end architecture;\n", true);
    check(count(result, "GJB9765.4.6.b.1") == 5,
          "4.6.b.1 reports widening, truncating, sequential, comparison and bitwise width mismatches");
    check(std::ranges::count_if(result.violations, [](const auto& e) {
        return e.ruleId == "GJB9765.4.6.b.1" &&
               e.message.find("Assignment operands have different widths") != std::string::npos;
    }) == 3, "4.6.b.1 treats either assignment width direction as unequal");
    result = analyze(
        "entity probe is port(a, b: in std_logic_vector(7 downto 0); q: out bit); end;\n"
        "architecture rtl of probe is signal same: std_logic_vector(7 downto 0);\n"
        "begin same <= a; process(all) begin if a = b then q <= '1'; end if;\n"
        "same <= a and b; end process; end architecture;\n", true);
    check(count(result, "GJB9765.4.6.b.1") == 0,
          "4.6.b.1 accepts equal known widths across all three native branches");

    result = analyze(
        "entity probe is port(a, b: in std_logic_vector(7 downto 0); q: out std_logic_vector(7 downto 0)); end;\n"
        "architecture rtl of probe is begin process(all) begin\n"
        "q <= a(7 downto 0) and b(3 downto 0);\n"
        "end process; end architecture;\n", true);
    check(count(result, "GJB9765.4.6.b.1") == 1,
          "4.6.b.1 resolves both selected-name operand widths in a native bitwise node");
    check(std::ranges::any_of(result.violations, [](const auto& e) {
        return e.ruleId == "GJB9765.4.6.b.1" && e.line == 3 &&
               e.message.find("a(7 downto 0)") != std::string::npos &&
               e.message.find("b(3 downto 0)") != std::string::npos;
    }), "4.6.b.1 reports the selected-name bitwise mismatch at the operator");
    result = analyze(
        "entity probe is port(a, b: in std_logic_vector(7 downto 0); q: out std_logic_vector(3 downto 0)); end;\n"
        "architecture rtl of probe is begin process(all) begin\n"
        "q <= a(3 downto 0) xor b(3 downto 0);\n"
        "end process; end architecture;\n", true);
    check(count(result, "GJB9765.4.6.b.1") == 0,
          "4.6.b.1 accepts equal statically known selected-name operand widths");
    result = analyze(
        "entity probe is port(a, b: in std_logic_vector(3 downto 0); wide: in std_logic_vector(8 downto 0); q: out std_logic_vector(8 downto 0)); end;\n"
        "architecture rtl of probe is begin process(all) begin\n"
        "q <= (a & b) or wide;\n"
        "end process; end architecture;\n", true);
    check(count(result, "GJB9765.4.6.b.1") == 1,
          "4.6.b.1 resolves a parenthesized VHDL concatenation before comparing bitwise operands");

    result = analyze(
        "entity probe is port(b: in unsigned(7 downto 0); q: out unsigned(7 downto 0)); end;\n"
        "architecture rtl of probe is begin process(all) begin\n"
        "q <= to_unsigned(3, 4);\n"
        "end process; end architecture;\n", true);
    check(count(result, "GJB9765.4.6.b.1") == 1,
          "4.6.b.1 uses the explicit size argument of a native-whitelisted conversion call");
    check(std::ranges::any_of(result.violations, [](const auto& e) {
        return e.ruleId == "GJB9765.4.6.b.1" && e.line == 3 &&
               e.message.find("to_unsigned(3,4)") != std::string::npos;
    }), "4.6.b.1 locates an assignment mismatch produced by a conversion result width");
    result = analyze(
        "entity probe is port(b: in unsigned(7 downto 0); q: out signed(7 downto 0)); end;\n"
        "architecture rtl of probe is begin process(all) begin\n"
        "q <= to_signed(3, b'length);\n"
        "end process; end architecture;\n", true);
    check(count(result, "GJB9765.4.6.b.1") == 0,
          "4.6.b.1 accepts a whitelisted conversion sized by a known object's length attribute");
    result = analyze(
        "entity probe is port(b: in std_logic_vector(7 downto 0); q: out std_logic_vector(3 downto 0)); end;\n"
        "architecture rtl of probe is begin process(all) begin\n"
        "q <= conv_std_logic_vector(3, b'length);\n"
        "end process; end architecture;\n", true);
    check(count(result, "GJB9765.4.6.b.1") == 1,
          "4.6.b.1 reports a whitelisted conversion whose length attribute exceeds the target width");
    result = analyze(
        "entity probe is port(b: in unsigned(7 downto 0); q: out unsigned(7 downto 0)); end;\n"
        "architecture rtl of probe is begin process(all) begin\n"
        "q <= to_unsigned(3, 4) and b;\n"
        "end process; end architecture;\n", true);
    check(count(result, "GJB9765.4.6.b.1") == 1,
          "4.6.b.1 compares a whitelisted conversion result with the other native bitwise operand");
    result = analyze(
        "entity probe is generic(WIDTH: positive := 4); port(q: out unsigned(7 downto 0)); end;\n"
        "architecture rtl of probe is begin process(all) begin\n"
        "q <= to_unsigned(3, WIDTH);\n"
        "end process; end architecture;\n", true);
    check(count(result, "GJB9765.4.6.b.1") == 1,
          "4.6.b.1 resolves a direct integer generic default used as a conversion width");
    result = analyze(
        "entity probe is generic(WIDTH: positive := 4); port(q: out unsigned(7 downto 0)); end;\n"
        "architecture rtl of probe is constant WIDTH: positive := 8; begin process(all) begin\n"
        "q <= to_unsigned(3, WIDTH);\n"
        "end process; end architecture;\n", true);
    check(count(result, "GJB9765.4.6.b.1") == 0,
          "4.6.b.1 lets a direct architecture constant override an owning-entity generic");
    result = analyze(
        "entity probe is port(b: in unsigned(7 downto 0); q: out unsigned(7 downto 0)); end;\n"
        "architecture rtl of probe is constant WIDTH: positive := 4; begin process(all) begin\n"
        "q <= to_unsigned(3, WIDTH) and b;\n"
        "end process; end architecture;\n", true);
    check(count(result, "GJB9765.4.6.b.1") == 1,
          "4.6.b.1 resolves an architecture constant in a native bitwise conversion operand");
    result = analyze(
        "entity probe is generic(WIDTH: positive := 2 + 2); port(q: out unsigned(7 downto 0)); end;\n"
        "architecture rtl of probe is begin process(all) begin\n"
        "q <= to_unsigned(3, WIDTH);\n"
        "end process; end architecture;\n", true);
    check(count(result, "GJB9765.4.6.b.1") == 1,
          "4.6.b.1 evaluates native-supported addition in a generic width expression");
    result = analyze(
        "entity probe is generic(BASE: positive := 2; WIDTH: positive := BASE * 2); port(q: out unsigned(7 downto 0)); end;\n"
        "architecture rtl of probe is begin process(all) begin\n"
        "q <= to_unsigned(3, WIDTH);\n"
        "end process; end architecture;\n", true);
    check(count(result, "GJB9765.4.6.b.1") == 1,
          "4.6.b.1 resolves an earlier generic name inside a multiplication expression");
    result = analyze(
        "entity probe is port(q: out unsigned(7 downto 0)); end;\n"
        "architecture rtl of probe is constant NEG: integer := -4; constant WIDTH: positive := NEG + 12; begin process(all) begin\n"
        "q <= to_unsigned(3, WIDTH);\n"
        "end process; end architecture;\n", true);
    check(count(result, "GJB9765.4.6.b.1") == 0,
          "4.6.b.1 preserves a native-supported negative intermediate used by a later constant");
    result = analyze(
        "entity probe is generic(WIDTH: positive := 2 + 3 * 2); port(q: out unsigned(7 downto 0)); end;\n"
        "architecture rtl of probe is begin process(all) begin\n"
        "q <= to_unsigned(3, WIDTH);\n"
        "end process; end architecture;\n", true);
    check(count(result, "GJB9765.4.6.b.1") == 0,
          "4.6.b.1 applies VHDL multiplication precedence in a recovered arithmetic expression");
    result = analyze(
        "entity probe is generic(WIDTH: positive := 16 / 4); port(q: out unsigned(7 downto 0)); end;\n"
        "architecture rtl of probe is begin process(all) begin\n"
        "q <= to_unsigned(3, WIDTH);\n"
        "end process; end architecture;\n", true);
    check(count(result, "GJB9765.4.6.b.1") == 1,
          "4.6.b.1 evaluates native-supported integer division in a generic width expression");
    result = analyze(
        "entity probe is generic(WIDTH: positive := 8 mod 4); port(q: out unsigned(7 downto 0)); end;\n"
        "architecture rtl of probe is begin process(all) begin\n"
        "q <= to_unsigned(3, WIDTH);\n"
        "end process; end architecture;\n", true);
    check(count(result, "GJB9765.4.6.b.1") == 0 &&
          std::ranges::any_of(result.standardAssessments, [](const auto& item) {
              return item.clauseId == "GJB9765.4.6.b.1" &&
                     item.state == analysis::StandardRuleState::ManualReviewRequired;
          }),
          "4.6.b.1 fails closed for MOD, which is outside the recovered evaluator type set");
    result = analyze(
        "entity probe is generic(WIDTH: integer := 8 / 0); port(q: out unsigned(7 downto 0)); end;\n"
        "architecture rtl of probe is begin process(all) begin\n"
        "q <= to_unsigned(3, WIDTH);\n"
        "end process; end architecture;\n", true);
    check(count(result, "GJB9765.4.6.b.1") == 0,
          "4.6.b.1 fails closed instead of evaluating a zero divisor");
    result = analyze(
        "entity probe is generic(WIDTH: positive := LATER; LATER: positive := 4); port(q: out unsigned(7 downto 0)); end;\n"
        "architecture rtl of probe is begin process(all) begin\n"
        "q <= to_unsigned(3, WIDTH);\n"
        "end process; end architecture;\n", true);
    check(count(result, "GJB9765.4.6.b.1") == 0,
          "4.6.b.1 does not guess a forward generic reference absent from the native traversal map");
    result = analyze(
        "entity probe is port(q: out unsigned(7 downto 0)); end;\n"
        "architecture rtl of probe is begin process constant WIDTH: positive := 4; begin\n"
        "q <= to_unsigned(3, WIDTH); wait;\n"
        "end process; end architecture;\n", true);
    check(count(result, "GJB9765.4.6.b.1") == 1,
          "4.6.b.1 collects a process-declarative constant from the architecture AST root");
    result = analyze(
        "entity probe is port(q: out unsigned(7 downto 0)); end;\n"
        "architecture rtl of probe is begin b: block constant WIDTH: positive := 4; begin\n"
        "q <= to_unsigned(3, WIDTH); end block;\n"
        "end architecture;\n", true);
    check(count(result, "GJB9765.4.6.b.1") == 1,
          "4.6.b.1 collects a block-declarative constant from the architecture AST root");
    result = analyze(
        "entity probe is port(q: out unsigned(7 downto 0)); end;\n"
        "architecture rtl of probe is procedure drive(signal x: out unsigned(7 downto 0)) is\n"
        "constant WIDTH: positive := 4; begin x <= to_unsigned(3, WIDTH); end procedure;\n"
        "begin process begin drive(q); wait; end process; end architecture;\n", true);
    check(count(result, "GJB9765.4.6.b.1") == 1,
          "4.6.b.1 collects a subprogram-declarative constant from the architecture AST root");
    result = analyze(
        "entity probe is generic(WIDTH: positive := 4); port(q: out unsigned(7 downto 0)); end;\n"
        "architecture rtl of probe is begin process constant WIDTH: positive := 8; begin\n"
        "q <= to_unsigned(3, WIDTH); wait; end process; end architecture;\n", true);
    check(count(result, "GJB9765.4.6.b.1") == 0,
          "4.6.b.1 preserves native preorder overwrite when a later local constant reuses a generic name");
    result = analyze(
        "package cfg is constant WIDTH: positive := 4; end package;\n"
        "entity probe is port(q: out unsigned(7 downto 0)); end;\n"
        "architecture rtl of probe is begin q <= to_unsigned(3, WIDTH); end architecture;\n", true);
    check(count(result, "GJB9765.4.6.b.1") == 0 &&
          std::ranges::any_of(result.standardAssessments, [](const auto& item) {
              return item.clauseId == "GJB9765.4.6.b.1" &&
                     item.state == analysis::StandardRuleState::ManualReviewRequired;
          }),
          "4.6.b.1 does not leak a constant from a separate package root into an architecture");
    result = analyze(
        "package helpers is procedure drive(signal q: out unsigned(7 downto 0)); end package;\n"
        "package body helpers is procedure drive(signal q: out unsigned(7 downto 0)) is\n"
        "constant WIDTH: positive := 4; begin q <= to_unsigned(3, WIDTH);\n"
        "end procedure; end package body;\n", true);
    check(count(result, "GJB9765.4.6.b.1") == 1,
          "4.6.b.1 analyzes conversion assignment widths inside a package-body procedure root");
    result = analyze(
        "package body helpers is procedure drive(signal q: out unsigned(7 downto 0)) is\n"
        "constant WIDTH: positive := 8; begin q <= to_unsigned(3, WIDTH);\n"
        "end procedure; end package body;\n", true);
    check(count(result, "GJB9765.4.6.b.1") == 0,
          "4.6.b.1 accepts an equal conversion assignment width inside a package body");
    result = analyze(
        "package body helpers is procedure drive is\n"
        "variable value: unsigned(7 downto 0); begin\n"
        "value := to_unsigned(3, 4); end procedure; end package body;\n", true);
    check(count(result, "GJB9765.4.6.b.1") == 0 &&
          std::ranges::any_of(result.standardAssessments, [](const auto& item) {
              return item.clauseId == "GJB9765.4.6.b.1" &&
                     item.state == analysis::StandardRuleState::ManualReviewRequired;
          }),
          "4.6.b.1 excludes AST_VARASGN variable assignment nodes and keeps clean results under review");
    result = analyze(
        "package body helpers is procedure compare is\n"
        "variable wide: unsigned(7 downto 0); variable narrow: unsigned(3 downto 0);\n"
        "begin if wide = narrow then null; end if; end procedure; end package body;\n", true);
    check(count(result, "GJB9765.4.6.b.1") == 1,
          "4.6.b.1 retains package-body AST_VARIABLE widths for relation operands");
    result = analyze(
        "package body helpers is procedure combine(a: in unsigned(7 downto 0);\n"
        "b: in unsigned(3 downto 0); q: out unsigned(7 downto 0)) is begin\n"
        "q <= a and b; end procedure; end package body;\n", true);
    check(count(result, "GJB9765.4.6.b.1") == 1,
          "4.6.b.1 analyzes bitwise operand widths in a package-body subprogram root");
    result = analyze(
        "package body helpers is procedure compare(a: in unsigned(7 downto 0);\n"
        "b: in unsigned(3 downto 0)) is begin\n"
        "if a = b then null; end if; end procedure; end package body;\n", true);
    check(count(result, "GJB9765.4.6.b.1") == 1,
          "4.6.b.1 analyzes relation operand widths in a package-body subprogram root");
    result = analyze(
        "package body helpers is procedure drive(signal q: out unsigned(7 downto 0)) is\n"
        "constant WIDTH: positive := 4; begin q <= to_unsigned(3, WIDTH);\n"
        "end procedure; end package body;\n"
        "entity probe is port(q: out unsigned(7 downto 0)); end;\n"
        "architecture rtl of probe is constant WIDTH: positive := 8; begin\n"
        "q <= to_unsigned(3, WIDTH); end architecture;\n", true);
    check(count(result, "GJB9765.4.6.b.1") == 1,
          "4.6.b.1 keeps package-body and architecture constant maps isolated by design root");
    result = analyze(
        "entity probe is port(q: out bit); end;\n"
        "architecture rtl of probe is begin process\n"
        "variable wide: unsigned(7 downto 0); variable narrow: unsigned(3 downto 0);\n"
        "begin if wide = narrow then q <= '0'; end if; wait; end process;\n"
        "end architecture;\n", true);
    check(count(result, "GJB9765.4.6.b.1") == 1,
          "4.6.b.1 resolves process-local variables used by a relation node");
    result = analyze(
        "entity probe is port(q: out unsigned(7 downto 0)); end;\n"
        "architecture rtl of probe is begin process\n"
        "variable wide: unsigned(7 downto 0); variable narrow: unsigned(3 downto 0);\n"
        "begin q <= wide and narrow; wait; end process; end architecture;\n", true);
    check(count(result, "GJB9765.4.6.b.1") == 1,
          "4.6.b.1 resolves process-local variables used by a bitwise node");
    result = analyze(
        "entity probe is port(d: in unsigned(3 downto 0); q: out unsigned(7 downto 0)); end;\n"
        "architecture rtl of probe is procedure drive(signal target: out unsigned(7 downto 0);\n"
        "signal source: in unsigned(3 downto 0)) is begin target <= source; end procedure;\n"
        "begin drive(q,d); end architecture;\n", true);
    check(count(result, "GJB9765.4.6.b.1") == 1,
          "4.6.b.1 resolves architecture-subprogram formals used by AST_ASSIGN_LE");
    result = analyze(
        "entity probe is port(q: out bit); end;\n"
        "architecture rtl of probe is begin process\n"
        "variable wide: unsigned(7 downto 0); variable narrow: unsigned(3 downto 0);\n"
        "begin wide := narrow; q <= '0'; wait; end process; end architecture;\n", true);
    check(count(result, "GJB9765.4.6.b.1") == 0,
          "4.6.b.1 does not promote process-local AST_VARASGN into the native assignment branch");
    result = analyze(
        "entity probe is generic(mask: unsigned(7 downto 0) := (others => '0'));\n"
        "port(q: out bit); end;\n"
        "architecture rtl of probe is signal narrow: unsigned(3 downto 0); begin\n"
        "process(all) begin if mask = narrow then q <= '0'; end if; end process;\n"
        "end architecture;\n", true);
    check(count(result, "GJB9765.4.6.b.1") == 1,
          "4.6.b.1 resolves a fixed-width vector generic from the owning entity");
    result = analyze(
        "entity probe is port(value, other: in unsigned(7 downto 0); q: out bit); end;\n"
        "architecture rtl of probe is begin process(all)\n"
        "variable value: unsigned(3 downto 0); begin\n"
        "if value = other then q <= '0'; end if; end process; end architecture;\n", true);
    check(count(result, "GJB9765.4.6.b.1") == 0,
          "4.6.b.1 preserves native port-over-variable width lookup priority");
    result = analyze(
        "entity probe is port(value, other: in unsigned(7 downto 0); q: out bit); end;\n"
        "architecture rtl of probe is begin process(all)\n"
        "constant value: unsigned(3 downto 0) := (others => '0'); begin\n"
        "if value = other then q <= '0'; end if; end process; end architecture;\n", true);
    check(count(result, "GJB9765.4.6.b.1") == 1,
          "4.6.b.1 preserves native localparam-over-port width lookup priority");
    result = analyze(
        "entity probe is generic(value: unsigned(3 downto 0) := (others => '0'));\n"
        "port(other: in unsigned(7 downto 0); q: out bit); end;\n"
        "architecture rtl of probe is signal value: unsigned(7 downto 0); begin\n"
        "process(all) begin if value = other then q <= '0'; end if; end process;\n"
        "end architecture;\n", true);
    check(count(result, "GJB9765.4.6.b.1") == 1,
          "4.6.b.1 preserves native generic-over-signal width lookup priority");
    result = analyze(
        "entity probe is port(other: in unsigned(7 downto 0); q: out bit); end;\n"
        "architecture rtl of probe is signal value: unsigned(7 downto 0); begin\n"
        "process(all) variable value: unsigned(3 downto 0); begin\n"
        "if value = other then q <= '0'; end if; end process; end architecture;\n", true);
    check(count(result, "GJB9765.4.6.b.1") == 0,
          "4.6.b.1 preserves native signal-over-variable width lookup priority");
    result = analyze(
        "entity probe is port(b: in unsigned(7 downto 0); q: out unsigned(7 downto 0)); end;\n"
        "architecture rtl of probe is begin process(all) begin\n"
        "q <= resize(b, 4);\n"
        "end process; end architecture;\n", true);
    check(count(result, "GJB9765.4.6.b.1") == 0,
          "4.6.b.1 does not infer widths for a function outside the recovered native whitelist");

    result = analyze(
        "entity probe is port(a: in std_logic_vector(7 downto 0); q: out bit); end;\n"
        "architecture rtl of probe is\n"
        "constant c_bool: boolean := true;\n"
        "constant c_word: std_logic_vector(7 downto 0) := \"00000000\";\n"
        "begin process(all) begin case a is\n"
        "when c_bool => q <= '0'; when c_word => q <= '1';\n"
        "when \"000\" => q <= '0'; when \"11111111\" => q <= '1';\n"
        "when others => null; end case; end process; end architecture;\n", true);
    check(count(result, "GJB9765.4.6.b.2") == 2,
          "4.6.b.2 compares direct named constants and direct literals with a simple-name selector");
    check(std::ranges::any_of(result.violations, [](const auto& e) {
        return e.ruleId == "GJB9765.4.6.b.2" && e.line == 6;
    }) && std::ranges::any_of(result.violations, [](const auto& e) {
        return e.ruleId == "GJB9765.4.6.b.2" && e.line == 7;
    }), "4.6.b.2 reports the mismatched named constant and string literal separately");
    result = analyze(
        "entity probe is port(a: in std_logic_vector(7 downto 0); q: out bit); end;\n"
        "architecture rtl of probe is\n"
        "constant c_word: std_logic_vector(7 downto 0) := \"00000000\";\n"
        "begin process(all) begin case a(2 downto 0) is\n"
        "when \"00\" => q <= '0'; when others => null; end case;\n"
        "case a is when c_word => q <= '1'; when others => null; end case;\n"
        "end process; end architecture;\n", true);
    check(count(result, "GJB9765.4.6.b.2") == 0,
          "4.6.b.2 excludes selected-name selectors and accepts equal direct named widths");
    result = analyze(
        "entity probe is generic(sel: unsigned(7 downto 0) := (others => '0'));\n"
        "port(q: out bit); end; architecture rtl of probe is\n"
        "constant narrow: unsigned(3 downto 0) := (others => '0'); begin\n"
        "process(all) begin case sel is when narrow => q <= '0';\n"
        "when others => q <= '1'; end case; end process; end architecture;\n", true);
    check(count(result, "GJB9765.4.6.b.2") == 1,
          "4.6.b.2 resolves a direct selector from an owning-entity vector generic");
    result = analyze(
        "entity probe is port(q: out bit); end; architecture rtl of probe is begin\n"
        "process(all) variable sel: unsigned(7 downto 0);\n"
        "constant narrow: unsigned(3 downto 0) := (others => '0'); begin\n"
        "case sel is when narrow => q <= '0'; when others => q <= '1'; end case;\n"
        "end process; end architecture;\n", true);
    check(count(result, "GJB9765.4.6.b.2") == 1,
          "4.6.b.2 resolves process-local variables and constants from the complete architecture root");
    result = analyze(
        "package body helpers is procedure choose is\n"
        "variable sel: unsigned(7 downto 0);\n"
        "constant narrow: unsigned(3 downto 0) := (others => '0'); begin\n"
        "case sel is when narrow => null; when others => null; end case;\n"
        "end procedure; end package body;\n", true);
    check(count(result, "GJB9765.4.6.b.2") == 1,
          "4.6.b.2 checks direct CASE names inside an independent package-body root");
    result = analyze(
        "entity probe is port(value: in unsigned(7 downto 0); q: out bit); end;\n"
        "architecture rtl of probe is begin process(all)\n"
        "variable value: unsigned(3 downto 0);\n"
        "constant equal_port: unsigned(7 downto 0) := (others => '0'); begin\n"
        "case value is when equal_port => q <= '0'; when others => q <= '1'; end case;\n"
        "end process; end architecture;\n", true);
    check(count(result, "GJB9765.4.6.b.2") == 0,
          "4.6.b.2 preserves native port-over-variable selector width priority");
    result = analyze(
        "entity probe is port(value, other: in unsigned(7 downto 0); q: out bit); end;\n"
        "architecture rtl of probe is begin process(all)\n"
        "constant value: unsigned(3 downto 0) := (others => '0'); begin\n"
        "case value is when other => q <= '0'; when others => q <= '1'; end case;\n"
        "end process; end architecture;\n", true);
    check(count(result, "GJB9765.4.6.b.2") == 1,
          "4.6.b.2 preserves native localparam-over-port selector width priority");

    result = analyze(
        "entity probe is port(a, b: in bit; q: out bit); end;\n"
        "architecture rtl of probe is begin process(all) begin\n"
        "case a is when '0' => q <= '0'; when '1' => q <= '1'; end case;\n"
        "end process; end architecture;\n", true);
    check(count(result, "GJB9765.4.8.c") == 1,
          "4.8.c native predicate still requires others when explicit bit choices are exhaustive");
    result = analyze(
        "entity probe is port(a, b: in bit; q: out bit); end;\n"
        "architecture rtl of probe is begin process(all) begin\n"
        "case a is when '0' => case b is when '0' => q <= '0';\n"
        "when '1' => q <= '1'; end case; when others => q <= '0'; end case;\n"
        "end process; end architecture;\n", true);
    check(count(result, "GJB9765.4.8.c") == 1,
          "4.8.c evaluates nested cases independently and reports only the inner missing default");
    result = analyze(
        "entity probe is port(a: in bit; q: out bit); end;\n"
        "architecture rtl of probe is begin process(all) begin\n"
        "case a is when '0' => q <= '0'; -- when others => q <= '1';\n"
        "when '1' => q <= '1'; end case; end process; end architecture;\n", true);
    check(count(result, "GJB9765.4.8.c") == 1,
          "4.8.c ignores commented lookalike others choices");
    result = analyze(
        "entity probe is port(a: in bit; q: out bit); end;\n"
        "architecture rtl of probe is begin process(all) begin\n"
        "case a is when '0' => q <= '0'; when others => q <= '1'; end case;\n"
        "end process; end architecture;\n", true);
    check(count(result, "GJB9765.4.8.c") == 0,
          "4.8.c accepts a direct when others choice");
    result = analyze(
        "entity probe is port(a, b: in bit; q: out bit); end;\n"
        "architecture rtl of probe is begin process(all) begin\n"
        "if a = '0' then q <= '0'; elsif b = '1' then q <= '1'; end if;\n"
        "end process; end architecture;\n", true);
    check(count(result, "GJB9765.4.8.c") == 0,
          "4.8.c excludes VHDL if/elsif trees lowered by the native frontend into marked CASE nodes");
    result = analyze(
        "entity probe is port(a: in bit; q: out bit); end;\n"
        "architecture rtl of probe is\n"
        "procedure choose(signal x: in bit; signal y: out bit) is begin\n"
        "case x is when '0' => y <= '0'; when '1' => y <= '1'; end case;\n"
        "end procedure; begin choose(a,q); end architecture;\n", true);
    check(count(result, "GJB9765.4.8.c") == 1,
          "4.8.c recursive design-root traversal includes architecture-declarative subprogram CASE nodes");
    result = analyze(
        "package helpers is procedure choose(x: in bit; y: out bit); end package;\n"
        "package body helpers is\n"
        "procedure choose(x: in bit; y: out bit) is begin\n"
        "case x is when '0' => y := '0'; when '1' => y := '1'; end case;\n"
        "end procedure; end package body;\n", true);
    check(count(result, "GJB9765.4.8.c") == 1,
          "4.8.c recursive design-root traversal includes package-body subprogram CASE nodes");
    result = analyze(
        "package body helpers is\n"
        "function choose(x: bit) return bit is begin\n"
        "case x is when '0' => return '0'; when '1' => return '1'; end case;\n"
        "end function; end helpers;\n", true);
    check(count(result, "GJB9765.4.8.c") == 1,
          "4.8.c package-body traversal accepts a named package-body terminator");
    result = analyze(
        "package body helpers is\n"
        "function choose(x: bit) return bit is begin\n"
        "case x is when '0' => return '0'; when '1' => return '1'; end case;\n"
        "end function; end;\n"
        "entity probe is port(a: in bit; q: out bit); end;\n"
        "architecture rtl of probe is begin process(all) begin\n"
        "case a is when '0' => q <= '0'; when others => q <= '1'; end case;\n"
        "end process; end architecture;\n", true);
    check(count(result, "GJB9765.4.8.c") == 1,
          "4.8.c bare package-body end does not absorb a following architecture root");
    result = analyze(
        "package body helpers is\n"
        "function choose(x: bit) return bit is begin\n"
        "case x is when '0' => return '0'; when others => return '1'; end case;\n"
        "end function; end package body;\n", true);
    check(count(result, "GJB9765.4.8.c") == 0,
          "4.8.c accepts a direct when others inside a package-body subprogram");
    result = analyze(
        "entity probe is generic(g_mode: integer := 0); port(q: out bit); end;\n"
        "architecture rtl of probe is begin\n"
        "g_choice: case g_mode generate\n"
        "when 0 => q <= '0'; when 1 => q <= '1'; end generate;\n"
        "end architecture;\n", true);
    check(count(result, "GJB9765.4.8.c") == 0,
          "4.8.c excludes AST_GENCASE case-generate roots even without a when others alternative");
    result = analyze(
        "entity probe is generic(g_mode: integer := 0); port(a: in bit; q: out bit); end;\n"
        "architecture rtl of probe is begin\n"
        "g_choice: case g_mode generate\n"
        "when 0 => process(all) begin\n"
        "case a is when '0' => q <= '0'; when '1' => q <= '1'; end case;\n"
        "end process; when others => q <= '0'; end generate;\n"
        "end architecture;\n", true);
    check(count(result, "GJB9765.4.8.c") == 1,
          "4.8.c still checks a sequential AST_CASE nested inside an AST_GENCASE branch");
    result = analyze(
        "entity probe is port(a: in bit; q: out bit); end;\n"
        "architecture rtl of probe is begin process(all) begin\n"
        "case a is end case;\n"
        "end process; end architecture;\n", true);
    check(count(result, "GJB9765.4.8.c") == 1,
          "4.8.c admits an empty but paired CASE node and reports its missing when others choice");

    result = analyze(
        "entity probe is port(a,tmp: in bit; q,d: out bit); end;\n"
        "architecture rtl of probe is begin process(a,tmp) begin\n"
        "case a is when '0' => q <= '0'; when tmp => q <= '1'; when others => null; end case;\n"
        "end process; with a select d <= '0' when '0', '1' when tmp; end architecture;\n", true);
    check(count(result, "GJB9765.4.8.b") == 2,
          "4.8.b reports runtime port choices in ordinary CASE and WITH SELECT independently");
    result = analyze(
        "entity probe is generic(g_choice: bit := '0'); port(a: in bit; q: out bit); end;\n"
        "architecture rtl of probe is constant c_choice: bit := '1';\n"
        "type state_t is (idle, active); signal state: state_t; begin process(all) begin\n"
        "case a is when g_choice => q <= '0'; when c_choice => q <= '1'; when others => null; end case;\n"
        "case state is when idle => q <= '0'; when active => q <= '1'; end case;\n"
        "end process; end architecture;\n", true);
    check(count(result, "GJB9765.4.8.b") == 0,
          "4.8.b accepts generic, constant, and enumeration-literal static choices");
    result = analyze(
        "entity static_owner is generic(CHOICE: bit := '0'); end;\n"
        "architecture rtl of static_owner is begin end architecture;\n"
        "entity runtime_owner is port(sel, CHOICE: in bit; q: out bit); end;\n"
        "architecture rtl of runtime_owner is begin process(all) begin\n"
        "case sel is when CHOICE => q <= '1'; when others => null; end case;\n"
        "end process; end architecture;\n", true);
    check(count(result, "GJB9765.4.8.b") == 1,
          "4.8.b rebuilds its static-name set per design root and does not leak another entity generic");
    result = analyze(
        "entity static_owner is generic(CHOICE: bit := '0'); end;\n"
        "architecture rtl of static_owner is begin end rtl;\n"
        "entity runtime_owner is port(sel, CHOICE: in bit; q: out bit); end;\n"
        "architecture rtl of runtime_owner is begin process(all) begin\n"
        "case sel is when CHOICE => q <= '1'; when others => null; end case;\n"
        "end process; end rtl;\n", true);
    check(count(result, "GJB9765.4.8.b") == 1,
          "4.8.b design-root isolation accepts the legal named architecture end form");
    result = analyze(
        "entity static_owner is generic(CHOICE: bit := '0'); end;\n"
        "architecture rtl of static_owner is begin end;\n"
        "entity runtime_owner is port(sel, CHOICE: in bit; q: out bit); end;\n"
        "architecture rtl of runtime_owner is begin process(all) begin\n"
        "case sel is when CHOICE => q <= '1'; when others => null; end case;\n"
        "end process; end;\n", true);
    check(count(result, "GJB9765.4.8.b") == 1,
          "4.8.b design-root isolation accepts the legal bare architecture end form");
    result = analyze(
        "entity probe is port(a,b,sel: in bit; q: out bit); end;\n"
        "architecture rtl of probe is begin process(all) begin\n"
        "case sel is when a | b => q <= '1'; when others => null; end case;\n"
        "end process; end architecture;\n", true);
    check(count(result, "GJB9765.4.8.b") == 1,
          "4.8.b emits at most once for one variable choice expression");
    result = analyze(
        "entity probe is port(sel: in bit); end; architecture rtl of probe is begin\n"
        "g: case sel generate when '0' => b0: block begin end block;\n"
        "when others => b1: block begin end block; end generate; end architecture;\n", true);
    check(count(result, "GJB9765.4.8.b") == 0,
          "CASE generate alternatives are outside the recovered sequential and selected-choice roots");

    result = analyze(
        "entity probe is end; architecture rtl of probe is begin process(all) begin\n"
        "for i in 1 to 3 loop i := i + 1; end loop;\n"
        "end process; end architecture;\n", true);
    check(count(result, "GJB9765.4.7.b") == 1,
          "4.7.b reports direct assignment to the active FOR loop variable");
    result = analyze(
        "entity probe is end; architecture rtl of probe is begin process(all) variable sum: integer; begin\n"
        "for i in 1 to 3 loop sum := sum + i; end loop;\n"
        "end process; end architecture;\n", true);
    check(count(result, "GJB9765.4.7.b") == 0,
          "reading a loop variable and assigning another variable stays clear");
    result = analyze(
        "entity probe is end; architecture rtl of probe is begin process(all) begin\n"
        "for outer_i in 1 to 3 loop for inner_i in 1 to 2 loop\n"
        "OUTER_I := outer_i + 1; inner_i := inner_i + 1; end loop; end loop;\n"
        "end process; end architecture;\n", true);
    check(count(result, "GJB9765.4.7.b") == 2,
          "nested loops protect both current and outer variables with VHDL case-insensitive names");
    result = analyze(
        "entity probe is end; architecture rtl of probe is begin process(all) variable keep: boolean; begin\n"
        "for i in 1 to 3 loop while keep loop null; end loop; i := i + 1; end loop;\n"
        "end process; end architecture;\n", true);
    check(count(result, "GJB9765.4.7.b") == 1,
          "an inner WHILE loop does not terminate the enclosing FOR protection scope");
    result = analyze(
        "entity probe is end; architecture rtl of probe is begin process(all) begin\n"
        "for i in 1 to 2 loop for i in 1 to 2 loop i := i + 1; end loop; end loop;\n"
        "end process; end architecture;\n", true);
    check(count(result, "GJB9765.4.7.b") == 1,
          "a same-named nested loop assignment is deduplicated by source location");
    result = analyze(
        "entity probe is end; architecture rtl of probe is begin\n"
        "g: for i in 0 to 3 generate b: block begin end block; end generate;\n"
        "end architecture;\n", true);
    check(count(result, "GJB9765.4.7.b") == 0,
          "FOR generate is outside the recovered sequential AST_FOR assignment predicate");

    result = analyze(
        "entity probe is end; architecture rtl of probe is\n"
        "constant c_b : boolean := true; begin process(all) begin\n"
        "while (3 > 1) loop null; end loop; while c_b loop null; end loop;\n"
        "end process; end architecture;\n", true);
    check(count(result, "GJB9765.4.7.c") == 2,
          "4.7.c reports literal comparisons and resolved named constants independently");
    result = analyze(
        "entity probe is port(run: in boolean); end; architecture rtl of probe is begin\n"
        "process(all) variable i: integer := 0; begin while run loop i := i + 1; end loop;\n"
        "while i /= 3 loop i := i + 1; end loop; end process; end architecture;\n", true);
    check(count(result, "GJB9765.4.7.c") == 0,
          "runtime names and mixed runtime/literal comparisons remain clear for 4.7.c");
    result = analyze(
        "entity probe is generic(G_STOP: integer := 3); end; architecture rtl of probe is\n"
        "constant C_ONE: integer := 1; begin process(all) begin\n"
        "while (C_ONE <= G_STOP) loop null; end loop; end process; end architecture;\n", true);
    check(count(result, "GJB9765.4.7.c") == 1,
          "4.7.c treats VHDL constants and generics as static relation operands");
    result = analyze(
        "entity probe is port(run: in boolean); end; architecture rtl of probe is begin\n"
        "process(all) begin if true then null; end if; if 3 > 1 then null; end if;\n"
        "while run loop null; end loop; end process; end architecture;\n", true);
    check(count(result, "GJB9765.4.7.c") == 0,
          "constant IF conditions do not leak into the WHILE-only 4.7.c predicate");
    result = analyze(
        "entity probe is end; architecture rtl of probe is constant C : boolean; begin\n"
        "process(all) begin while C loop null; end loop; end process; end architecture;\n", true);
    check(count(result, "GJB9765.4.7.c") == 0,
          "a deferred constant without a local value is not guessed static");
    result = analyze(
        "entity first is end; architecture rtl of first is constant STOP: boolean := true; begin end;\n"
        "entity second is port(STOP: in boolean); end; architecture rtl of second is begin\n"
        "process(all) begin while STOP loop null; end loop; end process; end architecture;\n", true);
    check(count(result, "GJB9765.4.7.c") == 0,
          "4.7.c does not borrow a same-named constant from another architecture");

    result = analyze(
        "entity probe is port(b: in bit); end; architecture rtl of probe is\n"
        "constant c_a: boolean := true; constant c_num1: integer := 4;\n"
        "constant c_num2: integer := 5; begin process(all) begin\n"
        "if c_a then null; end if; if c_num2 < c_num1 then null;\n"
        "elsif b = '1' then null; end if; end process; end architecture;\n", true);
    check(count(result, "GJB9765.4.7.a") == 2,
          "4.7.a reports direct named constants and constant relations in IF/ELSIF controls");
    result = analyze(
        "entity probe is port(b: in bit); end; architecture rtl of probe is\n"
        "constant c_a: boolean := true; constant c_num1: integer := 4;\n"
        "constant c_num2: integer := 5; begin process(all) begin\n"
        "case c_a is when true => null; when others => null; end case;\n"
        "case (c_num2 >= c_num1) is when true => null; when others => null; end case;\n"
        "case b is when '1' => null; when others => null; end case;\n"
        "end process; end architecture;\n", true);
    check(count(result, "GJB9765.4.7.a") == 2,
          "4.7.a reports constant CASE selectors while a runtime selector stays clear");
    result = analyze(
        "entity probe is port(low_bound, high_bound: in integer); end;\n"
        "architecture rtl of probe is constant C_HIGH: integer := 7; begin process(all) begin\n"
        "for i in low_bound to high_bound loop null; end loop;\n"
        "for j in 0 to C_HIGH loop null; end loop; end process; end architecture;\n", true);
    check(count(result, "GJB9765.4.7.a") == 2,
          "4.7.a reports each direct runtime FOR bound and accepts literal/constant bounds");
    result = analyze(
        "entity probe is generic(G_HIGH: integer := 3); port(run: in boolean); end;\n"
        "architecture rtl of probe is begin process(all) begin\n"
        "while true loop null; end loop; if run then null; end if;\n"
        "for i in 0 to G_HIGH loop null; end loop; end process; end architecture;\n", true);
    check(count(result, "GJB9765.4.7.a") == 0 &&
          count(result, "GJB9765.4.7.c") == 1,
          "4.7.a excludes WHILE constants and accepts generic bounds while 4.7.c remains independent");
    result = analyze(
        "entity probe is port(high_bound: in integer); end; architecture rtl of probe is begin\n"
        "g: for i in 0 to high_bound generate b: block begin end block; end generate;\n"
        "end architecture;\n", true);
    check(count(result, "GJB9765.4.7.a") == 1,
          "4.7.a applies the recovered second loop-root branch to FOR generate bounds");

    result = analyze(
        "entity probe is port(a,b,sel: in bit; q1,q2: out bit); end;\n"
        "architecture rtl of probe is begin\n"
        "p1: process(all) begin if a='1' then q1<=a; elsif b='1' then q1<=b; end if; end process;\n"
        "p2: process(all) begin case sel is when '0'=>q2<=a; when others=>null; end case; end process;\n"
        "end architecture;\n", true);
    check(count(result, "GJB9765.4.2.f") == 2,
          "attachment 4.2.f missing IF ELSE and empty CASE OTHERS each report once");
    result = analyze(
        "entity probe is port(a: in bit; q: out bit); end;\n"
        "architecture rtl of probe is begin process(all) begin\n"
        "if a='1' then null; end if;\n"
        "case a is when '0'=>null; when '1'=>null; end case;\n"
        "end process; end architecture;\n", true);
    check(count(result, "GJB9765.4.2.f") == 2,
          "4.2.f structural IF/CASE omissions report even with no assignment target");
    result = analyze(
        "entity probe is port(a,b: in bit; q1,q2: out bit); end;\n"
        "architecture rtl of probe is begin process(all) begin\n"
        "if a='1' then q1<=a; q2<=b; else q1<=b; end if;\n"
        "case a is when '0'=>q1<=a; q2<=b; when others=>q1<=b; end case;\n"
        "end process; end architecture;\n", true);
    check(count(result, "GJB9765.4.2.f") == 2,
          "4.2.f reports each IF/CASE branch missing a target from the construct union");
    result = analyze(
        "entity probe is port(a,b: in bit; q1,q2,q3: out bit); end;\n"
        "architecture rtl of probe is begin process(all) begin\n"
        "q1 <= a when b='1';\n"
        "q2 <= a when b='1' else q2;\n"
        "q3 <= a when b='1' else b;\n"
        "end process; end architecture;\n", true);
    check(count(result, "GJB9765.4.2.f") == 2,
          "4.2.f reports conditional assignment without ELSE and simple ELSE self-hold");
    result = analyze(
        "entity probe is port(a,b: in bit; q1,q2: out bit); end;\n"
        "architecture rtl of probe is begin process(all) begin\n"
        "if a='1' then q1<=a; q2<=b; else q1<=b; q2<=a; end if;\n"
        "case a is when '0'=>q1<=a; q2<=b; when others=>q1<=b; q2<=a; end case;\n"
        "end process; end architecture;\n", true);
    check(count(result, "GJB9765.4.2.f") == 0,
          "4.2.f accepts complete IF and CASE assignment sets");
    result = analyze(
        "entity probe is port(clk,a: in bit; q: out bit); end;\n"
        "architecture rtl of probe is begin process(clk) begin\n"
        "if rising_edge(clk) then if a='1' then q<=a; end if; end if;\n"
        "end process; end architecture;\n", true);
    check(count(result, "GJB9765.4.2.f") == 0,
          "4.2.f excludes an edge-triggered process from all latch subchecks");

    result = analyze(
        "entity probe is port(a: in bit; q: out bit); end;\n"
        "architecture rtl of probe is begin process(all) begin\n"
        "q <= '0'; if a = '1' then q <= '1'; end if;\n"
        "end process; end architecture;\n", true);
    check(count(result, "GJB9765.4.8.d") == 1,
          "4.8.d reports a missing final else even when a prior default prevents a latch");
    result = analyze(
        "entity probe is port(a,b: in bit; q: out bit); end;\n"
        "architecture rtl of probe is begin process(all) begin\n"
        "if a = '1' then if b = '1' then q <= '1'; end if; else q <= '0'; end if;\n"
        "end process; end architecture;\n", true);
    check(count(result, "GJB9765.4.8.d") == 1 &&
          std::ranges::any_of(result.violations, [](const auto& item) {
              return item.ruleId == "GJB9765.4.8.d" && item.line == 3 && item.column > 1;
          }), "4.8.d evaluates nested IF statements independently and reports only the inner IF");
    result = analyze(
        "entity probe is port(a,b: in bit; q: out bit); end;\n"
        "architecture rtl of probe is begin process(all) begin\n"
        "if a = '1' then q <= '1' when b = '1' else '0'; end if;\n"
        "end process; end architecture;\n", true);
    check(count(result, "GJB9765.4.8.d") == 1,
          "a conditional-assignment else does not satisfy the enclosing IF");
    result = analyze(
        "entity probe is port(clk,a: in bit; q: out bit); end;\n"
        "architecture rtl of probe is begin process(clk) begin\n"
        "if rising_edge(clk) then if a = '1' then q <= '1'; end if; end if;\n"
        "end process; end architecture;\n", true);
    check(count(result, "GJB9765.4.8.d") == 0,
          "4.8.d excludes an entire edge-triggered process, including nested clock-enable IFs");
    result = analyze(
        "entity probe is port(a,b: in bit; q: out bit); end;\n"
        "architecture rtl of probe is begin process(all) begin\n"
        "if a = '1' then q <= '1'; elsif b = '1' then q <= '0'; else q <= a; end if;\n"
        "end process; end architecture;\n", true);
    check(count(result, "GJB9765.4.8.d") == 0,
          "4.8.d accepts an IF/ELSIF chain with a final ELSE");
    result = analyze(
        "entity probe is port(a: in bit; q: out bit); end;\n"
        "architecture rtl of probe is begin process(all) begin\n"
        "case a is when '0' => q <= '0'; end case;\n"
        "end process; end architecture;\n", true);
    check(count(result, "GJB9765.4.8.d") == 0,
          "a CASE without OTHERS is not mislabelled as a missing IF ELSE");
    result = analyze(
        "entity probe is generic(g_enable: boolean := true); port(q: out bit); end;\n"
        "architecture rtl of probe is begin\n"
        "g_if: if g_enable generate q <= '1'; end generate;\n"
        "end architecture;\n", true);
    check(count(result, "GJB9765.4.8.d") == 0,
          "4.8.d excludes an IF-generate root because the native candidate is AST_ALWAYS(0x52)");
    result = analyze(
        "entity probe is generic(g_enable: boolean := true); port(a: in bit; q: out bit); end;\n"
        "architecture rtl of probe is begin\n"
        "g_if: if g_enable generate\n"
        "process(all) begin if a = '1' then q <= '1'; end if; end process;\n"
        "end generate; end architecture;\n", true);
    check(count(result, "GJB9765.4.8.d") == 1,
          "4.8.d recursively reaches a combinational AST_ALWAYS nested below an IF-generate root");
    result = analyze(
        "entity probe is port(a: in bit; q: out bit); end;\n"
        "architecture rtl of probe is\n"
        "procedure choose(signal x: in bit; signal y: out bit) is begin\n"
        "if x = '1' then y <= '1'; end if; end procedure;\n"
        "begin choose(a,q); end architecture;\n", true);
    check(count(result, "GJB9765.4.8.d") == 1,
          "4.8.d checks a source IF lowered below an architecture-declarative procedure root");
    result = analyze(
        "package body helpers is\n"
        "function choose(x: bit) return bit is begin\n"
        "if x = '1' then return '1'; end if; return '0'; end choose;\n"
        "end package body;\n", true);
    check(count(result, "GJB9765.4.8.d") == 1,
          "4.8.d checks a source IF in a package-body function with a named terminator");
    result = analyze(
        "package body helpers is\n"
        "function choose(x: bit) return bit is begin\n"
        "if x = '1' then return '1'; else return '0'; end if; end;\n"
        "end package body;\n", true);
    check(count(result, "GJB9765.4.8.d") == 0,
          "4.8.d accepts a package-body function IF with a final ELSE and bare terminator");
    result = analyze(
        "package body helpers is\n"
        "function \"+\"(l, r: bit) return bit is begin\n"
        "if l = '1' then return r; end if; return '0'; end function \"+\";\n"
        "end package body;\n", true);
    check(count(result, "GJB9765.4.8.d") == 1,
          "4.8.d checks a missing final ELSE in a string-designator operator function");
    result = analyze(
        "package body helpers is\n"
        "function \"and\"(l, r: bit) return bit is begin\n"
        "if l = '1' then return r; else return l; end if; end \"and\";\n"
        "end package body;\n", true);
    check(count(result, "GJB9765.4.8.d") == 0,
          "4.8.d accepts an operator function IF with final ELSE and named string terminator");
    result = analyze(
        "package body helpers is\n"
        "function outer(x: bit) return bit is\n"
        "function inner(y: bit) return bit is begin\n"
        "if y = '1' then return '1'; end if; return '0'; end function;\n"
        "begin if x = '1' then return inner(x); end if; return '0'; end function;\n"
        "end package body;\n", true);
    check(count(result, "GJB9765.4.8.d") == 2,
          "4.8.d checks nested and enclosing subprogram IF bodies independently");

    analysis::AnalysisOptions inoutOptions;
    inoutOptions.standardId = "GJB10157-2021";
    const auto analyzeInoutProject = [&](std::string child, std::string parent) {
        return analysis::Analyzer(inoutOptions).analyzeSources({
            {"child.sv", core::HdlLanguage::SystemVerilog, std::move(child)},
            {"parent.sv", core::HdlLanguage::SystemVerilog, std::move(parent)}});
    };
    result = analyzeInoutProject(
        "module child(input IN1, output OUT1, inout IO1); endmodule\n",
        "module parent(inout P1, inout P2, inout P3);\n"
        "child u0(.IN1(P1), .OUT1(P2), .IO1(P3)); endmodule\n");
    check(count(result, "GJB10157.7.3.3") == 2,
          "7.3.3 reports project-resolved inout actuals bound to input and output formals");
    result = analyzeInoutProject(
        "module child(input IN1, output OUT1, inout IO1); endmodule\n",
        "module parent(inout P1, inout P2, inout P3);\n"
        "child u0(P1, P2, P3); endmodule\n");
    check(count(result, "GJB10157.7.3.3") == 2,
          "7.3.3 applies the recovered direction predicate to positional bindings");
    result = analyzeInoutProject(
        "module child(input IN1, output OUT1, inout IO1); endmodule\n",
        "module parent(input P1, output P2, inout P3);\n"
        "child u0(.IN1(P1), .OUT1(P2), .IO1(P3)); endmodule\n");
    check(count(result, "GJB10157.7.3.3") == 0,
          "7.3.3 accepts matching unidirectional actuals and inout-to-inout binding");
    result = analyzeInoutProject(
        "module child(input IN1); endmodule\n",
        "module parent(inout [1:0] BUS); child u0(.IN1(BUS[0])); endmodule\n");
    check(count(result, "GJB10157.7.3.3") == 1,
          "7.3.3 resolves a selected bit back to its inout port declaration");
    result = analyze(
        "module parent(inout P1, inout P2);\n"
        "unknown_child u0(.IN1(P1), .OUT1(P2)); endmodule\n");
    check(count(result, "GJB10157.7.3.3") == 0,
          "7.3.3 does not guess formal directions for an unresolved library cell");
    check(std::ranges::any_of(result.standardAssessments, [](const auto& e) {
        return e.clauseId == "GJB10157.7.3.3" &&
               e.state == analysis::StandardRuleState::ProjectContextRequired;
    }), "7.3.3 unresolved no-alarm result remains project context required");

    result = analyze(
        "entity utest_GJB_4_2_d_ecode is\n"
        " port(CLK,IN1,IN2: in bit; OUT1,OUT2,OUT3,OUT4: out bit); end;\n"
        "architecture rtl of utest_GJB_4_2_d_ecode is\n"
        " component my_module1 port(CLK,IN1: in bit; OUT1: out bit); end component;\n"
        " component my_bb port(CLK,IN1: in bit; OUT1: out bit); end component;\n"
        "begin\n"
        "u_my_module1: my_module1 port map(CLK,IN1,OUT2);\n"
        "u_my_bb: my_bb port map(CLK,IN1,OUT3);\n"
        "OUT1 <= IN1 or IN2;\n"
        "my_proc: process(CLK) begin if rising_edge(CLK) then OUT4 <= IN1; end if; end process;\n"
        "end architecture;\n", true);
    check(count(result, "GJB9765.4.2.d") == 2,
          "attachment 4.2.d reports top-level OR glue and a direct process");
    check(std::ranges::any_of(result.violations, [](const auto& item) {
              return item.ruleId == "GJB9765.4.2.d" && item.line == 9;
          }) && std::ranges::any_of(result.violations, [](const auto& item) {
              return item.ruleId == "GJB9765.4.2.d" && item.line == 10;
          }), "4.2.d reports the assignment and process nodes separately");
    result = analyze(
        "entity utest_GJB_4_2_d_ccode is\n"
        " port(CLK,IN1: in bit; OUT2,OUT3,OUT4: out bit); end;\n"
        "architecture rtl of utest_GJB_4_2_d_ccode is\n"
        " component my_module1 port(CLK,IN1: in bit; OUT1: out bit); end component;\n"
        " component my_bb port(CLK,IN1: in bit; OUT1: out bit); end component;\n"
        "begin\n"
        "u_my_module1: my_module1 port map(CLK,IN1,OUT2);\n"
        "u_my_bb: my_bb port map(CLK,IN1,OUT3);\n"
        "OUT4 <= IN1;\n"
        "end architecture;\n", true);
    check(count(result, "GJB9765.4.2.d") == 0,
          "attachment 4.2.d permits direct top-level instance wiring and identifier buffering");
    result = analyze(
        "entity probe is port(a,b,s: in bit; q1,q2,q3,q4,q5: out bit); end;\n"
        "architecture rtl of probe is begin\n"
        "q1 <= not a;\n"
        "q2 <= a xor b;\n"
        "q3 <= a + b;\n"
        "q4 <= a when s='1' else b;\n"
        "with s select q5 <= a when '0', b when others;\n"
        "end architecture;\n", true);
    check(count(result, "GJB9765.4.2.d") == 4,
          "4.2.d follows the recovered logical/When operator set and excludes arithmetic nodes");
    result = analyze(
        "entity child is port(a,b: in bit; q: out bit); end;\n"
        "architecture rtl of child is begin q <= a; end architecture;\n"
        "entity probe is port(a,b: in bit; q1,q2: out bit); end;\n"
        "architecture rtl of probe is begin\n"
        "u0: entity work.child port map(a => a, b => b and a, q => q1);\n"
        "u1: entity work.child port map(a => a, b => b, q => q2);\n"
        "end architecture;\n", true);
    check(count(result, "GJB9765.4.2.d") == 1,
          "4.2.d filters every direct entity instantiation for compound port-map actuals");
    result = analyze(
        "entity probe is port(a: in bit; q: out bit); end;\n"
        "architecture rtl of probe is begin\n"
        "g: if true generate p: process(a) begin q <= a or a; end process; end generate;\n"
        "end architecture;\n", true);
    check(count(result, "GJB9765.4.2.d") == 0,
          "4.2.d does not promote a process nested below a generate to a direct architecture child");
    result = analyze(
        "entity probe is port(a,b: in bit; q: out bit); end;\n"
        "architecture rtl of probe is begin\n"
        "assert (a and b) = '1' report \"check\" severity warning;\n"
        "q <= a;\n"
        "end architecture;\n", true);
    check(count(result, "GJB9765.4.2.d") == 0,
          "4.2.d excludes concurrent assertions from the native assignment/instance collector");
    {
        analysis::AnalysisOptions configuredTopOptions;
        configuredTopOptions.standardId = "GJB9765-2020";
        configuredTopOptions.topModule = "TOP_B";
        const auto configured = analysis::Analyzer(configuredTopOptions).analyzeSources({
            {"top_a.vhd", core::HdlLanguage::Vhdl,
             "entity top_a is port(a,b: in bit; q: out bit); end;\n"
             "architecture rtl of top_a is begin q <= a or b; end architecture;\n"},
            {"top_b.vhd", core::HdlLanguage::Vhdl,
             "entity top_b is port(a,b: in bit; q: out bit); end;\n"
             "architecture rtl of top_b is begin q <= a xor b; end architecture;\n"}
        });
        check(configured.errors.empty() && count(configured, "GJB9765.4.2.d") == 1 &&
                  std::ranges::any_of(configured.violations, [](const auto& item) {
                      return item.ruleId == "GJB9765.4.2.d" &&
                             item.file.filename() == "top_b.vhd";
                  }),
              "configured VHDL top uses case-insensitive entity resolution and checks only that architecture");
    }

    result = analyze(
        "entity probe is port(a,b: in bit_vector(1 downto 0); c1,c2,c3: inout bit); end;\n"
        "architecture rtl of probe is signal s: bit_vector(1 downto 0); begin\n"
        "c1 <= a(0) when (a(0)='1') else 'Z';\n"
        "s <= a and b;\n"
        "c2 <= s(0) when (s(0)='1') else 'Z';\n"
        "c3 <= b(0) when (b(0)='1') else 'Z';\n"
        "end architecture;\n", true);
    check(count(result, "GJB9765.4.6.h") == 3,
          "attachment 4.6.h identifies shared data/enable sources regardless of driver origin");
    result = analyze(
        "entity probe is port(a,b: in bit_vector(1 downto 0); c1,c2,c3: inout bit); end;\n"
        "architecture rtl of probe is signal s: bit_vector(1 downto 0); begin\n"
        "c1 <= a(0) when b(0)='1' else 'Z';\n"
        "s <= a and b;\n"
        "c2 <= s(0) when s(1)='1' else 'Z';\n"
        "c3 <= b(0) when b(0)='1' else '0';\n"
        "end architecture;\n", true);
    check(count(result, "GJB9765.4.6.h") == 0,
          "attachment 4.6.h excludes distinct sources, distinct selects and non-Z branches");
    result = analyze(
        "entity probe is port(a: in bit_vector(0 downto 0); c: inout bit); end;\n"
        "architecture rtl of probe is begin\n"
        "c <= a(0) when a=\"1\" else 'Z';\n"
        "end architecture;\n", true);
    check(count(result, "GJB9765.4.6.h") == 1,
          "4.6.h preserves the native base-name match when only the data side has a select");
    result = analyze(
        "entity probe is port(a: in bit_vector(1 downto 0); c: inout bit); end;\n"
        "architecture rtl of probe is begin\n"
        "c <= a(0) when a(1)='1' else 'Z';\n"
        "end architecture;\n", true);
    check(count(result, "GJB9765.4.6.h") == 0,
          "4.6.h still excludes unequal select fields when both sides contain a simple select");

    result = analyze(
        "entity probe is port(clk: in bit; a,a2: in bit_vector(1 downto 0); c1: inout bit); end;\n"
        "architecture rtl of probe is signal r1,r2,comb: bit_vector(1 downto 0); begin\n"
        "p: process(clk) begin if rising_edge(clk) then r1<=a; r2<=a2; end if; end process;\n"
        "comb <= r1 and r2;\n"
        "c1 <= r1(0) when (comb(0)='0') else 'Z';\n"
        "end architecture;\n", true);
    check(count(result, "GJB9765.4.6.i") == 1,
          "attachment 4.6.i reports a concurrent combinational output used as tri-state enable");
    result = analyze(
        "entity probe is port(clk,en,a: in bit; c1,c2,c3: inout bit); end;\n"
        "architecture rtl of probe is component bb port(clk: in bit; q: out bit); end component;\n"
        "signal registered,bb_ctrl: bit; begin\n"
        "p: process(clk) begin if rising_edge(clk) then registered<=en; end if; end process;\n"
        "u0: bb port map(clk=>clk,q=>bb_ctrl);\n"
        "c1 <= a when en='1' else 'Z';\n"
        "c2 <= a when registered='0' else 'Z';\n"
        "c3 <= a when bb_ctrl='1' else 'Z';\n"
        "end architecture;\n", true);
    check(count(result, "GJB9765.4.6.i") == 0,
          "attachment 4.6.i permits input, register and unresolved black-box enable origins");
    result = analyze(
        "entity probe is port(a,b: in bit; c: inout bit); end;\n"
        "architecture rtl of probe is signal logic_en,alias_en: bit; begin\n"
        "logic_en <= a xor b; alias_en <= logic_en;\n"
        "c <= a when alias_en='1' else 'Z';\n"
        "end architecture;\n", true);
    check(count(result, "GJB9765.4.6.i") == 1,
          "4.6.i follows a direct alias to the concurrent combinational origin");
    result = analyze(
        "entity probe is port(a,b: in bit; y: out bit; c: inout bit); end;\n"
        "architecture rtl of probe is signal logic_data,alias_en: bit; begin\n"
        "logic_data <= a and b; alias_en <= a; y <= logic_data;\n"
        "c <= b when alias_en='1' else '0';\n"
        "end architecture;\n", true);
    check(count(result, "GJB9765.4.6.i") == 0,
          "4.6.i excludes unused combinational outputs, direct aliases and non-Z branches");
    result = analyze(
        "library ieee; use ieee.std_logic_1164.all; use ieee.numeric_std.all;\n"
        "entity probe is port(a,b: in signed(0 downto 0); c: inout std_logic); end;\n"
        "architecture rtl of probe is signal logic_en: signed(0 downto 0); begin\n"
        "logic_en <= -a;\n"
        "c <= b(0) when logic_en(0)='1' else 'Z';\n"
        "end architecture;\n", true);
    check(count(result, "GJB9765.4.6.i") == 0,
          "4.6.i excludes unary NEG because native AST_NEG 0x44 is outside the admitted root set");
    result = analyze(
        "library ieee; use ieee.std_logic_1164.all; use ieee.numeric_std.all;\n"
        "entity probe is port(a,b: in signed(0 downto 0); c: inout std_logic); end;\n"
        "architecture rtl of probe is signal logic_en: signed(0 downto 0); begin\n"
        "logic_en <= a + b;\n"
        "c <= b(0) when logic_en(0)='1' else 'Z';\n"
        "end architecture;\n", true);
    check(count(result, "GJB9765.4.6.i") == 1,
          "4.6.i retains binary ADD because native AST_ADD 0x3c is an admitted combinational root");

    result = analyze(
        "entity probe is port(clk: in bit); end entity;\n"
        "architecture rtl of probe is signal s: integer;\n"
        "constant Cint1: integer := 1;\n"
        "constant Cint2: integer := s + 1; begin end architecture;\n", true);
    check(count(result, "GJB9765.4.6.c") == 1,
          "attachment 4.6.c reports a constant initializer that depends on a signal");
    result = analyze(
        "entity probe is port(runtime_bool: in boolean); end entity;\n"
        "architecture rtl of probe is\n"
        "constant Cbool: boolean := runtime_bool; begin end architecture;\n", true);
    check(count(result, "GJB9765.4.6.c") == 0,
          "4.6.c admits a direct boolean initializer through the native +0x10e type flag");
    result = analyze(
        "entity probe is port(runtime_value: in integer); end entity;\n"
        "architecture rtl of probe is\n"
        "constant Crel: boolean := runtime_value > 0; begin end architecture;\n", true);
    check(count(result, "GJB9765.4.6.c") == 0,
          "4.6.c exempts a boolean-valued relation before identifier dependency checks");
    result = analyze(
        "entity probe is end entity; architecture rtl of probe is\n"
        "signal runtime_value, target: integer;\n"
        "constant Cbad: integer := runtime_value + 1; begin\n"
        "target <= Cbad; end architecture;\n", true);
    check(count(result, "GJB9765.4.6.c") == 2,
          "4.6.c revisits an integer signal assignment that uses a rejected constant");
    result = analyze(
        "entity probe is end entity; architecture rtl of probe is\n"
        "signal runtime_value, target: integer;\n"
        "constant Cbad: integer := runtime_value + 1; begin\n"
        "target <= Cbad + Cbad; end architecture;\n", true);
    check(count(result, "GJB9765.4.6.c") == 3,
          "4.6.c preserves one assignment diagnostic per rejected RHS identifier occurrence");
    result = analyze(
        "entity probe is end entity; architecture rtl of probe is\n"
        "signal runtime_value: integer; signal target: bit;\n"
        "constant Cbad: integer := runtime_value + 1; begin\n"
        "target <= Cbad; end architecture;\n", true);
    check(count(result, "GJB9765.4.6.c") == 1,
          "4.6.c applies the native +0x109 integer gate to assignment targets");
    result = analyze(
        "entity probe is end entity; architecture rtl of probe is\n"
        "signal runtime_value: integer;\n"
        "constant Cbad: integer := runtime_value + 1; begin\n"
        "process variable target: integer; begin target := Cbad; end process;\n"
        "end architecture;\n", true);
    check(count(result, "GJB9765.4.6.c") == 1,
          "4.6.c excludes variable := assignments from the AST_ASSIGN_LE branch");
    result = analyze(
        "entity probe is end entity; architecture rtl of probe is\n"
        "signal target: integer; constant Cunknown: integer := external_value + 1; begin\n"
        "target <= Cunknown; end architecture;\n", true);
    check(count(result, "GJB9765.4.6.c") == 0,
          "4.6.c does not turn an unresolved constant into a rejected assignment dependency");
    result = analyze(
        "entity probe is end entity; architecture rtl of probe is\n"
        "signal runtime_value: integer; constant Cbad: integer := runtime_value + 1;\n"
        "signal target: integer; begin target <= Cbad; end architecture;\n", true);
    check(count(result, "GJB9765.4.6.c") == 1,
          "4.6.c preserves the native physical-line filter for declaration-line assignments");
    result = analyze(
        "package config_pkg is constant Cdeferred: integer;\n"
        "constant Cderived: integer := Cdeferred + 1; end package;\n", true);
    check(count(result, "GJB9765.4.6.c") == 0,
          "4.6.c admits a zero-child deferred constant and a later dependency on it");
    result = analyze(
        "entity probe is end entity; architecture rtl of probe is\n"
        "constant Cdeferred: integer; signal target: integer; begin\n"
        "target <= Cdeferred; end architecture;\n", true);
    check(count(result, "GJB9765.4.6.c") == 0,
          "4.6.c keeps an admitted deferred constant out of the rejected assignment branch");
    result = analyze(
        "package config_pkg is constant Cdeferred: integer; end package;\n"
        "package body config_pkg is signal runtime_value: integer;\n"
        "constant Cdeferred: integer := runtime_value + 1; end package body;\n", true);
    check(count(result, "GJB9765.4.6.c") == 1,
          "4.6.c re-evaluates a deferred constant completion in its package-body root");
    result = analyze(
        "entity probe is generic(WIDTH: integer := 4); port(clk: in bit); end entity;\n"
        "architecture rtl of probe is type state_t is (IDLE, RUN);\n"
        "constant Cint1: integer := 1;\n"
        "constant Cint2: integer := Cint1 + WIDTH;\n"
        "constant Cstate: state_t := IDLE; begin end architecture;\n", true);
    check(count(result, "GJB9765.4.6.c") == 0,
          "4.6.c admits literal, prior constant, bound generic and enumeration-literal dependencies");
    result = analyze(
        "entity probe is port(clk: in bit); end entity;\n"
        "architecture rtl of probe is signal runtime_value: integer;\n"
        "constant Cbad: integer := runtime_value + 1;\n"
        "constant Cworse: integer := Cbad + 1; begin end architecture;\n", true);
    check(count(result, "GJB9765.4.6.c") == 2,
          "4.6.c propagates a rejected constant dependency to later constants");
    result = analyze(
        "entity probe is port(clk: in bit); end entity;\n"
        "architecture rtl of probe is begin\n"
        "process variable runtime_value: integer;\n"
        "constant Cbad: integer := runtime_value + 1; begin null; end process;\n"
        "end architecture;\n", true);
    check(count(result, "GJB9765.4.6.c") == 1,
          "4.6.c recursively checks process-local constants in the complete architecture root");
    result = analyze(
        "entity probe is end entity;\n"
        "architecture rtl of probe is\n"
        "procedure p is variable runtime_value: integer;\n"
        "constant Cbad: integer := runtime_value + 1; begin null; end procedure;\n"
        "begin p; end architecture;\n", true);
    check(count(result, "GJB9765.4.6.c") == 1,
          "4.6.c recursively checks subprogram-local constants in the architecture root");
    result = analyze(
        "entity probe is end entity; architecture rtl of probe is begin\n"
        "guard: block signal runtime_value: integer;\n"
        "constant Cbad: integer := runtime_value + 1; begin end block;\n"
        "end architecture;\n", true);
    check(count(result, "GJB9765.4.6.c") == 1,
          "4.6.c recursively checks block-local constants in the architecture root");
    result = analyze(
        "entity first is end entity; architecture rtl of first is\n"
        "signal runtime_value: integer; constant Cbad: integer := runtime_value + 1;\n"
        "begin end architecture;\n"
        "entity second is end entity; architecture rtl of second is\n"
        "constant Cgood: integer := 1; begin end architecture;\n", true);
    check(count(result, "GJB9765.4.6.c") == 1,
          "4.6.c isolates admitted and rejected names at each architecture terminator");
    result = analyze(
        "entity first is end; architecture rtl of first is\n"
        "constant Cunknown: integer := later_port + 1; begin end;\n"
        "entity second is port(signal later_port: in integer); end;\n"
        "architecture rtl of second is begin end;\n", true);
    check(count(result, "GJB9765.4.6.c") == 0,
          "4.6.c stops a bare-ended architecture before the next entity interface");
    result = analyze(
        "entity probe is end; architecture rtl of probe is\n"
        "procedure p is variable runtime_value: integer;\n"
        "constant Cbad: integer := runtime_value + 1; begin null; end;\n"
        "begin end;\n", true);
    check(count(result, "GJB9765.4.6.c") == 1,
          "4.6.c distinguishes a bare subprogram end from the enclosing bare architecture end");
    result = analyze(
        "entity probe is end; architecture rtl of probe is\n"
        "procedure rtl is begin null; end rtl;\n"
        "signal runtime_value: integer;\n"
        "constant Cbad: integer := runtime_value + 1; begin end rtl;\n", true);
    check(count(result, "GJB9765.4.6.c") == 1,
          "4.6.c does not confuse a same-named subprogram terminator with the architecture terminator");
    result = analyze(
        "entity probe is end; architecture rtl of probe is\n"
        "package local_values is constant Cone: integer := 1; end;\n"
        "signal runtime_value: integer;\n"
        "constant Cbad: integer := runtime_value + 1; begin end;\n", true);
    check(count(result, "GJB9765.4.6.c") == 1,
          "4.6.c distinguishes a local package bare end from the enclosing architecture end");
    result = analyze(
        "package config_pkg is signal runtime_value: integer;\n"
        "constant Cbad: integer := runtime_value + 1; end package;\n", true);
    check(count(result, "GJB9765.4.6.c") == 1,
          "4.6.c checks constant dependencies in an independent package specification root");
    result = analyze(
        "package config_pkg is procedure p; end;\n"
        "package body config_pkg is procedure p is\n"
        "variable runtime_value: integer;\n"
        "constant Cbad: integer := runtime_value + 1; begin null; end;\n"
        "end package body;\n", true);
    check(count(result, "GJB9765.4.6.c") == 1,
          "4.6.c checks subprogram-local constants in an independent package-body root");
    result = analyze(
        "package config_pkg is end; package body config_pkg is\n"
        "constant Cone: integer := 1;\n"
        "constant Ctwo: integer := Cone + 1; end config_pkg;\n", true);
    check(count(result, "GJB9765.4.6.c") == 0,
          "4.6.c admits a static constant chain in a named package-body root");
    result = analyze(
        "package config_pkg is signal runtime_value: integer;\n"
        "constant Cbad: integer := runtime_value + 1; end;\n"
        "entity probe is end; architecture rtl of probe is\n"
        "constant Cunknown: integer := Cbad + 1; begin end;\n", true);
    check(count(result, "GJB9765.4.6.c") == 1,
          "4.6.c resets accepted and rejected names between package and architecture roots");
    result = analyze(
        "entity probe is end entity; architecture rtl of probe is begin\n"
        "process constant Cone: integer := 1;\n"
        "constant Ctwo: integer := Cone + 1; begin null; end process;\n"
        "end architecture;\n", true);
    check(count(result, "GJB9765.4.6.c") == 0,
          "4.6.c admits a process-local constant that depends on an earlier static constant");
    result = analyze(
        "entity probe is end entity;\n"
        "architecture rtl of probe is\n"
        "constant Cpkg: integer := work.config_pkg.VALUE;\n"
        "constant Cfun: integer := user_static_function(1); begin end architecture;\n", true);
    const auto unresolvedConstant = std::ranges::find_if(result.standardAssessments,
        [](const auto& assessment) { return assessment.clauseId == "GJB9765.4.6.c"; });
    check(count(result, "GJB9765.4.6.c") == 0 &&
          unresolvedConstant != result.standardAssessments.end() &&
          unresolvedConstant->state == analysis::StandardRuleState::ManualReviewRequired,
          "4.6.c leaves unresolved package/function constant expressions under manual review");

    result = analyze(
        "library ieee; use ieee.std_logic_1164.all; use ieee.numeric_std.all;\n"
        "entity probe is end entity; architecture rtl of probe is\n"
        "signal a1: unsigned std_logic_vector(7 downto 0);\n"
        "signal a2: integer std_logic_vector(7 downto 0); begin\n"
        "a1 <= a2; end architecture;\n", true);
    check(count(result, "GJB9765.4.6.d") == 1,
          "attachment 4.6.d reports an integer-to-unsigned direct assignment");
    result = analyze(
        "library ieee; use ieee.numeric_std.all; entity probe is end entity;\n"
        "architecture rtl of probe is signal u: unsigned(7 downto 0);\n"
        "constant Cint: integer := u; begin end architecture;\n", true);
    check(count(result, "GJB9765.4.6.d") == 1,
          "4.6.d checks declaration initializers lowered to native AST_VARASGN nodes");
    result = analyze(
        "library ieee; use ieee.numeric_std.all; package values_pkg is\n"
        "constant U: unsigned(7 downto 0) := (others => '0');\n"
        "constant I: integer := U; end package;\n", true);
    check(count(result, "GJB9765.4.6.d") == 1,
          "4.6.d checks implicit conversion initializers in an independent package root");
    result = analyze(
        "library ieee; use ieee.numeric_std.all; package body values_pkg is\n"
        "procedure p is variable u: unsigned(7 downto 0); variable i: integer;\n"
        "begin i := u; end procedure; end package body;\n", true);
    check(count(result, "GJB9765.4.6.d") == 1,
          "4.6.d recursively checks variable assignments in an independent package body");
    result = analyze(
        "library ieee; use ieee.numeric_std.all; package values_pkg is\n"
        "constant U: unsigned(7 downto 0) := (others => '0');\n"
        "constant I: integer := integer(U); end package;\n", true);
    check(count(result, "GJB9765.4.6.d") == 0,
          "4.6.d keeps a complete explicit conversion opaque in a package initializer");
    result = analyze(
        "library ieee; use ieee.numeric_std.all; entity probe is end entity;\n"
        "architecture rtl of probe is signal i: integer; signal r: real;\n"
        "signal u: unsigned(7 downto 0); signal s: signed(7 downto 0); begin\n"
        "i <= r; r <= i; i <= u; u <= i; r <= u; u <= r;\n"
        "i <= s; s <= i; u <= s; s <= u;\n"
        "r <= s; s <= r; end architecture;\n", true);
    check(count(result, "GJB9765.4.6.d") == 10,
          "4.6.d follows all ten native directed type-pair branches and excludes signed-real");
    result = analyze(
        "library ieee; use ieee.numeric_std.all; entity probe is end entity;\n"
        "architecture rtl of probe is signal i1,i2: integer; signal u1,u2: unsigned(7 downto 0); begin\n"
        "i1 <= i2; u1 <= u2; u1 <= unsigned(i1); end architecture;\n", true);
    const auto implicitConversionReview = std::ranges::find_if(result.standardAssessments,
        [](const auto& assessment) { return assessment.clauseId == "GJB9765.4.6.d"; });
    check(count(result, "GJB9765.4.6.d") == 0 &&
          implicitConversionReview != result.standardAssessments.end() &&
          implicitConversionReview->state == analysis::StandardRuleState::ManualReviewRequired,
          "4.6.d accepts same-type assignments and leaves explicit cast lowering under review");
    result = analyze(
        "library ieee; use ieee.numeric_std.all; entity first is end entity;\n"
        "architecture rtl of first is signal u: unsigned(7 downto 0); begin null; "
        "end architecture;\n"
        "package values_pkg is constant U: unsigned(7 downto 0) := (others => '0');\n"
        "constant I: integer := U; end package;\n", true);
    check(count(result, "GJB9765.4.6.d") == 1,
          "4.6.d starts a fresh package root after an explicitly ended architecture");
    result = analyze(
        "library ieee; use ieee.numeric_std.all; entity first is end;\n"
        "architecture rtl of first is procedure helper is variable x: integer;\n"
        "begin x := x; end; begin null; end rtl;\n"
        "package body values_pkg is variable U: unsigned(7 downto 0); variable I: integer;\n"
        "begin I := U; end values_pkg;\n", true);
    check(count(result, "GJB9765.4.6.d") == 1,
          "4.6.d distinguishes a bare subprogram end from a named architecture end");
    result = analyze(
        "library ieee; use ieee.numeric_std.all; entity first is end;\n"
        "architecture rtl of first is signal shared_value: unsigned(7 downto 0); "
        "begin null; end;\n"
        "entity second is end; architecture rtl of second is\n"
        "signal shared_value: integer; signal target: integer;\n"
        "begin target <= shared_value; end architecture;\n", true);
    check(count(result, "GJB9765.4.6.d") == 0,
          "4.6.d resets declaration types across interleaved entity/architecture roots");

    result = analyze(
        "library ieee; use ieee.std_logic_1164.all; use ieee.std_logic_arith.all;\n"
        "entity probe is end entity; architecture rtl of probe is\n"
        "signal a1: unsigned std_logic_vector(7 downto 0);\n"
        "signal a2: signed std_logic_vector(7 downto 0);\n"
        "signal a3: std_logic_vector(7 downto 0); begin\n"
        "a3 <= a1 + a2; end architecture;\n", true);
    check(count(result, "GJB9765.4.6.e") == 1,
          "attachment 4.6.e reports signed and unsigned names in one addition");
    result = analyze(
        "library ieee; use ieee.numeric_std.all; entity probe is end entity;\n"
        "architecture rtl of probe is signal s: signed(7 downto 0);\n"
        "signal u: unsigned(7 downto 0); signal q: signed(7 downto 0); begin\n"
        "q <= (s + u) * (s and u); end architecture;\n", true);
    check(count(result, "GJB9765.4.6.e") == 3,
          "4.6.e follows nested native operator subtrees and reports each mixed node");
    result = analyze(
        "library ieee; use ieee.numeric_std.all; entity probe is end entity;\n"
        "architecture rtl of probe is signal s: signed(7 downto 0);\n"
        "signal u: unsigned(7 downto 0); signal q: boolean; begin process(s,u) begin\n"
        "if s = u then q <= true; end if; end process; end architecture;\n", true);
    check(count(result, "GJB9765.4.6.e") == 1,
          "4.6.e includes the six native relation-node classes in control expressions");
    result = analyze(
        "library ieee; use ieee.numeric_std.all; entity probe is end entity;\n"
        "architecture rtl of probe is signal s,s2: signed(7 downto 0);\n"
        "signal u,u2: unsigned(7 downto 0); signal flag: boolean;\n"
        "signal q: signed(7 downto 0); begin\n"
        "q <= s + s2 when flag else signed(u + u2);\n"
        "q <= s mod u; q <= s rem u; q <= s ** u; end architecture;\n", true);
    const auto mixedSignednessReview = std::ranges::find_if(result.standardAssessments,
        [](const auto& assessment) { return assessment.clauseId == "GJB9765.4.6.e"; });
    check(count(result, "GJB9765.4.6.e") == 0 &&
          mixedSignednessReview != result.standardAssessments.end() &&
          mixedSignednessReview->state == analysis::StandardRuleState::ManualReviewRequired,
          "4.6.e keeps conditional branches separate and excludes mod/rem/power from the native mask");
    result = analyze(
        "library ieee; use ieee.numeric_std.all; package values_pkg is\n"
        "constant S: signed(7 downto 0) := (others => '0');\n"
        "constant U: unsigned(7 downto 0) := (others => '0');\n"
        "constant Q: signed(7 downto 0) := S + U; end package;\n", true);
    check(count(result, "GJB9765.4.6.e") == 1,
          "4.6.e checks a mixed signed/unsigned initializer in an independent package root");
    result = analyze(
        "library ieee; use ieee.numeric_std.all; package body values_pkg is\n"
        "procedure p is variable s,q: signed(7 downto 0);\n"
        "variable u: unsigned(7 downto 0); begin q := s and u; end procedure;\n"
        "end package body;\n", true);
    check(count(result, "GJB9765.4.6.e") == 1,
          "4.6.e recursively checks package-body subprogram expression roots");
    result = analyze(
        "library ieee; use ieee.numeric_std.all; package values_pkg is\n"
        "constant shared_value: signed(7 downto 0) := (others => '0'); end;\n"
        "entity probe is end; architecture rtl of probe is\n"
        "signal shared_value,u,q: unsigned(7 downto 0);\n"
        "begin q <= shared_value + u; end architecture;\n", true);
    check(count(result, "GJB9765.4.6.e") == 0,
          "4.6.e resets signed/unsigned declaration maps between package and architecture roots");

    result = analyze(
        "entity top is port(clk,rst,rst2,a: in bit; q: out bit); end;\n"
        "architecture rtl of top is\n"
        "component child port(clk,rst,a: in bit; q: out bit); end component;\n"
        "signal bridge: bit; begin\n"
        "u0: child port map(clk => clk, rst => rst2, a => a, q => bridge);\n"
        "end architecture;\n", true);
    check(count(result, "GJB9765.4.5.g") == 2,
          "attachment 4.5.g reports mismatched direct formal/actual names");
    result = analyze(
        "entity top is port(clk,rst,a: in bit; q: out bit); end;\n"
        "architecture rtl of top is\n"
        "component child port(clk,rst,a: in bit; q: out bit); end component;\n"
        "begin u0: child port map(clk => CLK, rst => rst, a => a, q => q);\n"
        "end architecture;\n", true);
    check(count(result, "GJB9765.4.5.g") == 0,
          "4.5.g compares direct mapped names case-insensitively");
    result = analyze(
        "entity top is port(a,b: in bit; q: out bit); end;\n"
        "architecture rtl of top is\n"
        "component child port(a: in bit; q: out bit); end component;\n"
        "begin u0: child port map(a => a and b, q => open);\n"
        "end architecture;\n", true);
    check(count(result, "GJB9765.4.5.g") == 0,
          "4.5.g leaves expression actuals outside the recovered direct-name predicate");
    result = analyze(
        "entity top is port(clk,rst,rst2,a: in bit; q: out bit); end;\n"
        "architecture rtl of top is\n"
        "component child port(clk,rst,a: in bit; q: out bit); end component;\n"
        "signal bridge: bit; begin\n"
        "u0: child port map(clk, rst2, a, bridge);\n"
        "end architecture;\n", true);
    check(count(result, "GJB9765.4.5.g") == 2,
          "4.5.g native positional identifier branch pairs actuals with formal ordinals");
    result = analyze(
        "entity top is port(clk,rst,a: in bit; q: out bit); end;\n"
        "architecture rtl of top is\n"
        "component child port(clk,rst,a: in bit; q: out bit); end component;\n"
        "begin u0: child port map(CLK, rst, a, q); end architecture;\n", true);
    check(count(result, "GJB9765.4.5.g") == 0,
          "4.5.g positional direct names compare case-insensitively");
    result = analyze(
        "entity top is port(a,b: in bit; q: out bit); end;\n"
        "architecture rtl of top is\n"
        "component child port(a,b: in bit; q: out bit); end component;\n"
        "begin u0: child port map(a and b, open, q); end architecture;\n", true);
    check(count(result, "GJB9765.4.5.g") == 0,
          "4.5.g positional expression and OPEN actuals remain outside the direct-name branch");
    analysis::AnalysisOptions splitHierarchyNameOptions;
    splitHierarchyNameOptions.standardId = "GJB9765-2020";
    const auto splitHierarchyNames = analysis::Analyzer(splitHierarchyNameOptions).analyzeSources({
        {"child_entity.vhd", core::HdlLanguage::Vhdl,
         "entity child is port(clk, rst, a: in bit; q: out bit); end;\n"},
        {"top_with_split_child.vhd", core::HdlLanguage::Vhdl,
         "entity top is port(clk, bad_reset, a: in bit; q: out bit); end;\n"
         "architecture rtl of top is begin\n"
         "u_named: entity work.child port map(clk => CLK, rst => bad_reset, a => a, q => q);\n"
         "u_positional: entity work.child port map(CLK, bad_reset, a, q);\n"
         "end architecture;\n"}
    });
    check(count(splitHierarchyNames, "GJB9765.4.5.g") == 2,
          "4.5.g resolves named and positional connections to a uniquely bound entity in another file");
    check(std::ranges::all_of(splitHierarchyNames.violations, [](const auto& item) {
        return item.ruleId != "GJB9765.4.5.g" ||
               item.file.filename() == "top_with_split_child.vhd";
    }), "4.5.g locates split-target connection mismatches at the parent actuals");
    const auto ambiguousHierarchyNames =
        analysis::Analyzer(splitHierarchyNameOptions).analyzeSources({
            {"child_a.vhd", core::HdlLanguage::Vhdl,
             "entity duplicate_child is port(clk, rst: in bit); end;\n"},
            {"child_b.vhd", core::HdlLanguage::Vhdl,
             "entity duplicate_child is port(clock, reset: in bit); end;\n"},
            {"ambiguous_parent.vhd", core::HdlLanguage::Vhdl,
             "entity parent is port(clk, bad_reset: in bit); end;\n"
             "architecture rtl of parent is begin\n"
             "u0: entity work.duplicate_child port map(clk => clk, rst => bad_reset);\n"
             "end architecture;\n"}
        });
    check(count(ambiguousHierarchyNames, "GJB9765.4.5.g") == 0,
          "4.5.g does not guess formal names when a cross-file entity target is ambiguous");
    check(std::ranges::any_of(ambiguousHierarchyNames.standardAssessments, [](const auto& e) {
        return e.clauseId == "GJB9765.4.5.g" &&
               e.state == analysis::StandardRuleState::ManualReviewRequired;
    }), "4.5.g ambiguous cross-file binding remains manual review rather than compliant");

    result = analyze(
        "library ieee; use ieee.std_logic_1164.all;\n"
        "entity probe is\n"
        " generic(g: std_logic_vector := \"0101\");\n"
        " port(a: in std_logic_vector(7 downto 0); q: out std_logic_vector(7 downto 0));\n"
        "end;\n"
        "architecture rtl of probe is\n"
        " constant c: std_logic_vector := \"00000000\";\n"
        " signal s: std_logic_vector;\n"
        " type t is array (0 to 1) of std_logic_vector;\n"
        "begin\n"
        " process(all) variable v: std_logic_vector; begin q <= a(0); end process;\n"
        "end architecture;\n", true);
    check(count(result, "GJB9765.4.4.a") == 5,
          "attachment 4.4.a reports unconstrained generic, constant, signal, array element and variable subtypes");
    result = analyze(
        "entity probe is generic(g: std_logic_vector(3 downto 0) := \"0101\");\n"
        " port(a: in std_logic_vector(7 downto 0); q: out std_logic_vector(7 downto 0)); end;\n"
        "architecture rtl of probe is\n"
        " constant c: std_logic_vector(7 downto 0) := (others => '0');\n"
        " signal s: std_logic_vector(0 to 0);\n"
        " type t is array (0 to 1) of std_logic_vector(7 downto 0);\n"
        "begin end architecture;\n", true);
    check(count(result, "GJB9765.4.4.a") == 0,
          "4.4.a accepts every constrained std_logic_vector declaration and array element subtype");
    result = analyze(
        "entity probe is port(a: in std_logic_vector(1 downto 0); q: out std_logic_vector(1 downto 0)); end;\n"
        "architecture rtl of probe is begin\n"
        "q <= std_logic_vector(a); -- std_logic_vector without range in conversion and comment\n"
        "end architecture;\n", true);
    check(count(result, "GJB9765.4.4.a") == 0,
          "4.4.a does not misread a constrained type conversion or comment text as a declaration");

    result = analyze(
        "entity probe is port(CLK,RST,IN1: in bit; B,C1: out bit); end;\n"
        "architecture rtl of probe is signal async_s: bit; begin\n"
        "p1: process(RST,CLK) begin\n"
        " if RST='1' then B<='0'; elsif RST='0' then B<='1'; elsif rising_edge(CLK) then B<=IN1; end if;\n"
        "end process;\n"
        "async_s <= IN1 and RST;\n"
        "p2: process(async_s,CLK) begin\n"
        " if async_s='1' then C1<='0'; elsif async_s='0' then C1<='1'; elsif rising_edge(CLK) then C1<=IN1; end if;\n"
        "end process; end architecture;\n", true);
    check(count(result, "GJB9765.4.6.g") == 2 && count(result, "GJB9765.4.10.b") == 2,
          "attachment 4.6.g/4.10.b reports shared port and combinational async controls");
    result = analyze(
        "entity probe is port(CLK,RST,SET,IN1: in bit; B,C1: out bit); end;\n"
        "architecture rtl of probe is signal async1,async2: bit; begin\n"
        "p1: process(RST,SET,CLK) begin\n"
        " if RST='1' then B<='0'; elsif SET='1' then B<='1'; elsif rising_edge(CLK) then B<=IN1; end if;\n"
        "end process;\n"
        "async1 <= IN1 and RST; async2 <= IN1 or SET;\n"
        "p2: process(async1,async2,CLK) begin\n"
        " if async1='1' then C1<='0'; elsif async2='1' then C1<='1'; elsif rising_edge(CLK) then C1<=IN1; end if;\n"
        "end process; end architecture;\n", true);
    check(count(result, "GJB9765.4.6.g") == 0 && count(result, "GJB9765.4.10.b") == 0,
          "distinct reset and set controls remain compliant for both shared branches");
    result = analyze(
        "entity probe is port(CLK,RST,A,B: in bit; Q,R: out bit); end;\n"
        "architecture rtl of probe is begin p: process(RST,CLK) begin\n"
        "if RST='1' then Q<='0'; elsif RST='0' then R<='1'; elsif rising_edge(CLK) then Q<=A; R<=B; end if;\n"
        "end process; end architecture;\n", true);
    check(count(result, "GJB9765.4.6.g") == 0 && count(result, "GJB9765.4.10.b") == 0,
          "one shared control across different register targets is not promoted to the per-register predicate");
    result = analyze(
        "entity probe is port(CLK,RST,A: in bit; Q: out bit); end;\n"
        "architecture rtl of probe is begin p: process(RST,CLK) begin\n"
        "if RST='1' then Q<='0'; elsif RST='0' then Q<='0'; "
        "elsif rising_edge(CLK) then Q<=A; end if;\n"
        "end process; end architecture;\n", true);
    check(count(result, "GJB9765.4.6.g") == 0 && count(result, "GJB9765.4.10.b") == 0,
          "one shared control performing reset in both branches is not misclassified as reset and set");
    result = analyze(
        "entity probe is port(CLK,RST,A: in bit; Q: out bit); end;\n"
        "architecture rtl of probe is begin p: process(RST,CLK) begin\n"
        "if RST='1' then Q<=A; elsif RST='0' then Q<='1'; "
        "elsif rising_edge(CLK) then Q<=A; end if;\n"
        "end process; end architecture;\n", true);
    check(count(result, "GJB9765.4.6.g") == 0 && count(result, "GJB9765.4.10.b") == 0,
          "dynamic asynchronous data is not guessed to be the missing reset action");
    result = analyze(
        "entity probe is port(CLK,RST,A: in bit; Q: out bit); end;\n"
        "architecture rtl of probe is begin p: process(RST,CLK) begin\n"
        "if '1'=RST then Q<='0'; elsif '0'=RST then Q<='1'; "
        "elsif rising_edge(CLK) then Q<=A; end if;\n"
        "end process; end architecture;\n", true);
    check(count(result, "GJB9765.4.6.g") == 1 && count(result, "GJB9765.4.10.b") == 1,
          "literal-left guards preserve the direct shared reset and set source predicate");
    return failures == 0 ? 0 : 1;
}
