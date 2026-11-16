#pragma once

#include <algorithm>
#include <optional>
#include <string_view>

namespace fpga::analysis {

// The caller must supply an established declaration category, not arbitrary
// identifier occurrences. Native run 0x1805306e0 selects only AST_WIRE(6),
// AST_PARAMETER(9), AST_LOCALPARAM(10); its collector stops at task/function
// subtrees. No lexical category guessing or full language coverage is implied.
enum class VerilogNamingSubject { Parameter, Localparam, WireLike, Unsupported };

struct VerilogNamingPolicy {
    bool checkParameterUppercase{true};   // native byte +0x159
    bool checkWireNames{true};            // native byte +0x15a
    bool requireWireAllLowercase{false};  // native byte +0x158
};

// Pure predicate recovered from
// ev_ec2de6887151950345c09c26e8681066402a4d6fe1589de3f8fd54b1bd393a74.
// Input is a normalized source identifier WITHOUT a Yosys internal prefix or
// escaped-identifier delimiter. Unknown category -> nullopt, true -> violation.
// This checks ASCII case only, not identifier lexical validity. In particular,
// the attachment's "parameter Data // follow" contradicts the native default;
// this helper faithfully treats it as a parameter naming violation.
[[nodiscard]] inline std::optional<bool>
checkVerilogNamingStyle(std::string_view name, VerilogNamingSubject subject,
                        VerilogNamingPolicy policy = {}) {
    if (subject == VerilogNamingSubject::Unsupported) return std::nullopt;
    if (name.empty()) return false; // the native run skips empty normalized names
    const auto upper = [](char value) { return value >= 'A' && value <= 'Z'; };
    const auto lower = [](char value) { return value >= 'a' && value <= 'z'; };
    if (subject == VerilogNamingSubject::Parameter || subject == VerilogNamingSubject::Localparam)
        return policy.checkParameterUppercase && std::any_of(name.begin(), name.end(), lower);
    if (!policy.checkWireNames) return false;
    if (policy.requireWireAllLowercase) return std::any_of(name.begin(), name.end(), upper);
    return (!upper(name.front()) && !lower(name.front())) ||
        std::any_of(name.begin() + 1, name.end(), upper);
}

} // namespace fpga::analysis
