#include "project/config_file.h"
#include "analysis/rule_catalog.h"
#include "analysis/standard_profiles.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iomanip>
#include <sstream>

namespace fpga::project {
namespace {
std::string trim(std::string text) {
    const auto notSpace = [](unsigned char character) { return !std::isspace(character); };
    const auto begin = std::ranges::find_if(text, notSpace);
    if (begin == text.end()) return {};
    const auto end = std::find_if(text.rbegin(), text.rend(), notSpace).base();
    return {begin, end};
}

bool validFailOn(const std::string& value) {
    return value == "info" || value == "warning" || value == "error" ||
           value == "critical" || value == "none";
}
} // namespace

ConfigLoadResult loadConfigFile(const std::filesystem::path& path) {
    ConfigLoadResult result;
    std::ifstream stream(path);
    if (!stream) {
        result.errors.push_back("Cannot read config file: " + path.string());
        return result;
    }
    std::string line;
    std::size_t lineNumber = 0;
    while (std::getline(stream, line)) {
        ++lineNumber;
        const auto comment = line.find('#');
        if (comment != std::string::npos) line.erase(comment);
        line = trim(std::move(line));
        if (line.empty()) continue;
        std::istringstream parser(line);
        std::string directive;
        parser >> directive;
        std::string value;
        std::getline(parser, value);
        value = trim(std::move(value));
        const auto error = [&](std::string message) {
            result.errors.push_back(path.string() + ':' + std::to_string(lineNumber) +
                                    ": " + std::move(message));
        };
        if (value.empty()) {
            error("Missing value for directive " + directive);
        } else if (directive == "standard") {
            const auto profile = analysis::findStandardProfile(value);
            if (!profile) error("Unknown standard ID: " + value + "; expected gjb10157 or gjb9765");
            else result.config.analysis.standardId = std::string(profile->id);
        } else if (directive == "fpga-technology") {
            const auto technology = analysis::parseFpgaTechnology(value);
            if (!technology) error("器件类型须为 sram|non-sram|unspecified");
            else result.config.analysis.powerOnPolicy.technology = *technology;
        } else if (directive == "initial-blocks") {
            if (value == "accept") result.config.analysis.powerOnPolicy.acceptInitialBlocks = true;
            else if (value == "ignore") result.config.analysis.powerOnPolicy.acceptInitialBlocks = false;
            else if (value == "unspecified") result.config.analysis.powerOnPolicy.acceptInitialBlocks.reset();
            else error("上电初始化 initial 策略须为 accept|ignore|unspecified");
        } else if (directive == "fsm-priority") {
            const auto priority = analysis::parseFsmOptimizationPriority(value);
            if (!priority) error("Invalid FSM priority: " + value + "; expected area|speed|unspecified");
            else result.config.analysis.fsmOptimizationPriority = *priority;
        } else if (directive == "top-module") {
            std::istringstream setting(value);
            std::string name, extra;
            setting >> name >> extra;
            if (name.empty() || !extra.empty()) error("格式：top-module NAME");
            else result.config.analysis.topModule = std::move(name);
        } else if (directive == "keyword-policy") {
            if (value == "unspecified") result.config.analysis.verilogKeywordPolicy = {};
            else if (value == "explicit" || value == "disabled")
                result.config.analysis.verilogKeywordPolicy = {true, {}};
            else error("关键字策略须为 explicit|disabled|unspecified");
        } else if (directive == "keyword-category") {
            std::istringstream setting(value);
            std::string name, flag, extra;
            setting >> name >> flag >> extra;
            const auto category = analysis::parseVerilogKeywordCategory(name);
            auto& policy = result.config.analysis.verilogKeywordPolicy;
            if (!category || (flag != "on" && flag != "off") || !extra.empty())
                error("格式：keyword-category vhdl|verilog|systemverilog|sdf-edif|psl|windows on|off");
            else if (!policy.configurationKnown)
                error("请先设置 keyword-policy explicit，再配置关键字类别");
            else policy.enabled[static_cast<std::size_t>(*category)] = flag == "on";
        } else if (directive == "header-policy") {
            if (value == "template") result.config.analysis.verilogHeaderPolicy = {};
            else if (value == "explicit" || value == "disabled")
                result.config.analysis.verilogHeaderPolicy = {true, {}};
            else error("文件头策略须为 template|explicit|disabled");
        } else if (directive == "header-keyword") {
            std::istringstream setting(value);
            std::string mode, keyword, extra;
            setting >> mode >> std::quoted(keyword);
            const bool validInput = !setting.fail();
            setting >> extra;
            auto& policy = result.config.analysis.verilogHeaderPolicy;
            if (!validInput || !extra.empty() ||
                (mode != "sensitive" && mode != "insensitive")) {
                error("格式：header-keyword sensitive|insensitive \"关键字\"");
            } else if (!policy.configurationKnown) {
                error("请先设置 header-policy explicit，再添加文件头关键字");
            } else if (!std::ranges::all_of(keyword, [](unsigned char c) { return c > 0 && c < 128; })) {
                error("当前已恢复的文件头匹配仅支持 ASCII 关键字");
            } else if (std::ranges::any_of(policy.requirements, [&](const auto& item) {
                           return item.keyword == keyword;
                       })) {
                error("文件头关键字重复：" + keyword);
            } else policy.requirements.push_back({keyword, mode == "sensitive"});
        } else if (directive == "disable") {
            if (value != "all" && !analysis::findRule(value) &&
                !analysis::findStandardRuleDetail(value)) error("Unknown rule ID: " + value);
            else result.config.analysis.disabledRules.push_back(value);
        } else if (directive == "severity") {
            std::istringstream setting(value);
            std::string rule;
            std::string level;
            std::string extra;
            setting >> rule >> level >> extra;
            if (rule.empty() || level.empty() || !extra.empty()) {
                error("Expected: severity RULE info|warning|error|critical");
            } else if (!analysis::findRule(rule) && !analysis::findStandardRuleDetail(rule)) {
                error("Unknown rule ID: " + rule);
            } else if (const auto severity = analysis::parseSeverity(level)) {
                result.config.analysis.severityOverrides.push_back({rule, *severity});
            } else {
                error("Invalid severity level: " + level);
            }
        } else if (directive == "blackbox") {
            result.config.analysis.blackBoxes.push_back(value);
        } else if (directive == "exclude") {
            result.config.analysis.excludePatterns.push_back(value);
        } else if (directive == "include-dir") {
            auto directory = std::filesystem::path(value);
            if (directory.is_relative()) directory = path.parent_path() / directory;
            result.config.analysis.includeDirectories.push_back(directory.lexically_normal());
        } else if (directive == "define") {
            result.config.analysis.defines.push_back(value);
        } else if (directive == "fail-on") {
            if (!validFailOn(value)) error("Invalid fail-on level: " + value);
            else result.config.failOn = value;
        } else if (directive == "baseline") {
            auto baseline = std::filesystem::path(value);
            if (baseline.is_relative()) baseline = path.parent_path() / baseline;
            result.config.baseline = baseline.lexically_normal();
        } else if (directive == "file-list") {
            auto fileList = std::filesystem::path(value);
            if (fileList.is_relative()) fileList = path.parent_path() / fileList;
            result.config.fileLists.push_back(fileList.lexically_normal());
        } else {
            error("Unknown directive: " + directive);
        }
    }
    return result;
}

} // namespace fpga::project
