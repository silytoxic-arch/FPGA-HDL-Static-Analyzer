#include "analysis/default_assignment_unknown.h"

#include <iostream>
#include <string>
#include <vector>

int main() {
    using namespace fpga;
    int failures = 0;
    const auto check = [&](std::vector<std::string> words, std::size_t expected,
                           const char* label, bool incomplete = false) {
        std::vector<parser::HdlToken> tokens;
        for (const auto& word : words) {
            const auto kind = word == "'x" || word == "'z" ||
                (!word.empty() && word.front() >= '0' && word.front() <= '9') ?
                parser::HdlTokenKind::Number :
                (!word.empty() && ((word.front() >= 'a' && word.front() <= 'z') ||
                    word.front() == '$')) ? parser::HdlTokenKind::Identifier : parser::HdlTokenKind::Symbol;
            tokens.push_back({kind, word, 1, tokens.size() + 1, tokens.size()});
        }
        const auto result = analysis::detail::defaultAssignmentUnknowns(tokens, 0, tokens.size(),
            [](const auto& token) { return token.text == "'x"; });
        std::size_t hits = 0;
        for (const auto& range : result.assignments)
            if (!range.unknownLiteralIndices.empty()) ++hits;
        if (hits != expected || result.incomplete != incomplete) {
            ++failures;
            std::cerr << "FAIL " << label << ": hits=" << hits
                      << " incomplete=" << result.incomplete << '\n';
        }
    };
    check({"qq", "=", "'x", ";"}, 1, "blocking RHS");
    check({"qq", "<=", "'x", ";"}, 1, "nonblocking RHS");
    check({"qq", "[", "'x", "]", "=", "0", ";"}, 1, "selected LHS");
    check({"qq", "[", "7", ":", "0", "]", "=", "'x", ";"}, 1, "part-selected LHS");
    check({"qq", "[", "'x", "+:", "2", "]", "=", "0", ";"}, 1, "indexed part-select LHS");
    check({"{", "qq", ",", "rr", "[", "'x", "]", "}", "=", "0", ";"}, 1, "concat LHS");
    check({"if", "(", "aa", "==", "'x", ")", "qq", "=", "0", ";"}, 0, "condition excluded");
    check({"while", "(", "aa", "<=", "'x", ")", "qq", "=", "0", ";"}, 0, "while comparison");
    check({"for", "(", "aa", "=", "'x", ";", "aa", "<", "2", ";", "aa", "++", ")",
           "qq", "=", "0", ";"}, 0, "for header excluded");
    check({"repeat", "(", "'x", ")", "qq", "=", "0", ";"}, 0, "repeat count excluded");
    check({"case", "(", "'x", ")", "0", ":", "qq", "=", "0", ";", "endcase"}, 0, "nested case control");
    check({"$display", "(", "'x", ")", ";"}, 0, "standalone task excluded");
    check({"obj", ".", "task", "(", "'x", ",", "aa", "<=", "'x", ")", ";"}, 0,
          "hierarchical task arguments excluded");
    check({"return", "'x", ";"}, 0, "return excluded");
    check({"qq", "=", "aa", "<=", "'x", ";"}, 1, "RHS relation retained");
    check({"begin", ":", "blk", "if", "(", "aa", ")", "begin", "qq", "=", "'x", ";", "end",
           "else", "begin", "qq", "<=", "'x", ";", "end", "end"}, 2, "compound branches");
    check({"begin", "qq", "=", "'x", ";", "qq", "=", "'x", "|", "'x", ";", "end"},
          2, "multiple assignments in one default block");
    check({"qq", "=", "fn", "(", "'x", ")", ";"}, 1, "RHS call retained");
    check({"qq", "=", "'z", ";"}, 0, "Z is not X");
    check({"qq", "=", "'x"}, 0, "missing semicolon", true);
    check({"qq", "+", "rr", "=", "'x", ";"}, 0, "invalid LHS", true);
    return failures ? 1 : 0;
}
