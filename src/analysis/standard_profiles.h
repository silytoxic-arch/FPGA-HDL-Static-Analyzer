#pragma once

#include "analysis/violation.h"

#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace fpga::analysis {

struct StandardProfile {
    std::string_view id;
    std::string_view title;
    std::string_view language;
    std::size_t observedRules;
    std::size_t compulsoryRules;
};

struct StandardRuleEntry {
    std::string_view standardId;
    std::string_view clauseId;
    Severity severity;
    bool compulsory;
};

enum class StandardAssessmentMode { Automated, ProjectContext, ManualReview };

struct StandardRuleDetail {
    std::string_view clauseId;
    std::string_view summary;
    StandardAssessmentMode assessmentMode;
};

struct StandardRuleExample {
    std::string clauseId;
    std::string compliant;
    std::string violating;
    std::string evidenceBasis;
};

enum class StandardCoverage { Implemented, Partial };

struct StandardRuleBinding {
    std::string_view standardId;
    std::string_view clauseId;
    std::string_view analyzerRuleId;
    StandardCoverage coverage;
    std::string_view evidence;
};

[[nodiscard]] std::span<const StandardProfile> standardProfiles() noexcept;
[[nodiscard]] std::optional<StandardProfile> findStandardProfile(std::string_view id) noexcept;
[[nodiscard]] std::span<const StandardRuleEntry> standardRuleInventory() noexcept;
[[nodiscard]] std::span<const StandardRuleDetail> standardRuleDetails() noexcept;
// Public-facing identifier used by the GUI and exported reports.  The full
// GJB clause remains the internal compatibility key and traceability field.
// Examples: GJB9765.4.2.a -> R-2-1, GJB9765.4.8.e -> A-8-1,
// GJB10157.5.2.1 -> R-2-1. Public IDs are unique within a standard profile;
// the full GJB clause remains the unambiguous cross-profile traceability key.
[[nodiscard]] std::string standardRuleDisplayId(std::string_view clauseId);
[[nodiscard]] std::optional<StandardRuleDetail> findStandardRuleDetail(std::string_view clauseId) noexcept;
[[nodiscard]] std::optional<StandardRuleDetail> findStandardRuleDetail(
    std::string_view standardId, std::string_view ruleId) noexcept;
[[nodiscard]] const std::vector<StandardRuleExample>& standardRuleExamples();
[[nodiscard]] const StandardRuleExample* findStandardRuleExample(std::string_view clauseId) noexcept;
[[nodiscard]] const StandardRuleExample* findStandardRuleExample(
    std::string_view standardId, std::string_view ruleId) noexcept;
[[nodiscard]] std::span<const StandardRuleBinding> standardRuleBindings() noexcept;
[[nodiscard]] std::string_view standardCoverageName(StandardCoverage coverage) noexcept;
[[nodiscard]] std::string_view standardAssessmentModeName(StandardAssessmentMode mode) noexcept;

} // namespace fpga::analysis
