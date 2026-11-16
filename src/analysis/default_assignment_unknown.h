#pragma once

#include "parser/design_parser.h"

#include <algorithm>
#include <cstddef>
#include <functional>
#include <string_view>
#include <vector>

namespace fpga::analysis::detail {

struct DefaultAssignmentRange {
    std::size_t lhsBegin;
    std::size_t assignmentOperator;
    std::size_t rhsBegin;
    std::size_t end; // half-open, does not include the terminating semicolon
    std::vector<std::size_t> unknownLiteralIndices;
};
struct DefaultAssignmentScan {
    std::vector<DefaultAssignmentRange> assignments;
    bool incomplete{false}; // unsupported/malformed assignment candidate, not compliance
};

// Independent source-level approximation of the observed 9.2.9 assignment
// subtree predicate. Input is ONLY a caller-validated default branch body.
// The caller retains unknown scope/AST-elaboration status separately.
// ev_8874f9a800dc5a78dd52b5e1ca936091c78aa6f519187a241c03fd329b381181
// ev_b9a9f627c79ce2dee1a57669f56d84139fe5549d6cd3427a32c322f5dba8f4d2
class DefaultAssignmentScanner {
public:
    using Predicate = std::function<bool(const parser::HdlToken&)>;
    DefaultAssignmentScanner(const std::vector<parser::HdlToken>& tokens,
                             std::size_t begin, std::size_t end, Predicate unknown)
        : tokens_(tokens), begin_(std::min(begin, tokens.size())),
          end_(std::min(end, tokens.size())), unknown_(std::move(unknown)) {}

    DefaultAssignmentScan scan() {
        DefaultAssignmentScan result;
        auto statement = begin_;
        for (auto i = begin_; i < end_;) {
            const auto& word = tokens_[i].text;
            if ((word == "if" || word == "while" || word == "repeat" || word == "for" ||
                 word == "case" || word == "casex" || word == "casez") &&
                i + 1 < end_ && tokens_[i + 1].text == "(") {
                const auto next = balanced(i + 1, "(", ")", end_);
                if (next == end_ && tokens_[end_ - 1].text != ")") result.incomplete = true;
                i = next;
                statement = i;
                continue;
            }
            if (word == "return" || word == "break" || word == "continue") {
                while (i < end_ && tokens_[i].text != ";") ++i;
                if (i < end_) ++i;
                statement = i;
                continue;
            }
            if (word == "[") {
                const auto next = balanced(i, "[", "]", end_);
                if (next == end_ && tokens_[end_ - 1].text != "]") result.incomplete = true;
                // LHS part-select colons and index comparisons are internal to
                // the target, not case labels or assignment operators.
                i = next;
                continue;
            }
            if (word == "begin" || word == "end" || word == "else" || word == "endcase" ||
                word == ";" || word == ":") {
                ++i;
                if ((word == "begin" || word == "end") && i + 1 < end_ &&
                    tokens_[i].text == ":" &&
                    tokens_[i + 1].kind == parser::HdlTokenKind::Identifier) i += 2;
                statement = i;
                continue;
            }
            // Standalone task/call is not an assignment subtree. RHS calls are
            // consumed below as part of their enclosing assignment, preserving X.
            if (i == statement && tokens_[i].kind == parser::HdlTokenKind::Identifier &&
                i + 1 < end_ && tokens_[i + 1].text == "(") {
                i = balanced(i + 1, "(", ")", end_);
                statement = i;
                continue;
            }
            if (word == "(") {
                // Hierarchical task calls and unsupported standalone grouped
                // syntax cannot introduce assignment candidates from arguments.
                i = balanced(i, "(", ")", end_);
                statement = i;
                continue;
            }
            if (word != "=" && word != "<=") { ++i; continue; }
            if (!lhs(statement, i)) {
                // A <= in a comparison is not silently accepted as assignment.
                // For =, an unsupported LHS prevents a definitive assessment.
                if (word == "=") result.incomplete = true;
                ++i;
                continue;
            }
            const auto finish = rhsEnd(i + 1);
            if (finish == end_ || tokens_[finish].text != ";") {
                result.incomplete = true;
                i = finish;
                statement = i;
                continue;
            }
            DefaultAssignmentRange range{statement, i, i + 1, finish, {}};
            for (auto j = statement; j < finish; ++j)
                if (unknown_(tokens_[j])) range.unknownLiteralIndices.push_back(j);
            result.assignments.push_back(std::move(range));
            i = finish + 1;
            statement = i;
        }
        return result;
    }

private:
    const std::vector<parser::HdlToken>& tokens_;
    std::size_t begin_;
    std::size_t end_;
    Predicate unknown_;

    std::size_t balanced(std::size_t start, std::string_view open,
                         std::string_view close, std::size_t limit) const {
        unsigned depth = 0;
        for (auto i = start; i < limit; ++i) {
            if (tokens_[i].text == open) ++depth;
            if (tokens_[i].text == close && --depth == 0) return i + 1;
        }
        return limit;
    }
    bool lhs(std::size_t start, std::size_t end) const {
        auto cursor = start;
        return lhsPart(cursor, end, 0) && cursor == end;
    }
    bool lhsPart(std::size_t& cursor, std::size_t end, unsigned depth) const {
        if (cursor >= end || depth > 128) return false;
        if (tokens_[cursor].text == "{") {
            ++cursor;
            for (;;) {
                if (!lhsPart(cursor, end, depth + 1) || cursor >= end) return false;
                if (tokens_[cursor].text == "}") { ++cursor; return true; }
                if (tokens_[cursor++].text != ",") return false;
            }
        }
        if (tokens_[cursor].kind != parser::HdlTokenKind::Identifier) return false;
        ++cursor;
        while (cursor < end) {
            if (tokens_[cursor].text == "[") {
                const auto next = balanced(cursor, "[", "]", end);
                if (next == end && tokens_[end - 1].text != "]") return false;
                cursor = next;
            } else if (tokens_[cursor].text == "." || tokens_[cursor].text == "::") {
                ++cursor;
                if (cursor >= end || tokens_[cursor].kind != parser::HdlTokenKind::Identifier)
                    return false;
                ++cursor;
            } else break;
        }
        return true;
    }
    std::size_t rhsEnd(std::size_t start) const {
        int parens = 0, brackets = 0, braces = 0;
        for (auto i = start; i < end_; ++i) {
            const auto& text = tokens_[i].text;
            if (text == "(") ++parens;
            else if (text == ")") --parens;
            else if (text == "[") ++brackets;
            else if (text == "]") --brackets;
            else if (text == "{") ++braces;
            else if (text == "}") --braces;
            if (parens < 0 || brackets < 0 || braces < 0) return i;
            if (parens == 0 && brackets == 0 && braces == 0 &&
                (text == ";" || text == "end" || text == "endcase" || text == "else")) return i;
        }
        return end_;
    }
};

[[nodiscard]] inline DefaultAssignmentScan defaultAssignmentUnknowns(
    const std::vector<parser::HdlToken>& tokens, std::size_t begin, std::size_t end,
    DefaultAssignmentScanner::Predicate predicate) {
    return DefaultAssignmentScanner(tokens, begin, end, std::move(predicate)).scan();
}

} // namespace fpga::analysis::detail
