#include "cli/command_line.h"
#include "analysis/analyzer.h"
#include "analysis/baseline.h"
#include "analysis/rule_catalog.h"
#include "analysis/rule_localization.h"
#include "analysis/standard_profiles.h"
#include "core/application_info.h"
#include "project/config_file.h"
#include "project/file_list.h"
#include "report/report_writer.h"

#include <filesystem>
#include <fstream>
#include <iterator>
#include <optional>
#include <ostream>
#include <string>
#include <vector>

namespace fpga::cli {
namespace {
std::string_view severityChinese(analysis::Severity severity) {
    switch (severity) {
    case analysis::Severity::Critical: return "严重";
    case analysis::Severity::Error: return "错误";
    case analysis::Severity::Warning: return "警告";
    case analysis::Severity::Info: return "提示";
    }
    return "未知";
}

void help(std::ostream& out) {
    const auto info = application_info();
    out << info.name << "\n"
        << "用法：fpga_analyzer_cli [选项] <文件或目录>...\n\n"
        << "选项：\n"
        << "  -h, --help          显示此帮助\n"
        << "  --version           显示版本\n"
        << "  --list-rules        列出全部内建规则\n"
        << "  --list-standards    列出 GJB 标准规则集及当前实现覆盖\n"
        << "  --list-standard-rules ID  列出标准条款、级别、强制属性和实现映射\n"
        << "  --standard ID       选择规则集：gjb10157 或 gjb9765（默认：gjb10157）\n"
        << "  --top-module NAME   指定唯一顶层 module/entity（顶层相关规则使用）\n"
        << "  --fsm-priority P    FSM 优化目标：area（面积）、speed（速度）、unspecified（未指定，默认）\n"
        << "  --fpga-technology T 器件类型：sram、non-sram、unspecified（默认）\n"
        << "  --initial-blocks P 上电初始化 initial 策略：accept、ignore、unspecified（默认）\n"
        << "  --line-count        仅统计原始 HDL 源码行数，不运行规则检查\n"
        << "  --describe-rule ID  显示指定规则的元数据和修改建议\n"
        << "  --format FORMAT     text、json、sarif、html 或 csv（行数统计，默认：text）\n\n"
        << "  -o, --output FILE   将报告写入文件，而不是标准输出\n"
        << "  --fail-on LEVEL     达到指定级别时返回 1：info、warning、error、critical、none\n"
        << "                      （默认：warning）\n\n"
        << "  --config FILE       加载工程配置（默认读取当前目录的 .fpga-lint.conf）\n"
        << "                      文件头策略：header-policy；关键字：header-keyword（见项目配置文档）\n"
        << "                      命名关键字：keyword-policy、keyword-category（厂商默认类别未知）\n"
        << "  -f, --file-list F   从清单加载 HDL 输入、include 路径和宏\n"
        << "                      保留输入/清单顺序；目录内按路径排序，跨文件宏请按定义顺序列出\n"
        << "  --baseline FILE     忽略基线文件中已记录的问题\n"
        << "  --write-baseline F  写入当前问题基线并成功退出\n"
        << "  --disable RULE      禁用规则，可重复指定\n"
        << "  --severity R=L      覆盖规则级别：info、warning、error 或 critical\n"
        << "  --blackbox UNIT     将未解析 module/entity 作为外部 IP\n"
        << "  --exclude GLOB      排除匹配的 HDL 路径，可重复指定\n\n"
        << "  -I, --include-dir D 添加 Verilog/SystemVerilog include 目录\n"
        << "  -D, --define MACRO  定义条件编译宏 MACRO 或 MACRO=value\n\n"
        << "退出码：0 无阻断问题，1 发现达到阈值的问题，2 输入或分析错误。\n"
        << "支持扩展名：.v .vh .sv .svh .vhd .vhdl\n";
}
} // namespace

int run(std::span<const std::string_view> arguments, std::ostream& out, std::ostream& err) {
    const auto info = application_info();
    if (arguments.empty() || (arguments.size() == 1 &&
        (arguments[0] == "--help" || arguments[0] == "-h"))) {
        help(out);
        return 0;
    }
    if (arguments.size() == 1 && arguments[0] == "--version") {
        out << info.name << ' ' << info.version << '\n';
        return 0;
    }
    if (arguments.size() == 1 && arguments[0] == "--list-rules") {
        out << "规则 ID\t严重级\t适用语言\t规则摘要\n";
        for (const auto& rule : analysis::ruleCatalog()) {
            out << rule.id << '\t' << severityChinese(rule.severity) << '\t'
                << rule.languages << '\t' << analysis::chineseRuleSummary(rule.id) << '\n';
        }
        return 0;
    }
    if (arguments.size() == 1 && arguments[0] == "--list-standards") {
        out << "标准 ID\t语言\t规则数\t强制规则\t自动检查\t工程上下文\t人工复核\t名称\n";
        for (const auto& profile : analysis::standardProfiles()) {
            std::size_t automated = 0;
            std::size_t projectContext = 0;
            std::size_t manualReview = 0;
            for (const auto& entry : analysis::standardRuleInventory()) {
                if (entry.standardId != profile.id) continue;
                const auto detail = analysis::findStandardRuleDetail(entry.clauseId);
                if (!detail) continue;
                if (detail->assessmentMode == analysis::StandardAssessmentMode::Automated) ++automated;
                else if (detail->assessmentMode == analysis::StandardAssessmentMode::ProjectContext) ++projectContext;
                else ++manualReview;
            }
            out << profile.id << '\t' << profile.language << '\t' << profile.observedRules
                << '\t' << profile.compulsoryRules << '\t' << automated << '\t'
                << projectContext << '\t' << manualReview << '\t' << profile.title << '\n';
        }
        out << "说明：209 条均登记；工程上下文与人工复核项不会被静默判定为通过。\n";
        return 0;
    }
    if (arguments.size() == 1 && arguments[0] == "--list-standard-rules") {
        err << "--list-standard-rules 缺少标准 ID。\n";
        return 2;
    }
    if (arguments.size() == 2 && arguments[0] == "--list-standard-rules") {
        const auto profile = analysis::findStandardProfile(arguments[1]);
        if (!profile) {
            err << "未知标准 ID：" << arguments[1]
                << "。请使用 --list-standards 查看标准规则集。\n";
            return 2;
        }
        out << profile->id << "（" << profile->language << "）\n"
            << "规则编号\t严重级\t强制\t评估方式\t规则摘要\n";
        for (const auto& entry : analysis::standardRuleInventory()) {
            if (entry.standardId != profile->id) continue;
            const auto detail = analysis::findStandardRuleDetail(entry.clauseId);
            out << analysis::standardRuleDisplayId(entry.clauseId) << '\t'
                << severityChinese(entry.severity) << '\t'
                << (entry.compulsory ? "是" : "否") << '\t'
                << (detail ? analysis::standardAssessmentModeName(detail->assessmentMode) : "missing")
                << '\t' << (detail ? detail->summary : "缺少规则元数据");
            out << '\n';
        }
        return 0;
    }
    if (arguments.size() == 1 && arguments[0] == "--describe-rule") {
        err << "--describe-rule 缺少规则 ID。\n";
        return 2;
    }
    if (arguments.size() == 2 && arguments[0] == "--describe-rule") {
        const auto rule = analysis::findRule(arguments[1]);
        if (!rule) {
            const auto separator = arguments[1].find(':');
            const auto detail = separator == std::string_view::npos
                ? analysis::findStandardRuleDetail(arguments[1])
                : analysis::findStandardRuleDetail(arguments[1].substr(0, separator),
                      arguments[1].substr(separator + 1));
            const auto* example = separator == std::string_view::npos
                ? analysis::findStandardRuleExample(arguments[1])
                : analysis::findStandardRuleExample(arguments[1].substr(0, separator),
                      arguments[1].substr(separator + 1));
            if (!detail || !example) {
                err << "未知规则 ID 或跨标准歧义：" << arguments[1]
                    << "。公开编号重复时请使用 标准ID:规则ID，例如 GJB9765:R-2-1。\n";
                return 2;
            }
            out << analysis::standardRuleDisplayId(detail->clauseId) << '\n'
                << "判定方式：" << analysis::standardAssessmentModeName(detail->assessmentMode) << '\n'
                << "规则摘要：" << detail->summary << '\n'
                << example->compliant << '\n'
                << example->violating << '\n'
                << "证据：" << example->evidenceBasis << '\n';
            return 0;
        }
        out << rule->id << '\n'
            << "严重级：" << severityChinese(rule->severity) << '\n'
            << "适用语言：" << rule->languages << '\n'
            << "规则摘要：" << analysis::chineseRuleSummary(rule->id) << '\n'
            << "修改建议：请根据规则要求检查并修正相关 HDL 代码；如果该结构属于明确的设计意图，"
               "可在评审后添加局部规则豁免。\n";
        return 0;
    }
    std::optional<std::filesystem::path> configPath;
    for (std::size_t i = 0; i < arguments.size(); ++i) {
        if (arguments[i] == "--config") {
            if (++i == arguments.size()) {
                err << "--config 缺少文件路径。\n";
                return 2;
            }
            configPath = std::filesystem::path(arguments[i]);
        }
    }
    if (!configPath && std::filesystem::is_regular_file(".fpga-lint.conf")) {
        configPath = ".fpga-lint.conf";
    }
    project::ConfigFile config;
    if (configPath) {
        auto loaded = project::loadConfigFile(*configPath);
        if (!loaded.errors.empty()) {
            for (const auto& error : loaded.errors) err << error << '\n';
            return 2;
        }
        config = std::move(loaded.config);
    }

    std::string format = "text";
    std::string failOn = config.failOn.value_or("warning");
    auto analysisOptions = std::move(config.analysis);
    std::optional<std::filesystem::path> baselinePath = std::move(config.baseline);
    std::optional<std::filesystem::path> writeBaselinePath;
    bool explicitBaseline = false;
    std::optional<std::filesystem::path> outputPath;
    std::vector<std::filesystem::path> inputs;
    const auto appendFileList = [&](const std::filesystem::path& path) {
        auto loaded = project::loadFileList(path);
        if (!loaded.errors.empty()) {
            for (const auto& error : loaded.errors) err << error << '\n';
            return false;
        }
        inputs.insert(inputs.end(),
                      std::make_move_iterator(loaded.inputs.begin()),
                      std::make_move_iterator(loaded.inputs.end()));
        analysisOptions.includeDirectories.insert(
            analysisOptions.includeDirectories.end(),
            std::make_move_iterator(loaded.includeDirectories.begin()),
            std::make_move_iterator(loaded.includeDirectories.end()));
        analysisOptions.defines.insert(
            analysisOptions.defines.end(),
            std::make_move_iterator(loaded.defines.begin()),
            std::make_move_iterator(loaded.defines.end()));
        return true;
    };
    for (const auto& fileList : config.fileLists) {
        if (!appendFileList(fileList)) return 2;
    }
    for (std::size_t i = 0; i < arguments.size(); ++i) {
        if (arguments[i] == "--line-count") {
            analysisOptions.countOnly = true;
        } else if (arguments[i] == "--standard") {
            if (++i == arguments.size()) {
                err << "--standard 缺少标准 ID。\n";
                return 2;
            }
            const auto profile = analysis::findStandardProfile(arguments[i]);
            if (!profile) {
                err << "未知标准 ID：" << arguments[i]
                    << "。仅支持 gjb10157 或 gjb9765。\n";
                return 2;
            }
            analysisOptions.standardId = std::string(profile->id);
        } else if (arguments[i] == "--top-module") {
            if (++i == arguments.size() || arguments[i].empty() || arguments[i].front() == '-') {
                err << "--top-module 缺少顶层设计名称。\n";
                return 2;
            }
            analysisOptions.topModule = std::string(arguments[i]);
        } else if (arguments[i] == "--fpga-technology") {
            if (++i == arguments.size()) { err << "--fpga-technology 缺少参数值。\n"; return 2; }
            const auto technology = analysis::parseFpgaTechnology(arguments[i]);
            if (!technology) { err << "器件类型须为 sram、non-sram 或 unspecified。\n"; return 2; }
            analysisOptions.powerOnPolicy.technology = *technology;
        } else if (arguments[i] == "--initial-blocks") {
            if (++i == arguments.size()) { err << "--initial-blocks 缺少参数值。\n"; return 2; }
            if (arguments[i] == "accept") analysisOptions.powerOnPolicy.acceptInitialBlocks = true;
            else if (arguments[i] == "ignore") analysisOptions.powerOnPolicy.acceptInitialBlocks = false;
            else if (arguments[i] == "unspecified") analysisOptions.powerOnPolicy.acceptInitialBlocks.reset();
            else { err << "initial 策略须为 accept、ignore 或 unspecified。\n"; return 2; }
        } else if (arguments[i] == "--fsm-priority") {
            if (++i == arguments.size()) {
                err << "--fsm-priority 缺少参数值，应为 area、speed 或 unspecified。\n";
                return 2;
            }
            const auto priority = analysis::parseFsmOptimizationPriority(arguments[i]);
            if (!priority) {
                err << "--fsm-priority 值无效，应为 area、speed 或 unspecified。\n";
                return 2;
            }
            analysisOptions.fsmOptimizationPriority = *priority;
        } else if (arguments[i] == "--format") {
            if (++i == arguments.size() ||
                (arguments[i] != "text" && arguments[i] != "json" &&
                 arguments[i] != "sarif" && arguments[i] != "html" && arguments[i] != "csv")) {
                err << "--format 值无效，应为 text、json、sarif、html 或 csv。\n";
                return 2;
            }
            format = arguments[i];
        } else if (arguments[i] == "--output" || arguments[i] == "-o") {
            const auto option = arguments[i];
            if (++i == arguments.size()) {
                err << option << " 缺少参数值。\n";
                return 2;
            }
            outputPath = std::filesystem::path(arguments[i]);
        } else if (arguments[i] == "--config") {
            ++i;
        } else if (arguments[i] == "--file-list" || arguments[i] == "-f") {
            const auto option = arguments[i];
            if (++i == arguments.size()) {
                err << option << " 缺少参数值。\n";
                return 2;
            }
            if (!appendFileList(std::filesystem::path(arguments[i]))) return 2;
        } else if (arguments[i] == "--baseline" || arguments[i] == "--write-baseline") {
            const auto option = arguments[i];
            if (++i == arguments.size()) {
                err << option << " 缺少参数值。\n";
                return 2;
            }
            if (option == "--baseline") {
                baselinePath = std::filesystem::path(arguments[i]);
                explicitBaseline = true;
            } else {
                writeBaselinePath = std::filesystem::path(arguments[i]);
            }
        } else if (arguments[i] == "--fail-on") {
            if (++i == arguments.size() ||
                (arguments[i] != "info" && arguments[i] != "warning" &&
                 arguments[i] != "error" && arguments[i] != "critical" &&
                 arguments[i] != "none")) {
                err << "--fail-on 值无效，应为 info、warning、error、critical 或 none。\n";
                return 2;
            }
            failOn = arguments[i];
        } else if (arguments[i] == "--disable" || arguments[i] == "--blackbox" ||
                   arguments[i] == "--exclude" || arguments[i] == "--include-dir" ||
                   arguments[i] == "-I" || arguments[i] == "--define" || arguments[i] == "-D" ||
                   arguments[i] == "--severity") {
            const auto option = arguments[i];
            if (++i == arguments.size()) {
                err << option << " 缺少参数值。\n";
                return 2;
            }
            if (option == "--disable") {
                if (arguments[i] != "all" && !analysis::findRule(arguments[i]) &&
                    !analysis::findStandardRuleDetail(arguments[i])) {
                    err << "未知规则 ID：" << arguments[i] << "。请使用 --list-rules 查看规则。\n";
                    return 2;
                }
                analysisOptions.disabledRules.emplace_back(arguments[i]);
            } else if (option == "--severity") {
                const auto value = arguments[i];
                const auto separator = value.find('=');
                if (separator == std::string_view::npos || separator == 0 ||
                    separator + 1 == value.size() || value.find('=', separator + 1) != std::string_view::npos) {
                    err << "--severity 值无效，应为 RULE=info|warning|error|critical。\n";
                    return 2;
                }
                const auto rule = value.substr(0, separator);
                const auto severity = analysis::parseSeverity(value.substr(separator + 1));
                if (!analysis::findRule(rule) && !analysis::findStandardRuleDetail(rule)) {
                    err << "未知规则 ID：" << rule << "。请使用 --list-rules 查看规则。\n";
                    return 2;
                }
                if (!severity) {
                    err << "--severity 严重级无效，应为 info、warning、error 或 critical。\n";
                    return 2;
                }
                analysisOptions.severityOverrides.push_back({std::string(rule), *severity});
            }
            else if (option == "--blackbox") analysisOptions.blackBoxes.emplace_back(arguments[i]);
            else if (option == "--exclude") analysisOptions.excludePatterns.emplace_back(arguments[i]);
            else if (option == "--include-dir" || option == "-I") {
                analysisOptions.includeDirectories.emplace_back(arguments[i]);
            } else {
                analysisOptions.defines.emplace_back(arguments[i]);
            }
        } else if (arguments[i].starts_with("-I") && arguments[i].size() > 2) {
            analysisOptions.includeDirectories.emplace_back(arguments[i].substr(2));
        } else if (arguments[i].starts_with("-D") && arguments[i].size() > 2) {
            analysisOptions.defines.emplace_back(arguments[i].substr(2));
        } else if (arguments[i].starts_with("-f") && arguments[i].size() > 2) {
            if (!appendFileList(std::filesystem::path(arguments[i].substr(2)))) return 2;
        } else if (!arguments[i].empty() && arguments[i].front() == '-') {
            err << "未知选项：" << arguments[i] << "。请使用 --help 查看帮助。\n";
            return 2;
        } else {
            inputs.emplace_back(arguments[i]);
        }
    }
    if (inputs.empty()) {
        err << "未提供 HDL 输入。请使用 --help 查看帮助。\n";
        return 2;
    }
    if (analysisOptions.countOnly) {
        if (explicitBaseline || writeBaselinePath) {
            err << "--line-count 不能与 --baseline 或 --write-baseline 同时使用。\n";
            return 2;
        }
        baselinePath.reset();
    }
    if (explicitBaseline && writeBaselinePath) {
        err << "--baseline 与 --write-baseline 不能同时使用。\n";
        return 2;
    }
    if (writeBaselinePath) baselinePath.reset();
    if (outputPath && writeBaselinePath &&
        std::filesystem::absolute(*outputPath).lexically_normal() ==
            std::filesystem::absolute(*writeBaselinePath).lexically_normal()) {
        err << "--output 与 --write-baseline 必须使用不同文件。\n";
        return 2;
    }

    const analysis::Analyzer analyzer(std::move(analysisOptions));
    auto result = analyzer.analyzePaths(inputs);
    if (baselinePath) {
        auto baseline = analysis::loadBaseline(*baselinePath);
        if (!baseline.errors.empty()) {
            for (const auto& error : baseline.errors) err << error << '\n';
            return 2;
        }
        analysis::applyBaseline(result, baseline.entries);
    }
    if (writeBaselinePath && result.errors.empty() && result.filesAnalyzed != 0) {
        std::string baselineError;
        if (!analysis::writeBaseline(result, *writeBaselinePath, baselineError)) {
            err << baselineError << '\n';
            return 2;
        }
    }
    std::ofstream reportFile;
    std::ostream* reportOutput = &out;
    if (outputPath) {
        reportFile.open(*outputPath, std::ios::binary | std::ios::trunc);
        if (!reportFile) {
            err << "无法写入报告文件：" << outputPath->string() << '\n';
            return 2;
        }
        reportOutput = &reportFile;
    }
    if (format == "json") report::writeJson(result, *reportOutput);
    else if (format == "sarif") report::writeSarif(result, *reportOutput);
    else if (format == "html") report::writeHtml(result, *reportOutput);
    else if (format == "csv") report::writeLineStatisticsCsv(result, *reportOutput);
    else report::writeText(result, *reportOutput);
    reportOutput->flush();
    if (!*reportOutput) {
        err << "写入分析报告失败。\n";
        return 2;
    }
    if (!result.errors.empty() || result.filesAnalyzed == 0) return 2;
    if (writeBaselinePath) return 0;
    if (failOn == "none") return 0;
    const auto rank = [](analysis::Severity severity) {
        switch (severity) {
        case analysis::Severity::Info: return 0;
        case analysis::Severity::Warning: return 1;
        case analysis::Severity::Error: return 2;
        case analysis::Severity::Critical: return 3;
        }
        return 4;
    };
    const int threshold = failOn == "info" ? 0 : failOn == "warning" ? 1 :
                          failOn == "error" ? 2 : 3;
    for (const auto& violation : result.violations) {
        if (rank(violation.severity) >= threshold) return 1;
    }
    return 0;
}
}
