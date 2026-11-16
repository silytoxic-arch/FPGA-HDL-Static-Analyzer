#pragma once

#include "analysis/analyzer.h"

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace fpga::project {

struct ConfigFile {
    analysis::AnalysisOptions analysis;
    std::optional<std::string> failOn;
    std::optional<std::filesystem::path> baseline;
    std::vector<std::filesystem::path> fileLists;
};

struct ConfigLoadResult {
    ConfigFile config;
    std::vector<std::string> errors;
};

// Line-oriented, dependency-free project configuration. Supported directives:
// standard, top-module, disable, severity, blackbox, exclude, include-dir,
// define, fail-on, baseline, and file-list.
// Blank lines and # comments are ignored.
[[nodiscard]] ConfigLoadResult loadConfigFile(const std::filesystem::path& path);

} // namespace fpga::project
