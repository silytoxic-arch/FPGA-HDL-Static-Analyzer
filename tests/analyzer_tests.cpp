#include "analysis/analyzer.h"
#include "core/source.h"
#include "report/report_writer.h"

#include <algorithm>
#include <iostream>
#include <sstream>

namespace {
class TestContext {
public:
    void check(bool condition, const char* message) {
        if (!condition) { std::cerr << "FAIL: " << message << '\n'; ++failures_; }
    }
    [[nodiscard]] int result() const noexcept { return failures_ == 0 ? 0 : 1; }
private:
    int failures_{0};
};

bool hasRule(const fpga::analysis::AnalysisResult& result, std::string_view id,
             std::size_t line = 0) {
    for (const auto& violation : result.violations) {
        if (violation.ruleId == id && (line == 0 || violation.line == line)) return true;
    }
    return false;
}

std::size_t countRule(const fpga::analysis::AnalysisResult& result, std::string_view id) {
    return static_cast<std::size_t>(std::ranges::count_if(
        result.violations, [&](const auto& violation) { return violation.ruleId == id; }));
}

const fpga::parser::DesignUnitInfo* findUnit(const fpga::analysis::AnalysisResult& result,
                                             std::string_view name) {
    for (const auto& unit : result.structure) if (unit.name == name) return &unit;
    return nullptr;
}
} // namespace

int main() {
    using namespace fpga;
    TestContext test;
    const analysis::Analyzer analyzer;

    test.check(analysis::Analyzer::languageFromPath("top.v") == core::HdlLanguage::Verilog,
               "Verilog extension is recognized");
    test.check(analysis::Analyzer::languageFromPath("TOP.SV") == core::HdlLanguage::SystemVerilog,
               "SystemVerilog extension is case insensitive");
    test.check(analysis::Analyzer::languageFromPath("core.vhdl") == core::HdlLanguage::Vhdl,
               "VHDL extension is recognized");

    const core::SourceFile verilog("rtl/risky.sv", core::HdlLanguage::SystemVerilog,
R"(module risky(input logic clk, input logic d, output logic q);
always @(posedge clk) begin
  q = d;
end
always @(*) begin
  q <= d;
end
always_comb begin
  casex (q)
    1'b0: q = d;
  endcase
end
endmodule
)");
    const auto verilogResult = analyzer.analyzeSource(verilog);
    test.check(verilogResult.filesAnalyzed == 1, "one Verilog source is analyzed");
    test.check(verilogResult.designUnits == 1, "Verilog module is counted");
    test.check(hasRule(verilogResult, "VL-SEQ-BLOCKING", 3),
               "blocking assignment in sequential logic is located");
    test.check(hasRule(verilogResult, "VL-COMB-NONBLOCKING", 6),
               "nonblocking assignment in combinational logic is located");
    test.check(hasRule(verilogResult, "VL-CASEX", 9), "casex risk is reported");
    test.check(hasRule(verilogResult, "VL-CASE-DEFAULT", 9),
               "missing case default is reported at case start");

    const core::SourceFile systemVerilogProcessControls(
        "rtl/systemverilog_process_controls.sv", core::HdlLanguage::SystemVerilog,
R"(module systemverilog_process_controls(
  input logic clk, input logic clk2, input logic a,
  output logic q_comb, output logic q_missing, output logic q_multiple, output logic q_ok,
  output logic q_latch, output logic q_latch_ok, output logic q_no_latch, q_nonedge, q_wildcard);
always_comb @(a) q_comb = a;
always_ff begin q_missing <= a; end
always_ff @(posedge clk) @(posedge clk2) q_multiple <= a;
always_ff @(posedge clk) q_ok <= a;
always_latch @(a) if (a) q_latch = 1'b1;
always_latch if (a) q_latch_ok = 1'b1;
always_latch if (a) q_no_latch = 1'b1; else q_no_latch = 1'b0;
always_ff @(clk) q_nonedge <= a;
always_ff @(*) q_wildcard <= a;
endmodule
)");
    const auto systemVerilogProcessControlResult =
        analyzer.analyzeSource(systemVerilogProcessControls);
    test.check(countRule(systemVerilogProcessControlResult,
                         "SV-ALWAYS-COMB-EVENT-CONTROL") == 1 &&
                   hasRule(systemVerilogProcessControlResult,
                           "SV-ALWAYS-COMB-EVENT-CONTROL", 5),
               "always_comb explicit event control is reported at the at-sign");
    test.check(countRule(systemVerilogProcessControlResult,
                         "SV-ALWAYS-FF-EVENT-CONTROL") == 2 &&
                   hasRule(systemVerilogProcessControlResult,
                           "SV-ALWAYS-FF-EVENT-CONTROL", 6) &&
                   hasRule(systemVerilogProcessControlResult,
                           "SV-ALWAYS-FF-EVENT-CONTROL", 7),
               "always_ff missing and multiple event controls are reported while one control is accepted");
    test.check(countRule(systemVerilogProcessControlResult,
                         "SV-ALWAYS-LATCH-EVENT-CONTROL") == 1 &&
                   hasRule(systemVerilogProcessControlResult,
                           "SV-ALWAYS-LATCH-EVENT-CONTROL", 9),
               "always_latch explicit event control is reported while automatic sensitivity is accepted");
    test.check(countRule(systemVerilogProcessControlResult,
                         "SV-ALWAYS-LATCH-NO-LATCH") == 1 &&
                   hasRule(systemVerilogProcessControlResult,
                           "SV-ALWAYS-LATCH-NO-LATCH", 11),
               "always_latch with complete assignment is reported while real latch paths are accepted");
    test.check(countRule(systemVerilogProcessControlResult,
                         "SV-ALWAYS-FF-NONEDGE-EVENT") == 2 &&
                   hasRule(systemVerilogProcessControlResult,
                           "SV-ALWAYS-FF-NONEDGE-EVENT", 12) &&
                   hasRule(systemVerilogProcessControlResult,
                           "SV-ALWAYS-FF-NONEDGE-EVENT", 13),
               "always_ff level-change and wildcard event controls are rejected while edge event controls are accepted");

    const core::SourceFile resetPolarityMismatch(
        "rtl/reset_polarity_mismatch.sv", core::HdlLanguage::SystemVerilog,
R"(module reset_polarity_mismatch(
  input logic clk, rst_n, reset, clear_n, d,
  output logic q_bad_low, q_bad_high, q_good_low, q_good_high, q_clear,
  output logic q_good_bitwise, q_good_inequality, q_good_reverse);
always_ff @(posedge clk or negedge rst_n)
  if (rst_n) q_bad_low <= 1'b0; else q_bad_low <= d;
always_ff @(posedge clk or posedge reset)
  if (!reset) q_bad_high <= 1'b0; else q_bad_high <= d;
always_ff @(posedge clk or negedge rst_n)
  if (!(rst_n)) q_good_low <= 1'b0; else q_good_low <= d;
always_ff @(posedge clk or posedge reset)
  if (reset === 1'b1) q_good_high <= 1'b0; else q_good_high <= d;
always @(posedge clk or negedge clear_n)
  if (clear_n != 1'b0) q_clear <= 1'b0; else q_clear <= d;
always_ff @(posedge clk or negedge rst_n)
  if (~rst_n) q_good_bitwise <= 1'b0; else q_good_bitwise <= d;
always_ff @(posedge clk or posedge reset)
  if (reset !== 1'b0) q_good_inequality <= 1'b0; else q_good_inequality <= d;
always_ff @(posedge clk or negedge rst_n)
  if (1'b0 == rst_n) q_good_reverse <= 1'b0; else q_good_reverse <= d;
endmodule
)");
    const auto resetPolarityMismatchResult =
        analyzer.analyzeSource(resetPolarityMismatch);
    test.check(countRule(resetPolarityMismatchResult,
                         "RTL-ASYNC-RESET-POLARITY-MISMATCH") == 3 &&
                   hasRule(resetPolarityMismatchResult,
                           "RTL-ASYNC-RESET-POLARITY-MISMATCH", 5) &&
                   hasRule(resetPolarityMismatchResult,
                           "RTL-ASYNC-RESET-POLARITY-MISMATCH", 7) &&
                   hasRule(resetPolarityMismatchResult,
                           "RTL-ASYNC-RESET-POLARITY-MISMATCH", 13),
               "asynchronous reset edge and leading branch polarity mismatches are reported while matching forms are accepted");
    const auto resetMismatchFinding = std::ranges::find_if(
        resetPolarityMismatchResult.violations,
        [](const auto& violation) {
            return violation.ruleId ==
                       "RTL-ASYNC-RESET-POLARITY-MISMATCH" &&
                   violation.signal == "rst_n";
        });
    test.check(resetMismatchFinding != resetPolarityMismatchResult.violations.end() &&
                   resetMismatchFinding->severity == analysis::Severity::Error &&
                   resetMismatchFinding->module == "reset_polarity_mismatch" &&
                   resetMismatchFinding->column == 36,
               "reset polarity mismatch retains error severity and event location context");

    const core::SourceFile resetOrderSystemVerilog(
        "rtl/reset_order.sv", core::HdlLanguage::SystemVerilog,
R"(module reset_order(
  input logic clk, rst_n, reset, enable, d,
  output logic q_masked, q_combined, q_good, q_declared, q_unguarded);
always_ff @(posedge clk or negedge rst_n) begin
  if (enable) q_masked <= d;
  else if (!rst_n) q_masked <= 1'b0;
end
always_ff @(posedge clk or posedge reset) begin
  if (reset && enable) q_combined <= 1'b0;
  else q_combined <= d;
end
always_ff @(posedge clk or negedge rst_n) begin
  if (!rst_n) q_good <= 1'b0;
  else if (enable) q_good <= d;
end
always_ff @(posedge clk or negedge rst_n) begin
  logic local_copy;
  if (!rst_n) q_declared <= 1'b0;
  else begin local_copy = d; q_declared <= local_copy; end
end
always_ff @(posedge clk or negedge rst_n) begin
  if (!rst_n) q_unguarded <= 1'b0;
  if (enable) q_unguarded <= d;
end
endmodule
)");
    const auto resetOrderSystemVerilogResult =
        analyzer.analyzeSource(resetOrderSystemVerilog);
    test.check(countRule(resetOrderSystemVerilogResult,
                         "RTL-ASYNC-RESET-NOT-FIRST") == 2 &&
                   !hasRule(resetOrderSystemVerilogResult,
                            "RTL-ASYNC-RESET-POLARITY-MISMATCH"),
               "masked and compound SystemVerilog asynchronous reset branches are rejected while isolated leading branches and local declarations are accepted");
    test.check(countRule(resetOrderSystemVerilogResult,
                         "RTL-ASYNC-RESET-UNGUARDED-LOGIC") == 1,
               "SystemVerilog logic after a leading reset conditional is reported once");
    const auto resetOrderSystemVerilogFinding = std::ranges::find_if(
        resetOrderSystemVerilogResult.violations,
        [](const auto& violation) {
            return violation.ruleId == "RTL-ASYNC-RESET-NOT-FIRST" &&
                   violation.signal == "rst_n";
        });
    test.check(resetOrderSystemVerilogFinding !=
                   resetOrderSystemVerilogResult.violations.end() &&
                   resetOrderSystemVerilogFinding->severity == analysis::Severity::Error &&
                   resetOrderSystemVerilogFinding->module == "reset_order" &&
                   resetOrderSystemVerilogFinding->line == 4,
               "SystemVerilog reset-order finding retains event location and design context");

    const core::SourceFile parenthesizedReset(
        "rtl/parenthesized_reset.sv", core::HdlLanguage::SystemVerilog,
R"(module parenthesized_reset(
  input logic clk, rst_n, enable, d, output logic q);
always_ff @(posedge ((clk)) or negedge ((rst_n))) begin
  if (enable) q <= d;
  else if (!rst_n) q <= 1'b0;
end
endmodule
)");
    const auto parenthesizedResetResult = analyzer.analyzeSource(parenthesizedReset);
    const auto* parenthesizedResetUnit =
        findUnit(parenthesizedResetResult, "parenthesized_reset");
    test.check(countRule(parenthesizedResetResult,
                         "RTL-ASYNC-RESET-NOT-FIRST") == 1 &&
                   !hasRule(parenthesizedResetResult,
                            "RTL-COMPLEX-EDGE-EVENT") &&
                   parenthesizedResetUnit &&
                   parenthesizedResetUnit->clocks.size() == 1 &&
                   parenthesizedResetUnit->resets.size() == 1 &&
                   parenthesizedResetUnit->sequentialProcesses.size() == 1,
               "parenthesized scalar clock and reset events retain timing and reset-order analysis without a complex-edge warning");

    const core::SourceFile resetOrderVhdl(
        "rtl/reset_order.vhd", core::HdlLanguage::Vhdl,
R"(entity reset_order is
  port (clk, rst_n, reset, enable, d : in bit;
        q_masked, q_combined, q_good, q_unguarded : out bit);
end entity;
architecture rtl of reset_order is begin
  masked : process(clk, rst_n) begin
    if enable = '1' then q_masked <= d;
    elsif rst_n = '0' then q_masked <= '0';
    elsif rising_edge(clk) then q_masked <= d; end if;
  end process;
  combined : process(clk, reset) begin
    if reset = '1' and enable = '1' then q_combined <= '0';
    elsif rising_edge(clk) then q_combined <= d; end if;
  end process;
  good : process(clk, rst_n) begin
    if rst_n = '0' then q_good <= '0';
    elsif rising_edge(clk) then q_good <= d; end if;
  end process;
  unguarded : process(clk, rst_n) begin
    if rst_n = '0' then q_unguarded <= '0'; end if;
    if rising_edge(clk) and enable = '1' then q_unguarded <= d; end if;
  end process;
end architecture;
)");
    const auto resetOrderVhdlResult = analyzer.analyzeSource(resetOrderVhdl);
    test.check(countRule(resetOrderVhdlResult,
                         "RTL-ASYNC-RESET-NOT-FIRST") == 2,
               "masked and compound VHDL asynchronous reset branches are rejected while an isolated leading branch is accepted");
    test.check(countRule(resetOrderVhdlResult,
                         "RTL-ASYNC-RESET-UNGUARDED-LOGIC") == 1,
               "VHDL logic after a leading reset conditional is reported once");
    const auto resetOrderVhdlFinding = std::ranges::find_if(
        resetOrderVhdlResult.violations,
        [](const auto& violation) {
            return violation.ruleId == "RTL-ASYNC-RESET-NOT-FIRST" &&
                   violation.signal == "rst_n";
        });
    test.check(resetOrderVhdlFinding != resetOrderVhdlResult.violations.end() &&
                   resetOrderVhdlFinding->severity == analysis::Severity::Error &&
                   resetOrderVhdlFinding->module == "reset_order" &&
                   resetOrderVhdlFinding->line == 6,
               "VHDL reset-order finding retains sensitivity location and design context");

    const core::SourceFile emptySystemVerilogLatch(
        "rtl/empty_systemverilog_latch.sv", core::HdlLanguage::SystemVerilog,
        "module empty_systemverilog_latch(output logic q); "
        "always_latch begin end endmodule\n");
    test.check(countRule(analyzer.analyzeSource(emptySystemVerilogLatch),
                         "SV-ALWAYS-LATCH-NO-LATCH") == 1,
               "always_latch without a recognized assignment reports missing latch behavior");

    const core::SourceFile verilogProcessControlNames(
        "rtl/verilog_process_control_names.v", core::HdlLanguage::Verilog,
        "module verilog_process_control_names(input a, output reg q); "
        "always_comb @(a) q = a; always_latch @(a) q = a; endmodule\n");
    test.check(!hasRule(analyzer.analyzeSource(verilogProcessControlNames),
                        "SV-ALWAYS-COMB-EVENT-CONTROL") &&
                   !hasRule(analyzer.analyzeSource(verilogProcessControlNames),
                            "SV-ALWAYS-LATCH-EVENT-CONTROL") &&
                   !hasRule(analyzer.analyzeSource(verilogProcessControlNames),
                            "SV-ALWAYS-LATCH-NO-LATCH"),
               "SystemVerilog process-control rules do not run for a Verilog source");

    const core::SourceFile complexEdgeSystemVerilog(
        "rtl/complex_edge.sv", core::HdlLanguage::SystemVerilog,
R"(module complex_edge(
  input logic clk, enable, input logic [1:0] clocks,
  output logic q_expr, q_select, q_plain, q_wrapped);
always_ff @(posedge (clk & enable)) q_expr <= enable;
always_ff @(posedge clocks[0]) q_select <= enable;
always_ff @(posedge clk) q_plain <= enable;
always_ff @(posedge ((clk))) q_wrapped <= enable;
endmodule
)");
    const auto complexEdgeSystemVerilogResult =
        analyzer.analyzeSource(complexEdgeSystemVerilog);
    test.check(countRule(complexEdgeSystemVerilogResult,
                         "RTL-COMPLEX-EDGE-EVENT") == 2 &&
                   hasRule(complexEdgeSystemVerilogResult,
                           "RTL-COMPLEX-EDGE-EVENT", 4) &&
                   hasRule(complexEdgeSystemVerilogResult,
                           "RTL-COMPLEX-EDGE-EVENT", 5),
               "SystemVerilog expression and bit-select edge sources are reported while direct and parenthesized scalar clocks are accepted");
    const auto* complexEdgeSystemVerilogUnit =
        findUnit(complexEdgeSystemVerilogResult, "complex_edge");
    test.check(complexEdgeSystemVerilogUnit &&
                   complexEdgeSystemVerilogUnit->clocks.size() == 1 &&
                   complexEdgeSystemVerilogUnit->sequentialProcesses.size() == 2,
               "SystemVerilog timing model retains direct and parenthesized scalar clocks but excludes complex edge sources");

    const core::SourceFile complexEdgeVhdl(
        "rtl/complex_edge.vhd", core::HdlLanguage::Vhdl,
R"(entity complex_edge is
  port (clk, enable : in bit; clocks : in bit_vector(1 downto 0);
        q_expr, q_select, q_plain, q_wrapped : out bit);
end entity;
architecture rtl of complex_edge is begin
  p_expr : process(clk, enable) begin
    if rising_edge(clk and enable) then q_expr <= enable; end if;
  end process;
  p_select : process(clocks) begin
    if rising_edge(clocks(0)) then q_select <= enable; end if;
  end process;
  p_plain : process(clk) begin
    if rising_edge(clk) then q_plain <= enable; end if;
  end process;
  p_wrapped : process(clk) begin
    if rising_edge((clk)) then q_wrapped <= enable; end if;
  end process;
end architecture;
)");
    const auto complexEdgeVhdlResult = analyzer.analyzeSource(complexEdgeVhdl);
    test.check(countRule(complexEdgeVhdlResult,
                         "RTL-COMPLEX-EDGE-EVENT") == 2 &&
                   hasRule(complexEdgeVhdlResult,
                           "RTL-COMPLEX-EDGE-EVENT", 7) &&
                   hasRule(complexEdgeVhdlResult,
                           "RTL-COMPLEX-EDGE-EVENT", 10),
               "VHDL expression and indexed edge sources are reported while direct and parenthesized scalar clocks are accepted");
    const auto* complexEdgeVhdlUnit = findUnit(complexEdgeVhdlResult, "complex_edge");
    test.check(complexEdgeVhdlUnit && complexEdgeVhdlUnit->clocks.size() == 1 &&
                   complexEdgeVhdlUnit->sequentialProcesses.size() == 2,
               "VHDL timing model retains direct and parenthesized scalar clocks but excludes complex edge sources");

    const core::SourceFile legacyVhdlEdges(
        "rtl/legacy_vhdl_edges.vhd", core::HdlLanguage::Vhdl,
R"(entity legacy_vhdl_edges is
  port (clk_a, clk_b, d : in bit; q : out bit);
end entity;
architecture rtl of legacy_vhdl_edges is
  signal source_a : bit;
begin
  source_domain : process(clk_a) begin
    if CLK_A'event and clk_a = '1' then source_a <= d; end if;
  end process;
  destination_domain : process(clk_b) begin
    if '0' = CLK_B and clk_b'event then q <= source_a; end if;
  end process;
end architecture;
)");
    const auto legacyVhdlEdgeResult = analyzer.analyzeSource(legacyVhdlEdges);
    const auto* legacyVhdlEdgeUnit =
        findUnit(legacyVhdlEdgeResult, "legacy_vhdl_edges");
    test.check(legacyVhdlEdgeUnit && legacyVhdlEdgeUnit->clocks.size() == 2 &&
                   legacyVhdlEdgeUnit->sequentialProcesses.size() == 2 &&
                   legacyVhdlEdgeUnit->clocks[0].risingEdge &&
                   !legacyVhdlEdgeUnit->clocks[1].risingEdge &&
                   countRule(legacyVhdlEdgeResult, "CDC-DIRECT-CROSSING") == 1 &&
                   !hasRule(legacyVhdlEdgeResult, "RTL-COMPLEX-EDGE-EVENT"),
               "legacy VHDL event-and-level templates preserve case-insensitive rising/falling clock domains and CDC analysis");

    const core::SourceFile comments("rtl/comments.v", core::HdlLanguage::Verilog,
R"(module comments;
// always @(posedge clk) q = d;
/* casex (x) */
endmodule
)");
    test.check(analyzer.analyzeSource(comments).violations.empty(),
               "comment contents do not create findings");

    const core::SourceFile tokenizedCaseRules(
        "rtl/tokenized_case_rules.sv", core::HdlLanguage::SystemVerilog,
R"(module tokenized_case_rules(input logic [1:0] sel, output logic q, output logic r);
always_comb begin
  case (
    sel
  )
    2'b00: begin
      case (sel[0])
        1'b0: q = 1'b0;
        default: q = 1'b1;
      endcase
    end
    default: q = 1'b0;
  endcase
  case (sel) 2'b00: r = 1'b0; endcase
  casex (sel) default: r = 1'b1; endcase
end
endmodule
)");
    const auto tokenizedCaseResult = analyzer.analyzeSource(tokenizedCaseRules);
    const core::SourceFile uppercaseIdentifiers("rtl/uppercase_identifiers.sv", core::HdlLanguage::SystemVerilog,
        "module probe #(parameter MODULE=1, parameter TYPE=2)(input INPUT, output OUTPUT);\n"
        "wire BEGIN, END, ENDCASE, FUNCTION, TASK, ENDMODULE;\nendmodule\n");
    test.check(parser::checkSyntax(uppercaseIdentifiers).empty(),
               "uppercase Verilog identifiers do not open or close keyword constructs");
    const auto upperUnits = parser::parseDesignUnits(uppercaseIdentifiers);
    test.check(upperUnits.size() == 1 && upperUnits[0].parameters.size() == 2 && upperUnits[0].ports.size() == 2 &&
               upperUnits[0].parameters[0].name == "MODULE" && upperUnits[0].ports[0].name == "INPUT",
               "uppercase keyword-shaped parameter/port identifiers retained in design parser");
    const core::SourceFile escapedKeywords("rtl/escaped_keywords.sv", core::HdlLanguage::SystemVerilog,
        "module probe; wire \\module , \\end , \\endcase ; endmodule\n");
    test.check(parser::checkSyntax(escapedKeywords).empty(), "escaped identifiers do not alter syntax construct balance");
    test.check(countRule(tokenizedCaseResult, "VL-CASE-DEFAULT") == 1 &&
                   hasRule(tokenizedCaseResult, "VL-CASE-DEFAULT", 14),
               "nested defaults remain scoped and same-line case without default is reported");
    test.check(countRule(tokenizedCaseResult, "VL-CASEX") == 1 &&
                   hasRule(tokenizedCaseResult, "VL-CASEX", 15),
               "tokenized casex rule reports the actual nested-scope keyword");
    test.check(!hasRule(tokenizedCaseResult, "SYNTAX-UNCLOSED-CONSTRUCT") &&
                   !hasRule(tokenizedCaseResult, "SYNTAX-UNEXPECTED-CLOSER"),
               "shared endcase closer matches the most recent case, casez, or casex opener");

    const core::SourceFile verilogConstantConditions(
        "rtl/verilog_constant_conditions.sv", core::HdlLanguage::SystemVerilog,
R"(module verilog_constant_conditions(input logic d, output logic q, r);
always_comb begin
  if (1'b0) q = d;
  while (1) begin
    q = d;
  end
  if (!(2 === 2)) q = d;
  if (d && 1'b0) q = d;
  if (1'b1 || d) q = d;
  if (d || 1'b0) q = d;
  if (d) q = 1'b0;
  q = 1'b0 ? d : 1'b0;
  q = d ? d : 1'b0;
  q = d ? 1'b0 : 1'b1 ? d : 1'b0;
end
assign r = 1'b1 ? d : 1'b0;
generate if (!(2 === 2)) begin : disabled end endgenerate
endmodule
)");
    const auto verilogConstantConditionResult =
        analyzer.analyzeSource(verilogConstantConditions);
    test.check(countRule(verilogConstantConditionResult,
                         "RTL-CONSTANT-CONDITION") == 8 &&
                   hasRule(verilogConstantConditionResult,
                           "RTL-CONSTANT-CONDITION", 3) &&
                   hasRule(verilogConstantConditionResult,
                           "RTL-CONSTANT-CONDITION", 4) &&
                   hasRule(verilogConstantConditionResult,
                           "RTL-CONSTANT-CONDITION", 7) &&
                   hasRule(verilogConstantConditionResult,
                           "RTL-CONSTANT-CONDITION", 8) &&
                   hasRule(verilogConstantConditionResult,
                           "RTL-CONSTANT-CONDITION", 9) &&
                   hasRule(verilogConstantConditionResult,
                           "RTL-CONSTANT-CONDITION", 12) &&
                   hasRule(verilogConstantConditionResult,
                           "RTL-CONSTANT-CONDITION", 14) &&
                   hasRule(verilogConstantConditionResult,
                           "RTL-CONSTANT-CONDITION", 16),
               "Verilog procedural and runtime ternary constants are reported while runtime-dependent and generate conditions are excluded");

    const core::SourceFile verilogConditionSideEffects(
        "rtl/verilog_condition_side_effects.sv", core::HdlLanguage::SystemVerilog,
R"(module verilog_condition_side_effects(output logic q);
integer a, b;
initial begin
  a = 0; b = 1; q = 1'b0;
  if (a = b) q = 1'b1;
  while ((a += 1) < 2) q = 1'b0;
  if (b++ > 0) q = 1'b1;
  if ((a <<= 1) != 0) q = 1'b0;
  if (a == b) q = 1'b1;
  if (a <= b) q = 1'b0;
  if (a != b) q = 1'b1;
  q = (a = b) ? 1'b1 : 1'b0;
  q = (a++) ? 1'b1 : 1'b0;
  q = (a == b) ? 1'b1 : 1'b0;
  q = b ? 1'b0 : (a += 1) ? 1'b1 : 1'b0;
end
endmodule
)");
    const auto verilogConditionSideEffectResult =
        analyzer.analyzeSource(verilogConditionSideEffects);
    test.check(countRule(verilogConditionSideEffectResult,
                         "VL-SIDE-EFFECT-CONDITION") == 7 &&
                   hasRule(verilogConditionSideEffectResult,
                           "VL-SIDE-EFFECT-CONDITION", 5) &&
                   hasRule(verilogConditionSideEffectResult,
                           "VL-SIDE-EFFECT-CONDITION", 6) &&
                   hasRule(verilogConditionSideEffectResult,
                           "VL-SIDE-EFFECT-CONDITION", 7) &&
                   hasRule(verilogConditionSideEffectResult,
                           "VL-SIDE-EFFECT-CONDITION", 8) &&
                   hasRule(verilogConditionSideEffectResult,
                           "VL-SIDE-EFFECT-CONDITION", 12) &&
                   hasRule(verilogConditionSideEffectResult,
                           "VL-SIDE-EFFECT-CONDITION", 13) &&
                   !hasRule(verilogConditionSideEffectResult,
                            "VL-SIDE-EFFECT-CONDITION", 14) &&
                   hasRule(verilogConditionSideEffectResult,
                           "VL-SIDE-EFFECT-CONDITION", 15),
               "Verilog assignment, compound assignment, increment, and shift-assignment side effects are reported in branch and ternary guards while comparisons remain excluded");

    const core::SourceFile verilogEmptyConditionalBranches(
        "rtl/verilog_empty_conditional_branches.sv", core::HdlLanguage::SystemVerilog,
R"(module verilog_empty_conditional_branches(input logic a, b, output logic q);
always_comb begin
  q = 1'b0;
  if (a);
  if (b) begin end
  if (a) q = 1'b1; else;
  if (b) begin : empty_named ; end else begin q = 1'b1; end
  if (a) begin if (b); else q = 1'b1; end
  if (a) q = 1'b1; else if (b) q = 1'b0;
end
endmodule
)");
    const auto verilogEmptyConditionalBranchResult =
        analyzer.analyzeSource(verilogEmptyConditionalBranches);
    test.check(countRule(verilogEmptyConditionalBranchResult,
                         "VL-EMPTY-CONDITIONAL-BRANCH") == 5 &&
                   hasRule(verilogEmptyConditionalBranchResult,
                           "VL-EMPTY-CONDITIONAL-BRANCH", 4) &&
                   hasRule(verilogEmptyConditionalBranchResult,
                           "VL-EMPTY-CONDITIONAL-BRANCH", 5) &&
                   hasRule(verilogEmptyConditionalBranchResult,
                           "VL-EMPTY-CONDITIONAL-BRANCH", 6) &&
                   hasRule(verilogEmptyConditionalBranchResult,
                           "VL-EMPTY-CONDITIONAL-BRANCH", 7) &&
                   hasRule(verilogEmptyConditionalBranchResult,
                           "VL-EMPTY-CONDITIONAL-BRANCH", 8),
               "Verilog empty statements and empty named or unnamed conditional blocks are reported without flagging populated or else-if branches");

    const core::SourceFile verilogEmptyCaseItems(
        "rtl/verilog_empty_case_items.sv", core::HdlLanguage::SystemVerilog,
R"(module verilog_empty_case_items(input logic [1:0] sel, output logic q);
always_comb begin
  q = 1'b0;
  case (sel)
    2'b00:;
    2'b01: begin end
    2'b10: begin : empty_named ; end
    default: q = 1'b1;
  endcase
  case (sel)
    2'b00: begin
      case (sel)
        2'b00:;
        default: q = 1'b0;
      endcase
    end
    default: q = 1'b1;
  endcase
  case (sel) default:; endcase
  case (sel) default: q = 1'b0; endcase
end
endmodule
)");
    const auto verilogEmptyCaseItemResult = analyzer.analyzeSource(verilogEmptyCaseItems);
    test.check(countRule(verilogEmptyCaseItemResult,
                         "VL-EMPTY-CASE-ITEM") == 5 &&
                   hasRule(verilogEmptyCaseItemResult, "VL-EMPTY-CASE-ITEM", 5) &&
                   hasRule(verilogEmptyCaseItemResult, "VL-EMPTY-CASE-ITEM", 6) &&
                   hasRule(verilogEmptyCaseItemResult, "VL-EMPTY-CASE-ITEM", 7) &&
                   hasRule(verilogEmptyCaseItemResult, "VL-EMPTY-CASE-ITEM", 13) &&
                   hasRule(verilogEmptyCaseItemResult, "VL-EMPTY-CASE-ITEM", 19),
               "Verilog empty case statements and named or unnamed blocks are reported with nested case scope isolation");

    const core::SourceFile verilogSelfAssignments(
        "rtl/verilog_self_assignments.sv", core::HdlLanguage::SystemVerilog,
R"(module verilog_self_assignments(input logic clk, input logic d,
  output logic q, output logic r, output logic [1:0] v);
always @(posedge clk) begin
  q <= q;
  r = r;
  v[0] <= v[0];
  q <= q ^ d;
end
endmodule
)");
    const auto verilogSelfAssignmentResult =
        analyzer.analyzeSource(verilogSelfAssignments);
    test.check(countRule(verilogSelfAssignmentResult,
                         "RTL-SELF-ASSIGNMENT") == 2 &&
                   hasRule(verilogSelfAssignmentResult,
                           "RTL-SELF-ASSIGNMENT", 4) &&
                   hasRule(verilogSelfAssignmentResult,
                           "RTL-SELF-ASSIGNMENT", 5),
               "Verilog blocking and nonblocking scalar self-assignments are reported while indexed and expression updates are excluded");

    const core::SourceFile verilogEmptyProcesses(
        "rtl/verilog_empty_processes.sv", core::HdlLanguage::SystemVerilog,
R"(module verilog_empty_processes(input logic a, output logic q);
always @(*) begin : named_empty end
initial begin end
always @(*) begin q = a; end
endmodule
)");
    const auto verilogEmptyProcessResult =
        analyzer.analyzeSource(verilogEmptyProcesses);
    test.check(countRule(verilogEmptyProcessResult,
                         "RTL-EMPTY-PROCESS") == 2 &&
                   hasRule(verilogEmptyProcessResult,
                           "RTL-EMPTY-PROCESS", 2) &&
                   hasRule(verilogEmptyProcessResult,
                           "RTL-EMPTY-PROCESS", 3),
               "Verilog named empty always and initial blocks are reported while an assigning process is excluded");

    const core::SourceFile verilogAlwaysNoSuspension(
        "rtl/verilog_always_no_suspension.sv", core::HdlLanguage::SystemVerilog,
R"(module verilog_always_no_suspension(
  input logic a,
  output logic q0, q1, q2, q3, q4, q5, q6, q7
);
always begin
  q0 = ~q0;
end
always q1 = a;
always @(a) q2 = a;
always begin
  #1;
  q3 = a;
end
always begin
  wait (a);
  q4 = a;
end
always_comb q5 = a;
always_ff @(posedge a) q6 <= a;
initial q7 = a;
endmodule
)");
    const auto verilogAlwaysNoSuspensionResult =
        analyzer.analyzeSource(verilogAlwaysNoSuspension);
    test.check(countRule(verilogAlwaysNoSuspensionResult,
                         "VL-ALWAYS-NO-SUSPENSION") == 2 &&
                   hasRule(verilogAlwaysNoSuspensionResult,
                           "VL-ALWAYS-NO-SUSPENSION", 5) &&
                   hasRule(verilogAlwaysNoSuspensionResult,
                           "VL-ALWAYS-NO-SUSPENSION", 8),
               "Plain always processes without any suspension point are reported while event, delay, wait, always_comb, always_ff, and initial processes are excluded");

    const core::SourceFile verilogUnusedParameters(
        "rtl/verilog_unused_parameters.sv", core::HdlLanguage::SystemVerilog,
R"(module parameter_child #(parameter UNUSED = 1)(output logic [UNUSED-1:0] y);
assign y = '0;
endmodule
module verilog_unused_parameters #(
  parameter USED = 2, parameter UNUSED = 3,
  parameter type TYPE_USED = logic, parameter type TYPE_UNUSED = logic
)(input TYPE_USED [USED-1:0] d, output logic q);
localparam LOCAL_USED = USED;
localparam LOCAL_UNUSED = 4;
parameter_child #(.UNUSED(1)) u_child();
assign q = d[LOCAL_USED-1];
endmodule
)");
    const auto verilogUnusedParameterResult =
        analyzer.analyzeSource(verilogUnusedParameters);
    test.check(countRule(verilogUnusedParameterResult,
                         "RTL-UNUSED-PARAMETER") == 3 &&
                   hasRule(verilogUnusedParameterResult,
                           "RTL-UNUSED-PARAMETER", 5) &&
                   hasRule(verilogUnusedParameterResult,
                           "RTL-UNUSED-PARAMETER", 6) &&
                   hasRule(verilogUnusedParameterResult,
                           "RTL-UNUSED-PARAMETER", 9),
               "Verilog unused value, type, and local parameters are reported while interface, expression, and named-formal uses are classified correctly");

    const core::SourceFile mixedAssignmentStyle(
        "rtl/mixed_assignment_style.sv", core::HdlLanguage::SystemVerilog,
R"(module mixed_assignment_style(input logic clk, input logic enable, input logic d,
  output logic [1:0] q, output logic cross_process);
always_ff @(posedge clk) begin
  if (enable) q[0] = d;
  else q[1] <= d;
end
always_comb cross_process = d;
always_ff @(posedge clk) cross_process <= d;
endmodule
)");
    const auto mixedAssignmentStyleResult = analyzer.analyzeSource(mixedAssignmentStyle);
    const auto mixedAssignmentFinding = std::ranges::find_if(
        mixedAssignmentStyleResult.violations,
        [](const auto& violation) {
            return violation.ruleId == "VL-MIXED-ASSIGNMENT-STYLE";
        });
    test.check(countRule(mixedAssignmentStyleResult,
                         "VL-MIXED-ASSIGNMENT-STYLE") == 1 &&
                   mixedAssignmentFinding != mixedAssignmentStyleResult.violations.end() &&
                   mixedAssignmentFinding->severity == analysis::Severity::Warning &&
                   mixedAssignmentFinding->line == 5 &&
                   mixedAssignmentFinding->module == "mixed_assignment_style" &&
                   mixedAssignmentFinding->signal == "q",
               "mixed assignment style is reported once per signal and process at the conflicting assignment");

    const core::SourceFile loopControlAssignmentStyle(
        "rtl/loop_control_assignment_style.sv", core::HdlLanguage::SystemVerilog,
R"(module loop_control_assignment_style(input logic d);
integer i;
always_comb begin
  for (i = 0; i < 2; i = i + 1) i <= d;
end
endmodule
)");
    test.check(!hasRule(analyzer.analyzeSource(loopControlAssignmentStyle),
                        "VL-MIXED-ASSIGNMENT-STYLE"),
               "for-loop control assignments do not create a mixed-style finding for the iterator");

    const core::SourceFile assignmentStyleForBoundary(
        "rtl/assignment_style_for_boundary.sv", core::HdlLanguage::SystemVerilog,
R"(module assignment_style_for_boundary(input logic clk, input logic d,
  output logic [1:0] seq_q, output logic [1:0] comb_q);
integer i;
always_ff @(posedge clk) begin
  for (i = 0; i < 2; i = i + 1) seq_q[i] <= d;
  seq_q[0] = d;
end
always_comb begin
  for (i = 0; i < 2; i = i + 1) comb_q[i] <= d;
  comb_q[0] = d;
end
endmodule
)");
    const auto assignmentStyleForBoundaryResult =
        analyzer.analyzeSource(assignmentStyleForBoundary);
    test.check(countRule(assignmentStyleForBoundaryResult, "VL-SEQ-BLOCKING") == 1 &&
                   hasRule(assignmentStyleForBoundaryResult, "VL-SEQ-BLOCKING", 6) &&
                   countRule(assignmentStyleForBoundaryResult, "VL-COMB-NONBLOCKING") == 1 &&
                   hasRule(assignmentStyleForBoundaryResult, "VL-COMB-NONBLOCKING", 9),
               "GJB10157.9.1.3/.4 exclude for headers but retain body assignment findings");

    const core::SourceFile tokenizedAssignmentStyles(
        "rtl/tokenized_assignment_styles.sv", core::HdlLanguage::SystemVerilog,
R"(module tokenized_assignment_styles(input logic clk, input logic [3:0] d,
  output logic [7:0] q, output logic [7:0] r);
always_ff @(posedge clk) begin
  q
    = d;
  if (d <= q) q <= {4'b0, d};
end
always_comb begin
  r
    <= q;
  if (d <= q) r = q;
end
endmodule
)");
    const auto tokenizedAssignmentStyleResult =
        analyzer.analyzeSource(tokenizedAssignmentStyles);
    test.check(countRule(tokenizedAssignmentStyleResult, "VL-SEQ-BLOCKING") == 1 &&
                   hasRule(tokenizedAssignmentStyleResult, "VL-SEQ-BLOCKING", 4) &&
                   countRule(tokenizedAssignmentStyleResult,
                             "VL-COMB-NONBLOCKING") == 1 &&
                   hasRule(tokenizedAssignmentStyleResult,
                           "VL-COMB-NONBLOCKING", 9),
               "tokenized process analysis detects assignment targets split across lines");
    test.check(!hasRule(tokenizedAssignmentStyleResult, "RTL-INPUT-DRIVEN") &&
                   !hasRule(tokenizedAssignmentStyleResult, "RTL-WIDTH-TRUNCATION"),
               "less-than-or-equal comparisons are not interpreted as assignment targets");

    const core::SourceFile assignmentAfterNestedBlock(
        "rtl/assignment_after_nested_block.sv", core::HdlLanguage::SystemVerilog,
R"(module assignment_after_nested_block(input logic clk, input logic enable, input logic d,
  output logic q, output logic r);
always_ff @(posedge clk) begin
  if (enable) begin
    q <= d;
  end
  r <= d;
end
endmodule
)");
    test.check(!hasRule(analyzer.analyzeSource(assignmentAfterNestedBlock),
                        "RTL-UNDRIVEN-OUTPUT"),
               "assignment immediately following a nested end remains a recognized process driver");


    const core::SourceFile implicitNets("rtl/implicit_nets.v", core::HdlLanguage::Verilog,
R"(module child(input wire a, output wire y);
assign y = a;
endmodule
module implicit_nets(declared_in, declared_out);
input wire declared_in;
output wire declared_out;
parameter integer CONST = 1;
assign implicit_lhs = declared_in;
assign declared_out = implicit_rhs;
child u0(.a(declared_in), .y(instance_net));
child u2(declared_in, positional_net);
child u1(.a(CONST), .y(declared_out));
endmodule
)");
    const auto implicitNetResult = analyzer.analyzeSource(implicitNets);
    test.check(countRule(implicitNetResult, "VL-IMPLICIT-NET") == 4,
               "undeclared continuous endpoints and simple instance actuals are reported");
    test.check(hasRule(implicitNetResult, "VL-IMPLICIT-NET", 8) &&
                   hasRule(implicitNetResult, "VL-IMPLICIT-NET", 9) &&
                   hasRule(implicitNetResult, "VL-IMPLICIT-NET", 10) &&
                   hasRule(implicitNetResult, "VL-IMPLICIT-NET", 11),
               "implicit-net findings retain their first precise source locations");
    bool implicitNetContext = true;
    for (const auto& violation : implicitNetResult.violations) {
        if (violation.ruleId == "VL-IMPLICIT-NET") {
            implicitNetContext = implicitNetContext &&
                                 violation.severity == analysis::Severity::Warning &&
                                 violation.module == "implicit_nets" &&
                                 violation.signal && !violation.signal->empty();
        }
    }
    test.check(implicitNetContext,
               "implicit-net findings retain warning severity, unit, and signal context");

    const core::SourceFile systemVerilogTypedSignal(
        "rtl/systemverilog_typed_signal.sv", core::HdlLanguage::SystemVerilog,
R"(module systemverilog_typed_signal(input logic d, output logic q);
typedef logic state_t;
state_t state;
assign state = d;
assign q = state;
endmodule
)");
    test.check(!hasRule(analyzer.analyzeSource(systemVerilogTypedSignal), "VL-IMPLICIT-NET"),
               "Verilog-only implicit-net scan does not guess across SystemVerilog user types");

    const core::SourceFile malformed("rtl/malformed.sv", core::HdlLanguage::SystemVerilog,
R"(module malformed(input logic a;
always_comb begin
  case (a)
    1'b0: a = 1'b1;
end
/* unterminated comment
)");
    const auto malformedResult = analyzer.analyzeSource(malformed);
    test.check(hasRule(malformedResult, "SYNTAX-UNCLOSED-DELIMITER", 1),
               "unclosed delimiter is reported at its opening location");
    test.check(hasRule(malformedResult, "SYNTAX-UNCLOSED-CONSTRUCT", 1) &&
                   hasRule(malformedResult, "SYNTAX-UNCLOSED-CONSTRUCT", 3),
               "unclosed module and case constructs are reported");
    test.check(hasRule(malformedResult, "SYNTAX-UNTERMINATED-COMMENT", 6),
               "unterminated block comment is reported");

    const core::SourceFile unexpectedCloser("rtl/unexpected.v", core::HdlLanguage::Verilog,
                                            "endmodule\n");
    test.check(hasRule(analyzer.analyzeSource(unexpectedCloser),
                       "SYNTAX-UNEXPECTED-CLOSER", 1),
               "unexpected structural closer is reported");

    const core::SourceFile badVhdl("rtl/bad.vhd", core::HdlLanguage::Vhdl,
                                  "entity bad is\nconstant text : string := \"broken\nend;\n");
    test.check(hasRule(analyzer.analyzeSource(badVhdl), "SYNTAX-UNTERMINATED-STRING", 2),
               "unterminated VHDL string is reported");

    const core::SourceFile conditional("rtl/conditional.sv", core::HdlLanguage::SystemVerilog,
R"(`ifdef FEATURE_ENABLED
module enabled_feature;
  always_comb begin
    casex (1'b0)
      1'b0: ;
    endcase
  end
endmodule
`else
module disabled_feature; endmodule
`endif
)");
    const auto featureOff = analyzer.analyzeSource(conditional);
    test.check(findUnit(featureOff, "disabled_feature") && !findUnit(featureOff, "enabled_feature"),
               "inactive conditional branch is excluded from structure analysis");
    test.check(!hasRule(featureOff, "VL-CASEX"),
               "inactive conditional branch does not create lint findings");
    const analysis::Analyzer featureAnalyzer({{}, {}, {}, {}, {"FEATURE_ENABLED=1"}, {}});
    const auto featureOn = featureAnalyzer.analyzeSource(conditional);
    test.check(findUnit(featureOn, "enabled_feature") && !findUnit(featureOn, "disabled_feature"),
               "command/config macro selects the enabled branch");
    test.check(hasRule(featureOn, "VL-CASEX"), "active conditional branch is linted");

    const core::SourceFile badPreprocessor("rtl/bad_pp.v", core::HdlLanguage::Verilog,
R"(`else
`ifdef NEVER_CLOSED
module hidden; endmodule
)");
    const auto badPpResult = analyzer.analyzeSource(badPreprocessor);
    test.check(hasRule(badPpResult, "PP-UNEXPECTED-DIRECTIVE", 1),
               "unexpected preprocessor branch directive is reported");
    test.check(hasRule(badPpResult, "PP-UNCLOSED-CONDITIONAL", 2),
               "unclosed preprocessor conditional is reported");

    const core::SourceFile missingInclude("rtl/include_test.v", core::HdlLanguage::Verilog,
                                         "`include \"missing_header.vh\"\nmodule include_test; endmodule\n");
    test.check(hasRule(analyzer.analyzeSource(missingInclude), "PP-INCLUDE-NOT-FOUND", 1),
               "missing include file is reported at the directive");

    const core::SourceFile semanticRisks("rtl/semantic_risks.sv", core::HdlLanguage::SystemVerilog,
R"(module semantic_risks(
  input logic clk,
  input logic assigned_input,
  output logic multi_driver,
  output logic undriven
);
always_ff @(posedge clk) begin
  multi_driver <= 1'b0;
end
always_ff @(negedge clk) begin
  multi_driver <= 1'b1;
  assigned_input <= 1'b0;
end
endmodule
)");
    const auto semanticResult = analyzer.analyzeSource(semanticRisks);
    test.check(hasRule(semanticResult, "RTL-MULTIPLE-DRIVERS", 11),
               "assignments from distinct processes create a multiple-driver finding");
    test.check(hasRule(semanticResult, "RTL-INPUT-DRIVEN", 12),
               "internal assignment to an input port is reported");
    test.check(!hasRule(semanticResult, "RTL-UNUSED-INPUT-PORT", 3),
               "an illegally driven input does not also produce a secondary unused-input finding");
    test.check(hasRule(semanticResult, "RTL-UNDRIVEN-OUTPUT", 5),
               "output without a local driver is reported at its declaration");
    bool semanticContext = false;
    for (const auto& violation : semanticResult.violations) {
        if (violation.ruleId == "RTL-MULTIPLE-DRIVERS") {
            semanticContext = violation.severity == analysis::Severity::Error &&
                              violation.module == "semantic_risks" &&
                              violation.signal == "multi_driver";
        }
    }
    test.check(semanticContext, "semantic finding retains severity, unit, and signal context");
    std::ostringstream semanticJson;
    report::writeJson(semanticResult, semanticJson);
    test.check(semanticJson.str().find("\"signal\":\"multi_driver\"") != std::string::npos,
               "JSON report preserves semantic signal context");
    std::ostringstream semanticSarif;
    report::writeSarif(semanticResult, semanticSarif);
    test.check(semanticSarif.str().find("\"signal\":\"multi_driver\"") != std::string::npos,
               "SARIF properties preserve semantic signal context");

    const core::SourceFile verilogCombinationalLoop(
        "rtl/verilog_comb_loop.sv", core::HdlLanguage::SystemVerilog,
R"(module verilog_comb_loop(input logic enable, output logic out);
logic feedback_a, feedback_b;
assign feedback_a = feedback_b;
always_comb begin
  feedback_b = feedback_a & enable;
  out = feedback_b;
end
endmodule
)");
    const auto verilogLoopResult = analyzer.analyzeSource(verilogCombinationalLoop);
    test.check(countRule(verilogLoopResult, "RTL-COMBINATIONAL-LOOP") == 1,
               "Verilog continuous and procedural combinational dependency cycle is reported once");
    const auto verilogLoop = std::ranges::find_if(verilogLoopResult.violations,
        [](const auto& violation) { return violation.ruleId == "RTL-COMBINATIONAL-LOOP"; });
    test.check(verilogLoop != verilogLoopResult.violations.end() &&
                   verilogLoop->severity == analysis::Severity::Error &&
                   verilogLoop->module == "verilog_comb_loop" && verilogLoop->signal,
               "combinational-loop finding retains error severity and signal context");

    const core::SourceFile selfCombinationalLoop(
        "rtl/self_comb_loop.sv", core::HdlLanguage::SystemVerilog,
R"(module self_comb_loop(output logic feedback);
always_comb feedback = ~feedback;
endmodule
)");
    test.check(countRule(analyzer.analyzeSource(selfCombinationalLoop),
                         "RTL-COMBINATIONAL-LOOP") == 1,
               "uninitialized combinational self-reference remains a reported loop");

    const core::SourceFile acyclicBitChain(
        "rtl/acyclic_bit_chain.sv", core::HdlLanguage::SystemVerilog,
R"(module acyclic_bit_chain(input logic [2:0] data, input logic sel,
                         output wire [2:0] temp1);
assign temp1[0] = data[0];
assign temp1[1] = temp1[0] ^ data[1];
assign temp1[2] = sel ? data[0] : temp1[1];
endmodule
)");
    test.check(!hasRule(analyzer.analyzeSource(acyclicBitChain),
                        "RTL-COMBINATIONAL-LOOP"),
               "distinct bit-select nodes do not create a false vector-level feedback cycle");

    const core::SourceFile cyclicBitChain(
        "rtl/cyclic_bit_chain.sv", core::HdlLanguage::SystemVerilog,
R"(module cyclic_bit_chain(input logic [2:0] data, input logic sel,
                        output wire [2:0] temp1);
assign temp1[0] = data[0] & temp1[2];
assign temp1[1] = temp1[0] ^ data[1];
assign temp1[2] = sel ? data[0] : temp1[1];
endmodule
)");
    test.check(countRule(analyzer.analyzeSource(cyclicBitChain),
                         "RTL-COMBINATIONAL-LOOP") == 1,
               "constant bit selects preserve the real three-bit combinational cycle");

    const core::SourceFile sequentialFeedback(
        "rtl/sequential_feedback.sv", core::HdlLanguage::SystemVerilog,
R"(module sequential_feedback(input logic clk, output logic [3:0] counter);
always_ff @(posedge clk) counter <= counter + 1'b1;
endmodule
)");
    test.check(!hasRule(analyzer.analyzeSource(sequentialFeedback), "RTL-COMBINATIONAL-LOOP"),
               "sequential feedback is a graph boundary and is not a combinational loop");

    const core::SourceFile implicitSensitivityNames(
        "rtl/implicit_sensitivity.v", core::HdlLanguage::Verilog,
R"(module implicit_sensitivity(input A, B, output reg C, output reg Q);
always @(A, CLK) C = A;
always @(A) if (B_COND) C = A & B;
always @(B_COND) C = A;
always @(posedge CLK) Q <= A;
endmodule
)");
    const auto sensitivityResult = analyzer.analyzeSource(implicitSensitivityNames);
    test.check(countRule(sensitivityResult, "RTL-REDUNDANT-SENSITIVITY") == 2 &&
               countRule(sensitivityResult, "RTL-INCOMPLETE-SENSITIVITY") == 3,
               "plain Verilog sensitivity names are reported per relation difference while edge-triggered lists are excluded");

    const core::SourceFile loopControl(
        "rtl/loop_control.sv", core::HdlLanguage::SystemVerilog,
R"(module loop_control(input logic [3:0] data, output logic parity);
integer i;
always_comb begin
  parity = 1'b0;
  for (i = 0; i < 4; i = i + 1) parity = parity ^ data[i];
end
endmodule
)");
    test.check(!hasRule(analyzer.analyzeSource(loopControl), "RTL-COMBINATIONAL-LOOP"),
               "for-loop control updates are excluded from combinational dependency cycles");

    const core::SourceFile vhdlCombinationalLoop(
        "rtl/vhdl_comb_loop.vhd", core::HdlLanguage::Vhdl,
R"(entity vhdl_comb_loop is
  port (enable : in std_logic; result_value : out std_logic);
end entity;
architecture rtl of vhdl_comb_loop is
  signal feedback_a, feedback_b : std_logic;
begin
  feedback_a <= feedback_b;
  process(feedback_a, enable) begin
    feedback_b <= feedback_a and enable;
  end process;
  result_value <= feedback_b;
end architecture;
)");
    test.check(countRule(analyzer.analyzeSource(vhdlCombinationalLoop),
                         "RTL-COMBINATIONAL-LOOP") == 1,
               "VHDL concurrent/process combinational dependency cycle is reported once");

    const core::SourceFile vhdlSequentialFeedback(
        "rtl/vhdl_sequential_feedback.vhd", core::HdlLanguage::Vhdl,
R"(entity vhdl_sequential_feedback is
  port (clk : in std_logic; q : out std_logic);
end entity;
architecture rtl of vhdl_sequential_feedback is begin
  process(clk) begin
    if rising_edge(clk) then q <= not q; end if;
  end process;
end architecture;
)");
    test.check(!hasRule(analyzer.analyzeSource(vhdlSequentialFeedback),
                        "RTL-COMBINATIONAL-LOOP"),
               "VHDL clocked feedback is not treated as a combinational loop");

    const core::SourceFile verilogWidthTruncation(
        "rtl/verilog_width.sv", core::HdlLanguage::SystemVerilog,
R"(module verilog_width(input logic [15:0] wide_value,
                       output logic [7:0] narrow_value,
                       output logic [15:0] preserved_value);
assign narrow_value = wide_value;
assign preserved_value = narrow_value;
endmodule
)");
    const auto verilogWidthResult = analyzer.analyzeSource(verilogWidthTruncation);
    test.check(countRule(verilogWidthResult, "RTL-WIDTH-TRUNCATION") == 1,
               "Verilog direct wider-to-narrower assignment is reported once");
    const auto widthFinding = std::ranges::find_if(verilogWidthResult.violations,
        [](const auto& violation) { return violation.ruleId == "RTL-WIDTH-TRUNCATION"; });
    test.check(widthFinding != verilogWidthResult.violations.end() &&
                   widthFinding->severity == analysis::Severity::Warning &&
                   widthFinding->module == "verilog_width" &&
                   widthFinding->signal == "narrow_value" &&
                   widthFinding->message.find("16 bits to 8 bits") != std::string::npos,
               "width finding retains sizes, unit, destination signal, and warning severity");

    const core::SourceFile explicitWidthIntent(
        "rtl/explicit_width.sv", core::HdlLanguage::SystemVerilog,
R"(module explicit_width #(parameter WIDTH = 16)(
  input logic [WIDTH-1:0] parameterized_value,
  input logic [15:0] wide_value,
  output logic [7:0] sliced_value,
  output logic [7:0] unknown_value
);
assign sliced_value = wide_value[7:0];
assign unknown_value = parameterized_value;
endmodule
)");
    test.check(countRule(analyzer.analyzeSource(explicitWidthIntent), "RTL-WIDTH-TRUNCATION") == 1,
               "default parameter width exposes truncation while explicit slice stays safe");
    const core::SourceFile unresolvedParameterizedWidth(
        "rtl/unresolved_parameterized_width.sv", core::HdlLanguage::SystemVerilog,
R"(module unresolved_parameterized_width #(parameter WIDTH = UNKNOWN)(
  input logic [WIDTH-1:0] parameterized_value, output logic [7:0] narrow_value);
assign narrow_value = parameterized_value;
endmodule
)");
    test.check(!hasRule(analyzer.analyzeSource(unresolvedParameterizedWidth),
                        "RTL-WIDTH-TRUNCATION"),
               "unresolved parameterized width is not guessed as truncation");

    const core::SourceFile vhdlWidthTruncation(
        "rtl/vhdl_width.vhd", core::HdlLanguage::Vhdl,
R"(entity vhdl_width is
  port (wide_value : in std_logic_vector(15 downto 0);
        narrow_value : out std_logic_vector(7 downto 0);
        preserved_value : out std_logic_vector(15 downto 0));
end entity;
architecture rtl of vhdl_width is begin
  narrow_value <= wide_value;
  preserved_value <= narrow_value;
end architecture;
)");
    test.check(countRule(analyzer.analyzeSource(vhdlWidthTruncation),
                         "RTL-WIDTH-TRUNCATION") == 2,
               "VHDL native width rule reports both truncating and extending direct assignments");

    const core::SourceFile verilogLiteralWidthTruncation(
        "rtl/verilog_literal_width.sv", core::HdlLanguage::SystemVerilog,
R"(module verilog_literal_width(output logic [7:0] q0, q1, q2, q3, q4, q5, q6);
always_comb begin
  q0 = 16'h1234;
  q1 = ((12'd7));
  q2 = 8'hff;
  q3 = '0;
  q4 = -16'sd1;
  q5 = {8'h12};
end
assign q6 = 9'b1_0000_0000;
endmodule
)");
    const auto verilogLiteralWidthResult =
        analyzer.analyzeSource(verilogLiteralWidthTruncation);
    test.check(countRule(verilogLiteralWidthResult,
                         "RTL-WIDTH-TRUNCATION") == 3 &&
                   hasRule(verilogLiteralWidthResult,
                           "RTL-WIDTH-TRUNCATION", 3) &&
                   hasRule(verilogLiteralWidthResult,
                           "RTL-WIDTH-TRUNCATION", 4) &&
                   hasRule(verilogLiteralWidthResult,
                           "RTL-WIDTH-TRUNCATION", 10),
               "Verilog procedural and continuous explicit literals wider than direct targets are reported while equal, unbased, signed-expression, and concatenated sources stay conservative");

    const core::SourceFile vhdlLiteralWidthTruncation(
        "rtl/vhdl_literal_width.vhd", core::HdlLanguage::Vhdl,
R"(entity vhdl_literal_width is
  port (q0, q1, q2 : out bit_vector(7 downto 0));
end entity;
architecture rtl of vhdl_literal_width is begin
  process(all) begin
    q0 <= "101010101010";
    q1 <= (("111111111"));
    q2 <= "00000000";
  end process;
end architecture;
)");
    const auto vhdlLiteralWidthResult =
        analyzer.analyzeSource(vhdlLiteralWidthTruncation);
    test.check(countRule(vhdlLiteralWidthResult,
                         "RTL-WIDTH-TRUNCATION") == 2 &&
                   hasRule(vhdlLiteralWidthResult,
                           "RTL-WIDTH-TRUNCATION", 6) &&
                   hasRule(vhdlLiteralWidthResult,
                           "RTL-WIDTH-TRUNCATION", 7),
               "VHDL direct string literals wider than fixed vector targets are reported with redundant parentheses supported");

    const core::SourceFile verilogSelectedTargetWidth(
        "rtl/verilog_selected_target_width.sv", core::HdlLanguage::SystemVerilog,
R"(module verilog_selected_target_width(input logic [2:0] idx, output logic [7:0] q0, q1, q2, q3);
always_comb begin
  q0[3:0] = 8'hff;
  q1[0] = 2'b10;
  q2[3:0] = 4'hf;
  q3[idx] = 2'b10;
end
endmodule
)");
    const auto verilogSelectedTargetWidthResult =
        analyzer.analyzeSource(verilogSelectedTargetWidth);
    test.check(countRule(verilogSelectedTargetWidthResult,
                         "RTL-WIDTH-TRUNCATION") == 2 &&
                   hasRule(verilogSelectedTargetWidthResult,
                           "RTL-WIDTH-TRUNCATION", 3) &&
                   hasRule(verilogSelectedTargetWidthResult,
                           "RTL-WIDTH-TRUNCATION", 4) &&
                   !hasRule(verilogSelectedTargetWidthResult,
                            "RTL-UNDRIVEN-OUTPUT"),
               "Verilog constant slice and bit assignment targets use selected widths while equal-width and dynamic targets remain excluded");

    const core::SourceFile vhdlSelectedTargetWidth(
        "rtl/vhdl_selected_target_width.vhd", core::HdlLanguage::Vhdl,
R"(entity vhdl_selected_target_width is
  port (idx : in integer; q0, q1, q2, q3 : out bit_vector(7 downto 0));
end entity;
architecture rtl of vhdl_selected_target_width is begin
  process(all) begin
    q0(3 downto 0) <= "11111111";
    q1(0) <= "10";
    q2(3 downto 0) <= "1111";
    q3(idx) <= "10";
  end process;
end architecture;
)");
    const auto vhdlSelectedTargetWidthResult =
        analyzer.analyzeSource(vhdlSelectedTargetWidth);
    test.check(countRule(vhdlSelectedTargetWidthResult,
                         "RTL-WIDTH-TRUNCATION") == 2 &&
                   hasRule(vhdlSelectedTargetWidthResult,
                           "RTL-WIDTH-TRUNCATION", 6) &&
                   hasRule(vhdlSelectedTargetWidthResult,
                           "RTL-WIDTH-TRUNCATION", 7) &&
                   !hasRule(vhdlSelectedTargetWidthResult,
                            "RTL-UNDRIVEN-OUTPUT"),
               "VHDL constant slice and index assignment targets use selected widths while equal-width and dynamic targets remain excluded");

    const core::SourceFile verilogSelectedSourceWidth(
        "rtl/verilog_selected_source_width.sv", core::HdlLanguage::SystemVerilog,
R"(module verilog_selected_source_width(input logic [15:0] data, input logic [3:0] idx,
  output logic [3:0] q0, q1, q2, output logic [1:0] q3);
always_comb begin
  q0 = data[7:0];
  q1 = data[3:0];
  q2 = data[idx +: 4];
  q3[1:0] = data[7:4];
end
endmodule
)");
    const auto verilogSelectedSourceWidthResult =
        analyzer.analyzeSource(verilogSelectedSourceWidth);
    test.check(countRule(verilogSelectedSourceWidthResult,
                         "RTL-WIDTH-TRUNCATION") == 2 &&
                   hasRule(verilogSelectedSourceWidthResult,
                           "RTL-WIDTH-TRUNCATION", 4) &&
                   hasRule(verilogSelectedSourceWidthResult,
                           "RTL-WIDTH-TRUNCATION", 7),
               "Verilog constant selected sources use their slice widths against direct and selected targets while equal-width and indexed part-select sources stay conservative");

    const core::SourceFile vhdlSelectedSourceWidth(
        "rtl/vhdl_selected_source_width.vhd", core::HdlLanguage::Vhdl,
R"(entity vhdl_selected_source_width is
  port (data : in bit_vector(15 downto 0); idx : in integer;
        q0, q1, q2 : out bit_vector(3 downto 0); q3 : out bit_vector(1 downto 0));
end entity;
architecture rtl of vhdl_selected_source_width is begin
  process(all) begin
    q0 <= data(7 downto 0);
    q1 <= data(3 downto 0);
    q2 <= data(idx downto idx - 3);
    q3(1 downto 0) <= data(7 downto 4);
  end process;
end architecture;
)");
    const auto vhdlSelectedSourceWidthResult =
        analyzer.analyzeSource(vhdlSelectedSourceWidth);
    test.check(countRule(vhdlSelectedSourceWidthResult,
                         "RTL-WIDTH-TRUNCATION") == 2 &&
                   hasRule(vhdlSelectedSourceWidthResult,
                           "RTL-WIDTH-TRUNCATION", 7) &&
                   hasRule(vhdlSelectedSourceWidthResult,
                           "RTL-WIDTH-TRUNCATION", 10),
               "VHDL constant selected sources use their slice widths against direct and selected targets while equal-width and dynamic slices stay conservative");

    const core::SourceFile verilogConcatenationWidth(
        "rtl/verilog_concatenation_width.sv", core::HdlLanguage::SystemVerilog,
R"(module verilog_concatenation_width(input logic [3:0] a, input logic [1:0] b, input logic [1:0] idx,
  output logic [3:0] q0, q2, output logic [5:0] q1, output logic [3:0] q3);
always_comb begin
  q0 = {a, b};
  q1 = {a, b};
  q2 = {{2'b01}, b};
  q3[1:0] = {a[1:0], b};
  q3 = {2{b}};
  q2 = {3{b}};
  q2 = {a[idx], b};
end
endmodule
)");
    const auto verilogConcatenationWidthResult =
        analyzer.analyzeSource(verilogConcatenationWidth);
    test.check(countRule(verilogConcatenationWidthResult,
                         "RTL-WIDTH-TRUNCATION") == 3 &&
                   hasRule(verilogConcatenationWidthResult,
                           "RTL-WIDTH-TRUNCATION", 4) &&
                   hasRule(verilogConcatenationWidthResult,
                           "RTL-WIDTH-TRUNCATION", 7) &&
                   hasRule(verilogConcatenationWidthResult,
                           "RTL-WIDTH-TRUNCATION", 9),
               "Verilog flat, nested, and constant replication concatenation widths are compared with direct and selected targets while equal-width and dynamic members stay conservative");

    const core::SourceFile vhdlConcatenationWidth(
        "rtl/vhdl_concatenation_width.vhd", core::HdlLanguage::Vhdl,
R"(entity vhdl_concatenation_width is
  port (a : in bit_vector(3 downto 0); b : in bit_vector(1 downto 0); idx : in integer;
        q0, q2 : out bit_vector(3 downto 0); q1 : out bit_vector(5 downto 0);
        q3 : out bit_vector(1 downto 0));
end entity;
architecture rtl of vhdl_concatenation_width is begin
  process(all) begin
    q0 <= a & b;
    q1 <= a & b;
    q2 <= ("01" & b);
    q3 <= a(1 downto 0) & b;
    q3 <= a(1 downto 0) & '0';
    q3 <= a(1 downto 0) & 'X';
    q2 <= a(2 downto 0) & '0';
    q2 <= a(idx) & b;
  end process;
end architecture;
)");
    const auto vhdlConcatenationWidthResult =
        analyzer.analyzeSource(vhdlConcatenationWidth);
    test.check(countRule(vhdlConcatenationWidthResult,
                         "RTL-WIDTH-TRUNCATION") == 4 &&
                   hasRule(vhdlConcatenationWidthResult,
                           "RTL-WIDTH-TRUNCATION", 8) &&
                   hasRule(vhdlConcatenationWidthResult,
                           "RTL-WIDTH-TRUNCATION", 11) &&
                   hasRule(vhdlConcatenationWidthResult,
                           "RTL-WIDTH-TRUNCATION", 12) &&
                   hasRule(vhdlConcatenationWidthResult,
                           "RTL-WIDTH-TRUNCATION", 13),
               "VHDL static concatenation widths include string and scalar logic literals while equal-width and dynamic selected members stay conservative");

    const core::SourceFile verilogComparisonWidth(
        "rtl/verilog_comparison_width.sv", core::HdlLanguage::SystemVerilog,
R"(module verilog_comparison_width(input logic [3:0] a, c, input logic [7:0] b,
  input logic en, output logic q); integer i;
always_comb begin
  q = 1'b0;
  if (a == b) q = 1'b1;
  if (a != c) q = 1'b1;
  if (en && (a[3:2] === b[1:0])) q = 1'b1;
  if (en && (a[1:0] !== b[3:0])) q = 1'b1;
  if ((a + c) == b) q = 1'b1;
  if (a == 8'h0f) q = 1'b1;
  if (a == 4'hf) q = 1'b1;
  if (a == {b[3:0], c}) q = 1'b1;
  if (a == {c[1:0], c[1:0]}) q = 1'b1;
  if (a < b) q = 1'b1;
  if (a <= c) q = 1'b1;
  if (a[1:0] >= b[3:0]) q = 1'b1;
  if ((a << 1) > b) q = 1'b1;
  while (a < b) q = 1'b1;
  for (i = 0; a[1:0] >= b[3:0]; i = i + 1) q = 1'b1;
  if (!(a == b)) q = 1'b1;
  if (!(a == c)) q = 1'b1;
  if (a ==? b) q = 1'b1;
  if (a !=? c) q = 1'b1;
  q = (a < b) ? 1'b1 : 1'b0;
  q = (a <= c) ? 1'b1 : 1'b0;
  q = a < b ? 1'b1 : 1'b0;
  q = a <= c ? 1'b1 : 1'b0;
  q = en ? 1'b0 : a < b ? 1'b1 : 1'b0;
  q = en ? a <= c ? 1'b1 : 1'b0 : 1'b0;
  if (a < b ? en : 1'b0) q = 1'b1;
  if (a <= c ? en : 1'b0) q = 1'b1;
end
endmodule
)");
    const auto verilogComparisonWidthResult =
        analyzer.analyzeSource(verilogComparisonWidth);
    test.check(countRule(verilogComparisonWidthResult,
                         "RTL-COMPARISON-WIDTH-MISMATCH") == 15 &&
                   hasRule(verilogComparisonWidthResult,
                           "RTL-COMPARISON-WIDTH-MISMATCH", 5) &&
                   hasRule(verilogComparisonWidthResult,
                           "RTL-COMPARISON-WIDTH-MISMATCH", 8) &&
                   hasRule(verilogComparisonWidthResult,
                           "RTL-COMPARISON-WIDTH-MISMATCH", 9) &&
                   hasRule(verilogComparisonWidthResult,
                           "RTL-COMPARISON-WIDTH-MISMATCH", 10) &&
                   hasRule(verilogComparisonWidthResult,
                           "RTL-COMPARISON-WIDTH-MISMATCH", 12) &&
                   hasRule(verilogComparisonWidthResult,
                           "RTL-COMPARISON-WIDTH-MISMATCH", 14) &&
                   hasRule(verilogComparisonWidthResult,
                           "RTL-COMPARISON-WIDTH-MISMATCH", 16) &&
                   hasRule(verilogComparisonWidthResult,
                           "RTL-COMPARISON-WIDTH-MISMATCH", 18) &&
                   hasRule(verilogComparisonWidthResult,
                           "RTL-COMPARISON-WIDTH-MISMATCH", 19) &&
                   hasRule(verilogComparisonWidthResult,
                           "RTL-COMPARISON-WIDTH-MISMATCH", 20) &&
                   hasRule(verilogComparisonWidthResult,
                           "RTL-COMPARISON-WIDTH-MISMATCH", 22) &&
                   hasRule(verilogComparisonWidthResult,
                           "RTL-COMPARISON-WIDTH-MISMATCH", 24) &&
                   hasRule(verilogComparisonWidthResult,
                           "RTL-COMPARISON-WIDTH-MISMATCH", 26) &&
                   !hasRule(verilogComparisonWidthResult,
                            "RTL-COMPARISON-WIDTH-MISMATCH", 27) &&
                   hasRule(verilogComparisonWidthResult,
                           "RTL-COMPARISON-WIDTH-MISMATCH", 28) &&
                   !hasRule(verilogComparisonWidthResult,
                            "RTL-COMPARISON-WIDTH-MISMATCH", 29) &&
                   hasRule(verilogComparisonWidthResult,
                           "RTL-COMPARISON-WIDTH-MISMATCH", 30) &&
                   !hasRule(verilogComparisonWidthResult,
                            "RTL-COMPARISON-WIDTH-MISMATCH", 31) &&
                   !hasRule(verilogComparisonWidthResult, "RTL-MULTIPLE-DRIVERS"),
               "Verilog comparison widths cover flag-zero nested arithmetic, branch, loop and control-context ternary guards plus negated and wildcard equality forms");

    const core::SourceFile verilogContinuousTernaryComparisonWidth(
        "rtl/verilog_continuous_ternary_comparison_width.sv",
        core::HdlLanguage::SystemVerilog,
R"(module verilog_continuous_ternary_comparison_width(
  input logic [3:0] a, c, input logic [7:0] b,
  output logic mismatch, equal_width);
assign mismatch = a < b ? 1'b1 : 1'b0;
assign equal_width = a <= c ? 1'b1 : 1'b0;
endmodule
)");
    const auto verilogContinuousTernaryComparisonWidthResult =
        analyzer.analyzeSource(verilogContinuousTernaryComparisonWidth);
    test.check(countRule(verilogContinuousTernaryComparisonWidthResult,
                         "RTL-COMPARISON-WIDTH-MISMATCH") == 1 &&
                   hasRule(verilogContinuousTernaryComparisonWidthResult,
                           "RTL-COMPARISON-WIDTH-MISMATCH", 4),
               "Verilog continuous assignments expose unparenthesized ternary comparison guards without reporting equal-width guards");

    const core::SourceFile verilogBitwiseWidth(
        "rtl/verilog_bitwise_width.sv", core::HdlLanguage::SystemVerilog,
R"(module verilog_bitwise_width(
  input logic [7:0] a, input logic [5:0] b,
  output logic [7:0] mismatch, equal_slice, equal_literal);
assign mismatch = a & b;
assign equal_slice = a[5:0] | b;
assign equal_literal = a[3:0] ^ 4'h0;
always_comb if (a ~^ b) mismatch = a;
endmodule
)");
    const auto verilogBitwiseWidthResult = analyzer.analyzeSource(verilogBitwiseWidth);
    test.check(countRule(verilogBitwiseWidthResult,
                         "RTL-BITWISE-WIDTH-MISMATCH") == 2 &&
                   hasRule(verilogBitwiseWidthResult,
                           "RTL-BITWISE-WIDTH-MISMATCH", 4) &&
                   hasRule(verilogBitwiseWidthResult,
                           "RTL-BITWISE-WIDTH-MISMATCH", 7),
               "binary bitwise width checks cover continuous assignments and process conditions while accepting equal slices and literals");

    const core::SourceFile verilogMixedSignednessArithmetic(
        "rtl/verilog_mixed_signedness_arithmetic.sv",
        core::HdlLanguage::SystemVerilog,
R"(module verilog_mixed_signedness_arithmetic(
  input logic clk, input logic signed [3:0] s, input logic [3:0] u,
  output logic signed [7:0] q);
always_ff @(posedge clk) begin
  q <= s * u;
  q <= s + s;
  q <= u - u;
  q <= s & u;
end
assign q = (s) / (u);
endmodule
)" );
    const auto verilogMixedSignednessArithmeticResult =
        analyzer.analyzeSource(verilogMixedSignednessArithmetic);
    test.check(countRule(verilogMixedSignednessArithmeticResult,
                         "RTL-MIXED-SIGNEDNESS-ARITHMETIC") == 2 &&
                   hasRule(verilogMixedSignednessArithmeticResult,
                           "RTL-MIXED-SIGNEDNESS-ARITHMETIC", 5) &&
                   hasRule(verilogMixedSignednessArithmeticResult,
                           "RTL-MIXED-SIGNEDNESS-ARITHMETIC", 10),
               "mixed signedness arithmetic reports multiplication and division while same-type and bitwise expressions stay clear");

    const core::SourceFile verilogNonblockingTernaryComparisonWidth(
        "rtl/verilog_nonblocking_ternary_comparison_width.sv",
        core::HdlLanguage::SystemVerilog,
R"(module verilog_nonblocking_ternary_comparison_width(
  input logic clk, input logic [3:0] a, c, input logic [7:0] b,
  output logic mismatch, equal_width);
always_ff @(posedge clk) begin
  mismatch <= a < b ? 1'b1 : 1'b0;
  equal_width <= a <= c ? 1'b1 : 1'b0;
end
endmodule
)");
    const auto verilogNonblockingTernaryComparisonWidthResult =
        analyzer.analyzeSource(verilogNonblockingTernaryComparisonWidth);
    test.check(countRule(verilogNonblockingTernaryComparisonWidthResult,
                         "RTL-COMPARISON-WIDTH-MISMATCH") == 1 &&
                   hasRule(verilogNonblockingTernaryComparisonWidthResult,
                           "RTL-COMPARISON-WIDTH-MISMATCH", 5) &&
                   !hasRule(verilogNonblockingTernaryComparisonWidthResult,
                            "VL-COMB-NONBLOCKING") &&
                   !hasRule(verilogNonblockingTernaryComparisonWidthResult,
                            "VL-MIXED-ASSIGNMENT-STYLE"),
               "SystemVerilog nonblocking assignments remain distinct from relational operators in unparenthesized ternary guards");

    const core::SourceFile vhdlComparisonWidth(
        "rtl/vhdl_comparison_width.vhd", core::HdlLanguage::Vhdl,
R"(entity vhdl_comparison_width is
  port (a, c : in bit_vector(3 downto 0); b : in bit_vector(7 downto 0);
        en : in bit; q : out bit);
end entity;
architecture rtl of vhdl_comparison_width is begin
  process(all) begin
    q <= '0';
    if a = b then q <= '1'; end if;
    if a /= c then q <= '1'; end if;
    if en = '1' and a(3 downto 2) = b(1 downto 0) then q <= '1'; end if;
    if a(1 downto 0) /= b(3 downto 0) then q <= '1'; end if;
    if (a & c) = b then q <= '1'; end if;
    if a = "00000000" then q <= '1'; end if;
    if a = "0000" then q <= '1'; end if;
    if a = (c & c) then q <= '1'; end if;
    if a < b then q <= '1'; end if;
    if a <= c then q <= '1'; end if;
    if a(1 downto 0) >= b(3 downto 0) then q <= '1'; end if;
    if (a & c) > b then q <= '1'; end if;
    while a < b loop
      q <= '1'; exit;
    end loop;
    while a(1 downto 0) >= b(3 downto 0) loop
      q <= '1'; exit;
    end loop;
    if not (a = b) then q <= '1'; end if;
    if not (a = c) then q <= '1'; end if;
    q <= '1' when a < b else '0';
    q <= '1' when a <= c else '0';
  end process;
end architecture;
)");
    const auto vhdlComparisonWidthResult = analyzer.analyzeSource(vhdlComparisonWidth);
    test.check(countRule(vhdlComparisonWidthResult,
                         "RTL-COMPARISON-WIDTH-MISMATCH") == 10 &&
                   hasRule(vhdlComparisonWidthResult,
                           "RTL-COMPARISON-WIDTH-MISMATCH", 8) &&
                   hasRule(vhdlComparisonWidthResult,
                           "RTL-COMPARISON-WIDTH-MISMATCH", 11) &&
                   hasRule(vhdlComparisonWidthResult,
                           "RTL-COMPARISON-WIDTH-MISMATCH", 13) &&
                   hasRule(vhdlComparisonWidthResult,
                           "RTL-COMPARISON-WIDTH-MISMATCH", 15) &&
                   hasRule(vhdlComparisonWidthResult,
                           "RTL-COMPARISON-WIDTH-MISMATCH", 16) &&
                   hasRule(vhdlComparisonWidthResult,
                           "RTL-COMPARISON-WIDTH-MISMATCH", 18) &&
                   hasRule(vhdlComparisonWidthResult,
                           "RTL-COMPARISON-WIDTH-MISMATCH", 20) &&
                   hasRule(vhdlComparisonWidthResult,
                           "RTL-COMPARISON-WIDTH-MISMATCH", 23) &&
                   hasRule(vhdlComparisonWidthResult,
                           "RTL-COMPARISON-WIDTH-MISMATCH", 26) &&
                   hasRule(vhdlComparisonWidthResult,
                           "RTL-COMPARISON-WIDTH-MISMATCH", 28) &&
                   !hasRule(vhdlComparisonWidthResult, "RTL-INPUT-DRIVEN") &&
                   !hasRule(vhdlComparisonWidthResult, "RTL-COMBINATIONAL-LOOP"),
               "VHDL equality and relational comparisons cover branch, loop, and when-else guards plus whole-condition not");

    const core::SourceFile verilogOutOfRange(
        "rtl/verilog_out_of_range.sv", core::HdlLanguage::SystemVerilog,
R"(module verilog_out_of_range(input logic [15:8] data,
  output logic bit_o, output logic [3:0] slice_o, output logic [3:0] safe_o);
assign bit_o = data[5'h10];
assign slice_o = data[12:7];
assign safe_o = data[15:12];
endmodule
)");
    const auto verilogOutOfRangeResult = analyzer.analyzeSource(verilogOutOfRange);
    test.check(countRule(verilogOutOfRangeResult, "RTL-OUT-OF-RANGE-SELECT") == 2,
               "Verilog based index and slice boundary are checked against nonzero bounds");
    const auto verilogSelectFinding = std::ranges::find_if(
        verilogOutOfRangeResult.violations, [](const auto& violation) {
            return violation.ruleId == "RTL-OUT-OF-RANGE-SELECT";
        });
    test.check(verilogSelectFinding != verilogOutOfRangeResult.violations.end() &&
                   verilogSelectFinding->severity == analysis::Severity::Error &&
                   verilogSelectFinding->module == "verilog_out_of_range" &&
                   verilogSelectFinding->signal == "data" &&
                   verilogSelectFinding->line == 3,
               "Verilog out-of-range finding retains error severity and source context");

    const core::SourceFile signedOutOfRange(
        "rtl/signed_out_of_range.sv", core::HdlLanguage::SystemVerilog,
R"(module signed_out_of_range(input logic [3:1] data, output wire [3:0] out);
assign out[0] = data[-1];
assign out[1] = data[+1];
assign out[2] = data[3];
assign out[3] = data[-1:2];
endmodule
)");
    test.check(countRule(analyzer.analyzeSource(signedOutOfRange),
                         "RTL-OUT-OF-RANGE-SELECT") == 2,
               "negative index and slice boundary are rejected while exact endpoints remain valid");

    const core::SourceFile immutableIndexSelect(
        "rtl/immutable_index_select.sv", core::HdlLanguage::SystemVerilog,
R"(module immutable_index_select(input logic [3:1] data,
                              output wire bad, good, sliced);
localparam integer BAD_INDEX = 4;
localparam integer GOOD_INDEX = 1;
assign bad = data[BAD_INDEX];
assign good = data[GOOD_INDEX];
assign sliced = data[BAD_INDEX:2];
endmodule
)");
    test.check(countRule(analyzer.analyzeSource(immutableIndexSelect),
                         "RTL-OUT-OF-RANGE-SELECT") == 2,
               "direct numeric module localparams resolve in bit and slice bounds checks");

    const core::SourceFile uncertainIndexSelect(
        "rtl/uncertain_index_select.sv", core::HdlLanguage::SystemVerilog,
R"(module uncertain_index_select #(parameter OVERRIDABLE = 4)(
    input logic [3:1] data, input logic [2:0] runtime_index,
    output wire a, b, c);
localparam integer SHADOWED = 4;
always_comb begin
  integer SHADOWED;
  SHADOWED = 2;
end
assign a = data[OVERRIDABLE];
assign b = data[runtime_index];
assign c = data[SHADOWED];
endmodule
)");
    test.check(!hasRule(analyzer.analyzeSource(uncertainIndexSelect),
                        "RTL-OUT-OF-RANGE-SELECT"),
               "overridable parameters, runtime indices and shadowed names stay unresolved");

    const core::SourceFile vhdlOutOfRange(
        "rtl/vhdl_out_of_range.vhd", core::HdlLanguage::Vhdl,
R"(entity vhdl_out_of_range is
  port (data : in std_logic_vector(8 to 15); bit_o : out std_logic;
        slice_o, safe_o : out std_logic_vector(3 downto 0));
end entity;
architecture rtl of vhdl_out_of_range is begin
  bit_o <= data(7);
  slice_o <= data(16 downto 13);
  safe_o <= data(8 to 11);
end architecture;
)");
    const auto vhdlOutOfRangeResult = analyzer.analyzeSource(vhdlOutOfRange);
    test.check(countRule(vhdlOutOfRangeResult, "RTL-OUT-OF-RANGE-SELECT") == 2,
               "VHDL index and slice boundaries are checked against ascending bounds");

    const core::SourceFile unresolvedSelects(
        "rtl/unresolved_selects.sv", core::HdlLanguage::SystemVerilog,
R"(module unresolved_selects #(parameter WIDTH = 8)(
  input logic [WIDTH-1:0] data, input logic [2:0] index_value, output logic q);
assign q = data[index_value];
endmodule
)");
    test.check(!hasRule(analyzer.analyzeSource(unresolvedSelects),
                        "RTL-OUT-OF-RANGE-SELECT"),
               "dynamic indices and unresolved parameterized ranges are not guessed");

    const core::SourceFile verilogDerivedClock(
        "rtl/verilog_derived_clock.sv", core::HdlLanguage::SystemVerilog,
R"(module verilog_derived_clock(input logic clk, input logic enable, input logic d,
                                output logic q);
logic gated_clk;
assign gated_clk = clk & enable;
always_ff @(posedge gated_clk) q <= d;
endmodule
)");
    const auto verilogDerivedClockResult = analyzer.analyzeSource(verilogDerivedClock);
    test.check(countRule(verilogDerivedClockResult, "RTL-DERIVED-CLOCK") == 1,
               "Verilog locally gated clock is reported once");
    const auto derivedClockFinding = std::ranges::find_if(verilogDerivedClockResult.violations,
        [](const auto& violation) { return violation.ruleId == "RTL-DERIVED-CLOCK"; });
    test.check(derivedClockFinding != verilogDerivedClockResult.violations.end() &&
                   derivedClockFinding->severity == analysis::Severity::Warning &&
                   derivedClockFinding->module == "verilog_derived_clock" &&
                   derivedClockFinding->signal == "gated_clk" &&
                   derivedClockFinding->line == 4,
               "derived-clock finding points to the local driver with unit and signal context");

    const core::SourceFile directAndPrimitiveClocks(
        "rtl/direct_and_primitive_clocks.sv", core::HdlLanguage::SystemVerilog,
R"(module direct_and_primitive_clocks(input logic clk, input logic d,
                                      output logic q_input, output logic q_primitive);
logic primitive_clk;
vendor_clock u_clock(.clock_out(primitive_clk));
always_ff @(posedge clk) q_input <= d;
always_ff @(posedge primitive_clk) q_primitive <= d;
endmodule
)");
    test.check(!hasRule(analyzer.analyzeSource(directAndPrimitiveClocks), "RTL-DERIVED-CLOCK"),
               "input clocks and instance-connected clock primitive outputs are not locally derived");

    const core::SourceFile vhdlDerivedClock(
        "rtl/vhdl_derived_clock.vhd", core::HdlLanguage::Vhdl,
R"(entity vhdl_derived_clock is
  port (clk, enable, d : in std_logic; q : out std_logic);
end entity;
architecture rtl of vhdl_derived_clock is
  signal gated_clk : std_logic;
begin
  gated_clk <= clk and enable;
  process(gated_clk) begin
    if rising_edge(gated_clk) then q <= d; end if;
  end process;
end architecture;
)");
    test.check(countRule(analyzer.analyzeSource(vhdlDerivedClock),
                         "RTL-DERIVED-CLOCK") == 1,
               "VHDL locally gated clock is reported once");

    const core::SourceFile verilogDelays(
        "rtl/verilog_delays.sv", core::HdlLanguage::SystemVerilog,
R"(module delay_child #(parameter WIDTH = 1)(input logic d, output logic q);
assign q = d;
endmodule
module verilog_delays(input logic d, output logic q, output logic delayed_wire);
delay_child #(.WIDTH(1)) u_child(.d(d), .q());
always begin
  #5 q = d;
end
assign #2 delayed_wire = d;
endmodule
)");
    const auto verilogDelayResult = analyzer.analyzeSource(verilogDelays);
    test.check(countRule(verilogDelayResult, "RTL-NONSYNTHESIZABLE-DELAY") == 2,
               "Verilog procedural and continuous delays are reported without counting parameter overrides");
    bool verilogDelayContext = true;
    for (const auto& violation : verilogDelayResult.violations) {
        if (violation.ruleId == "RTL-NONSYNTHESIZABLE-DELAY") {
            verilogDelayContext = verilogDelayContext &&
                                   violation.module == "verilog_delays" &&
                                   violation.severity == analysis::Severity::Warning;
        }
    }
    test.check(verilogDelayContext,
               "Verilog delay findings retain warning severity and design-unit context");

    const core::SourceFile parameterOverrideOnly(
        "rtl/parameter_override_only.sv", core::HdlLanguage::SystemVerilog,
R"(module parameter_override_only(input logic d, output logic q);
delay_child #(.WIDTH(4)) u_child(.d(d), .q(q));
endmodule
)");
    test.check(!hasRule(analyzer.analyzeSource(parameterOverrideOnly),
                        "RTL-NONSYNTHESIZABLE-DELAY"),
               "module instance parameter override is not interpreted as a delay");

    const core::SourceFile vhdlDelay(
        "rtl/vhdl_delay.vhd", core::HdlLanguage::Vhdl,
R"(entity vhdl_delay is
  port (d : in std_logic; q : out std_logic);
end entity;
architecture rtl of vhdl_delay is begin
  q <= d after 5 ns;
end architecture;
)");
    test.check(countRule(analyzer.analyzeSource(vhdlDelay),
                         "RTL-NONSYNTHESIZABLE-DELAY") == 1,
               "VHDL after-clause simulation delay is reported");

    const core::SourceFile verilogDuplicateCase(
        "rtl/verilog_duplicate_case.sv", core::HdlLanguage::SystemVerilog,
R"(module verilog_duplicate_case(input logic [1:0] select_value,
                                  input logic nested_select, output logic q);
always_comb begin
  case (select_value)
    2'b00, 2'b01: q = 1'b0;
    2'b01: q = 1'b1;
    default: q = 1'b0;
  endcase
  case (nested_select)
    1'b0: begin
      case (select_value)
        2'b00: q = 1'b0;
        2'b01: q = 1'b1;
        default: q = 1'b0;
      endcase
    end
    1'b1: q = 1'b1;
  endcase
end
endmodule
)");
    const auto verilogDuplicateCaseResult = analyzer.analyzeSource(verilogDuplicateCase);
    test.check(countRule(verilogDuplicateCaseResult, "RTL-DUPLICATE-CASE-ITEM") == 1,
               "Verilog duplicate comma-separated choice is reported only within its own case");
    const auto duplicateCaseFinding = std::ranges::find_if(verilogDuplicateCaseResult.violations,
        [](const auto& violation) { return violation.ruleId == "RTL-DUPLICATE-CASE-ITEM"; });
    test.check(duplicateCaseFinding != verilogDuplicateCaseResult.violations.end() &&
                   duplicateCaseFinding->severity == analysis::Severity::Warning &&
                   duplicateCaseFinding->module == "verilog_duplicate_case" &&
                   duplicateCaseFinding->line == 6,
               "Verilog duplicate choice points to the later branch with unit context");

    const core::SourceFile vhdlDuplicateCase(
        "rtl/vhdl_duplicate_case.vhd", core::HdlLanguage::Vhdl,
R"(entity vhdl_duplicate_case is
  port (select_value : in std_logic_vector(1 downto 0); q : out std_logic);
end entity;
architecture rtl of vhdl_duplicate_case is begin
  process(select_value) begin
    case select_value is
      when "00" | "01" => q <= '0';
      when "01" => q <= '1';
      when others => q <= '0';
    end case;
  end process;
end architecture;
)");
    test.check(countRule(analyzer.analyzeSource(vhdlDuplicateCase),
                         "RTL-DUPLICATE-CASE-ITEM") == 1,
               "VHDL duplicate multi-choice item is reported once");

    const core::SourceFile verilogMultipleCaseDefaults(
        "rtl/verilog_multiple_case_defaults.sv", core::HdlLanguage::SystemVerilog,
R"(module verilog_multiple_case_defaults(input logic sel, output logic q);
always_comb begin
  q = 1'b0;
  case (sel)
    1'b0: q = 1'b0;
    default: q = 1'b1;
    default: q = 1'b0;
  endcase
  case (sel)
    1'b0: case (sel) default: q = 1'b0; endcase
    default: q = 1'b1;
  endcase
end
endmodule
)");
    const auto verilogMultipleDefaultResult = analyzer.analyzeSource(
        verilogMultipleCaseDefaults);
    test.check(countRule(verilogMultipleDefaultResult,
                         "RTL-MULTIPLE-CASE-DEFAULT") == 1 &&
                   hasRule(verilogMultipleDefaultResult,
                           "RTL-MULTIPLE-CASE-DEFAULT", 7),
               "Verilog duplicate default is reported within one case without crossing nested case scope");

    const core::SourceFile vhdlMultipleCaseDefaults(
        "rtl/vhdl_multiple_case_defaults.vhd", core::HdlLanguage::Vhdl,
R"(entity vhdl_multiple_case_defaults is
  port (sel : in std_logic; q : out std_logic);
end entity;
architecture rtl of vhdl_multiple_case_defaults is begin
  process(all) begin
    case sel is
      when '0' => q <= '0';
      when others => q <= '1';
      when others => q <= '0';
    end case;
  end process;
end architecture;
)");
    const auto vhdlMultipleDefaultResult = analyzer.analyzeSource(
        vhdlMultipleCaseDefaults);
    test.check(countRule(vhdlMultipleDefaultResult,
                         "RTL-MULTIPLE-CASE-DEFAULT") == 1 &&
                   hasRule(vhdlMultipleDefaultResult,
                           "RTL-MULTIPLE-CASE-DEFAULT", 9),
               "VHDL duplicate when others is reported at the later catch-all choice");

    const core::SourceFile verilogCaseWidthMismatch(
        "rtl/verilog_case_width_mismatch.sv", core::HdlLanguage::SystemVerilog,
R"(module verilog_case_width_mismatch(input logic [3:0] sel, output logic q);
always_comb begin
  q = 1'b0;
  case (sel)
    3'b000: q = 1'b0;
    4'b0001: q = 1'b1;
    5'h02: q = 1'b0;
    0: q = 1'b1;
    default: q = 1'b0;
  endcase
  case (sel[2:0])
    2'b00: q = 1'b0;
    3'b001: q = 1'b1;
    default: q = 1'b1;
  endcase
  case (sel[0])
    2'b00: q = 1'b0;
    1'b1: q = 1'b1;
    default: q = 1'b0;
  endcase
end
endmodule
)");
    const auto verilogCaseWidthResult = analyzer.analyzeSource(
        verilogCaseWidthMismatch);
    test.check(countRule(verilogCaseWidthResult,
                         "RTL-CASE-ITEM-WIDTH-MISMATCH") == 4 &&
                   hasRule(verilogCaseWidthResult,
                           "RTL-CASE-ITEM-WIDTH-MISMATCH", 5) &&
                   hasRule(verilogCaseWidthResult,
                           "RTL-CASE-ITEM-WIDTH-MISMATCH", 7) &&
                   hasRule(verilogCaseWidthResult,
                           "RTL-CASE-ITEM-WIDTH-MISMATCH", 12) &&
                   hasRule(verilogCaseWidthResult,
                           "RTL-CASE-ITEM-WIDTH-MISMATCH", 17),
               "Verilog explicitly sized case items are compared with direct, slice, and bit selectors while unsized choices remain conservative");

    const core::SourceFile vhdlCaseWidthMismatch(
        "rtl/vhdl_case_width_mismatch.vhd", core::HdlLanguage::Vhdl,
R"(entity vhdl_case_width_mismatch is
  port (sel : in std_logic_vector(3 downto 0); q : out std_logic);
end entity;
architecture rtl of vhdl_case_width_mismatch is begin
  process(all) begin
    case sel is
      when "000" => q <= '0';
      when "0001" => q <= '1';
      when "00010" => q <= '0';
      when others => q <= '0';
    end case;
    case sel(2 downto 0) is
      when "00" => q <= '0';
      when "001" => q <= '1';
      when others => q <= '0';
    end case;
    case sel(0) is
      when "00" => q <= '0';
      when "1" => q <= '1';
      when others => q <= '0';
    end case;
  end process;
end architecture;
)");
    const auto vhdlCaseWidthResult = analyzer.analyzeSource(vhdlCaseWidthMismatch);
    test.check(countRule(vhdlCaseWidthResult,
                         "RTL-CASE-ITEM-WIDTH-MISMATCH") == 2 &&
                   hasRule(vhdlCaseWidthResult,
                           "RTL-CASE-ITEM-WIDTH-MISMATCH", 7) &&
                   hasRule(vhdlCaseWidthResult,
                           "RTL-CASE-ITEM-WIDTH-MISMATCH", 9) &&
                   !hasRule(vhdlCaseWidthResult,
                            "RTL-CASE-ITEM-WIDTH-MISMATCH", 13) &&
                   !hasRule(vhdlCaseWidthResult,
                            "RTL-CASE-ITEM-WIDTH-MISMATCH", 18),
               "native VHDL case-width branch admits direct identifier selectors only");

    const core::SourceFile verilogCaseValueOutOfRange(
        "rtl/verilog_case_value_out_of_range.sv", core::HdlLanguage::SystemVerilog,
R"(module verilog_case_value_out_of_range #(parameter P = 7)(input logic [1:0] sel, output logic q);
always_comb begin
  case (sel)
    0: q = 1'b0;
    3: q = 1'b1;
    4: q = 1'b0;
    5, 2'd2: q = 1'b1;
    -1: q = 1'b0;
    P: q = 1'b1;
    3'd4: q = 1'b0;
    default: q = 1'b0;
  endcase
  case (sel[0])
    0: q = 1'b0;
    2: q = 1'b1;
    default: q = 1'b0;
  endcase
  case (sel[1:0])
    4: q = 1'b0;
    default: q = 1'b1;
  endcase
end
endmodule
)");
    const auto verilogCaseValueOutOfRangeResult =
        analyzer.analyzeSource(verilogCaseValueOutOfRange);
    test.check(countRule(verilogCaseValueOutOfRangeResult,
                         "VL-CASE-ITEM-VALUE-OUT-OF-RANGE") == 4 &&
                   hasRule(verilogCaseValueOutOfRangeResult,
                           "VL-CASE-ITEM-VALUE-OUT-OF-RANGE", 6) &&
                   hasRule(verilogCaseValueOutOfRangeResult,
                           "VL-CASE-ITEM-VALUE-OUT-OF-RANGE", 7) &&
                   hasRule(verilogCaseValueOutOfRangeResult,
                           "VL-CASE-ITEM-VALUE-OUT-OF-RANGE", 15) &&
                   hasRule(verilogCaseValueOutOfRangeResult,
                           "VL-CASE-ITEM-VALUE-OUT-OF-RANGE", 19),
               "Unsized nonnegative case values outside direct, bit, and slice selector ranges are reported while negative, parameter, explicitly sized, and representable choices stay conservative");

    const core::SourceFile verilogShiftAmounts(
        "rtl/verilog_shift_amounts.sv", core::HdlLanguage::SystemVerilog,
R"(module verilog_shift_amounts(input logic [7:0] data, output logic [7:0] q);
always_comb begin
  q = data << 8;
  q = data >> 7;
  q = data[3:0] << 4;
  q = data[0] >>> 1;
  q = data[3:0] << 3;
  q <<= 8;
end
endmodule
)");
    const auto verilogShiftAmountResult = analyzer.analyzeSource(verilogShiftAmounts);
    test.check(countRule(verilogShiftAmountResult,
                         "RTL-SHIFT-AMOUNT-OUT-OF-RANGE") == 4 &&
                   hasRule(verilogShiftAmountResult,
                           "RTL-SHIFT-AMOUNT-OUT-OF-RANGE", 3) &&
                   hasRule(verilogShiftAmountResult,
                           "RTL-SHIFT-AMOUNT-OUT-OF-RANGE", 5) &&
                   hasRule(verilogShiftAmountResult,
                           "RTL-SHIFT-AMOUNT-OUT-OF-RANGE", 6) &&
                   hasRule(verilogShiftAmountResult,
                           "RTL-SHIFT-AMOUNT-OUT-OF-RANGE", 8),
               "Verilog direct, sliced, indexed, arithmetic, and compound constant shifts are checked against operand width");

    const core::SourceFile vhdlShiftAmounts(
        "rtl/vhdl_shift_amounts.vhd", core::HdlLanguage::Vhdl,
R"(entity vhdl_shift_amounts is
  port (data : in unsigned(7 downto 0); q : out unsigned(7 downto 0));
end entity;
architecture rtl of vhdl_shift_amounts is begin
  process(all) begin
    q <= data sll 8;
    q <= data srl 7;
    q <= data(3 downto 0) sll 4;
    q(0) <= data(0) sra 1;
    q <= shift_left(data, 8);
    q <= shift_right(data, 7);
    q(3 downto 0) <= shift_left(data(3 downto 0), 4);
  end process;
end architecture;
)");
    const auto vhdlShiftAmountResult = analyzer.analyzeSource(vhdlShiftAmounts);
    test.check(countRule(vhdlShiftAmountResult,
                         "RTL-SHIFT-AMOUNT-OUT-OF-RANGE") == 5 &&
                   hasRule(vhdlShiftAmountResult,
                           "RTL-SHIFT-AMOUNT-OUT-OF-RANGE", 6) &&
                   hasRule(vhdlShiftAmountResult,
                           "RTL-SHIFT-AMOUNT-OUT-OF-RANGE", 8) &&
                   hasRule(vhdlShiftAmountResult,
                           "RTL-SHIFT-AMOUNT-OUT-OF-RANGE", 9) &&
                   hasRule(vhdlShiftAmountResult,
                           "RTL-SHIFT-AMOUNT-OUT-OF-RANGE", 10) &&
                   hasRule(vhdlShiftAmountResult,
                           "RTL-SHIFT-AMOUNT-OUT-OF-RANGE", 12),
               "VHDL infix and numeric_std-style direct, sliced, and indexed constant shifts are checked against operand width");

    const core::SourceFile verilogNegativeShifts(
        "rtl/verilog_negative_shifts.sv", core::HdlLanguage::SystemVerilog,
R"(module verilog_negative_shifts(input logic [7:0] data, output logic [7:0] q);
always_comb begin
  q = data << -1;
  q = data >> (-2);
  q >>>= -3;
  q = (data << -4) | data;
  q = data << 1;
  q = data << -0;
  q = data << (0 - 1);
  q = data << (-1 + 2);
end
endmodule
)");
    const auto verilogNegativeShiftResult =
        analyzer.analyzeSource(verilogNegativeShifts);
    test.check(countRule(verilogNegativeShiftResult,
                         "RTL-NEGATIVE-SHIFT-AMOUNT") == 4 &&
                   hasRule(verilogNegativeShiftResult,
                           "RTL-NEGATIVE-SHIFT-AMOUNT", 3) &&
                   hasRule(verilogNegativeShiftResult,
                           "RTL-NEGATIVE-SHIFT-AMOUNT", 4) &&
                   hasRule(verilogNegativeShiftResult,
                           "RTL-NEGATIVE-SHIFT-AMOUNT", 5) &&
                   hasRule(verilogNegativeShiftResult,
                           "RTL-NEGATIVE-SHIFT-AMOUNT", 6),
               "Verilog logical, arithmetic, compound, and parenthesized shifts report negative literals while nonnegative and compound expressions stay conservative");

    const core::SourceFile vhdlNegativeShifts(
        "rtl/vhdl_negative_shifts.vhd", core::HdlLanguage::Vhdl,
R"(library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;
entity vhdl_negative_shifts is
  port (data : in unsigned(7 downto 0); q : out unsigned(7 downto 0));
end entity;
architecture rtl of vhdl_negative_shifts is begin
  process(all) begin
    q <= data sll -1;
    q <= data sra (-2);
    q <= shift_left(data, -3);
    q <= data srl 1;
    q <= data sll -0;
    q <= data sll (0 - 1);
    q <= shift_right(data, (-1 + 2));
  end process;
end architecture;
)");
    const auto vhdlNegativeShiftResult = analyzer.analyzeSource(vhdlNegativeShifts);
    test.check(countRule(vhdlNegativeShiftResult,
                         "RTL-NEGATIVE-SHIFT-AMOUNT") == 3 &&
                   hasRule(vhdlNegativeShiftResult,
                           "RTL-NEGATIVE-SHIFT-AMOUNT", 9) &&
                   hasRule(vhdlNegativeShiftResult,
                           "RTL-NEGATIVE-SHIFT-AMOUNT", 10) &&
                   hasRule(vhdlNegativeShiftResult,
                           "RTL-NEGATIVE-SHIFT-AMOUNT", 11),
               "VHDL infix and numeric_std shift calls report negative literals while zero, positive, and compound expressions stay conservative");

    const core::SourceFile verilogDivideByZero(
        "rtl/verilog_divide_by_zero.sv", core::HdlLanguage::SystemVerilog,
R"(module verilog_divide_by_zero(input logic [7:0] data, output logic [7:0] q);
always_comb begin
  q = data / 0;
  q = data % 4'b0000;
  q /= (0);
  q = data / ((0));
  q = data / 1;
  q = data / (0 + 1);
  if (data != 0) q = data % 3;
end
endmodule
)");
    const auto verilogDivideByZeroResult = analyzer.analyzeSource(verilogDivideByZero);
    test.check(countRule(verilogDivideByZeroResult, "RTL-DIVIDE-BY-ZERO") == 4 &&
                   hasRule(verilogDivideByZeroResult, "RTL-DIVIDE-BY-ZERO", 3) &&
                   hasRule(verilogDivideByZeroResult, "RTL-DIVIDE-BY-ZERO", 4) &&
                   hasRule(verilogDivideByZeroResult, "RTL-DIVIDE-BY-ZERO", 5) &&
                   hasRule(verilogDivideByZeroResult, "RTL-DIVIDE-BY-ZERO", 6),
               "Verilog division, modulo, compound assignment, based zero, and nested parentheses are checked while nonzero denominators remain excluded");

    const core::SourceFile vhdlDivideByZero(
        "rtl/vhdl_divide_by_zero.vhd", core::HdlLanguage::Vhdl,
R"(entity vhdl_divide_by_zero is
  port (data : in integer; q : out integer);
end entity;
architecture rtl of vhdl_divide_by_zero is begin
  process(all) begin
    q <= data / 0;
    q <= data mod (0);
  q <= data rem ((0));
  q <= data / 1;
  q <= data / (0 + 1);
  if data /= 0 then q <= data mod 3; end if;
  end process;
end architecture;
)");
    const auto vhdlDivideByZeroResult = analyzer.analyzeSource(vhdlDivideByZero);
    test.check(countRule(vhdlDivideByZeroResult, "RTL-DIVIDE-BY-ZERO") == 3 &&
                   hasRule(vhdlDivideByZeroResult, "RTL-DIVIDE-BY-ZERO", 6) &&
                   hasRule(vhdlDivideByZeroResult, "RTL-DIVIDE-BY-ZERO", 7) &&
                   hasRule(vhdlDivideByZeroResult, "RTL-DIVIDE-BY-ZERO", 8),
               "VHDL divide, mod, and rem literal zero denominators are checked without confusing inequality or nonzero operations");

    const core::SourceFile verilogDuplicateConditions(
        "rtl/verilog_duplicate_conditions.sv", core::HdlLanguage::SystemVerilog,
R"(module verilog_duplicate_conditions(input logic a, b, c, output logic q);
always_comb begin
  q = 1'b0;
  if (a) q = 1'b1;
  else if ((a)) q = 1'b0;
  else if (b) begin
    if (c) q = 1'b1;
    else if (c) q = 1'b0;
  end
  if (a) q = 1'b1; else if (b) q = 1'b0;
end
endmodule
)");
    const auto verilogDuplicateConditionResult =
        analyzer.analyzeSource(verilogDuplicateConditions);
    test.check(countRule(verilogDuplicateConditionResult,
                         "RTL-DUPLICATE-CONDITION") == 2 &&
                   hasRule(verilogDuplicateConditionResult,
                           "RTL-DUPLICATE-CONDITION", 5) &&
                   hasRule(verilogDuplicateConditionResult,
                           "RTL-DUPLICATE-CONDITION", 8),
               "Verilog duplicate conditions are detected within outer and nested chains without crossing independent chains");

    const core::SourceFile vhdlDuplicateConditions(
        "rtl/vhdl_duplicate_conditions.vhd", core::HdlLanguage::Vhdl,
R"(entity vhdl_duplicate_conditions is
  port (a, b, c : in boolean; q : out boolean);
end entity;
architecture rtl of vhdl_duplicate_conditions is begin
  process(all) begin
    q <= false;
    if a then q <= true;
    elsif (A) then q <= false;
    elsif b then
      if c then q <= true;
      elsif C then q <= false;
      end if;
    end if;
    if a then q <= true; elsif b then q <= false; end if;
  end process;
end architecture;
)");
    const auto vhdlDuplicateConditionResult =
        analyzer.analyzeSource(vhdlDuplicateConditions);
    test.check(countRule(vhdlDuplicateConditionResult,
                         "RTL-DUPLICATE-CONDITION") == 2 &&
                   hasRule(vhdlDuplicateConditionResult,
                           "RTL-DUPLICATE-CONDITION", 8) &&
                   hasRule(vhdlDuplicateConditionResult,
                           "RTL-DUPLICATE-CONDITION", 11),
               "VHDL duplicate conditions are case-insensitive and isolated between outer, nested, and independent chains");

    const core::SourceFile overlappingWildcardCases(
        "rtl/overlapping_wildcards.sv", core::HdlLanguage::SystemVerilog,
R"(module overlapping_wildcards(input logic [3:0] select_value, output logic q);
always_comb begin
  casez (select_value)
    4'b1?00: q = 1'b0;
    4'b10?0: q = 1'b1;
    4'b01??: q = 1'b0;
    default: q = 1'b0;
  endcase
  casex (select_value)
    4'bx100: q = 1'b0;
    4'b01?0: q = 1'b1;
    default: q = 1'b0;
  endcase
  casez (select_value)
    4'hz: q = 1'b0;
    4'ha: q = 1'b1;
    default: q = 1'b0;
  endcase
  casez (select_value)
    4'bx000: q = 1'b0;
    4'b0000: q = 1'b1;
    default: q = 1'b0;
  endcase
end
endmodule
)");
    const auto wildcardResult = analyzer.analyzeSource(overlappingWildcardCases);
    test.check(countRule(wildcardResult, "VL-OVERLAPPING-WILDCARD-CASE") == 3,
               "binary casez, casex, and hexadecimal wildcard overlaps are reported");
    bool wildcardContext = true;
    for (const auto& violation : wildcardResult.violations) {
        if (violation.ruleId == "VL-OVERLAPPING-WILDCARD-CASE") {
            wildcardContext = wildcardContext &&
                              violation.severity == analysis::Severity::Warning &&
                              violation.module == "overlapping_wildcards";
        }
    }
    test.check(wildcardContext,
               "wildcard overlap findings retain warning severity and design-unit context");

    const core::SourceFile verilogMultipleClocks(
        "rtl/verilog_multiple_clocks.sv", core::HdlLanguage::SystemVerilog,
R"(module verilog_multiple_clocks(input logic clk_a, input logic clk_b,
                                  input logic d, output logic q);
always_ff @(posedge clk_a or negedge clk_b) q <= d;
endmodule
)");
    const auto verilogMultipleClocksResult = analyzer.analyzeSource(verilogMultipleClocks);
    test.check(countRule(verilogMultipleClocksResult,
                         "RTL-MULTIPLE-CLOCKS-PER-PROCESS") == 1,
               "Verilog process with two independent clock events is reported once");
    const auto multipleClockFinding = std::ranges::find_if(
        verilogMultipleClocksResult.violations, [](const auto& violation) {
            return violation.ruleId == "RTL-MULTIPLE-CLOCKS-PER-PROCESS";
        });
    test.check(multipleClockFinding != verilogMultipleClocksResult.violations.end() &&
                   multipleClockFinding->severity == analysis::Severity::Error &&
                   multipleClockFinding->module == "verilog_multiple_clocks" &&
                   multipleClockFinding->signal == "clk_b",
               "multiple-clock finding retains error severity, unit, and conflicting clock context");
    const auto* multipleClockUnit = findUnit(verilogMultipleClocksResult,
                                              "verilog_multiple_clocks");
    test.check(multipleClockUnit && multipleClockUnit->sequentialProcesses.size() == 1 &&
                   multipleClockUnit->sequentialProcesses[0].clockEvents.size() == 2,
               "all Verilog clock events are retained on the sequential process");
    std::ostringstream multipleClockJson;
    report::writeJson(verilogMultipleClocksResult, multipleClockJson);
    test.check(multipleClockJson.str().find("\"clockEvents\":[{\"signal\":\"clk_a\"") !=
                   std::string::npos &&
                   multipleClockJson.str().find("{\"signal\":\"clk_b\",\"edge\":\"falling\"") !=
                   std::string::npos,
               "JSON publishes every clock event associated with a sequential process");

    const core::SourceFile asynchronousClear(
        "rtl/asynchronous_clear.sv", core::HdlLanguage::SystemVerilog,
R"(module asynchronous_clear(input logic clk, input logic clear_n,
                              input logic d, output logic q);
always_ff @(posedge clk or negedge clear_n)
  if (!clear_n) q <= 1'b0; else q <= d;
endmodule
)");
    test.check(!hasRule(analyzer.analyzeSource(asynchronousClear),
                        "RTL-MULTIPLE-CLOCKS-PER-PROCESS"),
               "common asynchronous clear naming is classified as reset rather than a second clock");
    test.check(!hasRule(analyzer.analyzeSource(asynchronousClear),
                        "RTL-UNUSED-ASYNC-RESET"),
               "asynchronous clear consumed by its reset branch is not reported as unused");

    const core::SourceFile unusedVerilogReset(
        "rtl/unused_verilog_reset.sv", core::HdlLanguage::SystemVerilog,
R"(module unused_verilog_reset(input logic clk, input logic reset_n,
                                input logic d, output logic q);
always_ff @(posedge clk or negedge reset_n) q <= d;
endmodule
)");
    const auto unusedVerilogResetResult = analyzer.analyzeSource(unusedVerilogReset);
    test.check(countRule(unusedVerilogResetResult, "RTL-UNUSED-ASYNC-RESET") == 1,
               "Verilog asynchronous reset event omitted from the body is reported once");
    const auto unusedResetFinding = std::ranges::find_if(
        unusedVerilogResetResult.violations, [](const auto& violation) {
            return violation.ruleId == "RTL-UNUSED-ASYNC-RESET";
        });
    test.check(unusedResetFinding != unusedVerilogResetResult.violations.end() &&
                   unusedResetFinding->severity == analysis::Severity::Error &&
                   unusedResetFinding->module == "unused_verilog_reset" &&
                   unusedResetFinding->signal == "reset_n" &&
                   unusedResetFinding->line == 3,
               "unused asynchronous reset retains error severity and event location");

    const core::SourceFile dualEdgeOneClock(
        "rtl/dual_edge_one_clock.sv", core::HdlLanguage::SystemVerilog,
R"(module dual_edge_one_clock(input logic clk, input logic d, output logic q);
always @(posedge clk or negedge clk) q <= d;
endmodule
)");
    test.check(!hasRule(analyzer.analyzeSource(dualEdgeOneClock),
                        "RTL-MULTIPLE-CLOCKS-PER-PROCESS"),
               "two edges of one signal remain a mixed-edge issue, not a multiple-clock issue");

    const core::SourceFile vhdlMultipleClocks(
        "rtl/vhdl_multiple_clocks.vhd", core::HdlLanguage::Vhdl,
R"(entity vhdl_multiple_clocks is
  port (clk_a, clk_b, d : in std_logic; q : out std_logic);
end entity;
architecture rtl of vhdl_multiple_clocks is begin
  process(clk_a, clk_b) begin
    if rising_edge(clk_a) then q <= d;
    elsif falling_edge(clk_b) then q <= not d;
    end if;
  end process;
end architecture;
)");
    test.check(countRule(analyzer.analyzeSource(vhdlMultipleClocks),
                         "RTL-MULTIPLE-CLOCKS-PER-PROCESS") == 1,
               "VHDL process with two independent clock edges is reported once");

    const core::SourceFile unusedVhdlReset(
        "rtl/unused_vhdl_reset.vhd", core::HdlLanguage::Vhdl,
R"(entity unused_vhdl_reset is
  port (clk, reset_n, clear_n, d : in std_logic; q_good, q_bad : out std_logic);
end entity;
architecture rtl of unused_vhdl_reset is begin
  good : process(clk, reset_n) begin
    if reset_n = '0' then q_good <= '0';
    elsif rising_edge(clk) then q_good <= d; end if;
  end process;
  bad : process(clk, clear_n) begin
    if rising_edge(clk) then q_bad <= d; end if;
  end process;
end architecture;
)");
    const auto unusedVhdlResetResult = analyzer.analyzeSource(unusedVhdlReset);
    test.check(countRule(unusedVhdlResetResult, "RTL-UNUSED-ASYNC-RESET") == 1,
               "VHDL reset-like sensitivity signal omitted from the body is reported once");
    const auto unusedVhdlResetFinding = std::ranges::find_if(
        unusedVhdlResetResult.violations, [](const auto& violation) {
            return violation.ruleId == "RTL-UNUSED-ASYNC-RESET";
        });
    test.check(unusedVhdlResetFinding != unusedVhdlResetResult.violations.end() &&
                   unusedVhdlResetFinding->signal == "clear_n" &&
                   unusedVhdlResetFinding->module == "unused_vhdl_reset",
               "VHDL unused asynchronous control retains signal and entity context");

    const core::SourceFile verilogTiming("rtl/verilog_timing.sv", core::HdlLanguage::SystemVerilog,
R"(module verilog_timing(
  input logic clk, input logic reset, input logic sync_reset, input logic d,
  output logic q_async, output logic q_sync
);
always_ff @(posedge clk or negedge reset) begin
  if (!reset) q_async <= 1'b0; else q_async <= d;
end
always_ff @(posedge clk) begin
  if (sync_reset) q_sync <= 1'b0; else q_sync <= d;
end
endmodule
)");
    const auto verilogTimingResult = analyzer.analyzeSource(verilogTiming);
    const auto* verilogTimingUnit = findUnit(verilogTimingResult, "verilog_timing");
    test.check(verilogTimingUnit && verilogTimingUnit->clocks.size() == 1 &&
                   verilogTimingUnit->clocks[0].signal == "clk" &&
                   verilogTimingUnit->clocks[0].risingEdge,
               "Verilog clock and active edge are extracted");
    test.check(verilogTimingUnit && verilogTimingUnit->resets.size() == 2,
               "Verilog synchronous and asynchronous resets are extracted");
    if (verilogTimingUnit && verilogTimingUnit->resets.size() == 2) {
        const auto asyncReset = std::ranges::find_if(verilogTimingUnit->resets,
            [](const auto& reset) { return reset.signal == "reset"; });
        const auto syncReset = std::ranges::find_if(verilogTimingUnit->resets,
            [](const auto& reset) { return reset.signal == "sync_reset"; });
        test.check(asyncReset != verilogTimingUnit->resets.end() && asyncReset->asynchronous &&
                       !asyncReset->activeHigh,
                   "Verilog asynchronous active-low reset attributes are retained");
        test.check(syncReset != verilogTimingUnit->resets.end() && !syncReset->asynchronous &&
                       syncReset->activeHigh,
                   "Verilog synchronous active-high reset attributes are retained");
    }

    const core::SourceFile singleOwner("rtl/single_owner.sv", core::HdlLanguage::SystemVerilog,
R"(module single_owner(input logic clk, input logic sel, output logic q);
always_ff @(posedge clk) begin
  if (sel) q <= 1'b1;
  else q <= 1'b0;
end
endmodule
)");
    test.check(!hasRule(analyzer.analyzeSource(singleOwner), "RTL-MULTIPLE-DRIVERS"),
               "multiple conditional assignments in one process count as one driver");

    const core::SourceFile initializedOwner("rtl/initialized_owner.sv", core::HdlLanguage::SystemVerilog,
R"(module initialized_owner(input logic clk, output logic q);
initial q = 1'b0;
always_ff @(posedge clk) q <= ~q;
endmodule
)");
    const auto initializedResult = analyzer.analyzeSource(initializedOwner);
    test.check(!hasRule(initializedResult, "RTL-MULTIPLE-DRIVERS"),
               "an initialization source does not count as a second runtime driver");
    test.check(!hasRule(initializedResult, "RTL-UNDRIVEN-OUTPUT"),
               "an initialization source still establishes that a signal is driven");

    const core::SourceFile initializationSources(
        "rtl/initialization_sources.sv", core::HdlLanguage::SystemVerilog,
R"(module initialization_sources(output logic scalar_q, output logic memory_q);
wire scalar_source = 1'b1;
reg memory_source [0:3];
initial begin
  memory_source[0] = 1'b0;
end
assign scalar_q = scalar_source;
assign memory_q = memory_source[0];
endmodule
)");
    const auto initializationResult = analyzer.analyzeSource(initializationSources);
    test.check(!hasRule(initializationResult, "RTL-UNDRIVEN-SIGNAL"),
               "declaration and initial-block memory initialization count as signal sources");
    test.check(!hasRule(initializationResult, "RTL-MULTIPLE-DRIVERS"),
               "initialization sources do not create runtime multiple-driver findings");

    const core::SourceFile usageRisks("rtl/usage_risks.sv", core::HdlLanguage::SystemVerilog,
R"(module usage_risks(input logic a, output logic q);
wire undriven_read;
wire unused;
logic write_only;
assign q = undriven_read;
always_comb begin
  write_only = a;
end
endmodule
)");
    const auto usageResult = analyzer.analyzeSource(usageRisks);
    test.check(hasRule(usageResult, "RTL-UNDRIVEN-SIGNAL", 5),
               "read internal signal without a driver is reported at its first use");
    test.check(hasRule(usageResult, "RTL-UNUSED-SIGNAL", 3) &&
                   hasRule(usageResult, "RTL-WRITE-ONLY-SIGNAL", 4),
               "unused and write-only internal signals are distinguished and reported");

    const core::SourceFile unusedVerilogInputs(
        "rtl/unused_inputs.sv", core::HdlLanguage::SystemVerilog,
R"(module unused_inputs(
  input logic used,
  input logic forwarded,
  input logic unused,
  output logic q);
assign q = used;
sink u_sink(.a(forwarded));
endmodule
module sink(input logic a);
logic consumed;
always_comb consumed = a;
endmodule
)");
    const auto unusedVerilogInputResult = analyzer.analyzeSource(unusedVerilogInputs);
    test.check(countRule(unusedVerilogInputResult, "RTL-UNUSED-INPUT-PORT") == 1 &&
                   hasRule(unusedVerilogInputResult, "RTL-UNUSED-INPUT-PORT", 4),
               "unused Verilog input is reported while read and forwarded inputs are accepted");

    const core::SourceFile duplicateVerilogSignals(
        "rtl/duplicate_signals.sv", core::HdlLanguage::SystemVerilog,
R"(module duplicate_signals(input logic a, output logic q);
logic duplicate = 1'b0;
wire duplicate;
logic grouped = 1'b0, grouped;
wire signed [3:0] signed_value = 4'b0;
logic a;
assign q = a ^ duplicate ^ grouped ^ signed_value[0];
endmodule
module legacy_redeclaration(a, q);
input a;
output q;
wire a;
assign q = a;
endmodule
)");
    const auto duplicateVerilogSignalResult = analyzer.analyzeSource(duplicateVerilogSignals);
    test.check(countRule(duplicateVerilogSignalResult,
                         "STRUCT-DUPLICATE-SIGNAL-DECLARATION") == 3 &&
                   hasRule(duplicateVerilogSignalResult,
                           "STRUCT-DUPLICATE-SIGNAL-DECLARATION", 3) &&
                   hasRule(duplicateVerilogSignalResult,
                           "STRUCT-DUPLICATE-SIGNAL-DECLARATION", 4) &&
                   hasRule(duplicateVerilogSignalResult,
                           "STRUCT-DUPLICATE-SIGNAL-DECLARATION", 6),
               "duplicate Verilog signals and ANSI port collisions are located without rejecting a legacy port net redeclaration");

    const core::SourceFile connectedOutput("rtl/connected_output.sv", core::HdlLanguage::SystemVerilog,
R"(module child(output logic out); assign out = 1'b0; endmodule
module wrapper(output logic q); child u_child(.out(q)); endmodule
)");
    const auto connectedResult = analyzer.analyzeSource(connectedOutput);
    test.check(!hasRule(connectedResult, "RTL-UNDRIVEN-OUTPUT", 2),
               "output connected to a child instance is not guessed to be undriven");

    const core::SourceFile verilogSensitivity(
        "rtl/verilog_sensitivity.sv", core::HdlLanguage::SystemVerilog,
R"(module incomplete_comb(input logic a, input logic b, output logic q);
always @(a) begin
  q = a & b;
end
endmodule
module complete_comb(input logic a, input logic b, output logic q);
always @(a or b) q = a & b;
endmodule
module clocked(input logic clk, input logic rst_n, input logic d, output logic q);
always @(posedge clk or negedge rst_n) begin
  if (!rst_n) q <= 1'b0; else q <= d;
end
endmodule
)");
    const auto verilogSensitivityResult = analyzer.analyzeSource(verilogSensitivity);
    test.check(hasRule(verilogSensitivityResult, "RTL-INCOMPLETE-SENSITIVITY", 3),
               "legacy combinational always block locates a missing sensitivity signal at its body read");
    test.check(countRule(verilogSensitivityResult, "RTL-INCOMPLETE-SENSITIVITY") == 1,
               "complete and edge-triggered Verilog processes do not report sensitivity findings");

    const core::SourceFile redundantVerilogSensitivity(
        "rtl/redundant_sensitivity.sv", core::HdlLanguage::SystemVerilog,
R"(module redundant_sensitivity(input logic a, input logic b, input logic spare,
  output logic q);
always @(a or b or spare) q = a & b;
endmodule
module read_write_sensitivity(input logic a, output logic q);
always @(a) q = q ^ a;
endmodule
)");
    const auto redundantVerilogSensitivityResult =
        analyzer.analyzeSource(redundantVerilogSensitivity);
    test.check(hasRule(redundantVerilogSensitivityResult,
                       "RTL-REDUNDANT-SENSITIVITY", 3) &&
                   countRule(redundantVerilogSensitivityResult,
                             "RTL-REDUNDANT-SENSITIVITY") == 1,
               "Verilog explicit sensitivity list reports only unread entries");
    test.check(hasRule(redundantVerilogSensitivityResult,
                       "RTL-INCOMPLETE-SENSITIVITY", 6),
               "Verilog signal that is both read and written is still required in the sensitivity list");

    const core::SourceFile duplicateVerilogSensitivity(
        "rtl/duplicate_sensitivity.sv", core::HdlLanguage::SystemVerilog,
R"(module duplicate_comb(input logic a, input logic b, output logic q);
always @(a or a or b) q = a & b;
endmodule
module duplicate_edge(input logic clk, input logic rst_n, input logic d, output logic q);
always @(posedge clk or posedge clk or negedge rst_n) q <= d;
endmodule
module distinct_edges(input logic clk, input logic d, output logic q);
always @(posedge clk or negedge clk) q <= d;
endmodule
module compound_event(input logic a, input logic b, output logic q);
always @((a && a) or b) q = a & b;
endmodule
)");
    const auto duplicateVerilogSensitivityResult =
        analyzer.analyzeSource(duplicateVerilogSensitivity);
    test.check(countRule(duplicateVerilogSensitivityResult,
                         "RTL-DUPLICATE-SENSITIVITY") == 2 &&
                   hasRule(duplicateVerilogSensitivityResult,
                           "RTL-DUPLICATE-SENSITIVITY", 2) &&
                   hasRule(duplicateVerilogSensitivityResult,
                           "RTL-DUPLICATE-SENSITIVITY", 5),
               "Verilog duplicate level and exact edge events are reported while opposite edges and compound expressions remain distinct");

    const core::SourceFile verilogLatch("rtl/verilog_latch.sv", core::HdlLanguage::SystemVerilog,
R"(module verilog_latch(input logic a, input logic en,
  output logic missing_else, output logic with_default,
  output logic complete_if, output logic missing_case, output logic complete_case,
  output logic intentional_latch, output logic automatic_star);
always_comb begin
  if (en) missing_else = a;
end
always @(*) begin
  with_default = 1'b0;
  if (en) with_default = a;
end
always @(*) begin
  if (en) complete_if = a;
  else complete_if = 1'b0;
end
always @(*) begin
  unique case (a)
    1'b0: missing_case = 1'b0;
  endcase
end
always @(*) begin
  priority case (a)
    1'b0: complete_case = 1'b0;
    default: complete_case = 1'b1;
  endcase
end
always_latch begin
  if (en) intentional_latch = a;
end
always @* begin
  automatic_star = a;
end
endmodule
)");
    const auto verilogLatchResult = analyzer.analyzeSource(verilogLatch);
    test.check(hasRule(verilogLatchResult, "RTL-LATCH-INFERRED", 5) &&
                   hasRule(verilogLatchResult, "RTL-LATCH-INFERRED", 16),
               "missing else and missing case default infer Verilog latch risks");
    test.check(countRule(verilogLatchResult, "RTL-LATCH-INFERRED") == 2,
               "defaults, complete branches, and intentional always_latch prevent findings");
    test.check(!hasRule(verilogLatchResult, "RTL-INCOMPLETE-SENSITIVITY"),
               "always @* is treated as an automatic sensitivity list");

    const core::SourceFile vhdlSemantic("rtl/vhdl_semantic.vhd", core::HdlLanguage::Vhdl,
R"(entity vhdl_semantic is
  port (
    clk : in std_logic;
    assigned_input : in std_logic;
    multi_driver : out std_logic;
    undriven : out std_logic
  );
end entity;
architecture rtl of vhdl_semantic is begin
  first_driver : process(clk) begin
    if rising_edge(clk) then multi_driver <= '0'; end if;
  end process;
  second_driver : process(clk) begin
    if rising_edge(clk) then
      multi_driver <= '1';
      assigned_input <= '0';
    end if;
  end process;
end architecture;
)");
    const auto vhdlSemanticResult = analyzer.analyzeSource(vhdlSemantic);
    test.check(hasRule(vhdlSemanticResult, "RTL-MULTIPLE-DRIVERS", 15),
               "VHDL assignments from distinct processes create a multiple-driver finding");
    test.check(hasRule(vhdlSemanticResult, "RTL-INPUT-DRIVEN", 16),
               "VHDL input port assignment is reported");
    test.check(!hasRule(vhdlSemanticResult, "RTL-UNUSED-INPUT-PORT", 4),
               "an illegally driven VHDL input avoids a secondary unused-input finding");
    test.check(hasRule(vhdlSemanticResult, "RTL-UNDRIVEN-OUTPUT", 6),
               "VHDL undriven output is reported at its declaration");

    const core::SourceFile vhdlUsage("rtl/vhdl_usage.vhd", core::HdlLanguage::Vhdl,
R"(entity vhdl_usage is port (a : in std_logic; q : out std_logic); end entity;
architecture rtl of vhdl_usage is
  signal undriven_read : std_logic;
  signal unused : std_logic;
  signal write_only : std_logic;
begin
  q <= undriven_read;
  process(a) begin write_only <= a; end process;
end architecture;
)");
    const auto vhdlUsageResult = analyzer.analyzeSource(vhdlUsage);
    test.check(hasRule(vhdlUsageResult, "RTL-UNDRIVEN-SIGNAL", 7),
               "VHDL read internal signal without a driver is reported at its first use");
    test.check(hasRule(vhdlUsageResult, "RTL-UNUSED-SIGNAL", 4) &&
                   hasRule(vhdlUsageResult, "RTL-UNUSED-SIGNAL", 5),
               "VHDL unused and write-only signals are reported");

    const core::SourceFile unusedVhdlInputs(
        "rtl/unused_inputs.vhd", core::HdlLanguage::Vhdl,
R"(entity unused_inputs is port (
  Used : in std_logic;
  forwarded : in std_logic;
  UNUSED : in std_logic;
  q : out std_logic);
end entity;
architecture rtl of unused_inputs is begin
  q <= used;
  u_sink : entity work.sink port map (a => forwarded);
end architecture;
)");
    const auto unusedVhdlInputResult = analyzer.analyzeSource(unusedVhdlInputs);
    test.check(countRule(unusedVhdlInputResult, "RTL-UNUSED-INPUT-PORT") == 1 &&
                   hasRule(unusedVhdlInputResult, "RTL-UNUSED-INPUT-PORT", 4),
               "unused VHDL input is reported case-insensitively while mapped inputs are accepted");

    const core::SourceFile duplicateVhdlSignals(
        "rtl/duplicate_signals.vhd", core::HdlLanguage::Vhdl,
R"(entity duplicate_signals is port (a : in std_logic; q : out std_logic); end entity;
architecture rtl of duplicate_signals is
  signal duplicate : std_logic;
  signal DUPLICATE : std_logic;
  signal a : std_logic;
begin
  q <= a;
end architecture;
)");
    const auto duplicateVhdlSignalResult = analyzer.analyzeSource(duplicateVhdlSignals);
    test.check(countRule(duplicateVhdlSignalResult,
                         "STRUCT-DUPLICATE-SIGNAL-DECLARATION") == 2 &&
                   hasRule(duplicateVhdlSignalResult,
                           "STRUCT-DUPLICATE-SIGNAL-DECLARATION", 4) &&
                   hasRule(duplicateVhdlSignalResult,
                           "STRUCT-DUPLICATE-SIGNAL-DECLARATION", 5),
               "VHDL duplicate signals and port collisions are detected case-insensitively");

    const core::SourceFile vhdlSensitivity(
        "rtl/vhdl_sensitivity.vhd", core::HdlLanguage::Vhdl,
R"(entity vhdl_sensitivity is
  port (a : in std_logic; b : in std_logic; q : out std_logic);
end entity;
architecture rtl of vhdl_sensitivity is begin
  incomplete_comb : process(a) begin
    q <= a and b;
  end process;
end architecture;
)");
    test.check(hasRule(analyzer.analyzeSource(vhdlSensitivity),
                       "RTL-INCOMPLETE-SENSITIVITY", 6),
               "VHDL combinational process reports a missing signal at its body read");

    const core::SourceFile vhdlSensitivitySafe(
        "rtl/vhdl_sensitivity_safe.vhd", core::HdlLanguage::Vhdl,
R"(entity vhdl_sensitivity_safe is
  port (clk : in std_logic; a : in std_logic; b : in std_logic;
        q : out std_logic; r : out std_logic);
end entity;
architecture rtl of vhdl_sensitivity_safe is begin
  complete_comb : process(a, b) begin q <= a and b; end process;
  automatic_comb : process(all) begin r <= a or b; end process;
  clocked : process(clk) begin
    if rising_edge(clk) then q <= a; end if;
  end process;
end architecture;
)");
    test.check(!hasRule(analyzer.analyzeSource(vhdlSensitivitySafe),
                        "RTL-INCOMPLETE-SENSITIVITY"),
               "complete, process(all), and clocked VHDL processes are exempt");

    const core::SourceFile redundantVhdlSensitivity(
        "rtl/redundant_sensitivity.vhd", core::HdlLanguage::Vhdl,
R"(entity redundant_vhdl_sensitivity is
  port (a : in std_logic; b : in std_logic; spare : in std_logic;
        q : out std_logic_vector(1 downto 0));
end entity;
architecture rtl of redundant_vhdl_sensitivity is begin
  comb : process(a, b, spare, q) begin q(0) <= a and b; end process;
end architecture;
)");
    const auto redundantVhdlSensitivityResult =
        analyzer.analyzeSource(redundantVhdlSensitivity);
    test.check(countRule(redundantVhdlSensitivityResult,
                         "RTL-REDUNDANT-SENSITIVITY") == 2 &&
                   std::ranges::any_of(redundantVhdlSensitivityResult.violations,
                       [](const auto& violation) {
                           return violation.ruleId == "RTL-REDUNDANT-SENSITIVITY" &&
                                  violation.line == 6 && violation.signal &&
                                  *violation.signal == "q";
                       }) &&
                   std::ranges::any_of(redundantVhdlSensitivityResult.violations,
                       [](const auto& violation) {
                           return violation.ruleId == "RTL-REDUNDANT-SENSITIVITY" &&
                                  violation.line == 6 && violation.signal &&
                                  *violation.signal == "spare";
                       }),
               "VHDL sensitivity analysis reports each indexed target or unused entry separately");

    const core::SourceFile duplicateVhdlSensitivity(
        "rtl/duplicate_sensitivity.vhd", core::HdlLanguage::Vhdl,
R"(entity duplicate_sensitivity is
  port (a, b : in std_logic; q : out std_logic);
end entity;
architecture rtl of duplicate_sensitivity is begin
  comb : process(a, A, b) begin q <= a and b; end process;
end architecture;
)");
    const auto duplicateVhdlSensitivityResult =
        analyzer.analyzeSource(duplicateVhdlSensitivity);
    const auto duplicateVhdlFinding = std::ranges::find_if(
        duplicateVhdlSensitivityResult.violations,
        [](const auto& violation) {
            return violation.ruleId == "RTL-DUPLICATE-SENSITIVITY";
        });
    test.check(countRule(duplicateVhdlSensitivityResult,
                         "RTL-DUPLICATE-SENSITIVITY") == 1 &&
                   duplicateVhdlFinding != duplicateVhdlSensitivityResult.violations.end() &&
                   duplicateVhdlFinding->line == 5 &&
                   duplicateVhdlFinding->signal == "a" &&
                   duplicateVhdlFinding->severity == analysis::Severity::Info,
               "VHDL duplicate sensitivity items are matched case-insensitively with source context");

    const core::SourceFile vhdlLatch("rtl/vhdl_latch.vhd", core::HdlLanguage::Vhdl,
R"(entity vhdl_latch is
  port (a : in std_logic; en : in std_logic; sel : in std_logic;
        missing_else : out std_logic; with_default : out std_logic;
        complete_if : out std_logic; missing_case : out std_logic;
        complete_case : out std_logic);
end entity;
architecture rtl of vhdl_latch is begin
  comb : process(all) begin
    if en = '1' then missing_else <= a; end if;
    with_default <= '0';
    if en = '1' then with_default <= a; end if;
    if en = '1' then complete_if <= a; else complete_if <= '0'; end if;
    case sel is
      when '0' => missing_case <= '0';
      when others => null;
    end case;
    case sel is
      when '0' => complete_case <= '0';
      when others => complete_case <= '1';
    end case;
  end process;
end architecture;
)");
    const auto vhdlLatchResult = analyzer.analyzeSource(vhdlLatch);
    test.check(hasRule(vhdlLatchResult, "RTL-LATCH-INFERRED", 8),
               "VHDL incomplete if and case paths infer latch risks");
    test.check(countRule(vhdlLatchResult, "RTL-LATCH-INFERRED") == 2,
               "VHDL defaults and complete branches prevent latch findings");

    const core::SourceFile vhdlTiming("rtl/vhdl_timing.vhd", core::HdlLanguage::Vhdl,
R"(entity vhdl_timing is
  port (clk : in std_logic; reset_async : in std_logic; sync_reset : in std_logic;
        d : in std_logic; q_async : out std_logic; q_sync : out std_logic);
end entity;
architecture rtl of vhdl_timing is begin
  async_register : process(clk, reset_async) begin
    if reset_async = '0' then q_async <= '0';
    elsif rising_edge(clk) then q_async <= d; end if;
  end process;
  sync_register : process(clk) begin
    if rising_edge(clk) then
      if sync_reset = '1' then q_sync <= '0'; else q_sync <= d; end if;
    end if;
  end process;
end architecture;
)");
    const auto vhdlTimingResult = analyzer.analyzeSource(vhdlTiming);
    const auto* vhdlTimingUnit = findUnit(vhdlTimingResult, "vhdl_timing");
    test.check(vhdlTimingUnit && vhdlTimingUnit->clocks.size() == 1 &&
                   vhdlTimingUnit->clocks[0].signal == "clk",
               "VHDL rising-edge clock is extracted");
    test.check(vhdlTimingUnit && vhdlTimingUnit->resets.size() == 2,
               "VHDL synchronous and asynchronous resets are extracted");
    if (vhdlTimingUnit && vhdlTimingUnit->resets.size() == 2) {
        const auto asyncReset = std::ranges::find_if(vhdlTimingUnit->resets,
            [](const auto& reset) { return reset.signal == "reset_async"; });
        const auto syncReset = std::ranges::find_if(vhdlTimingUnit->resets,
            [](const auto& reset) { return reset.signal == "sync_reset"; });
        test.check(asyncReset != vhdlTimingUnit->resets.end() && asyncReset->asynchronous &&
                       !asyncReset->activeHigh,
                   "VHDL asynchronous active-low reset attributes are retained");
        test.check(syncReset != vhdlTimingUnit->resets.end() && !syncReset->asynchronous &&
                       syncReset->activeHigh,
                   "VHDL synchronous active-high reset attributes are retained");
    }

    const core::SourceFile vhdlArchitectures(
        "rtl/vhdl_architectures.vhd", core::HdlLanguage::Vhdl,
R"(entity vhdl_architectures is
  port (clk_a : in std_logic; clk_b : in std_logic; d : in std_logic; q : out std_logic);
end entity;
architecture first of vhdl_architectures is begin
  process(clk_a) begin if rising_edge(clk_a) then q <= d; end if; end process;
end architecture;
architecture second of vhdl_architectures is begin
  process(clk_b) begin if rising_edge(clk_b) then q <= d; end if; end process;
end architecture;
)");
    const auto architectureResult = analyzer.analyzeSource(vhdlArchitectures);
    const auto* architectureUnit = findUnit(architectureResult, "vhdl_architectures");
    test.check(architectureUnit && architectureUnit->clocks.size() == 2,
               "timing summaries from multiple VHDL architectures are merged");

    const core::SourceFile verilogTimingConflicts(
        "rtl/verilog_timing_conflicts.sv", core::HdlLanguage::SystemVerilog,
R"(module verilog_timing_conflicts(
  input logic clk, input logic reset, input logic d,
  output logic q_async, output logic q_falling, output logic q_low
);
always_ff @(posedge clk or posedge reset) begin
  if (reset) q_async <= 1'b0; else q_async <= d;
end
always_ff @(negedge clk) begin
  if (reset) q_falling <= 1'b0; else q_falling <= d;
end
always_ff @(posedge clk) begin
  if (!reset) q_low <= 1'b0; else q_low <= d;
end
endmodule
)");
    const auto verilogTimingConflictsResult = analyzer.analyzeSource(verilogTimingConflicts);
    test.check(hasRule(verilogTimingConflictsResult, "RTL-MIXED-CLOCK-EDGE"),
               "Verilog mixed clock edges are reported across processes");
    test.check(hasRule(verilogTimingConflictsResult, "RTL-MIXED-RESET-MODE"),
               "Verilog synchronous/asynchronous reset mode mixing is reported");
    test.check(hasRule(verilogTimingConflictsResult, "RTL-RESET-POLARITY-CONFLICT"),
               "Verilog reset polarity conflict is reported");
    const auto polarityConflict = std::ranges::find_if(verilogTimingConflictsResult.violations,
        [](const auto& violation) { return violation.ruleId == "RTL-RESET-POLARITY-CONFLICT"; });
    test.check(polarityConflict != verilogTimingConflictsResult.violations.end() &&
                   polarityConflict->severity == analysis::Severity::Error &&
                   polarityConflict->module == "verilog_timing_conflicts" &&
                   polarityConflict->signal == "reset",
               "polarity conflict retains error severity and timing context");
    std::ostringstream timingConflictJson;
    report::writeJson(verilogTimingConflictsResult, timingConflictJson);
    test.check(timingConflictJson.str().find("\"synchronousObserved\":true,"
                                             "\"asynchronousObserved\":true,"
                                             "\"activeHighObserved\":true,"
                                             "\"activeLowObserved\":true") != std::string::npos,
               "JSON preserves all observed reset modes and active levels");

    const core::SourceFile vhdlTimingConflicts(
        "rtl/vhdl_timing_conflicts.vhd", core::HdlLanguage::Vhdl,
R"(entity vhdl_timing_conflicts is
  port (clk : in std_logic; reset : in std_logic; d : in std_logic;
        q_async : out std_logic; q_falling : out std_logic; q_low : out std_logic);
end entity;
architecture rtl of vhdl_timing_conflicts is begin
  process(clk, reset) begin
    if reset = '1' then q_async <= '0';
    elsif rising_edge(clk) then q_async <= d; end if;
  end process;
  process(clk) begin
    if falling_edge(clk) then
      if reset = '1' then q_falling <= '0'; else q_falling <= d; end if;
    end if;
  end process;
  process(clk) begin
    if rising_edge(clk) then
      if reset = '0' then q_low <= '0'; else q_low <= d; end if;
    end if;
  end process;
end architecture;
)");
    const auto vhdlTimingConflictsResult = analyzer.analyzeSource(vhdlTimingConflicts);
    test.check(hasRule(vhdlTimingConflictsResult, "RTL-MIXED-CLOCK-EDGE") &&
                   hasRule(vhdlTimingConflictsResult, "RTL-MIXED-RESET-MODE") &&
                   hasRule(vhdlTimingConflictsResult, "RTL-RESET-POLARITY-CONFLICT"),
               "VHDL timing consistency rules match equivalent cross-process conflicts");

    const core::SourceFile verilogCdc("rtl/verilog_cdc.sv", core::HdlLanguage::SystemVerilog,
R"(module verilog_cdc(
  input logic clk_a, input logic clk_b, input logic d,
  output logic source_a, output logic same_a, output logic destination_b
);
always_ff @(posedge clk_a) begin
  source_a <= d;
  same_a <= source_a;
end
always_ff @(posedge clk_b) begin
  destination_b <= source_a;
end
endmodule
)");
    const auto verilogCdcResult = analyzer.analyzeSource(verilogCdc);
    test.check(countRule(verilogCdcResult, "CDC-DIRECT-CROSSING") == 1,
               "Verilog direct register-to-register clock-domain crossing is reported once");
    const auto* verilogCdcUnit = findUnit(verilogCdcResult, "verilog_cdc");
    test.check(verilogCdcUnit && verilogCdcUnit->sequentialProcesses.size() == 2,
               "Verilog sequential process domains are retained in design structure");
    bool cdcContext = false;
    for (const auto& violation : verilogCdcResult.violations) {
        if (violation.ruleId == "CDC-DIRECT-CROSSING") {
            cdcContext = violation.module == "verilog_cdc" &&
                         violation.signal == "source_a" && violation.line == 9;
        }
    }
    test.check(cdcContext, "CDC finding retains destination process and signal context");
    std::ostringstream cdcJson;
    report::writeJson(verilogCdcResult, cdcJson);
    test.check(cdcJson.str().find("\"sequentialProcesses\":2") != std::string::npos &&
                   cdcJson.str().find("\"clock\":\"clk_b\"") != std::string::npos &&
                   cdcJson.str().find("\"writes\":[\"destination_b\"]") != std::string::npos,
               "JSON publishes sequential process domains and access sets");

    const core::SourceFile verilogCdcSynchronizer(
        "rtl/verilog_cdc_synchronizer.sv", core::HdlLanguage::SystemVerilog,
R"(module verilog_cdc_synchronizer(
  input logic clk_a, clk_b, d, output logic source_a, synchronized);
logic sync_ff1;
always_ff @(posedge clk_a) source_a <= d;
always_ff @(posedge clk_b) begin
  sync_ff1 <= source_a;
  synchronized <= sync_ff1;
end
endmodule
)");
    const auto verilogCdcSynchronizerResult =
        analyzer.analyzeSource(verilogCdcSynchronizer);
    test.check(!hasRule(verilogCdcSynchronizerResult,
                        "CDC-DIRECT-CROSSING"),
               "a proven two-stage scalar synchronizer suppresses the direct CDC warning");
    const auto* verilogSynchronizerUnit = findUnit(
        verilogCdcSynchronizerResult, "verilog_cdc_synchronizer");
    test.check(verilogSynchronizerUnit &&
                   verilogSynchronizerUnit->sequentialProcesses.size() == 2 &&
                   verilogSynchronizerUnit->sequentialProcesses[1].directTransfers.size() == 2 &&
                   std::ranges::none_of(
                       verilogSynchronizerUnit->sequentialProcesses[1].complexReads,
                       [](const std::string& read) {
                           return read == "source_a" || read == "sync_ff1";
                       }),
               "Verilog timing model retains direct register transfers separately from complex reads");
    test.check(verilogSynchronizerUnit &&
                   verilogSynchronizerUnit->cdcSynchronizers.size() == 1 &&
                   verilogSynchronizerUnit->cdcSynchronizers[0].sourceSignal == "source_a" &&
                   verilogSynchronizerUnit->cdcSynchronizers[0].firstStage == "sync_ff1" &&
                   verilogSynchronizerUnit->cdcSynchronizers[0].secondStage == "synchronized" &&
                   verilogSynchronizerUnit->cdcSynchronizers[0].destinationClock == "clk_b" &&
                   verilogSynchronizerUnit->cdcSynchronizers[0].risingEdge,
               "certified CDC synchronizer structure retains both stages and destination domain");
    std::ostringstream synchronizerJson;
    report::writeJson(verilogCdcSynchronizerResult, synchronizerJson);
    test.check(synchronizerJson.str().find("\"cdcSynchronizers\":1") !=
                   std::string::npos &&
                   synchronizerJson.str().find(
                       "\"sourceSignal\":\"source_a\",\"firstStage\":\"sync_ff1\","
                       "\"secondStage\":\"synchronized\",\"destinationClock\":\"clk_b\"") !=
                   std::string::npos,
               "JSON publishes certified CDC synchronizer summary and chain details");
    std::ostringstream synchronizerText;
    report::writeText(verilogCdcSynchronizerResult, synchronizerText);
    test.check(synchronizerText.str().find(
                   "1 个已认证 CDC 同步器") != std::string::npos,
               "text report summarizes certified CDC synchronizers");
    std::ostringstream synchronizerSarif;
    report::writeSarif(verilogCdcSynchronizerResult, synchronizerSarif);
    test.check(synchronizerSarif.str().find(
                   "\"cdcSynchronizers\":1") != std::string::npos,
               "SARIF run properties summarize certified CDC synchronizers");
    std::ostringstream synchronizerHtml;
    report::writeHtml(verilogCdcSynchronizerResult, synchronizerHtml);
    test.check(synchronizerHtml.str().find("CDC 同步器</span>") !=
                   std::string::npos &&
                   synchronizerHtml.str().find(
                       "CDC 同步器 <code>source_a</code> → <code>sync_ff1</code> → "
                       "<code>synchronized</code>") != std::string::npos,
               "HTML dashboard renders certified CDC synchronizer count and chain");

    const core::SourceFile unsafeCdcBypass(
        "rtl/unsafe_cdc_bypass.sv", core::HdlLanguage::SystemVerilog,
R"(module unsafe_cdc_bypass(
  input logic clk_a, clk_b, d, enable,
  output logic source_a, synchronized, bypass);
logic sync_ff1;
always_ff @(posedge clk_a) source_a <= d;
always_ff @(posedge clk_b) begin
  sync_ff1 <= source_a;
  synchronized <= sync_ff1;
  bypass <= source_a & enable;
end
endmodule
)");
    test.check(countRule(analyzer.analyzeSource(unsafeCdcBypass),
                         "CDC-DIRECT-CROSSING") == 1,
               "a parallel complex use of the asynchronous source prevents synchronizer suppression");

    const core::SourceFile vectorCdcPipeline(
        "rtl/vector_cdc_pipeline.sv", core::HdlLanguage::SystemVerilog,
R"(module vector_cdc_pipeline(
  input logic clk_a, clk_b, input logic [1:0] d,
  output logic [1:0] source_a, synchronized);
logic [1:0] sync_bus1;
always_ff @(posedge clk_a) source_a <= d;
always_ff @(posedge clk_b) begin
  sync_bus1 <= source_a;
  synchronized <= sync_bus1;
end
endmodule
)");
    test.check(countRule(analyzer.analyzeSource(vectorCdcPipeline),
                         "CDC-DIRECT-CROSSING") == 1,
               "a whole-vector register pipeline is not certified as a scalar CDC synchronizer");

    const core::SourceFile sameClock("rtl/same_clock.sv", core::HdlLanguage::SystemVerilog,
R"(module same_clock(input logic clk, input logic d, output logic a, output logic b);
always_ff @(posedge clk) a <= d;
always_ff @(posedge clk) b <= a;
endmodule
)");
    test.check(!hasRule(analyzer.analyzeSource(sameClock), "CDC-DIRECT-CROSSING"),
               "same-clock register pipeline is not reported as a crossing");

    const core::SourceFile dualClockMemory(
        "rtl/dual_clock_memory.sv", core::HdlLanguage::SystemVerilog,
R"(module dual_clock_memory(
  input logic clk_a, input logic clk_b, input logic we,
  input logic [1:0] waddr, input logic [1:0] raddr, input logic [7:0] d,
  output logic [7:0] q
);
logic [7:0] memory [0:3];
always_ff @(posedge clk_a) if (we) memory[waddr] <= d;
always_ff @(posedge clk_b) q <= memory[raddr];
endmodule
)");
    test.check(!hasRule(analyzer.analyzeSource(dualClockMemory), "CDC-DIRECT-CROSSING"),
               "indexed dual-clock memory storage is not treated as a direct register crossing");

    const core::SourceFile vhdlCdc("rtl/vhdl_cdc.vhd", core::HdlLanguage::Vhdl,
R"(entity vhdl_cdc is
  port (clk_a, clk_b, d : in std_logic; source_a, destination_b : out std_logic);
end entity;
architecture rtl of vhdl_cdc is begin
  process(clk_a) begin
    if rising_edge(clk_a) then source_a <= d; end if;
  end process;
  process(clk_b) begin
    if rising_edge(clk_b) then destination_b <= source_a; end if;
  end process;
end architecture;
)");
    const auto vhdlCdcResult = analyzer.analyzeSource(vhdlCdc);
    test.check(countRule(vhdlCdcResult, "CDC-DIRECT-CROSSING") == 1,
               "VHDL direct register-to-register clock-domain crossing is reported once");

    const core::SourceFile vhdlCdcSynchronizer(
        "rtl/vhdl_cdc_synchronizer.vhd", core::HdlLanguage::Vhdl,
R"(entity vhdl_cdc_synchronizer is
  port (clk_a, clk_b, d : in std_logic; source_a, synchronized : out std_logic);
end entity;
architecture rtl of vhdl_cdc_synchronizer is
  signal sync_ff1 : std_logic;
begin
  process(clk_a) begin
    if rising_edge(clk_a) then source_a <= d; end if;
  end process;
  process(clk_b) begin
    if rising_edge(clk_b) then
      sync_ff1 <= source_a;
      synchronized <= sync_ff1;
    end if;
  end process;
end architecture;
)");
    test.check(!hasRule(analyzer.analyzeSource(vhdlCdcSynchronizer),
                        "CDC-DIRECT-CROSSING"),
               "VHDL two-stage scalar synchronizers receive the same proven-chain suppression");

    const core::SourceFile splitTimingEntity(
        "rtl/split_timing_entity.vhd", core::HdlLanguage::Vhdl,
R"(entity split_timing is
  port (clk_a, clk_b, d : in std_logic; source_a, synchronized : out std_logic);
end entity;
)");
    const core::SourceFile splitTimingArchitecture(
        "rtl/split_timing_architecture.vhd", core::HdlLanguage::Vhdl,
R"(architecture rtl of split_timing is
  signal sync_ff1 : std_logic;
begin
  process(clk_a) begin
    if rising_edge(clk_a) then source_a <= d; end if;
  end process;
  process(clk_b) begin
    if rising_edge(clk_b) then
      sync_ff1 <= source_a;
      synchronized <= sync_ff1;
    end if;
  end process;
end architecture;
)");
    const auto splitTimingResult = analyzer.analyzeSources(
        {splitTimingEntity, splitTimingArchitecture});
    const auto* splitTimingUnit = findUnit(splitTimingResult, "split_timing");
    test.check(splitTimingUnit && splitTimingUnit->clocks.size() == 2 &&
                   splitTimingUnit->sequentialProcesses.size() == 2 &&
                   splitTimingUnit->cdcSynchronizers.size() == 1 &&
                   splitTimingUnit->cdcSynchronizers[0].location.file ==
                       splitTimingArchitecture.path() &&
                   !hasRule(splitTimingResult, "CDC-DIRECT-CROSSING"),
               "VHDL timing and certified synchronizer structure merge across separate entity and architecture files");

    const core::SourceFile splitUnsafeEntity(
        "rtl/split_unsafe_entity.vhd", core::HdlLanguage::Vhdl,
R"(entity split_unsafe is
  port (clk_a, clk_b, d : in std_logic; source_a, destination_b : out std_logic);
end entity;
)");
    const core::SourceFile splitUnsafeArchitecture(
        "rtl/split_unsafe_architecture.vhd", core::HdlLanguage::Vhdl,
R"(architecture rtl of split_unsafe is begin
  process(clk_a) begin
    if rising_edge(clk_a) then source_a <= d; end if;
  end process;
  process(clk_b) begin
    if rising_edge(clk_b) then destination_b <= source_a; end if;
  end process;
end architecture;
)");
    const auto splitUnsafeResult = analyzer.analyzeSources(
        {splitUnsafeEntity, splitUnsafeArchitecture});
    test.check(countRule(splitUnsafeResult, "CDC-DIRECT-CROSSING") == 1,
               "VHDL direct CDC is reported when its entity and architecture are separate files");

    const core::SourceFile verilogRdc("rtl/verilog_rdc.sv", core::HdlLanguage::SystemVerilog,
R"(module verilog_rdc(
  input logic clk_a, input logic clk_b, input logic rst_n, input logic d,
  output logic qa, output logic qb
);
always_ff @(posedge clk_a or negedge rst_n)
  if (!rst_n) qa <= 1'b0; else qa <= d;
always_ff @(posedge clk_b or negedge rst_n)
  if (!rst_n) qb <= 1'b0; else qb <= d;
endmodule
)");
    const auto verilogRdcResult = analyzer.analyzeSource(verilogRdc);
    test.check(countRule(verilogRdcResult, "RDC-MULTI-CLOCK-ASYNC-RESET") == 1,
               "Verilog asynchronous reset spanning two clock domains is reported once");
    bool rdcContext = false;
    for (const auto& violation : verilogRdcResult.violations) {
        if (violation.ruleId == "RDC-MULTI-CLOCK-ASYNC-RESET") {
            rdcContext = violation.module == "verilog_rdc" && violation.signal == "rst_n" &&
                         violation.message.find("clk_a rising") != std::string::npos &&
                         violation.message.find("clk_b rising") != std::string::npos;
        }
    }
    test.check(rdcContext, "RDC finding retains reset, unit, and both clock domains");
    const auto* verilogRdcUnit = findUnit(verilogRdcResult, "verilog_rdc");
    test.check(verilogRdcUnit && verilogRdcUnit->sequentialProcesses.size() == 2 &&
                   verilogRdcUnit->sequentialProcesses[0].resets.size() == 1,
               "per-process Verilog reset association is retained");
    std::ostringstream rdcJson;
    report::writeJson(verilogRdcResult, rdcJson);
    test.check(rdcJson.str().find("\"resets\":[{\"signal\":\"rst_n\","
                                                "\"activeLevel\":\"low\","
                                                "\"asynchronous\":true") != std::string::npos,
               "JSON publishes reset association for each sequential process");

    const core::SourceFile safeResetScope(
        "rtl/safe_reset_scope.sv", core::HdlLanguage::SystemVerilog,
R"(module safe_reset_scope(input logic clk_a, input logic clk_b, input logic rst_n,
                            input logic d, output logic qa, output logic qb);
always_ff @(posedge clk_a or negedge rst_n)
  if (!rst_n) qa <= 1'b0; else qa <= d;
always_ff @(posedge clk_b)
  if (!rst_n) qb <= 1'b0; else qb <= d;
endmodule
)");
    test.check(!hasRule(analyzer.analyzeSource(safeResetScope),
                        "RDC-MULTI-CLOCK-ASYNC-RESET"),
               "one asynchronous and one synchronous reset use do not form a multi-clock async RDC");

    const core::SourceFile vhdlRdc("rtl/vhdl_rdc.vhd", core::HdlLanguage::Vhdl,
R"(entity vhdl_rdc is
  port (clk_a, clk_b, rst_n, d : in std_logic; qa, qb : out std_logic);
end entity;
architecture rtl of vhdl_rdc is begin
  process(clk_a, rst_n) begin
    if rst_n = '0' then qa <= '0'; elsif rising_edge(clk_a) then qa <= d; end if;
  end process;
  process(clk_b, rst_n) begin
    if rst_n = '0' then qb <= '0'; elsif rising_edge(clk_b) then qb <= d; end if;
  end process;
end architecture;
)");
    test.check(countRule(analyzer.analyzeSource(vhdlRdc),
                         "RDC-MULTI-CLOCK-ASYNC-RESET") == 1,
               "VHDL asynchronous reset spanning two clock domains is reported once");

    const std::vector<core::SourceFile> hierarchySources{
        core::SourceFile("rtl/hierarchy.sv", core::HdlLanguage::SystemVerilog,
R"(module leaf; endmodule
module top;
  leaf #(.WIDTH(1)) u_leaf();
  vendor_ip u_external();
endmodule
)")};
    const auto hierarchy = analyzer.analyzeSources(hierarchySources);
    test.check(hierarchy.designUnits == 2, "multiple modules are structurally extracted");
    const auto* top = findUnit(hierarchy, "top");
    const auto* leaf = findUnit(hierarchy, "leaf");
    test.check(top && top->topLevel, "uninstantiated module is marked top-level");
    test.check(leaf && !leaf->topLevel, "resolved child module is not marked top-level");
    test.check(top && top->instances.size() == 2, "parameterized and plain instances are extracted");
    test.check(top && top->instances[0].resolved, "project-local instance target is resolved");
    test.check(top && !top->instances[1].resolved, "external target remains unresolved");
    test.check(hasRule(hierarchy, "STRUCT-UNRESOLVED-INSTANCE", 4),
               "unresolved instance is reported at its target");

    const core::SourceFile namedPortStructure(
        "rtl/named_port_structure.sv", core::HdlLanguage::SystemVerilog,
R"(module interface_child(input logic clk, input logic [7:0] data_i,
                           output logic valid_o);
assign valid_o = clk & data_i[0];
endmodule
module interface_top(input logic clk, input logic [7:0] data,
                     output logic valid);
interface_child u_bad(.clk(clk), .data_typo(data), .valid_o(valid), .clk(clk));
endmodule
)");
    const auto namedPortResult = analyzer.analyzeSources({namedPortStructure});
    test.check(countRule(namedPortResult, "STRUCT-DUPLICATE-NAMED-PORT") == 1,
               "duplicate named connection is reported once at the repeated formal");
    test.check(countRule(namedPortResult, "STRUCT-UNKNOWN-NAMED-PORT") == 1,
               "named connection absent from the resolved target interface is reported once");
    const auto* interfaceChild = findUnit(namedPortResult, "interface_child");
    const auto* interfaceTop = findUnit(namedPortResult, "interface_top");
    test.check(interfaceChild && interfaceChild->ports.size() == 3 &&
                   interfaceChild->ports[0].name == "clk" &&
                   interfaceChild->ports[0].direction == parser::PortDirection::Input &&
                   interfaceChild->ports[2].name == "valid_o" &&
                   interfaceChild->ports[2].direction == parser::PortDirection::Output,
               "ANSI module ports retain order, direction, and declaration locations");
    test.check(interfaceTop && interfaceTop->instances.size() == 1 &&
                   interfaceTop->instances[0].namedConnections.size() == 4 &&
                   interfaceTop->instances[0].namedConnections[1].formal == "data_typo",
               "instance model retains every named formal connection in source order");
    bool namedPortContext = true;
    for (const auto& violation : namedPortResult.violations) {
        if (violation.ruleId == "STRUCT-DUPLICATE-NAMED-PORT" ||
            violation.ruleId == "STRUCT-UNKNOWN-NAMED-PORT") {
            namedPortContext = namedPortContext &&
                               violation.severity == analysis::Severity::Error &&
                               violation.module == "interface_top" && violation.signal;
        }
    }
    test.check(namedPortContext,
               "named-port structure findings retain error severity, parent, and formal context");
    std::ostringstream namedPortJson;
    report::writeJson(namedPortResult, namedPortJson);
    test.check(namedPortJson.str().find("\"ports\":[{\"name\":\"clk\",\"direction\":\"input\"") !=
                   std::string::npos &&
                   namedPortJson.str().find("\"namedConnections\":[{\"formal\":\"clk\"") !=
                   std::string::npos &&
                   namedPortJson.str().find("\"portConnectionCount\":4,\"hasPortConnectionList\":true,\"connectionStyle\":\"named\"") !=
                   std::string::npos,
               "JSON publishes module ports, named connections, count, and style");

    const core::SourceFile positionalPorts(
        "rtl/positional_ports.v", core::HdlLanguage::Verilog,
R"(module positional_child(a, y); input a; output y; assign y = a; endmodule
module positional_top(input a, output y); positional_child u_pos(a, y); endmodule
)");
    const auto positionalPortResult = analyzer.analyzeSources({positionalPorts});
    const auto* positionalChild = findUnit(positionalPortResult, "positional_child");
    test.check(positionalChild && positionalChild->ports.size() == 2,
               "legacy non-ANSI module port declarations are retained");
    const auto* positionalTop = findUnit(positionalPortResult, "positional_top");
    test.check(positionalTop && positionalTop->instances.size() == 1 &&
                   positionalTop->instances[0].portConnectionCount == 2 &&
                   positionalTop->instances[0].positionalConnections.size() == 2 &&
                   positionalTop->instances[0].hasPositionalConnections &&
                   !positionalTop->instances[0].hasNamedConnections,
               "positional instance model retains connection count and style");
    test.check(!hasRule(positionalPortResult, "STRUCT-DUPLICATE-NAMED-PORT") &&
                   !hasRule(positionalPortResult, "STRUCT-UNKNOWN-NAMED-PORT"),
               "positional connections remain outside named-port validation");

    const core::SourceFile positionalMismatch(
        "rtl/positional_mismatch.sv", core::HdlLanguage::SystemVerilog,
R"(module count_child(input logic a, input logic b, output logic y);
assign y = a & b;
endmodule
module count_top(input logic a, output logic y);
count_child u_few(a, y);
count_child u_many(a, a, y, a);
count_child u_mixed(a, .b(a), .y(y));
endmodule
)");
    const auto positionalMismatchResult = analyzer.analyzeSources({positionalMismatch});
    test.check(countRule(positionalMismatchResult, "STRUCT-POSITIONAL-PORT-COUNT") == 1,
               "only too many positional module connections are reported");
    test.check(countRule(positionalMismatchResult,
                         "STRUCT-MIXED-PORT-CONNECTION-STYLE") == 1,
               "Verilog instance mixing positional and named connections is reported");
    bool positionalContext = true;
    for (const auto& violation : positionalMismatchResult.violations) {
        if (violation.ruleId == "STRUCT-POSITIONAL-PORT-COUNT") {
            positionalContext = positionalContext &&
                                violation.severity == analysis::Severity::Warning &&
                                violation.module == "count_top";
        } else if (violation.ruleId == "STRUCT-MIXED-PORT-CONNECTION-STYLE") {
            positionalContext = positionalContext &&
                                violation.severity == analysis::Severity::Error &&
                                violation.module == "count_top";
        }
    }
    test.check(positionalContext,
               "positional count and mixed-style findings retain their intended severities and parent");

    const core::SourceFile positionalRequiredInputs(
        "rtl/positional_required_inputs.sv", core::HdlLanguage::SystemVerilog,
R"(module positional_required_child(input logic a, input logic b,
                                     input logic optional_c = 1'b0, output logic y);
assign y = a & b & optional_c;
endmodule
module positional_required_top(input logic a, input logic b, output logic y);
positional_required_child u_missing(a);
positional_required_child u_empty(a, , , y);
positional_required_child u_ok(a, b);
positional_required_child u_many(a, b, 1'b0, y, a);
endmodule
)");
    const auto positionalRequiredResult = analyzer.analyzeSources({positionalRequiredInputs});
    test.check(countRule(positionalRequiredResult,
                         "STRUCT-UNCONNECTED-INPUT-PORT") == 2,
               "missing and empty required positional inputs are reported");
    test.check(countRule(positionalRequiredResult,
                         "STRUCT-POSITIONAL-PORT-COUNT") == 1,
               "omitted optional inputs and outputs do not create positional count warnings");
    const auto* positionalRequiredTop = findUnit(
        positionalRequiredResult, "positional_required_top");
    test.check(positionalRequiredTop && positionalRequiredTop->instances.size() == 4 &&
                   positionalRequiredTop->instances[1].positionalConnections.size() == 4 &&
                   !positionalRequiredTop->instances[1].positionalConnections[1].actualConnected &&
                   !positionalRequiredTop->instances[1].positionalConnections[2].actualConnected,
               "positional connection model retains empty actual state in declaration order");
    std::ostringstream positionalRequiredJson;
    report::writeJson(positionalRequiredResult, positionalRequiredJson);
    test.check(positionalRequiredJson.str().find(
                   "\"positionalConnections\":[{\"actualConnected\":true") !=
                   std::string::npos &&
                   positionalRequiredJson.str().find("{\"actualConnected\":false") !=
                   std::string::npos,
               "JSON publishes positional actual connectivity state");

    const core::SourceFile unconnectedNamedInputs(
        "rtl/unconnected_named_inputs.sv", core::HdlLanguage::SystemVerilog,
R"(module required_child(input logic required_a, input logic required_b,
                          input logic optional_c = 1'b0, output logic y);
assign y = required_a & required_b & optional_c;
endmodule
module required_top(input logic a, input logic required_b, output logic y);
required_child u_missing(.required_a(a), .y(y));
required_child u_empty(.required_a(a), .required_b(), .y(y));
required_child u_wildcard(.*);
required_child u_shorthand(.required_a(a), .required_b, .y);
endmodule
)");
    const auto unconnectedResult = analyzer.analyzeSources({unconnectedNamedInputs});
    test.check(countRule(unconnectedResult, "STRUCT-UNCONNECTED-INPUT-PORT") == 2,
               "omitted and explicitly empty required named inputs are reported");
    const auto* requiredChild = findUnit(unconnectedResult, "required_child");
    const auto* requiredTop = findUnit(unconnectedResult, "required_top");
    test.check(requiredChild && requiredChild->ports.size() == 4 &&
                   requiredChild->ports[2].name == "optional_c" &&
                   requiredChild->ports[2].hasDefault,
               "SystemVerilog input defaults are retained and exempt omitted optional inputs");
    test.check(requiredTop && requiredTop->instances.size() == 4 &&
                   !requiredTop->instances[1].namedConnections[1].actualConnected &&
                   requiredTop->instances[2].hasWildcardConnection &&
                   requiredTop->instances[3].namedConnections.size() == 3,
               "empty, wildcard, and shorthand named connections are represented explicitly");
    bool unconnectedContext = true;
    for (const auto& violation : unconnectedResult.violations) {
        if (violation.ruleId == "STRUCT-UNCONNECTED-INPUT-PORT") {
            unconnectedContext = unconnectedContext &&
                                 violation.severity == analysis::Severity::Warning &&
                                 violation.module == "required_top" &&
                                 violation.signal == "required_b";
        }
    }
    test.check(unconnectedContext,
               "unconnected-input findings retain warning severity, parent, and port context");
    std::ostringstream unconnectedJson;
    report::writeJson(unconnectedResult, unconnectedJson);
    test.check(unconnectedJson.str().find("\"hasDefault\":true") != std::string::npos &&
                   unconnectedJson.str().find("\"actualConnected\":false") != std::string::npos &&
                   unconnectedJson.str().find("\"hasWildcardConnection\":true") !=
                       std::string::npos,
               "JSON publishes port defaults, empty actual state, and wildcard connections");
    std::ostringstream unconnectedHtml;
    report::writeHtml(unconnectedResult, unconnectedHtml);
    test.check(unconnectedHtml.str().find("optional_c</code>（输入，有默认值）") !=
                   std::string::npos &&
                   unconnectedHtml.str().find("命名关联，通配连接") != std::string::npos,
               "HTML publishes port defaults and wildcard connection state");

    const core::SourceFile unconnectedOutputs(
        "rtl/unconnected_outputs.sv", core::HdlLanguage::SystemVerilog,
R"(module output_child(input logic a, output logic y, inout wire pad);
assign y = a;
endmodule
module output_top(input logic a, output logic y, inout wire pad);
output_child u_omitted(.a(a));
output_child u_open(.a(a), .y(), .pad(pad));
output_child u_positional(a, y);
output_child u_ok(.a(a), .y(y), .pad(pad));
output_child u_wildcard(.*);
endmodule
)");
    const auto unconnectedOutputResult = analyzer.analyzeSources({unconnectedOutputs});
    test.check(countRule(unconnectedOutputResult,
                         "STRUCT-UNCONNECTED-OUTPUT-PORT") == 4,
               "omitted, explicitly open, and trailing positional outputs are reported while complete and wildcard instances are accepted");
    bool unconnectedOutputContext = true;
    for (const auto& violation : unconnectedOutputResult.violations) {
        if (violation.ruleId == "STRUCT-UNCONNECTED-OUTPUT-PORT") {
            unconnectedOutputContext = unconnectedOutputContext &&
                violation.severity == analysis::Severity::Info &&
                violation.module == "output_top" && violation.signal;
        }
    }
    test.check(unconnectedOutputContext,
               "unconnected-output findings retain info severity, parent, and formal context");

    const analysis::Analyzer blackBoxAnalyzer({{}, {"vendor_ip"}, {}, {}, {}, {}});
    const auto blackBoxHierarchy = blackBoxAnalyzer.analyzeSources(hierarchySources);
    const auto* blackBoxTop = findUnit(blackBoxHierarchy, "top");
    test.check(!hasRule(blackBoxHierarchy, "STRUCT-UNRESOLVED-INSTANCE"),
               "declared external IP does not create an unresolved finding");
    test.check(blackBoxTop && blackBoxTop->instances[1].resolved &&
                   blackBoxTop->instances[1].external,
               "black-box instance is marked resolved and external");

    const std::vector<core::SourceFile> cycleSources{
        core::SourceFile("rtl/cycle.v", core::HdlLanguage::Verilog,
                         "module a; b u_b(); endmodule\nmodule b; a u_a(); endmodule\n")};
    test.check(hasRule(analyzer.analyzeSources(cycleSources), "STRUCT-RECURSIVE-INSTANCE"),
               "recursive hierarchy is detected");

    const std::vector<core::SourceFile> duplicateSources{
        core::SourceFile("rtl/a.v", core::HdlLanguage::Verilog, "module duplicate; endmodule\n"),
        core::SourceFile("rtl/b.sv", core::HdlLanguage::SystemVerilog, "module duplicate; endmodule\n")};
    const auto duplicates = analyzer.analyzeSources(duplicateSources);
    test.check(hasRule(duplicates, "STRUCT-DUPLICATE-UNIT"),
               "duplicate module definitions are detected");

    const core::SourceFile vhdl("rtl/risky.vhd", core::HdlLanguage::Vhdl,
R"(library ieee;
use ieee.std_logic_unsigned.all;
entity risky is end entity;
architecture rtl of risky is
  shared variable count : integer;
begin
  process begin
    wait until rising_edge(clk);
  end process;
end architecture;
)");
    const auto vhdlResult = analyzer.analyzeSource(vhdl);
    test.check(vhdlResult.designUnits == 1, "VHDL entity is counted");
    test.check(hasRule(vhdlResult, "VHDL-NONSTD-NUMERIC", 2),
               "non-standard VHDL arithmetic package is reported");
    test.check(hasRule(vhdlResult, "VHDL-SHARED-VARIABLE", 5),
               "shared variable is reported");
    test.check(hasRule(vhdlResult, "VHDL-WAIT", 8), "wait statement is reported");

    const core::SourceFile tokenizedVhdlKeywords(
        "rtl/tokenized_vhdl_keywords.vhd", core::HdlLanguage::Vhdl,
R"(library ieee;
use ieee.
  std_logic_arith
  .all;
entity tokenized_vhdl_keywords is end entity;
architecture rtl of tokenized_vhdl_keywords is
  constant note : string := "shared variable std_logic_signed";
  shared
    variable count : integer;
begin
  -- use ieee.std_logic_unsigned.all; shared variable ignored : integer;
end architecture;
)");
    const auto tokenizedVhdlKeywordResult = analyzer.analyzeSource(tokenizedVhdlKeywords);
    test.check(countRule(tokenizedVhdlKeywordResult, "VHDL-NONSTD-NUMERIC") == 1 &&
                   hasRule(tokenizedVhdlKeywordResult, "VHDL-NONSTD-NUMERIC", 3),
               "tokenized VHDL use clause recognizes a package name split across lines");
    const auto sharedVariableFinding = std::ranges::find_if(
        tokenizedVhdlKeywordResult.violations,
        [](const auto& violation) {
            return violation.ruleId == "VHDL-SHARED-VARIABLE";
        });
    test.check(countRule(tokenizedVhdlKeywordResult, "VHDL-SHARED-VARIABLE") == 1 &&
                   sharedVariableFinding != tokenizedVhdlKeywordResult.violations.end() &&
                   sharedVariableFinding->line == 8 &&
                   sharedVariableFinding->module == "tokenized_vhdl_keywords",
               "cross-line shared variable is reported with architecture context while strings and comments are ignored");

    const core::SourceFile vhdlConstantConditions(
        "rtl/vhdl_constant_conditions.vhd", core::HdlLanguage::Vhdl,
R"(entity vhdl_constant_conditions is
  port (clk, d : in std_logic; enable : in boolean; q, r : out std_logic);
end entity;
architecture rtl of vhdl_constant_conditions is begin
  worker : process(clk) begin
    if rising_edge(clk) then
      if false then q <= d; end if;
      while (true) loop
        exit;
      end loop;
      if enable and false then q <= d; end if;
      if true or enable then q <= d; end if;
      if not (2 = 2) then q <= d; end if;
      if enable or false then q <= d; end if;
      if d = '1' then q <= d; elsif false then q <= d; end if;
      q <= d when false else '0';
      q <= d when enable else '0';
      q <= d;
    end if;
  end process;
  r <= d when true else '0';
end architecture;
)");
    const auto vhdlConstantConditionResult =
        analyzer.analyzeSource(vhdlConstantConditions);
    test.check(countRule(vhdlConstantConditionResult,
                         "RTL-CONSTANT-CONDITION") == 8 &&
                   hasRule(vhdlConstantConditionResult,
                           "RTL-CONSTANT-CONDITION", 7) &&
                   hasRule(vhdlConstantConditionResult,
                           "RTL-CONSTANT-CONDITION", 8) &&
                   hasRule(vhdlConstantConditionResult,
                           "RTL-CONSTANT-CONDITION", 11) &&
                   hasRule(vhdlConstantConditionResult,
                           "RTL-CONSTANT-CONDITION", 12) &&
                   hasRule(vhdlConstantConditionResult,
                           "RTL-CONSTANT-CONDITION", 13) &&
                   hasRule(vhdlConstantConditionResult,
                           "RTL-CONSTANT-CONDITION", 15) &&
                   hasRule(vhdlConstantConditionResult,
                           "RTL-CONSTANT-CONDITION", 16) &&
                   hasRule(vhdlConstantConditionResult,
                           "RTL-CONSTANT-CONDITION", 21),
               "VHDL procedural and when-else constants are reported while runtime-dependent conditions remain excluded");

    const core::SourceFile vhdlSelfAssignments(
        "rtl/vhdl_self_assignments.vhd", core::HdlLanguage::Vhdl,
R"(entity vhdl_self_assignments is
  port (clk, d : in std_logic; q, r : out std_logic;
        v : out std_logic_vector(1 downto 0));
end entity;
architecture rtl of vhdl_self_assignments is begin
  worker : process(clk) begin
    if rising_edge(clk) then
      q <= Q;
      r <= r and d;
      v(0) <= v(0);
    end if;
  end process;
end architecture;
)");
    const auto vhdlSelfAssignmentResult =
        analyzer.analyzeSource(vhdlSelfAssignments);
    const auto vhdlSelfFinding = std::ranges::find_if(
        vhdlSelfAssignmentResult.violations,
        [](const auto& violation) {
            return violation.ruleId == "RTL-SELF-ASSIGNMENT";
        });
    test.check(countRule(vhdlSelfAssignmentResult,
                         "RTL-SELF-ASSIGNMENT") == 1 &&
                   vhdlSelfFinding != vhdlSelfAssignmentResult.violations.end() &&
                   vhdlSelfFinding->line == 8 &&
                   vhdlSelfFinding->signal == "q" &&
                   vhdlSelfFinding->severity == analysis::Severity::Info,
               "VHDL scalar self-assignment is matched case-insensitively while indexed and expression updates are excluded");

    const core::SourceFile vhdlEmptyProcesses(
        "rtl/vhdl_empty_processes.vhd", core::HdlLanguage::Vhdl,
R"(entity vhdl_empty_processes is
  port (a : in std_logic; q : out std_logic);
end entity;
architecture rtl of vhdl_empty_processes is begin
  simple_empty : process(a) begin null; end process;
  declared_empty : process(a)
    function passthrough(v : std_logic) return std_logic is
    begin return v; end function;
  begin
    null;
  end process;
  active : process(a) begin q <= a; end process;
end architecture;
)");
    const auto vhdlEmptyProcessResult =
        analyzer.analyzeSource(vhdlEmptyProcesses);
    test.check(countRule(vhdlEmptyProcessResult,
                         "RTL-EMPTY-PROCESS") == 2 &&
                   hasRule(vhdlEmptyProcessResult,
                           "RTL-EMPTY-PROCESS", 5) &&
                   hasRule(vhdlEmptyProcessResult,
                           "RTL-EMPTY-PROCESS", 6),
               "VHDL null-only processes are reported while local subprogram bodies and active process assignments are handled correctly");

    const core::SourceFile vhdlUnusedGenerics(
        "rtl/vhdl_unused_generics.vhd", core::HdlLanguage::Vhdl,
R"(entity generic_child is
  generic (UNUSED : positive := 1);
  port (y : out bit_vector(UNUSED-1 downto 0));
end entity;
architecture rtl of generic_child is begin y <= (others => '0'); end architecture;
entity vhdl_unused_generics is
  generic (USED : positive := 2; UNUSED : positive := 1);
  port (d : in bit_vector(USED-1 downto 0); q : out bit);
end entity;
architecture rtl of vhdl_unused_generics is begin
  u_child : entity work.generic_child generic map (UNUSED => 1) port map (open);
  q <= d(0);
end architecture;
)");
    const auto vhdlUnusedGenericResult =
        analyzer.analyzeSources({vhdlUnusedGenerics});
    const auto unusedGenericFinding = std::ranges::find_if(
        vhdlUnusedGenericResult.violations,
        [](const auto& violation) {
            return violation.ruleId == "RTL-UNUSED-PARAMETER";
        });
    test.check(countRule(vhdlUnusedGenericResult,
                         "RTL-UNUSED-PARAMETER") == 1 &&
                   unusedGenericFinding != vhdlUnusedGenericResult.violations.end() &&
                   unusedGenericFinding->line == 7 &&
                   unusedGenericFinding->module == "vhdl_unused_generics" &&
                   unusedGenericFinding->signal == "UNUSED" &&
                   unusedGenericFinding->severity == analysis::Severity::Info,
               "VHDL unused generic analysis is entity-scoped and excludes named generic-map formals");

    const core::SourceFile splitGenericEntity(
        "rtl/split_generic_entity.vhd", core::HdlLanguage::Vhdl,
R"(entity split_generic is
  generic (USED : positive := 2; UNUSED : positive := 1);
  port (d : in bit_vector(USED-1 downto 0); q : out bit);
end entity;
)");
    const core::SourceFile splitGenericArchitecture(
        "rtl/split_generic_architecture.vhd", core::HdlLanguage::Vhdl,
R"(architecture rtl of split_generic is begin
  q <= d(USED-2);
end architecture;
)");
    const auto splitGenericResult = analyzer.analyzeSources(
        {splitGenericEntity, splitGenericArchitecture});
    const auto splitGenericFinding = std::ranges::find_if(
        splitGenericResult.violations,
        [](const auto& violation) {
            return violation.ruleId == "RTL-UNUSED-PARAMETER";
        });
    test.check(countRule(splitGenericResult, "RTL-UNUSED-PARAMETER") == 1 &&
                   splitGenericFinding != splitGenericResult.violations.end() &&
                   splitGenericFinding->file == splitGenericEntity.path() &&
                   splitGenericFinding->line == 2 &&
                   splitGenericFinding->signal == "UNUSED",
               "VHDL generic use is resolved across separate entity and architecture files");
    const auto entityOnlyGenericResult =
        analyzer.analyzeSources({splitGenericEntity});
    test.check(countRule(entityOnlyGenericResult,
                         "RTL-UNUSED-PARAMETER") == 0,
               "VHDL generic analysis avoids incomplete-project findings when no architecture is analyzed");

    const core::SourceFile vhdlProcessSuspension(
        "rtl/vhdl_process_suspension.vhd", core::HdlLanguage::Vhdl,
R"(entity vhdl_process_suspension is
  port (a : in std_logic; q_bad, q_wait, q_list, q_all : out std_logic);
end entity;
architecture rtl of vhdl_process_suspension is begin
  no_suspend : process begin q_bad <= a; end process;
  explicit_wait : process begin wait on a; q_wait <= a; end process;
  explicit_list : process(a) begin q_list <= a; end process;
  automatic_list : process(all) begin q_all <= a; end process;
end architecture;
)");
    const auto vhdlProcessSuspensionResult = analyzer.analyzeSource(vhdlProcessSuspension);
    test.check(countRule(vhdlProcessSuspensionResult,
                         "VHDL-PROCESS-NO-SUSPENSION") == 1 &&
                   hasRule(vhdlProcessSuspensionResult,
                           "VHDL-PROCESS-NO-SUSPENSION", 5),
               "only a VHDL process without sensitivity or wait is reported");
    bool suspensionContext = false;
    for (const auto& violation : vhdlProcessSuspensionResult.violations) {
        if (violation.ruleId == "VHDL-PROCESS-NO-SUSPENSION") {
            suspensionContext = violation.severity == analysis::Severity::Error &&
                                violation.module == "vhdl_process_suspension";
        }
    }
    test.check(suspensionContext,
               "non-suspending VHDL process retains error severity and entity context");

    const core::SourceFile vhdlWaitWithSensitivity(
        "rtl/vhdl_wait_with_sensitivity.vhd", core::HdlLanguage::Vhdl,
R"(entity vhdl_wait_with_sensitivity is
  port (a : in std_logic; q_bad, q_safe, q_bare, q_multiline : out std_logic);
end entity;
architecture rtl of vhdl_wait_with_sensitivity is begin
  illegal_wait : process(a) begin wait on a; q_bad <= a; end process;
  legal_wait : process begin wait on a; q_safe <= a; end process;
  bare_wait : process begin wait; q_bare <= a; end process;
  multiline_wait : process begin
    wait
      until a = '1';
    q_multiline <= a;
  end process;
end architecture;
)");
    const auto vhdlWaitWithSensitivityResult =
        analyzer.analyzeSource(vhdlWaitWithSensitivity);
    test.check(countRule(vhdlWaitWithSensitivityResult,
                         "VHDL-WAIT-WITH-SENSITIVITY") == 1 &&
                   hasRule(vhdlWaitWithSensitivityResult,
                           "VHDL-WAIT-WITH-SENSITIVITY", 5),
               "wait in a sensitivity-list process is reported at the explicit wait");
    test.check(countRule(vhdlWaitWithSensitivityResult, "VHDL-WAIT") == 3 &&
                   hasRule(vhdlWaitWithSensitivityResult, "VHDL-WAIT", 6) &&
                   hasRule(vhdlWaitWithSensitivityResult, "VHDL-WAIT", 7) &&
                   hasRule(vhdlWaitWithSensitivityResult, "VHDL-WAIT", 9),
               "specific sensitivity error replaces the generic warning while tokenized wait forms remain");

    const core::SourceFile vhdlLocalProcedureWait(
        "rtl/vhdl_local_procedure_wait.vhd", core::HdlLanguage::Vhdl,
R"(entity vhdl_local_procedure_wait is
  port (a : in std_logic; q : out std_logic);
end entity;
architecture rtl of vhdl_local_procedure_wait is begin
  worker : process(a)
    procedure pause is begin wait; end procedure;
  begin
    q <= a;
  end process;
end architecture;
)");
    const auto vhdlLocalProcedureWaitResult =
        analyzer.analyzeSource(vhdlLocalProcedureWait);
    test.check(countRule(vhdlLocalProcedureWaitResult, "VHDL-WAIT") == 1 &&
                   hasRule(vhdlLocalProcedureWaitResult, "VHDL-WAIT", 6) &&
                   !hasRule(vhdlLocalProcedureWaitResult,
                            "VHDL-WAIT-WITH-SENSITIVITY"),
               "local subprogram wait remains a generic synthesis risk without direct-process legality error");


    const core::SourceFile vhdlHierarchy("rtl/hierarchy.vhd", core::HdlLanguage::Vhdl,
R"(entity child is end entity;
entity parent is end entity;
architecture rtl of parent is begin
  u_child : entity work.child port map ();
end architecture;
)");
    const auto vhdlStructure = analyzer.analyzeSources({vhdlHierarchy});
    const auto* parent = findUnit(vhdlStructure, "parent");
    test.check(parent && parent->instances.size() == 1,
               "direct VHDL entity instantiation is extracted");
    test.check(parent && parent->instances[0].resolved,
               "VHDL entity target is resolved case-insensitively");

    const core::SourceFile vhdlNamedPorts(
        "rtl/vhdl_named_ports.vhd", core::HdlLanguage::Vhdl,
R"(entity vhdl_port_child is
  port (A : in std_logic; Y : out std_logic);
end entity;
entity vhdl_port_parent is
  port (source_value : in std_logic; result_value : out std_logic);
end entity;
architecture rtl of vhdl_port_parent is begin
  u_bad : entity work.vhdl_port_child port map (
    a => source_value, Missing => result_value, A => source_value);
end architecture;
)");
    const auto vhdlNamedPortResult = analyzer.analyzeSources({vhdlNamedPorts});
    test.check(countRule(vhdlNamedPortResult, "STRUCT-DUPLICATE-NAMED-PORT") == 1,
               "VHDL named formal duplicate is compared case-insensitively");
    test.check(countRule(vhdlNamedPortResult, "STRUCT-UNKNOWN-NAMED-PORT") == 1,
               "VHDL named formal absent from the resolved entity is reported");
    const auto* vhdlPortChild = findUnit(vhdlNamedPortResult, "vhdl_port_child");
    const auto* vhdlPortParent = findUnit(vhdlNamedPortResult, "vhdl_port_parent");
    test.check(vhdlPortChild && vhdlPortChild->ports.size() == 2 &&
                   vhdlPortChild->ports[0].name == "A" &&
                   vhdlPortChild->ports[0].direction == parser::PortDirection::Input &&
                   vhdlPortChild->ports[1].direction == parser::PortDirection::Output,
               "VHDL entity ports retain declaration order, spelling, and direction");
    test.check(vhdlPortParent && vhdlPortParent->instances.size() == 1 &&
                   vhdlPortParent->instances[0].namedConnections.size() == 3 &&
                   vhdlPortParent->instances[0].namedConnections[1].formal == "Missing",
               "VHDL port map associations retain formal spelling and source order");
    bool vhdlNamedPortContext = true;
    for (const auto& violation : vhdlNamedPortResult.violations) {
        if (violation.ruleId == "STRUCT-DUPLICATE-NAMED-PORT" ||
            violation.ruleId == "STRUCT-UNKNOWN-NAMED-PORT") {
            vhdlNamedPortContext = vhdlNamedPortContext &&
                                   violation.severity == analysis::Severity::Error &&
                                   violation.module == "vhdl_port_parent" && violation.signal;
        }
    }
    test.check(vhdlNamedPortContext,
               "VHDL named-port findings retain entity, formal, and error severity context");

    const core::SourceFile vhdlUnconnectedOutputs(
        "rtl/vhdl_unconnected_outputs.vhd", core::HdlLanguage::Vhdl,
R"(entity output_child is
  port (a : in std_logic; y : out std_logic; pad : inout std_logic);
end entity;
entity output_parent is
  port (a : in std_logic; y : out std_logic; pad : inout std_logic);
end entity;
architecture rtl of output_parent is begin
  u_omitted : entity work.output_child port map (a => a);
  u_open : entity work.output_child port map (a => a, y => open, pad => pad);
  u_positional : entity work.output_child port map (a, y);
  u_ok : entity work.output_child port map (a => a, y => y, pad => pad);
end architecture;
)");
    const auto vhdlUnconnectedOutputResult =
        analyzer.analyzeSources({vhdlUnconnectedOutputs});
    test.check(countRule(vhdlUnconnectedOutputResult,
                         "STRUCT-UNCONNECTED-OUTPUT-PORT") == 4,
               "VHDL omitted, open, and trailing positional output/inout associations are reported");
    bool vhdlUnconnectedOutputContext = true;
    for (const auto& violation : vhdlUnconnectedOutputResult.violations) {
        if (violation.ruleId == "STRUCT-UNCONNECTED-OUTPUT-PORT") {
            vhdlUnconnectedOutputContext = vhdlUnconnectedOutputContext &&
                violation.severity == analysis::Severity::Info &&
                violation.module == "output_parent" && violation.signal;
        }
    }
    test.check(vhdlUnconnectedOutputContext,
               "VHDL unconnected-output findings retain parent and case-preserved formal context");

    const core::SourceFile vhdlPositionalPorts(
        "rtl/vhdl_positional_ports.vhd", core::HdlLanguage::Vhdl,
R"(entity vhdl_count_child is
  port (source_value : in std_logic; result_value : out std_logic);
end entity;
entity vhdl_count_parent is
  port (source_value : in std_logic; result_value : out std_logic);
end entity;
architecture rtl of vhdl_count_parent is begin
  u_few : entity work.vhdl_count_child port map (source_value);
  u_legal_mixed : entity work.vhdl_count_child
    port map (source_value, result_value => result_value);
end architecture;
)");
    const auto vhdlPositionalPortResult = analyzer.analyzeSources({vhdlPositionalPorts});
    test.check(countRule(vhdlPositionalPortResult, "STRUCT-POSITIONAL-PORT-COUNT") == 0,
               "VHDL may omit trailing output associations without a count warning");
    test.check(!hasRule(vhdlPositionalPortResult,
                        "STRUCT-MIXED-PORT-CONNECTION-STYLE"),
               "VHDL positional associations followed by named associations remain legal");

    const core::SourceFile vhdlPositionalRequiredInputs(
        "rtl/vhdl_positional_required_inputs.vhd", core::HdlLanguage::Vhdl,
R"(entity positional_required_vhdl_child is
  port (a : in std_logic; b : in std_logic; optional_c : in std_logic := '0';
        y : out std_logic);
end entity;
entity positional_required_vhdl_top is
  port (a : in std_logic; y : out std_logic);
end entity;
architecture rtl of positional_required_vhdl_top is begin
  u_missing : entity work.positional_required_vhdl_child port map (a);
  u_open : entity work.positional_required_vhdl_child port map (a, open, open, y);
  u_mixed_missing : entity work.positional_required_vhdl_child port map (a, y => y);
  u_mixed_ok : entity work.positional_required_vhdl_child port map (a, b => a, y => y);
  u_many : entity work.positional_required_vhdl_child port map (a, a, a, y, a);
end architecture;
)");
    const auto vhdlPositionalRequiredResult =
        analyzer.analyzeSources({vhdlPositionalRequiredInputs});
    test.check(countRule(vhdlPositionalRequiredResult,
                         "STRUCT-UNCONNECTED-INPUT-PORT") == 3,
               "VHDL missing, open, and mixed-map required inputs are reported");
    test.check(countRule(vhdlPositionalRequiredResult,
                         "STRUCT-POSITIONAL-PORT-COUNT") == 1,
               "VHDL positional actuals beyond the target interface are reported");
    test.check(!hasRule(vhdlPositionalRequiredResult,
                        "STRUCT-MIXED-PORT-CONNECTION-STYLE"),
               "VHDL mixed positional/named port maps remain legal while inputs are checked");

    const core::SourceFile vhdlUnconnectedInputs(
        "rtl/vhdl_unconnected_inputs.vhd", core::HdlLanguage::Vhdl,
R"(entity required_vhdl_child is
  port (required_a : in std_logic; required_b : in std_logic;
        optional_c : in std_logic := '0'; y : out std_logic);
end entity;
entity required_vhdl_top is
  port (a : in std_logic; y : out std_logic);
end entity;
architecture rtl of required_vhdl_top is begin
  u_missing : entity work.required_vhdl_child
    port map (required_a => a, y => y);
  u_open : entity work.required_vhdl_child
    port map (required_a => a, required_b => open, y => y);
  u_mixed : entity work.required_vhdl_child
    port map (a, required_b => a, y => y);
end architecture;
)");
    const auto vhdlUnconnectedResult = analyzer.analyzeSources({vhdlUnconnectedInputs});
    test.check(countRule(vhdlUnconnectedResult, "STRUCT-UNCONNECTED-INPUT-PORT") == 2,
               "VHDL omitted and open required named inputs are reported without flagging mixed maps");
    const auto* requiredVhdlChild = findUnit(vhdlUnconnectedResult, "required_vhdl_child");
    const auto* requiredVhdlTop = findUnit(vhdlUnconnectedResult, "required_vhdl_top");
    test.check(requiredVhdlChild && requiredVhdlChild->ports.size() == 4 &&
                   requiredVhdlChild->ports[2].hasDefault,
               "VHDL input defaults are retained and exempt omitted optional inputs");
    test.check(requiredVhdlTop && requiredVhdlTop->instances.size() == 3 &&
                   !requiredVhdlTop->instances[1].namedConnections[1].actualConnected &&
                   requiredVhdlTop->instances[2].hasNamedConnections &&
                   requiredVhdlTop->instances[2].hasPositionalConnections,
               "VHDL open actuals and legal mixed association shape remain explicit");

    const core::SourceFile namedParameters(
        "rtl/named_parameters.sv", core::HdlLanguage::SystemVerilog,
R"(module parameter_child #(
  parameter integer WIDTH = 8,
  parameter type VALUE_T = logic,
  parameter DEPTH = 4
)(input logic a, output logic y);
assign y = a;
endmodule
module no_parameter_child(input logic a, output logic y);
assign y = a;
endmodule
module parameter_top(input logic a, output logic y);
parameter_child #(.WIDTH(16), .TYPO(2), .WIDTH(32)) u_bad(.a(a), .y(y));
parameter_child #(16, logic, 4) u_positional(.a(a), .y(y));
no_parameter_child #(.GHOST(1)) u_none(.a(a), .y(y));
endmodule
)");
    const auto namedParameterResult = analyzer.analyzeSources({namedParameters});
    test.check(countRule(namedParameterResult,
                         "STRUCT-DUPLICATE-NAMED-PARAMETER") == 1,
               "duplicate SystemVerilog named parameter override is reported once");
    test.check(countRule(namedParameterResult,
                         "STRUCT-UNKNOWN-NAMED-PARAMETER") == 2,
               "unknown overrides are reported for parameterized and parameterless targets");
    const auto* parameterChild = findUnit(namedParameterResult, "parameter_child");
    const auto* parameterTop = findUnit(namedParameterResult, "parameter_top");
    test.check(parameterChild && parameterChild->parameters.size() == 3 &&
                   parameterChild->parameters[0].name == "WIDTH" &&
                   parameterChild->parameters[1].name == "VALUE_T" &&
                   parameterChild->parameters[2].name == "DEPTH",
               "SystemVerilog value and type parameter declarations retain source order");
    test.check(parameterTop && parameterTop->instances.size() == 3 &&
                   parameterTop->instances[0].namedParameterOverrides.size() == 3 &&
                   parameterTop->instances[0].parameterOverrideCount == 3 &&
                   parameterTop->instances[0].hasNamedParameterOverrides &&
                   parameterTop->instances[1].hasPositionalParameterOverrides,
               "SystemVerilog named and positional parameter override shapes are retained");
    bool namedParameterContext = true;
    for (const auto& violation : namedParameterResult.violations) {
        if (violation.ruleId == "STRUCT-DUPLICATE-NAMED-PARAMETER" ||
            violation.ruleId == "STRUCT-UNKNOWN-NAMED-PARAMETER") {
            namedParameterContext = namedParameterContext &&
                                    violation.severity == analysis::Severity::Error &&
                                    violation.module == "parameter_top" && violation.signal;
        }
    }
    test.check(namedParameterContext,
               "named-parameter findings retain error severity, parent, and formal context");
    std::ostringstream namedParameterJson;
    report::writeJson(namedParameterResult, namedParameterJson);
    test.check(namedParameterJson.str().find(
                   "\"parameters\":[{\"name\":\"WIDTH\",\"hasDefault\":true") !=
                   std::string::npos &&
                   namedParameterJson.str().find(
                   "\"namedParameterOverrides\":[{\"formal\":\"WIDTH\"") !=
                   std::string::npos &&
                   namedParameterJson.str().find(
                   "\"parameterOverrideCount\":3,\"parameterOverrideStyle\":\"named\"") !=
                   std::string::npos,
               "JSON publishes parameter declarations and override shape");
    std::ostringstream namedParameterHtml;
    report::writeHtml(namedParameterResult, namedParameterHtml);
    test.check(namedParameterHtml.str().find("参数 <code>WIDTH</code>（有默认值）") !=
                   std::string::npos &&
                   namedParameterHtml.str().find("参数覆盖 3，命名关联") !=
                   std::string::npos,
               "HTML publishes parameters and override count/style");

    const core::SourceFile vhdlNamedGenerics(
        "rtl/vhdl_named_generics.vhd", core::HdlLanguage::Vhdl,
R"(entity generic_child is
  generic (WIDTH : positive := 8; MODE, DEPTH : natural := 0);
  port (a : in std_logic; y : out std_logic);
end entity;
entity generic_top is
  port (a : in std_logic; y : out std_logic);
end entity;
architecture rtl of generic_top is begin
  u_bad : entity work.generic_child
    generic map (width => 16, MISSING => 1, WIDTH => 32)
    port map (a => a, y => y);
  u_positional : entity work.generic_child
    generic map (16, 1, 2) port map (a => a, y => y);
end architecture;
)");
    const auto vhdlGenericResult = analyzer.analyzeSources({vhdlNamedGenerics});
    test.check(countRule(vhdlGenericResult,
                         "STRUCT-DUPLICATE-NAMED-PARAMETER") == 1,
               "duplicate VHDL generic override is compared case-insensitively");
    test.check(countRule(vhdlGenericResult,
                         "STRUCT-UNKNOWN-NAMED-PARAMETER") == 1,
               "unknown VHDL generic override is reported once");
    const auto* genericChild = findUnit(vhdlGenericResult, "generic_child");
    const auto* genericTop = findUnit(vhdlGenericResult, "generic_top");
    test.check(genericChild && genericChild->parameters.size() == 3 &&
                   genericChild->parameters[1].name == "MODE" &&
                   genericChild->parameters[2].name == "DEPTH",
               "VHDL grouped generic declarations retain names and defaults");
    test.check(genericTop && genericTop->instances.size() == 2 &&
                   genericTop->instances[0].namedParameterOverrides.size() == 3 &&
                   genericTop->instances[1].hasPositionalParameterOverrides,
               "VHDL named and positional generic-map shapes are retained");

    const core::SourceFile requiredParameters(
        "rtl/required_parameters.sv", core::HdlLanguage::SystemVerilog,
R"(module required_parameter_child #(
  parameter WIDTH = 8,
  parameter type VALUE_T
)(input logic a, output logic y);
assign y = a;
endmodule
module required_parameter_top(input logic a, output logic y);
required_parameter_child u_missing(.a(a), .y(y));
required_parameter_child #(.WIDTH(16)) u_named_missing(.a(a), .y(y));
required_parameter_child #(.VALUE_T(logic)) u_named_ok(.a(a), .y(y));
required_parameter_child #(8, logic) u_positional_ok(.a(a), .y(y));
required_parameter_child #(8, logic, 3) u_many(.a(a), .y(y));
required_parameter_child #(8, .VALUE_T(logic)) u_mixed(.a(a), .y(y));
endmodule
)");
    const auto requiredParameterResult = analyzer.analyzeSources({requiredParameters});
    test.check(countRule(requiredParameterResult,
                         "STRUCT-MISSING-REQUIRED-PARAMETER") == 2,
               "absent and incomplete named overrides report required type parameters");
    test.check(countRule(requiredParameterResult,
                         "STRUCT-POSITIONAL-PARAMETER-COUNT") == 1,
               "too many positional SystemVerilog parameter overrides are reported");
    test.check(countRule(requiredParameterResult,
                         "STRUCT-MIXED-PARAMETER-OVERRIDE-STYLE") == 1,
               "mixed SystemVerilog parameter override style is rejected");
    bool requiredParameterContext = true;
    for (const auto& violation : requiredParameterResult.violations) {
        if (violation.ruleId == "STRUCT-MISSING-REQUIRED-PARAMETER") {
            requiredParameterContext = requiredParameterContext &&
                                       violation.severity == analysis::Severity::Error &&
                                       violation.module == "required_parameter_top" &&
                                       violation.signal == "VALUE_T";
        }
    }
    test.check(requiredParameterContext,
               "missing required parameter findings retain parent and parameter context");

    const core::SourceFile vhdlRequiredGenerics(
        "rtl/vhdl_required_generics.vhd", core::HdlLanguage::Vhdl,
R"(entity required_generic_child is
  generic (WIDTH : positive := 8; MODE : natural);
  port (a : in std_logic; y : out std_logic);
end entity;
entity required_generic_top is
  port (a : in std_logic; y : out std_logic);
end entity;
architecture rtl of required_generic_top is begin
  u_missing : entity work.required_generic_child port map (a => a, y => y);
  u_named_missing : entity work.required_generic_child
    generic map (WIDTH => 16) port map (a => a, y => y);
  u_named_ok : entity work.required_generic_child
    generic map (MODE => 1) port map (a => a, y => y);
  u_positional_ok : entity work.required_generic_child
    generic map (16, 1) port map (a => a, y => y);
  u_mixed_ok : entity work.required_generic_child
    generic map (16, MODE => 1) port map (a => a, y => y);
  u_many : entity work.required_generic_child
    generic map (16, 1, 2) port map (a => a, y => y);
end architecture;
)");
    const auto vhdlRequiredGenericResult = analyzer.analyzeSources({vhdlRequiredGenerics});
    test.check(countRule(vhdlRequiredGenericResult,
                         "STRUCT-MISSING-REQUIRED-PARAMETER") == 2,
               "VHDL required generics are checked for absent and incomplete maps");
    test.check(countRule(vhdlRequiredGenericResult,
                         "STRUCT-POSITIONAL-PARAMETER-COUNT") == 1,
               "too many positional VHDL generic overrides are reported");
    test.check(!hasRule(vhdlRequiredGenericResult,
                        "STRUCT-MIXED-PARAMETER-OVERRIDE-STYLE"),
               "VHDL positional associations followed by named generic associations remain legal");

    const core::SourceFile vhdlAssociationOrder(
        "rtl/vhdl_association_order.vhd", core::HdlLanguage::Vhdl,
R"(entity association_child is
  generic (WIDTH : positive := 8; MODE : natural := 0);
  port (a : in std_logic; b : in std_logic; y : out std_logic);
end entity;
entity association_top is
  port (a : in std_logic; y : out std_logic);
end entity;
architecture rtl of association_top is begin
  u_overlap : entity work.association_child
    generic map (8, WIDTH => 16)
    port map (a, a => a, b => a, y => y);
  u_order : entity work.association_child
    generic map (MODE => 1, 8)
    port map (b => a, a, y => y);
end architecture;
)");
    const auto vhdlAssociationResult = analyzer.analyzeSources({vhdlAssociationOrder});
    test.check(countRule(vhdlAssociationResult,
                         "STRUCT-DUPLICATE-NAMED-PARAMETER") == 1,
               "VHDL generic covered positionally and by name is reported as duplicate");
    test.check(countRule(vhdlAssociationResult,
                         "STRUCT-DUPLICATE-NAMED-PORT") == 1,
               "VHDL port covered positionally and by name is reported as duplicate");
    test.check(countRule(vhdlAssociationResult,
                         "STRUCT-VHDL-ASSOCIATION-ORDER") == 2,
               "VHDL generic and port maps reject positional associations after named ones");
    const auto* associationTop = findUnit(vhdlAssociationResult, "association_top");
    test.check(associationTop && associationTop->instances.size() == 2 &&
                   associationTop->instances[1].invalidParameterAssociationOrder &&
                   associationTop->instances[1].invalidPortAssociationOrder,
               "instance model retains invalid generic-map and port-map ordering state");
    std::ostringstream associationJson;
    report::writeJson(vhdlAssociationResult, associationJson);
    test.check(associationJson.str().find("\"invalidParameterAssociationOrder\":true") !=
                   std::string::npos &&
                   associationJson.str().find("\"invalidPortAssociationOrder\":true") !=
                   std::string::npos,
               "JSON publishes invalid VHDL association ordering state");

    const core::SourceFile duplicateInterfaceDeclarations(
        "rtl/duplicate_interface_declarations.sv", core::HdlLanguage::SystemVerilog,
R"(module duplicate_interface #(
  parameter WIDTH = 8,
  parameter WIDTH = 16
)(input logic a, input logic a, output logic y);
assign y = a;
endmodule
)");
    const auto duplicateInterfaceResult =
        analyzer.analyzeSources({duplicateInterfaceDeclarations});
    test.check(countRule(duplicateInterfaceResult,
                         "STRUCT-DUPLICATE-PARAMETER-DECLARATION") == 1,
               "duplicate SystemVerilog parameter declaration is reported");
    test.check(countRule(duplicateInterfaceResult,
                         "STRUCT-DUPLICATE-PORT-DECLARATION") == 1,
               "duplicate SystemVerilog ANSI port declaration is reported");
    const auto* duplicateInterface = findUnit(
        duplicateInterfaceResult, "duplicate_interface");
    test.check(duplicateInterface &&
                   duplicateInterface->duplicateParameterDeclarations.size() == 1 &&
                   duplicateInterface->duplicatePortDeclarations.size() == 1,
               "duplicate SystemVerilog interface declarations retain second locations");

    const core::SourceFile vhdlDuplicateInterfaceDeclarations(
        "rtl/vhdl_duplicate_interface_declarations.vhd", core::HdlLanguage::Vhdl,
R"(entity duplicate_vhdl_interface is
  generic (WIDTH : natural := 8; width : natural := 16);
  port (A : in std_logic; a : in std_logic; y : out std_logic);
end entity;
)");
    const auto vhdlDuplicateInterfaceResult =
        analyzer.analyzeSources({vhdlDuplicateInterfaceDeclarations});
    test.check(countRule(vhdlDuplicateInterfaceResult,
                         "STRUCT-DUPLICATE-PARAMETER-DECLARATION") == 1,
               "duplicate VHDL generic declaration is compared case-insensitively");
    test.check(countRule(vhdlDuplicateInterfaceResult,
                         "STRUCT-DUPLICATE-PORT-DECLARATION") == 1,
               "duplicate VHDL port declaration is compared case-insensitively");
    bool duplicateDeclarationContext = true;
    for (const auto& violation : vhdlDuplicateInterfaceResult.violations) {
        if (violation.ruleId == "STRUCT-DUPLICATE-PARAMETER-DECLARATION" ||
            violation.ruleId == "STRUCT-DUPLICATE-PORT-DECLARATION") {
            duplicateDeclarationContext = duplicateDeclarationContext &&
                                          violation.severity == analysis::Severity::Error &&
                                          violation.module == "duplicate_vhdl_interface" &&
                                          violation.signal;
        }
    }
    test.check(duplicateDeclarationContext,
               "duplicate declaration findings retain error, unit, and symbol context");

    const core::SourceFile inlineWaiver("rtl/waived.sv", core::HdlLanguage::SystemVerilog,
R"(module waived(input logic clk, input logic d, output logic q);
always_ff @(posedge clk) begin
  q = d; // fpga-lint-disable-line VL-SEQ-BLOCKING
end
always_comb begin
  // fpga-lint-disable-next-line VL-COMB-NONBLOCKING
  q <= d;
end
endmodule
)");
    const auto waived = analyzer.analyzeSource(inlineWaiver);
    test.check(!hasRule(waived, "VL-SEQ-BLOCKING") &&
                   !hasRule(waived, "VL-COMB-NONBLOCKING"),
               "line and next-line waiver directives suppress matching rules");
    test.check(waived.suppressedViolations == 2, "suppressed findings are counted");

    const core::SourceFile fileWaiver("rtl/file_waiver.sv", core::HdlLanguage::SystemVerilog,
R"(// fpga-lint-disable-file VL-CASEX
module file_waiver(input logic value);
always_comb begin
  casex (value)
    1'b0: value = 1'b1;
  endcase
end
endmodule
)");
    const auto fileWaived = analyzer.analyzeSource(fileWaiver);
    test.check(!hasRule(fileWaived, "VL-CASEX") &&
                   hasRule(fileWaived, "VL-CASE-DEFAULT"),
               "file waiver suppresses only its selected rule");

    const analysis::Analyzer disabledAnalyzer({{"VL-CASEX"}, {}, {}, {}, {}, {}});
    std::string manyFindings = "module many_findings;\n";
    for (int i = 0; i < 300; ++i) manyFindings += "wire data" + std::to_string(i) + "; \n";
    manyFindings += "endmodule\n";
    const auto unwaivedMany = analyzer.analyzeSource(
        {"rtl/many_findings.sv", core::HdlLanguage::SystemVerilog, manyFindings});
    test.check(std::ranges::count_if(unwaivedMany.violations, [](const auto& v) {
        return v.ruleId == "STYLE-001";
    }) == 300, "many findings remain intact with cached waiver source");
    manyFindings += "// fpga-lint-disable-file style-001\n";
    const auto waivedMany = analyzer.analyzeSource(
        {"rtl/many_findings.sv", core::HdlLanguage::SystemVerilog, manyFindings});
    test.check(!hasRule(waivedMany, "STYLE-001") && waivedMany.suppressedViolations == 300,
               "late file waiver remains case-insensitive across many findings");
    const auto disabled = disabledAnalyzer.analyzeSource(verilog);
    test.check(!hasRule(disabled, "VL-CASEX") && hasRule(disabled, "VL-CASE-DEFAULT"),
               "global rule disable only suppresses the selected rule");

    const analysis::Analyzer severityAnalyzer(
        {{}, {}, {}, {}, {}, {{"VL-CASEX", analysis::Severity::Error},
                              {"VL-CASE-DEFAULT", analysis::Severity::Info}}});
    const auto severityResult = severityAnalyzer.analyzeSource(verilog);
    const auto caseX = std::ranges::find_if(severityResult.violations,
        [](const auto& violation) { return violation.ruleId == "VL-CASEX"; });
    const auto caseDefault = std::ranges::find_if(severityResult.violations,
        [](const auto& violation) { return violation.ruleId == "VL-CASE-DEFAULT"; });
    test.check(caseX != severityResult.violations.end() &&
                   caseX->severity == analysis::Severity::Error &&
                   caseDefault != severityResult.violations.end() &&
                   caseDefault->severity == analysis::Severity::Info,
               "per-rule severity overrides change final diagnostic levels");
    std::ostringstream severityJson;
    report::writeJson(severityResult, severityJson);
    test.check(severityJson.str().find("\"severityOverrides\":[{\"ruleId\":\"VL-CASEX\","
                                       "\"severity\":\"error\"}") != std::string::npos,
               "JSON report records effective severity overrides");
    std::ostringstream severitySarif;
    report::writeSarif(severityResult, severitySarif);
    test.check(severitySarif.str().find("\"severityOverrides\":[{\"ruleId\":\"VL-CASEX\"") !=
                   std::string::npos,
               "SARIF run properties record effective severity overrides");
    std::ostringstream severityHtml;
    report::writeHtml(severityResult, severityHtml);
    test.check(severityHtml.str().find("<h2>严重级覆盖</h2>") != std::string::npos &&
                   severityHtml.str().find("<code>VL-CASEX</code>") != std::string::npos,
               "HTML report renders effective severity overrides");

    std::ostringstream json;
    report::writeJson(verilogResult, json);
    test.check(json.str().find("\"schemaVersion\":5") != std::string::npos,
               "JSON report has a versioned schema");
    test.check(json.str().find("\"ruleId\":\"VL-CASEX\"") != std::string::npos,
               "JSON report contains violations");
    std::ostringstream hierarchyJson;
    report::writeJson(hierarchy, hierarchyJson);
    test.check(hierarchyJson.str().find("\"target\":\"leaf\"") != std::string::npos,
               "JSON report contains design hierarchy");
    std::ostringstream blackBoxJson;
    report::writeJson(blackBoxHierarchy, blackBoxJson);
    test.check(blackBoxJson.str().find("\"external\":true") != std::string::npos,
               "JSON report identifies external black-box instances");
    std::ostringstream timingJson;
    report::writeJson(verilogTimingResult, timingJson);
    test.check(timingJson.str().find("\"clocks\":[{\"signal\":\"clk\",\"edge\":\"rising\"") !=
                   std::string::npos,
               "JSON report contains source-mapped clock information");
    test.check(timingJson.str().find("\"signal\":\"reset\",\"activeLevel\":\"low\","
                                     "\"asynchronous\":true") != std::string::npos &&
                   timingJson.str().find("\"signal\":\"sync_reset\",\"activeLevel\":\"high\","
                                         "\"asynchronous\":false") != std::string::npos,
               "JSON report distinguishes asynchronous and synchronous resets");
    std::ostringstream timingText;
    report::writeText(verilogTimingResult, timingText);
    test.check(timingText.str().find("时序结构：1 个时钟事件，2 个复位域，2 个时序过程，"
                                     "0 个已认证 CDC 同步器。") !=
                   std::string::npos,
               "text report summarizes timing structure");

    std::ostringstream sarif;
    report::writeSarif(verilogResult, sarif);
    test.check(sarif.str().find("\"version\":\"2.1.0\"") != std::string::npos,
               "SARIF report declares version 2.1.0");
    test.check(sarif.str().find("\"physicalLocation\"") != std::string::npos &&
                   sarif.str().find("\"startLine\":3") != std::string::npos,
               "SARIF report contains physical source locations");
    test.check(sarif.str().find("\"id\":\"PP-INCLUDE-NOT-FOUND\"") != std::string::npos &&
                   sarif.str().find("\"id\":\"RTL-LATCH-INFERRED\"") != std::string::npos &&
                   sarif.str().find("\"id\":\"VHDL-WAIT\"") != std::string::npos,
               "SARIF tool metadata contains the complete rule catalog, including unmatched rules");

    analysis::AnalysisResult unsafeText;
    unsafeText.filesAnalyzed = 1;
    unsafeText.violations.push_back({"TEST-HTML", analysis::Severity::Warning,
        "Unsafe <script>alert('x')</script>", "A & B", "Use \"safe\" text",
        "rtl/a&b.sv", 1, 2, std::nullopt, std::nullopt});
    std::ostringstream html;
    report::writeHtml(unsafeText, html);
    test.check(html.str().starts_with("<!doctype html>"), "HTML report has a document type");
    test.check(html.str().find("<script>alert") == std::string::npos &&
                   html.str().find("rtl/a&amp;b.sv") != std::string::npos,
               "HTML report escapes diagnostic and path content");
    analysis::AnalysisResult summaryResult;
    summaryResult.filesAnalyzed = 1;
    summaryResult.violations.push_back({"RULE-A", analysis::Severity::Critical,
        "critical", "description", "recommendation", "rtl/a.sv", 1, 1,
        std::nullopt, std::nullopt});
    summaryResult.violations.push_back({"RULE-A", analysis::Severity::Error,
        "error", "description", "recommendation", "rtl/a.sv", 2, 1,
        std::nullopt, std::nullopt});
    summaryResult.violations.push_back({"RULE-B", analysis::Severity::Warning,
        "warning", "description", "recommendation", "rtl/a.sv", 3, 1,
        std::nullopt, std::nullopt});
    summaryResult.violations.push_back({"RULE-C", analysis::Severity::Info,
        "info", "description", "recommendation", "rtl/a.sv", 4, 1,
        std::nullopt, std::nullopt});
    std::ostringstream summaryJson;
    report::writeJson(summaryResult, summaryJson);
    test.check(summaryJson.str().find(
                   "\"severityCounts\":{\"critical\":1,\"error\":1,\"warning\":1,\"info\":1}") !=
                   std::string::npos &&
                   summaryJson.str().find(
                   "\"ruleCounts\":[{\"ruleId\":\"RULE-A\",\"count\":2}") !=
                   std::string::npos,
               "JSON summary aggregates effective severities and deterministic rule counts");
    std::ostringstream summaryText;
    report::writeText(summaryResult, summaryText);
    test.check(summaryText.str().find(
                   "级别统计：严重 1，错误 1，警告 1，提示 1。") !=
                   std::string::npos &&
                   summaryText.str().find("规则统计： RULE-A=2 RULE-B=1 RULE-C=1。") !=
                   std::string::npos,
               "text report publishes severity and rule totals");
    std::ostringstream summarySarif;
    report::writeSarif(summaryResult, summarySarif);
    test.check(summarySarif.str().find("\"severityCounts\":{\"critical\":1") !=
                   std::string::npos &&
                   summarySarif.str().find("\"ruleId\":\"RULE-A\",\"count\":2") !=
                   std::string::npos,
               "SARIF run properties publish severity and rule totals");
    std::ostringstream summaryHtml;
    report::writeHtml(summaryResult, summaryHtml);
    test.check(summaryHtml.str().find("<span>严重</span>") != std::string::npos &&
                   summaryHtml.str().find("<h2>规则汇总</h2>") != std::string::npos &&
                   summaryHtml.str().find("<code>RULE-A</code></td><td>2") !=
                       std::string::npos,
               "HTML dashboard renders severity cards and rule summary table");
    analysis::AnalysisResult standardIdResult;
    standardIdResult.standardId = "GJB9765-2020";
    standardIdResult.violations.push_back({"GJB9765.4.8.e", analysis::Severity::Warning,
        "empty statement", "description", "recommendation", "rtl/a.vhd", 4, 1,
        std::nullopt, std::nullopt});
    standardIdResult.standardAssessments.push_back(
        {"GJB9765.4.8.e", analysis::StandardRuleState::Violated});
    auto unknownLineResult = standardIdResult;
    unknownLineResult.violations.front().line = 0;
    std::ostringstream unknownLineSarif, unknownLineJson;
    report::writeSarif(unknownLineResult, unknownLineSarif);
    report::writeJson(unknownLineResult, unknownLineJson);
    test.check(unknownLineSarif.str().find("\"ruleId\":\"A-8-1\"") != std::string::npos &&
                   unknownLineSarif.str().find("rtl/a.vhd") != std::string::npos &&
                   unknownLineSarif.str().find("\"region\"") == std::string::npos &&
                   unknownLineJson.str().find("\"line\":0") != std::string::npos,
               "unknown native lines retain file-level SARIF diagnostics and raw JSON line zero");
    auto unknownColumnResult = standardIdResult;
    unknownColumnResult.violations.front().column = 0;
    std::ostringstream unknownColumnSarif;
    report::writeSarif(unknownColumnResult, unknownColumnSarif);
    test.check(unknownColumnSarif.str().find("\"startLine\":4") != std::string::npos &&
                   unknownColumnSarif.str().find("\"startColumn\"") == std::string::npos,
               "a known SARIF line does not invent an unknown column");
    std::ostringstream standardIdJson, standardIdText, standardIdSarif, standardIdHtml;
    report::writeJson(standardIdResult, standardIdJson);
    report::writeText(standardIdResult, standardIdText);
    report::writeSarif(standardIdResult, standardIdSarif);
    report::writeHtml(standardIdResult, standardIdHtml);
    test.check(standardIdJson.str().find("\"ruleId\":\"A-8-1\"") !=
                   std::string::npos &&
                   standardIdJson.str().find("GJB9765.4.8.e") == std::string::npos &&
                   standardIdText.str().find("A-8-1") != std::string::npos &&
                   standardIdSarif.str().find("\"ruleId\":\"A-8-1\"") !=
                       std::string::npos &&
                   standardIdSarif.str().find("GJB9765.4.8.e") == std::string::npos &&
                   standardIdHtml.str().find("<code>A-8-1</code>") !=
                       std::string::npos,
               "all public report formats use only the official three-part R/A rule IDs");
    std::ostringstream timingHtml;
    report::writeHtml(verilogTimingResult, timingHtml);
    test.check(timingHtml.str().find("时钟 <code>clk</code>（上升沿）") != std::string::npos &&
                   timingHtml.str().find("复位 <code>reset</code>（低电平有效，异步）") !=
                       std::string::npos,
               "HTML hierarchy renders clock and reset attributes");

    std::ostringstream text;
    report::writeText(vhdlResult, text);
    test.check(text.str().find("rtl/risky.vhd:2:") != std::string::npos,
               "text report uses compiler-style source locations");
    test.check(text.str().find("已分析 1 个文件") != std::string::npos,
               "text report includes summary");

    return test.result();
}
