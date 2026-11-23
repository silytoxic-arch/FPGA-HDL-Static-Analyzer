#pragma once
#include "analysis/power_on_policy.h"
#include "parser/design_parser.h"
#include <map>
#include <set>
#include <utility>
#include <vector>

namespace fpga::analysis {

// Bounded source counterpart of native 6.2.5's single-edge ALWAYS target
// minus initialization set. FUN_18055ae60 keys assignment records by
// normalized target name plus physical line; FUN_1805596f0 then tests the
// target name against the initialization-name set. Complex reset/default-case
// paths handled by FUN_180558ea0 stay unresolved.
inline std::vector<std::size_t> uninitializedPowerOnRegisters(
    const std::vector<parser::HdlToken>& tokens, const PowerOnPolicy& policy) {
    std::vector<std::size_t> result;
    if (policy.technology != FpgaTechnology::NonSram) return result;
    using parser::HdlTokenKind;
    for (std::size_t module = 0; module < tokens.size(); ++module) {
        if (tokens[module].text != "module") continue;
        auto end = module + 1;
        while (end < tokens.size() && tokens[end].text != "endmodule") ++end;
        bool supported = end < tokens.size();
        std::set<std::string> initialTargets;
        std::map<std::pair<std::string, std::size_t>, std::size_t> clockedTargets;
        std::map<std::string, std::size_t> clockedOwners;
        const auto blockEnd = [&](std::size_t begin) {
            if (begin >= end) return tokens.size();
            if (tokens[begin].text != "begin") {
                while (begin < end && tokens[begin].text != ";") ++begin;
                return begin < end ? begin + 1 : tokens.size();
            }
            int depth = 1;
            for (auto at = begin + 1; at < end; ++at) {
                if (tokens[at].text == "begin") ++depth;
                else if (tokens[at].text == "end" && --depth == 0) return at + 1;
            }
            return tokens.size();
        };
        // These constructs can hide reset paths, shadowing, declaration
        // initialization or another driver. Do not turn uncertainty into errors.
        for (auto at = module + 1; at < end; ++at) {
            const auto& word = tokens[at].text;
            if (word == "if" || word == "case" || word == "casex" || word == "casez" ||
                word == "for" || word == "while" || word == "repeat" || word == "generate" ||
                word == "function" || word == "task" || word == "assign" || word == "#" ||
                word == "`" || word == "fork") { supported = false; break; }
            if (word != "initial" && word != "always" && word != "always_ff") {
                if (word == "=") { supported = false; break; }
                continue;
            }
            const auto constructToken = at;
            const bool initial = word == "initial";
            auto body = at + 1;
            if (!initial) {
                if (body + 5 >= end || tokens[body].text != "@" || tokens[body + 1].text != "(" ||
                    (tokens[body + 2].text != "posedge" && tokens[body + 2].text != "negedge") ||
                    tokens[body + 3].kind != HdlTokenKind::Identifier || tokens[body + 4].text != ")") {
                    supported = false; break;
                }
                body += 5;
            }
            const auto next = blockEnd(body);
            if (next > end) { supported = false; break; }
            for (auto cursor = body; cursor < next;) {
                const auto& text = tokens[cursor].text;
                if (text == "begin" || text == "end" || text == ";") { ++cursor; continue; }
                if (cursor + 3 >= next || tokens[cursor].kind != HdlTokenKind::Identifier ||
                    (tokens[cursor + 1].text != "=" && tokens[cursor + 1].text != "<=")) {
                    supported = false; break;
                }
                const auto target = cursor;
                cursor += 2;
                while (cursor < next && tokens[cursor].text != ";") {
                    const auto& rhs = tokens[cursor].text;
                    if (rhs == "=" || rhs == "<=" || rhs == "#" || rhs == "begin" || rhs == "end")
                        supported = false;
                    ++cursor;
                }
                if (cursor >= next) { supported = false; break; }
                if (initial) initialTargets.insert(tokens[target].text);
                else {
                    const auto [owner, inserted] =
                        clockedOwners.emplace(tokens[target].text, constructToken);
                    if (!inserted && owner->second != constructToken) supported = false;
                    clockedTargets.emplace(
                        std::pair{tokens[target].text, tokens[target].line}, target);
                }
                ++cursor;
            }
            at = next - 1;
        }
        if (supported) {
            for (const auto& [key, token] : clockedTargets) {
                const auto& name = key.first;
                // A present INITIAL with unknown acceptance cannot prove an
                // omission. A configured false mirrors native selector != 1.
                if (initialTargets.contains(name) && policy.acceptInitialBlocks.value_or(true)) continue;
                result.push_back(token);
            }
        }
        module = end;
    }
    return result;
}
} // namespace fpga::analysis
