#include "project/config_file.h"
#include "project/file_list.h"

#include <filesystem>
#include <algorithm>
#include <iostream>

int main() {
    int failures = 0;
    const auto check = [&](bool condition, const char* message) {
        if (!condition) { std::cerr << "FAIL: " << message << '\n'; ++failures; }
    };
    const auto fixture = std::filesystem::path(FPGA_TEST_SOURCE_DIR) / "fixtures";
    const auto valid = fpga::project::loadConfigFile(fixture / "project.conf");
    check(valid.errors.empty(), "valid config has no errors");
    check(!valid.config.analysis.verilogHeaderPolicy.configurationKnown,
          "omitted header policy never fabricates native defaults");
    const auto header = fpga::project::loadConfigFile(fixture / "header_policy_explicit.conf");
    check(header.errors.empty() && header.config.analysis.verilogHeaderPolicy.configurationKnown &&
          header.config.analysis.verilogHeaderPolicy.requirements.size() == 2 &&
          header.config.analysis.verilogHeaderPolicy.requirements[0].keyword == "Author" &&
          !header.config.analysis.verilogHeaderPolicy.requirements[0].caseSensitive &&
          header.config.analysis.verilogHeaderPolicy.requirements[1].keyword == "Revision History" &&
          header.config.analysis.verilogHeaderPolicy.requirements[1].caseSensitive,
          "explicit quoted header map loads with independent flags");
    const auto disabledHeader = fpga::project::loadConfigFile(fixture / "header_policy_disabled.conf");
    check(disabledHeader.errors.empty() && disabledHeader.config.analysis.verilogHeaderPolicy.configurationKnown &&
          disabledHeader.config.analysis.verilogHeaderPolicy.requirements.empty(), "disabled policy clears prior keys");
    const auto templateHeader = fpga::project::loadConfigFile(fixture / "header_policy_template.conf");
    check(templateHeader.errors.empty() && !templateHeader.config.analysis.verilogHeaderPolicy.configurationKnown &&
          templateHeader.config.analysis.verilogHeaderPolicy.requirements.empty(), "template resets explicit policy");
    const auto badHeader = fpga::project::loadConfigFile(fixture / "header_policy_invalid.conf");
    check(badHeader.errors.size() == 8 && badHeader.config.analysis.verilogHeaderPolicy.requirements.size() == 1,
          "invalid policy/mode/missing/extra/non-ASCII/duplicate/unclosed values rejected without corrupting map");
    const auto emptyHeader = fpga::project::loadConfigFile(fixture / "header_policy_empty_key.conf");
    check(emptyHeader.errors.empty() && emptyHeader.config.analysis.verilogHeaderPolicy.requirements.size() == 1 &&
          emptyHeader.config.analysis.verilogHeaderPolicy.requirements[0].keyword.empty(), "explicit empty string key preserved");
    const auto keywords = fpga::project::loadConfigFile(fixture / "keyword_policy_explicit.conf");
    check(keywords.errors.empty() && keywords.config.analysis.verilogKeywordPolicy.configurationKnown &&
          std::ranges::all_of(keywords.config.analysis.verilogKeywordPolicy.enabled, [](bool enabled) { return enabled; }),
          "all six keyword categories explicitly configured");
    const auto disabledKeywords = fpga::project::loadConfigFile(fixture / "keyword_policy_disabled.conf");
    check(disabledKeywords.errors.empty() && disabledKeywords.config.analysis.verilogKeywordPolicy.configurationKnown &&
          std::ranges::none_of(disabledKeywords.config.analysis.verilogKeywordPolicy.enabled, [](bool enabled) { return enabled; }),
          "explicit all-off clears earlier category flags");
    const auto unknownKeywords = fpga::project::loadConfigFile(fixture / "keyword_policy_unspecified.conf");
    check(unknownKeywords.errors.empty() && !unknownKeywords.config.analysis.verilogKeywordPolicy.configurationKnown,
          "unspecified restores unknown category policy");
    const auto badKeywords = fpga::project::loadConfigFile(fixture / "keyword_policy_invalid.conf");
    check(badKeywords.errors.size() == 6 && badKeywords.config.analysis.verilogKeywordPolicy.configurationKnown &&
          std::ranges::none_of(badKeywords.config.analysis.verilogKeywordPolicy.enabled, [](bool enabled) { return enabled; }),
          "invalid policy/key/case/flag/extra/missing inputs rejected without enabling categories");
    using Priority = fpga::analysis::FsmOptimizationPriority;
    const auto power = fpga::project::loadConfigFile(fixture / "power_on_non_sram.conf");
    check(power.errors.empty() &&
          power.config.analysis.powerOnPolicy.technology == fpga::analysis::FpgaTechnology::NonSram &&
          power.config.analysis.powerOnPolicy.acceptInitialBlocks == true,
          "explicit target technology and initial acceptance policy load");
    const auto badPower = fpga::project::loadConfigFile(fixture / "power_on_invalid.conf");
    check(badPower.errors.size() == 4 &&
          badPower.config.analysis.powerOnPolicy.technology == fpga::analysis::FpgaTechnology::Unspecified &&
          !badPower.config.analysis.powerOnPolicy.acceptInitialBlocks,
          "invalid power-on policies do not fabricate known defaults");
    check(valid.config.analysis.fsmOptimizationPriority == Priority::Unspecified,
          "FSM priority omitted defaults to unspecified");
    for (const auto& [name, priority] : std::vector<std::pair<const char*, Priority>>{
            {"fsm_priority_area.conf", Priority::Area}, {"fsm_priority_speed.conf", Priority::Speed},
            {"fsm_priority_unspecified.conf", Priority::Unspecified}}) {
        const auto loaded = fpga::project::loadConfigFile(fixture / name);
        check(loaded.errors.empty() && loaded.config.analysis.fsmOptimizationPriority == priority,
              "valid FSM priority and explicit unspecified override loaded");
    }
    const auto badPriority = fpga::project::loadConfigFile(fixture / "fsm_priority_invalid.conf");
    check(badPriority.errors.size() == 4 &&
              badPriority.config.analysis.fsmOptimizationPriority == Priority::Unspecified,
          "unknown/case mismatch/extra/missing FSM priorities rejected without changing default");
    for (std::size_t i = 0; i < badPriority.errors.size(); ++i)
        check(badPriority.errors[i].find(":" + std::to_string(i + 1) + ":") != std::string::npos,
              "FSM config invalid value retains line location");
    const auto topModule = fpga::project::loadConfigFile(fixture / "top_module.conf");
    check(topModule.errors.empty() && topModule.config.analysis.topModule == "SelectedTop",
          "explicit top module is loaded without case rewriting");
    const auto badTopModule = fpga::project::loadConfigFile(fixture / "top_module_invalid.conf");
    check(badTopModule.errors.size() == 2 && !badTopModule.config.analysis.topModule,
          "missing and multi-token top module values are rejected");
    check(valid.config.analysis.disabledRules == std::vector<std::string>{"STYLE-001"},
          "disabled rule is loaded");
    check(valid.config.analysis.blackBoxes == std::vector<std::string>{"vendor_fifo"},
          "black box is loaded");
    check(valid.config.analysis.excludePatterns == std::vector<std::string>{"**/generated/**"},
          "exclude pattern is loaded");
    check(valid.config.analysis.includeDirectories ==
              std::vector<std::filesystem::path>{fixture / "includes"},
          "relative include directory is resolved from the config location");
    check(valid.config.analysis.defines == std::vector<std::string>{"FEATURE_ENABLED=1"},
          "preprocessor definition is loaded");
    check(valid.config.failOn == "error", "failure threshold is loaded");
    check(valid.config.analysis.severityOverrides.size() == 1 &&
              valid.config.analysis.severityOverrides[0].ruleId == "STYLE-002" &&
              valid.config.analysis.severityOverrides[0].severity == fpga::analysis::Severity::Warning,
          "per-rule severity override is loaded");
    const auto baseline = fpga::project::loadConfigFile(fixture / "baseline.conf");
    check(baseline.errors.empty() &&
              baseline.config.baseline == fixture / "existing.fpga-lint-baseline",
          "relative baseline path is resolved from the config location");
    const auto fileListConfig = fpga::project::loadConfigFile(fixture / "file_list.conf");
    check(fileListConfig.errors.empty() &&
              fileListConfig.config.fileLists ==
                  std::vector<std::filesystem::path>{fixture / "filelists/root.f"},
          "relative file-list path is resolved from the config location");

    const auto fileList = fpga::project::loadFileList(fixture / "filelists/root.f");
    check(fileList.errors.empty(), "valid nested HDL file list has no errors");
    check(fileList.inputs == std::vector<std::filesystem::path>{
              (fixture / "filelists/nested/../rtl/child.vhd").lexically_normal(),
              fixture / "filelists/rtl/top.sv"},
          "nested and root HDL paths are expanded in deterministic order");
    check(fileList.includeDirectories ==
              std::vector<std::filesystem::path>{fixture / "includes",
                  fixture / "filelists/unused_include"},
          "file-list include directories resolve from the containing list");
    check(fileList.defines == std::vector<std::string>{
              "FILELIST_FEATURE=1", "BUS_WIDTH=8", "NESTED_FEATURE=1"},
          "file-list plus defines are expanded in order");
    const auto cyclic = fpga::project::loadFileList(fixture / "filelists/cycle-a.f");
    check(cyclic.errors.size() == 1 &&
              cyclic.errors[0].find("Recursive file-list inclusion") != std::string::npos,
          "recursive nested file lists are rejected");
    const auto invalidFileList = fpga::project::loadFileList(fixture / "filelists/invalid.f");
    check(invalidFileList.errors.size() == 2 &&
              invalidFileList.errors[0].find(":1:") != std::string::npos &&
              invalidFileList.errors[1].find(":2:") != std::string::npos,
          "unsupported options and malformed quoting retain file-list line locations");

    const auto invalid = fpga::project::loadConfigFile(fixture / "invalid.conf");
    check(invalid.errors.size() == 7, "all invalid config lines are diagnosed");
    check(invalid.errors[0].find(":1:") != std::string::npos,
          "config error includes a source line");
    check(invalid.errors[3].find("Unknown rule ID") != std::string::npos,
          "unknown disabled rule is rejected");
    check(invalid.errors[4].find("Invalid severity level") != std::string::npos &&
              invalid.errors[5].find("Unknown rule ID") != std::string::npos &&
              invalid.errors[6].find("Expected: severity") != std::string::npos,
          "invalid severity directives are diagnosed precisely");

    const auto missing = fpga::project::loadConfigFile(fixture / "missing.conf");
    check(missing.errors.size() == 1 && missing.errors[0].find("Cannot read") != std::string::npos,
          "missing config is diagnosed");
    return failures == 0 ? 0 : 1;
}
