#pragma once
#include "parser/preprocessor.h"
#include <memory>

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
    // Exact initial defined-name set after configured/cache merge. The legacy
    // physical-position projection uses presence only, never guessed bodies.
    std::vector<std::string> initialMacroNames;
    [[nodiscard]] core::SourceLocation origin(std::size_t virtualLine) const;
};
// One native-style design import session. Source defines/undef/resetall update
// a cache shared by subsequent roots in their explicit processing order.
// Construction/reset models a fresh public clear() boundary. No cache escapes
// into another analyzer invocation, and no opaque vendor state is executed.
class FrontendPreprocessorSession {
public:
    FrontendPreprocessorSession();
    ~FrontendPreprocessorSession();
    FrontendPreprocessorSession(const FrontendPreprocessorSession&)=delete;
    FrontendPreprocessorSession& operator=(const FrontendPreprocessorSession&)=delete;
    [[nodiscard]] FrontendPreprocessorResult expand(
        const core::SourceFile& source,const PreprocessorOptions& options={},
        const FrontendPreprocessorLimits& limits={});
    void reset();
private:
    struct State;
    std::unique_ptr<State> state_;
};
// Isolated single-root convenience call. Physical-line rules keep their
// original input. Default read_hdl defines YOSYS and SYNTHESIS;
// formal/nosynthesis modes are not exposed or guessed.
[[nodiscard]] FrontendPreprocessorResult expandFrontendSource(
    const core::SourceFile& source,const PreprocessorOptions& options={},
    const FrontendPreprocessorLimits& limits={});
} // namespace fpga::parser
