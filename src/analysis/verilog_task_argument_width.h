#pragma once

#include "parser/design_parser.h"

#include <algorithm>
#include <charconv>
#include <cstddef>
#include <functional>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <vector>

namespace fpga::analysis::detail {

// Native 9.4.6 selects AST_ASSIGN_EQ named "$para_assign" with a child
// containing "$task$", then reports every binding whose two child widths
// differ. The recovered frontend producer creates one binding per task formal,
// ordering input children as formal/actual and output children as actual/formal.
// This source adapter supports same-module ANSI and legacy task declarations
// with fixed decimal formal ranges and bounded statically sized actuals.
// ev_99bc8498ab2e03c2678324195c94fd9c9ddc22b01bdbd4068275f9c34d1c6fb8
// ev_36a5079953d99b2b23b59544c7d8aac2853b3045ef9a2a780d323d93b1396288
enum class TaskBindingDirection { Input, Output };

struct TaskArgumentWidthMismatch {
    std::size_t token{};
    std::size_t leftWidth{};
    std::size_t rightWidth{};
    TaskBindingDirection direction{TaskBindingDirection::Input};
    std::string formalName;
};

[[nodiscard]] inline std::vector<TaskArgumentWidthMismatch> taskArgumentWidthMismatches(
    const std::vector<parser::HdlToken>& tokens) {
    using parser::HdlTokenKind;
    std::vector<TaskArgumentWidthMismatch> found;
    const auto decimal = [&](std::size_t i) -> std::optional<std::size_t> {
        if (i >= tokens.size() || tokens[i].kind != HdlTokenKind::Number) return {};
        std::size_t value = 0;
        const auto& text = tokens[i].text;
        const auto [last, error] = std::from_chars(text.data(), text.data() + text.size(), value);
        if (error != std::errc{} || last != text.data() + text.size() || value > 65535) return {};
        return value;
    };
    const auto sliceWidth = [&](std::size_t open, std::size_t end) -> std::optional<std::size_t> {
        if (open + 5 != end || end > tokens.size() || tokens[open].text != "[" ||
            tokens[open + 2].text != ":" || tokens[open + 4].text != "]") return {};
        const auto left = decimal(open + 1), right = decimal(open + 3);
        if (!left || !right) return {};
        return *left > *right ? *left - *right + 1 : *right - *left + 1;
    };
    const auto selectionWidth = [&](std::size_t open,
                                    std::size_t end) -> std::optional<std::size_t> {
        if (open + 3 == end && end <= tokens.size() && tokens[open].text == "[" &&
            tokens[open + 2].text == "]" && decimal(open + 1)) return 1;
        return sliceWidth(open, end);
    };
    const auto closeParen = [&](std::size_t open, std::size_t limit) {
        int depth = 0;
        for (auto i = open; i < limit; ++i) {
            if (tokens[i].text == "(") ++depth;
            else if (tokens[i].text == ")" && --depth == 0) return i;
        }
        return limit;
    };
    struct Formal {
        std::size_t width{};
        TaskBindingDirection direction{TaskBindingDirection::Input};
        std::string name;
    };
    struct Task { std::size_t begin; std::size_t end; std::vector<Formal> formals; };
    for (auto module = std::size_t{0}; module < tokens.size(); ++module) {
        if (tokens[module].text != "module" || tokens[module].kind != HdlTokenKind::Identifier)
            continue;
        auto moduleEnd = module + 1;
        while (moduleEnd < tokens.size() && tokens[moduleEnd].text != "endmodule") ++moduleEnd;
        if (moduleEnd == tokens.size()) break;
        std::set<std::string> moduleSignals;
        std::map<std::string, std::size_t> moduleSignalWidths;
        auto headerEnd = module + 1;
        while (headerEnd < moduleEnd && tokens[headerEnd].text != ";") ++headerEnd;
        auto portOpen = module + 1;
        while (portOpen < headerEnd) {
            if (tokens[portOpen].text != "(") { ++portOpen; continue; }
            if (portOpen > module && tokens[portOpen - 1].text == "#") {
                portOpen = closeParen(portOpen, headerEnd);
                if (portOpen < headerEnd) ++portOpen;
                continue;
            }
            break;
        }
        if (portOpen < headerEnd) {
            const auto portClose = closeParen(portOpen, headerEnd);
            if (portClose < headerEnd) {
                auto segment = portOpen + 1;
                std::optional<std::size_t> inheritedWidth;
                int nested = 0;
                for (auto j = segment; j <= portClose; ++j) {
                    if (j < portClose) {
                        if (tokens[j].text == "(" || tokens[j].text == "[") ++nested;
                        else if (tokens[j].text == ")" || tokens[j].text == "]") --nested;
                        if (tokens[j].text != "," || nested != 0) continue;
                    }
                    bool hasDirection = false;
                    std::optional<std::size_t> segmentWidth;
                    for (auto cursor = segment; cursor < j; ++cursor) {
                        if (tokens[cursor].text == "input" || tokens[cursor].text == "output" ||
                            tokens[cursor].text == "inout") {
                            hasDirection = true;
                            segmentWidth = 1;
                        }
                        if (hasDirection && tokens[cursor].text == "[") {
                            const auto parsed = cursor + 5 <= j ?
                                sliceWidth(cursor, cursor + 5) : std::optional<std::size_t>{};
                            segmentWidth = parsed;
                            break;
                        }
                    }
                    if (hasDirection) inheritedWidth = segmentWidth;
                    for (auto name = j; name > segment;) {
                        --name;
                        if (tokens[name].kind == HdlTokenKind::Identifier) {
                            moduleSignals.insert(tokens[name].text);
                            if (inheritedWidth)
                                moduleSignalWidths[tokens[name].text] = *inheritedWidth;
                            break;
                        }
                    }
                    segment = j + 1;
                }
            }
        }
        std::map<std::string, Task> tasks;
        for (auto i = module + 1; i < moduleEnd; ++i) {
            if (tokens[i].text != "task" || tokens[i].kind != HdlTokenKind::Identifier) continue;
            auto end = i + 1;
            while (end < moduleEnd && tokens[end].text != "endtask") ++end;
            if (end == moduleEnd) break;
            auto headerEnd = i + 1;
            while (headerEnd < end && tokens[headerEnd].text != ";") ++headerEnd;
            if (headerEnd == end) { i = end; continue; }
            auto open = i + 1;
            while (open < headerEnd && tokens[open].text != "(") ++open;
            auto name = open < headerEnd ? open : headerEnd;
            while (name > i + 1 && tokens[name - 1].kind != HdlTokenKind::Identifier) --name;
            if (name == i + 1 || tokens[name - 1].kind != HdlTokenKind::Identifier) {
                i = end; continue;
            }
            const auto taskName = tokens[name - 1].text;
            std::vector<Formal> formals;
            bool supported = true;
            const auto appendDeclaration = [&](std::size_t first, std::size_t last) {
                if (first >= last || (tokens[first].text != "input" &&
                    tokens[first].text != "output" && tokens[first].text != "inout")) return false;
                if (tokens[first].text == "inout") return false;
                const auto direction = tokens[first].text == "input" ?
                    TaskBindingDirection::Input : TaskBindingDirection::Output;
                auto cursor = first + 1;
                while (cursor < last && (tokens[cursor].text == "reg" ||
                    tokens[cursor].text == "logic" || tokens[cursor].text == "signed" ||
                    tokens[cursor].text == "wire")) ++cursor;
                std::size_t width = 1;
                if (cursor < last && tokens[cursor].text == "[") {
                    if (cursor + 5 > last) return false;
                    const auto parsed = sliceWidth(cursor, cursor + 5);
                    if (!parsed) return false;
                    width = *parsed;
                    cursor += 5;
                }
                bool added = false;
                for (; cursor < last; ++cursor) {
                    if (tokens[cursor].text == ",") continue;
                    if (tokens[cursor].kind != HdlTokenKind::Identifier) return false;
                    formals.push_back({width, direction, tokens[cursor].text});
                    added = true;
                }
                return added;
            };
            if (open < headerEnd) {
                const auto close = closeParen(open, headerEnd + 1);
                if (close + 1 != headerEnd) { supported = false; }
                else {
                    auto segment = open + 1;
                    int nested = 0;
                    for (auto j = segment; j <= close && supported; ++j) {
                        if (j < close) {
                            if (tokens[j].text == "(" || tokens[j].text == "[") ++nested;
                            else if (tokens[j].text == ")" || tokens[j].text == "]") --nested;
                            if (tokens[j].text != "," || nested != 0) continue;
                        }
                        supported = appendDeclaration(segment, j);
                        segment = j + 1;
                    }
                }
            } else {
                auto cursor = headerEnd + 1;
                while (cursor < end && supported) {
                    if (tokens[cursor].text == "begin") break;
                    if (tokens[cursor].text != "input" && tokens[cursor].text != "output" &&
                        tokens[cursor].text != "inout") { ++cursor; continue; }
                    auto statementEnd = cursor + 1;
                    while (statementEnd < end && tokens[statementEnd].text != ";") ++statementEnd;
                    if (statementEnd == end) { supported = false; break; }
                    supported = appendDeclaration(cursor, statementEnd);
                    cursor = statementEnd + 1;
                }
            }
            if (supported && !formals.empty())
                tasks.emplace(taskName, Task{i, end, std::move(formals)});
            i = end;
        }
        // Fixed-width module-scope declarations are valid task actuals too.
        // Exclude all task bodies so their formal/local declarations do not
        // leak into the enclosing module's symbol table.
        const auto insideTask = [&](std::size_t index) {
            return std::ranges::any_of(tasks, [&](const auto& item) {
                return index >= item.second.begin && index <= item.second.end;
            });
        };
        for (auto i = headerEnd + 1; i < moduleEnd; ++i) {
            if (insideTask(i) || (tokens[i].text != "input" && tokens[i].text != "output" &&
                tokens[i].text != "inout" && tokens[i].text != "wire" &&
                tokens[i].text != "reg" && tokens[i].text != "logic")) continue;
            auto statementEnd = i + 1;
            while (statementEnd < moduleEnd && tokens[statementEnd].text != ";") ++statementEnd;
            auto cursor = i + 1;
            while (cursor < statementEnd && (tokens[cursor].text == "signed" ||
                tokens[cursor].text == "unsigned" || tokens[cursor].text == "wire" ||
                tokens[cursor].text == "reg" || tokens[cursor].text == "logic")) ++cursor;
            std::size_t width = 1;
            if (cursor < statementEnd && tokens[cursor].text == "[") {
                const auto parsed = cursor + 5 <= statementEnd ?
                    sliceWidth(cursor, cursor + 5) : std::optional<std::size_t>{};
                if (!parsed) { i = statementEnd; continue; }
                width = *parsed;
                cursor += 5;
            }
            bool expectName = true;
            for (; cursor < statementEnd; ++cursor) {
                if (tokens[cursor].text == "=") break;
                if (tokens[cursor].text == ",") { expectName = true; continue; }
                if (expectName && tokens[cursor].kind == HdlTokenKind::Identifier) {
                    moduleSignals.insert(tokens[cursor].text);
                    moduleSignalWidths[tokens[cursor].text] = width;
                    expectName = false;
                }
            }
            i = statementEnd;
        }
        const auto literalWidth = [&](std::size_t index) -> std::optional<std::size_t> {
            if (index >= tokens.size() || tokens[index].kind != HdlTokenKind::Number) return {};
            const auto quote = tokens[index].text.find('\'');
            if (quote == std::string::npos || quote == 0) return {};
            std::size_t width = 0;
            for (std::size_t at = 0; at < quote; ++at) {
                const auto ch = tokens[index].text[at];
                if (ch == '_') continue;
                if (ch < '0' || ch > '9' || width > 65535) return {};
                width = width * 10 + static_cast<std::size_t>(ch - '0');
            }
            return width == 0 || width > 65535 ? std::optional<std::size_t>{} : width;
        };
        std::function<std::optional<std::size_t>(std::size_t, std::size_t)> expressionWidth;
        expressionWidth = [&](std::size_t first, std::size_t last)
            -> std::optional<std::size_t> {
            if (first >= last) return {};
            while (first + 1 < last && tokens[first].text == "(" &&
                   closeParen(first, last) == last - 1) { ++first; --last; }
            if (first + 1 == last) {
                if (tokens[first].kind == HdlTokenKind::Identifier) {
                    const auto known = moduleSignalWidths.find(tokens[first].text);
                    return known == moduleSignalWidths.end() ? std::optional<std::size_t>{} :
                        std::optional<std::size_t>{known->second};
                }
                return literalWidth(first);
            }
            if (tokens[first].kind == HdlTokenKind::Identifier &&
                moduleSignals.contains(tokens[first].text) && tokens[first + 1].text == "[") {
                if (const auto selected = selectionWidth(first + 1, last)) return selected;
            }
            if (tokens[first].text == "{" && tokens[last - 1].text == "}") {
                if (first + 3 < last && decimal(first + 1) && tokens[first + 2].text == "{" &&
                    tokens[last - 2].text == "}") {
                    const auto inner = expressionWidth(first + 3, last - 2);
                    if (inner && *decimal(first + 1) <= 65535 / std::max<std::size_t>(*inner, 1))
                        return *decimal(first + 1) * *inner;
                    return {};
                }
                std::size_t total = 0, segment = first + 1;
                int parens = 0, brackets = 0, braces = 0;
                for (auto at = segment; at < last; ++at) {
                    const auto& word = tokens[at].text;
                    const bool final = at + 1 == last;
                    if (!final) {
                        if (word == "(") ++parens; else if (word == ")") --parens;
                        else if (word == "[") ++brackets; else if (word == "]") --brackets;
                        else if (word == "{") ++braces; else if (word == "}") --braces;
                    }
                    if (!final && (word != "," || parens || brackets || braces)) continue;
                    const auto end = final ? last - 1 : at;
                    const auto item = expressionWidth(segment, end);
                    if (!item || total > 65535 - *item) return {};
                    total += *item;
                    segment = at + 1;
                }
                return total == 0 ? std::optional<std::size_t>{} : total;
            }
            int parens = 0, brackets = 0, braces = 0;
            for (auto at = last; at-- > first;) {
                const auto& word = tokens[at].text;
                if (word == ")") ++parens; else if (word == "(") --parens;
                else if (word == "]") ++brackets; else if (word == "[") --brackets;
                else if (word == "}") ++braces; else if (word == "{") --braces;
                if (parens || brackets || braces || at == first) continue;
                if (word != "+" && word != "-" && word != "*" && word != "/" &&
                    word != "%" && word != "&" && word != "|" && word != "^") continue;
                const auto left = expressionWidth(first, at);
                const auto right = expressionWidth(at + 1, last);
                if (left && right) return std::max(*left, *right);
                return {};
            }
            if ((tokens[first].text == "+" || tokens[first].text == "-" ||
                 tokens[first].text == "~") && first + 1 < last)
                return expressionWidth(first + 1, last);
            return {};
        };
        for (auto i = module + 1; i + 1 < moduleEnd; ++i) {
            if (tokens[i].kind != HdlTokenKind::Identifier || tokens[i + 1].text != "(") continue;
            const auto task = tasks.find(tokens[i].text);
            if (task == tasks.end() || (i >= task->second.begin && i <= task->second.end) ||
                (i > 0 && (tokens[i - 1].text == "." || tokens[i - 1].text == "::" ||
                           tokens[i - 1].text == "`"))) continue;
            const auto close = closeParen(i + 1, moduleEnd);
            if (close == moduleEnd || close + 1 >= moduleEnd || tokens[close + 1].text != ";")
                continue;
            std::vector<std::pair<std::size_t, std::size_t>> actuals;
            auto segment = i + 2;
            int parens = 0, brackets = 0, braces = 0;
            for (auto j = segment; j <= close; ++j) {
                if (j < close) {
                    if (tokens[j].text == "(") ++parens;
                    else if (tokens[j].text == ")") --parens;
                    else if (tokens[j].text == "[") ++brackets;
                    else if (tokens[j].text == "]") --brackets;
                    else if (tokens[j].text == "{") ++braces;
                    else if (tokens[j].text == "}") --braces;
                    if (tokens[j].text != "," || parens || brackets || braces) continue;
                }
                actuals.emplace_back(segment, j);
                segment = j + 1;
            }
            if (actuals.size() != task->second.formals.size()) { i = close; continue; }
            for (auto index = std::size_t{0}; index < actuals.size(); ++index) {
                const auto [first, last] = actuals[index];
                if (first >= last) continue;
                const auto actualWidth = expressionWidth(first, last);
                const auto& formal = task->second.formals[index];
                if (!actualWidth || *actualWidth == formal.width) continue;
                if (formal.direction == TaskBindingDirection::Input) {
                    found.push_back({first, formal.width, *actualWidth,
                                     formal.direction, formal.name});
                } else {
                    found.push_back({first, *actualWidth, formal.width,
                                     formal.direction, formal.name});
                }
            }
            i = close;
        }
        module = moduleEnd;
    }
    return found;
}

} // namespace fpga::analysis::detail
