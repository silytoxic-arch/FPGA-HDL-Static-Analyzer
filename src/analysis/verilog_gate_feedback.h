#pragma once

#include "parser/design_parser.h"

#include <algorithm>
#include <cstddef>
#include <map>
#include <set>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace fpga::analysis {

struct VerilogGateFeedbackFinding {
    std::size_t token{};
    std::vector<std::string> signals;
    unsigned countedGateCells{};
    std::string module;
};

// Source lowering for native 7.2.2: SCCs with at most two AND/OR cells.
// Identifiers retain Verilog case sensitivity. Unknown expressions, procedural
// drivers, vectors, generated scopes and unresolved instances are not lowered.
class VerilogGateFeedback {
    struct Node {
        std::vector<std::size_t> inputs;
        std::size_t token{};
        bool cell{};
        bool counted{};
        std::string signal;
    };
    const std::vector<parser::HdlToken>& tokens_;
    std::vector<Node> nodes_;
    std::map<std::string, std::size_t> wires_;
    std::set<std::string> driven_;
    bool valid_{true};
    std::size_t limit_{};

    std::size_t wire(std::size_t at) {
        const auto& name = tokens_[at].text;
        if (const auto it = wires_.find(name); it != wires_.end()) return it->second;
        const auto id = nodes_.size();
        nodes_.push_back({{}, at, false, false, name});
        wires_.emplace(name, id);
        return id;
    }
    std::size_t cell(std::size_t at, bool counted, std::vector<std::size_t> inputs) {
        const auto id = nodes_.size();
        nodes_.push_back({std::move(inputs), at, true, counted, {}});
        return id;
    }
    static int precedence(std::string_view op) {
        if (op == "||") return 1;
        if (op == "&&") return 2;
        if (op == "|") return 3;
        if (op == "^" || op == "^~" || op == "~^") return 4;
        if (op == "&") return 5;
        return 0;
    }
    std::size_t expression(std::size_t& at, int minimum = 1) {
        if (at >= limit_) { valid_ = false; return 0; }
        std::size_t left = 0;
        const auto start = at;
        const auto text = tokens_[at].text;
        if (text == "~" || text == "!") {
            ++at;
            left = cell(start, false, {expression(at, 6)});
        } else if (text == "(") {
            ++at;
            left = expression(at);
            if (at >= limit_ || tokens_[at].text != ")") valid_ = false;
            else ++at;
        } else if (tokens_[at].kind == parser::HdlTokenKind::Identifier) {
            left = wire(at++);
        } else {
            // Constants need native folding before deciding whether feedback
            // survives (q & 0 is not a feedback circuit).
            valid_ = false;
            ++at;
        }
        while (valid_ && at < limit_) {
            const auto op = tokens_[at].text;
            const auto priority = precedence(op);
            if (priority < minimum) break;
            const auto operation = at++;
            const auto right = expression(at, priority + 1);
            left = cell(operation, op == "&" || op == "|" || op == "&&" || op == "||",
                        {left, right});
        }
        return left;
    }
    bool connect(std::size_t target, std::size_t input) {
        if (!driven_.insert(tokens_[target].text).second) return false;
        const auto output = wire(target);
        nodes_[output].inputs.push_back(input);
        return true;
    }
    std::size_t matching(std::size_t at, std::string_view open,
                         std::string_view close, std::size_t end) const {
        int depth = 0;
        for (; at < end; ++at) {
            if (tokens_[at].text == open) ++depth;
            else if (tokens_[at].text == close && --depth == 0) return at;
        }
        return end;
    }
    std::size_t statementEnd(std::size_t at, std::size_t end) const {
        if (at >= end) return end;
        if (tokens_[at].text == "begin") {
            const auto close = matching(at, "begin", "end", end);
            return close == end ? end : close + 1;
        }
        if (tokens_[at].text == "if") {
            if (at + 1 >= end || tokens_[at + 1].text != "(") return end;
            auto next = statementEnd(matching(at + 1, "(", ")", end) + 1, end);
            if (next < end && tokens_[next].text == "else")
                next = statementEnd(next + 1, end);
            return next;
        }
        if (tokens_[at].text == "case" || tokens_[at].text == "casez" ||
            tokens_[at].text == "casex") {
            int depth = 1;
            for (++at; at < end; ++at) {
                if (tokens_[at].text == "case" || tokens_[at].text == "casez" ||
                    tokens_[at].text == "casex") ++depth;
                if (tokens_[at].text == "endcase" && --depth == 0) return at + 1;
            }
            return end;
        }
        while (at < end && tokens_[at].text != ";") ++at;
        return at < end ? at + 1 : end;
    }
    void lowerModule(std::size_t begin, std::size_t end) {
        for (auto at = begin; valid_ && at < end;) {
            const auto text = tokens_[at].text;
            if (text == "generate" || text == "[" || text == "`") { valid_ = false; break; }
            if (text == "function" || text == "task") {
                const auto terminator = text == "function" ? "endfunction" : "endtask";
                while (at < end && tokens_[at].text != terminator) ++at;
                if (at < end) ++at;
                continue;
            }
            if (text == "always" || text == "always_ff" || text == "always_comb" ||
                text == "always_latch" || text == "initial") {
                auto body = at + 1;
                if (body < end && tokens_[body].text == "@") {
                    ++body;
                    if (body < end && tokens_[body].text == "(")
                        body = matching(body, "(", ")", end) + 1;
                    else if (body < end) ++body;
                }
                const auto next = statementEnd(body, end);
                // A procedural writer can invalidate a continuous-driver proof.
                for (auto i = body; i + 1 < next; ++i)
                    if ((tokens_[i + 1].text == "=" || tokens_[i + 1].text == "<=") &&
                        tokens_[i].kind == parser::HdlTokenKind::Identifier &&
                        !driven_.insert(tokens_[i].text).second) valid_ = false;
                at = next;
                continue;
            }
            if (text == "assign") {
                ++at;
                while (valid_ && at < end) {
                    const auto target = at;
                    if (at + 2 >= end || tokens_[at].kind != parser::HdlTokenKind::Identifier ||
                        tokens_[at + 1].text != "=") { valid_ = false; break; }
                    at += 2;
                    limit_ = end;
                    const auto input = expression(at);
                    if (!valid_ || !connect(target, input) || at >= end) { valid_ = false; break; }
                    if (tokens_[at].text == ";") { ++at; break; }
                    if (tokens_[at].text != ",") { valid_ = false; break; }
                    ++at;
                }
                continue;
            }
            if (text == "and" || text == "or" || text == "nand" || text == "nor" ||
                text == "xor" || text == "xnor" || text == "not" || text == "buf") {
                const auto operation = at++;
                while (valid_ && at < end) {
                    if (tokens_[at].kind == parser::HdlTokenKind::Identifier) ++at;
                    if (at >= end || tokens_[at].text != "(") { valid_ = false; break; }
                    const auto close = matching(at, "(", ")", end);
                    const auto target = ++at;
                    std::vector<std::size_t> inputs;
                    bool first = true;
                    while (at < close) {
                        if (!first) {
                            if (tokens_[at++].text != ",") { valid_ = false; break; }
                        }
                        if (at >= close || tokens_[at].kind != parser::HdlTokenKind::Identifier) {
                            valid_ = false; break;
                        }
                        if (!first) inputs.push_back(wire(at));
                        ++at;
                        first = false;
                    }
                    if (!valid_ || close == end || inputs.empty() ||
                        ((text == "not" || text == "buf") && inputs.size() != 1)) {
                        valid_ = false; break;
                    }
                    auto input = text == "buf" ? inputs.front() :
                        cell(operation, text == "and" || text == "or" ||
                             text == "nand" || text == "nor", std::move(inputs));
                    if (text == "nand" || text == "nor" || text == "xnor")
                        input = cell(operation, false, {input});
                    if (!connect(target, input)) { valid_ = false; break; }
                    at = close + 1;
                    if (at < end && tokens_[at].text == ";") { ++at; break; }
                    if (at >= end || tokens_[at++].text != ",") { valid_ = false; break; }
                }
                continue;
            }
            if (text == "input" || text == "output" || text == "inout" || text == "wire" ||
                text == "reg" || text == "logic" || text == "bit" || text == ";") {
                while (at < end && tokens_[at].text != ";") {
                    if (tokens_[at].text == "[" || tokens_[at].text == "=") valid_ = false;
                    ++at;
                }
                if (at < end) ++at;
                continue;
            }
            valid_ = false; // unresolved instance or scoped structural statement
        }
    }
    std::vector<VerilogGateFeedbackFinding> components(bool gateLatchOnly) const {
        const auto size = nodes_.size();
        std::vector<std::vector<std::size_t>> reverse(size);
        for (std::size_t id = 0; id < size; ++id)
            for (auto input : nodes_[id].inputs) reverse[input].push_back(id);
        std::vector<bool> seen(size);
        std::vector<std::size_t> order;
        for (std::size_t id = 0; id < size; ++id) {
            if (seen[id]) continue;
            std::vector<std::pair<std::size_t, std::size_t>> stack{{id, 0}};
            seen[id] = true;
            while (!stack.empty()) {
                auto& [node, next] = stack.back();
                if (next == nodes_[node].inputs.size()) {
                    order.push_back(node); stack.pop_back(); continue;
                }
                const auto input = nodes_[node].inputs[next++];
                if (!seen[input]) { seen[input] = true; stack.emplace_back(input, 0); }
            }
        }
        std::fill(seen.begin(), seen.end(), false);
        std::vector<VerilogGateFeedbackFinding> result;
        for (auto item = order.rbegin(); item != order.rend(); ++item) {
            if (seen[*item]) continue;
            std::vector<std::size_t> stack{*item}, group;
            seen[*item] = true;
            while (!stack.empty()) {
                const auto id = stack.back(); stack.pop_back(); group.push_back(id);
                for (auto input : reverse[id])
                    if (!seen[input]) { seen[input] = true; stack.push_back(input); }
            }
            unsigned counted = 0, cells = 0;
            VerilogGateFeedbackFinding finding{tokens_.size(), {}, 0, {}};
            for (auto id : group) {
                counted += nodes_[id].counted;
                cells += nodes_[id].cell;
                if (!nodes_[id].signal.empty()) {
                    finding.signals.push_back(nodes_[id].signal);
                    finding.token = std::min(finding.token, nodes_[id].token);
                }
            }
            // At least two cells excludes an unproven singleton-cell native
            // feedback path; aliases alone cannot be gate-latch evidence.
            if (cells >= 2 && (!gateLatchOnly || counted <= 2) && !finding.signals.empty()) {
                std::ranges::sort(finding.signals);
                finding.countedGateCells = counted;
                result.push_back(std::move(finding));
            }
        }
        return result;
    }

public:
    explicit VerilogGateFeedback(const std::vector<parser::HdlToken>& tokens) : tokens_(tokens) {}
    std::vector<VerilogGateFeedbackFinding> findings(bool gateLatchOnly = true) {
        std::vector<VerilogGateFeedbackFinding> result;
        for (std::size_t at = 0; at < tokens_.size(); ++at) {
            if (tokens_[at].text != "module") continue;
            auto end = at + 1;
            while (end < tokens_.size() && tokens_[end].text != "endmodule") ++end;
            auto body = at + 1;
            bool scalarHeader = true;
            while (body < end && tokens_[body].text != ";") {
                if (tokens_[body].text == "[") scalarHeader = false;
                ++body;
            }
            nodes_.clear(); wires_.clear(); driven_.clear(); valid_ = scalarHeader;
            if (body < end) lowerModule(body + 1, end);
            if (valid_) {
                auto found = components(gateLatchOnly);
                for (auto& finding : found)
                    if (at + 1 < body) finding.module = tokens_[at + 1].text;
                result.insert(result.end(), found.begin(), found.end());
            }
            at = end;
        }
        return result;
    }
};

} // namespace fpga::analysis
