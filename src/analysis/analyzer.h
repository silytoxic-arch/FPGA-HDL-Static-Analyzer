#pragma once

#include "analysis/violation.h"
#include "analysis/line_statistics.h"
#include "analysis/verilog_header_policy.h"
#include "analysis/verilog_keyword_names.h"
#include "analysis/power_on_policy.h"
#include "core/source.h"
#include "parser/design_parser.h"

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace fpga::analysis {

struct RuleSeverityOverride {
    std::string ruleId;
    Severity severity{Severity::Warning};
};

enum class StandardRuleState { Passed, Violated, ProjectContextRequired, ManualReviewRequired };

// Product applicability context, not a recovered native FSM admission gate.
enum class FsmOptimizationPriority { Unspecified, Area, Speed };

[[nodiscard]] inline std::optional<FsmOptimizationPriority>
parseFsmOptimizationPriority(std::string_view value) {
    if (value == "unspecified") return FsmOptimizationPriority::Unspecified;
    if (value == "area") return FsmOptimizationPriority::Area;
    if (value == "speed") return FsmOptimizationPriority::Speed;
    return std::nullopt;
}

struct StandardRuleAssessment {
    std::string clauseId;
    StandardRuleState state{StandardRuleState::ManualReviewRequired};
};

struct AnalysisOptions {
    std::vector<std::string> disabledRules;
    std::vector<std::string> blackBoxes;
    std::vector<std::string> excludePatterns;
    std::vector<std::filesystem::path> includeDirectories;
    std::vector<std::string> defines;
    std::vector<RuleSeverityOverride> severityOverrides;
    std::string standardId;
    // Native T-Linter setTopModule() stores one configured design name. Keep
    // absence distinct from an explicit name so top-sensitive checks can
    // retain the inferred-root fallback without claiming native equivalence.
    std::optional<std::string> topModule;
    FsmOptimizationPriority fsmOptimizationPriority{FsmOptimizationPriority::Unspecified};
    PowerOnPolicy powerOnPolicy;
    bool countOnly{false};
    VerilogHeaderPolicy verilogHeaderPolicy;
    VerilogKeywordPolicy verilogKeywordPolicy;
};

struct AnalysisResult {
    LineStatistics lineStatistics;
    std::size_t filesAnalyzed{0};
    std::size_t designUnits{0};
    std::vector<parser::DesignUnitInfo> structure;
    std::vector<Violation> violations;
    std::vector<std::string> errors;
    std::size_t suppressedViolations{0};
    std::size_t baselinedViolations{0};
    std::vector<RuleSeverityOverride> severityOverrides;
    std::string standardId;
    std::vector<StandardRuleAssessment> standardAssessments;
    PowerOnPolicy powerOnPolicy;
    VerilogHeaderPolicy verilogHeaderPolicy;
    VerilogKeywordPolicy verilogKeywordPolicy;
};

// Discovers HDL inputs, extracts design hierarchy, applies deterministic rules,
// and returns source-mapped diagnostics suitable for CLI/GUI/reporting.
class Analyzer {
public:
    explicit Analyzer(AnalysisOptions options = {});

    [[nodiscard]] AnalysisResult
    analyzePaths(const std::vector<std::filesystem::path>& inputs) const;

    [[nodiscard]] AnalysisResult
    analyzeSource(const core::SourceFile& source) const;

    [[nodiscard]] AnalysisResult
    analyzeSources(const std::vector<core::SourceFile>& sources) const;

    [[nodiscard]] static core::HdlLanguage
    languageFromPath(const std::filesystem::path& path) noexcept;

private:
    [[nodiscard]] AnalysisResult
    analyzeSourceImpl(const core::SourceFile& source, bool runTimingRules) const;

    AnalysisOptions options_;
};

[[nodiscard]] std::string_view severityName(Severity severity) noexcept;

} // namespace fpga::analysis
