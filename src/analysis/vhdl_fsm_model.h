#pragma once

#include "parser/design_parser.h"

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <vector>

namespace fpga::analysis {

struct VhdlFsmTransition {
    std::string from, to;
    std::size_t assignmentToken{0};
    bool defaultBranch{false};
};
struct VhdlFsmModel {
    std::string architecture, control;
    std::string clock;
    std::size_t clockToken{0}, encodingToken{0};
    bool clockBindingComplete{false}, encodingExplicit{false}, safeEncoding{false};
    bool encodingResolved{true};
    int encoding{0}; // native model: sequential/default=0, onehot=1, gray=2
    std::string encodingAttributeValue;
    std::size_t caseToken{0}, processToken{0};
    std::vector<std::string> declaredStates, branchStates, sourceStates, destinationStates;
    std::vector<VhdlFsmTransition> transitions;
    std::vector<std::string> declarationInitialStates, outsideCaseAssignments;
    bool partial{true}; // conditions/assignment priority/full elaboration remain unproved
};
struct VhdlFsmStateCountCandidate {
    std::string architecture, control;
    std::size_t caseToken{0};
    std::vector<std::string> sourceStates;
    bool partial{true};
};
struct VhdlFsmExtraction {
    std::vector<VhdlFsmModel> models;
    std::vector<VhdlFsmStateCountCandidate> oversizedStateCandidates;
    std::vector<std::string> unresolvedReasons;
};

// Independent bounded source model, not a vendor implementation. Native
// candidate admission requires identifier-controlled case and actual supported
// transitions; enum alone is not an FSM.
// ev_0d7041d4f4bda3b5c3f0ec98c66422e36cc3307225c2e6d05c342930e8588227
// ev_ab95757791e9973734b61acf08d62ccf2f799814a511041c9470e991cf50c06d
// Native adjacency stores source keys only; destinationStates is intentionally
// separate (ev_b3ceab6cb262e3b768f796dbeb36161686c7e984a693d327c11bb48b2c33c91c).
class VhdlFsmExtractor {
public:
    explicit VhdlFsmExtractor(const std::vector<parser::HdlToken>& tokens) : tokens_(tokens) {
        for (const auto& token : tokens) {
            auto text = token.text;
            std::transform(text.begin(), text.end(), text.begin(),
                           [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
            words_.push_back(std::move(text));
        }
    }
    [[nodiscard]] VhdlFsmExtraction extract() const {
        VhdlFsmExtraction result;
        for (std::size_t i = 0; i + 3 < words_.size(); ++i) {
            if (words_[i] != "architecture" || words_[i + 2] != "of" ||
                (i > 0 && words_[i - 1] == "end")) continue;
            const auto name = words_[i + 1];
            auto end = i + 3;
            bool insideProcess = false;
            for (; end < words_.size(); ++end) {
                if (words_[end] == "process") {
                    insideProcess = end == 0 || words_[end - 1] != "end";
                }
                if (words_[end] == "end" && end + 1 < words_.size() &&
                    (words_[end + 1] == "architecture" || words_[end + 1] == name ||
                     (words_[end + 1] == ";" && !insideProcess))) break;
                if (end > i + 3 && words_[end] == "architecture" && words_[end - 1] != "end") break;
            }
            if (end == words_.size()) {
                result.unresolvedReasons.push_back(name + ": unsupported or unterminated architecture ending");
                continue;
            }
            architecture(result, name, words_[i + 3], i, i + 3, end);
            i = end;
        }
        return result;
    }
private:
    const std::vector<parser::HdlToken>& tokens_;
    std::vector<std::string> words_;
    struct Signal { std::string type, initial; bool ambiguous{false}; };
    struct Process { std::size_t start, end; bool clocked; };
    static std::vector<std::string> vectorOf(const std::set<std::string>& values) {
        return {values.begin(), values.end()};
    }
    bool identifier(std::size_t i) const {
        return i < tokens_.size() && tokens_[i].kind == parser::HdlTokenKind::Identifier;
    }
    std::size_t semicolon(std::size_t start, std::size_t end) const {
        for (auto i = start; i < end; ++i) if (words_[i] == ";") return i;
        return end;
    }
    // Accept normalized operators or the public lexer's two symbols.
    std::size_t op(std::size_t i, std::size_t end, const std::string& value) const {
        if (i < end && words_[i] == value) return 1;
        if (i + 1 < end && words_[i] == value.substr(0, 1) &&
            words_[i + 1] == value.substr(1, 1)) return 2;
        return 0;
    }
    std::size_t assignment(std::size_t i, std::size_t end, const std::string& control) const {
        if (!identifier(i) || words_[i] != control ||
            (i > 0 && (words_[i - 1] == "." || words_[i - 1] == "'"))) return 0;
        return op(i + 1, end, "<=");
    }
    // Locate the syntactic guard first; architecture() additionally requires
    // bounded same-file IEEE scalar binding and excludes visible shadowing.
    std::optional<std::size_t> insideEdgeGuard(std::size_t start, std::size_t at) const {
        std::vector<std::optional<std::size_t>> guards;
        for (auto j = start; j < at; ++j) {
            if (words_[j] == "end" && j + 1 < at && words_[j + 1] == "if") {
                if (guards.empty()) return {};
                guards.pop_back(); ++j;
            } else if (words_[j] == "if" || words_[j] == "elsif") {
                const bool edge = j + 5 < at &&
                    (words_[j + 1] == "rising_edge" || words_[j + 1] == "falling_edge") &&
                    words_[j + 2] == "(" && identifier(j + 3) &&
                    words_[j + 4] == ")" && words_[j + 5] == "then";
                const auto clock = edge ? std::optional<std::size_t>(j + 3) : std::nullopt;
                if (words_[j] == "if") guards.push_back(clock);
                else if (guards.empty()) return {};
                else guards.back() = clock;
            } else if (words_[j] == "else") {
                if (guards.empty()) return {};
                guards.back() = {};
            }
        }
        for (const auto& guard : guards) if (guard) return guard;
        return {};
    }
    bool ieeeContext() const {
        bool ieee = false, use = false;
        for (std::size_t j = 0; j < words_.size(); ++j) {
            if (words_[j] == "context" || words_[j] == "package") return false;
            if ((words_[j] == "type" || words_[j] == "subtype" || words_[j] == "function" ||
                 words_[j] == "alias") && j + 1 < words_.size() &&
                (words_[j + 1] == "std_logic" || words_[j + 1] == "std_ulogic" ||
                 words_[j + 1] == "rising_edge" || words_[j + 1] == "falling_edge")) return false;
            if (words_[j] == "library") {
                if (j + 2 >= words_.size() || words_[j + 1] != "ieee" || words_[j + 2] != ";") return false;
                ieee = true;
            }
            if (words_[j] == "use") {
                if (j + 6 >= words_.size() || words_[j + 1] != "ieee" || words_[j + 2] != "." ||
                    (words_[j + 3] != "std_logic_1164" && words_[j + 3] != "numeric_std") || words_[j + 4] != "." ||
                    words_[j + 5] != "all" || words_[j + 6] != ";") return false;
                use |= words_[j + 3] == "std_logic_1164";
            }
        }
        return ieee && use;
    }
    bool explicitContextBefore(std::size_t designUnit) const {
        std::size_t begin = 0;
        for (std::size_t j = 0; j < designUnit; ++j) if (words_[j] == "end") {
            const auto finish = semicolon(j + 1, designUnit);
            if (finish < designUnit) begin = finish + 1;
        }
        bool library = false, use = false;
        for (auto j = begin; j < designUnit;) {
            if (j + 2 < designUnit && words_[j] == "library" &&
                words_[j + 1] == "ieee" && words_[j + 2] == ";") {
                library = true; j += 3;
            } else if (j + 6 < designUnit && words_[j] == "use" && words_[j + 1] == "ieee" &&
                words_[j + 2] == "." &&
                (words_[j + 3] == "std_logic_1164" || words_[j + 3] == "numeric_std") && words_[j + 4] == "." &&
                words_[j + 5] == "all" && words_[j + 6] == ";") {
                use |= words_[j + 3] == "std_logic_1164"; j += 7;
            } else return false;
        }
        return library && use;
    }
    std::map<std::string, unsigned> entityClocks(const std::string& entity,
                                               std::size_t before) const {
        std::map<std::string, unsigned> clocks;
        unsigned matches = 0;
        for (std::size_t j = 0; j + 2 < before; ++j) {
            if (words_[j] != "entity" || words_[j + 1] != entity || words_[j + 2] != "is") continue;
            if (!explicitContextBefore(j)) return {};
            ++matches;
            auto finish = j + 3;
            for (; finish + 1 < before; ++finish)
                if (words_[finish] == "end" && (words_[finish + 1] == "entity" ||
                    words_[finish + 1] == entity || words_[finish + 1] == ";")) break;
            if (finish + 1 >= before) return {};
            for (auto k = j + 3; k < finish; ++k)
                if (words_[k] == "generic" || words_[k] == "function" || words_[k] == "alias") return {};
            auto port = j + 3;
            while (port < finish && words_[port] != "port") ++port;
            if (port + 1 >= finish || words_[port + 1] != "(") continue;
            for (auto from = port + 2; from < finish && words_[from] != ")";) {
                auto colon = from;
                while (colon < finish && words_[colon] != ":" && words_[colon] != ";" && words_[colon] != ")") ++colon;
                if (colon >= finish || words_[colon] != ":") break;
                auto type = colon + 1;
                if (words_[type] == "in") ++type;
                const bool scalar = type < finish && (words_[type] == "std_logic" || words_[type] == "std_ulogic") &&
                    type + 1 < finish && (words_[type + 1] == ";" || words_[type + 1] == ")");
                for (auto k = from; k < colon; ++k) if (identifier(k)) clocks[words_[k]] += scalar ? 1 : 100;
                from = type + 1;
                int depth = 0;
                while (from < finish) {
                    if (words_[from] == "(") ++depth;
                    if (words_[from] == ")") { if (!depth) break; --depth; }
                    if (words_[from] == ";" && !depth) { ++from; break; }
                    ++from;
                }
            }
        }
        if (matches != 1) return {};
        clocks[""] = 1; // source-bound same-file entity proof, even with no ports
        return clocks;
    }
    void bindEncoding(VhdlFsmModel& model, std::size_t begin, std::size_t end) const {
        unsigned matches = 0;
        std::map<std::string, unsigned> declarations;
        std::map<std::string, unsigned> validDeclarations;
        const auto encodingAttribute = [](const std::string& name) {
            return name == "fsm_encoding" || name == "syn_encoding";
        };
        for (auto j = begin; j + 2 < end; ++j) {
            if (words_[j] != "attribute" || !encodingAttribute(words_[j + 1]) ||
                words_[j + 2] != ":") continue;
            ++declarations[words_[j + 1]];
            if (j + 4 < end && words_[j + 3] == "string" && words_[j + 4] == ";")
                ++validDeclarations[words_[j + 1]];
        }
        for (auto j = begin; j + 7 < end; ++j) {
            if (words_[j] != "attribute" || !encodingAttribute(words_[j + 1]) ||
                words_[j + 2] != "of") continue;
            const auto finish = semicolon(j + 3, end);
            bool targets = false;
            for (auto k = j + 3; k < finish && words_[k] != ":"; ++k) targets |= words_[k] == model.control;
            if (!targets) continue;
            model.encodingExplicit = true; model.encodingToken = j;
            const auto& attributeName = words_[j + 1];
            // syn_encoding is a commonly predefined synthesis attribute and
            // the packaged examples intentionally omit a local declaration.
            // A present declaration must still be unique and string-typed.
            const bool declarationValid = attributeName == "syn_encoding"
                ? (declarations[attributeName] == 0 ||
                   (declarations[attributeName] == 1 && validDeclarations[attributeName] == 1))
                : (declarations[attributeName] == 1 && validDeclarations[attributeName] == 1);
            if (++matches > 1 || !declarationValid || words_[j + 3] != model.control || words_[j + 4] != ":" ||
                words_[j + 5] != "signal" || words_[j + 6] != "is" || finish != j + 8 ||
                tokens_[j + 7].kind != parser::HdlTokenKind::String) {
                model.encodingResolved = false; continue;
            }
            const auto raw = tokens_[j + 7].text;
            model.encodingAttributeValue = raw;
            const auto has = [&](const std::string& a, const std::string& b) {
                return raw.find(a) != std::string::npos || raw.find(b) != std::string::npos;
            };
            // 022 raw classifier + 020 field mapping, exact lower/upper
            // substrings and ordered precedence, not arbitrary case folding.
            if (has("one-hot", "ONE-HOT") || has("onehot", "ONEHOT")) model.encoding = 1;
            else if (has("sequential", "SEQUENTIAL")) model.encoding = 0;
            else if (has("gray", "GRAY")) model.encoding = 2;
            else if (has("safe", "SAFE")) model.safeEncoding = true;
            else model.encoding = 0; // native unknown/default fallback
        }
    }
    void architecture(VhdlFsmExtraction& result, const std::string& name, const std::string& entity,
                      std::size_t architectureToken, std::size_t begin, std::size_t end) const {
        std::map<std::string, std::vector<std::string>> enums;
        std::set<std::string> ambiguousTypes;
        std::map<std::string, Signal> signals;
        std::vector<Process> processes;
        auto clocks = entityClocks(entity, architectureToken);
        auto declarationEnd = begin;
        while (declarationEnd < end && words_[declarationEnd] != "begin") ++declarationEnd;
        for (auto i = begin; i < end; ++i) {
            if (words_[i] == "process" && (i == begin || words_[i - 1] != "end")) {
                auto close = i + 1;
                for (; close + 1 < end; ++close)
                    if (words_[close] == "end" && words_[close + 1] == "process") break;
                if (close + 1 >= end) continue;
                bool clocked = false;
                for (auto j = i + 1; j + 3 < close; ++j)
                    if ((words_[j] == "rising_edge" || words_[j] == "falling_edge") &&
                        words_[j + 1] == "(" && identifier(j + 2) && words_[j + 3] == ")") clocked = true;
                processes.push_back({i, close, clocked});
                i = close + 1;
                continue;
            }
            if (words_[i] == "type" && i + 3 < end && identifier(i + 1) &&
                words_[i + 2] == "is" && words_[i + 3] == "(") {
                std::vector<std::string> states;
                auto j = i + 4;
                bool expect = true, valid = true;
                for (; j < end && words_[j] != ")"; ++j) {
                    if (expect && identifier(j)) { states.push_back(words_[j]); expect = false; }
                    else if (!expect && words_[j] == ",") expect = true;
                    else valid = false;
                }
                if (valid && !expect && j + 1 < end && words_[j + 1] == ";") {
                    if (enums.contains(words_[i + 1])) ambiguousTypes.insert(words_[i + 1]);
                    enums[words_[i + 1]] = states;
                }
            }
            if (words_[i] == "signal") {
                const auto finish = semicolon(i + 1, end);
                auto colon = i + 1;
                while (colon < finish && words_[colon] != ":") ++colon;
                if (colon + 1 >= finish || !identifier(colon + 1)) continue;
                Signal signal{words_[colon + 1], {}, false};
                const bool scalarClock = colon + 2 == finish &&
                    (signal.type == "std_logic" || signal.type == "std_ulogic");
                if (colon + 2 < finish) {
                    const auto init = op(colon + 2, finish, ":=");
                    if (!init || colon + 3 + init != finish || !identifier(colon + 2 + init))
                        signal.ambiguous = true;
                    else signal.initial = words_[colon + 2 + init];
                }
                for (auto j = i + 1; j < colon; ++j) {
                    if (words_[j] == ",") continue;
                    if (!identifier(j)) { signal.ambiguous = true; continue; }
                    clocks[words_[j]] += scalarClock ? 1 : 100;
                    if (signals.contains(words_[j])) signals[words_[j]].ambiguous = true;
                    else signals.emplace(words_[j], signal);
                }
            }
        }
        for (const auto& process : processes) {
            for (auto i = process.start + 1; i + 2 < process.end; ++i) {
                if (words_[i] != "case" || words_[i - 1] == "end" ||
                    !identifier(i + 1) || words_[i + 2] != "is") continue;
                const auto control = words_[i + 1];
                const auto declaration = signals.find(control);
                if (declaration == signals.end() || declaration->second.ambiguous ||
                    !enums.contains(declaration->second.type) ||
                    ambiguousTypes.contains(declaration->second.type)) continue;
                auto close = i + 3;
                bool nested = false;
                int depth = 1;
                for (; close + 1 < process.end; ++close) {
                    if (words_[close] == "case" && words_[close - 1] != "end") { ++depth; nested = true; }
                    else if (words_[close] == "end" && words_[close + 1] == "case" && --depth == 0) break;
                }
                if (close + 1 >= process.end) continue;
                bool actualUpdate = false;
                for (auto j = i + 3; j < close; ++j) actualUpdate |= assignment(j, close, control) != 0;
                const auto reject = [&](const std::string& why) {
                    result.unresolvedReasons.push_back(name + "." + control + ": " + why);
                };
                std::string transitionSignal = control;
                const Process* clockProcess = &process;
                auto guardClock = insideEdgeGuard(process.start, i);
                const bool twoProcess = !actualUpdate;
                if (twoProcess) {
                    // A combinational case can define a next-state signal while
                    // one separate edge-guarded process updates the state.
                    std::set<std::string> candidates;
                    for (const auto& [signalName, signal] : signals) {
                        if (signalName == control || signal.ambiguous ||
                            signal.type != declaration->second.type) continue;
                        for (auto j = i + 3; j < close; ++j)
                            if (assignment(j, close, signalName)) candidates.insert(signalName);
                    }
                    if (candidates.empty()) { i = close + 1; continue; } // decoder only
                    if (candidates.size() != 1 || process.clocked) {
                        reject("ambiguous next-state signal or clocked decoder");
                        i = close + 1; continue;
                    }
                    transitionSignal = *candidates.begin();
                    unsigned links = 0;
                    for (const auto& candidate : processes) {
                        if (!candidate.clocked || candidate.start == process.start) continue;
                        for (auto j = candidate.start; j < candidate.end; ++j) {
                            const auto size = assignment(j, candidate.end, control);
                            if (!size) continue;
                            const auto rhs = j + 1 + size;
                            if (rhs + 1 != semicolon(rhs, candidate.end) ||
                                words_[rhs] != transitionSignal) continue;
                            if (const auto guard = insideEdgeGuard(candidate.start, j)) {
                                ++links; clockProcess = &candidate; guardClock = guard;
                            }
                        }
                    }
                    if (links != 1) {
                        reject("next-state transfer needs one edge-guarded state assignment");
                        i = close + 1; continue;
                    }
                }
                // T-Linter's packaged 4.11.f.b/.c examples place the IEEE
                // context only before the entity and keep the matching
                // architecture in the same source file. entityClocks() has
                // already proved one uniquely named, explicitly bound entity;
                // admit that exact paired-unit recovery in addition to a
                // standards-strict architecture-local context clause.
                const bool architectureContextBound = explicitContextBefore(architectureToken) ||
                    clocks.contains("");
                if (!guardClock || nested || !ieeeContext() ||
                    !clocks.contains("") || clocks[words_[*guardClock]] != 1) {
                    reject("requires case inside a syntactic edge guard and non-nested case");
                    i = close + 1; continue;
                }
                unsigned driverProcesses = 0, cases = 0;
                unsigned nextDriverProcesses = 0;
                bool shadow = false;
                for (auto j = begin; j < end; ++j) {
                    const bool inProcess = std::any_of(processes.begin(), processes.end(),
                        [&](const auto& p) { return j >= p.start && j <= p.end + 1; });
                    if (!inProcess && assignment(j, end, control)) ++driverProcesses;
                    if (twoProcess && !inProcess && assignment(j, end, transitionSignal)) ++nextDriverProcesses;
                    if (!inProcess && (words_[j] == "function" || words_[j] == "procedure" ||
                        words_[j] == "block" || words_[j] == "generate" || words_[j] == "alias"))
                        shadow = true; // unsupported nested scope/driver binding
                }
                for (const auto& p : processes) {
                    bool writes = false, writesNext = false;
                    for (auto j = p.start; j < p.end; ++j) {
                        if (words_[j] == "function" || words_[j] == "procedure" || words_[j] == "alias" ||
                            words_[j] == "type" || words_[j] == "subtype") shadow = true;
                        writes |= assignment(j, p.end, control) != 0;
                        if (twoProcess) writesNext |= assignment(j, p.end, transitionSignal) != 0;
                        if ((p.start == process.start || p.start == clockProcess->start) && (words_[j] == "variable" ||
                            words_[j] == "constant" || words_[j] == "signal")) {
                            auto colon = j + 1;
                            while (colon < p.end && words_[colon] != ":" && words_[colon] != ";") {
                                shadow |= words_[colon] == control || words_[colon] == transitionSignal ||
                                          words_[colon] == words_[*guardClock]; ++colon;
                            }
                        }
                        if (words_[j] == "case" && j + 1 < p.end && words_[j + 1] == control) ++cases;
                    }
                    driverProcesses += writes;
                    nextDriverProcesses += writesNext;
                }
                if (driverProcesses != 1 || cases != 1 || shadow ||
                    (twoProcess && nextDriverProcesses != 1)) {
                    reject("multiple drivers/cases or local shadowing"); i = close + 1; continue;
                }
                VhdlFsmModel model;
                model.architecture = name; model.control = control;
                model.clock = words_[*guardClock]; model.clockToken = *guardClock;
                model.clockBindingComplete = true;
                model.caseToken = i; model.processToken = clockProcess->start;
                model.declaredStates = enums.at(declaration->second.type);
                bindEncoding(model, begin, declarationEnd);
                const std::set<std::string> states(model.declaredStates.begin(), model.declaredStates.end());
                // Native f.a counts the recovered FSM object's transition-source
                // records, not enum declaration entries. Preserve a narrow
                // source-count candidate even when duplicate enum literals or
                // an undeclared case label make the stricter semantic model
                // unsuitable for graph/encoding rules.
                std::set<std::string> provisionalSources;
                bool provisionalValid = true;
                auto provisionalCursor = i + 3;
                while (provisionalCursor < close) {
                    if (words_[provisionalCursor] != "when") { provisionalValid = false; break; }
                    const auto label = provisionalCursor + 1;
                    auto arrow = label;
                    while (arrow < close && !op(arrow, close, "=>")) ++arrow;
                    if (arrow != label + 1 || !identifier(label)) { provisionalValid = false; break; }
                    const auto state = words_[label];
                    const bool others = state == "others";
                    auto next = arrow + op(arrow, close, "=>");
                    auto bodyEnd = next;
                    while (bodyEnd < close && words_[bodyEnd] != "when") ++bodyEnd;
                    bool branchUpdates = false;
                    for (auto j = next; j < bodyEnd; ++j) {
                        const auto size = assignment(j, bodyEnd, transitionSignal);
                        if (!size) continue;
                        const auto rhs = j + 1 + size;
                        if (rhs + 1 != semicolon(rhs, bodyEnd) || !identifier(rhs)) {
                            provisionalValid = false; break;
                        }
                        branchUpdates = true;
                    }
                    if (!provisionalValid || !branchUpdates) { provisionalValid = false; break; }
                    if (!others) provisionalSources.insert(state);
                    provisionalCursor = bodyEnd;
                }
                bool valid = architectureContextBound &&
                    states.size() == model.declaredStates.size();
                std::set<std::string> labels, sources, destinations;
                bool hasOthers = false;
                auto cursor = i + 3;
                while (cursor < close) {
                    if (words_[cursor] != "when") { valid = false; break; }
                    const auto label = cursor + 1;
                    auto arrow = label;
                    while (arrow < close && !op(arrow, close, "=>")) ++arrow;
                    if (arrow != label + 1 || !identifier(label)) { valid = false; break; }
                    const auto state = words_[label];
                    const bool others = state == "others";
                    if ((!others && !states.contains(state)) || labels.contains(state)) { valid = false; break; }
                    labels.insert(state); hasOthers |= others;
                    auto next = arrow + op(arrow, close, "=>");
                    auto bodyEnd = next;
                    while (bodyEnd < close && words_[bodyEnd] != "when") ++bodyEnd;
                    bool branchUpdates = false;
                    for (auto j = next; j < bodyEnd; ++j) {
                        const auto size = assignment(j, bodyEnd, transitionSignal);
                        if (!size) continue;
                        const auto rhs = j + 1 + size;
                        const auto finish = semicolon(rhs, bodyEnd);
                        if (rhs + 1 != finish || !identifier(rhs) ||
                            (!states.contains(words_[rhs]) && words_[rhs] != control)) { valid = false; break; }
                        const auto target = words_[rhs] == control ? state : words_[rhs];
                        if (others && target == "others") { valid = false; break; }
                        model.transitions.push_back({state, target, j, others});
                        branchUpdates = true;
                        if (!others) sources.insert(state);
                        destinations.insert(target);
                    }
                    // Missing assignment is implicit hold; do not invent edges
                    // or use it as proof of unreachable/deadlock.
                    if (!branchUpdates) { valid = false; break; }
                    cursor = bodyEnd;
                }
                if (!hasOthers && labels.size() != states.size()) valid = false;
                for (auto j = clockProcess->start; j < clockProcess->end; ++j) {
                    if (!twoProcess && j >= i && j <= close + 1) continue;
                    const auto size = assignment(j, clockProcess->end, control);
                    if (!size) continue;
                    const auto rhs = j + 1 + size, finish = semicolon(rhs, clockProcess->end);
                    if (twoProcess && rhs + 1 == finish && words_[rhs] == transitionSignal &&
                        insideEdgeGuard(clockProcess->start, j)) continue;
                    if (rhs + 1 != finish || !states.contains(words_[rhs])) { valid = false; break; }
                    model.outsideCaseAssignments.push_back(words_[rhs]);
                }
                if (!declaration->second.initial.empty()) {
                    if (!states.contains(declaration->second.initial)) valid = false;
                    else model.declarationInitialStates.push_back(declaration->second.initial);
                }
                if (!valid || model.transitions.empty()) {
                    reject("unsupported label/target/hold/initialization or incomplete case");
                    if (provisionalValid && provisionalSources.size() > 40) {
                        VhdlFsmStateCountCandidate candidate;
                        candidate.architecture = name;
                        candidate.control = control;
                        candidate.caseToken = i;
                        candidate.sourceStates = vectorOf(provisionalSources);
                        result.oversizedStateCandidates.push_back(std::move(candidate));
                    }
                }
                else {
                    model.branchStates = vectorOf(labels);
                    model.sourceStates = vectorOf(sources);
                    model.destinationStates = vectorOf(destinations);
                    result.models.push_back(std::move(model));
                }
                i = close + 1;
            }
        }
    }
};

[[nodiscard]] inline VhdlFsmExtraction extractBoundedVhdlFsmModels(
    const core::SourceFile& source, const std::vector<parser::HdlToken>& tokens) {
    if (source.language() != core::HdlLanguage::Vhdl) return {};
    return VhdlFsmExtractor(tokens).extract();
}

} // namespace fpga::analysis
