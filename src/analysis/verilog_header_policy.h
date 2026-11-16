#pragma once

#include "core/source.h"

#include <algorithm>
#include <cstddef>
#include <ranges>
#include <string>
#include <string_view>
#include <vector>

namespace fpga::analysis {

struct VerilogHeaderKeyword {
    std::string keyword;
    bool caseSensitive{false};
};

struct VerilogHeaderPolicy {
    // Default keyword map was NOT recovered. Never silently substitute an
    // author/filename/history template for the native configurable map.
    bool configurationKnown{false};
    std::vector<VerilogHeaderKeyword> requirements;
};
// GJB9765.4.13.a explicitly requires an English comment at the file start.
// Requiring the VHDL comment introducer proves the no-header violation without
// inventing the vendor's configurable filename/author/history keyword map.
inline const VerilogHeaderPolicy Gjb9765DocumentedCommentPolicy{
    true, {{"--", true}}};

enum class VerilogHeaderPolicyStatus {
    UnknownConfiguration, Disabled, Complete, Incomplete, Unsupported
};

struct VerilogHeaderPolicyResult {
    VerilogHeaderPolicyStatus status{VerilogHeaderPolicyStatus::UnknownConfiguration};
    bool missingHeader{false};
    std::vector<std::string> missingKeywords;
};

// Configurable ASCII-only subset shared by the recovered Verilog and VHDL
// file-header predicates.  The historical type name is retained so existing
// project/API configuration remains source compatible.
// Verilog native run 0x1805258c0:
// ev_351f2d01a7e2d8efb05ecb58c1a4ff21ebc46f16acca2df1b165c89b9c8bc30c
// substring implementation:
// ev_4ead1cd3df3fd24c1039b4d47a24d251880a61f562e6ccc0853e8c1dab4d9a85
// UTF8 -> Windows ACP line conversion (NOT trim) limits the ASCII subset:
// ev_630d95ea5bfb21f6bf8afbde4450ed38ec1652fdb24df59cb1258a1475b7e48e
// Complete concerns only this explicitly supplied policy predicate, NOT whole
// GJB10157.5.1.1 coverage or an established vendor default configuration.
// Assembly 0x180525fb7 loads R9 from keyword string size at [RBP+0x150]
// before CALL 0x1801a2d00 at 0x180525fc8: whole keyword length, offset zero.
// ev_af5d79ef77ddca2d17f43bdd4311ebf0cdb99613f35752cb69d82c291e2d7b2f
// Caller must supply a confirmed one-based module declaration line. Native
// checks physical nonempty lines preceding that module, not only comments.
namespace header_policy_detail {

[[nodiscard]] inline VerilogHeaderPolicyResult
evaluate(const core::SourceFile& source, std::size_t declarationLine,
         const VerilogHeaderPolicy& policy, core::HdlLanguage expectedLanguage) {
    VerilogHeaderPolicyResult result;
    if (!policy.configurationKnown) return result;
    if (policy.requirements.empty()) {
        result.status = VerilogHeaderPolicyStatus::Disabled;
        return result; // native +0x150 == 0 gate
    }
    const bool languageMatches = expectedLanguage == core::HdlLanguage::Vhdl
        ? source.language() == core::HdlLanguage::Vhdl
        : (source.language() == core::HdlLanguage::Verilog ||
           source.language() == core::HdlLanguage::SystemVerilog);
    if (!languageMatches) {
        result.status = VerilogHeaderPolicyStatus::Unsupported;
        return result;
    }
    result.status = VerilogHeaderPolicyStatus::Incomplete;
    if (declarationLine == 0 || source.content().empty()) return result;
    const auto ascii = [](std::string_view text) {
        return std::all_of(text.begin(), text.end(), [](unsigned char c) { return c != 0 && c < 128; });
    };
    const auto lower = [](std::string text) {
        for (auto& c : text) if (c >= 'A' && c <= 'Z') c = static_cast<char>(c - 'A' + 'a');
        return text;
    };
    std::vector<std::string> keys;
    for (const auto& item : policy.requirements) {
        if (!ascii(item.keyword) || std::find(keys.begin(), keys.end(), item.keyword) != keys.end())
            return result; // invalid/ambiguous map or non-ASCII environment dependency
        keys.push_back(item.keyword);
    }
    std::vector<std::string> lines;
    std::size_t offset = 0;
    for (std::size_t line = 1; line < declarationLine; ++line) {
        const auto newline = source.content().find('\n', offset);
        if (newline == std::string::npos) return result;
        auto text = source.content().substr(offset, newline - offset);
        if (!ascii(text)) return result;
        if (!text.empty()) lines.push_back(std::move(text));
        offset = newline + 1;
    }
    if (offset >= source.content().size()) return result;
    result.status = VerilogHeaderPolicyStatus::Complete;
    if (lines.empty()) {
        result.missingHeader = true;
        return result; // native emits header-missing instead of per-keyword data
    }
    for (const auto& item : policy.requirements) {
        const auto key = item.caseSensitive ? item.keyword : lower(item.keyword);
        const bool found = std::any_of(lines.begin(), lines.end(), [&](const auto& line) {
            return (item.caseSensitive ? line : lower(line)).find(key) != std::string::npos;
        });
        if (!found) result.missingKeywords.push_back(item.keyword);
    }
    // Native +0x148 is an ordered map.  Its red-black-tree successor walk
    // therefore reports absent keys in bytewise key order, independently of
    // the order in which an external client supplied them.
    std::ranges::sort(result.missingKeywords);
    return result;
}

} // namespace header_policy_detail

[[nodiscard]] inline VerilogHeaderPolicyResult
evaluateVerilogHeaderPolicy(const core::SourceFile& source, std::size_t moduleLine,
                           const VerilogHeaderPolicy& policy) {
    return header_policy_detail::evaluate(
        source, moduleLine, policy, core::HdlLanguage::Verilog);
}

// VHDL native run 0x1805bb190 uses the same configured keyword container and
// per-entry case flag.  It gates on an empty map, gathers nonempty physical
// prefix lines before the first admitted design root, then performs substring
// searches.  Unknown/default keyword maps must not be fabricated.
[[nodiscard]] inline VerilogHeaderPolicyResult
evaluateVhdlHeaderPolicy(const core::SourceFile& source, std::size_t declarationLine,
                         const VerilogHeaderPolicy& policy) {
    return header_policy_detail::evaluate(
        source, declarationLine, policy, core::HdlLanguage::Vhdl);
}

} // namespace fpga::analysis
