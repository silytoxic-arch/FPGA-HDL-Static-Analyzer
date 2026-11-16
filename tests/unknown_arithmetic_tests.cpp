#include "analysis/unknown_arithmetic.h"

#include <iostream>
#include <string>
#include <vector>

// Deliberately normalized tokens isolate this expression helper from the HDL
// lexer's separately tested number normalization and process-range selection.
int main() {
    using namespace fpga;
    int failures = 0;
    const auto check = [&](std::vector<std::string> words, std::size_t expected,
                           const char* label) {
        std::vector<parser::HdlToken> tokens;
        for (auto& word : words) {
            const auto kind = word == "'x" || word == "'z" ||
                (!word.empty() && word.front() >= '0' && word.front() <= '9') ?
                parser::HdlTokenKind::Number :
                (!word.empty() && ((word.front() >= 'a' && word.front() <= 'z') ||
                    word.front() == '$')) ? parser::HdlTokenKind::Identifier : parser::HdlTokenKind::Symbol;
            tokens.push_back({kind, word, 1, tokens.size() + 1, tokens.size()});
        }
        const auto hits = analysis::detail::unknownArithmeticOccurrences(tokens, 0, tokens.size(),
            [](const auto& token) {
                return (token.text.find('x') != std::string::npos ? 1U : 0U) |
                    (token.text.find('z') != std::string::npos ? 2U : 0U);
            });
        if (hits.size() != expected) {
            ++failures;
            std::cerr << "FAIL " << label << ": " << hits.size() << " != " << expected << '\n';
        }
    };
    check({"{", "'x", ",", "aa", "}", "+", "1"}, 1, "concat descendant");
    check({"fn", "(", "'x", ",", "aa", "+", "1", ")"}, 0, "argument sibling");
    check({"(", "aa", "+", "1", ")", "?", "'x", ":", "0"}, 0, "ternary ancestor");
    check({"(", "'x", "?", "aa", ":", "0", ")", "+", "1"}, 1, "ternary descendant");
    check({"'x", "+", "aa", "*", "2"}, 1, "unrelated nested arithmetic");
    check({"(", "'x", "+", "aa", ")", "*", "2"}, 2, "ancestor arithmetic");
    check({"aa", "+", "fn", "(", "'x", ",", "bb", ")"}, 1, "call descendant");
    check({"{", "2", "{", "'x", ",", "aa", "}", "}", "**", "2"}, 1, "replication");
    check({"'z", ";", "aa", "+", "1"}, 0, "statement sibling");
    check({"-", "'x"}, 0, "unary excluded");
    check({"'x", "%", "2"}, 1, "modulo");
    check({"aa", "[", "'x", "]", "+", "1"}, 1, "select descendant");
    check({"(", "aa", "+", "'x"}, 0, "malformed parenthesis stays conservative");
    check({"aa", "+", "1"}, 0, "known operands");
    check({"$signed", "(", "'z", ")", "/", "2"}, 1, "cast descendant");
    check({"8'hxz", "+", "1"}, 2, "X and Z report independently");
    check({"(", "8'hxz", "+", "1", ")", "*", "2"}, 4,
          "each arithmetic ancestor reports X and Z independently");
    check({"aa", "+", "1", "?", "'x", "+", "1", ":", "bb", "+", "2"}, 1,
          "ternary branches isolated");
    return failures ? 1 : 0;
}
