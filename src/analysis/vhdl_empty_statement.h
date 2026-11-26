#pragma once

#include "parser/design_parser.h"

#include <cctype>
#include <string>
#include <vector>

namespace fpga::analysis {

// Bounded source adapter for the recovered ALWAYS/NULL/empty-BLOCK consumer.
// Native producer evidence: docs/native_rule_recovery_pass126.md.
// This does not reconstruct all proprietary VHDL lowering or parser recovery.
[[nodiscard]] inline std::vector<std::size_t> nativeVhdlEmptyStatementLines(
    const std::vector<parser::HdlToken>& tokens) {
    std::vector<std::size_t> lines;
    const auto word = [&](std::size_t i, const char* expected) {
        if (i >= tokens.size() || tokens[i].kind != parser::HdlTokenKind::Identifier) return false;
        std::string text = tokens[i].text;
        for (auto& c : text) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        return text == expected;
    };
    const auto symbol = [&](std::size_t i, const char* expected) {
        return i < tokens.size() && tokens[i].text == expected;
    };
    for (std::size_t process = 0; process < tokens.size(); ++process) {
        if (!word(process, "process") || (process > 0 && word(process - 1, "end")) ||
            (process > 1 && word(process - 1, "postponed") && word(process - 2, "end"))) continue;
        auto start = process;
        if (start > 0 && word(start - 1, "postponed")) --start;
        if (start > 1 && symbol(start - 1, ":") &&
            tokens[start - 2].kind == parser::HdlTokenKind::Identifier) start -= 2;

        auto closing = process + 1;
        auto closingProcess = tokens.size();
        for (; closing < tokens.size(); ++closing) {
            if (!word(closing, "end")) continue;
            auto next = closing + 1;
            if (word(next, "postponed")) ++next;
            if (word(next, "process")) { closingProcess = next; break; }
        }
        if (closingProcess == tokens.size()) continue;
        auto terminator = closingProcess + 1;
        if (terminator < tokens.size() &&
            tokens[terminator].kind == parser::HdlTokenKind::Identifier) ++terminator;
        if (!symbol(terminator, ";")) continue;
        bool hasBegin = false;
        bool nestedProcess = false;
        for (auto i = process + 1; i < closing; ++i) {
            hasBegin = hasBegin || word(i, "begin");
            nestedProcess = nestedProcess || word(i, "process");
        }
        if (!hasBegin || nestedProcess) continue;

        // Process + optional sensitivity list + optional IS + BEGIN. Only an
        // actually empty root (or a sole sensitivity EDGE child) qualifies;
        // declarations are AST children and cannot be ignored as empty text.
        auto header = process + 1;
        if (symbol(header, "(")) {
            unsigned depth = 1;
            while (++header < closing && depth != 0) {
                if (symbol(header, "(")) ++depth;
                else if (symbol(header, ")")) --depth;
            }
            if (depth != 0) continue;
        }
        if (word(header, "is")) ++header;
        if (word(header, "begin") && header + 1 == closing)
            lines.push_back(tokens[start].line);

        bool inBody = false;
        for (auto i = process + 1; i < closing; ++i) {
            if (word(i, "begin")) inBody = true;
            // LiteralContext produces type 0xb0 in expressions/declarations;
            // bare NULL; instead leaves its Sequential_statement BLOCK empty.
            if (word(i, "null")) {
                auto location = i;
                if (symbol(i + 1, ";") && i >= 2 && symbol(i - 1, ":") &&
                    tokens[i - 2].kind == parser::HdlTokenKind::Identifier) location -= 2;
                lines.push_back(tokens[location].line);
                continue;
            }
            if (!inBody) continue;
            // IF lowering creates a true-arm BLOCK at the first condition's
            // start line. ELSIF lowering leaves an empty true BLOCK's +0x214
            // at its constructor default 0 (it is set only when moving bodies).
            // ELSE, CASE alternatives and loops are not inherently empty BLOCKs.
            if (word(i, "then") && i + 1 < closing &&
                (word(i + 1, "elsif") || word(i + 1, "else") ||
                 (word(i + 1, "end") && word(i + 2, "if")))) {
                auto condition = i;
                while (condition > process + 1) {
                    --condition;
                    if (word(condition, "if") || word(condition, "elsif")) {
                        lines.push_back(word(condition, "elsif") ? 0 : tokens[condition + 1].line);
                        break;
                    }
                }
            }

        }
        process = terminator;
    }
    return lines;
}

} // namespace fpga::analysis
