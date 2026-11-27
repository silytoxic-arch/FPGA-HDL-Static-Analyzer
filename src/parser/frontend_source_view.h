#pragma once
#include "parser/frontend_preprocessor.h"
#include "parser/semantic_analyzer.h"

namespace fpga::parser {
// Parsed locations belong to input(). Reporting produces separate values;
// callers must not use reported byte offsets to index the expanded text.
class FrontendSourceView {
public:
    FrontendSourceView(const core::SourceFile& original,
                       const FrontendPreprocessorResult& frontend);
    [[nodiscard]] const core::SourceFile& input() const { return frontend_.source; }
    [[nodiscard]] bool admitted() const { return frontend_.issues.empty(); }
    [[nodiscard]] core::SourceLocation report(const core::SourceLocation& parsed) const;
    [[nodiscard]] DesignUnitInfo report(DesignUnitInfo parsed) const;
    [[nodiscard]] UnitTimingInfo report(UnitTimingInfo parsed) const;
private:
    void reportProcess(SequentialProcessInfo& process) const;
    const core::SourceFile& original_;
    const FrontendPreprocessorResult& frontend_;
    std::vector<std::size_t> originalLines_, expandedLines_;
};
} // namespace fpga::parser
