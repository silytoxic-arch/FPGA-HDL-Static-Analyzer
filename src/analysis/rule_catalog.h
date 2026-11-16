#pragma once

#include "analysis/violation.h"

#include <optional>
#include <span>
#include <string_view>

namespace fpga::analysis {

struct RuleDefinition {
    std::string_view id;
    Severity severity;
    std::string_view languages;
    std::string_view title;
    std::string_view description;
    std::string_view recommendation;
};

[[nodiscard]] std::span<const RuleDefinition> ruleCatalog() noexcept;
[[nodiscard]] std::optional<RuleDefinition> findRule(std::string_view id) noexcept;
[[nodiscard]] std::optional<Severity> parseSeverity(std::string_view value) noexcept;

} // namespace fpga::analysis
