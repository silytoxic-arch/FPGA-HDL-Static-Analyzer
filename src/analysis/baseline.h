#pragma once

#include "analysis/analyzer.h"

#include <filesystem>
#include <string>
#include <vector>

namespace fpga::analysis {

struct BaselineEntry {
    std::string ruleId;
    std::string severity;
    std::filesystem::path file;
    std::string message;
    std::string designUnit;
    std::string signal;
};

struct BaselineLoadResult {
    std::vector<BaselineEntry> entries;
    std::vector<std::string> errors;
};

[[nodiscard]] BaselineLoadResult loadBaseline(const std::filesystem::path& path);

// Removes only the counted findings present in the baseline and returns the
// number removed. Line/column are deliberately excluded from the fingerprint.
std::size_t applyBaseline(AnalysisResult& result,
                          const std::vector<BaselineEntry>& entries);

[[nodiscard]] bool writeBaseline(const AnalysisResult& result,
                                 const std::filesystem::path& path,
                                 std::string& error);

} // namespace fpga::analysis
