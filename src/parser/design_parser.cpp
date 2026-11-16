#include "parser/design_parser.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <optional>
#include <unordered_map>
#include <string_view>
#include <unordered_set>

namespace fpga::parser {
namespace {

using TokenKind = HdlTokenKind;
using Token = HdlToken;

std::string lower(std::string value) {
    std::ranges::transform(value, value.begin(),
                           [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return value;
}

bool identifierStart(unsigned char character) {
    return std::isalpha(character) || character == '_' || character == '$';
}

bool identifierContinue(unsigned char character) {
    return std::isalnum(character) || character == '_' || character == '$';
}

std::vector<Token> lex(const core::SourceFile& source) {
    const auto& text = source.content();
    const bool vhdl = source.language() == core::HdlLanguage::Vhdl;
    std::vector<Token> tokens;
    std::size_t i = 0;
    std::size_t line = 1;
    std::size_t column = 1;
    const auto advance = [&](char character, std::size_t& position,
                             std::size_t& currentLine, std::size_t& currentColumn) {
        ++position;
        if (character == '\n') { ++currentLine; currentColumn = 1; }
        else { ++currentColumn; }
    };

    while (i < text.size()) {
        const auto character = static_cast<unsigned char>(text[i]);
        if (std::isspace(character)) {
            advance(text[i], i, line, column);
            continue;
        }
        if (vhdl && i + 1 < text.size() && text[i] == '-' && text[i + 1] == '-') {
            while (i < text.size() && text[i] != '\n') advance(text[i], i, line, column);
            continue;
        }
        if (!vhdl && i + 1 < text.size() && text[i] == '/' && text[i + 1] == '/') {
            while (i < text.size() && text[i] != '\n') advance(text[i], i, line, column);
            continue;
        }
        if (!vhdl && i + 1 < text.size() && text[i] == '/' && text[i + 1] == '*') {
            advance(text[i], i, line, column);
            advance(text[i], i, line, column);
            while (i < text.size()) {
                if (i + 1 < text.size() && text[i] == '*' && text[i + 1] == '/') {
                    advance(text[i], i, line, column);
                    advance(text[i], i, line, column);
                    break;
                }
                advance(text[i], i, line, column);
            }
            continue;
        }

        const auto tokenLine = line;
        const auto tokenColumn = column;
        const auto tokenOffset = i;
        if (!vhdl && text[i] == '\\') {
            advance(text[i], i, line, column);
            const auto begin = i;
            while (i < text.size() && !std::isspace(static_cast<unsigned char>(text[i]))) {
                advance(text[i], i, line, column);
            }
            tokens.push_back({TokenKind::Identifier, text.substr(begin, i - begin),
                              tokenLine, tokenColumn, tokenOffset});
            continue;
        }
        if (identifierStart(character)) {
            const auto begin = i;
            while (i < text.size() && identifierContinue(static_cast<unsigned char>(text[i]))) {
                advance(text[i], i, line, column);
            }
            tokens.push_back({TokenKind::Identifier, text.substr(begin, i - begin),
                              tokenLine, tokenColumn, tokenOffset});
            continue;
        }
        if (std::isdigit(character)) {
            const auto begin = i;
            bool basedLiteral = false;
            while (i < text.size()) {
                const auto current = static_cast<unsigned char>(text[i]);
                if (text[i] == '\'') basedLiteral = true;
                if (!(std::isalnum(current) || text[i] == '_' || text[i] == '\'' || text[i] == '.' ||
                      (text[i] == '?' && basedLiteral))) break;
                advance(text[i], i, line, column);
            }
            tokens.push_back({TokenKind::Number, text.substr(begin, i - begin),
                              tokenLine, tokenColumn, tokenOffset});
            continue;
        }
        if (text[i] == '"') {
            const auto begin = i;
            advance(text[i], i, line, column);
            while (i < text.size()) {
                if (text[i] == '"') {
                    advance(text[i], i, line, column);
                    if (vhdl && i < text.size() && text[i] == '"') {
                        advance(text[i], i, line, column);
                        continue;
                    }
                    break;
                }
                if (!vhdl && text[i] == '\\' && i + 1 < text.size()) {
                    advance(text[i], i, line, column);
                }
                advance(text[i], i, line, column);
            }
            tokens.push_back({TokenKind::String, text.substr(begin, i - begin),
                              tokenLine, tokenColumn, tokenOffset});
            continue;
        }
        tokens.push_back({TokenKind::Symbol, std::string(1, text[i]),
                          tokenLine, tokenColumn, tokenOffset});
        advance(text[i], i, line, column);
    }
    return tokens;
}

std::vector<SyntaxIssue> checkLexicalBalance(const core::SourceFile& source) {
    struct Delimiter {
        char open;
        core::SourceLocation location;
    };
    std::vector<SyntaxIssue> issues;
    std::vector<Delimiter> delimiters;
    const auto& text = source.content();
    const bool vhdl = source.language() == core::HdlLanguage::Vhdl;
    bool blockComment = false;
    bool lineComment = false;
    bool string = false;
    bool escaped = false;
    core::SourceLocation constructStart{source.path(), 1, 1, 0};
    std::size_t line = 1;
    std::size_t column = 1;
    const auto currentLocation = [&](std::size_t offset) {
        return core::SourceLocation{source.path(), line, column, offset};
    };
    const auto advance = [&](char character) {
        if (character == '\n') { ++line; column = 1; }
        else { ++column; }
    };
    for (std::size_t i = 0; i < text.size(); ++i) {
        const char character = text[i];
        const char next = i + 1 < text.size() ? text[i + 1] : '\0';
        if (lineComment) {
            if (character == '\n') lineComment = false;
            advance(character);
            continue;
        }
        if (blockComment) {
            if (character == '*' && next == '/') {
                advance(character);
                advance(next);
                ++i;
                blockComment = false;
            } else {
                advance(character);
            }
            continue;
        }
        if (string) {
            if (!vhdl && escaped) {
                escaped = false;
                advance(character);
                continue;
            }
            if (!vhdl && character == '\\') {
                escaped = true;
                advance(character);
                continue;
            }
            if (character == '"') {
                if (vhdl && next == '"') {
                    advance(character);
                    advance(next);
                    ++i;
                    continue;
                }
                string = false;
            } else if (character == '\n') {
                issues.push_back({"SYNTAX-UNTERMINATED-STRING",
                                  "String literal is not terminated before end of line",
                                  constructStart});
                string = false;
            }
            advance(character);
            continue;
        }
        if ((!vhdl && character == '/' && next == '/') ||
            (vhdl && character == '-' && next == '-')) {
            lineComment = true;
            advance(character);
            advance(next);
            ++i;
            continue;
        }
        if (!vhdl && character == '/' && next == '*') {
            constructStart = currentLocation(i);
            blockComment = true;
            advance(character);
            advance(next);
            ++i;
            continue;
        }
        if (character == '"') {
            constructStart = currentLocation(i);
            string = true;
            escaped = false;
            advance(character);
            continue;
        }
        const bool isOpen = character == '(' || (!vhdl && (character == '[' || character == '{'));
        const bool isClose = character == ')' || (!vhdl && (character == ']' || character == '}'));
        if (isOpen) {
            delimiters.push_back({character, currentLocation(i)});
        } else if (isClose) {
            const char expected = character == ')' ? '(' : character == ']' ? '[' : '{';
            if (delimiters.empty() || delimiters.back().open != expected) {
                issues.push_back({"SYNTAX-UNMATCHED-DELIMITER",
                                  std::string("Unmatched closing delimiter '") + character + "'",
                                  currentLocation(i)});
            } else {
                delimiters.pop_back();
            }
        }
        advance(character);
    }
    if (blockComment) {
        issues.push_back({"SYNTAX-UNTERMINATED-COMMENT",
                          "Block comment is not terminated before end of file", constructStart});
    }
    if (string) {
        issues.push_back({"SYNTAX-UNTERMINATED-STRING",
                          "String literal is not terminated before end of file", constructStart});
    }
    for (const auto& delimiter : delimiters) {
        issues.push_back({"SYNTAX-UNCLOSED-DELIMITER",
                          std::string("Opening delimiter '") + delimiter.open + "' is not closed",
                          delimiter.location});
    }
    return issues;
}

std::vector<SyntaxIssue> checkVerilogConstructs(const core::SourceFile& source,
                                                const std::vector<Token>& tokens) {
    struct ConstructSpec { std::string_view open; std::vector<std::string_view> closes; };
    static const std::vector<ConstructSpec> specs{
        {"module", {"endmodule"}}, {"begin", {"end"}},
        {"case", {"endcase"}}, {"casex", {"endcase"}}, {"casez", {"endcase"}},
        {"function", {"endfunction"}}, {"task", {"endtask"}},
        {"generate", {"endgenerate"}}, {"interface", {"endinterface"}},
        {"package", {"endpackage"}}, {"class", {"endclass"}},
        {"fork", {"join", "join_any", "join_none"}}
    };
    std::unordered_map<std::string, std::vector<Token>> stacks;
    std::vector<SyntaxIssue> issues;
    for (const auto& token : tokens) {
        if (token.kind != TokenKind::Identifier) continue;
        if (token.offset < source.content().size() && source.content()[token.offset] == '\\') continue;
        const auto& word = token.text; // Verilog/SV keywords are case-sensitive.
        bool handled = false;
        for (const auto& spec : specs) {
            if (word == spec.open) {
                stacks[std::string(spec.open)].push_back(token);
                handled = true;
                break;
            }
        }
        if (handled) continue;
        const ConstructSpec* fallback = nullptr;
        const ConstructSpec* matched = nullptr;
        std::size_t latestOffset = 0;
        for (const auto& spec : specs) {
            if (std::ranges::find(spec.closes, word) == spec.closes.end()) continue;
            if (!fallback) fallback = &spec;
            const auto found = stacks.find(std::string(spec.open));
            if (found == stacks.end() || found->second.empty()) continue;
            if (!matched || found->second.back().offset > latestOffset) {
                matched = &spec;
                latestOffset = found->second.back().offset;
            }
        }
        if (matched) {
            stacks[std::string(matched->open)].pop_back();
            continue;
        }
        if (fallback) {
            issues.push_back({"SYNTAX-UNEXPECTED-CLOSER",
                              "Unexpected '" + token.text + "' without matching '" +
                                  std::string(fallback->open) + "'",
                              {source.path(), token.line, token.column, token.offset}});
        }
    }
    for (const auto& spec : specs) {
        const auto found = stacks.find(std::string(spec.open));
        if (found == stacks.end()) continue;
        for (const auto& token : found->second) {
            issues.push_back({"SYNTAX-UNCLOSED-CONSTRUCT",
                              "'" + token.text + "' has no matching '" +
                                  std::string(spec.closes.front()) + "'",
                              {source.path(), token.line, token.column, token.offset}});
        }
    }
    return issues;
}

core::SourceLocation location(const core::SourceFile& source, const Token& token) {
    return {source.path(), token.line, token.column, token.offset};
}

bool isIdentifier(const Token& token) { return token.kind == TokenKind::Identifier; }

bool tokenIs(const Token& token, std::string_view value, bool caseInsensitive = true) {
    return caseInsensitive ? lower(token.text) == value : token.text == value;
}

std::optional<std::size_t> skipParenthesized(const std::vector<Token>& tokens,
                                             std::size_t open, std::size_t limit) {
    if (open >= limit || tokens[open].text != "(") return std::nullopt;
    int depth = 0;
    for (std::size_t i = open; i < limit; ++i) {
        if (tokens[i].text == "(") ++depth;
        else if (tokens[i].text == ")" && --depth == 0) return i + 1;
    }
    return std::nullopt;
}

bool verilogKeyword(std::string_view word) {
    static const std::unordered_set<std::string> words{
        "always", "always_comb", "always_ff", "always_latch", "and", "assign", "automatic",
        "begin", "buf", "bufif0", "bufif1", "case", "casex", "casez", "class", "clocking",
        "const", "deassign", "default", "defparam", "disable", "do", "else", "end", "endcase",
        "endclass", "endfunction", "endgenerate", "endmodule", "endpackage", "endtask", "enum",
        "event", "final", "for", "force", "forever", "fork", "function", "generate", "genvar",
        "if", "initial", "inout", "input", "integer", "interface", "localparam", "logic", "module",
        "nand", "negedge", "nor", "not", "notif0", "notif1", "or", "output", "package",
        "parameter", "posedge", "primitive", "program", "property", "pull0", "pull1", "pulldown",
        "pullup", "reg", "release", "repeat", "return", "sequence", "signed", "specify", "static",
        "struct", "supply0", "supply1", "task", "tran", "tranif0", "tranif1", "tri", "typedef",
        "union", "unsigned", "var", "wait", "wand", "while", "wire", "wor", "xnor", "xor"
    };
    return words.contains(std::string(word));
}

bool verilogPortDirection(const Token& token) {
    return tokenIs(token, "input", false) || tokenIs(token, "output", false) || tokenIs(token, "inout", false);
}

PortDirection portDirection(const Token& token) {
    if (tokenIs(token, "input", false)) return PortDirection::Input;
    if (tokenIs(token, "output", false)) return PortDirection::Output;
    return PortDirection::Inout;
}

void addPort(std::vector<PortInfo>& ports, const Token& token, PortDirection direction,
             bool hasDefault, std::vector<DuplicateDeclarationInfo>& duplicates,
             const core::SourceFile& source) {
    const auto existing = std::ranges::find_if(ports, [&](const PortInfo& port) {
        return port.name == token.text;
    });
    if (existing == ports.end()) {
        ports.push_back({token.text, direction, location(source, token), hasDefault});
    } else if (existing->direction == PortDirection::Unknown) {
        existing->direction = direction;
        existing->location = location(source, token);
        existing->hasDefault = hasDefault;
    } else {
        duplicates.push_back({token.text, location(source, token)});
    }
}

std::size_t appendVerilogPortDeclaration(const std::vector<Token>& tokens,
                                         std::size_t directionToken, std::size_t limit,
                                         std::vector<PortInfo>& ports,
                                         std::vector<DuplicateDeclarationInfo>& duplicates,
                                         const core::SourceFile& source) {
    const auto direction = portDirection(tokens[directionToken]);
    std::size_t segment = directionToken + 1;
    int delimiters = 0;
    const auto appendSegment = [&](std::size_t begin, std::size_t end) {
        const Token* candidate = nullptr;
        int nested = 0;
        bool hasDefault = false;
        for (std::size_t token = begin; token < end; ++token) {
            if (tokens[token].text == "[" || tokens[token].text == "(" ||
                tokens[token].text == "{") { ++nested; continue; }
            if (tokens[token].text == "]" || tokens[token].text == ")" ||
                tokens[token].text == "}") { --nested; continue; }
            if (nested == 0 && tokens[token].text == "=") {
                hasDefault = true;
                break;
            }
            if (nested == 0 && isIdentifier(tokens[token]) &&
                !verilogKeyword(tokens[token].text)) candidate = &tokens[token];
        }
        if (candidate) addPort(ports, *candidate, direction, hasDefault, duplicates, source);
    };
    for (std::size_t token = segment; token < limit; ++token) {
        if (delimiters == 0 && verilogPortDirection(tokens[token])) {
            appendSegment(segment, token);
            return token;
        }
        if (delimiters == 0 && (tokens[token].text == ";" || tokens[token].text == ")")) {
            appendSegment(segment, token);
            return token;
        }
        if (delimiters == 0 && tokens[token].text == ",") {
            appendSegment(segment, token);
            segment = token + 1;
            continue;
        }
        if (tokens[token].text == "[" || tokens[token].text == "(" ||
            tokens[token].text == "{") ++delimiters;
        else if (tokens[token].text == "]" || tokens[token].text == ")" ||
                 tokens[token].text == "}") --delimiters;
    }
    appendSegment(segment, limit);
    return limit;
}

struct ConnectionShape {
    std::size_t count{0};
    bool named{false};
    bool positional{false};
    bool positionalAfterNamed{false};
};

ConnectionShape connectionShape(const std::vector<Token>& tokens,
                                std::size_t open, std::size_t after, bool vhdl) {
    ConnectionShape shape;
    const auto close = after - 1;
    if (open + 1 >= close) return shape;
    std::size_t segment = open + 1;
    int depth = 0;
    bool seenNamed = false;
    const auto classify = [&](std::size_t begin, std::size_t end) {
        ++shape.count;
        bool named = !vhdl && begin < end && tokens[begin].text == ".";
        if (vhdl) {
            int nested = 0;
            for (std::size_t token = begin; token + 1 < end; ++token) {
                if (tokens[token].text == "(" || tokens[token].text == "[" ||
                    tokens[token].text == "{") ++nested;
                else if (tokens[token].text == ")" || tokens[token].text == "]" ||
                         tokens[token].text == "}") --nested;
                else if (nested == 0 && tokens[token].text == "=" &&
                         tokens[token + 1].text == ">") named = true;
            }
        }
        if (!named && seenNamed) shape.positionalAfterNamed = true;
        seenNamed = seenNamed || named;
        shape.named = shape.named || named;
        shape.positional = shape.positional || !named;
    };
    for (std::size_t token = segment; token <= close; ++token) {
        if (token == close || (depth == 0 && tokens[token].text == ",")) {
            classify(segment, token);
            segment = token + 1;
            continue;
        }
        if (tokens[token].text == "(" || tokens[token].text == "[" ||
            tokens[token].text == "{") ++depth;
        else if (tokens[token].text == ")" || tokens[token].text == "]" ||
                 tokens[token].text == "}") --depth;
    }
    return shape;
}

// The native 5.3.1 rule admits AST_CONSTANT, AST_IDENTIFIER and AST_PREFIX as
// port actual roots. This lexical adapter recognizes their common source
// forms and marks only bounded, definite expression shapes. Unknown forms
// remain unclassified instead of being certified as simple connections.
bool verilogPortActualExpression(const std::vector<Token>& tokens,
                                 std::size_t begin, std::size_t end) {
    while (begin < end && tokens[begin].text == "(" && tokens[end - 1].text == ")") {
        int depth = 0;
        bool wrapsAll = true;
        for (std::size_t i = begin; i < end; ++i) {
            if (tokens[i].text == "(") ++depth;
            else if (tokens[i].text == ")" && --depth == 0 && i != end - 1) {
                wrapsAll = false;
                break;
            }
        }
        if (!wrapsAll || depth != 0) break;
        ++begin;
        --end;
    }
    if (begin >= end) return false;
    if (end == begin + 1)
        return false; // one identifier, numeric literal or string literal

    if (isIdentifier(tokens[begin])) {
        std::size_t i = begin + 1;
        while (i < end) {
            if (tokens[i].text == "." && i + 1 < end && isIdentifier(tokens[i + 1])) {
                i += 2; // hierarchical identifier / AST_PREFIX
                continue;
            }
            if (tokens[i].text == "[") {
                int depth = 1;
                ++i;
                while (i < end && depth > 0) {
                    if (tokens[i].text == "[") ++depth;
                    else if (tokens[i].text == "]") --depth;
                    ++i;
                }
                if (depth != 0) return false;
                continue; // select remains rooted at AST_IDENTIFIER
            }
            break;
        }
        if (i == end) return false;
        if (tokens[i].text == "(" || tokens[i].text == "{" ||
            tokens[i].text == "?" || tokens[i].text == ":") return true;
    }
    if (tokens[begin].kind == TokenKind::Number ||
        tokens[begin].kind == TokenKind::String) return true;
    if (tokens[begin].text == "{") return true; // concatenation
    if (tokens[begin].text == "'" && begin + 2 == end &&
        (tokens[begin + 1].text == "0" || tokens[begin + 1].text == "1" ||
         tokens[begin + 1].text == "x" || tokens[begin + 1].text == "z")) return false;
    constexpr std::array<std::string_view, 15> operators{
        "+", "-", "*", "/", "%", "&", "|", "^", "~", "!", "<", ">", "=", "?", ":"};
    for (std::size_t i = begin; i < end; ++i) {
        if (std::ranges::find(operators, tokens[i].text) != operators.end()) return true;
    }
    return false;
}

void appendPositionalPortConnections(
    const std::vector<Token>& tokens, std::size_t open, std::size_t after, bool vhdl,
    std::vector<PositionalPortConnectionInfo>& connections,
    const core::SourceFile& source) {
    const auto close = after - 1;
    if (open + 1 >= close) return;
    std::size_t segment = open + 1;
    int depth = 0;
    const auto appendSegment = [&](std::size_t begin, std::size_t end) {
        bool named = !vhdl && begin < end && tokens[begin].text == ".";
        if (vhdl) {
            int nested = 0;
            for (std::size_t token = begin; token + 1 < end; ++token) {
                if (tokens[token].text == "(" || tokens[token].text == "[" ||
                    tokens[token].text == "{") ++nested;
                else if (tokens[token].text == ")" || tokens[token].text == "]" ||
                         tokens[token].text == "}") --nested;
                else if (nested == 0 && tokens[token].text == "=" &&
                         tokens[token + 1].text == ">") named = true;
            }
        }
        if (named) return;
        const bool connected = begin < end && !(vhdl && tokenIs(tokens[begin], "open"));
        const auto directIdentifier = connected && begin + 1 == end &&
            isIdentifier(tokens[begin]) ? tokens[begin].text : std::string{};
        connections.push_back({location(source, tokens[begin]), connected,
            !vhdl && connected && verilogPortActualExpression(tokens, begin, end),
            directIdentifier});
    };
    for (std::size_t token = segment; token <= close; ++token) {
        if (token == close || (depth == 0 && tokens[token].text == ",")) {
            appendSegment(segment, token);
            segment = token + 1;
            continue;
        }
        if (tokens[token].text == "(" || tokens[token].text == "[" ||
            tokens[token].text == "{") ++depth;
        else if (tokens[token].text == ")" || tokens[token].text == "]" ||
                 tokens[token].text == "}") --depth;
    }
}

void appendVerilogParameters(const std::vector<Token>& tokens, std::size_t open,
                             std::size_t after, std::vector<ParameterInfo>& parameters,
                             std::vector<DuplicateDeclarationInfo>& duplicates,
                             const core::SourceFile& source) {
    const auto close = after - 1;
    std::size_t segment = open + 1;
    int depth = 0;
    const auto appendSegment = [&](std::size_t begin, std::size_t end) {
        if (begin >= end || tokenIs(tokens[begin], "localparam", false)) return;
        const Token* candidate = nullptr;
        bool hasDefault = false;
        int nested = 0;
        for (std::size_t token = begin; token < end; ++token) {
            if (tokens[token].text == "(" || tokens[token].text == "[" ||
                tokens[token].text == "{") { ++nested; continue; }
            if (tokens[token].text == ")" || tokens[token].text == "]" ||
                tokens[token].text == "}") { --nested; continue; }
            if (nested == 0 && tokens[token].text == "=") {
                hasDefault = true;
                break;
            }
            if (nested == 0 && isIdentifier(tokens[token]) &&
                !tokenIs(tokens[token], "parameter", false) && !tokenIs(tokens[token], "type", false) &&
                !verilogKeyword(tokens[token].text)) candidate = &tokens[token];
        }
        if (!candidate) return;
        const auto exists = std::ranges::any_of(parameters, [&](const ParameterInfo& parameter) {
            return parameter.name == candidate->text;
        });
        if (!exists) {
            parameters.push_back({candidate->text, location(source, *candidate), hasDefault});
        } else {
            duplicates.push_back({candidate->text, location(source, *candidate)});
        }
    };
    for (std::size_t token = segment; token <= close; ++token) {
        if (token == close || (depth == 0 && tokens[token].text == ",")) {
            appendSegment(segment, token);
            segment = token + 1;
            continue;
        }
        if (tokens[token].text == "(" || tokens[token].text == "[" ||
            tokens[token].text == "{") ++depth;
        else if (tokens[token].text == ")" || tokens[token].text == "]" ||
                 tokens[token].text == "}") --depth;
    }
}

void appendVhdlGenerics(const std::vector<Token>& tokens, std::size_t open,
                        std::size_t after, std::vector<ParameterInfo>& parameters,
                        std::vector<DuplicateDeclarationInfo>& duplicates,
                        const core::SourceFile& source) {
    const auto close = after - 1;
    std::size_t segment = open + 1;
    int depth = 0;
    const auto appendSegment = [&](std::size_t begin, std::size_t end) {
        std::size_t colon = begin;
        for (; colon < end && tokens[colon].text != ":"; ++colon) {}
        if (colon >= end) return;
        bool hasDefault = false;
        for (std::size_t token = colon + 1; token + 1 < end; ++token) {
            if (tokens[token].text == ":" && tokens[token + 1].text == "=") {
                hasDefault = true;
                break;
            }
        }
        for (std::size_t token = begin; token < colon; ++token) {
            if (!isIdentifier(tokens[token])) continue;
            const auto exists = std::ranges::any_of(parameters,
                [&](const ParameterInfo& parameter) {
                    return lower(parameter.name) == lower(tokens[token].text);
                });
            if (!exists) {
                parameters.push_back(
                    {tokens[token].text, location(source, tokens[token]), hasDefault});
            } else {
                duplicates.push_back(
                    {tokens[token].text, location(source, tokens[token])});
            }
        }
    };
    for (std::size_t token = segment; token <= close; ++token) {
        if (token == close || (depth == 0 && tokens[token].text == ";")) {
            appendSegment(segment, token);
            segment = token + 1;
            continue;
        }
        if (tokens[token].text == "(" || tokens[token].text == "[") ++depth;
        else if (tokens[token].text == ")" || tokens[token].text == "]") --depth;
    }
}

void appendNamedParameterOverrides(const std::vector<Token>& tokens, std::size_t open,
                                   std::size_t after, bool vhdl,
                                   std::vector<NamedParameterOverrideInfo>& overrides,
                                   const core::SourceFile& source) {
    int depth = 0;
    for (std::size_t token = open + 1; token + 2 < after; ++token) {
        if (!vhdl && depth == 0 && tokens[token].text == "." &&
            isIdentifier(tokens[token + 1]) && tokens[token + 2].text == "(") {
            overrides.push_back(
                {tokens[token + 1].text, location(source, tokens[token + 1])});
            const auto afterActual = skipParenthesized(tokens, token + 2, after);
            if (afterActual) { token = *afterActual - 1; continue; }
        }
        if (vhdl && depth == 0 && isIdentifier(tokens[token]) &&
            tokens[token + 1].text == "=" && tokens[token + 2].text == ">") {
            overrides.push_back({tokens[token].text, location(source, tokens[token])});
        }
        if (tokens[token].text == "(" || tokens[token].text == "[" ||
            tokens[token].text == "{") ++depth;
        else if (tokens[token].text == ")" || tokens[token].text == "]" ||
                 tokens[token].text == "}") --depth;
    }
}

std::vector<DesignUnitInfo> parseVerilog(const core::SourceFile& source,
                                         const std::vector<Token>& tokens) {
    std::vector<DesignUnitInfo> units;
    for (std::size_t i = 0; i + 1 < tokens.size(); ++i) {
        if (!tokenIs(tokens[i], "module", false) || !isIdentifier(tokens[i + 1])) continue;
        DesignUnitInfo unit{tokens[i + 1].text, DesignUnitKind::VerilogModule,
                            location(source, tokens[i + 1]), {}, {}, {}, {}, {}, {}, true, {}, {}, {}, {}, {}};
        std::size_t body = i + 2;
        int parenthesisDepth = 0;
        for (; body < tokens.size(); ++body) {
            if (tokens[body].text == "(") ++parenthesisDepth;
            else if (tokens[body].text == ")") --parenthesisDepth;
            else if (tokens[body].text == ";" && parenthesisDepth == 0) { ++body; break; }
        }
        std::size_t end = body;
        while (end < tokens.size() && !tokenIs(tokens[end], "endmodule", false)) ++end;

        if (i + 3 < body && tokens[i + 2].text == "#" && tokens[i + 3].text == "(") {
            const auto afterParameters = skipParenthesized(tokens, i + 3, body);
            if (afterParameters) appendVerilogParameters(
                tokens, i + 3, *afterParameters, unit.parameters,
                unit.duplicateParameterDeclarations, source);
        }

        for (std::size_t token = i + 2; token < body; ++token) {
            if (!verilogPortDirection(tokens[token])) continue;
            const auto next = appendVerilogPortDeclaration(
                tokens, token, body, unit.ports, unit.duplicatePortDeclarations, source);
            token = next > token ? next - 1 : token;
        }
        bool inSubprogram = false;
        for (std::size_t token = body; token < end; ++token) {
            if (tokenIs(tokens[token], "function", false) || tokenIs(tokens[token], "task", false)) {
                inSubprogram = true;
            } else if (tokenIs(tokens[token], "endfunction", false) ||
                       tokenIs(tokens[token], "endtask", false)) {
                inSubprogram = false;
            }
            if (inSubprogram || !verilogPortDirection(tokens[token])) continue;
            if (token > body && tokens[token - 1].text != ";") continue;
            const auto next = appendVerilogPortDeclaration(
                tokens, token, end, unit.ports, unit.duplicatePortDeclarations, source);
            token = next > token ? next - 1 : token;
        }

        for (std::size_t candidate = body; candidate + 2 < end; ++candidate) {
            if (!isIdentifier(tokens[candidate]) || verilogKeyword(tokens[candidate].text)) continue;
            if (candidate > body && (tokens[candidate - 1].text == "." ||
                                     tokens[candidate - 1].text == "`")) continue;
            std::size_t instance = candidate + 1;
            std::optional<std::size_t> parameterOpen;
            std::optional<std::size_t> afterParameters;
            if (tokens[instance].text == "#") {
                parameterOpen = instance + 1;
                afterParameters = skipParenthesized(tokens, *parameterOpen, end);
                if (!afterParameters) continue;
                instance = *afterParameters;
            }
            if (instance + 1 >= end || !isIdentifier(tokens[instance])) continue;
            std::size_t connection = instance + 1;
            if (connection < end && tokens[connection].text == "[") {
                int bracketDepth = 0;
                do {
                    if (tokens[connection].text == "[") ++bracketDepth;
                    else if (tokens[connection].text == "]") --bracketDepth;
                    ++connection;
                } while (connection < end && bracketDepth > 0);
            }
            if (connection >= end || tokens[connection].text != "(") continue;
            const auto afterConnections = skipParenthesized(tokens, connection, end);
            InstanceInfo parsed{tokens[instance].text, tokens[candidate].text,
                                location(source, tokens[candidate]), false, false, {}, {},
                                0, false, false, false, false, {}, 0, false, false,
                                false, false};
            if (parameterOpen && afterParameters) {
                const auto shape = connectionShape(
                    tokens, *parameterOpen, *afterParameters, false);
                parsed.parameterOverrideCount = shape.count;
                parsed.hasNamedParameterOverrides = shape.named;
                parsed.hasPositionalParameterOverrides = shape.positional;
                parsed.invalidParameterAssociationOrder = shape.positionalAfterNamed;
                appendNamedParameterOverrides(tokens, *parameterOpen, *afterParameters,
                                              false, parsed.namedParameterOverrides, source);
            }
            if (afterConnections) {
                const auto shape = connectionShape(tokens, connection, *afterConnections, false);
                parsed.portConnectionCount = shape.count;
                parsed.hasPortConnectionList = true;
                parsed.hasNamedConnections = shape.named;
                parsed.hasPositionalConnections = shape.positional;
                parsed.invalidPortAssociationOrder = shape.positionalAfterNamed;
                appendPositionalPortConnections(tokens, connection, *afterConnections, false,
                                                parsed.positionalConnections, source);
                int depth = 0;
                for (std::size_t token = connection + 1; token + 1 < *afterConnections; ++token) {
                    if (depth == 0 && tokens[token].text == "." &&
                        tokens[token + 1].text == "*") {
                        parsed.hasWildcardConnection = true;
                    }
                    if (depth == 0 && tokens[token].text == "." &&
                        isIdentifier(tokens[token + 1]) && token + 2 < *afterConnections &&
                        tokens[token + 2].text == "(") {
                        const auto afterActual = skipParenthesized(tokens, token + 2,
                                                                   *afterConnections);
                        if (afterActual) {
                            const bool actualConnected = token + 3 < *afterActual - 1;
                            parsed.namedConnections.push_back(
                                {tokens[token + 1].text,
                                 location(source, tokens[token + 1]), actualConnected,
                                 actualConnected && verilogPortActualExpression(
                                     tokens, token + 3, *afterActual - 1),
                                 actualConnected ? location(source, tokens[token + 3]) :
                                     core::SourceLocation{},
                                 actualConnected && token + 4 == *afterActual - 1 &&
                                     isIdentifier(tokens[token + 3]) ?
                                     tokens[token + 3].text : std::string{}});
                            token = *afterActual - 1;
                            continue;
                        }
                    }
                    if (depth == 0 && tokens[token].text == "." &&
                        isIdentifier(tokens[token + 1]) && token + 2 < *afterConnections &&
                        (token + 2 == *afterConnections - 1 ||
                         tokens[token + 2].text == ",")) {
                        parsed.namedConnections.push_back(
                            {tokens[token + 1].text,
                             location(source, tokens[token + 1]), true, false,
                             location(source, tokens[token + 1]), tokens[token + 1].text});
                        ++token;
                        continue;
                    }
                    if (tokens[token].text == "(" || tokens[token].text == "[" ||
                        tokens[token].text == "{") ++depth;
                    else if (tokens[token].text == ")" || tokens[token].text == "]" ||
                             tokens[token].text == "}") --depth;
                }
                candidate = *afterConnections - 1;
            }
            unit.instances.push_back(std::move(parsed));
        }
        units.push_back(std::move(unit));
        i = end;
    }
    return units;
}

bool vhdlNonInstanceTarget(const Token& token) {
    static const std::unordered_set<std::string> excluded{
        "process", "block", "for", "if", "case", "assert", "report", "entity", "configuration"
    };
    return excluded.contains(lower(token.text));
}

PortDirection vhdlPortDirection(const std::vector<Token>& tokens,
                                std::size_t begin, std::size_t end) {
    for (std::size_t token = begin; token < end; ++token) {
        if (tokenIs(tokens[token], "in")) return PortDirection::Input;
        if (tokenIs(tokens[token], "out") || tokenIs(tokens[token], "buffer")) {
            return PortDirection::Output;
        }
        if (tokenIs(tokens[token], "inout")) return PortDirection::Inout;
    }
    return PortDirection::Unknown;
}

void appendVhdlPorts(const std::vector<Token>& tokens, std::size_t open,
                     std::size_t after, std::vector<PortInfo>& ports,
                     std::vector<DuplicateDeclarationInfo>& duplicates,
                     const core::SourceFile& source) {
    const auto close = after - 1;
    std::size_t segment = open + 1;
    int depth = 0;
    const auto appendSegment = [&](std::size_t begin, std::size_t end) {
        std::size_t colon = begin;
        int nested = 0;
        for (; colon < end; ++colon) {
            if (tokens[colon].text == "(" || tokens[colon].text == "[") ++nested;
            else if (tokens[colon].text == ")" || tokens[colon].text == "]") --nested;
            else if (nested == 0 && tokens[colon].text == ":") break;
        }
        if (colon >= end) return;
        const auto direction = vhdlPortDirection(tokens, colon + 1, end);
        bool hasDefault = false;
        for (std::size_t token = colon + 1; token + 1 < end; ++token) {
            if (tokens[token].text == ":" && tokens[token + 1].text == "=") {
                hasDefault = true;
                break;
            }
        }
        for (std::size_t token = begin; token < colon; ++token) {
            if (!isIdentifier(tokens[token])) continue;
            const auto existing = std::ranges::find_if(ports, [&](const PortInfo& port) {
                return lower(port.name) == lower(tokens[token].text);
            });
            if (existing == ports.end()) {
                ports.push_back({tokens[token].text, direction, location(source, tokens[token]),
                                 hasDefault});
            } else {
                duplicates.push_back({tokens[token].text, location(source, tokens[token])});
            }
        }
    };
    for (std::size_t token = segment; token <= close; ++token) {
        if (token == close || (depth == 0 && tokens[token].text == ";")) {
            appendSegment(segment, token);
            segment = token + 1;
            continue;
        }
        if (tokens[token].text == "(" || tokens[token].text == "[") ++depth;
        else if (tokens[token].text == ")" || tokens[token].text == "]") --depth;
    }
}

std::vector<DesignUnitInfo> parseVhdl(const core::SourceFile& source,
                                      const std::vector<Token>& tokens) {
    std::vector<DesignUnitInfo> units;
    for (std::size_t i = 0; i + 2 < tokens.size(); ++i) {
        if (tokenIs(tokens[i], "entity") && isIdentifier(tokens[i + 1]) &&
            tokenIs(tokens[i + 2], "is")) {
            DesignUnitInfo unit{tokens[i + 1].text, DesignUnitKind::VhdlEntity,
                                location(source, tokens[i + 1]), {}, {}, {}, {}, {}, {}, true, {}, {}, {}, {}, {}};
            std::size_t entityEnd = i + 3;
            while (entityEnd + 1 < tokens.size() &&
                   !(tokenIs(tokens[entityEnd], "end") &&
                     (tokenIs(tokens[entityEnd + 1], "entity") ||
                      tokenIs(tokens[entityEnd + 1], tokens[i + 1].text) ||
                      tokens[entityEnd + 1].text == ";"))) ++entityEnd;
            for (std::size_t token = i + 3; token + 1 < entityEnd; ++token) {
                if ((!tokenIs(tokens[token], "port") && !tokenIs(tokens[token], "generic")) ||
                    tokens[token + 1].text != "(") continue;
                const auto after = skipParenthesized(tokens, token + 1, entityEnd);
                if (!after) continue;
                if (tokenIs(tokens[token], "port")) {
                    appendVhdlPorts(tokens, token + 1, *after, unit.ports,
                                    unit.duplicatePortDeclarations, source);
                } else {
                    appendVhdlGenerics(tokens, token + 1, *after, unit.parameters,
                                       unit.duplicateParameterDeclarations, source);
                }
                token = *after - 1;
            }
            units.push_back(std::move(unit));
        }
    }
    for (std::size_t i = 0; i + 4 < tokens.size(); ++i) {
        if (!tokenIs(tokens[i], "architecture") || !isIdentifier(tokens[i + 1]) ||
            !tokenIs(tokens[i + 2], "of") || !isIdentifier(tokens[i + 3])) continue;
        const auto ownerName = lower(tokens[i + 3].text);
        auto owner = std::ranges::find_if(units, [&](const DesignUnitInfo& unit) {
            return lower(unit.name) == ownerName;
        });
        if (owner == units.end()) continue;
        std::size_t body = i + 4;
        while (body < tokens.size() && !tokenIs(tokens[body], "begin")) ++body;
        std::size_t end = body;
        while (end + 1 < tokens.size()) {
            if (tokenIs(tokens[end], "end") &&
                (tokenIs(tokens[end + 1], "architecture") ||
                 tokenIs(tokens[end + 1], tokens[i + 1].text))) break;
            ++end;
        }
        // Preserve the high-confidence subset needed by hierarchy-aware clock
        // path checks.  Do not fold process, generate or block contents into
        // architecture-level wiring: those nodes have separate native
        // ownership and may represent sequential rather than structural paths.
        unsigned nestedStatements = 0;
        for (std::size_t token = body + 1; token + 2 < end; ++token) {
            if (tokenIs(tokens[token], "end") && token + 1 < end &&
                (tokenIs(tokens[token + 1], "process") ||
                 tokenIs(tokens[token + 1], "generate") ||
                 tokenIs(tokens[token + 1], "block"))) {
                if (nestedStatements != 0) --nestedStatements;
                ++token;
                continue;
            }
            if ((tokenIs(tokens[token], "process") ||
                 tokenIs(tokens[token], "generate") ||
                 tokenIs(tokens[token], "block")) &&
                (token == body + 1 || !tokenIs(tokens[token - 1], "end"))) {
                ++nestedStatements;
                continue;
            }
            if (nestedStatements != 0 || !isIdentifier(tokens[token])) continue;
            std::size_t rhs = token + 1;
            if (tokens[rhs].text == "<=") {
                ++rhs;
            } else if (rhs + 1 < end && tokens[rhs].text == "<" &&
                       tokens[rhs + 1].text == "=") {
                rhs += 2;
            } else {
                continue;
            }
            if (rhs + 1 < end && isIdentifier(tokens[rhs]) &&
                tokens[rhs + 1].text == ";") {
                owner->simpleSignalTransfers.push_back(
                    {tokens[token].text, tokens[rhs].text, false,
                     location(source, tokens[token])});
            } else if (rhs + 2 < end && tokenIs(tokens[rhs], "not") &&
                       isIdentifier(tokens[rhs + 1]) &&
                       tokens[rhs + 2].text == ";") {
                owner->simpleSignalTransfers.push_back(
                    {tokens[token].text, tokens[rhs + 1].text, true,
                     location(source, tokens[token])});
            }
        }
        for (std::size_t label = body; label + 2 < end; ++label) {
            if (!isIdentifier(tokens[label]) || tokens[label + 1].text != ":") continue;
            std::size_t target = label + 2;
            if (tokenIs(tokens[target], "entity")) {
                if (++target >= end || !isIdentifier(tokens[target])) continue;
                if (target + 2 < end && tokens[target + 1].text == "." &&
                    isIdentifier(tokens[target + 2])) target += 2;
            }
            if (target >= end || !isIdentifier(tokens[target]) || vhdlNonInstanceTarget(tokens[target])) continue;
            bool hasMap = false;
            std::optional<std::size_t> portMapOpen;
            std::optional<std::size_t> genericMapOpen;
            for (std::size_t look = target + 1; look + 1 < end; ++look) {
                if ((tokenIs(tokens[look], "port") || tokenIs(tokens[look], "generic")) &&
                    tokenIs(tokens[look + 1], "map")) {
                    hasMap = true;
                    if (tokenIs(tokens[look], "port") && look + 2 < end &&
                        tokens[look + 2].text == "(") portMapOpen = look + 2;
                    if (tokenIs(tokens[look], "generic") && look + 2 < end &&
                        tokens[look + 2].text == "(") genericMapOpen = look + 2;
                }
                if (tokens[look].text == ";") break;
            }
            if (!hasMap) continue;
            InstanceInfo parsed{tokens[label].text, tokens[target].text,
                                location(source, tokens[target]), false, false, {}, {},
                                0, false, false, false, false, {}, 0, false, false,
                                false, false};
            if (genericMapOpen) {
                const auto afterMap = skipParenthesized(tokens, *genericMapOpen, end);
                if (afterMap) {
                    const auto shape = connectionShape(tokens, *genericMapOpen, *afterMap, true);
                    parsed.parameterOverrideCount = shape.count;
                    parsed.hasNamedParameterOverrides = shape.named;
                    parsed.hasPositionalParameterOverrides = shape.positional;
                    parsed.invalidParameterAssociationOrder = shape.positionalAfterNamed;
                    appendNamedParameterOverrides(tokens, *genericMapOpen, *afterMap, true,
                                                  parsed.namedParameterOverrides, source);
                }
            }
            if (portMapOpen) {
                const auto afterMap = skipParenthesized(tokens, *portMapOpen, end);
                if (afterMap) {
                    const auto shape = connectionShape(tokens, *portMapOpen, *afterMap, true);
                    parsed.portConnectionCount = shape.count;
                    parsed.hasPortConnectionList = true;
                    parsed.hasNamedConnections = shape.named;
                    parsed.hasPositionalConnections = shape.positional;
                    parsed.invalidPortAssociationOrder = shape.positionalAfterNamed;
                    appendPositionalPortConnections(tokens, *portMapOpen, *afterMap, true,
                                                    parsed.positionalConnections, source);
                    int depth = 0;
                    for (std::size_t token = *portMapOpen + 1;
                         token + 2 < *afterMap; ++token) {
                        if (depth == 0 && isIdentifier(tokens[token]) &&
                            tokens[token + 1].text == "=" && tokens[token + 2].text == ">") {
                            const bool actualConnected = token + 3 < *afterMap - 1 &&
                                tokens[token + 3].text != "," &&
                                !tokenIs(tokens[token + 3], "open");
                            const bool directIdentifier = actualConnected &&
                                isIdentifier(tokens[token + 3]) &&
                                (token + 4 == *afterMap - 1 ||
                                 tokens[token + 4].text == ",");
                            parsed.namedConnections.push_back(
                                {tokens[token].text, location(source, tokens[token]),
                                 actualConnected, false,
                                 actualConnected ? location(source, tokens[token + 3]) :
                                     core::SourceLocation{},
                                 directIdentifier ? tokens[token + 3].text : std::string{}});
                        }
                        if (tokens[token].text == "(" || tokens[token].text == "[") ++depth;
                        else if (tokens[token].text == ")" || tokens[token].text == "]") --depth;
                    }
                }
            }
            owner->instances.push_back(std::move(parsed));
        }
        i = end;
    }
    return units;
}

} // namespace

std::vector<DesignUnitInfo> parseDesignUnits(const core::SourceFile& source) {
    const auto tokens = lex(source);
    if (source.language() == core::HdlLanguage::Vhdl) return parseVhdl(source, tokens);
    if (source.language() == core::HdlLanguage::Verilog ||
        source.language() == core::HdlLanguage::SystemVerilog) return parseVerilog(source, tokens);
    return {};
}

std::vector<HdlToken> tokenize(const core::SourceFile& source) {
    return lex(source);
}

std::vector<SyntaxIssue> checkSyntax(const core::SourceFile& source) {
    auto issues = checkLexicalBalance(source);
    if (source.language() == core::HdlLanguage::Verilog ||
        source.language() == core::HdlLanguage::SystemVerilog) {
        auto structural = checkVerilogConstructs(source, lex(source));
        issues.insert(issues.end(), std::make_move_iterator(structural.begin()),
                      std::make_move_iterator(structural.end()));
    }
    std::ranges::sort(issues, [](const SyntaxIssue& left, const SyntaxIssue& right) {
        return std::tie(left.location.line, left.location.column, left.ruleId) <
               std::tie(right.location.line, right.location.column, right.ruleId);
    });
    return issues;
}

std::string_view designUnitKindName(DesignUnitKind kind) noexcept {
    switch (kind) {
    case DesignUnitKind::VerilogModule: return "module";
    case DesignUnitKind::VhdlEntity: return "entity";
    }
    return "unknown";
}

std::string_view portDirectionName(PortDirection direction) noexcept {
    switch (direction) {
    case PortDirection::Input: return "input";
    case PortDirection::Output: return "output";
    case PortDirection::Inout: return "inout";
    case PortDirection::Unknown: return "unknown";
    }
    return "unknown";
}

} // namespace fpga::parser
