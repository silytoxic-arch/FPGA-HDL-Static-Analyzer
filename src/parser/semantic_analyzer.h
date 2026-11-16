#pragma once

#include "core/source.h"
#include "parser/design_parser.h"

#include <optional>
#include <string>
#include <vector>

namespace fpga::parser {

struct SemanticIssue {
    std::string ruleId;
    std::string message;
    std::string description;
    std::string recommendation;
    core::SourceLocation location;
    std::optional<std::string> designUnit;
    std::optional<std::string> signal;
};

struct UnitTimingInfo {
    std::string designUnit;
    DesignUnitKind kind{DesignUnitKind::VerilogModule};
    std::vector<ClockInfo> clocks;
    std::vector<ResetInfo> resets;
    std::vector<SequentialProcessInfo> sequentialProcesses;
    std::vector<std::string> scalarSignals;
};

// Builds a conservative driver/read/sensitivity model from HDL tokens. Findings
// are emitted only for declared signals/ports and distinct procedural/concurrent sources.
[[nodiscard]] std::vector<SemanticIssue>
analyzeSemantics(const core::SourceFile& source);

// Bounded, source-mapped range-comparison records, separate from the broader
// constant-condition analysis. Unknown widths and overloaded types are deferred.
[[nodiscard]] std::vector<SemanticIssue>
analyzeVhdlUnreachableComparisons(const core::SourceFile& source);

// Resolves VHDL generic use across entity and architecture files. This is a
// project-level pass because architectures may be stored separately.
[[nodiscard]] std::vector<SemanticIssue>
analyzeUnusedVhdlGenerics(const std::vector<core::SourceFile>& sources);

// Extracts source-mapped clock/reset events for hierarchy and report views.
[[nodiscard]] std::vector<UnitTimingInfo>
extractTimingInfo(const core::SourceFile& source);

} // namespace fpga::parser
