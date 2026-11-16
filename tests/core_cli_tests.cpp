#include "core/application_info.h"
#include "cli/command_line.h"
#include <array>
#include <filesystem>
#include <iostream>
#include <sstream>

// Tiny dependency-free test harness: checks remain enabled in Release builds.
int main() {
    int failures = 0;
    const auto check = [&failures](bool condition, const char* message) {
        if (!condition) { std::cerr << "FAIL: " << message << '\n'; ++failures; }
    };
    check(!fpga::application_info().name.empty(), "application name exists");
    const auto runCase = [&check](std::span<const std::string_view> args, int expected,
                                  std::string_view expectedText) {
        std::ostringstream out, err;
        check(fpga::cli::run(args, out, err) == expected, "exit code");
        const auto text = expected == 0 ? out.str() : err.str();
        check(text.find(expectedText) != std::string::npos, "expected message");
        check((expected == 0 ? err.str() : out.str()).empty(), "correct output stream");
    };
    runCase({}, 0, "支持扩展名");
    for (auto arg : {"--help", "-h"}) {
        const std::array<std::string_view, 1> args{arg};
        runCase(args, 0, "--top-module NAME");
    }
    const std::array<std::string_view, 1> version{"--version"};
    runCase(version, 0, fpga::application_info().version);
    const std::array<std::string_view, 1> listRules{"--list-rules"};
    runCase(listRules, 0, "RTL-LATCH-INFERRED");
    const std::array<std::string_view, 1> listStandards{"--list-standards"};
    runCase(listStandards, 0, "GJB10157-2021");
    const std::array<std::string_view, 2> listStandardRules{
        "--list-standard-rules", "gjb9765"};
    runCase(listStandardRules, 0, "R-11-1\t");
    const std::array<std::string_view, 1> missingStandardRules{"--list-standard-rules"};
    runCase(missingStandardRules, 2, "缺少标准 ID");
    const std::array<std::string_view, 2> unknownStandardRules{
        "--list-standard-rules", "GJB0000"};
    runCase(unknownStandardRules, 2, "未知标准 ID");
    const std::array<std::string_view, 2> describeRule{"--describe-rule", "RTL-LATCH-INFERRED"};
    runCase(describeRule, 0, "组合过程部分控制路径未赋值");
    const std::array<std::string_view, 2> describeStandardRule{
        "--describe-rule", "GJB9765:A-8-1"};
    runCase(describeStandardRule, 0, "A-8-1\n判定方式：");
    const std::array<std::string_view, 2> ambiguousStandardRule{
        "--describe-rule", "R-2-1"};
    runCase(ambiguousStandardRule, 2, "跨标准歧义");
    const std::array<std::string_view, 2> unknownRule{"--describe-rule", "NOT-A-RULE"};
    runCase(unknownRule, 2, "未知规则 ID");
    const std::array<std::string_view, 1> missingRule{"--describe-rule"};
    runCase(missingRule, 2, "--describe-rule 缺少规则 ID");
    for (auto arg : {"--analyze", "--unknown"}) {
        const std::array<std::string_view, 1> args{arg};
        runCase(args, 2, "未知选项");
    }
    const std::array<std::string_view, 2> extra{"--help", "source.v"};
    runCase(extra, 2, "未知选项");
    const std::array<std::string_view, 2> badFormat{"--format", "xml"};
    runCase(badFormat, 2, "--format 值无效");
    const std::array<std::string_view, 2> badThreshold{"--fail-on", "hint"};
    runCase(badThreshold, 2, "--fail-on 值无效");
    const std::array<std::string_view, 1> missingFsmPriority{"--fsm-priority"};
    runCase(missingFsmPriority, 2, "--fsm-priority 缺少参数值");
    const std::array<std::string_view, 1> missingTopModule{"--top-module"};
    runCase(missingTopModule, 2, "--top-module 缺少顶层设计名称");
    const std::array<std::string_view, 2> optionAsTopModule{"--top-module", "--line-count"};
    runCase(optionAsTopModule, 2, "--top-module 缺少顶层设计名称");
    for (auto option : {"--fpga-technology", "--initial-blocks"}) {
        const std::array<std::string_view, 1> missing{option};
        runCase(missing, 2, "缺少参数值");
        const std::array<std::string_view, 2> invalid{option, "unknown"};
        runCase(invalid, 2, "须为");
    }
    for (const auto value : {"balanced", "Area", "area speed", "--line-count"}) {
        const std::array<std::string_view, 2> badPriority{"--fsm-priority", value};
        runCase(badPriority, 2, "--fsm-priority 值无效");
    }
    const auto fixture = std::filesystem::path(__FILE__).parent_path() / "fixtures";
    const auto priorityInput = (fixture / "risky.vhd").string();
    const auto areaConfig = (fixture / "fsm_priority_area.conf").string();
    for (const auto value : {"area", "speed", "unspecified"}) {
        const std::array<std::string_view, 8> validPriority{
            "--config", areaConfig, "--fsm-priority", value,
            "--line-count", "--format", "json", priorityInput};
        runCase(validPriority, 0, "lineStatistics");
    }
    const std::array<std::string_view, 3> badDisable{"--disable", "NOT-A-RULE", "rtl"};
    runCase(badDisable, 2, "未知规则 ID");
    const std::array<std::string_view, 1> missingSeverity{"--severity"};
    runCase(missingSeverity, 2, "--severity 缺少参数值");
    const std::array<std::string_view, 3> malformedSeverity{"--severity", "STYLE-001", "rtl"};
    runCase(malformedSeverity, 2, "--severity 值无效");
    const std::array<std::string_view, 3> unknownSeverity{"--severity", "NO-RULE=error", "rtl"};
    runCase(unknownSeverity, 2, "未知规则 ID");
    const std::array<std::string_view, 3> badSeverityLevel{"--severity", "STYLE-001=fatal", "rtl"};
    runCase(badSeverityLevel, 2, "--severity 严重级无效");
    const std::array<std::string_view, 1> missingOutput{"--output"};
    runCase(missingOutput, 2, "--output 缺少参数值");
    const std::array<std::string_view, 1> missingDefine{"-D"};
    runCase(missingDefine, 2, "-D 缺少参数值");
    const std::array<std::string_view, 1> missingInclude{"-I"};
    runCase(missingInclude, 2, "-I 缺少参数值");
    const std::array<std::string_view, 1> missingFileList{"-f"};
    runCase(missingFileList, 2, "-f 缺少参数值");
    const std::array<std::string_view, 1> missingBaseline{"--baseline"};
    runCase(missingBaseline, 2, "--baseline 缺少参数值");
    const std::array<std::string_view, 1> missingWriteBaseline{"--write-baseline"};
    runCase(missingWriteBaseline, 2, "--write-baseline 缺少参数值");
    const std::array<std::string_view, 5> conflictingBaseline{
        "--baseline", "old.baseline", "--write-baseline", "new.baseline", "rtl"};
    runCase(conflictingBaseline, 2, "不能同时使用");
    return failures == 0 ? 0 : 1;
}
