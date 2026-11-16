#pragma once

#include "core/source.h"

#include <span>
#include <string_view>

namespace fpga::analysis {

struct AnalysisOptions;
struct AnalysisResult;
enum class StandardRuleState;

void applyStandardProfile(AnalysisResult& result,
                          std::span<const core::SourceFile> sources,
                          const AnalysisOptions& options);

[[nodiscard]] std::string_view standardRuleStateName(StandardRuleState state) noexcept;
[[nodiscard]] bool hasAutomatedStandardDetector(std::string_view clauseId) noexcept;

} // namespace fpga::analysis
