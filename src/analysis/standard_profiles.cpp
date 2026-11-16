#include "analysis/standard_profiles.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <string>

namespace fpga::analysis {
namespace {

constexpr std::array Profiles{
    StandardProfile{"GJB10157-2021", "军用可编程逻辑器件软件语言编程安全子集", "Verilog", 103, 65},
    StandardProfile{"GJB9765-2020", "军用可编程逻辑器件软件编程语言安全子集—VHDL语言篇", "VHDL", 106, 39},
};

constexpr std::array Inventory{
#include "analysis/standard_rule_inventory.inc"
};

constexpr std::array Details{
#include "analysis/standard_rule_details.inc"
};

struct StandardRulePublicId {
    std::string_view clauseId;
    std::string_view publicId;
};

// The official R/A index is explicit for both standards. Keep the mapping
// table-driven: clause letters and nested subitems are not public rule IDs,
// omitted rules keep their index gaps, and the two profiles may legitimately
// reuse the same public ID (for example each profile contains R-2-1).
constexpr std::array PublicIds{
#include "analysis/standard_rule_public_ids.inc"
};

consteval bool isOfficialThreePartPublicId(std::string_view id) {
    if (id.size() < 5 || (id.front() != 'R' && id.front() != 'A') || id[1] != '-')
        return false;
    const auto secondDash = id.find('-', 2);
    if (secondDash == std::string_view::npos || secondDash == 2 ||
        secondDash + 1 == id.size() ||
        id.find('-', secondDash + 1) != std::string_view::npos)
        return false;
    for (auto index = std::size_t{2}; index < secondDash; ++index)
        if (id[index] < '0' || id[index] > '9') return false;
    for (auto index = secondDash + 1; index < id.size(); ++index)
        if (id[index] < '0' || id[index] > '9') return false;
    return true;
}

consteval bool publicIdsHaveOfficialFormat() {
    for (const auto& publicId : PublicIds)
        if (!isOfficialThreePartPublicId(publicId.publicId)) return false;
    return true;
}

static_assert(PublicIds.size() == Inventory.size(),
    "Every observed GJB rule must have one public ID entry");
static_assert(publicIdsHaveOfficialFormat(),
    "Every observed GJB rule must use the official R-X-X/A-X-X public ID format");

// Only bindings supported by direct T-Linter artifact evidence are recorded here.
// A missing binding means "not implemented", not "compliant".
constexpr std::array Bindings{
    StandardRuleBinding{"GJB10157-2021", "GJB10157.5.2.16", "VL-DEFPARAM",
        StandardCoverage::Implemented, "T-Linter.dll places the defparam token with this rule implementation."},
    StandardRuleBinding{"GJB10157-2021", "GJB10157.9.2.8", "VL-CASEX",
        StandardCoverage::Implemented, "Packaged 9.2.8 examples prohibit both casex and casez; the standard detector checks both tokens."},
    StandardRuleBinding{"GJB9765-2020", "GJB9765.4.12.a", "RTL-NONSYNTHESIZABLE-DELAY",
        StandardCoverage::Partial, "Native evidence admits non-integer entity generics, exact empty-string constant initializers and wait-for statements. User-defined generic subtypes and frontend type propagation still require review."},
    StandardRuleBinding{"GJB9765-2020", "GJB9765.4.12.c", "VHDL-BUFFER-PORT",
        StandardCoverage::Implemented, "T-Linter.dll places the buffer port-mode token with this rule implementation."},
};

std::string normalized(std::string_view value) {
    std::string result;
    result.reserve(value.size());
    for (const unsigned char character : value) {
        if (character == '_' || character == ' ' || character == '-') continue;
        result.push_back(static_cast<char>(std::toupper(character)));
    }
    return result;
}

} // namespace

std::span<const StandardProfile> standardProfiles() noexcept { return Profiles; }

std::optional<StandardProfile> findStandardProfile(std::string_view id) noexcept {
    const auto key = normalized(id);
    const auto found = std::find_if(Profiles.begin(), Profiles.end(), [&](const auto& profile) {
        const auto profileKey = normalized(profile.id);
        return key == profileKey || key == normalized(profile.id.substr(0, profile.id.find('-')));
    });
    if (found == Profiles.end()) return std::nullopt;
    return *found;
}

std::span<const StandardRuleEntry> standardRuleInventory() noexcept { return Inventory; }
std::span<const StandardRuleDetail> standardRuleDetails() noexcept { return Details; }

std::string standardRuleDisplayId(std::string_view clauseId) {
    const auto official = std::find_if(PublicIds.begin(), PublicIds.end(),
        [&](const auto& entry) { return entry.clauseId == clauseId; });
    if (official != PublicIds.end()) return std::string(official->publicId);

    return std::string(clauseId);
}

const std::vector<StandardRuleExample>& standardRuleExamples() {
    // PDF examples include both code and diagrams. Normalize each pair to a
    // stable review oracle so non-source checks are never reported as passed.
    static const std::vector<StandardRuleExample> examples = [] {
        std::vector<StandardRuleExample> result;
        result.reserve(Details.size());
        for (const auto& detail : Details) {
            result.push_back({std::string(detail.clauseId),
                "正确案例判据：" + std::string(detail.summary),
                "违反案例判据：代码或工程配置未满足“" + std::string(detail.summary) + "”。",
                "GJB 正文正例/反例与 T-Linter 规则标识交叉核对"});
        }
        return result;
    }();
    return examples;
}

const StandardRuleExample* findStandardRuleExample(std::string_view clauseId) noexcept {
    const auto key = normalized(clauseId);
    const auto& examples = standardRuleExamples();
    if (const auto exact = std::find_if(examples.begin(), examples.end(), [&](const auto& example) {
            return normalized(example.clauseId) == key;
        }); exact != examples.end()) return &*exact;
    const StandardRuleExample* match = nullptr;
    for (const auto& example : examples) {
        if (normalized(standardRuleDisplayId(example.clauseId)) != key) continue;
        if (match) return nullptr; // Public IDs are profile-local and may be ambiguous globally.
        match = &example;
    }
    return match;
}

std::optional<StandardRuleDetail> findStandardRuleDetail(std::string_view clauseId) noexcept {
    const auto key = normalized(clauseId);
    if (const auto exact = std::find_if(Details.begin(), Details.end(), [&](const auto& detail) {
            return normalized(detail.clauseId) == key;
        }); exact != Details.end()) return *exact;
    const StandardRuleDetail* match = nullptr;
    for (const auto& detail : Details) {
        if (normalized(standardRuleDisplayId(detail.clauseId)) != key) continue;
        if (match) return std::nullopt; // Require a profile when the public ID is reused.
        match = &detail;
    }
    return match ? std::optional<StandardRuleDetail>{*match} : std::nullopt;
}

std::optional<StandardRuleDetail> findStandardRuleDetail(
    std::string_view standardId, std::string_view ruleId) noexcept {
    const auto profile = findStandardProfile(standardId);
    if (!profile) return std::nullopt;
    const auto key = normalized(ruleId);
    for (const auto& entry : Inventory) {
        if (entry.standardId != profile->id) continue;
        if (normalized(entry.clauseId) != key &&
            normalized(standardRuleDisplayId(entry.clauseId)) != key) continue;
        const auto detail = std::find_if(Details.begin(), Details.end(), [&](const auto& candidate) {
            return candidate.clauseId == entry.clauseId;
        });
        return detail == Details.end() ? std::nullopt : std::optional<StandardRuleDetail>{*detail};
    }
    return std::nullopt;
}

const StandardRuleExample* findStandardRuleExample(
    std::string_view standardId, std::string_view ruleId) noexcept {
    const auto detail = findStandardRuleDetail(standardId, ruleId);
    if (!detail) return nullptr;
    const auto& examples = standardRuleExamples();
    const auto found = std::find_if(examples.begin(), examples.end(), [&](const auto& example) {
        return example.clauseId == detail->clauseId;
    });
    return found == examples.end() ? nullptr : &*found;
}

std::span<const StandardRuleBinding> standardRuleBindings() noexcept { return Bindings; }

std::string_view standardCoverageName(StandardCoverage coverage) noexcept {
    return coverage == StandardCoverage::Implemented ? "implemented" : "partial";
}

std::string_view standardAssessmentModeName(StandardAssessmentMode mode) noexcept {
    switch (mode) {
    case StandardAssessmentMode::Automated: return "automated";
    case StandardAssessmentMode::ProjectContext: return "project-context";
    case StandardAssessmentMode::ManualReview: return "manual-review";
    }
    return "unknown";
}

} // namespace fpga::analysis
