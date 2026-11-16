#pragma once

#include "core/source.h"

#include <filesystem>
#include <string>
#include <vector>

namespace fpga::parser {

struct PreprocessorOptions {
    std::vector<std::filesystem::path> includeDirectories;
    std::vector<std::string> defines;
};

struct PreprocessorIssue {
    std::string ruleId;
    std::string message;
    core::SourceLocation location;
};

struct PreprocessorResult {
    core::SourceFile source;
    std::vector<PreprocessorIssue> issues;
};

// Applies high-confidence Verilog conditional compilation while preserving
// byte length and line positions. Include files are resolved and checked, but
// are not textually injected into the returned source buffer.
[[nodiscard]] PreprocessorResult preprocess(
    const core::SourceFile& source, const PreprocessorOptions& options = {});

} // namespace fpga::parser
