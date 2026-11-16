#pragma once

#include <string_view>

namespace fpga::analysis {

// 面向最终用户的简体中文规则摘要。规则 ID 仍作为稳定机器标识。
std::string_view chineseRuleSummary(std::string_view ruleId);

} // namespace fpga::analysis
