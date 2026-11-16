#pragma once

#include "parser/design_parser.h"

#include <algorithm>
#include <charconv>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <limits>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <vector>

namespace fpga::analysis {

struct VerilogFsmStateCountCandidate {
    std::size_t caseToken{};
    std::string control;
    std::vector<std::string> sourceStates;
    std::map<std::string, std::set<std::string>> transitions;
    bool transitionGraphComplete{};
    bool hasDefaultBranch{};
    std::size_t defaultToken{};
    bool defaultFilteredResetSetEmpty{};
    bool defaultResetSetComplete{};
    bool initialized{};
    std::size_t initializationToken{};
    std::string initialState;
};

// Conservative source adapter for GJB10157.10.3/.10.4.  The recovered native
// rules share a per-FSM relation model.  The .10.3 reporter checks the model's
// synthetic `init` relation; .10.4 reports only when its distinct state set is
// larger than 0x28.  This adapter admits a case statement only when the case
// control is a plain identifier and a state update is source-proved either in
// the same edge-triggered process or through a direct next-state register.
class VerilogFsmStateCount {
public:
    explicit VerilogFsmStateCount(const std::vector<parser::HdlToken>& tokens)
        : tokens_(tokens) {}

    [[nodiscard]] std::vector<VerilogFsmStateCountCandidate> candidates() const {
        std::vector<VerilogFsmStateCountCandidate> result;
        const auto edgeAlways = edgeTriggeredAlwaysRanges();
        for (std::size_t caseToken = 0; caseToken + 3 < tokens_.size(); ++caseToken) {
            if (!isCase(tokens_[caseToken].text) || tokens_[caseToken + 1].text != "(") continue;
            const auto close = matching(caseToken + 1, "(", ")");
            if (close != caseToken + 3 ||
                tokens_[caseToken + 2].kind != parser::HdlTokenKind::Identifier) continue;
            const auto caseEnd = matchingCase(caseToken);
            if (caseEnd == tokens_.size()) continue;

            const auto control = tokens_[caseToken + 2].text;
            std::set<std::string> states;
            std::multimap<std::string, std::string> assignments;
            std::set<std::string> defaultResetInputs;
            bool defaultResetSetComplete = true;
            bool hasDefaultBranch = false;
            std::size_t defaultToken = tokens_.size();
            struct StateBranch {
                std::vector<std::string> labels;
                std::multimap<std::string, std::string> assignments;
            };
            std::vector<StateBranch> branches;
            auto cursor = close + 1;
            bool complete = true;
            while (cursor < caseEnd) {
                const auto colon = labelColon(cursor, caseEnd);
                if (colon == tokens_.size()) { complete = false; break; }
                StateBranch branch;
                const bool defaultBranch = tokens_[cursor].text == "default";
                if (!defaultBranch) {
                    auto segment = cursor;
                    int parens = 0, brackets = 0, braces = 0;
                    for (auto at = cursor; at <= colon; ++at) {
                        const bool separator = at == colon ||
                            (tokens_[at].text == "," && parens == 0 && brackets == 0 && braces == 0);
                        if (separator) {
                            if (at == segment + 1 && isStateAtom(tokens_[segment])) {
                                const auto state = atomName(tokens_[segment]);
                                states.insert(state);
                                branch.labels.push_back(state);
                            } else
                                complete = false;
                            segment = at + 1;
                        }
                        if (at == colon) break;
                        if (tokens_[at].text == "(") ++parens;
                        else if (tokens_[at].text == ")") --parens;
                        else if (tokens_[at].text == "[") ++brackets;
                        else if (tokens_[at].text == "]") --brackets;
                        else if (tokens_[at].text == "{") ++braces;
                        else if (tokens_[at].text == "}") --braces;
                    }
                } else {
                    hasDefaultBranch = true;
                    defaultToken = cursor;
                }
                const auto next = statementEnd(colon + 1);
                if (next <= colon + 1 || next > caseEnd) { complete = false; break; }
                collectAssignments(colon + 1, next, branch.assignments);
                if (defaultBranch)
                    collectDefaultResetInputs(colon + 1, next, defaultResetInputs,
                                              defaultResetSetComplete);
                assignments.insert(branch.assignments.begin(), branch.assignments.end());
                if (!branch.labels.empty()) branches.push_back(std::move(branch));
                cursor = next;
            }
            if (!complete || states.empty()) continue;

            bool admitted = false;
            std::string nextState;
            const bool caseInEdgeProcess = std::ranges::any_of(edgeAlways, [&](const auto& range) {
                return range.first <= caseToken && caseToken < range.second;
            });
            const auto direct = assignments.equal_range(control);
            for (auto it = direct.first; it != direct.second; ++it)
                if (caseInEdgeProcess && states.contains(it->second)) admitted = true;

            if (!admitted) {
                for (const auto& [candidateNextState, value] : assignments) {
                    if (candidateNextState == control || !states.contains(value)) continue;
                    if (hasEdgeTransfer(edgeAlways, control, candidateNextState)) {
                        admitted = true;
                        nextState = candidateNextState;
                        break;
                    }
                }
            }
            if (admitted) {
                std::map<std::string, std::set<std::string>> transitions;
                bool transitionGraphComplete = true;
                const auto& transitionTarget = nextState.empty() ? control : nextState;
                for (const auto& branch : branches) {
                    const auto range = branch.assignments.equal_range(transitionTarget);
                    if (range.first == range.second) {
                        transitionGraphComplete = false;
                        continue;
                    }
                    for (auto it = range.first; it != range.second; ++it) {
                        if (!states.contains(it->second)) {
                            transitionGraphComplete = false;
                            continue;
                        }
                        for (const auto& label : branch.labels)
                            transitions[label].insert(it->second);
                    }
                }
                for (const auto& state : states)
                    if (!transitions.contains(state) || transitions[state].empty())
                        transitionGraphComplete = false;
                std::size_t initializationToken = tokens_.size();
                std::size_t explicitInitializationToken = tokens_.size();
                std::size_t resetInitializationToken = tokens_.size();
                std::string explicitInitialState;
                std::string resetInitialState;
                const bool explicitInitialized = hasExplicitInitial(
                    control, explicitInitializationToken, explicitInitialState);
                const bool resetInitialized = hasResetInitialization(
                    edgeAlways, control, nextState, caseToken,
                    resetInitializationToken, resetInitialState);
                std::size_t nativeInitializationToken = tokens_.size();
                const bool initialized = hasNativeSyntheticInitRelation(
                    control, nextState, caseToken, nativeInitializationToken);
                initializationToken = explicitInitialized
                    ? explicitInitializationToken
                    : (resetInitialized ? resetInitializationToken
                                        : nativeInitializationToken);
                std::string initialState;
                if (explicitInitialized && resetInitialized) {
                    if (!explicitInitialState.empty() &&
                        explicitInitialState == resetInitialState)
                        initialState = explicitInitialState;
                } else if (explicitInitialized) {
                    initialState = explicitInitialState;
                } else if (resetInitialized) {
                    initialState = resetInitialState;
                }
                // The native unreachable-state path starts from the synthetic
                // exact key `init`.  Only expose a source-level initial node
                // when the recovered assignment is a simple member of this
                // candidate's explicit state set.  Other initialization still
                // satisfies 10.3, but cannot prove reachability for 10.1.
                if (!states.contains(initialState)) initialState.clear();
                if (!initialized) initialState.clear();
                // FUN_18050aa20 records simple RHS constants/identifiers for
                // assignments under each case item. FUN_180506fa0 then removes
                // RHS identifiers that are themselves assignment targets and
                // reports exactly once when the existing `default` entry has
                // no terminal value left. Numeric X/Z/? literals are still
                // terminal strings in that native relation and are not a
                // separate violation predicate.
                const auto dependencyTargets = directAssignmentTargets(caseToken);
                bool hasTerminalResetValue = false;
                for (const auto& input : defaultResetInputs) {
                    const bool identifier = !input.empty() &&
                        (std::isalpha(static_cast<unsigned char>(input.front())) ||
                         input.front() == '_' || input.front() == '$' || input.front() == '\\');
                    if (!identifier || !dependencyTargets.contains(input)) {
                        hasTerminalResetValue = true;
                        break;
                    }
                }
                result.push_back({caseToken, control,
                                  std::vector<std::string>(states.begin(), states.end()),
                                  std::move(transitions), transitionGraphComplete,
                                  hasDefaultBranch, defaultToken,
                                  hasDefaultBranch && !hasTerminalResetValue,
                                  defaultResetSetComplete,
                                  initialized, initializationToken,
                                  std::move(initialState)});
            }
        }
        return result;
    }

    [[nodiscard]] std::vector<VerilogFsmStateCountCandidate> oversized() const {
        auto result = candidates();
        std::erase_if(result, [](const auto& candidate) {
            return candidate.sourceStates.size() <= 40;
        });
        return result;
    }

private:
    const std::vector<parser::HdlToken>& tokens_;

    static std::string lower(std::string_view value) {
        std::string result(value);
        std::ranges::transform(result, result.begin(), [](unsigned char ch) {
            return static_cast<char>(std::tolower(ch));
        });
        return result;
    }
    static std::optional<std::string> canonicalNumericState(std::string value) {
        value.erase(std::remove(value.begin(), value.end(), '_'), value.end());
        if (value.empty()) return std::nullopt;
        if ((value == "'0") || (value == "'1")) return value.substr(1);

        int base = 10;
        bool signedValue = false;
        std::optional<unsigned> width;
        std::string digits = value;
        if (const auto quote = value.find('\''); quote != std::string::npos) {
            if (quote != 0) {
                unsigned parsedWidth = 0;
                const auto [end, error] = std::from_chars(
                    value.data(), value.data() + quote, parsedWidth, 10);
                if (error != std::errc{} || end != value.data() + quote ||
                    parsedWidth == 0 || parsedWidth > 64) return std::nullopt;
                width = parsedWidth;
            }
            auto marker = quote + 1;
            if (marker < value.size() &&
                (value[marker] == 's' || value[marker] == 'S')) {
                signedValue = true;
                ++marker;
            }
            if (marker >= value.size()) return std::nullopt;
            switch (static_cast<char>(std::tolower(
                static_cast<unsigned char>(value[marker])))) {
            case 'b': base = 2; break;
            case 'o': base = 8; break;
            case 'd': base = 10; break;
            case 'h': base = 16; break;
            default: return std::nullopt;
            }
            digits = value.substr(marker + 1);
        }
        if (digits.empty() || digits.find_first_of("xXzZ?") != std::string::npos)
            return std::nullopt;
        std::uint64_t parsed = 0;
        const auto [end, error] = std::from_chars(
            digits.data(), digits.data() + digits.size(), parsed, base);
        if (error != std::errc{} || end != digits.data() + digits.size())
            return std::nullopt;
        if (width && *width < 64)
            parsed &= (std::uint64_t{1} << *width) - 1;
        if (signedValue && width &&
            (parsed & (std::uint64_t{1} << (*width - 1)))) {
            const auto signedResult = *width == 64
                ? static_cast<std::int64_t>(parsed)
                : static_cast<std::int64_t>(parsed |
                    (~std::uint64_t{0} << *width));
            return std::to_string(signedResult);
        }
        return std::to_string(parsed);
    }
    static std::string atomName(const parser::HdlToken& token) {
        // The native relation builder converts AST_CONSTANT labels through its
        // numeric frontend value before using them as map keys. Preserve
        // Verilog identifier case, but collapse equivalent supported numeric
        // encodings (for example 2'd1 and 2'b01) to the same state key.
        if (token.kind != parser::HdlTokenKind::Number) return token.text;
        if (const auto canonical = canonicalNumericState(token.text)) return *canonical;
        return lower(token.text);
    }
    static bool isCase(std::string_view word) {
        return word == "case" || word == "casex" || word == "casez";
    }
    static bool isStateAtom(const parser::HdlToken& token) {
        return token.kind == parser::HdlTokenKind::Identifier ||
               token.kind == parser::HdlTokenKind::Number;
    }
    std::size_t matching(std::size_t open, std::string_view left,
                         std::string_view right) const {
        int depth = 0;
        for (auto i = open; i < tokens_.size(); ++i) {
            if (tokens_[i].text == left) ++depth;
            else if (tokens_[i].text == right && --depth == 0) return i;
        }
        return tokens_.size();
    }
    std::size_t matchingCase(std::size_t open) const {
        int depth = 0;
        for (auto i = open; i < tokens_.size(); ++i) {
            if (isCase(tokens_[i].text)) ++depth;
            else if (tokens_[i].text == "endcase" && --depth == 0) return i;
        }
        return tokens_.size();
    }
    std::size_t labelColon(std::size_t start, std::size_t limit) const {
        int parens = 0, brackets = 0, braces = 0, ternary = 0;
        for (auto i = start; i < limit; ++i) {
            const auto& text = tokens_[i].text;
            const bool flat = parens == 0 && brackets == 0 && braces == 0;
            if (flat && (text == ";" || text == "begin" || text == "end"))
                return tokens_.size();
            if (flat && text == "?") ++ternary;
            else if (flat && text == ":") {
                if (i + 1 < limit && tokens_[i + 1].text == ":") { ++i; continue; }
                if (ternary != 0) --ternary;
                else return i;
            }
            if (text == "(") ++parens;
            else if (text == ")") --parens;
            else if (text == "[") ++brackets;
            else if (text == "]") --brackets;
            else if (text == "{") ++braces;
            else if (text == "}") --braces;
        }
        return tokens_.size();
    }
    std::size_t statementEnd(std::size_t start) const {
        if (start >= tokens_.size()) return tokens_.size();
        const auto word = tokens_[start].text;
        if (word == "unique" || word == "unique0" || word == "priority")
            return statementEnd(start + 1);
        if (word == "begin") {
            int depth = 1;
            for (auto i = start + 1; i < tokens_.size(); ++i) {
                if (tokens_[i].text == "begin") ++depth;
                else if (tokens_[i].text == "end" && --depth == 0) {
                    auto next = i + 1;
                    if (next + 1 < tokens_.size() && tokens_[next].text == ":") next += 2;
                    return next;
                }
            }
            return tokens_.size();
        }
        if (isCase(word)) {
            const auto end = matchingCase(start);
            return end == tokens_.size() ? end : end + 1;
        }
        if (word == "if" || word == "for" || word == "while" || word == "repeat" ||
            word == "foreach") {
            if (start + 1 >= tokens_.size() || tokens_[start + 1].text != "(") return tokens_.size();
            const auto close = matching(start + 1, "(", ")");
            if (close == tokens_.size()) return close;
            auto next = statementEnd(close + 1);
            if (word == "if" && next < tokens_.size() && tokens_[next].text == "else")
                next = statementEnd(next + 1);
            return next;
        }
        int parens = 0, braces = 0, brackets = 0;
        for (auto i = start; i < tokens_.size(); ++i) {
            const auto& text = tokens_[i].text;
            if (parens == 0 && braces == 0 && brackets == 0 && text == ";") return i + 1;
            if (text == "(") ++parens;
            else if (text == ")") --parens;
            else if (text == "{") ++braces;
            else if (text == "}") --braces;
            else if (text == "[") ++brackets;
            else if (text == "]") --brackets;
        }
        return tokens_.size();
    }
    void collectAssignments(std::size_t begin, std::size_t end,
                            std::multimap<std::string, std::string>& result) const {
        for (auto i = begin; i + 3 < end; ++i) {
            if (tokens_[i].kind != parser::HdlTokenKind::Identifier ||
                (tokens_[i + 1].text != "=" && tokens_[i + 1].text != "<=") ||
                !isStateAtom(tokens_[i + 2]) || tokens_[i + 3].text != ";") continue;
            result.emplace(tokens_[i].text, atomName(tokens_[i + 2]));
        }
    }
    void collectDefaultResetInputs(std::size_t begin, std::size_t end,
                                   std::set<std::string>& inputs,
                                   bool& complete) const {
        for (auto i = begin; i + 2 < end; ++i) {
            if (tokens_[i].kind != parser::HdlTokenKind::Identifier ||
                (tokens_[i + 1].text != "=" && tokens_[i + 1].text != "<=")) continue;
            auto semicolon = i + 2;
            int parens = 0, brackets = 0, braces = 0;
            for (; semicolon < end; ++semicolon) {
                const auto& text = tokens_[semicolon].text;
                if (text == ";" && parens == 0 && brackets == 0 && braces == 0) break;
                if (text == "(") ++parens;
                else if (text == ")") --parens;
                else if (text == "[") ++brackets;
                else if (text == "]") --brackets;
                else if (text == "{") ++braces;
                else if (text == "}") --braces;
            }
            if (semicolon >= end) {
                complete = false;
                return;
            }
            const auto expressionBegin = i + 2;
            if (expressionBegin + 1 == semicolon && isStateAtom(tokens_[expressionBegin])) {
                inputs.insert(atomName(tokens_[expressionBegin]));
            } else if (expressionBegin + 5 == semicolon &&
                       tokens_[expressionBegin + 1].text == "?" &&
                       tokens_[expressionBegin + 3].text == ":" &&
                       isStateAtom(tokens_[expressionBegin + 2]) &&
                       isStateAtom(tokens_[expressionBegin + 4])) {
                inputs.insert(atomName(tokens_[expressionBegin + 2]));
                inputs.insert(atomName(tokens_[expressionBegin + 4]));
            } else {
                complete = false;
            }
            i = semicolon;
        }
    }
    [[nodiscard]] std::pair<std::size_t, std::size_t>
    moduleBounds(std::size_t anchor) const {
        std::size_t begin = 0;
        for (auto i = anchor; i > 0; --i) {
            if (tokens_[i - 1].text == "module") { begin = i - 1; break; }
        }
        auto end = tokens_.size();
        for (auto i = anchor; i < tokens_.size(); ++i) {
            if (tokens_[i].text == "endmodule") { end = i; break; }
        }
        return {begin, end};
    }

    bool caseControlContains(std::size_t caseToken,
                             const std::string& control) const {
        if (caseToken + 1 >= tokens_.size() ||
            tokens_[caseToken + 1].text != "(") return false;
        const auto close = matching(caseToken + 1, "(", ")");
        if (close == tokens_.size()) return false;
        for (auto i = caseToken + 2; i < close; ++i)
            if (tokens_[i].kind == parser::HdlTokenKind::Identifier &&
                tokens_[i].text == control) return true;
        return false;
    }

    bool containsNativeInitAssignment(
        std::size_t begin, std::size_t end,
        const std::set<std::string>& targets, const std::string& control,
        std::size_t& assignmentToken) const {
        end = std::min(end, tokens_.size());
        for (auto i = begin; i + 3 < end; ++i) {
            if (isCase(tokens_[i].text) && caseControlContains(i, control)) {
                const auto nestedEnd = matchingCase(i);
                if (nestedEnd == tokens_.size() || nestedEnd >= end) return false;
                i = nestedEnd;
                continue;
            }
            if (tokens_[i].kind != parser::HdlTokenKind::Identifier ||
                !targets.contains(tokens_[i].text) ||
                (tokens_[i + 1].text != "=" && tokens_[i + 1].text != "<="))
                continue;
            auto semicolon = i + 2;
            int parens = 0, brackets = 0, braces = 0;
            for (; semicolon < end; ++semicolon) {
                const auto& text = tokens_[semicolon].text;
                if (text == ";" && parens == 0 && brackets == 0 && braces == 0) break;
                if (text == "(") ++parens;
                else if (text == ")") --parens;
                else if (text == "[") ++brackets;
                else if (text == "]") --brackets;
                else if (text == "{") ++braces;
                else if (text == "}") --braces;
            }
            if (semicolon >= end) return false;
            const auto expression = i + 2;
            const bool simpleAtom = expression + 1 == semicolon &&
                                    isStateAtom(tokens_[expression]);
            const bool simpleTernary = expression + 5 == semicolon &&
                isStateAtom(tokens_[expression]) &&
                tokens_[expression + 1].text == "?" &&
                isStateAtom(tokens_[expression + 2]) &&
                tokens_[expression + 3].text == ":" &&
                isStateAtom(tokens_[expression + 4]);
            if (simpleAtom || simpleTernary) {
                assignmentToken = i;
                return true;
            }
            i = semicolon;
        }
        return false;
    }

    bool hasNativeSyntheticInitRelation(
        const std::string& control, const std::string& nextState,
        std::size_t candidateCase, std::size_t& initializationToken) const {
        const auto [begin, end] = moduleBounds(candidateCase);
        std::set<std::string> targets{control};
        if (!nextState.empty()) targets.insert(nextState);

        for (auto i = begin; i < end; ++i) {
            if (isCase(tokens_[i].text) &&
                (i == candidateCase || caseControlContains(i, control))) {
                const auto stateCaseEnd = matchingCase(i);
                if (stateCaseEnd < end) i = stateCaseEnd;
                continue;
            }
            if (tokens_[i].text == "initial") {
                const auto statement = statementEnd(i + 1);
                if (statement <= end && containsNativeInitAssignment(
                        i + 1, statement, targets, control, initializationToken))
                    return true;
            }
            if (tokens_[i].text == "if" && i + 1 < end &&
                tokens_[i + 1].text == "(") {
                const auto close = matching(i + 1, "(", ")");
                if (close >= end) continue;
                const auto thenEnd = statementEnd(close + 1);
                if (thenEnd <= end && containsNativeInitAssignment(
                        close + 1, thenEnd, targets, control, initializationToken))
                    return true;
            }
            if (!isCase(tokens_[i].text) || i + 1 >= end ||
                tokens_[i + 1].text != "(") continue;
            const auto close = matching(i + 1, "(", ")");
            const auto caseEnd = matchingCase(i);
            if (close >= end || caseEnd >= end) continue;
            auto cursor = close + 1;
            while (cursor < caseEnd) {
                const auto colon = labelColon(cursor, caseEnd);
                if (colon == tokens_.size()) break;
                const auto branchEnd = statementEnd(colon + 1);
                if (branchEnd <= colon + 1 || branchEnd > caseEnd) break;
                if (tokens_[cursor].text != "default" &&
                    containsNativeInitAssignment(colon + 1, branchEnd, targets,
                                                 control, initializationToken))
                    return true;
                cursor = branchEnd;
            }
        }
        return false;
    }

    [[nodiscard]] std::set<std::string> directAssignmentTargets(std::size_t anchor) const {
        const auto [begin, end] = moduleBounds(anchor);
        static const std::set<std::string_view> declarationWords = {
            "input", "output", "inout", "wire", "reg", "logic", "bit",
            "integer", "parameter", "localparam", "genvar", "typedef"
        };
        std::set<std::string> result;
        for (auto i = begin; i + 1 < end; ++i) {
            if (tokens_[i].kind != parser::HdlTokenKind::Identifier ||
                (tokens_[i + 1].text != "=" && tokens_[i + 1].text != "<=")) continue;
            bool declaration = false;
            for (auto at = i; at > begin; --at) {
                const auto& previous = tokens_[at - 1].text;
                if (previous == ";" || previous == "begin" || previous == "end" ||
                    previous == "module") break;
                if (declarationWords.contains(previous)) { declaration = true; break; }
            }
            if (!declaration) result.insert(tokens_[i].text);
        }
        return result;
    }
    [[nodiscard]] std::vector<std::pair<std::size_t, std::size_t>>
    edgeTriggeredAlwaysRanges() const {
        std::vector<std::pair<std::size_t, std::size_t>> result;
        for (std::size_t i = 0; i + 3 < tokens_.size(); ++i) {
            if (tokens_[i].text != "always" && tokens_[i].text != "always_ff") continue;
            auto at = i + 1;
            if (at < tokens_.size() && tokens_[at].text == "@") ++at;
            if (at >= tokens_.size() || tokens_[at].text != "(") continue;
            const auto close = matching(at, "(", ")");
            if (close == tokens_.size()) continue;
            bool edge = tokens_[i].text == "always_ff";
            for (auto j = at + 1; j < close; ++j)
                if (tokens_[j].text == "posedge" || tokens_[j].text == "negedge") edge = true;
            if (!edge) continue;
            const auto end = statementEnd(close + 1);
            if (end != tokens_.size()) result.emplace_back(i, end);
        }
        return result;
    }
    bool hasEdgeTransfer(const std::vector<std::pair<std::size_t, std::size_t>>& ranges,
                         const std::string& control, const std::string& nextState) const {
        for (const auto& [begin, end] : ranges) {
            for (auto i = begin; i + 3 < end; ++i) {
                if (tokens_[i].text == control && tokens_[i + 1].text == "<=" &&
                    atomName(tokens_[i + 2]) == nextState && tokens_[i + 3].text == ";")
                    return true;
            }
        }
        return false;
    }

    bool containsAssignment(std::size_t begin, std::size_t end,
                            const std::string& target, const std::string* rhs,
                            std::size_t& assignmentToken) const {
        end = std::min(end, tokens_.size());
        for (auto i = begin; i + 3 < end; ++i) {
            if (tokens_[i].kind != parser::HdlTokenKind::Identifier ||
                tokens_[i].text != target ||
                (tokens_[i + 1].text != "=" && tokens_[i + 1].text != "<=") ||
                !isStateAtom(tokens_[i + 2]) || tokens_[i + 3].text != ";") continue;
            if (rhs != nullptr && atomName(tokens_[i + 2]) != *rhs) continue;
            assignmentToken = i;
            return true;
        }
        return false;
    }

    bool hasExplicitInitial(const std::string& control,
                            std::size_t& initializationToken,
                            std::string& initialState) const {
        bool found = false;
        bool ambiguous = false;
        std::string candidate;
        for (std::size_t i = 0; i < tokens_.size(); ++i) {
            if (tokens_[i].text != "initial") continue;
            const auto end = statementEnd(i + 1);
            if (end == tokens_.size()) continue;
            const bool complexControl = containsControlFlow(i + 1, end);
            for (auto at = i + 1; at + 3 < end; ++at) {
                if (tokens_[at].kind != parser::HdlTokenKind::Identifier ||
                    tokens_[at].text != control ||
                    (tokens_[at + 1].text != "=" && tokens_[at + 1].text != "<=") ||
                    !isStateAtom(tokens_[at + 2]) || tokens_[at + 3].text != ";") continue;
                const auto value = atomName(tokens_[at + 2]);
                if (!found) {
                    found = true;
                    initializationToken = at;
                    candidate = value;
                } else if (candidate != value) {
                    ambiguous = true;
                }
                if (complexControl) ambiguous = true;
            }
        }
        initialState = ambiguous ? std::string{} : candidate;
        return found;
    }

    bool containsInitializationAssignment(std::size_t begin, std::size_t end,
                                          const std::string& control,
                                          const std::string& nextState,
                                          std::size_t& assignmentToken) const {
        end = std::min(end, tokens_.size());
        for (auto i = begin; i + 3 < end; ++i) {
            if (tokens_[i].kind != parser::HdlTokenKind::Identifier ||
                tokens_[i].text != control ||
                (tokens_[i + 1].text != "=" && tokens_[i + 1].text != "<=") ||
                !isStateAtom(tokens_[i + 2]) || tokens_[i + 3].text != ";") continue;
            if (!nextState.empty() && atomName(tokens_[i + 2]) == nextState) continue;
            assignmentToken = i;
            return true;
        }
        return false;
    }

    bool hasResetInitialization(
        const std::vector<std::pair<std::size_t, std::size_t>>& ranges,
        const std::string& control, const std::string& nextState, std::size_t caseToken,
        std::size_t& initializationToken, std::string& initialState) const {
        bool found = false;
        bool ambiguous = false;
        std::string candidate;
        auto record = [&](std::size_t assignment, bool trustworthy) {
            const auto value = atomName(tokens_[assignment + 2]);
            if (!found) {
                found = true;
                initializationToken = assignment;
                candidate = value;
            } else if (candidate != value) {
                ambiguous = true;
            }
            if (!trustworthy) ambiguous = true;
        };
        for (const auto& [processBegin, processEnd] : ranges) {
            for (auto i = processBegin; i + 2 < processEnd; ++i) {
                if (tokens_[i].text != "if" || tokens_[i + 1].text != "(") continue;
                const auto close = matching(i + 1, "(", ")");
                if (close >= processEnd) continue;
                const auto thenBegin = close + 1;
                const auto thenEnd = statementEnd(thenBegin);
                if (thenEnd >= processEnd || tokens_[thenEnd].text != "else") continue;
                const auto elseBegin = thenEnd + 1;
                const auto elseEnd = statementEnd(elseBegin);
                if (elseEnd > processEnd) continue;

                std::size_t thenAssignment = tokens_.size();
                std::size_t elseAssignment = tokens_.size();
                const bool thenInitializes = containsInitializationAssignment(
                    thenBegin, thenEnd, control, nextState, thenAssignment);
                const bool elseInitializes = containsInitializationAssignment(
                    elseBegin, elseEnd, control, nextState, elseAssignment);
                std::size_t ignored = tokens_.size();
                const bool thenUpdates =
                    (thenBegin <= caseToken && caseToken < thenEnd) ||
                    (!nextState.empty() && containsAssignment(
                        thenBegin, thenEnd, control, &nextState, ignored));
                ignored = tokens_.size();
                const bool elseUpdates =
                    (elseBegin <= caseToken && caseToken < elseEnd) ||
                    (!nextState.empty() && containsAssignment(
                        elseBegin, elseEnd, control, &nextState, ignored));
                if (thenInitializes && elseUpdates) {
                    record(thenAssignment, !containsControlFlow(thenBegin, thenEnd));
                }
                if (elseInitializes && thenUpdates) {
                    record(elseAssignment, !containsControlFlow(elseBegin, elseEnd));
                }
            }
        }
        initialState = ambiguous ? std::string{} : candidate;
        return found;
    }

    bool containsControlFlow(std::size_t begin, std::size_t end) const {
        end = std::min(end, tokens_.size());
        for (auto i = begin; i < end; ++i) {
            const auto& word = tokens_[i].text;
            if (word == "if" || isCase(word) || word == "for" || word == "while" ||
                word == "repeat" || word == "foreach") return true;
        }
        return false;
    }
};

} // namespace fpga::analysis
