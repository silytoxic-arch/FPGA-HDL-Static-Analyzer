#include "parser/frontend_source_view.h"
#include <string_view>

namespace fpga::parser {
namespace {
std::vector<std::size_t> lineStarts(const std::string& text) {
    std::vector<std::size_t> starts{0};
    for(std::size_t i=0;i<text.size();++i) if(text[i]=='\n') starts.push_back(i+1);
    return starts;
}
std::string_view lineText(const std::string& text,const std::vector<std::size_t>& starts,
                          std::size_t line) {
    if(line==0 || line>starts.size()) return {};
    const auto begin=starts[line-1];
    auto end=line<starts.size() ? starts[line]-1 : text.size();
    if(end>begin && text[end-1]=='\r') --end;
    return std::string_view(text).substr(begin,end-begin);
}
}
FrontendSourceView::FrontendSourceView(const core::SourceFile& original,
    const FrontendPreprocessorResult& frontend):original_(original),frontend_(frontend),
    originalLines_(lineStarts(original.content())),expandedLines_(lineStarts(frontend.source.content())) {}

core::SourceLocation FrontendSourceView::report(const core::SourceLocation& parsed) const {
    if(parsed.file.empty() || parsed.file.lexically_normal()!=input().path().lexically_normal()) return parsed;
    auto origin=frontend_.origin(parsed.line);
    for(const auto& file:frontend_.ambiguousInputFiles)
        if(file.lexically_normal()==origin.file.lexically_normal()) return origin;
    // Equality of whole lines plus the parser's own offset/column agreement is
    // a bounded proof of unchanged literal coordinates. Macro/include/line
    // origins without that proof retain the established column 1/offset 0.
    if(origin.file.lexically_normal()==original_.path().lexically_normal() &&
       origin.line>0 && origin.line<=originalLines_.size() &&
       parsed.line>0 && parsed.line<=expandedLines_.size()) {
        const auto original=lineText(original_.content(),originalLines_,origin.line);
        const auto expanded=lineText(input().content(),expandedLines_,parsed.line);
        if(original==expanded && parsed.column>0 && parsed.column<=expanded.size()+1 &&
           parsed.byteOffset==expandedLines_[parsed.line-1]+parsed.column-1) {
            origin.column=parsed.column;
            origin.byteOffset=originalLines_[origin.line-1]+parsed.column-1;
        }
    }
    return origin;
}
void FrontendSourceView::reportProcess(SequentialProcessInfo& process) const {
    process.location=report(process.location);
    for(auto& reset:process.resets) reset.location=report(reset.location);
    for(auto& event:process.clockEvents) event.location=report(event.location);
}
DesignUnitInfo FrontendSourceView::report(DesignUnitInfo unit) const {
    unit.location=report(unit.location);
    for(auto& instance:unit.instances) {
        instance.location=report(instance.location);
        for(auto& connection:instance.namedConnections) {
            connection.location=report(connection.location);
            connection.actualLocation=report(connection.actualLocation);
        }
        for(auto& connection:instance.positionalConnections) connection.location=report(connection.location);
        for(auto& parameter:instance.namedParameterOverrides) parameter.location=report(parameter.location);
    }
    for(auto& port:unit.ports) port.location=report(port.location);
    for(auto& parameter:unit.parameters) parameter.location=report(parameter.location);
    for(auto& duplicate:unit.duplicatePortDeclarations) duplicate.location=report(duplicate.location);
    for(auto& duplicate:unit.duplicateParameterDeclarations) duplicate.location=report(duplicate.location);
    for(auto& clock:unit.clocks) clock.location=report(clock.location);
    for(auto& reset:unit.resets) reset.location=report(reset.location);
    for(auto& process:unit.sequentialProcesses) reportProcess(process);
    for(auto& synchronizer:unit.cdcSynchronizers) synchronizer.location=report(synchronizer.location);
    for(auto& transfer:unit.simpleSignalTransfers) transfer.location=report(transfer.location);
    return unit;
}
UnitTimingInfo FrontendSourceView::report(UnitTimingInfo timing) const {
    for(auto& clock:timing.clocks) clock.location=report(clock.location);
    for(auto& reset:timing.resets) reset.location=report(reset.location);
    for(auto& process:timing.sequentialProcesses) reportProcess(process);
    return timing;
}
} // namespace fpga::parser
