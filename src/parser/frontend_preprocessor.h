#pragma once
#include "parser/preprocessor.h"

namespace fpga::parser {
struct FrontendPreprocessorLimits {
    std::size_t maxOperations{256*1024*1024};
    std::size_t maxOutputBytes{64*1024*1024};
    std::size_t maxIncludeDepth{128};
    std::size_t maxMacroRestoreDepth{256};
    std::size_t maxInputBytes{64*1024*1024};
};
struct FrontendLineOrigin {
    std::filesystem::path file;
    std::size_t line{};
};
struct FrontendPreprocessorResult {
    core::SourceFile source;
    // One origin per virtual line, using the recovered lexer file-stack/line
    // protocol. Expanded offsets/columns are not physical source offsets.
    std::vector<FrontendLineOrigin> origins;
    std::vector<PreprocessorIssue> issues;
    std::vector<std::filesystem::path> includedFiles;
    [[nodiscard]] core::SourceLocation origin(std::size_t virtualLine) const;
};
// Separate frontend input: physical-line rules must keep preprocess()/original
// source. Each call is one configured compilation unit; project-wide macro
// cache sharing is not inferred. Default read_hdl defines YOSYS and SYNTHESIS;
// formal/nosynthesis modes are not exposed or guessed.
[[nodiscard]] FrontendPreprocessorResult expandFrontendSource(
    const core::SourceFile& source,const PreprocessorOptions& options={},
    const FrontendPreprocessorLimits& limits={});
} // namespace fpga::parser
