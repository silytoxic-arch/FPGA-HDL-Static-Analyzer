#pragma once

#include "core/source.h"
#include "parser/frontend_preprocessor.h"
#include "parser/design_parser.h"

#include <span>
#include <string_view>

namespace fpga::analysis {

struct AnalysisOptions;
struct AnalysisResult;
enum class StandardRuleState;

void applyStandardProfile(AnalysisResult& result,
                          std::span<const core::SourceFile> sources,
                          const AnalysisOptions& options,
                          std::span<const parser::FrontendPreprocessorResult* const> frontendInputs,
                          std::vector<parser::DesignUnitInfo> sourceBoundStructure);

[[nodiscard]] std::string_view standardRuleStateName(StandardRuleState state) noexcept;
[[nodiscard]] bool hasAutomatedStandardDetector(std::string_view clauseId) noexcept;

} // namespace fpga::analysis
