#include "analysis/line_statistics.h"
#include "analysis/analyzer.h"
#include "report/report_writer.h"
#include <iostream>
#include <sstream>

int main() {
    using namespace fpga;
    int failures = 0;
    const auto check = [&](bool ok, const char* message) {
        if (!ok) { ++failures; std::cerr << "FAIL: " << message << '\n'; }
    };
    const auto counts = [&](core::HdlLanguage language, const std::string& source,
                            std::size_t total, std::size_t code, std::size_t comment, std::size_t blank) {
        const auto result = analysis::countSourceLines({"test.sv", language, source});
        check(result.lines.total == total && result.lines.code == code &&
              result.lines.comment == comment && result.lines.blank == blank, "physical line classification");
        check(result.lines.total == result.lines.code + result.lines.comment + result.lines.blank,
              "each physical line belongs to exactly one category");
    };
    counts(core::HdlLanguage::Verilog, "", 0, 0, 0, 0);
    counts(core::HdlLanguage::Verilog, "\n", 1, 0, 0, 1);
    counts(core::HdlLanguage::Verilog, "\xEF\xBB\xBF  \n// comment", 2, 0, 1, 1);
    counts(core::HdlLanguage::Verilog, "module a;\nendmodule\n", 2, 2, 0, 0);
    counts(core::HdlLanguage::SystemVerilog,
           "// 中文注释\r\n  \r\n/* block\r\n\r\n end */ logic a; // mixed\r\nendmodule",
           6, 2, 3, 1);
    counts(core::HdlLanguage::SystemVerilog,
           "string s = \"// /* not comment \\\"\";\nwire \\a//b ;\n/*only*/\n/*a*/ // b\n",
           4, 2, 2, 0);
    counts(core::HdlLanguage::SystemVerilog,
           "string s = \"first\\\r\n// continuation\";\r\n// real comment\r\n", 3, 2, 1, 0);
    counts(core::HdlLanguage::Vhdl,
           "-- 注释\n\nreport \"-- with \"\"quotes\"\"\"; -- mixed\nx <= '\"'; -- mixed\n/* start\n\nend */\n",
           7, 2, 4, 1);
    analysis::AnalysisOptions options;
    options.countOnly = true;
    const auto result = analysis::Analyzer(options).analyzeSources({
        {"rtl/a.sv", core::HdlLanguage::SystemVerilog, "// test\nmodule broken"},
        {"rtl/b.vhd", core::HdlLanguage::Vhdl, "-- test\n\nentity broken"}});
    check(result.filesAnalyzed == 2 && result.lineStatistics.total.total == 5 &&
          result.lineStatistics.languages.size() == 2 && result.lineStatistics.files.size() == 2,
          "multiple files aggregate without preprocessing or duplicate counting");
    check(result.violations.empty() && result.structure.empty(), "count-only works on incomplete HDL");
    const auto normal = analysis::Analyzer().analyzeSource(
        {"a.v", core::HdlLanguage::Verilog, "// header\nmodule a;\nendmodule\n"});
    check(normal.lineStatistics.total.total == 3 && normal.lineStatistics.total.comment == 1 &&
          normal.designUnits == 1, "normal lint also includes original source statistics");
    std::ostringstream json, html, csv;
    report::writeJson(result, json); report::writeHtml(result, html); report::writeLineStatisticsCsv(result, csv);
    check(json.str().find("\"total\":5,\"code\":2,\"comment\":2,\"blank\":1") != std::string::npos,
          "JSON exports consistent physical line totals");
    check(html.str().find("逐文件行数") != std::string::npos && html.str().find("rtl/a.sv") != std::string::npos,
          "HTML contains file detail");
    check(csv.str().starts_with("\xEF\xBB\xBF") && csv.str().find("汇总,\"全部 HDL\",2,5,2,2,1") != std::string::npos,
          "CSV is UTF-8 Excel compatible");
    analysis::AnalysisResult escaping;
    escaping.lineStatistics.append({"a,\"b.sv", core::HdlLanguage::SystemVerilog, {1, 1, 0, 0}});
    std::ostringstream escapedCsv;
    report::writeLineStatisticsCsv(escaping, escapedCsv);
    check(escapedCsv.str().find("\"a,\"\"b.sv\"") != std::string::npos, "CSV escapes quotes and commas");
    return failures ? 1 : 0;
}
