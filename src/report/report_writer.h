#pragma once

#include "analysis/analyzer.h"

#include <iosfwd>

namespace fpga::report {

void writeText(const analysis::AnalysisResult& result, std::ostream& output);
void writeJson(const analysis::AnalysisResult& result, std::ostream& output);
void writeSarif(const analysis::AnalysisResult& result, std::ostream& output);
void writeHtml(const analysis::AnalysisResult& result, std::ostream& output);
void writeLineStatisticsCsv(const analysis::AnalysisResult& result, std::ostream& output);

} // namespace fpga::report
