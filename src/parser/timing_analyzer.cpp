#include "parser/semantic_analyzer.h"

#include <algorithm>
#include <cctype>
#include <optional>
#include <set>
#include <string_view>
#include <unordered_set>

namespace fpga::parser {
namespace {

std::string lower(std::string value) {
    std::ranges::transform(value, value.begin(),
                           [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return value;
}

bool tokenIs(const HdlToken& token, std::string_view value) {
    return lower(token.text) == value;
}

bool identifier(const HdlToken& token) { return token.kind == HdlTokenKind::Identifier; }

core::SourceLocation location(const core::SourceFile& source, const HdlToken& token) {
    return {source.path(), token.line, token.column, token.offset};
}

std::optional<std::size_t> skipBalanced(const std::vector<HdlToken>& tokens,
                                        std::size_t open, std::size_t limit,
                                        std::string_view left, std::string_view right) {
    if (open >= limit || tokens[open].text != left) return std::nullopt;
    int depth = 0;
    for (std::size_t i = open; i < limit; ++i) {
        if (tokens[i].text == left) ++depth;
        else if (tokens[i].text == right && --depth == 0) return i + 1;
    }
    return std::nullopt;
}

std::optional<const HdlToken*> simpleIdentifierOperand(
    const std::vector<HdlToken>& tokens, std::size_t begin, std::size_t end) {
    while (begin < end && tokens[begin].text == "(") {
        const auto after = skipBalanced(tokens, begin, end, "(", ")");
        if (!after || *after != end) break;
        ++begin;
        --end;
    }
    if (begin + 1 == end && identifier(tokens[begin])) return &tokens[begin];
    return std::nullopt;
}

std::optional<bool> vhdlBitLiteralAt(const std::vector<HdlToken>& tokens,
                                     std::size_t position, std::size_t limit,
                                     std::size_t& after) {
    if (position + 1 >= limit || tokens[position].text != "'" ||
        tokens[position + 1].kind != HdlTokenKind::Number) return std::nullopt;
    const auto value = lower(tokens[position + 1].text);
    if (value != "0'" && value != "1'") return std::nullopt;
    after = position + 2;
    return value == "1'";
}

struct LegacyVhdlEdge {
    const HdlToken* signal;
    bool rising;
};

std::optional<LegacyVhdlEdge> legacyVhdlEdgeAt(
    const std::vector<HdlToken>& tokens, std::size_t event,
    std::size_t processBegin, std::size_t processEnd) {
    if (event < processBegin + 2 || event >= processEnd ||
        !tokenIs(tokens[event], "event") || tokens[event - 1].text != "'" ||
        !identifier(tokens[event - 2])) return std::nullopt;
    const auto* signal = &tokens[event - 2];

    std::optional<std::size_t> conditionBegin;
    for (std::size_t cursor = event; cursor > processBegin; ) {
        --cursor;
        if (tokenIs(tokens[cursor], "then")) break;
        if (tokenIs(tokens[cursor], "if") || tokenIs(tokens[cursor], "elsif")) {
            conditionBegin = cursor + 1;
            break;
        }
    }
    if (!conditionBegin) return std::nullopt;
    std::size_t conditionEnd = event + 1;
    while (conditionEnd < processEnd && !tokenIs(tokens[conditionEnd], "then")) {
        ++conditionEnd;
    }
    if (conditionEnd >= processEnd) return std::nullopt;
    const bool conjunction = std::ranges::any_of(
        tokens.begin() + static_cast<std::ptrdiff_t>(*conditionBegin),
        tokens.begin() + static_cast<std::ptrdiff_t>(conditionEnd),
        [](const HdlToken& token) { return tokenIs(token, "and"); });
    if (!conjunction) return std::nullopt;

    for (std::size_t token = *conditionBegin; token < conditionEnd; ++token) {
        if (identifier(tokens[token]) &&
            lower(tokens[token].text) == lower(signal->text) &&
            token + 2 < conditionEnd && tokens[token + 1].text == "=") {
            std::size_t afterLiteral = token + 2;
            const auto value = vhdlBitLiteralAt(
                tokens, token + 2, conditionEnd, afterLiteral);
            if (value) return LegacyVhdlEdge{signal, *value};
        }
        std::size_t afterLiteral = token;
        const auto value = vhdlBitLiteralAt(tokens, token, conditionEnd, afterLiteral);
        if (!value || afterLiteral + 1 >= conditionEnd ||
            tokens[afterLiteral].text != "=" ||
            !identifier(tokens[afterLiteral + 1]) ||
            lower(tokens[afterLiteral + 1].text) != lower(signal->text)) continue;
        return LegacyVhdlEdge{signal, *value};
    }
    return std::nullopt;
}

bool resetLike(std::string_view name) {
    const auto folded = lower(std::string(name));
    return folded.find("reset") != std::string::npos || folded.find("rst") != std::string::npos;
}

bool asynchronousControlLike(std::string_view name) {
    const auto folded = lower(std::string(name));
    return resetLike(name) || folded.find("clear") != std::string::npos ||
           folded.find("clr") != std::string::npos ||
           folded.find("preset") != std::string::npos || folded == "set" ||
           folded.starts_with("set_") || folded.ends_with("_set") ||
           folded.find("_set_") != std::string::npos;
}

bool activeHighByName(std::string_view name) {
    const auto folded = lower(std::string(name));
    return !(folded.ends_with("_n") || folded.ends_with("rstn") ||
             folded.ends_with("resetn") || folded.ends_with("reset_n"));
}

bool activeHighAt(const std::vector<HdlToken>& tokens, std::size_t signal,
                  std::size_t limit) {
    if (signal > 0 && tokens[signal - 1].text == "!") return false;
    const auto end = std::min(limit, signal + 8);
    for (std::size_t i = signal + 1; i < end; ++i) {
        if (tokenIs(tokens[i], "then") || tokens[i].text == ";" || tokens[i].text == ")") break;
        const auto value = lower(tokens[i].text);
        if (value == "0" || value == "0'" || value.ends_with("'b0") ||
            value.ends_with("'h0")) return false;
        if (value == "1" || value == "1'" || value.ends_with("'b1") ||
            value.ends_with("'h1")) return true;
    }
    return activeHighByName(tokens[signal].text);
}

void addClock(std::vector<ClockInfo>& clocks, std::string signal, bool rising,
              const core::SourceLocation& sourceLocation) {
    if (std::ranges::none_of(clocks, [&](const ClockInfo& clock) {
            return lower(clock.signal) == lower(signal) && clock.risingEdge == rising;
        })) clocks.push_back({std::move(signal), rising, sourceLocation});
}

void addReset(std::vector<ResetInfo>& resets, std::string signal, bool activeHigh,
              bool asynchronous, const core::SourceLocation& sourceLocation) {
    const auto found = std::ranges::find_if(resets, [&](const ResetInfo& reset) {
        return lower(reset.signal) == lower(signal);
    });
    if (found == resets.end()) {
        ResetInfo reset;
        reset.signal = std::move(signal);
        reset.activeHigh = activeHigh;
        reset.asynchronous = asynchronous;
        reset.location = sourceLocation;
        reset.synchronousObserved = !asynchronous;
        reset.asynchronousObserved = asynchronous;
        reset.activeHighObserved = activeHigh;
        reset.activeLowObserved = !activeHigh;
        resets.push_back(std::move(reset));
    } else {
        const bool polarityConflict = (activeHigh && found->activeLowObserved) ||
                                      (!activeHigh && found->activeHighObserved);
        found->synchronousObserved = found->synchronousObserved || !asynchronous;
        found->asynchronousObserved = found->asynchronousObserved || asynchronous;
        found->activeHighObserved = found->activeHighObserved || activeHigh;
        found->activeLowObserved = found->activeLowObserved || !activeHigh;
        if (asynchronous) {
            found->asynchronous = true;
            found->activeHigh = activeHigh;
        }
        if (asynchronous || polarityConflict) found->location = sourceLocation;
    }
}

void addProcessReset(std::vector<ProcessResetInfo>& resets, std::string signal,
                     bool activeHigh, bool asynchronous,
                     const core::SourceLocation& sourceLocation) {
    const auto found = std::ranges::find_if(resets, [&](const ProcessResetInfo& reset) {
        return lower(reset.signal) == lower(signal);
    });
    if (found == resets.end()) {
        resets.push_back({std::move(signal), activeHigh, asynchronous, sourceLocation});
    } else if (asynchronous) {
        found->asynchronous = true;
        found->activeHigh = activeHigh;
        found->location = sourceLocation;
    }
}

std::optional<std::size_t> assignmentAfter(const std::vector<HdlToken>& tokens,
                                           std::size_t target, std::size_t limit,
                                           bool vhdl) {
    std::size_t position = target + 1;
    while (position < limit && tokens[position].text == "[") {
        const auto after = skipBalanced(tokens, position, limit, "[", "]");
        if (!after) return std::nullopt;
        position = *after;
    }
    if (position + 1 < limit &&
        ((tokens[position].text == "<" && tokens[position + 1].text == "=") ||
         (vhdl && tokens[position].text == ":" && tokens[position + 1].text == "="))) {
        return position + 2;
    }
    if (!vhdl && position < limit && tokens[position].text == "=" &&
        (position + 1 >= limit || tokens[position + 1].text != "=") &&
        (position == 0 || tokens[position - 1].text != "=")) return position + 1;
    return std::nullopt;
}

struct ProcessAccesses {
    std::vector<std::string> reads;
    std::vector<std::string> writes;
    std::vector<std::string> indexedSignals;
    std::vector<RegisterTransferInfo> directTransfers;
    std::vector<std::string> complexReads;
};

ProcessAccesses
processAccesses(const std::vector<HdlToken>& tokens, std::size_t begin,
                std::size_t end, bool vhdl) {
    std::set<std::string> reads;
    std::set<std::string> writes;
    std::set<std::string> indexedSignals;
    std::unordered_set<std::size_t> targets;
    std::unordered_set<std::size_t> directSources;
    std::vector<RegisterTransferInfo> directTransfers;
    for (std::size_t i = begin; i < end; ++i) {
        if (identifier(tokens[i]) && i + 1 < end && tokens[i + 1].text == "[") {
            indexedSignals.insert(tokens[i].text);
        }
        if (!identifier(tokens[i])) continue;
        const auto assignment = assignmentAfter(tokens, i, end, vhdl);
        if (!assignment) continue;
        writes.insert(tokens[i].text);
        targets.insert(i);
        const auto rhs = *assignment;
        if (rhs + 1 < end && identifier(tokens[rhs]) &&
            tokens[rhs + 1].text == ";") {
            directSources.insert(rhs);
            directTransfers.push_back({tokens[i].text, tokens[rhs].text});
        }
    }
    std::set<std::string> complexReads;
    for (std::size_t i = begin; i < end; ++i) {
        if (!identifier(tokens[i]) || targets.contains(i)) continue;
        reads.insert(tokens[i].text);
        if (!directSources.contains(i)) complexReads.insert(tokens[i].text);
    }
    return {{reads.begin(), reads.end()}, {writes.begin(), writes.end()},
            {indexedSignals.begin(), indexedSignals.end()},
            std::move(directTransfers),
            {complexReads.begin(), complexReads.end()}};
}

std::vector<std::string> verilogScalarSignals(
    const std::vector<HdlToken>& tokens, std::size_t begin, std::size_t end) {
    std::set<std::string> signals;
    for (std::size_t token = begin; token < end; ++token) {
        if (!tokenIs(tokens[token], "logic") && !tokenIs(tokens[token], "reg") &&
            !tokenIs(tokens[token], "wire") && !tokenIs(tokens[token], "bit")) continue;
        std::size_t statementEnd = token + 1;
        bool hasRange = false;
        while (statementEnd < end && tokens[statementEnd].text != ";") {
            hasRange = hasRange || tokens[statementEnd].text == "[";
            ++statementEnd;
        }
        if (hasRange || statementEnd >= end) {
            token = statementEnd;
            continue;
        }
        bool initializer = false;
        for (std::size_t name = token + 1; name < statementEnd; ++name) {
            if (tokens[name].text == ",") { initializer = false; continue; }
            if (tokens[name].text == "=") { initializer = true; continue; }
            if (initializer || !identifier(tokens[name]) ||
                tokenIs(tokens[name], "signed") || tokenIs(tokens[name], "unsigned") ||
                tokenIs(tokens[name], "automatic") || tokenIs(tokens[name], "static")) continue;
            signals.insert(tokens[name].text);
        }
        token = statementEnd;
    }
    return {signals.begin(), signals.end()};
}

std::vector<std::string> vhdlScalarSignals(
    const std::vector<HdlToken>& tokens, std::size_t begin, std::size_t end) {
    std::set<std::string> signals;
    for (std::size_t token = begin; token < end; ++token) {
        if (!tokenIs(tokens[token], "signal")) continue;
        std::size_t colon = token + 1;
        while (colon < end && tokens[colon].text != ":" &&
               tokens[colon].text != ";") ++colon;
        if (colon >= end || tokens[colon].text != ":") continue;
        std::size_t statementEnd = colon + 1;
        bool hasRange = false;
        while (statementEnd < end && tokens[statementEnd].text != ";") {
            hasRange = hasRange || tokens[statementEnd].text == "(";
            ++statementEnd;
        }
        if (colon + 1 >= statementEnd || hasRange ||
            (!tokenIs(tokens[colon + 1], "std_logic") &&
             !tokenIs(tokens[colon + 1], "bit") &&
             !tokenIs(tokens[colon + 1], "boolean"))) continue;
        for (std::size_t name = token + 1; name < colon; ++name) {
            if (identifier(tokens[name])) signals.insert(lower(tokens[name].text));
        }
        token = statementEnd;
    }
    return {signals.begin(), signals.end()};
}

std::size_t verilogStatementEnd(const std::vector<HdlToken>& tokens, std::size_t statement,
                                std::size_t limit) {
    if (statement < limit && tokenIs(tokens[statement], "begin")) {
        int depth = 0;
        for (std::size_t i = statement; i < limit; ++i) {
            if (tokenIs(tokens[i], "begin")) ++depth;
            else if (tokenIs(tokens[i], "end") && --depth == 0) return i + 1;
        }
        return limit;
    }
    while (statement < limit && tokens[statement].text != ";") ++statement;
    return statement < limit ? statement + 1 : statement;
}

std::vector<UnitTimingInfo> verilogTiming(const core::SourceFile& source) {
    const auto tokens = tokenize(source);
    std::vector<UnitTimingInfo> result;
    for (std::size_t module = 0; module + 1 < tokens.size(); ++module) {
        if (!tokenIs(tokens[module], "module") || !identifier(tokens[module + 1])) continue;
        std::size_t end = module + 2;
        while (end < tokens.size() && !tokenIs(tokens[end], "endmodule")) ++end;
        std::size_t body = module + 2;
        int parentheses = 0;
        for (; body < end; ++body) {
            if (tokens[body].text == "(") ++parentheses;
            else if (tokens[body].text == ")") --parentheses;
            else if (tokens[body].text == ";" && parentheses == 0) { ++body; break; }
        }
        UnitTimingInfo unit{tokens[module + 1].text, DesignUnitKind::VerilogModule,
                            {}, {}, {}, verilogScalarSignals(tokens, body, end)};
        for (std::size_t i = module + 2; i < end; ++i) {
            if (!tokenIs(tokens[i], "always") && !tokenIs(tokens[i], "always_ff")) continue;
            std::size_t sensitivity = i + 1;
            if (sensitivity >= end || tokens[sensitivity].text != "@") continue;
            ++sensitivity;
            if (sensitivity >= end || tokens[sensitivity].text != "(") continue;
            const auto after = skipBalanced(tokens, sensitivity, end, "(", ")");
            if (!after) continue;

            struct EdgeEvent { const HdlToken* signal; bool rising; };
            std::vector<EdgeEvent> events;
            for (std::size_t event = sensitivity + 1; event + 1 < *after; ++event) {
                if (!tokenIs(tokens[event], "posedge") &&
                    !tokenIs(tokens[event], "negedge")) continue;
                const HdlToken* signal = nullptr;
                if (identifier(tokens[event + 1])) {
                    const auto following = event + 2;
                    if (following >= *after || tokens[following].text == ")" ||
                        tokens[following].text == "," || tokenIs(tokens[following], "or")) {
                        signal = &tokens[event + 1];
                    }
                } else if (tokens[event + 1].text == "(") {
                    const auto afterOperand = skipBalanced(
                        tokens, event + 1, *after, "(", ")");
                    if (afterOperand) {
                        const auto simple = simpleIdentifierOperand(
                            tokens, event + 2, *afterOperand - 1);
                        if (simple) signal = *simple;
                    }
                }
                if (signal) events.push_back(
                    {signal, tokenIs(tokens[event], "posedge")});
            }
            if (events.empty()) continue;
            bool foundClock = false;
            const EdgeEvent* primaryClock = nullptr;
            std::vector<ProcessResetInfo> processResets;
            std::vector<ClockInfo> processClocks;
            for (const auto& event : events) {
                if (resetLike(event.signal->text) ||
                    (events.size() > 1 && asynchronousControlLike(event.signal->text))) {
                    addReset(unit.resets, event.signal->text, event.rising, true,
                             location(source, *event.signal));
                    addProcessReset(processResets, event.signal->text, event.rising, true,
                                    location(source, *event.signal));
                } else {
                    foundClock = true;
                    if (!primaryClock) primaryClock = &event;
                    addClock(unit.clocks, event.signal->text, event.rising,
                             location(source, *event.signal));
                    addClock(processClocks, event.signal->text, event.rising,
                             location(source, *event.signal));
                }
            }
            if (!foundClock) {
                primaryClock = &events.front();
                addClock(unit.clocks, events.front().signal->text, events.front().rising,
                         location(source, *events.front().signal));
                addClock(processClocks, events.front().signal->text, events.front().rising,
                         location(source, *events.front().signal));
            }

            const auto processEnd = verilogStatementEnd(tokens, *after, end);
            for (std::size_t token = *after; token < processEnd; ++token) {
                if (!identifier(tokens[token]) || !resetLike(tokens[token].text)) continue;
                const auto event = std::ranges::find_if(events, [&](const EdgeEvent& candidate) {
                    return candidate.signal->text == tokens[token].text;
                });
                const bool asynchronous = event != events.end();
                addReset(unit.resets, tokens[token].text,
                         asynchronous ? event->rising : activeHighAt(tokens, token, processEnd),
                         asynchronous, location(source, tokens[token]));
                addProcessReset(processResets, tokens[token].text,
                         asynchronous ? event->rising : activeHighAt(tokens, token, processEnd),
                         asynchronous, location(source, tokens[token]));
            }
            if (primaryClock) {
                auto accesses = processAccesses(tokens, *after, processEnd, false);
                unit.sequentialProcesses.push_back({primaryClock->signal->text,
                    primaryClock->rising, location(source, *primaryClock->signal),
                    std::move(accesses.reads), std::move(accesses.writes),
                    std::move(accesses.indexedSignals), std::move(processResets),
                    std::move(processClocks), std::move(accesses.directTransfers),
                    std::move(accesses.complexReads)});
            }
            i = processEnd > i ? processEnd - 1 : i;
        }
        result.push_back(std::move(unit));
        module = end;
    }
    return result;
}

std::vector<UnitTimingInfo> vhdlTiming(const core::SourceFile& source) {
    const auto tokens = tokenize(source);
    std::vector<UnitTimingInfo> result;
    for (std::size_t architecture = 0; architecture + 4 < tokens.size(); ++architecture) {
        if (!tokenIs(tokens[architecture], "architecture") ||
            !tokenIs(tokens[architecture + 2], "of") || !identifier(tokens[architecture + 3])) continue;
        std::size_t end = architecture + 4;
        while (end + 1 < tokens.size() &&
               !(tokenIs(tokens[end], "end") &&
                 (tokenIs(tokens[end + 1], "architecture") ||
                  tokenIs(tokens[end + 1], tokens[architecture + 1].text)))) ++end;
        std::size_t architectureBody = architecture + 4;
        while (architectureBody < end && !tokenIs(tokens[architectureBody], "begin")) {
            ++architectureBody;
        }
        UnitTimingInfo unit{tokens[architecture + 3].text, DesignUnitKind::VhdlEntity,
                            {}, {}, {},
                            vhdlScalarSignals(tokens, architecture + 4, architectureBody)};
        for (std::size_t process = architecture + 4; process < end; ++process) {
            if (!tokenIs(tokens[process], "process")) continue;
            std::size_t processEnd = process + 1;
            while (processEnd + 1 < end &&
                   !(tokenIs(tokens[processEnd], "end") &&
                     tokenIs(tokens[processEnd + 1], "process"))) ++processEnd;
            if (processEnd + 1 < end) processEnd += 2;

            std::set<std::string> sensitivitySignals;
            std::vector<const HdlToken*> sensitivityResets;
            std::size_t sensitivity = process + 1;
            std::optional<std::size_t> after;
            if (sensitivity < processEnd && tokens[sensitivity].text == "(") {
                after = skipBalanced(tokens, sensitivity, processEnd, "(", ")");
                if (after) {
                    for (std::size_t token = sensitivity + 1; token + 1 < *after; ++token) {
                        if (!identifier(tokens[token])) continue;
                        sensitivitySignals.insert(lower(tokens[token].text));
                        if (asynchronousControlLike(tokens[token].text) &&
                            std::ranges::none_of(sensitivityResets, [&](const HdlToken* existing) {
                                return lower(existing->text) == lower(tokens[token].text);
                            })) {
                            sensitivityResets.push_back(&tokens[token]);
                        }
                    }
                }
            }
            bool foundClock = false;
            const HdlToken* primaryClock = nullptr;
            bool primaryRising = true;
            std::vector<ClockInfo> processClocks;
            for (std::size_t token = after.value_or(process + 1);
                 token + 2 < processEnd; ++token) {
                if ((!tokenIs(tokens[token], "rising_edge") &&
                     !tokenIs(tokens[token], "falling_edge")) ||
                    tokens[token + 1].text != "(") continue;
                const auto afterOperand = skipBalanced(
                    tokens, token + 1, processEnd, "(", ")");
                if (!afterOperand) continue;
                const auto clock = simpleIdentifierOperand(
                    tokens, token + 2, *afterOperand - 1);
                if (!clock) continue;
                foundClock = true;
                if (!primaryClock) {
                    primaryClock = *clock;
                    primaryRising = tokenIs(tokens[token], "rising_edge");
                }
                addClock(unit.clocks, (*clock)->text,
                         tokenIs(tokens[token], "rising_edge"), location(source, **clock));
                addClock(processClocks, (*clock)->text,
                         tokenIs(tokens[token], "rising_edge"), location(source, **clock));
            }
            for (std::size_t token = after.value_or(process + 1);
                 token < processEnd; ++token) {
                const auto edge = legacyVhdlEdgeAt(
                    tokens, token, after.value_or(process + 1), processEnd);
                if (!edge) continue;
                foundClock = true;
                if (!primaryClock) {
                    primaryClock = edge->signal;
                    primaryRising = edge->rising;
                }
                addClock(unit.clocks, edge->signal->text, edge->rising,
                         location(source, *edge->signal));
                addClock(processClocks, edge->signal->text, edge->rising,
                         location(source, *edge->signal));
            }
            if (foundClock) {
                std::vector<ProcessResetInfo> processResets;
                for (std::size_t token = after.value_or(process + 1);
                     token < processEnd; ++token) {
                    if (!identifier(tokens[token]) ||
                        !asynchronousControlLike(tokens[token].text)) continue;
                    addReset(unit.resets, tokens[token].text,
                             activeHighAt(tokens, token, processEnd),
                             sensitivitySignals.contains(lower(tokens[token].text)),
                                 location(source, tokens[token]));
                    addProcessReset(processResets, tokens[token].text,
                             activeHighAt(tokens, token, processEnd),
                             sensitivitySignals.contains(lower(tokens[token].text)),
                             location(source, tokens[token]));
                }
                for (const auto* reset : sensitivityResets) {
                    const auto explicitCondition = std::ranges::find_if(
                        processResets, [&](const ProcessResetInfo& candidate) {
                            return lower(candidate.signal) == lower(reset->text);
                        });
                    if (explicitCondition != processResets.end()) continue;
                    const bool activeHigh = activeHighByName(reset->text);
                    addReset(unit.resets, reset->text, activeHigh, true,
                             location(source, *reset));
                    addProcessReset(processResets, reset->text, activeHigh, true,
                                    location(source, *reset));
                }
                auto accesses = processAccesses(
                    tokens, after.value_or(process + 1), processEnd, true);
                unit.sequentialProcesses.push_back({primaryClock->text, primaryRising,
                    location(source, *primaryClock), std::move(accesses.reads),
                    std::move(accesses.writes), std::move(accesses.indexedSignals),
                    std::move(processResets), std::move(processClocks),
                    std::move(accesses.directTransfers),
                    std::move(accesses.complexReads)});
            }
            process = processEnd > process ? processEnd - 1 : process;
        }
        result.push_back(std::move(unit));
        architecture = end;
    }
    return result;
}

} // namespace

std::vector<UnitTimingInfo> extractTimingInfo(const core::SourceFile& source) {
    if (source.language() == core::HdlLanguage::Verilog ||
        source.language() == core::HdlLanguage::SystemVerilog) return verilogTiming(source);
    if (source.language() == core::HdlLanguage::Vhdl) return vhdlTiming(source);
    return {};
}

} // namespace fpga::parser
