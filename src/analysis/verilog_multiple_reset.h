#pragma once

#include "parser/design_parser.h"

#include <cstddef>
#include <set>
#include <string>
#include <vector>

namespace fpga::analysis {

// Bounded source counterpart of the attachment's GJB10157.6.2.10 native core.
// Native 0x180557340 counts process groups whose byte-key set contains value 1
// and reports once as soon as the second qualifying group is found. Yosys uses
// value 1 for both active-high State::S1 and level-sensitive SyncType::ST1;
// the proprietary group producer is still unavailable (see the evidence note).
// Only simple edge-event lists and explicit if-guarded assignments are admitted.
class VerilogMultipleReset {
public:
    explicit VerilogMultipleReset(const std::vector<parser::HdlToken>& tokens) : tokens_(tokens) {}

    [[nodiscard]] std::vector<std::size_t> violatingAlwaysIndices() const {
        std::vector<std::size_t> result;
        for (std::size_t i = 0; i + 2 < tokens_.size(); ++i) {
            if ((tokens_[i].text != "always" && tokens_[i].text != "always_ff") ||
                tokens_[i + 1].text != "@" ||
                tokens_[i + 2].text != "(") continue;
            const auto close = closing(i + 2, "(", ")", tokens_.size());
            if (close == tokens_.size()) continue;
            std::set<std::string> events;
            bool valid = true, expectEdge = true;
            for (auto j = i + 3; j < close;) {
                if (tokens_[j].text == "`") { valid = false; break; }
                if (!expectEdge) {
                    if (tokens_[j].text != "or" && tokens_[j].text != ",") {
                        valid = false; break;
                    }
                    expectEdge = true; ++j; continue;
                }
                if (j + 1 >= close ||
                    (tokens_[j].text != "posedge" && tokens_[j].text != "negedge") ||
                    tokens_[j + 1].kind != parser::HdlTokenKind::Identifier) {
                    valid = false; break;
                }
                events.insert(tokens_[j + 1].text);
                expectEdge = false; j += 2;
            }
            if (!valid || expectEdge || events.size() < 3 || close + 1 >= tokens_.size() ||
                tokens_[close + 1].text != "begin") { i = close; continue; }
            const auto bodyEnd = closing(close + 1, "begin", "end", tokens_.size());
            if (bodyEnd == tokens_.size()) { i = close; continue; }
            std::set<std::string> guarded;
            bool hasAssignment = false, unknown = false;
            for (auto j = close + 2; j < bodyEnd; ++j) {
                if (tokens_[j].text == "`") { unknown = true; break; }
                if (tokens_[j].text == "if" && j + 1 < bodyEnd && tokens_[j + 1].text == "(") {
                    const auto conditionEnd = closing(j + 1, "(", ")", bodyEnd);
                    if (conditionEnd == bodyEnd) { unknown = true; break; }
                    for (auto at = j + 2; at < conditionEnd; ++at)
                        if (tokens_[at].kind == parser::HdlTokenKind::Identifier &&
                            events.contains(tokens_[at].text)) guarded.insert(tokens_[at].text);
                    j = conditionEnd;
                } else if (tokens_[j].text == "<=") {
                    hasAssignment = true;
                } else if (tokens_[j].text == "<" && j + 1 < bodyEnd &&
                           tokens_[j + 1].text == "=") {
                    hasAssignment = true;
                    ++j;
                }
            }
            if (!unknown && hasAssignment && guarded.size() >= 2 && guarded.size() < events.size())
                result.push_back(i);
            i = bodyEnd;
        }
        return result;
    }

private:
    const std::vector<parser::HdlToken>& tokens_;
    [[nodiscard]] std::size_t closing(std::size_t start, const char* open,
                                       const char* close, std::size_t limit) const {
        int depth = 0;
        for (auto j = start; j < limit; ++j) {
            if (tokens_[j].text == open) ++depth;
            else if (tokens_[j].text == close && --depth == 0) return j;
        }
        return limit;
    }
};

} // namespace fpga::analysis
