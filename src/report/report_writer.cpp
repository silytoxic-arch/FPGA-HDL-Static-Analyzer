#include "report/report_writer.h"
#include "analysis/rule_catalog.h"
#include "analysis/rule_localization.h"
#include "analysis/standard_evaluator.h"
#include "analysis/standard_profiles.h"
#include "core/application_info.h"

#include <cctype>
#include <map>
#include <ostream>
#include <sstream>
#include <string_view>

namespace fpga::report {
using analysis::lineStatisticsLanguageName;
namespace {
std::string_view diagnosticSummary(std::string_view ruleId) {
    if (const auto detail = analysis::findStandardRuleDetail(ruleId)) return detail->summary;
    return analysis::chineseRuleSummary(ruleId);
}

std::string reportedRuleId(std::string_view ruleId) {
    return analysis::standardRuleDisplayId(ruleId);
}

std::string statisticsPath(const std::filesystem::path& path) {
    const auto utf8 = path.generic_u8string();
    return std::string(utf8.begin(), utf8.end());
}
void jsonString(std::ostream& output, std::string_view text) {
    output << '"';
    for (const unsigned char character : text) {
        switch (character) {
        case '"': output << "\\\""; break;
        case '\\': output << "\\\\"; break;
        case '\b': output << "\\b"; break;
        case '\f': output << "\\f"; break;
        case '\n': output << "\\n"; break;
        case '\r': output << "\\r"; break;
        case '\t': output << "\\t"; break;
        default:
            if (character < 0x20) {
                constexpr char digits[] = "0123456789abcdef";
                output << "\\u00" << digits[character >> 4] << digits[character & 0x0f];
            } else {
                output << static_cast<char>(character);
            }
        }
    }
    output << '"';
}

void htmlText(std::ostream& output, std::string_view text) {
    for (const char character : text) {
        switch (character) {
        case '&': output << "&amp;"; break;
        case '<': output << "&lt;"; break;
        case '>': output << "&gt;"; break;
        case '"': output << "&quot;"; break;
        case '\'': output << "&#39;"; break;
        default: output << character;
        }
    }
}

std::string sarifLevel(analysis::Severity severity) {
    switch (severity) {
    case analysis::Severity::Critical:
    case analysis::Severity::Error: return "error";
    case analysis::Severity::Warning: return "warning";
    case analysis::Severity::Info: return "note";
    }
    return "none";
}

std::string_view severityChinese(analysis::Severity severity) {
    switch (severity) {
    case analysis::Severity::Critical: return "严重";
    case analysis::Severity::Error: return "错误";
    case analysis::Severity::Warning: return "警告";
    case analysis::Severity::Info: return "提示";
    }
    return "未知";
}

std::string_view designUnitKindChinese(parser::DesignUnitKind kind) {
    switch (kind) {
    case parser::DesignUnitKind::VerilogModule: return "Verilog 模块";
    case parser::DesignUnitKind::VhdlEntity: return "VHDL 实体";
    }
    return "未知";
}

std::string_view portDirectionChinese(parser::PortDirection direction) {
    switch (direction) {
    case parser::PortDirection::Input: return "输入";
    case parser::PortDirection::Output: return "输出";
    case parser::PortDirection::Inout: return "双向";
    case parser::PortDirection::Unknown: return "未知";
    }
    return "未知";
}

std::string uriReference(const std::filesystem::path& path) {
    const auto value = path.generic_string();
    std::ostringstream encoded;
    constexpr char digits[] = "0123456789ABCDEF";
    for (const unsigned char character : value) {
        if (std::isalnum(character) || character == '/' || character == ':' ||
            character == '.' || character == '_' || character == '-' || character == '~') {
            encoded << static_cast<char>(character);
        } else {
            encoded << '%' << digits[character >> 4] << digits[character & 0x0f];
        }
    }
    return encoded.str();
}

struct RuleMetadata {
    analysis::Severity severity;
    std::string message;
    std::string description;
    std::string recommendation;
};

struct ViolationSummary {
    std::size_t critical{0};
    std::size_t error{0};
    std::size_t warning{0};
    std::size_t info{0};
    std::map<std::string, std::size_t> rules;
};

ViolationSummary summarizeViolations(const analysis::AnalysisResult& result) {
    ViolationSummary summary;
    for (const auto& violation : result.violations) {
        switch (violation.severity) {
        case analysis::Severity::Critical: ++summary.critical; break;
        case analysis::Severity::Error: ++summary.error; break;
        case analysis::Severity::Warning: ++summary.warning; break;
        case analysis::Severity::Info: ++summary.info; break;
        }
        ++summary.rules[reportedRuleId(violation.ruleId)];
    }
    return summary;
}

std::map<std::string, RuleMetadata> ruleMetadata(const analysis::AnalysisResult& result) {
    std::map<std::string, RuleMetadata> rules;
    for (const auto& rule : analysis::ruleCatalog()) {
        rules.emplace(rule.id, RuleMetadata{rule.severity, std::string(rule.title),
                      std::string(rule.description), std::string(rule.recommendation)});
    }
    for (const auto& violation : result.violations) {
        rules.try_emplace(reportedRuleId(violation.ruleId), RuleMetadata{violation.severity, violation.message,
                          violation.description, violation.recommendation});
    }
    return rules;
}
} // namespace

void writeLineStatisticsCsv(const analysis::AnalysisResult& result, std::ostream& output) {
    // UTF-8 BOM lets Windows Excel recognize Chinese column names.
    output << "\xEF\xBB\xBF分类,文件或语言,文件数,总行数,代码行,注释行,空白行\r\n";
    const auto row = [&](std::string_view category, std::string_view name,
                         std::size_t files, const analysis::LineCounts& lines) {
        output << category << ",\"";
        for (char c : name) { if (c == '"') output << '"'; output << c; }
        output << "\"," << files << ',' << lines.total << ',' << lines.code << ','
               << lines.comment << ',' << lines.blank << "\r\n";
    };
    row("汇总", "全部 HDL", result.lineStatistics.files.size(), result.lineStatistics.total);
    for (const auto& language : result.lineStatistics.languages)
        row("语言", lineStatisticsLanguageName(language.language), language.files, language.lines);
    for (const auto& file : result.lineStatistics.files)
        row("文件", statisticsPath(file.file), 1, file.lines);
}

void writeText(const analysis::AnalysisResult& result, std::ostream& output) {
    const auto violationSummary = summarizeViolations(result);
    const auto& statistics = result.lineStatistics;
    output << "代码行数统计：总行数 " << statistics.total.total << "，代码行 "
           << statistics.total.code << "，注释行 " << statistics.total.comment
           << "，空白行 " << statistics.total.blank << "。\n";
    output << "统计口径：混合代码/注释行计为代码行；统计原始源码，包含条件编译分支。\n";
    for (const auto& language : statistics.languages) {
        output << lineStatisticsLanguageName(language.language) << "：" << language.files
               << " 个文件，总行数 " << language.lines.total << "，代码行 " << language.lines.code
               << "，注释行 " << language.lines.comment << "，空白行 " << language.lines.blank << "。\n";
    }
    for (const auto& file : statistics.files) {
        output << statisticsPath(file.file) << "：总行数 " << file.lines.total << "，代码行 "
               << file.lines.code << "，注释行 " << file.lines.comment << "，空白行 " << file.lines.blank << "。\n";
    }
    for (const auto& violation : result.violations) {
        output << violation.file.string() << ':' << violation.line << ':' << violation.column
               << ": " << severityChinese(violation.severity) << ' '
               << reportedRuleId(violation.ruleId) << ": " << diagnosticSummary(violation.ruleId) << '\n';
    }
    for (const auto& error : result.errors) output << "错误：" << error << '\n';
    std::size_t instanceCount = 0;
    std::size_t topLevelCount = 0;
    std::size_t clockCount = 0;
    std::size_t resetCount = 0;
    std::size_t sequentialProcessCount = 0;
    std::size_t cdcSynchronizerCount = 0;
    for (const auto& unit : result.structure) {
        instanceCount += unit.instances.size();
        clockCount += unit.clocks.size();
        resetCount += unit.resets.size();
        sequentialProcessCount += unit.sequentialProcesses.size();
        cdcSynchronizerCount += unit.cdcSynchronizers.size();
        if (unit.topLevel) ++topLevelCount;
    }
    output << "已分析 " << result.filesAnalyzed << " 个文件，发现 "
           << result.designUnits << " 个设计单元和 " << result.violations.size()
           << " 个问题。\n";
    output << "级别统计：严重 " << violationSummary.critical
           << "，错误 " << violationSummary.error
           << "，警告 " << violationSummary.warning
           << "，提示 " << violationSummary.info << "。\n";
    if (!result.standardId.empty()) {
        std::size_t passed = 0, violated = 0, project = 0, manual = 0;
        for (const auto& assessment : result.standardAssessments) {
            switch (assessment.state) {
            case analysis::StandardRuleState::Passed: ++passed; break;
            case analysis::StandardRuleState::Violated: ++violated; break;
            case analysis::StandardRuleState::ProjectContextRequired: ++project; break;
            case analysis::StandardRuleState::ManualReviewRequired: ++manual; break;
            }
        }
        output << "规则集：" << result.standardId << "；自动检查未检出 " << passed
               << "，违反 " << violated << "，需工程上下文 " << project
               << "，需人工复核 " << manual << "（未检出不代表标准合规）。\n";
    }
    if (!violationSummary.rules.empty()) {
        output << "规则统计：";
        for (const auto& [rule, count] : violationSummary.rules) {
            output << ' ' << rule << '=' << count;
        }
        output << "。\n";
    }
    if (result.suppressedViolations) {
        output << "已豁免 " << result.suppressedViolations << " 个问题。\n";
    }
    if (result.baselinedViolations) {
        output << "基线命中 " << result.baselinedViolations << " 个既有问题。\n";
    }
    for (const auto& override : result.severityOverrides) {
        output << "级别覆盖：" << reportedRuleId(override.ruleId) << '='
               << severityChinese(override.severity) << "。\n";
    }
    output << "设计层次：" << topLevelCount << " 个顶层单元，"
           << instanceCount << " 个实例。\n";
    output << "时序结构：" << clockCount << " 个时钟事件，"
           << resetCount << " 个复位域，" << sequentialProcessCount
           << " 个时序过程，" << cdcSynchronizerCount
           << " 个已认证 CDC 同步器。\n";
}

void writeJson(const analysis::AnalysisResult& result, std::ostream& output) {
    const auto violationSummary = summarizeViolations(result);
    std::size_t instanceCount = 0;
    std::size_t topLevelCount = 0;
    std::size_t clockCount = 0;
    std::size_t resetCount = 0;
    std::size_t sequentialProcessCount = 0;
    std::size_t cdcSynchronizerCount = 0;
    for (const auto& unit : result.structure) {
        instanceCount += unit.instances.size();
        clockCount += unit.clocks.size();
        resetCount += unit.resets.size();
        sequentialProcessCount += unit.sequentialProcesses.size();
        cdcSynchronizerCount += unit.cdcSynchronizers.size();
        if (unit.topLevel) ++topLevelCount;
    }
    const auto countsJson = [&](const analysis::LineCounts& lines) {
        output << "{\"total\":" << lines.total << ",\"code\":" << lines.code
               << ",\"comment\":" << lines.comment << ",\"blank\":" << lines.blank << '}';
    };
    output << "{\"schemaVersion\":5,\"lineStatistics\":{\"total\":";
    countsJson(result.lineStatistics.total);
    output << ",\"languages\":[";
    for (std::size_t i = 0; i < result.lineStatistics.languages.size(); ++i) {
        if (i) output << ',';
        const auto& language = result.lineStatistics.languages[i];
        output << "{\"language\":"; jsonString(output, lineStatisticsLanguageName(language.language));
        output << ",\"files\":" << language.files << ",\"lines\":"; countsJson(language.lines);
        output << '}';
    }
    output << "],\"files\":[";
    for (std::size_t i = 0; i < result.lineStatistics.files.size(); ++i) {
        if (i) output << ',';
        const auto& file = result.lineStatistics.files[i];
        output << "{\"file\":"; jsonString(output, statisticsPath(file.file));
        output << ",\"language\":"; jsonString(output, lineStatisticsLanguageName(file.language));
        output << ",\"lines\":"; countsJson(file.lines); output << '}';
    }
    output << "]},\"summary\":{\"filesAnalyzed\":" << result.filesAnalyzed
           << ",\"designUnits\":" << result.designUnits << ",\"violations\":"
           << result.violations.size() << ",\"topLevelUnits\":" << topLevelCount
           << ",\"instances\":" << instanceCount << ",\"clocks\":" << clockCount
           << ",\"resets\":" << resetCount << ",\"sequentialProcesses\":"
           << sequentialProcessCount << ",\"cdcSynchronizers\":"
           << cdcSynchronizerCount << ",\"suppressedViolations\":"
           << result.suppressedViolations << ",\"baselinedViolations\":"
           << result.baselinedViolations << ",\"severityCounts\":{\"critical\":"
           << violationSummary.critical << ",\"error\":" << violationSummary.error
           << ",\"warning\":" << violationSummary.warning << ",\"info\":"
           << violationSummary.info << "},\"ruleCounts\":[";
    std::size_t summaryRuleIndex = 0;
    for (const auto& [rule, count] : violationSummary.rules) {
        if (summaryRuleIndex++) output << ',';
        output << "{\"ruleId\":"; jsonString(output, rule);
        output << ",\"count\":" << count << '}';
    }
    output << "]},\"configuration\":{\"powerOnPolicy\":{\"technology\":";
    jsonString(output, analysis::fpgaTechnologyName(result.powerOnPolicy.technology));
    output << ",\"acceptInitialBlocks\":" <<
        (!result.powerOnPolicy.acceptInitialBlocks ? "null" :
         *result.powerOnPolicy.acceptInitialBlocks ? "true" : "false");
    output << "},\"severityOverrides\":[";
    for (std::size_t i = 0; i < result.severityOverrides.size(); ++i) {
        if (i) output << ',';
        output << "{\"ruleId\":";
        jsonString(output, reportedRuleId(result.severityOverrides[i].ruleId));
        output << ",\"severity\":";
        jsonString(output, analysis::severityName(result.severityOverrides[i].severity));
        output << '}';
    }
    output << "],\"verilogHeaderPolicy\":{\"origin\":";
    const bool documentedVhdlCommentPolicy =
        !result.verilogHeaderPolicy.configurationKnown && result.standardId == "GJB9765-2020";
    const auto& reportedHeaderPolicy = documentedVhdlCommentPolicy
        ? analysis::Gjb9765DocumentedCommentPolicy : result.verilogHeaderPolicy;
    jsonString(output, documentedVhdlCommentPolicy ? "documented-comment-default" :
        !result.verilogHeaderPolicy.configurationKnown ? "product-template" :
        result.verilogHeaderPolicy.requirements.empty() ? "disabled" : "explicit-project");
    output << ",\"requirements\":[";
    for (std::size_t i = 0; i < reportedHeaderPolicy.requirements.size(); ++i) {
        if (i) output << ',';
        const auto& keyword = reportedHeaderPolicy.requirements[i];
        output << "{\"keyword\":";
        jsonString(output, keyword.keyword);
        output << ",\"caseSensitive\":" << (keyword.caseSensitive ? "true" : "false") << '}';
    }
    output << "]},\"verilogKeywordPolicy\":{\"origin\":";
    const auto& reportedKeywordPolicy = result.verilogKeywordPolicy.configurationKnown
        ? result.verilogKeywordPolicy : analysis::Gjb10157DocumentedKeywordPolicy;
    jsonString(output, result.verilogKeywordPolicy.configurationKnown
        ? "explicit-project" : "documented-standard-default");
    output << ",\"categories\":[";
    for (std::size_t i = 0; i < reportedKeywordPolicy.enabled.size(); ++i) {
        if (i) output << ',';
        output << "{\"category\":";
        jsonString(output, analysis::VerilogKeywordCategoryNames[i]);
        output << ",\"enabled\":" << (reportedKeywordPolicy.enabled[i] ? "true" : "false") << '}';
    }
    output << "]}},\"standard\":{\"id\":";
    jsonString(output, result.standardId);
    output << ",\"assessments\":[";
    for (std::size_t i = 0; i < result.standardAssessments.size(); ++i) {
        if (i) output << ',';
        output << "{\"ruleId\":";
        jsonString(output, reportedRuleId(result.standardAssessments[i].clauseId));
        output << ",\"state\":";
        jsonString(output, analysis::standardRuleStateName(result.standardAssessments[i].state));
        output << '}';
    }
    output << "]},\"design\":{\"units\":[";
    for (std::size_t i = 0; i < result.structure.size(); ++i) {
        if (i) output << ',';
        const auto& unit = result.structure[i];
        output << "{\"name\":"; jsonString(output, unit.name);
        output << ",\"kind\":"; jsonString(output, parser::designUnitKindName(unit.kind));
        output << ",\"file\":"; jsonString(output, unit.location.file.generic_string());
        output << ",\"line\":" << unit.location.line << ",\"column\":" << unit.location.column
               << ",\"topLevel\":" << (unit.topLevel ? "true" : "false") << ",\"ports\":[";
        for (std::size_t j = 0; j < unit.ports.size(); ++j) {
            if (j) output << ',';
            const auto& port = unit.ports[j];
            output << "{\"name\":"; jsonString(output, port.name);
            output << ",\"direction\":"; jsonString(output, parser::portDirectionName(port.direction));
            output << ",\"hasDefault\":" << (port.hasDefault ? "true" : "false")
                   << ",\"file\":";
            jsonString(output, port.location.file.generic_string());
            output << ",\"line\":" << port.location.line << ",\"column\":"
                   << port.location.column << '}';
        }
        output << "],\"parameters\":[";
        for (std::size_t j = 0; j < unit.parameters.size(); ++j) {
            if (j) output << ',';
            const auto& parameter = unit.parameters[j];
            output << "{\"name\":"; jsonString(output, parameter.name);
            output << ",\"hasDefault\":" << (parameter.hasDefault ? "true" : "false")
                   << ",\"file\":";
            jsonString(output, parameter.location.file.generic_string());
            output << ",\"line\":" << parameter.location.line << ",\"column\":"
                   << parameter.location.column << '}';
        }
        output << "],\"clocks\":[";
        for (std::size_t j = 0; j < unit.clocks.size(); ++j) {
            if (j) output << ',';
            const auto& clock = unit.clocks[j];
            output << "{\"signal\":"; jsonString(output, clock.signal);
            output << ",\"edge\":"; jsonString(output, clock.risingEdge ? "rising" : "falling");
            output << ",\"file\":"; jsonString(output, clock.location.file.generic_string());
            output << ",\"line\":" << clock.location.line << ",\"column\":"
                   << clock.location.column << '}';
        }
        output << "],\"resets\":[";
        for (std::size_t j = 0; j < unit.resets.size(); ++j) {
            if (j) output << ',';
            const auto& reset = unit.resets[j];
            output << "{\"signal\":"; jsonString(output, reset.signal);
            output << ",\"activeLevel\":"; jsonString(output, reset.activeHigh ? "high" : "low");
            output << ",\"asynchronous\":" << (reset.asynchronous ? "true" : "false");
            output << ",\"synchronousObserved\":"
                   << (reset.synchronousObserved ? "true" : "false")
                   << ",\"asynchronousObserved\":"
                   << (reset.asynchronousObserved ? "true" : "false")
                   << ",\"activeHighObserved\":"
                   << (reset.activeHighObserved ? "true" : "false")
                   << ",\"activeLowObserved\":"
                   << (reset.activeLowObserved ? "true" : "false");
            output << ",\"file\":"; jsonString(output, reset.location.file.generic_string());
            output << ",\"line\":" << reset.location.line << ",\"column\":"
                   << reset.location.column << '}';
        }
        output << "],\"sequentialProcesses\":[";
        for (std::size_t j = 0; j < unit.sequentialProcesses.size(); ++j) {
            if (j) output << ',';
            const auto& process = unit.sequentialProcesses[j];
            output << "{\"clock\":"; jsonString(output, process.clock);
            output << ",\"edge\":"; jsonString(output, process.risingEdge ? "rising" : "falling");
            output << ",\"file\":"; jsonString(output, process.location.file.generic_string());
            output << ",\"line\":" << process.location.line << ",\"column\":"
                   << process.location.column << ",\"reads\":[";
            for (std::size_t k = 0; k < process.reads.size(); ++k) {
                if (k) output << ',';
                jsonString(output, process.reads[k]);
            }
            output << "],\"writes\":[";
            for (std::size_t k = 0; k < process.writes.size(); ++k) {
                if (k) output << ',';
                jsonString(output, process.writes[k]);
            }
            output << "],\"indexedSignals\":[";
            for (std::size_t k = 0; k < process.indexedSignals.size(); ++k) {
                if (k) output << ',';
                jsonString(output, process.indexedSignals[k]);
            }
            output << "],\"resets\":[";
            for (std::size_t k = 0; k < process.resets.size(); ++k) {
                if (k) output << ',';
                const auto& reset = process.resets[k];
                output << "{\"signal\":"; jsonString(output, reset.signal);
                output << ",\"activeLevel\":";
                jsonString(output, reset.activeHigh ? "high" : "low");
                output << ",\"asynchronous\":"
                       << (reset.asynchronous ? "true" : "false") << ",\"file\":";
                jsonString(output, reset.location.file.generic_string());
                output << ",\"line\":" << reset.location.line << ",\"column\":"
                       << reset.location.column << '}';
            }
            output << "],\"clockEvents\":[";
            for (std::size_t k = 0; k < process.clockEvents.size(); ++k) {
                if (k) output << ',';
                const auto& clock = process.clockEvents[k];
                output << "{\"signal\":"; jsonString(output, clock.signal);
                output << ",\"edge\":"; jsonString(output, clock.risingEdge ? "rising" : "falling");
                output << ",\"file\":"; jsonString(output, clock.location.file.generic_string());
                output << ",\"line\":" << clock.location.line << ",\"column\":"
                       << clock.location.column << '}';
            }
            output << "]}";
        }
        output << "],\"cdcSynchronizers\":[";
        for (std::size_t j = 0; j < unit.cdcSynchronizers.size(); ++j) {
            if (j) output << ',';
            const auto& synchronizer = unit.cdcSynchronizers[j];
            output << "{\"sourceSignal\":"; jsonString(output, synchronizer.sourceSignal);
            output << ",\"firstStage\":"; jsonString(output, synchronizer.firstStage);
            output << ",\"secondStage\":"; jsonString(output, synchronizer.secondStage);
            output << ",\"destinationClock\":";
            jsonString(output, synchronizer.destinationClock);
            output << ",\"edge\":";
            jsonString(output, synchronizer.risingEdge ? "rising" : "falling");
            output << ",\"file\":";
            jsonString(output, synchronizer.location.file.generic_string());
            output << ",\"line\":" << synchronizer.location.line << ",\"column\":"
                   << synchronizer.location.column << '}';
        }
        output << "],\"instances\":[";
        for (std::size_t j = 0; j < unit.instances.size(); ++j) {
            if (j) output << ',';
            const auto& instance = unit.instances[j];
            output << "{\"name\":"; jsonString(output, instance.name);
            output << ",\"target\":"; jsonString(output, instance.target);
            output << ",\"file\":"; jsonString(output, instance.location.file.generic_string());
            output << ",\"line\":" << instance.location.line
                   << ",\"column\":" << instance.location.column
                   << ",\"resolved\":" << (instance.resolved ? "true" : "false")
                   << ",\"external\":" << (instance.external ? "true" : "false")
                   << ",\"namedParameterOverrides\":[";
            for (std::size_t k = 0; k < instance.namedParameterOverrides.size(); ++k) {
                if (k) output << ',';
                const auto& override = instance.namedParameterOverrides[k];
                output << "{\"formal\":"; jsonString(output, override.formal);
                output << ",\"file\":";
                jsonString(output, override.location.file.generic_string());
                output << ",\"line\":" << override.location.line << ",\"column\":"
                       << override.location.column << '}';
            }
            output << "],\"parameterOverrideCount\":" << instance.parameterOverrideCount
                   << ",\"parameterOverrideStyle\":";
            jsonString(output,
                instance.hasNamedParameterOverrides && instance.hasPositionalParameterOverrides ?
                    "mixed" : instance.hasNamedParameterOverrides ? "named" :
                    instance.hasPositionalParameterOverrides ? "positional" : "none");
            output << ",\"namedConnections\":[";
            for (std::size_t k = 0; k < instance.namedConnections.size(); ++k) {
                if (k) output << ',';
                const auto& connection = instance.namedConnections[k];
                output << "{\"formal\":"; jsonString(output, connection.formal);
                output << ",\"actualConnected\":"
                       << (connection.actualConnected ? "true" : "false")
                       << ",\"file\":";
                jsonString(output, connection.location.file.generic_string());
                output << ",\"line\":" << connection.location.line << ",\"column\":"
                       << connection.location.column << '}';
            }
            output << "],\"positionalConnections\":[";
            for (std::size_t k = 0; k < instance.positionalConnections.size(); ++k) {
                if (k) output << ',';
                const auto& connection = instance.positionalConnections[k];
                output << "{\"actualConnected\":"
                       << (connection.actualConnected ? "true" : "false")
                       << ",\"file\":";
                jsonString(output, connection.location.file.generic_string());
                output << ",\"line\":" << connection.location.line << ",\"column\":"
                       << connection.location.column << '}';
            }
            output << "],\"portConnectionCount\":" << instance.portConnectionCount
                   << ",\"hasPortConnectionList\":"
                   << (instance.hasPortConnectionList ? "true" : "false")
                   << ",\"connectionStyle\":";
            jsonString(output,
                instance.hasNamedConnections && instance.hasPositionalConnections ? "mixed" :
                instance.hasNamedConnections ? "named" :
                instance.hasPositionalConnections ? "positional" : "none");
            output << ",\"hasWildcardConnection\":"
                   << (instance.hasWildcardConnection ? "true" : "false")
                   << ",\"invalidPortAssociationOrder\":"
                   << (instance.invalidPortAssociationOrder ? "true" : "false")
                   << ",\"invalidParameterAssociationOrder\":"
                   << (instance.invalidParameterAssociationOrder ? "true" : "false");
            output << '}';
        }
        output << "]}";
    }
    output << "]},\"violations\":[";
    for (std::size_t i = 0; i < result.violations.size(); ++i) {
        if (i) output << ',';
        const auto& violation = result.violations[i];
        output << "{\"ruleId\":"; jsonString(output, reportedRuleId(violation.ruleId));
        output << ",\"severity\":"; jsonString(output, analysis::severityName(violation.severity));
        output << ",\"message\":"; jsonString(output, violation.message);
        output << ",\"description\":"; jsonString(output, violation.description);
        output << ",\"recommendation\":"; jsonString(output, violation.recommendation);
        output << ",\"file\":"; jsonString(output, violation.file.generic_string());
        output << ",\"line\":" << violation.line << ",\"column\":" << violation.column;
        if (violation.module) { output << ",\"designUnit\":"; jsonString(output, *violation.module); }
        if (violation.signal) { output << ",\"signal\":"; jsonString(output, *violation.signal); }
        output << '}';
    }
    output << "],\"errors\":[";
    for (std::size_t i = 0; i < result.errors.size(); ++i) {
        if (i) output << ',';
        jsonString(output, result.errors[i]);
    }
    output << "]}\n";
}

void writeSarif(const analysis::AnalysisResult& result, std::ostream& output) {
    const auto info = application_info();
    const auto rules = ruleMetadata(result);
    const auto violationSummary = summarizeViolations(result);
    std::size_t cdcSynchronizerCount = 0;
    for (const auto& unit : result.structure) {
        cdcSynchronizerCount += unit.cdcSynchronizers.size();
    }
    output << "{\"$schema\":\"https://docs.oasis-open.org/sarif/sarif/v2.1.0/errata01/os/"
              "schemas/sarif-schema-2.1.0.json\",\"version\":\"2.1.0\",\"runs\":[{"
              "\"tool\":{\"driver\":{\"name\":";
    jsonString(output, info.name);
    output << ",\"version\":"; jsonString(output, info.version);
    output << ",\"rules\":[";
    std::size_t ruleIndex = 0;
    for (const auto& [id, metadata] : rules) {
        if (ruleIndex++) output << ',';
        output << "{\"id\":"; jsonString(output, id);
        output << ",\"shortDescription\":{\"text\":"; jsonString(output, metadata.message);
        output << "},\"fullDescription\":{\"text\":"; jsonString(output, metadata.description);
        output << "},\"help\":{\"text\":"; jsonString(output, metadata.recommendation);
        output << "},\"defaultConfiguration\":{\"level\":";
        jsonString(output, sarifLevel(metadata.severity));
        output << "}}";
    }
    output << "]}},\"results\":[";
    for (std::size_t i = 0; i < result.violations.size(); ++i) {
        if (i) output << ',';
        const auto& violation = result.violations[i];
        output << "{\"ruleId\":"; jsonString(output, reportedRuleId(violation.ruleId));
        output << ",\"level\":"; jsonString(output, sarifLevel(violation.severity));
        output << ",\"message\":{\"text\":"; jsonString(output, violation.message);
        output << "},\"locations\":[{\"physicalLocation\":{\"artifactLocation\":{\"uri\":";
        jsonString(output, uriReference(violation.file));
        output << "},\"region\":{\"startLine\":" << violation.line
               << ",\"startColumn\":" << violation.column << "}}}]";
        if (violation.module || violation.signal) {
            output << ",\"properties\":{";
            bool property = false;
            if (violation.module) {
                if (property) output << ',';
                output << "\"designUnit\":";
                jsonString(output, *violation.module);
                property = true;
            }
            if (violation.signal) {
                if (property) output << ',';
                output << "\"signal\":";
                jsonString(output, *violation.signal);
            }
            output << '}';
        }
        output << '}';
    }
    output << "],\"invocations\":[{\"executionSuccessful\":"
           << (result.errors.empty() ? "true" : "false") << "}],\"properties\":{"
           << "\"filesAnalyzed\":" << result.filesAnalyzed
           << ",\"lineStatistics\":{\"total\":" << result.lineStatistics.total.total
           << ",\"code\":" << result.lineStatistics.total.code
           << ",\"comment\":" << result.lineStatistics.total.comment
           << ",\"blank\":" << result.lineStatistics.total.blank << '}'
           << ",\"designUnits\":" << result.designUnits
           << ",\"cdcSynchronizers\":" << cdcSynchronizerCount
           << ",\"suppressedViolations\":" << result.suppressedViolations
           << ",\"baselinedViolations\":" << result.baselinedViolations
           << ",\"severityCounts\":{\"critical\":" << violationSummary.critical
           << ",\"error\":" << violationSummary.error
           << ",\"warning\":" << violationSummary.warning
           << ",\"info\":" << violationSummary.info << "},\"ruleCounts\":[";
    std::size_t sarifRuleCountIndex = 0;
    for (const auto& [rule, count] : violationSummary.rules) {
        if (sarifRuleCountIndex++) output << ',';
        output << "{\"ruleId\":"; jsonString(output, rule);
        output << ",\"count\":" << count << '}';
    }
    output << "],\"severityOverrides\":[";
    for (std::size_t i = 0; i < result.severityOverrides.size(); ++i) {
        if (i) output << ',';
        output << "{\"ruleId\":";
        jsonString(output, reportedRuleId(result.severityOverrides[i].ruleId));
        output << ",\"severity\":";
        jsonString(output, analysis::severityName(result.severityOverrides[i].severity));
        output << '}';
    }
    output << "]}}]}\n";
}

void writeHtml(const analysis::AnalysisResult& result, std::ostream& output) {
    const auto violationSummary = summarizeViolations(result);
    std::size_t instanceCount = 0;
    std::size_t topLevelCount = 0;
    std::size_t clockCount = 0;
    std::size_t resetCount = 0;
    std::size_t sequentialProcessCount = 0;
    std::size_t cdcSynchronizerCount = 0;
    for (const auto& unit : result.structure) {
        instanceCount += unit.instances.size();
        clockCount += unit.clocks.size();
        resetCount += unit.resets.size();
        sequentialProcessCount += unit.sequentialProcesses.size();
        cdcSynchronizerCount += unit.cdcSynchronizers.size();
        if (unit.topLevel) ++topLevelCount;
    }
    output << R"(<!doctype html><html lang="zh-CN"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>FPGA HDL 静态分析报告</title><style>
:root{color-scheme:light dark;--bg:#f5f7fb;--panel:#fff;--text:#172033;--muted:#667085;--line:#d9dfeb;--error:#b42318;--warning:#b54708;--info:#175cd3}
@media(prefers-color-scheme:dark){:root{--bg:#101521;--panel:#171e2d;--text:#ecf0f8;--muted:#a5afc2;--line:#303a4d;--error:#ff8a80;--warning:#ffc46b;--info:#84adff}}
*{box-sizing:border-box}body{margin:0;background:var(--bg);color:var(--text);font:14px/1.5 system-ui,-apple-system,"Segoe UI",sans-serif}.page{max-width:1440px;margin:auto;padding:32px}h1{margin:0 0 4px;font-size:28px}h2{margin:32px 0 12px;font-size:19px}.sub{color:var(--muted)}.cards{display:grid;grid-template-columns:repeat(6,minmax(120px,1fr));gap:12px;margin:24px 0}.card,.panel{background:var(--panel);border:1px solid var(--line);border-radius:10px}.card{padding:15px}.card b{display:block;font-size:24px}.card span{color:var(--muted)}.panel{overflow:auto}table{width:100%;border-collapse:collapse}th,td{padding:10px 12px;text-align:left;vertical-align:top;border-bottom:1px solid var(--line)}th{white-space:nowrap;color:var(--muted);font-size:12px;text-transform:uppercase}.sev{font-weight:700}.critical,.error{color:var(--error)}.warning{color:var(--warning)}.info{color:var(--info)}code{font-family:ui-monospace,SFMono-Regular,Consolas,monospace}.empty{padding:22px;color:var(--muted)}@media(max-width:800px){.page{padding:18px}.cards{grid-template-columns:repeat(2,1fr)}}
</style></head><body><main class="page"><h1>FPGA HDL 静态分析报告</h1><div class="sub">静态分析、编码规则检查与设计层次报告</div><div class="cards">)";
    const auto card = [&](std::string_view label, std::size_t value) {
        output << "<div class=\"card\"><b>" << value << "</b><span>";
        htmlText(output, label);
        output << "</span></div>";
    };
    card("文件", result.filesAnalyzed);
    card("总行数", result.lineStatistics.total.total);
    card("代码行", result.lineStatistics.total.code);
    card("注释行", result.lineStatistics.total.comment);
    card("空白行", result.lineStatistics.total.blank);
    card("设计单元", result.designUnits);
    card("顶层单元", topLevelCount);
    card("实例", instanceCount);
    card("时钟", clockCount);
    card("复位", resetCount);
    card("时序过程", sequentialProcessCount);
    card("CDC 同步器", cdcSynchronizerCount);
    card("问题", result.violations.size());
    card("严重", violationSummary.critical);
    card("错误", violationSummary.error);
    card("警告", violationSummary.warning);
    card("提示", violationSummary.info);
    card("已抑制", result.suppressedViolations);
    card("基线内", result.baselinedViolations);
    output << "</div>";
    output << "<h2>代码行数统计</h2><p class=\"sub\">统计原始源码，包含条件编译分支。"
              "混合代码/注释行计为代码行；总行数 = 代码行 + 注释行 + 空白行。</p>"
              "<div class=\"panel\"><table><thead><tr><th>语言</th><th>文件数</th>"
              "<th>总行数</th><th>代码行</th><th>注释行</th><th>空白行</th></tr></thead><tbody>";
    for (const auto& language : result.lineStatistics.languages) {
        output << "<tr><td>"; htmlText(output, lineStatisticsLanguageName(language.language));
        output << "</td><td>" << language.files << "</td><td>" << language.lines.total
               << "</td><td>" << language.lines.code << "</td><td>" << language.lines.comment
               << "</td><td>" << language.lines.blank << "</td></tr>";
    }
    output << "</tbody></table></div><h2>逐文件行数</h2><div class=\"panel\"><table><thead><tr>"
              "<th>文件</th><th>语言</th><th>总行数</th><th>代码行</th><th>注释行</th><th>空白行</th>"
              "</tr></thead><tbody>";
    for (const auto& file : result.lineStatistics.files) {
        output << "<tr><td><code>"; htmlText(output, statisticsPath(file.file));
        output << "</code></td><td>"; htmlText(output, lineStatisticsLanguageName(file.language));
        output << "</td><td>" << file.lines.total << "</td><td>" << file.lines.code
               << "</td><td>" << file.lines.comment << "</td><td>" << file.lines.blank << "</td></tr>";
    }
    output << "</tbody></table></div>";
    if (!result.severityOverrides.empty()) {
        output << "<h2>严重级覆盖</h2><div class=\"panel\"><table><thead><tr>"
                  "<th>规则</th><th>生效严重级</th></tr></thead><tbody>";
        for (const auto& override : result.severityOverrides) {
            output << "<tr><td><code>"; htmlText(output, reportedRuleId(override.ruleId));
            output << "</code></td><td>"; htmlText(output, severityChinese(override.severity));
            output << "</td></tr>";
        }
        output << "</tbody></table></div>";
    }
    if (!violationSummary.rules.empty()) {
        output << "<h2>规则汇总</h2><div class=\"panel\"><table><thead><tr>"
                  "<th>规则</th><th>数量</th></tr></thead><tbody>";
        for (const auto& [rule, count] : violationSummary.rules) {
            output << "<tr><td><code>"; htmlText(output, rule);
            output << "</code></td><td>" << count << "</td></tr>";
        }
        output << "</tbody></table></div>";
    }
    output << "<h2>问题列表</h2><div class=\"panel\">";
    if (result.violations.empty()) {
        output << "<div class=\"empty\">未发现活动问题。</div>";
    } else {
        output << "<table><thead><tr><th>严重级</th><th>规则</th><th>位置</th>"
                  "<th>问题说明</th><th>修改建议</th></tr></thead><tbody>";
        for (const auto& violation : result.violations) {
            output << "<tr><td><span class=\"sev ";
            htmlText(output, analysis::severityName(violation.severity));
            output << "\">"; htmlText(output, severityChinese(violation.severity));
            output << "</span></td><td><code>"; htmlText(output, reportedRuleId(violation.ruleId));
            output << "</code></td><td><code>"; htmlText(output, violation.file.generic_string());
            output << ':' << violation.line << ':' << violation.column << "</code></td><td>";
            htmlText(output, diagnosticSummary(violation.ruleId));
            output << "</td><td>请根据规则要求检查并修正相关 HDL 代码；如果该结构属于明确的设计意图，"
                      "可在评审后添加局部规则豁免。";
            output << "</td></tr>";
        }
        output << "</tbody></table>";
    }
    output << "</div><h2>设计层次</h2><div class=\"panel\">";
    if (result.structure.empty()) {
        output << "<div class=\"empty\">未找到设计单元。</div>";
    } else {
        output << "<table><thead><tr><th>设计单元</th><th>类型</th><th>顶层</th><th>位置</th>"
                  "<th>参数 / 端口</th><th>时钟 / 复位</th><th>直接实例</th></tr></thead><tbody>";
        for (const auto& unit : result.structure) {
            output << "<tr><td><code>"; htmlText(output, unit.name);
            output << "</code></td><td>"; htmlText(output, designUnitKindChinese(unit.kind));
            output << "</td><td>" << (unit.topLevel ? "是" : "否") << "</td><td><code>";
            htmlText(output, unit.location.file.generic_string());
            output << ':' << unit.location.line << "</code></td><td>";
            if (unit.parameters.empty() && unit.ports.empty()) {
                output << "<span class=\"sub\">无</span>";
            }
            bool interfaceLine = false;
            for (const auto& parameter : unit.parameters) {
                if (interfaceLine) output << "<br>";
                output << "参数 <code>"; htmlText(output, parameter.name);
                output << "</code>" << (parameter.hasDefault ? "（有默认值）" : "");
                interfaceLine = true;
            }
            for (std::size_t i = 0; i < unit.ports.size(); ++i) {
                if (interfaceLine || i) output << "<br>";
                output << "<code>"; htmlText(output, unit.ports[i].name); output << "</code>（";
                htmlText(output, portDirectionChinese(unit.ports[i].direction));
                if (unit.ports[i].hasDefault) output << "，有默认值";
                output << "）";
            }
            output << "</td><td>";
            if (unit.clocks.empty() && unit.resets.empty()) output << "<span class=\"sub\">无</span>";
            bool timingLine = false;
            for (const auto& clock : unit.clocks) {
                if (timingLine) output << "<br>";
                output << "时钟 <code>"; htmlText(output, clock.signal); output << "</code>（"
                       << (clock.risingEdge ? "上升沿" : "下降沿") << "）";
                timingLine = true;
            }
            for (const auto& reset : unit.resets) {
                if (timingLine) output << "<br>";
                output << "复位 <code>"; htmlText(output, reset.signal); output << "</code>（"
                       << (reset.activeHighObserved && reset.activeLowObserved ? "混合极性，" :
                           reset.activeHigh ? "高电平有效，" : "低电平有效，")
                       << (reset.synchronousObserved && reset.asynchronousObserved ? "同步/异步混合" :
                           reset.asynchronous ? "异步" : "同步") << "）";
                timingLine = true;
            }
            for (const auto& process : unit.sequentialProcesses) {
                if (timingLine) output << "<br>";
                output << "时序过程 <code>"; htmlText(output, process.clock); output << "</code>（"
                       << (process.risingEdge ? "上升沿" : "下降沿") << "，读取 "
                       << process.reads.size() << "，写入 " << process.writes.size()
                       << "，复位 " << process.resets.size() << "，时钟事件 "
                       << process.clockEvents.size() << "）";
                timingLine = true;
            }
            for (const auto& synchronizer : unit.cdcSynchronizers) {
                if (timingLine) output << "<br>";
                output << "CDC 同步器 <code>"; htmlText(output, synchronizer.sourceSignal);
                output << "</code> → <code>"; htmlText(output, synchronizer.firstStage);
                output << "</code> → <code>"; htmlText(output, synchronizer.secondStage);
                output << "</code>（<code>"; htmlText(output, synchronizer.destinationClock);
                output << "</code> " << (synchronizer.risingEdge ? "上升沿" : "下降沿")
                       << "）";
                timingLine = true;
            }
            output << "</td><td>";
            if (unit.instances.empty()) output << "<span class=\"sub\">无</span>";
            for (std::size_t i = 0; i < unit.instances.size(); ++i) {
                if (i) output << "<br>";
                const auto& instance = unit.instances[i];
                output << "<code>"; htmlText(output, instance.name); output << "</code> → <code>";
                htmlText(output, instance.target); output << "</code> <span class=\"sub\">（";
                output << (instance.external ? "外部" : instance.resolved ? "已解析" : "未解析")
                       << "，参数覆盖 " << instance.parameterOverrideCount << "，"
                       << (instance.hasNamedParameterOverrides && instance.hasPositionalParameterOverrides ?
                           "混合关联" : instance.hasNamedParameterOverrides ? "命名关联" :
                           instance.hasPositionalParameterOverrides ? "位置关联" : "无关联")
                       << (instance.invalidParameterAssociationOrder ? "，参数顺序无效" : "")
                       << "，端口连接 " << instance.portConnectionCount << "，"
                       << (instance.hasNamedConnections && instance.hasPositionalConnections ? "混合关联" :
                           instance.hasNamedConnections ? "命名关联" :
                           instance.hasPositionalConnections ? "位置关联" : "无关联")
                       << (instance.hasWildcardConnection ? "，通配连接" : "")
                       << (instance.invalidPortAssociationOrder ? "，端口顺序无效" : "")
                       << "）</span>";
            }
            output << "</td></tr>";
        }
        output << "</tbody></table>";
    }
    output << "</div>";
    if (!result.errors.empty()) {
        output << "<h2>分析错误</h2><div class=\"panel\"><ul>";
        for (const auto& error : result.errors) { output << "<li>"; htmlText(output, error); output << "</li>"; }
        output << "</ul></div>";
    }
    output << "</main></body></html>\n";
}

} // namespace fpga::report
