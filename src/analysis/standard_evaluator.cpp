#include "analysis/standard_evaluator.h"

#include "analysis/analyzer.h"
#include "analysis/standard_profiles.h"
#include "parser/design_parser.h"
#include "parser/preprocessor.h"
#include "parser/semantic_analyzer.h"
#include "analysis/verilog_constant_expression.h"
#include "analysis/native_verilog_integer.h"
#include "analysis/verilog_function_checks.h"
#include "analysis/unknown_arithmetic.h"
#include "analysis/verilog_unknown_relation.h"
#include "analysis/default_assignment_unknown.h"
#include "analysis/verilog_case_labels.h"
#include "analysis/verilog_repeat_checks.h"
#include "analysis/function_recursion.h"
#include "analysis/verilog_index_checks.h"
#include "analysis/verilog_naming_declarations.h"
#include "analysis/vhdl_clock_control.h"
#include "analysis/vhdl_fsm_model.h"
#include "analysis/verilog_naming_shape.h"
#include "analysis/verilog_name_length.h"
#include "analysis/verilog_power_names.h"
#include "analysis/verilog_global_assignment.h"
#include "analysis/verilog_function_return_width.h"
#include "analysis/verilog_task_argument_width.h"
#include "analysis/verilog_override_assignment.h"
#include "analysis/verilog_multiple_reset.h"
#include "analysis/verilog_sized_literal.h"
#include "analysis/verilog_fsm_state_count.h"
#include "analysis/verilog_gate_feedback.h"
#include "analysis/verilog_power_initialization.h"

#include <algorithm>
#include <array>
#include <charconv>
#include <cctype>
#include <cstdint>
#include <functional>
#include <limits>
#include <map>
#include <optional>
#include <regex>
#include <set>
#include <sstream>
#include <string>
#include <tuple>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#endif

namespace fpga::analysis {
static std::map<std::string, std::size_t> nativeResetSignalWidths(
    const parser::DesignUnitInfo& unit, const core::SourceFile& source,
    const std::vector<parser::HdlToken>& tokens);
namespace {

struct DiagnosticMapping {
    std::string_view standardId;
    std::string_view analyzerRuleId;
    std::string_view clauseId;
};

constexpr std::array Mappings{
    DiagnosticMapping{"GJB10157-2021", "STRUCT-MIXED-PORT-CONNECTION-STYLE", "GJB10157.5.2.9"},
    DiagnosticMapping{"GJB10157-2021", "STRUCT-POSITIONAL-PORT-COUNT", "GJB10157.5.2.9"},
    DiagnosticMapping{"GJB10157-2021", "RTL-UNUSED-INPUT-PORT", "GJB10157.5.2.18"},
    DiagnosticMapping{"GJB10157-2021", "RTL-UNUSED-SIGNAL", "GJB10157.5.2.18"},
    // Native FUN_18052cd30 collects internal driven nets, collects identifier
    // load references, and reports the set difference.  Keep the broader
    // redundancy clause mapping too because a write-only net is also redundant.
    // ev_6bc1e894e3076c31b7a4412d4c1ff3a616a58190705bb50cb97af2a2335c0043
    DiagnosticMapping{"GJB10157-2021", "RTL-WRITE-ONLY-SIGNAL", "GJB10157.5.2.10"},
    DiagnosticMapping{"GJB10157-2021", "RTL-WRITE-ONLY-SIGNAL", "GJB10157.5.2.18"},
    DiagnosticMapping{"GJB10157-2021", "VL-DEFPARAM", "GJB10157.5.2.16"},
    // Kept as the catalog's executable-detector registration. Standard
    // translation below deliberately suppresses the generic aggregate; the
    // native-shaped source adapter emits GJB10157.6.2.2 directly.
    DiagnosticMapping{"GJB10157-2021", "RTL-MIXED-RESET-MODE", "GJB10157.6.2.2"},
    DiagnosticMapping{"GJB10157-2021", "RTL-COMBINATIONAL-LOOP", "GJB10157.7.2.1"},
    // The packaged 7.2.1 example also marks a combinational process with
    // incomplete assignment as violating. This is a documented subpredicate,
    // not a reconstruction of the native netlist-cycle implementation.
    DiagnosticMapping{"GJB10157-2021", "RTL-LATCH-INFERRED", "GJB10157.7.2.1"},
    DiagnosticMapping{"GJB10157-2021", "RTL-COMPARISON-WIDTH-MISMATCH", "GJB10157.8.1"},
    DiagnosticMapping{"GJB10157-2021", "RTL-BITWISE-WIDTH-MISMATCH", "GJB10157.8.2"},
    DiagnosticMapping{"GJB10157-2021", "RTL-MIXED-SIGNEDNESS-ARITHMETIC", "GJB10157.8.3"},
    DiagnosticMapping{"GJB10157-2021", "RTL-WIDTH-TRUNCATION", "GJB10157.8.7"},
    DiagnosticMapping{"GJB10157-2021", "RTL-ARITHMETIC-RESULT-WIDTH-RISK", "GJB10157.8.7"},
    DiagnosticMapping{"GJB10157-2021", "VL-COMB-NONBLOCKING", "GJB10157.9.1.3"},
    DiagnosticMapping{"GJB10157-2021", "VL-SEQ-BLOCKING", "GJB10157.9.1.4"},
    DiagnosticMapping{"GJB10157-2021", "RTL-NONSYNTHESIZABLE-DELAY", "GJB10157.9.1.6"},
    DiagnosticMapping{"GJB10157-2021", "RTL-COMBINATIONAL-LOOP", "GJB10157.9.4.7"},
    DiagnosticMapping{"GJB10157-2021", "RTL-OUT-OF-RANGE-SELECT", "GJB10157.9.4.8"},
    DiagnosticMapping{"GJB10157-2021", "RTL-INCOMPLETE-SENSITIVITY", "GJB10157.9.4.9"},
    DiagnosticMapping{"GJB10157-2021", "RTL-REDUNDANT-SENSITIVITY", "GJB10157.9.4.9"},
    DiagnosticMapping{"GJB9765-2020", "STRUCT-MIXED-PORT-CONNECTION-STYLE", "GJB9765.4.1.a"},
    DiagnosticMapping{"GJB9765-2020", "STRUCT-POSITIONAL-PORT-COUNT", "GJB9765.4.1.a"},
    DiagnosticMapping{"GJB9765-2020", "RTL-INCOMPLETE-SENSITIVITY", "GJB9765.4.3.a"},
    DiagnosticMapping{"GJB9765-2020", "RTL-REDUNDANT-SENSITIVITY", "GJB9765.4.3.a"},
    // Native FUN_1805d7f20 gathers declaration candidates (types 0x78/0x06)
    // and a distinct referenced-name set. For AST_ASSIGN_LE (0x56),
    // FUN_1805d8120 walks every child, including the target, so a signal that
    // appears only on an assignment LHS is used for this clause. The report
    // helper FUN_1805d7440 emits only candidates absent from that use set.
    // ev_91424f44d5d399dbcf2cde3ba70d8f0abf889530256558ba1a6dfb09aebf77d4
    DiagnosticMapping{"GJB9765-2020", "RTL-UNUSED-SIGNAL", "GJB9765.4.4.b"},
    // Native FUN_1805fa8a0 builds a VHDL signal candidate set and a distinct
    // driver set from assignments plus elaborated connections.  Its expression
    // helper FUN_1805fbf60 walks resolved identifier reads and reports a
    // candidate only when it is absent from the driver set, deduplicating by
    // signal name.  An unused declaration is therefore not a 4.6.a violation.
    // ev_f2d417d75228aa444f0bee46faed19340470eeea24e79afc2cfd3fff000e5ec0
    DiagnosticMapping{"GJB9765-2020", "RTL-UNDRIVEN-SIGNAL", "GJB9765.4.6.a"},
    DiagnosticMapping{"GJB9765-2020", "RTL-WIDTH-TRUNCATION", "GJB9765.4.6.b.1"},
    DiagnosticMapping{"GJB9765-2020", "RTL-COMPARISON-WIDTH-MISMATCH", "GJB9765.4.6.b.1"},
    DiagnosticMapping{"GJB9765-2020", "RTL-BITWISE-WIDTH-MISMATCH", "GJB9765.4.6.b.1"},
    DiagnosticMapping{"GJB9765-2020", "RTL-CASE-ITEM-WIDTH-MISMATCH", "GJB9765.4.6.b.2"},
    DiagnosticMapping{"GJB9765-2020", "VHDL-CASE-OTHERS", "GJB9765.4.8.c"},
    DiagnosticMapping{"GJB9765-2020", "RTL-RESET-POLARITY-CONFLICT", "GJB9765.4.10.j"},
    DiagnosticMapping{"GJB9765-2020", "VHDL-BUFFER-PORT", "GJB9765.4.12.c"},
};

// Every clause in this list has a dedicated check in directChecks().  Keeping
// the registry separate from the PDF-derived catalogue lets the tests reject a
// rule marked "automated" unless executable evidence exists for it.
constexpr std::array DirectClauses{
    std::string_view{"GJB10157.10.1"}, std::string_view{"GJB10157.10.2"},
    std::string_view{"GJB10157.10.3"}, std::string_view{"GJB10157.10.4"},
    std::string_view{"GJB10157.12.2"},
    std::string_view{"GJB10157.5.2.3"},
    std::string_view{"GJB10157.5.1.1"}, std::string_view{"GJB10157.5.1.2"},
    std::string_view{"GJB10157.5.1.3"}, std::string_view{"GJB10157.5.1.5"},
    std::string_view{"GJB10157.5.1.4"},
    std::string_view{"GJB10157.5.1.6"}, std::string_view{"GJB10157.5.1.7"},
    std::string_view{"GJB10157.5.1.8"},
    std::string_view{"GJB10157.5.2.5"}, std::string_view{"GJB10157.5.2.6"},
    std::string_view{"GJB10157.5.2.1"}, std::string_view{"GJB10157.5.2.2"},
    std::string_view{"GJB10157.5.2.4"},
    std::string_view{"GJB10157.5.2.7"}, std::string_view{"GJB10157.5.2.8"},
    std::string_view{"GJB10157.5.2.9"},
    std::string_view{"GJB10157.5.2.11"}, std::string_view{"GJB10157.5.2.12"},
    std::string_view{"GJB10157.5.2.13"}, std::string_view{"GJB10157.5.2.14"},
    std::string_view{"GJB10157.5.2.15"},
    std::string_view{"GJB10157.5.2.17"},
    std::string_view{"GJB10157.5.3.1"}, std::string_view{"GJB10157.5.3.2"},
    std::string_view{"GJB10157.5.3.3"}, std::string_view{"GJB10157.5.3.4"},
    std::string_view{"GJB10157.5.3.5"},
    std::string_view{"GJB10157.6.1.1"},
    std::string_view{"GJB10157.6.1.2"},
    std::string_view{"GJB10157.6.1.3"},
    std::string_view{"GJB10157.6.1.4"},
    std::string_view{"GJB10157.6.1.6"},
    std::string_view{"GJB10157.6.1.7"},
    std::string_view{"GJB10157.6.2.1"},
    std::string_view{"GJB10157.6.2.5"},
    std::string_view{"GJB10157.6.2.3"},
    std::string_view{"GJB10157.6.2.7"},
    std::string_view{"GJB10157.6.2.8"},
    std::string_view{"GJB10157.6.2.9"},
    std::string_view{"GJB10157.6.2.10"},
    std::string_view{"GJB10157.7.1.1"},
    std::string_view{"GJB10157.7.1.2"},
    std::string_view{"GJB10157.7.1.3"},
    std::string_view{"GJB10157.7.2.2"}, std::string_view{"GJB10157.7.2.3"},
    std::string_view{"GJB10157.7.3.1"},
    std::string_view{"GJB10157.7.3.2"},
    std::string_view{"GJB10157.7.3.3"},
    std::string_view{"GJB10157.9.1.1"},
    std::string_view{"GJB10157.9.1.2"},
    std::string_view{"GJB10157.9.2.1"},
    std::string_view{"GJB10157.9.2.3"},
    std::string_view{"GJB10157.9.2.7"},
    std::string_view{"GJB10157.9.2.10"},
    std::string_view{"GJB10157.9.2.11"},
    std::string_view{"GJB10157.9.3.4"},
    std::string_view{"GJB10157.9.3.2"},
    std::string_view{"GJB10157.9.3.1"},
    std::string_view{"GJB10157.9.3.3"},
    std::string_view{"GJB10157.8.4"}, std::string_view{"GJB10157.8.5"},
    std::string_view{"GJB10157.8.6"}, std::string_view{"GJB10157.8.9"},
    std::string_view{"GJB10157.8.8"},
    std::string_view{"GJB10157.9.4.1"}, std::string_view{"GJB10157.9.4.2"},
    std::string_view{"GJB10157.9.4.4"},
    std::string_view{"GJB10157.9.4.5"},
    std::string_view{"GJB10157.9.4.6"},
    std::string_view{"GJB10157.9.4.3"},
    std::string_view{"GJB10157.9.1.5"},
    std::string_view{"GJB10157.9.2.5"},
    std::string_view{"GJB10157.9.2.2"}, std::string_view{"GJB10157.9.2.4"},
    std::string_view{"GJB10157.9.2.6"},
    std::string_view{"GJB10157.9.2.8"}, std::string_view{"GJB10157.9.2.9"},
    std::string_view{"GJB10157.9.3.5"}, std::string_view{"GJB10157.9.3.6"},
    std::string_view{"GJB10157.9.4.10"},
    std::string_view{"GJB10157.12.1"}, std::string_view{"GJB10157.12.3"},
    std::string_view{"GJB9765.4.1.a"}, std::string_view{"GJB9765.4.10.h"},
    std::string_view{"GJB9765.4.14.d.3"},
    std::string_view{"GJB9765.4.14.d.4"},
    std::string_view{"GJB9765.4.14.c.2"},
    std::string_view{"GJB9765.4.14.c.3"},
    std::string_view{"GJB9765.4.14.c.1"},
    std::string_view{"GJB9765.4.14.a.1"},
    std::string_view{"GJB9765.4.14.b.1"},
    std::string_view{"GJB9765.4.14.b.2"},
    std::string_view{"GJB9765.4.14.b.3"},
    std::string_view{"GJB9765.4.14.b.4"},
    std::string_view{"GJB9765.4.14.b.5"},
    std::string_view{"GJB9765.4.14.b.6"},
    std::string_view{"GJB9765.4.8.a"},
    std::string_view{"GJB9765.4.8.b"},
    std::string_view{"GJB9765.4.8.d"},
    std::string_view{"GJB9765.4.8.e"},
    std::string_view{"GJB9765.4.2.b"}, std::string_view{"GJB9765.4.2.d"},
    std::string_view{"GJB9765.4.2.e"}, std::string_view{"GJB9765.4.2.f"},
    std::string_view{"GJB9765.4.4.a"},
    std::string_view{"GJB9765.4.5.g"},
    std::string_view{"GJB9765.4.6.c"}, std::string_view{"GJB9765.4.6.d"},
    std::string_view{"GJB9765.4.6.e"}, std::string_view{"GJB9765.4.6.g"},
    std::string_view{"GJB9765.4.10.a"}, std::string_view{"GJB9765.4.10.b"},
    std::string_view{"GJB9765.4.10.c"}, std::string_view{"GJB9765.4.10.d"},
    std::string_view{"GJB9765.4.10.e"}, std::string_view{"GJB9765.4.10.f"},
    std::string_view{"GJB9765.4.10.i"}, std::string_view{"GJB9765.4.10.k"},
    std::string_view{"GJB9765.4.6.h"},
    std::string_view{"GJB9765.4.6.i"},
    std::string_view{"GJB9765.4.9.a"}, std::string_view{"GJB9765.4.9.c"},
    std::string_view{"GJB9765.4.9.e"},
    std::string_view{"GJB9765.4.9.j"},
    std::string_view{"GJB9765.4.9.b"},
    std::string_view{"GJB9765.4.9.d"},
    std::string_view{"GJB9765.4.9.g"},
    std::string_view{"GJB9765.4.9.h"},
    std::string_view{"GJB9765.4.9.i"},
    std::string_view{"GJB9765.4.7.a"},
    std::string_view{"GJB9765.4.7.b"},
    std::string_view{"GJB9765.4.7.c"},
    std::string_view{"GJB9765.4.6.f"},
    std::string_view{"GJB9765.4.1.b"},
    std::string_view{"GJB9765.4.1.c"},
    std::string_view{"GJB9765.4.11.f.a"},
    std::string_view{"GJB9765.4.11.a"}, std::string_view{"GJB9765.4.11.c"},
    std::string_view{"GJB9765.4.11.d"}, std::string_view{"GJB9765.4.11.e"},
    std::string_view{"GJB9765.4.11.f.b"}, std::string_view{"GJB9765.4.11.f.c"},
    std::string_view{"GJB9765.4.12.a"}, std::string_view{"GJB9765.4.12.c"},
    std::string_view{"GJB9765.4.2.a"}, std::string_view{"GJB9765.4.2.c"},
    std::string_view{"GJB9765.4.2.g"},
    std::string_view{"GJB9765.4.4.c"}, std::string_view{"GJB9765.4.4.d"},
    std::string_view{"GJB9765.4.5.e"},
    std::string_view{"GJB9765.4.5.a"}, std::string_view{"GJB9765.4.5.b"},
    std::string_view{"GJB9765.4.5.c"},
    std::string_view{"GJB9765.4.5.d"},
    std::string_view{"GJB9765.4.5.f.1"}, std::string_view{"GJB9765.4.5.f.2"},
    std::string_view{"GJB9765.4.5.f.3"}, std::string_view{"GJB9765.4.5.f.4"},
    std::string_view{"GJB9765.4.5.f.5"}, std::string_view{"GJB9765.4.5.f.6"},
    std::string_view{"GJB9765.4.5.f.8"}, std::string_view{"GJB9765.4.5.f.9"},
    std::string_view{"GJB9765.4.5.f.10"},
    std::string_view{"GJB9765.4.5.f.14"}, std::string_view{"GJB9765.4.5.f.15"},
    std::string_view{"GJB9765.4.5.f.16"}, std::string_view{"GJB9765.4.5.f.18"},
    std::string_view{"GJB9765.4.5.f.19"}, std::string_view{"GJB9765.4.5.f.20"},
    std::string_view{"GJB9765.4.5.f.22"},
    std::string_view{"GJB9765.4.5.f.23"}, std::string_view{"GJB9765.4.5.f.24"},
    std::string_view{"GJB9765.4.5.f.26"}, std::string_view{"GJB9765.4.5.f.28"},
    std::string_view{"GJB9765.4.5.f.29"},
    std::string_view{"GJB9765.4.5.f.30"}, std::string_view{"GJB9765.4.13.a"},
    std::string_view{"GJB9765.4.13.b"}, std::string_view{"GJB9765.4.13.c"},
    std::string_view{"GJB9765.4.14.d.1"}, std::string_view{"GJB9765.4.14.d.2"},
};

std::string lower(std::string value) {
    std::ranges::transform(value, value.begin(),
        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return value;
}

std::string_view configuredVerilogTop(std::string_view name) {
    // Verilog escaped identifiers retain a leading '\\' in the native Yosys
    // key but the product lexer stores the identifier text without it.
    return name.starts_with('\\') ? name.substr(1) : name;
}

bool selectedTop(const parser::DesignUnitInfo& unit,
                 const AnalysisOptions& options) {
    if (!options.topModule) return unit.topLevel;
    if (unit.kind == parser::DesignUnitKind::VhdlEntity)
        return lower(unit.name) == lower(*options.topModule);
    return unit.name == configuredVerilogTop(*options.topModule);
}

bool nativeVerilogClockName(std::string_view name) {
    // Native 5.2.13 builds these two full-match regexes before reporting:
    // clk_[0-9a-z]+[0-9a-z_]* | Clk_[0-9A-Z]+[0-9A-Z_]*.
    // ev_dcb50dd75f1506a4f3dbcf18f09002f009f339d7b0325419d104fbfe2c4c1dba
    if (name.size() < 5) return false;
    const bool lowercase = name.starts_with("clk_");
    if (!lowercase && !name.starts_with("Clk_")) return false;
    const auto alnum = [lowercase](char c) {
        return (c >= '0' && c <= '9') ||
            (lowercase ? (c >= 'a' && c <= 'z') : (c >= 'A' && c <= 'Z'));
    };
    if (!alnum(name[4])) return false;
    return std::ranges::all_of(name.substr(5), [&](char c) {
        return alnum(c) || c == '_';
    });
}

std::string nativeVerilogFileStem(const std::filesystem::path& path) {
    // Native 5.2.11 first replaces every '\\' with '/', then takes bytes
    // after the last slash and before the last dot.  A dot preceding the last
    // slash does not delimit the filename; a leading filename dot yields an
    // empty stem.  This intentionally does not use host filesystem semantics.
    auto normalized = path.string();
    std::ranges::replace(normalized, '\\', '/');
    const auto slash = normalized.rfind('/');
    const auto begin = slash == std::string::npos ? 0 : slash + 1;
    const auto dot = normalized.rfind('.');
    const auto end = dot == std::string::npos || dot < begin ? normalized.size() : dot;
    return normalized.substr(begin, end - begin);
}

bool nativeVerilogResetName(std::string_view name) {
    // Native 5.2.14 checks lowercase rst_/reset_ or uppercase RST_/Reset_,
    // then a polarity suffix (_n/_p or _N/_P) or an interior marker (_n_/_p_
    // or _N_/_P_). Its packaged prose mentions Rst_, but the binary literals
    // are RST_. Preserve that discrepancy rather than silently broadening.
    // ev_29543dbe1bd7a19cae64cf4e7f68104436a1560be3e97ca6bbe32f584be2c598
    // ev_94d61d36751872130c99d88ea9fed972d66e226bf6adeb51b34da825864c0b6b
    const auto polarityMarker = [](std::string_view value, char negative, char positive) {
        const std::string n = std::string{"_"} + negative;
        const std::string p = std::string{"_"} + positive;
        // FUN_18052f910 calls string::find from offset zero and compares that
        // first occurrence with length-2. A later suffix does not rescue an
        // earlier non-suffix marker (for example rst_nx_n).
        const auto suffix = value.size() >= 2 ? value.size() - 2 : std::string_view::npos;
        return value.find(n) == suffix || value.find(p) == suffix ||
            value.find(n + "_") != std::string_view::npos ||
            value.find(p + "_") != std::string_view::npos;
    };
    if (name.starts_with("rst_") || name.starts_with("reset_"))
        return polarityMarker(name, 'n', 'p');
    if (name.starts_with("RST_") || name.starts_with("Reset_"))
        return polarityMarker(name, 'N', 'P');
    return false;
}

const StandardRuleEntry* inventoryEntry(std::string_view clauseId) {
    const auto found = std::ranges::find_if(standardRuleInventory(), [&](const auto& entry) {
        return entry.clauseId == clauseId;
    });
    return found == standardRuleInventory().end() ? nullptr : &*found;
}

void emit(AnalysisResult& result, std::string_view clauseId,
          const core::SourceFile& source, std::size_t line, std::size_t column,
          std::string message, std::optional<std::string> signal = std::nullopt) {
    const auto* entry = inventoryEntry(clauseId);
    const auto detail = findStandardRuleDetail(clauseId);
    if (!entry || !detail) return;
    result.violations.push_back({std::string(clauseId), entry->severity, std::move(message),
        std::string(detail->summary), "按照该 GJB 条款及标准中的遵循示例修改代码。",
        source.path(), line, column, std::nullopt, std::move(signal)});
}

void emitAtLocation(AnalysisResult& result, std::string_view clauseId,
                    const core::SourceLocation& location, std::string message) {
    const auto* entry = inventoryEntry(clauseId);
    const auto detail = findStandardRuleDetail(clauseId);
    if (!entry || !detail) return;
    result.violations.push_back({std::string(clauseId), entry->severity, std::move(message),
        std::string(detail->summary), "按照该 GJB 条款及标准中的遵循示例修改代码。",
        location.file, location.line, location.column, std::nullopt, std::nullopt});
}

bool applicable(const core::SourceFile& source, std::string_view standardId) {
    if (standardId == "GJB10157-2021") {
        return source.language() == core::HdlLanguage::Verilog ||
               source.language() == core::HdlLanguage::SystemVerilog;
    }
    return standardId == "GJB9765-2020" && source.language() == core::HdlLanguage::Vhdl;
}

std::vector<std::string> linesOf(const std::string& content) {
    std::vector<std::string> lines;
    std::istringstream stream(content);
    for (std::string line; std::getline(stream, line);) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        lines.push_back(std::move(line));
    }
    return lines;
}

// The recovered VHDL text rules use fopen(..., "r") plus fgets(). T-Linter is
// a Windows PE, so reproduce ordinary CRLF text translation before splitting
// the resulting byte stream into the caller's recovered maximum payload size.
std::vector<std::string> nativeVhdlTextChunks(const std::string& content,
                                              std::size_t maximumBytes = 254) {
    std::string translated;
    translated.reserve(content.size());
    for (std::size_t index = 0; index < content.size(); ++index) {
        if (content[index] == '\r' && index + 1 < content.size() &&
            content[index + 1] == '\n') {
            translated.push_back('\n');
            ++index;
        } else {
            translated.push_back(content[index]);
        }
    }
    std::vector<std::string> chunks;
    for (std::size_t offset = 0; offset < translated.size();) {
        const auto start = offset;
        std::size_t count = 0;
        while (offset < translated.size() && count < maximumBytes) {
            const auto value = translated[offset++];
            ++count;
            if (value == '\n') break;
        }
        chunks.emplace_back(translated.substr(start, count));
    }
    return chunks;
}

void checkVhdlPortDeclarationComments(AnalysisResult& result,
                                      const core::SourceFile& source,
                                      const std::vector<std::string>& lines) {
    if (!parser::checkSyntax(source).empty()) return;
    for (const auto& unit : result.structure) {
        if (unit.kind != parser::DesignUnitKind::VhdlEntity ||
            unit.location.file.lexically_normal() != source.path().lexically_normal()) continue;
        for (const auto& port : unit.ports) {
            const auto line = port.location.line;
            if (line == 0 || line > lines.size()) continue;
            // Native FUN_1806b1620 seeks to the identifier node's physical
            // line and performs a raw byte search for the exact pair "--".
            // It does not inspect an adjacent comment or require that the
            // pair lexically begins a comment.
            // ev_8e80f92a8bcb56dba75d1df83d2670364220dcf892a20ed0da0c94bd7785365b
            if (lines[line - 1].find("--") != std::string::npos) continue;
            emit(result, "GJB9765.4.13.b", source, line, port.location.column,
                 "端口 " + port.name + " 的定义行缺少注释");
        }
    }
}

void checkVhdlInternalDeclarationComments(AnalysisResult& result,
                                          const core::SourceFile& source,
                                          const std::vector<std::string>& lines) {
    if (!parser::checkSyntax(source).empty()) return;
    const auto tokens = parser::tokenize(source);
    std::size_t portDepth = 0;
    bool enteringPort = false;
    for (std::size_t i = 0; i < tokens.size(); ++i) {
        const auto& token = tokens[i];
        if (token.kind == parser::HdlTokenKind::Identifier && lower(token.text) == "port" &&
            i + 1 < tokens.size() && tokens[i + 1].text == "(") enteringPort = true;
        if (token.text == "(") {
            if (enteringPort) {
                portDepth = 1;
                enteringPort = false;
            } else if (portDepth != 0) {
                ++portDepth;
            }
            continue;
        }
        if (token.text == ")" && portDepth != 0) {
            --portDepth;
            continue;
        }
        if (portDepth != 0 || token.kind != parser::HdlTokenKind::Identifier) continue;
        const auto kind = lower(token.text);
        if (kind != "signal" && kind != "variable") continue;

        std::vector<const parser::HdlToken*> names;
        std::size_t at = i + 1;
        for (; at < tokens.size() && tokens[at].text != ":"; ++at) {
            if (tokens[at].text == ";" || tokens[at].text == "(") break;
            if (tokens[at].kind == parser::HdlTokenKind::Identifier)
                names.push_back(&tokens[at]);
        }
        if (at == tokens.size() || tokens[at].text != ":") continue;
        for (const auto* name : names) {
            const auto line = name->line;
            if (line == 0 || line > lines.size() ||
                lines[line - 1].find("--") != std::string::npos) continue;
            // Native FUN_1805bc5a0 collects declaration types 0x78/0x79,
            // seeks to each declaration node's +0x214 physical line, and
            // performs the same raw exact-line "--" byte search as 4.13.b.
            // ev_20c19ea2c50eacc7b9803363001f4de61e5b26a8472669f5334fd11ce412ca36
            emit(result, "GJB9765.4.13.c", source, line, name->column,
                 std::string(kind == "signal" ? "内部信号 " : "变量 ") +
                 name->text + " 的定义行缺少注释");
        }
    }
}

std::size_t tab4DisplayColumn(const std::string& line, std::size_t sourceColumn) {
    std::size_t display = 0;
    const auto prefix = std::min(sourceColumn > 0 ? sourceColumn - 1 : 0, line.size());
    for (std::size_t i = 0; i < prefix; ++i) {
        if (line[i] == '\t') display += 4 - display % 4;
        else ++display;
    }
    return display + 1;
}

std::size_t nativeExpandedIndent(std::string_view line) {
    std::size_t column = 0;
    for (const char c : line) {
        if (c == ' ') ++column;
        else if (c == '\t') column += 4;
        else break;
    }
    return column;
}

std::size_t nativePortDeclarationColumn(std::string_view line) {
    std::string expanded;
    expanded.reserve(line.size());
    for (const char c : line) {
        if (c == '\t') expanded += "    ";
        else expanded.push_back(c);
    }
    // FUN_180527d90 searches these literals in this exact order, then falls
    // back to indentation for a continued declaration without a direction.
    for (const auto keyword : {std::string_view{"input"}, std::string_view{"output"},
                               std::string_view{"inout"}}) {
        const auto found = expanded.find(keyword);
        if (found != std::string::npos) return found;
    }
    // FUN_180527d90 leaves the failed uint32 search result at 0xffffffff,
    // then increments it once for every leading space. Preserve that wrap:
    // a continued declaration with N>0 spaces has fallback column N-1.
    const auto indent = nativeExpandedIndent(line);
    return indent == 0 ? std::numeric_limits<std::uint32_t>::max() : indent - 1;
}

void checkVerilogDeclarationLeftAlignment(
    AnalysisResult& result, const core::SourceFile& source,
    const std::vector<parser::HdlToken>& tokens, const std::vector<std::string>& lines) {
    // Native GJB10157.5.1.3 admits AST_MODULE(2), recursively collects
    // AST_WIRE(6), separates port nodes by input/output bytes +0xa0/+0xa1,
    // and establishes one first-record baseline for each group.
    // ev_5184e185fe9f98ae94b96102f5e80121c0b163833724e2af2ecf4029abd35091
    // Parsed source locations are a bounded adapter; duplicate wire nodes on
    // the same physical line are deliberately coalesced into one diagnostic.
    const auto declarationType = [](std::string_view word) {
        static constexpr std::array types{
            std::string_view{"wire"}, std::string_view{"reg"},
            std::string_view{"logic"}, std::string_view{"bit"},
            std::string_view{"byte"}, std::string_view{"shortint"},
            std::string_view{"int"}, std::string_view{"longint"},
            std::string_view{"integer"}, std::string_view{"time"},
            std::string_view{"realtime"}, std::string_view{"tri"},
            std::string_view{"tri0"}, std::string_view{"tri1"},
            std::string_view{"supply0"}, std::string_view{"supply1"},
            std::string_view{"wand"}, std::string_view{"wor"},
            std::string_view{"trireg"}, std::string_view{"uwire"}
        };
        return std::ranges::find(types, word) != types.end();
    };
    const auto qualifier = [&](std::string_view word) {
        return declarationType(word) || word == "signed" || word == "unsigned" ||
               word == "automatic" || word == "static" || word == "var" ||
               word == "const";
    };
    const auto sourceLine = [&](std::size_t line) -> std::string_view {
        return line > 0 && line <= lines.size() ? std::string_view{lines[line - 1]}
                                                : std::string_view{};
    };
    using Candidate = std::pair<std::size_t, std::string>;
    const auto reportGroup = [&](const std::vector<Candidate>& group, bool ports) {
        if (group.size() < 2) return;
        const auto column = [&](std::size_t line) {
            return ports ? nativePortDeclarationColumn(sourceLine(line))
                         : nativeExpandedIndent(sourceLine(line));
        };
        const auto expected = column(group.front().first);
        for (std::size_t index = 1; index < group.size(); ++index) {
            if (column(group[index].first) == expected) continue;
            emit(result, "GJB10157.5.1.3", source, group[index].first, 1,
                 group[index].second + "定义左端未对齐");
        }
    };

    for (std::size_t module = 0; module < tokens.size(); ++module) {
        if (tokens[module].text != "module") continue;
        std::size_t moduleEnd = module + 1;
        while (moduleEnd < tokens.size() && tokens[moduleEnd].text != "endmodule")
            ++moduleEnd;

        std::vector<Candidate> portLines;
        for (const auto& unit : result.structure) {
            if (unit.kind != parser::DesignUnitKind::VerilogModule ||
                unit.location.file.lexically_normal() != source.path().lexically_normal() ||
                unit.location.line < tokens[module].line ||
                (moduleEnd < tokens.size() && unit.location.line > tokens[moduleEnd].line)) continue;
            for (const auto& port : unit.ports)
                portLines.emplace_back(port.location.line, port.name);
            break;
        }
        std::ranges::stable_sort(portLines, {}, [](const auto& item) { return item.first; });
        portLines.erase(std::unique(portLines.begin(), portLines.end(),
            [](const auto& left, const auto& right) { return left.first == right.first; }),
            portLines.end());

        std::vector<Candidate> internalLines;
        for (std::size_t index = module + 1; index < moduleEnd; ++index) {
            if (!declarationType(lower(tokens[index].text))) continue;
            bool expectName = true;
            int squareDepth = 0;
            int roundDepth = 0;
            int braceDepth = 0;
            for (std::size_t cursor = index + 1; cursor < moduleEnd; ++cursor) {
                const auto& token = tokens[cursor];
                const auto word = lower(token.text);
                if (token.text == ";" && squareDepth == 0 && roundDepth == 0 && braceDepth == 0) {
                    index = cursor;
                    break;
                }
                if (token.text == "[") { ++squareDepth; continue; }
                if (token.text == "]") { if (squareDepth > 0) --squareDepth; continue; }
                if (token.text == "(") { ++roundDepth; continue; }
                if (token.text == ")") { if (roundDepth > 0) --roundDepth; continue; }
                if (token.text == "{") { ++braceDepth; continue; }
                if (token.text == "}") { if (braceDepth > 0) --braceDepth; continue; }
                if (squareDepth != 0 || roundDepth != 0 || braceDepth != 0) continue;
                if (token.text == ",") { expectName = true; continue; }
                if (!expectName || token.kind != parser::HdlTokenKind::Identifier || qualifier(word))
                    continue;
                expectName = false;
                const bool portLine = std::ranges::any_of(portLines, [&](const auto& item) {
                    return item.first == token.line;
                });
                if (!portLine) internalLines.emplace_back(token.line, token.text);
            }
        }
        std::ranges::stable_sort(internalLines, {}, [](const auto& item) { return item.first; });
        internalLines.erase(std::unique(internalLines.begin(), internalLines.end(),
            [](const auto& left, const auto& right) { return left.first == right.first; }),
            internalLines.end());
        reportGroup(portLines, true);
        reportGroup(internalLines, false);
        module = moduleEnd;
    }
}

// Native GJB9765.4.14.b.1 scans fgets() chunks and accepts a leading Tab
// before the next recognised code byte. These two byte sets are the literal
// comparisons recovered from FUN_1805bda30, not a VHDL lexer or a style
// preference. ev_18d4943dfaaee8f0aa7b3c890ddda29c8b39c5fe4fd31f7481114ce8ba9ee4f6
bool nativeIndentCodeByte(unsigned char c, bool initial) {
    const auto alpha = initial ? static_cast<unsigned char>(c + 0x9f) < 0x1a
                               : static_cast<unsigned char>(c + 0x9e) < 0x18;
    const auto offset = static_cast<unsigned>(c) - 0x22u;
    const auto mask = initial ? 0x2fffffff9fffbfd1ULL : 0x2effffff1f7f3fd1ULL;
    return alpha || (offset < 0x3e && ((mask >> (offset & 0x3f)) & 1ULL) != 0) || c == '\n';
}

bool nativeVhdlLeadingSpaceViolation(std::string_view line) {
    for (std::size_t i = 0; i < line.size(); ++i) {
        const auto c = static_cast<unsigned char>(line[i]);
        if (c == '\t') return false;
        if (nativeIndentCodeByte(c, i == 0)) return i != 0;
    }
    return !line.empty();
}

bool nativeVhdlPreviousLineHasCode(std::string_view line) {
    for (std::size_t i = 0; i < line.size(); ++i) {
        if (line[i] == '-' && i + 1 < line.size() && line[i + 1] == '-') break;
        if (line[i] != ' ' && line[i] != '\t' && line[i] != '\r' && line[i] != '\n')
            return true;
    }
    return false;
}

// The public lexer excludes comments and preserves source locations. Join its
// single-character symbols here, without changing the general HDL parser.
std::vector<parser::HdlToken> standardTokens(const core::SourceFile& source) {
    auto raw = parser::tokenize(source);
    std::vector<parser::HdlToken> tokens;
    const bool vhdl = source.language() == core::HdlLanguage::Vhdl;
    for (std::size_t i = 0; i < raw.size(); ++i) {
        auto token = raw[i];
        if (token.text == "'" && i + 1 < raw.size() &&
            raw[i + 1].offset == token.offset + 1) {
            if (vhdl && i + 2 < raw.size() && raw[i + 1].text.size() == 1 &&
                raw[i + 2].text == "'" && raw[i + 2].offset == token.offset + 2) {
                token.text += raw[i + 1].text + "'";
                token.kind = parser::HdlTokenKind::Number;
                i += 2;
            } else if (!vhdl && (raw[i + 1].kind == parser::HdlTokenKind::Identifier ||
                                raw[i + 1].kind == parser::HdlTokenKind::Number)) {
                token.text += raw[++i].text;
                token.kind = parser::HdlTokenKind::Number;
            }
        } else if (token.kind == parser::HdlTokenKind::Symbol && i + 1 < raw.size() &&
                   raw[i + 1].offset == token.offset + token.text.size()) {
            const auto pair = token.text + raw[i + 1].text;
            if (pair == "==" || pair == "!=" || pair == "<=" || pair == ">=" ||
                pair == "/=" || pair == "?=" || pair == ":=" || pair == "&&" || pair == "||" ||
                pair == "=>" || pair == "**" || (!vhdl && (pair == "<<" ||
                pair == ">>" || pair == "::" || pair == "+:" || pair == "-:" ||
                pair == "~&" || pair == "~|" || pair == "~^" || pair == "^~"))) {
                token.text = pair;
                ++i;
                if (!vhdl && (pair == "==" || pair == "!=") && i + 1 < raw.size() &&
                    (raw[i + 1].text == "=" || raw[i + 1].text == "?") &&
                    raw[i + 1].offset == token.offset + 2) {
                    token.text += raw[i + 1].text;
                    ++i;
                } else if (!vhdl && (pair == "<<" || pair == ">>") && i + 1 < raw.size() &&
                           raw[i + 1].text == pair.substr(0, 1) &&
                           raw[i + 1].offset == token.offset + 2) {
                    token.text += raw[i + 1].text;
                    ++i;
                }
            }
        }
        tokens.push_back(std::move(token));
    }
    return tokens;
}

// The native rule compares driver structures rather than forwarding every
// generic multiple-driver finding. Its example explicitly permits several
// conditional concurrent assignments whose inactive branch is 'Z'. Keep this
// exemption narrow: both records must expose a direct data name, simple
// relation control and Z branch, and every pair must resolve the same control
// source. An ordinary/complex driver or different control source still
// reports. ev_982a769700f868228aa94da64e031116a9f0b1037f4c9199e0811e3831a9582c
// ev_a0769ec98ec45f1313372f18cd322b34143713336780574d1f0b0958a2c8a15b
bool explicitConditionalTriStateDrivers(const std::vector<parser::HdlToken>& tokens,
                                        std::string_view signal) {
    std::size_t assignments = 0;
    std::optional<std::string> commonControl;
    for (std::size_t i = 0; i + 2 < tokens.size(); ++i) {
        if (lower(tokens[i].text) != signal || tokens[i + 1].text != "<=") continue;
        ++assignments;
        std::size_t end = i + 2;
        while (end < tokens.size() && tokens[end].text != ";") ++end;
        if (end == tokens.size() || end < i + 8 || lower(tokens[end - 2].text) != "else" ||
            lower(tokens[end - 1].text) != "'z'") return false;

        std::size_t when = i + 2;
        while (when < end && lower(tokens[when].text) != "when") ++when;
        if (when != i + 3 || tokens[i + 2].kind != parser::HdlTokenKind::Identifier)
            return false; // Native helper requires a direct data identifier.

        const auto relationBegin = when + 1;
        const auto relationEnd = end - 2;
        std::size_t relationToken = relationEnd;
        for (auto token = relationBegin; token < relationEnd; ++token) {
            const auto& text = tokens[token].text;
            if (text != "=" && text != "/=" && text != "<" && text != "<=" &&
                text != ">" && text != ">=") continue;
            if (relationToken != relationEnd) return false;
            relationToken = token;
        }
        if (relationToken == relationBegin || relationToken + 1 >= relationEnd)
            return false;
        const auto simpleAtom = [&](std::size_t begin, std::size_t atomEnd) {
            if (begin + 1 == atomEnd)
                return tokens[begin].kind == parser::HdlTokenKind::Identifier ||
                       tokens[begin].kind == parser::HdlTokenKind::Number ||
                       tokens[begin].kind == parser::HdlTokenKind::String;
            return begin + 2 == atomEnd && tokens[begin].text == "'" &&
                   !tokens[begin + 1].text.empty() &&
                   tokens[begin + 1].text.back() == '\'';
        };
        if (!simpleAtom(relationBegin, relationToken) ||
            !simpleAtom(relationToken + 1, relationEnd)) return false;
        std::optional<std::string> control;
        if (relationBegin + 1 == relationToken &&
            tokens[relationBegin].kind == parser::HdlTokenKind::Identifier)
            control = lower(tokens[relationBegin].text);
        else if (relationToken + 2 == relationEnd &&
                 tokens[relationToken + 1].kind == parser::HdlTokenKind::Identifier)
            control = lower(tokens[relationToken + 1].text);
        if (!control) return false;
        if (!commonControl) commonControl = *control;
        else if (*commonControl != *control) return false;
        i = end;
    }
    return assignments > 1;
}

void checkVhdlMultipleDrivers(AnalysisResult& result, const core::SourceFile& source,
                              const std::vector<parser::HdlToken>& tokens) {
    std::set<std::string> reported;
    const auto genericIssues = result.violations;
    for (const auto& issue : genericIssues) {
        if (issue.ruleId != "RTL-MULTIPLE-DRIVERS" || issue.file != source.path() ||
            !issue.signal) continue;
        const auto name = lower(*issue.signal);
        if (explicitConditionalTriStateDrivers(tokens, name)) continue;
        if (reported.insert(name).second)
            emit(result, "GJB9765.4.6.f", source, issue.line, issue.column,
                 "同一模块中的信号存在多个独立驱动源：" + *issue.signal);
    }

    // A component OUT port is a driver of its mapped actual even though it
    // does not appear as a local <= assignment. Admit only a declared OUT
    // formal and a simple named association; unresolved bindings need review.
    std::map<std::string, std::set<std::string>> outputs;
    for (std::size_t i = 0; i + 4 < tokens.size(); ++i) {
        if (lower(tokens[i].text) != "component" ||
            lower(tokens[i + 2].text) != "is") continue;
        const auto component = lower(tokens[i + 1].text);
        std::size_t port = i + 3;
        while (port + 1 < tokens.size() && lower(tokens[port].text) != "port" &&
               lower(tokens[port].text) != "begin" &&
               lower(tokens[port].text) != "end") ++port;
        if (port + 1 >= tokens.size() || lower(tokens[port].text) != "port" ||
            tokens[port + 1].text != "(") continue;
        std::size_t segment = port + 2;
        for (std::size_t j = segment; j < tokens.size(); ++j) {
            if (tokens[j].text != ";" && tokens[j].text != ")") continue;
            std::size_t colon = segment;
            while (colon < j && tokens[colon].text != ":") ++colon;
            if (colon + 1 < j && (lower(tokens[colon + 1].text) == "out" ||
                                  lower(tokens[colon + 1].text) == "inout")) {
                for (std::size_t name = segment; name < colon; ++name)
                    if (tokens[name].kind == parser::HdlTokenKind::Identifier)
                        outputs[component].insert(lower(tokens[name].text));
            }
            segment = j + 1;
            if (tokens[j].text == ")") break;
        }
    }
    std::size_t architecture = 0;
    std::vector<std::size_t> architectureAt(tokens.size());
    for (std::size_t i = 0; i < tokens.size(); ++i) {
        if (lower(tokens[i].text) == "architecture" &&
            (i == 0 || lower(tokens[i - 1].text) != "end")) ++architecture;
        architectureAt[i] = architecture;
    }
    for (std::size_t i = 2; i + 2 < tokens.size(); ++i) {
        if (lower(tokens[i].text) != "port" || lower(tokens[i + 1].text) != "map" ||
            tokens[i + 2].text != "(") continue;
        std::size_t label = i;
        while (label > 0 && i - label < 16 && tokens[label].text != ":" &&
               tokens[label].text != ";") --label;
        if (tokens[label].text != ":" || label + 1 >= i) continue;
        const auto component = lower(tokens[label + 1].text);
        const auto known = outputs.find(component);
        if (known == outputs.end()) continue;
        for (std::size_t j = i + 3; j + 2 < tokens.size() && tokens[j].text != ")"; ++j) {
            if (tokens[j + 1].text != "=>" ||
                !known->second.contains(lower(tokens[j].text)) ||
                tokens[j + 2].kind != parser::HdlTokenKind::Identifier) continue;
            const auto actual = lower(tokens[j + 2].text);
            if (!reported.insert(actual).second) continue;
            bool locallyDriven = false;
            for (std::size_t k = 0; k + 1 < tokens.size(); ++k) {
                if (architectureAt[k] == architectureAt[i] &&
                    lower(tokens[k].text) == actual && tokens[k + 1].text == "<=") {
                    locallyDriven = true;
                    break;
                }
            }
            if (locallyDriven)
                emit(result, "GJB9765.4.6.f", source, tokens[j + 2].line,
                     tokens[j + 2].column,
                     "组件输出端口与本地赋值共同驱动信号：" + tokens[j + 2].text);
            else reported.erase(actual);
        }
    }
}

struct VhdlComponentPort {
    std::string name;
    parser::PortDirection direction{parser::PortDirection::Unknown};
    bool hasDefault{false};
};

std::vector<std::size_t> vhdlArchitectureOrdinals(
    const std::vector<parser::HdlToken>& tokens) {
    std::vector<std::size_t> ordinals(tokens.size());
    std::size_t current = 0;
    for (std::size_t i = 0; i < tokens.size(); ++i) {
        if (lower(tokens[i].text) == "architecture" &&
            (i == 0 || lower(tokens[i - 1].text) != "end")) ++current;
        ordinals[i] = current;
    }
    return ordinals;
}

std::map<std::pair<std::size_t, std::string>, std::vector<VhdlComponentPort>>
localVhdlComponentPorts(const std::vector<parser::HdlToken>& tokens,
                        const std::vector<std::size_t>& architectureAt) {
    std::map<std::pair<std::size_t, std::string>, std::vector<VhdlComponentPort>> components;
    for (std::size_t i = 0; i + 1 < tokens.size(); ++i) {
        if (lower(tokens[i].text) != "component" ||
            (i > 0 && lower(tokens[i - 1].text) == "end") ||
            tokens[i + 1].kind != parser::HdlTokenKind::Identifier) continue;
        const auto component = lower(tokens[i + 1].text);
        std::size_t end = i + 2;
        while (end + 1 < tokens.size() &&
               !(lower(tokens[end].text) == "end" &&
                 lower(tokens[end + 1].text) == "component")) ++end;
        if (end + 1 >= tokens.size()) continue;
        const auto key = std::pair{architectureAt[i], component};
        for (std::size_t port = i + 2; port + 1 < end; ++port) {
            if (lower(tokens[port].text) != "port" || tokens[port + 1].text != "(") continue;
            const auto appendSegment = [&](std::size_t first, std::size_t last) {
                std::size_t colon = first;
                while (colon < last && tokens[colon].text != ":") ++colon;
                if (colon == last) return;
                parser::PortDirection direction = parser::PortDirection::Unknown;
                bool hasDefault = false;
                for (std::size_t k = colon + 1; k < last; ++k) {
                    const auto word = lower(tokens[k].text);
                    if (word == "in") direction = parser::PortDirection::Input;
                    else if (word == "out" || word == "buffer")
                        direction = parser::PortDirection::Output;
                    else if (word == "inout") direction = parser::PortDirection::Inout;
                    else if (tokens[k].text == ":=") hasDefault = true;
                }
                for (std::size_t k = first; k < colon; ++k) {
                    if (tokens[k].kind == parser::HdlTokenKind::Identifier)
                        components[key].push_back({tokens[k].text, direction, hasDefault});
                }
            };
            std::size_t first = port + 2;
            int depth = 0;
            for (std::size_t k = first; k < end; ++k) {
                if (tokens[k].text == "(" || tokens[k].text == "[") ++depth;
                else if (tokens[k].text == ")" || tokens[k].text == "]") {
                    if (depth == 0) {
                        appendSegment(first, k);
                        break;
                    }
                    --depth;
                }
                if (depth == 0 && tokens[k].text == ";") {
                    appendSegment(first, k);
                    first = k + 1;
                }
            }
            break;
        }
        i = end + 1;
    }
    return components;
}

void checkVhdlComponentPortConnections(AnalysisResult& result, const core::SourceFile& source,
                                       const std::vector<parser::HdlToken>& tokens) {
    const auto architectureAt = vhdlArchitectureOrdinals(tokens);
    const auto components = localVhdlComponentPorts(tokens, architectureAt);
    const auto explicitOpen = [&](std::size_t line, std::size_t column, bool named) {
        const auto found = std::ranges::find_if(tokens, [&](const auto& token) {
            return token.line == line && token.column == column;
        });
        if (found == tokens.end()) return false;
        const auto index = static_cast<std::size_t>(found - tokens.begin());
        return named ? index + 2 < tokens.size() && tokens[index + 1].text == "=>" &&
                           lower(tokens[index + 2].text) == "open"
                     : lower(tokens[index].text) == "open";
    };
    for (const auto& unit : result.structure) {
        if (unit.location.file.lexically_normal() != source.path().lexically_normal()) continue;
        for (const auto& instance : unit.instances) {
            const auto targetToken = std::ranges::find_if(tokens, [&](const auto& token) {
                return token.line == instance.location.line &&
                       token.column == instance.location.column;
            });
            if (targetToken == tokens.end()) continue;
            const auto tokenIndex = static_cast<std::size_t>(targetToken - tokens.begin());
            if (!instance.hasPortConnectionList) continue;
            std::vector<VhdlComponentPort> ports;
            if (instance.resolved && !instance.external) {
                const auto target = std::ranges::find_if(result.structure, [&](const auto& candidate) {
                    return candidate.kind == parser::DesignUnitKind::VhdlEntity &&
                           lower(candidate.name) == lower(instance.target);
                });
                if (target != result.structure.end()) {
                    for (const auto& port : target->ports)
                        ports.push_back({port.name, port.direction, port.hasDefault});
                }
            } else {
                const auto found = components.find(
                    {architectureAt[tokenIndex], lower(instance.target)});
                if (found != components.end()) ports = found->second;
            }
            for (std::size_t index = 0; index < ports.size(); ++index) {
                const auto& port = ports[index];
                if (port.direction != parser::PortDirection::Input &&
                    port.direction != parser::PortDirection::Output) continue;
                bool connected = false;
                bool present = false;
                bool open = false;
                std::size_t line = instance.location.line;
                std::size_t column = instance.location.column;
                if (index < instance.positionalConnections.size()) {
                    const auto& positional = instance.positionalConnections[index];
                    present = true;
                    connected = positional.actualConnected;
                    line = positional.location.line;
                    column = positional.location.column;
                    open = explicitOpen(line, column, false);
                }
                for (const auto& named : instance.namedConnections) {
                    if (lower(named.formal) != lower(port.name)) continue;
                    present = true;
                    connected = named.actualConnected;
                    line = named.location.line;
                    column = named.location.column;
                    open = explicitOpen(line, column, true);
                }
                if (port.direction == parser::PortDirection::Input &&
                    !connected && !port.hasDefault)
                    emit(result, "GJB9765.4.1.b", source, line, column,
                         "实例的必需输入端口未连接：" + instance.name + "." + port.name);
                if (port.direction == parser::PortDirection::Output &&
                    (!present || (!connected && !open)))
                    emit(result, "GJB9765.4.1.c", source, line, column,
                         "未使用的输出端口应显式映射为 open：" + instance.name + "." + port.name);
            }
        }
    }
}

void checkVhdlResetPortsNotOpen(AnalysisResult& result, const core::SourceFile& source,
                                const std::vector<parser::HdlToken>& tokens) {
    // Native GJB9765_VHDL.4.10.d entry FUN_1805aee10 builds component/entity
    // connection relations and reports the connection record when a reset
    // formal is not driven. Its direct diagnostic is "未被驱动,处于悬空.", and
    // helper FUN_1805b04b0 follows the same relation graph for unresolved
    // records. The packaged violating/compliant pair differs only at
    // `rst_n => open` versus `rst_n => rst_n`.
    //
    // Keep this source adapter deliberately bounded: a formal must be a
    // declared input and have the reset spelling established by the packaged
    // examples/rule naming model. Missing associations and arbitrary open
    // input ports remain under their dedicated connectivity rule instead of
    // being guessed as reset ports here.
    // ev_4_10_d_native_entry_1805aee10
    // ev_4_10_d_native_helper_1805b04b0
    const auto resetFormal = [](std::string name) {
        name = lower(std::move(name));
        return name.starts_with("rst") || name.starts_with("reset");
    };
    const auto architectureAt = vhdlArchitectureOrdinals(tokens);
    const auto components = localVhdlComponentPorts(tokens, architectureAt);
    const auto explicitOpen = [&](std::size_t line, std::size_t column, bool named) {
        const auto found = std::ranges::find_if(tokens, [&](const auto& token) {
            return token.line == line && token.column == column;
        });
        if (found == tokens.end()) return false;
        const auto index = static_cast<std::size_t>(found - tokens.begin());
        return named ? index + 2 < tokens.size() && tokens[index + 1].text == "=>" &&
                           lower(tokens[index + 2].text) == "open"
                     : lower(tokens[index].text) == "open";
    };

    for (const auto& unit : result.structure) {
        if (unit.location.file.lexically_normal() != source.path().lexically_normal()) continue;
        for (const auto& instance : unit.instances) {
            if (!instance.hasPortConnectionList) continue;
            const auto targetToken = std::ranges::find_if(tokens, [&](const auto& token) {
                return token.line == instance.location.line &&
                    token.column == instance.location.column;
            });
            if (targetToken == tokens.end()) continue;
            const auto tokenIndex = static_cast<std::size_t>(targetToken - tokens.begin());

            std::vector<VhdlComponentPort> ports;
            if (instance.resolved && !instance.external) {
                const auto target = std::ranges::find_if(result.structure, [&](const auto& candidate) {
                    return candidate.kind == parser::DesignUnitKind::VhdlEntity &&
                        lower(candidate.name) == lower(instance.target);
                });
                if (target != result.structure.end()) {
                    for (const auto& port : target->ports)
                        ports.push_back({port.name, port.direction, port.hasDefault});
                }
            } else if (const auto found = components.find(
                           {architectureAt[tokenIndex], lower(instance.target)});
                       found != components.end()) {
                ports = found->second;
            }
            for (std::size_t index = 0; index < ports.size(); ++index) {
                const auto& port = ports[index];
                if (port.direction != parser::PortDirection::Input ||
                    !resetFormal(port.name)) continue;

                bool open = false;
                std::size_t line = instance.location.line;
                std::size_t column = instance.location.column;
                if (index < instance.positionalConnections.size()) {
                    const auto& positional = instance.positionalConnections[index];
                    line = positional.location.line;
                    column = positional.location.column;
                    open = explicitOpen(line, column, false);
                }
                for (const auto& named : instance.namedConnections) {
                    if (lower(named.formal) != lower(port.name)) continue;
                    line = named.location.line;
                    column = named.location.column;
                    open = explicitOpen(line, column, true);
                }
                if (open)
                    emit(result, "GJB9765.4.10.d", source, line, column,
                         "寄存器复位端口未被驱动，处于悬空：" +
                             instance.name + "." + port.name);
            }
        }
    }
}

void checkVhdlSingleResetDomain(AnalysisResult& result, const core::SourceFile& source,
                                const std::vector<parser::HdlToken>& tokens) {
    // Native GJB9765_VHDL.4.10.f slot 0 FUN_18069fcf0 calls FUN_1804c5a00
    // for each elaborated instance target, sorts and de-duplicates two
    // 32-byte name collections, and enters its comparison loop only when one
    // collection has exactly one member and the other has more than one.
    // FUN_1804c5a00 fills those collections from the target's semantic clock
    // and asynchronous-reset port metadata. The comparison keeps the first
    // reset actual as its baseline and reports a later, different actual.
    // The packaged pair fixes the observable boundary: SUBMOD1 has one CLK
    // formal and two ARST formals, so different ARST actuals report; SUBMOD2
    // has two CLK formals and is deliberately outside this rule even when its
    // two ARST actuals differ.
    //
    // The proprietary semantic-port classifier is unavailable here. Admit
    // only declared input formals with the packaged clock/reset spelling and
    // direct identifier actuals. Expressions, OPEN/omitted associations,
    // custom semantic names and unresolved positional shape remain review-
    // required rather than being treated as compliant proof.
    // ev_4_10_f_native_entry_18069fcf0
    // ev_4_10_f_native_port_sets_1804c5a00
    const auto clockFormal = [](std::string name) {
        name = lower(std::move(name));
        return name.starts_with("clk") || name.starts_with("clock");
    };
    const auto resetFormal = [](std::string name) {
        name = lower(std::move(name));
        return name.starts_with("arst") || name.starts_with("rst") ||
            name.starts_with("reset") || name.starts_with("srst");
    };
    const auto architectureAt = vhdlArchitectureOrdinals(tokens);
    const auto components = localVhdlComponentPorts(tokens, architectureAt);

    struct DirectActual {
        std::string normalized;
        std::string spelling;
        std::size_t line{};
        std::size_t column{};
    };
    const auto tokenAt = [&](std::size_t line, std::size_t column) {
        return std::ranges::find_if(tokens, [&](const auto& token) {
            return token.line == line && token.column == column;
        });
    };
    const auto namedActual = [&](const parser::NamedPortConnectionInfo& connection)
        -> std::optional<DirectActual> {
        const auto formal = tokenAt(connection.location.line, connection.location.column);
        if (formal == tokens.end()) return std::nullopt;
        const auto index = static_cast<std::size_t>(formal - tokens.begin());
        if (!connection.actualConnected || index + 3 >= tokens.size() ||
            tokens[index + 1].text != "=>" ||
            tokens[index + 2].kind != parser::HdlTokenKind::Identifier ||
            (tokens[index + 3].text != "," && tokens[index + 3].text != ")"))
            return std::nullopt;
        return DirectActual{lower(tokens[index + 2].text), tokens[index + 2].text,
                            tokens[index + 2].line, tokens[index + 2].column};
    };
    const auto positionalActual = [&](const parser::PositionalPortConnectionInfo& connection)
        -> std::optional<DirectActual> {
        const auto actual = tokenAt(connection.location.line, connection.location.column);
        if (actual == tokens.end()) return std::nullopt;
        const auto index = static_cast<std::size_t>(actual - tokens.begin());
        if (!connection.actualConnected ||
            actual->kind != parser::HdlTokenKind::Identifier ||
            index + 1 >= tokens.size() ||
            (tokens[index + 1].text != "," && tokens[index + 1].text != ")"))
            return std::nullopt;
        return DirectActual{lower(actual->text), actual->text,
                            actual->line, actual->column};
    };

    for (const auto& unit : result.structure) {
        if (unit.location.file.lexically_normal() != source.path().lexically_normal()) continue;
        for (const auto& instance : unit.instances) {
            if (!instance.hasPortConnectionList) continue;
            const auto target = tokenAt(instance.location.line, instance.location.column);
            if (target == tokens.end()) continue;
            const auto targetIndex = static_cast<std::size_t>(target - tokens.begin());

            std::vector<VhdlComponentPort> ports;
            if (instance.resolved && !instance.external) {
                const auto entity = std::ranges::find_if(result.structure, [&](const auto& candidate) {
                    return candidate.kind == parser::DesignUnitKind::VhdlEntity &&
                        lower(candidate.name) == lower(instance.target);
                });
                if (entity != result.structure.end()) {
                    for (const auto& port : entity->ports)
                        ports.push_back({port.name, port.direction, port.hasDefault});
                }
            } else if (const auto found = components.find(
                           {architectureAt[targetIndex], lower(instance.target)});
                       found != components.end()) {
                ports = found->second;
            }
            if (ports.empty()) continue;

            std::vector<std::size_t> clocks;
            std::vector<std::size_t> resets;
            for (std::size_t index = 0; index < ports.size(); ++index) {
                if (ports[index].direction != parser::PortDirection::Input) continue;
                if (clockFormal(ports[index].name)) clocks.push_back(index);
                else if (resetFormal(ports[index].name)) resets.push_back(index);
            }
            if (clocks.size() != 1 || resets.size() < 2) continue;

            const auto actualFor = [&](std::size_t port)
                -> std::optional<DirectActual> {
                for (const auto& named : instance.namedConnections)
                    if (lower(named.formal) == lower(ports[port].name))
                        return namedActual(named);
                if (port < instance.positionalConnections.size())
                    return positionalActual(instance.positionalConnections[port]);
                return std::nullopt;
            };
            // A direct clock binding is required before applying the recovered
            // one-clock-domain predicate to this source-level instance.
            if (!actualFor(clocks.front())) continue;

            std::optional<DirectActual> baseline;
            for (const auto reset : resets) {
                const auto actual = actualFor(reset);
                if (!actual) continue;
                if (!baseline) {
                    baseline = actual;
                    continue;
                }
                if (actual->normalized == baseline->normalized) continue;
                emit(result, "GJB9765.4.10.f", source, actual->line, actual->column,
                     "同一时钟域实例使用了不同复位域：" + instance.name + "（" +
                         baseline->spelling + " / " + actual->spelling + "）");
            }
        }
    }
}

void checkVhdlCrossHierarchySignalNames(
    AnalysisResult& result, const core::SourceFile& source,
    const std::vector<parser::HdlToken>& tokens) {
    // Native FUN_1806bc5c0 compares two case-folded names from connection
    // records assembled by FUN_1804d0140. The latter admits entity/component
    // instances (AST 0x93/0x94). FUN_1804d0140 handles named association
    // nodes (0x8b) by extracting the formal and actual, and handles direct
    // identifier positional actuals (0x13) by pairing them with the formal at
    // the same ordinal. FUN_1806bc5c0 calls the builder once per already-bound
    // root and supplies both project root maps, so a direct entity target may
    // be declared in a different source file. Keep expressions, OPEN and
    // ambiguous project bindings outside this source adapter.
    const auto architectureAt = vhdlArchitectureOrdinals(tokens);
    const auto components = localVhdlComponentPorts(tokens, architectureAt);
    const auto matchingEntityPorts = [&](std::string_view target)
        -> std::optional<std::vector<std::string>> {
        const auto normalizedTarget = lower(std::string{target});
        std::vector<const parser::DesignUnitInfo*> matches;
        for (const auto& candidate : result.structure)
            if (candidate.kind == parser::DesignUnitKind::VhdlEntity &&
                lower(candidate.name) == normalizedTarget)
                matches.push_back(&candidate);
        if (matches.size() != 1) return std::nullopt;
        std::vector<std::string> names;
        for (const auto& port : matches.front()->ports) names.push_back(lower(port.name));
        return names;
    };
    const auto matchingParen = [&](std::size_t open) {
        std::size_t depth = 0;
        for (auto index = open; index < tokens.size(); ++index) {
            if (tokens[index].text == "(") ++depth;
            else if (tokens[index].text == ")" && depth && --depth == 0) return index;
        }
        return tokens.size();
    };
    const auto reportMismatch = [&](std::string_view formal, std::size_t actual) {
        if (formal == lower(tokens[actual].text)) return;
        emit(result, "GJB9765.4.5.g", source,
             tokens[actual].line, tokens[actual].column,
             "跨层级信号命名不一致：" + std::string(formal) + " => " +
                 tokens[actual].text);
    };

    for (std::size_t label = 0; label + 4 < tokens.size(); ++label) {
        if (tokens[label].kind != parser::HdlTokenKind::Identifier ||
            tokens[label + 1].text != ":") continue;
        auto target = label + 2;
        const bool directEntity = lower(tokens[target].text) == "entity";
        if (directEntity) ++target;
        if (target >= tokens.size() ||
            tokens[target].kind != parser::HdlTokenKind::Identifier) continue;
        if (target + 2 < tokens.size() && tokens[target + 1].text == "." &&
            tokens[target + 2].kind == parser::HdlTokenKind::Identifier)
            target += 2;
        const auto targetName = lower(tokens[target].text);

        std::size_t portMap = tokens.size();
        for (auto cursor = target + 1; cursor + 2 < tokens.size(); ++cursor) {
            if (tokens[cursor].text == ";") break;
            if (lower(tokens[cursor].text) == "port" &&
                lower(tokens[cursor + 1].text) == "map" &&
                tokens[cursor + 2].text == "(") {
                portMap = cursor + 2;
                break;
            }
        }
        if (portMap == tokens.size()) continue;
        const auto close = matchingParen(portMap);
        if (close == tokens.size()) continue;

        std::vector<std::string> formalNames;
        if (directEntity) {
            const auto entity = matchingEntityPorts(targetName);
            if (!entity) continue;
            formalNames = *entity;
        } else if (const auto component = components.find(
                       {architectureAt[label], targetName});
                   component != components.end()) {
            for (const auto& port : component->second)
                formalNames.push_back(lower(port.name));
        } else {
            // The proprietary component binder is not yet recovered. A bare
            // component without a local declaration therefore remains review.
            continue;
        }
        if (formalNames.empty()) continue;

        struct AssociationSegment {
            std::size_t begin{};
            std::size_t end{};
            std::optional<std::size_t> arrow;
        };
        std::vector<AssociationSegment> associations;
        auto segment = portMap + 1;
        std::size_t depth = 0;
        for (auto cursor = segment; cursor <= close; ++cursor) {
            if (cursor < close && (tokens[cursor].text == "(" ||
                                   tokens[cursor].text == "[")) ++depth;
            else if (cursor < close && (tokens[cursor].text == ")" ||
                                        tokens[cursor].text == "]") && depth) --depth;
            if (cursor != close && !(depth == 0 && tokens[cursor].text == ","))
                continue;
            std::optional<std::size_t> arrow;
            std::size_t nested = 0;
            for (auto item = segment; item < cursor; ++item) {
                if (tokens[item].text == "(" || tokens[item].text == "[") ++nested;
                else if ((tokens[item].text == ")" || tokens[item].text == "]") && nested)
                    --nested;
                else if (nested == 0 && tokens[item].text == "=>") {
                    arrow = item;
                    break;
                }
            }
            if (segment < cursor) associations.push_back({segment, cursor, arrow});
            segment = cursor + 1;
        }
        const bool anyNamed = std::ranges::any_of(
            associations, [](const auto& association) { return association.arrow.has_value(); });
        for (std::size_t ordinal = 0; ordinal < associations.size(); ++ordinal) {
            const auto& association = associations[ordinal];
            std::string formal;
            std::size_t actual = association.begin;
            if (association.arrow) {
                if (*association.arrow != association.begin + 1 ||
                    tokens[association.begin].kind != parser::HdlTokenKind::Identifier)
                    continue;
                formal = lower(tokens[association.begin].text);
                if (std::ranges::find(formalNames, formal) == formalNames.end()) continue;
                actual = *association.arrow + 1;
            } else {
                if (anyNamed || ordinal >= formalNames.size()) continue;
                formal = formalNames[ordinal];
            }
            if (actual + 1 != association.end ||
                tokens[actual].kind != parser::HdlTokenKind::Identifier ||
                lower(tokens[actual].text) == "open") continue;
            reportMismatch(formal, actual);
        }
        label = close;
    }
}

void checkVhdlHierarchyUndrivenReads(
    AnalysisResult& result, const core::SourceFile& source,
    const std::vector<parser::HdlToken>& tokens) {
    // Native R-6-1 FUN_1805fa8a0 calls the same project connection builder
    // FUN_1804d0140 used by A-5-32.  Connection kinds 1/2/7 are inserted into
    // the driver's set.  The builder derives kinds 0/1/2 from the formal-port
    // bytes +0xa0/+0xa1/+0x104, independently identified as IN/OUT/INOUT, and
    // uses kind 3 for an internal actual.  The later `(kind == 3 && flag == 0)`
    // branch reports that internal actual only when it is a candidate, absent
    // from the assignment/connection driver set, and not already reported.
    // Resolve only the source-level subset whose formal direction is proven:
    // a uniquely declared entity or a component declared in this architecture.
    // Ambiguous/implicit bindings, expressions and OPEN remain manual review.
    const auto architectureAt = vhdlArchitectureOrdinals(tokens);
    const auto components = localVhdlComponentPorts(tokens, architectureAt);

    std::vector<std::size_t> activeArchitecture(tokens.size());
    for (std::size_t index = 0; index + 3 < tokens.size(); ++index) {
        if (lower(tokens[index].text) != "architecture" ||
            tokens[index + 1].kind != parser::HdlTokenKind::Identifier ||
            lower(tokens[index + 2].text) != "of") continue;
        const auto architectureName = lower(tokens[index + 1].text);
        auto end = index + 4;
        for (; end + 1 < tokens.size(); ++end) {
            if (lower(tokens[end].text) != "end") continue;
            const auto suffix = lower(tokens[end + 1].text);
            if (suffix == "architecture" || suffix == architectureName) break;
        }
        const auto finish = end + 1 < tokens.size() ? end + 2 : tokens.size();
        for (auto token = index; token < finish; ++token)
            activeArchitecture[token] = architectureAt[index];
        index = finish == 0 ? index : finish - 1;
    }

    using SignalKey = std::pair<std::size_t, std::string>;
    std::map<SignalKey, core::SourceLocation> candidates;
    for (std::size_t index = 0; index + 1 < tokens.size(); ++index) {
        const auto architecture = activeArchitecture[index];
        if (architecture == 0 || lower(tokens[index].text) != "signal") continue;
        auto colon = index + 1;
        while (colon < tokens.size() && activeArchitecture[colon] == architecture &&
               tokens[colon].text != ":" && tokens[colon].text != ";") ++colon;
        if (colon >= tokens.size() || tokens[colon].text != ":") continue;
        for (auto name = index + 1; name < colon; ++name) {
            if (tokens[name].kind != parser::HdlTokenKind::Identifier) continue;
            candidates.try_emplace(
                SignalKey{architecture, lower(tokens[name].text)},
                core::SourceLocation{source.path(), tokens[name].line,
                                     tokens[name].column, tokens[name].offset});
        }
        index = colon;
    }

    std::set<SignalKey> drivers;
    for (std::size_t index = 0; index + 1 < tokens.size(); ++index) {
        const auto architecture = activeArchitecture[index];
        if (architecture == 0 ||
            tokens[index].kind != parser::HdlTokenKind::Identifier ||
            tokens[index + 1].text != "<=") continue;
        const SignalKey key{architecture, lower(tokens[index].text)};
        if (candidates.contains(key)) drivers.insert(key);
    }

    const auto matchingEntityPorts = [&](std::string_view target)
        -> std::optional<std::vector<VhdlComponentPort>> {
        const auto normalized = lower(std::string{target});
        std::vector<const parser::DesignUnitInfo*> matches;
        for (const auto& unit : result.structure) {
            if (unit.kind == parser::DesignUnitKind::VhdlEntity &&
                lower(unit.name) == normalized) matches.push_back(&unit);
        }
        if (matches.size() != 1) return std::nullopt;
        std::vector<VhdlComponentPort> ports;
        for (const auto& port : matches.front()->ports)
            ports.push_back({port.name, port.direction, port.hasDefault});
        return ports;
    };
    const auto matchingParen = [&](std::size_t open) {
        std::size_t depth = 0;
        for (auto index = open; index < tokens.size(); ++index) {
            if (tokens[index].text == "(") ++depth;
            else if (tokens[index].text == ")" && depth && --depth == 0) return index;
        }
        return tokens.size();
    };

    struct HierarchyRead {
        std::string spelling;
        std::size_t line{};
        std::size_t column{};
    };
    std::map<SignalKey, HierarchyRead> reads;
    for (std::size_t label = 0; label + 4 < tokens.size(); ++label) {
        const auto architecture = activeArchitecture[label];
        if (architecture == 0 ||
            tokens[label].kind != parser::HdlTokenKind::Identifier ||
            tokens[label + 1].text != ":") continue;
        auto target = label + 2;
        const bool directEntity = lower(tokens[target].text) == "entity";
        if (directEntity) ++target;
        if (target >= tokens.size() ||
            tokens[target].kind != parser::HdlTokenKind::Identifier) continue;
        if (target + 2 < tokens.size() && tokens[target + 1].text == "." &&
            tokens[target + 2].kind == parser::HdlTokenKind::Identifier)
            target += 2;
        const auto targetName = lower(tokens[target].text);

        std::size_t portMap = tokens.size();
        for (auto cursor = target + 1; cursor + 2 < tokens.size(); ++cursor) {
            if (tokens[cursor].text == ";") break;
            if (lower(tokens[cursor].text) == "port" &&
                lower(tokens[cursor + 1].text) == "map" &&
                tokens[cursor + 2].text == "(") {
                portMap = cursor + 2;
                break;
            }
        }
        if (portMap == tokens.size()) continue;
        const auto close = matchingParen(portMap);
        if (close == tokens.size()) continue;

        std::vector<VhdlComponentPort> ports;
        if (directEntity) {
            const auto entity = matchingEntityPorts(targetName);
            if (!entity) continue;
            ports = *entity;
        } else if (const auto component = components.find(
                       {architectureAt[label], targetName});
                   component != components.end()) {
            ports = component->second;
        } else {
            continue;
        }
        if (ports.empty()) continue;

        struct Association {
            std::size_t begin{};
            std::size_t end{};
            std::optional<std::size_t> arrow;
        };
        std::vector<Association> associations;
        auto segment = portMap + 1;
        std::size_t depth = 0;
        for (auto cursor = segment; cursor <= close; ++cursor) {
            if (cursor < close && (tokens[cursor].text == "(" ||
                                   tokens[cursor].text == "[")) ++depth;
            else if (cursor < close && (tokens[cursor].text == ")" ||
                                        tokens[cursor].text == "]") && depth) --depth;
            if (cursor != close && !(depth == 0 && tokens[cursor].text == ","))
                continue;
            std::optional<std::size_t> arrow;
            std::size_t nested = 0;
            for (auto item = segment; item < cursor; ++item) {
                if (tokens[item].text == "(" || tokens[item].text == "[") ++nested;
                else if ((tokens[item].text == ")" || tokens[item].text == "]") && nested)
                    --nested;
                else if (nested == 0 && tokens[item].text == "=>") {
                    arrow = item;
                    break;
                }
            }
            if (segment < cursor) associations.push_back({segment, cursor, arrow});
            segment = cursor + 1;
        }

        bool namedSeen = false;
        for (std::size_t ordinal = 0; ordinal < associations.size(); ++ordinal) {
            const auto& association = associations[ordinal];
            std::optional<std::size_t> formalIndex;
            std::size_t actual = association.begin;
            if (association.arrow) {
                namedSeen = true;
                if (*association.arrow != association.begin + 1 ||
                    tokens[association.begin].kind != parser::HdlTokenKind::Identifier)
                    continue;
                const auto formal = lower(tokens[association.begin].text);
                const auto found = std::ranges::find_if(ports, [&](const auto& port) {
                    return lower(port.name) == formal;
                });
                if (found == ports.end()) continue;
                formalIndex = static_cast<std::size_t>(found - ports.begin());
                actual = *association.arrow + 1;
            } else {
                if (namedSeen || ordinal >= ports.size()) continue;
                formalIndex = ordinal;
            }
            if (!formalIndex || actual + 1 != association.end ||
                tokens[actual].kind != parser::HdlTokenKind::Identifier ||
                lower(tokens[actual].text) == "open") continue;
            const SignalKey key{architecture, lower(tokens[actual].text)};
            if (!candidates.contains(key)) continue;
            const auto direction = ports[*formalIndex].direction;
            if (direction == parser::PortDirection::Output ||
                direction == parser::PortDirection::Inout) {
                drivers.insert(key);
            } else if (direction == parser::PortDirection::Input) {
                reads.try_emplace(key, HierarchyRead{tokens[actual].text,
                                                     tokens[actual].line,
                                                     tokens[actual].column});
            }
        }
        label = close;
    }

    for (const auto& [key, read] : reads) {
        if (drivers.contains(key)) continue;
        emit(result, "GJB9765.4.6.a", source, read.line, read.column,
             "内部信号在没有赋值或层次驱动时被实例输入端口读取：" +
                 read.spelling, read.spelling);
    }
}

bool verilogUnknown(const parser::HdlToken& token, bool xOnly = false) {
    if (token.kind != parser::HdlTokenKind::Number) return false;
    const auto text = lower(token.text);
    const auto quote = text.find('\'');
    if (quote == std::string::npos) return false;
    auto digits = quote + 1;
    if (digits < text.size() && text[digits] == 's') ++digits;
    if (digits < text.size() && std::string_view("bodh").find(text[digits]) != std::string_view::npos)
        ++digits;
    return text.find('x', digits) != std::string::npos ||
        (!xOnly && (text.find('z', digits) != std::string::npos ||
                    text.find('?', digits) != std::string::npos));
}

unsigned verilogUnknownMask(const parser::HdlToken& token) {
    if (token.kind != parser::HdlTokenKind::Number) return 0;
    const auto text = lower(token.text);
    const auto quote = text.find('\'');
    if (quote == std::string::npos) return 0;
    auto digits = quote + 1;
    if (digits < text.size() && text[digits] == 's') ++digits;
    if (digits < text.size() && std::string_view("bodh").find(text[digits]) != std::string_view::npos)
        ++digits;
    unsigned mask = 0;
    if (text.find('x', digits) != std::string::npos) mask |= 1U;
    if (text.find('z', digits) != std::string::npos ||
        text.find('?', digits) != std::string::npos) mask |= 2U;
    return mask;
}

bool vhdlMetaValue(const parser::HdlToken& token) {
    if (token.kind != parser::HdlTokenKind::Number && token.kind != parser::HdlTokenKind::String)
        return false;
    const auto text = lower(token.text);
    if (text.size() < 3 || (text.front() != '\'' && text.front() != '"')) return false;
    bool meta = false;
    for (std::size_t i = 1; i + 1 < text.size(); ++i) {
        if (std::string_view("01xzuhwl-_").find(text[i]) == std::string_view::npos) return false;
        meta |= std::string_view("xzuhwl-").find(text[i]) != std::string_view::npos;
    }
    return meta;
}

bool vhdlForbiddenComparisonValue(const parser::HdlToken& token) {
    if (token.kind != parser::HdlTokenKind::Number &&
        token.kind != parser::HdlTokenKind::String) return false;
    const auto& text = token.text;
    if (text.size() < 3 || (text.front() != '\'' && text.front() != '"')) return false;
    return text.find_first_of("XZUHL-W") != std::string::npos;
}

std::size_t closeParen(const std::vector<parser::HdlToken>& tokens, std::size_t open) {
    std::size_t depth = 0;
    for (auto i = open; i < tokens.size(); ++i) {
        if (tokens[i].text == "(") ++depth;
        else if (tokens[i].text == ")" && depth > 0 && --depth == 0) return i;
    }
    return tokens.size();
}

// Bound one default branch by its HDL statement, not by physical lines. This
// handles begin/end, nested case and if/else without borrowing the next item.
std::size_t statementEnd(const std::vector<parser::HdlToken>& tokens, std::size_t start) {
    if (start >= tokens.size()) return start;
    const auto& word = tokens[start].text;
    if (word == "begin") {
        int depth = 1;
        for (auto i = start + 1; i < tokens.size(); ++i) {
            if (tokens[i].text == "begin") ++depth;
            else if (tokens[i].text == "end" && --depth == 0) return i + 1;
        }
    } else if (word == "case" || word == "casex" || word == "casez") {
        int depth = 1;
        for (auto i = start + 1; i < tokens.size(); ++i) {
            const auto& text = tokens[i].text;
            if (text == "case" || text == "casex" || text == "casez") ++depth;
            else if (text == "endcase" && --depth == 0) return i + 1;
        }
    } else if (word == "if" && start + 1 < tokens.size() && tokens[start + 1].text == "(") {
        const auto close = closeParen(tokens, start + 1);
        if (close == tokens.size()) return tokens.size();
        auto end = statementEnd(tokens, close + 1);
        if (end < tokens.size() && tokens[end].text == "else") end = statementEnd(tokens, end + 1);
        return end;
    } else if ((word == "for" || word == "while" || word == "repeat") &&
               start + 1 < tokens.size() && tokens[start + 1].text == "(") {
        const auto close = closeParen(tokens, start + 1);
        return close == tokens.size() ? close : statementEnd(tokens, close + 1);
    } else if (word == "forever") {
        return statementEnd(tokens, start + 1);
    } else {
        for (auto i = start; i < tokens.size(); ++i) {
            if (tokens[i].text == ";") return i + 1;
            if (tokens[i].text == "endcase") return i;
        }
    }
    return tokens.size();
}

std::size_t matchingNativeCaseEnd(const std::vector<parser::HdlToken>& tokens,
                                  std::size_t caseToken) {
    if (caseToken + 1 >= tokens.size() || tokens[caseToken + 1].text != "(")
        return tokens.size();
    const auto selectorClose = closeParen(tokens, caseToken + 1);
    if (selectorClose == tokens.size()) return tokens.size();
    int caseDepth = 1;
    for (auto at = selectorClose + 1; at < tokens.size(); ++at) {
        const auto& text = tokens[at].text;
        if (text == "case" || text == "casez" || text == "casex") ++caseDepth;
        else if (text == "endcase" && --caseDepth == 0) return at;
    }
    return tokens.size();
}

bool nativeCaseSelectorIsIdentifier(const std::vector<parser::HdlToken>& tokens,
                                    std::size_t caseToken) {
    if (caseToken + 1 >= tokens.size() || tokens[caseToken + 1].text != "(")
        return false;
    const auto close = closeParen(tokens, caseToken + 1);
    if (close == tokens.size() || caseToken + 2 >= close ||
        tokens[caseToken + 2].kind != parser::HdlTokenKind::Identifier)
        return false;
    auto cursor = caseToken + 3;
    while (cursor < close) {
        if (tokens[cursor].text == "[") {
            unsigned depth = 1;
            ++cursor;
            while (cursor < close && depth != 0) {
                if (tokens[cursor].text == "[") ++depth;
                else if (tokens[cursor].text == "]") --depth;
                ++cursor;
            }
            if (depth != 0) return false;
        } else if ((tokens[cursor].text == "." || tokens[cursor].text == "::") &&
                   cursor + 1 < close &&
                   tokens[cursor + 1].kind == parser::HdlTokenKind::Identifier) {
            cursor += 2;
        } else {
            return false;
        }
    }
    return true;
}

bool nativeDirectCaseDefault(const std::vector<parser::HdlToken>& tokens,
                             std::size_t caseToken, std::size_t candidate) {
    if (candidate >= tokens.size() || tokens[candidate].text != "default" ||
        caseToken + 1 >= tokens.size() || tokens[caseToken + 1].text != "(")
        return false;
    const auto close = closeParen(tokens, caseToken + 1);
    if (close == tokens.size() || candidate <= close) return false;
    int nestedCases = 0;
    int blocks = 0;
    int round = 0;
    int square = 0;
    int brace = 0;
    for (auto at = close + 1; at <= candidate; ++at) {
        if (at == candidate)
            return nestedCases == 0 && blocks == 0 && round == 0 &&
                   square == 0 && brace == 0;
        const auto& text = tokens[at].text;
        if (text == "case" || text == "casex" || text == "casez") ++nestedCases;
        else if (text == "endcase" && nestedCases > 0) --nestedCases;
        else if (nestedCases == 0) {
            if (text == "begin") ++blocks;
            else if (text == "end" && blocks > 0) --blocks;
            else if (text == "(") ++round;
            else if (text == ")" && round > 0) --round;
            else if (text == "[") ++square;
            else if (text == "]" && square > 0) --square;
            else if (text == "{") ++brace;
            else if (text == "}" && brace > 0) --brace;
        }
    }
    return false;
}

bool nativeWildcardCaseHasItem(const std::vector<parser::HdlToken>& tokens,
                               std::size_t caseToken) {
    // Native A-11-2 slot 0 scans direct AST_CASE children and reports only on
    // AST_CONDX/AST_CONDZ (0x59/0x5a), once per case.  The Yosys parser creates
    // those nodes per case_item, so an empty or incomplete casex/casez has no
    // reportable child even though its keyword is present in the source.
    if (caseToken + 1 >= tokens.size() || tokens[caseToken + 1].text != "(")
        return false;
    const auto selectorClose = closeParen(tokens, caseToken + 1);
    if (selectorClose == tokens.size()) return false;
    const auto caseEnd = matchingNativeCaseEnd(tokens, caseToken);
    if (caseEnd == tokens.size()) return false;

    int round = 0, square = 0, brace = 0, ternary = 0;
    for (auto at = selectorClose + 1; at < caseEnd; ++at) {
        const auto& text = tokens[at].text;
        if (round == 0 && square == 0 && brace == 0 && text == "default")
            return true;
        if (text == "(") ++round;
        else if (text == ")") --round;
        else if (text == "[") ++square;
        else if (text == "]") --square;
        else if (text == "{") ++brace;
        else if (text == "}") --brace;
        else if (round == 0 && square == 0 && brace == 0 && text == "?") ++ternary;
        else if (round == 0 && square == 0 && brace == 0 && text == ":") {
            if (ternary > 0) --ternary;
            else return true;
        }
        if (round < 0 || square < 0 || brace < 0) return false;
    }
    return false;
}

void checkVerilogStatementLeftAlignment(AnalysisResult& result,
                                        const core::SourceFile& source,
                                        const std::vector<parser::HdlToken>& tokens) {
    // Native slot 2 admits AST_CASE(0x57). FUN_180526b30 has two branches:
    // an AST_REDUCE_BOOL(0x2d) first child identifies an if/else chain and
    // FUN_180527490 follows its nested default/block/case shape; otherwise it
    // compares CASE item children from AST_COND/CONDX/CONDZ (0x58..0x5a).
    // Every branch compares source column +0x208 against the first record.
    // ev_e4a68e44f81363c13a1facdbbbc50f1ff77d7b2000c704f1f20e67e851c9cf64
    // ev_f20266ff4ce6c3d1c5b4b07d7744332226e766b2aefb2c54b8e1cf9d7ec8b292
    const auto afterStatement = [&](std::size_t start) {
        auto end = statementEnd(tokens, start);
        if (end + 1 < tokens.size() && tokens[end].text == ":" &&
            tokens[end + 1].kind == parser::HdlTokenKind::Identifier) end += 2;
        return end;
    };

    // Only the leading IF of an else-if chain establishes the native group.
    // Reprocessing the nested `if` would double-report its later ELSE nodes.
    for (std::size_t index = 0; index < tokens.size(); ++index) {
        if (tokens[index].text != "if" ||
            (index > 0 && tokens[index - 1].text == "else") ||
            index + 1 >= tokens.size() || tokens[index + 1].text != "(") continue;
        const auto expected = tokens[index].column;
        auto currentIf = index;
        while (currentIf + 1 < tokens.size() && tokens[currentIf + 1].text == "(") {
            const auto close = closeParen(tokens, currentIf + 1);
            if (close == tokens.size() || close + 1 >= tokens.size()) break;
            const auto branchEnd = afterStatement(close + 1);
            if (branchEnd >= tokens.size() || tokens[branchEnd].text != "else") break;
            if (tokens[branchEnd].column != expected)
                emit(result, "GJB10157.5.1.2", source,
                     tokens[branchEnd].line, 1,
                     "建议同一层次的所有语句左端对齐");
            if (branchEnd + 1 >= tokens.size() || tokens[branchEnd + 1].text != "if") break;
            currentIf = branchEnd + 1;
        }
    }

    for (std::size_t index = 0; index < tokens.size(); ++index) {
        if (tokens[index].text != "case" && tokens[index].text != "casex" &&
            tokens[index].text != "casez") continue;
        auto cursor = index + 1;
        if (cursor >= tokens.size() || tokens[cursor].text != "(") continue;
        const auto controlEnd = closeParen(tokens, cursor);
        if (controlEnd == tokens.size()) continue;
        cursor = controlEnd + 1;
        const auto expected = tokens[index].column;
        while (cursor < tokens.size() && tokens[cursor].text != "endcase") {
            std::vector<std::size_t> labels{cursor};
            std::size_t colon = cursor;
            int parentheses = 0;
            int brackets = 0;
            int braces = 0;
            int ternary = 0;
            for (; colon < tokens.size(); ++colon) {
                const auto& text = tokens[colon].text;
                if (text == "(") ++parentheses;
                else if (text == ")") --parentheses;
                else if (text == "[") ++brackets;
                else if (text == "]") --brackets;
                else if (text == "{") ++braces;
                else if (text == "}") --braces;
                else if (parentheses == 0 && brackets == 0 && braces == 0 && text == "?")
                    ++ternary;
                else if (parentheses == 0 && brackets == 0 && braces == 0 && text == ":") {
                    if (ternary > 0) --ternary;
                    else break;
                } else if (parentheses == 0 && brackets == 0 && braces == 0 &&
                           ternary == 0 && text == "," && colon + 1 < tokens.size()) {
                    labels.push_back(colon + 1);
                } else if (text == "endcase") break;
            }
            if (colon >= tokens.size() || tokens[colon].text != ":") break;
            for (const auto label : labels) {
                if (tokens[label].column == expected) continue;
                emit(result, "GJB10157.5.1.2", source,
                     tokens[label].line, 1,
                     "建议同一层次的所有语句左端对齐");
            }
            cursor = afterStatement(colon + 1);
            if (cursor <= colon) ++cursor;
        }
    }
}

void checkVerilogPortOrder(AnalysisResult& result, const core::SourceFile& source) {
    // Native FUN_180529870 gathers AST_WIRE(6) declarations and partitions
    // them with +0xa0/+0xa1: 1/0 input, 0/1 output, 1/1 inout. It then places
    // relation categories 2/3 selected by FUN_1804dedf0 are clocks; categories
    // 0/1 selected by FUN_1804dee00 are resets. Names that are also inputs are
    // placed first, followed by remaining inputs, outputs and inouts.
    // FUN_180529750 is an
    // exact case-sensitive name membership count; FUN_1800dc750 compares the
    // actual and expected records positionally. Expected names are unique, so
    // duplicate actual port names make the vector sizes differ and suppress
    // comparison. The first mismatch reports Error at the actual port's line
    // without a native column, then breaks.
    for (const auto& unit : result.structure) {
        if (unit.kind != parser::DesignUnitKind::VerilogModule ||
            unit.location.file.lexically_normal() != source.path().lexically_normal()) continue;

        std::set<std::string> clocks;
        std::set<std::string> resets;
        for (const auto& clock : unit.clocks) clocks.insert(clock.signal);
        for (const auto& reset : unit.resets) resets.insert(reset.signal);

        std::vector<const parser::PortInfo*> expected;
        expected.reserve(unit.ports.size());
        const auto append = [&](const auto& predicate) {
            for (const auto& port : unit.ports) {
                if (predicate(port) && std::ranges::find(expected, &port) == expected.end())
                    expected.push_back(&port);
            }
        };
        append([&](const auto& port) {
            return port.direction == parser::PortDirection::Input && clocks.contains(port.name);
        });
        append([&](const auto& port) {
            return port.direction == parser::PortDirection::Input &&
                   !clocks.contains(port.name) && resets.contains(port.name);
        });
        append([&](const auto& port) {
            return port.direction == parser::PortDirection::Input;
        });
        append([&](const auto& port) {
            return port.direction == parser::PortDirection::Output;
        });
        append([&](const auto& port) {
            return port.direction == parser::PortDirection::Inout;
        });

        // Unknown-direction declarations are not admitted by the native flag
        // triples, so they remain outside the automated comparison.
        std::vector<const parser::PortInfo*> actual;
        for (const auto& port : unit.ports) {
            if (port.direction != parser::PortDirection::Unknown) actual.push_back(&port);
        }
        std::set<std::string> actualNames;
        if (std::ranges::any_of(actual, [&](const auto* port) {
                return !actualNames.insert(port->name).second;
            })) continue;
        if (actual.size() != expected.size()) continue;
        for (std::size_t index = 0; index < actual.size(); ++index) {
            if (actual[index]->name == expected[index]->name) continue;
            emit(result, "GJB10157.5.1.5", source,
                 actual[index]->location.line, 1,
                 "端口列表应按照“时钟、复位、输入、输出、双向端口”的顺序描述");
            break;
        }
    }
}

void checkTopLevelVerilogPortTypes(AnalysisResult& result,
                                   const core::SourceFile& source,
                                   const std::vector<parser::HdlToken>& tokens,
                                   const AnalysisOptions& options) {
    // Native FUN_180543030 resolves the configured top-module entry, walks
    // its AST_MODULE(2) records, and considers only AST_WIRE(6) nodes carrying
    // an input or output direction byte (+0xa0/+0xa1). It reports a port when
    // any of twelve non-wire/non-reg type flags is set. The packaged ANSI and
    // non-ANSI examples establish wand, wor, tri, integer and time; the parser
    // writer chain also sets the adjacent supply/tri-family flags independently.
    // Every admitted port node emits the same 49-byte GB18030 message as a
    // Warning at the node's physical line; the native record has no column.
    // ev_5da57c31449ffea7cd8cad5c3aa007b65d121a2f6873052df4067356883c0a49
    // ev_aaa493aac258e6c8e9f4d04fcd2ebeb575f37e5d55e363a21c32a358a3f3965b
    const std::set<std::string_view> rejectedTypes{
        "wand", "wor", "tri", "triand", "trior", "trireg",
        "tri0", "tri1", "supply0", "supply1", "integer", "time"
    };
    const auto sameFile = [&](const auto& unit) {
        return unit.kind == parser::DesignUnitKind::VerilogModule && selectedTop(unit, options) &&
            unit.location.file.lexically_normal() == source.path().lexically_normal();
    };
    for (const auto& unit : result.structure) {
        if (!sameFile(unit)) continue;
        std::size_t module = tokens.size();
        for (std::size_t i = 0; i + 1 < tokens.size(); ++i) {
            if (tokens[i].text == "module" && tokens[i + 1].text == unit.name &&
                tokens[i + 1].line == unit.location.line) {
                module = i;
                break;
            }
        }
        if (module == tokens.size()) continue;
        auto moduleEnd = module + 2;
        while (moduleEnd < tokens.size() && tokens[moduleEnd].text != "endmodule") ++moduleEnd;

        bool inSubprogram = false;
        for (std::size_t direction = module + 2; direction < moduleEnd; ++direction) {
            const auto& word = tokens[direction].text;
            if (word == "function" || word == "task") { inSubprogram = true; continue; }
            if (word == "endfunction" || word == "endtask") { inSubprogram = false; continue; }
            if (inSubprogram || (word != "input" && word != "output" && word != "inout"))
                continue;

            auto limit = direction + 1;
            int square = 0;
            int round = 0;
            int brace = 0;
            for (; limit < moduleEnd; ++limit) {
                const auto& text = tokens[limit].text;
                if (text == "[") ++square;
                else if (text == "]") --square;
                else if (text == "{") ++brace;
                else if (text == "}") --brace;
                else if (text == "(") ++round;
                else if (text == ")") {
                    if (round == 0 && square == 0 && brace == 0) break;
                    --round;
                }
                if (square == 0 && round == 0 && brace == 0 &&
                    (text == ";" || (limit != direction &&
                     (text == "input" || text == "output" || text == "inout")))) break;
            }

            std::vector<std::pair<const parser::PortInfo*, std::size_t>> declared;
            for (const auto& port : unit.ports) {
                for (auto index = direction + 1; index < limit; ++index) {
                    if (tokens[index].line == port.location.line &&
                        tokens[index].column == port.location.column &&
                        tokens[index].text == port.name) {
                        declared.emplace_back(&port, index);
                        break;
                    }
                }
            }
            if (declared.empty()) continue;
            const auto firstName = std::ranges::min_element(
                declared, {}, [](const auto& item) { return item.second; })->second;
            bool rejected = false;
            for (auto index = direction + 1; index < firstName; ++index) {
                if (rejectedTypes.contains(tokens[index].text)) {
                    rejected = true;
                    break;
                }
            }
            if (!rejected) continue;
            for (const auto& [port, index] : declared) {
                static_cast<void>(index);
                emit(result, "GJB10157.5.3.2", source,
                     port->location.line, 1,
                     "顶层模块的输入输出端口必须只使用reg型和wire型数据");
            }
            direction = limit == 0 ? direction : limit - 1;
        }
    }
}

bool nativeDirectIdentifierRhs(const std::vector<parser::HdlToken>& tokens,
                               std::size_t begin, std::size_t end) {
    while (begin < end && tokens[begin].text == "(" &&
           closeParen(tokens, begin) + 1 == end) {
        ++begin;
        --end;
    }
    if (begin >= end || tokens[begin].kind != parser::HdlTokenKind::Identifier) return false;
    ++begin;
    while (begin < end && tokens[begin].text == "[") {
        int depth = 1;
        ++begin;
        while (begin < end && depth > 0) {
            if (tokens[begin].text == "[") ++depth;
            else if (tokens[begin].text == "]") --depth;
            ++begin;
        }
        if (depth != 0) return false;
    }
    return begin == end;
}

bool nativeVhdlTopLevelGlueOperator(
    const std::vector<parser::HdlToken>& tokens,
    std::size_t begin, std::size_t end) {
    // FUN_180782640 walks the candidate subtree and stops at the first node
    // from the recovered operator set. The mask decodes to AST_BIT_NOT,
    // BIT_AND/OR/XOR/XNOR, REDUCE_AND/OR/XOR/XNOR, SHIFT_LEFT/RIGHT/
    // SLEFT/SRIGHT, ROL, ROR, LOGIC_AND/OR/NOT and WHEN. Arithmetic,
    // comparison, concatenation, identifier and constant nodes are excluded.
    // ev_e827820d401af751c4d921e0e8b0a69f8e02df96562782c1ccc5cec52f55aee0
    static const std::set<std::string_view> operators{
        "not", "and", "or", "xor", "xnor",
        "sll", "srl", "sla", "sra", "rol", "ror", "when"
    };
    for (std::size_t index = begin; index < end; ++index) {
        if (operators.contains(lower(tokens[index].text))) return true;
    }
    return false;
}

std::size_t vhdlEndConstruct(const std::vector<parser::HdlToken>& tokens,
                             std::size_t begin, std::size_t limit,
                             std::string_view construct) {
    for (std::size_t index = begin + 1; index + 1 < limit; ++index) {
        if (lower(tokens[index].text) != "end" ||
            lower(tokens[index + 1].text) != construct) continue;
        auto end = index + 2;
        while (end < limit && tokens[end].text != ";") ++end;
        return end < limit ? end : limit - 1;
    }
    return limit - 1;
}

void checkVhdlTopLevelInstantiationOnly(
    AnalysisResult& result, const core::SourceFile& source,
    const std::vector<parser::HdlToken>& tokens,
    const AnalysisOptions& options) {
    // Native slot 0 finds the configured top design record, collects direct
    // AST_ARCHITECTURE children, then reports AST_ASSIGN_CASE(0x8c),
    // AST_VARASGN(0xa2) and AST_ALWAYS(0x52) directly. AST_ASSIGN/EQ/LE
    // (0x4d/0x55/0x56) report only when FUN_180782640 finds one of the
    // recovered operators. AST_INSTANCE_ENTITY/COMPONENT (0x93/0x94) use the
    // same operator filter, which admits direct port wiring and reports a
    // compound actual in a port map. A direct identifier buffer assignment
    // remains compliant, as confirmed by the packaged example pair.
    for (std::size_t architecture = 0; architecture + 3 < tokens.size();
         ++architecture) {
        if (lower(tokens[architecture].text) != "architecture" ||
            (architecture > 0 && lower(tokens[architecture - 1].text) == "end") ||
            lower(tokens[architecture + 2].text) != "of") continue;
        const auto architectureName = lower(tokens[architecture + 1].text);
        const auto entityName = lower(tokens[architecture + 3].text);
        const auto top = std::ranges::find_if(result.structure, [&](const auto& unit) {
            return unit.kind == parser::DesignUnitKind::VhdlEntity && selectedTop(unit, options) &&
                lower(unit.name) == entityName;
        });
        if (top == result.structure.end()) continue;

        std::size_t architectureEnd = tokens.size();
        for (std::size_t index = architecture + 4; index + 1 < tokens.size(); ++index) {
            if (lower(tokens[index].text) != "end") continue;
            const auto following = lower(tokens[index + 1].text);
            if (following == "architecture" || following == architectureName) {
                architectureEnd = index;
                break;
            }
        }
        if (architectureEnd == tokens.size()) continue;

        // Locate the architecture statement part while excluding a subprogram
        // declaration body in the declarative region.
        int subprogramDepth = 0;
        std::size_t bodyBegin = architectureEnd;
        for (std::size_t index = architecture + 4; index + 1 < architectureEnd; ++index) {
            const auto word = lower(tokens[index].text);
            if (word == "end" &&
                (lower(tokens[index + 1].text) == "function" ||
                 lower(tokens[index + 1].text) == "procedure")) {
                if (subprogramDepth != 0) --subprogramDepth;
                ++index;
                continue;
            }
            if (word == "function" || word == "procedure") {
                ++subprogramDepth;
                continue;
            }
            if (word == "begin" && subprogramDepth == 0) {
                bodyBegin = index;
                break;
            }
        }
        if (bodyBegin == architectureEnd) continue;

        std::vector<bool> ignored(tokens.size(), false);
        for (std::size_t index = bodyBegin + 1; index < architectureEnd; ++index) {
            const auto word = lower(tokens[index].text);
            if (word == "generate" || word == "block") {
                const auto end = vhdlEndConstruct(tokens, index, architectureEnd, word);
                std::fill(ignored.begin() + static_cast<std::ptrdiff_t>(index),
                          ignored.begin() + static_cast<std::ptrdiff_t>(end + 1), true);
                index = end;
                continue;
            }
            if (word != "process" ||
                (index > 0 && lower(tokens[index - 1].text) == "end")) continue;
            emit(result, "GJB9765.4.2.d", source, tokens[index].line,
                 tokens[index].column, "顶层架构不能包含进程逻辑：" + entityName);
            const auto end = vhdlEndConstruct(tokens, index, architectureEnd, "process");
            std::fill(ignored.begin() + static_cast<std::ptrdiff_t>(index),
                      ignored.begin() + static_cast<std::ptrdiff_t>(end + 1), true);
            index = end;
        }

        const auto itemLocation = [&](std::size_t begin, std::size_t end) {
            if (begin + 2 < end &&
                tokens[begin + 1].text == ":" &&
                tokens[begin].kind == parser::HdlTokenKind::Identifier) return begin + 2;
            if (lower(tokens[begin].text) == "postponed" && begin + 1 < end) return begin + 1;
            return begin;
        };
        const auto inspectItem = [&](std::size_t begin, std::size_t end) {
            while (begin < end && tokens[begin].text == ";") ++begin;
            if (begin >= end) return;
            const auto semantic = itemLocation(begin, end);
            if (semantic >= end) return;
            // The native collector visits concurrent assignments and instance
            // nodes, not arbitrary concurrent statements (for example ASSERT).
            const auto firstWord = lower(tokens[semantic].text);
            if (firstWord == "assert" || firstWord == "report" ||
                firstWord == "disconnect") return;
            bool assignment = false;
            bool instantiation = false;
            for (auto index = semantic; index + 1 < end; ++index) {
                if (tokens[index].text == "<=" ||
                    (tokens[index].text == "<" && tokens[index + 1].text == "="))
                    assignment = true;
                if ((lower(tokens[index].text) == "port" ||
                     lower(tokens[index].text) == "generic") &&
                    lower(tokens[index + 1].text) == "map")
                    instantiation = true;
            }
            if (!assignment && !instantiation) return;
            std::size_t with = end;
            std::size_t select = end;
            for (auto index = semantic; index < end; ++index) {
                const auto word = lower(tokens[index].text);
                if (word == "with" && with == end) with = index;
                else if (word == "select" && select == end) select = index;
            }
            if (with != end && select != end) {
                emit(result, "GJB9765.4.2.d", source, tokens[with].line,
                     tokens[with].column, "顶层架构不能包含选择信号赋值");
                return;
            }
            if (nativeVhdlTopLevelGlueOperator(tokens, semantic, end)) {
                for (auto index = semantic; index < end; ++index) {
                    const auto word = lower(tokens[index].text);
                    if (word == "not" || word == "and" || word == "or" ||
                        word == "xor" || word == "xnor" || word == "sll" ||
                        word == "srl" || word == "sla" || word == "sra" ||
                        word == "rol" || word == "ror" || word == "when") {
                        emit(result, "GJB9765.4.2.d", source, tokens[semantic].line,
                             tokens[semantic].column,
                             "顶层架构不能包含运算符或条件粘合逻辑");
                        return;
                    }
                }
            }
        };

        std::size_t segment = bodyBegin + 1;
        int roundDepth = 0;
        for (std::size_t index = bodyBegin + 1; index < architectureEnd; ++index) {
            if (ignored[index]) {
                if (roundDepth == 0) segment = index + 1;
                roundDepth = 0;
                continue;
            }
            if (tokens[index].text == "(") ++roundDepth;
            else if (tokens[index].text == ")") roundDepth = std::max(0, roundDepth - 1);
            if (tokens[index].text != ";" || roundDepth != 0) continue;
            inspectItem(segment, index);
            segment = index + 1;
        }
    }
}

void checkVhdlStdLogicVectorRanges(
    AnalysisResult& result, const core::SourceFile& source,
    const std::vector<parser::HdlToken>& tokens) {
    // Native FUN_180630500 recursively collects declaration/subtype nodes
    // 0x78, 0x79, 0x0a and 0x9c. FUN_18062f980 then examines each 0x9c,
    // compares its type name exactly with "std_logic_vector", and reports
    // when the node has fewer than two children. In the packaged syntax the
    // second child is the required range; an unconstrained occurrence has
    // no following parenthesized range.
    for (std::size_t index = 0; index < tokens.size(); ++index) {
        if (lower(tokens[index].text) != "std_logic_vector") continue;
        if (index + 1 < tokens.size() && tokens[index + 1].text == "(") continue;
        emit(result, "GJB9765.4.4.a", source, tokens[index].line,
             tokens[index].column,
             "std_logic_vector 数据类型的对象在定义时应有位宽范围限制");
    }
}

struct VhdlAsyncBranch {
    std::string signal;
    std::size_t signalToken{};
    std::map<std::string, std::set<unsigned>> directBinaryValues;
};

std::size_t vhdlThenToken(const std::vector<parser::HdlToken>& tokens,
                          std::size_t begin, std::size_t limit) {
    int depth = 0;
    for (auto index = begin; index < limit; ++index) {
        if (tokens[index].text == "(") ++depth;
        else if (tokens[index].text == ")") depth = std::max(0, depth - 1);
        if (depth == 0 && lower(tokens[index].text) == "then") return index;
    }
    return limit;
}

std::size_t vhdlNextBranchBoundary(const std::vector<parser::HdlToken>& tokens,
                                   std::size_t begin, std::size_t limit) {
    int nestedIf = 0;
    for (auto index = begin; index < limit; ++index) {
        const auto word = lower(tokens[index].text);
        if (word == "if") {
            ++nestedIf;
            continue;
        }
        if (word == "end" && index + 1 < limit &&
            lower(tokens[index + 1].text) == "if") {
            if (nestedIf == 0) return index;
            --nestedIf;
            ++index;
            continue;
        }
        if (nestedIf == 0 && (word == "elsif" || word == "else" ||
                              word == "end")) return index;
    }
    return limit;
}

std::optional<std::pair<std::string, std::size_t>> vhdlAsyncControl(
    const std::vector<parser::HdlToken>& tokens,
    std::size_t begin, std::size_t end) {
    while (begin < end && tokens[begin].text == "(" &&
           closeParen(tokens, begin) + 1 == end) {
        ++begin;
        --end;
    }
    std::size_t relation = end;
    std::size_t relationWidth = 0;
    for (auto index = begin; index < end; ++index) {
        if (tokens[index].text == "=" || tokens[index].text == "/=") {
            if (relation != end) return std::nullopt;
            relation = index;
            relationWidth = 1;
        } else if (tokens[index].text == "/" && index + 1 < end &&
                   tokens[index + 1].text == "=") {
            if (relation != end) return std::nullopt;
            relation = index;
            relationWidth = 2;
            ++index;
        }
    }
    if (relation == begin || relation + relationWidth >= end) return std::nullopt;
    const auto binaryLiteral = [&](std::size_t first, std::size_t last) {
        while (first < last && tokens[first].text == "(") ++first;
        while (first < last && tokens[last - 1].text == ")") --last;
        if (first + 1 == last) {
            const auto text = lower(tokens[first].text);
            return text == "0" || text == "1" || text == "0'" || text == "1'" ||
                   text == "'0'" || text == "'1'" || text == "\"0\"" || text == "\"1\"";
        }
        return first + 2 == last && tokens[first].text == "'" &&
            (tokens[first + 1].text == "0" || tokens[first + 1].text == "1" ||
             tokens[first + 1].text == "0'" || tokens[first + 1].text == "1'");
    };
    const auto directIdentifier = [&](std::size_t first, std::size_t last)
        -> std::optional<std::size_t> {
        while (first < last && tokens[first].text == "(") ++first;
        while (first < last && tokens[last - 1].text == ")") --last;
        if (first + 1 == last && tokens[first].kind == parser::HdlTokenKind::Identifier)
            return first;
        return std::nullopt;
    };
    const auto leftIdentifier = directIdentifier(begin, relation);
    const auto rightIdentifier = directIdentifier(relation + relationWidth, end);
    if (leftIdentifier && binaryLiteral(relation + relationWidth, end))
        return std::pair{lower(tokens[*leftIdentifier].text), *leftIdentifier};
    if (rightIdentifier && binaryLiteral(begin, relation))
        return std::pair{lower(tokens[*rightIdentifier].text), *rightIdentifier};
    return std::nullopt;
}

std::set<std::string> vhdlBranchAssignmentTargets(
    const std::vector<parser::HdlToken>& tokens,
    std::size_t begin, std::size_t end) {
    std::set<std::string> targets;
    for (auto index = begin + 1; index < end; ++index) {
        const bool signalAssignment = tokens[index].text == "<=" ||
            (tokens[index].text == "<" && index + 1 < end && tokens[index + 1].text == "=");
        const bool variableAssignment = tokens[index].text == ":=" ||
            (tokens[index].text == ":" && index + 1 < end && tokens[index + 1].text == "=");
        if (!signalAssignment && !variableAssignment) continue;
        auto target = index;
        while (target > begin && tokens[target - 1].text == "]") {
            --target;
            int depth = 1;
            while (target > begin && depth > 0) {
                --target;
                if (tokens[target].text == "]") ++depth;
                else if (tokens[target].text == "[") --depth;
            }
        }
        if (target > begin &&
            tokens[target - 1].kind == parser::HdlTokenKind::Identifier) {
            targets.insert(lower(tokens[target - 1].text));
        }
    }
    return targets;
}

std::map<std::string, std::set<unsigned>> vhdlBranchDirectBinaryAssignments(
    const std::vector<parser::HdlToken>& tokens,
    std::size_t begin, std::size_t end) {
    const auto literal = [&](std::size_t first, std::size_t last)
        -> std::optional<unsigned> {
        while (first < last && tokens[first].text == "(") ++first;
        while (first < last && tokens[last - 1].text == ")") --last;
        if (first + 1 == last) {
            const auto value = lower(tokens[first].text);
            if (value == "0" || value == "'0'" || value == "\"0\"") return 0U;
            if (value == "1" || value == "'1'" || value == "\"1\"") return 1U;
        }
        if (first + 2 == last && tokens[first].text == "'") {
            if (tokens[first + 1].text == "0'" || tokens[first + 1].text == "0") return 0U;
            if (tokens[first + 1].text == "1'" || tokens[first + 1].text == "1") return 1U;
        }
        return std::nullopt;
    };
    const auto aggregate = [&](std::size_t first, std::size_t last)
        -> std::optional<unsigned> {
        if (first >= last || tokens[first].text != "(") return std::nullopt;
        const auto close = closeParen(tokens, first);
        if (close + 1 != last || first + 4 >= close ||
            lower(tokens[first + 1].text) != "others" ||
            tokens[first + 2].text != "=" || tokens[first + 3].text != ">")
            return std::nullopt;
        return literal(first + 4, close);
    };

    std::map<std::string, std::set<unsigned>> assignments;
    for (auto op = begin + 1; op < end; ++op) {
        const bool signalAssignment = tokens[op].text == "<=" ||
            (tokens[op].text == "<" && op + 1 < end && tokens[op + 1].text == "=");
        const bool variableAssignment = tokens[op].text == ":=" ||
            (tokens[op].text == ":" && op + 1 < end && tokens[op + 1].text == "=");
        if (!signalAssignment && !variableAssignment) continue;
        const auto operatorWidth = (tokens[op].text == "<" || tokens[op].text == ":") ? 2U : 1U;
        if (op == begin || tokens[op - 1].kind != parser::HdlTokenKind::Identifier)
            continue; // Keep this adapter on direct scalar targets only.
        auto rhsBegin = op + operatorWidth;
        auto rhsEnd = rhsBegin;
        int parenthesisDepth = 0;
        while (rhsEnd < end) {
            if (tokens[rhsEnd].text == "(") ++parenthesisDepth;
            else if (tokens[rhsEnd].text == ")") --parenthesisDepth;
            if (parenthesisDepth == 0 && tokens[rhsEnd].text == ";") break;
            ++rhsEnd;
        }
        if (rhsEnd == end) continue;
        auto value = literal(rhsBegin, rhsEnd);
        if (!value) value = aggregate(rhsBegin, rhsEnd);
        if (value) assignments[lower(tokens[op - 1].text)].insert(*value);
        op = rhsEnd;
    }
    return assignments;
}

void checkVhdlSharedAsyncControl(
    AnalysisResult& result, const core::SourceFile& source,
    const std::vector<parser::HdlToken>& tokens) {
    // GJB9765.4.6.g and GJB9765.4.10.b share native vtable slot 0
    // FUN_180650a90. Its relation graph compares reset/set controls that
    // reach one register. The packaged pair establishes the directly
    // recoverable branch predicate: two asynchronous IF/ELSIF guards for one
    // assignment target resolve to the same signal, before the clock event.
    for (std::size_t process = 0; process < tokens.size(); ++process) {
        if (lower(tokens[process].text) != "process" ||
            (process > 0 && lower(tokens[process - 1].text) == "end")) continue;
        const auto processEnd = vhdlEndConstruct(tokens, process, tokens.size(), "process");
        auto bodyBegin = process + 1;
        while (bodyBegin < processEnd && lower(tokens[bodyBegin].text) != "begin")
            ++bodyBegin;
        if (bodyBegin == processEnd) continue;
        std::size_t clockEvent = processEnd;
        for (auto index = bodyBegin + 1; index < processEnd; ++index) {
            const auto word = lower(tokens[index].text);
            if (word == "rising_edge" || word == "falling_edge") {
                clockEvent = index;
                break;
            }
        }
        if (clockEvent == processEnd) continue;

        auto branchToken = bodyBegin + 1;
        while (branchToken < clockEvent && lower(tokens[branchToken].text) != "if")
            ++branchToken;
        if (branchToken >= clockEvent) continue;

        std::vector<VhdlAsyncBranch> branches;
        while (branchToken < clockEvent) {
            const auto thenToken = vhdlThenToken(tokens, branchToken + 1, processEnd);
            if (thenToken == processEnd) break;
            if (const auto control = vhdlAsyncControl(tokens, branchToken + 1, thenToken)) {
                const auto boundary = vhdlNextBranchBoundary(
                    tokens, thenToken + 1, processEnd);
                auto directBinaryValues = vhdlBranchDirectBinaryAssignments(
                    tokens, thenToken + 1, boundary);
                if (!directBinaryValues.empty())
                    branches.push_back({control->first, control->second,
                        std::move(directBinaryValues)});
            }
            const auto boundary = vhdlNextBranchBoundary(tokens, thenToken + 1, processEnd);
            if (boundary < processEnd && lower(tokens[boundary].text) == "elsif") {
                branchToken = boundary;
                continue;
            }
            break;
        }

        struct AsyncSetResetOccurrence {
            std::vector<std::size_t> locations;
            std::set<unsigned> values;
        };
        std::map<std::string, std::map<std::string, AsyncSetResetOccurrence>> occurrences;
        for (const auto& branch : branches) {
            for (const auto& [target, values] : branch.directBinaryValues) {
                auto& occurrence = occurrences[target][branch.signal];
                occurrence.locations.push_back(branch.signalToken);
                occurrence.values.insert(values.begin(), values.end());
            }
        }
        std::set<std::string> reported;
        for (const auto& [target, bySignal] : occurrences) {
            static_cast<void>(target);
            for (const auto& [signal, occurrence] : bySignal) {
                if (occurrence.locations.size() < 2 ||
                    !occurrence.values.contains(0U) || !occurrence.values.contains(1U) ||
                    !reported.insert(signal).second) continue;
                const auto token = tokens[occurrence.locations[1]];
                emit(result, "GJB9765.4.6.g", source, token.line, token.column,
                     "同一信号源驱动同一寄存器的多个异步控制端：" + signal);
                emit(result, "GJB9765.4.10.b", source, token.line, token.column,
                     "复位与置位信号来自同一个信号：" + signal);
            }
        }
        process = processEnd;
    }
}

void checkVhdlAsyncResetPaths(
    AnalysisResult& result, const core::SourceFile& source,
    const std::vector<parser::HdlToken>& tokens) {
    // Native GJB9765_VHDL_4_10_a slot 0 FUN_180593380 first builds the
    // process/reset relation model (FUN_1804d56f0), gathers assignments and
    // their signal dependencies, then intersects the asynchronous-control
    // path with the register-output records belonging to that process. The
    // packaged violation is `my_temp1 <= A and my_temp2`, where my_temp1 is
    // the leading asynchronous guard of the process writing my_temp2. This
    // adapter implements the provable same-architecture subset: one directly
    // parsed asynchronous guard and reset-branch target, plus a unique-driver
    // concurrent dependency path containing explicit combinational logic.
    // Component/black-box paths, multiple drivers and composite guards remain
    // ManualReviewRequired. Native GJB9765_VHDL_4_10_k slot 0
    // FUN_1805b31e0 uses the same process relation builder, collects assignment
    // target/source records, and reports an intersection between a
    // combinationally driven name and an asynchronous-control relation. Its
    // packaged compliant example drives the reset through two clocked stages;
    // process-local sequential drivers are therefore deliberately excluded.
    struct Driver {
        std::vector<std::string> dependencies;
        std::size_t targetToken{};
        bool combinational{};
    };
    const std::set<std::string_view> operators{
        "not", "and", "or", "xor", "xnor", "nand", "nor", "when",
        "sll", "srl", "sla", "sra", "rol", "ror", "+", "-", "*", "/",
        "mod", "rem", "&"
    };
    const std::set<std::string_view> ignored{
        "not", "and", "or", "xor", "xnor", "nand", "nor", "when", "else",
        "sll", "srl", "sla", "sra", "rol", "ror", "mod", "rem", "true",
        "false", "others", "after", "transport", "inertial"
    };

    for (std::size_t architecture = 0; architecture + 3 < tokens.size(); ++architecture) {
        if (lower(tokens[architecture].text) != "architecture" ||
            lower(tokens[architecture + 2].text) != "of") continue;
        auto architectureEnd = architecture + 4;
        while (architectureEnd < tokens.size() &&
               !(lower(tokens[architectureEnd].text) == "architecture" &&
                 (architectureEnd == 0 ||
                  lower(tokens[architectureEnd - 1].text) != "end")))
            ++architectureEnd;

        std::vector<bool> insideProcess(architectureEnd - architecture, false);
        int processDepth = 0;
        for (auto cursor = architecture; cursor < architectureEnd; ++cursor) {
            const auto word = lower(tokens[cursor].text);
            if (word == "process" &&
                !(cursor > architecture && lower(tokens[cursor - 1].text) == "end"))
                ++processDepth;
            insideProcess[cursor - architecture] = processDepth > 0;
            if (word == "process" && cursor > architecture &&
                lower(tokens[cursor - 1].text) == "end" && processDepth > 0)
                --processDepth;
        }

        std::map<std::string, std::vector<Driver>> drivers;
        for (auto cursor = architecture; cursor + 2 < architectureEnd; ++cursor) {
            if (insideProcess[cursor - architecture] ||
                tokens[cursor].kind != parser::HdlTokenKind::Identifier ||
                tokens[cursor + 1].text != "<=") continue;
            if (cursor > architecture) {
                const auto previous = lower(tokens[cursor - 1].text);
                if (tokens[cursor - 1].text != ";" && tokens[cursor - 1].text != ":" &&
                    previous != "begin" && previous != "select") continue;
            }
            auto end = cursor + 2;
            int parentheses = 0;
            for (; end < architectureEnd; ++end) {
                if (tokens[end].text == "(") ++parentheses;
                else if (tokens[end].text == ")" && parentheses > 0) --parentheses;
                else if (parentheses == 0 && tokens[end].text == ";") break;
            }
            if (end == architectureEnd) continue;
            Driver driver;
            driver.targetToken = cursor;
            const auto target = lower(tokens[cursor].text);
            for (auto rhs = cursor + 2; rhs < end; ++rhs) {
                const auto word = lower(tokens[rhs].text);
                driver.combinational |= operators.contains(word);
                if (tokens[rhs].kind == parser::HdlTokenKind::Identifier &&
                    word != target && !ignored.contains(word))
                    driver.dependencies.push_back(word);
            }
            drivers[target].push_back(std::move(driver));
            cursor = end;
        }

        const auto reachesRegister = [&](auto&& self, const std::string& signal,
                                         const std::string& registerName,
                                         bool sawLogic, std::set<std::string>& active)
            -> bool {
            if (signal == registerName) return sawLogic;
            if (!active.insert(signal).second) return false;
            const auto found = drivers.find(signal);
            if (found == drivers.end() || found->second.size() != 1) {
                active.erase(signal);
                return false;
            }
            const auto& driver = found->second.front();
            for (const auto& dependency : driver.dependencies) {
                if (self(self, dependency, registerName,
                         sawLogic || driver.combinational, active)) {
                    active.erase(signal);
                    return true;
                }
            }
            active.erase(signal);
            return false;
        };
        const auto combinationalOrigin = [&](auto&& self, const std::string& signal,
                                             std::set<std::string>& active)
            -> std::optional<std::size_t> {
            if (!active.insert(signal).second) return std::nullopt;
            const auto found = drivers.find(signal);
            if (found == drivers.end() || found->second.size() != 1) {
                active.erase(signal);
                return std::nullopt;
            }
            const auto& driver = found->second.front();
            if (driver.combinational) {
                active.erase(signal);
                return driver.targetToken;
            }
            if (driver.dependencies.size() == 1) {
                auto origin = self(self, driver.dependencies.front(), active);
                active.erase(signal);
                return origin;
            }
            active.erase(signal);
            return std::nullopt;
        };

        std::set<std::pair<std::size_t, std::string>> reported;
        std::set<std::size_t> reportedCombinationalResets;
        for (auto process = architecture; process < architectureEnd; ++process) {
            if (lower(tokens[process].text) != "process" ||
                (process > architecture && lower(tokens[process - 1].text) == "end"))
                continue;
            const auto processEnd = vhdlEndConstruct(
                tokens, process, architectureEnd, "process");
            if (processEnd == architectureEnd) continue;
            auto bodyBegin = process + 1;
            while (bodyBegin < processEnd && lower(tokens[bodyBegin].text) != "begin")
                ++bodyBegin;
            if (bodyBegin == processEnd) continue;
            auto branch = bodyBegin + 1;
            while (branch < processEnd && lower(tokens[branch].text) != "if") ++branch;
            if (branch == processEnd) continue;
            const auto thenToken = vhdlThenToken(tokens, branch + 1, processEnd);
            if (thenToken == processEnd) continue;
            const auto control = vhdlAsyncControl(tokens, branch + 1, thenToken);
            if (!control) continue;
            const auto boundary = vhdlNextBranchBoundary(
                tokens, thenToken + 1, processEnd);
            bool clockAfterReset = false;
            for (auto cursor = boundary; cursor < processEnd; ++cursor) {
                const auto word = lower(tokens[cursor].text);
                if (word == "rising_edge" || word == "falling_edge" ||
                    (cursor + 2 < processEnd && tokens[cursor].kind ==
                         parser::HdlTokenKind::Identifier &&
                     tokens[cursor + 1].text == "'" &&
                     lower(tokens[cursor + 2].text) == "event")) {
                    clockAfterReset = true;
                    break;
                }
            }
            if (!clockAfterReset) continue;
            {
                std::set<std::string> active;
                if (const auto origin = combinationalOrigin(
                        combinationalOrigin, control->first, active);
                    origin && reportedCombinationalResets.insert(
                        control->second).second) {
                    const auto& token = tokens[control->second];
                    emit(result, "GJB9765.4.10.k", source,
                         token.line, token.column,
                         "组合逻辑输出 " + tokens[*origin].text +
                             " 未经时序同步即作为异步复位或置位信号 " +
                             token.text);
                }
            }
            const auto targets = vhdlBranchAssignmentTargets(
                tokens, thenToken + 1, boundary);
            for (const auto& target : targets) {
                std::set<std::string> active;
                if (!reachesRegister(reachesRegister, control->first, target,
                                     false, active) ||
                    !reported.emplace(control->second, target).second) continue;
                const auto& token = tokens[control->second];
                emit(result, "GJB9765.4.10.a", source, token.line, token.column,
                     "寄存器输出 " + target +
                         " 经组合逻辑反馈到异步复位或置位端 " + token.text);
            }
            process = processEnd;
        }
        architecture = architectureEnd == tokens.size()
            ? architecture : architectureEnd - 1;
    }
}

void checkVhdlAsyncResetSynchronousRelease(
    AnalysisResult& result, const core::SourceFile& source,
    const std::vector<parser::HdlToken>& tokens) {
    // Native GJB9765_VHDL.4.10.c slot 0 FUN_1805a4390 builds reset/process
    // relation maps, extracts direct AST_ASSIGN_LE(0x56) identifier-to-
    // identifier edges below process-like node 0x75, and reports a reset use
    // when the candidate path is absent from both its reachable-relation and
    // synchronized-property maps. The packaged compliant example establishes
    // the source shape: raw asynchronous reset asserts two stages together,
    // while the clock branch deasserts stage 0 and shifts it into stage 1;
    // business registers use stage 1 rather than the raw reset.
    // Noncanonical expressions, aliases, hierarchy and primitive chains stay
    // ManualReviewRequired rather than being guessed safe.
    // ev_4_10_c_native_entry_1805a4390
    struct AssignmentValue {
        enum class Kind { Identifier, Literal, Other } kind{Kind::Other};
        std::string text;
    };
    struct AsyncProcess {
        std::size_t process{};
        std::size_t resetToken{};
        std::string reset;
        char asserted{};
        std::map<std::string, AssignmentValue> resetAssignments;
        std::map<std::string, AssignmentValue> clockAssignments;
    };

    const auto literal = [&](std::size_t index, std::size_t end)
        -> std::optional<char> {
        if (index >= end) return std::nullopt;
        const auto text = lower(tokens[index].text);
        if (text == "0" || text == "0'" || text == "'0'" || text == "\"0\"")
            return '0';
        if (text == "1" || text == "1'" || text == "'1'" || text == "\"1\"")
            return '1';
        if (text == "'" && index + 1 < end) {
            const auto next = lower(tokens[index + 1].text);
            if (next == "0" || next == "0'") return '0';
            if (next == "1" || next == "1'") return '1';
        }
        return std::nullopt;
    };
    const auto assignments = [&](std::size_t begin, std::size_t end) {
        std::map<std::string, AssignmentValue> found;
        for (auto index = begin; index + 2 < end; ++index) {
            if (tokens[index].kind != parser::HdlTokenKind::Identifier ||
                tokens[index + 1].text != "<=") continue;
            auto statementEnd = index + 2;
            while (statementEnd < end && tokens[statementEnd].text != ";")
                ++statementEnd;
            if (statementEnd >= end) break;
            AssignmentValue value;
            if (const auto bit = literal(index + 2, statementEnd)) {
                value.kind = AssignmentValue::Kind::Literal;
                value.text.assign(1, *bit);
            } else if (index + 3 == statementEnd &&
                       tokens[index + 2].kind == parser::HdlTokenKind::Identifier) {
                value.kind = AssignmentValue::Kind::Identifier;
                value.text = lower(tokens[index + 2].text);
            }
            found[lower(tokens[index].text)] = std::move(value);
            index = statementEnd;
        }
        return found;
    };
    const auto conditionLiteral = [&](std::size_t signal, std::size_t begin,
                                      std::size_t end) -> std::optional<char> {
        if (signal + 2 < end &&
            (tokens[signal + 1].text == "=" || tokens[signal + 1].text == "/="))
            return literal(signal + 2, end);
        for (auto index = begin; index + 1 < signal; ++index)
            if (const auto bit = literal(index, signal);
                bit && (tokens[index + 1].text == "=" ||
                        tokens[index + 1].text == "/=")) return bit;
        return std::nullopt;
    };

    std::vector<AsyncProcess> processes;
    for (std::size_t process = 0; process < tokens.size(); ++process) {
        if (lower(tokens[process].text) != "process" ||
            (process > 0 && lower(tokens[process - 1].text) == "end")) continue;
        const auto processEnd = vhdlEndConstruct(tokens, process, tokens.size(), "process");
        if (processEnd == tokens.size()) continue;
        std::set<std::string> sensitivity;
        if (process + 1 < processEnd && tokens[process + 1].text == "(") {
            const auto close = closeParen(tokens, process + 1);
            if (close >= processEnd) continue;
            for (auto index = process + 2; index < close; ++index)
                if (tokens[index].kind == parser::HdlTokenKind::Identifier)
                    sensitivity.insert(lower(tokens[index].text));
        }
        auto body = process + 1;
        while (body < processEnd && lower(tokens[body].text) != "begin") ++body;
        if (body == processEnd) continue;
        auto branch = body + 1;
        while (branch < processEnd && lower(tokens[branch].text) != "if") ++branch;
        if (branch == processEnd) continue;
        const auto resetThen = vhdlThenToken(tokens, branch + 1, processEnd);
        if (resetThen == processEnd) continue;
        const auto control = vhdlAsyncControl(tokens, branch + 1, resetThen);
        if (!control || !sensitivity.contains(control->first)) continue;
        const auto asserted = conditionLiteral(control->second, branch + 1, resetThen);
        if (!asserted) continue;
        const auto resetBoundary = vhdlNextBranchBoundary(
            tokens, resetThen + 1, processEnd);
        if (resetBoundary >= processEnd ||
            lower(tokens[resetBoundary].text) != "elsif") continue;
        const auto clockThen = vhdlThenToken(tokens, resetBoundary + 1, processEnd);
        if (clockThen == processEnd) continue;
        bool edge = false;
        for (auto index = resetBoundary + 1; index < clockThen; ++index) {
            const auto word = lower(tokens[index].text);
            if (word == "rising_edge" || word == "falling_edge" ||
                (index + 2 < clockThen &&
                 tokens[index].kind == parser::HdlTokenKind::Identifier &&
                 tokens[index + 1].text == "'" &&
                 lower(tokens[index + 2].text) == "event")) {
                edge = true;
                break;
            }
        }
        if (!edge) continue;
        const auto clockBoundary = vhdlNextBranchBoundary(
            tokens, clockThen + 1, processEnd);
        processes.push_back({process, control->second, control->first, *asserted,
            assignments(resetThen + 1, resetBoundary),
            assignments(clockThen + 1, clockBoundary)});
        process = processEnd;
    }

    std::set<std::string> synchronizedResets;
    std::set<std::size_t> synchronizerProcesses;
    for (const auto& process : processes) {
        const char deasserted = process.asserted == '0' ? '1' : '0';
        for (const auto& [first, resetValue] : process.resetAssignments) {
            if (resetValue.kind != AssignmentValue::Kind::Literal ||
                resetValue.text != std::string(1, process.asserted)) continue;
            const auto firstClock = process.clockAssignments.find(first);
            if (firstClock == process.clockAssignments.end() ||
                firstClock->second.kind != AssignmentValue::Kind::Literal ||
                firstClock->second.text != std::string(1, deasserted)) continue;
            for (const auto& [second, secondReset] : process.resetAssignments) {
                if (second == first ||
                    secondReset.kind != AssignmentValue::Kind::Literal ||
                    secondReset.text != std::string(1, process.asserted)) continue;
                const auto secondClock = process.clockAssignments.find(second);
                if (secondClock == process.clockAssignments.end() ||
                    secondClock->second.kind != AssignmentValue::Kind::Identifier ||
                    secondClock->second.text != first) continue;
                synchronizedResets.insert(second);
                synchronizerProcesses.insert(process.process);
            }
        }
    }

    std::set<std::pair<std::string, std::size_t>> reported;
    for (const auto& process : processes) {
        if (synchronizerProcesses.contains(process.process) ||
            synchronizedResets.contains(process.reset) ||
            !reported.emplace(process.reset, process.resetToken).second) continue;
        const auto& token = tokens[process.resetToken];
        emit(result, "GJB9765.4.10.c", source, token.line, token.column,
             "异步复位信号 " + token.text +
                 " 未经两级寄存器同步释放即直接控制业务寄存器");
    }
}

void checkVhdlExternalSynchronousReset(
    AnalysisResult& result, const core::SourceFile& source,
    const std::vector<parser::HdlToken>& tokens) {
    // Native GJB9765_VHDL.4.10.i slot 0 FUN_1805b2230 first gathers design-
    // unit port names from AST type 6 into a de-duplicated name set. It then
    // visits AST_ALWAYS(0x52), builds each process relation tree through
    // FUN_1804d56f0, selects relation category 2 with its validity flag set,
    // and reports when a relation name is present in the external-port set.
    // In the rule context and packaged pair, category 2 is the synchronous-
    // reset relation: raw input RST used inside the clock edge reports, while
    // an internal second-stage RST_N_SYNL does not intersect the port set.
    // The embedded diagnostic is assembled as
    // "同步复位时,外部复位信号" + name + "宜使用两级级联寄存器同步".
    //
    // Use parser-proven sequential processes, but recover the synchronous
    // reset from the source condition immediately nested below its clock-edge
    // branch. The broad timing summary intentionally records reset-like names
    // read anywhere in a process (including a synchronizer assignment RHS),
    // so using it directly would invert the packaged compliant example.
    // This intentionally mirrors the recovered external/direct intersection
    // rather than claiming that every unreported internal signal has a valid
    // two-stage provenance.
    // ev_4_10_i_native_entry_1805b2230
    for (const auto& unit : result.structure) {
        if (unit.kind != parser::DesignUnitKind::VhdlEntity ||
            unit.location.file.lexically_normal() != source.path().lexically_normal())
            continue;
        std::set<std::string> inputs;
        for (const auto& port : unit.ports)
            if (port.direction == parser::PortDirection::Input)
                inputs.insert(lower(port.name));

        const auto resetLike = [](std::string name) {
            name = lower(std::move(name));
            return name.starts_with("arst") || name.starts_with("rst") ||
                name.starts_with("reset") || name.starts_with("srst");
        };
        std::set<std::pair<std::string, std::size_t>> reported;
        std::set<std::size_t> inspectedProcesses;
        for (const auto& timing : unit.sequentialProcesses) {
            std::size_t process = tokens.size();
            std::size_t processEnd = tokens.size();
            for (std::size_t candidate = 0; candidate < tokens.size(); ++candidate) {
                if (lower(tokens[candidate].text) != "process" ||
                    (candidate > 0 && lower(tokens[candidate - 1].text) == "end"))
                    continue;
                const auto end = vhdlEndConstruct(tokens, candidate, tokens.size(), "process");
                if (tokens[candidate].line <= timing.location.line &&
                    (end == tokens.size() || timing.location.line <= tokens[end].line)) {
                    process = candidate;
                    processEnd = end;
                    break;
                }
            }
            if (process == tokens.size() || processEnd == tokens.size() ||
                !inspectedProcesses.insert(process).second) continue;
            auto body = process + 1;
            while (body < processEnd && lower(tokens[body].text) != "begin") ++body;
            if (body == processEnd) continue;
            auto edgeIf = body + 1;
            while (edgeIf < processEnd && lower(tokens[edgeIf].text) != "if") ++edgeIf;
            if (edgeIf == processEnd) continue;
            const auto edgeThen = vhdlThenToken(tokens, edgeIf + 1, processEnd);
            if (edgeThen == processEnd) continue;
            bool clockEdge = false;
            for (auto index = edgeIf + 1; index < edgeThen; ++index) {
                const auto word = lower(tokens[index].text);
                if (word == "rising_edge" || word == "falling_edge" ||
                    (index + 2 < edgeThen &&
                     tokens[index].kind == parser::HdlTokenKind::Identifier &&
                     tokens[index + 1].text == "'" &&
                     lower(tokens[index + 2].text) == "event")) {
                    clockEdge = true;
                    break;
                }
            }
            if (!clockEdge) continue;

            auto resetIf = edgeThen + 1;
            while (resetIf < processEnd && lower(tokens[resetIf].text) != "if") {
                if (tokens[resetIf].text == "<=" || tokens[resetIf].text == ":=") {
                    resetIf = processEnd;
                    break;
                }
                ++resetIf;
            }
            if (resetIf == processEnd) continue;
            const auto resetThen = vhdlThenToken(tokens, resetIf + 1, processEnd);
            if (resetThen == processEnd) continue;
            const auto control = vhdlAsyncControl(tokens, resetIf + 1, resetThen);
            if (!control || !inputs.contains(control->first) ||
                !resetLike(control->first) ||
                !reported.emplace(control->first, tokens[control->second].line).second)
                continue;
            const auto& reset = tokens[control->second];
            emit(result, "GJB9765.4.10.i", source, reset.line, reset.column,
                 "同步复位时，外部复位信号 " + reset.text +
                     " 宜使用两级级联寄存器同步");
        }
    }
}

void checkVhdlRegisterResetInitialization(
    AnalysisResult& result, const core::SourceFile& source,
    const std::vector<parser::HdlToken>& tokens) {
    // Native GJB9765_VHDL_4_10_e slot 0 FUN_1805b0f10 gathers AST_ALWAYS
    // (0x52), builds each process relation model with FUN_1804d56f0, and uses
    // recursive FUN_1805b1e80 to put direct AST_ASSIGN_LE(0x56) identifier
    // targets into two maps: all register targets and targets covered by a
    // recognised reset-condition branch. It reports every all-target name
    // absent from the reset-covered map. This adapter implements that same
    // target-level set difference for parser-confirmed sequential VHDL
    // processes and direct signal targets. Composite targets, unresolved
    // reset relations and noncanonical control trees remain review-required.
    const auto directTargets = [&](std::size_t begin, std::size_t end) {
        std::map<std::string, std::size_t> found;
        for (auto token = begin; token + 1 < end; ++token) {
            if (tokens[token].kind != parser::HdlTokenKind::Identifier ||
                tokens[token + 1].text != "<=") continue;
            if (token > begin) {
                const auto previous = lower(tokens[token - 1].text);
                if (tokens[token - 1].text != ";" &&
                    tokens[token - 1].text != ":" &&
                    previous != "begin" && previous != "then" &&
                    previous != "else" && previous != "loop" &&
                    previous != "is") continue;
            }
            found.try_emplace(lower(tokens[token].text), token);
        }
        return found;
    };

    for (const auto& unit : result.structure) {
        if (unit.kind != parser::DesignUnitKind::VhdlEntity ||
            unit.location.file.lexically_normal() != source.path().lexically_normal())
            continue;
        for (const auto& timing : unit.sequentialProcesses) {
            if (timing.location.file.lexically_normal() !=
                source.path().lexically_normal()) continue;
            std::size_t process = tokens.size();
            std::size_t processEnd = tokens.size();
            for (std::size_t candidate = 0; candidate < tokens.size(); ++candidate) {
                if (lower(tokens[candidate].text) != "process" ||
                    (candidate > 0 && lower(tokens[candidate - 1].text) == "end"))
                    continue;
                const auto end = vhdlEndConstruct(
                    tokens, candidate, tokens.size(), "process");
                const bool containsLocation = tokens[candidate].line <= timing.location.line &&
                    (end == tokens.size() || timing.location.line <= tokens[end].line);
                const bool containsClock = std::ranges::any_of(
                    timing.clockEvents, [&](const auto& clock) {
                        return tokens[candidate].line <= clock.location.line &&
                            (end == tokens.size() || clock.location.line <= tokens[end].line);
                    });
                if (!containsLocation && !containsClock) continue;
                process = candidate;
                processEnd = end;
                break;
            }
            if (process == tokens.size() || processEnd == tokens.size()) continue;

            auto bodyBegin = process + 1;
            while (bodyBegin < processEnd && lower(tokens[bodyBegin].text) != "begin")
                ++bodyBegin;
            if (bodyBegin == processEnd) continue;
            const auto allTargets = directTargets(bodyBegin + 1, processEnd);
            if (allTargets.empty()) continue;

            std::set<std::string> resetNames;
            for (const auto& reset : timing.resets)
                resetNames.insert(lower(reset.signal));
            std::set<std::string> resetTargets;
            if (!resetNames.empty()) {
                for (auto branch = bodyBegin + 1; branch < processEnd; ++branch) {
                    const auto word = lower(tokens[branch].text);
                    if (word != "if" && word != "elsif") continue;
                    if (word == "if" && branch > bodyBegin + 1 &&
                        lower(tokens[branch - 1].text) == "end") continue;
                    const auto thenToken = vhdlThenToken(
                        tokens, branch + 1, processEnd);
                    if (thenToken == processEnd) continue;
                    const auto control = vhdlAsyncControl(
                        tokens, branch + 1, thenToken);
                    if (!control || !resetNames.contains(control->first)) continue;
                    const auto boundary = vhdlNextBranchBoundary(
                        tokens, thenToken + 1, processEnd);
                    const auto covered = directTargets(thenToken + 1, boundary);
                    for (const auto& [name, location] : covered) {
                        static_cast<void>(location);
                        resetTargets.insert(name);
                    }
                }
            }

            for (const auto& [name, location] : allTargets) {
                if (resetTargets.contains(name)) continue;
                const auto& token = tokens[location];
                emit(result, "GJB9765.4.10.e", source,
                     token.line, token.column,
                     "寄存器 " + token.text + " 未通过复位分支初始化");
            }
        }
    }
}

void checkVhdlFsmEnumerationType(
    AnalysisResult& result, const core::SourceFile& source,
    const std::vector<parser::HdlToken>& tokens) {
    // Native GJB9765_VHDL_4_11_e slot 0 FUN_180653b10 resolves case
    // candidates, enumerates their branch names, ignores the case-folded
    // literal "others", and reports a candidate when one or more remaining
    // state names are absent from the resolved enumeration-literal table.
    // This adapter admits only a source-proved one-process FSM, or a
    // two-process FSM with one direct next-state transfer. Ordinary decoder
    // cases and unresolved scopes remain review-required.
    // Native 4.11.c slot 0 FUN_1805b60a0 uses the same model collector. It
    // reports a null initialization field at model+0x28 immediately; otherwise
    // it lowercases *(model+0xc0)+8 and requires an exact match in one of two
    // legal-state tables, DAT_18133c7d0 or DAT_18133c818.
    // Native 4.11.a slot 0 FUN_1805b51d0 special-cases the lowercased branch
    // name "others", compares its assignment-target records with ordinary
    // branches, and reports missing targets. Without others it compares the
    // model state count with 1 << model-width to find uncovered encodings.
    // Native 4.11.d slot 0 FUN_1805b6fb0 accepts a recovered FSM model only
    // when its classifier field at +0x7c equals 2. The packaged pair proves
    // classifier 2 is the three-process form: state register, pure next-state
    // decoder, and separate output decoder.
    struct Signal {
        std::string type;
        std::size_t declaration{0};
        std::optional<std::string> initial;
        bool ambiguous{false};
    };
    struct Process {
        std::size_t begin{0}, end{0};
        bool clocked{false};
    };
    const auto directAssignments = [&](std::size_t begin, std::size_t end) {
        std::set<std::string> names;
        for (auto token = begin; token + 1 < end; ++token)
            if (tokens[token].kind == parser::HdlTokenKind::Identifier &&
                tokens[token + 1].text == "<=")
                names.insert(lower(tokens[token].text));
        return names;
    };

    for (std::size_t architecture = 0; architecture + 3 < tokens.size(); ++architecture) {
        if (lower(tokens[architecture].text) != "architecture" ||
            (architecture > 0 && lower(tokens[architecture - 1].text) == "end")) continue;
        auto architectureEnd = architecture + 4;
        while (architectureEnd < tokens.size() &&
               !(lower(tokens[architectureEnd].text) == "architecture" &&
                 (architectureEnd == 0 ||
                  lower(tokens[architectureEnd - 1].text) != "end")))
            ++architectureEnd;
        auto declarationEnd = architecture + 4;
        while (declarationEnd < architectureEnd &&
               lower(tokens[declarationEnd].text) != "begin") ++declarationEnd;
        if (declarationEnd == architectureEnd) continue;

        std::map<std::string, std::set<std::string>> enumTypes;
        std::map<std::string, Signal> signals;
        for (auto token = architecture + 4; token < declarationEnd; ++token) {
            const auto word = lower(tokens[token].text);
            if (word == "type" && token + 3 < declarationEnd &&
                tokens[token + 1].kind == parser::HdlTokenKind::Identifier &&
                lower(tokens[token + 2].text) == "is" && tokens[token + 3].text == "(") {
                const auto close = closeParen(tokens, token + 3);
                if (close < declarationEnd) {
                    std::set<std::string> literals;
                    bool valid = true, expectLiteral = true;
                    for (auto literal = token + 4; literal < close; ++literal) {
                        if (expectLiteral &&
                            tokens[literal].kind == parser::HdlTokenKind::Identifier) {
                            literals.insert(lower(tokens[literal].text));
                            expectLiteral = false;
                        } else if (!expectLiteral && tokens[literal].text == ",") {
                            expectLiteral = true;
                        } else valid = false;
                    }
                    if (valid && !expectLiteral)
                        enumTypes[lower(tokens[token + 1].text)] = std::move(literals);
                }
            }
            if (word != "signal") continue;
            auto finish = token + 1;
            while (finish < declarationEnd && tokens[finish].text != ";") ++finish;
            auto colon = token + 1;
            while (colon < finish && tokens[colon].text != ":") ++colon;
            if (colon + 1 >= finish ||
                tokens[colon + 1].kind != parser::HdlTokenKind::Identifier) continue;
            const auto type = lower(tokens[colon + 1].text);
            std::optional<std::string> initial;
            for (auto cursor = colon + 2; cursor + 1 < finish; ++cursor) {
                if (tokens[cursor].text != ":=") continue;
                if (cursor + 2 == finish) initial = lower(tokens[cursor + 1].text);
                break;
            }
            for (auto name = token + 1; name < colon; ++name) {
                if (tokens[name].text == ",") continue;
                if (tokens[name].kind != parser::HdlTokenKind::Identifier) continue;
                const auto key = lower(tokens[name].text);
                const auto [found, inserted] = signals.emplace(
                    key, Signal{type, name, initial, false});
                if (!inserted) found->second.ambiguous = true;
            }
            token = finish;
        }

        std::vector<Process> processes;
        for (auto token = declarationEnd + 1; token < architectureEnd; ++token) {
            if (lower(tokens[token].text) != "process" ||
                (token > declarationEnd && lower(tokens[token - 1].text) == "end")) continue;
            const auto finish = vhdlEndConstruct(tokens, token, architectureEnd, "process");
            if (finish == architectureEnd) continue;
            bool clocked = false;
            for (auto cursor = token + 1; cursor < finish; ++cursor) {
                const auto edge = lower(tokens[cursor].text);
                if (edge == "rising_edge" || edge == "falling_edge" ||
                    (cursor + 2 < finish && tokens[cursor + 1].text == "'" &&
                     lower(tokens[cursor + 2].text) == "event")) {
                    clocked = true;
                    break;
                }
            }
            processes.push_back({token, finish, clocked});
            token = finish;
        }

        std::set<std::string> reportedEnumeration, reportedInitialization,
            reportedInvalidStateHandling, reportedThreeStage;
        for (const auto& process : processes) {
            for (auto caseToken = process.begin + 1; caseToken + 2 < process.end; ++caseToken) {
                if (lower(tokens[caseToken].text) != "case" ||
                    (caseToken > process.begin && lower(tokens[caseToken - 1].text) == "end") ||
                    tokens[caseToken + 1].kind != parser::HdlTokenKind::Identifier ||
                    lower(tokens[caseToken + 2].text) != "is") continue;
                const auto control = lower(tokens[caseToken + 1].text);
                const auto declaration = signals.find(control);
                if (declaration == signals.end() || declaration->second.ambiguous) continue;

                auto finish = caseToken + 3;
                int depth = 1;
                bool nestedCase = false;
                for (; finish + 1 < process.end; ++finish) {
                    if (lower(tokens[finish].text) == "case" &&
                        lower(tokens[finish - 1].text) != "end") {
                        ++depth;
                        nestedCase = true;
                    }
                    else if (lower(tokens[finish].text) == "end" &&
                             lower(tokens[finish + 1].text) == "case" && --depth == 0) break;
                }
                if (finish + 1 >= process.end || nestedCase) continue;
                std::set<std::string> legalStates;
                if (const auto enumeration = enumTypes.find(declaration->second.type);
                    enumeration != enumTypes.end())
                    legalStates.insert(enumeration->second.begin(), enumeration->second.end());
                unsigned explicitBranches = 0;
                std::set<std::string> explicitTargets, othersTargets, explicitLabels;
                std::optional<std::size_t> othersToken;
                for (auto cursor = caseToken + 3; cursor + 2 < finish;) {
                    if (lower(tokens[cursor].text) != "when") { ++cursor; continue; }
                    const auto label = lower(tokens[cursor + 1].text);
                    auto arrow = cursor + 2;
                    while (arrow < finish && tokens[arrow].text != "=>") ++arrow;
                    if (arrow == finish) break;
                    auto bodyEnd = arrow + 1;
                    while (bodyEnd < finish && lower(tokens[bodyEnd].text) != "when") ++bodyEnd;
                    const auto targets = directAssignments(arrow + 1, bodyEnd);
                    if (label == "others") {
                        othersToken = cursor;
                        othersTargets.insert(targets.begin(), targets.end());
                    } else {
                        ++explicitBranches;
                        explicitLabels.insert(label);
                        explicitTargets.insert(targets.begin(), targets.end());
                        if (arrow == cursor + 2) legalStates.insert(label);
                    }
                    cursor = bodyEnd;
                }
                if (explicitBranches < 2) continue;

                const auto writes = directAssignments(caseToken + 3, finish);
                bool recognized = process.clocked && writes.contains(control);
                const Process* stateProcess = recognized ? &process : nullptr;
                std::optional<std::string> nextState;
                if (!recognized) {
                    for (const auto& next : writes) {
                        const auto nextDeclaration = signals.find(next);
                        if (nextDeclaration == signals.end() || next == control ||
                            nextDeclaration->second.ambiguous ||
                            nextDeclaration->second.type != declaration->second.type) continue;
                        unsigned transfers = 0;
                        const Process* transferProcess = nullptr;
                        for (const auto& clockProcess : processes) {
                            if (!clockProcess.clocked || clockProcess.begin == process.begin) continue;
                            for (auto cursor = clockProcess.begin + 1;
                                 cursor + 2 < clockProcess.end; ++cursor)
                                if (lower(tokens[cursor].text) == control &&
                                    tokens[cursor + 1].text == "<=" &&
                                    lower(tokens[cursor + 2].text) == next) {
                                    ++transfers;
                                    transferProcess = &clockProcess;
                                }
                        }
                        if (transfers == 1) {
                            recognized = true;
                            stateProcess = transferProcess;
                            nextState = next;
                            break;
                        }
                    }
                }
                if (!recognized || stateProcess == nullptr) continue;
                const auto& location = tokens[declaration->second.declaration];
                if (!enumTypes.contains(declaration->second.type) &&
                    reportedEnumeration.insert(control).second)
                    emit(result, "GJB9765.4.11.e", source,
                         location.line, location.column,
                         "已识别状态机 " + location.text + " 未使用枚举类型声明");

                std::vector<std::string> missingTargets;
                if (othersToken)
                    for (const auto& target : explicitTargets)
                        if (!othersTargets.contains(target)) missingTargets.push_back(target);
                std::optional<std::size_t> stateCapacity;
                if (const auto enumeration = enumTypes.find(declaration->second.type);
                    enumeration != enumTypes.end() && !enumeration->second.empty()) {
                    std::size_t capacity = 1;
                    while (capacity < enumeration->second.size()) capacity <<= 1;
                    stateCapacity = capacity;
                } else if (!explicitLabels.empty()) {
                    std::optional<std::size_t> width;
                    bool binary = true;
                    for (const auto& label : explicitLabels) {
                        if (label.size() < 3 || label.front() != '"' ||
                            label.back() != '"') { binary = false; break; }
                        const auto bits = label.size() - 2;
                        if (!std::ranges::all_of(label.substr(1, bits),
                            [](char ch) { return ch == '0' || ch == '1'; })) {
                            binary = false;
                            break;
                        }
                        if (width && *width != bits) { binary = false; break; }
                        width = bits;
                    }
                    if (binary && width && *width < sizeof(std::size_t) * 8)
                        stateCapacity = std::size_t{1} << *width;
                }
                const bool uncoveredEncoding = !othersToken && stateCapacity &&
                    explicitLabels.size() < *stateCapacity;
                if ((!missingTargets.empty() || uncoveredEncoding) &&
                    reportedInvalidStateHandling.insert(control).second) {
                    const auto& reportToken = tokens[othersToken.value_or(caseToken)];
                    const auto message = uncoveredEncoding
                        ? "已识别状态机 " + location.text + " 存在未处理的无效状态编码"
                        : "已识别状态机 " + location.text + " 的 others 分支遗漏赋值目标：" +
                            missingTargets.front();
                    emit(result, "GJB9765.4.11.a", source,
                         reportToken.line, reportToken.column, message);
                }

                bool threeStage = false;
                if (nextState) {
                    std::set<std::string> transitionTargets = explicitTargets;
                    transitionTargets.insert(othersTargets.begin(), othersTargets.end());
                    const bool pureTransition = !transitionTargets.empty() &&
                        std::ranges::all_of(transitionTargets, [&](const auto& target) {
                            return target == *nextState;
                        });
                    bool separateOutputDecoder = false;
                    if (pureTransition) {
                        for (const auto& outputProcess : processes) {
                            if (outputProcess.clocked || outputProcess.begin == process.begin ||
                                outputProcess.begin == stateProcess->begin) continue;
                            for (auto candidate = outputProcess.begin + 1;
                                 candidate + 2 < outputProcess.end; ++candidate) {
                                if (lower(tokens[candidate].text) != "case" ||
                                    tokens[candidate + 1].kind != parser::HdlTokenKind::Identifier ||
                                    lower(tokens[candidate + 1].text) != control ||
                                    lower(tokens[candidate + 2].text) != "is") continue;
                                auto candidateEnd = candidate + 3;
                                int candidateDepth = 1;
                                bool candidateNested = false;
                                for (; candidateEnd + 1 < outputProcess.end; ++candidateEnd) {
                                    if (lower(tokens[candidateEnd].text) == "case" &&
                                        lower(tokens[candidateEnd - 1].text) != "end") {
                                        ++candidateDepth;
                                        candidateNested = true;
                                    } else if (lower(tokens[candidateEnd].text) == "end" &&
                                               lower(tokens[candidateEnd + 1].text) == "case" &&
                                               --candidateDepth == 0) break;
                                }
                                if (candidateEnd + 1 >= outputProcess.end || candidateNested) continue;
                                const auto outputTargets = directAssignments(candidate + 3, candidateEnd);
                                if (!outputTargets.empty() && !outputTargets.contains(control) &&
                                    !outputTargets.contains(*nextState)) {
                                    separateOutputDecoder = true;
                                    break;
                                }
                            }
                            if (separateOutputDecoder) break;
                        }
                    }
                    threeStage = pureTransition && separateOutputDecoder;
                }
                if (!threeStage && reportedThreeStage.insert(control).second)
                    emit(result, "GJB9765.4.11.d", source,
                         tokens[caseToken].line, tokens[caseToken].column,
                         "已识别状态机 " + location.text +
                             " 未分离状态寄存器、next-state 组合逻辑和输出译码逻辑");

                std::set<std::string> initialStates;
                if (declaration->second.initial)
                    initialStates.insert(*declaration->second.initial);
                for (auto cursor = stateProcess->begin + 1;
                     cursor + 3 < stateProcess->end; ++cursor) {
                    if (stateProcess->begin == process.begin &&
                        cursor >= caseToken && cursor <= finish + 1) continue;
                    if (lower(tokens[cursor].text) != control ||
                        tokens[cursor + 1].text != "<=" ||
                        tokens[cursor + 3].text != ";") continue;
                    const auto value = lower(tokens[cursor + 2].text);
                    if (nextState && value == *nextState) continue;
                    initialStates.insert(value);
                }
                const bool validInitialization = !initialStates.empty() &&
                    std::ranges::all_of(initialStates, [&](const auto& state) {
                        return legalStates.contains(state);
                    });
                if (!validInitialization && reportedInitialization.insert(control).second) {
                    const auto reason = initialStates.empty()
                        ? " 缺少复位或声明初始化"
                        : " 的初始化值不属于已解析合法状态集合";
                    emit(result, "GJB9765.4.11.c", source,
                         location.line, location.column,
                         "已识别状态机 " + location.text + reason);
                }
                caseToken = finish + 1;
            }
        }
        architecture = architectureEnd == tokens.size()
            ? architecture : architectureEnd - 1;
    }
}

struct VhdlDirectTriSource {
    std::string base;
    std::optional<std::string> select;

    [[nodiscard]] std::string display() const {
        return select ? base + "(" + *select + ")" : base;
    }
};

std::optional<VhdlDirectTriSource> vhdlDirectTriSourceParts(
    const std::vector<parser::HdlToken>& tokens,
    std::size_t begin, std::size_t end) {
    // The native 4.6.h path distinguishes vec(0) from vec(1).
    if (begin >= end || tokens[begin].kind != parser::HdlTokenKind::Identifier)
        return std::nullopt;
    auto name = lower(tokens[begin].text);
    if (end == begin + 1) return VhdlDirectTriSource{std::move(name), std::nullopt};
    if (end != begin + 4 || tokens[begin + 1].text != "(" ||
        tokens[begin + 3].text != ")" ||
        (tokens[begin + 2].kind != parser::HdlTokenKind::Number &&
         tokens[begin + 2].kind != parser::HdlTokenKind::Identifier))
        return std::nullopt;
    return VhdlDirectTriSource{std::move(name), lower(tokens[begin + 2].text)};
}

std::optional<std::string> vhdlDirectTriSource(
    const std::vector<parser::HdlToken>& tokens,
    std::size_t begin, std::size_t end) {
    const auto source = vhdlDirectTriSourceParts(tokens, begin, end);
    return source ? std::optional<std::string>{source->display()} : std::nullopt;
}

void checkVhdlTriStateSharedSource(
    AnalysisResult& result, const core::SourceFile& source,
    const std::vector<parser::HdlToken>& tokens) {
    // Native 4.6.h slot 0 FUN_18078cf60 gathers AST_ASSIGN_LE(0x56) via
    // FUN_18078d950 and resolves tri-state data/control candidates through
    // FUN_18078e060. It compares source names and simple selects. This
    // source-level subset admits only data when selected='0/1' else 'Z'.
    for (std::size_t assign = 0; assign + 6 < tokens.size(); ++assign) {
        if (tokens[assign].text != "<=") continue;
        auto end = assign + 1;
        while (end < tokens.size() && tokens[end].text != ";") ++end;
        if (end >= tokens.size() || end < assign + 6 ||
            lower(tokens[end - 2].text) != "else" ||
            lower(tokens[end - 1].text) != "'z'") continue;
        auto when = assign + 1;
        while (when < end && lower(tokens[when].text) != "when") ++when;
        if (when >= end) continue;
        const auto data = vhdlDirectTriSourceParts(tokens, assign + 1, when);
        if (!data) continue;
        auto conditionBegin = when + 1;
        auto conditionEnd = end - 2;
        while (conditionBegin < conditionEnd &&
               tokens[conditionBegin].text == "(" &&
               closeParen(tokens, conditionBegin) + 1 == conditionEnd) {
            ++conditionBegin;
            --conditionEnd;
        }
        auto equal = conditionBegin;
        while (equal < conditionEnd && tokens[equal].text != "=") ++equal;
        if (equal >= conditionEnd) continue;
        const auto left = vhdlDirectTriSourceParts(tokens, conditionBegin, equal);
        const auto right = vhdlDirectTriSourceParts(tokens, equal + 1, conditionEnd);
        const auto binaryLiteral = [&](std::size_t begin, std::size_t finish) {
            if (begin + 1 == finish) {
                const auto value = lower(tokens[begin].text);
                return value == "'0'" || value == "'1'" ||
                       value == "\"0\"" || value == "\"1\"";
            }
            return begin + 2 == finish && tokens[begin].text == "'" &&
                (tokens[begin + 1].text == "0'" || tokens[begin + 1].text == "1'");
        };
        const bool leftLiteral = binaryLiteral(conditionBegin, equal);
        const bool rightLiteral = binaryLiteral(equal + 1, conditionEnd);
        const auto control = rightLiteral ? left : leftLiteral ? right : std::nullopt;
        if (!control || control->base != data->base) continue;
        // FUN_18078cf60 compares the base names first. It compares select
        // fields only when both candidate records begin with AST_SELECT(0x0e)
        // and both selects contain a child. A select on only one side does not
        // suppress the shared-source diagnostic.
        if (control->select && data->select && *control->select != *data->select)
            continue;
        const auto location = tokens[assign + 1];
        emit(result, "GJB9765.4.6.h", source, location.line, location.column,
             "三态数据端与使能端由同一信号源驱动：" + data->display());
    }
}

void checkVhdlCombinationalTriStateEnable(
    AnalysisResult& result, const core::SourceFile& source,
    const std::vector<parser::HdlToken>& tokens) {
    // Native 4.6.i slot 0 FUN_180796d70 recursively invokes FUN_1807981c0.
    // For direct AST_ARCHITECTURE(0x75) children, that helper records
    // conditional tri-state AST_ASSIGN_LE(0x56) controls, and separately
    // records assignment targets whose RHS root is one of
    // 0x24..0x28, 0x2b..0x2c, 0x3c..0x3f, 0x45..0x47 or AST_WHEN(0xb6).
    // The main routine propagates hierarchy/connectivity records and reports
    // intersections of those sets. The packaged pair proves the direct source
    // boundary: a concurrent AND output used as a conditional tri-state
    // enable reports, while an input, sequential-process target or black-box
    // output does not. This adapter preserves that direct concurrent/alias
    // subset and leaves hierarchy/primitive provenance unresolved.
    // ev_4d22f40c42455c53fa232188a5a2c4d569699d4f481a7e79bbacdbe8c2e13f6e
    struct Assignment {
        std::string target;
        std::size_t targetToken{};
        std::size_t rhsBegin{};
        std::size_t rhsEnd{};
    };
    struct Driver {
        std::size_t targetToken{};
        std::set<std::string> sources;
        bool hasCombinationalRoot{};
    };

    const std::set<std::string_view> rootOperators{
        "not", "and", "or", "xor", "xnor", "nand", "nor",
        "+", "-", "*", "/", "when"
    };
    const std::set<std::string_view> ignoredIdentifiers{
        "not", "and", "or", "xor", "xnor", "nand", "nor", "when",
        "else", "true", "false", "others"
    };
    const auto stripOuterParentheses = [&](std::size_t& begin, std::size_t& end) {
        while (begin < end && tokens[begin].text == "(" &&
               closeParen(tokens, begin) + 1 == end) {
            ++begin;
            --end;
        }
    };
    const auto hasCombinationalRoot = [&](std::size_t begin, std::size_t end) {
        stripOuterParentheses(begin, end);
        if (begin >= end) return false;
        if (lower(tokens[begin].text) == "not") return true;
        int parentheses = 0;
        for (auto cursor = begin; cursor < end; ++cursor) {
            if (tokens[cursor].text == "(") ++parentheses;
            else if (tokens[cursor].text == ")") --parentheses;
            else if (parentheses == 0 &&
                     rootOperators.contains(lower(tokens[cursor].text))) {
                const auto op = lower(tokens[cursor].text);
                // Native 4.6.i admits binary AST_ADD/SUB (0x3c/0x3d) but
                // not unary AST_POS/NEG (0x43/0x44). After removing outer
                // parentheses, a leading +/- is therefore not a recovered
                // combinational root for this rule.
                if ((op == "+" || op == "-") && cursor == begin) continue;
                return true;
            }
        }
        return false;
    };
    const auto controlSource = [&](std::size_t begin, std::size_t end)
        -> std::optional<std::pair<std::string, std::size_t>> {
        stripOuterParentheses(begin, end);
        auto equal = begin;
        int parentheses = 0;
        for (; equal < end; ++equal) {
            if (tokens[equal].text == "(") ++parentheses;
            else if (tokens[equal].text == ")") --parentheses;
            else if (parentheses == 0 && tokens[equal].text == "=") break;
        }
        if (equal >= end) return std::nullopt;
        const auto left = vhdlDirectTriSource(tokens, begin, equal);
        const auto right = vhdlDirectTriSource(tokens, equal + 1, end);
        const auto literal = [&](std::size_t first, std::size_t last) {
            if (first + 1 == last) {
                const auto value = lower(tokens[first].text);
                return value == "'0'" || value == "'1'";
            }
            return first + 2 == last && tokens[first].text == "'" &&
                (tokens[first + 1].text == "0'" || tokens[first + 1].text == "1'");
        };
        if (right && literal(begin, equal))
            return std::pair{right->substr(0, right->find('(')), equal + 1};
        if (left && literal(equal + 1, end))
            return std::pair{left->substr(0, left->find('(')), begin};
        return std::nullopt;
    };

    for (std::size_t architecture = 0; architecture + 3 < tokens.size(); ++architecture) {
        if (lower(tokens[architecture].text) != "architecture" ||
            lower(tokens[architecture + 2].text) != "of") continue;
        auto architectureEnd = architecture + 4;
        while (architectureEnd < tokens.size() &&
               lower(tokens[architectureEnd].text) != "architecture") ++architectureEnd;

        std::vector<bool> insideProcess(architectureEnd - architecture, false);
        int processDepth = 0;
        for (auto cursor = architecture; cursor < architectureEnd; ++cursor) {
            const auto word = lower(tokens[cursor].text);
            if (word == "process" &&
                !(cursor > architecture && lower(tokens[cursor - 1].text) == "end"))
                ++processDepth;
            insideProcess[cursor - architecture] = processDepth > 0;
            if (word == "process" && cursor > architecture &&
                lower(tokens[cursor - 1].text) == "end" && processDepth > 0)
                --processDepth;
        }

        std::vector<Assignment> assignments;
        for (auto cursor = architecture; cursor + 2 < architectureEnd; ++cursor) {
            if (insideProcess[cursor - architecture] ||
                tokens[cursor].kind != parser::HdlTokenKind::Identifier ||
                tokens[cursor + 1].text != "<=") continue;
            if (cursor > architecture) {
                const auto previous = lower(tokens[cursor - 1].text);
                if (tokens[cursor - 1].text != ";" && tokens[cursor - 1].text != ":" &&
                    previous != "begin") continue;
            }
            auto end = cursor + 2;
            int parentheses = 0;
            for (; end < architectureEnd; ++end) {
                if (tokens[end].text == "(") ++parentheses;
                else if (tokens[end].text == ")") --parentheses;
                else if (parentheses == 0 && tokens[end].text == ";") break;
            }
            if (end >= architectureEnd) continue;
            assignments.push_back({lower(tokens[cursor].text), cursor, cursor + 2, end});
            cursor = end;
        }

        std::map<std::string, Driver> drivers;
        struct TriStateUse { std::string control; std::size_t token; };
        std::vector<TriStateUse> triStateUses;
        for (const auto& assignment : assignments) {
            auto when = assignment.rhsBegin;
            int parentheses = 0;
            for (; when < assignment.rhsEnd; ++when) {
                if (tokens[when].text == "(") ++parentheses;
                else if (tokens[when].text == ")") --parentheses;
                else if (parentheses == 0 && lower(tokens[when].text) == "when") break;
            }
            const bool triState = when < assignment.rhsEnd &&
                assignment.rhsEnd >= when + 3 &&
                lower(tokens[assignment.rhsEnd - 2].text) == "else" &&
                lower(tokens[assignment.rhsEnd - 1].text) == "'z'";
            if (triState) {
                if (const auto control = controlSource(when + 1, assignment.rhsEnd - 2))
                    triStateUses.push_back({control->first, control->second});
                continue;
            }

            Driver driver{assignment.targetToken, {},
                          hasCombinationalRoot(assignment.rhsBegin, assignment.rhsEnd)};
            for (auto rhs = assignment.rhsBegin; rhs < assignment.rhsEnd; ++rhs) {
                if (tokens[rhs].kind != parser::HdlTokenKind::Identifier) continue;
                const auto name = lower(tokens[rhs].text);
                if (ignoredIdentifiers.contains(name) || name == assignment.target) continue;
                driver.sources.insert(name);
            }
            drivers[assignment.target] = std::move(driver);
        }

        const auto origin = [&](std::string current) -> std::optional<std::size_t> {
            std::set<std::string> visited;
            while (visited.insert(current).second) {
                const auto driver = drivers.find(current);
                if (driver == drivers.end()) return std::nullopt;
                if (driver->second.hasCombinationalRoot) return driver->second.targetToken;
                if (driver->second.sources.size() != 1) return std::nullopt;
                current = *driver->second.sources.begin();
            }
            return std::nullopt;
        };
        std::set<std::size_t> reported;
        for (const auto& use : triStateUses) {
            const auto driver = origin(use.control);
            if (!driver || !reported.insert(use.token).second) continue;
            const auto& token = tokens[use.token];
            emit(result, "GJB9765.4.6.i", source, token.line, token.column,
                 "组合逻辑输出 " + tokens[*driver].text + " 被用作三态门使能");
        }
        architecture = architectureEnd == tokens.size() ? architecture : architectureEnd - 1;
    }
}

std::optional<std::string> nativeResetAliasSource(
    const std::vector<parser::HdlToken>& tokens,
    std::size_t begin, std::size_t end) {
    // FUN_180758410 admits a direct AST_IDENTIFIER(0x13), or exactly one
    // AST_LOGIC_NOT(0x47)/AST_BIT_NOT(0x24) root whose sole direct child is
    // AST_IDENTIFIER.  Identifier selects remain identifier-rooted in the
    // native frontend; calls, reductions and a second unary root do not.
    while (begin < end && tokens[begin].text == "(" &&
           closeParen(tokens, begin) + 1 == end) {
        ++begin;
        --end;
    }
    if (nativeDirectIdentifierRhs(tokens, begin, end))
        return tokens[begin].text;
    if (begin >= end || (tokens[begin].text != "!" && tokens[begin].text != "~"))
        return std::nullopt;
    ++begin;
    while (begin < end && tokens[begin].text == "(" &&
           closeParen(tokens, begin) + 1 == end) {
        ++begin;
        --end;
    }
    if (!nativeDirectIdentifierRhs(tokens, begin, end)) return std::nullopt;
    return tokens[begin].text;
}

void checkTopLevelVerilogGlueLogic(AnalysisResult& result,
                                   const core::SourceFile& source,
                                   const std::vector<parser::HdlToken>& tokens,
                                   const AnalysisOptions& options) {
    // Native FUN_1806fbb10 matches only the configured top-module name, then
    // recursively gathers AST_ASSIGN(0x4d), AST_ASSIGN_EQ(0x55) and
    // AST_ASSIGN_LE(0x56). Every procedural assignment is reported. A
    // continuous AST_ASSIGN is reported only when its second child is not
    // AST_IDENTIFIER(0x13), so a direct wire-through (including an identifier
    // select represented on that node) remains admitted. Both report branches
    // build the exact message 顶层<module>包含胶连逻辑<lhs>, use Warning and the
    // assignment node's physical line, and write no column. Every gathered
    // assignment independently reaches the result append.
    // ev_0a46121daec621b5ae294aa176f16861520f38e9ef553edcf3337f53012651e5
    const auto sameFileTop = [&](const auto& unit) {
        return unit.kind == parser::DesignUnitKind::VerilogModule && selectedTop(unit, options) &&
            unit.location.file.lexically_normal() == source.path().lexically_normal();
    };
    const std::set<std::string_view> declarationWords{
        "parameter", "localparam", "wire", "reg", "logic", "integer", "int",
        "genvar", "input", "output", "inout", "time", "real", "bit", "tri",
        "specparam", "typedef"
    };
    for (const auto& unit : result.structure) {
        if (!sameFileTop(unit)) continue;
        std::size_t module = tokens.size();
        for (std::size_t i = 0; i + 1 < tokens.size(); ++i) {
            if (tokens[i].text == "module" && tokens[i + 1].text == unit.name &&
                tokens[i + 1].line == unit.location.line) {
                module = i;
                break;
            }
        }
        if (module == tokens.size()) continue;
        auto moduleEnd = module + 2;
        while (moduleEnd < tokens.size() && tokens[moduleEnd].text != "endmodule") ++moduleEnd;

        const auto assignmentTarget = [&](std::size_t begin, std::size_t assignment) {
            auto cursor = assignment;
            while (cursor > begin) {
                if (tokens[cursor - 1].text == "]") {
                    int depth = 1;
                    --cursor;
                    while (cursor > begin && depth > 0) {
                        --cursor;
                        if (tokens[cursor].text == "]") ++depth;
                        else if (tokens[cursor].text == "[") --depth;
                    }
                    continue;
                }
                if (tokens[cursor - 1].text == "}") return std::string{};
                if (tokens[cursor - 1].kind == parser::HdlTokenKind::Identifier)
                    return tokens[cursor - 1].text;
                --cursor;
            }
            return std::string{};
        };
        const auto nativeMessage = [&](std::string_view target) {
            return "顶层" + unit.name + "包含胶连逻辑" + std::string(target);
        };

        std::vector<bool> continuous(tokens.size(), false);
        for (auto index = module + 2; index < moduleEnd; ++index) {
            if (tokens[index].text != "assign") continue;
            auto end = index + 1;
            int round = 0, square = 0, brace = 0;
            for (; end < moduleEnd; ++end) {
                const auto& text = tokens[end].text;
                if (text == "(") ++round;
                else if (text == ")") --round;
                else if (text == "[") ++square;
                else if (text == "]") --square;
                else if (text == "{") ++brace;
                else if (text == "}") --brace;
                if (round == 0 && square == 0 && brace == 0 && text == ";") break;
            }
            if (end == moduleEnd) continue;
            for (auto mark = index; mark <= end; ++mark) continuous[mark] = true;
            auto clauseBegin = index + 1;
            round = square = brace = 0;
            for (auto cursor = index + 1; cursor <= end; ++cursor) {
                const bool clauseEnd = cursor == end ||
                    (tokens[cursor].text == "," && round == 0 && square == 0 && brace == 0);
                if (clauseEnd) {
                    std::size_t equal = cursor;
                    int lhsRound = 0, lhsSquare = 0, lhsBrace = 0;
                    for (auto candidate = clauseBegin; candidate < cursor; ++candidate) {
                        const auto& text = tokens[candidate].text;
                        if (text == "(") ++lhsRound;
                        else if (text == ")") --lhsRound;
                        else if (text == "[") ++lhsSquare;
                        else if (text == "]") --lhsSquare;
                        else if (text == "{") ++lhsBrace;
                        else if (text == "}") --lhsBrace;
                        else if (lhsRound == 0 && lhsSquare == 0 && lhsBrace == 0 && text == "=") {
                            equal = candidate;
                            break;
                        }
                    }
                    if (equal != cursor && !nativeDirectIdentifierRhs(tokens, equal + 1, cursor)) {
                        const auto target = assignmentTarget(clauseBegin, equal);
                        emit(result, "GJB10157.5.3.3", source,
                             tokens[clauseBegin].line, 1, nativeMessage(target));
                    }
                    clauseBegin = cursor + 1;
                }
                if (cursor == end) break;
                const auto& text = tokens[cursor].text;
                if (text == "(") ++round;
                else if (text == ")") --round;
                else if (text == "[") ++square;
                else if (text == "]") --square;
                else if (text == "{") ++brace;
                else if (text == "}") --brace;
            }
            index = end;
        }

        auto segment = module + 2;
        int round = 0;
        for (auto index = module + 2; index < moduleEnd; ++index) {
            if (tokens[index].text == "(") ++round;
            else if (tokens[index].text == ")") --round;
            if (tokens[index].text != ";" || round != 0) continue;
            bool declaration = false;
            std::size_t assignment = tokens.size();
            int localRound = 0, square = 0, brace = 0;
            for (auto cursor = segment; cursor < index; ++cursor) {
                const auto& text = tokens[cursor].text;
                if (text == "(") ++localRound;
                else if (text == ")") --localRound;
                else if (text == "[") ++square;
                else if (text == "]") --square;
                else if (text == "{") ++brace;
                else if (text == "}") --brace;
                if (localRound == 0 && square == 0 && brace == 0 &&
                    declarationWords.contains(text)) declaration = true;
                if (!continuous[cursor] && localRound == 0 && square == 0 && brace == 0 &&
                    (text == "=" || text == "<=")) assignment = cursor;
            }
            if (!declaration && assignment != tokens.size()) {
                const auto target = assignmentTarget(segment, assignment);
                emit(result, "GJB10157.5.3.3", source,
                     tokens[assignment].line, 1, nativeMessage(target));
            }
            segment = index + 1;
        }
    }
}

void checkNonTopVerilogHierarchyLogic(AnalysisResult& result,
                                      const core::SourceFile& source,
                                      const std::vector<parser::HdlToken>& tokens,
                                      const AnalysisOptions& options) {
    // Native slot 2 admits AST_MODULE(2), AST_ASSIGN/CELL/INITIAL
    // (0x4d/0x4f/0x53), and AST_ASSIGN_EQ/LE (0x55/0x56). FUN_1805437e0
    // skips the configured top name, then emits once only when a non-top
    // module contains both AST_CELL and assignment/selected-initial logic.
    // A two-child AST_ASSIGN whose identifier LHS shares a source line with
    // an AST_WIRE declaration is excluded as a declaration initializer.
    // ev_8e9b976ed5217b248bc57a745aa824ab32b5fba322e2cde9b3484662ebed14e7
    // ev_78d795ebd176b417805c74b812e23800113bb8b71fba4363401690e27a9424da
    // Exported setTopModule (0x1806298c0) prefixes ordinary names with '\\',
    // validates them against the parsed design and stores the canonical name
    // in DAT_18135b808, which the rule compares exactly. The unique recovered
    // +0x2a9 writer marks an ANTLR initial_construct frontend root.
    // The recovered report path uses the fixed GB18030 text
    // "除顶层模块外，上层模块建议仅包含子模块的例化.", Warning severity,
    // the AST_MODULE physical line and no column.
    // reverse-tools/evidence/t_linter_pass67/a3_2_hierarchy_only.txt
    // docs/evidence/native_verilog_hierarchy_only_5_3_4_findings.md
    const std::set<std::string_view> declarationWords{
        "parameter", "localparam", "wire", "reg", "logic", "integer", "int",
        "genvar", "input", "output", "inout", "time", "real", "bit", "tri",
        "specparam", "typedef"
    };
    for (const auto& unit : result.structure) {
        if (unit.kind != parser::DesignUnitKind::VerilogModule || selectedTop(unit, options) ||
            unit.instances.empty() ||
            unit.location.file.lexically_normal() != source.path().lexically_normal()) continue;
        std::size_t module = tokens.size();
        for (std::size_t i = 0; i + 1 < tokens.size(); ++i) {
            if (tokens[i].text == "module" && tokens[i + 1].text == unit.name &&
                tokens[i + 1].line == unit.location.line) {
                module = i;
                break;
            }
        }
        if (module == tokens.size()) continue;
        auto moduleEnd = module + 2;
        while (moduleEnd < tokens.size() && tokens[moduleEnd].text != "endmodule") ++moduleEnd;

        bool hasLogic = false;
        unsigned subprogramDepth = 0;
        for (auto index = module + 2; index < moduleEnd && !hasLogic; ++index) {
            if (tokens[index].text == "task" || tokens[index].text == "function")
                ++subprogramDepth;
            else if ((tokens[index].text == "endtask" ||
                      tokens[index].text == "endfunction") && subprogramDepth != 0)
                --subprogramDepth;
            if ((tokens[index].text == "assign" || tokens[index].text == "initial") &&
                subprogramDepth == 0)
                hasLogic = true;
        }
        auto segment = module + 2;
        int round = 0;
        unsigned assignmentSubprogramDepth = 0;
        for (auto index = module + 2; index < moduleEnd && !hasLogic; ++index) {
            if (tokens[index].text == "task" || tokens[index].text == "function")
                ++assignmentSubprogramDepth;
            else if ((tokens[index].text == "endtask" ||
                      tokens[index].text == "endfunction") &&
                     assignmentSubprogramDepth != 0)
                --assignmentSubprogramDepth;
            if (tokens[index].text == "(") ++round;
            else if (tokens[index].text == ")") --round;
            if (tokens[index].text != ";" || round != 0) continue;
            if (assignmentSubprogramDepth != 0) {
                segment = index + 1;
                continue;
            }
            bool declaration = false;
            bool assignment = false;
            int localRound = 0, square = 0, brace = 0;
            for (auto cursor = segment; cursor < index; ++cursor) {
                const auto& text = tokens[cursor].text;
                if (text == "(") ++localRound;
                else if (text == ")") --localRound;
                else if (text == "[") ++square;
                else if (text == "]") --square;
                else if (text == "{") ++brace;
                else if (text == "}") --brace;
                if (localRound == 0 && square == 0 && brace == 0 &&
                    declarationWords.contains(text)) declaration = true;
                if (localRound == 0 && square == 0 && brace == 0 &&
                    (text == "=" || text == "<=")) assignment = true;
            }
            if (!declaration && assignment) hasLogic = true;
            segment = index + 1;
        }
        if (hasLogic)
            emit(result, "GJB10157.5.3.4", source,
                 unit.location.line, 1,
                 "除顶层模块外，上层模块建议仅包含子模块的例化.");
    }
}

std::optional<std::size_t> simpleAssignmentTarget(
    const std::vector<parser::HdlToken>& tokens, std::size_t begin, std::size_t op) {
    if (begin >= op) return std::nullopt;
    auto cursor = op;
    while (cursor > begin && tokens[cursor - 1].text == "]") {
        --cursor;
        int depth = 1;
        while (cursor > begin && depth > 0) {
            --cursor;
            if (tokens[cursor].text == "]") ++depth;
            else if (tokens[cursor].text == "[") --depth;
        }
        if (depth != 0) return std::nullopt;
    }
    if (cursor == begin || tokens[cursor - 1].kind != parser::HdlTokenKind::Identifier)
        return std::nullopt;
    return cursor - 1;
}

void checkRegisteredVerilogModuleOutputs(AnalysisResult& result,
                                         const core::SourceFile& source,
                                         const std::vector<parser::HdlToken>& tokens) {
    // Native slot 1 admits AST_ASSIGN(0x4d), AST_CELL(0x4f) and AST_ALWAYS(0x52).
    // FUN_1807023a0 records output identifiers (direction byte +0xa1), expands
    // scalar/bit/range targets, and groups adjacent uncovered bit indices before
    // reporting. FUN_1804e66e0 returns true only when AST_ALWAYS has no direct
    // AST_POSEDGE/AST_NEGEDGE child. A continuous/combinational assignment becomes
    // a candidate only when its RHS contains an identifier whose resolved WIRE
    // has the parser-proven register byte +0xa2 clear. A direct registered source,
    // edge-triggered process and cell-driven output therefore remain clear.
    // Full-width groups use the exact text "未寄存的模块输出端口<name>"; partial
    // groups append [bit] or [high:low]. Native locations carry physical line but
    // no column and severity text 警告.
    // ev_b5c70dab5cd2580aad460545696268fa8567ca8e10b239ae5ce3c2254bb479a6
    // ev_774788b2878e063d47b80fc5bed9ebcbbcc8d340b766339c20b94cff92e7144d
    for (const auto& unit : result.structure) {
        if (unit.kind != parser::DesignUnitKind::VerilogModule ||
            unit.location.file.lexically_normal() != source.path().lexically_normal()) continue;
        std::set<std::string> outputNames;
        for (const auto& port : unit.ports)
            if (port.direction == parser::PortDirection::Output) outputNames.insert(port.name);
        if (outputNames.empty()) continue;

        std::size_t module = tokens.size();
        for (std::size_t i = 0; i + 1 < tokens.size(); ++i) {
            if (tokens[i].text == "module" && tokens[i + 1].text == unit.name &&
                tokens[i + 1].line == unit.location.line) {
                module = i;
                break;
            }
        }
        if (module == tokens.size()) continue;
        auto moduleEnd = module + 2;
        while (moduleEnd < tokens.size() && tokens[moduleEnd].text != "endmodule") ++moduleEnd;

        const auto decimal = [](std::string_view text) -> std::optional<int> {
            int value = 0;
            const auto [last, error] = std::from_chars(
                text.data(), text.data() + text.size(), value);
            if (error != std::errc{} || last != text.data() + text.size() || value < 0)
                return std::nullopt;
            return value;
        };

        std::map<std::string, std::pair<int, int>> outputBounds;
        for (const auto& name : outputNames) outputBounds.emplace(name, std::pair{0, 0});
        const std::set<std::string_view> directionWords{"input", "output", "inout"};
        for (auto index = module + 2; index < moduleEnd; ++index) {
            if (tokens[index].text != "output") continue;
            auto limit = index + 1;
            while (limit < moduleEnd && tokens[limit].text != ";" &&
                   tokens[limit].text != ")" &&
                   !directionWords.contains(tokens[limit].text)) ++limit;
            std::pair<int, int> bounds{0, 0};
            for (auto cursor = index + 1; cursor + 4 < limit; ++cursor) {
                if (tokens[cursor].text != "[" || tokens[cursor + 2].text != ":" ||
                    tokens[cursor + 4].text != "]") continue;
                const auto left = decimal(tokens[cursor + 1].text);
                const auto right = decimal(tokens[cursor + 3].text);
                if (left && right) bounds = {*left, *right};
                break;
            }
            for (auto cursor = index + 1; cursor < limit; ++cursor)
                if (outputNames.contains(tokens[cursor].text))
                    outputBounds[tokens[cursor].text] = bounds;
        }

        std::set<std::string> registers;
        for (auto index = module + 2; index < moduleEnd; ++index) {
            if (tokens[index].text != "reg" && tokens[index].text != "logic") continue;
            int round = 0, square = 0, brace = 0;
            bool initializer = false;
            for (auto cursor = index + 1; cursor < moduleEnd; ++cursor) {
                const auto& text = tokens[cursor].text;
                if (text == "(") ++round;
                else if (text == ")") {
                    if (round == 0) break;
                    --round;
                } else if (text == "[") ++square;
                else if (text == "]") --square;
                else if (text == "{") ++brace;
                else if (text == "}") --brace;
                if (round == 0 && square == 0 && brace == 0) {
                    if (text == ";" || directionWords.contains(text)) break;
                    if (text == ",") { initializer = false; continue; }
                    if (text == "=") { initializer = true; continue; }
                }
                if (!initializer && square == 0 &&
                    tokens[cursor].kind == parser::HdlTokenKind::Identifier &&
                    text != "signed" && text != "unsigned" && text != "wire")
                    registers.insert(text);
            }
        }

        const std::set<std::string_view> rhsKeywords{
            "begin", "end", "if", "else", "case", "casez", "casex", "endcase",
            "for", "while", "repeat", "forever", "posedge", "negedge", "or",
            "assign", "always", "always_comb", "always_latch"
        };
        const auto hasNonRegisterRhs = [&](std::size_t begin, std::size_t end) {
            for (auto cursor = begin; cursor < end; ++cursor) {
                if (tokens[cursor].kind != parser::HdlTokenKind::Identifier ||
                    rhsKeywords.contains(tokens[cursor].text)) continue;
                if (!registers.contains(tokens[cursor].text)) return true;
            }
            return false;
        };

        std::map<std::pair<std::string, std::size_t>, std::set<int>> candidates;
        const auto record = [&](std::size_t begin, std::size_t op, std::size_t rhsEnd) {
            const auto target = simpleAssignmentTarget(tokens, begin, op);
            if (!target || !outputNames.contains(tokens[*target].text) ||
                !hasNonRegisterRhs(op + 1, rhsEnd)) return;
            const auto& name = tokens[*target].text;
            const auto [left, right] = outputBounds[name];
            auto& bits = candidates[{name, tokens[op].line}];
            if (*target + 1 < op && tokens[*target + 1].text == "[") {
                if (*target + 3 < op && tokens[*target + 3].text == "]") {
                    if (const auto bit = decimal(tokens[*target + 2].text))
                        if (*bit >= std::min(left, right) && *bit <= std::max(left, right))
                            bits.insert(*bit);
                    return;
                }
                if (*target + 5 < op && tokens[*target + 3].text == ":" &&
                    tokens[*target + 5].text == "]") {
                    const auto high = decimal(tokens[*target + 2].text);
                    const auto low = decimal(tokens[*target + 4].text);
                    if (!high || !low) return;
                    for (auto bit = std::min(*high, *low); bit <= std::max(*high, *low); ++bit)
                        if (bit >= std::min(left, right) && bit <= std::max(left, right))
                            bits.insert(bit);
                    return;
                }
                return;
            }
            for (auto bit = std::min(left, right); bit <= std::max(left, right); ++bit)
                bits.insert(bit);
        };

        for (auto index = module + 2; index < moduleEnd; ++index) {
            if (tokens[index].text == "assign") {
                auto end = index + 1;
                int round = 0, square = 0, brace = 0;
                for (; end < moduleEnd; ++end) {
                    const auto& text = tokens[end].text;
                    if (text == "(") ++round;
                    else if (text == ")") --round;
                    else if (text == "[") ++square;
                    else if (text == "]") --square;
                    else if (text == "{") ++brace;
                    else if (text == "}") --brace;
                    if (round == 0 && square == 0 && brace == 0 && text == ";") break;
                }
                auto segment = index + 1;
                round = square = brace = 0;
                for (auto cursor = index + 1; cursor <= end; ++cursor) {
                    const auto text = cursor < end ? tokens[cursor].text : ";";
                    if (text == "(") ++round;
                    else if (text == ")") --round;
                    else if (text == "[") ++square;
                    else if (text == "]") --square;
                    else if (text == "{") ++brace;
                    else if (text == "}") --brace;
                    if (round != 0 || square != 0 || brace != 0 ||
                        (text != "," && text != ";")) continue;
                    auto equal = cursor;
                    int localRound = 0, localSquare = 0, localBrace = 0;
                    for (auto item = segment; item < cursor; ++item) {
                        const auto& itemText = tokens[item].text;
                        if (itemText == "(") ++localRound;
                        else if (itemText == ")") --localRound;
                        else if (itemText == "[") ++localSquare;
                        else if (itemText == "]") --localSquare;
                        else if (itemText == "{") ++localBrace;
                        else if (itemText == "}") --localBrace;
                        else if (localRound == 0 && localSquare == 0 && localBrace == 0 &&
                                 itemText == "=") { equal = item; break; }
                    }
                    if (equal != cursor) record(segment, equal, cursor);
                    segment = cursor + 1;
                }
                index = end == moduleEnd ? index : end;
                continue;
            }

            const auto& word = tokens[index].text;
            if (word != "always" && word != "always_comb" && word != "always_latch") continue;
            auto body = index + 1;
            bool sequential = false;
            if (word == "always") {
                if (body >= moduleEnd || tokens[body].text != "@") continue;
                ++body;
                if (body < moduleEnd && tokens[body].text == "*") {
                    ++body;
                } else if (body < moduleEnd && tokens[body].text == "(") {
                    const auto close = closeParen(tokens, body);
                    if (close >= moduleEnd) continue;
                    for (auto event = body + 1; event < close; ++event)
                        sequential |= tokens[event].text == "posedge" || tokens[event].text == "negedge";
                    body = close + 1;
                } else continue;
            }
            const auto end = statementEnd(tokens, body);
            if (end > moduleEnd) continue;
            if (!sequential) {
                auto segment = body;
                for (auto cursor = body; cursor <= end; ++cursor) {
                    if (cursor < end && tokens[cursor].text != ";") continue;
                    auto op = cursor;
                    int round = 0, square = 0, brace = 0;
                    for (auto item = segment; item < cursor; ++item) {
                        const auto& text = tokens[item].text;
                        if (text == "(") ++round;
                        else if (text == ")") --round;
                        else if (text == "[") ++square;
                        else if (text == "]") --square;
                        else if (text == "{") ++brace;
                        else if (text == "}") --brace;
                        else if (round == 0 && square == 0 && brace == 0 &&
                                 (text == "=" || text == "<=")) { op = item; break; }
                    }
                    if (op != cursor) record(segment, op, cursor);
                    segment = cursor + 1;
                }
            }
            index = end == 0 ? index : end - 1;
        }

        for (const auto& [key, bits] : candidates) {
            if (bits.empty()) continue;
            const auto& [name, line] = key;
            const auto [left, right] = outputBounds[name];
            const auto declaredLow = std::min(left, right);
            const auto declaredHigh = std::max(left, right);
            const bool fullWidth = bits.size() == static_cast<std::size_t>(declaredHigh - declaredLow + 1) &&
                                   *bits.begin() == declaredLow && *bits.rbegin() == declaredHigh;
            if (fullWidth) {
                emit(result, "GJB10157.5.3.5", source, line, 1,
                     "未寄存的模块输出端口" + name);
                continue;
            }
            auto first = bits.begin();
            while (first != bits.end()) {
                auto last = first;
                auto next = std::next(last);
                while (next != bits.end() && *next == *last + 1) {
                    last = next;
                    ++next;
                }
                auto message = "未寄存的模块输出端口" + name + "[";
                if (first == last) message += std::to_string(*first);
                else message += std::to_string(*last) + ":" + std::to_string(*first);
                message += "]";
                emit(result, "GJB10157.5.3.5", source, line, 1, std::move(message));
                first = next;
            }
        }
    }
}

void checkRegisterOutputUsedAsClock(AnalysisResult& result,
                                    const core::SourceFile& source) {
    // Native FUN_180547580 builds a synthesized connectivity model through
    // FUN_180546030/FUN_18054b890/FUN_18054be20, then FUN_18054c850 walks
    // register-cell connections and emits when a register output reaches a
    // register clock input. The native report is attached to the source
    // register process, uses no column, and formats the message as
    // "禁止将寄存器的输出<name>直接连接到其他寄存器的时钟管脚". The bounded source
    // counterpart requires a signal written by an edge-triggered process to be
    // the primary clock of another edge-triggered process. Instance/PLL outputs
    // have no local process writer. One source output is reported once even if
    // it clocks multiple downstream processes.
    // ev_928bda12810b138cd816ae975f012c3fe992313f57f51b26e6723f8dc285cafd
    for (const auto& unit : result.structure) {
        if (unit.kind != parser::DesignUnitKind::VerilogModule ||
            unit.location.file.lexically_normal() != source.path().lexically_normal()) continue;
        std::map<std::string, const parser::SequentialProcessInfo*> registerWriters;
        for (const auto& process : unit.sequentialProcesses)
            for (const auto& signal : process.writes)
                registerWriters.try_emplace(signal, &process);
        std::set<std::string> reported;
        for (const auto& consumer : unit.sequentialProcesses) {
            const auto writer = registerWriters.find(consumer.clock);
            if (writer == registerWriters.end() ||
                !reported.insert(consumer.clock).second) continue;
            emit(result, "GJB10157.6.1.1", source,
                 writer->second->location.line, 1,
                 "禁止将寄存器的输出" + consumer.clock +
                 "直接连接到其他寄存器的时钟管脚");
        }
    }
}

void checkSequentiallyGeneratedAsyncReset(AnalysisResult& result,
                                          const core::SourceFile& source,
                                          const std::vector<parser::HdlToken>& tokens) {
    // Native slot 1 wrapper FUN_180564050 delegates to FUN_1804e6df0. The
    // latter admits only AST_ALWAYS(0x52) nodes having an immediate
    // AST_POSEDGE/AST_NEGEDGE(0x65/0x66) child. FUN_1806f8890 recursively
    // gathers AST_ASSIGN_EQ/LE(0x55/0x56) below those admitted processes and
    // emits once for every matching assignment node when its normalized target
    // name matches an asynchronous-reset relation. The native report is Warning,
    // uses the assignment file and physical line without a column, and formats
    // the default-mode message as "异步复位<name>由时序逻辑驱动". This is the
    // register-generated branch of 6.2.8; the packaged combinational-reset
    // control is deliberately outside this predicate.
    // ev_5134c94e891c153e7d80bced19ef594689c4eac2a62a449ed9797b5ba29af105
    // ev_16b894aa8c8c4e639d06ec49e60914d47df3cf0a8dc74fe48f5c0a9f0c8c24a3
    for (std::size_t module = 0; module + 1 < tokens.size(); ++module) {
        if (tokens[module].text != "module" ||
            tokens[module + 1].kind != parser::HdlTokenKind::Identifier) continue;
        auto moduleEnd = module + 2;
        while (moduleEnd < tokens.size() && tokens[moduleEnd].text != "endmodule")
            ++moduleEnd;
        const auto unit = std::ranges::find_if(result.structure, [&](const auto& candidate) {
            return candidate.kind == parser::DesignUnitKind::VerilogModule &&
                   candidate.location.file.lexically_normal() == source.path().lexically_normal() &&
                   candidate.name == tokens[module + 1].text &&
                   candidate.location.line == tokens[module + 1].line;
        });
        if (unit == result.structure.end()) {
            module = moduleEnd;
            continue;
        }
        std::set<std::string> asynchronousResets;
        for (const auto& reset : unit->resets)
            if (reset.asynchronousObserved) asynchronousResets.insert(reset.signal);
        if (asynchronousResets.empty()) {
            module = moduleEnd;
            continue;
        }

        for (auto process = module + 2; process + 3 < moduleEnd; ++process) {
            if ((tokens[process].text != "always" && tokens[process].text != "always_ff") ||
                tokens[process + 1].text != "@" || tokens[process + 2].text != "(") continue;
            const auto sensitivityEnd = closeParen(tokens, process + 2);
            if (sensitivityEnd >= moduleEnd || sensitivityEnd + 1 >= moduleEnd) continue;
            const bool edgeTriggered = std::ranges::any_of(
                tokens.begin() + static_cast<std::ptrdiff_t>(process + 3),
                tokens.begin() + static_cast<std::ptrdiff_t>(sensitivityEnd),
                [](const auto& token) {
                    return token.text == "posedge" || token.text == "negedge";
                });
            if (!edgeTriggered) continue;
            const auto bodyBegin = sensitivityEnd + 1;
            const auto bodyEnd = statementEnd(tokens, bodyBegin);
            if (bodyEnd <= bodyBegin || bodyEnd > moduleEnd) continue;
            auto segment = bodyBegin;
            int parentheses = 0;
            for (auto at = bodyBegin; at < bodyEnd; ++at) {
                if (tokens[at].text == "(") ++parentheses;
                else if (tokens[at].text == ")" && parentheses > 0) --parentheses;
                if (tokens[at].text == ";") {
                    segment = at + 1;
                    continue;
                }
                if (parentheses != 0 ||
                    (tokens[at].text != "=" && tokens[at].text != "<=")) continue;
                // Native target normalization for selected/compound lvalues has
                // not been recovered; keep those paths review-required.
                if (at > segment && tokens[at - 1].text == "]") continue;
                const auto target = simpleAssignmentTarget(tokens, segment, at);
                if (!target || !asynchronousResets.contains(tokens[*target].text)) continue;
                const auto& written = tokens[*target];
                emit(result, "GJB10157.6.2.8", source,
                     written.line, 1,
                     "异步复位" + written.text + "由时序逻辑驱动", written.text);
            }
            process = bodyEnd > process ? bodyEnd - 1 : process;
        }
        module = moduleEnd;
    }
}

void checkResetConditionEdgePolarity(
    AnalysisResult& result, const core::SourceFile& source,
    const std::vector<parser::HdlToken>& tokens) {
    // Native GJB10157_VLOG.7.1.3 slot 0 (FUN_1805629e0) builds a map from
    // AST_POSEDGE/AST_NEGEDGE (0x65/0x66) identifier children to a polarity
    // byte. It then visits IF nodes (0x57) and reports when an identifier,
    // negated identifier, or identifier/constant equality expresses the
    // opposite active level. FUN_180562930 preserves the relation-name string
    // and its polarity byte. Slot 1 delegates to the shared edge-triggered
    // always filter FUN_1804e6df0.
    // This source adapter deliberately handles only those unambiguous condition
    // shapes; compound Boolean conditions remain review-required.
    // ev_5227f8580d7594cfd1ce8c4870778518f68bfc4c2bd6d9667b892eb70eb9cacb
    // ev_16b894aa8c8c4e639d06ec49e60914d47df3cf0a8dc74fe48f5c0a9f0c8c24a3
    // FUN_1808e3930 builds +0xbc from exactly the low 32 constant bits,
    // sign-extending a signed vector. R-6-3 compares that integer to 0/1.
    // X/Z/? become states 2/3/4, so they do not set cache bits. This mirrors
    // the native cache, not an HDL four-state evaluation or Boolean conversion.
    const auto constantValue = [](const parser::HdlToken& token)
        -> std::optional<std::uint32_t> {
        if (token.kind != parser::HdlTokenKind::Number) return std::nullopt;
        auto text = token.text;
        text.erase(std::remove(text.begin(), text.end(), '_'), text.end());
        if (text == "'0") return 0;
        if (text == "'1") return 1;
        if (text == "'x" || text == "'X" || text == "'z" || text == "'Z") return 0;
        const auto quote = text.find('\'');
        std::uint32_t width = 0;
        bool signedValue = false;
        unsigned base = 10;
        std::size_t digits = 0;
        if (quote != std::string::npos) {
            for (std::size_t at = 0; at < quote; ++at) {
                if (text[at] < '0' || text[at] > '9' ||
                    width > (0x7fffffffU - (text[at] - '0')) / 10U)
                    return std::nullopt;
                width = width * 10U + static_cast<unsigned>(text[at] - '0');
            }
            if (quote != 0 && width == 0) return std::nullopt;
            digits = quote + 1;
            if (digits < text.size() && text[digits] == 's') {
                signedValue = true;
                ++digits;
            }
            if (digits >= text.size()) return std::nullopt;
            switch (text[digits++]) {
            case 'b': case 'B': base = 2; break;
            case 'o': case 'O': base = 8; break;
            case 'd': case 'D': base = 10; break;
            case 'h': case 'H': base = 16; break;
            default: return std::nullopt;
            }
        }
        if (digits == text.size()) return std::nullopt;
        const auto valueBegin = digits;
        std::uint32_t value = 0;
        for (; digits < text.size(); ++digits) {
            const auto digit = text[digits];
            unsigned number;
            if (quote != std::string::npos &&
                (digit == 'x' || digit == 'X' || digit == 'z' || digit == 'Z' || digit == '?')) {
                // Decimal permits a single unknown marker; mixed decimal
                // digits/markers take the native invalid-literal path.
                if (base == 10) {
                    if (text.size() == valueBegin + 1) return 0;
                    return std::nullopt;
                }
                number = 0;
            } else if (digit >= '0' && digit <= '9') number = digit - '0';
            else if (digit >= 'a' && digit <= 'f') number = digit - 'a' + 10;
            else if (digit >= 'A' && digit <= 'F') number = digit - 'A' + 10;
            else return std::nullopt;
            if (number >= base) return std::nullopt;
            value = value * base + number;
        }
        if (width != 0 && width < 32) {
            const auto mask = (std::uint32_t{1} << width) - 1U;
            value &= mask;
            if (signedValue && (value & (std::uint32_t{1} << (width - 1U))))
                value |= ~mask;
        }
        return value;
    };

    // Primary -> mintypmax -> expression transfers the existing AST pointer.
    // One child is preserved; three children select the typical (middle) one.
    const auto primaryRange = [&](std::size_t begin, std::size_t end) {
        while (begin < end && tokens[begin].text == "(" &&
               closeParen(tokens, begin) + 1 == end) {
            ++begin;
            --end;
            int depth = 0;
            int conditional = 0;
            std::vector<std::size_t> separators;
            for (auto at = begin; at < end; ++at) {
                const auto& word = tokens[at].text;
                if (word == "(" || word == "[" || word == "{") ++depth;
                else if (word == ")" || word == "]" || word == "}") --depth;
                else if (depth == 0 && word == "?") ++conditional;
                else if (depth == 0 && word == ":") {
                    if (conditional > 0) --conditional;
                    else separators.push_back(at);
                }
            }
            if (conditional != 0 || (!separators.empty() && separators.size() != 2))
                return std::pair{end, end};
            if (separators.size() == 2) {
                begin = separators[0] + 1;
                end = separators[1];
            }
        }
        return std::pair{begin, end};
    };
    const auto identifierRange = [&](std::size_t begin, std::size_t end) {
        if (begin == end || tokens[begin].kind != parser::HdlTokenKind::Identifier) return false;
        auto at = begin + 1;
        while (at < end && tokens[at].text == "[") {
            const auto opening = at++;
            int depth = 1;
            for (; at < end && depth != 0; ++at) {
                if (tokens[at].text == "[") ++depth;
                else if (tokens[at].text == "]") --depth;
            }
            if (depth != 0 || at == opening + 2) return false;
        }
        return at == end;
    };
    const auto comparisonRoot = [&](std::size_t begin, std::size_t end)
        -> std::optional<std::size_t> {
        std::optional<std::size_t> comparison;
        int depth = 0;
        bool operand = true;
        for (auto at = begin; at < end; ++at) {
            const auto& word = tokens[at].text;
            if (word == "(" || word == "[" || word == "{") ++depth;
            else if (word == ")" || word == "]" || word == "}") {
                if (--depth < 0) return std::nullopt;
                if (depth == 0) operand = false;
            } else if (depth == 0) {
                if (operand) {
                    if (word == "+" || word == "-" || word == "!" || word == "~" ||
                        word == "&" || word == "|" || word == "^" || word == "~&" ||
                        word == "~|" || word == "~^" || word == "^~") continue;
                    operand = false;
                } else {
                    // These binary operators bind below equality. Their AST
                    // roots cannot enter the native equality report branch.
                    if (word == "?" || word == ":" || word == "||" || word == "&&" ||
                        word == "|" || word == "^" || word == "~^" || word == "^~" || word == "&")
                        return std::nullopt;
                    if (word == "==" || word == "!=" || word == "===" || word == "!==")
                        comparison = at; // Equal precedence is left-associative.
                    operand = true;
                }
            }
        }
        return depth == 0 && !operand ? comparison : std::nullopt;
    };
    const auto statementRange = [&](std::size_t begin, std::size_t end) {
        while (begin < end && tokens[begin].text == "begin" &&
               tokens[end - 1].text == "end" && statementEnd(tokens, begin) == end) {
            ++begin;
            --end;
            if (begin + 1 < end && tokens[begin].text == ":") begin += 2;
        }
        return std::pair{begin, end};
    };
    using ResetValueRange = std::pair<std::size_t, std::size_t>;
    struct ResetAssignment {
        std::size_t target;
        ResetValueRange value;
    };
    const auto simpleAssignment = [&](std::size_t begin, std::size_t end)
        -> std::optional<ResetAssignment> {
        const auto [first, last] = statementRange(begin, end);
        if (last < first + 4 || tokens[first].kind != parser::HdlTokenKind::Identifier ||
            (tokens[first + 1].text != "=" && tokens[first + 1].text != "<=") ||
            tokens[last - 1].text != ";") return std::nullopt;
        const auto [rhs, rhsEnd] = primaryRange(first + 2, last - 1);
        if (rhs >= rhsEnd) return std::nullopt;
        return ResetAssignment{first, {rhs, rhsEnd}};
    };
    using ResetAssignments = std::map<std::string, ResetValueRange>;
    std::function<std::optional<ResetAssignments>(std::size_t, std::size_t)> branchAssignments;
    branchAssignments = [&](std::size_t begin, std::size_t end)
        -> std::optional<ResetAssignments> {
        const auto [first, last] = statementRange(begin, end);
        ResetAssignments assignments;
        for (auto at = first; at < last;) {
            if (tokens[at].text == ";") { ++at; continue; }
            const auto memberEnd = statementEnd(tokens, at);
            if (memberEnd <= at || memberEnd > last) return std::nullopt;
            if (tokens[at].text == "begin") {
                const auto nested = branchAssignments(at, memberEnd);
                if (!nested) return std::nullopt;
                for (const auto& [name, rhs] : *nested)
                    if (!assignments.emplace(name, rhs).second) return std::nullopt;
            } else {
                const auto assignment = simpleAssignment(at, memberEnd);
                if (!assignment ||
                    !assignments.emplace(tokens[assignment->target].text, assignment->value).second)
                    return std::nullopt;
            }
            at = memberEnd;
        }
        if (assignments.empty()) return std::nullopt;
        return assignments;
    };
    const VerilogConstantExpressions symbols(tokens);
    detail::NativeVerilogInteger integerFold(tokens, symbols);
    std::vector<ResetValueRange> activeInitializers;
    std::function<bool(std::size_t, std::size_t, unsigned)> constantResetValue;
    constantResetValue = [&](std::size_t begin, std::size_t end, unsigned depth) {
        if (depth >= 256) return false;
        const auto [first, last] = primaryRange(begin, end);
        if (first >= last) return false;
        // A recovered immutable value also proves constant reset data, including
        // supported concatenations; overridable bindings retain grammar checks.
        if (integerFold.evaluate(first, last)) return true;
        bool operand = true;
        for (auto at = first; at < last;) {
            const auto& word = tokens[at].text;
            if (operand) {
                if (word == "+" || word == "-" || word == "!" || word == "~" ||
                    word == "&" || word == "|" || word == "^" || word == "~&" ||
                    word == "~|" || word == "~^" || word == "^~") { ++at; continue; }
                if (word == "(") {
                    const auto close = closeParen(tokens, at);
                    if (close >= last || !constantResetValue(at, close + 1, depth + 1))
                        return false;
                    at = close + 1;
                } else {
                    if (!constantValue(tokens[at])) {
                        const auto initializer = symbols.parameterInitializer(at);
                        if (!initializer ||
                            std::ranges::find(activeInitializers, *initializer) != activeInitializers.end())
                            return false;
                        activeInitializers.push_back(*initializer);
                        const bool known = constantResetValue(
                            initializer->first, initializer->second, depth + 1);
                        activeInitializers.pop_back();
                        if (!known) return false;
                    }
                    ++at;
                }
                operand = false;
            } else {
                if (word != "+" && word != "-" && word != "*" && word != "/" &&
                    word != "%" && word != "**" && word != "<<" && word != ">>" &&
                    word != "<<<" && word != ">>>" && word != "<" && word != ">" &&
                    word != "<=" && word != ">=" && word != "==" && word != "!=" &&
                    word != "===" && word != "!==" && word != "&&" && word != "||" &&
                    word != "&" && word != "|" && word != "^" && word != "~^" && word != "^~")
                    return false;
                operand = true;
                ++at;
            }
        }
        return !operand;
    };

    // runRulesPrepare snapshots modules after proc_arst. The native rule
    // accepts only level-sensitive reset syncs (type < 2); proc_arst keeps a
    // sole edge sync in its edge-trigger branch. A complete leaf module with
    // no multi-edge process cannot establish that reset catalog in this path.
    // This is a necessary gate, not recovery of the full synthesized catalog.
    std::vector<bool> resetCatalogPossible(tokens.size(), true);
    std::vector<std::size_t> resetCatalogOwner(tokens.size(), tokens.size());
    // Publish names only when every multi-edge process is recovered below.
    // Otherwise absence from a partial set cannot exclude a native candidate.
    std::map<std::size_t, std::set<std::string>> resetNameCatalogs;
    std::map<std::size_t, std::set<std::string>> nonScalarEventNames;
    for (std::size_t module = 0; module < tokens.size(); ++module) {
        if (tokens[module].text != "module") continue;
        auto end = module + 1;
        while (end < tokens.size() && tokens[end].text != "endmodule") ++end;
        if (end == tokens.size()) continue;
        auto name = module + 1;
        if (tokens[name].text == "automatic" || tokens[name].text == "static") ++name;
        bool knownLeaf = false;
        const parser::DesignUnitInfo* moduleInfo = nullptr;
        if (name < end) {
            for (const auto& unit : result.structure) {
                if (unit.kind == parser::DesignUnitKind::VerilogModule &&
                    unit.name == tokens[name].text &&
                    unit.location.file.lexically_normal() == source.path().lexically_normal() &&
                    unit.location.line >= tokens[module].line && unit.location.line <= tokens[end].line) {
                    knownLeaf = unit.instances.empty();
                    moduleInfo = &unit;
                    break;
                }
            }
        }
        const auto signalWidths = moduleInfo ? nativeResetSignalWidths(*moduleInfo, source, tokens)
                                            : std::map<std::string, std::size_t>{};
        const auto scalar = [&](const std::string& signal) {
            const auto found = signalWidths.find(signal);
            return found != signalWidths.end() && found->second == 1;
        };
        const auto oneBitConstantPort = [&](std::size_t begin, std::size_t finish) {
            const auto [first, last] = primaryRange(begin, finish);
            if (last != first + 1 || !constantValue(tokens[first])) return false;
            auto text = tokens[first].text;
            text.erase(std::remove(text.begin(), text.end(), '_'), text.end());
            const auto quote = text.find('\'');
            if (quote == std::string::npos || quote == 0) return false;
            unsigned width = 0;
            for (std::size_t at = 0; at < quote; ++at) {
                if (text[at] < '0' || text[at] > '9') return false;
                width = width * 10U + static_cast<unsigned>(text[at] - '0');
                if (width > 1) return false;
            }
            return width == 1;
        };
        // Native root matching includes $eq/$eqx/$ne/$nex with a constant
        // port and a recursively matched one-bit port. This recovers the
        // unique scalar origin only; it does not extend diagnostic IF shapes.
        std::function<std::optional<std::size_t>(std::size_t, std::size_t, bool)> resetRoot;
        resetRoot = [&](std::size_t begin, std::size_t finish, bool oneBitContext)
            -> std::optional<std::size_t> {
            const auto [first, last] = primaryRange(begin, finish);
            if (first >= last) return std::nullopt;
            if (last == first + 1 &&
                tokens[first].kind == parser::HdlTokenKind::Identifier &&
                scalar(tokens[first].text) && symbols.isDeclaredRuntimeIdentifier(first))
                return first;
            // Identifier generation preserves its wire width. Operator Y
            // widths can instead grow with a wider comparison operand hint.
            if (!oneBitContext) return std::nullopt;
            const auto comparison = comparisonRoot(first, last);
            if (comparison) {
                const auto [left, leftEnd] = primaryRange(first, *comparison);
                const auto [right, rightEnd] = primaryRange(*comparison + 1, last);
                if (constantResetValue(left, leftEnd, 0))
                    return resetRoot(right, rightEnd, oneBitConstantPort(left, leftEnd));
                if (constantResetValue(right, rightEnd, 0))
                    return resetRoot(left, leftEnd, oneBitConstantPort(right, rightEnd));
                return std::nullopt;
            }
            if (tokens[first].text == "!" || tokens[first].text == "~")
                return resetRoot(first + 1, last, true);
            return std::nullopt;
        };
        bool catalogClosed = knownLeaf;
        std::set<std::string> resetNames;
        // Alias-producing declarations and elaboration can change the netlist
        // names. Do not publish a closed catalog for those unresolved forms.
        for (auto at = module + 1; at < end; ++at) {
            const auto& word = tokens[at].text;
            if (word == "assign" || word == "generate" || word == "genvar" ||
                word == "function" || word == "task" || word == "alias" || word == "bind" ||
                word == "integer" || word == "int" || word == "shortint" || word == "longint" ||
                word == "byte" || word == "time" || word == "real" || word == "shortreal" ||
                word == "realtime" || word == "struct" || word == "union" || word == "typedef" ||
                word == "enum" || word == "type")
                catalogClosed = false;
            if (word == "wire" || word == "reg" || word == "logic" || word == "bit") {
                for (auto declaration = at + 1; declaration < end &&
                     tokens[declaration].text != ";"; ++declaration) {
                    if (tokens[declaration].text == "=") catalogClosed = false;
                }
            }
        }
        // Level-reset roots are one bit. Formatting a slice of a wider wire
        // includes its range, while this rule looks up the AST base name.
        // Keep this necessary name filter separate from catalog completeness.
        if (catalogClosed) {
            for (const auto& [signal, width] : signalWidths)
                if (width > 1) nonScalarEventNames[module].insert(signal);
        }
        bool multiEdge = false;
        for (auto process = module + 1; process + 3 < end; ++process) {
            if ((tokens[process].text != "always" && tokens[process].text != "always_ff") ||
                tokens[process + 1].text != "@" || tokens[process + 2].text != "(") continue;
            const auto eventEnd = closeParen(tokens, process + 2);
            if (eventEnd >= end) continue;
            std::size_t edges = 0;
            for (auto at = process + 3; at < eventEnd; ++at) {
                if (tokens[at].text == "posedge" || tokens[at].text == "negedge") ++edges;
            }
            if (edges > 1) {
                multiEdge = true;
                if (tokens[process].text != "always") catalogClosed = false;
                std::set<std::string> events;
                for (auto at = process + 3; at < eventEnd; ++at) {
                    if (tokens[at].text != "posedge" && tokens[at].text != "negedge") continue;
                    auto eventLast = at + 1;
                    int depth = 0;
                    for (; eventLast < eventEnd; ++eventLast) {
                        const auto& word = tokens[eventLast].text;
                        if (depth == 0 && (word == "or" || word == ",")) break;
                        if (word == "(" || word == "[" || word == "{") ++depth;
                        else if (word == ")" || word == "]" || word == "}") --depth;
                    }
                    const auto [first, last] = primaryRange(at + 1, eventLast);
                    if (last == first + 1 && tokens[first].kind == parser::HdlTokenKind::Identifier &&
                        scalar(tokens[first].text)) events.insert(tokens[first].text);
                    at = eventLast;
                }
                const auto bodyEnd = statementEnd(tokens, eventEnd + 1);
                if (events.size() != edges || bodyEnd > end) { catalogClosed = false; continue; }
                const auto [root, rootEnd] = statementRange(eventEnd + 1, bodyEnd);
                if (root + 2 >= rootEnd || tokens[root].text != "if" ||
                    tokens[root + 1].text != "(" || statementEnd(tokens, root) != rootEnd) {
                    catalogClosed = false; continue;
                }
                const auto conditionEnd = closeParen(tokens, root + 1);
                const auto condition = resetRoot(root + 2, conditionEnd, true);
                const auto thenEnd = statementEnd(tokens, conditionEnd + 1);
                if (!condition || thenEnd >= rootEnd || tokens[thenEnd].text != "else") {
                    catalogClosed = false; continue;
                }
                const auto thenAssignments = branchAssignments(conditionEnd + 1, thenEnd);
                const auto elseAssignments = branchAssignments(thenEnd + 1, rootEnd);
                bool assignmentsKnown = thenAssignments && elseAssignments &&
                    thenAssignments->size() == elseAssignments->size();
                if (assignmentsKnown) {
                    for (const auto& [target, rhs] : *thenAssignments) {
                        const auto other = elseAssignments->find(target);
                        const auto targetWidth = signalWidths.find(target);
                        const auto runtimeData = [&](ResetValueRange value) {
                            const auto [at, last] = value;
                            if (last != at + 1) return false;
                            if (!symbols.isDeclaredRuntimeIdentifier(at)) return false;
                            const auto dataWidth = signalWidths.find(tokens[at].text);
                            return dataWidth != signalWidths.end() &&
                                targetWidth != signalWidths.end() &&
                                dataWidth->second == targetWidth->second;
                        };
                        if (targetWidth == signalWidths.end() || other == elseAssignments->end() ||
                            !((constantResetValue(rhs.first, rhs.second, 0) &&
                               runtimeData(other->second)) ||
                              (constantResetValue(other->second.first, other->second.second, 0) &&
                               runtimeData(rhs)))) {
                            assignmentsKnown = false;
                            break;
                        }
                    }
                }
                if (!assignmentsKnown) {
                    catalogClosed = false; continue;
                }
                if (events.contains(tokens[*condition].text)) resetNames.insert(tokens[*condition].text);
            }
            process = eventEnd;
        }
        for (auto at = module; at <= end; ++at) resetCatalogOwner[at] = module;
        if (catalogClosed) resetNameCatalogs.emplace(module, std::move(resetNames));
        if (knownLeaf && !multiEdge) {
            for (auto at = module; at <= end; ++at) resetCatalogPossible[at] = false;
        }
        module = end;
    }

    for (std::size_t index = 0; index + 3 < tokens.size(); ++index) {
        if (!resetCatalogPossible[index]) continue;
        if (tokens[index].text != "always" || tokens[index + 1].text != "@" ||
            tokens[index + 2].text != "(") continue;
        const auto sensitivityEnd = closeParen(tokens, index + 2);
        if (sensitivityEnd == tokens.size()) continue;

        std::map<std::string, bool> edgePolarity;
        for (auto at = index + 3; at + 1 < sensitivityEnd; ++at) {
            if (tokens[at].text != "posedge" && tokens[at].text != "negedge") continue;
            auto eventEnd = at + 1;
            int depth = 0;
            for (; eventEnd < sensitivityEnd; ++eventEnd) {
                const auto& word = tokens[eventEnd].text;
                if (depth == 0 && (word == "or" || word == ",")) break;
                if (word == "(" || word == "[" || word == "{") ++depth;
                else if (word == ")" || word == "]" || word == "}") --depth;
            }
            const auto [first, last] = primaryRange(at + 1, eventEnd);
            if (identifierRange(first, last) && !constantResetValue(first, last, 0))
                edgePolarity[tokens[first].text] = tokens[at].text == "posedge";
            at = eventEnd;
        }
        if (const auto wide = nonScalarEventNames.find(resetCatalogOwner[index]);
            wide != nonScalarEventNames.end()) {
            std::erase_if(edgePolarity, [&](const auto& event) {
                return wide->second.contains(event.first);
            });
        }
        if (const auto catalog = resetNameCatalogs.find(resetCatalogOwner[index]);
            catalog != resetNameCatalogs.end()) {
            std::erase_if(edgePolarity, [&](const auto& event) {
                return !catalog->second.contains(event.first);
            });
        }
        if (edgePolarity.empty() || sensitivityEnd + 1 >= tokens.size()) continue;
        const auto bodyEnd = statementEnd(tokens, sensitivityEnd + 1);
        if (bodyEnd <= sensitivityEnd + 1 || bodyEnd > tokens.size()) continue;

        for (auto at = sensitivityEnd + 1; at + 2 < bodyEnd; ++at) {
            if (tokens[at].text != "if" || tokens[at + 1].text != "(") continue;
            const auto conditionEnd = closeParen(tokens, at + 1);
            if (conditionEnd >= bodyEnd) continue;
            const auto [first, last] = primaryRange(at + 2, conditionEnd);
            if (first == last) continue;

            std::string signal;
            std::optional<std::uint32_t> conditionPolarity;
            auto diagnosticAt = first;
            if (identifierRange(first, last)) {
                signal = tokens[first].text;
                conditionPolarity = true;
            } else if (tokens[first].text == "!") {
                const auto [operand, operandEnd] = primaryRange(first + 1, last);
                if (identifierRange(operand, operandEnd)) {
                    signal = tokens[operand].text;
                    conditionPolarity = false;
                }
            } else {
                const auto equality = comparisonRoot(first, last);
                if (equality && tokens[*equality].text == "==") {
                    const auto [left, leftEnd] = primaryRange(first, *equality);
                    const auto [right, rightEnd] = primaryRange(*equality + 1, last);
                    const auto cacheExpression = [&](std::size_t begin, std::size_t end)
                        -> std::optional<std::pair<std::uint32_t, std::size_t>> {
                        if (end == begin + 1) {
                            if (const auto cached = constantValue(tokens[begin]))
                                return std::pair{*cached, begin};
                        }
                        if (const auto value = integerFold.evaluate(begin, end))
                            return std::pair{value->cache(), value->source};
                        return std::nullopt;
                    };
                    const auto compare = [&](std::size_t name, std::size_t nameEnd,
                                             std::size_t value, std::size_t valueEnd) {
                        if (!identifierRange(name, nameEnd)) return false;
                        const auto cached = cacheExpression(value, valueEnd);
                        if (!cached) return false;
                        signal = tokens[name].text;
                        conditionPolarity = cached->first;
                        diagnosticAt = cached->second;
                        return true;
                    };
                    if (!compare(left, leftEnd, right, rightEnd))
                        compare(right, rightEnd, left, leftEnd);
                }
            }
            const auto edge = edgePolarity.find(signal);
            if (!conditionPolarity || edge == edgePolarity.end() ||
                *conditionPolarity == edge->second) continue;
            // Native equality branches copy the constant child's +0x210;
            // direct/unary branches copy the condition node's +0x210.
            // The native report has no column field.
            emit(result, "GJB10157.7.1.3", source, tokens[diagnosticAt].line,
                 1,
                 "复位信号的触发条件与进入条件必须一致.");
            result.violations.back().severity = Severity::Warning;
        }
        index = bodyEnd == 0 ? index : bodyEnd - 1;
    }
}

void checkFlipFlopAsyncResetCoverage(
    AnalysisResult& result, const core::SourceFile& source,
    const std::vector<parser::HdlToken>& tokens) {
    // GJB10157_VLOG.7.1.1 FUN_18055e770 and GJB10157_VLOG.7.1.2
    // FUN_180560210 both consume synthesized relation records and AST_ALWAYS
    // (0x52) nodes. For 7.1.1, FUN_18055e020 finds the final AST_DEFAULT branch
    // of an IF/ELSE chain, builds one normalized assignment-target map from
    // that branch and another from assignments at or before the default line,
    // then emits once for every default-branch target absent from the earlier
    // map. Empty earlier maps therefore report every default target. Native
    // output is fixed Error, uses the default-branch assignment line with no
    // column, and composes
    // "always语句中混合使用有复位和无复位的触发器：触发器“<name>”未进行复位".
    // 7.1.2 additionally gathers ASSIGN_EQ/LE (0x55/0x56) targets;
    // FUN_180561f70 records identifier targets and a Boolean coverage flag.
    // This adapter retains simple event identifiers and a leading reset IF,
    // while 7.1.1 now mirrors final-default comparison and per-target grain.
    // ev_34073b3f9020270ea752c6de0e419d56d4a95fc558a0ca1cb938eb6a6d76db53
    // ev_a506aeee1095bbd38ec023f42e6be00489019077d677fb81e1dfed6c1e051cb2
    const auto constantBit = [](const parser::HdlToken& token) -> std::optional<bool> {
        if (token.kind != parser::HdlTokenKind::Number) return std::nullopt;
        auto text = lower(token.text);
        text.erase(std::remove(text.begin(), text.end(), '_'), text.end());
        static const std::set<std::string> zeros{
            "0", "1'b0", "1'd0", "1'h0", "1'o0"
        };
        static const std::set<std::string> ones{
            "1", "1'b1", "1'd1", "1'h1", "1'o1"
        };
        if (zeros.contains(text)) return false;
        if (ones.contains(text)) return true;
        return std::nullopt;
    };
    const auto condition = [&](std::size_t open, std::size_t close)
        -> std::optional<std::pair<std::string, bool>> {
        auto first = open + 1;
        auto last = close;
        while (first < last && tokens[first].text == "(" &&
               closeParen(tokens, first) + 1 == last) {
            ++first;
            --last;
        }
        if (last == first + 1 &&
            tokens[first].kind == parser::HdlTokenKind::Identifier)
            return std::pair{tokens[first].text, true};
        if (last == first + 2 &&
            (tokens[first].text == "!" || tokens[first].text == "~") &&
            tokens[first + 1].kind == parser::HdlTokenKind::Identifier)
            return std::pair{tokens[first + 1].text, false};
        if (last == first + 3 && tokens[first + 1].text == "==") {
            if (tokens[first].kind == parser::HdlTokenKind::Identifier) {
                if (const auto bit = constantBit(tokens[first + 2]))
                    return std::pair{tokens[first].text, *bit};
            } else if (tokens[first + 2].kind == parser::HdlTokenKind::Identifier) {
                if (const auto bit = constantBit(tokens[first]))
                    return std::pair{tokens[first + 2].text, *bit};
            }
        }
        return std::nullopt;
    };
    const auto targets = [&](std::size_t begin, std::size_t end) {
        std::map<std::string, std::size_t> found;
        auto segment = begin;
        static const std::set<std::string> declarations{
            "reg", "wire", "logic", "bit", "byte", "shortint", "int",
            "longint", "integer", "time", "realtime"
        };
        for (auto at = begin; at < end; ++at) {
            if (tokens[at].text == ";") {
                segment = at + 1;
                continue;
            }
            if (tokens[at].text != "=" && tokens[at].text != "<=") continue;
            bool declaration = false;
            for (auto cursor = segment; cursor < at; ++cursor)
                declaration |= declarations.contains(tokens[cursor].text);
            if (declaration) continue;
            if (const auto target = simpleAssignmentTarget(tokens, segment, at))
                found.try_emplace(tokens[*target].text, *target);
        }
        return found;
    };

    for (std::size_t index = 0; index + 3 < tokens.size(); ++index) {
        if (tokens[index].text != "always" || tokens[index + 1].text != "@" ||
            tokens[index + 2].text != "(") continue;
        const auto sensitivityEnd = closeParen(tokens, index + 2);
        if (sensitivityEnd == tokens.size() || sensitivityEnd + 1 >= tokens.size()) continue;
        std::map<std::string, bool> edges;
        for (auto at = index + 3; at + 1 < sensitivityEnd; ++at) {
            if ((tokens[at].text != "posedge" && tokens[at].text != "negedge") ||
                tokens[at + 1].kind != parser::HdlTokenKind::Identifier) continue;
            edges[tokens[at + 1].text] = tokens[at].text == "posedge";
        }
        if (edges.empty()) continue;
        const auto bodyStart = sensitivityEnd + 1;
        const auto bodyEnd = statementEnd(tokens, bodyStart);
        if (bodyEnd <= bodyStart || bodyEnd > tokens.size()) continue;

        const auto checkNativeCoverage = [&](std::size_t bodyStart, std::size_t bodyEnd,
                                              bool singleEdge) {
            // Native FUN_180561f70 writes true for a target with an
            // identifier-free RHS. Dynamic RHS inserts false only if the key
            // is absent, so later dynamic assignments cannot erase true.
            // A parameter/localparam identifier anywhere in the RHS also
            // writes true, even when other RHS names are dynamic.
            auto processTargets = targets(bodyStart, bodyEnd);
            std::size_t moduleBegin = index;
            while (moduleBegin > 0 && tokens[moduleBegin].text != "module") --moduleBegin;
            auto moduleEnd = index;
            while (moduleEnd < tokens.size() && tokens[moduleEnd].text != "endmodule") ++moduleEnd;
            std::set<std::string> parameterNames;
            for (auto declaration = moduleBegin; declaration < moduleEnd; ++declaration) {
                if (tokens[declaration].text != "parameter" &&
                    tokens[declaration].text != "localparam") continue;
                auto segment = declaration + 1;
                int depth = 0;
                for (auto cursor = segment; cursor < moduleEnd; ++cursor) {
                    const auto& text = tokens[cursor].text;
                    if (depth == 0 && text == "=") {
                        if (cursor > segment &&
                            tokens[cursor - 1].kind == parser::HdlTokenKind::Identifier)
                            parameterNames.insert(tokens[cursor - 1].text);
                    }
                    if (depth == 0 && (text == ";" || text == ")")) break;
                    if (depth == 0 && text == ",") segment = cursor + 1;
                    if (text == "(" || text == "[" || text == "{") ++depth;
                    else if (text == ")" || text == "]" || text == "}") --depth;
                }
            }
            std::map<std::string, bool> coverage;
            int conditionDepth = 0;
            for (auto op = bodyStart; op + 2 < bodyEnd; ++op) {
                if (tokens[op].text == "(") ++conditionDepth;
                else if (tokens[op].text == ")") --conditionDepth;
                if (conditionDepth != 0) continue;
                if (tokens[op].text != "=" && tokens[op].text != "<=") continue;
                auto rhsEnd = op + 1;
                while (rhsEnd < bodyEnd && tokens[rhsEnd].text != ";") ++rhsEnd;
                if (rhsEnd == bodyEnd) continue;
                bool hasIdentifier = false, hasParameter = false, supported = true;
                for (auto rhs = op + 1; rhs < rhsEnd; ++rhs) {
                    const auto& token = tokens[rhs];
                    if (token.kind == parser::HdlTokenKind::Identifier) {
                        hasIdentifier = true;
                        hasParameter |= parameterNames.contains(token.text);
                        if (rhs + 1 < rhsEnd && tokens[rhs + 1].text == "(") supported = false;
                    }
                    if (token.text.starts_with("$") || token.text == ";" ||
                        token.text == "begin" || token.text == "end") supported = false;
                }
                std::vector<std::size_t> assignmentTargets;
                if (const auto target = simpleAssignmentTarget(tokens, bodyStart, op)) {
                    if (processTargets.contains(tokens[*target].text)) assignmentTargets.push_back(*target);
                } else if (op > bodyStart && tokens[op - 1].text == "}") {
                    // Native recursive collection visits reverse-ordered
                    // AST_CONCAT children, including nested concat roots.
                    auto cursor = op;
                    std::function<bool(unsigned)> collectLvalue = [&](unsigned depth) {
                        if (cursor <= bodyStart || depth > 64) return false;
                        --cursor;
                        if (tokens[cursor].kind == parser::HdlTokenKind::Identifier) {
                            assignmentTargets.push_back(cursor);
                            return true;
                        }
                        if (tokens[cursor].text != "}") return false;
                        if (!collectLvalue(depth + 1)) return false;
                        while (cursor > bodyStart && tokens[cursor - 1].text == ",") {
                            --cursor;
                            if (!collectLvalue(depth + 1)) return false;
                        }
                        if (cursor <= bodyStart || tokens[cursor - 1].text != "{") return false;
                        --cursor;
                        return true;
                    };
                    if (!collectLvalue(0)) assignmentTargets.clear();
                    for (const auto target : assignmentTargets)
                        processTargets.try_emplace(tokens[target].text, target);
                }
                if (assignmentTargets.empty()) continue;
                if (!supported) {
                    for (const auto target : assignmentTargets)
                        coverage.try_emplace(tokens[target].text, false);
                } else if (!hasIdentifier || hasParameter) {
                    for (const auto target : assignmentTargets) coverage[tokens[target].text] = true;
                } else {
                    // Each non-parameter RHS identifier inserts only the first
                    // absent target, then breaks. Do not eagerly seed every
                    // concat target: absent keys do not produce native reports.
                    for (auto rhs = op + 1; rhs < rhsEnd; ++rhs) {
                        if (tokens[rhs].kind != parser::HdlTokenKind::Identifier) continue;
                        for (const auto target : assignmentTargets) {
                            if (coverage.try_emplace(tokens[target].text, false).second) break;
                        }
                    }
                }
            }
            for (const auto& [name, token] : processTargets) {
                if (!coverage.contains(name) || coverage.at(name)) continue;
                // Native single-edge report reads the process +0x200 start
                // token line (FUN_1800dcc60 -> FUN_1808d8dc0) and no column.
                emit(result, "GJB10157.7.1.2", source,
                     singleEdge ? tokens[index].line : tokens[bodyStart].line, 1,
                     "触发器" + name + (singleEdge ? "无异步复位或置位" : "无初值"));
            }
        };
        if (edges.size() == 1) {
            checkNativeCoverage(bodyStart, bodyEnd, true);
            index = bodyEnd - 1;
            continue;
        }

        auto resetIf = bodyStart;
        if (tokens[resetIf].text == "begin") {
            ++resetIf;
            if (resetIf + 1 < bodyEnd && tokens[resetIf].text == ":" &&
                tokens[resetIf + 1].kind == parser::HdlTokenKind::Identifier)
                resetIf += 2;
        }
        if (resetIf + 2 >= bodyEnd || tokens[resetIf].text != "if" ||
            tokens[resetIf + 1].text != "(") {
            index = bodyEnd - 1;
            continue;
        }
        const auto conditionEnd = closeParen(tokens, resetIf + 1);
        if (conditionEnd >= bodyEnd) {
            index = bodyEnd - 1;
            continue;
        }
        const auto resetCondition = condition(resetIf + 1, conditionEnd);
        if (!resetCondition || !edges.contains(resetCondition->first) ||
            edges.at(resetCondition->first) != resetCondition->second) {
            index = bodyEnd - 1;
            continue;
        }
        const auto resetBegin = conditionEnd + 1;
        const auto resetEnd = statementEnd(tokens, resetBegin);
        if (resetEnd >= bodyEnd || tokens[resetEnd].text != "else") {
            index = bodyEnd - 1;
            continue;
        }
        const auto dataBegin = resetEnd + 1;
        const auto dataEnd = statementEnd(tokens, dataBegin);
        if (dataEnd > bodyEnd) {
            index = bodyEnd - 1;
            continue;
        }
        // FUN_1804f33d0 admits AST_CASE roots under ALWAYS->BLOCK and
        // ALWAYS->BLOCK->BLOCK. Each root gets its own state map; never
        // descend through an admitted IF/CASE to create overlapping roots.
        std::function<void(std::size_t, std::size_t, unsigned, unsigned)> checkCandidates =
            [&](std::size_t begin, std::size_t end, unsigned blocks, unsigned maxBlocks) {
                if (begin >= end) return;
                if (tokens[begin].text == "begin") {
                    if (blocks >= maxBlocks) return;
                    auto child = begin + 1;
                    if (child + 1 < end && tokens[child].text == ":" &&
                        tokens[child + 1].kind == parser::HdlTokenKind::Identifier) child += 2;
                    while (child < end && tokens[child].text != "end") {
                        const auto next = statementEnd(tokens, child);
                        if (next <= child || next > end) break;
                        checkCandidates(child, next, blocks + 1, maxBlocks);
                        child = next;
                    }
                    return;
                }
                const auto& kind = tokens[begin].text;
                if (kind == "if" || kind == "case" || kind == "casex" || kind == "casez")
                    checkNativeCoverage(begin, end, false);
            };
        // The native output vector appends without deduplication: the first
        // pass permits two block ancestors, the second permits one. Direct
        // candidates therefore appear in both passes, preserving reports.
        checkCandidates(bodyStart, bodyEnd, 0, 2);
        checkCandidates(bodyStart, bodyEnd, 0, 1);

        // FUN_180560070 follows the same-line nested CASE representation of an
        // else-if chain until its final AST_DEFAULT. At source level, walk the
        // equivalent chain: every branch preceding the final else contributes
        // to the earlier-target map, and the final else is the compared map.
        std::map<std::string, std::size_t> earlierTargets;
        auto chainIf = resetIf;
        bool hasFinalDefault = false;
        std::size_t finalDefaultLine = 0;
        std::map<std::string, std::size_t> defaultTargets;
        while (chainIf + 2 < bodyEnd && tokens[chainIf].text == "if" &&
               tokens[chainIf + 1].text == "(") {
            const auto chainConditionEnd = closeParen(tokens, chainIf + 1);
            if (chainConditionEnd >= bodyEnd) break;
            const auto branchBegin = chainConditionEnd + 1;
            const auto branchEnd = statementEnd(tokens, branchBegin);
            if (branchEnd > bodyEnd) break;
            for (const auto& [name, token] : targets(branchBegin, branchEnd))
                earlierTargets.try_emplace(name, token);
            if (branchEnd >= bodyEnd || tokens[branchEnd].text != "else" ||
                branchEnd + 1 >= bodyEnd) break;
            const auto next = branchEnd + 1;
            if (tokens[next].text == "if") {
                chainIf = next;
                continue;
            }
            const auto finalEnd = statementEnd(tokens, next);
            if (finalEnd > bodyEnd) break;
            defaultTargets = targets(next, finalEnd);
            // FUN_18055f9a0 walks the complete IF tree and retains assignments
            // whose physical line is <= the AST_DEFAULT line. Preserve the
            // observable same-line cutoff by treating source `else` as the
            // bounded default-line proxy.
            finalDefaultLine = tokens[branchEnd].line;
            for (const auto& [name, token] : defaultTargets) {
                if (tokens[token].line <= finalDefaultLine)
                    earlierTargets.try_emplace(name, token);
            }
            hasFinalDefault = true;
            break;
        }
        if (hasFinalDefault) {
            for (const auto& [name, token] : defaultTargets) {
                if (earlierTargets.contains(name)) continue;
                emit(result, "GJB10157.7.1.1", source,
                     tokens[token].line, 1,
                     "always语句中混合使用有复位和无复位的触发器：触发器“" +
                         name + "”未进行复位");
            }
        }
        index = bodyEnd - 1;
    }
}

void checkInternalVerilogTristate(
    AnalysisResult& result, const core::SourceFile& source,
    const std::vector<parser::HdlToken>& tokens) {
    // Native GJB10157_VLOG.7.3.2 slot 1 FUN_1807095f0 admits only
    // AST_ASSIGN(0x4d). FUN_180707090 then walks the synthesized connectivity
    // model and distinguishes internal tri-state nodes from module boundary
    // ports. The packaged example proves the direct source subset: a
    // continuous assignment containing a Z value is a violation when its
    // target is internal, but is allowed on an output/inout port.
    // ev_2209a945bd76ac52c180022bc03deeec1625d228b0f614923da576ab9dbb2a73
    // ev_e331c21365e2bf52ef4363c343c0903f327a1e6fd4ae50bd935b7c520c1b078d
    for (const auto& unit : result.structure) {
        if (unit.kind != parser::DesignUnitKind::VerilogModule ||
            unit.location.file.lexically_normal() != source.path().lexically_normal()) continue;
        std::set<std::string> boundaryTargets;
        for (const auto& port : unit.ports)
            if (port.direction == parser::PortDirection::Output ||
                port.direction == parser::PortDirection::Inout)
                boundaryTargets.insert(port.name);

        auto module = tokens.size();
        for (std::size_t index = 0; index + 1 < tokens.size(); ++index) {
            if (tokens[index].text == "module" && tokens[index + 1].text == unit.name &&
                tokens[index + 1].line == unit.location.line) {
                module = index;
                break;
            }
        }
        if (module == tokens.size()) continue;
        auto moduleEnd = module + 2;
        while (moduleEnd < tokens.size() && tokens[moduleEnd].text != "endmodule") ++moduleEnd;

        for (auto index = module + 2; index < moduleEnd; ++index) {
            if (tokens[index].text != "assign") continue;
            auto end = index + 1;
            int round = 0, square = 0, brace = 0;
            for (; end < moduleEnd; ++end) {
                const auto& text = tokens[end].text;
                if (text == "(") ++round;
                else if (text == ")") --round;
                else if (text == "[") ++square;
                else if (text == "]") --square;
                else if (text == "{") ++brace;
                else if (text == "}") --brace;
                if (round == 0 && square == 0 && brace == 0 && text == ";") break;
            }
            auto equal = end;
            round = square = brace = 0;
            for (auto cursor = index + 1; cursor < end; ++cursor) {
                const auto& text = tokens[cursor].text;
                if (text == "(") ++round;
                else if (text == ")") --round;
                else if (text == "[") ++square;
                else if (text == "]") --square;
                else if (text == "{") ++brace;
                else if (text == "}") --brace;
                else if (round == 0 && square == 0 && brace == 0 && text == "=") {
                    equal = cursor;
                    break;
                }
            }
            const auto target = simpleAssignmentTarget(tokens, index + 1, equal);
            if (!target || boundaryTargets.contains(tokens[*target].text)) {
                index = end == moduleEnd ? index : end;
                continue;
            }
            std::optional<std::size_t> zLiteral;
            for (auto cursor = equal + 1; cursor < end; ++cursor) {
                if ((verilogUnknownMask(tokens[cursor]) & 2U) != 0) zLiteral = cursor;
            }
            if (zLiteral)
                emit(result, "GJB10157.7.3.2", source,
                     tokens[*target].line, tokens[*target].column,
                     "内部信号 " + tokens[*target].text + " 使用了三态 Z 驱动");
            index = end == moduleEnd ? index : end;
        }
    }
}

void checkCombinationalTristateEnable(
    AnalysisResult& result, const core::SourceFile& source,
    const std::vector<parser::HdlToken>& tokens) {
    // Native GJB10157_VLOG.7.3.1 slot 1 admits AST_MODULE(2), while
    // FUN_18070b040 builds and walks a synthesized connectivity model. The
    // packaged cases establish the source-visible boundary used here: a net
    // produced by combinational logic and then used as a tri-state enable is
    // reported, while an input, register output or black-box output is not.
    // ev_188aca2a42524e501637351540e16f64eba88a95345836312f576d307c154699
    // ev_aaa493aac258e6c8e9f4d04fcd2ebeb575f37e5d55e363a21c32a358a3f3965b
    struct Driver {
        std::size_t target{};
        std::set<std::string> sources;
        bool hasLogic{};
    };
    const std::set<std::string_view> logicOperators{
        "!", "~", "&", "|", "^", "^~", "~^", "&&", "||", "+", "-",
        "*", "/", "%", "<<", ">>", "<<<", ">>>", "==", "!=", "===",
        "!==", "<", ">", "<=", ">=", "?"
    };
    for (const auto& unit : result.structure) {
        if (unit.kind != parser::DesignUnitKind::VerilogModule ||
            unit.location.file.lexically_normal() != source.path().lexically_normal()) continue;
        std::size_t module = tokens.size();
        for (std::size_t index = 0; index + 1 < tokens.size(); ++index) {
            if (tokens[index].text == "module" && tokens[index + 1].text == unit.name &&
                tokens[index + 1].line == unit.location.line) {
                module = index;
                break;
            }
        }
        if (module == tokens.size()) continue;
        auto moduleEnd = module + 2;
        while (moduleEnd < tokens.size() && tokens[moduleEnd].text != "endmodule") ++moduleEnd;

        struct Assignment { std::size_t equal, end, target; };
        std::vector<Assignment> assignments;
        std::map<std::string, Driver> drivers;
        for (auto index = module + 2; index < moduleEnd; ++index) {
            if (tokens[index].text != "assign") continue;
            auto end = index + 1;
            int round = 0, square = 0, brace = 0;
            for (; end < moduleEnd; ++end) {
                const auto& text = tokens[end].text;
                if (text == "(") ++round;
                else if (text == ")") --round;
                else if (text == "[") ++square;
                else if (text == "]") --square;
                else if (text == "{") ++brace;
                else if (text == "}") --brace;
                if (round == 0 && square == 0 && brace == 0 && text == ";") break;
            }
            auto equal = end;
            round = square = brace = 0;
            for (auto cursor = index + 1; cursor < end; ++cursor) {
                const auto& text = tokens[cursor].text;
                if (text == "(") ++round;
                else if (text == ")") --round;
                else if (text == "[") ++square;
                else if (text == "]") --square;
                else if (text == "{") ++brace;
                else if (text == "}") --brace;
                else if (round == 0 && square == 0 && brace == 0 && text == "=") {
                    equal = cursor;
                    break;
                }
            }
            const auto target = simpleAssignmentTarget(tokens, index + 1, equal);
            if (target) {
                assignments.push_back({equal, end, *target});
                Driver driver{*target, {}, false};
                for (auto cursor = equal + 1; cursor < end; ++cursor) {
                    if (tokens[cursor].kind == parser::HdlTokenKind::Identifier)
                        driver.sources.insert(tokens[cursor].text);
                    if (logicOperators.contains(tokens[cursor].text)) driver.hasLogic = true;
                }
                drivers[tokens[*target].text] = std::move(driver);
            }
            index = end == moduleEnd ? index : end;
        }

        const auto combinationalOrigin = [&](const std::string& name)
            -> std::optional<std::size_t> {
            std::string current = name;
            std::set<std::string> visited;
            while (visited.insert(current).second) {
                const auto driver = drivers.find(current);
                if (driver == drivers.end()) return std::nullopt;
                if (driver->second.hasLogic) return driver->second.target;
                if (driver->second.sources.size() != 1) return std::nullopt;
                current = *driver->second.sources.begin();
            }
            return std::nullopt;
        };

        std::set<std::string> reported;
        for (const auto& assignment : assignments) {
            std::size_t question = assignment.end;
            int round = 0, square = 0, brace = 0;
            for (auto cursor = assignment.equal + 1; cursor < assignment.end; ++cursor) {
                const auto& text = tokens[cursor].text;
                if (text == "(") ++round;
                else if (text == ")") --round;
                else if (text == "[") ++square;
                else if (text == "]") --square;
                else if (text == "{") ++brace;
                else if (text == "}") --brace;
                else if (question == assignment.end && round == 0 && square == 0 &&
                         brace == 0 && text == "?") {
                    question = cursor;
                }
            }
            if (question == assignment.end) continue;
            bool hasZ = false;
            for (auto cursor = question + 1; cursor < assignment.end; ++cursor)
                hasZ |= (verilogUnknownMask(tokens[cursor]) & 2U) != 0;
            if (!hasZ) continue;
            for (auto cursor = assignment.equal + 1; cursor < question; ++cursor) {
                if (tokens[cursor].kind != parser::HdlTokenKind::Identifier) continue;
                const auto origin = combinationalOrigin(tokens[cursor].text);
                if (!origin) continue;
                const auto& target = tokens[*origin];
                if (!reported.insert(target.text).second) continue;
                emit(result, "GJB10157.7.3.1", source, target.line, target.column,
                     "组合逻辑输出 " + target.text + " 被用作三态缓冲器使能");
            }
        }
    }
}

void checkLatchMixedWithCombinationalLogic(
    AnalysisResult& result, const core::SourceFile& source,
    const std::vector<parser::HdlToken>& tokens) {
    // Native GJB10157_VLOG.7.2.3 slot 1 admits AST_MODULE(2).
    // FUN_18056e500 gathers AST_ALWAYS(0x52), rejects processes containing
    // AST_POSEDGE/AST_NEGEDGE(0x65/0x66), locates IF default branches through
    // FUN_180560070 and builds target/RHS-identifier records for
    // AST_ASSIGN_EQ/LE(0x55/0x56) through FUN_18056f800. The packaged case
    // proves the explicit self-hold latch plus other-target combination.
    // ev_ef7a798da6d1f5a123bf8eacbd11ee8ac475347f5231741fde070b34055cd06d
    // ev_aaa493aac258e6c8e9f4d04fcd2ebeb575f37e5d55e363a21c32a358a3f3965b
    for (const auto& unit : result.structure) {
        if (unit.kind != parser::DesignUnitKind::VerilogModule ||
            unit.location.file.lexically_normal() != source.path().lexically_normal()) continue;
        std::size_t module = tokens.size();
        for (std::size_t index = 0; index + 1 < tokens.size(); ++index) {
            if (tokens[index].text == "module" && tokens[index + 1].text == unit.name &&
                tokens[index + 1].line == unit.location.line) {
                module = index;
                break;
            }
        }
        if (module == tokens.size()) continue;
        auto moduleEnd = module + 2;
        while (moduleEnd < tokens.size() && tokens[moduleEnd].text != "endmodule") ++moduleEnd;

        for (auto index = module + 2; index < moduleEnd; ++index) {
            const auto& word = tokens[index].text;
            if (word != "always" && word != "always_comb") continue;
            auto body = index + 1;
            bool sequential = false;
            if (word == "always") {
                if (body >= moduleEnd || tokens[body].text != "@") continue;
                ++body;
                if (body < moduleEnd && tokens[body].text == "*") {
                    ++body;
                } else if (body < moduleEnd && tokens[body].text == "(") {
                    const auto close = closeParen(tokens, body);
                    if (close >= moduleEnd) continue;
                    for (auto event = body + 1; event < close; ++event)
                        sequential |= tokens[event].text == "posedge" ||
                                      tokens[event].text == "negedge";
                    body = close + 1;
                } else continue;
            }
            const auto end = statementEnd(tokens, body);
            if (sequential || end > moduleEnd) continue;

            std::map<std::string, std::size_t> targets;
            std::set<std::string> latchTargets;
            auto segment = body;
            int round = 0;
            for (auto cursor = body; cursor < end; ++cursor) {
                if (tokens[cursor].text == "(") { ++round; continue; }
                if (tokens[cursor].text == ")") { --round; continue; }
                if (tokens[cursor].text == ";") {
                    segment = cursor + 1;
                    continue;
                }
                if (round != 0) continue;
                if (tokens[cursor].text != "=" && tokens[cursor].text != "<=") continue;
                const auto target = simpleAssignmentTarget(tokens, segment, cursor);
                auto rhsEnd = cursor + 1;
                while (rhsEnd < end && tokens[rhsEnd].text != ";") ++rhsEnd;
                if (target) {
                    const auto& targetToken = tokens[*target];
                    targets.try_emplace(targetToken.text, *target);
                    for (auto rhs = cursor + 1; rhs < rhsEnd; ++rhs)
                        if (tokens[rhs].kind == parser::HdlTokenKind::Identifier &&
                            tokens[rhs].text == targetToken.text)
                            latchTargets.insert(targetToken.text);
                }
                cursor = rhsEnd == end ? cursor : rhsEnd;
                segment = cursor + 1;
            }

            for (const auto& violation : result.violations) {
                if (violation.ruleId != "RTL-LATCH-INFERRED" ||
                    violation.file.lexically_normal() != source.path().lexically_normal() ||
                    violation.line != tokens[index].line || !violation.signal) continue;
                latchTargets.insert(*violation.signal);
            }
            if (latchTargets.empty()) {
                index = end == 0 ? index : end - 1;
                continue;
            }
            for (const auto& [name, token] : targets) {
                if (latchTargets.contains(name)) continue;
                emit(result, "GJB10157.7.2.3", source,
                     tokens[token].line, tokens[token].column,
                     "组合过程将锁存器与其他组合逻辑目标混合描述：" + name);
            }
            index = end == 0 ? index : end - 1;
        }
    }
}

void checkAsyncResetConnectedToNonResetUse(
    AnalysisResult& result, const core::SourceFile& source,
    const std::vector<parser::HdlToken>& tokens) {
    // Native FUN_1806e8d60 constructs asynchronous-reset relation name sets,
    // gathers AST_ALWAYS(0x52) assignments and AST_CELL(0x4f) connections,
    // then searches assignment data / cell actuals for AST_IDENTIFIER(0x13)
    // matches. Cell connection metadata is consulted to exclude reset/set
    // ports. The embedded constants at 0x180f07a48 / 0x180f126e0 compose
    // exact text "异步复位/置位信号<name>连接到非复位/置位端口". Every admitted
    // identifier AST is appended and reported independently at its physical
    // line with no column. This source adapter implements only definite
    // assignment-data uses and named non-reset/set formal connections.
    // ev_2be3a67f908304b85d468cb0368d9c237a2416337bc8bfe6a3136677de8e53e6
    // ev_aaa493aac258e6c8e9f4d04fcd2ebeb575f37e5d55e363a21c32a358a3f3965b
    const auto resetFormal = [](std::string name) {
        name = lower(std::move(name));
        return name.find("reset") != std::string::npos ||
            name.find("rst") != std::string::npos ||
            name.find("preset") != std::string::npos ||
            name.find("clear") != std::string::npos ||
            name.find("clr") != std::string::npos || name == "set" ||
            name.starts_with("set_") || name.ends_with("_set") ||
            name.find("_set_") != std::string::npos;
    };

    for (const auto& unit : result.structure) {
        if (unit.kind != parser::DesignUnitKind::VerilogModule ||
            unit.location.file.lexically_normal() != source.path().lexically_normal()) continue;
        std::set<std::string> asynchronousResets;
        for (const auto& reset : unit.resets)
            if (reset.asynchronousObserved) asynchronousResets.insert(reset.signal);
        if (asynchronousResets.empty()) continue;

        std::size_t module = tokens.size();
        for (std::size_t i = 0; i + 1 < tokens.size(); ++i) {
            if (tokens[i].text == "module" && tokens[i + 1].text == unit.name &&
                tokens[i + 1].line == unit.location.line) {
                module = i;
                break;
            }
        }
        if (module == tokens.size()) continue;
        auto moduleEnd = module + 2;
        while (moduleEnd < tokens.size() && tokens[moduleEnd].text != "endmodule") ++moduleEnd;

        std::set<std::size_t> reported;
        const auto report = [&](std::size_t token) {
            if (!reported.insert(token).second) return;
            emit(result, "GJB10157.6.2.1", source, tokens[token].line, 1,
                 "异步复位/置位信号" + tokens[token].text +
                     "连接到非复位/置位端口");
        };
        const auto resetsInRange = [&](std::size_t begin, std::size_t end) {
            std::vector<std::size_t> matches;
            for (auto index = begin; index < end; ++index)
                if (tokens[index].kind == parser::HdlTokenKind::Identifier &&
                    asynchronousResets.contains(tokens[index].text))
                    matches.push_back(index);
            return matches;
        };

        for (auto index = module + 2; index < moduleEnd; ++index) {
            if (tokens[index].text != "assign") continue;
            auto end = index + 1;
            int round = 0, square = 0, brace = 0;
            for (; end < moduleEnd; ++end) {
                const auto& text = tokens[end].text;
                if (text == "(") ++round;
                else if (text == ")") --round;
                else if (text == "[") ++square;
                else if (text == "]") --square;
                else if (text == "{") ++brace;
                else if (text == "}") --brace;
                if (round == 0 && square == 0 && brace == 0 && text == ";") break;
            }
            std::size_t equal = end;
            round = square = brace = 0;
            for (auto cursor = index + 1; cursor < end; ++cursor) {
                const auto& text = tokens[cursor].text;
                if (text == "(") ++round;
                else if (text == ")") --round;
                else if (text == "[") ++square;
                else if (text == "]") --square;
                else if (text == "{") ++brace;
                else if (text == "}") --brace;
                else if (round == 0 && square == 0 && brace == 0 && text == "=") {
                    equal = cursor;
                    break;
                }
            }
            const auto target = simpleAssignmentTarget(tokens, index + 1, equal);
            if (target && equal < end && !asynchronousResets.contains(tokens[*target].text))
                for (const auto reset : resetsInRange(equal + 1, end)) report(reset);
            index = end == moduleEnd ? index : end;
        }

        for (auto index = module + 2; index < moduleEnd; ++index) {
            const auto& word = tokens[index].text;
            if (word != "always" && word != "always_comb" && word != "always_latch") continue;
            auto body = index + 1;
            if (word == "always") {
                if (body >= moduleEnd || tokens[body].text != "@") continue;
                ++body;
                if (body < moduleEnd && tokens[body].text == "*") ++body;
                else if (body < moduleEnd && tokens[body].text == "(") {
                    const auto close = closeParen(tokens, body);
                    if (close >= moduleEnd) continue;
                    body = close + 1;
                } else continue;
            }
            const auto end = statementEnd(tokens, body);
            if (end > moduleEnd) continue;
            for (auto op = body; op < end; ++op) {
                if (tokens[op].text != "=" && tokens[op].text != "<=") continue;
                const auto target = simpleAssignmentTarget(tokens, body, op);
                auto rhsEnd = op + 1;
                while (rhsEnd < end && tokens[rhsEnd].text != ";") ++rhsEnd;
                if (target && !asynchronousResets.contains(tokens[*target].text))
                    for (const auto reset : resetsInRange(op + 1, rhsEnd)) report(reset);
                op = rhsEnd == end ? op : rhsEnd;
            }
            index = end == 0 ? index : end - 1;
        }

        for (const auto& instance : unit.instances) {
            for (const auto& connection : instance.namedConnections) {
                if (!connection.actualConnected || resetFormal(connection.formal)) continue;
                auto formal = moduleEnd;
                for (auto index = module + 2; index < moduleEnd; ++index) {
                    if (tokens[index].line == connection.location.line &&
                        tokens[index].column == connection.location.column) {
                        formal = index;
                        break;
                    }
                }
                if (formal == moduleEnd || formal + 1 >= moduleEnd ||
                    tokens[formal + 1].text != "(") continue;
                const auto close = closeParen(tokens, formal + 1);
                if (close >= moduleEnd) continue;
                for (const auto reset : resetsInRange(formal + 2, close)) report(reset);
            }
        }
    }
}

void checkSynchronousUseOfAsynchronousReset(
    AnalysisResult& result, const core::SourceFile& source,
    const std::vector<parser::HdlToken>& tokens) {
    // GJB10157_VLOG.6.2.2 constructor 0x180022670 installs vtable
    // 0x180ef1200. Selector FUN_1807238a0 admits AST_MODULE(2). Main
    // FUN_180557c70 builds a unique asynchronous-control name vector from
    // relation records whose type byte is 0 or 1 (FUN_1804dee00), then visits
    // AST_ALWAYS(0x52). It admits a process only when the recursive set of
    // AST_POSEDGE/AST_NEGEDGE(0x65/0x66) has exactly one member, selects the
    // first AST_REDUCE_BOOL(0x2d), and reports every matching
    // AST_IDENTIFIER(0x13) below that condition. Every identifier is emitted
    // independently at its physical line with no column. FUN_18003e420
    // appends DAT_180ef1248, producing exact text
    // "<name>同时作为了异步复位和同步复位" with severity "error".
    // This adapter preserves that definite same-module source subset. The
    // proprietary relation producer and cross-hierarchy normalization remain
    // outside the recovered boundary.
    // ev_c1ad9263fef41a23ccdd650fc33ddf40e671bbada095d390d35e8fe5380e3cf1
    // ev_aaa493aac258e6c8e9f4d04fcd2ebeb575f37e5d55e363a21c32a358a3f3965b
    for (const auto& unit : result.structure) {
        if (unit.kind != parser::DesignUnitKind::VerilogModule ||
            unit.location.file.lexically_normal() != source.path().lexically_normal()) continue;
        std::set<std::string> asynchronousControls;
        for (const auto& reset : unit.resets)
            if (reset.asynchronousObserved) asynchronousControls.insert(reset.signal);
        if (asynchronousControls.empty()) continue;

        std::size_t module = tokens.size();
        for (std::size_t index = 0; index + 1 < tokens.size(); ++index) {
            if (tokens[index].text == "module" && tokens[index + 1].text == unit.name &&
                tokens[index + 1].line == unit.location.line) {
                module = index;
                break;
            }
        }
        if (module == tokens.size()) continue;
        auto moduleEnd = module + 2;
        while (moduleEnd < tokens.size() && tokens[moduleEnd].text != "endmodule") ++moduleEnd;

        for (auto process = module + 2; process < moduleEnd; ++process) {
            if (tokens[process].text != "always" && tokens[process].text != "always_ff")
                continue;
            if (process + 2 >= moduleEnd || tokens[process + 1].text != "@" ||
                tokens[process + 2].text != "(") continue;
            const auto eventEnd = closeParen(tokens, process + 2);
            if (eventEnd >= moduleEnd) continue;
            const auto edgeCount = std::count_if(
                tokens.begin() + static_cast<std::ptrdiff_t>(process + 3),
                tokens.begin() + static_cast<std::ptrdiff_t>(eventEnd),
                [](const auto& token) {
                    return token.text == "posedge" || token.text == "negedge";
                });
            const auto bodyEnd = statementEnd(tokens, eventEnd + 1);
            if (bodyEnd > moduleEnd || edgeCount != 1) {
                process = bodyEnd > 0 && bodyEnd <= moduleEnd ? bodyEnd - 1 : process;
                continue;
            }

            std::size_t firstConditionControl = bodyEnd;
            for (auto token = eventEnd + 1; token + 1 < bodyEnd; ++token) {
                const auto& word = tokens[token].text;
                if ((word == "if" || word == "while" || word == "for") &&
                    tokens[token + 1].text == "(") {
                    firstConditionControl = token;
                    break;
                }
            }
            if (firstConditionControl != bodyEnd &&
                tokens[firstConditionControl].text == "if") {
                const auto conditionEnd = closeParen(tokens, firstConditionControl + 1);
                if (conditionEnd < bodyEnd) {
                    for (auto token = firstConditionControl + 2;
                         token < conditionEnd; ++token) {
                        if (tokens[token].kind != parser::HdlTokenKind::Identifier ||
                            !asynchronousControls.contains(tokens[token].text)) continue;
                        emit(result, "GJB10157.6.2.2", source, tokens[token].line, 1,
                             tokens[token].text + "同时作为了异步复位和同步复位",
                             tokens[token].text);
                    }
                }
            }
            process = bodyEnd == 0 ? process : bodyEnd - 1;
        }
    }
}

void checkClockUsedAsData(AnalysisResult& result,
                          const core::SourceFile& source,
                          const std::vector<parser::HdlToken>& tokens) {
    // Native GJB10157_VLOG_6_1_2 admits AST_ALWAYS only when its first child
    // is AST_POSEDGE(0x65) or AST_NEGEDGE(0x66). FUN_180729250 collects the
    // elaborated clock relations, recursively gathers AST_ASSIGN_EQ(0x55) and
    // AST_ASSIGN_LE(0x56) below those processes, and compares identifiers in
    // assignment data with clock names. The comparison reads the RHS root
    // node's +0x48 name directly; it does not recursively search conditions or
    // compound RHS expressions. Each matching assignment independently emits
    // exact text "时钟<name>驱动非时钟引脚." at the RHS identifier's physical
    // line, with no column. The packaged black-box connection is documented as
    // a violation but is not selected by this native implementation; cell-port
    // semantics therefore remain deliberately outside this adapter.
    // ev_1be0b0252eccce0f64a73f9a4c64990cec83b41408076e27ea7a5a8c3db26f21
    // ev_370fe66f55ae6530e42c0fca58ceaab7cef4d4724a76601782814c548ed7bcf6
    for (const auto& unit : result.structure) {
        if (unit.kind != parser::DesignUnitKind::VerilogModule ||
            unit.location.file.lexically_normal() != source.path().lexically_normal()) continue;
        std::set<std::string> clocks;
        for (const auto& process : unit.sequentialProcesses)
            for (const auto& event : process.clockEvents)
                clocks.insert(event.signal);
        if (clocks.empty()) continue;

        std::size_t module = tokens.size();
        for (std::size_t index = 0; index + 1 < tokens.size(); ++index) {
            if (tokens[index].text == "module" && tokens[index + 1].text == unit.name &&
                tokens[index + 1].line == unit.location.line) {
                module = index;
                break;
            }
        }
        if (module == tokens.size()) continue;
        auto moduleEnd = module + 2;
        while (moduleEnd < tokens.size() && tokens[moduleEnd].text != "endmodule") ++moduleEnd;

        const auto directRhsIdentifier = [&](std::size_t begin, std::size_t end)
            -> std::optional<std::size_t> {
            while (begin < end && tokens[begin].text == "(" &&
                   closeParen(tokens, begin) + 1 == end) {
                ++begin;
                --end;
            }
            if (begin >= end || tokens[begin].kind != parser::HdlTokenKind::Identifier)
                return std::nullopt;
            auto cursor = begin + 1;
            while (cursor < end && tokens[cursor].text == "[") {
                int depth = 1;
                for (++cursor; cursor < end && depth != 0; ++cursor) {
                    if (tokens[cursor].text == "[") ++depth;
                    else if (tokens[cursor].text == "]") --depth;
                }
                if (depth != 0) return std::nullopt;
            }
            return cursor == end ? std::optional<std::size_t>{begin} : std::nullopt;
        };

        for (auto index = module + 2; index < moduleEnd; ++index) {
            if (tokens[index].text != "always" || index + 2 >= moduleEnd ||
                tokens[index + 1].text != "@") continue;
            auto firstEvent = index + 2;
            auto body = moduleEnd;
            if (tokens[firstEvent].text == "(") {
                const auto close = closeParen(tokens, firstEvent);
                if (close >= moduleEnd) continue;
                ++firstEvent;
                body = close + 1;
            } else if (firstEvent + 1 < moduleEnd) {
                body = firstEvent + 2;
            }
            if (firstEvent >= moduleEnd ||
                (tokens[firstEvent].text != "posedge" &&
                 tokens[firstEvent].text != "negedge")) continue;
            const auto end = statementEnd(tokens, body);
            if (end > moduleEnd) continue;

            int round = 0;
            for (auto op = body; op < end; ++op) {
                if (tokens[op].text == "(") { ++round; continue; }
                if (tokens[op].text == ")") { --round; continue; }
                if (round != 0 ||
                    (tokens[op].text != "=" && tokens[op].text != "<=")) continue;
                if (!simpleAssignmentTarget(tokens, body, op)) continue;
                auto rhsEnd = op + 1;
                int rhsRound = 0, square = 0, brace = 0;
                for (; rhsEnd < end; ++rhsEnd) {
                    const auto& text = tokens[rhsEnd].text;
                    if (text == "(") ++rhsRound;
                    else if (text == ")") --rhsRound;
                    else if (text == "[") ++square;
                    else if (text == "]") --square;
                    else if (text == "{") ++brace;
                    else if (text == "}") --brace;
                    else if (text == ";" && rhsRound == 0 && square == 0 && brace == 0)
                        break;
                }
                const auto identifier = directRhsIdentifier(op + 1, rhsEnd);
                if (identifier && clocks.contains(tokens[*identifier].text))
                    emit(result, "GJB10157.6.1.2", source,
                         tokens[*identifier].line, 1,
                         "时钟" + tokens[*identifier].text + "驱动非时钟引脚.");
                op = rhsEnd == end ? op : rhsEnd;
            }
            index = end == 0 ? index : end - 1;
        }
    }
}

void checkCombinationalOutputUsedAsClock(
    AnalysisResult& result, const core::SourceFile& source,
    const std::vector<parser::HdlToken>& tokens) {
    // Native slot 1 admits AST_ASSIGN(0x4d) and AST_ALWAYS(0x52), but rejects
    // an AST_ALWAYS containing any AST_POSEDGE/AST_NEGEDGE child. Slot 0 then
    // gathers AST_ASSIGN_EQ/LE below admitted combinational processes and
    // compares their identifier targets with elaborated clock relations. For
    // continuous AST_ASSIGN, a direct-identifier RHS is excluded, so a plain
    // clock alias is not classified as combinational logic. Matched candidates
    // enter a string-keyed tree by normalized target name; duplicate drivers
    // of one clock name retain the first candidate and emit once. Native text
    // is "禁止将组合逻辑的输出<name>作为时钟信号", severity is Warning, and the
    // candidate carries a physical line but no column.
    // ev_a2c625015f0d0a597841603ab30ba60956e1da25c812a0993e3bd7b033990930
    // ev_728be2738a7301c4d1b1fcc211bbd2f86f712a45cdf20651ab93666ae09aebc3
    for (const auto& unit : result.structure) {
        if (unit.kind != parser::DesignUnitKind::VerilogModule ||
            unit.location.file.lexically_normal() != source.path().lexically_normal()) continue;
        std::set<std::string> clocks;
        for (const auto& clock : unit.clocks) clocks.insert(clock.signal);
        if (clocks.empty()) continue;

        std::size_t module = tokens.size();
        for (std::size_t i = 0; i + 1 < tokens.size(); ++i) {
            if (tokens[i].text == "module" && tokens[i + 1].text == unit.name &&
                tokens[i + 1].line == unit.location.line) {
                module = i;
                break;
            }
        }
        if (module == tokens.size()) continue;
        auto moduleEnd = module + 2;
        while (moduleEnd < tokens.size() && tokens[moduleEnd].text != "endmodule") ++moduleEnd;

        std::set<std::string> reported;
        const auto report = [&](std::size_t target, std::string_view driver) {
            const auto& token = tokens[target];
            if (!clocks.contains(token.text) ||
                !reported.insert(token.text).second) return;
            (void)driver;
            emit(result, "GJB10157.6.1.3", source, token.line, 1,
                 "禁止将组合逻辑的输出" + token.text + "作为时钟信号");
        };

        for (auto index = module + 2; index < moduleEnd; ++index) {
            if (tokens[index].text == "assign") {
                auto end = index + 1;
                int round = 0, square = 0, brace = 0;
                for (; end < moduleEnd; ++end) {
                    const auto& text = tokens[end].text;
                    if (text == "(") ++round;
                    else if (text == ")") --round;
                    else if (text == "[") ++square;
                    else if (text == "]") --square;
                    else if (text == "{") ++brace;
                    else if (text == "}") --brace;
                    if (round == 0 && square == 0 && brace == 0 && text == ";") break;
                }
                std::size_t equal = end;
                round = square = brace = 0;
                for (auto cursor = index + 1; cursor < end; ++cursor) {
                    const auto& text = tokens[cursor].text;
                    if (text == "(") ++round;
                    else if (text == ")") --round;
                    else if (text == "[") ++square;
                    else if (text == "]") --square;
                    else if (text == "{") ++brace;
                    else if (text == "}") --brace;
                    else if (round == 0 && square == 0 && brace == 0 && text == "=") {
                        equal = cursor;
                        break;
                    }
                }
                const auto target = simpleAssignmentTarget(tokens, index + 1, equal);
                if (target && equal < end &&
                    !nativeDirectIdentifierRhs(tokens, equal + 1, end))
                    report(*target, "输出");
                index = end == moduleEnd ? index : end;
                continue;
            }

            const auto& word = tokens[index].text;
            if (word != "always" && word != "always_comb" && word != "always_latch") continue;
            auto body = index + 1;
            bool sequential = false;
            if (word == "always") {
                if (body >= moduleEnd || tokens[body].text != "@") continue;
                ++body;
                if (body < moduleEnd && tokens[body].text == "*") {
                    ++body;
                } else if (body < moduleEnd && tokens[body].text == "(") {
                    const auto close = closeParen(tokens, body);
                    if (close >= moduleEnd) continue;
                    for (auto event = body + 1; event < close; ++event)
                        sequential |= tokens[event].text == "posedge" ||
                                      tokens[event].text == "negedge";
                    body = close + 1;
                } else continue;
            }
            const auto end = statementEnd(tokens, body);
            if (end > moduleEnd) continue;
            if (!sequential) {
                auto segment = body;
                for (auto cursor = body; cursor < end; ++cursor) {
                    if (tokens[cursor].text == ";") {
                        segment = cursor + 1;
                        continue;
                    }
                    if (tokens[cursor].text != "=" && tokens[cursor].text != "<=") continue;
                    if (const auto target = simpleAssignmentTarget(tokens, segment, cursor))
                        report(*target, "输出");
                }
            }
            index = end == 0 ? index : end - 1;
        }
    }
}

void checkCombinationalOutputUsedAsAsyncReset(
    AnalysisResult& result, const core::SourceFile& source,
    const std::vector<parser::HdlToken>& tokens) {
    // GJB10157_VLOG_6_2_3 and GJB10157_VLOG_6_2_7 both dispatch slot 0 to
    // FUN_180758410 and use the same AST_MODULE filter FUN_1807238a0. The
    // native function gathers AST_ASSIGN(0x4d) plus edge-free AST_ALWAYS(0x52)
    // assignments and compares their targets with asynchronous-reset
    // relations. Direct AST_IDENTIFIER RHS values and unary
    // AST_LOGIC_NOT(0x47)/AST_BIT_NOT(0x24) nodes containing exactly one
    // direct AST_IDENTIFIER child are propagated as aliases;
    // other continuous expressions and combinational procedural drivers
    // reach diagnostic construction.
    // ev_84ff04283d5bc45c830f2cd48b7030d46eb94114556b32aa556a0b8308ae6833
    // ev_aaa493aac258e6c8e9f4d04fcd2ebeb575f37e5d55e363a21c32a358a3f3965b
    for (const auto& unit : result.structure) {
        if (unit.kind != parser::DesignUnitKind::VerilogModule ||
            unit.location.file.lexically_normal() != source.path().lexically_normal()) continue;
        std::set<std::string> asynchronousResets;
        for (const auto& reset : unit.resets)
            if (reset.asynchronousObserved) asynchronousResets.insert(reset.signal);
        if (asynchronousResets.empty()) continue;

        std::size_t module = tokens.size();
        for (std::size_t i = 0; i + 1 < tokens.size(); ++i) {
            if (tokens[i].text == "module" && tokens[i + 1].text == unit.name &&
                tokens[i + 1].line == unit.location.line) {
                module = i;
                break;
            }
        }
        if (module == tokens.size()) continue;
        auto moduleEnd = module + 2;
        while (moduleEnd < tokens.size() && tokens[moduleEnd].text != "endmodule") ++moduleEnd;

        struct ResetDriver {
            std::size_t targetToken{};
            std::optional<std::string> aliasSource;
        };
        std::map<std::string, std::vector<ResetDriver>> drivers;
        // The native visited tree deduplicates relation names, not driver AST
        // nodes. Every distinct non-alias assignment matched for a visited
        // name reaches diagnostic construction, even when two assignments
        // share one physical line. Deduplicate only the source assignment
        // token in case multiple alias paths converge on it.
        std::set<std::size_t> reported;
        const auto report = [&](std::size_t target) {
            const auto& token = tokens[target];
            if (!reported.insert(target).second) return;
            // Both constructors clear object+0x158, so FUN_180758410 takes
            // the mode-zero branch. FUN_1800d37b0 prepends the GB18030
            // literal at 0x180f10888 ("异步复位") to the normalized target
            // name, then FUN_1801380f0 appends 0x180f1ab50
            // ("用组合逻辑驱动"). DAT_180ee83b0 is "警告". The report copies
            // the assignment AST source and physical line, but no column.
            const auto message = "异步复位" + token.text + "用组合逻辑驱动";
            emit(result, "GJB10157.6.2.3", source, token.line, 1, message);
            emit(result, "GJB10157.6.2.7", source, token.line, 1, message);
        };

        for (auto index = module + 2; index < moduleEnd; ++index) {
            if (tokens[index].text == "assign") {
                auto end = index + 1;
                int round = 0, square = 0, brace = 0;
                for (; end < moduleEnd; ++end) {
                    const auto& text = tokens[end].text;
                    if (text == "(") ++round;
                    else if (text == ")") --round;
                    else if (text == "[") ++square;
                    else if (text == "]") --square;
                    else if (text == "{") ++brace;
                    else if (text == "}") --brace;
                    if (round == 0 && square == 0 && brace == 0 && text == ";") break;
                }
                std::size_t equal = end;
                round = square = brace = 0;
                for (auto cursor = index + 1; cursor < end; ++cursor) {
                    const auto& text = tokens[cursor].text;
                    if (text == "(") ++round;
                    else if (text == ")") --round;
                    else if (text == "[") ++square;
                    else if (text == "]") --square;
                    else if (text == "{") ++brace;
                    else if (text == "}") --brace;
                    else if (round == 0 && square == 0 && brace == 0 && text == "=") {
                        equal = cursor;
                        break;
                    }
                }
                const auto target = simpleAssignmentTarget(tokens, index + 1, equal);
                if (target && equal < end) {
                    drivers[tokens[*target].text].push_back(
                        {*target, nativeResetAliasSource(tokens, equal + 1, end)});
                }
                index = end == moduleEnd ? index : end;
                continue;
            }

            const auto& word = tokens[index].text;
            if (word != "always" && word != "always_comb" && word != "always_latch") continue;
            auto body = index + 1;
            bool sequential = false;
            if (word == "always") {
                if (body >= moduleEnd || tokens[body].text != "@") continue;
                ++body;
                if (body < moduleEnd && tokens[body].text == "*") {
                    ++body;
                } else if (body < moduleEnd && tokens[body].text == "(") {
                    const auto close = closeParen(tokens, body);
                    if (close >= moduleEnd) continue;
                    for (auto event = body + 1; event < close; ++event)
                        sequential |= tokens[event].text == "posedge" ||
                                      tokens[event].text == "negedge";
                    body = close + 1;
                } else continue;
            }
            const auto end = statementEnd(tokens, body);
            if (end > moduleEnd) continue;
            if (!sequential) {
                auto segment = body;
                for (auto cursor = body; cursor < end; ++cursor) {
                    if (tokens[cursor].text == ";") {
                        segment = cursor + 1;
                        continue;
                    }
                    if (tokens[cursor].text != "=" && tokens[cursor].text != "<=") continue;
                    if (const auto target = simpleAssignmentTarget(tokens, segment, cursor)) {
                        // The native alias exemption is confined to AST_ASSIGN
                        // continuous assignments.  AST_ASSIGN_EQ/LE collected
                        // below an edge-free AST_ALWAYS remains reportable.
                        drivers[tokens[*target].text].push_back(
                            {*target, std::nullopt});
                    }
                }
            }
            index = end == 0 ? index : end - 1;
        }

        // After classifying one relation-name work set, FUN_180758410 clears
        // that set (FUN_18075a210), filters newly collected alias sources
        // against a visited-name tree (FUN_180676aa0), inserts unseen names
        // with FUN_180579cf0, and repeats while the work set is non-empty.
        // This is a transitive, cycle-safe alias walk rather than a one-level
        // suppression.  Report at the first non-alias driver(s) encountered
        // along every reachable source branch.
        std::vector<std::string> pending(asynchronousResets.begin(), asynchronousResets.end());
        std::set<std::string> visited;
        while (!pending.empty()) {
            auto signal = std::move(pending.back());
            pending.pop_back();
            if (!visited.insert(signal).second) continue;
            const auto found = drivers.find(signal);
            if (found == drivers.end()) continue;
            for (const auto& driver : found->second) {
                if (driver.aliasSource) pending.push_back(*driver.aliasSource);
                else report(driver.targetToken);
            }
        }
    }
}

void checkMixedClockEdges(AnalysisResult& result, const core::SourceFile& source,
                          const std::vector<parser::HdlToken>& tokens) {
    // Constructor 0x180022270 installs vtable 0x180ef0a40. Native
    // FUN_18054f6c0 builds a synthesized connectivity model; in
    // FUN_180552630 each admitted related clock endpoint contributes one of two
    // edge bits. When the mask reaches 3, the immediately retained endpoint
    // and the conflicting current endpoint each emit one Warning, then that
    // component stops. Exact text is
    // "时钟<name>不能同时使用上升沿和下降沿" and locations carry physical
    // lines but no columns. The packaged third case proves that a simple
    // inverter is traversed and flips the effective edge. This bounded source
    // adapter follows direct/inverted aliases and emits the first
    // source-representable endpoint of each effective polarity.
    // ev_f71f67d1bdf1d3f4bb0d6437bf70ea0394d40debcbb7e9945b8639a38cb929d2
    for (const auto& unit : result.structure) {
        if (unit.kind != parser::DesignUnitKind::VerilogModule ||
            unit.location.file.lexically_normal() != source.path().lexically_normal()) continue;
        std::size_t module = tokens.size();
        for (std::size_t i = 0; i + 1 < tokens.size(); ++i) {
            if (tokens[i].text == "module" && tokens[i + 1].text == unit.name &&
                tokens[i + 1].line == unit.location.line) {
                module = i;
                break;
            }
        }
        if (module == tokens.size()) continue;
        auto moduleEnd = module + 2;
        while (moduleEnd < tokens.size() && tokens[moduleEnd].text != "endmodule") ++moduleEnd;

        struct Alias { std::string source; bool inverted; };
        std::map<std::string, Alias> aliases;
        for (auto index = module + 2; index + 4 < moduleEnd; ++index) {
            if (tokens[index].text != "assign" ||
                tokens[index + 1].kind != parser::HdlTokenKind::Identifier ||
                tokens[index + 2].text != "=") continue;
            auto rhs = index + 3;
            bool inverted = false;
            if (rhs < moduleEnd && tokens[rhs].text == "~") {
                inverted = true;
                ++rhs;
            }
            if (rhs + 1 >= moduleEnd ||
                tokens[rhs].kind != parser::HdlTokenKind::Identifier ||
                tokens[rhs + 1].text != ";") continue;
            aliases[tokens[index + 1].text] = {tokens[rhs].text, inverted};
            index = rhs + 1;
        }

        const auto canonical = [&](const std::string& signal, bool rising) {
            std::string root = signal;
            bool effectiveRising = rising;
            std::set<std::string> visited;
            while (visited.insert(root).second) {
                const auto alias = aliases.find(root);
                if (alias == aliases.end()) break;
                if (alias->second.inverted) effectiveRising = !effectiveRising;
                root = alias->second.source;
            }
            return std::pair{root, effectiveRising};
        };

        struct EdgeEndpoints {
            std::optional<core::SourceLocation> rising;
            std::optional<core::SourceLocation> falling;
        };
        std::map<std::string, EdgeEndpoints> endpoints;
        for (const auto& clock : unit.clocks) {
            const auto [root, rising] = canonical(clock.signal, clock.risingEdge);
            auto& family = endpoints[root];
            auto& endpoint = rising ? family.rising : family.falling;
            if (!endpoint) endpoint = clock.location;
        }
        for (const auto& [root, family] : endpoints) {
            if (!family.rising || !family.falling) continue;
            std::array locations{*family.rising, *family.falling};
            std::ranges::sort(locations, {}, [](const auto& location) {
                return std::tuple{location.line, location.column};
            });
            const auto message = "时钟" + root + "不能同时使用上升沿和下降沿";
            for (const auto& location : locations)
                emit(result, "GJB10157.6.1.4", source,
                     location.line, 1, message);
        }
    }
}

void checkGatedClocks(AnalysisResult& result, const core::SourceFile& source,
                      const std::vector<parser::HdlToken>& tokens) {
    // Constructor 0x180022300 registers GJB10157_VLOG.6.1.6, clears mode byte
    // +0x158 and installs vtable 0x180ef0b30. Slot 1 FUN_1807238a0 admits only
    // AST_MODULE(2). Main FUN_180726d40 gathers AST_ASSIGN(0x4d),
    // AST_ASSIGN_EQ(0x55) and AST_ASSIGN_LE(0x56), resolves their targets
    // against elaborated clock relations, and searches RHS descendants for
    // operator types {0x45,0x46,0x48,0x25,0x26,0x27,0x28}. These are the
    // binary logical, conditional and binary bitwise families used to
    // gate/select clocks; unary inversion is deliberately absent. Each
    // matching assignment emits independently at its physical line without a
    // column and mode zero selects exact text "避免使用时钟门控.". The bounded
    // source adapter therefore requires a parser-confirmed clock target and
    // one of those binary/conditional operators in its assignment expression.
    // ev_a2fc415c6f5559f25a99482ccaf2b2692e429fc9515d892fe65cb2ee06dde088
    // ev_aaa493aac258e6c8e9f4d04fcd2ebeb575f37e5d55e363a21c32a358a3f3965b
    const std::set<std::string_view> gatingOperators{
        "&&", "||", "?", "&", "|", "^", "^~", "~^"
    };
    for (const auto& unit : result.structure) {
        if (unit.kind != parser::DesignUnitKind::VerilogModule ||
            unit.location.file.lexically_normal() != source.path().lexically_normal()) continue;
        std::set<std::string> clocks;
        for (const auto& clock : unit.clocks) clocks.insert(clock.signal);
        if (clocks.empty()) continue;

        std::size_t module = tokens.size();
        for (std::size_t i = 0; i + 1 < tokens.size(); ++i) {
            if (tokens[i].text == "module" && tokens[i + 1].text == unit.name &&
                tokens[i + 1].line == unit.location.line) {
                module = i;
                break;
            }
        }
        if (module == tokens.size()) continue;
        auto moduleEnd = module + 2;
        while (moduleEnd < tokens.size() && tokens[moduleEnd].text != "endmodule") ++moduleEnd;

        std::set<std::size_t> reported;
        const auto reportAssignment = [&](std::size_t begin, std::size_t op,
                                          std::size_t rhsEnd) {
            const auto target = simpleAssignmentTarget(tokens, begin, op);
            if (!target || !clocks.contains(tokens[*target].text)) return;
            bool gated = false;
            for (auto rhs = op + 1; rhs < rhsEnd; ++rhs) {
                if (gatingOperators.contains(tokens[rhs].text)) {
                    gated = true;
                    break;
                }
            }
            const auto& token = tokens[*target];
            if (!gated || !reported.insert(begin).second) return;
            emit(result, "GJB10157.6.1.6", source, token.line, 1,
                 "避免使用时钟门控.");
        };

        auto segment = module + 2;
        int round = 0, square = 0, brace = 0;
        for (auto index = module + 2; index < moduleEnd; ++index) {
            const auto& text = tokens[index].text;
            if (text == "(") ++round;
            else if (text == ")") --round;
            else if (text == "[") ++square;
            else if (text == "]") --square;
            else if (text == "{") ++brace;
            else if (text == "}") --brace;
            if (text != ";" || round != 0 || square != 0 || brace != 0) continue;

            std::size_t op = index;
            int localRound = 0, localSquare = 0, localBrace = 0;
            for (auto cursor = segment; cursor < index; ++cursor) {
                const auto& candidate = tokens[cursor].text;
                if (candidate == "(") ++localRound;
                else if (candidate == ")") --localRound;
                else if (candidate == "[") ++localSquare;
                else if (candidate == "]") --localSquare;
                else if (candidate == "{") ++localBrace;
                else if (candidate == "}") --localBrace;
                else if (localRound == 0 && localSquare == 0 && localBrace == 0 &&
                         (candidate == "=" || candidate == "<=")) {
                    op = cursor;
                    break;
                }
            }
            if (op != index) reportAssignment(segment, op, index);
            segment = index + 1;
        }
    }
}

void checkMultipleClockDomains(AnalysisResult& result, const core::SourceFile& source,
                               const std::vector<parser::HdlToken>& tokens,
                               const AnalysisOptions& options) {
    // Constructor 0x180022420 registers GJB10157_VLOG.6.1.7, clears mode byte
    // +0x158 and installs vtable 0x180ef0ea0. Slot 1 FUN_1807238a0 admits only
    // AST_MODULE(2). Main FUN_180555380 requires the setTopModule name, builds,
    // sorts and deduplicates 32-byte clock-relation records, then emits one
    // Warning when the distinct count exceeds one. Mode zero selects exact
    // text "建议仅使用一个时钟域"; both direct and later relation-graph reports
    // use the module source/physical line and no column. Packaged controls
    // prove rising and falling uses of the same signal remain one domain. The
    // later graph path is not fully recovered; this bounded direct adapter
    // follows direct and unary-inverted aliases and reports only a definite
    // second parser-confirmed clock root.
    // ev_c3a2ade2a32b69e0a081e0c5e0212f2276c281d75f59577e7dab31ab28b77f3d
    // ev_aaa493aac258e6c8e9f4d04fcd2ebeb575f37e5d55e363a21c32a358a3f3965b
    for (const auto& unit : result.structure) {
        if (unit.kind != parser::DesignUnitKind::VerilogModule ||
            unit.location.file.lexically_normal() != source.path().lexically_normal()) continue;
        // FUN_180555380 compares the record's internal design name with the
        // single value stored by setTopModule before entering both the direct
        // clock-record and later relation-graph paths. Preserve the legacy
        // all-module source adapter only when no explicit native top exists.
        if (options.topModule && !selectedTop(unit, options)) continue;

        std::size_t module = tokens.size();
        for (std::size_t i = 0; i + 1 < tokens.size(); ++i) {
            if (tokens[i].text == "module" && tokens[i + 1].text == unit.name &&
                tokens[i + 1].line == unit.location.line) {
                module = i;
                break;
            }
        }
        if (module == tokens.size()) continue;
        auto moduleEnd = module + 2;
        while (moduleEnd < tokens.size() && tokens[moduleEnd].text != "endmodule") ++moduleEnd;

        std::map<std::string, std::string> aliases;
        for (auto index = module + 2; index + 4 < moduleEnd; ++index) {
            if (tokens[index].text != "assign" ||
                tokens[index + 1].kind != parser::HdlTokenKind::Identifier ||
                tokens[index + 2].text != "=") continue;
            auto rhs = index + 3;
            if (rhs < moduleEnd && tokens[rhs].text == "~") ++rhs;
            if (rhs + 1 >= moduleEnd ||
                tokens[rhs].kind != parser::HdlTokenKind::Identifier ||
                tokens[rhs + 1].text != ";") continue;
            aliases[tokens[index + 1].text] = tokens[rhs].text;
            index = rhs + 1;
        }

        const auto canonical = [&](const std::string& signal) {
            std::string root = signal;
            std::set<std::string> visited;
            while (visited.insert(root).second) {
                const auto alias = aliases.find(root);
                if (alias == aliases.end()) break;
                root = alias->second;
            }
            return root;
        };

        std::set<std::string> domains;
        for (const auto& clock : unit.clocks) {
            const auto domain = canonical(clock.signal);
            if (!domains.insert(domain).second || domains.size() == 1) continue;
            emit(result, "GJB10157.6.1.7", source,
                 unit.location.line, 1, "建议仅使用一个时钟域");
            break;
        }
    }
}

void checkMixedAsynchronousSetReset(AnalysisResult& result,
                                    const core::SourceFile& source,
                                    const std::vector<parser::HdlToken>& tokens) {
    // Native FUN_18055c5a0 associates reset-relation records with AST_ALWAYS
    // roots. Helper FUN_18055bdb0 first requires more than one reset-condition
    // identifier, then collects direct constant assignment values below the
    // reset branches. The run emits only when that vector contains both 0 and
    // 1. A constant integer greater than one contributes both values. The
    // native report is fixed lowercase error, uses the qualifying AST_CASE
    // physical line with no column, and has the exact text
    // "always中同时使用了异步复位信号和异步置位信号".
    // ev_e5a7198b83488ab2b8c8e7ad94ad24067db34cd14cf9acdb3a38fff2dd192715
    const auto constantClass = [](std::string text) -> std::optional<unsigned> {
        text = lower(std::move(text));
        text.erase(std::remove(text.begin(), text.end(), '_'), text.end());
        if (text == "'0") return 0U;
        if (text == "'1") return 1U;
        auto digits = std::string_view{text};
        unsigned base = 10;
        if (const auto quote = text.find('\''); quote != std::string::npos) {
            auto at = quote + 1;
            if (at < text.size() && text[at] == 's') ++at;
            if (at >= text.size()) return std::nullopt;
            switch (text[at++]) {
            case 'b': base = 2; break;
            case 'o': base = 8; break;
            case 'd': base = 10; break;
            case 'h': base = 16; break;
            default: return std::nullopt;
            }
            digits = std::string_view{text}.substr(at);
        }
        if (digits.empty()) return std::nullopt;
        unsigned value = 0;
        for (const char digit : digits) {
            unsigned decoded = 0;
            if (digit >= '0' && digit <= '9') decoded = static_cast<unsigned>(digit - '0');
            else if (digit >= 'a' && digit <= 'f')
                decoded = 10U + static_cast<unsigned>(digit - 'a');
            else return std::nullopt;
            if (decoded >= base) return std::nullopt;
            if (value > 1U || decoded > 1U) return 2U;
            value = value * base + decoded;
            if (value > 1U) return 2U;
        }
        return value;
    };

    for (std::size_t index = 0; index + 2 < tokens.size(); ++index) {
        if ((tokens[index].text != "always" && tokens[index].text != "always_ff") ||
            tokens[index + 1].text != "@" ||
            tokens[index + 2].text != "(") continue;
        const auto sensitivityEnd = closeParen(tokens, index + 2);
        if (sensitivityEnd == tokens.size()) continue;
        std::set<std::string> events;
        for (auto at = index + 3; at + 1 < sensitivityEnd; ++at) {
            if ((tokens[at].text == "posedge" || tokens[at].text == "negedge") &&
                tokens[at + 1].kind == parser::HdlTokenKind::Identifier)
                events.insert(tokens[at + 1].text);
        }
        if (events.size() < 3 || sensitivityEnd + 1 >= tokens.size()) continue;
        const auto bodyBegin = sensitivityEnd + 1;
        const auto bodyEnd = statementEnd(tokens, bodyBegin);
        if (bodyEnd <= bodyBegin || bodyEnd > tokens.size()) continue;

        bool reported = false;
        for (auto root = bodyBegin; root < bodyEnd && !reported; ++root) {
            if (tokens[root].text != "if" || root + 1 >= bodyEnd ||
                tokens[root + 1].text != "(") continue;
            const auto treeEnd = statementEnd(tokens, root);
            if (treeEnd <= root || treeEnd > bodyEnd) continue;

            std::size_t matchingEventOccurrences = 0;
            std::set<std::string> conditionEvents;
            std::vector<std::pair<std::size_t, std::size_t>> conditions;
            for (auto nested = root; nested < treeEnd; ++nested) {
                if (tokens[nested].text != "if" || nested + 1 >= treeEnd ||
                    tokens[nested + 1].text != "(") continue;
                const auto conditionEnd = closeParen(tokens, nested + 1);
                if (conditionEnd >= treeEnd) break;
                conditions.emplace_back(nested + 1, conditionEnd);
                for (auto condition = nested + 2; condition < conditionEnd; ++condition) {
                    if (tokens[condition].kind != parser::HdlTokenKind::Identifier ||
                        !events.contains(tokens[condition].text)) continue;
                    ++matchingEventOccurrences;
                    conditionEvents.insert(tokens[condition].text);
                }
                nested = conditionEnd;
            }
            if (matchingEventOccurrences <= 1 || conditionEvents.size() >= events.size()) {
                root = treeEnd - 1;
                continue;
            }

            bool hasZero = false;
            bool hasOne = false;
            const auto inCondition = [&](std::size_t token) {
                return std::ranges::any_of(conditions, [&](const auto& range) {
                    return range.first <= token && token <= range.second;
                });
            };
            for (auto token = root; token + 2 < treeEnd; ++token) {
                if ((tokens[token].text != "=" && tokens[token].text != "<=") ||
                    inCondition(token) || tokens[token + 1].kind != parser::HdlTokenKind::Number ||
                    tokens[token + 2].text != ";") continue;
                const auto value = constantClass(tokens[token + 1].text);
                if (!value) continue;
                hasZero = hasZero || *value == 0U || *value > 1U;
                hasOne = hasOne || *value == 1U || *value > 1U;
            }
            if (hasZero && hasOne) {
                emit(result, "GJB10157.6.2.9", source,
                     tokens[root].line, 1,
                     "always中同时使用了异步复位信号和异步置位信号");
                reported = true;
            }
            root = treeEnd - 1;
        }
        index = bodyEnd - 1;
    }
}

void checkAlwaysIfElseNative(AnalysisResult& result, const core::SourceFile& source,
                             const std::vector<parser::HdlToken>& tokens) {
    // Native slot 0 processes admitted AST_ALWAYS(0x52) roots. Virtual slot 2
    // delegates admission to FUN_1804e66e0 and separately admits AST_TASK(3)
    // and AST_FUNCTION(4). FUN_1804e66e0 accepts an AST_ALWAYS exactly when
    // none of its immediate children is AST_POSEDGE/AST_NEGEDGE(0x65/0x66);
    // empty and edge-free roots pass. The embedded collector at 0x1805813e0
    // admits only nonempty
    // AST_CASE(0x57) nodes whose byte +0x183 is exactly one: the same frontend
    // marker that identifies source if/else lowering. A gathered node reports
    // when none of its direct AST_COND children starts with AST_DEFAULT.
    // ev_bb7fc67da6a764411eeddbfe03fc0357342ce0db1d93094802847cb0a8f8e33e
    // The run also resolves direct AST_FCALL/AST_TCALL descendants by name to
    // AST_FUNCTION(4)/AST_TASK(3) definitions and scans each resolved body once.
    struct SubroutineRange {
        std::size_t begin{};
        std::size_t end{};
    };
    std::map<std::string, SubroutineRange> subroutines;
    for (std::size_t index = 0; index < tokens.size(); ++index) {
        const auto& keyword = tokens[index].text;
        if (keyword != "task" && keyword != "function") continue;
        const auto terminator = keyword == "task" ? "endtask" : "endfunction";
        auto finish = index + 1;
        while (finish < tokens.size() && tokens[finish].text != terminator) ++finish;
        if (finish == tokens.size()) continue;
        auto headerEnd = index + 1;
        while (headerEnd < finish && tokens[headerEnd].text != ";") ++headerEnd;
        if (headerEnd == finish) {
            index = finish;
            continue;
        }
        auto nameLimit = index + 1;
        while (nameLimit < headerEnd && tokens[nameLimit].text != "(") ++nameLimit;
        std::size_t nameIndex = tokens.size();
        for (auto candidate = index + 1; candidate < nameLimit; ++candidate) {
            if (tokens[candidate].kind == parser::HdlTokenKind::Identifier)
                nameIndex = candidate;
        }
        if (nameIndex != tokens.size())
            subroutines.emplace(tokens[nameIndex].text,
                                SubroutineRange{headerEnd + 1, finish});
        index = finish;
    }

    const auto checkIfRange = [&](std::size_t begin, std::size_t end) {
        for (auto index = begin; index < end; ++index) {
            if (tokens[index].text != "if" || index + 1 >= end ||
                tokens[index + 1].text != "(") continue;
            const auto close = closeParen(tokens, index + 1);
            if (close >= end || close + 1 >= end) continue;
            const auto thenEnd = statementEnd(tokens, close + 1);
            if (thenEnd > end) continue;
            if (thenEnd == end || tokens[thenEnd].text != "else")
                emit(result, "GJB10157.9.2.11", source,
                     tokens[index].line, tokens[index].column,
                     "always 过程中的 if 分支缺少 else 分支");
        }
    };

    std::set<std::string> expandedSubroutines;
    for (std::size_t index = 0; index < tokens.size(); ++index) {
        const auto& word = tokens[index].text;
        if (word != "always" && word != "always_comb" &&
            word != "always_latch" && word != "always_ff") continue;
        bool combinational = true;
        auto body = index + 1;
        if (body < tokens.size() && tokens[body].text == "@") {
            ++body;
            if (body >= tokens.size()) continue;
            if (tokens[body].text == "*") {
                ++body;
            } else if (tokens[body].text == "(") {
                const auto close = closeParen(tokens, body);
                if (close == tokens.size()) continue;
                for (auto event = body + 1; event < close; ++event) {
                    if (tokens[event].text == "posedge" ||
                        tokens[event].text == "negedge") {
                        combinational = false;
                        break;
                    }
                }
                body = close + 1;
            } else if (tokens[body].kind == parser::HdlTokenKind::Identifier) {
                // A named event is an immediate AST_EDGE rather than one of
                // the two rejected edge types, so advance to its statement.
                ++body;
            }
        }
        if (!combinational || body >= tokens.size()) continue;
        const auto end = statementEnd(tokens, body);
        if (end <= body || end > tokens.size()) continue;
        checkIfRange(body, end);

        // The native run discovers calls from the AST_ALWAYS subtree first,
        // then scans the directly referenced definitions. It does not repeat
        // call discovery inside those definitions, so this adapter deliberately
        // performs one expansion level and de-duplicates each named definition.
        for (auto call = body; call < end; ++call) {
            const auto found = subroutines.find(tokens[call].text);
            if (found == subroutines.end() || call + 1 >= end) continue;
            if (tokens[call + 1].text != "(" && tokens[call + 1].text != ";") continue;
            if (!expandedSubroutines.insert(found->first).second) continue;
            checkIfRange(found->second.begin, found->second.end);
        }
        index = end - 1;
    }
}

void checkDefinitelyEmptyVerilogFor(AnalysisResult& result, const core::SourceFile& source,
                                    const std::vector<parser::HdlToken>& tokens) {
    // The frontend producer at 0x18091b7d0 clones and repeatedly simplifies
    // the FOR condition, then attaches is_count_zero exactly when the first
    // simplified AST_CONSTANT has integer field +0xbc equal to zero. The
    // iteration counter is still one, proving this is the first-test value.
    // This bounded adapter evaluates the complete first-test expression when
    // every operand is provably integral: integer literals, the initialized
    // loop counter, resolved parameter/localparam defaults and object-like
    // numeric macros. Unknown values and unsafe arithmetic remain unreported.
    // ev_e4e2ceb57f9c07a004df64df165d6990e0436f66847b89c973f3ef656e9d8565
    const auto literal = [](std::string value) -> std::optional<std::int64_t> {
        value.erase(std::remove(value.begin(), value.end(), '_'), value.end());
        if (value.empty()) return std::nullopt;
        int base = 10;
        auto digits = value;
        if (const auto quote = value.find('\''); quote != std::string::npos) {
            auto marker = quote + 1;
            if (marker < value.size() && (value[marker] == 's' || value[marker] == 'S')) ++marker;
            if (marker >= value.size()) return std::nullopt;
            switch (static_cast<char>(std::tolower(static_cast<unsigned char>(value[marker])))) {
            case 'b': base = 2; break;
            case 'o': base = 8; break;
            case 'd': base = 10; break;
            case 'h': base = 16; break;
            default: return std::nullopt;
            }
            digits = value.substr(marker + 1);
        }
        if (digits.empty() || digits.find_first_of("xXzZ?") != std::string::npos)
            return std::nullopt;
        bool negative = false;
        if (digits.front() == '+' || digits.front() == '-') {
            negative = digits.front() == '-';
            digits.erase(digits.begin());
        }
        if (digits.empty()) return std::nullopt;
        std::uint64_t parsed = 0;
        const auto [end, error] = std::from_chars(
            digits.data(), digits.data() + digits.size(), parsed, base);
        if (error != std::errc{} || end != digits.data() + digits.size() ||
            parsed > static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max()))
            return std::nullopt;
        return negative ? -static_cast<std::int64_t>(parsed) : static_cast<std::int64_t>(parsed);
    };

    std::map<std::string, std::int64_t> constants;
    // The conditional preprocessor intentionally blanks directive lines, so
    // recover only single-line, object-like numeric defines from the original
    // source. Function-like and expression macros remain unproved.
    std::istringstream sourceLines(source.content());
    for (std::string line; std::getline(sourceLines, line);) {
        const auto first = line.find_first_not_of(" \t\r");
        if (first == std::string::npos || line.compare(first, 7, "`define") != 0) continue;
        std::istringstream definition(line.substr(first + 7));
        std::string name, value, extra;
        if ((definition >> name >> value) && !(definition >> extra))
            if (const auto parsed = literal(value)) constants[name] = *parsed;
    }
    const auto checkedBinary = [](std::string_view op, std::int64_t left,
                                  std::int64_t right) -> std::optional<std::int64_t> {
        const auto checkedAdd = [](std::int64_t lhs, std::int64_t rhs)
                -> std::optional<std::int64_t> {
            if ((rhs > 0 && lhs > std::numeric_limits<std::int64_t>::max() - rhs) ||
                (rhs < 0 && lhs < std::numeric_limits<std::int64_t>::min() - rhs)) {
                return std::nullopt;
            }
            return lhs + rhs;
        };
        const auto checkedSubtract = [](std::int64_t lhs, std::int64_t rhs)
                -> std::optional<std::int64_t> {
            if ((rhs < 0 && lhs > std::numeric_limits<std::int64_t>::max() + rhs) ||
                (rhs > 0 && lhs < std::numeric_limits<std::int64_t>::min() + rhs)) {
                return std::nullopt;
            }
            return lhs - rhs;
        };
        const auto checkedMultiply = [](std::int64_t lhs, std::int64_t rhs)
                -> std::optional<std::int64_t> {
            const auto magnitude = [](std::int64_t value) -> std::uint64_t {
                return value >= 0 ? static_cast<std::uint64_t>(value)
                                  : static_cast<std::uint64_t>(-(value + 1)) + 1;
            };
            const bool negative = (lhs < 0) != (rhs < 0);
            const auto lhsMagnitude = magnitude(lhs);
            const auto rhsMagnitude = magnitude(rhs);
            const auto maximumMagnitude = negative
                ? static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max()) + 1
                : static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max());
            if (lhsMagnitude != 0 && rhsMagnitude > maximumMagnitude / lhsMagnitude) {
                return std::nullopt;
            }
            const auto product = lhsMagnitude * rhsMagnitude;
            if (!negative) return static_cast<std::int64_t>(product);
            if (product == maximumMagnitude) return std::numeric_limits<std::int64_t>::min();
            return -static_cast<std::int64_t>(product);
        };
        if (op == "+") return checkedAdd(left, right);
        if (op == "-") return checkedSubtract(left, right);
        if (op == "*") return checkedMultiply(left, right);
        if (op == "/") {
            if (right == 0 || (left == std::numeric_limits<std::int64_t>::min() && right == -1))
                return std::nullopt;
            return left / right;
        }
        if (op == "%") {
            if (right == 0 || (left == std::numeric_limits<std::int64_t>::min() && right == -1))
                return std::nullopt;
            return left % right;
        }
        if (op == "<<") {
            if (right < 0 || right > 62 || left < 0) return std::nullopt;
            if (left > (std::numeric_limits<std::int64_t>::max() >> right)) {
                return std::nullopt;
            }
            return left << right;
        }
        if (op == ">>") {
            if (right < 0 || right > 62 || left < 0) return std::nullopt;
            return left >> right;
        }
        if (op == "<") return left < right;
        if (op == "<=") return left <= right;
        if (op == ">") return left > right;
        if (op == ">=") return left >= right;
        if (op == "==" || op == "===") return left == right;
        if (op == "!=" || op == "!==") return left != right;
        if (op == "&") return left & right;
        if (op == "^") return left ^ right;
        if (op == "~^") return ~(left ^ right);
        if (op == "|") return left | right;
        if (op == "&&") return left != 0 && right != 0;
        if (op == "||") return left != 0 || right != 0;
        return std::nullopt;
    };
    const auto precedence = [](std::string_view op) {
        if (op == "||") return 1;
        if (op == "&&") return 2;
        if (op == "|") return 3;
        if (op == "^" || op == "~^") return 4;
        if (op == "&") return 5;
        if (op == "==" || op == "!=" || op == "===" || op == "!==") return 6;
        if (op == "<" || op == "<=" || op == ">" || op == ">=") return 7;
        if (op == "<<" || op == ">>") return 8;
        if (op == "+" || op == "-") return 9;
        if (op == "*" || op == "/" || op == "%") return 10;
        return 0;
    };
    std::function<std::optional<std::int64_t>(std::size_t, std::size_t,
                                               const std::string&,
                                               std::optional<std::int64_t>)> evaluate;
    evaluate = [&](std::size_t begin, std::size_t end, const std::string& counter,
                   std::optional<std::int64_t> counterValue) -> std::optional<std::int64_t> {
        std::function<std::optional<std::int64_t>(std::size_t&, int)> expression;
        const auto primary = [&](auto&& self, std::size_t& position)
            -> std::optional<std::int64_t> {
            if (position >= end) return std::nullopt;
            const auto op = tokens[position].text;
            if (op == "+" || op == "-" || op == "!" || op == "~") {
                ++position;
                auto value = self(self, position);
                if (!value) return std::nullopt;
                if (op == "+") return value;
                if (op == "!") return *value == 0;
                if (op == "~") return ~*value;
                if (*value == std::numeric_limits<std::int64_t>::min()) return std::nullopt;
                return -*value;
            }
            if (op == "(") {
                ++position;
                auto value = expression(position, 1);
                if (!value || position >= end || tokens[position].text != ")")
                    return std::nullopt;
                ++position;
                return value;
            }
            if (tokens[position].kind == parser::HdlTokenKind::Number) {
                ++position;
                return literal(op);
            }
            if (tokens[position].kind == parser::HdlTokenKind::Identifier) {
                ++position;
                if (op == counter && counterValue) return counterValue;
                if (const auto found = constants.find(op); found != constants.end())
                    return found->second;
                return std::nullopt;
            }
            if (op == "`" && position + 1 < end &&
                tokens[position + 1].kind == parser::HdlTokenKind::Identifier) {
                const auto name = tokens[position + 1].text;
                position += 2;
                if (const auto found = constants.find(name); found != constants.end())
                    return found->second;
            }
            return std::nullopt;
        };
        expression = [&](std::size_t& position, int minimum)
            -> std::optional<std::int64_t> {
            auto left = primary(primary, position);
            if (!left) return std::nullopt;
            while (position < end) {
                const auto op = tokens[position].text;
                const auto level = precedence(op);
                if (level < minimum) break;
                ++position;
                auto right = expression(position, level + 1);
                if (!right) return std::nullopt;
                left = checkedBinary(op, *left, *right);
                if (!left) return std::nullopt;
            }
            return left;
        };
        auto position = begin;
        auto value = expression(position, 1);
        return value && position == end ? value : std::nullopt;
    };
    // Resolve direct numeric parameter defaults in source order. This matches
    // the recovered parser-time simplification used by the packaged rule
    // example; expression/default cycles and instance overrides stay unknown.
    for (std::size_t i = 0; i < tokens.size(); ++i) {
        if (tokens[i].text != "parameter" && tokens[i].text != "localparam") continue;
        auto segment = i + 1;
        int brackets = 0;
        for (auto j = segment; j <= tokens.size(); ++j) {
            const bool atEnd = j == tokens.size();
            const auto text = atEnd ? std::string{} : tokens[j].text;
            const bool boundary = atEnd || (brackets == 0 &&
                (text == "," || text == ";" || text == ")"));
            if (boundary) {
                std::size_t equals = j, name = j;
                for (auto k = segment; k < j; ++k) {
                    if (tokens[k].text == "=") { equals = k; break; }
                    if (tokens[k].kind == parser::HdlTokenKind::Identifier &&
                        tokens[k].text != "signed" && tokens[k].text != "unsigned" &&
                        tokens[k].text != "integer" && tokens[k].text != "int" &&
                        tokens[k].text != "logic" && tokens[k].text != "reg" &&
                        tokens[k].text != "wire") name = k;
                }
                if (name < equals)
                    if (const auto value = evaluate(equals + 1, j, {}, std::nullopt))
                        constants[tokens[name].text] = *value;
                if (atEnd || text == ";" || text == ")") { i = j; break; }
                segment = j + 1;
            }
            if (!atEnd) {
                if (text == "[") ++brackets;
                else if (text == "]") --brackets;
            }
        }
    }
    for (std::size_t i = 0; i + 1 < tokens.size(); ++i) {
        if (tokens[i].text != "for" || tokens[i + 1].text != "(") continue;
        const auto close = closeParen(tokens, i + 1);
        if (close == tokens.size()) continue;
        std::array<std::size_t, 2> separators{tokens.size(), tokens.size()};
        std::size_t count = 0;
        int depth = 1;
        for (auto j = i + 2; j < close; ++j) {
            if (tokens[j].text == "(") ++depth;
            else if (tokens[j].text == ")") --depth;
            else if (tokens[j].text == ";" && depth == 1) {
                if (count < separators.size()) separators[count] = j;
                ++count;
            }
        }
        if (count != 2) continue;
        std::size_t initEquals = separators[0];
        for (auto j = i + 2; j < separators[0]; ++j)
            if (tokens[j].text == "=") { initEquals = j; break; }
        if (initEquals == separators[0] || initEquals == i + 2 ||
            tokens[initEquals - 1].kind != parser::HdlTokenKind::Identifier) continue;
        const auto counter = tokens[initEquals - 1].text;
        const auto initial = evaluate(initEquals + 1, separators[0], {}, std::nullopt);
        if (!initial) continue;
        const auto firstTest = evaluate(separators[0] + 1, separators[1], counter, initial);
        if (firstTest && *firstTest == 0)
            emit(result, "GJB10157.9.3.4", source, tokens[i].line, tokens[i].column,
                 "for 循环初始值不满足首次循环条件，循环体不会执行");
        i = close;
    }
}

void checkForBodyRuntimeArithmetic(AnalysisResult& result, const core::SourceFile& source,
                                   const std::vector<parser::HdlToken>& tokens) {
    // Native 9.3.2 admits AST_FOR(0x5c) with four children and an AST_BLOCK
    // body, gathers AST_ADD/SUB/MUL/DIV/MOD/POW descendants below body CASE
    // nodes, then gathers every AST_IDENTIFIER descendant of those arithmetic
    // nodes. FUN_180580440 is an ordered set keyed by the identifier AST
    // pointer: a source occurrence reached through both a parent and a nested
    // arithmetic node reports once, while two distinct occurrences of the
    // same name report separately. The loop-counter name and direct module
    // parameter/localparam names are excluded; diagnostics use the identifier
    // node's source line, not the operator line.
    // ev_f2d4ff2f81f3ca6bb409fa6cff035938526ab406ed22aac5eff28d013db70179
    // ev_512c915b90a165536016fafbb5a5ded6cfba13c0084ad97b3576b7906ee5547b
    const VerilogConstantExpressions declarations(tokens);

    struct OperatorInfo { int precedence; bool arithmetic; bool rightAssociative; };
    const auto operatorInfo = [](std::string_view text) -> std::optional<OperatorInfo> {
        if (text == "=") return OperatorInfo{0, false, false};
        if (text == "||") return OperatorInfo{1, false, false};
        if (text == "&&") return OperatorInfo{2, false, false};
        if (text == "|") return OperatorInfo{3, false, false};
        if (text == "^" || text == "^~" || text == "~^")
            return OperatorInfo{4, false, false};
        if (text == "&") return OperatorInfo{5, false, false};
        if (text == "==" || text == "!=" || text == "===" || text == "!==")
            return OperatorInfo{6, false, false};
        if (text == "<" || text == "<=" || text == ">" || text == ">=")
            return OperatorInfo{7, false, false};
        if (text == "<<" || text == ">>" || text == "<<<" || text == ">>>")
            return OperatorInfo{8, false, false};
        if (text == "+" || text == "-") return OperatorInfo{9, true, false};
        if (text == "*" || text == "/" || text == "%")
            return OperatorInfo{10, true, false};
        if (text == "**") return OperatorInfo{11, true, true};
        return std::nullopt;
    };
    const auto canEndOperand = [&](std::size_t index) {
        if (index >= tokens.size()) return false;
        return tokens[index].kind == parser::HdlTokenKind::Identifier ||
            tokens[index].kind == parser::HdlTokenKind::Number ||
            tokens[index].kind == parser::HdlTokenKind::String ||
            tokens[index].text == ")" || tokens[index].text == "]" ||
            tokens[index].text == "}";
    };

    for (std::size_t i = 0; i + 1 < tokens.size(); ++i) {
        if (tokens[i].text != "for" || tokens[i + 1].text != "(") continue;
        const auto close = closeParen(tokens, i + 1);
        if (close == tokens.size() || close + 1 >= tokens.size()) continue;
        auto firstSemicolon = i + 2;
        while (firstSemicolon < close && tokens[firstSemicolon].text != ";")
            ++firstSemicolon;
        if (firstSemicolon >= close) continue;
        std::size_t initEquals = firstSemicolon;
        for (auto cursor = i + 2; cursor < firstSemicolon; ++cursor)
            if (tokens[cursor].text == "=") { initEquals = cursor; break; }
        if (initEquals == firstSemicolon) continue;
        std::size_t counterToken = tokens.size();
        for (auto cursor = i + 2; cursor < initEquals; ++cursor)
            if (tokens[cursor].kind == parser::HdlTokenKind::Identifier &&
                tokens[cursor].text != "genvar" && tokens[cursor].text != "integer" &&
                tokens[cursor].text != "int" && tokens[cursor].text != "logic" &&
                tokens[cursor].text != "reg" && tokens[cursor].text != "bit" &&
                tokens[cursor].text != "signed" && tokens[cursor].text != "unsigned")
                counterToken = cursor;
        if (counterToken == tokens.size()) continue;
        const auto& counter = tokens[counterToken].text;
        const auto end = statementEnd(tokens, close + 1);
        if (end <= close + 1 || end > tokens.size()) continue;

        std::vector<bool> inNestedHeader(tokens.size(), false);
        for (auto cursor = close + 1; cursor + 1 < end; ++cursor) {
            if (tokens[cursor].text != "for" || tokens[cursor + 1].text != "(") continue;
            const auto nestedClose = closeParen(tokens, cursor + 1);
            if (nestedClose >= end) continue;
            for (auto header = cursor; header <= nestedClose; ++header)
                inNestedHeader[header] = true;
            cursor = nestedClose;
        }

        std::set<std::size_t> reportedIdentifiers;
        const auto collectExpression = [&](auto&& self, std::size_t begin,
                                           std::size_t expressionEnd) -> void {
            while (begin < expressionEnd &&
                   (tokens[begin].text == ";" || tokens[begin].text == "begin" ||
                    tokens[begin].text == "end")) ++begin;
            while (begin < expressionEnd && tokens[begin].text == "(" &&
                   closeParen(tokens, begin) + 1 == expressionEnd) {
                ++begin;
                --expressionEnd;
            }
            if (begin >= expressionEnd) return;

            int round = 0, square = 0, brace = 0;
            std::size_t segment = begin;
            bool split = false;
            for (auto cursor = begin; cursor < expressionEnd; ++cursor) {
                if (inNestedHeader[cursor]) continue;
                const auto& text = tokens[cursor].text;
                if (text == "(") ++round;
                else if (text == ")" && round > 0) --round;
                else if (text == "[") ++square;
                else if (text == "]" && square > 0) --square;
                else if (text == "{") ++brace;
                else if (text == "}" && brace > 0) --brace;
                else if (round == 0 && square == 0 && brace == 0 && text == ",") {
                    self(self, segment, cursor);
                    segment = cursor + 1;
                    split = true;
                }
            }
            if (split) {
                self(self, segment, expressionEnd);
                return;
            }

            std::optional<std::size_t> question;
            std::optional<std::size_t> colon;
            round = square = brace = 0;
            int ternaryDepth = 0;
            for (auto cursor = begin; cursor < expressionEnd; ++cursor) {
                if (inNestedHeader[cursor]) continue;
                const auto& text = tokens[cursor].text;
                if (text == "(") ++round;
                else if (text == ")" && round > 0) --round;
                else if (text == "[") ++square;
                else if (text == "]" && square > 0) --square;
                else if (text == "{") ++brace;
                else if (text == "}" && brace > 0) --brace;
                else if (round == 0 && square == 0 && brace == 0 && text == "?") {
                    if (ternaryDepth++ == 0) question = cursor;
                } else if (round == 0 && square == 0 && brace == 0 && text == ":" &&
                           ternaryDepth > 0 && --ternaryDepth == 0) {
                    colon = cursor;
                    break;
                }
            }
            if (question && colon) {
                self(self, begin, *question);
                self(self, *question + 1, *colon);
                self(self, *colon + 1, expressionEnd);
                return;
            }

            std::optional<std::size_t> root;
            std::optional<OperatorInfo> rootInfo;
            round = square = brace = 0;
            for (auto cursor = begin; cursor < expressionEnd; ++cursor) {
                if (inNestedHeader[cursor]) continue;
                const auto& text = tokens[cursor].text;
                if (text == "(") { ++round; continue; }
                if (text == ")") { if (round > 0) --round; continue; }
                if (text == "[") { ++square; continue; }
                if (text == "]") { if (square > 0) --square; continue; }
                if (text == "{") { ++brace; continue; }
                if (text == "}") { if (brace > 0) --brace; continue; }
                if (round != 0 || square != 0 || brace != 0) continue;
                const auto info = operatorInfo(text);
                if (!info || cursor == begin || cursor + 1 >= expressionEnd ||
                    ((text == "+" || text == "-") && !canEndOperand(cursor - 1))) continue;
                if (!rootInfo || info->precedence < rootInfo->precedence ||
                    (info->precedence == rootInfo->precedence && !info->rightAssociative)) {
                    root = cursor;
                    rootInfo = info;
                }
            }
            if (root && rootInfo) {
                if (rootInfo->arithmetic) {
                    for (auto cursor = begin; cursor < expressionEnd; ++cursor) {
                        if (inNestedHeader[cursor] || tokens[cursor].text == counter ||
                            !declarations.isDeclaredRuntimeIdentifier(cursor)) continue;
                        // AST_FCALL/AST_TCALL names are not AST_IDENTIFIER
                        // descendants; their argument identifiers are.
                        if (cursor + 1 < expressionEnd && tokens[cursor + 1].text == "(") continue;
                        reportedIdentifiers.insert(cursor);
                    }
                } else {
                    self(self, begin, *root);
                    self(self, *root + 1, expressionEnd);
                }
                return;
            }

            for (auto cursor = begin; cursor < expressionEnd; ++cursor) {
                if (inNestedHeader[cursor] ||
                    (tokens[cursor].text != "(" && tokens[cursor].text != "[" &&
                     tokens[cursor].text != "{")) continue;
                std::size_t nestedEnd = expressionEnd;
                if (tokens[cursor].text == "(") nestedEnd = closeParen(tokens, cursor);
                else {
                    const auto opener = tokens[cursor].text;
                    const auto closer = opener == "[" ? "]" : "}";
                    int depth = 1;
                    for (auto at = cursor + 1; at < expressionEnd; ++at) {
                        if (tokens[at].text == opener) ++depth;
                        else if (tokens[at].text == closer && --depth == 0) {
                            nestedEnd = at;
                            break;
                        }
                    }
                }
                if (nestedEnd >= expressionEnd) continue;
                self(self, cursor + 1, nestedEnd);
                cursor = nestedEnd;
            }
        };

        std::size_t segment = close + 1;
        for (auto cursor = segment; cursor <= end; ++cursor) {
            const bool boundary = cursor == end ||
                (cursor < end && tokens[cursor].text == ";" && !inNestedHeader[cursor]);
            if (!boundary) continue;
            collectExpression(collectExpression, segment, cursor);
            segment = cursor + 1;
        }
        for (const auto identifier : reportedIdentifiers)
            emit(result, "GJB10157.9.3.2", source,
                 tokens[identifier].line, tokens[identifier].column,
                 "for 循环体中的算术运算涉及循环变量 " + counter +
                     " 以外的运行时变量 " + tokens[identifier].text);
        i = end - 1;
    }
}

void checkForHeaderConsistency(AnalysisResult& result, const core::SourceFile& source,
                               const std::vector<parser::HdlToken>& tokens) {
    // Native 9.3.1 compares the bound objects on the left of the first and
    // third AST_ASSIGN_EQ children, then reports stored is_initVal_const /
    // is_loop_condition_const records.  Their producer in FUN_180914500 is
    // now recovered: each RHS/condition is cloned and repeatedly simplified,
    // and the attribute is stored exactly when the result is not
    // AST_CONSTANT(0x10).  Keep this source adapter conservative: a declared
    // runtime dependency proves the nonconstant result; an unresolved name,
    // macro or unsupported fold remains unreported and therefore manual review.
    // ev_7224de6505683c4c1fe46103380cea52bc9484884173055f73aa4f9335d92dee
    // r12_1_for_constant_attributes.txt SHA256
    // 3fa8e1135ca6c32ea0130b8b0bdb4a0477b0f52f4d7b7e506e031629353ab35a
    const VerilogConstantExpressions declarations(tokens);
    for (std::size_t i = 0; i + 1 < tokens.size(); ++i) {
        if (tokens[i].text != "for" || tokens[i + 1].text != "(") continue;
        const auto close = closeParen(tokens, i + 1);
        if (close == tokens.size()) continue;
        std::array<std::size_t, 2> separators{tokens.size(), tokens.size()};
        std::size_t count = 0;
        int depth = 1;
        for (auto j = i + 2; j < close; ++j) {
            if (tokens[j].text == "(") ++depth;
            else if (tokens[j].text == ")") --depth;
            else if (tokens[j].text == ";" && depth == 1) {
                if (count < separators.size()) separators[count] = j;
                ++count;
            }
        }
        if (count != 2) continue;
        const auto topLevelAssignment = [&](std::size_t begin, std::size_t end) {
            int round = 0, square = 0, brace = 0;
            for (auto at = begin; at < end; ++at) {
                const auto& text = tokens[at].text;
                if (text == "(") ++round;
                else if (text == ")") --round;
                else if (text == "[") ++square;
                else if (text == "]") --square;
                else if (text == "{") ++brace;
                else if (text == "}") --brace;
                else if (text == "=" && round == 0 && square == 0 && brace == 0)
                    return at;
            }
            return end;
        };
        const auto initEquals = topLevelAssignment(i + 2, separators[0]);
        const auto stepEquals = topLevelAssignment(separators[1] + 1, close);
        if (initEquals == separators[0] || stepEquals == close) continue;
        const auto assignmentTarget = [&](std::size_t begin, std::size_t equals) {
            for (auto at = begin; at < equals; ++at)
                if (tokens[at].kind == parser::HdlTokenKind::Identifier &&
                    tokens[at].text != "genvar" && tokens[at].text != "integer" &&
                    tokens[at].text != "int" && tokens[at].text != "logic" &&
                    tokens[at].text != "reg" && tokens[at].text != "bit" &&
                    tokens[at].text != "signed" && tokens[at].text != "unsigned")
                    return at;
            return tokens.size();
        };
        const auto counterToken = assignmentTarget(i + 2, initEquals);
        const auto stepTarget = assignmentTarget(separators[1] + 1, stepEquals);
        if (counterToken == tokens.size() || stepTarget == tokens.size() ||
            !declarations.isDeclaredRuntimeIdentifier(counterToken) ||
            !declarations.isDeclaredRuntimeIdentifier(stepTarget)) continue;
        const auto& counter = tokens[counterToken].text;
        if (declarations.hasRuntimeDependency(initEquals + 1, separators[0]))
            emit(result, "GJB10157.9.3.1", source, tokens[i].line, tokens[i].column,
                 "for 循环初值依赖运行时变量");
        if (declarations.hasRuntimeDependencyExcept(
                separators[0] + 1, separators[1], counter))
            emit(result, "GJB10157.9.3.1", source, tokens[i].line, tokens[i].column,
                 "for 循环条件依赖循环计数器以外的运行时变量");
        if (tokens[stepTarget].text != counter)
            emit(result, "GJB10157.9.3.1", source, tokens[i].line, tokens[i].column,
                 "for 循环步进写入的变量与初始计数器不一致");
        i = close;
    }
}

void checkResetBranchInsideFor(AnalysisResult& result, const core::SourceFile& source,
                               const std::vector<parser::HdlToken>& tokens) {
    // Native FUN_1805874a0 admits one AST_MODULE root, recursively gathers
    // AST_ALWAYS(0x52), and builds a name set from every AST_POSEDGE/NEGEDGE
    // (0x65/0x66) having exactly one AST_IDENTIFIER(0x13) child. It then visits
    // direct AST_FOR(0x5c) children of the process block and direct AST_CASE
    // (0x57) children of each for body. A CASE matches when a direct
    // AST_REDUCE_BOOL(0x2d) contains a bare identifier or one
    // AST_LOGIC_NOT(0x47) around an identifier present in that event-name set.
    // FUN_180560070 supplies the second gate: following same-line nested CASE
    // lowering, it requires an AST_COND(0x58) with AST_DEFAULT(0x5b), i.e. a
    // complete if/else chain. It never inspects branch assignment targets or
    // distinguishes clock from reset names. The report is per admitted CASE.
    // ev_8f101f9b2d643f53865518245673a2851837d98e45fab8793e73277b4505e60a
    // r12_3_for_reset_logic.txt SHA256
    // ccceaebac0503dbca1fade07f6377316e21d4a22f264d51d9995e109aae927b9
    if (std::ranges::count_if(tokens, [](const auto& token) {
            return token.text == "module";
        }) != 1)
        return; // Native FUN_1805874a0 requires exactly one AST_MODULE root.
    const auto blockStatements = [&](std::size_t statement,
                                     std::size_t limit) {
        std::vector<std::size_t> statements;
        if (statement >= limit || statement >= tokens.size()) return statements;
        if (tokens[statement].text != "begin") {
            statements.push_back(statement);
            return statements;
        }
        const auto blockEnd = statementEnd(tokens, statement);
        if (blockEnd == tokens.size() || blockEnd <= statement + 1) return statements;
        auto cursor = statement + 1;
        if (cursor + 1 < blockEnd && tokens[cursor].text == ":" &&
            tokens[cursor + 1].kind == parser::HdlTokenKind::Identifier)
            cursor += 2;
        const auto closingEnd = blockEnd - 1;
        while (cursor < closingEnd && cursor < limit) {
            if (tokens[cursor].text == ";") { ++cursor; continue; }
            statements.push_back(cursor);
            const auto next = statementEnd(tokens, cursor);
            if (next <= cursor || next > closingEnd) break;
            cursor = next;
        }
        return statements;
    };
    const auto unwrapFirstBlock = [&](std::size_t statement,
                                      std::size_t limit) {
        auto statements = blockStatements(statement, limit);
        if (!statements.empty() && tokens[statements.front()].text == "begin")
            statements = blockStatements(statements.front(), limit);
        return statements;
    };
    const auto completeIfElse = [&](auto&& self, std::size_t ifToken,
                                    std::size_t limit) -> bool {
        if (ifToken + 1 >= limit || tokens[ifToken].text != "if" ||
            tokens[ifToken + 1].text != "(") return false;
        const auto close = closeParen(tokens, ifToken + 1);
        if (close >= limit || close + 1 >= limit) return false;
        const auto thenEnd = statementEnd(tokens, close + 1);
        if (thenEnd >= limit || tokens[thenEnd].text != "else" ||
            thenEnd + 1 >= limit) return false;
        const auto elseBegin = thenEnd + 1;
        if (tokens[elseBegin].text == "if") return self(self, elseBegin, limit);
        const auto elseEnd = statementEnd(tokens, elseBegin);
        return elseEnd > elseBegin && elseEnd <= limit;
    };
    const auto directConditionIdentifier = [&](std::size_t ifToken,
                                               std::size_t limit)
        -> std::optional<std::size_t> {
        if (ifToken + 1 >= limit || tokens[ifToken + 1].text != "(")
            return std::nullopt;
        const auto close = closeParen(tokens, ifToken + 1);
        if (close >= limit) return std::nullopt;
        auto begin = ifToken + 2;
        auto end = close;
        while (begin < end && tokens[begin].text == "(" &&
               closeParen(tokens, begin) + 1 == end) {
            ++begin;
            --end;
        }
        if (begin < end && tokens[begin].text == "!") ++begin;
        return begin + 1 == end &&
               tokens[begin].kind == parser::HdlTokenKind::Identifier
            ? std::optional<std::size_t>{begin} : std::nullopt;
    };

    for (std::size_t process = 0; process + 3 < tokens.size(); ++process) {
        if ((tokens[process].text != "always" &&
             tokens[process].text != "always_ff") ||
            tokens[process + 1].text != "@" || tokens[process + 2].text != "(")
            continue;
        const auto eventEnd = closeParen(tokens, process + 2);
        if (eventEnd == tokens.size()) continue;
        std::set<std::string> eventNames;
        for (auto event = process + 3; event + 1 < eventEnd; ++event) {
            if (tokens[event].text != "posedge" && tokens[event].text != "negedge")
                continue;
            auto child = event + 1;
            auto childEnd = child + 1;
            while (child < eventEnd && tokens[child].text == "(") {
                const auto close = closeParen(tokens, child);
                if (close >= eventEnd) break;
                ++child;
                childEnd = close;
            }
            if (child < eventEnd && child + 1 == childEnd &&
                tokens[child].kind == parser::HdlTokenKind::Identifier)
                eventNames.insert(tokens[child].text);
        }
        if (eventNames.empty() || eventEnd + 1 >= tokens.size()) continue;
        const auto processEnd = statementEnd(tokens, eventEnd + 1);
        if (processEnd <= eventEnd + 1 || processEnd > tokens.size()) continue;
        auto processStatements = unwrapFirstBlock(eventEnd + 1, processEnd);
        for (const auto loop : processStatements) {
            if (loop + 1 >= processEnd || tokens[loop].text != "for" ||
                tokens[loop + 1].text != "(") continue;
            const auto headerEnd = closeParen(tokens, loop + 1);
            if (headerEnd >= processEnd || headerEnd + 1 >= processEnd) continue;
            const auto loopEnd = statementEnd(tokens, headerEnd + 1);
            if (loopEnd <= headerEnd + 1 || loopEnd > processEnd) continue;
            auto loopStatements = unwrapFirstBlock(headerEnd + 1, loopEnd);
            for (const auto statement : loopStatements) {
                if (tokens[statement].text != "if" ||
                    !completeIfElse(completeIfElse, statement, loopEnd)) continue;
                const auto condition = directConditionIdentifier(statement, loopEnd);
                if (!condition || !eventNames.contains(tokens[*condition].text)) continue;
                emit(result, "GJB10157.9.3.3", source,
                     tokens[loop].line, tokens[loop].column,
                     "for 循环中同时出现复位部分和逻辑部分");
            }
        }
        process = processEnd - 1;
    }
}

void expressionChecks(AnalysisResult& result, const core::SourceFile& source,
                      const std::vector<parser::HdlToken>& tokens, bool vhdl) {
    std::vector<bool> condition(tokens.size(), false);
    const VerilogConstantExpressions constants(tokens);
    if (!vhdl) {
        // Native 8.5: MODULE -> recursive ALWAYS/ASSIGN roots -> six binary
        // arithmetic types -> recursively inspect constant X/Z descendants.
        for (std::size_t i = 0; i < tokens.size(); ++i) {
            const auto& word = tokens[i].text;
            if (tokens[i].kind != parser::HdlTokenKind::Identifier ||
                source.content()[tokens[i].offset] == '\\') continue;
            if (word != "assign" && word != "always" && word != "always_comb" &&
                word != "always_ff" && word != "always_latch") continue;
            auto body = i + 1;
            if (word != "assign") {
                while (body < tokens.size() &&
                       (tokens[body].text == "@" || tokens[body].text == "#")) {
                    ++body;
                    if (body < tokens.size() && tokens[body].text == "(") {
                        const auto close = closeParen(tokens, body);
                        if (close == tokens.size()) { body = close; break; }
                        body = close + 1;
                    } else if (body < tokens.size()) ++body;
                }
            }
            if (body >= tokens.size()) continue;
            const auto end = statementEnd(tokens, body);
            for (const auto& occurrence : detail::unknownArithmeticOccurrences(
                     tokens, i + 1, end, verilogUnknownMask))
                emit(result, "GJB10157.8.5", source,
                     tokens[occurrence.operatorIndex].line,
                     tokens[occurrence.operatorIndex].column,
                     occurrence.state == 'x' ?
                         "算术表达式使用了包含 X 的值" :
                         "算术表达式使用了包含 Z 的值");
            i = end > i ? end - 1 : i;
        }
        const auto relationFilter = [&](std::size_t candidate) {
            if (tokens[candidate].text != "<=") return true;
            std::size_t statement = 0;
            for (std::size_t cursor = candidate; cursor > 0; --cursor) {
                const auto& previous = tokens[cursor - 1].text;
                if (previous == ";" || previous == "begin" || previous == "end") {
                    statement = cursor;
                    break;
                }
            }
            int parentheses = 0;
            int brackets = 0;
            int braces = 0;
            bool earlierAssignment = false;
            for (std::size_t cursor = statement; cursor < candidate; ++cursor) {
                const auto& text = tokens[cursor].text;
                if (text == "(") ++parentheses;
                else if (text == ")") --parentheses;
                else if (text == "[") ++brackets;
                else if (text == "]") --brackets;
                else if (text == "{") ++braces;
                else if (text == "}") --braces;
                else if (parentheses == 0 && brackets == 0 && braces == 0 &&
                         (text == "=" || text == "<=")) earlierAssignment = true;
            }
            // The first top-level <= after a direct procedural target is
            // AST_ASSIGN_LE. Nested <= and operators on an established RHS
            // remain AST_LE comparison nodes.
            return parentheses != 0 || brackets != 0 || braces != 0 || earlierAssignment;
        };
        for (const auto& occurrence : detail::unknownRelationOccurrences(
                 tokens, 0, tokens.size(), verilogUnknownMask, relationFilter)) {
            emit(result, "GJB10157.8.4", source,
                 tokens[occurrence.operatorIndex].line,
                 tokens[occurrence.operatorIndex].column,
                 occurrence.state == 'x' ?
                     "关系表达式使用了包含 X 的值" :
                     "关系表达式使用了包含 Z 的值");
        }
    }
    struct NativeDefaultCase {
        std::size_t start;
        std::size_t end;
        bool eligible;
    };
    std::vector<NativeDefaultCase> nativeDefaultCases;
    std::vector<std::size_t> nativeModuleLines;
    for (std::size_t i = 0; i < tokens.size(); ++i) {
        const auto& token = tokens[i];
        if (token.kind != parser::HdlTokenKind::Identifier ||
            source.content()[token.offset] == '\\') continue;
        const auto word = vhdl ? lower(token.text) : token.text;
        if (!vhdl && word == "endmodule") {
            if (!nativeModuleLines.empty()) nativeModuleLines.pop_back();
        } else if (!vhdl && word == "module") {
            nativeModuleLines.push_back(token.line);
        }
        while (!nativeDefaultCases.empty() && nativeDefaultCases.back().end <= i)
            nativeDefaultCases.pop_back();
        if (vhdl && (word == "if" || word == "elsif" || word == "when")) {
            for (auto j = i + 1; j < tokens.size(); ++j) {
                const auto next = lower(tokens[j].text);
                if (next == "then" || next == "else" || next == ";" || next == "=>") break;
                condition[j] = true;
            }
        }
        if (!vhdl && (word == "casex" || word == "casez") &&
            nativeWildcardCaseHasItem(tokens, i)) {
            emit(result, "GJB10157.9.2.8", source, token.line, token.column,
                 "避免使用 casex 或 casez 通配分支语句");
        }
        if (!vhdl && (word == "if" || word == "case" || word == "casex" || word == "casez") &&
            i + 1 < tokens.size() && tokens[i + 1].text == "(") {
            const auto end = closeParen(tokens, i + 1);
            if (end == tokens.size()) continue;
            if (word != "if") {
                const auto caseEnd = matchingNativeCaseEnd(tokens, i);
                if (caseEnd != tokens.size()) {
                    nativeDefaultCases.push_back(NativeDefaultCase{
                        i, caseEnd,
                        !nativeModuleLines.empty() &&
                        token.line != nativeModuleLines.back() &&
                        nativeCaseSelectorIsIdentifier(tokens, i)});
                }
            }
            bool unknown = false;
            const bool constant = constants.isConstant(i + 2, end);
            for (auto j = i + 2; j < end; ++j) {
                condition[j] = true;
                unknown |= verilogUnknown(tokens[j]);
            }
            if (unknown && word != "casex" && word != "casez")
                emit(result, "GJB10157.9.2.4", source, token.line, token.column,
                              "if 条件或 case 控制表达式使用了 X 或 Z 状态");
            if (word != "if" && constant &&
                matchingNativeCaseEnd(tokens, i) != tokens.size())
                emit(result, "GJB10157.9.2.2", source, token.line, token.column,
                     "禁止在case语句的控制表达式中使用固定值.");
        }
        if (!vhdl && word == "default" && !nativeDefaultCases.empty() &&
            nativeDefaultCases.back().eligible &&
            nativeDirectCaseDefault(tokens, nativeDefaultCases.back().start, i)) {
            auto start = i + 1;
            if (start < tokens.size() && tokens[start].text == ":") ++start;
            const auto end = statementEnd(tokens, start);
            const auto assignments = detail::defaultAssignmentUnknowns(tokens, start, end,
                [](const auto& value) { return verilogUnknown(value, true); });
            for (const auto& assignment : assignments.assignments) {
                if (assignment.unknownLiteralIndices.empty()) continue;
                const bool identifierRoot = assignment.lhsBegin < assignment.assignmentOperator &&
                    tokens[assignment.lhsBegin].kind == parser::HdlTokenKind::Identifier;
                const bool internalTarget = identifierRoot && std::ranges::any_of(
                    std::ranges::subrange(tokens.begin() + assignment.lhsBegin,
                                          tokens.begin() + assignment.assignmentOperator),
                    [](const auto& lhsToken) {
                        return lhsToken.kind == parser::HdlTokenKind::Identifier &&
                               lhsToken.text.find('$') != std::string::npos;
                    });
                if (internalTarget) continue;
                emit(result, "GJB10157.9.2.9", source, token.line, token.column,
                     "避免在default分支中出现包含'x'的值.");
            }
        }
    }

    if (vhdl) {
        const auto boundary = [&](std::size_t index) {
            const auto word = lower(tokens[index].text);
            return word == ";" || word == "," || word == ":" || word == "?" ||
                word == "begin" || word == "end" || word == "then" || word == "else" ||
                word == "when" || word == "if" || word == "elsif" || word == "return" ||
                word == "assert" || word == "report" || word == "and" || word == "or" ||
                word == "xor" || word == "nand" || word == "nor" || word == "xnor" ||
                word == ":=" || word == "=>" ||
                (word == "<=" && !condition[index]);
        };
        const auto lessEqualIsRelation = [&](std::size_t candidate) {
            if (condition[candidate]) return true;
            std::size_t statement = 0;
            for (std::size_t cursor = candidate; cursor > 0; --cursor) {
                const auto word = lower(tokens[cursor - 1].text);
                if (word == ";" || word == "begin" || word == "then" ||
                    word == "else" || word == "=>") {
                    statement = cursor;
                    break;
                }
            }
            int parentheses = 0;
            bool earlierAssignment = false;
            for (std::size_t cursor = statement; cursor < candidate; ++cursor) {
                const auto& text = tokens[cursor].text;
                if (text == "(") ++parentheses;
                else if (text == ")") --parentheses;
                else if (parentheses == 0 && (text == "<=" || text == ":="))
                    earlierAssignment = true;
            }
            return parentheses != 0 || earlierAssignment;
        };
        const auto directLiteral = [&](std::size_t begin, std::size_t end)
            -> std::optional<std::size_t> {
            while (begin < end && tokens[begin].text == "(" &&
                   closeParen(tokens, begin) == end - 1) {
                ++begin;
                --end;
            }
            if (begin + 1 == end && vhdlForbiddenComparisonValue(tokens[begin]))
                return begin;
            return std::nullopt;
        };
        for (std::size_t relation = 0; relation < tokens.size(); ++relation) {
            const auto& op = tokens[relation].text;
            if (op != "=" && op != "/=" && op != "<" && op != ">" &&
                op != ">=" && (op != "<=" || !lessEqualIsRelation(relation))) continue;

            std::size_t left = relation;
            int depth = 0;
            while (left > 0) {
                const auto index = left - 1;
                const auto& text = tokens[index].text;
                if (text == ")") ++depth;
                else if (text == "(") {
                    if (depth == 0) break;
                    --depth;
                }
                if (depth == 0 && boundary(index)) break;
                left = index;
            }
            std::size_t right = relation + 1;
            depth = 0;
            while (right < tokens.size()) {
                const auto& text = tokens[right].text;
                if (text == "(") ++depth;
                else if (text == ")") {
                    if (depth == 0) break;
                    --depth;
                }
                if (depth == 0 && boundary(right)) break;
                ++right;
            }
            for (const auto literal : {directLiteral(left, relation),
                                       directLiteral(relation + 1, right)}) {
                if (!literal) continue;
                emit(result, "GJB9765.4.4.c", source,
                     tokens[*literal].line, tokens[*literal].column,
                     "关系表达式使用了不可用于综合比较的逻辑状态值");
            }
        }
    }
}

bool startsWithHeaderComment(const std::vector<std::string>& lines, bool vhdl) {
    for (auto line : lines) {
        const auto first = line.find_first_not_of(" \t");
        if (first == std::string::npos) continue;
        const auto text = line.substr(first);
        return vhdl ? text.starts_with("--") :
                      (text.starts_with("//") || text.starts_with("/*"));
    }
    return false;
}

bool documentedHeader(const core::SourceFile& source,
                      const std::vector<parser::HdlToken>& tokens) {
    const auto header = lower(source.content().substr(0, tokens.empty() ?
        source.content().size() : tokens.front().offset));
    const auto field = [&](std::initializer_list<std::string_view> labels, bool valueRequired) {
        for (const auto label : labels) {
            const auto position = header.find(label);
            if (position == std::string::npos) continue;
            const auto colon = header.find(':', position + label.size());
            const auto newline = header.find('\n', position);
            if (colon == std::string::npos || (newline != std::string::npos && colon > newline)) continue;
            if (!valueRequired) return true;
            const auto value = header.find_first_not_of(" \t\r", colon + 1);
            if (value != std::string::npos && (newline == std::string::npos || value < newline) &&
                !header.substr(value).starts_with("//") && !header.substr(value).starts_with("--")) return true;
        }
        return false;
    };
    // The attachment's compact positive example 2 has an empty author field,
    // while detailed negative example 3 rejects one. This independent template
    // check distinguishes the forms; it is not the native keyword-map rule.
    const bool detailed = header.find("last modified") != std::string::npos ||
        header.find("last version") != std::string::npos;
    return field({"filename", "file name", "file", "文件名"}, true) &&
        field({"author", "作者", "修改人"}, detailed) &&
        field({"description", "功能描述", "说明"}, false) &&
        (!detailed || field({"history", "修改记录", "修改历史"}, false));
}

void checkVhdlPortlessComponents(AnalysisResult& result, const core::SourceFile& source,
                                 const std::vector<parser::HdlToken>& tokens) {
    // Native 4.2.a has two independent branches. Besides the direct entity
    // port container (0xa4), it walks nodes of type 0x75 and reports each
    // empty 0x7b child through the embedded VLR-010073 path. The shipped
    // VLR-010073 example identifies that child as a component declaration
    // without ports. Keep this adapter bounded to complete declarations.
    for (std::size_t index = 0; index + 1 < tokens.size(); ++index) {
        if (lower(tokens[index].text) != "component" ||
            (index > 0 && lower(tokens[index - 1].text) == "end") ||
            tokens[index + 1].kind != parser::HdlTokenKind::Identifier) continue;
        std::size_t end = index + 2;
        while (end + 1 < tokens.size() &&
               !(lower(tokens[end].text) == "end" &&
                 lower(tokens[end + 1].text) == "component")) ++end;
        if (end + 1 >= tokens.size()) continue;

        bool hasPortObject = false;
        for (std::size_t port = index + 2; port + 1 < end; ++port) {
            if (lower(tokens[port].text) != "port" || tokens[port + 1].text != "(") continue;
            std::size_t depth = 1;
            for (std::size_t cursor = port + 2; cursor < end && depth != 0; ++cursor) {
                if (tokens[cursor].text == "(") ++depth;
                else if (tokens[cursor].text == ")") --depth;
                else if (depth == 1 && tokens[cursor].text == ":") {
                    for (std::size_t name = port + 2; name < cursor; ++name) {
                        if (tokens[name].kind == parser::HdlTokenKind::Identifier) {
                            hasPortObject = true;
                            break;
                        }
                    }
                }
            }
            break;
        }
        if (!hasPortObject)
            emit(result, "GJB9765.4.2.a", source,
                 tokens[index + 1].line, tokens[index + 1].column,
                 "组件声明没有输入或输出端口：" + tokens[index + 1].text);
        index = end + 1;
    }
}

void checkVhdlEntityPortDirectionSuffixes(
    AnalysisResult& result, const core::SourceFile& source,
    const std::vector<parser::HdlToken>& tokens) {
    // Native f.22/f.23/f.24 read three distinct direction flags from VHDL
    // port objects (+0xa0/+0xa1/+0x104) and compare the port name with a
    // configured suffix through FUN_1804d6e80 mode 2. That helper extracts
    // the tail and case-folds both strings before comparison. The shipped
    // examples establish the defaults as _in, _out and _inout. Keep buffer
    // separate: its native flag is +0x105 and therefore it is not an OUT
    // candidate for these rules.
    for (std::size_t entity = 0; entity + 2 < tokens.size(); ++entity) {
        if (lower(tokens[entity].text) != "entity" ||
            (entity > 0 && lower(tokens[entity - 1].text) == "end") ||
            tokens[entity + 1].kind != parser::HdlTokenKind::Identifier ||
            lower(tokens[entity + 2].text) != "is") continue;

        std::size_t port = entity + 3;
        for (; port + 1 < tokens.size(); ++port) {
            const auto word = lower(tokens[port].text);
            if (word == "generic" && tokens[port + 1].text == "(") {
                const auto genericClose = closeParen(tokens, port + 1);
                if (genericClose == tokens.size()) break;
                port = genericClose;
                continue;
            }
            if (word == "end" || word == "begin" || word == "component" ||
                word == "signal" || word == "constant" || word == "type" ||
                word == "subtype" || word == "function" || word == "procedure") break;
            if (word == "port" && tokens[port + 1].text == "(") break;
        }
        if (port + 1 >= tokens.size() || lower(tokens[port].text) != "port" ||
            tokens[port + 1].text != "(") continue;
        const auto close = closeParen(tokens, port + 1);
        if (close == tokens.size()) continue;

        std::size_t segment = port + 2;
        int nested = 0;
        for (std::size_t end = segment; end <= close; ++end) {
            if (end < close && tokens[end].text == "(") { ++nested; continue; }
            if (end < close && tokens[end].text == ")" && nested > 0) { --nested; continue; }
            if (end < close && (tokens[end].text != ";" || nested != 0)) continue;

            std::size_t colon = segment;
            while (colon < end && tokens[colon].text != ":") ++colon;
            if (colon < end) {
                std::string_view clause;
                std::string_view suffix;
                auto mode = colon + 1;
                while (mode < end) {
                    const auto word = lower(tokens[mode].text);
                    if (word == "in" || word == "out" || word == "inout" ||
                        word == "buffer" || word == "linkage") break;
                    ++mode;
                }
                const auto direction = mode < end ? lower(tokens[mode].text) : "in";
                if (direction == "in") {
                    clause = "GJB9765.4.5.f.22"; suffix = "_in";
                } else if (direction == "out") {
                    clause = "GJB9765.4.5.f.23"; suffix = "_out";
                } else if (direction == "inout") {
                    clause = "GJB9765.4.5.f.24"; suffix = "_inout";
                }
                if (!clause.empty()) {
                    for (auto name = segment; name < colon; ++name) {
                        if (tokens[name].kind != parser::HdlTokenKind::Identifier) continue;
                        const auto word = lower(tokens[name].text);
                        if (word == "signal" || word == "constant" || word == "variable" ||
                            word == "file") continue;
                        if (!word.ends_with(suffix))
                            emit(result, clause, source, tokens[name].line, tokens[name].column,
                                 "端口名称未以方向后缀 " + std::string(suffix) +
                                 " 结束：" + tokens[name].text);
                    }
                }
            }
            segment = end + 1;
        }
        entity = close;
    }
}

enum class VhdlNameCategory {
    Signal,
    Variable,
    Constant,
    Type,
    Subtype,
    Entity,
    Architecture,
    Port,
    Generic,
};

struct VhdlNamedObject {
    std::size_t token{};
    VhdlNameCategory category{};
};

std::string_view vhdlNameCategoryName(VhdlNameCategory category) {
    switch (category) {
    case VhdlNameCategory::Signal: return "信号";
    case VhdlNameCategory::Variable: return "变量";
    case VhdlNameCategory::Constant: return "常量";
    case VhdlNameCategory::Type: return "类型";
    case VhdlNameCategory::Subtype: return "子类型";
    case VhdlNameCategory::Entity: return "实体";
    case VhdlNameCategory::Architecture: return "结构体";
    case VhdlNameCategory::Port: return "端口";
    case VhdlNameCategory::Generic: return "泛型";
    }
    return "标识符";
}

std::vector<VhdlNamedObject> collectVhdlNamedObjects(
    const core::SourceFile& source, const std::vector<parser::HdlToken>& tokens) {
    std::vector<VhdlNamedObject> objects;
    std::vector<bool> interfaceToken(tokens.size(), false);
    const auto simple = [&](std::size_t index) {
        if (index >= tokens.size() || tokens[index].text.empty() ||
            (tokens[index].kind != parser::HdlTokenKind::Identifier &&
             tokens[index].kind != parser::HdlTokenKind::Number) ||
            tokens[index].offset >= source.content().size()) return false;
        return source.content()[tokens[index].offset] != '\\';
    };
    const auto addNames = [&](std::size_t begin, std::size_t end,
                              VhdlNameCategory category) {
        for (auto name = begin; name < end; ++name) {
            if (!simple(name) || tokens[name].text == ",") continue;
            const auto word = lower(tokens[name].text);
            if (word == "signal" || word == "constant" || word == "variable" ||
                word == "file") continue;
            objects.push_back({name, category});
        }
    };

    // Native 4.5.b groups direct 0xa4/0xa3 children separately. Prior rule
    // recovery maps these containers to VHDL PORT and GENERIC interfaces.
    // Parse complete interface segments so invalid names such as 1_bad are
    // retained even though the general lexer classifies them as numbers.
    for (std::size_t i = 0; i + 1 < tokens.size(); ++i) {
        const auto word = lower(tokens[i].text);
        if ((word != "port" && word != "generic") || tokens[i + 1].text != "(")
            continue;
        if (i > 0 && lower(tokens[i - 1].text) == "map") continue;
        const auto close = closeParen(tokens, i + 1);
        if (close == tokens.size()) continue;
        for (auto mark = i; mark <= close; ++mark) interfaceToken[mark] = true;
        std::size_t segment = i + 2;
        int depth = 0;
        for (std::size_t end = segment; end <= close; ++end) {
            if (end < close && tokens[end].text == "(") { ++depth; continue; }
            if (end < close && tokens[end].text == ")" && depth > 0) { --depth; continue; }
            if (end < close && (tokens[end].text != ";" || depth != 0)) continue;
            auto colon = segment;
            while (colon < end && tokens[colon].text != ":") ++colon;
            if (colon < end)
                addNames(segment, colon, word == "port" ? VhdlNameCategory::Port
                                                         : VhdlNameCategory::Generic);
            segment = end + 1;
        }
        i = close;
    }

    for (std::size_t i = 0; i + 1 < tokens.size(); ++i) {
        const auto word = lower(tokens[i].text);
        if (word == "entity" && (i == 0 || lower(tokens[i - 1].text) != "end") &&
            i + 2 < tokens.size() && simple(i + 1) && lower(tokens[i + 2].text) == "is") {
            objects.push_back({i + 1, VhdlNameCategory::Entity});
            continue;
        }
        if (word == "architecture" &&
            (i == 0 || lower(tokens[i - 1].text) != "end") &&
            i + 4 < tokens.size() && simple(i + 1) &&
            lower(tokens[i + 2].text) == "of" && lower(tokens[i + 4].text) == "is") {
            objects.push_back({i + 1, VhdlNameCategory::Architecture});
            continue;
        }
        if ((word == "type" || word == "subtype") && i + 2 < tokens.size() &&
            simple(i + 1) && lower(tokens[i + 2].text) == "is") {
            objects.push_back({i + 1, word == "type" ? VhdlNameCategory::Type
                                                       : VhdlNameCategory::Subtype});
            continue;
        }
        VhdlNameCategory category;
        if (word == "signal") category = VhdlNameCategory::Signal;
        else if (word == "variable") category = VhdlNameCategory::Variable;
        else if (word == "constant") category = VhdlNameCategory::Constant;
        else continue;
        if (interfaceToken[i]) continue;
        std::size_t end = i + 1;
        while (end < tokens.size() && tokens[end].text != ";") ++end;
        auto colon = i + 1;
        while (colon < end && tokens[colon].text != ":") ++colon;
        if (colon < end) addNames(i + 1, colon, category);
        i = colon < end ? colon : i;
    }
    return objects;
}

void checkVhdlIdentifierFoundation(AnalysisResult& result,
                                   const core::SourceFile& source,
                                   const std::vector<parser::HdlToken>& tokens) {
    const auto objects = collectVhdlNamedObjects(source, tokens);
    const auto keywordCandidate = [](VhdlNameCategory category) {
        return category == VhdlNameCategory::Signal ||
            category == VhdlNameCategory::Variable ||
            category == VhdlNameCategory::Constant ||
            category == VhdlNameCategory::Type ||
            category == VhdlNameCategory::Subtype ||
            category == VhdlNameCategory::Port ||
            category == VhdlNameCategory::Generic;
    };
    const auto shortIdentifier = [](VhdlNameCategory category) {
        return category == VhdlNameCategory::Signal ||
            category == VhdlNameCategory::Variable ||
            category == VhdlNameCategory::Constant ||
            category == VhdlNameCategory::Type ||
            category == VhdlNameCategory::Port ||
            category == VhdlNameCategory::Generic;
    };

    // Native 4.5.a recursively admits AST 0x77/0x78/0x79/0x0a/0x9f and
    // direct 0xa4/0xa3 children: type, signal, variable, constant, subtype,
    // port and generic names.  Its VHDL dictionary is the exact embedded
    // IEEE 1076-2008 list below.  The binary also carries separately gated
    // Verilog/SystemVerilog/SDF-EDIF/PSL/Windows dictionaries; the published
    // GJB rule requires the VHDL set, so those optional policies are not
    // silently enabled here.  Fold before lookup because VHDL identifiers
    // and reserved words are case-insensitive.
    static const std::set<std::string_view> vhdlKeywords{
        "abs", "access", "after", "alias", "all", "and", "architecture",
        "array", "assert", "assume", "assume_guarantee", "attribute", "begin",
        "block", "body", "buffer", "bus", "case", "component", "configuration",
        "constant", "context", "cover", "default", "disconnect", "downto",
        "else", "elsif", "end", "entity", "exit", "fairness", "file", "for",
        "force", "function", "generate", "generic", "group", "guarded", "if",
        "impure", "in", "inertial", "inout", "is", "label", "library",
        "linkage", "literal", "loop", "map", "mod", "nand", "new", "next",
        "nor", "not", "null", "of", "on", "open", "or", "others", "out",
        "package", "parameter", "port", "postponed", "procedure", "process",
        "property", "protected", "pure", "range", "record", "register", "reject",
        "release", "rem", "report", "restrict", "restrict_guarantee", "return",
        "rol", "ror", "select", "sequence", "severity", "signal", "shared",
        "sla", "sll", "sra", "srl", "strong", "subtype", "then", "to",
        "transport", "type", "unaffected", "units", "until", "use", "variable",
        "vmode", "vprop", "vunit", "wait", "when", "while", "with", "xnor", "xor",
    };
    for (const auto& object : objects) {
        if (!keywordCandidate(object.category)) continue;
        const auto& token = tokens[object.token];
        const auto name = lower(token.text);
        if (!vhdlKeywords.contains(name)) continue;
        emit(result, "GJB9765.4.5.a", source, token.line, token.column,
             "自定义短标识符禁止与 VHDL 关键字重复：" + token.text + "（" +
             std::string(vhdlNameCategoryName(object.category)) + "）");
    }

    // Native 4.5.b builds eight maps for AST 0x78/0x79/0x0a/0x77/0x76/
    // 0x75 and the direct children of 0xa4/0xa3, then performs pairwise set
    // intersections. Those kinds map to the eight categories below. The map
    // keys preserve original spelling; case-only collisions belong to 4.5.d.
    std::map<std::string, std::vector<const VhdlNamedObject*>> byExactName;
    for (const auto& object : objects) {
        if (object.category == VhdlNameCategory::Subtype) continue;
        byExactName[tokens[object.token].text].push_back(&object);
    }
    for (const auto& [name, matches] : byExactName) {
        std::set<VhdlNameCategory> seen;
        for (const auto* object : matches) {
            if (seen.empty()) {
                seen.insert(object->category);
                continue;
            }
            if (seen.contains(object->category)) continue;
            emit(result, "GJB9765.4.5.b", source,
                 tokens[object->token].line, tokens[object->token].column,
                 "不同类型的标识符使用了相同名称：" + name + "（" +
                 std::string(vhdlNameCategoryName(object->category)) + "）");
            seen.insert(object->category);
        }
    }

    // Native 4.5.c lowercases each selected name, requires ASCII a-z in the
    // first byte, permits only a-z/0-9/_ afterwards, and emits independently
    // for a trailing underscore and every repeated underscore. Its recursive
    // collector admits exactly the short-identifier categories above.
    for (const auto& object : objects) {
        if (!shortIdentifier(object.category)) continue;
        const auto& token = tokens[object.token];
        const auto name = lower(token.text);
        if (name.empty()) continue;
        if (name.front() < 'a' || name.front() > 'z')
            emit(result, "GJB9765.4.5.c", source, token.line, token.column,
                 "标识符应以英文字母开头：" + token.text);
        for (std::size_t index = 1; index < name.size(); ++index) {
            const auto character = name[index];
            if (!((character >= 'a' && character <= 'z') ||
                  (character >= '0' && character <= '9') || character == '_'))
                emit(result, "GJB9765.4.5.c", source, token.line,
                     token.column + index,
                     "标识符只能包含字母、数字和下划线：" + token.text);
            if (character == '_' && name[index - 1] == '_')
                emit(result, "GJB9765.4.5.c", source, token.line,
                     token.column + index,
                     "标识符禁止连续使用下划线：" + token.text);
        }
        if (name.back() == '_')
            emit(result, "GJB9765.4.5.c", source, token.line,
                 token.column + name.size() - 1,
                 "标识符末字符禁止使用下划线：" + token.text);
    }

    // Native 4.5.d stores a folded copy beside the original spelling. It
    // reports declaration pairs whose folded names match but originals do
    // not, then applies the same comparison to identifier-use nodes. Keep the
    // source adapter to the recovered declaration kinds; unknown native kind
    // 0x06 remains outside automatic admission.
    std::vector<const VhdlNamedObject*> declarations;
    std::set<std::size_t> declarationTokens;
    for (const auto& object : objects) {
        if (object.category != VhdlNameCategory::Signal &&
            object.category != VhdlNameCategory::Variable &&
            object.category != VhdlNameCategory::Constant &&
            object.category != VhdlNameCategory::Type &&
            object.category != VhdlNameCategory::Subtype) continue;
        declarations.push_back(&object);
        declarationTokens.insert(object.token);
    }
    std::map<std::string, std::vector<const VhdlNamedObject*>> declarationsByFolded;
    for (const auto* declaration : declarations)
        declarationsByFolded[lower(tokens[declaration->token].text)].push_back(declaration);
    for (const auto& [folded, matches] : declarationsByFolded) {
        std::set<std::string> spellings;
        for (const auto* object : matches) {
            const auto& token = tokens[object->token];
            if (spellings.empty() || spellings.contains(token.text)) {
                spellings.insert(token.text);
                continue;
            }
            emit(result, "GJB9765.4.5.d", source, token.line, token.column,
                 "禁止仅用字母大小写区分不同命名：" + token.text);
            spellings.insert(token.text);
        }
    }
    for (std::size_t index = 0; index < tokens.size(); ++index) {
        if (declarationTokens.contains(index) ||
            tokens[index].kind != parser::HdlTokenKind::Identifier) continue;
        const auto found = declarationsByFolded.find(lower(tokens[index].text));
        if (found == declarationsByFolded.end()) continue;
        const bool exact = std::ranges::any_of(found->second, [&](const auto* declaration) {
            return tokens[declaration->token].text == tokens[index].text;
        });
        if (!exact)
            emit(result, "GJB9765.4.5.d", source, tokens[index].line,
                 tokens[index].column,
                 "同一标识符的字母大小写不一致：" + tokens[index].text);
    }
}

void checkVhdlDeclarationPrefixes(AnalysisResult& result,
                                  const core::SourceFile& source,
                                  const std::vector<parser::HdlToken>& tokens) {
    const auto simpleName = [&](std::size_t index) {
        return index < tokens.size() &&
            tokens[index].kind == parser::HdlTokenKind::Identifier &&
            tokens[index].offset < source.content().size() &&
            source.content()[tokens[index].offset] != '\\';
    };
    const auto checkPrefix = [&](std::size_t index, std::string_view clause,
                                 std::string_view prefix, std::string_view kind) {
        if (!simpleName(index)) return;
        const auto name = lower(tokens[index].text);
        if (!name.starts_with(prefix))
            emit(result, clause, source, tokens[index].line, tokens[index].column,
                 std::string(kind) + "名称未以 " + std::string(prefix) +
                 " 开始：" + tokens[index].text);
    };

    for (std::size_t i = 0; i + 1 < tokens.size(); ++i) {
        const auto word = lower(tokens[i].text);
        if (word == "variable") {
            // Native f.2 recursively gathers VHDL node type 0x79 and applies
            // case-insensitive FUN_1804d6e80 mode 0 to every nonempty name.
            // Bound the source adapter to complete variable declarations.
            std::size_t end = i + 1;
            while (end < tokens.size() && tokens[end].text != ";") ++end;
            std::size_t colon = i + 1;
            while (colon < end && tokens[colon].text != ":") ++colon;
            if (colon < end) {
                for (auto name = i + 1; name < colon; ++name)
                    if (tokens[name].text != ",")
                        checkPrefix(name, "GJB9765.4.5.f.2", "v", "变量");
                i = colon;
            }
            continue;
        }
        if ((word == "type" || word == "subtype") && i + 2 < tokens.size() &&
            lower(tokens[i + 2].text) == "is") {
            checkPrefix(i + 1,
                word == "type" ? "GJB9765.4.5.f.14" : "GJB9765.4.5.f.15",
                word == "type" ? "t" : "st",
                word == "type" ? "类型" : "子类型");
            continue;
        }
        if (word == "function" && (i == 0 || lower(tokens[i - 1].text) != "end"))
            checkPrefix(i + 1, "GJB9765.4.5.f.16", "f", "函数");
    }
}

void checkVhdlConstantAndDesignUnitNames(AnalysisResult& result,
                                         const core::SourceFile& source,
                                         const std::vector<parser::HdlToken>& tokens) {
    const auto simpleName = [&](std::size_t index) {
        return index < tokens.size() &&
            tokens[index].kind == parser::HdlTokenKind::Identifier &&
            tokens[index].offset < source.content().size() &&
            source.content()[tokens[index].offset] != '\\';
    };
    const auto reportPrefix = [&](std::size_t index, std::string_view clause,
                                  std::string_view prefix, std::string_view kind) {
        if (simpleName(index) && !lower(tokens[index].text).starts_with(prefix))
            emit(result, clause, source, tokens[index].line, tokens[index].column,
                 std::string(kind) + "名称未以 " + std::string(prefix) +
                 " 开始：" + tokens[index].text);
    };
    const auto reportSuffix = [&](std::size_t index, std::string_view clause,
                                  std::string_view suffix, std::string_view kind) {
        if (simpleName(index) && !lower(tokens[index].text).ends_with(suffix))
            emit(result, clause, source, tokens[index].line, tokens[index].column,
                 std::string(kind) + "名称未以 " + std::string(suffix) +
                 " 结束：" + tokens[index].text);
    };

    // Native f.3 reaches object nodes only through declarative containers.
    // VHDL generic/port interface constants are different frontend objects, so
    // keep the lexical adapter outside all complete interface clauses.
    std::vector<bool> interfaceToken(tokens.size(), false);
    for (std::size_t i = 0; i + 1 < tokens.size(); ++i) {
        const auto word = lower(tokens[i].text);
        if ((word != "generic" && word != "port") || tokens[i + 1].text != "(")
            continue;
        const auto close = closeParen(tokens, i + 1);
        if (close == tokens.size()) continue;
        for (auto mark = i; mark <= close; ++mark) interfaceToken[mark] = true;
        i = close;
    }

    // f.8 de-duplicates package names before checking them. Prefer a package
    // declaration location, then a package body/use location, for each folded
    // name so repeated USE clauses do not manufacture duplicate diagnostics.
    std::map<std::string, std::size_t> packageCandidates;
    std::map<std::string, std::size_t> packageDeclarations;
    std::map<std::string, std::size_t> packageBodies;
    for (std::size_t i = 0; i + 2 < tokens.size(); ++i) {
        const auto word = lower(tokens[i].text);
        if (word == "package" && (i == 0 || lower(tokens[i - 1].text) != "end")) {
            if (lower(tokens[i + 1].text) == "body" && simpleName(i + 2)) {
                const auto name = lower(tokens[i + 2].text);
                packageBodies.try_emplace(name, i + 2);
                packageCandidates.try_emplace(name, i + 2);
            } else if (simpleName(i + 1) && lower(tokens[i + 2].text) == "is") {
                const auto name = lower(tokens[i + 1].text);
                packageDeclarations.try_emplace(name, i + 1);
                packageCandidates.insert_or_assign(name, i + 1);
            }
        }
        if (word == "use" && simpleName(i + 1) && lower(tokens[i + 1].text) == "work" &&
            i + 3 < tokens.size() &&
            tokens[i + 2].text == "." && simpleName(i + 3)) {
            packageCandidates.try_emplace(lower(tokens[i + 3].text), i + 3);
        }
    }

    for (std::size_t i = 0; i + 1 < tokens.size(); ++i) {
        const auto word = lower(tokens[i].text);
        if (word == "constant" && !interfaceToken[i]) {
            std::size_t end = i + 1;
            while (end < tokens.size() && tokens[end].text != ";") ++end;
            std::size_t colon = i + 1;
            while (colon < end && tokens[colon].text != ":") ++colon;
            if (colon < end) {
                for (auto name = i + 1; name < colon; ++name)
                    if (tokens[name].text != ",")
                        reportPrefix(name, "GJB9765.4.5.f.3", "c", "常量");
                i = colon;
            }
            continue;
        }
        if (word == "entity" && (i == 0 || lower(tokens[i - 1].text) != "end") &&
            i + 2 < tokens.size() && simpleName(i + 1) &&
            lower(tokens[i + 2].text) == "is") {
            reportSuffix(i + 1, "GJB9765.4.5.f.4", "_ent", "实体");
            const auto expected = tokens[i + 1].text + ".vhd";
            if (source.path().filename().string() != expected)
                emit(result, "GJB9765.4.5.f.5", source, tokens[i + 1].line,
                     tokens[i + 1].column,
                     "实体声明文件名应为 " + expected + "，当前为：" +
                     source.path().filename().string());
            continue;
        }
        if (word == "architecture" && (i == 0 || lower(tokens[i - 1].text) != "end") &&
            i + 4 < tokens.size() && lower(tokens[i + 2].text) == "of" &&
            lower(tokens[i + 4].text) == "is")
            reportSuffix(i + 1, "GJB9765.4.5.f.6", "_arc", "结构体");
    }

    for (const auto& [name, index] : packageCandidates) {
        const bool declarationWithBody = packageBodies.contains(name);
        const bool referencedByUse = [&] {
            for (std::size_t i = 0; i + 3 < tokens.size(); ++i)
                if (lower(tokens[i].text) == "use" && simpleName(i + 1) &&
                    lower(tokens[i + 1].text) == "work" && tokens[i + 2].text == "." &&
                    simpleName(i + 3) && lower(tokens[i + 3].text) == name) return true;
            return false;
        }();
        if (declarationWithBody || referencedByUse)
            reportSuffix(index, "GJB9765.4.5.f.8", "_pack", "包");
        const auto declaration = packageDeclarations.find(name);
        if ((declarationWithBody || referencedByUse) &&
            declaration != packageDeclarations.end() &&
            source.path().stem().string() != tokens[declaration->second].text)
            emit(result, "GJB9765.4.5.f.9", source,
                 tokens[declaration->second].line, tokens[declaration->second].column,
                 "包声明文件基名应与包名一致：" + tokens[declaration->second].text);
    }
    for (const auto& [name, index] : packageBodies) {
        // Native f.10 explicitly excludes package bodies sharing a source path
        // with a matching package declaration before checking _body.vhd.
        if (packageDeclarations.contains(name)) continue;
        const auto expected = lower(tokens[index].text) + "_body.vhd";
        if (lower(source.path().filename().string()) != expected)
            emit(result, "GJB9765.4.5.f.10", source, tokens[index].line,
                 tokens[index].column,
                 "包体文件名应为 " + tokens[index].text + "_body.vhd，当前为：" +
                 source.path().filename().string());
    }
}

std::string nativeVhdlSignalBase(std::string_view name) {
    const auto underscore = name.find('_');
    return lower(std::string{name.substr(0, underscore)});
}

void checkVhdlSemanticSignalNames(AnalysisResult& result,
                                  const core::SourceFile& source,
                                  const std::vector<parser::HdlToken>& tokens) {
    const auto units = parser::parseDesignUnits(source);
    const auto timing = parser::extractTimingInfo(source);
    for (const auto& timingUnit : timing) {
        const auto unit = std::ranges::find_if(units, [&](const auto& candidate) {
            return candidate.kind == timingUnit.kind &&
                lower(candidate.name) == lower(timingUnit.designUnit);
        });
        if (unit == units.end()) continue;
        std::set<std::string> reportedClocks;
        for (const auto& clock : timingUnit.clocks) {
            const auto clockName = lower(clock.signal);
            const auto port = std::ranges::find_if(unit->ports, [&](const auto& candidate) {
                return lower(candidate.name) == clockName;
            });
            if (port == unit->ports.end() || !reportedClocks.insert(clockName).second)
                continue;
            // Native f.26 passes mode 0 and its case-fold flag to the shared
            // naming helper at 0x1804d6e80. The shipped default is clk_. Keep
            // the source adapter to ports actually bound as edge clocks.
            // Slot 0: FUN_1805f0300; call site 0x1805f09e0..0x1805f09f9.
            if (!clockName.starts_with("clk_"))
                emit(result, "GJB9765.4.5.f.26", source,
                     port->location.line, port->location.column,
                     "实际时钟端口名称未以 clk_ 开始：" + port->name);
        }

        std::set<std::string> reportedLowActive;
        for (const auto& reset : timingUnit.resets) {
            if (!reset.activeLowObserved) continue;
            const auto resetName = lower(reset.signal);
            if (!reportedLowActive.insert(resetName).second || resetName.ends_with("_n"))
                continue;
            // Native f.28 invokes the same helper with mode 2 at both recovered
            // call sites (0x1805f1540 and 0x1805f1b5b), hence a case-insensitive
            // suffix comparison. The parser contributes only polarity-proven
            // reset/set controls; arbitrary `data = '0'` conditions stay out.
            emit(result, "GJB9765.4.5.f.28", source,
                 reset.location.line, reset.location.column,
                 "低有效信号名称未以 _n 结束：" + reset.signal);
        }
    }

    // FUN_1804d24a0 produces category 1/4 only when the source IF contains an
    // ELSIF, or an ELSE containing another linked source IF. The edge is taken
    // from that later branch; a bare `if falling_edge` with a literal body is
    // therefore not category 1. Category 2 (FUN_1804d68a0) instead requires a
    // main AST_COND recognized as the edge, then examines only a linked source
    // IF that is the first node of its branch block. Within that linked IF it
    // calls FUN_1804c48c0/FUN_1804d20e0 on direct AST_COND records. Preserve a
    // deliberately bounded lexical image of that exact shape here.
    const auto nativeNamedAssignmentValues = [&](std::size_t process) {
        std::set<std::string> result;
        std::size_t architecture = tokens.size();
        for (std::size_t token = 0; token < process; ++token) {
            if (lower(tokens[token].text) == "architecture" &&
                (token == 0 || lower(tokens[token - 1].text) != "end"))
                architecture = token;
        }
        if (architecture == tokens.size() || architecture + 3 >= tokens.size() ||
            lower(tokens[architecture + 2].text) != "of") return result;

        // FUN_1804cbad0 visits AST_GENERIC records and architecture-local
        // AST_LOCALPARAM/AST_TYPE records while building its case-folded map.
        // Mirror only declarations belonging to the entity/architecture that
        // contains this process; do not leak names across other design roots.
        const auto entityName = lower(tokens[architecture + 3].text);
        std::size_t entity = tokens.size();
        for (std::size_t token = 0; token + 1 < architecture; ++token) {
            if (lower(tokens[token].text) == "entity" &&
                (token == 0 || lower(tokens[token - 1].text) != "end") &&
                lower(tokens[token + 1].text) == entityName) entity = token;
        }
        if (entity != tokens.size()) {
            std::size_t entityEnd = architecture;
            for (auto token = entity + 2; token < architecture; ++token) {
                if (lower(tokens[token].text) != "end") continue;
                const auto following = token + 1 < architecture ?
                    lower(tokens[token + 1].text) : std::string{};
                if (following == "entity" || following == entityName ||
                    tokens[token + 1].text == ";") {
                    entityEnd = token;
                    break;
                }
            }
            for (auto token = entity + 2; token + 1 < entityEnd; ++token) {
                if (lower(tokens[token].text) != "generic" ||
                    tokens[token + 1].text != "(") continue;
                const auto close = closeParen(tokens, token + 1);
                if (close == tokens.size() || close >= entityEnd) continue;
                auto segment = token + 2;
                int depth = 0;
                for (auto item = segment; item <= close; ++item) {
                    if (tokens[item].text == "(") ++depth;
                    else if (tokens[item].text == ")" && depth > 0) --depth;
                    if (item != close && (tokens[item].text != ";" || depth != 0))
                        continue;
                    auto colon = segment;
                    while (colon < item && tokens[colon].text != ":") ++colon;
                    for (auto name = segment; name < colon; ++name)
                        if (tokens[name].kind == parser::HdlTokenKind::Identifier &&
                            tokens[name].text != ",") result.insert(lower(tokens[name].text));
                    segment = item + 1;
                }
                token = close;
            }
        }

        std::size_t declarationEnd = process;
        int subprogramDepth = 0;
        for (auto token = architecture + 4; token < process; ++token) {
            const auto word = lower(tokens[token].text);
            if (word == "end" && token + 1 < process &&
                (lower(tokens[token + 1].text) == "function" ||
                 lower(tokens[token + 1].text) == "procedure")) {
                if (subprogramDepth > 0) --subprogramDepth;
                ++token;
                continue;
            }
            if ((word == "function" || word == "procedure") &&
                (token == 0 || lower(tokens[token - 1].text) != "end")) {
                ++subprogramDepth;
                continue;
            }
            if (word == "begin" && subprogramDepth == 0) {
                declarationEnd = token;
                break;
            }
        }
        for (auto token = architecture + 4; token < declarationEnd; ++token) {
            const auto word = lower(tokens[token].text);
            if (word == "constant") {
                auto finish = token + 1;
                while (finish < declarationEnd && tokens[finish].text != ";") ++finish;
                auto colon = token + 1;
                while (colon < finish && tokens[colon].text != ":") ++colon;
                for (auto name = token + 1; name < colon; ++name)
                    if (tokens[name].kind == parser::HdlTokenKind::Identifier)
                        result.insert(lower(tokens[name].text));
                token = finish;
                continue;
            }
            if (word == "type" && token + 3 < declarationEnd &&
                lower(tokens[token + 2].text) == "is" &&
                tokens[token + 3].text == "(") {
                const auto close = closeParen(tokens, token + 3);
                if (close == tokens.size() || close >= declarationEnd) continue;
                for (auto literal = token + 4; literal < close; ++literal)
                    if (tokens[literal].kind == parser::HdlTokenKind::Identifier &&
                        tokens[literal].text != ",")
                        result.insert(lower(tokens[literal].text));
                token = close;
            }
        }
        return result;
    };
    const auto hasDirectSupportedAssignment =
        [&](std::size_t begin, std::size_t end,
            const std::set<std::string>& namedValues) {
        int structuredDepth = 0;
        for (auto token = begin; token + 1 < end; ++token) {
            const auto word = lower(tokens[token].text);
            if (word == "end" && token + 1 < end) {
                const auto ended = lower(tokens[token + 1].text);
                if ((ended == "if" || ended == "case" || ended == "loop") &&
                    structuredDepth > 0) {
                    --structuredDepth;
                    ++token;
                    continue;
                }
            }
            if (word == "if" || word == "case" || word == "loop") {
                ++structuredDepth;
                continue;
            }
            if (structuredDepth != 0) continue;
            const bool splitAssignment = token + 1 < end &&
                (tokens[token].text == "<" || tokens[token].text == ":") &&
                tokens[token + 1].text == "=";
            if (tokens[token].text != "<=" && tokens[token].text != ":=" &&
                !splitAssignment) continue;
            const auto rhs = token + (splitAssignment ? 2 : 1);
            if (rhs >= end) continue;
            const bool scalarLiteral = rhs + 2 < end &&
                tokens[rhs].text == "'" &&
                (tokens[rhs + 1].kind == parser::HdlTokenKind::Number ||
                 tokens[rhs + 1].kind == parser::HdlTokenKind::Identifier) &&
                tokens[rhs + 2].text == "'";
            const bool compactScalarLiteral = rhs + 1 < end &&
                tokens[rhs].text == "'" &&
                tokens[rhs + 1].kind == parser::HdlTokenKind::Number &&
                tokens[rhs + 1].text.ends_with("'");
            const auto scalarEnd = scalarLiteral ? rhs + 3 :
                (compactScalarLiteral ? rhs + 2 : rhs + 1);
            const bool directLiteral =
                (tokens[rhs].kind == parser::HdlTokenKind::Number ||
                 tokens[rhs].kind == parser::HdlTokenKind::String || scalarLiteral ||
                 compactScalarLiteral) &&
                scalarEnd < end && tokens[scalarEnd].text == ";";
            const bool namedValue = tokens[rhs].kind == parser::HdlTokenKind::Identifier &&
                namedValues.contains(lower(tokens[rhs].text)) &&
                rhs + 1 < end && tokens[rhs + 1].text == ";";
            if (directLiteral || namedValue) return true;
        }
        return false;
    };
    const auto conditionHasNativeRelation = [&](std::size_t begin, std::size_t end) {
        bool identifier = false;
        for (auto token = begin; token < end; ++token) {
            const auto word = lower(tokens[token].text);
            if (tokens[token].kind == parser::HdlTokenKind::Identifier) {
                if (word == "not" || word == "and" || word == "or") continue;
                // FUN_1804c48c0 does not recursively admit arbitrary calls,
                // attributes or comparison operators outside EQ.
                if ((token + 1 < end && tokens[token + 1].text == "(") ||
                    (token + 1 < end && tokens[token + 1].text == "'")) return false;
                identifier = true;
                continue;
            }
            if (tokens[token].kind == parser::HdlTokenKind::Number) {
                if (word != "0" && word != "1" && word != "0'" && word != "1'")
                    return false;
                continue;
            }
            if (tokens[token].text == "(" || tokens[token].text == ")" ||
                tokens[token].text == "=" || tokens[token].text == "'") continue;
            return false;
        }
        return identifier;
    };
    std::set<std::pair<std::size_t, std::string>> reportedFallingControls;
    for (std::size_t process = 0; process < tokens.size(); ++process) {
        if (lower(tokens[process].text) != "process" ||
            (process > 0 && lower(tokens[process - 1].text) == "end")) continue;
        const auto processEnd = vhdlEndConstruct(tokens, process, tokens.size(), "process");
        const auto namedValues = nativeNamedAssignmentValues(process);
        for (auto branch = process + 1; branch < processEnd; ++branch) {
            if (lower(tokens[branch].text) != "if") continue;
            const auto thenToken = vhdlThenToken(tokens, branch + 1, processEnd);
            if (thenToken == processEnd) continue;
            const auto branchEnd = vhdlNextBranchBoundary(
                tokens, thenToken + 1, processEnd);
            auto nestedIf = thenToken + 1;
            while (nestedIf < branchEnd && tokens[nestedIf].text == ";") ++nestedIf;
            if (nestedIf >= branchEnd || lower(tokens[nestedIf].text) != "if") continue;
            const auto nestedThen = vhdlThenToken(tokens, nestedIf + 1, branchEnd);
            if (nestedThen == branchEnd ||
                !conditionHasNativeRelation(nestedIf + 1, nestedThen)) continue;
            const auto nestedBranchEnd = vhdlNextBranchBoundary(
                tokens, nestedThen + 1, branchEnd);
            if (!hasDirectSupportedAssignment(
                    nestedThen + 1, nestedBranchEnd, namedValues)) continue;
            for (auto token = branch + 1; token + 3 < thenToken; ++token) {
                if (lower(tokens[token].text) != "falling_edge" ||
                    tokens[token + 1].text != "(" ||
                    tokens[token + 2].kind != parser::HdlTokenKind::Identifier ||
                    tokens[token + 3].text != ")") continue;
                const auto name = lower(tokens[token + 2].text);
                if (name.ends_with("_n") ||
                    !reportedFallingControls.emplace(tokens[token + 2].line, name).second)
                    continue;
                emit(result, "GJB9765.4.5.f.28", source,
                     tokens[token + 2].line, tokens[token + 2].column,
                     "低有效信号名称未以 _n 结束：" + tokens[token + 2].text);
            }
        }
        process = processEnd > process ? processEnd : process;
    }

    // Native f.29 recursively collects type-0x56 assignments and admits only
    // an identifier destination plus an AST_BIT_NOT (0x24) containing exactly
    // one identifier. It does not restrict that shape to the concurrent region.
    // The shipped HTML additionally marks direct buffers as violations, despite
    // their absence from FUN_1805f21d0; retain that documented extension only
    // for complete concurrent assignments and keep it separate from the native
    // inverter core.
    const auto simple = [&](std::size_t index) {
        return index < tokens.size() &&
            tokens[index].kind == parser::HdlTokenKind::Identifier &&
            tokens[index].offset < source.content().size() &&
            source.content()[tokens[index].offset] != '\\';
    };
    unsigned architectureDepth = 0;
    unsigned processDepth = 0;
    unsigned subprogramDepth = 0;
    bool architectureBody = false;
    std::string architectureName;
    for (std::size_t i = 0; i < tokens.size(); ++i) {
        const auto word = lower(tokens[i].text);
        if (word == "end" && i + 1 < tokens.size()) {
            const auto ended = lower(tokens[i + 1].text);
            if (ended == "process" && processDepth) --processDepth;
            else if ((ended == "function" || ended == "procedure") && subprogramDepth)
                --subprogramDepth;
            else if (architectureDepth && !processDepth && !subprogramDepth &&
                     (ended == "architecture" || ended == architectureName)) {
                --architectureDepth;
                architectureBody = false;
                architectureName.clear();
            }
            continue;
        }
        if (word == "architecture" && (i == 0 || lower(tokens[i - 1].text) != "end")) {
            ++architectureDepth;
            architectureName = i + 1 < tokens.size() ? lower(tokens[i + 1].text) : "";
            architectureBody = false;
            continue;
        }
        if (!architectureDepth) continue;
        if (word == "begin" && processDepth == 0 && subprogramDepth == 0)
            architectureBody = true;
        if (word == "process" && (i == 0 || lower(tokens[i - 1].text) != "end")) {
            ++processDepth;
            continue;
        }
        if ((word == "function" || word == "procedure") &&
            (i == 0 || lower(tokens[i - 1].text) != "end")) {
            ++subprogramDepth;
            continue;
        }
        if (tokens[i].text != "<=" || i == 0 || !simple(i - 1)) continue;
        std::size_t rhs = i + 1;
        const bool inverter = rhs < tokens.size() && lower(tokens[rhs].text) == "not";
        if (inverter) ++rhs;
        if (!simple(rhs) || rhs + 1 >= tokens.size() || tokens[rhs + 1].text != ";")
            continue;
        if (!inverter && (!architectureBody || processDepth || subprogramDepth))
            continue;
        const auto& destination = tokens[i - 1];
        const auto& sourceName = tokens[rhs];
        if (nativeVhdlSignalBase(destination.text) == nativeVhdlSignalBase(sourceName.text))
            continue;
        emit(result, "GJB9765.4.5.f.29", source,
             destination.line, destination.column,
             std::string(inverter ? "反相" : "缓冲") +
                 "赋值两端的信号基名不同：" + destination.text + " / " +
                 sourceName.text);
    }
}

void checkVhdlStatementLabelSuffixes(AnalysisResult& result,
                                     const core::SourceFile& source,
                                     const std::vector<parser::HdlToken>& tokens) {
    const auto report = [&](std::size_t label, std::string_view clause,
                            std::string_view suffix, std::string_view kind) {
        if (tokens[label].kind != parser::HdlTokenKind::Identifier ||
            tokens[label].offset >= source.content().size() ||
            source.content()[tokens[label].offset] == '\\') return;
        if (!lower(tokens[label].text).ends_with(suffix))
            emit(result, clause, source, tokens[label].line, tokens[label].column,
                 std::string(kind) + "标号未以 " + std::string(suffix) +
                 " 结束：" + tokens[label].text);
    };
    for (std::size_t i = 0; i + 2 < tokens.size(); ++i) {
        if (tokens[i].kind != parser::HdlTokenKind::Identifier ||
            tokens[i + 1].text != ":") continue;
        const auto construct = lower(tokens[i + 2].text);
        if (construct == "block") {
            report(i, "GJB9765.4.5.f.18", "_block", "块");
            continue;
        }
        if (construct == "process" ||
            (construct == "postponed" && i + 3 < tokens.size() &&
             lower(tokens[i + 3].text) == "process")) {
            report(i, "GJB9765.4.5.f.20", "_proc", "进程");
            continue;
        }
        if (construct != "for" && construct != "if" && construct != "case") continue;
        int depth = 0;
        for (auto cursor = i + 3; cursor < tokens.size(); ++cursor) {
            if (tokens[cursor].text == "(") ++depth;
            else if (tokens[cursor].text == ")" && depth > 0) --depth;
            if (depth == 0 && lower(tokens[cursor].text) == "generate") {
                report(i, "GJB9765.4.5.f.19", "_gen", "generate");
                break;
            }
            if (depth == 0 && tokens[cursor].text == ";") break;
        }
    }
}

// Independent, conservative source admission. This is not the complete native
// frontend: preprocessing, escaped names, malformed/nested declarations and
// duplicate module bindings remain review-required. Use the module KEYWORD's
// physical line, not a possibly multiline module name or a comment match.
void configuredVerilogHeaderChecks(AnalysisResult& result, const core::SourceFile& source,
                                   const std::vector<parser::HdlToken>& tokens,
                                   const VerilogHeaderPolicy& policy) {
    if (policy.requirements.empty()) return;
    if (!parser::checkSyntax(source).empty()) return;
    const auto word = [&](std::size_t i, std::string_view text) {
        return i < tokens.size() && tokens[i].kind == parser::HdlTokenKind::Identifier &&
            tokens[i].text == text && tokens[i].offset < source.content().size() &&
            source.content()[tokens[i].offset] != '\\';
    };
    std::vector<std::pair<std::size_t, std::size_t>> modules;
    std::optional<std::pair<std::size_t, std::size_t>> current;
    for (std::size_t i = 0; i < tokens.size(); ++i) {
        if (tokens[i].text == "`" || word(i, "package") || word(i, "interface") ||
            word(i, "class") || word(i, "program")) return;
        if (word(i, "module")) {
            if (current) return;
            auto name = i + 1;
            if (word(name, "automatic") || word(name, "static")) ++name;
            if (name + 1 >= tokens.size() || !verilog_shape_detail::simple(tokens[name]) ||
                source.content()[tokens[name].offset] == '\\' ||
                (tokens[name + 1].text != "(" && tokens[name + 1].text != "#" &&
                 tokens[name + 1].text != ";")) return;
            current = std::pair{i, name};
        } else if (word(i, "endmodule")) {
            if (!current) return;
            modules.push_back(*current);
            current.reset();
        }
    }
    if (current) return;
    for (const auto& [start, name] : modules) {
        const auto moduleName = tokens[name].text;
        const auto bindings = std::ranges::count_if(result.structure, [&](const auto& unit) {
            return unit.kind == parser::DesignUnitKind::VerilogModule && unit.name == moduleName;
        });
        if (bindings != 1) continue;
        const auto checked = evaluateVerilogHeaderPolicy(source, tokens[start].line, policy);
        if (checked.status != VerilogHeaderPolicyStatus::Complete) continue;
        if (!checked.missingHeader && checked.missingKeywords.empty()) continue;
        std::string message = checked.missingHeader ?
            "文件缺少头部注释" : "文件头部注释中缺少\"";
        if (!checked.missingHeader) {
            for (std::size_t i = 0; i < checked.missingKeywords.size(); ++i) {
                if (i) message += "、";
                message += checked.missingKeywords[i];
            }
            message += "\"的注释声明";
        }
        // The native record stores the module physical line but no column.
        emit(result, "GJB10157.5.1.1", source, tokens[start].line, 1,
             std::move(message));
    }
}

void configuredVhdlHeaderChecks(AnalysisResult& result, const core::SourceFile& source,
                                const std::vector<parser::HdlToken>& tokens,
                                const VerilogHeaderPolicy& policy) {
    if (policy.requirements.empty()) return;
    if (!parser::checkSyntax(source).empty()) return;
    const auto declaration = std::ranges::find_if(tokens, [](const auto& token) {
        if (token.kind != parser::HdlTokenKind::Identifier) return false;
        const auto text = lower(token.text);
        return text == "library" || text == "use" || text == "entity";
    });
    if (declaration == tokens.end()) return;
    const auto checked = evaluateVhdlHeaderPolicy(source, declaration->line, policy);
    if (checked.status != VerilogHeaderPolicyStatus::Complete) return;
    if (!checked.missingHeader && checked.missingKeywords.empty()) return;
    const bool documentedCommentFallback = policy.requirements.size() == 1 &&
        policy.requirements.front().keyword == "--";
    std::string message = documentedCommentFallback ?
        "文件开头缺少规则要求的 VHDL 英文注释" : checked.missingHeader ?
        "文件首个设计单元前没有非空物理行，未满足显式文件头策略" :
        "显式文件头策略缺少关键字：";
    if (documentedCommentFallback) {
        emit(result, "GJB9765.4.13.a", source, declaration->line, declaration->column,
             std::move(message));
        return;
    }
    for (std::size_t i = 0; i < checked.missingKeywords.size(); ++i) {
        if (i) message += "、";
        message += checked.missingKeywords[i];
    }
    emit(result, "GJB9765.4.13.a", source, declaration->line, declaration->column,
         std::move(message));
}

std::vector<std::pair<std::string, core::SourceLocation>> declaredNames(
    const AnalysisResult& result, const core::SourceFile& source) {
    std::vector<std::pair<std::string, core::SourceLocation>> names;
    for (const auto& unit : result.structure) {
        if (unit.location.file.lexically_normal() != source.path().lexically_normal()) continue;
        names.emplace_back(unit.name, unit.location);
        for (const auto& port : unit.ports) names.emplace_back(port.name, port.location);
        for (const auto& parameter : unit.parameters) names.emplace_back(parameter.name, parameter.location);
        for (const auto& instance : unit.instances) names.emplace_back(instance.name, instance.location);
    }
    return names;
}

void caseOnlyDeclarationChecks(AnalysisResult& result, const core::SourceFile& source,
                               const std::vector<parser::HdlToken>& tokens,
                               const VerilogNamingShapeScan& scan) {
    // Native 5.2.6 walks each AST_MODULE independently and collects MODULE,
    // WIRE, PARAMETER, LOCALPARAM, CELL and TASK (not FUNCTION). Its core
    // predicate is unequal original names with equal lowercase names. The
    // module-name map admits only groups containing exactly one AST_MODULE.
    // ev_9f4b850ac513b5a9f63dffbc54f518d21028ea2923558edeee5753c9452398df
    // ev_512c915b90a165536016fafbb5a5ded6cfba13c0084ad97b3576b7906ee5547b
    std::map<std::size_t, const VerilogNamingShapeDeclaration*> declarations;
    std::map<std::string, std::size_t> moduleMultiplicity;
    for (const auto& declaration : scan.declarations) {
        if (declaration.subject == VerilogNamingShapeSubject::Module)
            ++moduleMultiplicity[declaration.moduleName];
        if (declaration.subject == VerilogNamingShapeSubject::Function ||
            declaration.subject == VerilogNamingShapeSubject::Unsupported) continue;
        declarations.emplace(declaration.tokenIndex, &declaration);
    }
    bool inModule = false;
    std::map<std::string, std::string> seen;
    for (std::size_t index = 0; index < tokens.size(); ++index) {
        if (tokens[index].text == "module") {
            inModule = true;
            seen.clear();
        } else if (tokens[index].text == "endmodule") {
            inModule = false;
            seen.clear();
        }
        if (!inModule) continue;
        const auto found = declarations.find(index);
        if (found == declarations.end()) continue;
        const auto& declaration = *found->second;
        const auto owner = moduleMultiplicity.find(declaration.moduleName);
        if (owner == moduleMultiplicity.end() || owner->second != 1) continue;
        const auto& name = declaration.normalizedName;
        const auto key = lower(name);
        const auto prior = seen.find(key);
        if (prior != seen.end() && prior->second != name)
            emit(result, "GJB10157.5.2.6", source, tokens[index].line, 1,
                 "禁止仅使用大小写区分命名" + name + "和" + prior->second);
        else if (prior == seen.end()) seen.emplace(key, name);
    }
}

// Native GJB9765.4.12.a entry FUN_1805b9590 and recursive collector
// FUN_1805ba200 expose three independent candidates:
//   * direct children of AST_GENERIC(0xa3) whose integer flags +0x109/+0x110
//     are both clear;
//   * string constants below an AST_LOCALPARAM/constant node (0x0a, +0xa6)
//     whose direct literal is exactly "\"\"";
//   * AST_WAIT(0x7c) whose first child is AST_FOR(0x5c).
// The source adapter admits standard, unambiguously non-integer generic type
// names. User-defined subtypes remain unresolved rather than being guessed.
void checkVhdlUnsynthesizableConstructs(AnalysisResult& result,
                                        const core::SourceFile& source,
                                        const std::vector<parser::HdlToken>& tokens) {
    const auto knownNonIntegerGenericType = [](std::string_view type) {
        static constexpr std::array names{
            std::string_view{"bit"}, std::string_view{"bit_vector"},
            std::string_view{"boolean"}, std::string_view{"character"},
            std::string_view{"real"}, std::string_view{"signed"},
            std::string_view{"std_logic"}, std::string_view{"std_logic_vector"},
            std::string_view{"std_ulogic"}, std::string_view{"std_ulogic_vector"},
            std::string_view{"string"}, std::string_view{"time"},
            std::string_view{"ufixed"}, std::string_view{"unsigned"},
            std::string_view{"sfixed"}
        };
        return std::ranges::find(names, type) != names.end();
    };

    for (std::size_t i = 0; i + 3 < tokens.size(); ++i) {
        if (lower(tokens[i].text) != "entity" ||
            lower(tokens[i + 2].text) != "is") continue;
        for (std::size_t j = i + 3; j + 1 < tokens.size(); ++j) {
            const auto word = lower(tokens[j].text);
            if (word == "port" || word == "end" || word == "architecture") break;
            if (word != "generic" || tokens[j + 1].text != "(") continue;
            std::size_t depth = 1;
            bool declarationStart = true;
            for (std::size_t k = j + 2; k < tokens.size() && depth > 0; ++k) {
                if (tokens[k].text == "(") { ++depth; continue; }
                if (tokens[k].text == ")") { --depth; continue; }
                if (depth != 1) continue;
                if (tokens[k].text == ";") { declarationStart = true; continue; }
                if (tokens[k].text != ":" || !declarationStart || k + 1 >= tokens.size()) continue;
                declarationStart = false;
                const auto type = lower(tokens[k + 1].text);
                if (knownNonIntegerGenericType(type))
                    emit(result, "GJB9765.4.12.a", source, tokens[k + 1].line,
                         tokens[k + 1].column,
                         "非整型 generic 不可综合：" + type);
            }
            break;
        }
    }

    for (std::size_t i = 0; i < tokens.size(); ++i) {
        const auto word = lower(tokens[i].text);
        if (word == "wait" && i + 1 < tokens.size() &&
            lower(tokens[i + 1].text) == "for") {
            emit(result, "GJB9765.4.12.a", source, tokens[i].line, tokens[i].column,
                 "wait for 语句不可综合");
            continue;
        }
        if (word != "constant") continue;
        auto end = i + 1;
        while (end < tokens.size() && tokens[end].text != ";") ++end;
        if (end == tokens.size()) continue;
        auto initializer = i + 1;
        while (initializer < end && tokens[initializer].text != ":=") ++initializer;
        if (initializer + 2 == end &&
            tokens[initializer + 1].kind == parser::HdlTokenKind::String &&
            tokens[initializer + 1].text == "\"\"") {
            emit(result, "GJB9765.4.12.a", source,
                 tokens[initializer + 1].line, tokens[initializer + 1].column,
                 "空字符串常量不可综合");
        }
        i = end;
    }
}

std::optional<std::string> nativeForbiddenVerilogType(std::string_view text) {
    // Native FUN_18053f190 visits direct module AST children. AST_WIRE (type
    // 6) has one diagnostic branch for each of these thirteen flags; type 10
    // (AST_LOCALPARAM) has a separate unconditional diagnostic. Type 9
    // (AST_PARAMETER) is explicitly skipped. The strings below were recovered
    // from the branch-specific diagnostic objects, not inferred from the rule
    // title. ev_a41348d4e435f12ae7e47a8a38453e7a1873d90fead3ec03f1cd1745ad93953f
    static constexpr std::array forbidden{
        std::string_view{"time"}, std::string_view{"realtime"},
        std::string_view{"tri0"}, std::string_view{"tri1"},
        std::string_view{"triand"}, std::string_view{"trior"},
        std::string_view{"trireg"}, std::string_view{"real"},
        std::string_view{"supply0"}, std::string_view{"supply1"},
        std::string_view{"uwire"}, std::string_view{"wand"},
        std::string_view{"wor"},
    };
    const auto word = lower(std::string(text));
    if (std::ranges::find(forbidden, word) == forbidden.end()) return std::nullopt;
    return word;
}

bool verilogDeclarationQualifier(std::string_view text) {
    static constexpr std::array qualifiers{
        std::string_view{"parameter"}, std::string_view{"localparam"},
        std::string_view{"input"}, std::string_view{"output"},
        std::string_view{"inout"}, std::string_view{"ref"},
        std::string_view{"wire"}, std::string_view{"reg"},
        std::string_view{"integer"}, std::string_view{"tri"},
        std::string_view{"logic"}, std::string_view{"bit"},
        std::string_view{"byte"}, std::string_view{"shortint"},
        std::string_view{"int"}, std::string_view{"longint"},
        std::string_view{"signed"}, std::string_view{"unsigned"},
        std::string_view{"var"}, std::string_view{"const"},
        std::string_view{"automatic"}, std::string_view{"static"},
        std::string_view{"type"},
    };
    const auto word = lower(std::string(text));
    return std::ranges::find(qualifiers, word) != qualifiers.end() ||
           nativeForbiddenVerilogType(word).has_value();
}

const parser::HdlToken* verilogDeclaredName(
    const std::vector<parser::HdlToken>& tokens, std::size_t begin, std::size_t end) {
    int paren = 0;
    int bracket = 0;
    int brace = 0;
    for (auto i = begin; i < end; ++i) {
        const auto& token = tokens[i];
        if (token.text == "(") { ++paren; continue; }
        if (token.text == ")") { if (paren > 0) --paren; continue; }
        if (token.text == "[") { ++bracket; continue; }
        if (token.text == "]") { if (bracket > 0) --bracket; continue; }
        if (token.text == "{") { ++brace; continue; }
        if (token.text == "}") { if (brace > 0) --brace; continue; }
        if (paren != 0 || bracket != 0 || brace != 0) continue;
        if (token.text == "=") break;
        if (token.kind == parser::HdlTokenKind::Identifier &&
            !verilogDeclarationQualifier(token.text)) return &token;
    }
    return nullptr;
}

enum class VerilogDeclarationListKind { Parameter, Port, Body };

void checkNativeVerilogDeclarationList(
    AnalysisResult& result, const core::SourceFile& source,
    const std::vector<parser::HdlToken>& tokens, std::size_t begin,
    std::size_t end, VerilogDeclarationListKind kind) {
    std::optional<std::string> inheritedForbidden;
    bool inheritedLocalparam = false;
    std::size_t segment = begin;
    int paren = 0;
    int bracket = 0;
    int brace = 0;
    const auto inspect = [&](std::size_t first, std::size_t last) {
        if (kind == VerilogDeclarationListKind::Body) {
            for (auto i = first; i < last; ++i) {
                if (tokens[i].kind != parser::HdlTokenKind::Identifier) continue;
                const auto firstWord = lower(tokens[i].text);
                if (firstWord == "typedef" || firstWord == "nettype") return;
                break;
            }
        }
        bool localparam = inheritedLocalparam;
        auto forbidden = inheritedForbidden;
        bool declaration = kind != VerilogDeclarationListKind::Body ||
                           localparam || forbidden.has_value();
        int nestedParen = 0;
        int nestedBracket = 0;
        int nestedBrace = 0;
        for (auto i = first; i < last; ++i) {
            const auto& token = tokens[i];
            if (token.text == "(") { ++nestedParen; continue; }
            if (token.text == ")") { if (nestedParen > 0) --nestedParen; continue; }
            if (token.text == "[") { ++nestedBracket; continue; }
            if (token.text == "]") { if (nestedBracket > 0) --nestedBracket; continue; }
            if (token.text == "{") { ++nestedBrace; continue; }
            if (token.text == "}") { if (nestedBrace > 0) --nestedBrace; continue; }
            if (nestedParen != 0 || nestedBracket != 0 || nestedBrace != 0) continue;
            if (token.text == "=") break;
            const auto word = lower(token.text);
            if (word == "localparam") {
                declaration = true;
                localparam = true;
                forbidden.reset();
            } else if (word == "parameter") {
                declaration = true;
                localparam = false;
                forbidden.reset();
            } else if (word == "input" || word == "output" || word == "inout" || word == "ref") {
                declaration = true;
                localparam = false;
                forbidden.reset();
            } else if (const auto recovered = nativeForbiddenVerilogType(word)) {
                declaration = true;
                forbidden = recovered;
            } else if (word == "wire" || word == "reg" || word == "integer" || word == "tri" ||
                       word == "logic" || word == "bit" || word == "byte" ||
                       word == "shortint" || word == "int" || word == "longint" || word == "type") {
                forbidden.reset();
            }
        }
        if (!declaration) return;
        inheritedLocalparam = localparam;
        inheritedForbidden = forbidden;
        const auto* name = verilogDeclaredName(tokens, first, last);
        if (!name) return;
        const auto reportedType = localparam ? std::string{"localparam"} :
                                  forbidden.value_or(std::string{});
        if (reportedType.empty()) return;
        const auto message = localparam
            ? "禁止使用除reg、wire、integer、tri和parameter之外的数据类型，" +
                  name->text + "是localparam类型违反."
            : "禁止使用除reg、wire、integer、tri和parameter之外的数据类型，信号" +
                  name->text + "是" + reportedType + "类型违反.";
        // The native record carries the candidate's physical line but no column.
        emit(result, "GJB10157.5.2.8", source, name->line, 1, message);
    };

    for (auto i = begin; i <= end; ++i) {
        if (i == end || (tokens[i].text == "," && paren == 0 && bracket == 0 && brace == 0)) {
            inspect(segment, i);
            segment = i + 1;
            continue;
        }
        if (tokens[i].text == "(") ++paren;
        else if (tokens[i].text == ")" && paren > 0) --paren;
        else if (tokens[i].text == "[") ++bracket;
        else if (tokens[i].text == "]" && bracket > 0) --bracket;
        else if (tokens[i].text == "{") ++brace;
        else if (tokens[i].text == "}" && brace > 0) --brace;
    }
}

void checkNativeVerilogForbiddenTypes(AnalysisResult& result,
                                      const core::SourceFile& source,
                                      const std::vector<parser::HdlToken>& tokens) {
    for (std::size_t module = 0; module < tokens.size(); ++module) {
        if (lower(tokens[module].text) != "module") continue;
        auto endmodule = module + 1;
        while (endmodule < tokens.size() && lower(tokens[endmodule].text) != "endmodule")
            ++endmodule;
        if (endmodule == tokens.size()) break;

        auto headerEnd = module + 1;
        int headerParen = 0;
        for (; headerEnd < endmodule; ++headerEnd) {
            if (tokens[headerEnd].text == "(") ++headerParen;
            else if (tokens[headerEnd].text == ")" && headerParen > 0) --headerParen;
            else if (tokens[headerEnd].text == ";" && headerParen == 0) break;
        }
        if (headerEnd == endmodule) { module = endmodule; continue; }

        std::size_t parameterClose = tokens.size();
        for (auto i = module + 1; i + 1 < headerEnd; ++i) {
            if (tokens[i].text != "#" || tokens[i + 1].text != "(") continue;
            parameterClose = closeParen(tokens, i + 1);
            if (parameterClose < headerEnd)
                checkNativeVerilogDeclarationList(result, source, tokens, i + 2,
                    parameterClose, VerilogDeclarationListKind::Parameter);
            break;
        }
        for (auto i = module + 1; i < headerEnd; ++i) {
            if (tokens[i].text != "(" || (i > 0 && tokens[i - 1].text == "#")) continue;
            if (parameterClose != tokens.size() && i < parameterClose) continue;
            const auto close = closeParen(tokens, i);
            if (close < headerEnd)
                checkNativeVerilogDeclarationList(result, source, tokens, i + 1,
                    close, VerilogDeclarationListKind::Port);
            break;
        }

        std::size_t statement = headerEnd + 1;
        int nested = 0;
        int paren = 0;
        int brace = 0;
        for (auto i = headerEnd + 1; i < endmodule; ++i) {
            const auto word = lower(tokens[i].text);
            if (tokens[i].text == "(") ++paren;
            else if (tokens[i].text == ")" && paren > 0) --paren;
            else if (tokens[i].text == "{") ++brace;
            else if (tokens[i].text == "}" && brace > 0) --brace;
            if (paren == 0 && brace == 0) {
                if (word == "begin" || word == "fork" || word == "generate" ||
                    word == "function" || word == "task" || word == "class") {
                    ++nested;
                } else if ((word == "end" || word == "join" || word == "join_any" ||
                            word == "join_none" || word == "endgenerate" ||
                            word == "endfunction" || word == "endtask" || word == "endclass") &&
                           nested > 0) {
                    --nested;
                    if (nested == 0) statement = i + 1;
                }
            }
            if (tokens[i].text != ";" || paren != 0 || brace != 0) continue;
            if (nested == 0)
                checkNativeVerilogDeclarationList(result, source, tokens, statement, i,
                    VerilogDeclarationListKind::Body);
            statement = i + 1;
        }
        module = endmodule;
    }
}

void checkVhdlResetNames(AnalysisResult& result, const core::SourceFile& source,
                         const std::vector<parser::HdlToken>& tokens) {
    // Native GJB9765.4.5.f.30 builds a case-folded signal-name universe and a
    // semantic reset relation set. FUN_1805f6790 links direct identifier and
    // unary-NOT assignments; FUN_1805f6590 walks the relation graph. The run
    // then applies case-insensitive mode-0 (prefix) matching to reset signals
    // and mode-1 (substring) matching to non-reset signals. The packaged rule
    // and examples establish the documented policy pattern as "rst". The
    // recovered graph is directed: architecture-direct assignment target ->
    // direct/NOT source. Reset seeds walk upstream only; nested process,
    // subprogram, block and generate assignments are not graph edges.
    // ev_3794492787079f7fca6b3155ec6e110f983b320b8444223e61f3d8e7e1574696
    // Native helpers: FUN_1805f6590, FUN_1805f6790, FUN_1804d6e80.
    struct ArchitectureScope {
        std::string entity;
        std::size_t declaration{};
        std::size_t body{};
        std::size_t end{};
    };
    std::vector<ArchitectureScope> scopes;
    for (std::size_t i = 0; i + 4 < tokens.size(); ++i) {
        if (lower(tokens[i].text) != "architecture" ||
            tokens[i + 1].kind != parser::HdlTokenKind::Identifier ||
            lower(tokens[i + 2].text) != "of" ||
            tokens[i + 3].kind != parser::HdlTokenKind::Identifier) continue;
        auto body = i + 4;
        while (body < tokens.size() && lower(tokens[body].text) != "begin") ++body;
        if (body == tokens.size()) continue;
        auto end = body + 1;
        const auto architectureName = lower(tokens[i + 1].text);
        while (end + 1 < tokens.size() &&
               !(lower(tokens[end].text) == "end" &&
                 (lower(tokens[end + 1].text) == "architecture" ||
                  lower(tokens[end + 1].text) == architectureName))) ++end;
        scopes.push_back({lower(tokens[i + 3].text), i, body,
                          std::min(end, tokens.size())});
        i = end > i ? end : i;
    }
    if (scopes.empty()) return;

    const auto timingUnits = parser::extractTimingInfo(source);
    std::set<std::tuple<std::size_t, std::size_t, bool, std::string>> reported;
    for (const auto& scope : scopes) {
        std::vector<const parser::DesignUnitInfo*> unitMatches;
        for (const auto& candidate : result.structure) {
            if (candidate.kind == parser::DesignUnitKind::VhdlEntity &&
                lower(candidate.name) == scope.entity)
                unitMatches.push_back(&candidate);
        }
        // Native receives an already-bound design root containing both its
        // AST_PORT and AST_ARCHITECTURE children. Reconstruct a split-file root
        // only when project-level entity binding is unique; duplicate library
        // units remain unresolved rather than being guessed by source order.
        if (unitMatches.size() != 1) continue;
        const auto* unit = unitMatches.front();

        std::map<std::string, core::SourceLocation> candidates;
        for (const auto& port : unit->ports)
            candidates.try_emplace(lower(port.name), port.location);

        // The native relation builder receives one design root and visits only
        // its architecture's direct declarations/assignments. Preserve that
        // ownership in multi-unit source files instead of merging file-global
        // names or disabling propagation altogether.
        for (auto i = scope.declaration + 4; i < scope.body; ++i) {
            if (lower(tokens[i].text) != "signal") continue;
            for (auto cursor = i + 1; cursor < scope.body; ++cursor) {
                if (tokens[cursor].text == ":") break;
                if (tokens[cursor].text == ";" || lower(tokens[cursor].text) == "begin")
                    break;
                if (tokens[cursor].kind == parser::HdlTokenKind::Identifier)
                    candidates.try_emplace(lower(tokens[cursor].text), core::SourceLocation{
                        source.path(), tokens[cursor].line, tokens[cursor].column,
                        tokens[cursor].offset});
            }
        }

        std::set<std::string> resetNames;
        const auto firstLine = tokens[scope.declaration].line;
        const auto lastLine = scope.end < tokens.size() ? tokens[scope.end].line
                                                        : tokens.back().line;
        for (const auto& timing : timingUnits) {
            if (timing.kind != parser::DesignUnitKind::VhdlEntity ||
                lower(timing.designUnit) != scope.entity) continue;
            for (const auto& reset : timing.resets) {
                if (reset.location.line < firstLine || reset.location.line > lastLine)
                    continue;
                const auto name = lower(reset.signal);
                resetNames.insert(name);
                candidates.try_emplace(name, reset.location);
            }
        }

        // Supplement the timing summary with the canonical asynchronous form
        // while remaining inside this architecture root.
        for (auto i = scope.body + 1; i + 4 < scope.end; ++i) {
            if (lower(tokens[i].text) != "if" ||
                tokens[i + 1].kind != parser::HdlTokenKind::Identifier ||
                (tokens[i + 2].text != "=" && tokens[i + 2].text != "/=")) continue;
            auto thenToken = i + 3;
            while (thenToken < scope.end && thenToken - i < 10 &&
                   lower(tokens[thenToken].text) != "then") ++thenToken;
            if (thenToken >= scope.end || thenToken - i >= 10) continue;
            bool edgeBranch = false;
            for (auto cursor = thenToken + 1; cursor + 1 < scope.end; ++cursor) {
                if (lower(tokens[cursor].text) == "end" &&
                    lower(tokens[cursor + 1].text) == "if") break;
                if (lower(tokens[cursor].text) != "elsif") continue;
                for (auto branch = cursor + 1;
                     branch < scope.end && branch - cursor < 10; ++branch) {
                    const auto word = lower(tokens[branch].text);
                    if (word == "rising_edge" || word == "falling_edge")
                        edgeBranch = true;
                    if (word == "then") break;
                }
                break;
            }
            if (!edgeBranch) continue;
            const auto name = lower(tokens[i + 1].text);
            resetNames.insert(name);
            candidates.try_emplace(name, core::SourceLocation{
                source.path(), tokens[i + 1].line, tokens[i + 1].column,
                tokens[i + 1].offset});
        }

        std::map<std::string, std::set<std::string>> dependencies;
        unsigned processDepth = 0;
        unsigned subprogramDepth = 0;
        unsigned generateDepth = 0;
        unsigned blockDepth = 0;
        for (auto i = scope.body + 1; i + 3 < scope.end; ++i) {
            const auto word = lower(tokens[i].text);
            if (word == "end" && i + 1 < scope.end) {
                const auto ended = lower(tokens[i + 1].text);
                if (ended == "process" && processDepth) --processDepth;
                else if ((ended == "function" || ended == "procedure") && subprogramDepth)
                    --subprogramDepth;
                else if (ended == "generate" && generateDepth) --generateDepth;
                else if (ended == "block" && blockDepth) --blockDepth;
                continue;
            }
            if (word == "process" && (i == 0 || lower(tokens[i - 1].text) != "end")) {
                ++processDepth;
                continue;
            }
            if ((word == "function" || word == "procedure") &&
                (i == 0 || lower(tokens[i - 1].text) != "end")) {
                ++subprogramDepth;
                continue;
            }
            if (word == "generate" && (i == 0 || lower(tokens[i - 1].text) != "end")) {
                ++generateDepth;
                continue;
            }
            if (word == "block" && i > 0 && tokens[i - 1].text == ":") {
                ++blockDepth;
                continue;
            }
            if (processDepth || subprogramDepth || generateDepth || blockDepth ||
                tokens[i].kind != parser::HdlTokenKind::Identifier ||
                tokens[i + 1].text != "<=") continue;
            auto rhs = i + 2;
            if (lower(tokens[rhs].text) == "not") ++rhs;
            if (rhs + 1 >= scope.end ||
                tokens[rhs].kind != parser::HdlTokenKind::Identifier ||
                tokens[rhs + 1].text != ";") continue;
            const auto left = lower(tokens[i].text);
            const auto right = lower(tokens[rhs].text);
            if (!candidates.contains(left) || !candidates.contains(right)) continue;
            dependencies[left].insert(right);
        }
        std::vector<std::string> pending(resetNames.begin(), resetNames.end());
        for (std::size_t cursor = 0; cursor < pending.size(); ++cursor) {
            const auto found = dependencies.find(pending[cursor]);
            if (found == dependencies.end()) continue;
            for (const auto& adjacent : found->second)
                if (resetNames.insert(adjacent).second) pending.push_back(adjacent);
        }

        for (const auto& [name, location] : candidates) {
            const bool reset = resetNames.contains(name);
            if ((reset && name.starts_with("rst")) ||
                (!reset && name.find("rst") == std::string::npos)) continue;
            if (!reported.emplace(location.line, location.column, reset, name).second)
                continue;
            if (reset) {
                emitAtLocation(result, "GJB9765.4.5.f.30", location,
                               "已识别复位信号名称未以 rst 开始：" + name);
            } else {
                emitAtLocation(result, "GJB9765.4.5.f.30", location,
                               "非复位信号名称不应包含 rst：" + name);
            }
        }
    }
}

bool nativeFullCaseHotComment(std::string_view body) {
    std::size_t cursor = 0;
    while (cursor < body.size() && std::isspace(static_cast<unsigned char>(body[cursor])))
        ++cursor;
    constexpr std::array tags{std::string_view{"synopsys"}, std::string_view{"synthesis"}};
    const auto tag = std::ranges::find_if(tags, [&](const auto candidate) {
        return body.substr(cursor).starts_with(candidate);
    });
    if (tag == tags.end()) return false;
    cursor += tag->size();
    if (cursor == body.size() || !std::isspace(static_cast<unsigned char>(body[cursor])))
        return false;
    while (cursor < body.size() && std::isspace(static_cast<unsigned char>(body[cursor])))
        ++cursor;
    // The recovered Yosys lexer action is entered as soon as the literal
    // `full_case` prefix wins in SYNOPSYS_FLAGS; the remainder of the hot
    // comment is subsequently ignored by that lexer state.
    return body.substr(cursor).starts_with("full_case");
}

bool nativeExplicitFullCaseAttribute(std::string_view body) {
    // Verilog attr_spec is attr_name [= constant_expression]. Inspect only the
    // key at the start of each top-level comma-separated spec. This avoids the
    // old false positive for (* note = "full_case" *).
    std::size_t cursor = 0;
    while (cursor < body.size()) {
        while (cursor < body.size() &&
               (std::isspace(static_cast<unsigned char>(body[cursor])) || body[cursor] == ','))
            ++cursor;
        if (cursor == body.size()) break;
        const auto keyStart = cursor;
        if (body[cursor] == '\\') {
            ++cursor;
            while (cursor < body.size() &&
                   !std::isspace(static_cast<unsigned char>(body[cursor])) &&
                   body[cursor] != ',' && body[cursor] != '=') ++cursor;
        } else {
            while (cursor < body.size() &&
                   (std::isalnum(static_cast<unsigned char>(body[cursor])) ||
                    body[cursor] == '_' || body[cursor] == '$')) ++cursor;
        }
        const auto key = body.substr(keyStart, cursor - keyStart);
        if (key == "full_case" || key == "\\full_case") return true;

        unsigned paren = 0, bracket = 0, brace = 0;
        bool quoted = false, escaped = false;
        for (; cursor < body.size(); ++cursor) {
            const char ch = body[cursor];
            if (quoted) {
                if (escaped) escaped = false;
                else if (ch == '\\') escaped = true;
                else if (ch == '"') quoted = false;
                continue;
            }
            if (ch == '"') { quoted = true; continue; }
            if (ch == '(') ++paren;
            else if (ch == ')' && paren) --paren;
            else if (ch == '[') ++bracket;
            else if (ch == ']' && bracket) --bracket;
            else if (ch == '{') ++brace;
            else if (ch == '}' && brace) --brace;
            else if (ch == ',' && !paren && !bracket && !brace) { ++cursor; break; }
        }
    }
    return false;
}

void checkVerilogFullCaseAttribute(AnalysisResult& result, const core::SourceFile& source,
                                   const std::vector<parser::HdlToken>& tokens) {
    // Native GJB10157.12.3 slot 1 admits only AST_CASE (0x57). Slot 0 walks
    // that node's attribute map and reports only the exact ten-byte key
    // "\\full_case". The bundled frontend normalizes an explicit Verilog
    // attribute or its supported Synopsys-style hot comment to that key. The
    // reporter uses Warning and constructs `Synopsys full case用于模块<name>第<line>行.`.
    // ev_416ffabc86b3852ae9ddcee08887a99abf8e81190052562e8f24f3dc3b056c94
    // FUN_1806ef640, FUN_1806eeba0.
    const auto& content = source.content();
    std::string moduleName;
    for (std::size_t i = 0; i < tokens.size(); ++i) {
        const auto keyword = lower(tokens[i].text);
        if (keyword == "module" && i + 1 < tokens.size() &&
            tokens[i + 1].kind == parser::HdlTokenKind::Identifier)
            moduleName = tokens[i + 1].text;
        if (keyword == "endmodule") moduleName.clear();
        if (keyword != "case" && keyword != "casex" && keyword != "casez") continue;
        bool attributed = false;

        // One or more explicit attribute blocks immediately attached before
        // the case statement. Attribute-map keys are exact and case-sensitive.
        auto before = tokens[i].offset;
        while (before > 0 && std::isspace(static_cast<unsigned char>(content[before - 1])))
            --before;
        while (before >= 2 && content.substr(before - 2, 2) == "*)") {
            const auto open = content.rfind("(*", before - 2);
            if (open == std::string::npos) break;
            const auto body = content.substr(open + 2, before - open - 4);
            if (nativeExplicitFullCaseAttribute(body)) { attributed = true; break; }
            before = open;
            while (before > 0 && std::isspace(static_cast<unsigned char>(content[before - 1])))
                --before;
        }

        // The packaged example places `// synopsys full_case` after the case
        // selector. Restrict recognition to the remainder of that physical
        // header line; an unrelated comment elsewhere must not become a case attr.
        auto headerEnd = tokens[i].offset + tokens[i].text.size();
        std::size_t openParen = i + 1;
        while (openParen < tokens.size() && tokens[openParen].text != "(" &&
               tokens[openParen].line == tokens[i].line) ++openParen;
        if (openParen < tokens.size() && tokens[openParen].text == "(") {
            unsigned depth = 0;
            for (std::size_t j = openParen; j < tokens.size(); ++j) {
                if (tokens[j].text == "(") ++depth;
                else if (tokens[j].text == ")" && depth && --depth == 0) {
                    headerEnd = tokens[j].offset + tokens[j].text.size();
                    break;
                }
            }
        }
        const auto lineEnd = content.find('\n', tokens[i].offset);
        const auto end = lineEnd == std::string::npos ? content.size() : lineEnd;
        const auto slash = content.find("//", headerEnd);
        if (!attributed && slash != std::string::npos && slash < end)
            attributed = nativeFullCaseHotComment(content.substr(slash + 2, end - slash - 2));
        const auto block = content.find("/*", headerEnd);
        if (!attributed && block != std::string::npos && block < end) {
            const auto close = content.find("*/", block + 2);
            if (close != std::string::npos)
                attributed = nativeFullCaseHotComment(
                    content.substr(block + 2, close - block - 2));
        }
        if (attributed)
            emit(result, "GJB10157.12.3", source, tokens[i].line, tokens[i].column,
                 "Synopsys full case用于模块" + moduleName + "第" +
                     std::to_string(tokens[i].line) + "行.");
    }
}

struct VerilogFsmDeclaration {
    std::size_t token{};
    bool widthGreaterThanOne{};
};

std::string commaSeparated(const std::vector<std::string>& values) {
    std::ostringstream stream;
    for (std::size_t i = 0; i < values.size(); ++i) {
        if (i != 0) stream << "、";
        stream << values[i];
    }
    return stream.str();
}

std::optional<VerilogFsmDeclaration> boundedVerilogFsmDeclaration(
    const std::vector<parser::HdlToken>& tokens,
    const VerilogFsmStateCountCandidate& candidate) {
    std::optional<VerilogFsmDeclaration> result;
    for (std::size_t name = 0; name < candidate.caseToken && name < tokens.size(); ++name) {
        if (tokens[name].kind != parser::HdlTokenKind::Identifier ||
            lower(tokens[name].text) != candidate.control) continue;

        std::size_t begin = name;
        while (begin > 0 && tokens[begin - 1].text != ";") --begin;
        std::size_t end = name;
        while (end < tokens.size() && tokens[end].text != ";") ++end;
        if (end == tokens.size()) continue;

        bool declaration = false;
        bool integer = false;
        for (auto i = begin; i < name; ++i) {
            const auto word = lower(tokens[i].text);
            if (word == "reg" || word == "logic" || word == "bit") declaration = true;
            if (word == "integer") { declaration = true; integer = true; }
            if (declaration && (tokens[i].text == "=" || tokens[i].text == "<=")) {
                declaration = false;
                break;
            }
        }
        if (!declaration) continue;

        bool widthResolved = integer;
        bool widthGreaterThanOne = integer;
        for (auto i = begin; i + 4 < name; ++i) {
            if (tokens[i].text != "[" || tokens[i + 2].text != ":" ||
                tokens[i + 4].text != "]" ||
                tokens[i + 1].kind != parser::HdlTokenKind::Number ||
                tokens[i + 3].kind != parser::HdlTokenKind::Number) continue;
            unsigned left = 0, right = 0;
            const auto leftText = tokens[i + 1].text;
            const auto rightText = tokens[i + 3].text;
            const auto leftResult = std::from_chars(leftText.data(), leftText.data() + leftText.size(), left);
            const auto rightResult = std::from_chars(rightText.data(), rightText.data() + rightText.size(), right);
            if (leftResult.ec != std::errc{} || leftResult.ptr != leftText.data() + leftText.size() ||
                rightResult.ec != std::errc{} || rightResult.ptr != rightText.data() + rightText.size())
                continue;
            widthResolved = true;
            widthGreaterThanOne = left != right;
            break;
        }
        if (!widthResolved) {
            // A scalar reg/logic/bit is the one-bit case excluded by the native
            // worker. More than two distinct source states prove a wider
            // representation even when its typedef is outside this adapter.
            widthResolved = true;
            widthGreaterThanOne = candidate.sourceStates.size() > 2;
        }
        VerilogFsmDeclaration found{name, widthGreaterThanOne};
        if (result && result->token != found.token) return std::nullopt;
        result = found;
    }
    return result;
}

enum class VerilogFsmSafeDirective { Missing, Safe, Disabled };

VerilogFsmSafeDirective nativeVerilogFsmSafeDirective(
    const core::SourceFile& source, const parser::HdlToken& declaration) {
    const auto& content = source.content();
    if (declaration.offset >= content.size()) return VerilogFsmSafeDirective::Missing;

    auto lineStart = content.rfind('\n', declaration.offset);
    lineStart = lineStart == std::string::npos ? 0 : lineStart + 1;
    auto previousStart = lineStart == 0 ? 0 : content.rfind('\n', lineStart - 2);
    previousStart = previousStart == std::string::npos ? 0 : previousStart + 1;
    auto lineEnd = content.find('\n', declaration.offset);
    lineEnd = lineEnd == std::string::npos ? content.size() : lineEnd;
    const auto window = content.substr(previousStart, lineEnd - previousStart);

    // The native worker first observes Yosys' exact fsm_encoding attribute.
    // `none` removes the wire from FSM detection; a value containing `safe`
    // satisfies this rule's source-attribute branch.
    static const std::regex attribute(
        R"regex(fsm_encoding\s*=\s*"([^"]*)")regex", std::regex::ECMAScript);
    for (std::sregex_iterator it(window.begin(), window.end(), attribute), last; it != last; ++it) {
        const auto value = (*it)[1].str();
        if (value == "none") return VerilogFsmSafeDirective::Disabled;
        if (value.find("safe") != std::string::npos) return VerilogFsmSafeDirective::Safe;
    }

    static const std::regex synEncoding(
        R"(^\s*synthesis\s*(syn_encoding)?\s*=\s*".*safe.*"\s*$)",
        std::regex::ECMAScript | std::regex::icase);
    static const std::regex safeState(
        R"(^\s*(synthesis)?\s*fsm_safe_state\s*=\s*".*state.*"\s*$)",
        std::regex::ECMAScript | std::regex::icase);
    const auto accepts = [&](std::string_view body) {
        const std::string text(body);
        return std::regex_match(text, synEncoding) || std::regex_match(text, safeState);
    };
    for (std::size_t at = 0; at < window.size();) {
        const auto lineComment = window.find("//", at);
        const auto blockComment = window.find("/*", at);
        const auto next = std::min(lineComment == std::string::npos ? window.size() : lineComment,
                                   blockComment == std::string::npos ? window.size() : blockComment);
        if (next == window.size()) break;
        if (next == lineComment) {
            const auto end = window.find('\n', next + 2);
            const auto finish = end == std::string::npos ? window.size() : end;
            if (accepts(std::string_view(window).substr(next + 2, finish - next - 2)))
                return VerilogFsmSafeDirective::Safe;
            at = finish;
        } else {
            const auto end = window.find("*/", next + 2);
            if (end == std::string::npos) break;
            if (accepts(std::string_view(window).substr(next + 2, end - next - 2)))
                return VerilogFsmSafeDirective::Safe;
            at = end + 2;
        }
    }
    return VerilogFsmSafeDirective::Missing;
}

void checkVhdlMultipleEventControls(
    AnalysisResult& result, const core::SourceFile& source,
    const std::vector<parser::HdlToken>& tokens) {
    // Native FUN_1805d8f40 resets one counter for every AST_ALWAYS (0x52)
    // subtree. FUN_1805d8fb0 recursively increments it for an exact,
    // case-folded AST attribute name "event" (type 0xad) and for every edge
    // node of type 0x65/0x66. The current node is queued whenever the counter
    // exceeds one, so the second and every later event are reported without
    // deduplicating their signal names or edge polarities.
    // ev_924083c6393377b4cf1c450f0c12d16bd223497f708b79524082bf1e09110e96
    const auto is = [](const parser::HdlToken& token, std::string_view value) {
        return lower(token.text) == value;
    };
    for (std::size_t process = 0; process < tokens.size(); ++process) {
        if (!is(tokens[process], "process") ||
            (process > 0 && is(tokens[process - 1], "end"))) continue;
        std::size_t processEnd = process + 1;
        while (processEnd + 1 < tokens.size() &&
               !(is(tokens[processEnd], "end") &&
                 is(tokens[processEnd + 1], "process"))) ++processEnd;
        if (processEnd + 1 >= tokens.size()) continue;

        std::vector<std::size_t> events;
        for (std::size_t token = process + 1; token < processEnd; ++token) {
            const bool eventAttribute = token >= process + 3 &&
                is(tokens[token], "event") && tokens[token - 1].text == "'" &&
                tokens[token - 2].kind == parser::HdlTokenKind::Identifier;
            const bool edgeFunction =
                (is(tokens[token], "rising_edge") ||
                 is(tokens[token], "falling_edge")) &&
                token + 1 < processEnd && tokens[token + 1].text == "(";
            if (eventAttribute || edgeFunction) events.push_back(token);
        }
        for (std::size_t event = 1; event < events.size(); ++event) {
            const auto& token = tokens[events[event]];
            emit(result, "GJB9765.4.4.d", source, token.line, token.column,
                 "同一进程中出现第二个或后续 event control 段");
        }
        process = processEnd + 1;
    }
}

void checkVhdlCombinationalIfElse(
    AnalysisResult& result, const core::SourceFile& source,
    const std::vector<parser::HdlToken>& tokens) {
    // GJB9765.4.8.d constructor 0x180029870 leaves only option byte +0x158
    // enabled. Slot 0 (0x18077ad60) admits an AST_ALWAYS(0x52) only when its
    // process classification has no edge/event record. FUN_18077b8e0 then
    // walks direct IF/ELSIF/ELSE branch nodes (0x58/0xab/0xac) and, with that
    // option, emits the message at 0x180f1d698 exactly when the IF has no final
    // ELSE. The other branch-completeness, CASE, and conditional-assignment
    // option bytes are cleared by the constructor and must not broaden this
    // clause to every inferred latch.
    const auto is = [](const parser::HdlToken& token, std::string_view value) {
        return lower(token.text) == value;
    };
    const auto checkIfRange = [&](std::size_t begin, std::size_t end) {
        struct IfFrame {
            std::size_t token{};
            bool hasElse{};
        };
        std::vector<IfFrame> stack;
        for (std::size_t token = begin; token < end; ++token) {
            const auto word = lower(tokens[token].text);
            if (word == "if") {
                if (token > begin && is(tokens[token - 1], "end")) {
                    if (stack.empty()) continue;
                    const auto frame = stack.back();
                    stack.pop_back();
                    if (!frame.hasElse)
                        emit(result, "GJB9765.4.8.d", source,
                             tokens[frame.token].line, tokens[frame.token].column,
                             "组合逻辑中的 if 语句缺少最终 else 分支");
                    continue;
                }
                bool statement = false;
                int parentheses = 0;
                for (std::size_t cursor = token + 1; cursor < end; ++cursor) {
                    if (tokens[cursor].text == "(") ++parentheses;
                    else if (tokens[cursor].text == ")" && parentheses > 0) --parentheses;
                    if (parentheses != 0) continue;
                    const auto marker = lower(tokens[cursor].text);
                    if (marker == "then") {
                        statement = true;
                        break;
                    }
                    if (marker == "generate" || tokens[cursor].text == ";") break;
                }
                if (statement) stack.push_back({token, false});
                continue;
            }
            if (word == "else" && !stack.empty()) {
                bool conditionalAssignment = false;
                for (auto cursor = token; cursor > begin;) {
                    --cursor;
                    if (tokens[cursor].text == ";" ||
                        is(tokens[cursor], "then") || is(tokens[cursor], "elsif") ||
                        is(tokens[cursor], "else")) break;
                    if (is(tokens[cursor], "when")) {
                        conditionalAssignment = true;
                        break;
                    }
                }
                if (!conditionalAssignment) stack.back().hasElse = true;
            }
        }
    };
    for (std::size_t process = 0; process < tokens.size(); ++process) {
        if (!is(tokens[process], "process") ||
            (process > 0 && is(tokens[process - 1], "end"))) continue;
        std::size_t processEnd = process + 1;
        while (processEnd + 1 < tokens.size() &&
               !(is(tokens[processEnd], "end") &&
                 is(tokens[processEnd + 1], "process"))) ++processEnd;
        if (processEnd + 1 >= tokens.size()) continue;

        bool sequential = false;
        for (std::size_t token = process + 1; token < processEnd; ++token) {
            const bool edgeFunction =
                (is(tokens[token], "rising_edge") ||
                 is(tokens[token], "falling_edge")) &&
                token + 1 < processEnd && tokens[token + 1].text == "(";
            const bool eventAttribute = token >= process + 3 &&
                is(tokens[token], "event") && tokens[token - 1].text == "'" &&
                tokens[token - 2].kind == parser::HdlTokenKind::Identifier;
            if (edgeFunction || eventAttribute) {
                sequential = true;
                break;
            }
        }
        if (sequential) {
            process = processEnd + 1;
            continue;
        }
        checkIfRange(process + 1, processEnd);
        // Malformed or unsupported IF trees remain unreported rather than
        // being treated as proven violations.
        process = processEnd + 1;
    }

    // FUN_1801c7b80 gives every source VHDL IF an origin handle at +0x188,
    // marks +0x183, and lowers AST_IF(0xaa) to AST_CASE(0x57). The recursive
    // rule walker calls FUN_18077b8e0 for such nodes even when they are below
    // a function/procedure rather than AST_ALWAYS(0x52). Scan only complete
    // lexical subprogram bodies; declarations and malformed bodies remain
    // unknown instead of being guessed.
    for (std::size_t start = 0; start + 1 < tokens.size(); ++start) {
        const auto kind = lower(tokens[start].text);
        const bool identifierDesignator =
            tokens[start + 1].kind == parser::HdlTokenKind::Identifier;
        const bool operatorDesignator = kind == "function" &&
            tokens[start + 1].kind == parser::HdlTokenKind::String;
        if ((kind != "function" && kind != "procedure") ||
            (start > 0 && is(tokens[start - 1], "end")) ||
            (!identifierDesignator && !operatorDesignator)) continue;
        const auto name = lower(tokens[start + 1].text);

        std::size_t bodyMarker = tokens.size();
        int parentheses = 0;
        for (std::size_t cursor = start + 2; cursor < tokens.size(); ++cursor) {
            if (tokens[cursor].text == "(") ++parentheses;
            else if (tokens[cursor].text == ")" && parentheses > 0) --parentheses;
            if (parentheses != 0) continue;
            if (is(tokens[cursor], "is")) {
                bodyMarker = cursor;
                break;
            }
            if (tokens[cursor].text == ";") break;
        }
        if (bodyMarker == tokens.size()) continue;

        struct SubprogramFrame {
            std::string kind;
            std::string name;
        };
        std::vector<SubprogramFrame> nested;
        std::size_t bodyBegin = tokens.size();
        for (std::size_t cursor = bodyMarker + 1; cursor < tokens.size(); ++cursor) {
            const auto word = lower(tokens[cursor].text);
            if ((word == "function" || word == "procedure") &&
                (cursor == 0 || !is(tokens[cursor - 1], "end")) &&
                cursor + 1 < tokens.size()) {
                const bool nestedIdentifier =
                    tokens[cursor + 1].kind == parser::HdlTokenKind::Identifier;
                const bool nestedOperator = word == "function" &&
                    tokens[cursor + 1].kind == parser::HdlTokenKind::String;
                if (nestedIdentifier || nestedOperator) {
                    int nestedParentheses = 0;
                    bool hasBody = false;
                    for (std::size_t look = cursor + 2; look < tokens.size(); ++look) {
                        if (tokens[look].text == "(") ++nestedParentheses;
                        else if (tokens[look].text == ")" && nestedParentheses > 0)
                            --nestedParentheses;
                        if (nestedParentheses != 0) continue;
                        if (is(tokens[look], "is")) {
                            hasBody = true;
                            break;
                        }
                        if (tokens[look].text == ";") break;
                    }
                    if (hasBody)
                        nested.push_back({word, lower(tokens[cursor + 1].text)});
                }
                continue;
            }
            if (word == "end" && !nested.empty() && cursor + 1 < tokens.size()) {
                const auto ended = lower(tokens[cursor + 1].text);
                if (ended == "if" || ended == "case" || ended == "loop" ||
                    ended == "process" || ended == "generate" || ended == "block" ||
                    ended == "record") continue;
                if (ended == nested.back().kind || ended == nested.back().name ||
                    tokens[cursor + 1].text == ";") {
                    nested.pop_back();
                    continue;
                }
            }
            if (word == "begin" && nested.empty()) {
                bodyBegin = cursor;
                break;
            }
            if (word == "end" && nested.empty()) break;
        }
        if (bodyBegin == tokens.size()) continue;

        std::size_t bodyEnd = tokens.size();
        for (std::size_t cursor = bodyBegin + 1; cursor + 1 < tokens.size(); ++cursor) {
            if (!is(tokens[cursor], "end")) continue;
            const auto ended = lower(tokens[cursor + 1].text);
            if (ended == "if" || ended == "case" || ended == "loop" ||
                ended == "process" || ended == "generate" || ended == "block" ||
                ended == "record") continue;
            const bool explicitKind = ended == kind;
            const bool named = ended == name && cursor + 2 < tokens.size() &&
                               tokens[cursor + 2].text == ";";
            const bool bare = tokens[cursor + 1].text == ";";
            if (explicitKind || named || bare) {
                bodyEnd = cursor;
                break;
            }
        }
        if (bodyEnd == tokens.size()) continue;
        checkIfRange(bodyBegin + 1, bodyEnd);
    }
}

void checkVhdlNativeLatchRisks(
    AnalysisResult& result, const core::SourceFile& source,
    const std::vector<parser::HdlToken>& tokens) {
    // GJB9765.4.2.f constructor FUN_180026c90 enables all seven option
    // bytes +0x158..+0x15e. Shared slot 0 FUN_18077ad60 excludes processes
    // classified with edge/event relations and dispatches three helpers:
    // FUN_18077b8e0 compares the assignment-name union with each direct
    // IF/ELSIF/ELSE branch (+0x159) and reports a missing final ELSE
    // (+0x158); FUN_18077b0c0 performs the analogous CASE/OTHERS checks
    // (+0x15a..+0x15c); FUN_18077c530 reports a conditional assignment with
    // no ELSE or a simple ELSE that reads back its target (+0x15d).
    const auto is = [](const parser::HdlToken& token, std::string_view value) {
        return lower(token.text) == value;
    };
    const auto assignments = [&](std::size_t begin, std::size_t end) {
        std::set<std::string> names;
        for (auto token = begin; token < end && token + 1 < tokens.size(); ++token) {
            if (tokens[token].kind != parser::HdlTokenKind::Identifier ||
                (tokens[token + 1].text != "<=" && tokens[token + 1].text != ":="))
                continue;
            names.insert(lower(tokens[token].text));
        }
        return names;
    };
    const auto statementIf = [&](std::size_t token, std::size_t limit) {
        if (!is(tokens[token], "if") || (token > 0 && is(tokens[token - 1], "end")))
            return false;
        int parentheses = 0;
        for (auto cursor = token + 1; cursor < limit; ++cursor) {
            if (tokens[cursor].text == "(") ++parentheses;
            else if (tokens[cursor].text == ")" && parentheses > 0) --parentheses;
            if (parentheses != 0) continue;
            if (is(tokens[cursor], "then")) return true;
            if (is(tokens[cursor], "generate") || tokens[cursor].text == ";") return false;
        }
        return false;
    };
    const auto matchingEnd = [&](std::size_t start, std::size_t limit,
                                 std::string_view kind) -> std::optional<std::size_t> {
        int depth = 1;
        for (auto cursor = start + 1; cursor + 1 < limit; ++cursor) {
            const bool opens = kind == "if" ? statementIf(cursor, limit) :
                is(tokens[cursor], kind) &&
                (cursor == 0 || !is(tokens[cursor - 1], "end"));
            if (opens) ++depth;
            if (is(tokens[cursor], "end") && is(tokens[cursor + 1], kind) && --depth == 0)
                return cursor;
        }
        return std::nullopt;
    };
    struct Branch {
        std::size_t marker{};
        std::size_t begin{};
        std::size_t end{};
        bool fallback{};
    };
    const auto reportMissing = [&](const std::vector<Branch>& branches,
                                   std::string_view construct) {
        std::set<std::string> all;
        std::vector<std::set<std::string>> perBranch;
        for (const auto& branch : branches) {
            perBranch.push_back(assignments(branch.begin, branch.end));
            all.insert(perBranch.back().begin(), perBranch.back().end());
        }
        for (std::size_t branch = 0; branch < branches.size(); ++branch) {
            for (const auto& name : all) {
                if (perBranch[branch].contains(name)) continue;
                emit(result, "GJB9765.4.2.f", source,
                     tokens[branches[branch].marker].line,
                     tokens[branches[branch].marker].column,
                     std::string(construct) +
                         (branches[branch].fallback ? " 默认分支未赋值：" : " 分支未赋值：") +
                         name);
            }
        }
    };

    for (std::size_t process = 0; process < tokens.size(); ++process) {
        if (!is(tokens[process], "process") ||
            (process > 0 && is(tokens[process - 1], "end"))) continue;
        std::size_t processEnd = process + 1;
        while (processEnd + 1 < tokens.size() &&
               !(is(tokens[processEnd], "end") &&
                 is(tokens[processEnd + 1], "process"))) ++processEnd;
        if (processEnd + 1 >= tokens.size()) continue;
        bool sequential = false;
        for (auto token = process + 1; token < processEnd; ++token) {
            const bool edgeFunction =
                (is(tokens[token], "rising_edge") || is(tokens[token], "falling_edge")) &&
                token + 1 < processEnd && tokens[token + 1].text == "(";
            const bool eventAttribute = token >= process + 3 && is(tokens[token], "event") &&
                tokens[token - 1].text == "'" &&
                tokens[token - 2].kind == parser::HdlTokenKind::Identifier;
            if (edgeFunction || eventAttribute) { sequential = true; break; }
        }
        if (sequential) { process = processEnd + 1; continue; }

        for (auto token = process + 1; token < processEnd; ++token) {
            if (statementIf(token, processEnd)) {
                const auto finish = matchingEnd(token, processEnd, "if");
                if (!finish) continue;
                auto thenToken = token + 1;
                while (thenToken < *finish && !is(tokens[thenToken], "then")) ++thenToken;
                if (thenToken == *finish) continue;
                std::vector<Branch> branches{{token, thenToken + 1, *finish, false}};
                int depth = 0;
                bool hasElse = false;
                for (auto cursor = thenToken + 1; cursor < *finish; ++cursor) {
                    if (statementIf(cursor, *finish)) { ++depth; continue; }
                    if (is(tokens[cursor], "end") && cursor + 1 < *finish &&
                        is(tokens[cursor + 1], "if")) { if (depth > 0) --depth; continue; }
                    if (depth != 0 || (!is(tokens[cursor], "elsif") &&
                                       !is(tokens[cursor], "else"))) continue;
                    branches.back().end = cursor;
                    if (is(tokens[cursor], "else")) {
                        hasElse = true;
                        branches.push_back({cursor, cursor + 1, *finish, true});
                    } else {
                        auto branchThen = cursor + 1;
                        while (branchThen < *finish && !is(tokens[branchThen], "then"))
                            ++branchThen;
                        if (branchThen == *finish) break;
                        branches.push_back({cursor, branchThen + 1, *finish, false});
                        cursor = branchThen;
                    }
                }
                reportMissing(branches, "IF");
                if (!hasElse)
                    emit(result, "GJB9765.4.2.f", source,
                         tokens[token].line, tokens[token].column,
                         "组合过程中的 IF 语句缺少最终 ELSE 分支");
                continue;
            }
            if (is(tokens[token], "case") &&
                (token == 0 || !is(tokens[token - 1], "end"))) {
                const auto finish = matchingEnd(token, processEnd, "case");
                if (!finish) continue;
                std::vector<Branch> branches;
                int depth = 0;
                for (auto cursor = token + 1; cursor < *finish; ++cursor) {
                    if (is(tokens[cursor], "case") &&
                        (cursor == 0 || !is(tokens[cursor - 1], "end"))) {
                        ++depth;
                        continue;
                    }
                    if (is(tokens[cursor], "end") && cursor + 1 < *finish &&
                        is(tokens[cursor + 1], "case")) { if (depth > 0) --depth; continue; }
                    if (depth != 0 || !is(tokens[cursor], "when")) continue;
                    if (!branches.empty()) branches.back().end = cursor;
                    auto arrow = cursor + 1;
                    bool others = false;
                    while (arrow < *finish && tokens[arrow].text != "=>") {
                        if (is(tokens[arrow], "others")) others = true;
                        ++arrow;
                    }
                    if (arrow == *finish) break;
                    branches.push_back({cursor, arrow + 1, *finish, others});
                    cursor = arrow;
                }
                if (!branches.empty()) {
                    reportMissing(branches, "CASE");
                    if (!std::ranges::any_of(branches, [](const auto& branch) {
                            return branch.fallback;
                        }))
                        emit(result, "GJB9765.4.2.f", source,
                             tokens[token].line, tokens[token].column,
                             "组合过程中的 CASE 语句缺少 OTHERS 分支");
                }
                continue;
            }
            if (tokens[token].kind != parser::HdlTokenKind::Identifier ||
                token + 1 >= processEnd || tokens[token + 1].text != "<=") continue;
            auto end = token + 2;
            while (end < processEnd && tokens[end].text != ";") ++end;
            std::optional<std::size_t> when;
            std::optional<std::size_t> otherwise;
            int parentheses = 0;
            for (auto cursor = token + 2; cursor < end; ++cursor) {
                if (tokens[cursor].text == "(") ++parentheses;
                else if (tokens[cursor].text == ")" && parentheses > 0) --parentheses;
                if (parentheses != 0) continue;
                if (is(tokens[cursor], "when")) when = cursor;
                else if (is(tokens[cursor], "else")) otherwise = cursor;
            }
            if (!when) { token = end; continue; }
            if (!otherwise) {
                emit(result, "GJB9765.4.2.f", source,
                     tokens[token].line, tokens[token].column,
                     "条件赋值语句缺少 ELSE 分支");
            } else if (*otherwise + 2 == end &&
                       tokens[*otherwise + 1].kind == parser::HdlTokenKind::Identifier &&
                       lower(tokens[*otherwise + 1].text) == lower(tokens[token].text)) {
                emit(result, "GJB9765.4.2.f", source,
                     tokens[token].line, tokens[token].column,
                     "条件赋值语句的 ELSE 分支保持目标原值：" + tokens[token].text);
            }
            token = end;
        }
        process = processEnd + 1;
    }
}

void checkVhdlStaticCaseChoices(
    AnalysisResult& result, const core::SourceFile& source,
    const std::vector<parser::HdlToken>& tokens) {
    // Native slot 0 FUN_18065f460 rebuilds one permitted static-name set per
    // design root from declaration kinds 0x77/0x0a and the cross-confirmed
    // entity(0x76) -> generic(0xa3) -> identifier(0x13) path.
    // It checks ordinary CASE(0x57)/COND(0x58) choices and selected assignment
    // 0xb7/0xb6 choices. FUN_1806603d0 gathers every identifier 0x13 in one
    // choice and reports the first name absent from that set. The embedded
    // diagnostic at 0x180f09fb8 is "选择参数不能使用变量".
    struct VhdlChoiceScope {
        std::string entity;
        std::size_t begin{};
        std::size_t end{};
        std::set<std::string> runtimeNames;
        std::set<std::string> staticNames;
    };
    std::vector<VhdlChoiceScope> scopes;
    for (std::size_t token = 0; token + 3 < tokens.size(); ++token) {
        if (lower(tokens[token].text) != "architecture" ||
            lower(tokens[token + 2].text) != "of" ||
            tokens[token + 3].kind != parser::HdlTokenKind::Identifier) continue;
        const auto architectureName = lower(tokens[token + 1].text);
        auto end = token + 4;
        std::optional<std::size_t> bareEnd;
        bool foundEnd = false;
        while (end + 1 < tokens.size()) {
            if (lower(tokens[end].text) == "end") {
                const auto endKind = lower(tokens[end + 1].text);
                if (endKind == "architecture" || endKind == architectureName) {
                    foundEnd = true;
                    break;
                }
                if (tokens[end + 1].text == ";") bareEnd = end;
            }
            const auto beginsNextUnit =
                (lower(tokens[end].text) == "entity" ||
                 lower(tokens[end].text) == "architecture") &&
                (end == 0 || lower(tokens[end - 1].text) != "end");
            if (beginsNextUnit && bareEnd) {
                end = *bareEnd;
                foundEnd = true;
                break;
            }
            ++end;
        }
        if (!foundEnd && bareEnd) end = *bareEnd;
        if (end + 1 < tokens.size()) end += 2;
        scopes.push_back({lower(tokens[token + 3].text), token,
                          std::min(end, tokens.size()), {}, {}});
        token = end == 0 ? token : end - 1;
    }
    const auto scopeAt = [&](std::size_t token) -> VhdlChoiceScope* {
        const auto found = std::ranges::find_if(scopes, [&](const auto& scope) {
            return token >= scope.begin && token < scope.end;
        });
        return found == scopes.end() ? nullptr : &*found;
    };
    for (const auto& unit : result.structure) {
        if (unit.kind != parser::DesignUnitKind::VhdlEntity ||
            unit.location.file.lexically_normal() != source.path().lexically_normal()) continue;
        for (auto& scope : scopes) {
            if (scope.entity != lower(unit.name)) continue;
            for (const auto& port : unit.ports)
                scope.runtimeNames.insert(lower(port.name));
            for (const auto& parameter : unit.parameters)
                scope.staticNames.insert(lower(parameter.name));
        }
    }
    for (const auto& object : collectVhdlNamedObjects(source, tokens)) {
        auto* scope = scopeAt(object.token);
        if (!scope) continue;
        const auto name = lower(tokens[object.token].text);
        if (object.category == VhdlNameCategory::Signal ||
            object.category == VhdlNameCategory::Variable)
            scope->runtimeNames.insert(name);
        else if (object.category == VhdlNameCategory::Constant)
            scope->staticNames.insert(name);
    }

    // Recover the proven enum-literal producer shape. Enumeration literals
    // are locally static choices even though their lexical kind is Identifier.
    for (std::size_t type = 0; type + 3 < tokens.size(); ++type) {
        if (lower(tokens[type].text) != "type" ||
            lower(tokens[type + 2].text) != "is" || tokens[type + 3].text != "(")
            continue;
        const auto close = closeParen(tokens, type + 3);
        if (close == tokens.size()) continue;
        auto* scope = scopeAt(type);
        if (!scope) { type = close; continue; }
        for (auto literal = type + 4; literal < close; ++literal)
            if (tokens[literal].kind == parser::HdlTokenKind::Identifier)
                scope->staticNames.insert(lower(tokens[literal].text));
        type = close;
    }
    for (auto& scope : scopes) {
        scope.staticNames.insert("true");
        scope.staticNames.insert("false");
    }

    const auto checkChoice = [&](std::size_t begin, std::size_t end) {
        const auto* scope = scopeAt(begin);
        if (!scope) return;
        for (auto token = begin; token < end; ++token) {
            if (tokens[token].kind != parser::HdlTokenKind::Identifier) continue;
            const auto name = lower(tokens[token].text);
            if (name == "others" || name == "to" || name == "downto" ||
                scope->staticNames.contains(name) || !scope->runtimeNames.contains(name)) continue;
            emit(result, "GJB9765.4.8.b", source,
                 tokens[token].line, tokens[token].column,
                 "CASE/SELECT 选择项使用了运行时变量或信号：" + tokens[token].text);
            break; // Native helper emits at most once for one choice expression.
        }
    };

    std::vector<std::size_t> caseStack;
    for (std::size_t token = 0; token < tokens.size(); ++token) {
        const auto word = lower(tokens[token].text);
        if (word == "case" && (token == 0 || lower(tokens[token - 1].text) != "end")) {
            bool sequential = false;
            for (auto cursor = token + 1; cursor < tokens.size(); ++cursor) {
                const auto marker = lower(tokens[cursor].text);
                if (marker == "is") { sequential = true; break; }
                if (marker == "generate" || tokens[cursor].text == ";") break;
            }
            if (sequential) caseStack.push_back(token);
            continue;
        }
        if (word == "end" && token + 1 < tokens.size() &&
            lower(tokens[token + 1].text) == "case") {
            if (!caseStack.empty()) caseStack.pop_back();
            ++token;
            continue;
        }
        if (word != "when" || caseStack.empty()) continue;
        auto arrow = token + 1;
        while (arrow < tokens.size() && tokens[arrow].text != "=>" &&
               tokens[arrow].text != ";") ++arrow;
        if (arrow < tokens.size() && tokens[arrow].text == "=>")
            checkChoice(token + 1, arrow);
    }

    // Selected signal assignments are represented by a separate native root
    // (0xb7), so scan only statements with an explicit WITH ... SELECT header.
    for (std::size_t with = 0; with < tokens.size(); ++with) {
        if (lower(tokens[with].text) != "with") continue;
        auto end = with + 1;
        auto select = tokens.size();
        for (; end < tokens.size() && tokens[end].text != ";"; ++end)
            if (lower(tokens[end].text) == "select") select = end;
        if (select == tokens.size() || end == tokens.size()) continue;
        for (auto token = select + 1; token < end; ++token) {
            if (lower(tokens[token].text) != "when") continue;
            auto choiceEnd = token + 1;
            int parentheses = 0;
            for (; choiceEnd < end; ++choiceEnd) {
                if (tokens[choiceEnd].text == "(") ++parentheses;
                else if (tokens[choiceEnd].text == ")" && parentheses > 0) --parentheses;
                else if (tokens[choiceEnd].text == "," && parentheses == 0) break;
            }
            checkChoice(token + 1, choiceEnd);
            token = choiceEnd;
        }
        with = end;
    }
}

void checkVhdlForVariableWrites(
    AnalysisResult& result, const core::SourceFile& source,
    const std::vector<parser::HdlToken>& tokens) {
    // Native FUN_180607b10 visits every AST_FOR(0x5c), saves its name, and
    // invokes recursive FUN_180607b90. That helper admits assignment kind
    // 0xa2 with at least two children, compares the first child's name with
    // the active loop name, and queues the second child for reporting. The
    // embedded diagnostic at 0x180f02838 is
    // "禁止在for循环体内修改循环变量".
    std::set<std::pair<std::size_t, std::string>> reported;
    for (std::size_t token = 0; token < tokens.size(); ++token) {
        const auto word = lower(tokens[token].text);
        if (word != "for" || token + 2 >= tokens.size() ||
            tokens[token + 1].kind != parser::HdlTokenKind::Identifier ||
            lower(tokens[token + 2].text) != "in") continue;
        auto loop = token + 3;
        for (; loop < tokens.size(); ++loop) {
            const auto marker = lower(tokens[loop].text);
            if (marker == "loop" || marker == "generate" || tokens[loop].text == ";")
                break;
        }
        if (loop == tokens.size() || lower(tokens[loop].text) != "loop") continue;
        int depth = 1;
        auto end = loop + 1;
        for (; end < tokens.size(); ++end) {
            const auto marker = lower(tokens[end].text);
            if (marker == "loop" &&
                (end == 0 || lower(tokens[end - 1].text) != "end")) ++depth;
            else if (marker == "end" && end + 1 < tokens.size() &&
                     lower(tokens[end + 1].text) == "loop") {
                if (--depth == 0) break;
            }
        }
        if (end == tokens.size()) continue;
        const auto loopName = lower(tokens[token + 1].text);
        for (auto body = loop + 1; body + 1 < end; ++body) {
            if (tokens[body].kind != parser::HdlTokenKind::Identifier ||
                tokens[body + 1].text != ":=" ||
                lower(tokens[body].text) != loopName ||
                !reported.emplace(tokens[body].offset, loopName).second) continue;
            emit(result, "GJB9765.4.7.b", source,
                 tokens[body].line, tokens[body].column,
                 "禁止在 FOR 循环体内修改循环变量：" + tokens[body].text);
        }
    }
}

void checkVhdlConstantStaticInitializers(
    AnalysisResult& result, const core::SourceFile& source,
    const std::vector<parser::HdlToken>& tokens) {
    // Native GJB9765_VHDL_4_6_c slot 0 FUN_1806016c0 first gathers
    // AST_LOCALPARAM(0x0a) records whose +0x178 VHDL-constant-origin byte is
    // set. Producer FUN_1801b2770 creates AST_LOCALPARAM plus an optional
    // AST_VARASGN(0xa2), sets +0x178 unconditionally, and destroys the latter
    // when no initializer exists. A zero-child constant is therefore admitted.
    // A literal AST_CONSTANT(0x10) is admitted. Field-producer FUN_1801c2660
    // proves +0x10e is the direct/resolved BOOLEAN type flag (not a generic
    // constant-folding flag), so a one-child boolean initializer is also
    // admitted without inspecting its runtime dependencies. An
    // AST_IDENTIFIER(0x13), or every identifier below a compound RHS, must
    // resolve to a previously admitted constant name. Unresolved dependencies
    // are retained in a separate rejected-name set and propagate to later
    // constants. The native function also gathers AST_SIGNAL(0x78) and
    // AST_VARIABLE(0x79) nodes whose +0x109 integer flag is set, then revisits
    // AST_ASSIGN_LE(0x56). If an integer target's RHS contains a rejected
    // constant, it reports the assignment site as well; assignments on the
    // declaration's physical line are skipped. Embedded fragments at
    // 0x180f018e0/0x180f018d0/0x180f018a8 compose
    // "常量<target>的赋值表达式中<dependency>不是固定整型值或可计算的固定整型值".
    // This source adapter walks complete architecture and package design roots,
    // including process, block and subprogram declarative parts.
    // It reports a dependency
    // only when that name is proven to be a local signal/variable/entity port
    // or a previously rejected constant; package names, user functions and
    // other unresolved elaboration-time objects remain ManualReviewRequired.
    // ev_5f591cf433b08c94fd59aae3b050b90a666c6db29bb7ea7f776975767cec926c
    struct DesignRootRange {
        std::string name;
        std::string entity;
        std::size_t begin{};
        std::size_t declarationsBegin{};
        std::size_t bodyBegin{};
        std::size_t end{};
        bool requiresArchitectureBody{true};
    };

    std::map<std::string, std::set<std::string>> entityPorts;
    std::map<std::string, std::set<std::string>> entityGenerics;
    for (const auto& unit : result.structure) {
        if (unit.kind != parser::DesignUnitKind::VhdlEntity ||
            unit.location.file.lexically_normal() != source.path().lexically_normal()) continue;
        auto& ports = entityPorts[lower(unit.name)];
        for (const auto& port : unit.ports) ports.insert(lower(port.name));
        auto& generics = entityGenerics[lower(unit.name)];
        for (const auto& parameter : unit.parameters) generics.insert(lower(parameter.name));
    }

    std::vector<DesignRootRange> roots;
    for (std::size_t index = 0; index + 3 < tokens.size(); ++index) {
        if (lower(tokens[index].text) != "architecture" ||
            (index > 0 && lower(tokens[index - 1].text) == "end") ||
            lower(tokens[index + 2].text) != "of") continue;
        DesignRootRange range;
        range.name = lower(tokens[index + 1].text);
        range.entity = lower(tokens[index + 3].text);
        range.begin = index;
        range.end = tokens.size();
        std::vector<std::string> subprogramNames;
        std::vector<std::string> localPackageNames;
        for (auto cursor = index + 4; cursor < tokens.size(); ++cursor) {
            const auto word = lower(tokens[cursor].text);
            if ((word == "function" || word == "procedure") &&
                (cursor == 0 || lower(tokens[cursor - 1].text) != "end")) {
                auto marker = cursor + 1;
                int parentheses = 0;
                for (; marker < tokens.size(); ++marker) {
                    if (tokens[marker].text == "(") ++parentheses;
                    else if (tokens[marker].text == ")" && parentheses > 0) --parentheses;
                    else if (parentheses == 0 &&
                             (lower(tokens[marker].text) == "is" ||
                              tokens[marker].text == ";")) break;
                }
                if (marker < tokens.size() && lower(tokens[marker].text) == "is")
                    subprogramNames.push_back(lower(tokens[cursor + 1].text));
                continue;
            }
            if (word == "package" &&
                (cursor == 0 || lower(tokens[cursor - 1].text) != "end")) {
                const auto nameToken = cursor + 1 < tokens.size() &&
                    lower(tokens[cursor + 1].text) == "body" ? cursor + 2 : cursor + 1;
                auto marker = nameToken + 1;
                while (marker < tokens.size() && lower(tokens[marker].text) != "is" &&
                       tokens[marker].text != ";") ++marker;
                if (marker + 1 < tokens.size() && lower(tokens[marker].text) == "is" &&
                    lower(tokens[marker + 1].text) != "new")
                    localPackageNames.push_back(lower(tokens[nameToken].text));
                continue;
            }
            if (word == "end" && cursor + 1 < tokens.size()) {
                const auto next = lower(tokens[cursor + 1].text);
                if (!subprogramNames.empty() &&
                    (next == "function" || next == "procedure" ||
                     next == subprogramNames.back() || tokens[cursor + 1].text == ";")) {
                    subprogramNames.pop_back();
                    continue;
                }
                if (!localPackageNames.empty() &&
                    (next == "package" || next == localPackageNames.back() ||
                     tokens[cursor + 1].text == ";")) {
                    localPackageNames.pop_back();
                    continue;
                }
                if (next != "architecture" && next != range.name &&
                    tokens[cursor + 1].text != ";") continue;
                auto semicolon = cursor + 2;
                while (semicolon < tokens.size() && tokens[semicolon].text != ";")
                    ++semicolon;
                if (tokens[cursor + 1].text == ";") semicolon = cursor + 1;
                range.end = semicolon < tokens.size() ? semicolon + 1 : cursor;
                break;
            }
            if (word == "architecture" &&
                (cursor == 0 || lower(tokens[cursor - 1].text) != "end")) {
                range.end = cursor;
                break;
            }
        }
        range.declarationsBegin = index + 4;
        while (range.declarationsBegin < range.end &&
               lower(tokens[range.declarationsBegin].text) != "is")
            ++range.declarationsBegin;
        if (range.declarationsBegin == range.end) continue;
        ++range.declarationsBegin;

        // Do not mistake a function/procedure body's BEGIN for the enclosing
        // architecture BEGIN. Forward declarations end at ';' before 'is'.
        std::vector<std::string> bodySubprogramNames;
        range.bodyBegin = range.end;
        for (auto cursor = range.declarationsBegin; cursor < range.end; ++cursor) {
            const auto word = lower(tokens[cursor].text);
            if ((word == "function" || word == "procedure") &&
                (cursor == 0 || lower(tokens[cursor - 1].text) != "end")) {
                auto marker = cursor + 1;
                int parentheses = 0;
                for (; marker < range.end; ++marker) {
                    if (tokens[marker].text == "(") ++parentheses;
                    else if (tokens[marker].text == ")" && parentheses > 0) --parentheses;
                    else if (parentheses == 0 &&
                             (lower(tokens[marker].text) == "is" ||
                              tokens[marker].text == ";")) break;
                }
                if (marker < range.end && lower(tokens[marker].text) == "is")
                    bodySubprogramNames.push_back(lower(tokens[cursor + 1].text));
                continue;
            }
            if (word == "end" && cursor + 1 < range.end &&
                !bodySubprogramNames.empty() &&
                (lower(tokens[cursor + 1].text) == "function" ||
                 lower(tokens[cursor + 1].text) == "procedure" ||
                 lower(tokens[cursor + 1].text) == bodySubprogramNames.back() ||
                 tokens[cursor + 1].text == ";")) {
                bodySubprogramNames.pop_back();
                continue;
            }
            if (word == "begin" && bodySubprogramNames.empty()) {
                range.bodyBegin = cursor;
                break;
            }
        }
        roots.push_back(range);
        index = range.end == 0 ? index : range.end - 1;
    }

    // The native entry takes the first AST root from every input record and has
    // no AST_ARCHITECTURE type gate. Preserve that root lifecycle for top-level
    // package specifications and package bodies, while excluding local packages
    // already contained by an architecture/package root and package instances.
    for (std::size_t index = 0; index + 2 < tokens.size(); ++index) {
        if (lower(tokens[index].text) != "package" ||
            (index > 0 && lower(tokens[index - 1].text) == "end")) continue;
        if (std::ranges::any_of(roots, [index](const auto& root) {
                return root.begin < index && index < root.end;
            })) continue;
        const bool isBody = lower(tokens[index + 1].text) == "body";
        const auto nameToken = index + (isBody ? 2U : 1U);
        if (nameToken >= tokens.size() ||
            tokens[nameToken].kind != parser::HdlTokenKind::Identifier) continue;
        auto isToken = nameToken + 1;
        while (isToken < tokens.size() && lower(tokens[isToken].text) != "is" &&
               tokens[isToken].text != ";") ++isToken;
        if (isToken >= tokens.size() || lower(tokens[isToken].text) != "is" ||
            (isToken + 1 < tokens.size() && lower(tokens[isToken + 1].text) == "new"))
            continue;

        DesignRootRange range;
        range.name = lower(tokens[nameToken].text);
        range.begin = index;
        range.declarationsBegin = isToken + 1;
        range.bodyBegin = range.declarationsBegin;
        range.end = tokens.size();
        range.requiresArchitectureBody = false;
        std::vector<std::string> subprogramNames;
        std::vector<std::string> localPackageNames;
        for (auto cursor = range.declarationsBegin; cursor < tokens.size(); ++cursor) {
            const auto word = lower(tokens[cursor].text);
            if ((word == "function" || word == "procedure") &&
                (cursor == 0 || lower(tokens[cursor - 1].text) != "end")) {
                auto marker = cursor + 1;
                int parentheses = 0;
                for (; marker < tokens.size(); ++marker) {
                    if (tokens[marker].text == "(") ++parentheses;
                    else if (tokens[marker].text == ")" && parentheses > 0) --parentheses;
                    else if (parentheses == 0 &&
                             (lower(tokens[marker].text) == "is" ||
                              tokens[marker].text == ";")) break;
                }
                if (marker < tokens.size() && lower(tokens[marker].text) == "is")
                    subprogramNames.push_back(lower(tokens[cursor + 1].text));
                continue;
            }
            if (word == "package" &&
                (cursor == 0 || lower(tokens[cursor - 1].text) != "end")) {
                const auto localNameToken = cursor + 1 < tokens.size() &&
                    lower(tokens[cursor + 1].text) == "body" ? cursor + 2 : cursor + 1;
                auto marker = localNameToken + 1;
                while (marker < tokens.size() && lower(tokens[marker].text) != "is" &&
                       tokens[marker].text != ";") ++marker;
                if (marker + 1 < tokens.size() && lower(tokens[marker].text) == "is" &&
                    lower(tokens[marker + 1].text) != "new")
                    localPackageNames.push_back(lower(tokens[localNameToken].text));
                continue;
            }
            if (word != "end" || cursor + 1 >= tokens.size()) continue;
            const auto next = lower(tokens[cursor + 1].text);
            if (!subprogramNames.empty() &&
                (next == "function" || next == "procedure" ||
                 next == subprogramNames.back() || tokens[cursor + 1].text == ";")) {
                subprogramNames.pop_back();
                continue;
            }
            if (!localPackageNames.empty() &&
                (next == "package" || next == localPackageNames.back() ||
                 tokens[cursor + 1].text == ";")) {
                localPackageNames.pop_back();
                continue;
            }
            if (next != "package" && next != range.name &&
                tokens[cursor + 1].text != ";") continue;
            auto semicolon = cursor + 2;
            while (semicolon < tokens.size() && tokens[semicolon].text != ";")
                ++semicolon;
            if (tokens[cursor + 1].text == ";") semicolon = cursor + 1;
            range.end = semicolon < tokens.size() ? semicolon + 1 : cursor;
            break;
        }
        roots.push_back(range);
        index = range.end == 0 ? index : range.end - 1;
    }

    const std::set<std::string_view> reserved{
        "abs", "access", "after", "alias", "all", "and", "array", "assert",
        "attribute", "begin", "bit", "bit_vector", "boolean", "buffer", "bus",
        "character", "constant", "downto", "else", "elsif", "end", "entity",
        "exit", "file", "for", "function", "generate", "generic", "if", "impure",
        "in", "inertial", "inout", "integer", "is", "label", "library", "linkage",
        "literal", "loop", "map", "mod", "nand", "natural", "new", "next", "nor",
        "not", "null", "of", "on", "open", "or", "others", "out", "package",
        "port", "positive", "postponed", "procedure", "process", "protected", "pure",
        "range", "real", "record", "register", "reject", "rem", "report", "return",
        "rol", "ror", "select", "severity", "shared", "signal", "signed", "sla",
        "sll", "sra", "srl", "std_logic", "std_logic_vector", "string", "subtype",
        "then", "time", "to", "transport", "type", "unaffected", "unsigned", "use",
        "variable", "wait", "when", "while", "with", "xnor", "xor", "true", "false",
        "fs", "ps", "ns", "us", "ms", "sec", "min", "hr"
    };

    enum class ConstantState { Static, Dynamic, Unknown };
    for (const auto& architecture : roots) {
        if (architecture.requiresArchitectureBody &&
            architecture.bodyBegin == architecture.end) continue;
        std::set<std::string> runtimeNames;
        std::set<std::string> integerRuntimeNames;
        std::set<std::size_t> integerDeclarationLines;
        if (const auto ports = entityPorts.find(architecture.entity);
            ports != entityPorts.end()) runtimeNames.insert(ports->second.begin(), ports->second.end());
        std::set<std::string> staticSeeds{"true", "false"};
        if (const auto generics = entityGenerics.find(architecture.entity);
            generics != entityGenerics.end())
            staticSeeds.insert(generics->second.begin(), generics->second.end());

        // Signals and variables anywhere below the architecture root are runtime
        // objects. Enumeration literals are static atoms and can participate in
        // a constant value.
        for (auto token = architecture.declarationsBegin; token < architecture.end; ++token) {
            const auto word = lower(tokens[token].text);
            if (word == "signal" || word == "variable") {
                auto colon = token + 1;
                while (colon < architecture.end && tokens[colon].text != ":" &&
                       tokens[colon].text != ";") ++colon;
                if (colon < architecture.end && tokens[colon].text == ":") {
                    auto typeEnd = colon + 1;
                    while (typeEnd < architecture.end &&
                           tokens[typeEnd].text != ":=" && tokens[typeEnd].text != ";")
                        ++typeEnd;
                    const bool isInteger = colon + 1 < typeEnd &&
                        lower(tokens[colon + 1].text) == "integer";
                    for (auto name = token + 1; name < colon; ++name) {
                        if (tokens[name].kind == parser::HdlTokenKind::Identifier) {
                            runtimeNames.insert(lower(tokens[name].text));
                            if (isInteger)
                                integerRuntimeNames.insert(lower(tokens[name].text));
                        }
                    }
                    if (isInteger) integerDeclarationLines.insert(tokens[token].line);
                }
            }
            if (word == "type") {
                auto isToken = token + 1;
                while (isToken < architecture.end &&
                       lower(tokens[isToken].text) != "is" && tokens[isToken].text != ";")
                    ++isToken;
                if (isToken + 1 < architecture.end && tokens[isToken + 1].text == "(") {
                    const auto close = closeParen(tokens, isToken + 1);
                    if (close < architecture.end)
                        for (auto literal = isToken + 2; literal < close; ++literal)
                            if (tokens[literal].kind == parser::HdlTokenKind::Identifier)
                                staticSeeds.insert(lower(tokens[literal].text));
                }
            }
        }

        std::map<std::string, ConstantState> states;
        for (const auto& seed : staticSeeds) states.emplace(seed, ConstantState::Static);
        for (auto token = architecture.declarationsBegin; token < architecture.end; ++token) {
            if (lower(tokens[token].text) != "constant") continue;
            auto colon = token + 1;
            while (colon < architecture.end && tokens[colon].text != ":" &&
                   tokens[colon].text != ";") ++colon;
            if (colon >= architecture.end || tokens[colon].text != ":") continue;
            std::vector<std::pair<std::string, std::size_t>> names;
            for (auto name = token + 1; name < colon; ++name)
                if (tokens[name].kind == parser::HdlTokenKind::Identifier)
                    names.emplace_back(lower(tokens[name].text), name);
            auto end = colon + 1;
            int parentheses = 0;
            for (; end < architecture.end; ++end) {
                if (tokens[end].text == "(") ++parentheses;
                else if (tokens[end].text == ")" && parentheses > 0) --parentheses;
                else if (parentheses == 0 && tokens[end].text == ";") break;
            }
            if (end >= architecture.end) break;
            auto assignment = colon + 1;
            for (; assignment < end && tokens[assignment].text != ":="; ++assignment) {}
            if (assignment == end || assignment + 1 == end) {
                for (const auto& [name, location] : names) {
                    (void)location;
                    states[name] = ConstantState::Static;
                }
                token = end;
                continue;
            }

            // Native FUN_1801c2660 sets +0x10e for the direct BOOLEAN type and
            // propagates type flags to expression nodes. FUN_1806016c0 accepts
            // a one-child initializer carrying that byte before its identifier
            // dependency branch. Restrict the source adapter to the provable
            // direct-type spelling; boolean subtypes remain unresolved.
            const bool directBooleanType = colon + 1 < assignment &&
                lower(tokens[colon + 1].text) == "boolean";
            if (directBooleanType) {
                for (const auto& [name, location] : names) {
                    (void)location;
                    states[name] = ConstantState::Static;
                }
                token = end;
                continue;
            }

            std::set<std::string> dependencies;
            for (auto rhs = assignment + 1; rhs < end; ++rhs) {
                if (tokens[rhs].kind != parser::HdlTokenKind::Identifier) continue;
                const auto name = lower(tokens[rhs].text);
                if (!reserved.contains(name)) dependencies.insert(name);
            }
            ConstantState state = ConstantState::Static;
            std::string dynamicDependency;
            for (const auto& dependency : dependencies) {
                if (runtimeNames.contains(dependency)) {
                    state = ConstantState::Dynamic;
                    dynamicDependency = dependency;
                    break;
                }
                const auto known = states.find(dependency);
                if (known == states.end()) {
                    if (state != ConstantState::Dynamic) state = ConstantState::Unknown;
                    continue;
                }
                if (known->second == ConstantState::Dynamic) {
                    state = ConstantState::Dynamic;
                    dynamicDependency = dependency;
                    break;
                }
                if (known->second == ConstantState::Unknown)
                    state = ConstantState::Unknown;
            }
            for (const auto& [name, location] : names) {
                states[name] = state;
                if (state == ConstantState::Dynamic)
                    emit(result, "GJB9765.4.6.c", source,
                         tokens[location].line, tokens[location].column,
                         "常量 " + tokens[location].text +
                             " 的初值依赖运行期对象：" + dynamicDependency);
            }
            token = end;
        }

        // Native second branch: only <= assignments whose directly declared
        // signal/variable target carries the integer flag. Unknown names are not
        // promoted to rejected constants, so they stay under manual review.
        for (auto target = architecture.declarationsBegin;
             target + 2 < architecture.end; ++target) {
            if (tokens[target].kind != parser::HdlTokenKind::Identifier ||
                tokens[target + 1].text != "<=" ||
                !integerRuntimeNames.contains(lower(tokens[target].text)) ||
                integerDeclarationLines.contains(tokens[target].line)) continue;
            auto end = target + 2;
            int parentheses = 0;
            for (; end < architecture.end; ++end) {
                if (tokens[end].text == "(") ++parentheses;
                else if (tokens[end].text == ")" && parentheses > 0) --parentheses;
                else if (parentheses == 0 && tokens[end].text == ";") break;
            }
            if (end >= architecture.end) break;
            for (auto rhs = target + 2; rhs < end; ++rhs) {
                if (tokens[rhs].kind != parser::HdlTokenKind::Identifier) continue;
                const auto dependency = lower(tokens[rhs].text);
                const auto known = states.find(dependency);
                if (known == states.end() || known->second != ConstantState::Dynamic)
                    continue;
                emit(result, "GJB9765.4.6.c", source,
                     tokens[target].line, tokens[target].column,
                     "常量 " + tokens[target].text + " 的赋值表达式中 " +
                         tokens[rhs].text + " 不是固定整型值或可计算的固定整型值");
            }
            target = end;
        }
    }
}

void checkVhdlImplicitNumericConversions(
    AnalysisResult& result, const core::SourceFile& source,
    const std::vector<parser::HdlToken>& tokens) {
    // Native GJB9765_VHDL_4_6_d helper FUN_180604790 records declaration
    // names by flags: +0x109 integer, +0x10d real, +0x10c unsigned and +0xa4
    // signed. It admits two-child AST_ASSIGN_LE(0x56)/AST_VARASGN(0xa2), and
    // FUN_1806050c0 recursively collects typed/identifier RHS descendants.
    // FUN_180603930 reports ten directed mismatches: every direction among
    // integer/real/unsigned plus signed<->integer and signed<->unsigned. There
    // is no signed<->real branch. The entry passes the first AST root of every
    // input record to FUN_180604790 without an architecture-type gate, so
    // independent package roots and declaration-initializer AST_VARASGN nodes
    // participate too. Casts, subtype/package resolution and unknown names
    // remain ManualReviewRequired.
    // ev_7f608e30d131fb45d96138de9db5bfeb18062745bb058c8c680617e43ab2b3fa
    enum class NumericKind { Integer, Real, Unsigned, Signed };
    struct ArchitectureRange {
        std::string entity;
        std::size_t begin{};
        std::size_t end{};
    };
    const auto numericKind = [&](std::size_t begin, std::size_t end)
        -> std::optional<NumericKind> {
        for (auto token = begin; token < end; ++token) {
            const auto word = lower(tokens[token].text);
            if (word == "integer") return NumericKind::Integer;
            if (word == "real") return NumericKind::Real;
            if (word == "unsigned") return NumericKind::Unsigned;
            if (word == "signed") return NumericKind::Signed;
        }
        return std::nullopt;
    };
    const auto mismatch = [](NumericKind left, NumericKind right) {
        if (left == right) return false;
        if ((left == NumericKind::Signed && right == NumericKind::Real) ||
            (left == NumericKind::Real && right == NumericKind::Signed)) return false;
        return true;
    };
    const auto kindName = [](NumericKind kind) -> std::string_view {
        switch (kind) {
        case NumericKind::Integer: return "整数";
        case NumericKind::Real: return "实数";
        case NumericKind::Unsigned: return "无符号数";
        case NumericKind::Signed: return "有符号数";
        }
        return "未知";
    };
    const auto collectDeclarations = [&](std::size_t begin, std::size_t end,
                                         std::map<std::string, NumericKind> declared) {
        for (auto token = begin; token < end; ++token) {
            const auto word = lower(tokens[token].text);
            if (word != "signal" && word != "variable" && word != "constant") continue;
            auto colon = token + 1;
            while (colon < end && tokens[colon].text != ":" &&
                   tokens[colon].text != ";") ++colon;
            if (colon >= end || tokens[colon].text != ":") continue;
            auto typeEnd = colon + 1;
            int depth = 0;
            for (; typeEnd < end; ++typeEnd) {
                if (tokens[typeEnd].text == "(") ++depth;
                else if (tokens[typeEnd].text == ")" && depth > 0) --depth;
                else if (depth == 0 && (tokens[typeEnd].text == ";" ||
                         tokens[typeEnd].text == ":=")) break;
            }
            if (const auto kind = numericKind(colon + 1, typeEnd))
                for (auto name = token + 1; name < colon; ++name)
                    if (tokens[name].kind == parser::HdlTokenKind::Identifier)
                        declared[lower(tokens[name].text)] = *kind;
        }
        return declared;
    };
    const auto scanImplicitAssignments = [&](std::size_t begin, std::size_t rangeEnd,
                                             const auto& declared) {
        std::set<std::tuple<std::size_t, std::string, std::string>> reported;
        const auto hasCompleteOuterCast = [&](std::size_t rhsBegin, std::size_t end) {
            if (rhsBegin + 1 >= end ||
                tokens[rhsBegin].kind != parser::HdlTokenKind::Identifier ||
                tokens[rhsBegin + 1].text != "(") return false;
            const auto cast = lower(tokens[rhsBegin].text);
            return (cast == "integer" || cast == "real" || cast == "unsigned" ||
                    cast == "signed") && closeParen(tokens, rhsBegin + 1) + 1 == end;
        };
        const auto inspectRhs = [&](std::size_t targetToken, std::string_view targetText,
                                    NumericKind targetKind, std::size_t rhsBegin,
                                    std::size_t end) {
            if (hasCompleteOuterCast(rhsBegin, end)) return;
            const auto target = lower(std::string(targetText));
            for (auto rhs = rhsBegin; rhs < end; ++rhs) {
                if (tokens[rhs].kind != parser::HdlTokenKind::Identifier) continue;
                const auto sourceName = lower(tokens[rhs].text);
                const auto sourceType = declared.find(sourceName);
                if (sourceType == declared.end() ||
                    !mismatch(targetKind, sourceType->second) ||
                    !reported.emplace(tokens[targetToken].offset, target, sourceName).second)
                    continue;
                emit(result, "GJB9765.4.6.d", source,
                     tokens[targetToken].line, tokens[targetToken].column,
                     "赋值在" + std::string(kindName(targetKind)) + " " +
                         std::string(targetText) + " 与" +
                         std::string(kindName(sourceType->second)) + " " +
                         tokens[rhs].text + " 之间使用了隐式转换");
            }
        };

        // VHDL object initializers lower to AST_VARASGN(0xa2) as well. Analyze
        // each declared target against the shared RHS before statement scans.
        for (auto token = begin; token < rangeEnd; ++token) {
            const auto word = lower(tokens[token].text);
            if (word != "signal" && word != "variable" && word != "constant") continue;
            auto colon = token + 1;
            while (colon < rangeEnd && tokens[colon].text != ":" &&
                   tokens[colon].text != ";") ++colon;
            if (colon >= rangeEnd || tokens[colon].text != ":") continue;
            auto assignment = colon + 1;
            int depth = 0;
            for (; assignment < rangeEnd; ++assignment) {
                if (tokens[assignment].text == "(") ++depth;
                else if (tokens[assignment].text == ")" && depth > 0) --depth;
                else if (depth == 0 && (tokens[assignment].text == ":=" ||
                         tokens[assignment].text == ";")) break;
            }
            if (assignment >= rangeEnd || tokens[assignment].text != ":=") continue;
            auto end = assignment + 1;
            depth = 0;
            for (; end < rangeEnd; ++end) {
                if (tokens[end].text == "(") ++depth;
                else if (tokens[end].text == ")" && depth > 0) --depth;
                else if (depth == 0 && tokens[end].text == ";") break;
            }
            if (end >= rangeEnd) break;
            for (auto name = token + 1; name < colon; ++name) {
                if (tokens[name].kind != parser::HdlTokenKind::Identifier) continue;
                const auto targetType = declared.find(lower(tokens[name].text));
                if (targetType != declared.end())
                    inspectRhs(name, tokens[name].text, targetType->second,
                               assignment + 1, end);
            }
            token = end;
        }

        for (auto token = begin; token + 2 < rangeEnd; ++token) {
            if (tokens[token].kind != parser::HdlTokenKind::Identifier ||
                (tokens[token + 1].text != "<=" && tokens[token + 1].text != ":=")) continue;
            const auto target = lower(tokens[token].text);
            const auto targetType = declared.find(target);
            if (targetType == declared.end()) continue;
            if (tokens[token + 1].text == "<=" && token > begin) {
                const auto previous = lower(tokens[token - 1].text);
                if (tokens[token - 1].text != ";" && tokens[token - 1].text != ":" &&
                    previous != "begin" && previous != "then" && previous != "else" &&
                    previous != "loop") continue;
            }
            auto end = token + 2;
            int depth = 0;
            bool controlExpression = false;
            for (; end < rangeEnd; ++end) {
                if (tokens[end].text == "(") ++depth;
                else if (tokens[end].text == ")" && depth > 0) --depth;
                else if (depth == 0 && tokens[end].text == ";") break;
                else if (depth == 0 && (lower(tokens[end].text) == "then" ||
                         lower(tokens[end].text) == "loop" ||
                         lower(tokens[end].text) == "generate")) {
                    controlExpression = true;
                    break;
                }
            }
            if (end >= rangeEnd || controlExpression) continue;
            inspectRhs(token, tokens[token].text, targetType->second, token + 2, end);
            token = end;
        }
    };

    std::map<std::string, std::map<std::string, NumericKind>> portsByEntity;
    for (std::size_t entity = 0; entity + 2 < tokens.size(); ++entity) {
        if (lower(tokens[entity].text) != "entity" ||
            (entity > 0 && lower(tokens[entity - 1].text) == "end") ||
            tokens[entity + 1].kind != parser::HdlTokenKind::Identifier) continue;
        const auto entityName = lower(tokens[entity + 1].text);
        auto entityEnd = entity + 2;
        while (entityEnd + 1 < tokens.size() &&
               !(lower(tokens[entityEnd].text) == "end" &&
                 (lower(tokens[entityEnd + 1].text) == "entity" ||
                  lower(tokens[entityEnd + 1].text) == entityName ||
                  tokens[entityEnd + 1].text == ";"))) ++entityEnd;
        for (auto port = entity + 2; port + 1 < entityEnd; ++port) {
            if (lower(tokens[port].text) != "port" || tokens[port + 1].text != "(") continue;
            const auto close = closeParen(tokens, port + 1);
            if (close >= entityEnd) break;
            auto segment = port + 2;
            int depth = 0;
            for (auto cursor = segment; cursor <= close; ++cursor) {
                if (cursor < close && tokens[cursor].text == "(") { ++depth; continue; }
                if (cursor < close && tokens[cursor].text == ")" && depth > 0) {
                    --depth;
                    continue;
                }
                if (cursor != close && !(depth == 0 && tokens[cursor].text == ";")) continue;
                auto colon = segment;
                while (colon < cursor && tokens[colon].text != ":") ++colon;
                if (colon < cursor) {
                    if (const auto kind = numericKind(colon + 1, cursor))
                        for (auto name = segment; name < colon; ++name)
                            if (tokens[name].kind == parser::HdlTokenKind::Identifier)
                                portsByEntity[entityName][lower(tokens[name].text)] = *kind;
                }
                segment = cursor + 1;
            }
            port = close;
        }
        entity = entityEnd;
    }

    std::vector<ArchitectureRange> architectures;
    for (std::size_t index = 0; index + 3 < tokens.size(); ++index) {
        if (lower(tokens[index].text) != "architecture" ||
            (index > 0 && lower(tokens[index - 1].text) == "end") ||
            lower(tokens[index + 2].text) != "of") continue;
        ArchitectureRange range{lower(tokens[index + 3].text), index, tokens.size()};
        const auto architectureName = lower(tokens[index + 1].text);
        std::vector<std::string> subprogramNames;
        std::vector<std::string> localPackageNames;
        for (auto cursor = index + 4; cursor < tokens.size(); ++cursor) {
            const auto word = lower(tokens[cursor].text);
            if ((word == "function" || word == "procedure") &&
                lower(tokens[cursor - 1].text) != "end") {
                auto marker = cursor + 1;
                int parentheses = 0;
                for (; marker < tokens.size(); ++marker) {
                    if (tokens[marker].text == "(") ++parentheses;
                    else if (tokens[marker].text == ")" && parentheses > 0) --parentheses;
                    else if (parentheses == 0 &&
                             (lower(tokens[marker].text) == "is" ||
                              tokens[marker].text == ";")) break;
                }
                if (marker < tokens.size() && lower(tokens[marker].text) == "is" &&
                    cursor + 1 < tokens.size())
                    subprogramNames.push_back(lower(tokens[cursor + 1].text));
                continue;
            }
            if (word == "package" && lower(tokens[cursor - 1].text) != "end") {
                const auto localName = cursor + 1 < tokens.size() &&
                    lower(tokens[cursor + 1].text) == "body" ? cursor + 2 : cursor + 1;
                auto marker = localName + 1;
                while (marker < tokens.size() && lower(tokens[marker].text) != "is" &&
                       tokens[marker].text != ";") ++marker;
                if (localName < tokens.size() && marker + 1 < tokens.size() &&
                    lower(tokens[marker].text) == "is" &&
                    lower(tokens[marker + 1].text) != "new")
                    localPackageNames.push_back(lower(tokens[localName].text));
                continue;
            }
            if (word == "end" && cursor + 1 < tokens.size()) {
                const auto next = lower(tokens[cursor + 1].text);
                if (!subprogramNames.empty() &&
                    (next == "function" || next == "procedure" ||
                     next == subprogramNames.back() || tokens[cursor + 1].text == ";")) {
                    subprogramNames.pop_back();
                    continue;
                }
                if (!localPackageNames.empty() &&
                    (next == "package" || next == localPackageNames.back() ||
                     tokens[cursor + 1].text == ";")) {
                    localPackageNames.pop_back();
                    continue;
                }
                if (next == "architecture" || next == architectureName ||
                    tokens[cursor + 1].text == ";") {
                    auto semicolon = cursor + 1;
                    while (semicolon < tokens.size() && tokens[semicolon].text != ";")
                        ++semicolon;
                    range.end = semicolon < tokens.size() ? semicolon + 1 : cursor + 1;
                    break;
                }
                continue;
            }
            if (!subprogramNames.empty() || !localPackageNames.empty()) continue;

            // A well-formed architecture has an explicit end. These syntactic
            // design-unit starts are a recovery boundary for incomplete input,
            // and mirror the native per-input-root state reset without treating
            // direct entity instantiations or package instantiations as roots.
            const bool startsEntity = word == "entity" && cursor + 2 < tokens.size() &&
                tokens[cursor + 1].kind == parser::HdlTokenKind::Identifier &&
                lower(tokens[cursor + 2].text) == "is";
            const bool startsArchitecture = word == "architecture" &&
                cursor + 3 < tokens.size() && lower(tokens[cursor + 2].text) == "of";
            const auto packageName = cursor + 1 < tokens.size() &&
                lower(tokens[cursor + 1].text) == "body" ? cursor + 2 : cursor + 1;
            const bool startsPackage = word == "package" && packageName < tokens.size() &&
                tokens[packageName].kind == parser::HdlTokenKind::Identifier &&
                packageName + 1 < tokens.size() &&
                lower(tokens[packageName + 1].text) == "is" &&
                (packageName + 2 >= tokens.size() ||
                 lower(tokens[packageName + 2].text) != "new");
            const bool startsConfiguration = word == "configuration" &&
                cursor + 4 < tokens.size() && lower(tokens[cursor + 2].text) == "of";
            if (startsEntity || startsArchitecture || startsPackage || startsConfiguration) {
                range.end = cursor;
                break;
            }
        }
        architectures.push_back(range);
        index = range.end == 0 ? index : range.end - 1;
    }

    for (const auto& architecture : architectures) {
        std::map<std::string, NumericKind> declared;
        if (const auto ports = portsByEntity.find(architecture.entity);
            ports != portsByEntity.end()) declared = ports->second;
        declared = collectDeclarations(architecture.begin, architecture.end,
                                       std::move(declared));
        scanImplicitAssignments(architecture.begin, architecture.end, declared);
    }

    const auto scanMixedSignedness = [&](std::size_t rootBegin, std::size_t rootEnd,
                                         const auto& declared) {
        // Native GJB9765_VHDL_4_6_e slot 0 FUN_180605b20 clears its
        // containers for every input record and calls FUN_1806067a0 on that
        // record's first AST root without an architecture-type gate.
        // FUN_1806067a0 records declarations carrying +0xa4 (signed)
        // and +0x10c (unsigned). For each exactly-two-child operator in the
        // native mask 0xe0fcf001f (AST 0x24..0x28, 0x34..0x37,
        // 0x3a..0x3f and 0x45..0x47), FUN_180607060 recursively collects
        // identifiers/typed descendants from both operands. One diagnostic is
        // emitted when that operator subtree resolves at least one signed and
        // one unsigned name. Independent enum evidence maps the admitted
        // binary families to bit/logical, six relations and +,-,*,/; mod,
        // rem, shifts, concatenation and power are not in this native mask.
        // This source adapter covers assignment/initializer expressions and
        // direct control expressions within each independent root. Subtype,
        // function-return and cross-unit type resolution remain
        // ManualReviewRequired.
        struct ExpressionRange { std::size_t begin; std::size_t end; };
        std::set<std::pair<std::size_t, std::size_t>> expressionKeys;
        std::vector<ExpressionRange> expressions;
        const auto addExpression = [&](std::size_t begin, std::size_t end) {
            if (begin >= end || begin >= rootEnd) return;
            end = std::min(end, rootEnd);
            if (expressionKeys.emplace(begin, end).second)
                expressions.push_back({begin, end});
        };
        const auto expressionEnd = [&](std::size_t begin,
                                       const std::set<std::string>& boundaries) {
            int depth = 0;
            for (auto cursor = begin; cursor < rootEnd; ++cursor) {
                if (tokens[cursor].text == "(") ++depth;
                else if (tokens[cursor].text == ")" && depth > 0) --depth;
                else if (depth == 0 && boundaries.contains(lower(tokens[cursor].text)))
                    return cursor;
            }
            return rootEnd;
        };
        for (auto token = rootBegin; token < rootEnd; ++token) {
            const auto word = lower(tokens[token].text);
            if (tokens[token].text == ":=") {
                addExpression(token + 1, expressionEnd(token + 1, {";"}));
                continue;
            }
            if (tokens[token].text == "<=" && token > rootBegin &&
                tokens[token - 1].kind == parser::HdlTokenKind::Identifier) {
                const auto previous = token >= rootBegin + 2
                    ? lower(tokens[token - 2].text) : std::string{};
                if (token == rootBegin + 1 || tokens[token - 2].text == ";" ||
                    tokens[token - 2].text == ":" || previous == "begin" ||
                    previous == "then" || previous == "else" || previous == "loop")
                    addExpression(token + 1, expressionEnd(token + 1, {";"}));
                continue;
            }
            if ((word == "if" || word == "elsif") &&
                !(token > rootBegin && lower(tokens[token - 1].text) == "end")) {
                addExpression(token + 1, expressionEnd(token + 1, {"then"}));
            } else if (word == "while") {
                addExpression(token + 1, expressionEnd(token + 1, {"loop"}));
            } else if (word == "assert") {
                addExpression(token + 1,
                              expressionEnd(token + 1, {"report", "severity", ";"}));
            } else if (word == "return") {
                addExpression(token + 1, expressionEnd(token + 1, {";"}));
            } else if (word == "case") {
                addExpression(token + 1, expressionEnd(token + 1, {"is"}));
            }
        }

        struct OperatorInfo { int precedence; bool reports; };
        const auto operatorInfo = [](std::string_view text)
            -> std::optional<OperatorInfo> {
            if (text == "or" || text == "nor" || text == "xor" || text == "xnor" ||
                text == "and" || text == "nand") return OperatorInfo{1, true};
            if (text == "=" || text == "/=" || text == "<" || text == "<=" ||
                text == ">" || text == ">=") return OperatorInfo{2, true};
            if (text == "sll" || text == "srl" || text == "sla" || text == "sra" ||
                text == "rol" || text == "ror") return OperatorInfo{3, false};
            if (text == "&") return OperatorInfo{4, false};
            if (text == "+" || text == "-") return OperatorInfo{4, true};
            if (text == "*" || text == "/") return OperatorInfo{5, true};
            if (text == "mod" || text == "rem") return OperatorInfo{5, false};
            if (text == "**") return OperatorInfo{6, false};
            return std::nullopt;
        };
        const auto canEndOperand = [&](std::size_t token) {
            if (tokens[token].kind == parser::HdlTokenKind::Identifier ||
                tokens[token].kind == parser::HdlTokenKind::Number ||
                tokens[token].kind == parser::HdlTokenKind::String ||
                tokens[token].text == ")") return true;
            const auto text = lower(tokens[token].text);
            return text == "'0'" || text == "'1'" || text == "true" || text == "false";
        };
        std::set<std::size_t> reportedOperators;
        const auto scanExpression = [&](auto&& self, std::size_t begin,
                                        std::size_t end) -> void {
            while (begin < end && tokens[begin].text == "(" &&
                   closeParen(tokens, begin) + 1 == end) {
                ++begin;
                --end;
            }
            if (begin >= end) return;

            int depth = 0;
            std::size_t segment = begin;
            bool split = false;
            for (auto cursor = begin; cursor < end; ++cursor) {
                if (tokens[cursor].text == "(") ++depth;
                else if (tokens[cursor].text == ")" && depth > 0) --depth;
                else if (depth == 0 && (lower(tokens[cursor].text) == "when" ||
                         lower(tokens[cursor].text) == "else" ||
                         tokens[cursor].text == "," || tokens[cursor].text == "=>")) {
                    self(self, segment, cursor);
                    segment = cursor + 1;
                    split = true;
                }
            }
            if (split) {
                self(self, segment, end);
                return;
            }

            std::optional<std::size_t> root;
            int rootPrecedence = std::numeric_limits<int>::max();
            depth = 0;
            for (auto cursor = begin; cursor < end; ++cursor) {
                if (tokens[cursor].text == "(") { ++depth; continue; }
                if (tokens[cursor].text == ")") { if (depth > 0) --depth; continue; }
                if (depth != 0) continue;
                const auto info = operatorInfo(lower(tokens[cursor].text));
                if (!info || cursor == begin || cursor + 1 >= end ||
                    !canEndOperand(cursor - 1)) continue;
                if (info->precedence <= rootPrecedence) {
                    root = cursor; // rightmost at equal precedence: left associative.
                    rootPrecedence = info->precedence;
                }
            }
            if (root) {
                const auto info = *operatorInfo(lower(tokens[*root].text));
                if (info.reports && reportedOperators.insert(tokens[*root].offset).second) {
                    std::optional<std::pair<std::string, std::size_t>> signedName;
                    std::optional<std::pair<std::string, std::size_t>> unsignedName;
                    for (auto cursor = begin; cursor < end; ++cursor) {
                        if (tokens[cursor].kind != parser::HdlTokenKind::Identifier) continue;
                        const auto found = declared.find(lower(tokens[cursor].text));
                        if (found == declared.end()) continue;
                        if (found->second == NumericKind::Signed && !signedName)
                            signedName = std::pair{tokens[cursor].text, cursor};
                        if (found->second == NumericKind::Unsigned && !unsignedName)
                            unsignedName = std::pair{tokens[cursor].text, cursor};
                    }
                    if (signedName && unsignedName)
                        emit(result, "GJB9765.4.6.e", source,
                             tokens[*root].line, tokens[*root].column,
                             "同一运算式混用了有符号数 " + signedName->first +
                                 " 和无符号数 " + unsignedName->first);
                }
                self(self, begin, *root);
                self(self, *root + 1, end);
                return;
            }

            // No top-level binary operator: inspect function/index arguments
            // and nested parenthesized expressions without combining siblings.
            for (auto cursor = begin; cursor < end; ++cursor) {
                if (tokens[cursor].text != "(") continue;
                const auto close = closeParen(tokens, cursor);
                if (close >= end) break;
                self(self, cursor + 1, close);
                cursor = close;
            }
        };
        for (const auto& expression : expressions)
            scanExpression(scanExpression, expression.begin, expression.end);
    };

    for (const auto& architecture : architectures) {
        std::map<std::string, NumericKind> declared;
        if (const auto ports = portsByEntity.find(architecture.entity);
            ports != portsByEntity.end()) declared = ports->second;
        declared = collectDeclarations(architecture.begin, architecture.end,
                                       std::move(declared));
        scanMixedSignedness(architecture.begin, architecture.end, declared);
    }

    // Both recovered native entries have no architecture gate. Cover
    // independent package specifications and bodies as separate roots for
    // 4.6.d and 4.6.e, resetting declaration maps at every root.
    struct PackageRange { std::size_t begin{}; std::size_t end{}; };
    std::vector<PackageRange> packages;
    for (std::size_t index = 0; index + 2 < tokens.size(); ++index) {
        if (lower(tokens[index].text) != "package" ||
            (index > 0 && lower(tokens[index - 1].text) == "end") ||
            std::ranges::any_of(architectures, [index](const auto& architecture) {
                return architecture.begin < index && index < architecture.end;
            })) continue;
        const bool isBody = lower(tokens[index + 1].text) == "body";
        const auto nameToken = index + (isBody ? 2U : 1U);
        if (nameToken >= tokens.size() ||
            tokens[nameToken].kind != parser::HdlTokenKind::Identifier) continue;
        auto isToken = nameToken + 1;
        while (isToken < tokens.size() && lower(tokens[isToken].text) != "is" &&
               tokens[isToken].text != ";") ++isToken;
        if (isToken >= tokens.size() || lower(tokens[isToken].text) != "is" ||
            (isToken + 1 < tokens.size() && lower(tokens[isToken + 1].text) == "new"))
            continue;

        PackageRange range{index, tokens.size()};
        const auto packageName = lower(tokens[nameToken].text);
        std::vector<std::string> subprogramNames;
        std::vector<std::string> localPackageNames;
        for (auto cursor = isToken + 1; cursor < tokens.size(); ++cursor) {
            const auto word = lower(tokens[cursor].text);
            if ((word == "function" || word == "procedure") &&
                (cursor == 0 || lower(tokens[cursor - 1].text) != "end")) {
                auto marker = cursor + 1;
                int parentheses = 0;
                for (; marker < tokens.size(); ++marker) {
                    if (tokens[marker].text == "(") ++parentheses;
                    else if (tokens[marker].text == ")" && parentheses > 0) --parentheses;
                    else if (parentheses == 0 &&
                             (lower(tokens[marker].text) == "is" ||
                              tokens[marker].text == ";")) break;
                }
                if (marker < tokens.size() && lower(tokens[marker].text) == "is")
                    subprogramNames.push_back(lower(tokens[cursor + 1].text));
                continue;
            }
            if (word == "package" &&
                (cursor == 0 || lower(tokens[cursor - 1].text) != "end")) {
                const auto localName = cursor + 1 < tokens.size() &&
                    lower(tokens[cursor + 1].text) == "body" ? cursor + 2 : cursor + 1;
                auto marker = localName + 1;
                while (marker < tokens.size() && lower(tokens[marker].text) != "is" &&
                       tokens[marker].text != ";") ++marker;
                if (marker + 1 < tokens.size() && lower(tokens[marker].text) == "is" &&
                    lower(tokens[marker + 1].text) != "new")
                    localPackageNames.push_back(lower(tokens[localName].text));
                continue;
            }
            if (word != "end" || cursor + 1 >= tokens.size()) continue;
            const auto next = lower(tokens[cursor + 1].text);
            if (!subprogramNames.empty() &&
                (next == "function" || next == "procedure" ||
                 next == subprogramNames.back() || tokens[cursor + 1].text == ";")) {
                subprogramNames.pop_back();
                continue;
            }
            if (!localPackageNames.empty() &&
                (next == "package" || next == localPackageNames.back() ||
                 tokens[cursor + 1].text == ";")) {
                localPackageNames.pop_back();
                continue;
            }
            if (next != "package" && next != packageName &&
                tokens[cursor + 1].text != ";") continue;
            auto semicolon = cursor + 2;
            while (semicolon < tokens.size() && tokens[semicolon].text != ";") ++semicolon;
            if (tokens[cursor + 1].text == ";") semicolon = cursor + 1;
            range.end = semicolon < tokens.size() ? semicolon + 1 : cursor;
            break;
        }
        packages.push_back(range);
        index = range.end == 0 ? index : range.end - 1;
    }
    for (const auto& package : packages) {
        const auto declared = collectDeclarations(package.begin, package.end, {});
        scanImplicitAssignments(package.begin, package.end, declared);
        scanMixedSignedness(package.begin, package.end, declared);
    }
}

void checkVhdlConstantControlConditions(
    AnalysisResult& result, const core::SourceFile& source,
    const std::vector<parser::HdlToken>& tokens) {
    // Native slot 0 FUN_1807849b0 builds declaration/name maps through
    // FUN_180785f30, then enables FUN_1807856c0 and FUN_1807857a0.  The latter
    // admits only type 0x58 (the packaged rule's WHILE node).  A one-child
    // condition is constant when it is a direct literal (0x10) or a name
    // resolved in the constant map (0x13).  A two-child condition is constant
    // only for relation types 0x34..0x37/0x3a..0x3b when both direct operands
    // are literals or resolved constant names.  It classifies static shape;
    // the truth value is irrelevant.
    const auto architectureAt = vhdlArchitectureOrdinals(tokens);
    std::map<std::size_t, std::vector<std::pair<std::size_t, std::string>>>
        constantsByArchitecture;
    for (std::size_t token = 0; token < tokens.size(); ++token) {
        if (lower(tokens[token].text) != "constant") continue;
        auto colon = token + 1;
        auto end = token + 1;
        for (; end < tokens.size() && tokens[end].text != ";"; ++end)
            if (tokens[end].text == ":" && colon == token + 1) colon = end;
        if (colon == token + 1 || end == tokens.size()) continue;
        const bool initialized = std::ranges::any_of(
            tokens.begin() + static_cast<std::ptrdiff_t>(colon + 1),
            tokens.begin() + static_cast<std::ptrdiff_t>(end),
            [](const auto& item) { return item.text == ":="; });
        if (!initialized) continue; // A deferred constant has no local value.
        for (auto name = token + 1; name < colon; ++name)
            if (tokens[name].kind == parser::HdlTokenKind::Identifier)
                constantsByArchitecture[architectureAt[token]].push_back(
                    {token, lower(tokens[name].text)});
        token = end;
    }

    // VHDL generics are elaboration-time constants and occupy the same native
    // resolved-name side of this predicate.  Bind entity GENERIC clauses to
    // the matching ARCHITECTURE ... OF entity instead of borrowing component
    // generics or names from a different design unit.
    std::map<std::string, std::set<std::string>> genericsByEntity;
    for (std::size_t entity = 0; entity + 2 < tokens.size(); ++entity) {
        if (lower(tokens[entity].text) != "entity" ||
            (entity > 0 && lower(tokens[entity - 1].text) == "end") ||
            tokens[entity + 1].kind != parser::HdlTokenKind::Identifier ||
            lower(tokens[entity + 2].text) != "is") continue;
        const auto entityName = lower(tokens[entity + 1].text);
        auto endEntity = entity + 3;
        for (; endEntity < tokens.size(); ++endEntity) {
            if (lower(tokens[endEntity].text) == "architecture") break;
            if (lower(tokens[endEntity].text) == "end" && endEntity + 1 < tokens.size() &&
                (tokens[endEntity + 1].text == ";" ||
                 lower(tokens[endEntity + 1].text) == "entity" ||
                 lower(tokens[endEntity + 1].text) == entityName)) break;
        }
        for (auto token = entity + 3; token + 1 < endEntity; ++token) {
            if (lower(tokens[token].text) != "generic" || tokens[token + 1].text != "(")
                continue;
            const auto close = closeParen(tokens, token + 1);
            if (close >= endEntity) break;
            auto segment = token + 2;
            for (auto cursor = segment; cursor <= close; ++cursor) {
                if (cursor != close && tokens[cursor].text != ";") continue;
                auto colon = segment;
                while (colon < cursor && tokens[colon].text != ":") ++colon;
                if (colon < cursor)
                    for (auto name = segment; name < colon; ++name)
                        if (tokens[name].kind == parser::HdlTokenKind::Identifier)
                            genericsByEntity[entityName].insert(lower(tokens[name].text));
                segment = cursor + 1;
            }
            token = close;
        }
        entity = endEntity;
    }
    std::map<std::size_t, std::string> architectureEntity;
    for (std::size_t token = 0; token + 1 < tokens.size(); ++token) {
        if (lower(tokens[token].text) != "architecture" ||
            (token > 0 && lower(tokens[token - 1].text) == "end")) continue;
        for (auto cursor = token + 1; cursor + 1 < tokens.size() &&
             lower(tokens[cursor].text) != "is"; ++cursor) {
            if (lower(tokens[cursor].text) == "of" &&
                tokens[cursor + 1].kind == parser::HdlTokenKind::Identifier) {
                architectureEntity[architectureAt[token]] = lower(tokens[cursor + 1].text);
                break;
            }
        }
    }
    const auto staticNamesAt = [&](std::size_t token) {
        std::set<std::string> staticNames{"true", "false"};
        const auto architecture = architectureAt[token];
        if (const auto declarations = constantsByArchitecture.find(architecture);
            declarations != constantsByArchitecture.end())
            for (const auto& [position, name] : declarations->second)
                if (position < token) staticNames.insert(name);
        if (const auto entity = architectureEntity.find(architecture);
            entity != architectureEntity.end())
            if (const auto generics = genericsByEntity.find(entity->second);
                generics != genericsByEntity.end())
                staticNames.insert(generics->second.begin(), generics->second.end());
        return staticNames;
    };

    const auto strip = [&](std::size_t& begin, std::size_t& end) {
        while (begin < end && tokens[begin].text == "(" &&
               closeParen(tokens, begin) + 1 == end) {
            ++begin;
            --end;
        }
    };
    const auto staticAtom = [&](std::size_t begin, std::size_t end,
                                const std::set<std::string>& staticNames) {
        strip(begin, end);
        if (begin + 1 != end) return false;
        const auto& item = tokens[begin];
        if (item.kind == parser::HdlTokenKind::Number ||
            item.kind == parser::HdlTokenKind::String) return true;
        return item.kind == parser::HdlTokenKind::Identifier &&
               staticNames.contains(lower(item.text));
    };
    const auto staticCondition = [&](std::size_t begin, std::size_t end,
                                     const std::set<std::string>& staticNames) {
        strip(begin, end);
        if (staticAtom(begin, end, staticNames)) return true;
        int depth = 0;
        std::size_t relation = end;
        for (auto cursor = begin; cursor < end; ++cursor) {
            if (tokens[cursor].text == "(") { ++depth; continue; }
            if (tokens[cursor].text == ")") { --depth; continue; }
            if (depth == 0 && (tokens[cursor].text == "=" ||
                tokens[cursor].text == "/=" || tokens[cursor].text == "<" ||
                tokens[cursor].text == "<=" || tokens[cursor].text == ">" ||
                tokens[cursor].text == ">=")) {
                if (relation != end) return false;
                relation = cursor;
            }
        }
        return relation != end && staticAtom(begin, relation, staticNames) &&
               staticAtom(relation + 1, end, staticNames);
    };

    for (std::size_t token = 0; token < tokens.size(); ++token) {
        if (lower(tokens[token].text) != "while" ||
            (token > 0 && lower(tokens[token - 1].text) == "end")) continue;
        auto loop = token + 1;
        int parentheses = 0;
        for (; loop < tokens.size(); ++loop) {
            if (tokens[loop].text == "(") ++parentheses;
            else if (tokens[loop].text == ")" && parentheses > 0) --parentheses;
            else if (parentheses == 0 && lower(tokens[loop].text) == "loop") break;
            else if (parentheses == 0 && tokens[loop].text == ";") break;
        }
        if (loop == tokens.size() || lower(tokens[loop].text) != "loop") continue;
        const auto staticNames = staticNamesAt(token);
        if (!staticCondition(token + 1, loop, staticNames)) continue;
        emit(result, "GJB9765.4.7.c", source,
             tokens[token].line, tokens[token].column,
             "WHILE 条件使用了常量表达式");
    }

    // GJB9765.4.7.a uses the same base analyser with a different option mask.
    // Constructor FUN_180029360 changes bytes +0x178..+0x17a to 0,1,1:
    // FUN_1807859e0 then checks constant IF/ELSIF condition records, while
    // FUN_180785240 checks the two direct bounds of types 0x5c/0x60 and emits
    // for each identifier absent from the constant-name map.  The packaged
    // examples additionally establish constant CASE selectors.  Embedded
    // GB18030 strings at 0x180f1e2b8/0x180f1e2d0 are respectively
    // "禁止非常量的循环条件" and "禁止常量条件表达式".
    for (std::size_t token = 0; token < tokens.size(); ++token) {
        const auto word = lower(tokens[token].text);
        if ((word == "if" || word == "elsif") &&
            !(word == "if" && token > 0 && lower(tokens[token - 1].text) == "end")) {
            auto thenToken = token + 1;
            int parentheses = 0;
            for (; thenToken < tokens.size(); ++thenToken) {
                if (tokens[thenToken].text == "(") ++parentheses;
                else if (tokens[thenToken].text == ")" && parentheses > 0) --parentheses;
                else if (parentheses == 0 && lower(tokens[thenToken].text) == "then") break;
                else if (parentheses == 0 && tokens[thenToken].text == ";") break;
            }
            if (thenToken < tokens.size() && lower(tokens[thenToken].text) == "then" &&
                staticCondition(token + 1, thenToken, staticNamesAt(token)))
                emit(result, "GJB9765.4.7.a", source,
                     tokens[token].line, tokens[token].column,
                     "禁止常量条件表达式");
            continue;
        }
        if (word == "case" &&
            !(token > 0 && lower(tokens[token - 1].text) == "end")) {
            auto isToken = token + 1;
            int parentheses = 0;
            for (; isToken < tokens.size(); ++isToken) {
                if (tokens[isToken].text == "(") ++parentheses;
                else if (tokens[isToken].text == ")" && parentheses > 0) --parentheses;
                else if (parentheses == 0 && lower(tokens[isToken].text) == "is") break;
                else if (parentheses == 0 &&
                         (lower(tokens[isToken].text) == "generate" ||
                          tokens[isToken].text == ";")) break;
            }
            if (isToken < tokens.size() && lower(tokens[isToken].text) == "is" &&
                staticCondition(token + 1, isToken, staticNamesAt(token)))
                emit(result, "GJB9765.4.7.a", source,
                     tokens[token].line, tokens[token].column,
                     "禁止常量条件表达式");
        }
    }

    for (std::size_t token = 0; token + 3 < tokens.size(); ++token) {
        if (lower(tokens[token].text) != "for" ||
            tokens[token + 1].kind != parser::HdlTokenKind::Identifier ||
            lower(tokens[token + 2].text) != "in") continue;
        auto direction = tokens.size();
        auto terminator = token + 3;
        int parentheses = 0;
        for (; terminator < tokens.size(); ++terminator) {
            if (tokens[terminator].text == "(") ++parentheses;
            else if (tokens[terminator].text == ")" && parentheses > 0) --parentheses;
            else if (parentheses == 0 &&
                     (lower(tokens[terminator].text) == "to" ||
                      lower(tokens[terminator].text) == "downto")) direction = terminator;
            else if (parentheses == 0 &&
                     (lower(tokens[terminator].text) == "loop" ||
                      lower(tokens[terminator].text) == "generate" ||
                      tokens[terminator].text == ";")) break;
        }
        if (direction == tokens.size() || terminator == tokens.size() ||
            (lower(tokens[terminator].text) != "loop" &&
             lower(tokens[terminator].text) != "generate")) continue;
        const auto staticNames = staticNamesAt(token);
        const auto checkBound = [&](std::size_t begin, std::size_t end) {
            strip(begin, end);
            if (begin + 1 != end ||
                tokens[begin].kind != parser::HdlTokenKind::Identifier ||
                staticNames.contains(lower(tokens[begin].text))) return;
            emit(result, "GJB9765.4.7.a", source,
                 tokens[begin].line, tokens[begin].column,
                 "禁止非常量的循环条件：" + tokens[begin].text);
        };
        checkBound(token + 3, direction);
        checkBound(direction + 1, terminator);
        token = terminator;
    }
}

struct VhdlNativeClockRelations {
    std::set<std::string> names;
    std::set<std::size_t> sourceLines;
};

std::vector<bool> vhdlDirectArchitectureTokenMask(
    const std::vector<parser::HdlToken>& tokens,
    std::size_t architecture, std::size_t limit) {
    std::vector<bool> direct(tokens.size(), false);
    std::vector<std::string> nestedScopes;
    bool architectureStatements = false;
    const std::set<std::string_view> nestedScopeWords{
        "process", "function", "procedure", "block", "generate"
    };
    for (auto cursor = architecture; cursor < limit; ++cursor) {
        const auto word = lower(tokens[cursor].text);
        if (word == "end") {
            const auto next = cursor + 1 < limit
                ? lower(tokens[cursor + 1].text) : std::string{};
            if (nestedScopeWords.contains(next)) {
                const auto scope = std::find(
                    nestedScopes.rbegin(), nestedScopes.rend(), next);
                if (scope != nestedScopes.rend())
                    nestedScopes.erase(std::next(scope).base());
            } else if (!nestedScopes.empty() &&
                       (nestedScopes.back() == "function" ||
                        nestedScopes.back() == "procedure") &&
                       (next == ";" ||
                        (cursor + 2 < limit &&
                         tokens[cursor + 1].kind == parser::HdlTokenKind::Identifier &&
                         tokens[cursor + 2].text == ";"))) {
                nestedScopes.pop_back();
            }
            continue;
        }
        if (word == "begin" && nestedScopes.empty()) {
            architectureStatements = true;
            continue;
        }
        direct[cursor] = architectureStatements && nestedScopes.empty();
        if (!nestedScopeWords.contains(word) ||
            (cursor > architecture && lower(tokens[cursor - 1].text) == "end"))
            continue;
        if (word == "function" || word == "procedure") {
            int parentheses = 0;
            bool hasBody = false;
            for (auto lookahead = cursor + 1; lookahead < limit; ++lookahead) {
                if (tokens[lookahead].text == "(") ++parentheses;
                else if (tokens[lookahead].text == ")" && parentheses > 0) --parentheses;
                else if (parentheses == 0 && lower(tokens[lookahead].text) == "is") {
                    hasBody = true;
                    break;
                } else if (parentheses == 0 && tokens[lookahead].text == ";") {
                    break;
                }
            }
            if (!hasBody) continue;
        }
        nestedScopes.push_back(word);
    }
    return direct;
}

VhdlNativeClockRelations vhdlNativeClockRelations12(
    const std::vector<parser::HdlToken>& tokens,
    std::size_t architecture, std::size_t limit,
    const parser::DesignUnitInfo& unit,
    const core::SourceFile& source,
    const std::set<std::string>& parsedClocks);

void checkVhdlGatedClocks(AnalysisResult& result, const core::SourceFile& source,
                          const std::vector<parser::HdlToken>& tokens) {
    // GJB9765_VHDL_4_9_c is not the same predicate as the generic
    // RTL-DERIVED-CLOCK diagnostic. Native slot 0 (FUN_1806826a0) first calls
    // FUN_1806841f0 to collect normalized assignment targets and their source
    // relationships, repeatedly propagates those relationships, intersects
    // the resulting graph with elaborated clock relations, and reports the
    // assignment source location. Its two diagnostic strings are
    // "避免使用门控生成时钟." and "禁止使用时钟门控". The packaged positive
    // example is `clk_g1 <= CLK and CLK_EN`, while a primary input clock is
    // the negative control. This bounded adapter consequently follows direct
    // and unary-inverted aliases backwards from parser-confirmed VHDL clocks,
    // but emits only at a binary logical or conditional-select assignment.
    // Arithmetic, direct aliases and unary inversion alone are not classified
    // as clock gating here. Separate 4.9.e slot 0 (FUN_18060a000) intersects
    // clock relations with FUN_1804d3050's assignment-target map. In its
    // AST_ASSIGN_LE(0x56) branch, parameter 0 admits expression RHS kinds
    // 0x24..0x2c, 0x3c..0x3f, 0x45..0x47 and 0xb6, but not a bare identifier.
    // We implement the recovered source-level operator subset for 4.9.e.
    // GJB9765_VHDL_4_9_j slot 0 FUN_18060d980 constructs an AST_ALWAYS/process
    // (0x52) filter and calls recursive FUN_1806124a0, so generate/block-nested
    // processes contribute clock-path relationships. The collector stops only
    // below type 0x7b component-declaration nodes. Helper FUN_18060fc80 admits
    // a direct assignment only when
    // its RHS root is AST type 0x24 or 0x47, has exactly one child and that
    // child is AST_IDENTIFIER(0x13). Packaged examples establish this bounded
    // branch as unary NOT inserted in a clock path. Component buffers are
    // handled by separate native hierarchy/library helpers and remain unknown.
    // ev_f52ed8b0a9c20494abc2ada298da375739758e68b40927def594883a8ded4b83
    // ev_29c219f05a7448393aa834253a168ef537297d0f9fee1930afd84c280ce8d6c4
    struct Assignment {
        std::string target;
        std::vector<std::string> dependencies;
        std::vector<std::string> reconvergenceIdentifiers;
        std::size_t targetToken{};
        bool gated{};
        bool gatingRelation{};
        bool combinational{};
        bool inverted{};
        bool reconvergenceRoot{};
    };
    struct ArchitectureRange {
        std::string entity;
        std::size_t begin{};
        std::size_t end{};
    };

    std::vector<ArchitectureRange> ranges;
    for (std::size_t index = 0; index + 3 < tokens.size(); ++index) {
        if (lower(tokens[index].text) != "architecture" ||
            lower(tokens[index + 2].text) != "of") continue;
        ranges.push_back({lower(tokens[index + 3].text), index, tokens.size()});
    }
    for (std::size_t index = 0; index + 1 < ranges.size(); ++index)
        ranges[index].end = ranges[index + 1].begin;

    const std::set<std::string_view> gatingOperators{"and", "or"};
    const std::set<std::string_view> combinationalOperators{
        "not", "and", "or", "xor", "nand", "nor", "xnor", "when",
        "sll", "srl", "sla", "sra", "rol", "ror", "+", "-", "*", "/"
    };
    const std::set<std::string_view> reconvergenceOperators{
        "not", "and", "or", "xor", "nand", "nor", "xnor", "when"
    };
    for (const auto& unit : result.structure) {
        if (unit.kind != parser::DesignUnitKind::VhdlEntity ||
            unit.location.file.lexically_normal() != source.path().lexically_normal()) continue;
        std::set<std::string> clocks;
        for (const auto& clock : unit.clocks) {
            if (clock.location.file.lexically_normal() == source.path().lexically_normal())
                clocks.insert(lower(clock.signal));
        }
        if (clocks.empty()) continue;

        for (const auto& range : ranges) {
            if (range.entity != lower(unit.name)) continue;
            const auto reconvergenceClocks = vhdlNativeClockRelations12(
                tokens, range.begin, range.end, unit, source, clocks).names;
            // FUN_1806841f0 creates both the R-9-3 dependency map and gate
            // candidate map only while visiting direct AST_ARCHITECTURE
            // children. Build a bounded source ownership mask so assignments
            // inside processes, subprograms, blocks and generate regions do
            // not enter either native map.
            const auto directArchitectureStatement = vhdlDirectArchitectureTokenMask(
                tokens, range.begin, range.end);

            std::map<std::string, std::vector<Assignment>> assignments;
            std::map<std::string, std::size_t> nativeGateCandidates;
            std::map<std::string, std::size_t> nativeGateRelations;
            for (std::size_t index = range.begin; index + 2 < range.end; ++index) {
                if (tokens[index].kind != parser::HdlTokenKind::Identifier ||
                    tokens[index + 1].text != "<=") continue;
                if (index > range.begin) {
                    const auto previous = lower(tokens[index - 1].text);
                    if (tokens[index - 1].text != ";" && tokens[index - 1].text != ":" &&
                        previous != "begin" && previous != "then" && previous != "else" &&
                        previous != "loop" && previous != "select") continue;
                }
                auto end = index + 2;
                int parentheses = 0;
                for (; end < range.end; ++end) {
                    if (tokens[end].text == "(") ++parentheses;
                    else if (tokens[end].text == ")") --parentheses;
                    else if (parentheses == 0 && tokens[end].text == ";") break;
                }
                if (end == range.end) continue;
                Assignment assignment;
                assignment.target = lower(tokens[index].text);
                assignment.targetToken = index;
                std::vector<std::size_t> rhsWithoutParentheses;
                for (auto rhs = index + 2; rhs < end; ++rhs) {
                    const auto word = lower(tokens[rhs].text);
                    if (tokens[rhs].text != "(" && tokens[rhs].text != ")")
                        rhsWithoutParentheses.push_back(rhs);
                    if (tokens[rhs].kind == parser::HdlTokenKind::Identifier &&
                        word != assignment.target)
                        assignment.dependencies.push_back(word);
                    if (tokens[rhs].kind == parser::HdlTokenKind::Identifier)
                        assignment.reconvergenceIdentifiers.push_back(word);
                }
                // Native A-9-6 accepts either AST_BIT_NOT(0x24) or
                // AST_LOGIC_NOT(0x47), but in both cases the root must have
                // exactly one AST_IDENTIFIER(0x13) child.  Source VHDL uses
                // the same `not` token for both predefined overload classes;
                // keep the adapter structural so calls, selections and a
                // second unary operator cannot masquerade as that child.
                assignment.inverted = rhsWithoutParentheses.size() == 2 &&
                    lower(tokens[rhsWithoutParentheses[0]].text) == "not" &&
                    tokens[rhsWithoutParentheses[1]].kind ==
                        parser::HdlTokenKind::Identifier;
                auto rootBegin = index + 2;
                auto rootEnd = end;
                while (rootBegin < rootEnd && tokens[rootBegin].text == "(" &&
                       closeParen(tokens, rootBegin) + 1 == rootEnd) {
                    ++rootBegin;
                    --rootEnd;
                }
                // Determine the source operator corresponding to the direct
                // RHS AST root. Logical and binary arithmetic productions are
                // left-associated, so the last operator at depth zero is the
                // root; unary NOT is a root only at the first token.
                std::optional<std::string> rootOperator;
                if (rootBegin < rootEnd &&
                    lower(tokens[rootBegin].text) == "not") {
                    rootOperator = "not";
                }
                int gateDepth = 0;
                bool hasTopLevelWhen = false;
                for (auto rhs = rootBegin; rhs < rootEnd; ++rhs) {
                    if (tokens[rhs].text == "(") ++gateDepth;
                    else if (tokens[rhs].text == ")" && gateDepth > 0) --gateDepth;
                    else if (gateDepth == 0) {
                        const auto op = lower(tokens[rhs].text);
                        hasTopLevelWhen = hasTopLevelWhen || op == "when";
                        if (combinationalOperators.contains(op) && op != "when")
                            rootOperator = op;
                    }
                }
                if (hasTopLevelWhen) rootOperator = "when";
                const bool directArchitecture =
                    index < directArchitectureStatement.size() &&
                    directArchitectureStatement[index];
                assignment.gated = directArchitecture && rootOperator &&
                    gatingOperators.contains(*rootOperator);
                // The broad native map accepts RHS roots 0x24..0x28,
                // 0x2b..0x2c, 0x3c..0x3f, 0x45..0x47 and AST_WHEN(0xb6).
                // These map to direct NOT/AND/OR/XOR/XNOR/NAND/NOR,
                // binary +,-,*,/ and conditional WHEN. Bare identifiers,
                // calls, shifts and sequential assignments do not propagate.
                static const std::set<std::string_view> relationOperators{
                    "not", "and", "or", "xor", "xnor", "nand", "nor",
                    "when", "+", "-", "*", "/"
                };
                assignment.gatingRelation = directArchitecture && rootOperator &&
                    relationOperators.contains(*rootOperator) &&
                    !((*rootOperator == "+" || *rootOperator == "-") &&
                      lower(tokens[rootBegin].text) == *rootOperator);
                if (assignment.gated)
                    nativeGateCandidates[assignment.target] = assignment.targetToken;
                if (assignment.gatingRelation)
                    nativeGateRelations[assignment.target] = assignment.targetToken;
                // Native 4.9.e helper FUN_1804d3050 admits the RHS AST root,
                // not an operator nested anywhere below a call or another
                // unsupported root. Its arithmetic set is binary
                // ADD/SUB/MUL/DIV (0x3c..0x3f); unary POS/NEG are 0x43/0x44
                // and must not be folded into that set just because their
                // source token is also '+' or '-'.
                if (rootBegin < rootEnd &&
                    lower(tokens[rootBegin].text) == "not") {
                    assignment.combinational = true;
                } else {
                    int rootDepth = 0;
                    for (auto rhs = rootBegin; rhs < rootEnd; ++rhs) {
                        if (tokens[rhs].text == "(") ++rootDepth;
                        else if (tokens[rhs].text == ")" && rootDepth > 0) --rootDepth;
                        else if (rootDepth == 0 &&
                                 combinationalOperators.contains(
                                     lower(tokens[rhs].text))) {
                            const auto op = lower(tokens[rhs].text);
                            if ((op == "+" || op == "-") && rhs == rootBegin)
                                continue;
                            assignment.combinational = true;
                            break;
                        }
                    }
                }
                if (rootBegin < rootEnd &&
                    lower(tokens[rootBegin].text) == "not") {
                    assignment.reconvergenceRoot = true;
                } else {
                    int rootDepth = 0;
                    for (auto rhs = rootBegin; rhs < rootEnd; ++rhs) {
                        if (tokens[rhs].text == "(") ++rootDepth;
                        else if (tokens[rhs].text == ")" && rootDepth > 0) --rootDepth;
                        else if (rootDepth == 0 &&
                                 reconvergenceOperators.contains(
                                     lower(tokens[rhs].text))) {
                            assignment.reconvergenceRoot = true;
                            break;
                        }
                    }
                }
                assignments[assignment.target].push_back(std::move(assignment));
                index = end;
            }

            // Native GJB9765_VHDL_4_9_a slot 0 FUN_18069ed00 builds
            // categorized clock relations with FUN_1804d56f0, gathers
            // AST_ASSIGN_LE(0x56), propagates related assignment names with
            // FUN_18069fb50, and admits RHS roots selected by mask
            // 0xe0000019f or AST_WHEN(0xb6). The helper recursively counts
            // identifier leaves that resolve to one clock-path relation; a
            // count greater than one takes the reconvergence diagnostic path.
            // The mask does not contain VHDL shift/rotate roots. Each helper
            // invocation recursively examines its current expression tree;
            // the container selected at each call site determines whether a
            // leaf is an original relation or a one-hop derived name.
            // Register-level evidence at both helper call sites proves that
            // the first pass always queries the original relation set, adds
            // only the target name of a non-candidate assignment to a second
            // propagated-name set, and appends an assignment as a candidate
            // when its own target is already in the original relation set.
            // The final pass counts RHS identifier leaves only against that
            // second set. This is a one-hop relation construction, not a
            // transitive dependency expansion; a post-convergence alias is
            // consequently not itself a reconvergence candidate.
            std::set<std::size_t> reportedReconvergence;
            std::set<std::string> propagatedClockNames;
            for (const auto& [target, drivers] : assignments) {
                if (reconvergenceClocks.contains(target)) continue;
                for (const auto& assignment : drivers) {
                    if (std::ranges::any_of(
                            assignment.reconvergenceIdentifiers,
                            [&](const auto& name) {
                                return reconvergenceClocks.contains(name);
                            })) {
                        propagatedClockNames.insert(target);
                        break;
                    }
                }
            }
            for (const auto& [target, drivers] : assignments) {
                if (!reconvergenceClocks.contains(target)) continue;
                for (const auto& assignment : drivers) {
                    if (!assignment.reconvergenceRoot) continue;
                    const auto relatedLeaves = std::ranges::count_if(
                        assignment.reconvergenceIdentifiers,
                        [&](const auto& name) {
                            return propagatedClockNames.contains(name);
                        });
                    if (relatedLeaves <= 1 ||
                        !reportedReconvergence.insert(assignment.targetToken).second)
                        continue;
                    const auto& token = tokens[assignment.targetToken];
                    emit(result, "GJB9765.4.9.a", source,
                         token.line, token.column,
                         "时钟相关信号经不同路径在 " + token.text + " 处重新汇聚");
                }
            }

            // R-9-3 uses one name-keyed relation record and one name-keyed
            // gate-candidate record per target. FUN_1806841f0 overwrites the
            // stored AST pointer on a repeated name, so the last admitted
            // direct architecture assignment wins in each map. Traverse only
            // the recovered broad relation-root set; do not borrow the wider
            // dependency walk used by adjacent R-9-1/R-9-5/R-9-6 adapters.
            std::set<std::size_t> reportedGating;
            for (const auto& clock : clocks) {
                std::vector<std::string> pending{clock};
                std::set<std::string> visited;
                while (!pending.empty()) {
                    auto signal = std::move(pending.back());
                    pending.pop_back();
                    if (!visited.insert(signal).second) continue;
                    if (const auto candidate = nativeGateCandidates.find(signal);
                        candidate != nativeGateCandidates.end() &&
                        reportedGating.insert(candidate->second).second) {
                        const auto& token = tokens[candidate->second];
                        emit(result, "GJB9765.4.9.c", source,
                             token.line, token.column,
                             "时钟信号 " + token.text + " 由门控逻辑生成");
                    }
                    const auto relation = nativeGateRelations.find(signal);
                    if (relation == nativeGateRelations.end()) continue;
                    const auto drivers = assignments.find(signal);
                    if (drivers == assignments.end()) continue;
                    const auto driver = std::ranges::find_if(
                        drivers->second, [&](const auto& assignment) {
                            return assignment.targetToken == relation->second;
                        });
                    if (driver != drivers->second.end())
                        pending.insert(pending.end(), driver->dependencies.begin(),
                                       driver->dependencies.end());
                }
            }

            std::set<std::size_t> reportedCombinational;
            struct ClockPathViolation {
                unsigned kinds{}; // native return bits: inverter=1, buffer=2
                core::SourceLocation location;
            };
            std::map<std::string, ClockPathViolation> clockPathViolations;
            std::map<std::string, std::set<std::string>> reachableByClock;
            for (const auto& clock : clocks) {
                std::vector<std::string> pending{clock};
                std::set<std::string> visited;
                while (!pending.empty()) {
                    auto signal = std::move(pending.back());
                    pending.pop_back();
                    if (!visited.insert(signal).second) continue;
                    const auto drivers = assignments.find(signal);
                    if (drivers == assignments.end()) continue;
                    for (const auto& assignment : drivers->second) {
                        if (assignment.combinational &&
                            reportedCombinational.insert(assignment.targetToken).second) {
                            const auto& token = tokens[assignment.targetToken];
                            emit(result, "GJB9765.4.9.e", source, token.line, token.column,
                                 "时钟信号 " + token.text + " 由组合逻辑生成");
                        }
                        if (assignment.inverted) {
                            const auto& token = tokens[assignment.targetToken];
                            auto& violation = clockPathViolations[clock];
                            violation.kinds |= 1U;
                            if (violation.location.line == 0)
                                violation.location = {source.path(), token.line, token.column};
                        }
                        pending.insert(pending.end(), assignment.dependencies.begin(),
                                       assignment.dependencies.end());
                    }
                }
                reachableByClock.emplace(clock, std::move(visited));
            }

            // Native A-9-6 recursively resolves user component/entity port
            // bindings.  Build the bounded source equivalent across uniquely
            // resolved VHDL entities: only plain-identifier actuals and direct
            // concurrent identifier/not-identifier transfers participate.
            // Cycles, unresolved/external units, expressions and configuration
            // bindings contribute no proof and remain review-required.
            static const std::set<std::string_view> dedicatedClockPrimitives{
                "ibufg", "ibufgds", "bufg", "bufgp", "bufgce", "bufgmux",
                "bufgdll", "dcm", "inbuf"
            };
            struct HierarchyTransfer {
                std::string output;
                std::string input;
                unsigned kinds{};
            };
            std::vector<std::optional<std::vector<HierarchyTransfer>>> hierarchyCache(
                result.structure.size());
            const std::vector<HierarchyTransfer> emptyHierarchyTransfers;
            const auto hierarchyTransfersFor = [&](auto&& self, std::size_t unitIndex,
                                                    std::set<std::size_t>& active)
                -> const std::vector<HierarchyTransfer>& {
                if (hierarchyCache[unitIndex]) return *hierarchyCache[unitIndex];
                if (!active.insert(unitIndex).second) return emptyHierarchyTransfers;
                const auto& candidate = result.structure[unitIndex];
                struct Edge { std::string source; unsigned kinds{}; };
                std::map<std::string, std::vector<Edge>> edges;
                for (const auto& transfer : candidate.simpleSignalTransfers)
                    edges[lower(transfer.target)].push_back(
                        {lower(transfer.source), transfer.inverted ? 1U : 2U});

                for (const auto& nested : candidate.instances) {
                    if (!nested.resolved || nested.external ||
                        !nested.hasPortConnectionList ||
                        dedicatedClockPrimitives.contains(lower(nested.target))) continue;
                    auto childIndex = result.structure.size();
                    for (std::size_t index = 0; index < result.structure.size(); ++index) {
                        const auto& possible = result.structure[index];
                        if (possible.kind == parser::DesignUnitKind::VhdlEntity &&
                            lower(possible.name) == lower(nested.target)) {
                            childIndex = index;
                            break;
                        }
                    }
                    if (childIndex == result.structure.size() || active.contains(childIndex))
                        continue;
                    const auto& child = result.structure[childIndex];
                    const auto actualFor = [&](std::string_view formal)
                        -> std::optional<std::string> {
                        for (const auto& connection : nested.namedConnections) {
                            if (lower(connection.formal) != lower(std::string(formal)) ||
                                connection.actualIdentifier.empty()) continue;
                            return lower(connection.actualIdentifier);
                        }
                        const auto port = std::ranges::find_if(
                            child.ports, [&](const auto& item) {
                                return lower(item.name) == lower(std::string(formal));
                            });
                        if (port == child.ports.end()) return std::nullopt;
                        const auto ordinal = static_cast<std::size_t>(port - child.ports.begin());
                        if (ordinal >= nested.positionalConnections.size() ||
                            nested.positionalConnections[ordinal].actualIdentifier.empty())
                            return std::nullopt;
                        return lower(nested.positionalConnections[ordinal].actualIdentifier);
                    };
                    for (const auto& relation : self(self, childIndex, active)) {
                        const auto outputActual = actualFor(relation.output);
                        const auto inputActual = actualFor(relation.input);
                        if (outputActual && inputActual)
                            edges[*outputActual].push_back({*inputActual, relation.kinds});
                    }
                }

                std::map<std::pair<std::string, std::string>, unsigned> relations;
                for (const auto& outputPort : candidate.ports) {
                    if (outputPort.direction != parser::PortDirection::Output) continue;
                    for (const auto& inputPort : candidate.ports) {
                        if (inputPort.direction != parser::PortDirection::Input) continue;
                        const auto output = lower(outputPort.name);
                        const auto input = lower(inputPort.name);
                        std::vector<std::pair<std::string, unsigned>> pending{{output, 0U}};
                        std::set<std::pair<std::string, unsigned>> visited;
                        while (!pending.empty()) {
                            const auto [signal, kinds] = std::move(pending.back());
                            pending.pop_back();
                            if (!visited.insert({signal, kinds}).second) continue;
                            if (signal == input && kinds != 0)
                                relations[{output, input}] |= kinds;
                            const auto drivers = edges.find(signal);
                            if (drivers == edges.end()) continue;
                            for (const auto& edge : drivers->second)
                                pending.emplace_back(edge.source, kinds | edge.kinds);
                        }
                    }
                }
                std::vector<HierarchyTransfer> recovered;
                recovered.reserve(relations.size());
                for (const auto& [ports, kinds] : relations)
                    recovered.push_back({ports.first, ports.second, kinds});
                active.erase(unitIndex);
                hierarchyCache[unitIndex] = std::move(recovered);
                return *hierarchyCache[unitIndex];
            };
            for (const auto& instance : unit.instances) {
                if (!instance.resolved || instance.external ||
                    !instance.hasPortConnectionList) continue;
                if (dedicatedClockPrimitives.contains(lower(instance.target))) continue;
                auto childIndex = result.structure.size();
                for (std::size_t index = 0; index < result.structure.size(); ++index) {
                    const auto& candidate = result.structure[index];
                    if (candidate.kind == parser::DesignUnitKind::VhdlEntity &&
                        lower(candidate.name) == lower(instance.target)) {
                        childIndex = index;
                        break;
                    }
                }
                if (childIndex == result.structure.size()) continue;
                const auto& child = result.structure[childIndex];

                const auto actualFor = [&](std::string_view formal)
                    -> std::optional<std::string> {
                    for (const auto& connection : instance.namedConnections)
                        if (lower(connection.formal) == lower(std::string(formal)) &&
                            !connection.actualIdentifier.empty())
                            return lower(connection.actualIdentifier);
                    const auto port = std::ranges::find_if(
                        child.ports, [&](const auto& item) {
                            return lower(item.name) == lower(std::string(formal));
                        });
                    if (port == child.ports.end()) return std::nullopt;
                    const auto ordinal = static_cast<std::size_t>(port - child.ports.begin());
                    if (ordinal < instance.positionalConnections.size() &&
                        !instance.positionalConnections[ordinal].actualIdentifier.empty())
                        return lower(instance.positionalConnections[ordinal].actualIdentifier);
                    return std::nullopt;
                };
                std::set<std::size_t> active;
                for (const auto& transfer : hierarchyTransfersFor(
                         hierarchyTransfersFor, childIndex, active)) {
                    const auto outputActual = actualFor(transfer.output);
                    const auto inputActual = actualFor(transfer.input);
                    if (!outputActual || !inputActual) continue;
                    for (const auto& [clock, reachable] : reachableByClock) {
                        if (!reachable.contains(*outputActual)) continue;
                        auto& violation = clockPathViolations[clock];
                        violation.kinds |= transfer.kinds;
                        if (violation.location.line == 0) violation.location = instance.location;
                    }
                }
            }

            // FUN_18060d980 ORs the direct-inversion result and every component
            // result for one clock relation, then emits exactly once.  Values
            // 1, 2 and 3 select the inverter, buffer and combined messages.
            for (const auto& [clock, violation] : clockPathViolations) {
                if (violation.kinds == 0) continue;
                const auto inserted = violation.kinds == 1 ? "反相器" :
                    violation.kinds == 2 ? "缓冲器" : "反相器和缓冲器";
                emit(result, "GJB9765.4.9.j", source,
                     violation.location.line, violation.location.column,
                     "时钟信号 " + clock + " 的路径中插入了" + inserted);
            }
        }
    }
}

void checkVhdlRippleClocks(AnalysisResult& result, const core::SourceFile& source,
                           const std::vector<parser::HdlToken>& tokens) {
    // Native GJB9765_VHDL_4_9_g slot 0 (FUN_180684a40) gathers process nodes
    // with FUN_1804c5890, builds their categorized clock/control relations
    // with FUN_1804d56f0, and retains the related clocked-process AST nodes.
    // It recursively collects AST_ASSIGN_LE (0x56) below those processes,
    // extracts the first child (the normalized assignment target plus source
    // location), and reports when that target name is also present in the
    // design-root clock-relation name vector. The RHS expression is not part
    // of the intersection predicate. Embedded text at 0x180f0bb68,
    // 0x180f0bb58 and 0x180f0bb40 reads "不能将触发器的输出" + name +
    // "作为时钟" / "避免使用内部生成时钟".
    // ev_ecc901e60709075f02956520f212ea3fc7b75bcf0603b10b9ebb0ee195c4f949
    struct ArchitectureRange {
        std::string entity;
        std::size_t begin{};
        std::size_t end{};
    };
    std::vector<ArchitectureRange> architectures;
    for (std::size_t token = 0; token + 3 < tokens.size(); ++token) {
        if (lower(tokens[token].text) == "architecture" &&
            lower(tokens[token + 2].text) == "of" &&
            (token == 0 || lower(tokens[token - 1].text) != "end"))
            architectures.push_back(
                {lower(tokens[token + 3].text), token, tokens.size()});
    }
    for (std::size_t index = 0; index + 1 < architectures.size(); ++index)
        architectures[index].end = architectures[index + 1].begin;

    for (const auto& unit : result.structure) {
        if (unit.kind != parser::DesignUnitKind::VhdlEntity ||
            unit.location.file.lexically_normal() != source.path().lexically_normal()) continue;
        struct ProcessRange {
            std::size_t begin{};
            std::size_t end{};
            std::size_t firstOffset{};
            std::size_t lastOffset{};
        };
        std::vector<ProcessRange> directSequentialProcesses;
        for (const auto& architecture : architectures) {
            if (architecture.entity != lower(unit.name)) continue;
            // FUN_1804c5890 is nonrecursive: only direct process children of
            // this architecture enter both the relation and assignment paths.
            // ev_fdfe77d6d9ec134b166d321c11ef651b5b7fbc15bba2280f725f8afe72d05f91
            const auto direct = vhdlDirectArchitectureTokenMask(
                tokens, architecture.begin, architecture.end);
            for (auto process = architecture.begin; process < architecture.end;
                 ++process) {
                if (lower(tokens[process].text) != "process" || !direct[process])
                    continue;
                auto end = process + 1;
                while (end + 1 < architecture.end &&
                       !(lower(tokens[end].text) == "end" &&
                         lower(tokens[end + 1].text) == "process")) ++end;
                if (end + 1 >= architecture.end) continue;
                end += 2;
                const auto firstOffset = tokens[process].offset;
                const auto lastOffset =
                    tokens[end - 1].offset + tokens[end - 1].text.size();
                const bool sequential = std::ranges::any_of(
                    unit.sequentialProcesses, [&](const auto& candidate) {
                        return candidate.location.file.lexically_normal() ==
                                   source.path().lexically_normal() &&
                               candidate.location.byteOffset >= firstOffset &&
                               candidate.location.byteOffset < lastOffset;
                    });
                if (sequential)
                    directSequentialProcesses.push_back(
                        {process, end, firstOffset, lastOffset});
                process = end - 1;
            }
        }

        std::set<std::string> clocks;
        for (const auto& clock : unit.clocks) {
            if (clock.location.file.lexically_normal() != source.path().lexically_normal())
                continue;
            const bool fromDirectProcess = std::ranges::any_of(
                directSequentialProcesses, [&](const auto& process) {
                    return clock.location.byteOffset >= process.firstOffset &&
                           clock.location.byteOffset < process.lastOffset;
                });
            if (fromDirectProcess) clocks.insert(lower(clock.signal));
        }
        if (clocks.empty()) continue;

        std::set<std::size_t> reported;
        for (const auto& process : directSequentialProcesses) {
            for (auto index = process.begin + 1; index + 1 < process.end; ++index) {
                if (tokens[index].kind != parser::HdlTokenKind::Identifier ||
                    tokens[index + 1].text != "<=") continue;
                const auto previous = index == process.begin + 1
                    ? std::string{} : lower(tokens[index - 1].text);
                if (index != process.begin + 1 && tokens[index - 1].text != ";" &&
                    tokens[index - 1].text != ":" && previous != "begin" &&
                    previous != "then" && previous != "else" && previous != "loop" &&
                    previous != "select") continue;
                const auto name = lower(tokens[index].text);
                if (!clocks.contains(name) || !reported.insert(tokens[index].offset).second) continue;
                emit(result, "GJB9765.4.9.g", source,
                     tokens[index].line, tokens[index].column,
                     "触发器输出 " + tokens[index].text + " 被用作后级时钟");
            }
        }
    }
}

void checkVhdlMultipleClockDomains(AnalysisResult& result,
                                   const core::SourceFile& source,
                                   const AnalysisOptions& options) {
    // Native GJB9765_VHDL_4_9_h constructor FUN_18002a0e0 installs vtable
    // 0x180f03478; slot 0 FUN_18060b400 asks FUN_18060c5c0 to collect
    // clock-relation names from VHDL process nodes. Unlike neighboring rules,
    // FUN_18060c5c0 constructs an AST_ALWAYS/process(0x52) type filter and
    // calls recursive FUN_1804f3870 on the design root, so processes below
    // generate/block nodes remain in the direct clock-name count. It sorts and
    // deduplicates the resulting 32-byte string records, and emits once at the
    // design-root location when the unique count is greater than one. Edge
    // polarity is not part of the record. The later relation-graph path runs only after
    // the direct unique-name count is at most one. This bounded adapter also
    // canonicalizes explicit concurrent direct and unary-NOT aliases, while
    // deliberately excluding assignments inside processes: a registered
    // clock remains a distinct domain. Embedded GB18030 text at 0x180f034c0
    // is "尽量减少时钟域,该项目有多个时钟域".
    // ev_2f01c5fe7f98824b9ff179cadf62dd1c3ecd82df3ab44b661018520cbef29c6a
    const auto tokens = standardTokens(source);
    for (const auto& unit : result.structure) {
        if (unit.kind != parser::DesignUnitKind::VhdlEntity ||
            unit.location.file.lexically_normal() != source.path().lexically_normal()) continue;
        // FUN_18060b400 admits a design record only after its name equals the
        // configured global top name. VHDL names remain case-insensitive in
        // the source adapter. Without an explicit top, keep the pre-existing
        // bounded per-entity behavior and its ProjectContext result state.
        if (options.topModule && !selectedTop(unit, options)) continue;

        std::map<std::string, std::string> aliases;
        std::size_t architecture = tokens.size();
        for (std::size_t i = 0; i + 3 < tokens.size(); ++i) {
            if (lower(tokens[i].text) == "architecture" &&
                lower(tokens[i + 2].text) == "of" &&
                lower(tokens[i + 3].text) == lower(unit.name)) {
                architecture = i;
                break;
            }
        }
        if (architecture != tokens.size()) {
            auto end = architecture + 1;
            while (end < tokens.size() &&
                   lower(tokens[end].text) != "architecture") ++end;
            int processDepth = 0;
            for (auto i = architecture + 1; i + 3 < end; ++i) {
                const auto word = lower(tokens[i].text);
                if (word == "process") {
                    if (i > architecture + 1 && lower(tokens[i - 1].text) == "end") {
                        if (processDepth > 0) --processDepth;
                    } else {
                        ++processDepth;
                    }
                    continue;
                }
                if (processDepth != 0 ||
                    tokens[i].kind != parser::HdlTokenKind::Identifier ||
                    tokens[i + 1].text != "<=") continue;
                auto rhs = i + 2;
                if (lower(tokens[rhs].text) == "not") ++rhs;
                if (rhs + 1 >= end ||
                    tokens[rhs].kind != parser::HdlTokenKind::Identifier ||
                    tokens[rhs + 1].text != ";") continue;
                aliases[lower(tokens[i].text)] = lower(tokens[rhs].text);
                i = rhs;
            }
        }
        const auto canonical = [&](std::string signal) {
            signal = lower(std::move(signal));
            std::set<std::string> visited;
            while (visited.insert(signal).second) {
                const auto alias = aliases.find(signal);
                if (alias == aliases.end()) break;
                signal = alias->second;
            }
            return signal;
        };
        std::set<std::string> domains;
        for (const auto& clock : unit.clocks) {
            if (clock.location.file.lexically_normal() != source.path().lexically_normal()) continue;
            domains.insert(canonical(clock.signal));
        }
        if (domains.size() <= 1) continue;
        emit(result, "GJB9765.4.9.h", source,
             unit.location.line, unit.location.column,
             "设计单元 " + unit.name + " 使用多个时钟域");
    }
}

std::set<std::string> vhdlNativeStaticActionNames(
    const std::vector<parser::HdlToken>& tokens,
    std::size_t architecture, std::size_t limit,
    const parser::DesignUnitInfo& unit) {
    std::set<std::string> names;
    for (const auto& parameter : unit.parameters)
        names.insert(lower(parameter.name));

    auto headerIs = architecture + 1;
    while (headerIs < limit && lower(tokens[headerIs].text) != "is") ++headerIs;
    if (headerIs >= limit) return names;

    // FUN_1804cbad0 consumes only declarations directly owned by the
    // architecture. Skip local packages and subprogram bodies before finding
    // the architecture statement part, so their constants cannot leak into
    // the per-root name/value tree.
    auto bodyBegin = limit;
    for (auto cursor = headerIs + 1; cursor < limit; ++cursor) {
        const auto word = lower(tokens[cursor].text);
        if (word == "package" &&
            (cursor == 0 || lower(tokens[cursor - 1].text) != "end")) {
            auto nameToken = cursor + 1;
            if (nameToken < limit && lower(tokens[nameToken].text) == "body")
                ++nameToken;
            const auto packageName = nameToken < limit
                ? lower(tokens[nameToken].text) : std::string{};
            auto ending = nameToken + 1;
            for (; ending + 1 < limit; ++ending) {
                if (lower(tokens[ending].text) != "end") continue;
                const auto next = lower(tokens[ending + 1].text);
                if (next == "package" || next == packageName ||
                    tokens[ending + 1].text == ";") break;
            }
            cursor = std::min(ending + 1, limit - 1);
            continue;
        }
        if ((word == "function" || word == "procedure") &&
            (cursor == 0 || lower(tokens[cursor - 1].text) != "end")) {
            auto marker = cursor + 1;
            int parentheses = 0;
            for (; marker < limit; ++marker) {
                if (tokens[marker].text == "(") ++parentheses;
                else if (tokens[marker].text == ")" && parentheses > 0)
                    --parentheses;
                else if (parentheses == 0 &&
                         (lower(tokens[marker].text) == "is" ||
                          tokens[marker].text == ";")) break;
            }
            if (marker < limit && lower(tokens[marker].text) == "is") {
                auto ending = marker + 1;
                for (; ending + 1 < limit; ++ending) {
                    if (lower(tokens[ending].text) != "end") continue;
                    const auto next = lower(tokens[ending + 1].text);
                    if (next == word || tokens[ending + 1].text == ";") break;
                }
                cursor = std::min(ending + 1, limit - 1);
            } else if (marker < limit) {
                cursor = marker;
            }
            continue;
        }
        if (word == "begin") {
            bodyBegin = cursor;
            break;
        }
    }
    if (bodyBegin == limit) return names;

    for (auto declaration = headerIs + 1; declaration < bodyBegin; ++declaration) {
        const auto word = lower(tokens[declaration].text);
        if (word == "package" &&
            (declaration == 0 || lower(tokens[declaration - 1].text) != "end")) {
            auto nameToken = declaration + 1;
            if (nameToken < bodyBegin && lower(tokens[nameToken].text) == "body")
                ++nameToken;
            const auto packageName = nameToken < bodyBegin
                ? lower(tokens[nameToken].text) : std::string{};
            auto ending = nameToken + 1;
            for (; ending + 1 < bodyBegin; ++ending) {
                if (lower(tokens[ending].text) != "end") continue;
                const auto next = lower(tokens[ending + 1].text);
                if (next == "package" || next == packageName ||
                    tokens[ending + 1].text == ";") break;
            }
            declaration = std::min(ending + 1, bodyBegin - 1);
            continue;
        }
        if ((word == "function" || word == "procedure") &&
            (declaration == 0 || lower(tokens[declaration - 1].text) != "end")) {
            auto marker = declaration + 1;
            int parentheses = 0;
            for (; marker < bodyBegin; ++marker) {
                if (tokens[marker].text == "(") ++parentheses;
                else if (tokens[marker].text == ")" && parentheses > 0)
                    --parentheses;
                else if (parentheses == 0 &&
                         (lower(tokens[marker].text) == "is" ||
                          tokens[marker].text == ";")) break;
            }
            if (marker < bodyBegin && lower(tokens[marker].text) == "is") {
                auto ending = marker + 1;
                for (; ending + 1 < bodyBegin; ++ending) {
                    if (lower(tokens[ending].text) != "end") continue;
                    const auto next = lower(tokens[ending + 1].text);
                    if (next == word || tokens[ending + 1].text == ";") break;
                }
                declaration = std::min(ending + 1, bodyBegin - 1);
            } else if (marker < bodyBegin) {
                declaration = marker;
            }
            continue;
        }
        if (word == "constant") {
            auto colon = declaration + 1;
            while (colon < bodyBegin && tokens[colon].text != ":" &&
                   tokens[colon].text != ";") ++colon;
            if (colon < bodyBegin && tokens[colon].text == ":")
                for (auto name = declaration + 1; name < colon; ++name)
                    if (tokens[name].kind == parser::HdlTokenKind::Identifier)
                        names.insert(lower(tokens[name].text));
            while (declaration < bodyBegin && tokens[declaration].text != ";")
                ++declaration;
            continue;
        }
        if (word == "type") {
            auto isToken = declaration + 1;
            auto semicolon = isToken;
            while (semicolon < bodyBegin && tokens[semicolon].text != ";")
                ++semicolon;
            while (isToken < semicolon && lower(tokens[isToken].text) != "is")
                ++isToken;
            if (isToken + 1 < semicolon && tokens[isToken + 1].text == "(") {
                const auto close = closeParen(tokens, isToken + 1);
                if (close < semicolon)
                    for (auto literal = isToken + 2; literal < close; ++literal)
                        if (tokens[literal].kind == parser::HdlTokenKind::Identifier)
                            names.insert(lower(tokens[literal].text));
            }
            declaration = semicolon;
        }
    }
    return names;
}

bool vhdlNativeStaticLiteralAtom(const std::vector<parser::HdlToken>& tokens,
                                 std::size_t begin, std::size_t end) {
    if (begin >= end) return false;
    if (end == begin + 1) {
        const auto& item = tokens[begin];
        const auto& value = item.text;
        return item.kind == parser::HdlTokenKind::Number ||
            item.kind == parser::HdlTokenKind::String ||
            (value.size() >= 2 &&
             ((value.front() == '\'' && value.back() == '\'') ||
              (value.front() == '"' && value.back() == '"')));
    }
    const bool character = end == begin + 3 && tokens[begin].text == "'" &&
        tokens[begin + 2].text == "'" &&
        tokens[begin + 1].offset == tokens[begin].offset + 1 &&
        tokens[begin + 2].offset == tokens[begin + 1].offset +
            tokens[begin + 1].text.size();
    const bool joinedCharacter = end == begin + 2 && tokens[begin].text == "'" &&
        tokens[begin + 1].kind == parser::HdlTokenKind::Number &&
        tokens[begin + 1].text.size() == 2 &&
        tokens[begin + 1].text.back() == '\'' &&
        tokens[begin + 1].offset == tokens[begin].offset + 1;
    return character || joinedCharacter;
}

bool vhdlNativeStaticRhs(const std::vector<parser::HdlToken>& tokens,
                         std::size_t begin, std::size_t end,
                         const std::set<std::string>& staticNames) {
    while (begin < end && tokens[begin].text == "(" &&
           closeParen(tokens, begin) + 1 == end) {
        ++begin;
        --end;
    }
    if (vhdlNativeStaticLiteralAtom(tokens, begin, end)) return true;
    if (end == begin + 1 &&
        tokens[begin].kind == parser::HdlTokenKind::Identifier &&
        staticNames.contains(lower(tokens[begin].text))) return true;
    if (begin + 2 < end && lower(tokens[begin].text) == "others" &&
        tokens[begin + 1].text == "=>") return true;

    std::vector<std::pair<std::size_t, std::size_t>> atoms;
    auto atom = begin;
    int depth = 0;
    for (auto cursor = begin; cursor <= end; ++cursor) {
        if (cursor < end && tokens[cursor].text == "(") ++depth;
        else if (cursor < end && tokens[cursor].text == ")" && depth > 0) --depth;
        const bool separator = cursor == end ||
            (depth == 0 && (tokens[cursor].text == "&" ||
                            tokens[cursor].text == ","));
        if (!separator) continue;
        atoms.push_back({atom, cursor});
        atom = cursor + 1;
    }
    return atoms.size() > 1 && std::ranges::all_of(atoms, [&](const auto& item) {
        return vhdlNativeStaticLiteralAtom(tokens, item.first, item.second);
    });
}

bool hasVhdlNativeStaticAssignment(
    const std::vector<parser::HdlToken>& tokens,
    std::size_t begin, std::size_t limit,
    const std::set<std::string>& staticNames) {
    for (auto token = begin; token + 2 < limit; ++token) {
        std::size_t rhs = token + 2;
        if (tokens[token + 1].text == "<=") {
            // already positioned
        } else if (token + 3 < limit && tokens[token + 1].text == "<" &&
                   tokens[token + 2].text == "=" &&
                   tokens[token + 2].offset == tokens[token + 1].offset + 1) {
            rhs = token + 3;
        } else {
            continue;
        }
        auto finish = rhs;
        while (finish < limit && tokens[finish].text != ";") ++finish;
        if (finish < limit && vhdlNativeStaticRhs(tokens, rhs, finish, staticNames))
            return true;
    }
    return false;
}

#include "analysis/vhdl_native_clock_relations.inc"

void checkVhdlClockAsControl(AnalysisResult& result, const core::SourceFile& source,
                             const std::vector<parser::HdlToken>& tokens) {
    // Native GJB9765_VHDL_4_9_b slot 0 FUN_180699180 builds clock-relation
    // names with FUN_1804d56f0, collects conditional control candidates from
    // AST_CASE/AST_COND (0x57/0x58) through FUN_180698e40 and intersects
    // candidate names against the clock-relation set before reporting. The
    // embedded text at 0x180f0c6c0/0x180f0cb60 forms "禁止时钟" + name +
    // "作为控制信号". Focused recovery of FUN_1804c48c0 proves that candidate
    // extraction admits identifiers, unary NOT identifiers, scalar equality
    // to '0'/'1', and recursively nested AND/OR expressions. Its helper
    // FUN_1804d4cc0 recognizes the event-plus-level edge idiom and prevents
    // that clock expression from becoming a control candidate. Calls and
    // unsupported operators remain outside this source-bound adapter. The
    // shared FUN_1804d56f0 relation builder obtains its edge through
    // FUN_1804c4430, whose recovered collector walks matching edge/AND nodes
    // in AST preorder. Relation classes 1/2 therefore use the same bounded
    // first-edge traversal as R-9-4 instead of requiring the entire source
    // condition to be one direct edge idiom.
    struct Range { std::string entity; std::size_t begin; std::size_t end; };
    std::vector<Range> ranges;
    for (std::size_t i = 0; i + 3 < tokens.size(); ++i) {
        if (lower(tokens[i].text) == "architecture" &&
            lower(tokens[i + 2].text) == "of")
            ranges.push_back({lower(tokens[i + 3].text), i, tokens.size()});
    }
    for (std::size_t i = 0; i + 1 < ranges.size(); ++i)
        ranges[i].end = ranges[i + 1].begin;

    for (const auto& unit : result.structure) {
        if (unit.kind != parser::DesignUnitKind::VhdlEntity ||
            unit.location.file.lexically_normal() != source.path().lexically_normal()) continue;
        std::set<std::string> clocks;
        for (const auto& clock : unit.clocks) {
            if (clock.location.file.lexically_normal() == source.path().lexically_normal())
                clocks.insert(lower(clock.signal));
        }
        if (clocks.empty()) continue;

        for (const auto& range : ranges) {
            if (range.entity != lower(unit.name)) continue;

            // FUN_180699180 consumes only relation-tree keys 1 and 2. It
            // deliberately skips key 3 (no recognized edge) and key 4 (the
            // asynchronous-control shape for which FUN_1804d24a0 did not
            // prove a constant action). Reconstruct the two admitted source
            // shapes before intersecting condition candidates with clocks.
            std::set<std::string> admittedClocks;
            std::set<std::size_t> relationLines;
            // FUN_180699180 obtains its process vector from FUN_1804c5890.
            // That collector admits only direct architecture process children;
            // processes nested below generate/block/subprogram nodes neither
            // establish clock relations nor contribute IF/CASE candidates.
            // ev_fdfe77d6d9ec134b166d321c11ef651b5b7fbc15bba2280f725f8afe72d05f91
            const auto directArchitectureToken = vhdlDirectArchitectureTokenMask(
                tokens, range.begin, range.end);
            const auto staticActionNames = vhdlNativeStaticActionNames(
                tokens, range.begin, range.end, unit);
            for (auto process = range.begin; process < range.end; ++process) {
                if (lower(tokens[process].text) != "process" ||
                    !directArchitectureToken[process] ||
                    (process > range.begin && lower(tokens[process - 1].text) == "end"))
                    continue;
                auto end = process + 1;
                while (end + 1 < range.end &&
                       !(lower(tokens[end].text) == "end" &&
                         lower(tokens[end + 1].text) == "process")) ++end;
                if (end + 1 >= range.end) continue;
                auto body = process + 1;
                int parentheses = 0;
                for (; body < end; ++body) {
                    if (tokens[body].text == "(") ++parentheses;
                    else if (tokens[body].text == ")" && parentheses > 0) --parentheses;
                    if (parentheses == 0 && lower(tokens[body].text) == "begin") {
                        ++body;
                        break;
                    }
                }
                if (body >= end) {
                    process = end + 1;
                    continue;
                }

                // FUN_1804c5070 admits only source-linked IF nodes at the
                // process statement-block depth. Enumerate every direct IF in
                // source order while excluding branch-, CASE- and LOOP-nested
                // IFs as independent relation roots.
                struct SourceIf { std::size_t begin{}, end{}; };
                std::vector<SourceIf> sourceIfs;
                std::optional<std::size_t> topLevelIf;
                unsigned ifDepth = 0;
                unsigned caseDepth = 0;
                unsigned loopDepth = 0;
                for (auto token = body; token + 1 < end; ++token) {
                    const auto word = lower(tokens[token].text);
                    if (word == "end") {
                        const auto closing = lower(tokens[token + 1].text);
                        if (closing == "if" && ifDepth != 0) {
                            --ifDepth;
                            if (ifDepth == 0 && topLevelIf) {
                                sourceIfs.push_back({*topLevelIf, token});
                                topLevelIf.reset();
                            }
                        } else if (closing == "case" && caseDepth != 0) {
                            --caseDepth;
                        } else if (closing == "loop" && loopDepth != 0) {
                            --loopDepth;
                        }
                        if (closing == "if" || closing == "case" || closing == "loop")
                            ++token;
                        continue;
                    }
                    if (word == "if") {
                        if (ifDepth == 0 && caseDepth == 0 && loopDepth == 0)
                            topLevelIf = token;
                        ++ifDepth;
                    } else if (word == "case") {
                        ++caseDepth;
                    } else if (word == "loop") {
                        ++loopDepth;
                    }
                }

                for (const auto& sourceIf : sourceIfs) {
                    auto firstThen = sourceIf.begin + 1;
                    int conditionDepth = 0;
                    for (; firstThen < sourceIf.end; ++firstThen) {
                        if (tokens[firstThen].text == "(") ++conditionDepth;
                        else if (tokens[firstThen].text == ")" && conditionDepth > 0)
                            --conditionDepth;
                        if (conditionDepth == 0 &&
                            lower(tokens[firstThen].text) == "then") break;
                    }
                    if (firstThen >= sourceIf.end) continue;

                    const auto mainEdge = findFirstVhdlEdgeCondition(
                        source, tokens, sourceIf.begin + 1, firstThen);
                    if (mainEdge.status == VhdlEdgeConditionStatus::Matched) {
                        if (clocks.contains(mainEdge.clockName)) {
                            admittedClocks.insert(mainEdge.clockName); // relation key 2
                            relationLines.insert(tokens[sourceIf.begin].line);
                            if (firstThen + 1 < sourceIf.end &&
                                lower(tokens[firstThen + 1].text) == "if")
                                relationLines.insert(tokens[firstThen + 1].line);
                        }
                        continue;
                    }

                    // FUN_1804d24a0 processes every direct ELSIF in order.
                    // A non-edge static branch may establish category 1 until
                    // a second edge record exists; later static branches do
                    // not promote category 4. Only the first edge is the
                    // relation object's clock name consumed by this rule;
                    // additional records contribute source-line exclusions.
                    std::vector<std::size_t> elsifTokens;
                    std::size_t elseToken = sourceIf.end;
                    unsigned nestedIf = 0;
                    for (auto token = firstThen + 1; token < sourceIf.end; ++token) {
                        const auto word = lower(tokens[token].text);
                        if (word == "end" && token + 1 < sourceIf.end &&
                            lower(tokens[token + 1].text) == "if") {
                            if (nestedIf != 0) --nestedIf;
                            ++token;
                            continue;
                        }
                        if (word == "if") {
                            ++nestedIf;
                        } else if (nestedIf == 0 && word == "elsif") {
                            elsifTokens.push_back(token);
                        } else if (nestedIf == 0 && word == "else") {
                            elseToken = token;
                            break;
                        }
                    }
                    const auto firstBranchEnd = !elsifTokens.empty()
                        ? elsifTokens.front()
                        : elseToken < sourceIf.end ? elseToken : sourceIf.end;
                    bool staticAction = hasVhdlNativeStaticAssignment(
                        tokens, firstThen + 1, firstBranchEnd, staticActionNames);
                    std::optional<VhdlEdgeCondition> relationEdge;
                    std::size_t extraEdges = 0;
                    for (std::size_t branch = 0; branch < elsifTokens.size(); ++branch) {
                        const auto elsifToken = elsifTokens[branch];
                        const auto branchEnd = branch + 1 < elsifTokens.size()
                            ? elsifTokens[branch + 1]
                            : elseToken < sourceIf.end ? elseToken : sourceIf.end;
                        auto thenToken = elsifToken + 1;
                        conditionDepth = 0;
                        for (; thenToken < branchEnd; ++thenToken) {
                            if (tokens[thenToken].text == "(") ++conditionDepth;
                            else if (tokens[thenToken].text == ")" && conditionDepth > 0)
                                --conditionDepth;
                            if (conditionDepth == 0 &&
                                lower(tokens[thenToken].text) == "then") break;
                        }
                        if (thenToken >= branchEnd) continue;
                        const auto edge = findFirstVhdlEdgeCondition(
                            source, tokens, elsifToken + 1, thenToken);
                        if (edge.status == VhdlEdgeConditionStatus::Matched) {
                            if (relationEdge) ++extraEdges;
                            else relationEdge = edge;
                        } else if (extraEdges == 0) {
                            staticAction = staticAction || hasVhdlNativeStaticAssignment(
                                tokens, thenToken + 1, branchEnd, staticActionNames);
                        }
                    }

                    // The native ELSE loop overwrites its source-IF pointer;
                    // only the last direct linked IF and only its main
                    // condition participate. CASE/LOOP-nested IFs are not
                    // direct links at this level.
                    std::optional<SourceIf> linkedElseIf;
                    if (elseToken < sourceIf.end) {
                        std::optional<std::size_t> directIf;
                        unsigned elseIfDepth = 0;
                        unsigned elseCaseDepth = 0;
                        unsigned elseLoopDepth = 0;
                        for (auto token = elseToken + 1; token + 1 < sourceIf.end;
                             ++token) {
                            const auto word = lower(tokens[token].text);
                            if (word == "end") {
                                const auto closing = lower(tokens[token + 1].text);
                                if (closing == "if" && elseIfDepth != 0) {
                                    --elseIfDepth;
                                    if (elseIfDepth == 0 && directIf) {
                                        linkedElseIf = SourceIf{*directIf, token};
                                        directIf.reset();
                                    }
                                } else if (closing == "case" && elseCaseDepth != 0) {
                                    --elseCaseDepth;
                                } else if (closing == "loop" && elseLoopDepth != 0) {
                                    --elseLoopDepth;
                                }
                                if (closing == "if" || closing == "case" ||
                                    closing == "loop") ++token;
                                continue;
                            }
                            if (word == "if") {
                                if (elseIfDepth == 0 && elseCaseDepth == 0 &&
                                    elseLoopDepth == 0) directIf = token;
                                ++elseIfDepth;
                            } else if (word == "case") {
                                ++elseCaseDepth;
                            } else if (word == "loop") {
                                ++elseLoopDepth;
                            }
                        }
                    }
                    if (linkedElseIf) {
                        auto linkedThen = linkedElseIf->begin + 1;
                        conditionDepth = 0;
                        for (; linkedThen < linkedElseIf->end; ++linkedThen) {
                            if (tokens[linkedThen].text == "(") ++conditionDepth;
                            else if (tokens[linkedThen].text == ")" && conditionDepth > 0)
                                --conditionDepth;
                            if (conditionDepth == 0 &&
                                lower(tokens[linkedThen].text) == "then") break;
                        }
                        if (linkedThen < linkedElseIf->end) {
                            const auto edge = findFirstVhdlEdgeCondition(
                                source, tokens, linkedElseIf->begin + 1, linkedThen);
                            if (edge.status == VhdlEdgeConditionStatus::Matched) {
                                if (relationEdge) ++extraEdges;
                                else relationEdge = edge;
                            } else if (extraEdges == 0) {
                                staticAction = staticAction || hasVhdlNativeStaticAssignment(
                                    tokens, linkedElseIf->begin, linkedElseIf->end,
                                    staticActionNames);
                            }
                        }
                    }

                    if (!staticAction || !relationEdge ||
                        !clocks.contains(relationEdge->clockName)) continue;
                    admittedClocks.insert(relationEdge->clockName); // relation key 1
                    relationLines.insert(tokens[sourceIf.begin].line);
                    for (const auto elsifToken : elsifTokens)
                        relationLines.insert(tokens[elsifToken].line);
                    if (linkedElseIf)
                        relationLines.insert(tokens[linkedElseIf->begin].line);
                }
                process = end + 1;
            }
            clocks = std::move(admittedClocks);
            if (clocks.empty()) continue;

            for (auto process = range.begin; process < range.end; ++process) {
                if (lower(tokens[process].text) != "process" ||
                    !directArchitectureToken[process]) continue;
                auto end = process + 1;
                while (end + 1 < range.end &&
                       !(lower(tokens[end].text) == "end" &&
                         lower(tokens[end + 1].text) == "process")) ++end;
                if (end + 1 >= range.end) continue;
                for (auto i = process + 1; i + 2 < end; ++i) {
                    const auto keyword = lower(tokens[i].text);
                    const bool conditional = keyword == "if" || keyword == "elsif";
                    const bool selector = keyword == "case" &&
                        !(i > process && lower(tokens[i - 1].text) == "end");
                    if (!conditional && !selector) continue;
                    if (relationLines.contains(tokens[i].line)) continue;

                    std::size_t expressionEnd = i + 1;
                    int depth = 0;
                    for (; expressionEnd < end; ++expressionEnd) {
                        const auto text = lower(tokens[expressionEnd].text);
                        if (text == "(") ++depth;
                        else if (text == ")" && depth > 0) --depth;
                        if (depth == 0 &&
                            ((conditional && text == "then") ||
                             (selector && text == "is"))) break;
                    }
                    if (expressionEnd >= end || expressionEnd == i + 1) continue;

                    bool unsupported = false;
                    bool hasEvent = false;
                    bool hasLogicalJoin = false;
                    bool hasEquality = false;
                    std::map<std::string, std::size_t> occurrences;
                    for (auto j = i + 1; j < expressionEnd; ++j) {
                        const auto text = lower(tokens[j].text);
                        hasEvent = hasEvent || text == "event";
                        hasLogicalJoin = hasLogicalJoin || text == "and" || text == "or";
                        hasEquality = hasEquality || text == "=";
                        if (text == "xor" || text == "xnor" || text == "nand" ||
                            text == "nor" || text == "/=" || text == "<" ||
                            text == ">" || text == "<=" || text == ">=" ||
                            text == "+" || text == "-" || text == "*" ||
                            text == "/" || text == "mod" || text == "rem")
                            unsupported = true;
                        if (tokens[j].kind == parser::HdlTokenKind::Identifier)
                            ++occurrences[text];
                    }
                    if (unsupported) continue;

                    // FUN_1804d4cc0 recognizes <name>'event AND/OR
                    // <same-name>='0'/'1' and suppresses recursive candidate
                    // collection for that edge expression.
                    bool nativeEdgeIdiom = false;
                    if (hasEvent && hasLogicalJoin && hasEquality) {
                        for (const auto& clock : clocks)
                            nativeEdgeIdiom = nativeEdgeIdiom || occurrences[clock] >= 2;
                    }
                    if (nativeEdgeIdiom) continue;

                    std::vector<bool> callStack;
                    int callDepth = 0;
                    // FUN_180698e40 uses a name-keyed map only for this one
                    // source condition. FUN_18069a710 appends every resulting
                    // 0x28-byte candidate record without comparing its name,
                    // AST pointer or line, so do not de-duplicate across later
                    // IF/CASE conditions or AST_WHEN children.
                    std::set<std::string> emittedNames;
                    for (auto j = i + 1; j < expressionEnd; ++j) {
                        const auto text = lower(tokens[j].text);
                        if (text == "(") {
                            const bool call = j > i + 1 &&
                                tokens[j - 1].kind == parser::HdlTokenKind::Identifier &&
                                lower(tokens[j - 1].text) != "not";
                            callStack.push_back(call);
                            if (call) ++callDepth;
                            continue;
                        }
                        if (text == ")") {
                            if (!callStack.empty()) {
                                if (callStack.back()) --callDepth;
                                callStack.pop_back();
                            }
                            continue;
                        }
                        if (callDepth != 0 ||
                            tokens[j].kind != parser::HdlTokenKind::Identifier ||
                            !clocks.contains(text)) continue;
                        if (j + 1 < expressionEnd && lower(tokens[j + 1].text) == "(")
                            continue;
                        if (!emittedNames.insert(text).second) continue;
                        const auto& control = tokens[j];
                        emit(result, "GJB9765.4.9.b", source,
                             control.line, control.column,
                             "时钟信号 " + control.text + " 被用作条件控制信号");
                    }
                }
                process = end + 1;
            }

            // A separate native branch scans every AST_ASSIGN_LE(0x56) whose
            // RHS root is AST_WHEN(0xb6). For each direct AST_COND child it
            // admits only a direct identifier or the first operand of a
            // binary AST_EQ(0x36); it does not call the general IF-condition
            // collector. Mirror that deliberately narrower candidate source.
            for (std::size_t assignment = range.begin;
                 assignment + 2 < range.end; ++assignment) {
                std::size_t rhs = assignment + 1;
                if (tokens[assignment].text == "<=") {
                    // combined token, if supplied by a future lexer
                } else if (tokens[assignment].text == "<" &&
                           tokens[assignment + 1].text == "=" &&
                           tokens[assignment + 1].offset ==
                               tokens[assignment].offset + 1) {
                    rhs = assignment + 2;
                } else {
                    continue;
                }
                auto statementEnd = rhs;
                int parentheses = 0;
                for (; statementEnd < range.end; ++statementEnd) {
                    if (tokens[statementEnd].text == "(") ++parentheses;
                    else if (tokens[statementEnd].text == ")" && parentheses > 0)
                        --parentheses;
                    if (parentheses == 0 && tokens[statementEnd].text == ";") break;
                }
                if (statementEnd >= range.end) continue;
                for (auto when = rhs; when < statementEnd; ++when) {
                    if (lower(tokens[when].text) != "when") continue;
                    auto conditionBegin = when + 1;
                    auto conditionEnd = conditionBegin;
                    int conditionDepth = 0;
                    for (; conditionEnd < statementEnd; ++conditionEnd) {
                        if (tokens[conditionEnd].text == "(") ++conditionDepth;
                        else if (tokens[conditionEnd].text == ")" && conditionDepth > 0)
                            --conditionDepth;
                        if (conditionDepth == 0 &&
                            lower(tokens[conditionEnd].text) == "else") break;
                    }
                    if (conditionEnd >= statementEnd) continue;
                    while (conditionEnd > conditionBegin + 1 &&
                           tokens[conditionBegin].text == "(" &&
                           tokens[conditionEnd - 1].text == ")") {
                        ++conditionBegin;
                        --conditionEnd;
                    }
                    std::size_t candidate = conditionEnd;
                    if (conditionEnd == conditionBegin + 1 &&
                        tokens[conditionBegin].kind == parser::HdlTokenKind::Identifier) {
                        candidate = conditionBegin;
                    } else if (conditionEnd > conditionBegin + 2 &&
                               tokens[conditionBegin].kind == parser::HdlTokenKind::Identifier &&
                               tokens[conditionBegin + 1].text == "=") {
                        candidate = conditionBegin;
                    }
                    if (candidate == conditionEnd ||
                        !clocks.contains(lower(tokens[candidate].text))) continue;
                    const auto& control = tokens[candidate];
                    emit(result, "GJB9765.4.9.b", source,
                         control.line, control.column,
                         "时钟信号 " + control.text +
                             " 被用作条件信号赋值的控制条件");
                }
                assignment = statementEnd;
            }
        }
    }
}

void directChecks(AnalysisResult& result, const core::SourceFile& source,
                  std::string_view standardId, const AnalysisOptions& options) {
    const bool vhdl = standardId == "GJB9765-2020";
    const auto lines = linesOf(source.content());
    const auto tokens = standardTokens(source);
    // Keep one configured preprocessing result for every Verilog rule whose
    // native input is a frontend AST. Raw physical-line predicates continue
    // to use source/tokens below. VHDL does not consume this Verilog branch.
    const auto preprocessed = parser::preprocess(
        source, {options.includeDirectories, options.defines});
    const auto activeTokens = preprocessed.issues.empty()
        ? standardTokens(preprocessed.source) : std::vector<parser::HdlToken>{};
    if (vhdl) {
        for (const auto& issue : parser::analyzeVhdlUnreachableComparisons(source))
            emit(result, "GJB9765.4.2.e", source, issue.location.line,
                 issue.location.column, "直接比较的常量超出无符号信号的可达范围：" +
                     issue.signal.value_or(""));
    }
    if (!vhdl) {
        // Native GJB10157.12.1 FUN_1805189f0 reads physical lines, finds the
        // first literal // byte pair, then searches that suffix for the exact
        // case-sensitive strings async_set_reset and sync_set_reset. The
        // async spelling is tested first and the rule emits at most once per
        // physical line. This is intentionally a raw byte predicate: block
        // comments do not qualify and a // sequence inside a quoted string is
        // still treated as the comment boundary by the native implementation.
        // ev_c948eda9a1c8d99739b284b3e656d671f783509a691270367c72e65b7f950fd9
        for (std::size_t index = 0; index < lines.size(); ++index) {
            const auto comment = lines[index].find("//");
            if (comment == std::string::npos) continue;
            const auto asyncReset = lines[index].find("async_set_reset", comment);
            const auto syncReset = asyncReset == std::string::npos
                ? lines[index].find("sync_set_reset", comment)
                : std::string::npos;
            const auto keyword = asyncReset != std::string::npos ? asyncReset : syncReset;
            if (keyword == std::string::npos) continue;
            emit(result, "GJB10157.12.1", source, index + 1, keyword + 1,
                 "禁止使用综合工具改变代码的复位属性.");
        }
    }
    if (!vhdl && preprocessed.issues.empty()) {
        checkVerilogStatementLeftAlignment(
            result, preprocessed.source, activeTokens);
        checkVerilogDeclarationLeftAlignment(
            result, preprocessed.source, activeTokens, lines);
    }
    if (!vhdl) checkVerilogPortOrder(result, source);
    if (vhdl) {
        // Native GJB9765.4.2.g FUN_1805d2930 filters entity records by an
        // exact same-file string key. When the resulting record count is
        // greater than one, it iterates the complete collection and reports
        // every entity at that entity node's +0x214 physical line. It does not
        // count architectures or entities in other source files.
        std::vector<const parser::DesignUnitInfo*> fileEntities;
        for (const auto& unit : result.structure) {
            if (unit.kind == parser::DesignUnitKind::VhdlEntity &&
                unit.location.file.lexically_normal() == source.path().lexically_normal())
                fileEntities.push_back(&unit);
        }
        if (fileEntities.size() > 1) {
            for (const auto* entity : fileEntities) {
                emit(result, "GJB9765.4.2.g", source,
                     entity->location.line, entity->location.column,
                     "一个文件宜对应一个设计实体：" + entity->name);
            }
        }
        checkVhdlGatedClocks(result, source, tokens);
        checkVhdlRippleClocks(result, source, tokens);
        checkVhdlMultipleClockDomains(result, source, options);
        checkVhdlClockAsControl(result, source, tokens);
        // Native GJB9765.4.9.d slot 0 (FUN_18065a0c0) consumes relation keys
        // 1, 2 and 3 separately.  The shared producer FUN_1804d56f0 proves
        // that key 3 is the no-edge class and therefore contributes no clock
        // name, while key 4 (an edge after an asynchronous branch whose action
        // is not proven constant) is deliberately absent from this rule.  Do
        // not use the broad timing model here: it also collects edge-function
        // calls in assignment RHS expressions and key-4 source shapes.
        //
        // Reconstruct the bounded key-1/key-2 source subset: a direct main-IF
        // edge is key 2; a non-edge first branch with an action accepted by
        // FUN_1804d20e0 followed by an ELSIF edge (or ELSE-first nested IF) is
        // key 1.  That helper accepts a direct literal, an all-literal concat,
        // an aggregate beginning with AST_OTHERS, or one bound identifier in
        // the per-root name/value tree built by FUN_1804cbad0.  The tree holds
        // owner-entity generics plus direct architecture constants and enum
        // literals, but not process-local constants or ordinary signals.
        // Each source IF selected at the process statement-block depth is
        // classified independently.  For key 2, FUN_1804d68a0 stores the
        // outer IF and its main-condition edge; linked source IFs beneath its
        // branch blocks only populate internal relation records and do not
        // add clocks to this rule.  Key 1 retains the admitted outer
        // IF/ELSIF chain.  Names are case-folded and equal name/edge records
        // are deduplicated before comparison.
        // ev_5e6f15370e774c6ffb365578fa6243dc83c96951956136536fb4ec8be605c7ef
        // /private/tmp/tlinter-494-main.txt, tlinter-494-builder.txt,
        // tlinter-494-rel3.txt, tlinter-494-rel14.txt and tlinter-494-rel2.txt
        struct NativeProcessClock {
            std::string name;
            bool rising{};
            std::size_t token{};
        };
        const auto firstEdgeInCondition = [&](std::size_t begin, std::size_t end)
            -> std::optional<NativeProcessClock> {
            const auto edge = findFirstVhdlEdgeCondition(source, tokens, begin, end);
            if (edge.status == VhdlEdgeConditionStatus::Matched)
                return NativeProcessClock{edge.clockName,
                    edge.direction == VhdlEdgeDirection::Rising, edge.clockToken};
            return std::nullopt;
        };

        const auto r94ArchitectureAt = vhdlArchitectureOrdinals(tokens);
        std::vector<std::size_t> r94ArchitectureStarts;
        for (std::size_t token = 0; token + 3 < tokens.size(); ++token) {
            if (lower(tokens[token].text) == "architecture" &&
                lower(tokens[token + 2].text) == "of" &&
                (token == 0 || lower(tokens[token - 1].text) != "end"))
                r94ArchitectureStarts.push_back(token);
        }
        std::vector<bool> r94DirectArchitectureToken(tokens.size(), false);
        for (std::size_t index = 0; index < r94ArchitectureStarts.size(); ++index) {
            const auto begin = r94ArchitectureStarts[index];
            const auto end = index + 1 < r94ArchitectureStarts.size()
                ? r94ArchitectureStarts[index + 1] : tokens.size();
            const auto direct = vhdlDirectArchitectureTokenMask(tokens, begin, end);
            for (auto token = begin; token < end; ++token)
                r94DirectArchitectureToken[token] =
                    r94DirectArchitectureToken[token] || direct[token];
        }
        std::map<std::string, std::set<std::string>> r94GenericsByEntity;
        for (const auto& unit : result.structure) {
            if (unit.kind != parser::DesignUnitKind::VhdlEntity) continue;
            auto& names = r94GenericsByEntity[lower(unit.name)];
            for (const auto& parameter : unit.parameters)
                names.insert(lower(parameter.name));
        }
        std::map<std::size_t, std::set<std::string>> r94StaticNames;
        for (std::size_t architecture = 0; architecture + 4 < tokens.size();
             ++architecture) {
            if (lower(tokens[architecture].text) != "architecture" ||
                (architecture > 0 && lower(tokens[architecture - 1].text) == "end"))
                continue;
            auto headerIs = architecture + 1;
            std::string entity;
            for (; headerIs < tokens.size() && lower(tokens[headerIs].text) != "is";
                 ++headerIs) {
                if (lower(tokens[headerIs].text) == "of" &&
                    headerIs + 1 < tokens.size() &&
                    tokens[headerIs + 1].kind == parser::HdlTokenKind::Identifier)
                    entity = lower(tokens[headerIs + 1].text);
            }
            if (headerIs >= tokens.size()) continue;
            const auto ordinal = r94ArchitectureAt[architecture];
            auto& names = r94StaticNames[ordinal];
            if (const auto generics = r94GenericsByEntity.find(entity);
                generics != r94GenericsByEntity.end())
                names.insert(generics->second.begin(), generics->second.end());

            // Skip nested subprogram/package bodies while locating the
            // architecture statement part.  FUN_1804cbad0 consumes only the
            // direct declarations of the owning design root for this tree.
            auto bodyBegin = tokens.size();
            for (auto cursor = headerIs + 1; cursor < tokens.size(); ++cursor) {
                const auto word = lower(tokens[cursor].text);
                if (word == "package" &&
                    (cursor == 0 || lower(tokens[cursor - 1].text) != "end")) {
                    auto nameToken = cursor + 1;
                    if (nameToken < tokens.size() && lower(tokens[nameToken].text) == "body")
                        ++nameToken;
                    const auto packageName = nameToken < tokens.size()
                        ? lower(tokens[nameToken].text) : std::string{};
                    auto ending = nameToken + 1;
                    for (; ending + 1 < tokens.size(); ++ending) {
                        if (lower(tokens[ending].text) != "end") continue;
                        const auto next = lower(tokens[ending + 1].text);
                        if (next == "package" || next == packageName ||
                            tokens[ending + 1].text == ";") break;
                    }
                    cursor = std::min(ending + 1, tokens.size() - 1);
                    continue;
                }
                if ((word == "function" || word == "procedure") &&
                    (cursor == 0 || lower(tokens[cursor - 1].text) != "end")) {
                    auto marker = cursor + 1;
                    int parentheses = 0;
                    for (; marker < tokens.size(); ++marker) {
                        if (tokens[marker].text == "(") ++parentheses;
                        else if (tokens[marker].text == ")" && parentheses > 0)
                            --parentheses;
                        else if (parentheses == 0 &&
                                 (lower(tokens[marker].text) == "is" ||
                                  tokens[marker].text == ";")) break;
                    }
                    if (marker < tokens.size() && lower(tokens[marker].text) == "is") {
                        auto ending = marker + 1;
                        for (; ending + 1 < tokens.size(); ++ending) {
                            if (lower(tokens[ending].text) != "end") continue;
                            const auto next = lower(tokens[ending + 1].text);
                            if (next == word || tokens[ending + 1].text == ";") break;
                        }
                        cursor = std::min(ending + 1, tokens.size() - 1);
                    } else if (marker < tokens.size()) {
                        cursor = marker;
                    }
                    continue;
                }
                if (word == "begin") {
                    bodyBegin = cursor;
                    break;
                }
                if (word == "end" && cursor + 1 < tokens.size() &&
                    lower(tokens[cursor + 1].text) == "architecture") break;
            }
            if (bodyBegin == tokens.size()) continue;
            for (auto declaration = headerIs + 1; declaration < bodyBegin; ++declaration) {
                const auto word = lower(tokens[declaration].text);
                if (word == "package" &&
                    (declaration == 0 || lower(tokens[declaration - 1].text) != "end")) {
                    auto nameToken = declaration + 1;
                    if (nameToken < bodyBegin && lower(tokens[nameToken].text) == "body")
                        ++nameToken;
                    const auto packageName = nameToken < bodyBegin
                        ? lower(tokens[nameToken].text) : std::string{};
                    auto ending = nameToken + 1;
                    for (; ending + 1 < bodyBegin; ++ending) {
                        if (lower(tokens[ending].text) != "end") continue;
                        const auto next = lower(tokens[ending + 1].text);
                        if (next == "package" || next == packageName ||
                            tokens[ending + 1].text == ";") break;
                    }
                    declaration = std::min(ending + 1, bodyBegin - 1);
                    continue;
                }
                if ((word == "function" || word == "procedure") &&
                    (declaration == 0 || lower(tokens[declaration - 1].text) != "end")) {
                    auto marker = declaration + 1;
                    int parentheses = 0;
                    for (; marker < bodyBegin; ++marker) {
                        if (tokens[marker].text == "(") ++parentheses;
                        else if (tokens[marker].text == ")" && parentheses > 0)
                            --parentheses;
                        else if (parentheses == 0 &&
                                 (lower(tokens[marker].text) == "is" ||
                                  tokens[marker].text == ";")) break;
                    }
                    if (marker < bodyBegin && lower(tokens[marker].text) == "is") {
                        auto ending = marker + 1;
                        for (; ending + 1 < bodyBegin; ++ending) {
                            if (lower(tokens[ending].text) != "end") continue;
                            const auto next = lower(tokens[ending + 1].text);
                            if (next == word || tokens[ending + 1].text == ";") break;
                        }
                        declaration = std::min(ending + 1, bodyBegin - 1);
                    } else if (marker < bodyBegin) {
                        declaration = marker;
                    }
                    continue;
                }
                if (word == "constant") {
                    auto colon = declaration + 1;
                    while (colon < bodyBegin && tokens[colon].text != ":" &&
                           tokens[colon].text != ";") ++colon;
                    if (colon < bodyBegin && tokens[colon].text == ":")
                        for (auto name = declaration + 1; name < colon; ++name)
                            if (tokens[name].kind == parser::HdlTokenKind::Identifier)
                                names.insert(lower(tokens[name].text));
                    while (declaration < bodyBegin && tokens[declaration].text != ";")
                        ++declaration;
                    continue;
                }
                if (word == "type") {
                    auto isToken = declaration + 1;
                    auto semicolon = isToken;
                    while (semicolon < bodyBegin && tokens[semicolon].text != ";")
                        ++semicolon;
                    while (isToken < semicolon && lower(tokens[isToken].text) != "is")
                        ++isToken;
                    if (isToken + 1 < semicolon && tokens[isToken + 1].text == "(") {
                        const auto close = closeParen(tokens, isToken + 1);
                        if (close < semicolon)
                            for (auto literal = isToken + 2; literal < close; ++literal)
                                if (tokens[literal].kind == parser::HdlTokenKind::Identifier)
                                    names.insert(lower(tokens[literal].text));
                    }
                    declaration = semicolon;
                }
            }
            architecture = bodyBegin;
        }

        const auto literalAtom = [&](std::size_t begin, std::size_t end) {
            if (begin >= end) return false;
            if (end == begin + 1) {
                const auto& item = tokens[begin];
                const auto& value = item.text;
                return item.kind == parser::HdlTokenKind::Number ||
                    item.kind == parser::HdlTokenKind::String ||
                    (value.size() >= 2 &&
                     ((value.front() == '\'' && value.back() == '\'') ||
                      (value.front() == '"' && value.back() == '"')));
            }
            const bool character = end == begin + 3 && tokens[begin].text == "'" &&
                tokens[begin + 2].text == "'" &&
                tokens[begin + 1].offset == tokens[begin].offset + 1 &&
                tokens[begin + 2].offset == tokens[begin + 1].offset +
                    tokens[begin + 1].text.size();
            const bool joinedCharacter = end == begin + 2 && tokens[begin].text == "'" &&
                tokens[begin + 1].kind == parser::HdlTokenKind::Number &&
                tokens[begin + 1].text.size() == 2 &&
                tokens[begin + 1].text.back() == '\'' &&
                tokens[begin + 1].offset == tokens[begin].offset + 1;
            return character || joinedCharacter;
        };
        const auto nativeStaticRhs = [&](std::size_t begin, std::size_t end,
                                         const std::set<std::string>& staticNames) {
            while (begin < end && tokens[begin].text == "(" &&
                   closeParen(tokens, begin) + 1 == end) {
                ++begin;
                --end;
            }
            if (literalAtom(begin, end)) return true;
            if (end == begin + 1 &&
                tokens[begin].kind == parser::HdlTokenKind::Identifier &&
                staticNames.contains(lower(tokens[begin].text))) return true;

            // The native AST_CONCAT path immediately accepts a first
            // AST_OTHERS child; otherwise every direct child must be a literal.
            if (begin + 2 < end && lower(tokens[begin].text) == "others" &&
                tokens[begin + 1].text == "=>") return true;
            std::vector<std::pair<std::size_t, std::size_t>> atoms;
            auto atom = begin;
            int depth = 0;
            for (auto cursor = begin; cursor <= end; ++cursor) {
                if (cursor < end && tokens[cursor].text == "(") ++depth;
                else if (cursor < end && tokens[cursor].text == ")" && depth > 0) --depth;
                const bool separator = cursor == end ||
                    (depth == 0 && (tokens[cursor].text == "&" ||
                                    tokens[cursor].text == ","));
                if (!separator) continue;
                atoms.push_back({atom, cursor});
                atom = cursor + 1;
            }
            return atoms.size() > 1 && std::ranges::all_of(atoms, [&](const auto& item) {
                return literalAtom(item.first, item.second);
            });
        };
        const auto directNativeStaticAssignment = [&](std::size_t begin,
                                                       std::size_t limit,
                                                       std::size_t architecture) {
            const auto foundNames = r94StaticNames.find(architecture);
            const std::set<std::string> emptyNames;
            const auto& staticNames = foundNames == r94StaticNames.end()
                ? emptyNames : foundNames->second;
            for (auto token = begin; token + 2 < limit; ++token) {
                std::size_t rhs = token + 2;
                if (tokens[token + 1].text == "<=") {
                    // already positioned
                } else if (token + 3 < limit && tokens[token + 1].text == "<" &&
                           tokens[token + 2].text == "=" &&
                           tokens[token + 2].offset == tokens[token + 1].offset + 1) {
                    rhs = token + 3;
                } else {
                    continue;
                }
                auto finish = rhs;
                while (finish < limit && tokens[finish].text != ";") ++finish;
                if (finish >= limit) continue;
                if (nativeStaticRhs(rhs, finish, staticNames)) return true;
            }
            return false;
        };
        for (std::size_t process = 0; process + 1 < tokens.size(); ++process) {
            if (lower(tokens[process].text) != "process" ||
                !r94DirectArchitectureToken[process] ||
                (process > 0 && lower(tokens[process - 1].text) == "end")) continue;
            auto processEnd = process + 1;
            while (processEnd + 1 < tokens.size() &&
                   !(lower(tokens[processEnd].text) == "end" &&
                     lower(tokens[processEnd + 1].text) == "process")) ++processEnd;
            if (processEnd + 1 >= tokens.size()) continue;
            auto body = process + 1;
            int parentheses = 0;
            for (; body < processEnd; ++body) {
                if (tokens[body].text == "(") ++parentheses;
                else if (tokens[body].text == ")" && parentheses > 0) --parentheses;
                if (parentheses == 0 && lower(tokens[body].text) == "begin") {
                    ++body;
                    break;
                }
            }
            if (body >= processEnd) {
                process = processEnd + 1;
                continue;
            }
            std::vector<NativeProcessClock> clocks;
            struct TopLevelSourceIf {
                std::size_t begin{};
                std::size_t end{}; // index of the matching `end` token
            };
            std::vector<TopLevelSourceIf> sourceIfs;
            std::optional<std::size_t> topLevelIf;
            unsigned ifDepth = 0;
            unsigned caseDepth = 0;
            unsigned loopDepth = 0;
            for (auto token = body; token + 1 < processEnd; ++token) {
                const auto word = lower(tokens[token].text);
                if (word == "end") {
                    const auto closing = lower(tokens[token + 1].text);
                    if (closing == "if" && ifDepth != 0) {
                        --ifDepth;
                        if (ifDepth == 0 && topLevelIf) {
                            sourceIfs.push_back({*topLevelIf, token});
                            topLevelIf.reset();
                        }
                    } else if (closing == "case" && caseDepth != 0) {
                        --caseDepth;
                    } else if (closing == "loop" && loopDepth != 0) {
                        --loopDepth;
                    }
                    if (closing == "if" || closing == "case" || closing == "loop")
                        ++token; // do not reinterpret the closing keyword as an opener
                    continue;
                }
                if (word == "if") {
                    if (ifDepth == 0 && caseDepth == 0 && loopDepth == 0)
                        topLevelIf = token;
                    ++ifDepth;
                } else if (word == "case") {
                    ++caseDepth;
                } else if (word == "loop") {
                    ++loopDepth;
                }
            }

            for (const auto& sourceIf : sourceIfs) {
                auto firstThen = sourceIf.begin + 1;
                int conditionDepth = 0;
                for (; firstThen < sourceIf.end; ++firstThen) {
                    if (tokens[firstThen].text == "(") ++conditionDepth;
                    else if (tokens[firstThen].text == ")" && conditionDepth > 0)
                        --conditionDepth;
                    if (conditionDepth == 0 && lower(tokens[firstThen].text) == "then") break;
                }
                if (firstThen >= sourceIf.end) continue;

                if (auto mainEdge = firstEdgeInCondition(sourceIf.begin + 1, firstThen)) {
                    // Native category 2 stores only the outer main-condition
                    // relation.  ELSIF and nested linked-source IF clocks are
                    // internal metadata for neighboring rules.
                    clocks.push_back(std::move(*mainEdge));
                    continue;
                }

                // Category 1 walks every direct ELSIF in source order.  Its
                // static-action flag may be established by the main branch or
                // a non-edge ELSIF while no *extra* edge record exists.  Once
                // a second edge has been appended, later non-edge branches no
                // longer change the category-1/category-4 decision.
                std::vector<std::size_t> elsifTokens;
                std::size_t elseToken = sourceIf.end;
                unsigned nestedIf = 0;
                for (auto token = firstThen + 1; token < sourceIf.end; ++token) {
                    const auto word = lower(tokens[token].text);
                    if (word == "end" && token + 1 < sourceIf.end &&
                        lower(tokens[token + 1].text) == "if") {
                        if (nestedIf != 0) --nestedIf;
                        ++token;
                        continue;
                    }
                    if (word == "if") {
                        ++nestedIf;
                    } else if (nestedIf == 0 && word == "elsif") {
                        elsifTokens.push_back(token);
                    } else if (nestedIf == 0 && word == "else") {
                        elseToken = token;
                        break;
                    }
                }

                const auto firstBranchEnd = !elsifTokens.empty() ? elsifTokens.front()
                    : elseToken < sourceIf.end ? elseToken : sourceIf.end;
                bool staticAction = directNativeStaticAssignment(
                    firstThen + 1, firstBranchEnd, r94ArchitectureAt[process]);
                std::vector<NativeProcessClock> categoryClocks;
                std::size_t extraEdges = 0;
                for (std::size_t branch = 0; branch < elsifTokens.size(); ++branch) {
                    const auto elsifToken = elsifTokens[branch];
                    const auto branchEnd = branch + 1 < elsifTokens.size()
                        ? elsifTokens[branch + 1]
                        : elseToken < sourceIf.end ? elseToken : sourceIf.end;
                    auto thenToken = elsifToken + 1;
                    conditionDepth = 0;
                    for (; thenToken < branchEnd; ++thenToken) {
                        if (tokens[thenToken].text == "(") ++conditionDepth;
                        else if (tokens[thenToken].text == ")" && conditionDepth > 0)
                            --conditionDepth;
                        if (conditionDepth == 0 && lower(tokens[thenToken].text) == "then") break;
                    }
                    if (thenToken >= branchEnd) continue;
                    if (auto edge = firstEdgeInCondition(elsifToken + 1, thenToken)) {
                        if (!categoryClocks.empty()) ++extraEdges;
                        categoryClocks.push_back(std::move(*edge));
                    } else if (extraEdges == 0) {
                        staticAction = staticAction || directNativeStaticAssignment(
                            thenToken + 1, branchEnd, r94ArchitectureAt[process]);
                    }
                }

                // AST_ELSE may contain several direct source-linked IFs.  The
                // native loop overwrites its pointer and classifies only the
                // last such IF; its direct main AST_COND is the relation root.
                std::optional<TopLevelSourceIf> linkedElseIf;
                if (elseToken < sourceIf.end) {
                    std::optional<std::size_t> directIf;
                    unsigned elseIfDepth = 0;
                    unsigned elseCaseDepth = 0;
                    unsigned elseLoopDepth = 0;
                    for (auto token = elseToken + 1; token + 1 < sourceIf.end; ++token) {
                        const auto word = lower(tokens[token].text);
                        if (word == "end") {
                            const auto closing = lower(tokens[token + 1].text);
                            if (closing == "if" && elseIfDepth != 0) {
                                --elseIfDepth;
                                if (elseIfDepth == 0 && directIf) {
                                    linkedElseIf = TopLevelSourceIf{*directIf, token};
                                    directIf.reset();
                                }
                            } else if (closing == "case" && elseCaseDepth != 0) {
                                --elseCaseDepth;
                            } else if (closing == "loop" && elseLoopDepth != 0) {
                                --elseLoopDepth;
                            }
                            if (closing == "if" || closing == "case" || closing == "loop")
                                ++token;
                            continue;
                        }
                        if (word == "if") {
                            if (elseIfDepth == 0 && elseCaseDepth == 0 && elseLoopDepth == 0)
                                directIf = token;
                            ++elseIfDepth;
                        } else if (word == "case") {
                            ++elseCaseDepth;
                        } else if (word == "loop") {
                            ++elseLoopDepth;
                        }
                    }
                }
                if (linkedElseIf) {
                    auto linkedThen = linkedElseIf->begin + 1;
                    conditionDepth = 0;
                    for (; linkedThen < linkedElseIf->end; ++linkedThen) {
                        if (tokens[linkedThen].text == "(") ++conditionDepth;
                        else if (tokens[linkedThen].text == ")" && conditionDepth > 0)
                            --conditionDepth;
                        if (conditionDepth == 0 && lower(tokens[linkedThen].text) == "then") break;
                    }
                    if (linkedThen < linkedElseIf->end) {
                        if (auto edge = firstEdgeInCondition(
                                linkedElseIf->begin + 1, linkedThen)) {
                            if (!categoryClocks.empty()) ++extraEdges;
                            categoryClocks.push_back(std::move(*edge));
                        } else if (extraEdges == 0) {
                            staticAction = staticAction || directNativeStaticAssignment(
                                linkedElseIf->begin, linkedElseIf->end,
                                r94ArchitectureAt[process]);
                        }
                    }
                }

                if (!staticAction || categoryClocks.empty())
                    continue; // no edge is key 3; unproven static action is key 4
                clocks.insert(clocks.end(), categoryClocks.begin(), categoryClocks.end());
            }
            std::map<std::string, bool> clockEdges;
            for (const auto& clock : clocks) {
                const auto existing = clockEdges.find(clock.name);
                const auto& location = tokens[clock.token];
                if (existing != clockEdges.end()) {
                    if (existing->second != clock.rising) {
                        emit(result, "GJB9765.4.9.d", source, location.line, location.column,
                             "同一进程中时钟信号 " + location.text + " 出现多个沿触发");
                    }
                    continue;
                }
                if (!clockEdges.empty()) {
                    emit(result, "GJB9765.4.9.d", source, location.line, location.column,
                         "同一进程中出现多个时钟信号，新增时钟：" + location.text);
                }
                clockEdges.emplace(clock.name, clock.rising);
            }
            process = processEnd + 1;
        }
        // Native GJB9765.4.9.i FUN_180694590 obtains processes through
        // nonrecursive FUN_1804c5890, so only direct architecture process
        // children contribute category-1/category-2 clock relations. It then
        // recursively scans signal assignments (type 0x56): when
        // `target <= not source` connects two
        // clock relations having the same physical edge, the inverter makes
        // their effective edges opposite and the rule reports that pair.
        // ev_584130e2c20c552bc1230f76738008586ee766dae4048b6b9944b992adccd291
        struct VhdlArchitectureRange {
            std::string entity;
            std::size_t begin{};
            std::size_t end{};
        };
        std::vector<VhdlArchitectureRange> architectureRanges;
        for (std::size_t index = 0; index + 3 < tokens.size(); ++index) {
            if (lower(tokens[index].text) != "architecture" ||
                lower(tokens[index + 2].text) != "of") continue;
            const auto entity = lower(tokens[index + 3].text);
            auto end = index + 4;
            for (; end + 1 < tokens.size(); ++end) {
                if (lower(tokens[end].text) == "end" &&
                    lower(tokens[end + 1].text) == "architecture") {
                    end += 2;
                    break;
                }
            }
            architectureRanges.push_back({entity, index, std::min(end, tokens.size())});
            index = end == 0 ? index : end - 1;
        }
        for (const auto& unit : result.structure) {
            if (unit.kind != parser::DesignUnitKind::VhdlEntity) continue;
            struct DirectProcessOffsets {
                std::size_t begin{};
                std::size_t end{};
            };
            std::vector<DirectProcessOffsets> directProcesses;
            for (const auto& range : architectureRanges) {
                if (range.entity != lower(unit.name)) continue;
                for (auto process = range.begin; process < range.end; ++process) {
                    if (lower(tokens[process].text) != "process" ||
                        !r94DirectArchitectureToken[process] ||
                        (process > range.begin &&
                         lower(tokens[process - 1].text) == "end")) continue;
                    auto end = process + 1;
                    while (end + 1 < range.end &&
                           !(lower(tokens[end].text) == "end" &&
                             lower(tokens[end + 1].text) == "process")) ++end;
                    if (end + 1 >= range.end) continue;
                    end += 2;
                    directProcesses.push_back({
                        tokens[process].offset,
                        tokens[end - 1].offset + tokens[end - 1].text.size()});
                    process = end - 1;
                }
            }
            std::map<std::string, unsigned> edgeMasks;
            std::map<std::string, const parser::ClockInfo*> lastClock;
            std::set<std::string> reportedDirect;
            for (const auto& clock : unit.clocks) {
                if (clock.location.file.lexically_normal() !=
                    source.path().lexically_normal()) continue;
                const bool fromDirectProcess = std::ranges::any_of(
                    directProcesses, [&](const auto& process) {
                        return clock.location.byteOffset >= process.begin &&
                               clock.location.byteOffset < process.end;
                    });
                if (!fromDirectProcess) continue;
                const auto name = lower(clock.signal);
                auto& mask = edgeMasks[name];
                mask |= clock.risingEdge ? 1U : 2U;
                lastClock[name] = &clock;
                if (mask != 3U || !reportedDirect.insert(name).second) continue;
                emit(result, "GJB9765.4.9.i", source,
                     clock.location.line, clock.location.column,
                     "同一时钟源同时驱动上升沿和下降沿寄存器：" + clock.signal);
            }
            std::set<std::pair<std::string, std::string>> reportedInverters;
            for (const auto& range : architectureRanges) {
                if (range.entity != lower(unit.name)) continue;
                for (auto index = range.begin; index + 4 < range.end; ++index) {
                    if (tokens[index].kind != parser::HdlTokenKind::Identifier ||
                        tokens[index + 1].text != "<=" ||
                        lower(tokens[index + 2].text) != "not" ||
                        tokens[index + 3].kind != parser::HdlTokenKind::Identifier ||
                        tokens[index + 4].text != ";") continue;
                    const auto target = lower(tokens[index].text);
                    const auto sourceName = lower(tokens[index + 3].text);
                    if (target == sourceName) continue;
                    const auto targetEdges = edgeMasks.find(target);
                    const auto sourceEdges = edgeMasks.find(sourceName);
                    if (targetEdges == edgeMasks.end() || sourceEdges == edgeMasks.end() ||
                        (targetEdges->second & sourceEdges->second) == 0) continue;
                    const auto pair = std::minmax(target, sourceName);
                    if (!reportedInverters.emplace(pair.first, pair.second).second) continue;
                    const auto location = lastClock.find(target);
                    const auto line = location != lastClock.end()
                        ? location->second->location.line : tokens[index].line;
                    const auto column = location != lastClock.end()
                        ? location->second->location.column : tokens[index].column;
                    emit(result, "GJB9765.4.9.i", source, line, column,
                         "反相时钟别名使同一时钟源驱动不同触发沿：" +
                             tokens[index + 3].text + " / " + tokens[index].text);
                }
            }
        }
        checkVhdlPortDeclarationComments(result, source, lines);
        checkVhdlInternalDeclarationComments(result, source, lines);
        checkVhdlMultipleEventControls(result, source, tokens);
        checkVhdlCombinationalIfElse(result, source, tokens);
        checkVhdlNativeLatchRisks(result, source, tokens);
        checkVhdlStaticCaseChoices(result, source, tokens);
        checkVhdlForVariableWrites(result, source, tokens);
        checkVhdlConstantStaticInitializers(result, source, tokens);
        checkVhdlImplicitNumericConversions(result, source, tokens);
        checkVhdlConstantControlConditions(result, source, tokens);
        checkVhdlResetNames(result, source, tokens);
        checkVhdlUnsynthesizableConstructs(result, source, tokens);
        checkVhdlStdLogicVectorRanges(result, source, tokens);
        checkVhdlSharedAsyncControl(result, source, tokens);
        checkVhdlAsyncResetPaths(result, source, tokens);
        checkVhdlAsyncResetSynchronousRelease(result, source, tokens);
        checkVhdlExternalSynchronousReset(result, source, tokens);
        checkVhdlRegisterResetInitialization(result, source, tokens);
        checkVhdlFsmEnumerationType(result, source, tokens);
        checkVhdlTriStateSharedSource(result, source, tokens);
        checkVhdlCombinationalTriStateEnable(result, source, tokens);
        checkVhdlMultipleDrivers(result, source, tokens);
        checkVhdlTopLevelInstantiationOnly(result, source, tokens, options);
        checkVhdlComponentPortConnections(result, source, tokens);
        checkVhdlResetPortsNotOpen(result, source, tokens);
        checkVhdlSingleResetDomain(result, source, tokens);
        checkVhdlCrossHierarchySignalNames(result, source, tokens);
        checkVhdlHierarchyUndrivenReads(result, source, tokens);
        checkVhdlPortlessComponents(result, source, tokens);
        checkVhdlEntityPortDirectionSuffixes(result, source, tokens);
        checkVhdlIdentifierFoundation(result, source, tokens);
        checkVhdlDeclarationPrefixes(result, source, tokens);
        checkVhdlConstantAndDesignUnitNames(result, source, tokens);
        checkVhdlSemanticSignalNames(result, source, tokens);
        checkVhdlStatementLabelSuffixes(result, source, tokens);
        // Native 4.14.b.2 tests only a literal ASCII space immediately
        // before LF, or before CRLF. A trailing Tab and an unterminated final
        // line do not satisfy its condition. Do not translate generic
        // STYLE-001 here: it has a wider whitespace predicate.
        // ev_85d543f2dad139b11c46626c705dbcaade605b36564317949dfe2ab259432a27
        const auto bChunks = nativeVhdlTextChunks(source.content(), 511);
        for (std::size_t index = 0; index < bChunks.size(); ++index) {
            const auto& chunk = bChunks[index];
            std::size_t space = std::string::npos;
            if (chunk.size() > 1 && chunk.back() == '\n' &&
                chunk[chunk.size() - 2] == ' ')
                space = chunk.size() - 2;
            else if (chunk.size() > 2 && chunk.back() == '\n' &&
                     chunk[chunk.size() - 2] == '\r' &&
                     chunk[chunk.size() - 3] == ' ')
                space = chunk.size() - 3;
            if (space != std::string::npos)
                emit(result, "GJB9765.4.14.b.2", source, index + 1,
                     space + 1, "换行前存在多余的 ASCII 空格");
        }
        // Native 4.14.a.1 checks the physical line immediately preceding a
        // selected AST node: any non-whitespace byte before -- is a violation.
        // The AST collector includes several still unnamed types. Admit only
        // a standalone sequential PROCESS start here; other node types stay
        // under review. ev_790668283906571028e81f81bf57008817febb31ba396b5d38567cb6d83dcb5b
        // ev_3dd60213fcc200982d81dcaa3f48fa82e51baff98d2c033bfd5171bd187bf08a
        for (std::size_t index = 0; index < tokens.size(); ++index) {
            if (lower(tokens[index].text) != "process" ||
                (index > 0 && lower(tokens[index - 1].text) == "end")) continue;
            const auto line = tokens[index].line;
            if (line < 2 || line > lines.size() || lines[line - 2].size() > 511 ||
                lines[line - 1].size() > 511) continue;
            std::size_t first = index;
            while (first > 0 && tokens[first - 1].line == line) --first;
            // The process keyword may follow one label and/or POSTPONED on
            // its own line. Avoid treating arbitrary expressions as AST nodes.
            auto cursor = first;
            if (cursor + 1 < index && tokens[cursor + 1].text == ":") cursor += 2;
            if (cursor < index && lower(tokens[cursor].text) == "postponed") ++cursor;
            if (cursor != index || !nativeVhdlPreviousLineHasCode(lines[line - 2])) continue;
            emit(result, "GJB9765.4.14.a.1", source, line, 1,
                 "该过程语句与前一节之间缺少空行或独立注释行");
        }
        // The native rule checks source bytes, including whitespace-only
        // lines. An unindented first code byte and a leading Tab are accepted;
        // leading spaces before a recognised code byte are reported.
        for (std::size_t index = 0; index < bChunks.size(); ++index) {
            if (nativeVhdlLeadingSpaceViolation(bChunks[index]))
                emit(result, "GJB9765.4.14.b.1", source, index + 1, 1,
                     "行首缩进未使用 Tab（原生逐行扫描判断）");
        }
        const auto tokenColumn = [&](const parser::HdlToken& token) {
            return token.line > 0 && token.line <= lines.size()
                ? tab4DisplayColumn(lines[token.line - 1], token.column)
                : token.column;
        };
        const auto startsIfStatement = [&](std::size_t index) {
            if (index > 0 && lower(tokens[index - 1].text) == "end") return false;
            for (auto cursor = index + 1; cursor < tokens.size(); ++cursor) {
                const auto word = lower(tokens[cursor].text);
                if (word == "then") return true;
                if (word == "generate" || tokens[cursor].text == ";") return false;
            }
            return false;
        };
        // Native 4.8.e FUN_180609b30 visits only AST_ALWAYS(0x52) roots.
        // FUN_1806097d0 recursively appends every AST_NULL(0xb0) and every
        // zero-child AST_BLOCK(0x54), without deduplication. The entry also
        // appends an AST_ALWAYS whose first AST_BLOCK has exactly one
        // AST_EDGE(0x67), covering a sensitivity/event-only empty process.
        // Mirror the unequivocal source forms: NULL, empty process bodies,
        // empty IF/ELSIF/ELSE arms, empty CASE alternatives and empty loops.
        // docs/evidence/native_vhdl_empty_statement_4_8_e_findings.md
        bool inSequentialProcess = false;
        bool inProcessBody = false;
        for (std::size_t index = 0; index < tokens.size(); ++index) {
            const auto word = lower(tokens[index].text);
            if (word == "end" && index + 1 < tokens.size() &&
                lower(tokens[index + 1].text) == "process") {
                inSequentialProcess = false;
                inProcessBody = false;
                continue;
            }
            if (word == "process" &&
                (index == 0 || lower(tokens[index - 1].text) != "end")) {
                inSequentialProcess = true;
                inProcessBody = false;
                continue;
            }
            if (!inSequentialProcess) continue;
            if (!inProcessBody) {
                if (word == "begin") {
                    inProcessBody = true;
                    if (index + 2 < tokens.size() &&
                        lower(tokens[index + 1].text) == "end" &&
                        lower(tokens[index + 2].text) == "process")
                        emit(result, "GJB9765.4.8.e", source,
                             tokens[index].line, tokens[index].column,
                             "进程体没有语句");
                }
                continue;
            }
            if (word == "null" && index + 1 < tokens.size() &&
                tokens[index + 1].text == ";") {
                emit(result, "GJB9765.4.8.e", source,
                     tokens[index].line, tokens[index].column,
                     "进程中存在显式空语句 NULL");
                continue;
            }
            if ((word == "then" || word == "else") && index + 1 < tokens.size()) {
                const auto next = lower(tokens[index + 1].text);
                if (next == "elsif" || next == "else" ||
                    (next == "end" && index + 2 < tokens.size() &&
                     lower(tokens[index + 2].text) == "if"))
                    emit(result, "GJB9765.4.8.e", source,
                         tokens[index].line, tokens[index].column,
                         "进程中的 if 分支没有语句");
            }
            if (tokens[index].text == "=>" && index + 1 < tokens.size()) {
                const auto next = lower(tokens[index + 1].text);
                if (next == "when" ||
                    (next == "end" && index + 2 < tokens.size() &&
                     lower(tokens[index + 2].text) == "case"))
                    emit(result, "GJB9765.4.8.e", source,
                         tokens[index].line, tokens[index].column,
                         "进程中的 case 分支没有语句");
            }
            if (word == "loop" &&
                (index == 0 || lower(tokens[index - 1].text) != "end") &&
                index + 2 < tokens.size() &&
                lower(tokens[index + 1].text) == "end" &&
                lower(tokens[index + 2].text) == "loop")
                emit(result, "GJB9765.4.8.e", source,
                     tokens[index].line, tokens[index].column,
                     "进程中的 loop 循环体没有语句");
        }
        // Native 4.14.b.3 builds two bounded alignment groups from the VHDL
        // AST: an IF group containing its if/elsif/else keywords, and a CASE
        // group containing case plus its direct when alternatives. It locates
        // each keyword in the comment-stripped physical line and expands tabs
        // to four-column stops. IF branches must equal the opening IF column.
        // Every WHEN must be strictly right of CASE and equal the first WHEN
        // column; a later shallow/misaligned WHEN can therefore report both
        // native messages. The recovered predicate does not scan every source
        // statement and does not require an exact four-column depth increase.
        // ev_a39fe443829bce6c25db2f71db7f0951e6cda79dac416c76278906e0bc593bca
        // ev_aa1ed509b11af39111945ecd1a31ee93dde701d51e407026fbf41f6c58e00228
        // ev_66ec6161160253483e284ae1b60b5068478ce309bf81dd16a35c283a93dd5670
        // ev_f31132677fd202edca2f346ccc9d0177554b977f02a4f4c5fb576326b48c84d7
        // ev_2cf36ac4802a0e52d54cf39e117f828436954264b4a65bb6e37fe94939763965
        struct IfAlignmentGroup { const parser::HdlToken* opening; };
        std::vector<IfAlignmentGroup> ifAlignment;
        for (std::size_t index = 0; index < tokens.size(); ++index) {
            if (tokens[index].kind != parser::HdlTokenKind::Identifier) continue;
            const auto word = lower(tokens[index].text);
            if (word == "end" && index + 1 < tokens.size() &&
                lower(tokens[index + 1].text) == "if") {
                if (!ifAlignment.empty()) ifAlignment.pop_back();
                ++index;
                continue;
            }
            if (word == "elsif") {
                if (!ifAlignment.empty() &&
                    tokenColumn(tokens[index]) != tokenColumn(*ifAlignment.back().opening))
                    emit(result, "GJB9765.4.14.b.3", source,
                         tokens[index].line, tokens[index].column,
                         "同一 if 语句的 elsif 未与起始 if 左端对齐");
                continue;
            }
            if (word == "else") {
                const bool branchKeyword = index > 0 &&
                    (tokens[index - 1].text == ";" || lower(tokens[index - 1].text) == "then");
                if (branchKeyword && !ifAlignment.empty() &&
                    tokenColumn(tokens[index]) != tokenColumn(*ifAlignment.back().opening))
                    emit(result, "GJB9765.4.14.b.3", source,
                         tokens[index].line, tokens[index].column,
                         "同一 if 语句的 else 未与起始 if 左端对齐");
                continue;
            }
            if (word == "if" && startsIfStatement(index))
                ifAlignment.push_back({&tokens[index]});
        }

        struct CaseAlignmentGroup {
            const parser::HdlToken* opening;
            std::optional<std::size_t> firstWhenColumn;
        };
        const auto startsCaseStatement = [&](std::size_t index) {
            if (index > 0 && lower(tokens[index - 1].text) == "end") return false;
            for (auto cursor = index + 1; cursor < tokens.size(); ++cursor) {
                const auto word = lower(tokens[cursor].text);
                if (word == "is") return true;
                if (word == "generate" || tokens[cursor].text == ";") return false;
            }
            return false;
        };
        const auto startsCaseAlternative = [&](std::size_t index) {
            for (auto cursor = index + 1; cursor < tokens.size(); ++cursor) {
                if (tokens[cursor].text == "=>") return true;
                if (tokens[cursor].text == ";") return false;
            }
            return false;
        };
        // Native 4.8.a collects declared constant names, then inspects the
        // first child of each sequential CASE. A literal is reported; an
        // identifier is reported when its case-folded name is in the constant
        // set; a compound expression is reported when none of its identifier
        // descendants depends on a non-constant name. The AST collector's
        // recovered 0x76 -> 0xa3 -> 0x13 path is entity -> generic -> direct
        // identifier, while flagged 0x0a records cover constants below the
        // owning root. Admit owner generics plus architecture/process constant
        // declarations and bounded selector expressions.
        // ev_bc0f722c9b21f3f1bebb5edbc5e7316797c851c5d91e88e91a9096d6f7d98ca4
        // ev_0045125577774a59b95281d68c8a30bc680ec3ce1750c75bddd6a5789dc89c47
        // ev_175fe37265fd337fda2ce81459024ad8d4cf613bc779baa1430a338955202284
        std::map<std::string, std::set<std::string>> entityGenericConstants;
        for (const auto& unit : result.structure) {
            if (unit.kind != parser::DesignUnitKind::VhdlEntity ||
                unit.location.file.lexically_normal() != source.path().lexically_normal()) continue;
            auto& generics = entityGenericConstants[lower(unit.name)];
            for (const auto& parameter : unit.parameters)
                generics.insert(lower(parameter.name));
        }
        std::set<std::string> architectureConstants;
        bool architectureHeader = false;
        bool architectureDeclarations = false;
        bool architectureBody = false;
        for (std::size_t index = 0; index < tokens.size(); ++index) {
            if (tokens[index].kind != parser::HdlTokenKind::Identifier) continue;
            const auto word = lower(tokens[index].text);
            if (word == "architecture" &&
                (index == 0 || lower(tokens[index - 1].text) != "end")) {
                architectureConstants.clear();
                for (auto cursor = index + 1; cursor + 1 < tokens.size(); ++cursor) {
                    const auto headerWord = lower(tokens[cursor].text);
                    if (headerWord == "is") break;
                    if (headerWord == "of" &&
                        tokens[cursor + 1].kind == parser::HdlTokenKind::Identifier) {
                        const auto found = entityGenericConstants.find(lower(tokens[cursor + 1].text));
                        if (found != entityGenericConstants.end())
                            architectureConstants.insert(found->second.begin(), found->second.end());
                        break;
                    }
                }
                architectureHeader = true;
                architectureDeclarations = false;
                architectureBody = false;
                continue;
            }
            if (word == "end" && index + 1 < tokens.size() &&
                lower(tokens[index + 1].text) == "architecture") {
                architectureHeader = false;
                architectureDeclarations = false;
                architectureBody = false;
                continue;
            }
            if (architectureHeader && word == "is") {
                architectureHeader = false;
                architectureDeclarations = true;
                continue;
            }
            if (architectureDeclarations && word == "begin") {
                architectureDeclarations = false;
                architectureBody = true;
                continue;
            }
            if ((architectureDeclarations || architectureBody) && word == "constant") {
                auto cursor = index + 1;
                std::vector<std::string> declaredNames;
                while (cursor < tokens.size() &&
                       tokens[cursor].kind == parser::HdlTokenKind::Identifier) {
                    declaredNames.push_back(lower(tokens[cursor].text));
                    ++cursor;
                    if (cursor >= tokens.size() || tokens[cursor].text != ",") break;
                    ++cursor;
                }
                if (cursor < tokens.size() && tokens[cursor].text == ":")
                    architectureConstants.insert(declaredNames.begin(), declaredNames.end());
                continue;
            }
            if (!architectureBody || word != "case" || !startsCaseStatement(index)) continue;
            auto end = index + 1;
            while (end < tokens.size() && lower(tokens[end].text) != "is") ++end;
            if (end == tokens.size() || end == index + 1) continue;
            auto begin = index + 1;
            while (begin < end && tokens[begin].text == "(" &&
                   closeParen(tokens, begin) == end - 1) {
                ++begin;
                --end;
            }
            if (begin >= end) continue;
            bool hasOperand = false;
            bool allKnown = true;
            for (auto cursor = begin; cursor < end; ++cursor) {
                const auto& part = tokens[cursor];
                // The shared lexer may split a VHDL character literal into
                // apostrophe plus a one-character token ending in apostrophe.
                if (part.text == "'" && cursor + 1 < end &&
                    tokens[cursor + 1].offset == part.offset + 1 &&
                    tokens[cursor + 1].text.size() == 2 &&
                    tokens[cursor + 1].text.back() == '\'') {
                    hasOperand = true;
                    ++cursor;
                    continue;
                }
                if (part.kind == parser::HdlTokenKind::Number ||
                    part.kind == parser::HdlTokenKind::String) {
                    hasOperand = true;
                } else if (part.kind == parser::HdlTokenKind::Identifier) {
                    const auto name = lower(part.text);
                    if (name == "and" || name == "or" || name == "xor" ||
                        name == "xnor" || name == "nand" || name == "nor" ||
                        name == "not" || name == "mod" || name == "rem" ||
                        name == "abs" || name == "sll" || name == "srl" ||
                        name == "sla" || name == "sra" || name == "rol" ||
                        name == "ror") continue;
                    if (!architectureConstants.contains(name)) { allKnown = false; break; }
                    hasOperand = true;
                } else if (part.kind == parser::HdlTokenKind::Symbol) {
                    if (part.text != "(" && part.text != ")" && part.text != "+" &&
                        part.text != "-" && part.text != "*" && part.text != "/" &&
                        part.text != "**" && part.text != "&" && part.text != "=" &&
                        part.text != "/=" && part.text != "<" && part.text != ">" &&
                        part.text != "<=" && part.text != ">=") {
                        allKnown = false;
                        break;
                    }
                }
            }
            if (hasOperand && allKnown)
                emit(result, "GJB9765.4.8.a", source,
                     tokens[begin].line, tokens[begin].column,
                     "case 选择表达式仅由已知常量组成");
        }
        std::vector<CaseAlignmentGroup> caseAlignment;
        for (std::size_t index = 0; index < tokens.size(); ++index) {
            if (tokens[index].kind != parser::HdlTokenKind::Identifier) continue;
            const auto word = lower(tokens[index].text);
            if (word == "end" && index + 1 < tokens.size() &&
                lower(tokens[index + 1].text) == "case") {
                if (!caseAlignment.empty()) caseAlignment.pop_back();
                ++index;
                continue;
            }
            if (word == "case" && startsCaseStatement(index)) {
                caseAlignment.push_back({&tokens[index], std::nullopt});
                continue;
            }
            if (word != "when" || caseAlignment.empty() || !startsCaseAlternative(index)) continue;
            auto& group = caseAlignment.back();
            const auto currentColumn = tokenColumn(tokens[index]);
            const auto caseColumn = tokenColumn(*group.opening);
            if (currentColumn <= caseColumn)
                emit(result, "GJB9765.4.14.b.3", source,
                     tokens[index].line, tokens[index].column,
                     "case 分支 when 未相对 case 增加缩进");
            if (!group.firstWhenColumn) group.firstWhenColumn = currentColumn;
            else if (currentColumn != *group.firstWhenColumn)
                emit(result, "GJB9765.4.14.b.3", source,
                     tokens[index].line, tokens[index].column,
                     "同一 case 语句的 when 未与首个分支左端对齐");
        }

        // Native 4.14.b.4 builds an if/elsif/else branch tree and recursively
        // links the first nested IF found below each branch. It locates the
        // branch keyword and nested "if" in comment-stripped source, expands
        // tabs to four-column stops, and warns when the child column is less
        // than or equal to its parent branch column. Exact one-tab depth is a
        // separate style rule and is deliberately not inferred here.
        // ev_37e19937fe90d98240c2159037ca786ce936b9e7211afb729ee6b0873675f7d8
        // ev_d7422141e14ebceda8bcf2a88688e89dbcdea92477af9ab085c8902f049d508c
        // ev_4bcab576fca1868e7a0d2e63b68be85dcb6153133d132da8a59c89be50b57e0d
        // ev_016bfbc44bffe9b8849d844cb85929b760af4d72d169ba3b57927984afafb671
        struct IfBranch {
            const parser::HdlToken* keyword;
            bool firstNestedIfLinked{false};
        };
        std::vector<IfBranch> ifBranches;
        for (std::size_t index = 0; index < tokens.size(); ++index) {
            if (tokens[index].kind != parser::HdlTokenKind::Identifier) continue;
            const auto word = lower(tokens[index].text);
            if (word == "end" && index + 1 < tokens.size() &&
                lower(tokens[index + 1].text) == "if") {
                if (!ifBranches.empty()) ifBranches.pop_back();
                ++index;
                continue;
            }
            if (word == "elsif") {
                if (!ifBranches.empty()) {
                    ifBranches.back().keyword = &tokens[index];
                    ifBranches.back().firstNestedIfLinked = false;
                }
                continue;
            }
            if (word == "else") {
                const bool branchKeyword = index > 0 &&
                    (tokens[index - 1].text == ";" || lower(tokens[index - 1].text) == "then");
                if (branchKeyword && !ifBranches.empty()) {
                    ifBranches.back().keyword = &tokens[index];
                    ifBranches.back().firstNestedIfLinked = false;
                }
                continue;
            }
            if (word != "if" || !startsIfStatement(index)) continue;
            if (!ifBranches.empty() && !ifBranches.back().firstNestedIfLinked) {
                ifBranches.back().firstNestedIfLinked = true;
                const auto parentColumn = tokenColumn(*ifBranches.back().keyword);
                const auto childColumn = tokenColumn(tokens[index]);
                if (childColumn <= parentColumn)
                    emit(result, "GJB9765.4.14.b.4", source,
                         tokens[index].line, tokens[index].column,
                         "嵌套 if 未相对父分支增加缩进");
            }
            ifBranches.push_back({&tokens[index]});
        }
        // Native 4.14.b.5 recursively creates one group for each AST 0xa4
        // node, records the first child and later-line children except type
        // 0x53, then finds each recorded name on its comment-stripped source
        // line. Tabs advance to four-column stops and every item is compared
        // with the preceding display column. The exact proprietary AST enum
        // names remain unknown; port clauses and consecutive signal/variable
        // declaration runs are the source constructs established by the rule
        // example and conservatively representable by our lexer.
        // ev_acc99eee4c201699c1c866e866290ba2436a1fdbdbcf3c139871051839e08dc9
        // ev_d59ff35799c0a91827200d23d8202f7645b633a56c140dacecc10166c29c5f39
        const auto reportDeclarationAlignment = [&](const std::vector<const parser::HdlToken*>& names) {
            if (names.size() < 2) return;
            std::vector<const parser::HdlToken*> laterLines{names.front()};
            for (std::size_t index = 1; index < names.size(); ++index)
                if (names[index]->line > names.front()->line) laterLines.push_back(names[index]);
            if (laterLines.size() < 2) return;
            auto displayColumn = [&](const parser::HdlToken& token) {
                return token.line > 0 && token.line <= lines.size()
                    ? tab4DisplayColumn(lines[token.line - 1], token.column)
                    : token.column;
            };
            auto previousColumn = displayColumn(*laterLines.front());
            for (std::size_t index = 1; index < laterLines.size(); ++index) {
                const auto currentColumn = displayColumn(*laterLines[index]);
                if (currentColumn != previousColumn)
                    emit(result, "GJB9765.4.14.b.5", source,
                         laterLines[index]->line, laterLines[index]->column,
                         "声明名称未与前一项对齐：" + laterLines[index]->text);
                previousColumn = currentColumn;
            }
        };
        const auto declarationName = [&](std::size_t begin, std::size_t end)
            -> const parser::HdlToken* {
            static constexpr std::array<std::string_view, 4> InterfaceClasses{
                "signal", "constant", "variable", "file"};
            int depth = 0;
            for (auto index = begin; index < end; ++index) {
                if (tokens[index].text == "(") { ++depth; continue; }
                if (tokens[index].text == ")") { --depth; continue; }
                if (depth != 0) continue;
                if (tokens[index].text == ":") break;
                if (tokens[index].kind != parser::HdlTokenKind::Identifier) continue;
                const auto word = lower(tokens[index].text);
                if (std::ranges::find(InterfaceClasses, word) != InterfaceClasses.end()) continue;
                return &tokens[index];
            }
            return nullptr;
        };
        std::vector<std::pair<std::size_t, std::size_t>> portRanges;
        for (std::size_t port = 0; port + 1 < tokens.size(); ++port) {
            if (tokens[port].kind != parser::HdlTokenKind::Identifier ||
                lower(tokens[port].text) != "port" || tokens[port + 1].text != "(") continue;
            const auto close = closeParen(tokens, port + 1);
            if (close == tokens.size()) continue;
            portRanges.emplace_back(port, close);
            std::vector<const parser::HdlToken*> names;
            auto clauseBegin = port + 2;
            int depth = 0;
            for (auto index = clauseBegin; index <= close; ++index) {
                if (index < close && tokens[index].text == "(") { ++depth; continue; }
                if (index < close && tokens[index].text == ")") { --depth; continue; }
                if (index < close && (depth != 0 || tokens[index].text != ";")) continue;
                if (const auto* name = declarationName(clauseBegin, index)) names.push_back(name);
                clauseBegin = index + 1;
            }
            reportDeclarationAlignment(names);
            port = close;
        }
        const auto insidePort = [&](std::size_t index) {
            return std::ranges::any_of(portRanges, [&](const auto& range) {
                return index >= range.first && index <= range.second;
            });
        };
        for (std::size_t index = 0; index < tokens.size();) {
            if (insidePort(index) || tokens[index].kind != parser::HdlTokenKind::Identifier) {
                ++index;
                continue;
            }
            auto word = lower(tokens[index].text);
            auto keyword = index;
            if (word == "shared" && index + 1 < tokens.size() &&
                lower(tokens[index + 1].text) == "variable") {
                keyword = ++index;
                word = "variable";
            }
            if (word != "signal" && word != "variable") { ++index; continue; }
            std::vector<const parser::HdlToken*> names;
            while (keyword < tokens.size() && !insidePort(keyword)) {
                auto end = keyword + 1;
                while (end < tokens.size() && tokens[end].text != ";") ++end;
                if (end == tokens.size()) { index = tokens.size(); break; }
                if (const auto* name = declarationName(keyword + 1, end)) names.push_back(name);
                auto next = end + 1;
                if (next < tokens.size() && lower(tokens[next].text) == "shared" &&
                    next + 1 < tokens.size() && lower(tokens[next + 1].text) == word) ++next;
                if (next >= tokens.size() || lower(tokens[next].text) != word || insidePort(next)) {
                    index = end + 1;
                    break;
                }
                keyword = next;
                index = next;
            }
            reportDeclarationAlignment(names);
        }
        // Native 4.14.c.1 scans raw fgets chunks until the first literal --
        // and retains one mutable flag per chunk. The exact admitted-byte
        // bitmaps are Tab/LF/space/quotes/'('/digit after '(' and
        // Tab/LF/space/quotes/')'/digit before ')'. A case-insensitive
        // "(others" clears the flag, including a violation seen earlier.
        // ev_605bc62247ae26de1fd44533e8f014675f07dbb94c4994938c014d6bfa0bb603
        const auto cChunks = nativeVhdlTextChunks(source.content());
        for (std::size_t chunkIndex = 0; chunkIndex < cChunks.size(); ++chunkIndex) {
            const auto& chunk = cChunks[chunkIndex];
            const auto comment = chunk.find("--");
            const auto limit = comment == std::string::npos ? chunk.size() : comment;
            bool flagged = false;
            std::size_t flagColumn = 1;
            const auto admittedCommon = [](char value) {
                return value == '\t' || value == '\n' || value == ' ' ||
                       value == '\'' || value == '"' ||
                       (value >= '0' && value <= '9');
            };
            for (std::size_t position = 0; position < limit; ++position) {
                if (chunk[position] == '(' && position + 1 < chunk.size()) {
                    const auto adjacent = chunk[position + 1];
                    const bool admitted = admittedCommon(adjacent) || adjacent == '(';
                    if (!admitted) {
                        if (position + 7 <= chunk.size() &&
                            lower(chunk.substr(position + 1, 6)) == "others") {
                            flagged = false;
                            flagColumn = 1;
                        } else {
                            flagged = true;
                            flagColumn = position + 1;
                        }
                    }
                }
                if (chunk[position] == ')' && position > 0) {
                    const auto adjacent = chunk[position - 1];
                    if (!admittedCommon(adjacent) && adjacent != ')') {
                        flagged = true;
                        flagColumn = position + 1;
                    }
                }
            }
            if (flagged)
                emit(result, "GJB9765.4.14.c.1", source, chunkIndex + 1, flagColumn,
                     "括号内侧缺少规则允许的分隔字符");
        }
        // Native 4.14.c.2 scans raw fgets chunks up to the first literal --
        // and reports every <= or := whose immediate left or right byte is
        // neither ASCII space nor Tab. It does not parse quoted strings.
        // ev_2b0d5e0146b2eec171471cfa7f8b03c888c826162d2b552cd597ee4fe2ab7016
        // ev_a6ad172e51fa76ab9c38d541c28cd4625d828a98c026fa3688a8fff928516997
        for (std::size_t chunkIndex = 0; chunkIndex < cChunks.size(); ++chunkIndex) {
            const auto& chunk = cChunks[chunkIndex];
            const auto comment = chunk.find("--");
            const auto limit = comment == std::string::npos ? chunk.size() : comment;
            for (std::size_t position = 0; position < limit; ++position) {
                if (position + 2 >= chunk.size()) continue;
                const std::string_view op{chunk.data() + position, 2};
                if (op != "<=" && op != ":=") continue;
                const auto separated = [](char value) {
                    return value == ' ' || value == '\t';
                };
                const bool leftSeparated = position > 0 && separated(chunk[position - 1]);
                const bool rightSeparated = separated(chunk[position + 2]);
                if (!leftSeparated || !rightSeparated)
                    emit(result, "GJB9765.4.14.c.2", source,
                         chunkIndex + 1, position + 1,
                         "赋值符号 " + std::string(op) + " 两侧应保留空格或 Tab");
                ++position;
            }
        }
        // Native 4.14.c.3 scans raw fgets chunks up to the first literal --.
        // It covers /= and single =, +, -, *, /, but excludes <=, :=, =>,
        // ** and the anomalous /=> sequence. Only ASCII space or Tab counts
        // as separation; quoted text is not treated specially.
        // ev_4714c6cdd73e074b9417da0ba3b318724c7493e865252ce39215b2c0ef88d199
        for (std::size_t chunkIndex = 0; chunkIndex < cChunks.size(); ++chunkIndex) {
            const auto& chunk = cChunks[chunkIndex];
            const auto comment = chunk.find("--");
            const auto limit = comment == std::string::npos ? chunk.size() : comment;
            for (std::size_t position = 0; position < limit; ++position) {
                const auto current = chunk[position];
                const auto previous = position > 0 ? chunk[position - 1] : '\0';
                const auto hasNext = position + 1 < chunk.size();
                const auto next = hasNext ? chunk[position + 1] : '\0';
                std::size_t width = 0;
                if (current == '/' && next == '=') {
                    // The native /= branch rejects a following > before its
                    // spacing predicate, even though /=> is not valid VHDL.
                    if (position + 2 < chunk.size() && chunk[position + 2] != '>') width = 2;
                } else if (hasNext && (current == '+' || current == '-')) {
                    width = 1;
                } else if (hasNext && current == '*') {
                    if (previous != '*' && next != '*') width = 1;
                } else if (hasNext && current == '/') {
                    if (next != '=') width = 1;
                } else if (hasNext && current == '=') {
                    if (previous != '/' && previous != ':' && previous != '<' && next != '>')
                        width = 1;
                }
                if (width == 0) continue;
                const auto separated = [](char value) {
                    return value == ' ' || value == '\t';
                };
                const bool leftSeparated = position > 0 && separated(previous);
                const bool rightSeparated = position + width < chunk.size() &&
                    separated(chunk[position + width]);
                if (!leftSeparated || !rightSeparated) {
                    const auto op = chunk.substr(position, width);
                    emit(result, "GJB9765.4.14.c.3", source,
                         chunkIndex + 1, position + 1,
                         "运算符 " + op + " 两侧应保留空格或 Tab");
                }
                if (width == 2) ++position;
            }
        }
        // Native collector recursively selects case-folded file_open calls
        // (AST 0x92) and their direct string-literal children (AST 0xb2).
        // The run reports only a literal containing ":/" or ":\\".
        // ev_647f0cf8f8988d7a1509761e855ab44dc954cdf83ab996266e37f9484aa7916b
        // ev_dc2d8ad1268eba2174e70d1886a991d63c82f4428732222e895fd8be082b0534
        for (std::size_t i = 0; i + 1 < tokens.size(); ++i) {
            if (tokens[i].kind != parser::HdlTokenKind::Identifier ||
                lower(tokens[i].text) != "file_open" || tokens[i + 1].text != "(") continue;
            const auto close = closeParen(tokens, i + 1);
            if (close == tokens.size()) continue;
            int depth = 1;
            for (auto j = i + 2; j < close; ++j) {
                if (tokens[j].text == "(") { ++depth; continue; }
                if (tokens[j].text == ")") { --depth; continue; }
                if (depth != 1 || tokens[j].kind != parser::HdlTokenKind::String) continue;
                const auto& literal = tokens[j].text;
                if (literal.find(":/") == std::string::npos &&
                    literal.find(":\\") == std::string::npos) continue;
                emit(result, "GJB9765.4.14.d.4", source,
                     tokens[j].line, tokens[j].column,
                     "FILE_OPEN 使用了含盘符或协议前缀的路径字面量：" + literal);
            }
        }
        const auto rawTokens = parser::tokenize(source);
        const auto control = scanVhdlClockControl(source, rawTokens);
        for (const auto& overlap : control.overlaps) {
            const auto& token = rawTokens[overlap.clockToken];
            emit(result, "GJB9765.4.10.h", source, token.line, token.column,
                 "时钟信号同时作为已识别时序进程的条件控制信号：" + overlap.name);
        }
        for (const auto& model : control.models) {
            if (!model.clockDrivesOnlyConstants) continue;
            const auto duplicate = std::ranges::any_of(control.overlaps,
                [&](const auto& overlap) {
                    return overlap.architectureToken == model.architectureToken &&
                        overlap.clockToken == model.edge.clockToken;
                });
            if (duplicate) continue;
            const auto& token = rawTokens[model.edge.clockToken];
            emit(result, "GJB9765.4.10.h", source, token.line, token.column,
                 "时钟沿进程仅执行常量复位或置位赋值：" + model.edge.clockName);
        }
        const auto fsmExtraction = extractBoundedVhdlFsmModels(source, rawTokens);
        for (const auto& model : fsmExtraction.models) {
            if (!model.clockBindingComplete) continue;
            const auto& token = rawTokens[model.caseToken];
            if (model.sourceStates.size() > 40)
                emit(result, "GJB9765.4.11.f.a", source, token.line, token.column,
                     "已识别状态机的显式转换源状态超过 40 个：" + model.control);
            if (!model.encodingResolved) continue;
            const auto& encodingToken = rawTokens[model.encodingExplicit ? model.encodingToken : model.caseToken];
            // Native 4.11.f.b and 4.11.f.c are independently enabled rules in
            // the shipped GJB9765 profiles. Their recovered entries compare
            // the recognized encoding directly; neither reads the product's
            // later-added area/speed preference. Preserve those two native
            // predicates without inventing an external run gate.
            if (model.encoding != 0)
                emit(result, "GJB9765.4.11.f.b", source, encodingToken.line, encodingToken.column,
                     "已识别状态机未使用顺序编码：" + model.control);
            if (model.encoding != 1)
                emit(result, "GJB9765.4.11.f.c", source, encodingToken.line, encodingToken.column,
                     "已识别状态机未使用独热编码：" + model.control);
        }
        for (const auto& candidate : fsmExtraction.oversizedStateCandidates) {
            const auto& token = rawTokens[candidate.caseToken];
            emit(result, "GJB9765.4.11.f.a", source, token.line, token.column,
                 "已识别状态机的显式转换源状态超过 40 个：" + candidate.control);
        }
    }
    if (!vhdl) {
        if (preprocessed.issues.empty()) {
            for (auto target : uninitializedPowerOnRegisters(activeTokens, options.powerOnPolicy)) {
                const auto& token = activeTokens[target];
                // FUN_1805596f0 composes DAT_180ef14e8 + normalized target +
                // DAT_180ef14d0, yielding the exact GB18030 diagnostic below.
                // Its report stores lowercase "error", the assignment-target
                // physical line and no native column.
                emit(result, "GJB10157.6.2.5", source, token.line, 1,
                     "必须对寄存器" + token.text + "进行上电初始化,赋固定值");
            }
            for (const auto& feedback : VerilogGateFeedback(activeTokens).findings(false)) {
                const auto& token = activeTokens[feedback.token];
                // 7.2.1 slot 0 is a thunk to FUN_180666520, which reports
                // every admitted combinational SCC. Reuse an existing semantic
                // cycle diagnostic when it already represents this component;
                // primitive gates need this netlist-oriented source adapter.
                const bool alreadyReported = std::ranges::any_of(result.violations,
                    [&](const auto& violation) {
                        return violation.ruleId == "RTL-COMBINATIONAL-LOOP" &&
                            violation.file == source.path() &&
                            violation.module == feedback.module && violation.signal &&
                            std::ranges::find(feedback.signals, *violation.signal) != feedback.signals.end();
                    });
                if (!alreadyReported)
                    emit(result, "GJB10157.7.2.1", source, token.line, token.column,
                         "检测到基本门组合回路：" + commaSeparated(feedback.signals));
                // R-13-7 builds the generic elaborated module connectivity
                // model through FUN_1806e04f0 and reports every returned SCC;
                // it is not limited to source-level assign/always edges.  The
                // semantic analyzer already maps source-proved cycles below,
                // while this bounded netlist adapter supplies primitive-only
                // SCCs.  Do not duplicate a component already represented by
                // RTL-COMBINATIONAL-LOOP.
                if (!alreadyReported)
                    emit(result, "GJB10157.9.4.7", source, token.line, token.column,
                         "检测到组合回路" + commaSeparated(feedback.signals));
                if (feedback.countedGateCells <= 2)
                    emit(result, "GJB10157.7.2.2", source, token.line, token.column,
                         "发现使用基本逻辑门描述的反馈锁存结构：" + commaSeparated(feedback.signals));
            }
            checkVerilogFullCaseAttribute(result, preprocessed.source, activeTokens);
            checkNativeVerilogForbiddenTypes(result, preprocessed.source, activeTokens);
            // Native FUN_1805306e0 consumes AST_PARAMETER/LOCALPARAM/WIRE
            // after frontend conditional selection. Apply the recovered case
            // predicate to the configured active token stream, not raw source
            // containing mutually exclusive branches. Unresolved preprocessing
            // deliberately produces no guessed 5.2.15 finding.
            // FUN_18003e420 builds the native message as normalized name plus
            // one fixed GB18030 suffix. The native location record carries the
            // owning module source path and declaration line, but no column.
            // a2_5_naming_case_entry.txt SHA256
            // 13a1afd1f7ffad5be0c2a08657d7e67a149cc0ddef9b861841aa76deafb598f4
            for (const auto& declaration :
                 VerilogNamingDeclarations(preprocessed.source, activeTokens).collect()) {
                if (checkVerilogNamingStyle(
                        declaration.normalizedName, declaration.subject) != true) continue;
                const auto& token = activeTokens[declaration.tokenIndex];
                emit(result, "GJB10157.5.2.15", source, token.line, 1,
                     declaration.subject == VerilogNamingSubject::WireLike
                         ? declaration.normalizedName +
                               ":建议变量的命名中字母全部采用小写或者仅首字母大写"
                         : declaration.normalizedName +
                               ":建议常量的命名中字母全部采用大写");
            }
            // Native 5.2.6 compares parsed declaration names within each
            // AST_MODULE. Use the same configured active branch boundary as
            // the frontend-facing naming checks; mutually exclusive source
            // declarations must never occupy the same comparison set.
            const auto activeNamingShape =
                scanVerilogNamingShape(preprocessed.source, activeTokens);
            caseOnlyDeclarationChecks(
                result, source, activeTokens, activeNamingShape);
            std::map<std::string, std::size_t> activeModuleMultiplicity;
            for (const auto& declaration : activeNamingShape.declarations)
                if (declaration.subject == VerilogNamingShapeSubject::Module)
                    ++activeModuleMultiplicity[declaration.moduleName];
            // Native 5.2.1/.2/.4/.12 consume the same frontend AST declaration
            // families as the adjacent naming rules. Evaluate the configured
            // active branch rather than dropping every declaration enclosed by
            // an ifdef/ifndef in the raw source adapter.
            for (const auto& declaration : activeNamingShape.declarations) {
                const auto& token = activeTokens[declaration.tokenIndex];
                const auto owner = activeModuleMultiplicity.find(declaration.moduleName);
                const bool uniqueModule = owner != activeModuleMultiplicity.end() &&
                    owner->second == 1;
                // Native FUN_18052c420 iterates the module-name map and admits
                // only a group containing exactly one AST_MODULE(2). It emits
                // Error once per selected declaration node, at the declaration
                // physical line with no column, using normalized-name plus the
                // exact GB18030 suffix 命名必须以字母开始.
                if (uniqueModule && declaration.predicates.firstByteViolation == true)
                    emit(result, "GJB10157.5.2.1", source, token.line, 1,
                         declaration.normalizedName + "命名必须以字母开始");
                // Native FUN_180535370 uses the same unique-module-map gate as
                // the adjacent naming collectors, but excludes FUNCTION(4)
                // itself.  It tests "__" before the trailing-byte predicate,
                // so a name satisfying both branches emits only the first
                // fixed message.  The native record carries no column.
                if (uniqueModule && declaration.predicates.underscoreViolation == true) {
                    const bool consecutive =
                        declaration.normalizedName.find("__") != std::string::npos;
                    emit(result, "GJB10157.5.2.2", source, token.line, 1,
                         declaration.normalizedName +
                             (consecutive
                                  ? ":禁止命名中使用连续的\"__\""
                                  : ":禁止命名末尾使用\"_\""));
                }
                // Native FUN_180539910 admits only a module-name map group
                // containing exactly one AST_MODULE.  It lowercases both the
                // five fixed names and a candidate copy, performs equal-length
                // byte comparison, and reports every selected node.  The
                // message retains the original normalized name; the report has
                // a candidate integer location but no column.
                if (uniqueModule && checkVerilogPowerName(
                        declaration.normalizedName, declaration.subject) == true)
                    emit(result, "GJB10157.5.2.4", source, token.line, 1,
                         "禁止使用" + declaration.normalizedName + "命名");
                // FUN_18052d8a0 iterates the module-name map and admits a
                // group only when it contains exactly one AST_MODULE.  Every
                // selected descendant of duplicate same-name modules is
                // therefore skipped together with both module declarations.
                if (uniqueModule &&
                    declaration.subject != VerilogNamingShapeSubject::Function &&
                    checkVerilogNameLength(declaration.normalizedName) == true)
                    emit(result, "GJB10157.5.2.12", source, token.line, 1,
                         declaration.normalizedName + ":建议命名长度在2到32个字符之间");
            }
            const auto& keywordPolicy = options.verilogKeywordPolicy.configurationKnown
                ? options.verilogKeywordPolicy : Gjb10157DocumentedKeywordPolicy;
            const bool keywordSourceAdmitted =
                parser::checkSyntax(preprocessed.source).empty();
            if (keywordSourceAdmitted) {
                for (const auto& declaration : activeNamingShape.declarations) {
                    // FUN_180535e60 iterates the ordered module-name map and
                    // admits a group only when its vector contains exactly one
                    // AST_MODULE. A duplicate group does not suppress a
                    // different uniquely bound module in the same source.
                    const auto ownerCount = std::ranges::count_if(
                        result.structure, [&](const auto& unit) {
                            return unit.kind == parser::DesignUnitKind::VerilogModule &&
                                   unit.name == declaration.moduleName;
                        });
                    if (ownerCount != 1) continue;
                    const auto match = checkVerilogKeywordName(
                        declaration.normalizedName, declaration.subject, keywordPolicy);
                    if (match.status != VerilogKeywordStatus::Matched) continue;
                    const auto& token = activeTokens[declaration.tokenIndex];
                    emit(result, "GJB10157.5.2.3", source, token.line, 1,
                         nativeVerilogKeywordMessage(
                             *match.category, declaration.normalizedName));
                }
            }
            checkTopLevelVerilogPortTypes(result, source, activeTokens, options);
            checkTopLevelVerilogGlueLogic(result, source, activeTokens, options);
            checkNonTopVerilogHierarchyLogic(result, source, activeTokens, options);
            checkRegisteredVerilogModuleOutputs(result, source, activeTokens);
            checkRegisterOutputUsedAsClock(result, source);
            checkSequentiallyGeneratedAsyncReset(result, source, activeTokens);
            checkFlipFlopAsyncResetCoverage(result, source, activeTokens);
            checkResetConditionEdgePolarity(result, source, activeTokens);
            checkLatchMixedWithCombinationalLogic(result, source, activeTokens);
            checkCombinationalTristateEnable(result, source, activeTokens);
            checkInternalVerilogTristate(result, source, activeTokens);
            checkAsyncResetConnectedToNonResetUse(result, source, activeTokens);
            checkSynchronousUseOfAsynchronousReset(result, source, activeTokens);
            checkClockUsedAsData(result, source, activeTokens);
            checkCombinationalOutputUsedAsClock(result, source, activeTokens);
            checkCombinationalOutputUsedAsAsyncReset(result, source, activeTokens);
            checkMixedClockEdges(result, source, activeTokens);
            checkGatedClocks(result, source, activeTokens);
            checkMultipleClockDomains(result, source, activeTokens, options);
            checkMixedAsynchronousSetReset(result, source, activeTokens);
            // Native GJB10157.10.3/.10.4 share the recovered FSM relation
            // model.  FUN_18050d6e0 reports .10.3 only when its synthetic
            // `init` relation is absent; FUN_1805138b0 reports .10.4 only for
            // `0x28 < set.size()`.  Keep both adapters bounded to direct
            // state/next-state updates and source-proved initialization.
            for (const auto& candidate : VerilogFsmStateCount(activeTokens).candidates()) {
                const auto& token = activeTokens[candidate.caseToken];
                // Native R-14-2 builds a map keyed by case labels, collects
                // simple RHS constants/identifiers, removes values that still
                // name an assigned intermediate, and reports once only when
                // the exact existing key `default` has an empty filtered set
                // (FUN_180506fa0/FUN_18050af80/FUN_18050aa20).
                if (candidate.hasDefaultBranch && candidate.defaultResetSetComplete &&
                    candidate.defaultFilteredResetSetEmpty) {
                    const auto& defaultToken = activeTokens[candidate.defaultToken];
                    emit(result, "GJB10157.10.2", source,
                         defaultToken.line, defaultToken.column,
                         "状态机信号" + candidate.control +
                             "在default状态时，没有把状态机中所有寄存器重置为确定状态.");
                }
                // GJB10157.12.2 is a modified Yosys FSM-detection pass in the
                // shipped DLL. FUN_180524530 excludes fsm_encoding="none" and
                // one-bit wires, then FUN_180522090 accepts an fsm_encoding
                // value containing "safe" or one of two exact synthesis-comment
                // forms (compiled with the native case-insensitive flag) before
                // emitting Warning "状态机未设置为safe模式". Keep the source
                // adapter to a unique, width-proved declaration and the same
                // bounded FSM model used by the recovered 10.x rules.
                if (const auto declaration = boundedVerilogFsmDeclaration(activeTokens, candidate);
                    declaration && declaration->widthGreaterThanOne) {
                    const auto directive = nativeVerilogFsmSafeDirective(
                        preprocessed.source, activeTokens[declaration->token]);
                    if (directive == VerilogFsmSafeDirective::Missing) {
                        const auto& declarationToken = activeTokens[declaration->token];
                        emit(result, "GJB10157.12.2", source,
                             declarationToken.line, declarationToken.column,
                             "状态机未设置为safe模式");
                    }
                }
                if (token.text == "case" && candidate.transitionGraphComplete) {
                    for (const auto& [state, destinations] : candidate.transitions) {
                        if (destinations.size() == 1 && destinations.contains(state))
                            emit(result, "GJB10157.10.1", source, token.line, token.column,
                                 "禁止使用死循环状态机，" + candidate.control +
                                     "状态" + state);
                    }
                    // FUN_1805000b0 looks up the exact synthetic relation key
                    // `init`, walks the reachable-state relation, and reports
                    // every graph node absent from that traversal.  Mirror the
                    // bounded source counterpart only when the initial node and
                    // every transition are simple, explicit and resolved.
                    if (!candidate.initialState.empty()) {
                        std::set<std::string> reachable{candidate.initialState};
                        std::vector<std::string> pending{candidate.initialState};
                        while (!pending.empty()) {
                            const auto current = std::move(pending.back());
                            pending.pop_back();
                            const auto outgoing = candidate.transitions.find(current);
                            if (outgoing == candidate.transitions.end()) continue;
                            for (const auto& destination : outgoing->second)
                                if (reachable.insert(destination).second)
                                    pending.push_back(destination);
                        }
                        for (const auto& state : candidate.sourceStates) {
                            if (!reachable.contains(state))
                                emit(result, "GJB10157.10.1", source,
                                     token.line, token.column,
                                     "状态机变量" + candidate.control +
                                         "状态" + state + "不可达");
                        }
                    }
                }
                if (!candidate.initialized)
                    emit(result, "GJB10157.10.3", source, token.line, token.column,
                         "状态机变量" + candidate.control + "未设置初始状态");
                if (candidate.sourceStates.size() > 40)
                    emit(result, "GJB10157.10.4", source, token.line, token.column,
                         "状态机状态数不应超过40个,当前" +
                             std::to_string(candidate.sourceStates.size()) + "个");
            }
            // GJB10157.5.1.7 constructor FUN_180020f40 installs vtable
            // 0x180eeb778, whose active slot 0 is FUN_18052ae80. It compares
            // every later pair of same-file port records and reports the later
            // record when their physical line fields are equal. Preserve that
            // pair multiplicity: N ports on one line produce N*(N-1)/2 alarms.
            // ev_43873e8a6dc54857da21d8dd2be2dfd83486000a8270327af75ec80b312d790e
            std::map<std::size_t, std::size_t> portLineCounts;
            for (const auto& unit : parser::parseDesignUnits(preprocessed.source)) {
                if (unit.kind != parser::DesignUnitKind::VerilogModule) continue;
                for (const auto& port : unit.ports) {
                    if (port.direction == parser::PortDirection::Unknown ||
                        port.location.line == 0) continue;
                    const auto earlierPorts = portLineCounts[port.location.line]++;
                    for (std::size_t pair = 0; pair < earlierPorts; ++pair)
                        emit(result, "GJB10157.5.1.7", source,
                             port.location.line, port.location.column,
                             "一行中存在多个端口声明");
                }
            }
            checkAlwaysIfElseNative(result, source, activeTokens);
            checkDefinitelyEmptyVerilogFor(result, source, activeTokens);
            checkForBodyRuntimeArithmetic(result, source, activeTokens);
            checkForHeaderConsistency(result, source, activeTokens);
            checkResetBranchInsideFor(result, source, activeTokens);
            for (const auto index : VerilogMultipleReset(activeTokens).violatingAlwaysIndices())
                emit(result, "GJB10157.6.2.10", source, activeTokens[index].line,
                     activeTokens[index].column,
                     "同一 always 过程的事件列表含多个显式复位/置位条件，应只使用一个复位信号");
            for (const auto index : VerilogSizedLiteral(activeTokens).violatingLiteralIndices())
                emit(result, "GJB10157.8.9", source, activeTokens[index].line,
                     activeTokens[index].column,
                     "数值常量缺少显式正位宽或 b/o/d/h 进制标记");
            for (const auto index : VerilogSizedLiteral(activeTokens).overflowingLiteralIndices())
                emit(result, "GJB10157.8.6", source, activeTokens[index].line,
                     activeTokens[index].column,
                     "显式位宽小于字面量所需位数（二进制前导零及未知位计入位数）");
            // These native predicates consume AST/module/process/function
            // structures. Run their bounded adapters only on the configured
            // active branch, so an inactive directive arm cannot create a
            // finding and an active arm is not discarded wholesale.
            for (const auto index : detail::VerilogOverrideAssignments(activeTokens).find())
                emit(result, "GJB10157.9.1.5", source,
                     activeTokens[index].line, activeTokens[index].column,
                     "同一控制路径中对相同信号位重复赋值，后一次赋值覆盖前一次");
            // Native GJB10157.9.2.5 selects every AST_ALWAYS, including the
            // always_ff frontend form, checks its immediate AST_POSEDGE /
            // AST_NEGEDGE / AST_EDGE(0x65..0x67) children, and recursively
            // searches each event for AST_BIT_OR / AST_LOGIC_OR(0x26/0x46).
            // It reports once per always node, not once per operator.
            // ev_165fc19c9ef8d627b96bd5d6a92697bc84caaaae45cfcbaa1c0f8d3e2d47a00e
            // ev_2a16e02acb25139a18a51d208245698c12f17a782f0668b01b45b260614ed9bc
            for (std::size_t i = 0; i + 2 < activeTokens.size(); ++i) {
                if ((activeTokens[i].text != "always" &&
                     activeTokens[i].text != "always_ff") ||
                    activeTokens[i + 1].text != "@" ||
                    activeTokens[i + 2].text != "(") continue;
                const auto close = closeParen(activeTokens, i + 2);
                if (close == activeTokens.size()) continue;
                const bool hasOrOperator = std::ranges::any_of(
                    activeTokens.begin() + static_cast<std::ptrdiff_t>(i + 3),
                    activeTokens.begin() + static_cast<std::ptrdiff_t>(close),
                    [](const auto& token) { return token.text == "|" || token.text == "||"; });
                if (hasOrOperator)
                    emit(result, "GJB10157.9.2.5", source,
                         activeTokens[i].line, activeTokens[i].column,
                         "显式敏感列表使用了 | 或 || 连接事件，应使用 or 或逗号分隔");
                i = close;
            }
            const VerilogConstantExpressions activeConstants(activeTokens);
            for (const auto index :
                 VerilogCaseLabels(activeTokens, activeConstants).runtimeLabelIndices())
                emit(result, "GJB10157.9.2.6", source,
                     activeTokens[index].line, activeTokens[index].column,
                     "case 分支标签包含运行时变量，必须使用常量选择值");
            for (const auto index : verilogRepeatRuntimeDependencies(
                     preprocessed.source, activeTokens, activeConstants))
                emit(result, "GJB10157.9.3.6", source,
                     activeTokens[index].line, activeTokens[index].column,
                     "避免在repeat语句表达式中使用变量.");
            const auto recursion = detail::functionRecursion(
                preprocessed.source, activeTokens);
            // Native R-12-5 stores one finding per distinct cycle-member set,
            // locates it at the first function in the discovered path, and
            // formats the complete module/function path. Its internal text
            // says Warning, but the official mandatory profile remains Error.
            for (const auto& cycle : recursion.cycles) {
                if (cycle.functions.empty()) continue;
                const auto function = cycle.functions.front();
                const auto index = recursion.functions[function].nameToken;
                std::string message = "禁止调用递归函数.函数";
                for (std::size_t i = 0; i < cycle.functions.size(); ++i) {
                    const auto& item = recursion.functions[cycle.functions[i]];
                    auto functionName = item.name;
                    if (!functionName.empty() && functionName.front() == '\\')
                        functionName.erase(functionName.begin());
                    if (i != 0) message += "、";
                    message += item.moduleName + "/" + functionName;
                }
                message += cycle.functions.size() == 1 ? "存在递归调用" : "组成递归调用";
                emit(result, "GJB10157.9.3.5", source,
                     activeTokens[index].line, activeTokens[index].column,
                     message);
            }
            for (const auto index :
                 VerilogIndexChecks(preprocessed.source, activeTokens).complexIndexIndices())
                emit(result, "GJB10157.9.4.10", source,
                     activeTokens[index].line, activeTokens[index].column,
                     "建议数组索引为简单的信号，避免使用表达式");
            // Native slot 2 admits only AST type 4 (FUNCTION); the run
            // recursively collects every AST_ASSIGN_LE descendant.
            // ev_6228d5daa98a640bf83594b507d1ca5a22af12a6d3e31f0474d77867446620e9
            // ev_479db7b1c96fbb20ca9f2d8e00fe26f5b1fa2aa94d3fe2b8beaaf441093a18c1
            for (const auto index : functionNonblockingAssignments(activeTokens))
                emit(result, "GJB10157.9.4.3", source,
                     activeTokens[index].line, activeTokens[index].column,
                     "禁止在function语句中使用非阻塞赋值.");
            // Native R-13-4 emits at the ASSIGN_EQ first child and uses a
            // distinct function/task prefix followed by the normalized signal
            // name and the shared suffix. The inventory severity is Warning.
            for (const auto& write : detail::globalRegBlockingWrites(activeTokens)) {
                const auto& token = activeTokens[write.token];
                emit(result, "GJB10157.9.4.4", source, token.line, token.column,
                     std::string{write.function ? "在function语句中，禁止对全局信号" :
                                                  "在task语句中，禁止对全局信号"} +
                         write.signal + "进行赋值");
            }
            // Native R-13-5 reports the assignment root location and embeds
            // both independently inferred child widths in this exact text.
            for (const auto& mismatch : detail::functionReturnWidthMismatches(activeTokens)) {
                const auto& token = activeTokens[mismatch.token];
                emit(result, "GJB10157.9.4.5", source, token.line, token.column,
                     "在function语句中，返回值的位宽" +
                         std::to_string(mismatch.returnWidth) +
                         "与输入信号的位宽" + std::to_string(mismatch.inputWidth) +
                         "不一致");
            }
            // Native R-13-6 formats widths in assignment-child order and
            // appends the internal task formal's source name by direction.
            for (const auto& mismatch : detail::taskArgumentWidthMismatches(activeTokens)) {
                const auto& token = activeTokens[mismatch.token];
                emit(result, "GJB10157.9.4.6", source, token.line, token.column,
                     "task语句，输入信号的位宽" + std::to_string(mismatch.leftWidth) +
                         "与输出信号的位宽" + std::to_string(mismatch.rightWidth) +
                         "不一致" +
                         (mismatch.direction == detail::TaskBindingDirection::Input ?
                              "，任务输入：" : "，任务输出：") +
                         mismatch.formalName);
            }
            // Native 8.4/8.5 and 9.2.2/9.2.4/9.2.8/9.2.9 consume
            // expression/case nodes produced by the frontend. Run the bounded
            // source adapters on the configured active branch only. The
            // preprocessed SourceFile is required because active-token offsets
            // address its content; it preserves the original path and physical
            // line mapping used by diagnostics.
            expressionChecks(result, preprocessed.source, activeTokens, false);
            unsigned activeModuleDepth = 0;
            unsigned activeTaskDepth = 0;
            unsigned activeSubprogramDepth = 0;
            for (std::size_t i = 0; i < activeTokens.size(); ++i) {
                const auto& token = activeTokens[i];
                if (token.kind == parser::HdlTokenKind::Identifier) {
                    if (token.text == "module" || token.text == "macromodule" ||
                        token.text == "interface" || token.text == "program") {
                        ++activeModuleDepth;
                    } else if (token.text == "endmodule" ||
                               token.text == "endinterface" ||
                               token.text == "endprogram") {
                        if (activeModuleDepth != 0) --activeModuleDepth;
                        activeTaskDepth = 0;
                        activeSubprogramDepth = 0;
                    } else if (token.text == "task") {
                        ++activeTaskDepth;
                        ++activeSubprogramDepth;
                    } else if (token.text == "function") {
                        ++activeSubprogramDepth;
                    } else if (token.text == "endtask") {
                        if (activeTaskDepth != 0) --activeTaskDepth;
                        if (activeSubprogramDepth != 0) --activeSubprogramDepth;
                    } else if (token.text == "endfunction" &&
                               activeSubprogramDepth != 0) {
                        --activeSubprogramDepth;
                    }
                }
                const bool assignmentDelay = i > 0 &&
                    (activeTokens[i - 1].text == "=" ||
                     activeTokens[i - 1].text == "<=");
                if (activeTaskDepth != 0 && token.text == "#" &&
                    !assignmentDelay) {
                    // Native 9.4.1 selects AST_TASK descendants, and its
                    // assignment branch requires +0x29b. The only recovered
                    // real writer of that byte is Bison production 545:
                    // behavioral_stmt -> non_opt_delay behavioral_stmt.
                    // Therefore every active task-body prefix '#' is a proven
                    // source adapter, including macro and parenthesized forms.
                    // Intra-assignment delay productions do not set this flag.
                    // ev_9a12ca9f702b1999aa1ef7e39b23adad3bd6fa78676f156ac98d9b3bfc5fa0cb
                    // docs/evidence/native_verilog_task_timing_9_4_1_findings.md
                    emit(result, "GJB10157.9.4.1", source, token.line, token.column,
                         "task 中使用了前置 # 延时控制语句");
                }
                if (token.text == "initial" &&
                    token.kind == parser::HdlTokenKind::Identifier &&
                    activeModuleDepth != 0 && activeSubprogramDepth == 0) {
                    // Native 9.4.2 collects AST_INITIAL(0x53) nodes carrying
                    // +0x2a9 == 1. The unique recovered constant-one producer
                    // creates AST_INITIAL from the ANTLR initial_construct
                    // frontend context, sets the byte, then attaches its
                    // statement/block. The native recursive selector can only
                    // receive a node accepted by that parser production, so
                    // malformed task/function-local tokens and text outside a
                    // module-like design unit are excluded. Generate regions
                    // remain admitted because they are module items. Generated
                    // AST nodes without the frontend marker are also excluded.
                    // ev_b433931bb0da8d65d2a7ef81e810377c6eaab9d239e0985dd106560ff457f050
                    // docs/evidence/native_verilog_initial_9_4_2_findings.md
                    emit(result, "GJB10157.9.4.2", source, token.line, token.column,
                         "禁止使用initial语句");
                }
            }
        }
    }
    auto names = declaredNames(result, source);
    if (vhdl) expressionChecks(result, source, tokens, true);

    if (!vhdl) {
        for (std::size_t i = 0; i < tokens.size(); ++i) {
            if (tokens[i].text != "wire" && tokens[i].text != "reg" &&
                tokens[i].text != "logic" && tokens[i].text != "integer") continue;
            int bracketDepth = 0;
            bool expectsName = true;
            for (auto j = i + 1; j < tokens.size() && tokens[j].text != ";"; ++j) {
                const auto& token = tokens[j];
                if (token.text == "(") break; // port list/initializer expression is not a declaration list
                if (token.text == "[") ++bracketDepth;
                else if (token.text == "]") --bracketDepth;
                else if (bracketDepth == 0 && token.text == ",") expectsName = true;
                else if (bracketDepth == 0 && token.text == "=") expectsName = false;
                else if (bracketDepth == 0 && expectsName && token.kind == parser::HdlTokenKind::Identifier) {
                    if (token.text == "signed" || token.text == "unsigned") continue;
                    if (token.text == "input" || token.text == "output" || token.text == "inout") break;
                    const bool present = std::ranges::any_of(names, [&](const auto& name) {
                        return name.first == token.text && name.second.line == token.line &&
                               name.second.column == token.column;
                    });
                    if (!present) names.emplace_back(token.text, core::SourceLocation{
                        source.path(), token.line, token.column, token.offset});
                    expectsName = false;
                }
            }
        }
    }

    if (vhdl) {
        const auto& vhdlHeaderPolicy = options.verilogHeaderPolicy.configurationKnown
            ? options.verilogHeaderPolicy : Gjb9765DocumentedCommentPolicy;
        configuredVhdlHeaderChecks(result, source, tokens, vhdlHeaderPolicy);
    } else if (!vhdl && options.verilogHeaderPolicy.configurationKnown) {
        configuredVerilogHeaderChecks(result, source, tokens, options.verilogHeaderPolicy);
    } else if (!vhdl && (!startsWithHeaderComment(lines, false) ||
                         !documentedHeader(source, tokens))) {
        emit(result, "GJB10157.5.1.1", source, 1, 1,
             "产品默认文件头模板缺少文件名、作者、功能描述或修改记录字段（非厂商默认策略）");
    }

    std::vector<std::size_t> statementCounts(lines.size(), 0);
    std::vector<std::vector<bool>> assignmentCandidates(lines.size());
    std::vector<std::size_t> codeEnds(lines.size(), 0);
    // Native 5.1.6 groups AST assignment candidates; semicolons in declarations,
    // conditions and FOR headers are not assignment evidence. This is a bounded
    // source adapter, not a reconstruction of the full native AST grouping.
    const auto& statementTokens = !vhdl && preprocessed.issues.empty()
        ? activeTokens : tokens;
    std::vector<bool> forHeaderToken(statementTokens.size(), false);
    std::vector<bool> directLoopChildAssignment(statementTokens.size(), false);
    if (!vhdl && preprocessed.issues.empty())
    for (std::size_t i = 0; i + 1 < statementTokens.size(); ++i) {
        if (statementTokens[i].text != "for" || statementTokens[i + 1].text != "(") continue;
        int depth = 1;
        auto close = statementTokens.size();
        for (auto j = i + 2; j < statementTokens.size() && depth > 0; ++j) {
            forHeaderToken[j] = true;
            if (statementTokens[j].text == "(") ++depth;
            else if (statementTokens[j].text == ")" && --depth == 0) close = j;
        }
        if (close == statementTokens.size() || close + 1 >= statementTokens.size()) continue;
        const auto body = close + 1;
        const auto bodyWord = statementTokens[body].text;
        // FUN_1806de620 removes AST_ASSIGN/AST_ASSIGN_EQ/AST_ASSIGN_LE nodes
        // that are direct children of AST_FOR(0x5c)/AST_GENFOR(0x60). A
        // BEGIN/IF/CASE/loop wrapper makes the assignment a descendant rather
        // than a direct child, so only a bare assignment statement is removed.
        if (bodyWord == "begin" || bodyWord == "if" || bodyWord == "case" ||
            bodyWord == "casex" || bodyWord == "casez" || bodyWord == "for" ||
            bodyWord == "while" || bodyWord == "repeat" || bodyWord == "forever") continue;
        int bodyParentheses = 0;
        for (auto j = body; j < statementTokens.size(); ++j) {
            if (statementTokens[j].text == "(") ++bodyParentheses;
            else if (statementTokens[j].text == ")" && bodyParentheses > 0)
                --bodyParentheses;
            if (bodyParentheses == 0 && statementTokens[j].text == ";") break;
            if (bodyParentheses == 0 &&
                (statementTokens[j].text == "=" || statementTokens[j].text == "<=")) {
                directLoopChildAssignment[j] = true;
                break;
            }
        }
    }
    std::size_t segmentStart = 0;
    int parenthesisDepth = 0;
    std::vector<int> tokenDepth(statementTokens.size(), 0);
    for (std::size_t tokenIndex = 0; tokenIndex < statementTokens.size(); ++tokenIndex) {
        const auto& token = statementTokens[tokenIndex];
        if (token.text == "(") ++parenthesisDepth;
        tokenDepth[tokenIndex] = parenthesisDepth;
        if (token.text == ")") --parenthesisDepth;
        if (token.line == 0 || token.line > lines.size()) continue;
        auto& codeEnd = codeEnds[token.line - 1];
        codeEnd = std::max(codeEnd, token.column - 1 + token.text.size());
        if (token.text != ";") continue;
        if (vhdl) ++statementCounts[token.line - 1];
        else if (!forHeaderToken[tokenIndex]) {
            bool declaration = false;
            std::size_t assignment = statementTokens.size();
            for (auto j = segmentStart; j < tokenIndex; ++j) {
                if (forHeaderToken[j]) continue;
                if (tokenDepth[j] == 0 && statementTokens[j].kind == parser::HdlTokenKind::Identifier) {
                    const auto& word = statementTokens[j].text;
                    if (word == "parameter" || word == "localparam" || word == "wire" ||
                        word == "reg" || word == "logic" || word == "integer" ||
                        word == "int" || word == "genvar" || word == "input" ||
                        word == "output" || word == "inout" || word == "time" ||
                        word == "real" || word == "bit" || word == "tri" ||
                        word == "specparam" || word == "typedef") declaration = true;
                }
                if (tokenDepth[j] == 0 && (statementTokens[j].text == "=" || statementTokens[j].text == "<=")) {
                    if (assignment == statementTokens.size()) assignment = j;
                }
            }
            if (!declaration && assignment != statementTokens.size() &&
                !directLoopChildAssignment[assignment] &&
                statementTokens[assignment].line > 0 &&
                statementTokens[assignment].line <= lines.size()) {
                // The native pair test requires the earlier candidate's first
                // child to be AST_IDENTIFIER(0x13). Direct identifiers may
                // carry bit/part selections; concatenation or other compound
                // LHS roots do not qualify as the earlier half of a pair.
                bool directIdentifierLhs = false;
                auto lhsCursor = assignment;
                // Work backwards from the assignment operator so procedural
                // wrappers (always/begin/if/for) and the continuous-assignment
                // keyword cannot become part of the LHS.  Peel any number of
                // balanced bit/part selections, then require a base identifier.
                while (lhsCursor > segmentStart &&
                       statementTokens[lhsCursor - 1].text == "]") {
                    int brackets = 1;
                    --lhsCursor;
                    while (lhsCursor > segmentStart && brackets > 0) {
                        --lhsCursor;
                        if (statementTokens[lhsCursor].text == "]") ++brackets;
                        else if (statementTokens[lhsCursor].text == "[") --brackets;
                    }
                    if (brackets != 0) break;
                }
                if (lhsCursor > segmentStart &&
                    statementTokens[lhsCursor - 1].kind ==
                        parser::HdlTokenKind::Identifier) {
                    directIdentifierLhs = true;
                }
                assignmentCandidates[statementTokens[assignment].line - 1]
                    .push_back(directIdentifierLhs);
            }
        }
        segmentStart = tokenIndex + 1;
    }
    if (vhdl) {
        // Native 4.14.d.1 and d.2 share the same raw fgets(0xff) loop. Each
        // chunk ends at LF or 254 bytes, stops scanning at its first literal
        // --, and is numbered as one diagnostic line. d.1 counts every byte
        // except ASCII Tab, LF and space; d.2 counts every semicolon, including
        // those inside quoted text.
        // ev_de05796012609d038f81de9e112f2310faa3e25feaf0bcca39a86639c4955f75
        // ev_1368fc8f429eba7e2bc3986daf88028e9d33f5cbf274f32757b2333040e50c6a
        const auto chunks = nativeVhdlTextChunks(source.content());
        for (std::size_t index = 0; index < chunks.size(); ++index) {
            const auto& chunk = chunks[index];
            const auto comment = chunk.find("--");
            const auto limit = comment == std::string::npos ? chunk.size() : comment;
            std::size_t nonblank = 0;
            std::size_t overflowColumn = 1;
            std::size_t semicolons = 0;
            for (std::size_t position = 0; position < limit; ++position) {
                const auto value = chunk[position];
                if (value == ';') ++semicolons;
                if (value != '\t' && value != '\n' && value != ' ' && ++nonblank == 73)
                    overflowColumn = position + 1;
            }
            if (nonblank > 72)
                emit(result, "GJB9765.4.14.d.1", source, index + 1, overflowColumn,
                     "原生文本块在注释前包含超过 72 个非空白字节");
            if (semicolons > 1)
                emit(result, "GJB9765.4.14.d.2", source, index + 1, 1,
                     "原生文本块在注释前包含多个分号");
        }
    }
    for (std::size_t index = 0; index < lines.size(); ++index) {
        const auto& line = lines[index];
        const auto first = line.find_first_not_of(" \t");
        if (first == std::string::npos) continue;

        if (!vhdl) {
            const auto& candidates = assignmentCandidates[index];
            const bool nativePair = preprocessed.issues.empty() && candidates.size() > 1 &&
                std::ranges::any_of(candidates.begin(), candidates.end() - 1,
                                    [](bool eligible) { return eligible; });
            if (nativePair)
                emit(result, "GJB10157.5.1.6", source, index + 1, 1,
                     "一行中不止一个信号赋值语句");
        }
    }
    std::size_t nativeIncludeFileScans = 0;
    if (!vhdl) {
        // Native 5.2.7 iterates the design map keyed by module name.  Each
        // map value is a vector of AST roots; it opens root+0x1e0 only when
        // that vector contains exactly one AST_MODULE(2).  Consequently one
        // physical file is scanned once per uniquely bound module in it,
        // while duplicate definitions of the same module name are skipped.
        // T-Linter.dll FUN_18053afd0, SHA256
        // 60e6b52875f4d7c3fbdf494d869b3a4bcc3fdc8ee63d8620ea9614da5d6599b0.
        std::map<std::string, std::size_t> moduleDefinitions;
        for (const auto& unit : result.structure) {
            if (unit.kind == parser::DesignUnitKind::VerilogModule)
                ++moduleDefinitions[unit.name];
        }
        const auto sourcePath = source.path().lexically_normal();
        for (const auto& unit : result.structure) {
            if (unit.kind == parser::DesignUnitKind::VerilogModule &&
                moduleDefinitions[unit.name] == 1 &&
                unit.location.file.lexically_normal() == sourcePath)
                ++nativeIncludeFileScans;
        }
    }
    if (!vhdl) for (std::size_t index = 0; index < lines.size(); ++index) {
        // Native 5.2.7 opens the AST_MODULE source path and scans each
        // physical line for the exact eight-byte `include sequence, then for
        // ':' anywhere on that same line. It is deliberately not a lexer or
        // a general absolute-path predicate: comments, strings, malformed or
        // inactive directives remain visible, while a Unix /absolute path
        // without a colon does not match.
        // ev_c5b4c51bb57ffc67beaaca42512641c74f78e00d37435de2af5ba16ade471b59
        // ev_6abeb4923565b69c010a094b3e4106e612e4e7fc64537e6e37b6d4290670a65d
        const auto include = lines[index].find("`include");
        if (include == std::string::npos || lines[index].find(':') == std::string::npos)
            continue;
        for (std::size_t scan = 0; scan < nativeIncludeFileScans; ++scan)
            emit(result, "GJB10157.5.2.7", source, index + 1, 1,
                 "文件引用时,必须使用相对路径而不是绝对路径");
    }
    if (vhdl) {
        // Native 4.2.b collects output-port declarations and compares their
        // case-folded names with expression identifier nodes (0x13). The
        // helper excludes an identifier carrying AST attribute 0xad. Admit
        // only the unambiguous assignment-RHS subset here: declarations,
        // assignment targets, and multi-entity scope need no guessed result.
        // FUN_1805ceb20 -> FUN_1805cf7b0, T-Linter.dll SHA256
        // 60e6b52875f4d7c3fbdf494d869b3a4bcc3fdc8ee63d8620ea9614da5d6599b0.
        std::vector<const parser::DesignUnitInfo*> localUnits;
        for (const auto& unit : result.structure) {
            if (unit.location.file.lexically_normal() == source.path().lexically_normal())
                localUnits.push_back(&unit);
        }
        if (localUnits.size() == 1) {
            std::set<std::string> outputPorts;
            for (const auto& port : localUnits.front()->ports) {
                if (port.direction == parser::PortDirection::Output)
                    outputPorts.insert(lower(port.name));
            }
            bool architectureBody = false;
            bool awaitingArchitectureBegin = false;
            for (std::size_t index = 0; index < tokens.size(); ++index) {
                const auto word = lower(tokens[index].text);
                if (word == "architecture") {
                    architectureBody = false;
                    awaitingArchitectureBegin = true;
                } else if (awaitingArchitectureBegin && word == "begin") {
                    architectureBody = true;
                    awaitingArchitectureBegin = false;
                } else if (architectureBody && word == "end" && index + 1 < tokens.size() &&
                           lower(tokens[index + 1].text) == "architecture") {
                    architectureBody = false;
                }
                if (!architectureBody || tokens[index].text != "<=" || index == 0) continue;
                if (tokens[index - 1].kind != parser::HdlTokenKind::Identifier) continue;
                // The same VHDL token is a relational operator in conditions.
                // Only a statement-head assignment is admitted by this
                // source adapter; an earlier <= in this statement means the
                // current operator is part of its RHS expression.
                bool assignmentHead = true;
                for (auto previous = index; previous > 0;) {
                    const auto before = lower(tokens[--previous].text);
                    if (before == ";" || before == "then" || before == "else" ||
                        before == "loop" || before == "begin" || before == "=>") break;
                    if (before == "if" || before == "elsif" || before == "while" ||
                        before == "assert" || before == "wait" || before == "until" ||
                        before == "return" || before == "exit" || before == "next" ||
                        before == "<=" || before == ":=" || before == "(") {
                        assignmentHead = false;
                        break;
                    }
                }
                if (!assignmentHead) continue;
                for (auto rhs = index + 1; rhs < tokens.size() && tokens[rhs].text != ";";
                     ++rhs) {
                    if (tokens[rhs].kind != parser::HdlTokenKind::Identifier ||
                        !outputPorts.contains(lower(tokens[rhs].text))) continue;
                    if (rhs + 2 < tokens.size() && tokens[rhs + 1].text == "'" &&
                        lower(tokens[rhs + 2].text) == "event") continue;
                    emit(result, "GJB9765.4.2.b", source,
                         tokens[rhs].line, tokens[rhs].column,
                         "输出端口被同模块赋值表达式回读：" + tokens[rhs].text);
                }
            }
        }
        // Native 4.2.c collects declaration/port names and inspects selected
        // assignment AST shapes for Z-valued literals. FUN_18078ae30 exempts
        // a target only when the current design name equals setTopModule and
        // the target occurs in the entity-port set carrying byte +0x104. The
        // adjacent native suffix rule identifies +0x104 as VHDL INOUT: an
        // INOUT on a non-top entity is therefore internal hierarchy tri-state,
        // while only the selected top's INOUT boundary is accepted.
        // ev_bab82d7ffc9b587caf647c21c1f812b7a9b987bc893d0b6468e92689d22a6208
        // ev_af5e7a852961241391732336b1e1511cf33e73275aef08013c217669a51b0ba9
        struct VhdlTriStateScope {
            std::size_t begin{0};
            std::size_t end{0};
            std::string entity;
            std::set<std::string> targets;
            std::set<std::string> controls;
            std::set<std::string> inoutPorts;
            bool selectedBoundary{false};
        };
        std::vector<VhdlTriStateScope> triStateScopes;
        for (std::size_t index = 0; index + 3 < tokens.size(); ++index) {
            if (lower(tokens[index].text) != "architecture" ||
                lower(tokens[index + 2].text) != "of" ||
                tokens[index + 3].kind != parser::HdlTokenKind::Identifier) continue;
            if (!triStateScopes.empty()) triStateScopes.back().end = index;
            VhdlTriStateScope scope;
            scope.begin = index;
            scope.end = tokens.size();
            scope.entity = lower(tokens[index + 3].text);
            const auto unit = std::ranges::find_if(result.structure, [&](const auto& candidate) {
                return candidate.kind == parser::DesignUnitKind::VhdlEntity &&
                       candidate.location.file.lexically_normal() == source.path().lexically_normal() &&
                       lower(candidate.name) == scope.entity;
            });
            if (unit != result.structure.end()) {
                scope.selectedBoundary = selectedTop(*unit, options);
                for (const auto& port : unit->ports) {
                    scope.targets.insert(lower(port.name));
                    scope.controls.insert(lower(port.name));
                    if (port.direction == parser::PortDirection::Inout)
                        scope.inoutPorts.insert(lower(port.name));
                }
                for (const auto& generic : unit->parameters)
                    scope.controls.insert(lower(generic.name));
            }
            triStateScopes.push_back(std::move(scope));
        }
        for (auto& scope : triStateScopes) {
            for (auto index = scope.begin; index < scope.end; ++index) {
                if (lower(tokens[index].text) != "signal") continue;
                auto cursor = index + 1;
                std::vector<std::string> declared;
                while (cursor < scope.end &&
                       tokens[cursor].kind == parser::HdlTokenKind::Identifier) {
                    declared.push_back(lower(tokens[cursor++].text));
                    if (cursor >= scope.end || tokens[cursor].text != ",") break;
                    ++cursor;
                }
                if (cursor < scope.end && tokens[cursor].text == ":") {
                    scope.targets.insert(declared.begin(), declared.end());
                    scope.controls.insert(declared.begin(), declared.end());
                }
            }
        }
        for (std::size_t index = 0; index < tokens.size(); ++index) {
            if (!vhdlMetaValue(tokens[index]) ||
                lower(tokens[index].text).find('z') == std::string::npos) continue;
            const auto scope = std::ranges::find_if(triStateScopes, [&](const auto& candidate) {
                return index >= candidate.begin && index < candidate.end;
            });
            if (scope == triStateScopes.end()) continue;
            std::size_t begin = index;
            while (begin > 0) {
                const auto previous = lower(tokens[begin - 1].text);
                if (previous == ";" || previous == "then" ||
                    previous == "begin" || previous == "=>") break;
                --begin;
            }
            std::size_t assignment = index;
            while (assignment > begin && tokens[assignment - 1].text != "<=") --assignment;
            if (assignment == begin) continue;
            --assignment;
            bool inConditionalPredicate = false;
            for (auto cursor = assignment + 1; cursor < index; ++cursor) {
                const auto word = lower(tokens[cursor].text);
                if (word == "when") inConditionalPredicate = true;
                else if (word == "else") inConditionalPredicate = false;
            }
            if (inConditionalPredicate) continue;
            if (std::ranges::any_of(tokens.begin() + static_cast<std::ptrdiff_t>(begin),
                    tokens.begin() + static_cast<std::ptrdiff_t>(assignment),
                    [](const auto& token) {
                        const auto word = lower(token.text);
                        return word == "if" || word == "elsif" || word == "while";
                    })) continue;
            if (assignment == 0) continue;
            auto target = assignment - 1;
            if (tokens[target].text == ")") {
                int depth = 1;
                while (target > begin && depth > 0) {
                    --target;
                    if (tokens[target].text == ")") ++depth;
                    else if (tokens[target].text == "(") --depth;
                }
                if (depth != 0 || target == begin) continue;
                --target;
            }
            if (tokens[target].kind != parser::HdlTokenKind::Identifier) continue;
            const auto targetName = lower(tokens[target].text);
            const auto hasDeclaredComparison = [&](std::size_t first, std::size_t last) {
                const auto stripEnclosingParentheses = [&](std::size_t& rangeFirst,
                                                            std::size_t& rangeLast) {
                    while (rangeFirst < rangeLast && tokens[rangeFirst].text == "(" &&
                           tokens[rangeLast - 1].text == ")") {
                        int rangeDepth = 0;
                        bool enclosesWholeRange = true;
                        for (auto cursor = rangeFirst; cursor < rangeLast; ++cursor) {
                            if (tokens[cursor].text == "(") ++rangeDepth;
                            else if (tokens[cursor].text == ")") --rangeDepth;
                            if (rangeDepth == 0 && cursor + 1 < rangeLast) {
                                enclosesWholeRange = false;
                                break;
                            }
                            if (rangeDepth < 0) return false;
                        }
                        if (!enclosesWholeRange || rangeDepth != 0) break;
                        ++rangeFirst;
                        --rangeLast;
                    }
                    return true;
                };
                if (!stripEnclosingParentheses(first, last)) return false;
                std::size_t comparisonCount = 0;
                std::optional<std::size_t> comparison;
                int depth = 0;
                for (auto cursor = first; cursor < last; ++cursor) {
                    const auto word = lower(tokens[cursor].text);
                    if (word == "(") { ++depth; continue; }
                    if (word == ")") { --depth; if (depth < 0) return false; continue; }
                    if (depth == 0 && (word == "and" || word == "or" || word == "xor" ||
                        word == "xnor" || word == "nand" || word == "nor" || word == "not"))
                        return false;
                    if (depth == 0 && (word == "=" || word == "?=" || word == "<" ||
                        word == "<=" || word == ">" || word == ">=")) {
                        ++comparisonCount;
                        comparison = cursor;
                    }
                }
                if (depth != 0 || comparisonCount != 1 || !comparison) return false;
                const auto directDeclaredOperand = [&](std::size_t operandFirst,
                                                       std::size_t operandLast) {
                    if (!stripEnclosingParentheses(operandFirst, operandLast) ||
                        operandFirst >= operandLast ||
                        tokens[operandFirst].kind != parser::HdlTokenKind::Identifier ||
                        !scope->controls.contains(lower(tokens[operandFirst].text))) return false;
                    ++operandFirst;
                    while (operandFirst < operandLast) {
                        if (tokens[operandFirst].text != "(") return false;
                        int selectorDepth = 0;
                        do {
                            if (tokens[operandFirst].text == "(") ++selectorDepth;
                            else if (tokens[operandFirst].text == ")") --selectorDepth;
                            ++operandFirst;
                            if (selectorDepth < 0) return false;
                        } while (operandFirst < operandLast && selectorDepth != 0);
                        if (selectorDepth != 0) return false;
                    }
                    return true;
                };
                return directDeclaredOperand(first, *comparison) ||
                       directDeclaredOperand(*comparison + 1, last);
            };
            const auto directDataName = [&](std::size_t first, std::size_t last) {
                if (first >= last || tokens[first].kind != parser::HdlTokenKind::Identifier)
                    return false;
                int parentheses = 0;
                for (auto cursor = first + 1; cursor < last; ++cursor) {
                    const auto word = lower(tokens[cursor].text);
                    if (word == "(") ++parentheses;
                    else if (word == ")") --parentheses;
                    else if (parentheses == 0) return false;
                    if (parentheses < 0) return false;
                }
                return parentheses == 0;
            };

            // FUN_18078c370 admits two source-visible candidate shapes.  The
            // direct branch is `target <= data when relation else Z`, where
            // data is a direct name/select and relation contains a declared
            // operand.  The complex 0xaa/0x58/0xac branch is an IF with a
            // direct THEN assignment and a final ELSE Z assignment to the
            // same target.  A bare or one-armed process Z assignment is not a
            // native candidate merely because the literal occurs.
            bool nativeCandidate = false;
            std::optional<std::size_t> whenIndex;
            std::optional<std::size_t> elseIndex;
            for (auto cursor = assignment + 1; cursor < index; ++cursor) {
                const auto word = lower(tokens[cursor].text);
                if (word == "when" && !whenIndex) whenIndex = cursor;
                else if (word == "else") elseIndex = cursor;
            }
            if (whenIndex && elseIndex && *whenIndex < *elseIndex && *elseIndex < index &&
                directDataName(assignment + 1, *whenIndex) &&
                hasDeclaredComparison(*whenIndex + 1, *elseIndex)) {
                nativeCandidate = true;
            }

            std::optional<std::size_t> processElse;
            if (begin < index && lower(tokens[begin].text) == "else") processElse = begin;
            else if (begin > scope->begin && lower(tokens[begin - 1].text) == "else")
                processElse = begin - 1;
            if (!nativeCandidate && processElse) {
                const auto finalElse = *processElse;
                std::optional<std::size_t> matchingIf;
                int nestedIf = 0;
                for (auto cursor = finalElse; cursor > scope->begin;) {
                    --cursor;
                    if (lower(tokens[cursor].text) != "if") continue;
                    if (cursor > 0 && lower(tokens[cursor - 1].text) == "end") {
                        ++nestedIf;
                    } else if (nestedIf > 0) {
                        --nestedIf;
                    } else {
                        matchingIf = cursor;
                        break;
                    }
                }
                std::optional<std::size_t> thenIndex;
                if (matchingIf) {
                    for (auto cursor = *matchingIf + 1; cursor < finalElse; ++cursor) {
                        if (lower(tokens[cursor].text) == "then") {
                            thenIndex = cursor;
                            break;
                        }
                    }
                }
                bool matchingThenTarget = false;
                if (matchingIf && thenIndex &&
                    hasDeclaredComparison(*matchingIf + 1, *thenIndex)) {
                    int nestedControl = 0;
                    bool twoArmShape = true;
                    for (auto cursor = *thenIndex + 1; cursor < finalElse; ++cursor) {
                        const auto word = lower(tokens[cursor].text);
                        if (word == "if" || word == "case" || word == "loop") {
                            if (cursor > *thenIndex + 1 &&
                                lower(tokens[cursor - 1].text) == "end") {
                                if (nestedControl > 0) --nestedControl;
                            } else {
                                ++nestedControl;
                            }
                            continue;
                        }
                        if (word == "elsif" && nestedControl == 0) {
                            twoArmShape = false;
                            break;
                        }
                        if (nestedControl != 0) continue;
                        if (tokens[cursor].text != "<=" || cursor == 0) continue;
                        auto thenTarget = cursor - 1;
                        if (tokens[thenTarget].text == ")") {
                            int depth = 1;
                            while (thenTarget > *thenIndex && depth > 0) {
                                --thenTarget;
                                if (tokens[thenTarget].text == ")") ++depth;
                                else if (tokens[thenTarget].text == "(") --depth;
                            }
                            if (depth != 0 || thenTarget == *thenIndex) continue;
                            --thenTarget;
                        }
                        if (tokens[thenTarget].kind == parser::HdlTokenKind::Identifier &&
                            lower(tokens[thenTarget].text) == targetName) {
                            matchingThenTarget = true;
                        }
                    }
                    if (nestedControl != 0 || !twoArmShape) matchingThenTarget = false;
                }
                nativeCandidate = matchingThenTarget;
            }
            if (!nativeCandidate) continue;
            if (!scope->targets.contains(targetName) ||
                (scope->selectedBoundary && scope->inoutPorts.contains(targetName))) continue;
            emit(result, "GJB9765.4.2.c", source,
                 tokens[index].line, tokens[index].column,
                 "内部层次目标的赋值表达式含三态 Z：" + tokens[target].text);
        }
        // Native 4.5.f.1 defaults to a [1, 20]-byte name range. Its sole
        // collector FUN_1805e5770 recursively visits every child and admits
        // named AST_FUNCTION(0x04), AST_ALWAYS/process(0x52), AST_BLOCK(0x54),
        // AST_ARCHITECTURE(0x75), AST_ENTITY(0x76), AST_TYPE(0x77),
        // AST_PROCEDURE(0x8e) and AST_SUBTYPE(0x9f). Direct declaration
        // children of architecture/block/generic/port containers additionally
        // admit AST_WIRE(0x06), AST_LOCALPARAM(0x0a), AST_IDENTIFIER(0x13),
        // AST_SIGNAL(0x78) and AST_VARIABLE(0x79). Synthetic process/block
        // container names are excluded, but their declaration children remain
        // candidates. Mirror the source-identifiable subset below.
        // ev_3601a6511a5ae0ab2ab5774e646c4d539c09e490e215d0787a1eb795993a79bf
        // ev_ee40a7de917d054ad0f2ab65f470653ad85425410a04a4c35e2e9de587250563
        std::set<std::size_t> nativeNameLengthCandidates;
        const auto reportLongName = [&](std::size_t tokenIndex,
                                        std::string_view category,
                                        std::size_t maximum = 20) {
            if (tokenIndex >= tokens.size() ||
                !nativeNameLengthCandidates.insert(tokenIndex).second) return;
            const auto& token = tokens[tokenIndex];
            if (token.text.size() <= maximum) return;
            emit(result, "GJB9765.4.5.f.1", source,
                 token.line, token.column,
                 std::string(category) + "名称超过 20 个字符：" + token.text);
        };
        for (const auto& object : collectVhdlNamedObjects(source, tokens))
            reportLongName(object.token, vhdlNameCategoryName(object.category));

        for (std::size_t index = 0; index < tokens.size(); ++index) {
            const auto word = lower(tokens[index].text);
            if ((word == "function" || word == "procedure") &&
                (index == 0 || lower(tokens[index - 1].text) != "end")) {
                auto name = index + 1;
                while (name < tokens.size() &&
                       (lower(tokens[name].text) == "pure" ||
                        lower(tokens[name].text) == "impure")) ++name;
                if (name < tokens.size() &&
                    tokens[name].kind == parser::HdlTokenKind::Identifier)
                    reportLongName(name, word == "function" ? "函数" : "过程");
            }
            if ((word == "process" || word == "block") && index >= 2 &&
                tokens[index - 1].text == ":" &&
                tokens[index - 2].kind == parser::HdlTokenKind::Identifier) {
                // Native AST_BLOCK enters the candidate path only when its
                // stored name is longer than 0x16 bytes; 21- and 22-byte
                // labels therefore remain excluded despite the global 20-byte
                // maximum. AST_ALWAYS/process has no corresponding gate.
                reportLongName(index - 2, word == "process" ? "进程标号" : "块标号",
                               word == "block" ? 22 : 20);
            }
        }
    }

    // Native GJB10157.5.2.13 slot 1 admits AST_MODULE(2).  Its recursive
    // producer FUN_18062d430 inserts every AST_WIRE(6) into the rule object's
    // module/name map, and FUN_18062b0a0 stores that declaration node at the
    // map record's +0x40.  The run resolves an edge relation through this map
    // before reading source/line from the stored declaration.  Keep the
    // source adapter bounded to declarations admitted by the configured
    // frontend branch; an implicit or unsupported net is not proof that the
    // native lookup succeeds.
    // ev_aaa493aac258e6c8e9f4d04fcd2ebeb575f37e5d55e363a21c32a358a3f3965b
    // docs/evidence/native_verilog_clock_name_5_2_13_findings.md
    std::map<std::string, std::map<std::string, core::SourceLocation>>
        verilogWireDeclarations;
    if (!vhdl && preprocessed.issues.empty()) {
        for (const auto& declaration :
             VerilogNamingDeclarations(preprocessed.source, activeTokens).collect()) {
            if (declaration.subject != VerilogNamingSubject::WireLike ||
                declaration.moduleName.empty()) continue;
            const auto& token = activeTokens[declaration.tokenIndex];
            verilogWireDeclarations[declaration.moduleName].try_emplace(
                declaration.normalizedName,
                core::SourceLocation{source.path(), token.line, token.column, token.offset});
        }
    }

    std::vector<std::string> unitsInFile;
    std::size_t firstUnitLine = 0;
    for (const auto& unit : result.structure) {
        if (unit.location.file.lexically_normal() != source.path().lexically_normal()) continue;
        if (!vhdl) unitsInFile.push_back(unit.name);
        if (firstUnitLine == 0 ||
            (unit.location.line != 0 && unit.location.line < firstUnitLine)) {
            firstUnitLine = unit.location.line;
        }
        const auto verilogFileStem = !vhdl ? nativeVerilogFileStem(source.path()) : std::string{};
        if (!vhdl && verilogFileStem != unit.name) {
            emit(result, "GJB10157.5.2.11", source, unit.location.line, 1,
                 "模块名" + unit.name + "与文件名" + verilogFileStem + "不匹配.");
        }
        if (vhdl && unit.ports.empty()) {
            emit(result, "GJB9765.4.2.a", source, unit.location.line, unit.location.column,
                 "设计单元没有输入或输出端口");
        }
        for (const auto& instance : unit.instances) {
            if (vhdl) {
                // Native helper groups children of AST 0x93/0x94 whose type
                // is 0xa5, then run compares each adjacent child's source
                // line (+0x214). Each equal pair emits one Warning.
                // ev_0c4782c7bd332dc7f8b4f024f186a69acd10988c5e4f98a3f183fae508f6ad8e
                // ev_7566d444631fccdf6d648fff1078189bb9dd24356f7e3b56e2cff6e47e93d322
                for (std::size_t connection = 1;
                     connection < instance.namedConnections.size(); ++connection) {
                    const auto& previous = instance.namedConnections[connection - 1];
                    const auto& current = instance.namedConnections[connection];
                    if (previous.location.line != current.location.line) continue;
                    emit(result, "GJB9765.4.14.d.3", source,
                         current.location.line, current.location.column,
                         "同一物理行映射了多个端口信号：" + previous.formal + "、" + current.formal);
                }
                // Native 4.14.b.6 groups association ASTs beneath parent
                // types 0x93/0x94, finds each association name on its source
                // line, expands tabs to four-column stops and compares each
                // display column with the preceding association. The product
                // maps this conservatively to parsed named port associations
                // within one instance.
                // ev_8d5731fd12306939d435af014950f416636e7c20af1e9df28a4f9091cfd5b8e4
                // ev_e230f8f40b33d773122b5b6affbd3638b77186d5a11cea52e8f6ac9920c7b320
                if (instance.namedConnections.size() > 1) {
                    const auto& first = instance.namedConnections.front();
                    std::size_t expectedColumn = first.location.column;
                    if (first.location.line > 0 && first.location.line <= lines.size())
                        expectedColumn = tab4DisplayColumn(lines[first.location.line - 1], first.location.column);
                    for (std::size_t connection = 1;
                         connection < instance.namedConnections.size(); ++connection) {
                        const auto& current = instance.namedConnections[connection];
                        std::size_t displayColumn = current.location.column;
                        if (current.location.line > 0 && current.location.line <= lines.size())
                            displayColumn = tab4DisplayColumn(lines[current.location.line - 1], current.location.column);
                        if (displayColumn != expectedColumn)
                            emit(result, "GJB9765.4.14.b.6", source,
                                 current.location.line, current.location.column,
                                 "端口映射名称未与前一项对齐：" + current.formal);
                        expectedColumn = displayColumn;
                    }
                }
            }
            if (instance.hasPositionalConnections) {
                // Native VHDL 4.1.a helper FUN_180779190 recursively collects
                // association containers of type 0xa5. Slot 0 FUN_180778a50
                // inspects their direct children, accepts type 0x8b as named,
                // and emits at the first other child before breaking. Thus a
                // mixed or positional instance produces at most one finding.
                // ev_b5f39d3c2222897018de21db1dddbd6a429f292f203f844aa6413c58c004bb07
                // Verilog 5.2.9 slot 1 admits AST_CELL(0x4f). Slot 0 then
                // recursively checks that cell's AST_ARGUMENT descendants and
                // stops after the first empty argument name, so every
                // positional cell emits once (not once per design unit). The
                // native record uses the cell name/source/physical line and
                // has no column.
                // ev_2931001d50d0a817c327e36f2110c5a6b8262d6ef85f7f36049a336d94f9a687
                // ev_2148b633a698afcb26c5e94d994d52969dc623bbcd931daae04a4f0fa1f1b67c
                emit(result, vhdl ? "GJB9765.4.1.a" : "GJB10157.5.2.9",
                     source, instance.location.line,
                     vhdl ? instance.location.column : 1,
                     vhdl ? "实例使用了位置端口映射：" + instance.name
                          : "实例" + instance.name + "未使用命名关联端口映射连接");
            }
            if (!vhdl) {
                // Native 5.3.1 slot 2 admits AST_CELL(0x4f). Its run examines
                // every direct AST_ARGUMENT(13) child of each admitted cell
                // and reports independently when that argument's first child
                // is not AST_CONSTANT(16), AST_IDENTIFIER(19), or
                // AST_PREFIX(20). Source and physical line come from the first
                // child; the record has no column and uses one fixed message.
                // Our parser marks only source expressions with a known
                // non-admitted root; unknown syntax stays under review.
                // ev_efbd0c9b79d5a3d8a287fcabac46eb74e69b3fbc052895287945719c4057ea1a
                // ev_2931001d50d0a817c327e36f2110c5a6b8262d6ef85f7f36049a336d94f9a687
                // docs/evidence/native_verilog_port_actual_expression_5_3_1_findings.md
                for (const auto& connection : instance.namedConnections) {
                    if (!connection.actualExpression) continue;
                    emit(result, "GJB10157.5.3.1", source,
                         connection.actualLocation.line, 1,
                         "端口例化时禁止使用表达式");
                }
                for (const auto& connection : instance.positionalConnections) {
                    if (!connection.actualExpression) continue;
                    emit(result, "GJB10157.5.3.1", source,
                         connection.location.line, 1,
                         "端口例化时禁止使用表达式");
                }
            }
        }
        std::set<std::string> checkedVhdlClockNames;
        if (vhdl) {
            // Native 4.5.e passes literal "clk_" to a case-insensitive prefix
            // comparison (mode 0, case-fold flag 0). The packaged example
            // also marks ordinary output C1_clk as violating: admit only
            // ports with an explicit clock marker here, not every port.
            // ev_ec182ef40a94ff6e8d75359f85e530b722654f25849544326c8504e836a29d3a
            // ev_5bc843956998c4f19286c500ae9637aa4da9cad9861e845d50d703b9407649cb
            // ev_38d28a2a9a0f98140f3a209414e8ae07adc926bb0e02397f7b2b4429dd1fe789
            for (const auto& port : unit.ports) {
                const auto name = lower(port.name);
                if (name.find("clk") == std::string::npos) continue;
                checkedVhdlClockNames.insert(name);
                if (!name.starts_with("clk_"))
                    emit(result, "GJB9765.4.5.e", source,
                         port.location.line, port.location.column,
                         "时钟相关端口名称未以 clk_ 开始：" + port.name);
            }
        }
        std::set<std::string> checkedVerilogClockNames;
        const auto moduleDeclarations = !vhdl
            ? verilogWireDeclarations.find(unit.name)
            : verilogWireDeclarations.end();
        for (const auto& clock : unit.clocks) {
            const auto clockName = lower(clock.signal);
            if (vhdl && checkedVhdlClockNames.contains(clockName)) continue;
            // Native 5.2.13 admits relation categories 2/3, normalizes an
            // escaped name, and inserts that bytewise name into one set per
            // design record before evaluating it.  Consequently the same
            // invalid signal observed on both edges reports only once. The
            // diagnostic source and physical line come from the resolved
            // AST_WIRE declaration, not the edge occurrence; its record has
            // no column, so the source adapter uses column 1.
            if (!vhdl && !checkedVerilogClockNames.insert(clock.signal).second) continue;
            const core::SourceLocation* clockDeclaration = nullptr;
            if (!vhdl && moduleDeclarations != verilogWireDeclarations.end()) {
                const auto declaration = moduleDeclarations->second.find(clock.signal);
                if (declaration != moduleDeclarations->second.end())
                    clockDeclaration = &declaration->second;
            }
            if (!vhdl && clockDeclaration == nullptr) continue;
            if (vhdl ? !clockName.starts_with("clk_") : !nativeVerilogClockName(clock.signal)) {
                emit(result, vhdl ? "GJB9765.4.5.e" : "GJB10157.5.2.13",
                     source, vhdl ? clock.location.line : clockDeclaration->line,
                     vhdl ? clock.location.column : 1,
                     vhdl ? "时钟信号名称未以 clk_ 开始：" + clock.signal :
                         "时钟信号" + clock.signal +
                             "命名应采用“clk_x”或者“Clk_X”,其中“x”代表特定含义的时钟频率");
            }
        }
        if (!vhdl) for (const auto& reset : unit.resets) {
            if (!nativeVerilogResetName(reset.signal))
                emit(result, "GJB10157.5.2.14", source,
                     reset.location.line, 1,
                     "复位信号" + reset.signal +
                         "命名应采用“RST_X”、“Reset_X”、“rst_x”或者“reset_x”，"
                         "其中“x”为“p”时代表高电平复位，“x”为“n”时代表低电平复位");
        }
    }
    if (!vhdl && unitsInFile.size() > 1) {
        std::string message = "在源文件中找到多个模块:";
        for (std::size_t index = 0; index < unitsInFile.size(); ++index) {
            message += unitsInFile[index];
            message += index + 1 == unitsInFile.size() ? '.' : ',';
        }
        emit(result, "GJB10157.5.2.5", source,
             firstUnitLine == 0 ? 1 : firstUnitLine, 1, std::move(message));
    }
}

} // namespace

std::string_view standardRuleStateName(StandardRuleState state) noexcept {
    switch (state) {
    case StandardRuleState::Passed: return "no-violation-found";
    case StandardRuleState::Violated: return "violated";
    case StandardRuleState::ProjectContextRequired: return "project-context-required";
    case StandardRuleState::ManualReviewRequired: return "manual-review-required";
    }
    return "unknown";
}

bool hasAutomatedStandardDetector(std::string_view clauseId) noexcept {
    return std::ranges::any_of(Mappings, [&](const auto& mapping) {
               return mapping.clauseId == clauseId;
           }) || std::ranges::find(DirectClauses, clauseId) != DirectClauses.end();
}

struct FixedVerilogModuleWidthModel {
    const core::SourceFile* source{nullptr};
    const std::vector<parser::HdlToken>* tokens{nullptr};
    std::size_t begin{0}, end{0};
    std::map<std::string, std::optional<std::size_t>> signalWidths;
    std::map<std::string, std::optional<std::size_t>> portWidths;
};

std::optional<std::size_t> fixedDecimalToken(const parser::HdlToken& token) {
    if (token.kind != parser::HdlTokenKind::Number) return std::nullopt;
    std::string literal;
    for (const char ch : token.text)
        if (ch != '_') literal.push_back(static_cast<char>(std::tolower(
            static_cast<unsigned char>(ch))));
    int base = 10;
    std::string_view digits = literal;
    const auto quote = literal.find('\'');
    if (quote != std::string::npos) {
        auto baseAt = quote + 1;
        if (baseAt < literal.size() && literal[baseAt] == 's') ++baseAt;
        if (baseAt >= literal.size()) return std::nullopt;
        if (literal[baseAt] == 'b') base = 2;
        else if (literal[baseAt] == 'o') base = 8;
        else if (literal[baseAt] == 'd') base = 10;
        else if (literal[baseAt] == 'h') base = 16;
        else return std::nullopt;
        digits = std::string_view(literal).substr(baseAt + 1);
    }
    if (digits.empty() || digits.find_first_of("xz?") != std::string_view::npos)
        return std::nullopt;
    unsigned long long decoded = 0;
    const auto [end, error] = std::from_chars(
        digits.data(), digits.data() + digits.size(), decoded, base);
    return error == std::errc{} && end == digits.data() + digits.size() &&
           decoded <= 65535 ? std::optional<std::size_t>{decoded} : std::nullopt;
}

std::optional<std::size_t> fixedPackedWidth(
    const std::vector<parser::HdlToken>& tokens, std::size_t begin, std::size_t end,
    const std::map<std::string, std::size_t>* parameters = nullptr) {
    const auto atom = [&](std::size_t at) -> std::optional<std::size_t> {
        if (const auto decimal = fixedDecimalToken(tokens[at])) return decimal;
        if (parameters && tokens[at].kind == parser::HdlTokenKind::Identifier) {
            const auto found = parameters->find(tokens[at].text);
            if (found != parameters->end()) return found->second;
        }
        return std::nullopt;
    };
    const auto endpoint = [&](std::size_t first, std::size_t last) -> std::optional<std::size_t> {
        if (first >= last) return std::nullopt;
        if (last == first + 1) return atom(first);
        if (last != first + 3 || !parameters) return std::nullopt;
        const auto left = atom(first);
        const auto right = atom(first + 2);
        if (!left || !right) return std::nullopt;
        if (tokens[first + 1].text == "+" && *left <= 65535 - *right)
            return *left + *right;
        if (tokens[first + 1].text == "-" && *left >= *right)
            return *left - *right;
        return std::nullopt;
    };
    std::optional<std::size_t> width = 1;
    bool sawPackedRange = false;
    for (auto i = begin; i < end; ++i) {
        if (tokens[i].text != "[") continue;
        if (sawPackedRange) return std::nullopt;
        auto colon = i + 1;
        while (colon < end && tokens[colon].text != ":" && tokens[colon].text != "]") ++colon;
        if (colon >= end || tokens[colon].text != ":") return std::nullopt;
        auto close = colon + 1;
        while (close < end && tokens[close].text != "]") ++close;
        if (close >= end) return std::nullopt;
        const auto left = endpoint(i + 1, colon);
        const auto right = endpoint(colon + 1, close);
        if (!left || !right) return std::nullopt;
        width = *left > *right ? *left - *right + 1 : *right - *left + 1;
        sawPackedRange = true;
        i = close;
    }
    return width;
}

std::optional<std::size_t> tokenAtLocation(
    const std::vector<parser::HdlToken>& tokens, const core::SourceLocation& location) {
    for (std::size_t i = 0; i < tokens.size(); ++i)
        if (tokens[i].line == location.line && tokens[i].column == location.column)
            return i;
    return std::nullopt;
}

bool nativeVhdlSignalHasAnyUse(std::span<const core::SourceFile> sources,
                               const Violation& violation) {
    if (!violation.signal) return false;
    for (const auto& source : sources) {
        if (source.language() != core::HdlLanguage::Vhdl ||
            source.path().lexically_normal() != violation.file.lexically_normal()) continue;
        const auto tokens = standardTokens(source);
        const auto declaration = std::ranges::find_if(tokens, [&](const auto& token) {
            return token.line == violation.line && token.column == violation.column;
        });
        if (declaration == tokens.end()) return false;
        const auto declarationIndex = static_cast<std::size_t>(declaration - tokens.begin());
        std::size_t first = 0;
        for (std::size_t i = declarationIndex; i > 0; --i) {
            if (lower(tokens[i - 1].text) == "architecture" &&
                (i < 2 || lower(tokens[i - 2].text) != "end")) {
                first = i - 1;
                break;
            }
        }
        auto last = tokens.size();
        for (std::size_t i = declarationIndex + 1; i < tokens.size(); ++i) {
            if (lower(tokens[i].text) == "architecture" &&
                (i == 0 || lower(tokens[i - 1].text) != "end")) {
                last = i;
                break;
            }
        }
        const auto name = lower(*violation.signal);
        for (auto i = first; i < last; ++i) {
            if (i != declarationIndex &&
                tokens[i].kind == parser::HdlTokenKind::Identifier &&
                lower(tokens[i].text) == name) return true;
        }
        return false;
    }
    return false;
}

std::optional<FixedVerilogModuleWidthModel> fixedVerilogModuleWidthModel(
    const parser::DesignUnitInfo& unit, const core::SourceFile& source,
    const std::vector<parser::HdlToken>& tokens, bool allowParameterDefaults = false) {
    if (unit.kind != parser::DesignUnitKind::VerilogModule ||
        unit.location.file.lexically_normal() != source.path().lexically_normal()) return std::nullopt;
    const auto name = tokenAtLocation(tokens, unit.location);
    if (!name || *name == 0 || tokens[*name - 1].text != "module" ||
        tokens[*name].text != unit.name) return std::nullopt;
    const auto begin = *name - 1;
    auto end = *name + 1;
    while (end < tokens.size() && tokens[end].text != "endmodule") ++end;
    if (end == tokens.size()) return std::nullopt;
    FixedVerilogModuleWidthModel model{&source, &tokens, begin, end, {}, {}};
    std::map<std::string, std::size_t> parameterDefaults;
    if (allowParameterDefaults) {
        for (const auto& parameter : unit.parameters) {
            const auto at = tokenAtLocation(tokens, parameter.location);
            if (!at || *at + 2 >= end || tokens[*at + 1].text != "=") continue;
            const auto value = fixedDecimalToken(tokens[*at + 2]);
            if (value && *value > 0 && (*at + 3 == end ||
                tokens[*at + 3].text == "," || tokens[*at + 3].text == ")"))
                parameterDefaults[parameter.name] = *value;
        }
    }
    const auto declaredWidth = [&](std::size_t at) -> std::optional<std::size_t> {
        for (auto cursor = at; cursor > begin + 1;) {
            --cursor;
            if (tokens[cursor].text == ";") break;
            const auto& word = tokens[cursor].text;
            if (word != "input" && word != "output" && word != "inout" &&
                word != "wire" && word != "reg" && word != "logic" && word != "tri") continue;
            // A name in an initializer is not a declaration name.
            for (auto between = cursor + 1; between < at; ++between)
                if (tokens[between].text == "=") return std::nullopt;
            return fixedPackedWidth(tokens, cursor + 1, at,
                                    allowParameterDefaults ? &parameterDefaults : nullptr);
        }
        return std::nullopt;
    };
    for (const auto& port : unit.ports) {
        const auto at = tokenAtLocation(tokens, port.location);
        if (!at || *at <= begin || *at >= end || tokens[*at].text != port.name) continue;
        auto width = declaredWidth(*at);
        if (*at + 1 < end && tokens[*at + 1].text == "[") width = std::nullopt;
        model.portWidths[port.name] = width;
        model.signalWidths[port.name] = width;
    }
    for (const auto& duplicate : unit.duplicatePortDeclarations) {
        model.portWidths[duplicate.name] = std::nullopt;
        model.signalWidths[duplicate.name] = std::nullopt;
    }
    bool inSubprogram = false;
    for (auto i = begin + 2; i < end; ++i) {
        if (tokens[i].text == "function" || tokens[i].text == "task") inSubprogram = true;
        if (tokens[i].text == "endfunction" || tokens[i].text == "endtask") inSubprogram = false;
        if (inSubprogram || (tokens[i].text != "wire" && tokens[i].text != "reg" &&
                            tokens[i].text != "logic" && tokens[i].text != "tri")) continue;
        if (i > begin + 2 && tokens[i - 1].text != ";") continue;
        auto finish = i + 1;
        while (finish < end && tokens[finish].text != ";") ++finish;
        if (finish == end) continue;
        for (auto nameAt = i + 1; nameAt < finish; ++nameAt) {
            if (tokens[nameAt].kind != parser::HdlTokenKind::Identifier ||
                tokens[nameAt].text == "signed" || tokens[nameAt].text == "unsigned") continue;
            const auto name = tokens[nameAt].text;
            const auto next = nameAt + 1;
            if (next < finish && tokens[next].text != "," && tokens[next].text != "=" &&
                tokens[next].text != "[") continue;
            auto width = declaredWidth(nameAt);
            if (next < finish && tokens[next].text == "[") width = std::nullopt;
            if (!model.signalWidths.contains(name)) model.signalWidths.emplace(name, width);
            else model.signalWidths[name] = std::nullopt;
            if (next < finish && tokens[next].text == "=") {
                while (nameAt < finish && tokens[nameAt].text != ",") ++nameAt;
            }
        }
        i = finish;
    }
    return model;
}

std::map<std::string, std::size_t> nativeResetSignalWidths(
    const parser::DesignUnitInfo& unit, const core::SourceFile& source,
    const std::vector<parser::HdlToken>& tokens) {
    std::map<std::string, std::size_t> widths;
    if (const auto model = fixedVerilogModuleWidthModel(unit, source, tokens)) {
        for (const auto& [name, width] : model->signalWidths)
            if (width && *width > 0) widths.emplace(name, *width);
    }
    return widths;
}

std::optional<std::size_t> fixedBoundedExpressionWidth(
    const FixedVerilogModuleWidthModel& model, std::size_t begin, std::size_t end,
    bool multiplicationSum);

std::optional<std::size_t> fixedConnectionActualWidth(
    const FixedVerilogModuleWidthModel& parent, const core::SourceLocation& actual) {
    const auto& tokens = *parent.tokens;
    const auto at = tokenAtLocation(tokens, actual);
    if (!at || *at <= parent.begin || *at >= parent.end) return std::nullopt;
    int parens = 0, brackets = 0, braces = 0;
    auto end = *at;
    for (; end < parent.end; ++end) {
        const auto& text = tokens[end].text;
        if (text == "(") ++parens;
        else if (text == "[") ++brackets;
        else if (text == "{") ++braces;
        else if (text == ")") {
            if (parens == 0 && brackets == 0 && braces == 0) break;
            --parens;
        } else if (text == "]") --brackets;
        else if (text == "}") --braces;
        else if (text == "," && parens == 0 && brackets == 0 && braces == 0) break;
        if (parens < 0 || brackets < 0 || braces < 0) return std::nullopt;
    }
    if (end <= *at || end > parent.end || parens != 0 ||
        brackets != 0 || braces != 0) return std::nullopt;
    return fixedBoundedExpressionWidth(parent, *at, end, false);
}

std::optional<std::size_t> fixedSimpleConditionWidth(
    const FixedVerilogModuleWidthModel& model, std::size_t begin, std::size_t end) {
    const auto& tokens = *model.tokens;
    if (begin >= end || end > model.end) return std::nullopt;
    if (end == begin + 1 && tokens[begin].kind == parser::HdlTokenKind::Number) {
        const auto& literal = tokens[begin].text;
        const auto quote = literal.find('\'');
        if (quote == std::string::npos) return std::nullopt;
        std::size_t width = 0;
        const auto [last, error] = std::from_chars(literal.data(), literal.data() + quote, width);
        return error == std::errc{} && last == literal.data() + quote &&
               width > 0 && width <= 65536 ? std::optional<std::size_t>{width} : std::nullopt;
    }
    if (tokens[begin].kind != parser::HdlTokenKind::Identifier) return std::nullopt;
    const auto found = model.signalWidths.find(tokens[begin].text);
    if (found == model.signalWidths.end() || !found->second) return std::nullopt;
    if (end == begin + 1) return found->second;
    if (tokens[begin + 1].text != "[" || tokens[end - 1].text != "]") return std::nullopt;
    if (end == begin + 4 && fixedDecimalToken(tokens[begin + 2])) return 1;
    if (end == begin + 6 && tokens[begin + 3].text == ":") {
        const auto left = fixedDecimalToken(tokens[begin + 2]);
        const auto right = fixedDecimalToken(tokens[begin + 4]);
        if (left && right)
            return *left > *right ? *left - *right + 1 : *right - *left + 1;
    }
    return std::nullopt;
}

bool fixedBitwiseOperator(std::string_view token) {
    return token == "&" || token == "|" || token == "^" ||
           token == "^~" || token == "~^";
}

bool fixedVerilogLiteralEqualsTwo(const std::vector<parser::HdlToken>& tokens,
                                  std::size_t begin, std::size_t end) {
    while (end > begin + 1 && tokens[begin].text == "(" &&
           tokens[end - 1].text == ")") {
        int depth = 0;
        bool complete = true;
        for (auto at = begin; at < end; ++at) {
            if (tokens[at].text == "(") ++depth;
            else if (tokens[at].text == ")") --depth;
            if (depth == 0 && at + 1 != end) { complete = false; break; }
            if (depth < 0) return false;
        }
        if (!complete || depth != 0) break;
        ++begin;
        --end;
    }
    if (end != begin + 1 ||
        tokens[begin].kind != parser::HdlTokenKind::Number) return false;
    std::string literal;
    for (const char ch : tokens[begin].text)
        if (ch != '_') literal.push_back(static_cast<char>(std::tolower(
            static_cast<unsigned char>(ch))));
    const auto quote = literal.find('\'');
    int base = 10;
    std::string_view digits = literal;
    if (quote != std::string::npos) {
        auto baseAt = quote + 1;
        if (baseAt < literal.size() && literal[baseAt] == 's') ++baseAt;
        if (baseAt >= literal.size()) return false;
        if (literal[baseAt] == 'b') base = 2;
        else if (literal[baseAt] == 'o') base = 8;
        else if (literal[baseAt] == 'd') base = 10;
        else if (literal[baseAt] == 'h') base = 16;
        else return false;
        digits = std::string_view(literal).substr(baseAt + 1);
    }
    if (digits.empty() || digits.find_first_of("xz?") != std::string_view::npos)
        return false;
    unsigned long long value = 0;
    const auto [last, error] = std::from_chars(
        digits.data(), digits.data() + digits.size(), value, base);
    return error == std::errc{} && last == digits.data() + digits.size() && value == 2;
}

std::optional<std::size_t> fixedBoundedExpressionWidth(
    const FixedVerilogModuleWidthModel& model, std::size_t begin, std::size_t end,
    bool multiplicationSum) {
    const auto& tokens = *model.tokens;
    if (begin >= end || end > model.end) return std::nullopt;

    // Strip only a pair of parentheses that encloses the complete expression.
    while (end > begin + 1 && tokens[begin].text == "(" && tokens[end - 1].text == ")") {
        int depth = 0;
        bool enclosesAll = true;
        for (auto i = begin; i < end; ++i) {
            if (tokens[i].text == "(") ++depth;
            else if (tokens[i].text == ")") --depth;
            if (depth == 0 && i + 1 != end) { enclosesAll = false; break; }
            if (depth < 0) return std::nullopt;
        }
        if (!enclosesAll || depth != 0) break;
        ++begin;
        --end;
    }
    if (begin >= end) return std::nullopt;

    // Unary bitwise complement preserves the operand width. Reduction and
    // logical unary operators are deliberately not admitted here.
    if (tokens[begin].text == "~" || tokens[begin].text == "+" ||
        tokens[begin].text == "-")
        return fixedBoundedExpressionWidth(model, begin + 1, end, multiplicationSum);

    // A bounded concatenation has the sum of its directly resolvable items.
    if (tokens[begin].text == "{" && tokens[end - 1].text == "}") {
        // FUN_1808f46c0 flag-zero AST_REPLICATE multiplies the repeated
        // child width by its statically decoded count.
        if (begin + 4 < end &&
            tokens[begin + 1].kind == parser::HdlTokenKind::Number &&
            tokens[begin + 2].text == "{" && tokens[end - 2].text == "}") {
            int depth = 0;
            bool closesAtEnd = false;
            for (auto i = begin + 2; i < end - 1; ++i) {
                if (tokens[i].text == "{") ++depth;
                else if (tokens[i].text == "}" && --depth == 0) {
                    closesAtEnd = i == end - 2;
                    break;
                }
                if (depth < 0) return std::nullopt;
            }
            const auto count = fixedDecimalToken(tokens[begin + 1]);
            // AST_REPLICATE forces the helper flag back to zero for its
            // repeated child even when its caller selected flag one.
            const auto width = closesAtEnd
                ? fixedBoundedExpressionWidth(model, begin + 3, end - 2, false)
                : std::nullopt;
            if (!count || !width || *count == 0 ||
                *count > 65536 / *width) return std::nullopt;
            return *count * *width;
        }
        std::size_t total = 0;
        std::size_t item = begin + 1;
        int parens = 0;
        int braces = 0;
        for (auto i = item; i < end - 1; ++i) {
            if (tokens[i].text == "(") ++parens;
            else if (tokens[i].text == ")") --parens;
            else if (tokens[i].text == "{") ++braces;
            else if (tokens[i].text == "}") --braces;
            if (parens < 0 || braces < 0) return std::nullopt;
            if (tokens[i].text != "," || parens != 0 || braces != 0) continue;
            const auto width = fixedBoundedExpressionWidth(
                model, item, i, multiplicationSum);
            if (!width || total > 65536 - *width) return std::nullopt;
            total += *width;
            item = i + 1;
        }
        const auto width = fixedBoundedExpressionWidth(
            model, item, end - 1, multiplicationSum);
        if (!width || total > 65536 - *width) return std::nullopt;
        total += *width;
        return total == 0 ? std::nullopt : std::optional<std::size_t>{total};
    }

    // Binary bitwise expressions have the maximum operand width. Splitting at
    // any top-level bitwise operator is sufficient because all supported
    // operators share that width rule, even though their precedences differ.
    int parens = 0;
    int braces = 0;
    int brackets = 0;
    for (auto i = begin; i < end; ++i) {
        if (tokens[i].text == "(") ++parens;
        else if (tokens[i].text == ")") --parens;
        else if (tokens[i].text == "{") ++braces;
        else if (tokens[i].text == "}") --braces;
        else if (tokens[i].text == "[") ++brackets;
        else if (tokens[i].text == "]") --brackets;
        else if (parens == 0 && braces == 0 && brackets == 0 &&
                 fixedBitwiseOperator(tokens[i].text)) {
            const auto left = fixedBoundedExpressionWidth(
                model, begin, i, multiplicationSum);
            const auto right = fixedBoundedExpressionWidth(
                model, i + 1, end, multiplicationSum);
            if (!left || !right) return std::nullopt;
            return std::max(*left, *right);
        }
        if (parens < 0 || braces < 0 || brackets < 0) return std::nullopt;
    }
    if (parens != 0 || braces != 0 || brackets != 0) return std::nullopt;

    // The recovered fifth helper argument is zero. In this mode nested
    // ADD/SUB/MUL/DIV/MOD take max(child widths), while POW propagates the
    // first child. Split the lowest-precedence top-level arithmetic root.
    struct ArithmeticRoot {
        std::size_t token;
        std::size_t tokenCount;
        bool power;
        bool multiplication;
    };
    const auto binarySign = [&](std::size_t token) {
        if (token == begin) return false;
        const auto& previous = tokens[token - 1].text;
        return previous != "+" && previous != "-" && previous != "*" &&
            previous != "/" && previous != "%" && previous != "(" &&
            previous != "{" && previous != "," && previous != "?" &&
            previous != ":";
    };
    std::optional<ArithmeticRoot> additive;
    std::optional<ArithmeticRoot> multiplicative;
    std::optional<ArithmeticRoot> power;
    parens = braces = brackets = 0;
    for (auto i = begin; i < end; ++i) {
        const auto& text = tokens[i].text;
        if (text == "(") ++parens;
        else if (text == ")") --parens;
        else if (text == "{") ++braces;
        else if (text == "}") --braces;
        else if (text == "[") ++brackets;
        else if (text == "]") --brackets;
        else if (parens == 0 && braces == 0 && brackets == 0) {
            if ((text == "+" || text == "-") && binarySign(i)) {
                additive = ArithmeticRoot{i, 1, false, false};
            } else if (text == "/" || text == "%") {
                multiplicative = ArithmeticRoot{i, 1, false, false};
            } else if (text == "**") {
                if (!power) power = ArithmeticRoot{i, 1, true, false};
            } else if (text == "*") {
                const bool previousStar = i > begin && tokens[i - 1].text == "*";
                const bool nextStar = i + 1 < end && tokens[i + 1].text == "*";
                if (!previousStar && !nextStar)
                    multiplicative = ArithmeticRoot{i, 1, false, true};
                else if (nextStar && !power)
                    power = ArithmeticRoot{i, 2, true, false};
            }
        }
    }
    const auto root = additive ? additive : (multiplicative ? multiplicative : power);
    if (root && root->token > begin && root->token + root->tokenCount < end) {
        const auto left = fixedBoundedExpressionWidth(
            model, begin, root->token, multiplicationSum);
        const auto right = fixedBoundedExpressionWidth(
            model, root->token + root->tokenCount, end, multiplicationSum);
        if (!left || !right) return std::nullopt;
        if (root->power) return left;
        if (root->multiplication && multiplicationSum) {
            // In the recovered flag-one AST_MUL branch, the first direct
            // constant child whose decoded value is exactly two contributes
            // one bit; all other direct children contribute their inferred
            // widths. Preserve that unusual vendor special case.
            auto leftContribution = *left;
            auto rightContribution = *right;
            const bool leftTwo = fixedVerilogLiteralEqualsTwo(
                tokens, begin, root->token);
            if (leftTwo) leftContribution = 1;
            else if (fixedVerilogLiteralEqualsTwo(
                         tokens, root->token + root->tokenCount, end))
                rightContribution = 1;
            if (leftContribution > 65536 - rightContribution) return std::nullopt;
            return leftContribution + rightContribution;
        }
        return std::max(*left, *right);
    }
    return fixedSimpleConditionWidth(model, begin, end);
}

std::optional<std::size_t> fixedBoundedLogicalOperandWidth(
    const FixedVerilogModuleWidthModel& model, std::size_t begin, std::size_t end) {
    return fixedBoundedExpressionWidth(model, begin, end, false);
}

void checkNativeVerilogVectorLogical(AnalysisResult& result,
                                     std::span<const core::SourceFile> sources,
                                     const AnalysisOptions& options) {
    // Native 8.8 run selects AST_ALWAYS/assignment roots, then AST logical
    // operator kinds 0x45/0x46/0x47; any child with inferred width > 1 warns
    // once for that operator. The call at 0x18057a157 passes fifth argument
    // zero, so bounded arithmetic/concat/replicate children use the recovered
    // flag-zero width model. A nested logical child itself has width one.
    // ev_025e8620d4259b128b7931578411d267230b470f26259611fa352340b7fd16b4
    for (const auto& source : sources) {
        if (!applicable(source, "GJB10157-2021")) continue;
        const auto preprocessed = parser::preprocess(
            source, {options.includeDirectories, options.defines});
        if (!preprocessed.issues.empty()) continue;
        const auto tokens = standardTokens(preprocessed.source);
        const auto units = parser::parseDesignUnits(preprocessed.source);
        for (const auto& unit : units) {
            if (unit.kind != parser::DesignUnitKind::VerilogModule) continue;
            const bool overridden = std::ranges::any_of(result.structure, [&](const auto& parent) {
                return std::ranges::any_of(parent.instances, [&](const auto& instance) {
                    return instance.target == unit.name &&
                        (instance.hasNamedParameterOverrides || instance.hasPositionalParameterOverrides);
                });
            });
            const auto model = fixedVerilogModuleWidthModel(
                unit, preprocessed.source, tokens, !overridden);
            if (!model) continue;
            std::size_t body = model->begin + 2;
            int parentheses = 0;
            for (; body < model->end; ++body) {
                if (tokens[body].text == "(") ++parentheses;
                else if (tokens[body].text == ")") --parentheses;
                else if (tokens[body].text == ";" && parentheses == 0) { ++body; break; }
            }
            if (body >= model->end) continue;
            const auto allowedExpressionToken = [&](std::size_t at) {
                const auto& text = tokens[at].text;
                return tokens[at].kind == parser::HdlTokenKind::Identifier ||
                    tokens[at].kind == parser::HdlTokenKind::Number ||
                    text == "+" || text == "-" || text == "*" || text == "**" || text == "/" ||
                    text == "%" || text == "~" || text == "&" || text == "|" ||
                    text == "^" || text == "^~" || text == "~^" || text == "!" ||
                    text == "&&" || text == "||" || text == "(" || text == ")" ||
                    text == "[" || text == "]" || text == "{" || text == "}";
            };
            const auto logicalIsland = [&](std::size_t op) {
                std::size_t first = op;
                int parens = 0, brackets = 0, braces = 0;
                while (first > body) {
                    const auto candidate = first - 1;
                    const auto& text = tokens[candidate].text;
                    if (text == ")") ++parens;
                    else if (text == "]") ++brackets;
                    else if (text == "}") ++braces;
                    else if (text == "(") { if (parens == 0) break; --parens; }
                    else if (text == "[") { if (brackets == 0) break; --brackets; }
                    else if (text == "{") { if (braces == 0) break; --braces; }
                    else if (parens == 0 && brackets == 0 && braces == 0 &&
                             !allowedExpressionToken(candidate)) break;
                    first = candidate;
                }
                std::size_t last = op + 1;
                parens = brackets = braces = 0;
                while (last < model->end) {
                    const auto& text = tokens[last].text;
                    if (text == "(") ++parens;
                    else if (text == "[") ++brackets;
                    else if (text == "{") ++braces;
                    else if (text == ")") { if (parens == 0) break; --parens; }
                    else if (text == "]") { if (brackets == 0) break; --brackets; }
                    else if (text == "}") { if (braces == 0) break; --braces; }
                    else if (parens == 0 && brackets == 0 && braces == 0 &&
                             !allowedExpressionToken(last)) break;
                    ++last;
                }
                return std::pair{first, last};
            };
            const auto stripLogicalParentheses = [&](std::size_t& first,
                                                      std::size_t& last) {
                while (first < last && tokens[first].text == "(") {
                    int depth = 0;
                    std::size_t after = first;
                    for (; after < last; ++after) {
                        if (tokens[after].text == "(") ++depth;
                        else if (tokens[after].text == ")" && --depth == 0) {
                            ++after;
                            break;
                        }
                    }
                    if (after != last) break;
                    ++first;
                    --last;
                }
            };
            struct LogicalRoot { std::size_t token; bool unary; };
            const auto splitLogical = [&](std::size_t first, std::size_t last)
                    -> std::optional<LogicalRoot> {
                stripLogicalParentheses(first, last);
                std::optional<LogicalRoot> logicalOr;
                std::optional<LogicalRoot> logicalAnd;
                int parens = 0, brackets = 0, braces = 0;
                for (auto at = first; at < last; ++at) {
                    const auto& text = tokens[at].text;
                    if (text == "(") ++parens;
                    else if (text == ")") --parens;
                    else if (text == "[") ++brackets;
                    else if (text == "]") --brackets;
                    else if (text == "{") ++braces;
                    else if (text == "}") --braces;
                    else if (parens == 0 && brackets == 0 && braces == 0) {
                        if (text == "||") logicalOr = LogicalRoot{at, false};
                        else if (text == "&&") logicalAnd = LogicalRoot{at, false};
                    }
                }
                if (logicalOr) return logicalOr;
                if (logicalAnd) return logicalAnd;
                if (first < last && tokens[first].text == "!")
                    return LogicalRoot{first, true};
                return std::nullopt;
            };
            std::set<std::size_t> emittedLogical;
            const auto analyzeLogical = [&](auto&& self, std::size_t first,
                                            std::size_t last) -> void {
                stripLogicalParentheses(first, last);
                const auto root = splitLogical(first, last);
                if (!root) return;
                const auto childWidth = [&](std::size_t childBegin,
                                            std::size_t childEnd) {
                    std::size_t strippedBegin = childBegin;
                    std::size_t strippedEnd = childEnd;
                    stripLogicalParentheses(strippedBegin, strippedEnd);
                    if (splitLogical(strippedBegin, strippedEnd))
                        return std::optional<std::size_t>{1};
                    return fixedBoundedLogicalOperandWidth(
                        *model, strippedBegin, strippedEnd);
                };
                const auto right = childWidth(root->token + 1, last);
                const auto left = root->unary ? std::optional<std::size_t>{}
                                              : childWidth(first, root->token);
                if (((right && *right > 1) || (left && *left > 1)) &&
                    emittedLogical.insert(root->token).second) {
                    emit(result, "GJB10157.8.8", source,
                         tokens[root->token].line, tokens[root->token].column,
                         "逻辑运算符 " + tokens[root->token].text +
                             " 的已解析操作数是多位向量");
                }
                if (!root->unary) self(self, first, root->token);
                self(self, root->token + 1, last);
            };
            std::set<std::pair<std::size_t, std::size_t>> analyzedLogicalIslands;
            for (auto i = body; i < model->end; ++i) {
                if (tokens[i].text != "&&" && tokens[i].text != "||" &&
                    tokens[i].text != "!") continue;
                const auto island = logicalIsland(i);
                if (analyzedLogicalIslands.insert(island).second)
                    analyzeLogical(analyzeLogical, island.first, island.second);
            }
        }
    }
}

void checkNativeVerilogConditionWidths(AnalysisResult& result,
                                        std::span<const core::SourceFile> sources) {
    // Native 9.2.10 slot 0 infers the condition width for AST 0x57/0x2d
    // (if branch) and AST 0x48 (conditional operator); width != 1 warns.
    // Both recovered helper calls pass fifth argument zero. This source
    // adapter therefore admits the same bounded flag-zero expression subset
    // used by 8.8, while unknown expressions remain review-required.
    // ev_e0082122fcb1468be95b5422fb45284bdf8fa25597cb3114b809da50443e7a94
    std::map<std::string, std::vector<parser::HdlToken>> tokenCache;
    for (const auto& source : sources)
        if (applicable(source, "GJB10157-2021"))
            tokenCache[source.path().lexically_normal().generic_string()] = standardTokens(source);
    for (const auto& unit : result.structure) {
        if (unit.kind != parser::DesignUnitKind::VerilogModule) continue;
        const auto source = std::ranges::find_if(sources, [&](const auto& candidate) {
            return candidate.path().lexically_normal() == unit.location.file.lexically_normal();
        });
        if (source == sources.end()) continue;
        const auto cached = tokenCache.find(source->path().lexically_normal().generic_string());
        if (cached == tokenCache.end()) continue;
        const auto model = fixedVerilogModuleWidthModel(unit, *source, cached->second);
        if (!model) continue;
        const auto& tokens = cached->second;
        const auto conditionWidth = [&](std::size_t begin, std::size_t end)
                -> std::optional<std::size_t> {
            while (end > begin + 1 && tokens[begin].text == "(" &&
                   tokens[end - 1].text == ")") {
                int depth = 0;
                bool enclosesAll = true;
                for (auto at = begin; at < end; ++at) {
                    if (tokens[at].text == "(") ++depth;
                    else if (tokens[at].text == ")") --depth;
                    if (depth == 0 && at + 1 != end) { enclosesAll = false; break; }
                    if (depth < 0) return std::nullopt;
                }
                if (!enclosesAll || depth != 0) break;
                ++begin;
                --end;
            }
            if (begin >= end) return std::nullopt;
            if (tokens[begin].text == "!") return std::size_t{1};
            int parens = 0, brackets = 0, braces = 0;
            for (auto at = begin; at < end; ++at) {
                const auto& text = tokens[at].text;
                if (text == "(") ++parens;
                else if (text == ")") --parens;
                else if (text == "[") ++brackets;
                else if (text == "]") --brackets;
                else if (text == "{") ++braces;
                else if (text == "}") --braces;
                else if (parens == 0 && brackets == 0 && braces == 0 &&
                         (text == "&&" || text == "||" || text == "==" ||
                          text == "!=" || text == "===" || text == "!==" ||
                          text == "<" || text == "<=" || text == ">" ||
                          text == ">=")) {
                    return std::size_t{1};
                }
            }
            return fixedBoundedLogicalOperandWidth(*model, begin, end);
        };
        for (auto i = model->begin + 1; i + 2 < model->end; ++i) {
            if (tokens[i].text == "if" && tokens[i + 1].text == "(") {
                int depth = 1;
                auto close = i + 2;
                for (; close < model->end && depth != 0; ++close) {
                    if (tokens[close].text == "(") ++depth;
                    else if (tokens[close].text == ")") --depth;
                }
                if (close == model->end) continue;
                const auto width = conditionWidth(i + 2, close - 1);
                if (width && *width != 1)
                    emit(result, "GJB10157.9.2.10", *source,
                         tokens[i].line, tokens[i].column,
                         "if 条件为 " + std::to_string(*width) + " 位向量，应使用 1 位条件");
            }
            if (tokens[i].text != "?" || i <= model->begin + 1) continue;
            auto begin = i;
            int parens = 0, brackets = 0, braces = 0;
            while (begin > model->begin + 1) {
                const auto candidate = begin - 1;
                const auto& text = tokens[candidate].text;
                if (text == ")") ++parens;
                else if (text == "]") ++brackets;
                else if (text == "}") ++braces;
                else if (text == "(") {
                    if (parens == 0) break;
                    --parens;
                } else if (text == "[") {
                    if (brackets == 0) break;
                    --brackets;
                } else if (text == "{") {
                    if (braces == 0) break;
                    --braces;
                } else if (parens == 0 && brackets == 0 && braces == 0 &&
                           (text == "=" || text == "," || text == ":" ||
                            text == "?" || text == ";")) {
                    break;
                }
                begin = candidate;
            }
            if (begin <= model->begin) continue;
            const auto& before = tokens[begin - 1].text;
            if (before != "=" && before != "(" && before != "," &&
                before != ":" && before != "?") continue;
            const auto width = conditionWidth(begin, i);
            if (width && *width != 1)
                emit(result, "GJB10157.9.2.10", *source,
                     tokens[begin].line, tokens[begin].column,
                     "条件运算符条件为 " + std::to_string(*width) +
                     " 位向量，应使用 1 位条件");
        }
    }
}

void checkNativeVerilogAssignmentWidths(AnalysisResult& result,
                                         std::span<const core::SourceFile> sources,
                                         const AnalysisOptions& options) {
    // Native 9.1.2 slot 0 infers the LHS with flag zero. A non-ternary RHS is
    // first inferred with flag zero; on mismatch it is inferred again with
    // flag one, and a warning survives only when both widths differ from the
    // LHS. For AST_TERNARY, the true and false children are compared
    // independently with flag zero. The only recovered flag-one difference is
    // AST_MUL: it sums child widths instead of taking their maximum.
    // ev_ba7c5d2230d301bad7129b407ba296b167e5e1bd805b43f7d517d5fb85be2c25
    for (const auto& source : sources) {
        if (!applicable(source, "GJB10157-2021")) continue;
        const auto preprocessed = parser::preprocess(
            source, {options.includeDirectories, options.defines});
        if (!preprocessed.issues.empty()) continue;
        const auto tokens = standardTokens(preprocessed.source);
        for (const auto& unit : parser::parseDesignUnits(preprocessed.source)) {
            if (unit.kind != parser::DesignUnitKind::VerilogModule) continue;
            const auto model = fixedVerilogModuleWidthModel(
                unit, preprocessed.source, tokens);
            if (!model) continue;
            std::size_t body = model->begin + 2;
            int headerParens = 0;
            for (; body < model->end; ++body) {
                if (tokens[body].text == "(") ++headerParens;
                else if (tokens[body].text == ")") --headerParens;
                else if (tokens[body].text == ";" && headerParens == 0) {
                    ++body;
                    break;
                }
            }
            const auto matchingOpen = [&](std::size_t close,
                                          std::string_view left,
                                          std::string_view right)
                    -> std::optional<std::size_t> {
                int depth = 0;
                for (auto cursor = close + 1; cursor > body;) {
                    --cursor;
                    if (tokens[cursor].text == right) ++depth;
                    else if (tokens[cursor].text == left && --depth == 0)
                        return cursor;
                }
                return std::nullopt;
            };
            const auto lhsBegin = [&](std::size_t assignment)
                    -> std::optional<std::size_t> {
                if (assignment <= body) return std::nullopt;
                const auto last = assignment - 1;
                if (tokens[last].text == "]") {
                    const auto open = matchingOpen(last, "[", "]");
                    if (!open || *open == body ||
                        tokens[*open - 1].kind != parser::HdlTokenKind::Identifier)
                        return std::nullopt;
                    return *open - 1;
                }
                if (tokens[last].text == "}")
                    return matchingOpen(last, "{", "}");
                if (tokens[last].text == ")")
                    return matchingOpen(last, "(", ")");
                return tokens[last].kind == parser::HdlTokenKind::Identifier
                    ? std::optional<std::size_t>{last} : std::nullopt;
            };
            const auto rhsEnd = [&](std::size_t begin)
                    -> std::optional<std::size_t> {
                int parens = 0, brackets = 0, braces = 0;
                for (auto at = begin; at < model->end; ++at) {
                    const auto& text = tokens[at].text;
                    if (text == "(") ++parens;
                    else if (text == ")") --parens;
                    else if (text == "[") ++brackets;
                    else if (text == "]") --brackets;
                    else if (text == "{") ++braces;
                    else if (text == "}") --braces;
                    else if (text == ";" && parens == 0 && brackets == 0 && braces == 0)
                        return at;
                    if (parens < 0 || brackets < 0 || braces < 0) return std::nullopt;
                }
                return std::nullopt;
            };
            const auto stripParentheses = [&](std::size_t& first,
                                               std::size_t& last) {
                while (last > first + 1 && tokens[first].text == "(" &&
                       tokens[last - 1].text == ")") {
                    int depth = 0;
                    bool complete = true;
                    for (auto at = first; at < last; ++at) {
                        if (tokens[at].text == "(") ++depth;
                        else if (tokens[at].text == ")") --depth;
                        if (depth == 0 && at + 1 != last) { complete = false; break; }
                        if (depth < 0) { complete = false; break; }
                    }
                    if (!complete || depth != 0) break;
                    ++first;
                    --last;
                }
            };
            const auto ternarySplit = [&](std::size_t first, std::size_t last)
                    -> std::optional<std::pair<std::size_t, std::size_t>> {
                stripParentheses(first, last);
                int parens = 0, brackets = 0, braces = 0, ternaries = 0;
                std::optional<std::size_t> question;
                for (auto at = first; at < last; ++at) {
                    const auto& text = tokens[at].text;
                    if (text == "(") ++parens;
                    else if (text == ")") --parens;
                    else if (text == "[") ++brackets;
                    else if (text == "]") --brackets;
                    else if (text == "{") ++braces;
                    else if (text == "}") --braces;
                    else if (parens == 0 && brackets == 0 && braces == 0 && text == "?") {
                        if (!question) question = at;
                        ++ternaries;
                    } else if (parens == 0 && brackets == 0 && braces == 0 &&
                               text == ":" && question && --ternaries == 0) {
                        return std::pair{*question, at};
                    }
                }
                return std::nullopt;
            };
            for (auto assignment = body + 1;
                 assignment + 1 < model->end; ++assignment) {
                if (tokens[assignment].text != "=" &&
                    tokens[assignment].text != "<=") continue;
                const auto lhsFirst = lhsBegin(assignment);
                const auto rhsLast = rhsEnd(assignment + 1);
                if (!lhsFirst || !rhsLast || assignment + 1 >= *rhsLast) continue;
                const auto lhsWidth = fixedBoundedExpressionWidth(
                    *model, *lhsFirst, assignment, false);
                if (!lhsWidth) continue;
                const auto split = ternarySplit(assignment + 1, *rhsLast);
                if (split) {
                    const std::array branches{
                        std::pair{split->first + 1, split->second},
                        std::pair{split->second + 1, *rhsLast}};
                    for (std::size_t branch = 0; branch < branches.size(); ++branch) {
                        const auto width = fixedBoundedExpressionWidth(
                            *model, branches[branch].first, branches[branch].second, false);
                        if (!width || *width == *lhsWidth) continue;
                        emit(result, "GJB10157.9.1.2", preprocessed.source,
                             tokens[branches[branch].first].line,
                             tokens[branches[branch].first].column,
                             std::string(branch == 0 ? "三目表达式真分支" : "三目表达式假分支") +
                                 "为 " + std::to_string(*width) + " 位，赋值目标为 " +
                                 std::to_string(*lhsWidth) + " 位");
                    }
                    continue;
                }
                const auto flagZero = fixedBoundedExpressionWidth(
                    *model, assignment + 1, *rhsLast, false);
                if (!flagZero || *flagZero == *lhsWidth) continue;
                const auto flagOne = fixedBoundedExpressionWidth(
                    *model, assignment + 1, *rhsLast, true);
                if (!flagOne || *flagOne == *lhsWidth) continue;
                emit(result, "GJB10157.9.1.2", preprocessed.source,
                     tokens[assignment].line, tokens[assignment].column,
                     "赋值两侧位宽不一致：目标为 " +
                         std::to_string(*lhsWidth) + " 位，右值 flag-zero/flag-one 宽度为 " +
                         std::to_string(*flagZero) + "/" +
                         std::to_string(*flagOne) + " 位");
            }
        }
    }
}

void checkNativeVerilogCaseFallback(AnalysisResult& result,
                                    std::span<const core::SourceFile> sources,
                                    const AnalysisOptions& options) {
    // Native collector FUN_180716130 admits nonempty AST_CASE(0x57) nodes only
    // when byte +0x183 is zero, excluding the frontend's if-lowered CASE form.
    // FUN_1806df3d0 accepts a COND/CONDX/CONDZ(0x58..0x5a) child when its first
    // child is AST_DEFAULT(0x5b), or when its only child is AST_BLOCK(0x54).
    // The Yosys parser source corroborates the source mapping: `default:`
    // produces DEFAULT+BLOCK, while the legal `default <statement>` form
    // without a colon produces BLOCK alone. Emit once at each case node.
    // ev_e69cc1c5a34d9cccdde7754f61613b89544679ed77ae679a1ee94a84b9dff82c
    // ev_8e1f2c6a9a8140cca8d40a38d0ea81fab5d70826fe87a63eb8e495f0b446e2ae
    for (const auto& source : sources) {
        if (!applicable(source, "GJB10157-2021")) continue;
        const auto preprocessed = parser::preprocess(
            source, {options.includeDirectories, options.defines});
        if (!preprocessed.issues.empty()) continue;
        const auto tokens = standardTokens(preprocessed.source);
        for (std::size_t caseToken = 0; caseToken + 2 < tokens.size(); ++caseToken) {
            const auto& kind = tokens[caseToken].text;
            if ((kind != "case" && kind != "casez" && kind != "casex") ||
                tokens[caseToken + 1].text != "(") continue;
            const auto selectorClose = closeParen(tokens, caseToken + 1);
            if (selectorClose == tokens.size()) continue;
            int depth = 1;
            auto caseEnd = selectorClose + 1;
            for (; caseEnd < tokens.size(); ++caseEnd) {
                if (tokens[caseEnd].text == "case" || tokens[caseEnd].text == "casez" ||
                    tokens[caseEnd].text == "casex") ++depth;
                else if (tokens[caseEnd].text == "endcase" && --depth == 0) break;
            }
            if (caseEnd == tokens.size()) continue;

            bool fallback = false;
            auto cursor = selectorClose + 1;
            while (cursor < caseEnd) {
                while (cursor < caseEnd && tokens[cursor].text == ";") ++cursor;
                if (cursor >= caseEnd) break;
                // Verilog permits both `default: statement` and
                // `default statement`. The latter is the native one-BLOCK
                // branch and was missed by the former generic mapping.
                if (tokens[cursor].text == "default") {
                    fallback = true;
                    break;
                }
                int round = 0, square = 0, brace = 0, ternary = 0;
                std::optional<std::size_t> colon;
                for (auto at = cursor; at < caseEnd; ++at) {
                    const auto& text = tokens[at].text;
                    if (text == "(") ++round;
                    else if (text == ")") --round;
                    else if (text == "[") ++square;
                    else if (text == "]") --square;
                    else if (text == "{") ++brace;
                    else if (text == "}") --brace;
                    else if (round == 0 && square == 0 && brace == 0 && text == "?") ++ternary;
                    else if (round == 0 && square == 0 && brace == 0 && text == ":") {
                        if (ternary > 0) --ternary;
                        else { colon = at; break; }
                    }
                    if (round < 0 || square < 0 || brace < 0) break;
                }
                if (!colon) break;
                const auto next = statementEnd(tokens, *colon + 1);
                cursor = next > *colon + 1 ? next : *colon + 1;
            }
            if (!fallback) {
                const auto& token = tokens[caseToken];
                emit(result, "GJB10157.9.2.1", preprocessed.source,
                     token.line, token.column,
                     "case 语句缺少 default 回退分支");
            }
        }
    }
}

struct NativeCaseOverlapLabel {
    enum class Kind { Constant, Name } kind{Kind::Constant};
    // Yosys RTLIL state bytes used by the recovered native comparison:
    // 0/1 are binary values, 2/3 preserve X/Z, and 4 is don't-care.
    // Bits are stored least-significant first, matching the native vector.
    std::vector<unsigned char> states;
    std::string name;
    std::string display;
    std::size_t token{};
};

std::optional<NativeCaseOverlapLabel> nativeCaseOverlapLabel(
    const std::vector<parser::HdlToken>& tokens, std::size_t begin,
    std::size_t end, std::string_view caseKind) {
    if (begin + 1 != end) return std::nullopt;
    const auto& token = tokens[begin];
    if (token.kind == parser::HdlTokenKind::Identifier) {
        return NativeCaseOverlapLabel{NativeCaseOverlapLabel::Kind::Name, {},
                                      token.text, token.text, begin};
    }
    if (token.kind != parser::HdlTokenKind::Number) return std::nullopt;

    auto text = lower(token.text);
    text.erase(std::remove(text.begin(), text.end(), '_'), text.end());
    NativeCaseOverlapLabel label;
    label.kind = NativeCaseOverlapLabel::Kind::Constant;
    label.display = token.text;
    label.token = begin;

    const auto appendBinaryValue = [&](unsigned value, unsigned count) {
        for (unsigned bit = 0; bit < count; ++bit)
            label.states.push_back(static_cast<unsigned char>((value >> bit) & 1U));
    };
    const auto apostrophe = text.find('\'');
    if (apostrophe == std::string::npos) {
        unsigned long long value = 0;
        const auto parsed = std::from_chars(text.data(), text.data() + text.size(), value);
        if (parsed.ec != std::errc{} || parsed.ptr != text.data() + text.size())
            return std::nullopt;
        // Unsized integer constants are represented by the Verilog frontend at
        // integer width.  This bounded adapter handles nonnegative values only.
        for (unsigned bit = 0; bit < 32; ++bit)
            label.states.push_back(static_cast<unsigned char>((value >> bit) & 1ULL));
        return label;
    }
    if (apostrophe == 0) return std::nullopt;
    std::size_t width = 0;
    const auto widthParsed = std::from_chars(
        text.data(), text.data() + apostrophe, width);
    if (widthParsed.ec != std::errc{} ||
        widthParsed.ptr != text.data() + apostrophe || width == 0 || width > 4096)
        return std::nullopt;
    auto baseAt = apostrophe + 1;
    if (baseAt < text.size() && text[baseAt] == 's') ++baseAt;
    if (baseAt >= text.size()) return std::nullopt;
    const char base = text[baseAt++];
    if (baseAt >= text.size()) return std::nullopt;

    const auto stateForMeta = [&](char digit) -> std::optional<unsigned char> {
        if (digit == '?') return 4;
        if (digit == 'z') return caseKind == "casez" || caseKind == "casex" ? 4 : 3;
        if (digit == 'x') return caseKind == "casex" ? 4 : 2;
        return std::nullopt;
    };
    if (base == 'd') {
        unsigned long long value = 0;
        const auto parsed = std::from_chars(
            text.data() + baseAt, text.data() + text.size(), value);
        if (parsed.ec != std::errc{} || parsed.ptr != text.data() + text.size())
            return std::nullopt;
        for (std::size_t bit = 0; bit < width; ++bit)
            label.states.push_back(static_cast<unsigned char>(
                bit < 64 ? ((value >> bit) & 1ULL) : 0));
        return label;
    }
    const unsigned digitBits = base == 'b' ? 1U : base == 'o' ? 3U : base == 'h' ? 4U : 0U;
    if (digitBits == 0) return std::nullopt;
    for (auto position = text.size(); position-- > baseAt;) {
        const char digit = text[position];
        if (const auto state = stateForMeta(digit)) {
            label.states.insert(label.states.end(), digitBits, *state);
            continue;
        }
        unsigned value = 0;
        if (digit >= '0' && digit <= '9') value = static_cast<unsigned>(digit - '0');
        else if (digit >= 'a' && digit <= 'f') value = 10U + static_cast<unsigned>(digit - 'a');
        else return std::nullopt;
        if (value >= (1U << digitBits)) return std::nullopt;
        appendBinaryValue(value, digitBits);
    }
    label.states.resize(width, 0);
    return label;
}

bool nativeCaseLabelsOverlap(const NativeCaseOverlapLabel& left,
                             const NativeCaseOverlapLabel& right) {
    if (left.kind != right.kind) return false;
    if (left.kind == NativeCaseOverlapLabel::Kind::Name)
        return left.name == right.name;
    const auto shared = std::min(left.states.size(), right.states.size());
    for (std::size_t bit = 0; bit < shared; ++bit) {
        const auto a = left.states[bit];
        const auto b = right.states[bit];
        if (a != 4 && b != 4 && a != b) return false;
    }
    const auto& longer = left.states.size() > right.states.size()
        ? left.states : right.states;
    for (auto bit = shared; bit < longer.size(); ++bit)
        if (longer[bit] == 1) return false;
    return true;
}

void checkNativeVerilogCaseOverlap(AnalysisResult& result,
                                   std::span<const core::SourceFile> sources,
                                   const AnalysisOptions& options) {
    // Native FUN_180714dd0 gathers every label child before AST_BLOCK/DEFAULT
    // from AST_COND/CONDX/CONDZ groups, then compares every ordered pair. For
    // constants, encoded state 4 is a wildcard. Unequal widths compare their
    // shared low bits and remain compatible unless an extra high bit is 1.
    // Nonconstant labels use an exact name comparison. Each matching pair
    // emits at the later label; there is no first-match break.
    // ev_2df0538f0bc68b70437ba5da70a47c50a5c70f900c63bb943a806baf7392318f
    for (const auto& source : sources) {
        if (!applicable(source, "GJB10157-2021")) continue;
        const auto preprocessed = parser::preprocess(
            source, {options.includeDirectories, options.defines});
        if (!preprocessed.issues.empty()) continue;
        const auto tokens = standardTokens(preprocessed.source);
        for (std::size_t caseToken = 0; caseToken + 2 < tokens.size(); ++caseToken) {
            const auto& caseKind = tokens[caseToken].text;
            if ((caseKind != "case" && caseKind != "casez" && caseKind != "casex") ||
                tokens[caseToken + 1].text != "(") continue;
            const auto selectorClose = closeParen(tokens, caseToken + 1);
            if (selectorClose == tokens.size()) continue;
            int depth = 1;
            auto caseEnd = selectorClose + 1;
            for (; caseEnd < tokens.size(); ++caseEnd) {
                if (tokens[caseEnd].text == "case" || tokens[caseEnd].text == "casez" ||
                    tokens[caseEnd].text == "casex") ++depth;
                else if (tokens[caseEnd].text == "endcase" && --depth == 0) break;
            }
            if (caseEnd == tokens.size()) continue;

            std::vector<NativeCaseOverlapLabel> labels;
            auto cursor = selectorClose + 1;
            while (cursor < caseEnd) {
                while (cursor < caseEnd && tokens[cursor].text == ";") ++cursor;
                if (cursor >= caseEnd) break;
                const auto labelsBegin = cursor;
                int round = 0, square = 0, brace = 0, ternary = 0;
                std::optional<std::size_t> colon;
                for (; cursor < caseEnd; ++cursor) {
                    const auto& text = tokens[cursor].text;
                    if (text == "(") ++round;
                    else if (text == ")") --round;
                    else if (text == "[") ++square;
                    else if (text == "]") --square;
                    else if (text == "{") ++brace;
                    else if (text == "}") --brace;
                    else if (round == 0 && square == 0 && brace == 0 && text == "?") ++ternary;
                    else if (round == 0 && square == 0 && brace == 0 && text == ":") {
                        if (ternary > 0) --ternary;
                        else { colon = cursor; break; }
                    }
                    if (round < 0 || square < 0 || brace < 0) break;
                }
                if (!colon || labelsBegin >= *colon) break;
                if (tokens[labelsBegin].text != "default") {
                    auto labelBegin = labelsBegin;
                    round = square = brace = ternary = 0;
                    for (auto at = labelsBegin; at <= *colon; ++at) {
                        if (at < *colon) {
                            const auto& text = tokens[at].text;
                            if (text == "(") ++round;
                            else if (text == ")") --round;
                            else if (text == "[") ++square;
                            else if (text == "]") --square;
                            else if (text == "{") ++brace;
                            else if (text == "}") --brace;
                            else if (round == 0 && square == 0 && brace == 0 && text == "?") ++ternary;
                            else if (round == 0 && square == 0 && brace == 0 &&
                                     text == ":" && ternary > 0) --ternary;
                        }
                        if (at != *colon && !(tokens[at].text == "," && round == 0 &&
                            square == 0 && brace == 0 && ternary == 0)) continue;
                        if (const auto label = nativeCaseOverlapLabel(
                                tokens, labelBegin, at, caseKind)) {
                            for (const auto& previous : labels) {
                                if (!nativeCaseLabelsOverlap(previous, *label)) continue;
                                const auto& atToken = tokens[label->token];
                                emit(result, "GJB10157.9.2.7", preprocessed.source,
                                     atToken.line, atToken.column,
                                     "case 选择项 " + label->display +
                                         " 与先前选择项 " + previous.display + "（第 " +
                                         std::to_string(tokens[previous.token].line) + " 行第 " +
                                         std::to_string(tokens[previous.token].column) +
                                         " 列）重叠");
                            }
                            labels.push_back(*label);
                        }
                        labelBegin = at + 1;
                    }
                }
                const auto next = statementEnd(tokens, *colon + 1);
                cursor = next > *colon + 1 ? next : *colon + 1;
            }
        }
    }
}

void checkNativeVerilogCaseItemWidths(AnalysisResult& result,
                                      std::span<const core::SourceFile> sources,
                                      const AnalysisOptions& options) {
    // Native 9.2.3 FUN_180581c20 admits AST_CASE(0x57), infers the selector
    // and each non-block AST_COND(0x58) child through FUN_1808f4650 with the
    // fifth argument zero, and emits once for every inferable unequal item.
    // The item-side -1 result is skipped, but the selector-side -1 result is
    // deliberately not gated before comparison. AST_CONDX/Z(0x59/0x5a),
    // AST_DEFAULT(0x5b) groups and AST_BLOCK(0x54) children are excluded.
    // Static instruction evidence: 0x180581ef4, 0x180581f22-0x180581f55,
    // 0x180581f83-0x180581fbd.
    for (const auto& source : sources) {
        if (!applicable(source, "GJB10157-2021")) continue;
        const auto preprocessed = parser::preprocess(
            source, {options.includeDirectories, options.defines});
        if (!preprocessed.issues.empty()) continue;
        const auto tokens = standardTokens(preprocessed.source);
        const auto units = parser::parseDesignUnits(preprocessed.source);
        std::set<std::string> parameterOverriddenTargets;
        for (const auto& parent : units)
            for (const auto& instance : parent.instances)
                if (instance.hasNamedParameterOverrides ||
                    instance.hasPositionalParameterOverrides)
                    parameterOverriddenTargets.insert(instance.target);
        for (const auto& unit : units) {
            if (unit.kind != parser::DesignUnitKind::VerilogModule) continue;
            const auto model = fixedVerilogModuleWidthModel(
                unit, preprocessed.source, tokens,
                !parameterOverriddenTargets.contains(unit.name));
            if (!model) continue;

            const auto skipBalanced = [&](std::size_t open,
                                          std::string_view left,
                                          std::string_view right,
                                          std::size_t limit) {
                int depth = 0;
                for (auto at = open; at < limit; ++at) {
                    if (tokens[at].text == left) ++depth;
                    else if (tokens[at].text == right && --depth == 0)
                        return at + 1;
                    if (depth < 0) return limit;
                }
                return limit;
            };
            const auto skipStatement = [&](auto&& self, std::size_t at,
                                           std::size_t limit) -> std::size_t {
                if (at >= limit) return limit;
                if (tokens[at].text == ";") return at + 1;
                if (tokens[at].text == "begin") {
                    int depth = 1;
                    for (auto cursor = at + 1; cursor < limit; ++cursor) {
                        if (tokens[cursor].text == "begin") ++depth;
                        else if (tokens[cursor].text == "end" && --depth == 0) {
                            auto next = cursor + 1;
                            if (next + 1 < limit && tokens[next].text == ":" &&
                                tokens[next + 1].kind == parser::HdlTokenKind::Identifier)
                                next += 2;
                            return next;
                        }
                    }
                    return limit;
                }
                if (tokens[at].text == "case" || tokens[at].text == "casez" ||
                    tokens[at].text == "casex") {
                    int depth = 1;
                    for (auto cursor = at + 1; cursor < limit; ++cursor) {
                        if (tokens[cursor].text == "case" ||
                            tokens[cursor].text == "casez" ||
                            tokens[cursor].text == "casex") ++depth;
                        else if (tokens[cursor].text == "endcase" && --depth == 0)
                            return cursor + 1;
                    }
                    return limit;
                }
                if (tokens[at].text == "if") {
                    auto cursor = at + 1;
                    if (cursor >= limit || tokens[cursor].text != "(") return at + 1;
                    cursor = skipBalanced(cursor, "(", ")", limit);
                    if (cursor >= limit) return limit;
                    cursor = self(self, cursor, limit);
                    if (cursor < limit && tokens[cursor].text == "else")
                        cursor = self(self, cursor + 1, limit);
                    return cursor;
                }
                if (tokens[at].text == "for" || tokens[at].text == "while" ||
                    tokens[at].text == "repeat") {
                    auto cursor = at + 1;
                    if (cursor >= limit || tokens[cursor].text != "(") return at + 1;
                    cursor = skipBalanced(cursor, "(", ")", limit);
                    return cursor < limit ? self(self, cursor, limit) : limit;
                }
                if (tokens[at].text == "forever")
                    return self(self, at + 1, limit);
                if (tokens[at].text == "fork") {
                    int depth = 1;
                    for (auto cursor = at + 1; cursor < limit; ++cursor) {
                        if (tokens[cursor].text == "fork") ++depth;
                        else if ((tokens[cursor].text == "join" ||
                                  tokens[cursor].text == "join_any" ||
                                  tokens[cursor].text == "join_none") && --depth == 0)
                            return cursor + 1;
                    }
                    return limit;
                }
                int parens = 0, brackets = 0, braces = 0;
                for (auto cursor = at; cursor < limit; ++cursor) {
                    const auto& text = tokens[cursor].text;
                    if (text == "(") ++parens;
                    else if (text == ")") --parens;
                    else if (text == "[") ++brackets;
                    else if (text == "]") --brackets;
                    else if (text == "{") ++braces;
                    else if (text == "}") --braces;
                    else if (text == ";" && parens == 0 && brackets == 0 && braces == 0)
                        return cursor + 1;
                    else if ((text == "end" || text == "endcase") &&
                             parens == 0 && brackets == 0 && braces == 0)
                        return cursor;
                    if (parens < 0 || brackets < 0 || braces < 0) return cursor;
                }
                return limit;
            };

            for (auto caseToken = model->begin + 1;
                 caseToken + 2 < model->end; ++caseToken) {
                if (tokens[caseToken].text != "case" ||
                    tokens[caseToken + 1].text != "(") continue;
                const auto afterSelector = skipBalanced(
                    caseToken + 1, "(", ")", model->end);
                if (afterSelector >= model->end) continue;
                int caseDepth = 1;
                auto caseEnd = afterSelector;
                for (; caseEnd < model->end; ++caseEnd) {
                    if (tokens[caseEnd].text == "case" ||
                        tokens[caseEnd].text == "casez" ||
                        tokens[caseEnd].text == "casex") ++caseDepth;
                    else if (tokens[caseEnd].text == "endcase" && --caseDepth == 0)
                        break;
                }
                if (caseEnd >= model->end) continue;
                const auto selectorWidth = fixedBoundedExpressionWidth(
                    *model, caseToken + 2, afterSelector - 1, false);
                auto cursor = afterSelector;
                while (cursor < caseEnd) {
                    while (cursor < caseEnd && tokens[cursor].text == ";") ++cursor;
                    if (cursor >= caseEnd) break;
                    const auto labelsBegin = cursor;
                    int parens = 0, brackets = 0, braces = 0, ternaries = 0;
                    std::optional<std::size_t> colon;
                    for (; cursor < caseEnd; ++cursor) {
                        const auto& text = tokens[cursor].text;
                        if (text == "(") ++parens;
                        else if (text == ")") --parens;
                        else if (text == "[") ++brackets;
                        else if (text == "]") --brackets;
                        else if (text == "{") ++braces;
                        else if (text == "}") --braces;
                        else if (parens == 0 && brackets == 0 && braces == 0 &&
                                 text == "?") ++ternaries;
                        else if (parens == 0 && brackets == 0 && braces == 0 &&
                                 text == ":") {
                            if (ternaries > 0) --ternaries;
                            else { colon = cursor; break; }
                        }
                        if (parens < 0 || brackets < 0 || braces < 0) break;
                    }
                    if (!colon || labelsBegin >= *colon) break;
                    const bool defaultGroup = tokens[labelsBegin].text == "default";
                    if (!defaultGroup) {
                        std::size_t label = labelsBegin;
                        parens = brackets = braces = ternaries = 0;
                        for (auto at = labelsBegin; at <= *colon; ++at) {
                            if (at < *colon) {
                                const auto& text = tokens[at].text;
                                if (text == "(") ++parens;
                                else if (text == ")") --parens;
                                else if (text == "[") ++brackets;
                                else if (text == "]") --brackets;
                                else if (text == "{") ++braces;
                                else if (text == "}") --braces;
                                else if (parens == 0 && brackets == 0 && braces == 0 &&
                                         text == "?") ++ternaries;
                                else if (parens == 0 && brackets == 0 && braces == 0 &&
                                         text == ":" && ternaries > 0) --ternaries;
                            }
                            if (at != *colon && !(tokens[at].text == "," &&
                                parens == 0 && brackets == 0 && braces == 0 &&
                                ternaries == 0)) continue;
                            if (label < at) {
                                const auto itemWidth = fixedBoundedExpressionWidth(
                                    *model, label, at, false);
                                if (itemWidth && (!selectorWidth ||
                                    *itemWidth != *selectorWidth)) {
                                    const auto selectorText = selectorWidth
                                        ? std::to_string(*selectorWidth) + " 位"
                                        : std::string("未知宽度（原生值 -1）");
                                    emit(result, "GJB10157.9.2.3", preprocessed.source,
                                         tokens[label].line, tokens[label].column,
                                         "case 选择项为 " + std::to_string(*itemWidth) +
                                             " 位，选择器为" + selectorText);
                                }
                            }
                            label = at + 1;
                        }
                    }
                    const auto next = skipStatement(
                        skipStatement, *colon + 1, caseEnd);
                    cursor = next > *colon + 1 ? next : *colon + 1;
                }
            }
        }
    }
}

void checkNativeVerilogPortConnectionWidths(AnalysisResult& result,
                                             std::span<const core::SourceFile> sources,
                                             const AnalysisOptions& options) {
    // Native slot 0 records a formal width and calls FUN_1808f4650 on the
    // bound actual with fifth argument zero; width -1 is skipped and a known
    // unequal width emits Warning. This adapter applies the recovered bounded
    // flag-zero expression model after configured preprocessing.
    // ev_916c75cf9432a04dd9416f4288f21b1e91d878b083a98e8fd9520b68f5f02d40
    std::map<std::string, std::vector<parser::HdlToken>> tokenCache;
    std::map<std::string, core::SourceFile> activeSources;
    for (const auto& source : sources) {
        if (!applicable(source, "GJB10157-2021")) continue;
        const auto preprocessed = parser::preprocess(
            source, {options.includeDirectories, options.defines});
        if (!preprocessed.issues.empty()) continue;
        const auto key = source.path().lexically_normal().generic_string();
        activeSources.emplace(key, preprocessed.source);
        tokenCache.emplace(key, standardTokens(preprocessed.source));
    }
    std::map<std::string, FixedVerilogModuleWidthModel> models;
    std::set<std::string> ambiguous;
    for (const auto& unit : result.structure) {
        if (unit.kind != parser::DesignUnitKind::VerilogModule) continue;
        const auto key = unit.location.file.lexically_normal().generic_string();
        const auto source = activeSources.find(key);
        if (source == activeSources.end()) continue;
        const auto tokens = tokenCache.find(key);
        if (tokens == tokenCache.end()) continue;
        const auto model = fixedVerilogModuleWidthModel(unit, source->second, tokens->second);
        if (!model) continue;
        if (!models.emplace(unit.name, *model).second) ambiguous.insert(unit.name);
    }
    for (const auto& unit : result.structure) {
        if (unit.kind != parser::DesignUnitKind::VerilogModule || ambiguous.contains(unit.name)) continue;
        const auto parent = models.find(unit.name);
        if (parent == models.end()) continue;
        for (const auto& instance : unit.instances) {
            if (ambiguous.contains(instance.target) || instance.hasWildcardConnection ||
                (instance.hasNamedConnections && instance.hasPositionalConnections)) continue;
            const auto child = models.find(instance.target);
            if (child == models.end()) continue;
            const auto target = std::ranges::find_if(result.structure, [&](const auto& candidate) {
                return candidate.kind == parser::DesignUnitKind::VerilogModule &&
                       candidate.name == instance.target;
            });
            if (target == result.structure.end()) continue;
            std::map<std::string, unsigned> namedCounts;
            for (const auto& connection : instance.namedConnections) ++namedCounts[connection.formal];
            for (const auto& connection : instance.namedConnections) {
                if (!connection.actualConnected || namedCounts[connection.formal] != 1) continue;
                const auto formal = child->second.portWidths.find(connection.formal);
                if (formal == child->second.portWidths.end() || !formal->second) continue;
                std::optional<std::size_t> actual;
                if (connection.actualLocation.file.empty()) {
                    // SystemVerilog shorthand .name binds an actual of the same name.
                    const auto shorthand = parent->second.signalWidths.find(connection.formal);
                    if (shorthand != parent->second.signalWidths.end()) actual = shorthand->second;
                } else {
                    actual = fixedConnectionActualWidth(
                        parent->second, connection.actualLocation);
                }
                if (!actual || *actual == *formal->second) continue;
                emit(result, "GJB10157.9.1.1", *parent->second.source,
                     connection.location.line, connection.location.column,
                     "实例端口位宽不一致：" + instance.name + "." + connection.formal +
                     " 需要 " + std::to_string(*formal->second) + " 位，连接值为 " +
                     std::to_string(*actual) + " 位");
            }
            for (std::size_t index = 0; index < instance.positionalConnections.size() &&
                                        index < target->ports.size(); ++index) {
                const auto& connection = instance.positionalConnections[index];
                if (!connection.actualConnected) continue;
                const auto formal = child->second.portWidths.find(target->ports[index].name);
                if (formal == child->second.portWidths.end() || !formal->second) continue;
                const auto actual = fixedConnectionActualWidth(parent->second, connection.location);
                if (!actual || *actual == *formal->second) continue;
                emit(result, "GJB10157.9.1.1", *parent->second.source,
                     connection.location.line, connection.location.column,
                     "实例位置端口位宽不一致：" + instance.name + "." + target->ports[index].name +
                     " 需要 " + std::to_string(*formal->second) + " 位，连接值为 " +
                     std::to_string(*actual) + " 位");
            }
        }
    }
}

void checkNativeVerilogInoutToUnidirectionalPorts(
    AnalysisResult& result, std::span<const core::SourceFile> sources) {
    // Native FUN_1806c9ed0 resolves instance associations, then reports when
    // the actual declaration has both direction bytes +0xa0/+0xa1 set while
    // the formal declaration has either byte clear.  The packaged examples
    // identify that shape as a parent INOUT port bound to a child IN or OUT.
    // This adapter requires a unique project-local target before comparing
    // parsed port directions; unresolved library cells remain project context.
    // ev_9d37e17a014972cb692902d2d3196184236e04217b9f86a7ec11fe29ebe95fe0
    // ev_aaa493aac258e6c8e9f4d04fcd2ebeb575f37e5d55e363a21c32a358a3f3965b
    std::map<std::string, std::vector<const parser::DesignUnitInfo*>> modules;
    for (const auto& unit : result.structure) {
        if (unit.kind == parser::DesignUnitKind::VerilogModule)
            modules[unit.name].push_back(&unit);
    }
    for (const auto& parent : result.structure) {
        if (parent.kind != parser::DesignUnitKind::VerilogModule) continue;
        const auto source = std::ranges::find_if(sources, [&](const auto& candidate) {
            return candidate.path().lexically_normal() == parent.location.file.lexically_normal();
        });
        if (source == sources.end()) continue;
        const auto tokens = standardTokens(*source);
        const auto actualNameAt = [&](const core::SourceLocation& location)
            -> std::optional<std::string> {
            if (location.file.empty()) return std::nullopt;
            const auto token = tokenAtLocation(tokens, location);
            if (!token || tokens[*token].kind != parser::HdlTokenKind::Identifier)
                return std::nullopt;
            return tokens[*token].text;
        };
        const auto parentInout = [&](std::string_view name) {
            const auto port = std::ranges::find_if(parent.ports, [&](const auto& candidate) {
                return candidate.name == name;
            });
            return port != parent.ports.end() &&
                   port->direction == parser::PortDirection::Inout;
        };
        for (const auto& instance : parent.instances) {
            const auto targets = modules.find(instance.target);
            if (!instance.resolved || instance.external || targets == modules.end() ||
                targets->second.size() != 1) continue;
            const auto& target = *targets->second.front();
            const auto unidirectional = [](parser::PortDirection direction) {
                return direction == parser::PortDirection::Input ||
                       direction == parser::PortDirection::Output;
            };
            std::map<std::string, unsigned> namedCounts;
            for (const auto& connection : instance.namedConnections)
                ++namedCounts[connection.formal];
            for (const auto& connection : instance.namedConnections) {
                if (!connection.actualConnected || connection.actualExpression ||
                    namedCounts[connection.formal] != 1) continue;
                const auto formal = std::ranges::find_if(target.ports, [&](const auto& port) {
                    return port.name == connection.formal;
                });
                if (formal == target.ports.end() || !unidirectional(formal->direction)) continue;
                auto actual = actualNameAt(connection.actualLocation);
                // SystemVerilog shorthand .name binds the actual with the same name.
                if (!actual && connection.actualLocation.file.empty()) actual = connection.formal;
                if (!actual || !parentInout(*actual)) continue;
                emit(result, "GJB10157.7.3.3", *source,
                     connection.location.line, connection.location.column,
                     "双向端口 " + *actual + " 直接连接到单向形式端口 " +
                         instance.name + "." + formal->name);
            }
            for (std::size_t index = 0; index < instance.positionalConnections.size() &&
                                        index < target.ports.size(); ++index) {
                const auto& connection = instance.positionalConnections[index];
                const auto& formal = target.ports[index];
                if (!connection.actualConnected || connection.actualExpression ||
                    !unidirectional(formal.direction)) continue;
                const auto actual = actualNameAt(connection.location);
                if (!actual || !parentInout(*actual)) continue;
                emit(result, "GJB10157.7.3.3", *source,
                     connection.location.line, connection.location.column,
                     "双向端口 " + *actual + " 直接连接到单向形式端口 " +
                         instance.name + "." + formal.name);
            }
        }
    }
}

void checkNativeVerilogParameterReuse(AnalysisResult& result,
                                      std::span<const core::SourceFile> sources,
                                      const AnalysisOptions& options) {
    // Native 5.2.17 walks the ordered AST_MODULE map, inserts AST_PARAMETER(9)
    // names into one map for the run and reports every later duplicate.  The
    // mapped value retains the first module/name pair used by the fixed native
    // message. This parser-backed adapter intentionally excludes localparam.
    // ev_199080aae234fb8804491e9a25990f868c27c94170c9ff9f19af510083fd34ad
    // a2_7_parameter_reuse_entry.txt SHA256
    // fb5bfc569f9703ca25251c6386c17b3dbcf2307119671a0d162ff777696c91d2
    struct Candidate {
        std::size_t sourceIndex;
        std::size_t line;
        std::string name;
        std::string module;
    };
    std::vector<core::SourceFile> activeSources;
    std::vector<Candidate> candidates;
    activeSources.reserve(sources.size());
    for (const auto& source : sources) {
        if (!applicable(source, "GJB10157-2021")) continue;
        // The native predicate consumes AST_PARAMETER nodes, so conditional
        // compilation has already selected one source branch before the name
        // map is built. Mirror that boundary with the project macro set. If
        // preprocessing is unresolved, do not guess which declarations exist.
        const auto preprocessed = parser::preprocess(
            source, {options.includeDirectories, options.defines});
        if (!preprocessed.issues.empty()) continue;
        activeSources.push_back(preprocessed.source);
        const auto sourceIndex = activeSources.size() - 1;
        const auto tokens = standardTokens(activeSources.back());
        for (const auto& declaration :
             VerilogNamingDeclarations(activeSources.back(), tokens, true).collect()) {
            if (declaration.subject != VerilogNamingSubject::Parameter ||
                declaration.normalizedName.empty() || declaration.moduleName.empty()) continue;
            const auto& token = tokens[declaration.tokenIndex];
            candidates.push_back({sourceIndex, token.line,
                                  declaration.normalizedName, declaration.moduleName});
        }
    }
    // std::map traversal starts at its leftmost node. Preserve source/token
    // order only within an equal module key; duplicate module ownership remains
    // outside the bounded adapter and therefore is not claimed equivalent.
    std::stable_sort(candidates.begin(), candidates.end(), [](const auto& left,
                                                               const auto& right) {
        return left.module < right.module;
    });
    struct FirstOccurrence { std::string module; std::string name; };
    std::map<std::string, FirstOccurrence> seen;
    for (const auto& candidate : candidates) {
        const auto [found, inserted] = seen.emplace(
            candidate.name, FirstOccurrence{candidate.module, candidate.name});
        if (inserted) continue;
        emit(result, "GJB10157.5.2.17", activeSources[candidate.sourceIndex],
             candidate.line, 1,
             "参数" + candidate.name + "有" + found->second.module + "." +
                 found->second.name + "同名");
    }
}

struct NativeVerilogCommentCounts {
    std::size_t totalLines{0};
    std::size_t commentLines{0};
};

std::optional<std::size_t> nativeWindowsAcpLineLength(std::string_view line) {
    if (std::ranges::all_of(line, [](unsigned char value) { return value < 0x80; }))
        return line.size();
#ifdef _WIN32
    if (line.size() > static_cast<std::size_t>(std::numeric_limits<int>::max()))
        return std::nullopt;
    const auto inputSize = static_cast<int>(line.size());
    const auto wideSize = MultiByteToWideChar(CP_UTF8, 0, line.data(), inputSize,
                                               nullptr, 0);
    if (wideSize <= 0) return std::nullopt;
    std::wstring wide(static_cast<std::size_t>(wideSize), L'\0');
    if (MultiByteToWideChar(CP_UTF8, 0, line.data(), inputSize, wide.data(),
                            wideSize) != wideSize)
        return std::nullopt;
    const auto byteSize = WideCharToMultiByte(CP_ACP, 0, wide.data(), wideSize,
                                               nullptr, 0, nullptr, nullptr);
    if (byteSize < 0) return std::nullopt;
    return static_cast<std::size_t>(byteSize);
#else
    // FUN_1804df420 converts UTF-8 through the active Windows ANSI code page.
    // That environment is unavailable on non-Windows hosts. ASCII is invariant;
    // leave non-ASCII lines unknown instead of substituting UTF-8 byte length.
    return std::nullopt;
#endif
}

void checkNativeVerilogLineLength(AnalysisResult& result,
                                  std::span<const core::SourceFile> sources,
                                  const AnalysisOptions& options) {
    // GJB10157.5.1.4 FUN_180528fc0 walks the module-name map at +0x68,
    // admits only vectors containing exactly one nonnull AST_MODULE(2), and
    // reopens that root's +0x1e0 source path. Therefore one source containing
    // two distinct unique modules is scanned twice, while a duplicate same-name
    // module vector and a module-free source are skipped.
    std::map<std::string, std::vector<const parser::DesignUnitInfo*>> modules;
    for (const auto& unit : result.structure)
        if (unit.kind == parser::DesignUnitKind::VerilogModule)
            modules[unit.name].push_back(&unit);

    std::map<std::filesystem::path, const core::SourceFile*> sourceByPath;
    for (const auto& source : sources) {
        if (source.language() != core::HdlLanguage::Verilog &&
            source.language() != core::HdlLanguage::SystemVerilog) continue;
        const auto preprocessed = parser::preprocess(
            source, {options.includeDirectories, options.defines});
        if (!preprocessed.issues.empty()) return;
        sourceByPath.emplace(source.path().lexically_normal(), &source);
    }

    constexpr std::size_t threshold = 80;
    for (const auto& [name, definitions] : modules) {
        (void)name;
        if (definitions.size() != 1 || definitions.front() == nullptr) continue;
        const auto found = sourceByPath.find(
            definitions.front()->location.file.lexically_normal());
        if (found == sourceByPath.end()) continue;
        const auto lines = linesOf(found->second->content());
        for (std::size_t index = 0; index < lines.size(); ++index) {
            const auto length = nativeWindowsAcpLineLength(lines[index]);
            if (!length || *length < threshold) continue;
            emit(result, "GJB10157.5.1.4", *found->second, index + 1, 1,
                 "建议每行字符数小于80个");
        }
    }
}

NativeVerilogCommentCounts nativeVerilogCommentCounts(std::string_view content) {
    NativeVerilogCommentCounts counts;
    bool inBlockComment = false;
    std::istringstream stream{std::string(content)};
    for (std::string line; std::getline(stream, line);) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        ++counts.totalLines;
        std::string compact;
        compact.reserve(line.size());
        for (const char value : line)
            if (value != ' ' && value != '\t') compact.push_back(value);

        if (inBlockComment) {
            ++counts.commentLines;
            if (compact.find("*/") != std::string::npos) inBlockComment = false;
            continue;
        }
        if (compact.find("//") != std::string::npos ||
            compact.find("--") != std::string::npos) {
            ++counts.commentLines;
            continue;
        }
        const auto open = compact.find("/*");
        if (open == std::string::npos) continue;
        ++counts.commentLines;
        inBlockComment = compact.find("*/") == std::string::npos;
    }
    return counts;
}

void checkNativeVerilogCommentRatio(AnalysisResult& result,
                                    std::span<const core::SourceFile> sources,
                                    const AnalysisOptions& options) {
    // GJB10157.5.1.8 FUN_18052b5d0 clears the project total, then calls
    // FUN_18052bd10 for each uniquely owned AST_MODULE source path and makes
    // one project-level comparison. The helper removes only spaces and Tabs,
    // treats //, -- and /*...*/ as raw byte markers, and includes blank lines.
    // The secondary +0x78 map is populated by FUN_18062b8a0 from the alternate
    // (VHDL) frontend and is scanned by the same raw helper. Duplicate Verilog
    // module-name vectors are skipped by the native size==1 gate.
    std::map<std::string, std::size_t> moduleDefinitions;
    std::map<std::string, std::size_t> vhdlDefinitions;
    std::map<std::filesystem::path, std::size_t> modulesPerSource;
    for (const auto& unit : result.structure) {
        if (unit.kind == parser::DesignUnitKind::VerilogModule)
            ++moduleDefinitions[unit.name];
        else if (unit.kind == parser::DesignUnitKind::VhdlEntity)
            ++vhdlDefinitions[lower(unit.name)];
    }
    // The +0x78 map consumes its first root when a key owns more than one.
    // Which duplicate VHDL root wins is not recovered, so do not guess.
    if (std::ranges::any_of(vhdlDefinitions, [](const auto& entry) {
            return entry.second > 1;
        })) return;

    for (const auto& source : sources) {
        if (source.language() != core::HdlLanguage::Verilog &&
            source.language() != core::HdlLanguage::SystemVerilog) continue;
        const auto preprocessed = parser::preprocess(
            source, {options.includeDirectories, options.defines});
        if (!preprocessed.issues.empty()) return;
    }
    for (const auto& unit : result.structure) {
        if (unit.kind == parser::DesignUnitKind::VerilogModule &&
            moduleDefinitions[unit.name] == 1)
            ++modulesPerSource[unit.location.file.lexically_normal()];
        else if (unit.kind == parser::DesignUnitKind::VhdlEntity &&
                 vhdlDefinitions[lower(unit.name)] == 1)
            ++modulesPerSource[unit.location.file.lexically_normal()];
    }

    std::size_t totalLines = 0;
    std::size_t commentLines = 0;
    for (const auto& source : sources) {
        const auto found = modulesPerSource.find(source.path().lexically_normal());
        if (found == modulesPerSource.end()) continue;
        const auto counts = nativeVerilogCommentCounts(source.content());
        totalLines += counts.totalLines * found->second;
        commentLines += counts.commentLines * found->second;
    }

    const auto ratio = static_cast<float>(commentLines) /
                       static_cast<float>(totalLines);
    if (static_cast<double>(ratio) >= 0.2) return;
    emitAtLocation(result, "GJB10157.5.1.8", {{}, 1, 1, 0},
                   "该工程代码注释率小于20%");
}

void applyStandardProfile(AnalysisResult& result,
                          std::span<const core::SourceFile> sources,
                          const AnalysisOptions& options) {
    if (options.standardId.empty() || options.countOnly) return;
    const auto profile = findStandardProfile(options.standardId);
    if (!profile) {
        result.errors.push_back("Unknown GJB standard profile: " + options.standardId);
        return;
    }

    // Exported native setTopModule() returns failure unless the canonical name
    // resolves to exactly one parsed design record. Do not fall back to an
    // inferred graph root after the user explicitly selected a missing or
    // ambiguous design, because that would fabricate top-sensitive results.
    if (options.topModule) {
        const auto expectedKind = profile->id == "GJB9765-2020"
            ? parser::DesignUnitKind::VhdlEntity
            : parser::DesignUnitKind::VerilogModule;
        const auto matches = std::ranges::count_if(result.structure, [&](const auto& unit) {
            return unit.kind == expectedKind && selectedTop(unit, options);
        });
        if (matches != 1) {
            result.errors.push_back(matches == 0
                ? "指定的顶层设计不存在：" + *options.topModule
                : "指定的顶层设计名称不唯一：" + *options.topModule);
            return;
        }
    }

    bool hasApplicableSource = false;
    for (const auto& source : sources) {
        if (!applicable(source, profile->id)) continue;
        hasApplicableSource = true;
        directChecks(result, source, profile->id, options);
    }
    if (profile->id == "GJB10157-2021")
        checkNativeVerilogParameterReuse(result, sources, options);
    if (profile->id == "GJB10157-2021")
        checkNativeVerilogLineLength(result, sources, options);
    if (profile->id == "GJB10157-2021")
        checkNativeVerilogCommentRatio(result, sources, options);
    if (profile->id == "GJB10157-2021")
        checkNativeVerilogPortConnectionWidths(result, sources, options);
    if (profile->id == "GJB10157-2021")
        checkNativeVerilogInoutToUnidirectionalPorts(result, sources);
    if (profile->id == "GJB10157-2021")
        checkNativeVerilogAssignmentWidths(result, sources, options);
    if (profile->id == "GJB10157-2021")
        checkNativeVerilogCaseFallback(result, sources, options);
    if (profile->id == "GJB10157-2021")
        checkNativeVerilogCaseOverlap(result, sources, options);
    if (profile->id == "GJB10157-2021")
        checkNativeVerilogCaseItemWidths(result, sources, options);
    if (profile->id == "GJB10157-2021")
        checkNativeVerilogConditionWidths(result, sources);
    if (profile->id == "GJB10157-2021")
        checkNativeVerilogVectorLogical(result, sources, options);
    if (!hasApplicableSource && !sources.empty()) {
        result.errors.push_back("Selected standard " + std::string(profile->id) +
            " does not apply to the provided HDL language.");
    }

    const auto original = result.violations;
    std::vector<Violation> standardized;
    for (const auto& violation : original) {
        bool mapped = false;
        for (const auto& mapping : Mappings) {
            if (mapping.standardId != profile->id || mapping.analyzerRuleId != violation.ruleId) continue;
            if (mapping.clauseId == "GJB10157.6.2.2") continue;
            // FUN_18052cd30 resolves exactly the AST_MODULE named by the
            // setTopModule global before collecting driven and loaded nets.
            // Keep the generic redundancy mapping for 5.2.18, but do not map
            // a write-only signal in another root to native clause 5.2.10.
            if (mapping.clauseId == "GJB10157.5.2.10" && options.topModule &&
                (!violation.module ||
                 *violation.module != configuredVerilogTop(*options.topModule))) continue;
            // Native 4.4.b counts any resolved appearance in the architecture,
            // including an assignment target, as use. The generic diagnostic
            // is read-oriented, so suppress its translation when another
            // same-scope occurrence proves native use.
            if (mapping.clauseId == "GJB9765.4.4.b" &&
                nativeVhdlSignalHasAnyUse(sources, violation)) continue;
            const auto* entry = inventoryEntry(mapping.clauseId);
            const auto detail = findStandardRuleDetail(mapping.clauseId);
            if (!entry || !detail) continue;
            auto translated = violation;
            translated.ruleId = std::string(mapping.clauseId);
            translated.severity = entry->severity;
            if (mapping.clauseId == "GJB10157.5.2.10") {
                // Native FUN_18052cd30 constructs DAT_180eebb28 + normalized
                // candidate name + DAT_180eebb20, records the declaration line,
                // and has no native column field.
                translated.message = "禁止顶层的输入管脚" +
                    violation.signal.value_or(std::string{}) + "悬空";
                translated.column = 1;
            }
            if (mapping.clauseId == "GJB10157.5.2.16") {
                // Native FUN_180531530 emits the fixed 20-byte GB18030
                // message for every AST_DEFPARAM(11). Its location record
                // carries source path and line but no recovered column.
                translated.message = "禁止使用defparam语句";
                translated.column = 1;
            }
            if (mapping.clauseId == "GJB10157.5.2.18") {
                // The rule constructor writes the word 0x0001 at +0x158:
                // enable the internal-signal branch and keep +0x159 clear.
                // FUN_180533200 therefore selects DAT_180eec818 for port
                // candidates and DAT_180eec838 for internal candidates.
                translated.message = violation.ruleId == "RTL-UNUSED-INPUT-PORT" ?
                    "冗余的端口" : "冗余的内部信号";
            }
            if (mapping.clauseId == "GJB10157.9.4.7") {
                // Native FUN_1806e2530 concatenates the GB18030 literal
                // "检测到组合回路" with the recovered SCC member names, using
                // DAT_180eeaed4 ("、") between names, and emits Warning.
                // Preserve the independently recovered source graph and
                // location, but reproduce the observed diagnostic surface.
                constexpr std::string_view genericPrefix =
                    "Combinational dependency cycle: ";
                translated.message = "检测到组合回路";
                if (violation.message.starts_with(genericPrefix)) {
                    auto members = violation.message.substr(genericPrefix.size());
                    for (std::size_t separator = members.find(", ");
                         separator != std::string::npos;
                         separator = members.find(", ", separator + 3)) {
                        members.replace(separator, 2, "、");
                    }
                    translated.message += members;
                }
            } else if (mapping.clauseId == "GJB10157.9.4.8") {
                // Native FUN_18071f0a0 constructs "信号" + resolved name +
                // DAT_180f16538 ("索引超出指定的数组范围") and emits Warning.
                // The source adapter retains its proven location and name.
                translated.message = "信号";
                translated.message += violation.signal.value_or("<未知>");
                translated.message += "索引超出指定的数组范围";
            }
            translated.description = std::string(detail->summary);
            translated.recommendation = "按照该 GJB 条款及标准中的遵循示例修改代码。";
            standardized.push_back(std::move(translated));
            mapped = true;
        }
        if (!mapped && (violation.ruleId.starts_with("SYNTAX-") ||
                        violation.ruleId.starts_with("PP-"))) {
            standardized.push_back(violation);
        }
    }
    for (const auto& violation : result.violations) {
        if (violation.ruleId.starts_with("GJB")) standardized.push_back(violation);
    }
    result.violations = std::move(standardized);
    std::ranges::sort(result.violations, [](const auto& left, const auto& right) {
        return std::tie(left.file, left.line, left.column, left.ruleId, left.message) <
               std::tie(right.file, right.line, right.column, right.ruleId, right.message);
    });
    result.violations.erase(std::unique(result.violations.begin(), result.violations.end(),
        [](const auto& left, const auto& right) {
            // These native rules emit once per admitted AST object/pair. Their
            // byte-for-byte identical diagnostics therefore carry multiplicity.
            if ((left.ruleId == "GJB10157.7.1.3" &&
                 right.ruleId == "GJB10157.7.1.3") ||
                (left.ruleId == "GJB10157.7.1.2" &&
                 right.ruleId == "GJB10157.7.1.2") ||
                (left.ruleId == "GJB10157.5.1.4" &&
                 right.ruleId == "GJB10157.5.1.4") ||
                (left.ruleId == "GJB10157.5.1.7" &&
                 right.ruleId == "GJB10157.5.1.7") ||
                (left.ruleId == "GJB10157.5.2.7" &&
                 right.ruleId == "GJB10157.5.2.7") ||
                (left.ruleId == "GJB10157.5.2.1" &&
                 right.ruleId == "GJB10157.5.2.1") ||
                (left.ruleId == "GJB10157.5.2.2" &&
                 right.ruleId == "GJB10157.5.2.2") ||
                (left.ruleId == "GJB10157.5.2.4" &&
                 right.ruleId == "GJB10157.5.2.4") ||
                (left.ruleId == "GJB10157.5.2.16" &&
                 right.ruleId == "GJB10157.5.2.16") ||
                (left.ruleId == "GJB10157.5.2.15" &&
                 right.ruleId == "GJB10157.5.2.15") ||
                (left.ruleId == "GJB10157.5.2.17" &&
                 right.ruleId == "GJB10157.5.2.17") ||
                (left.ruleId == "GJB10157.5.3.1" &&
                 right.ruleId == "GJB10157.5.3.1") ||
                (left.ruleId == "GJB10157.5.3.2" &&
                 right.ruleId == "GJB10157.5.3.2") ||
                (left.ruleId == "GJB10157.5.3.3" &&
                 right.ruleId == "GJB10157.5.3.3") ||
                (left.ruleId == "GJB10157.5.3.4" &&
                 right.ruleId == "GJB10157.5.3.4") ||
                (left.ruleId == "GJB10157.5.3.5" &&
                 right.ruleId == "GJB10157.5.3.5") ||
                (left.ruleId == "GJB10157.6.1.2" &&
                 right.ruleId == "GJB10157.6.1.2") ||
                (left.ruleId == "GJB10157.6.1.4" &&
                 right.ruleId == "GJB10157.6.1.4") ||
                (left.ruleId == "GJB10157.6.1.6" &&
                 right.ruleId == "GJB10157.6.1.6") ||
                (left.ruleId == "GJB10157.6.2.1" &&
                 right.ruleId == "GJB10157.6.2.1") ||
                (left.ruleId == "GJB10157.6.2.2" &&
                 right.ruleId == "GJB10157.6.2.2") ||
                (left.ruleId == "GJB10157.6.2.3" &&
                 right.ruleId == "GJB10157.6.2.3") ||
                (left.ruleId == "GJB10157.6.2.7" &&
                 right.ruleId == "GJB10157.6.2.7") ||
                (left.ruleId == "GJB10157.9.2.9" &&
                 right.ruleId == "GJB10157.9.2.9") ||
                (left.ruleId == "GJB9765.4.6.c" &&
                 right.ruleId == "GJB9765.4.6.c")) return false;
            return left.ruleId == right.ruleId && left.file == right.file &&
                   left.line == right.line && left.column == right.column &&
                   left.message == right.message;
        }), result.violations.end());

    // Standard IDs are first-class rule IDs: command-line/config suppression
    // and severity overrides must be applied after generic diagnostics have
    // been translated into GJB clauses.
    result.violations.erase(std::remove_if(result.violations.begin(), result.violations.end(),
        [&](const auto& violation) {
            return std::ranges::any_of(options.disabledRules, [&](const auto& disabled) {
                return lower(disabled) == "all" || lower(disabled) == lower(violation.ruleId) ||
                       lower(disabled) == lower(standardRuleDisplayId(violation.ruleId));
            });
        }), result.violations.end());
    for (const auto& override : options.severityOverrides) {
        for (auto& violation : result.violations) {
            if (lower(override.ruleId) == lower(violation.ruleId) ||
                lower(override.ruleId) == lower(standardRuleDisplayId(violation.ruleId))) {
                violation.severity = override.severity;
            }
        }
    }

    std::set<std::string> violated;
    for (const auto& violation : result.violations) {
        if (violation.ruleId.starts_with("GJB")) violated.insert(violation.ruleId);
    }
    result.standardAssessments.clear();
    for (const auto& entry : standardRuleInventory()) {
        if (entry.standardId != profile->id) continue;
        const auto detail = findStandardRuleDetail(entry.clauseId);
        // An executable detector is not a complete rule proof. The native
        // ledger currently verifies zero full algorithms, so a clean detector
        // result still needs review. Preserve concrete violation evidence.
        StandardRuleState state = StandardRuleState::ManualReviewRequired;
        if (violated.contains(std::string(entry.clauseId))) {
            state = StandardRuleState::Violated;
        } else if (detail && detail->assessmentMode == StandardAssessmentMode::ProjectContext) {
            state = StandardRuleState::ProjectContextRequired;
        }
        result.standardAssessments.push_back({std::string(entry.clauseId), state});
    }
}

} // namespace fpga::analysis
