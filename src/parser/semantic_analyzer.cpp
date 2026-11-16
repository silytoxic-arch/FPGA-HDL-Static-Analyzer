#include "parser/semantic_analyzer.h"

#include "parser/design_parser.h"

#include <algorithm>
#include <charconv>
#include <cctype>
#include <cstdint>
#include <functional>
#include <limits>
#include <map>
#include <optional>
#include <set>
#include <sstream>
#include <string_view>
#include <unordered_set>

namespace fpga::parser {
namespace {

std::string lower(std::string value) {
    std::ranges::transform(value, value.begin(),
                           [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return value;
}

bool identifier(const HdlToken& token) { return token.kind == HdlTokenKind::Identifier; }
bool tokenIs(const HdlToken& token, std::string_view value) {
    return lower(token.text) == value;
}

bool equalInsensitive(std::string_view left, std::string_view right) {
    return lower(std::string(left)) == lower(std::string(right));
}

bool resetLikeName(std::string_view name) {
    const auto folded = lower(std::string(name));
    return folded.find("reset") != std::string::npos || folded.find("rst") != std::string::npos;
}

bool asynchronousControlLikeName(std::string_view name) {
    const auto folded = lower(std::string(name));
    return resetLikeName(name) || folded.find("clear") != std::string::npos ||
           folded.find("clr") != std::string::npos ||
           folded.find("preset") != std::string::npos;
}

core::SourceLocation location(const core::SourceFile& source, const HdlToken& token) {
    return {source.path(), token.line, token.column, token.offset};
}

enum class Direction { Internal, Input, Output, Inout };
enum class VerilogSignedness { Unknown, Unsigned, Signed };

struct Driver {
    int source;
    core::SourceLocation location;
    bool initialization{false};
};

struct ConstantRange {
    std::size_t low;
    std::size_t high;
};

struct SignalState {
    Direction direction{Direction::Internal};
    core::SourceLocation declaration;
    std::vector<Driver> drivers;
    bool memory{false};
    bool read{false};
    bool connected{false};
    std::optional<std::size_t> width{1};
    std::optional<ConstantRange> range;
    bool vhdlUnsignedNumeric{false};
    VerilogSignedness verilogSignedness{VerilogSignedness::Unknown};
    std::optional<core::SourceLocation> firstRead;
    // Verilog unpacked dimensions, in source order after the declared name.
    // Keep these separate from range, which describes the packed dimension.
    std::vector<ConstantRange> unpackedRanges;
};

struct VerilogModuleInterface {
    std::map<std::string, Direction> directions;
    std::vector<std::string> portOrder;
};

struct CaseSelectorShape {
    std::size_t width;
    std::string display;
};

using VhdlStaticIntegerMap = std::map<std::string, std::int64_t>;

struct ProcessRange {
    std::size_t begin;
    std::size_t end;
    int driverId;
    bool initialization{false};
    std::size_t keyword{0};
};

std::optional<std::size_t> skipBalanced(const std::vector<HdlToken>& tokens,
                                        std::size_t open, std::size_t limit,
                                        std::string_view left, std::string_view right);
std::optional<std::size_t> assignmentOperator(const std::vector<HdlToken>& tokens,
                                              std::size_t target, std::size_t limit);
std::optional<std::size_t> verilogAssignmentOperator(
    const std::vector<HdlToken>& tokens, std::size_t target,
    std::size_t statementBegin, std::size_t limit);
std::optional<std::size_t> vhdlAssignmentOperator(const std::vector<HdlToken>& tokens,
                                                  std::size_t target, std::size_t limit);
std::optional<std::size_t> verilogEmptyBranchToken(
    const std::vector<HdlToken>& tokens, std::size_t statement,
    std::size_t limit);
std::optional<std::size_t> simpleIdentifierToken(
    const std::vector<HdlToken>& tokens, std::size_t begin, std::size_t end);
std::optional<CaseSelectorShape> comparisonOperandShape(
    const std::vector<HdlToken>& tokens, std::size_t begin, std::size_t end,
    const std::map<std::string, SignalState>& signals, bool vhdl);
std::optional<CaseSelectorShape> vhdlNativeConversionShape(
    const std::vector<HdlToken>& tokens, std::size_t begin, std::size_t end,
    const std::map<std::string, SignalState>& signals,
    const VhdlStaticIntegerMap* staticIntegerValues = nullptr);
std::optional<CaseSelectorShape> verilogCaseSelectorShape(
    const std::vector<HdlToken>& tokens, std::size_t begin, std::size_t end,
    const std::map<std::string, SignalState>& signals);
bool verilogNativeCarryExemptConcat(
    const std::vector<HdlToken>& tokens, std::size_t begin, std::size_t end,
    const std::map<std::string, SignalState>& signals);

std::string joinedNames(const std::set<std::string>& names) {
    std::ostringstream output;
    bool first = true;
    for (const auto& name : names) {
        if (!first) output << ", ";
        output << name;
        first = false;
    }
    return output.str();
}

struct AssignmentFlow {
    std::set<std::string> maybe;
    std::set<std::string> definite;
    std::size_t next{0};
};

void unionInto(std::set<std::string>& target, const std::set<std::string>& source) {
    target.insert(source.begin(), source.end());
}

std::set<std::string> intersectionOf(const std::set<std::string>& left,
                                     const std::set<std::string>& right) {
    std::set<std::string> result;
    std::ranges::set_intersection(left, right, std::inserter(result, result.end()));
    return result;
}

AssignmentFlow parseVerilogFlow(const std::vector<HdlToken>& tokens, std::size_t position,
                                std::size_t limit, const std::set<std::string>& known);

AssignmentFlow parseVerilogBlock(const std::vector<HdlToken>& tokens, std::size_t position,
                                 std::size_t limit, const std::set<std::string>& known) {
    AssignmentFlow result{{}, {}, position};
    while (result.next < limit && !tokenIs(tokens[result.next], "end")) {
        const auto child = parseVerilogFlow(tokens, result.next, limit, known);
        unionInto(result.maybe, child.maybe);
        unionInto(result.definite, child.definite);
        if (child.next <= result.next) ++result.next;
        else result.next = child.next;
    }
    if (result.next < limit && tokenIs(tokens[result.next], "end")) ++result.next;
    if (result.next + 1 < limit && tokens[result.next].text == ":" &&
        identifier(tokens[result.next + 1])) result.next += 2;
    return result;
}

AssignmentFlow parseVerilogCase(const std::vector<HdlToken>& tokens, std::size_t position,
                                std::size_t limit, const std::set<std::string>& known) {
    std::size_t cursor = position + 1;
    if (cursor < limit && tokens[cursor].text == "(") {
        const auto after = skipBalanced(tokens, cursor, limit, "(", ")");
        if (!after) return {{}, {}, position + 1};
        cursor = *after;
    }
    std::vector<AssignmentFlow> branches;
    bool hasDefault = false;
    while (cursor < limit && !tokenIs(tokens[cursor], "endcase")) {
        const auto labelBegin = cursor;
        int nesting = 0;
        int ternary = 0;
        while (cursor < limit) {
            if (tokens[cursor].text == "(" || tokens[cursor].text == "[" ||
                tokens[cursor].text == "{") ++nesting;
            else if (tokens[cursor].text == ")" || tokens[cursor].text == "]" ||
                     tokens[cursor].text == "}") --nesting;
            else if (tokens[cursor].text == "?" && nesting == 0) ++ternary;
            else if (tokens[cursor].text == ":" && nesting == 0) {
                if (ternary > 0) --ternary;
                else break;
            }
            ++cursor;
        }
        if (cursor >= limit || tokenIs(tokens[cursor], "endcase")) break;
        for (std::size_t i = labelBegin; i < cursor; ++i) {
            if (tokenIs(tokens[i], "default")) hasDefault = true;
        }
        auto branch = parseVerilogFlow(tokens, cursor + 1, limit, known);
        branches.push_back(branch);
        cursor = branch.next > cursor ? branch.next : cursor + 1;
    }

    AssignmentFlow result{{}, {}, cursor < limit ? cursor + 1 : cursor};
    for (const auto& branch : branches) unionInto(result.maybe, branch.maybe);
    if (hasDefault && !branches.empty()) {
        result.definite = branches.front().definite;
        for (std::size_t i = 1; i < branches.size(); ++i) {
            result.definite = intersectionOf(result.definite, branches[i].definite);
        }
    }
    return result;
}

AssignmentFlow parseVerilogFlow(const std::vector<HdlToken>& tokens, std::size_t position,
                                std::size_t limit, const std::set<std::string>& known) {
    if (position >= limit) return {{}, {}, position};
    if (tokens[position].text == ";") return {{}, {}, position + 1};
    if ((tokenIs(tokens[position], "unique") || tokenIs(tokens[position], "unique0") ||
         tokenIs(tokens[position], "priority")) && position + 1 < limit &&
        (tokenIs(tokens[position + 1], "if") || tokenIs(tokens[position + 1], "case") ||
         tokenIs(tokens[position + 1], "casez") || tokenIs(tokens[position + 1], "casex"))) {
        return parseVerilogFlow(tokens, position + 1, limit, known);
    }
    if (tokenIs(tokens[position], "begin")) {
        std::size_t body = position + 1;
        if (body + 1 < limit && tokens[body].text == ":" && identifier(tokens[body + 1])) {
            body += 2;
        }
        return parseVerilogBlock(tokens, body, limit, known);
    }
    if (tokenIs(tokens[position], "if")) {
        std::size_t statement = position + 1;
        if (statement < limit && tokens[statement].text == "(") {
            const auto after = skipBalanced(tokens, statement, limit, "(", ")");
            if (!after) return {{}, {}, position + 1};
            statement = *after;
        }
        auto thenFlow = parseVerilogFlow(tokens, statement, limit, known);
        AssignmentFlow result{thenFlow.maybe, {}, thenFlow.next};
        if (result.next < limit && tokenIs(tokens[result.next], "else")) {
            auto elseFlow = parseVerilogFlow(tokens, result.next + 1, limit, known);
            unionInto(result.maybe, elseFlow.maybe);
            result.definite = intersectionOf(thenFlow.definite, elseFlow.definite);
            result.next = elseFlow.next;
        }
        return result;
    }
    if (tokenIs(tokens[position], "case") || tokenIs(tokens[position], "casez") ||
        tokenIs(tokens[position], "casex")) return parseVerilogCase(tokens, position, limit, known);
    if (tokenIs(tokens[position], "for") || tokenIs(tokens[position], "while") ||
        tokenIs(tokens[position], "repeat")) {
        std::size_t statement = position + 1;
        if (statement < limit && tokens[statement].text == "(") {
            const auto after = skipBalanced(tokens, statement, limit, "(", ")");
            if (!after) return {{}, {}, position + 1};
            statement = *after;
        }
        auto body = parseVerilogFlow(tokens, statement, limit, known);
        body.definite.clear();
        return body;
    }

    AssignmentFlow result{{}, {}, position};
    while (result.next < limit && tokens[result.next].text != ";" &&
           !tokenIs(tokens[result.next], "end") && !tokenIs(tokens[result.next], "endcase")) {
        if (identifier(tokens[result.next]) && known.contains(tokens[result.next].text) &&
            verilogAssignmentOperator(tokens, result.next, position, limit)) {
            result.maybe.insert(tokens[result.next].text);
            result.definite.insert(tokens[result.next].text);
        }
        ++result.next;
    }
    if (result.next < limit && tokens[result.next].text == ";") ++result.next;
    return result;
}

bool typeKeyword(const HdlToken& token) {
    static const std::unordered_set<std::string> words{
        "wire", "reg", "logic", "integer", "var", "bit", "byte", "shortint", "int",
        "longint", "time", "signed", "unsigned", "tri", "wand", "wor", "supply0", "supply1"
    };
    return words.contains(lower(token.text));
}

bool directionKeyword(const HdlToken& token) {
    return tokenIs(token, "input") || tokenIs(token, "output") || tokenIs(token, "inout");
}

Direction directionOf(const HdlToken& token) {
    if (tokenIs(token, "input")) return Direction::Input;
    if (tokenIs(token, "output")) return Direction::Output;
    return Direction::Inout;
}

std::vector<const HdlToken*> declarationNames(const std::vector<HdlToken>& tokens,
                                              std::size_t begin, std::size_t limit,
                                              bool stopAtDirection) {
    std::vector<const HdlToken*> names;
    const HdlToken* candidate = nullptr;
    int brackets = 0;
    bool initializer = false;
    for (std::size_t i = begin; i < limit; ++i) {
        const auto& token = tokens[i];
        if (token.text == "[") { ++brackets; continue; }
        if (token.text == "]") { if (brackets > 0) --brackets; continue; }
        if (brackets > 0) continue;
        if (stopAtDirection && directionKeyword(token)) {
            if (candidate) names.push_back(candidate);
            break;
        }
        if (token.text == ";" || token.text == ")") {
            if (candidate) names.push_back(candidate);
            break;
        }
        if (token.text == ",") {
            if (candidate) names.push_back(candidate);
            candidate = nullptr;
            initializer = false;
            continue;
        }
        if (token.text == "=") { initializer = true; continue; }
        if (!initializer && identifier(token) && !typeKeyword(token)) candidate = &token;
    }
    return names;
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

std::optional<std::size_t> decimalValue(std::string_view text) {
    std::size_t value = 0;
    const auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), value);
    if (error != std::errc{} || end != text.data() + text.size()) return std::nullopt;
    return value;
}

std::optional<std::size_t> verilogIntegerValue(std::string text) {
    text = lower(std::move(text));
    text.erase(std::remove(text.begin(), text.end(), '_'), text.end());
    const auto apostrophe = text.find('\'');
    if (apostrophe == std::string::npos) return decimalValue(text);
    std::size_t basePosition = apostrophe + 1;
    if (basePosition < text.size() && text[basePosition] == 's') ++basePosition;
    if (basePosition >= text.size()) return std::nullopt;
    int base = 0;
    switch (text[basePosition++]) {
    case 'b': base = 2; break;
    case 'o': base = 8; break;
    case 'd': base = 10; break;
    case 'h': base = 16; break;
    default: return std::nullopt;
    }
    if (basePosition >= text.size()) return std::nullopt;
    std::size_t value = 0;
    const auto [end, error] = std::from_chars(
        text.data() + basePosition, text.data() + text.size(), value, base);
    if (error != std::errc{} || end != text.data() + text.size()) return std::nullopt;
    return value;
}

std::optional<std::size_t> verilogLiteralDeclaredWidth(std::string_view text) {
    const auto apostrophe = text.find('\'');
    if (apostrophe == std::string_view::npos || apostrophe == 0) return std::nullopt;
    const auto width = decimalValue(text.substr(0, apostrophe));
    return width && *width > 0 ? width : std::nullopt;
}

std::optional<std::size_t> vhdlStringLiteralWidth(const HdlToken& token) {
    if (token.kind != HdlTokenKind::String || token.text.size() < 2 ||
        token.text.front() != '"' || token.text.back() != '"') return std::nullopt;
    const auto width = token.text.size() - 2;
    return width > 0 ? std::optional<std::size_t>(width) : std::nullopt;
}

std::optional<std::size_t> vhdlLogicCharacterLiteralWidth(
    const std::vector<HdlToken>& tokens, std::size_t begin, std::size_t end) {
    if (begin >= end || tokens[begin].text != "'") return std::nullopt;
    std::string value;
    if (begin + 3 == end && tokens[begin + 2].text == "'") {
        value = lower(tokens[begin + 1].text);
    } else if (begin + 2 == end && tokens[begin + 1].text.size() == 2 &&
               tokens[begin + 1].text.back() == '\'') {
        value = lower(tokens[begin + 1].text.substr(0, 1));
    } else {
        return std::nullopt;
    }
    static const std::unordered_set<std::string> logicValues{
        "u", "x", "0", "1", "z", "w", "l", "h", "-"};
    return logicValues.contains(value) ? std::optional<std::size_t>(1)
                                       : std::nullopt;
}

std::optional<ConstantRange> constantRangeBounds(
    const std::vector<HdlToken>& tokens, std::size_t open, std::size_t close, bool vhdl) {
    if (open + 4 >= close) return std::nullopt;
    const auto left = decimalValue(tokens[open + 1].text);
    const auto right = decimalValue(tokens[open + 3].text);
    const bool separator = vhdl ?
        (tokenIs(tokens[open + 2], "downto") || tokenIs(tokens[open + 2], "to")) :
        tokens[open + 2].text == ":";
    if (!left || !right || !separator || tokens[open + 4].text != (vhdl ? ")" : "]")) {
        return std::nullopt;
    }
    return ConstantRange{std::min(*left, *right), std::max(*left, *right)};
}

std::optional<std::size_t> constantRangeWidth(const std::vector<HdlToken>& tokens,
                                              std::size_t open, std::size_t close,
                                              bool vhdl) {
    const auto range = constantRangeBounds(tokens, open, close, vhdl);
    return range ? std::optional<std::size_t>(range->high - range->low + 1) : std::nullopt;
}

// A deliberately bounded default-parameter expression model. Unsupported
// arithmetic and overridden/elaborated parameter values remain unknown.
std::optional<std::size_t> verilogDefaultInteger(
    const std::vector<HdlToken>& tokens, std::size_t begin, std::size_t end,
    const std::map<std::string, std::size_t>& defaults) {
    constexpr std::size_t widthLimit = 1U << 24;
    const auto atom = [&](std::size_t index) -> std::optional<std::size_t> {
        if (index >= end) return std::nullopt;
        if (tokens[index].kind == HdlTokenKind::Number)
            return verilogIntegerValue(tokens[index].text);
        if (tokens[index].kind == HdlTokenKind::Identifier) {
            const auto found = defaults.find(tokens[index].text);
            if (found != defaults.end()) return found->second;
        }
        return std::nullopt;
    };
    if (begin >= end || ((end - begin) % 2) == 0) return std::nullopt;
    auto value = atom(begin);
    if (!value || *value > widthLimit) return std::nullopt;
    for (auto index = begin + 1; index < end; index += 2) {
        const auto next = atom(index + 1);
        if (!next || *next > widthLimit) return std::nullopt;
        if (tokens[index].text == "+") {
            if (*value > widthLimit - *next) return std::nullopt;
            *value += *next;
        } else if (tokens[index].text == "-") {
            if (*value < *next) return std::nullopt;
            *value -= *next;
        } else return std::nullopt;
    }
    return value;
}

std::optional<ConstantRange> verilogDeclarationRangeBounds(
    const std::vector<HdlToken>& tokens, std::size_t open, std::size_t after,
    const std::map<std::string, std::size_t>& defaults) {
    if (after <= open + 3 || tokens[after - 1].text != "]") return std::nullopt;
    std::optional<std::size_t> colon;
    for (auto index = open + 1; index + 1 < after; ++index)
        if (tokens[index].text == ":") {
            if (colon) return std::nullopt;
            colon = index;
        }
    if (!colon) return std::nullopt;
    const auto left = verilogDefaultInteger(tokens, open + 1, *colon, defaults);
    const auto right = verilogDefaultInteger(tokens, *colon + 1, after - 1, defaults);
    if (!left || !right) return std::nullopt;
    return ConstantRange{std::min(*left, *right), std::max(*left, *right)};
}

std::optional<ConstantRange> verilogDeclarationRange(
    const std::vector<HdlToken>& tokens, std::size_t declaration, std::size_t name,
    const std::map<std::string, std::size_t>& defaults) {
    if (tokenIs(tokens[declaration], "integer") || tokenIs(tokens[declaration], "time")) {
        return std::nullopt;
    }
    std::optional<ConstantRange> result = ConstantRange{0, 0};
    bool sawRange = false;
    for (std::size_t token = declaration; token < name; ++token) {
        if (tokens[token].text != "[") continue;
        const auto after = skipBalanced(tokens, token, name, "[", "]");
        if (!after || sawRange) return std::nullopt;
        result = verilogDeclarationRangeBounds(tokens, token, *after, defaults);
        if (!result) return std::nullopt;
        sawRange = true;
        token = *after - 1;
    }
    return result;
}

std::vector<ConstantRange> verilogUnpackedDeclarationRanges(
    const std::vector<HdlToken>& tokens, std::size_t name, std::size_t end,
    const std::map<std::string, std::size_t>& defaults) {
    std::vector<ConstantRange> result;
    for (std::size_t token = name + 1; token < end && tokens[token].text == "[";) {
        const auto after = skipBalanced(tokens, token, end, "[", "]");
        if (!after) return {};
        const auto range = verilogDeclarationRangeBounds(tokens, token, *after, defaults);
        if (!range) return {};
        result.push_back(*range);
        token = *after;
    }
    return result;
}

std::optional<std::size_t> verilogDeclarationWidth(const std::vector<HdlToken>& tokens,
                                                   std::size_t declaration,
                                                   std::size_t name,
                                                   const std::map<std::string, std::size_t>& defaults) {
    if (tokenIs(tokens[declaration], "integer") || tokenIs(tokens[declaration], "time")) {
        return std::nullopt;
    }
    std::size_t width = 1;
    bool sawRange = false;
    for (std::size_t token = declaration; token < name; ++token) {
        if (tokens[token].text != "[") continue;
        const auto after = skipBalanced(tokens, token, name, "[", "]");
        if (!after) return std::nullopt;
        const auto range = verilogDeclarationRangeBounds(tokens, token, *after, defaults);
        const auto rangeWidth = range ? std::optional<std::size_t>(range->high - range->low + 1)
                                      : std::nullopt;
        if (!rangeWidth) return std::nullopt;
        if (width > (1U << 24) / *rangeWidth) return std::nullopt;
        width *= *rangeWidth;
        sawRange = true;
        token = *after - 1;
    }
    return sawRange ? std::optional<std::size_t>(width) : std::optional<std::size_t>(1);
}

VerilogSignedness verilogDeclarationSignedness(const std::vector<HdlToken>& tokens,
                                               std::size_t declaration,
                                               std::size_t name) {
    for (std::size_t token = declaration; token < name; ++token) {
        if (tokenIs(tokens[token], "signed")) return VerilogSignedness::Signed;
        if (tokenIs(tokens[token], "unsigned")) return VerilogSignedness::Unsigned;
    }
    if (tokenIs(tokens[declaration], "integer") || tokenIs(tokens[declaration], "int") ||
        tokenIs(tokens[declaration], "shortint") || tokenIs(tokens[declaration], "longint") ||
        tokenIs(tokens[declaration], "byte")) {
        return VerilogSignedness::Signed;
    }
    // Verilog nets, reg/logic declarations, time and implicit port nets are
    // unsigned unless an explicit signed qualifier is present.
    return VerilogSignedness::Unsigned;
}

std::optional<std::size_t> vhdlTypeWidth(const std::vector<HdlToken>& tokens,
                                        std::size_t begin, std::size_t end) {
    for (std::size_t token = begin; token < end; ++token) {
        if (tokenIs(tokens[token], "std_logic_vector") || tokenIs(tokens[token], "bit_vector") ||
            tokenIs(tokens[token], "unsigned") || tokenIs(tokens[token], "signed")) {
            if (token + 1 >= end || tokens[token + 1].text != "(") return std::nullopt;
            const auto after = skipBalanced(tokens, token + 1, end, "(", ")");
            if (!after) return std::nullopt;
            return constantRangeWidth(tokens, token + 1, *after, true);
        }
        if (tokenIs(tokens[token], "std_logic") || tokenIs(tokens[token], "bit") ||
            tokenIs(tokens[token], "boolean")) return 1;
    }
    return std::nullopt;
}

std::optional<ConstantRange> vhdlTypeRange(const std::vector<HdlToken>& tokens,
                                           std::size_t begin, std::size_t end) {
    for (std::size_t token = begin; token < end; ++token) {
        if (tokenIs(tokens[token], "std_logic_vector") || tokenIs(tokens[token], "bit_vector") ||
            tokenIs(tokens[token], "unsigned") || tokenIs(tokens[token], "signed")) {
            if (token + 1 >= end || tokens[token + 1].text != "(") return std::nullopt;
            const auto after = skipBalanced(tokens, token + 1, end, "(", ")");
            return after ? constantRangeBounds(tokens, token + 1, *after, true) : std::nullopt;
        }
        if (tokenIs(tokens[token], "std_logic") || tokenIs(tokens[token], "bit") ||
            tokenIs(tokens[token], "boolean")) return ConstantRange{0, 0};
    }
    return std::nullopt;
}

bool inProcess(std::size_t position, const std::vector<ProcessRange>& ranges) {
    return std::ranges::any_of(ranges, [&](const ProcessRange& range) {
        return position >= range.begin && position < range.end;
    });
}

std::map<std::string, std::size_t> verilogModuleParameterDefaults(
    const std::vector<HdlToken>& tokens, std::size_t first, std::size_t body,
    std::size_t end, const std::vector<ProcessRange>& processes) {
    std::map<std::string, std::size_t> defaults;
    bool subprogram = false;
    unsigned generateDepth = 0;
    for (auto index = first; index < end; ++index) {
        if (index >= body) {
            if (tokenIs(tokens[index], "function") || tokenIs(tokens[index], "task")) {
                subprogram = true; continue;
            }
            if (tokenIs(tokens[index], "endfunction") || tokenIs(tokens[index], "endtask")) {
                subprogram = false; continue;
            }
            if (tokenIs(tokens[index], "generate")) { ++generateDepth; continue; }
            if (tokenIs(tokens[index], "endgenerate")) {
                if (generateDepth) --generateDepth;
                continue;
            }
            if (subprogram || generateDepth || inProcess(index, processes)) continue;
        }
        if (!tokenIs(tokens[index], "parameter") && !tokenIs(tokens[index], "localparam")) continue;
        auto name = index + 1;
        while (name < end && (tokenIs(tokens[name], "integer") || tokenIs(tokens[name], "int") ||
               tokenIs(tokens[name], "signed") || tokenIs(tokens[name], "unsigned") ||
               tokenIs(tokens[name], "logic") || tokenIs(tokens[name], "bit"))) ++name;
        if (name < end && tokens[name].text == "[") {
            const auto after = skipBalanced(tokens, name, end, "[", "]");
            if (!after) continue;
            name = *after;
        }
        while (name + 2 < end && identifier(tokens[name]) && tokens[name + 1].text == "=") {
            auto stop = name + 2;
            unsigned depth = 0;
            for (; stop < end; ++stop) {
                const auto& value = tokens[stop].text;
                if (value == "(" || value == "[" || value == "{") ++depth;
                else if (value == ")" || value == "]" || value == "}") {
                    if (!depth) break;
                    --depth;
                } else if (!depth && (value == "," || value == ";")) break;
            }
            if (const auto value = verilogDefaultInteger(tokens, name + 2, stop, defaults))
                defaults[tokens[name].text] = *value;
            if (stop + 2 >= end || tokens[stop].text != "," ||
                !identifier(tokens[stop + 1]) || tokens[stop + 2].text != "=") break;
            name = stop + 1;
        }
    }
    return defaults;
}

std::size_t verilogStatementEnd(const std::vector<HdlToken>& tokens,
                                std::size_t position, std::size_t limit) {
    while (position < limit && tokens[position].text == ";") ++position;
    if (position >= limit) return position;
    if ((tokenIs(tokens[position], "unique") || tokenIs(tokens[position], "unique0") ||
         tokenIs(tokens[position], "priority")) && position + 1 < limit) {
        return verilogStatementEnd(tokens, position + 1, limit);
    }
    if (tokenIs(tokens[position], "begin")) {
        int depth = 0;
        for (std::size_t token = position; token < limit; ++token) {
            if (tokenIs(tokens[token], "begin")) ++depth;
            else if (tokenIs(tokens[token], "end") && --depth == 0) return token + 1;
        }
        return limit;
    }
    if (tokenIs(tokens[position], "if")) {
        std::size_t statement = position + 1;
        if (statement < limit && tokens[statement].text == "(") {
            const auto after = skipBalanced(tokens, statement, limit, "(", ")");
            if (!after) return limit;
            statement = *after;
        }
        auto finish = verilogStatementEnd(tokens, statement, limit);
        if (finish < limit && tokenIs(tokens[finish], "else")) {
            finish = verilogStatementEnd(tokens, finish + 1, limit);
        }
        return finish;
    }
    if (tokenIs(tokens[position], "case") || tokenIs(tokens[position], "casez") ||
        tokenIs(tokens[position], "casex")) {
        int depth = 0;
        for (std::size_t token = position; token < limit; ++token) {
            if (tokenIs(tokens[token], "case") || tokenIs(tokens[token], "casez") ||
                tokenIs(tokens[token], "casex")) ++depth;
            else if (tokenIs(tokens[token], "endcase") && --depth == 0) return token + 1;
        }
        return limit;
    }
    if (tokenIs(tokens[position], "for") || tokenIs(tokens[position], "while") ||
        tokenIs(tokens[position], "repeat")) {
        std::size_t statement = position + 1;
        if (statement < limit && tokens[statement].text == "(") {
            const auto after = skipBalanced(tokens, statement, limit, "(", ")");
            if (!after) return limit;
            statement = *after;
        }
        return verilogStatementEnd(tokens, statement, limit);
    }
    if (tokens[position].text == "@") {
        std::size_t statement = position + 1;
        if (statement < limit && tokens[statement].text == "*") ++statement;
        else if (statement < limit && tokens[statement].text == "(") {
            const auto after = skipBalanced(tokens, statement, limit, "(", ")");
            if (!after) return limit;
            statement = *after;
        }
        return verilogStatementEnd(tokens, statement, limit);
    }
    while (position < limit && tokens[position].text != ";") ++position;
    return position < limit ? position + 1 : position;
}

std::vector<ProcessRange> verilogProcesses(const std::vector<HdlToken>& tokens,
                                           std::size_t body, std::size_t end) {
    std::vector<ProcessRange> ranges;
    int nextId = 1;
    for (std::size_t i = body; i < end; ++i) {
        if (!tokenIs(tokens[i], "always") && !tokenIs(tokens[i], "always_ff") &&
            !tokenIs(tokens[i], "always_comb") && !tokenIs(tokens[i], "always_latch") &&
            !tokenIs(tokens[i], "initial")) continue;
        std::size_t statement = i + 1;
        if (statement < end && tokens[statement].text == "@") ++statement;
        if (statement < end && tokens[statement].text == "*") ++statement;
        if (statement < end && tokens[statement].text == "(") {
            const auto after = skipBalanced(tokens, statement, end, "(", ")");
            if (!after) continue;
            statement = *after;
        }
        while (statement < end && tokenIs(tokens[statement], "begin") == false &&
               tokens[statement].text == ":") ++statement;
        const auto finish = verilogStatementEnd(tokens, statement, end);
        ranges.push_back({statement, finish, nextId++, tokenIs(tokens[i], "initial"), i});
        i = finish > 0 ? finish - 1 : i;
    }
    return ranges;
}

std::optional<std::size_t> assignmentOperator(const std::vector<HdlToken>& tokens,
                                              std::size_t target, std::size_t limit) {
    std::size_t position = target + 1;
    while (position < limit && tokens[position].text == "[") {
        const auto after = skipBalanced(tokens, position, limit, "[", "]");
        if (!after) return std::nullopt;
        position = *after;
    }
    if (position < limit && tokens[position].text == "<" && position + 1 < limit &&
        tokens[position + 1].text == "=") return position;
    if (position < limit && tokens[position].text == "=" &&
        (position + 1 >= limit || tokens[position + 1].text != "=") &&
        (position == 0 || tokens[position - 1].text != "=")) return position;
    return std::nullopt;
}

std::optional<std::size_t> verilogAssignmentOperator(
    const std::vector<HdlToken>& tokens, std::size_t target,
    std::size_t statementBegin, std::size_t limit) {
    const auto op = assignmentOperator(tokens, target, limit);
    if (!op) return std::nullopt;
    if (target <= statementBegin) return op;
    const auto previous = target - 1;
    if (tokens[previous].text == ";" || tokens[previous].text == ":" ||
        tokenIs(tokens[previous], "begin") || tokenIs(tokens[previous], "else") ||
        tokenIs(tokens[previous], "do") || tokenIs(tokens[previous], "forever") ||
        tokenIs(tokens[previous], "end") || tokenIs(tokens[previous], "endcase") ||
        tokenIs(tokens[previous], "join") || tokenIs(tokens[previous], "join_any") ||
        tokenIs(tokens[previous], "join_none")) return op;
    if (target >= statementBegin + 3 && identifier(tokens[previous]) &&
        tokens[target - 2].text == ":" && tokenIs(tokens[target - 3], "begin")) return op;
    if (tokens[previous].text != ")") return std::nullopt;

    int depth = 0;
    for (std::size_t cursor = previous + 1; cursor > statementBegin; ) {
        --cursor;
        if (tokens[cursor].text == ")") ++depth;
        else if (tokens[cursor].text == "(" && --depth == 0) {
            if (cursor == 0) return std::nullopt;
            const auto& introducer = tokens[cursor - 1];
            return tokenIs(introducer, "if") || tokenIs(introducer, "while") ||
                    tokenIs(introducer, "for") || tokenIs(introducer, "repeat") ||
                    introducer.text == "@" ? op : std::nullopt;
        }
    }
    return std::nullopt;
}

std::optional<std::size_t> vhdlAssignmentOperator(const std::vector<HdlToken>& tokens,
                                                  std::size_t target, std::size_t limit) {
    if (target > 0) {
        const auto& previous = tokens[target - 1];
        const bool statementStart = previous.text == ";" || previous.text == ":" ||
            previous.text == ">" || tokenIs(previous, "begin") ||
            tokenIs(previous, "then") || tokenIs(previous, "else") ||
            tokenIs(previous, "loop") || tokenIs(previous, "select");
        if (!statementStart) return std::nullopt;
    }
    std::size_t position = target + 1;
    while (position < limit && tokens[position].text == "(") {
        const auto after = skipBalanced(tokens, position, limit, "(", ")");
        if (!after) return std::nullopt;
        position = *after;
    }
    if (position + 1 < limit && tokens[position].text == "<" &&
        tokens[position + 1].text == "=") return position;
    return std::nullopt;
}

void addDriver(SignalState& signal, int source, const core::SourceLocation& sourceLocation,
               bool initialization = false) {
    if (std::ranges::none_of(signal.drivers, [&](const Driver& driver) {
            return driver.source == source;
    })) signal.drivers.push_back({source, sourceLocation, initialization});
}

struct CombinationalGraph {
    std::map<std::string, std::set<std::string>> edges;
    std::map<std::string, core::SourceLocation> locations;
    std::map<std::string, std::string> displayNames;
};

std::vector<std::string> verilogDependencyNodes(
    const std::vector<HdlToken>& tokens, std::size_t name,
    std::size_t limit, const SignalState& signal) {
    const auto base = tokens[name].text;
    if (name + 2 < limit && tokens[name + 1].text == "[") {
        const auto after = skipBalanced(tokens, name + 1, limit, "[", "]");
        if (after) {
            const auto first = verilogIntegerValue(tokens[name + 2].text);
            if (first && *after == name + 4)
                return {base + "[" + std::to_string(*first) + "]"};
            if (first && *after == name + 6 && tokens[name + 3].text == ":") {
                const auto last = verilogIntegerValue(tokens[name + 4].text);
                if (last && std::max(*first, *last) - std::min(*first, *last) <= 63) {
                    std::vector<std::string> nodes;
                    for (auto bit = std::min(*first, *last); bit <= std::max(*first, *last); ++bit)
                        nodes.push_back(base + "[" + std::to_string(bit) + "]");
                    return nodes;
                }
            }
        }
    }
    // Large or unknown ranges remain an unresolved whole-signal approximation.
    if (!signal.range || signal.range->high - signal.range->low > 63)
        return {base};
    std::vector<std::string> nodes;
    for (auto bit = signal.range->low; bit <= signal.range->high; ++bit)
        nodes.push_back(base + "[" + std::to_string(bit) + "]");
    return nodes;
}

void addCombinationalDependency(CombinationalGraph& graph, const std::string& target,
                                const std::string& source,
                                const core::SourceLocation& sourceLocation,
                                std::string targetDisplay, std::string sourceDisplay) {
    graph.edges[target].insert(source);
    graph.locations.try_emplace(target, sourceLocation);
    graph.displayNames.try_emplace(target, std::move(targetDisplay));
    graph.displayNames.try_emplace(source, std::move(sourceDisplay));
}

void appendCombinationalLoopIssues(const CombinationalGraph& graph,
                                   std::vector<SemanticIssue>& issues,
                                   const std::string& unitName) {
    std::map<std::string, int> index;
    std::map<std::string, int> lowLink;
    std::set<std::string> onStack;
    std::vector<std::string> stack;
    int nextIndex = 0;
    std::function<void(const std::string&)> visit = [&](const std::string& node) {
        index[node] = nextIndex;
        lowLink[node] = nextIndex++;
        stack.push_back(node);
        onStack.insert(node);
        const auto outgoing = graph.edges.find(node);
        if (outgoing != graph.edges.end()) {
            for (const auto& dependency : outgoing->second) {
                if (!index.contains(dependency)) {
                    visit(dependency);
                    lowLink[node] = std::min(lowLink[node], lowLink[dependency]);
                } else if (onStack.contains(dependency)) {
                    lowLink[node] = std::min(lowLink[node], index[dependency]);
                }
            }
        }
        if (lowLink[node] != index[node]) return;
        std::set<std::string> component;
        while (!stack.empty()) {
            const auto member = stack.back();
            stack.pop_back();
            onStack.erase(member);
            component.insert(member);
            if (member == node) break;
        }
        const bool selfCycle = component.size() == 1 &&
            graph.edges.contains(*component.begin()) &&
            graph.edges.at(*component.begin()).contains(*component.begin());
        if (component.size() < 2 && !selfCycle) return;
        std::set<std::string> display;
        for (const auto& member : component) {
            const auto name = graph.displayNames.find(member);
            display.insert(name == graph.displayNames.end() ? member : name->second);
        }
        const auto first = *component.begin();
        const auto sourceLocation = graph.locations.find(first);
        if (sourceLocation == graph.locations.end()) return;
        issues.push_back({"RTL-COMBINATIONAL-LOOP",
            "Combinational dependency cycle: " + joinedNames(display),
            "Combinational assignments form a feedback cycle with no sequential element to break it.",
            "Break the feedback path with registered state or correct the unintended dependency.",
            sourceLocation->second, unitName,
            graph.displayNames.contains(first) ?
                std::optional<std::string>(graph.displayNames.at(first)) :
                std::optional<std::string>(first)});
    };
    for (const auto& [node, dependencies] : graph.edges) {
        static_cast<void>(dependencies);
        if (!index.contains(node)) visit(node);
    }
}

void appendWidthTruncation(std::vector<SemanticIssue>& issues,
                           const std::string& unitName,
                           const std::string& targetName, const SignalState& target,
                           const std::string& sourceName, const SignalState& source,
                           const core::SourceLocation& sourceLocation) {
    if (!target.width || !source.width || *source.width <= *target.width) return;
    issues.push_back({"RTL-WIDTH-TRUNCATION",
        "Direct assignment truncates " + sourceName + " from " +
            std::to_string(*source.width) + " bits to " + std::to_string(*target.width) +
            " bits in " + targetName,
        "A wider declared signal is assigned directly to a narrower declared signal, so high bits are discarded.",
        "Resize or slice the source explicitly and document the intended truncation.",
        sourceLocation, unitName, targetName});
}

std::optional<std::size_t> singleExpressionToken(
    const std::vector<HdlToken>& tokens, std::size_t begin, std::size_t end) {
    while (begin < end && tokens[begin].text == "(") {
        const auto after = skipBalanced(tokens, begin, end, "(", ")");
        if (!after || *after != end) break;
        ++begin;
        --end;
    }
    return begin + 1 == end ? std::optional<std::size_t>(begin) : std::nullopt;
}

void appendLiteralWidthTruncation(
    std::vector<SemanticIssue>& issues, const std::string& unitName,
    const std::string& targetName, const SignalState& target,
    std::size_t literalWidth, const HdlToken& literal,
    const core::SourceFile& source) {
    if (!target.width || literalWidth <= *target.width) return;
    issues.push_back({"RTL-WIDTH-TRUNCATION",
        "Direct assignment truncates literal from " +
            std::to_string(literalWidth) + " bits to " +
            std::to_string(*target.width) + " bits in " + targetName,
        "An explicitly sized literal is assigned directly to a narrower declared signal, so high bits are discarded or rejected by the HDL type constraints.",
        "Resize the literal or target explicitly and document the intended value conversion.",
        location(source, literal), unitName, targetName});
}

void appendCompositeWidthTruncation(
    std::vector<SemanticIssue>& issues, const std::string& unitName,
    const std::string& targetName, const SignalState& target,
    std::size_t expressionWidth, const std::string& expressionDisplay,
    const core::SourceLocation& sourceLocation) {
    if (!target.width || expressionWidth <= *target.width) return;
    issues.push_back({"RTL-WIDTH-TRUNCATION",
        "Direct assignment truncates " + expressionDisplay + " from " +
            std::to_string(expressionWidth) + " bits to " +
            std::to_string(*target.width) + " bits in " + targetName,
        "A statically sized concatenation is assigned directly to a narrower target, so high bits are discarded or rejected by HDL type constraints.",
        "Resize or slice the concatenation explicitly and document the intended conversion.",
        sourceLocation, unitName, targetName});
}

void appendVhdlAssignmentWidthMismatch(
    std::vector<SemanticIssue>& issues, const std::string& unitName,
    const std::string& targetName, std::size_t targetWidth,
    const std::string& sourceName, std::size_t sourceWidth,
    const core::SourceLocation& sourceLocation) {
    if (targetWidth == sourceWidth) return;
    issues.push_back({"RTL-WIDTH-TRUNCATION",
        "Assignment operands have different widths: " + targetName + " is " +
            std::to_string(targetWidth) + " bits and " + sourceName + " is " +
            std::to_string(sourceWidth) + " bits",
        "The native GJB9765.4.6.b.1 assignment branch reports any statically known width inequality, including both truncation and implicit extension.",
        "Resize or slice the source and target to the same intended width.",
        sourceLocation, unitName, targetName});
}

void appendVerilogArithmeticResultWidthRisk(
    const core::SourceFile& source, const std::vector<HdlToken>& tokens,
    std::size_t begin, std::size_t end, std::size_t targetWidth,
    const std::string& targetDisplay,
    const std::map<std::string, SignalState>& signals,
    std::vector<SemanticIssue>& issues, const std::string& unitName) {
    while (begin < end && tokens[begin].text == "(") {
        const auto after = skipBalanced(tokens, begin, end, "(", ")");
        if (!after || *after != end) break;
        ++begin;
        --end;
    }

    struct ArithmeticRoot {
        std::size_t token;
        std::size_t tokenCount;
        std::string_view operation;
    };
    const auto stripParentheses = [&](std::size_t& first, std::size_t& last) {
        while (first < last && tokens[first].text == "(") {
            const auto after = skipBalanced(tokens, first, last, "(", ")");
            if (!after || *after != last) break;
            ++first;
            --last;
        }
    };
    const auto splitArithmetic = [&](std::size_t first, std::size_t last)
            -> std::optional<ArithmeticRoot> {
        stripParentheses(first, last);
        const auto binarySign = [&](std::size_t token) {
            if (token == first) return false;
            const auto& previous = tokens[token - 1].text;
            return previous != "+" && previous != "-" && previous != "*" &&
                previous != "/" && previous != "%" && previous != "(" &&
                previous != "{" && previous != "," && previous != "?" &&
                previous != ":";
        };
        std::optional<ArithmeticRoot> additive;
        std::optional<ArithmeticRoot> multiplicative;
        std::optional<ArithmeticRoot> power;
        int parentheses = 0;
        int brackets = 0;
        int braces = 0;
        for (std::size_t token = first; token < last; ++token) {
            const auto& text = tokens[token].text;
            if (text == "(") ++parentheses;
            else if (text == ")") --parentheses;
            else if (text == "[") ++brackets;
            else if (text == "]") --brackets;
            else if (text == "{") ++braces;
            else if (text == "}") --braces;
            else if (parentheses == 0 && brackets == 0 && braces == 0) {
                if ((text == "+" || text == "-") && binarySign(token)) {
                    additive = ArithmeticRoot{token, 1, text}; // rightmost: left associative
                } else if (text == "/" || text == "%") {
                    multiplicative = ArithmeticRoot{token, 1, text};
                } else if (text == "*") {
                    const bool previousStar = token > first && tokens[token - 1].text == "*";
                    const bool nextStar = token + 1 < last && tokens[token + 1].text == "*";
                    if (!previousStar && !nextStar) {
                        multiplicative = ArithmeticRoot{token, 1, "*"};
                    } else if (nextStar && !power) {
                        power = ArithmeticRoot{token, 2, "**"}; // leftmost: right associative
                    }
                }
            }
        }
        if (additive) return additive;
        if (multiplicative) return multiplicative;
        return power;
    };

    // FUN_1808f4650 initializes width to -1 and calls FUN_1808f46c0 with
    // the rule-provided low-byte flag cleared. In that mode nested arithmetic
    // ADD/SUB/MUL/DIV/MOD nodes take max(child widths), nested POW propagates
    // its base width, CONCAT sums children, and REPLICATE multiplies its
    // constant count by the repeated expression width. comparisonOperandShape
    // already provides the proven direct/select/literal/concat/replicate cases.
    const auto widthOf = [&](auto&& self, std::size_t first, std::size_t last)
            -> std::optional<std::size_t> {
        stripParentheses(first, last);
        if (const auto shape = comparisonOperandShape(
                tokens, first, last, signals, false)) {
            return shape->width < (1U << 24)
                ? std::optional<std::size_t>(shape->width) : std::nullopt;
        }
        if (first < last && (tokens[first].text == "+" || tokens[first].text == "-")) {
            return self(self, first + 1, last); // AST_POS/AST_NEG propagate child width.
        }
        const auto root = splitArithmetic(first, last);
        if (!root || root->token == first || root->token + root->tokenCount >= last) {
            return std::nullopt;
        }
        const auto left = self(self, first, root->token);
        const auto right = self(self, root->token + root->tokenCount, last);
        if (!left || !right) return std::nullopt;
        return root->operation == "**" ? left : std::max(*left, *right);
    };

    const auto root = splitArithmetic(begin, end);
    if (!root || root->token == begin || root->token + root->tokenCount >= end) return;
    const auto rightBegin = root->token + root->tokenCount;
    const auto leftWidth = widthOf(widthOf, begin, root->token);
    const auto rightWidth = widthOf(widthOf, rightBegin, end);
    if (!leftWidth || !rightWidth) return;

    if (root->operation == "+" || root->operation == "-") {
        const auto operandReports = [&](std::size_t width,
                                        std::size_t first, std::size_t last) {
            if (width > targetWidth) return true;
            return width == targetWidth &&
                !verilogNativeCarryExemptConcat(tokens, first, last, signals);
        };
        if (!operandReports(*leftWidth, begin, root->token) &&
            !operandReports(*rightWidth, rightBegin, end)) return;
        issues.push_back({"RTL-ARITHMETIC-RESULT-WIDTH-RISK",
            "Addition/subtraction operand width reaches or exceeds the " +
                std::to_string(targetWidth) + "-bit target " + targetDisplay,
            "The native ADD/SUB branch reports an operand wider than the target, "
            "or equally wide unless a multi-item concatenation carries the native "
            "leftmost empty/all-zero bit-vector exemption.",
            "Widen the target or use an explicit leading-zero concatenation where "
            "that is the intended carry/borrow bound.",
            location(source, tokens[root->token]), unitName, targetDisplay});
        return;
    }
    if (root->operation == "*") {
        // The outer AST_MUL rule branch sums its two direct child widths. A
        // nested arithmetic child has already been reduced by widthOf using
        // the helper's flag-zero max-width behavior.
        const auto required = *leftWidth + *rightWidth;
        if (targetWidth >= required) return;
        issues.push_back({"RTL-ARITHMETIC-RESULT-WIDTH-RISK",
            "Multiplication result may need " + std::to_string(required) +
                " bits but " + targetDisplay + " has " + std::to_string(targetWidth) + " bits",
            "The native multiplication branch sums the statically inferred widths of its operands before comparing them with the assignment target.",
            "Widen the multiplication target to the sum of the operand widths or explicitly size the intended truncated result.",
            location(source, tokens[root->token]), unitName, targetDisplay});
        return;
    }
    if (root->operation == "/" || root->operation == "%") {
        // Native FUN_1805790d0 cases AST_DIV/AST_MOD first infer the width of
        // the complete operator node. FUN_1808f46c0 handles both nodes by
        // taking the maximum child width, after which the rule reports only
        // when target_width < inferred_width - 1. Keep this unusual vendor
        // threshold instead of replacing it with a conventional HDL formula.
        const auto inferred = std::max(*leftWidth, *rightWidth);
        if (inferred <= 1 || targetWidth >= inferred - 1) return;
        const auto required = inferred - 1;
        issues.push_back({"RTL-ARITHMETIC-RESULT-WIDTH-RISK",
            std::string(root->operation == "/" ? "Division" : "Modulo") +
                " result uses native inferred threshold " + std::to_string(required) +
                " bits but " + targetDisplay + " has " + std::to_string(targetWidth) + " bits",
            "The native division/modulo branch compares the target with one less than the complete expression's inferred width.",
            "Widen the target or explicitly size the intended division/modulo result.",
            location(source, tokens[root->token]), unitName, targetDisplay});
        return;
    }
    if (root->operation == "**") {
        // Native AST_POW accepts exactly two children, requires the base child
        // to be AST_IDENTIFIER, and evaluates:
        //   (int)(pow(2.0, exponent_width) - 1.0) * base_width - 1
        // Report only the bounded range in which that native signed-int
        // arithmetic has an exact, non-overflowing integer counterpart.
        // The base child must itself remain AST_IDENTIFIER; a constant or
        // nested expression is excluded even when its width is known. An
        // identifier with a static select still has AST_IDENTIFIER type.
        const auto base = verilogCaseSelectorShape(tokens, begin, root->token, signals);
        if (!base) return;
        const auto baseWidth = base->width;
        const auto exponentWidth = *rightWidth;
        if (exponentWidth == 0 || exponentWidth >= 63 || baseWidth == 0) return;
        const auto multiplier = (std::uint64_t{1} << exponentWidth) - 1;
        if (multiplier > static_cast<std::uint64_t>(std::numeric_limits<int>::max()) /
                             baseWidth) return;
        const auto required = multiplier * baseWidth - 1;
        if (required > static_cast<std::uint64_t>(std::numeric_limits<int>::max()) ||
            targetWidth >= required) return;
        issues.push_back({"RTL-ARITHMETIC-RESULT-WIDTH-RISK",
            "Exponentiation result uses native threshold " + std::to_string(required) +
                " bits but " + targetDisplay + " has " + std::to_string(targetWidth) + " bits",
            "The native exponentiation branch derives its threshold from the base width and the exponent operand width, not from the exponent value.",
            "Widen the target or explicitly size the intended exponentiation result.",
            location(source, tokens[root->token]), unitName, targetDisplay});
        return;
    }
}

void appendConstantSelectIssues(const core::SourceFile& source,
                                const std::vector<HdlToken>& tokens,
                                std::size_t begin, std::size_t end,
                                const std::map<std::string, SignalState>& signals,
                                bool vhdl, std::vector<SemanticIssue>& issues,
                                const std::string& unitName) {
    struct IndexValue {
        bool negative;
        std::size_t magnitude;
    };
    const auto valueOf = [&](std::size_t first, std::size_t last) -> std::optional<IndexValue> {
        if (first >= last) return std::nullopt;
        bool negative = false;
        if (tokens[first].text == "-" || tokens[first].text == "+") {
            negative = tokens[first].text == "-";
            ++first;
        }
        if (first + 1 != last || tokens[first].kind != HdlTokenKind::Number)
            return std::nullopt;
        const auto magnitude = vhdl ? decimalValue(tokens[first].text)
                                    : verilogIntegerValue(tokens[first].text);
        if (!magnitude) return std::nullopt;
        return IndexValue{negative && *magnitude != 0, *magnitude};
    };
    std::map<std::string, IndexValue> immutableIndices;
    if (!vhdl && std::ranges::none_of(tokens.begin() + static_cast<std::ptrdiff_t>(begin),
                                     tokens.begin() + static_cast<std::ptrdiff_t>(end),
                                     [](const HdlToken& token) { return token.text == "`"; })) {
        std::map<std::string, std::size_t> declarations;
        std::size_t admissionEnd = end;
        for (std::size_t i = begin; i < end; ++i) {
            if (tokenIs(tokens[i], "always") || tokenIs(tokens[i], "always_comb") ||
                tokenIs(tokens[i], "always_ff") || tokenIs(tokens[i], "always_latch") ||
                tokenIs(tokens[i], "initial") || tokenIs(tokens[i], "assign") ||
                tokenIs(tokens[i], "generate") || tokenIs(tokens[i], "function") ||
                tokenIs(tokens[i], "task") || tokenIs(tokens[i], "if")) {
                admissionEnd = i;
                break;
            }
        }
        for (std::size_t i = begin; i < admissionEnd; ++i) {
            if (!tokenIs(tokens[i], "localparam")) continue;
            std::size_t statementEnd = i + 1;
            while (statementEnd < admissionEnd && tokens[statementEnd].text != ";")
                ++statementEnd;
            if (statementEnd == admissionEnd) break;
            const auto names = declarationNames(tokens, i + 1, statementEnd + 1, false);
            const auto equals = std::find_if(
                tokens.begin() + static_cast<std::ptrdiff_t>(i + 1),
                tokens.begin() + static_cast<std::ptrdiff_t>(statementEnd),
                [](const HdlToken& token) { return token.text == "="; });
            const auto equalIndex = static_cast<std::size_t>(equals - tokens.begin());
            if (names.size() == 1 && equalIndex < statementEnd &&
                std::ranges::none_of(tokens.begin() + static_cast<std::ptrdiff_t>(i + 1),
                                     tokens.begin() + static_cast<std::ptrdiff_t>(statementEnd),
                                     [](const HdlToken& token) { return token.text == ","; })) {
                const auto nameIndex = static_cast<std::size_t>(names.front() - tokens.data());
                const auto signal = signals.find(names.front()->text);
                const bool distinctSignal = signal != signals.end() &&
                    signal->second.declaration.byteOffset != tokens[nameIndex].offset;
                if (nameIndex + 1 == equalIndex && !distinctSignal) {
                    if (const auto value = valueOf(equalIndex + 1, statementEnd)) {
                        immutableIndices.emplace(names.front()->text, *value);
                        declarations.emplace(names.front()->text, nameIndex);
                    }
                }
            }
            i = statementEnd;
        }
        // Any second declaration, including a procedural shadow, makes a
        // module-wide lexical binding unsafe without full scope elaboration.
        for (std::size_t i = begin; i < end; ++i) {
            if (!typeKeyword(tokens[i]) && !directionKeyword(tokens[i]) &&
                !tokenIs(tokens[i], "parameter") && !tokenIs(tokens[i], "localparam") &&
                !tokenIs(tokens[i], "genvar")) continue;
            for (const auto* name : declarationNames(tokens, i + 1, end, false)) {
                const auto found = declarations.find(name->text);
                if (found != declarations.end() &&
                    found->second != static_cast<std::size_t>(name - tokens.data()))
                    immutableIndices.erase(name->text);
            }
        }
    }
    const auto resolvedValueOf = [&](std::size_t first, std::size_t last)
        -> std::optional<IndexValue> {
        if (const auto literal = valueOf(first, last)) return literal;
        if (!vhdl && first + 1 == last && identifier(tokens[first])) {
            if (const auto found = immutableIndices.find(tokens[first].text);
                found != immutableIndices.end()) return found->second;
        }
        return std::nullopt;
    };
    const auto outside = [](const IndexValue& index, const ConstantRange& range) {
        return index.negative || index.magnitude < range.low || index.magnitude > range.high;
    };
    const auto indexText = [](const IndexValue& index) {
        return std::string(index.negative ? "-" : "") + std::to_string(index.magnitude);
    };
    const auto report = [&](const HdlToken& signalToken, const HdlToken& boundaryToken,
                            const ConstantRange& range, std::string selection) {
        issues.push_back({"RTL-OUT-OF-RANGE-SELECT",
            "Constant " + selection + " is outside declared bounds " +
                std::to_string(range.low) + ".." + std::to_string(range.high) +
                " of " + signalToken.text,
            "A constant bit index or slice boundary falls outside the signal's declared range.",
            "Correct the index or slice, or resize the declaration to include the intended bits.",
            location(source, boundaryToken), unitName, signalToken.text});
    };
    const auto reportDynamicCapacity = [&](const HdlToken& signalToken,
                                           const HdlToken& indexToken,
                                           const ConstantRange& range,
                                           std::size_t indexWidth) {
        issues.push_back({"RTL-OUT-OF-RANGE-SELECT",
            "Dynamic index " + indexToken.text + " can address " +
                std::to_string(std::size_t{1} << indexWidth) +
                " values, exceeding the declared selection width " +
                std::to_string(range.high - range.low + 1) + " of " + signalToken.text,
            "The native frontend marks a directly assigned bit-select when two to the "
                "statically inferred index width exceeds the selected signal width.",
            "Narrow or constrain the index, or resize the declaration so every encoded "
                "index value is valid.",
            location(source, indexToken), unitName, signalToken.text});
    };
    const std::string_view opener = vhdl ? "(" : "[";
    const std::string_view closer = vhdl ? ")" : "]";
    for (std::size_t token = begin; token + 2 < end; ++token) {
        if (!identifier(tokens[token]) || tokens[token + 1].text != opener) continue;
        const auto signal = signals.find(vhdl ? lower(tokens[token].text) : tokens[token].text);
        if (signal == signals.end()) continue;
        std::size_t chainEnd = token + 1;
        while (chainEnd < end && tokens[chainEnd].text == opener) {
            const auto after = skipBalanced(tokens, chainEnd, end, opener, closer);
            if (!after) break;
            chainEnd = *after;
        }
        const bool directAssignmentTarget = !vhdl && chainEnd < end &&
            (tokens[chainEnd].text == "=" || tokens[chainEnd].text == "<=");
        std::size_t selectionOpen = token + 1;
        std::size_t dimension = 0;
        while (selectionOpen + 1 < end && tokens[selectionOpen].text == opener) {
            const auto after = skipBalanced(tokens, selectionOpen, end, opener, closer);
            if (!after) break;
            std::optional<ConstantRange> selectedRange;
            if (vhdl) {
                if (dimension == 0) selectedRange = signal->second.range;
            } else if (dimension < signal->second.unpackedRanges.size()) {
                selectedRange = signal->second.unpackedRanges[dimension];
            } else if (dimension == signal->second.unpackedRanges.size()) {
                selectedRange = signal->second.range;
            }
            const auto insideBegin = selectionOpen + 1;
            const auto insideEnd = *after - 1;
            if (selectedRange) {
                const auto& range = *selectedRange;
                if (const auto index = resolvedValueOf(insideBegin, insideEnd)) {
                    if (outside(*index, range)) {
                        report(tokens[token], tokens[insideBegin], range,
                               "index " + indexText(*index));
                    }
                } else {
                    std::size_t separator = insideBegin;
                    while (separator < insideEnd && tokens[separator].text != ":" &&
                           (!vhdl || (!tokenIs(tokens[separator], "downto") &&
                                       !tokenIs(tokens[separator], "to")))) ++separator;
                    if (separator < insideEnd && (vhdl || tokens[separator].text == ":")) {
                        const auto left = resolvedValueOf(insideBegin, separator);
                        const auto right = resolvedValueOf(separator + 1, insideEnd);
                        if (left && right) {
                            const bool leftOutside = outside(*left, range);
                            const bool rightOutside = outside(*right, range);
                            if (leftOutside || rightOutside) {
                                report(tokens[token], leftOutside ? tokens[insideBegin] :
                                           tokens[separator + 1], range,
                                       "slice " + indexText(*left) + ".." + indexText(*right));
                            }
                        }
                    } else if (directAssignmentTarget && signal->second.unpackedRanges.empty() &&
                               dimension == 0 && insideBegin + 1 == insideEnd &&
                               identifier(tokens[insideBegin])) {
                        // FUN_180914500 writes is_over_array_index while simplifying a
                        // direct assignment to an AST_IDENTIFIER bit-select.  The exact
                        // observed condition is declaration_width < 2^index_width.  Keep
                        // this adapter to a single declared index identifier; compound
                        // expressions and elaborated aliases remain unknown.
                        const auto indexSignal = signals.find(tokens[insideBegin].text);
                        if (indexSignal != signals.end() && indexSignal->second.width &&
                            *indexSignal->second.width <
                                std::numeric_limits<std::size_t>::digits) {
                            const auto capacity = std::size_t{1} << *indexSignal->second.width;
                            const auto selectedWidth = range.high - range.low + 1;
                            if (selectedWidth < capacity) {
                                reportDynamicCapacity(tokens[token], tokens[insideBegin], range,
                                                      *indexSignal->second.width);
                            }
                        }
                    }
                }
            }
            selectionOpen = *after;
            ++dimension;
            if (vhdl) break;
        }
        if (selectionOpen > token + 1) token = selectionOpen - 1;
    }
}

void appendDerivedClockIssues(const std::map<std::string, SignalState>& signals,
                              const std::map<std::string, std::string>& clockSignals,
                              std::vector<SemanticIssue>& issues,
                              const std::string& unitName) {
    for (const auto& [key, displayName] : clockSignals) {
        const auto signal = signals.find(key);
        if (signal == signals.end() || signal->second.direction == Direction::Input ||
            signal->second.direction == Direction::Inout) continue;
        const auto driver = std::ranges::find_if(signal->second.drivers,
            [](const Driver& candidate) { return !candidate.initialization; });
        if (driver == signal->second.drivers.end()) continue;
        issues.push_back({"RTL-DERIVED-CLOCK",
            "Locally generated signal is used as a clock: " + displayName,
            "A signal driven by local RTL logic is used in an edge-triggered process, which can create skew, glitches, or non-dedicated clock routing.",
            "Prefer a clock enable on the primary clock or instantiate a supported dedicated clocking primitive.",
            driver->location, unitName, displayName});
    }
}

void appendVerilogDelayIssues(const core::SourceFile& source,
                              const std::vector<HdlToken>& tokens,
                              std::size_t body, std::size_t end,
                              const std::vector<ProcessRange>& processes,
                              std::vector<SemanticIssue>& issues,
                              const std::string& unitName) {
    std::set<std::size_t> reported;
    const auto report = [&](std::size_t token, std::string_view construct) {
        if (!reported.insert(tokens[token].offset).second) return;
        issues.push_back({"RTL-NONSYNTHESIZABLE-DELAY",
            "Simulation delay control is used in RTL",
            "A " + std::string(construct) +
                " delay control affects simulation time but is not portable synthesizable RTL.",
            "Remove the delay and express behavior with clocked logic, counters, or constraints.",
            location(source, tokens[token]), unitName, std::nullopt});
    };
    for (const auto& process : processes) {
        for (std::size_t token = process.begin; token < process.end && token < end; ++token) {
            if (tokens[token].text != "#") continue;
            const bool assignmentDelay = token > process.begin &&
                tokens[token - 1].text == "=";
            report(token, assignmentDelay ? "procedural assignment" :
                                          "procedural statement");
        }
    }
    for (std::size_t token = body; token < end; ++token) {
        if (!tokenIs(tokens[token], "assign")) continue;
        while (token < end && tokens[token].text != ";") {
            if (tokens[token].text == "#") report(token, "continuous assignment");
            ++token;
        }
    }

    // The recovered native flag is produced by the grammar reduction
    // behavioral_stmt -> non_opt_delay behavioral_stmt. That production also
    // applies inside task bodies, which are not included in verilogProcesses().
    // Keep the product's example-backed intra-assignment coverage while adding
    // this native-reachable subprogram scope.
    bool inSubprogram = false;
    for (std::size_t token = body; token < end; ++token) {
        if (tokenIs(tokens[token], "task") || tokenIs(tokens[token], "function")) {
            inSubprogram = true;
            continue;
        }
        if (tokenIs(tokens[token], "endtask") || tokenIs(tokens[token], "endfunction")) {
            inSubprogram = false;
            continue;
        }
        if (inSubprogram && tokens[token].text == "#") {
            const bool assignmentDelay = token > body && tokens[token - 1].text == "=";
            report(token, assignmentDelay ? "subprogram assignment" :
                                          "subprogram statement");
        }
    }
}

void appendVhdlDelayIssues(const core::SourceFile& source,
                           const std::vector<HdlToken>& tokens,
                           std::size_t body, std::size_t end,
                           std::vector<SemanticIssue>& issues,
                           const std::string& unitName) {
    for (std::size_t token = body; token < end; ++token) {
        if (!tokenIs(tokens[token], "after")) continue;
        issues.push_back({"RTL-NONSYNTHESIZABLE-DELAY",
            "Simulation delay is used in a VHDL signal assignment",
            "A VHDL after clause schedules simulation time but is not portable synthesizable RTL.",
            "Remove the delay and express behavior with clocked logic, counters, or constraints.",
            location(source, tokens[token]), unitName, std::nullopt});
    }
}

std::optional<std::size_t> zeroLiteralAt(const std::vector<HdlToken>& tokens,
                                         std::size_t begin, std::size_t end,
                                         bool vhdl) {
    while (begin < end && tokens[begin].text == "(") {
        const auto after = skipBalanced(tokens, begin, end, "(", ")");
        if (!after || *after != end) break;
        ++begin;
        --end;
    }
    if (begin >= end || begin + 1 != end ||
        tokens[begin].kind != HdlTokenKind::Number) {
        return std::nullopt;
    }
    const auto value = vhdl ? decimalValue(tokens[begin].text)
                            : verilogIntegerValue(tokens[begin].text);
    return value && *value == 0 ? std::optional<std::size_t>(begin) : std::nullopt;
}

std::optional<std::size_t> negativeLiteralAt(
    const std::vector<HdlToken>& tokens, std::size_t begin, std::size_t end,
    bool vhdl) {
    while (begin < end && tokens[begin].text == "(") {
        const auto after = skipBalanced(tokens, begin, end, "(", ")");
        if (!after || *after != end) break;
        ++begin;
        --end;
    }
    if (begin + 2 != end || tokens[begin].text != "-" ||
        tokens[begin + 1].kind != HdlTokenKind::Number) return std::nullopt;
    const auto value = vhdl ? decimalValue(tokens[begin + 1].text)
                            : verilogIntegerValue(tokens[begin + 1].text);
    return value && *value > 0 ? std::optional<std::size_t>(begin + 1)
                               : std::nullopt;
}

void appendVerilogDivideByZeroIssues(
    const core::SourceFile& source, const std::vector<HdlToken>& tokens,
    std::size_t body, std::size_t end,
    std::vector<SemanticIssue>& issues, const std::string& unitName) {
    for (std::size_t token = body; token + 1 < end; ++token) {
        if (tokens[token].text != "/" && tokens[token].text != "%") continue;
        std::size_t denominator = token + 1;
        if (denominator < end && tokens[denominator].text == "=") ++denominator;
        if (denominator >= end) continue;
        std::size_t expressionEnd = denominator + 1;
        if (tokens[denominator].text == "(") {
            const auto after = skipBalanced(tokens, denominator, end, "(", ")");
            if (!after) continue;
            expressionEnd = *after;
        }
        const auto zero = zeroLiteralAt(tokens, denominator, expressionEnd, false);
        if (!zero) continue;
        issues.push_back({"RTL-DIVIDE-BY-ZERO",
            "Division or remainder uses a constant zero denominator",
            "A division, modulo, or compound arithmetic operation has a denominator that is the literal value zero, producing invalid or tool-dependent behavior.",
            "Replace the zero denominator with the intended nonzero value or guard the operation before it is evaluated.",
            location(source, tokens[token]), unitName, std::nullopt});
        token = expressionEnd - 1;
    }
}

void appendVhdlDivideByZeroIssues(
    const core::SourceFile& source, const std::vector<HdlToken>& tokens,
    std::size_t body, std::size_t end,
    std::vector<SemanticIssue>& issues, const std::string& unitName) {
    for (std::size_t token = body; token + 1 < end; ++token) {
        const bool divide = tokens[token].text == "/";
        if (!divide && !tokenIs(tokens[token], "mod") &&
            !tokenIs(tokens[token], "rem")) continue;
        if (divide && token + 1 < end && tokens[token + 1].text == "=") {
            ++token;
            continue;
        }
        const auto denominator = token + 1;
        std::size_t expressionEnd = denominator + 1;
        if (tokens[denominator].text == "(") {
            const auto after = skipBalanced(tokens, denominator, end, "(", ")");
            if (!after) continue;
            expressionEnd = *after;
        }
        const auto zero = zeroLiteralAt(tokens, denominator, expressionEnd, true);
        if (!zero) continue;
        issues.push_back({"RTL-DIVIDE-BY-ZERO",
            "Division or remainder uses a constant zero denominator",
            "A VHDL division, mod, or rem operation has a denominator that is the literal value zero, which is an arithmetic error.",
            "Replace the zero denominator with the intended nonzero value or guard the operation before it is evaluated.",
            location(source, tokens[token]), unitName, std::nullopt});
        token = expressionEnd - 1;
    }
}

std::string choiceSignature(const std::vector<HdlToken>& tokens,
                            std::size_t begin, std::size_t end,
                            bool caseInsensitive) {
    std::string signature;
    for (std::size_t token = begin; token < end; ++token) {
        if (!signature.empty()) signature.push_back('\x1f');
        signature += caseInsensitive ? lower(tokens[token].text) : tokens[token].text;
    }
    return signature;
}

bool appendDuplicateChoice(std::map<std::string, core::SourceLocation>& choices,
                           const std::vector<HdlToken>& tokens,
                           std::size_t begin, std::size_t end,
                           bool caseInsensitive, const core::SourceFile& source,
                           std::vector<SemanticIssue>& issues,
                           const std::string& unitName) {
    if (begin >= end || tokenIs(tokens[begin], "default") || tokenIs(tokens[begin], "others")) {
        return false;
    }
    const auto signature = choiceSignature(tokens, begin, end, caseInsensitive);
    if (signature.empty()) return false;
    const auto sourceLocation = location(source, tokens[begin]);
    if (choices.try_emplace(signature, sourceLocation).second) return false;
    std::string display;
    for (std::size_t token = begin; token < end; ++token) display += tokens[token].text;
    issues.push_back({"RTL-DUPLICATE-CASE-ITEM",
        "Duplicate case choice is unreachable: " + display,
        "The same exact choice appears earlier in this case statement, so this later branch cannot be selected.",
        "Remove or correct the duplicate choice and verify the intended branch coverage.",
        sourceLocation, unitName, std::nullopt});
    return true;
}

struct WildcardPattern {
    std::string bits;
    std::string display;
    core::SourceLocation location;
};

std::optional<WildcardPattern> parseWildcardPattern(
    const std::vector<HdlToken>& tokens, std::size_t begin, std::size_t end,
    bool casex, const core::SourceFile& source) {
    if (begin + 1 != end || tokens[begin].kind != HdlTokenKind::Number) return std::nullopt;
    auto text = lower(tokens[begin].text);
    text.erase(std::remove(text.begin(), text.end(), '_'), text.end());
    const auto apostrophe = text.find('\'');
    if (apostrophe == std::string::npos || apostrophe == 0) return std::nullopt;
    const auto width = decimalValue(std::string_view(text).substr(0, apostrophe));
    if (!width || *width == 0) return std::nullopt;
    std::size_t basePosition = apostrophe + 1;
    if (basePosition < text.size() && text[basePosition] == 's') ++basePosition;
    if (basePosition >= text.size() ||
        (text[basePosition] != 'b' && text[basePosition] != 'h')) return std::nullopt;
    const char base = text[basePosition++];
    if (basePosition >= text.size()) return std::nullopt;
    std::string bits;
    const auto appendWildcard = [&](char digit, std::size_t count) -> bool {
        const bool wildcard = digit == '?' || digit == 'z' || (casex && digit == 'x');
        if (wildcard) {
            bits.append(count, '?');
            return true;
        }
        if (digit == 'x' || digit == 'z') return false;
        return true;
    };
    for (std::size_t position = basePosition; position < text.size(); ++position) {
        const char digit = text[position];
        if (base == 'b') {
            if (digit == '0' || digit == '1') bits.push_back(digit);
            else if (!appendWildcard(digit, 1)) return std::nullopt;
        } else {
            if (digit == '?' || digit == 'x' || digit == 'z') {
                if (!appendWildcard(digit, 4)) return std::nullopt;
                continue;
            }
            unsigned value = 0;
            if (digit >= '0' && digit <= '9') value = static_cast<unsigned>(digit - '0');
            else if (digit >= 'a' && digit <= 'f') value = 10U + static_cast<unsigned>(digit - 'a');
            else return std::nullopt;
            for (int bit = 3; bit >= 0; --bit) bits.push_back((value & (1U << bit)) ? '1' : '0');
        }
    }
    if (bits.size() > *width) bits.erase(0, bits.size() - *width);
    if (bits.size() < *width) bits.insert(0, *width - bits.size(), '0');
    return WildcardPattern{std::move(bits), tokens[begin].text,
                           location(source, tokens[begin])};
}

bool wildcardPatternsOverlap(const WildcardPattern& left, const WildcardPattern& right) {
    if (left.bits.size() != right.bits.size()) return false;
    for (std::size_t bit = 0; bit < left.bits.size(); ++bit) {
        if (left.bits[bit] != '?' && right.bits[bit] != '?' &&
            left.bits[bit] != right.bits[bit]) return false;
    }
    return true;
}

void appendWildcardOverlap(std::vector<WildcardPattern>& patterns,
                           const std::vector<HdlToken>& tokens,
                           std::size_t begin, std::size_t end, bool casex,
                           const core::SourceFile& source,
                           std::vector<SemanticIssue>& issues,
                           const std::string& unitName) {
    const auto pattern = parseWildcardPattern(tokens, begin, end, casex, source);
    if (!pattern) return;
    for (const auto& previous : patterns) {
        if (!wildcardPatternsOverlap(previous, *pattern)) continue;
        issues.push_back({"VL-OVERLAPPING-WILDCARD-CASE",
            "Wildcard case pattern overlaps an earlier choice: " + pattern->display +
                " overlaps " + previous.display,
            "Different casez/casex choices can match at least one common value, so source order can hide the later branch.",
            "Make wildcard choices mutually exclusive or use explicit comparisons with documented priority.",
            pattern->location, unitName, std::nullopt});
        break;
    }
    patterns.push_back(*pattern);
}

std::optional<CaseSelectorShape> verilogCaseSelectorShape(
    const std::vector<HdlToken>& tokens, std::size_t begin, std::size_t end,
    const std::map<std::string, SignalState>& signals) {
    while (begin < end && tokens[begin].text == "(") {
        const auto after = skipBalanced(tokens, begin, end, "(", ")");
        if (!after || *after != end) break;
        ++begin;
        --end;
    }
    if (begin >= end || !identifier(tokens[begin])) return std::nullopt;
    const auto signal = signals.find(tokens[begin].text);
    if (signal == signals.end() || signal->second.memory || !signal->second.width) {
        return std::nullopt;
    }
    if (begin + 1 == end) return CaseSelectorShape{*signal->second.width, signal->first};
    if (begin + 4 == end && tokens[begin + 1].text == "[" &&
        tokens[begin + 2].kind == HdlTokenKind::Number &&
        tokens[begin + 3].text == "]" &&
        verilogIntegerValue(tokens[begin + 2].text)) {
        return CaseSelectorShape{1, signal->first + "[" +
            tokens[begin + 2].text + "]"};
    }
    if (begin + 6 == end && tokens[begin + 1].text == "[" &&
        tokens[begin + 2].kind == HdlTokenKind::Number &&
        tokens[begin + 3].text == ":" &&
        tokens[begin + 4].kind == HdlTokenKind::Number &&
        tokens[begin + 5].text == "]") {
        const auto left = verilogIntegerValue(tokens[begin + 2].text);
        const auto right = verilogIntegerValue(tokens[begin + 4].text);
        if (left && right) {
            return CaseSelectorShape{
                *left > *right ? *left - *right + 1 : *right - *left + 1,
                signal->first + "[" + tokens[begin + 2].text + ":" +
                    tokens[begin + 4].text + "]"};
        }
    }
    return std::nullopt;
}

std::optional<CaseSelectorShape> vhdlCaseSelectorShape(
    const std::vector<HdlToken>& tokens, std::size_t begin, std::size_t end,
    const std::map<std::string, SignalState>& signals) {
    while (begin < end && tokens[begin].text == "(") {
        const auto after = skipBalanced(tokens, begin, end, "(", ")");
        if (!after || *after != end) break;
        ++begin;
        --end;
    }
    if (begin >= end || !identifier(tokens[begin])) return std::nullopt;
    const auto signal = signals.find(lower(tokens[begin].text));
    if (signal == signals.end() || signal->second.memory || !signal->second.width) {
        return std::nullopt;
    }
    if (begin + 1 == end) return CaseSelectorShape{*signal->second.width, signal->first};
    if (begin + 4 == end && tokens[begin + 1].text == "(" &&
        tokens[begin + 2].kind == HdlTokenKind::Number &&
        tokens[begin + 3].text == ")" && decimalValue(tokens[begin + 2].text)) {
        return CaseSelectorShape{1, signal->first + "(" +
            tokens[begin + 2].text + ")"};
    }
    if (begin + 6 == end && tokens[begin + 1].text == "(" &&
        tokens[begin + 2].kind == HdlTokenKind::Number &&
        (tokenIs(tokens[begin + 3], "downto") || tokenIs(tokens[begin + 3], "to")) &&
        tokens[begin + 4].kind == HdlTokenKind::Number &&
        tokens[begin + 5].text == ")") {
        const auto left = decimalValue(tokens[begin + 2].text);
        const auto right = decimalValue(tokens[begin + 4].text);
        if (left && right) {
            return CaseSelectorShape{
                *left > *right ? *left - *right + 1 : *right - *left + 1,
                signal->first + "(" + tokens[begin + 2].text + " " +
                    lower(tokens[begin + 3].text) + " " + tokens[begin + 4].text + ")"};
        }
    }
    return std::nullopt;
}

std::optional<CaseSelectorShape> vhdlDirectIdentifierShape(
    const std::vector<HdlToken>& tokens, std::size_t begin, std::size_t end,
    const std::map<std::string, SignalState>& signals) {
    while (begin < end && tokens[begin].text == "(") {
        const auto after = skipBalanced(tokens, begin, end, "(", ")");
        if (!after || *after != end) break;
        ++begin;
        --end;
    }
    if (begin + 1 != end || !identifier(tokens[begin])) return std::nullopt;
    return vhdlCaseSelectorShape(tokens, begin, end, signals);
}

std::optional<std::int64_t> vhdlNativeStaticIntegerExpression(
    const std::vector<HdlToken>& tokens, std::size_t begin, std::size_t end,
    const VhdlStaticIntegerMap& values) {
    std::size_t cursor = begin;
    std::function<std::optional<std::int64_t>()> parseAddSub;
    std::function<std::optional<std::int64_t>()> parseMulDiv;
    std::function<std::optional<std::int64_t>()> parseUnary;
    std::function<std::optional<std::int64_t>()> parsePrimary;

    const auto checked = [](const __int128 value) -> std::optional<std::int64_t> {
        if (value < std::numeric_limits<std::int64_t>::min() ||
            value > std::numeric_limits<std::int64_t>::max()) return std::nullopt;
        return static_cast<std::int64_t>(value);
    };

    parsePrimary = [&]() -> std::optional<std::int64_t> {
        if (cursor >= end) return std::nullopt;
        if (tokens[cursor].text == "(") {
            ++cursor;
            const auto value = parseAddSub();
            if (!value || cursor >= end || tokens[cursor].text != ")") {
                return std::nullopt;
            }
            ++cursor;
            return value;
        }
        if (tokens[cursor].kind == HdlTokenKind::Number) {
            const auto raw = decimalValue(tokens[cursor++].text);
            if (!raw || *raw > static_cast<std::size_t>(
                    std::numeric_limits<std::int64_t>::max())) return std::nullopt;
            return static_cast<std::int64_t>(*raw);
        }
        if (!identifier(tokens[cursor])) return std::nullopt;
        const auto found = values.find(lower(tokens[cursor++].text));
        if (found == values.end()) return std::nullopt;
        return found->second;
    };
    parseUnary = [&]() -> std::optional<std::int64_t> {
        if (cursor < end && (tokens[cursor].text == "+" || tokens[cursor].text == "-")) {
            const bool negative = tokens[cursor++].text == "-";
            const auto value = parseUnary();
            if (!value) return std::nullopt;
            return negative ? checked(-static_cast<__int128>(*value)) : value;
        }
        return parsePrimary();
    };
    parseMulDiv = [&]() -> std::optional<std::int64_t> {
        auto value = parseUnary();
        if (!value) return std::nullopt;
        while (cursor < end &&
               (tokens[cursor].text == "*" || tokens[cursor].text == "/")) {
            const bool divide = tokens[cursor++].text == "/";
            const auto right = parseUnary();
            if (!right || (divide && *right == 0)) return std::nullopt;
            value = divide ? checked(static_cast<__int128>(*value) / *right)
                           : checked(static_cast<__int128>(*value) * *right);
            if (!value) return std::nullopt;
        }
        return value;
    };
    parseAddSub = [&]() -> std::optional<std::int64_t> {
        auto value = parseMulDiv();
        if (!value) return std::nullopt;
        while (cursor < end &&
               (tokens[cursor].text == "+" || tokens[cursor].text == "-")) {
            const bool subtract = tokens[cursor++].text == "-";
            const auto right = parseMulDiv();
            if (!right) return std::nullopt;
            value = subtract ? checked(static_cast<__int128>(*value) - *right)
                             : checked(static_cast<__int128>(*value) + *right);
            if (!value) return std::nullopt;
        }
        return value;
    };

    const auto value = parseAddSub();
    if (!value || cursor != end ||
        *value < std::numeric_limits<std::int32_t>::min() ||
        *value > std::numeric_limits<std::int32_t>::max()) return std::nullopt;
    return value;
}

void addVhdlNativeStaticIntegerDeclaration(
    const std::vector<HdlToken>& tokens, std::size_t first, std::size_t last,
    VhdlStaticIntegerMap& values) {
    std::optional<std::size_t> colon;
    std::optional<std::size_t> assignment;
    int depth = 0;
    for (std::size_t token = first; token < last; ++token) {
        if (tokens[token].text == "(") ++depth;
        else if (tokens[token].text == ")") --depth;
        else if (depth == 0 && tokens[token].text == ":") {
            if (token + 1 < last && tokens[token + 1].text == "=") {
                assignment = token;
                ++token;
            } else if (!colon) {
                colon = token;
            }
        }
    }
    if (!colon || !assignment || *colon >= *assignment ||
        *assignment + 2 >= last) return;
    const auto value = vhdlNativeStaticIntegerExpression(
        tokens, *assignment + 2, last, values);
    if (!value) return;
    for (std::size_t name = first; name < *colon; ++name) {
        if (identifier(tokens[name])) values[lower(tokens[name].text)] = *value;
    }
}

void appendVhdlNativeStaticIntegerDeclarations(
    const std::vector<HdlToken>& tokens, std::size_t begin, std::size_t end,
    VhdlStaticIntegerMap& values) {
    for (std::size_t token = begin; token < end; ++token) {
        if (!tokenIs(tokens[token], "constant")) continue;
        std::size_t declarationEnd = token + 1;
        while (declarationEnd < end && tokens[declarationEnd].text != ";") {
            ++declarationEnd;
        }
        if (declarationEnd < end) {
            addVhdlNativeStaticIntegerDeclaration(
                tokens, token + 1, declarationEnd, values);
            token = declarationEnd;
        }
    }
}

VhdlStaticIntegerMap vhdlNativeStaticIntegerValues(
    const std::vector<HdlToken>& tokens, std::size_t architecture,
    std::size_t architectureEnd,
    std::string_view entityName) {
    VhdlStaticIntegerMap values;
    // FUN_180600050 visits the matching entity generic container before the
    // architecture descendants and writes evaluable defaults into +0x1a8.
    for (std::size_t entity = 0; entity + 2 < architecture; ++entity) {
        if (!tokenIs(tokens[entity], "entity") ||
            !equalInsensitive(tokens[entity + 1].text, entityName) ||
            !tokenIs(tokens[entity + 2], "is")) continue;
        std::size_t entityEnd = entity + 3;
        while (entityEnd < architecture &&
               !(tokenIs(tokens[entityEnd], "end") && entityEnd + 1 < architecture &&
                 (tokenIs(tokens[entityEnd + 1], "entity") ||
                  equalInsensitive(tokens[entityEnd + 1].text, entityName) ||
                  tokens[entityEnd + 1].text == ";"))) ++entityEnd;
        for (std::size_t token = entity + 3; token + 1 < entityEnd; ++token) {
            if (!tokenIs(tokens[token], "generic") ||
                tokens[token + 1].text != "(") continue;
            const auto after = skipBalanced(
                tokens, token + 1, entityEnd, "(", ")");
            if (!after) break;
            std::size_t declaration = token + 2;
            int depth = 0;
            for (std::size_t cursor = declaration; cursor + 1 < *after; ++cursor) {
                if (tokens[cursor].text == "(") ++depth;
                else if (tokens[cursor].text == ")") --depth;
                if (tokens[cursor].text != ";" || depth != 0) continue;
                addVhdlNativeStaticIntegerDeclaration(
                    tokens, declaration, cursor, values);
                declaration = cursor + 1;
            }
            if (declaration < *after - 1) {
                addVhdlNativeStaticIntegerDeclaration(
                    tokens, declaration, *after - 1, values);
            }
            break;
        }
        break;
    }

    // Native recursive traversal subsequently visits every AST_LOCALPARAM-like
    // VHDL constant below this architecture root before collecting width
    // comparisons. Preserve source/preorder overwrite order across architecture,
    // block, process and subprogram declarative regions, but do not cross into a
    // separate package or architecture root.
    appendVhdlNativeStaticIntegerDeclarations(
        tokens, architecture + 4, architectureEnd, values);
    return values;
}

std::string expressionText(const std::vector<HdlToken>& tokens,
                           std::size_t begin, std::size_t end) {
    std::string result;
    for (std::size_t token = begin; token < end; ++token) {
        result += tokens[token].text;
    }
    return result;
}

std::optional<CaseSelectorShape> verilogConcatenationShape(
    const std::vector<HdlToken>& tokens, std::size_t begin, std::size_t end,
    const std::map<std::string, SignalState>& signals) {
    const auto originalBegin = begin;
    const auto originalEnd = end;
    while (begin < end && tokens[begin].text == "(") {
        const auto after = skipBalanced(tokens, begin, end, "(", ")");
        if (!after || *after != end) break;
        ++begin;
        --end;
    }
    if (begin >= end || tokens[begin].text != "{") return std::nullopt;
    const auto after = skipBalanced(tokens, begin, end, "{", "}");
    if (!after || *after != end) return std::nullopt;
    std::function<std::optional<std::size_t>(std::size_t, std::size_t)> widthOf;
    widthOf = [&](std::size_t first, std::size_t last)
        -> std::optional<std::size_t> {
        while (first < last && tokens[first].text == "(") {
            const auto close = skipBalanced(tokens, first, last, "(", ")");
            if (!close || *close != last) break;
            ++first;
            --last;
        }
        if (const auto signal = verilogCaseSelectorShape(
                tokens, first, last, signals)) return signal->width;
        if (first + 1 == last &&
            tokens[first].kind == HdlTokenKind::Number) {
            return verilogLiteralDeclaredWidth(tokens[first].text);
        }
        if (first >= last || tokens[first].text != "{") return std::nullopt;
        const auto close = skipBalanced(tokens, first, last, "{", "}");
        if (!close || *close != last) return std::nullopt;
        const auto outerClose = last - 1;
        const auto repeatToken = first + 1;
        const auto repeatedOpen = repeatToken + 1;
        if (repeatedOpen < outerClose &&
            tokens[repeatToken].kind == HdlTokenKind::Number &&
            tokens[repeatedOpen].text == "{") {
            const auto repeatedClose = skipBalanced(
                tokens, repeatedOpen, outerClose, "{", "}");
            const auto repeatCount = verilogIntegerValue(tokens[repeatToken].text);
            if (repeatedClose && *repeatedClose == outerClose &&
                repeatCount && *repeatCount > 0) {
                const auto repeatedWidth = widthOf(repeatedOpen, outerClose);
                if (!repeatedWidth ||
                    *repeatedWidth > std::numeric_limits<std::size_t>::max() /
                                         *repeatCount) {
                    return std::nullopt;
                }
                return *repeatedWidth * *repeatCount;
            }
        }
        std::size_t total = 0;
        std::size_t item = first + 1;
        int depth = 0;
        for (std::size_t token = item; token <= last - 1; ++token) {
            if (token < last - 1 &&
                (tokens[token].text == "(" || tokens[token].text == "[" ||
                 tokens[token].text == "{")) ++depth;
            else if (token < last - 1 &&
                     (tokens[token].text == ")" || tokens[token].text == "]" ||
                      tokens[token].text == "}")) --depth;
            if (token != last - 1 &&
                !(tokens[token].text == "," && depth == 0)) continue;
            const auto itemEnd = token;
            if (item >= itemEnd) return std::nullopt;
            const auto width = widthOf(item, itemEnd);
            if (!width || *width > std::numeric_limits<std::size_t>::max() - total) {
                return std::nullopt;
            }
            total += *width;
            item = token + 1;
        }
        return total > 0 ? std::optional<std::size_t>(total) : std::nullopt;
    };
    const auto width = widthOf(begin, end);
    return width ? std::optional<CaseSelectorShape>(CaseSelectorShape{
                       *width, expressionText(tokens, originalBegin, originalEnd)})
                 : std::nullopt;
}

bool verilogNativeCarryExemptConcat(
    const std::vector<HdlToken>& tokens, std::size_t begin, std::size_t end,
    const std::map<std::string, SignalState>& signals) {
    while (begin < end && tokens[begin].text == "(") {
        const auto after = skipBalanced(tokens, begin, end, "(", ")");
        if (!after || *after != end) break;
        ++begin;
        --end;
    }
    if (begin >= end || tokens[begin].text != "{") return false;
    const auto after = skipBalanced(tokens, begin, end, "{", "}");
    if (!after || *after != end) return false;

    // A normal concat has at least one top-level comma. Replication is an
    // AST_REPLICATE and must not receive the AST_CONCAT exemption.
    std::optional<std::size_t> firstComma;
    int parentheses = 0;
    int brackets = 0;
    int braces = 0;
    for (std::size_t token = begin + 1; token + 1 < end; ++token) {
        const auto& text = tokens[token].text;
        if (text == "(") ++parentheses;
        else if (text == ")") --parentheses;
        else if (text == "[") ++brackets;
        else if (text == "]") --brackets;
        else if (text == "{") ++braces;
        else if (text == "}") --braces;
        else if (text == "," && parentheses == 0 && brackets == 0 && braces == 0) {
            firstComma = token;
            break;
        }
    }
    if (!firstComma || begin + 1 == *firstComma) return false;

    std::size_t first = begin + 1;
    std::size_t last = *firstComma;
    while (first < last && tokens[first].text == "(") {
        const auto close = skipBalanced(tokens, first, last, "(", ")");
        if (!close || *close != last) break;
        ++first;
        --last;
    }
    if (first + 1 == last && tokens[first].kind == HdlTokenKind::Number) {
        const auto value = verilogIntegerValue(tokens[first].text);
        return value && *value == 0;
    }

    // For identifiers, selects, and nested structural expressions the native
    // AstNode bits vector is empty. The DLL deliberately treats empty exactly
    // like an all-S0 vector. Restrict the product counterpart to shapes whose
    // width we can also prove statically.
    return comparisonOperandShape(tokens, first, last, signals, false).has_value();
}

std::optional<CaseSelectorShape> vhdlConcatenationShape(
    const std::vector<HdlToken>& tokens, std::size_t begin, std::size_t end,
    const std::map<std::string, SignalState>& signals) {
    const auto originalBegin = begin;
    const auto originalEnd = end;
    std::function<std::optional<std::size_t>(std::size_t, std::size_t)> widthOf;
    widthOf = [&](std::size_t first, std::size_t last)
        -> std::optional<std::size_t> {
        while (first < last && tokens[first].text == "(") {
            const auto after = skipBalanced(tokens, first, last, "(", ")");
            if (!after || *after != last) break;
            ++first;
            --last;
        }
        if (const auto signal = vhdlCaseSelectorShape(
                tokens, first, last, signals)) return signal->width;
        if (first + 1 == last) return vhdlStringLiteralWidth(tokens[first]);
        if (const auto character = vhdlLogicCharacterLiteralWidth(
                tokens, first, last)) return character;
        std::size_t total = 0;
        std::size_t item = first;
        int depth = 0;
        bool sawConcat = false;
        for (std::size_t token = first; token <= last; ++token) {
            if (token < last && tokens[token].text == "(") ++depth;
            else if (token < last && tokens[token].text == ")") --depth;
            if (token < last && !(tokens[token].text == "&" && depth == 0)) continue;
            if (token < last) sawConcat = true;
            if (token == last && !sawConcat) return std::nullopt;
            const auto itemEnd = token;
            if (item >= itemEnd) return std::nullopt;
            const auto width = widthOf(item, itemEnd);
            if (!width || *width > std::numeric_limits<std::size_t>::max() - total) {
                return std::nullopt;
            }
            total += *width;
            item = token + 1;
        }
        return sawConcat && total > 0 ? std::optional<std::size_t>(total)
                                      : std::nullopt;
    };
    const auto width = widthOf(begin, end);
    return width ? std::optional<CaseSelectorShape>(CaseSelectorShape{
                       *width, expressionText(tokens, originalBegin, originalEnd)})
                 : std::nullopt;
}

std::optional<std::size_t> verilogShiftOperandBegin(
    const std::vector<HdlToken>& tokens, std::size_t end) {
    if (end > 0 && identifier(tokens[end - 1])) return end - 1;
    if (end >= 4 && tokens[end - 1].text == "]" &&
        tokens[end - 3].text == "[" && identifier(tokens[end - 4])) return end - 4;
    if (end >= 6 && tokens[end - 1].text == "]" &&
        tokens[end - 3].text == ":" && tokens[end - 5].text == "[" &&
        identifier(tokens[end - 6])) return end - 6;
    return std::nullopt;
}

std::optional<std::size_t> vhdlShiftOperandBegin(
    const std::vector<HdlToken>& tokens, std::size_t end) {
    if (end > 0 && identifier(tokens[end - 1])) return end - 1;
    if (end >= 4 && tokens[end - 1].text == ")" &&
        tokens[end - 3].text == "(" && identifier(tokens[end - 4])) return end - 4;
    if (end >= 6 && tokens[end - 1].text == ")" &&
        (tokenIs(tokens[end - 3], "downto") || tokenIs(tokens[end - 3], "to")) &&
        tokens[end - 5].text == "(" && identifier(tokens[end - 6])) return end - 6;
    return std::nullopt;
}

void appendShiftAmountIssue(const core::SourceFile& source,
                            const HdlToken& operatorToken,
                            const CaseSelectorShape& operand,
                            std::size_t amount,
                            std::vector<SemanticIssue>& issues,
                            const std::string& unitName) {
    if (amount < operand.width) return;
    issues.push_back({"RTL-SHIFT-AMOUNT-OUT-OF-RANGE",
        "Constant shift amount is not smaller than operand width",
        "Selector " + operand.display + " is " + std::to_string(operand.width) +
            " bits wide but is shifted by " + std::to_string(amount) +
            ", so every original bit is shifted out of the result.",
        "Use a shift amount smaller than the operand width or resize the operand and document the intended behavior.",
        location(source, operatorToken), unitName, operand.display});
}

void appendVerilogShiftAmountIssues(
    const core::SourceFile& source, const std::vector<HdlToken>& tokens,
    std::size_t body, std::size_t end,
    const std::map<std::string, SignalState>& signals,
    std::vector<SemanticIssue>& issues, const std::string& unitName) {
    for (std::size_t token = body; token + 2 < end; ++token) {
        if ((tokens[token].text != "<" && tokens[token].text != ">") ||
            tokens[token + 1].text != tokens[token].text) continue;
        std::size_t operatorWidth = 2;
        if (token + 2 < end && tokens[token + 2].text == tokens[token].text) {
            operatorWidth = 3;
        }
        std::size_t amountToken = token + operatorWidth;
        if (amountToken < end && tokens[amountToken].text == "=") ++amountToken;
        if (amountToken >= end || tokens[amountToken].kind != HdlTokenKind::Number) continue;
        const auto amount = verilogIntegerValue(tokens[amountToken].text);
        const auto operandBegin = verilogShiftOperandBegin(tokens, token);
        if (!amount || !operandBegin) continue;
        const auto operand = verilogCaseSelectorShape(tokens, *operandBegin, token, signals);
        if (operand) appendShiftAmountIssue(
            source, tokens[token], *operand, *amount, issues, unitName);
        token = amountToken;
    }
}

void appendVhdlShiftAmountIssues(
    const core::SourceFile& source, const std::vector<HdlToken>& tokens,
    std::size_t body, std::size_t end,
    const std::map<std::string, SignalState>& signals,
    std::vector<SemanticIssue>& issues, const std::string& unitName) {
    for (std::size_t token = body; token < end; ++token) {
        if (tokenIs(tokens[token], "sll") || tokenIs(tokens[token], "srl") ||
            tokenIs(tokens[token], "sla") || tokenIs(tokens[token], "sra")) {
            if (token + 1 >= end || tokens[token + 1].kind != HdlTokenKind::Number) continue;
            const auto amount = decimalValue(tokens[token + 1].text);
            const auto operandBegin = vhdlShiftOperandBegin(tokens, token);
            if (!amount || !operandBegin) continue;
            const auto operand = vhdlCaseSelectorShape(tokens, *operandBegin, token, signals);
            if (operand) appendShiftAmountIssue(
                source, tokens[token], *operand, *amount, issues, unitName);
            ++token;
            continue;
        }
        if (!tokenIs(tokens[token], "shift_left") &&
            !tokenIs(tokens[token], "shift_right")) continue;
        if (token + 5 >= end || tokens[token + 1].text != "(") continue;
        const auto after = skipBalanced(tokens, token + 1, end, "(", ")");
        if (!after) continue;
        int depth = 0;
        std::optional<std::size_t> comma;
        for (std::size_t argument = token + 2; argument + 1 < *after; ++argument) {
            if (tokens[argument].text == "(") ++depth;
            else if (tokens[argument].text == ")") --depth;
            else if (tokens[argument].text == "," && depth == 0) {
                comma = argument;
                break;
            }
        }
        if (!comma || *comma + 2 != *after - 1 ||
            tokens[*comma + 1].kind != HdlTokenKind::Number) continue;
        const auto amount = decimalValue(tokens[*comma + 1].text);
        if (!amount) continue;
        const auto operand = vhdlCaseSelectorShape(
            tokens, token + 2, *comma, signals);
        if (operand) appendShiftAmountIssue(
            source, tokens[token], *operand, *amount, issues, unitName);
        token = *after - 1;
    }
}

bool shiftAmountEndsAt(const std::vector<HdlToken>& tokens,
                       std::size_t position, std::size_t limit) {
    if (position >= limit) return true;
    const auto& text = tokens[position].text;
    return text == ";" || text == "," || text == ")" || text == "]" ||
           text == "}" || text == "?" || text == ":" ||
           tokenIs(tokens[position], "then") || tokenIs(tokens[position], "loop") ||
           tokenIs(tokens[position], "end") || tokenIs(tokens[position], "else");
}

std::optional<std::size_t> negativeShiftLiteral(
    const std::vector<HdlToken>& tokens, std::size_t begin, std::size_t limit,
    bool vhdl) {
    if (begin >= limit) return std::nullopt;
    if (tokens[begin].text == "(") {
        const auto after = skipBalanced(tokens, begin, limit, "(", ")");
        if (!after || !shiftAmountEndsAt(tokens, *after, limit)) {
            return std::nullopt;
        }
        return negativeLiteralAt(tokens, begin, *after, vhdl);
    }
    if (begin + 2 > limit || !shiftAmountEndsAt(tokens, begin + 2, limit)) {
        return std::nullopt;
    }
    return negativeLiteralAt(tokens, begin, begin + 2, vhdl);
}

void appendNegativeShiftIssue(const core::SourceFile& source,
                              const HdlToken& operatorToken,
                              const HdlToken& literalToken,
                              std::vector<SemanticIssue>& issues,
                              const std::string& unitName) {
    issues.push_back({"RTL-NEGATIVE-SHIFT-AMOUNT",
        "Shift operation uses a negative constant amount",
        "A shift count of -" + literalToken.text +
            " reverses direction, becomes an unexpectedly large count, or violates the operator contract depending on the HDL and operator.",
        "Use a nonnegative shift count and select the intended shift direction explicitly.",
        location(source, operatorToken), unitName, std::nullopt});
}

void appendVerilogNegativeShiftIssues(
    const core::SourceFile& source, const std::vector<HdlToken>& tokens,
    std::size_t body, std::size_t end,
    std::vector<SemanticIssue>& issues, const std::string& unitName) {
    for (std::size_t token = body; token + 2 < end; ++token) {
        if ((tokens[token].text != "<" && tokens[token].text != ">") ||
            tokens[token + 1].text != tokens[token].text) continue;
        std::size_t operatorWidth = 2;
        if (token + 2 < end && tokens[token + 2].text == tokens[token].text) {
            operatorWidth = 3;
        }
        std::size_t amount = token + operatorWidth;
        if (amount < end && tokens[amount].text == "=") ++amount;
        const auto literal = negativeShiftLiteral(tokens, amount, end, false);
        if (literal) appendNegativeShiftIssue(
            source, tokens[token], tokens[*literal], issues, unitName);
        token += operatorWidth - 1;
    }
}

void appendVhdlNegativeShiftIssues(
    const core::SourceFile& source, const std::vector<HdlToken>& tokens,
    std::size_t body, std::size_t end,
    std::vector<SemanticIssue>& issues, const std::string& unitName) {
    for (std::size_t token = body; token < end; ++token) {
        if (tokenIs(tokens[token], "sll") || tokenIs(tokens[token], "srl") ||
            tokenIs(tokens[token], "sla") || tokenIs(tokens[token], "sra")) {
            const auto literal = negativeShiftLiteral(tokens, token + 1, end, true);
            if (literal) appendNegativeShiftIssue(
                source, tokens[token], tokens[*literal], issues, unitName);
            continue;
        }
        if (!tokenIs(tokens[token], "shift_left") &&
            !tokenIs(tokens[token], "shift_right")) continue;
        if (token + 1 >= end || tokens[token + 1].text != "(") continue;
        const auto after = skipBalanced(tokens, token + 1, end, "(", ")");
        if (!after) continue;
        int depth = 0;
        std::optional<std::size_t> comma;
        for (std::size_t argument = token + 2; argument + 1 < *after; ++argument) {
            if (tokens[argument].text == "(") ++depth;
            else if (tokens[argument].text == ")") --depth;
            else if (tokens[argument].text == "," && depth == 0) {
                comma = argument;
                break;
            }
        }
        if (!comma) continue;
        const auto literal = negativeLiteralAt(
            tokens, *comma + 1, *after - 1, true);
        if (literal) appendNegativeShiftIssue(
            source, tokens[token], tokens[*literal], issues, unitName);
        token = *after - 1;
    }
}

void appendVerilogCaseIssues(const core::SourceFile& source,
                             const std::vector<HdlToken>& tokens,
                             std::size_t body, std::size_t end,
                             const std::map<std::string, SignalState>& signals,
                             std::vector<SemanticIssue>& issues,
                             const std::string& unitName) {
    for (std::size_t caseToken = body; caseToken < end; ++caseToken) {
        if (!tokenIs(tokens[caseToken], "case") && !tokenIs(tokens[caseToken], "casez") &&
            !tokenIs(tokens[caseToken], "casex")) continue;
        if (tokenIs(tokens[caseToken], "casex")) {
            issues.push_back({"VL-CASEX", "casex can hide unknown-state bugs",
                "casex treats X and Z bits as don't-care values in both the selector and case items.",
                "Use case, casez, or explicit masked comparisons.",
                location(source, tokens[caseToken]), unitName, std::nullopt});
        }
        std::size_t condition = caseToken + 1;
        if (condition >= end || tokens[condition].text != "(") continue;
        const auto afterCondition = skipBalanced(tokens, condition, end, "(", ")");
        if (!afterCondition) continue;
        const auto selectorShape = verilogCaseSelectorShape(
            tokens, condition + 1, *afterCondition - 1, signals);
        int caseDepth = 1;
        std::size_t caseEnd = *afterCondition;
        for (; caseEnd < end; ++caseEnd) {
            if (tokenIs(tokens[caseEnd], "case") || tokenIs(tokens[caseEnd], "casez") ||
                tokenIs(tokens[caseEnd], "casex")) ++caseDepth;
            else if (tokenIs(tokens[caseEnd], "endcase") && --caseDepth == 0) break;
        }
        if (caseEnd >= end) continue;
        const bool wildcardCase = tokenIs(tokens[caseToken], "casez") ||
                                  tokenIs(tokens[caseToken], "casex");
        const bool casex = tokenIs(tokens[caseToken], "casex");
        std::map<std::string, core::SourceLocation> choices;
        std::vector<WildcardPattern> patterns;
        bool hasDefault = false;
        std::size_t cursor = *afterCondition;
        while (cursor < caseEnd) {
            while (cursor < caseEnd && tokens[cursor].text == ";") ++cursor;
            if (cursor >= caseEnd) break;
            const auto labelsBegin = cursor;
            int delimiters = 0;
            while (cursor < caseEnd) {
                if (tokens[cursor].text == "(" || tokens[cursor].text == "[" ||
                    tokens[cursor].text == "{") ++delimiters;
                else if (tokens[cursor].text == ")" || tokens[cursor].text == "]" ||
                         tokens[cursor].text == "}") --delimiters;
                else if (tokens[cursor].text == ":" && delimiters == 0) break;
                ++cursor;
            }
            if (cursor >= caseEnd) break;
            std::size_t choiceBegin = labelsBegin;
            int choiceDelimiters = 0;
            for (std::size_t token = labelsBegin; token <= cursor; ++token) {
                if (token < cursor && (tokens[token].text == "(" || tokens[token].text == "[" ||
                                       tokens[token].text == "{")) ++choiceDelimiters;
                else if (token < cursor && (tokens[token].text == ")" || tokens[token].text == "]" ||
                                            tokens[token].text == "}")) --choiceDelimiters;
                if (token == cursor || (tokens[token].text == "," && choiceDelimiters == 0)) {
                    if (choiceBegin < token && tokenIs(tokens[choiceBegin], "default")) {
                        if (hasDefault) {
                            issues.push_back({"RTL-MULTIPLE-CASE-DEFAULT",
                                "Case statement contains more than one default item",
                                "Only one default item is legal in a case statement; every later default is unreachable and makes the case definition ambiguous or invalid.",
                                "Keep one default item and merge or remove the duplicate catch-all behavior.",
                                location(source, tokens[choiceBegin]), unitName, std::nullopt});
                        }
                        hasDefault = true;
                    }
                    if (selectorShape && choiceBegin + 1 == token &&
                        tokens[choiceBegin].kind == HdlTokenKind::Number) {
                        const auto choiceWidth = verilogLiteralDeclaredWidth(
                            tokens[choiceBegin].text);
                        if (choiceWidth && *choiceWidth != selectorShape->width) {
                            issues.push_back({"RTL-CASE-ITEM-WIDTH-MISMATCH",
                                "Case item width does not match selector " + selectorShape->display,
                                "The explicitly sized case item is " + std::to_string(*choiceWidth) +
                                    " bits wide while selector " + selectorShape->display + " is " +
                                    std::to_string(selectorShape->width) +
                                    " bits wide, so extension or truncation changes matching semantics.",
                                "Resize the case item to the selector width or explicitly resize the selector to document the intended comparison.",
                                location(source, tokens[choiceBegin]), unitName,
                                selectorShape->display});
                        }
                        if (!choiceWidth &&
                            selectorShape->width <
                                std::numeric_limits<unsigned int>::digits - 1) {
                            const auto choiceValue = verilogIntegerValue(
                                tokens[choiceBegin].text);
                            const auto limit = std::size_t{1} << selectorShape->width;
                            if (choiceValue && *choiceValue >= limit &&
                                *choiceValue <=
                                    static_cast<std::size_t>(
                                        std::numeric_limits<int>::max())) {
                                issues.push_back({"VL-CASE-ITEM-VALUE-OUT-OF-RANGE",
                                    "Case item value cannot be represented by selector " +
                                        selectorShape->display,
                                    "The unsized case item value " +
                                        tokens[choiceBegin].text + " is outside the 0.." +
                                        std::to_string(limit - 1) + " bit-pattern range of " +
                                        std::to_string(selectorShape->width) +
                                        "-bit selector " + selectorShape->display +
                                        " and cannot match it.",
                                    "Correct the case item value or resize the selector and use an explicitly sized item.",
                                    location(source, tokens[choiceBegin]), unitName,
                                    selectorShape->display});
                            }
                        }
                    }
                    const bool duplicate = appendDuplicateChoice(
                        choices, tokens, choiceBegin, token, false, source, issues, unitName);
                    if (wildcardCase && !duplicate) {
                        appendWildcardOverlap(patterns, tokens, choiceBegin, token, casex,
                                              source, issues, unitName);
                    }
                    choiceBegin = token + 1;
                }
            }
            if (const auto empty = verilogEmptyBranchToken(
                    tokens, cursor + 1, caseEnd)) {
                issues.push_back({"VL-EMPTY-CASE-ITEM",
                    "Case item has no executable statement",
                    "A case choice maps to an empty statement or structurally empty begin/end block, so selecting it performs no RTL behavior and may indicate an omitted assignment.",
                    "Implement the intended case behavior or document and locally suppress a deliberately empty choice.",
                    location(source, tokens[*empty]), unitName, std::nullopt});
            }
            const auto branch = parseVerilogFlow(tokens, cursor + 1, caseEnd, {});
            cursor = branch.next > cursor ? branch.next : cursor + 1;
        }
        if (!hasDefault) {
            issues.push_back({"VL-CASE-DEFAULT", "case statement has no default branch",
                "An uncovered selector value may infer storage or preserve stale state.",
                "Add an explicit safe default branch.",
                location(source, tokens[caseToken]), unitName, std::nullopt});
        }
    }
}

void appendVhdlDuplicateCaseIssues(const core::SourceFile& source,
                                   const std::vector<HdlToken>& tokens,
                                   std::size_t body, std::size_t end,
                                   const std::map<std::string, SignalState>& signals,
                                   std::vector<SemanticIssue>& issues,
                                   const std::string& unitName,
                                   bool caseOthersOnly = false,
                                   bool checkWidths = true) {
    for (std::size_t caseToken = body; caseToken + 1 < end; ++caseToken) {
        if (!tokenIs(tokens[caseToken], "case") ||
            (caseToken > body && tokenIs(tokens[caseToken - 1], "end"))) continue;
        int depth = 1;
        std::size_t caseEnd = caseToken + 1;
        for (; caseEnd + 1 < end; ++caseEnd) {
            if (tokenIs(tokens[caseEnd], "case") &&
                (caseEnd == 0 || !tokenIs(tokens[caseEnd - 1], "end"))) ++depth;
            if (tokenIs(tokens[caseEnd], "end") && tokenIs(tokens[caseEnd + 1], "case") &&
                --depth == 0) break;
        }
        if (caseEnd + 1 >= end) continue;
        std::size_t isToken = caseToken + 1;
        while (isToken < caseEnd && !tokenIs(tokens[isToken], "is")) ++isToken;
        // Native FUN_180601490 accepts an AST_IDENTIFIER selector.  Selected,
        // indexed and sliced names are deliberately outside this rule branch.
        const auto selectorShape = isToken < caseEnd ? vhdlDirectIdentifierShape(
            tokens, caseToken + 1, isToken, signals) : std::nullopt;
        std::map<std::string, core::SourceLocation> choices;
        bool hasOthers = false;
        depth = 1;
        for (std::size_t token = caseToken + 1; token < caseEnd; ++token) {
            if (tokenIs(tokens[token], "case") &&
                (token == 0 || !tokenIs(tokens[token - 1], "end"))) {
                ++depth;
                continue;
            }
            if (tokenIs(tokens[token], "end") && token + 1 < caseEnd &&
                tokenIs(tokens[token + 1], "case")) {
                --depth;
                continue;
            }
            if (depth != 1 || !tokenIs(tokens[token], "when")) continue;
            const auto labelsBegin = token + 1;
            std::size_t arrow = labelsBegin;
            while (arrow + 1 < caseEnd &&
                   !(tokens[arrow].text == "=" && tokens[arrow + 1].text == ">")) ++arrow;
            if (arrow + 1 >= caseEnd) break;
            std::size_t choiceBegin = labelsBegin;
            int delimiters = 0;
            for (std::size_t choice = labelsBegin; choice <= arrow; ++choice) {
                if (choice < arrow && tokens[choice].text == "(") ++delimiters;
                else if (choice < arrow && tokens[choice].text == ")") --delimiters;
                if (choice == arrow || (tokens[choice].text == "|" && delimiters == 0)) {
                    if (choiceBegin < choice && tokenIs(tokens[choiceBegin], "others")) {
                        if (hasOthers && !caseOthersOnly) {
                            issues.push_back({"RTL-MULTIPLE-CASE-DEFAULT",
                                "Case statement contains more than one when others choice",
                                "Only one when others choice is legal in a VHDL case statement; every later catch-all branch is unreachable and invalid.",
                                "Keep one when others branch and merge or remove the duplicate catch-all behavior.",
                                location(source, tokens[choiceBegin]), unitName, std::nullopt});
                        }
                        hasOthers = true;
                    }
                    if (checkWidths && selectorShape && choiceBegin + 1 == choice) {
                        auto choiceWidth = vhdlStringLiteralWidth(tokens[choiceBegin]);
                        if (!choiceWidth && identifier(tokens[choiceBegin])) {
                            if (const auto named = vhdlCaseSelectorShape(
                                    tokens, choiceBegin, choice, signals)) {
                                choiceWidth = named->width;
                            }
                        }
                        if (choiceWidth && *choiceWidth != selectorShape->width) {
                            issues.push_back({"RTL-CASE-ITEM-WIDTH-MISMATCH",
                                "Case choice width does not match selector " + selectorShape->display,
                                "The case choice is " + std::to_string(*choiceWidth) +
                                    " bits wide while selector " + selectorShape->display + " is " +
                                    std::to_string(selectorShape->width) +
                                    " bits wide, so the choice cannot represent the intended selector value.",
                                "Resize the case choice to the selector width or explicitly resize the selector to document the intended comparison.",
                                location(source, tokens[choiceBegin]), unitName,
                                selectorShape->display});
                        }
                    }
                    if (!caseOthersOnly) {
                        appendDuplicateChoice(choices, tokens, choiceBegin, choice, true,
                                              source, issues, unitName);
                    }
                    choiceBegin = choice + 1;
                }
            }
            token = arrow + 1;
        }
        if (!hasOthers) {
            // Native GJB9765.4.8.c does not prove that explicit choices exhaust
            // the selector domain. FUN_180632fc0 creates one candidate for each
            // admitted case node and FUN_180633070 clears it only for a direct
            // AST_DEFAULT (0x89) child of an AST_COND (0x58) choice group.
            issues.push_back({"VHDL-CASE-OTHERS",
                "Case statement has no when others choice",
                "GJB9765.4.8.c requires an explicit catch-all choice so unlisted selector values have defined behavior.",
                "Add a when others branch with explicit safe behavior.",
                location(source, tokens[caseToken]), unitName, std::nullopt});
        }
    }
}

struct VhdlPackageBodyRange {
    std::size_t root;
    std::size_t begin;
    std::size_t end;
    std::string name;
};

std::vector<VhdlPackageBodyRange> vhdlPackageBodyRanges(
    const std::vector<HdlToken>& tokens) {
    std::vector<VhdlPackageBodyRange> ranges;
    for (std::size_t package = 0; package + 3 < tokens.size(); ++package) {
        if (!tokenIs(tokens[package], "package") ||
            !tokenIs(tokens[package + 1], "body") ||
            !identifier(tokens[package + 2])) continue;

        const auto unitName = tokens[package + 2].text;
        std::size_t end = tokens.size();
        std::size_t lastBareEnd = tokens.size();
        for (std::size_t token = package + 3; token < tokens.size(); ++token) {
            if (token + 1 < tokens.size() && tokenIs(tokens[token], "end") &&
                tokens[token + 1].text == ";") {
                lastBareEnd = token;
            }
            if (token + 2 < tokens.size() && tokenIs(tokens[token], "end") &&
                tokenIs(tokens[token + 1], "package") &&
                tokenIs(tokens[token + 2], "body")) {
                end = token;
                break;
            }
            if (token + 2 < tokens.size() && tokenIs(tokens[token], "end") &&
                tokenIs(tokens[token + 1], unitName) && tokens[token + 2].text == ";") {
                end = token;
                break;
            }
            if (token <= package + 3 || tokens[token - 1].text != ";") continue;
            const bool nextDesignRoot =
                (tokenIs(tokens[token], "entity") && token + 2 < tokens.size() &&
                 identifier(tokens[token + 1]) && tokenIs(tokens[token + 2], "is")) ||
                (tokenIs(tokens[token], "architecture") && token + 3 < tokens.size() &&
                 identifier(tokens[token + 1]) && tokenIs(tokens[token + 2], "of")) ||
                (tokenIs(tokens[token], "package") && token + 2 < tokens.size());
            if (nextDesignRoot) {
                end = lastBareEnd < tokens.size() ? lastBareEnd : token;
                break;
            }
        }
        if (end == tokens.size() && lastBareEnd < tokens.size()) end = lastBareEnd;
        ranges.push_back({package, package + 3, end, unitName});
        package = end < tokens.size() ? end : tokens.size() - 1;
    }
    return ranges;
}

void addVhdlRootShapeDeclaration(
    const core::SourceFile& source, const std::vector<HdlToken>& tokens,
    std::size_t first, std::size_t colon, std::size_t typeBegin,
    std::size_t declarationEnd, Direction direction,
    std::map<std::string, SignalState>& shapes) {
    if (first >= colon || typeBegin >= declarationEnd) return;
    std::size_t typeEnd = declarationEnd;
    for (std::size_t token = typeBegin; token + 1 < declarationEnd; ++token) {
        if (tokens[token].text == ":" && tokens[token + 1].text == "=") {
            typeEnd = token;
            break;
        }
    }
    const auto width = vhdlTypeWidth(tokens, typeBegin, typeEnd);
    const auto range = vhdlTypeRange(tokens, typeBegin, typeEnd);
    if (!width) return;
    const bool unsignedNumeric = std::ranges::any_of(
        tokens.begin() + static_cast<std::ptrdiff_t>(typeBegin),
        tokens.begin() + static_cast<std::ptrdiff_t>(typeEnd),
        [](const HdlToken& token) { return tokenIs(token, "unsigned"); });
    for (std::size_t name = first; name < colon; ++name) {
        if (!identifier(tokens[name]) || tokenIs(tokens[name], "signal") ||
            tokenIs(tokens[name], "variable") || tokenIs(tokens[name], "constant") ||
            tokenIs(tokens[name], "file") || tokenIs(tokens[name], "shared")) continue;
        shapes[lower(tokens[name].text)] = SignalState{
            direction, location(source, tokens[name]), {}, false, false, false,
            width, range, unsignedNumeric, VerilogSignedness::Unknown,
            std::nullopt, {}};
    }
}

std::map<std::string, SignalState> vhdlRootShapes(
    const core::SourceFile& source, const std::vector<HdlToken>& tokens,
    std::size_t begin, std::size_t end,
    std::map<std::string, SignalState> signals = {},
    std::map<std::string, SignalState> ports = {},
    std::map<std::string, SignalState> generics = {}) {
    // FUN_1805fee80 does not perform one source-order lookup. It consults five
    // maps in the fixed order localparam -> port -> generic -> signal ->
    // variable. Keep those categories separate while walking the complete
    // design root, then flatten from lowest to highest priority for the product
    // expression helpers.
    std::map<std::string, SignalState> localparams;
    std::map<std::string, SignalState> variables;
    for (std::size_t token = begin; token < end; ++token) {
        if (tokenIs(tokens[token], "signal") || tokenIs(tokens[token], "variable") ||
            tokenIs(tokens[token], "constant")) {
            auto* category = tokenIs(tokens[token], "constant") ? &localparams :
                tokenIs(tokens[token], "variable") ? &variables : &signals;
            std::size_t colon = token + 1;
            while (colon < end && tokens[colon].text != ":" &&
                   tokens[colon].text != ";") ++colon;
            if (colon >= end || tokens[colon].text != ":") continue;
            std::size_t declarationEnd = colon + 1;
            while (declarationEnd < end &&
                   tokens[declarationEnd].text != ";") ++declarationEnd;
            addVhdlRootShapeDeclaration(
                source, tokens, token + 1, colon, colon + 1,
                declarationEnd, Direction::Internal, *category);
            token = declarationEnd < end ? declarationEnd : token;
            continue;
        }

        if (!tokenIs(tokens[token], "procedure") &&
            !tokenIs(tokens[token], "function")) continue;
        std::size_t open = token + 1;
        while (open < end && tokens[open].text != "(" &&
               tokens[open].text != ";" && !tokenIs(tokens[open], "is") &&
               !tokenIs(tokens[open], "return")) ++open;
        if (open >= end || tokens[open].text != "(") continue;
        const auto after = skipBalanced(tokens, open, end, "(", ")");
        if (!after) continue;
        const auto close = *after - 1;
        std::size_t segment = open + 1;
        int depth = 0;
        for (std::size_t cursor = segment; cursor <= close; ++cursor) {
            if (cursor < close && tokens[cursor].text == "(") ++depth;
            else if (cursor < close && tokens[cursor].text == ")") --depth;
            if (cursor < close && !(tokens[cursor].text == ";" && depth == 0)) continue;
            const auto segmentEnd = cursor;
            std::size_t colon = segment;
            while (colon < segmentEnd && tokens[colon].text != ":") ++colon;
            if (colon < segmentEnd) {
                Direction direction = Direction::Input;
                std::size_t typeBegin = colon + 1;
                enum class ParameterClass { Default, Constant, Signal, Variable, File };
                ParameterClass parameterClass = ParameterClass::Default;
                if (typeBegin < segmentEnd && tokenIs(tokens[typeBegin], "constant")) {
                    parameterClass = ParameterClass::Constant;
                    ++typeBegin;
                } else if (typeBegin < segmentEnd && tokenIs(tokens[typeBegin], "signal")) {
                    parameterClass = ParameterClass::Signal;
                    ++typeBegin;
                } else if (typeBegin < segmentEnd && tokenIs(tokens[typeBegin], "variable")) {
                    parameterClass = ParameterClass::Variable;
                    ++typeBegin;
                } else if (typeBegin < segmentEnd && tokenIs(tokens[typeBegin], "file")) {
                    parameterClass = ParameterClass::File;
                    ++typeBegin;
                }
                if (typeBegin < segmentEnd && tokenIs(tokens[typeBegin], "in")) {
                    direction = Direction::Input;
                    ++typeBegin;
                } else if (typeBegin < segmentEnd &&
                           (tokenIs(tokens[typeBegin], "out") ||
                            tokenIs(tokens[typeBegin], "buffer"))) {
                    direction = Direction::Output;
                    ++typeBegin;
                } else if (typeBegin < segmentEnd &&
                           tokenIs(tokens[typeBegin], "inout")) {
                    direction = Direction::Inout;
                    ++typeBegin;
                }
                // VHDL subprogram interface-object defaults lower an `in`
                // parameter as a constant and an out/inout/buffer parameter as
                // a variable. Explicit classes retain their native category.
                // FILE has no recovered width-map producer and is skipped.
                if (parameterClass != ParameterClass::File) {
                    auto* category = &variables;
                    if (parameterClass == ParameterClass::Constant ||
                        (parameterClass == ParameterClass::Default &&
                         direction == Direction::Input)) {
                        category = &localparams;
                    } else if (parameterClass == ParameterClass::Signal) {
                        category = &signals;
                    }
                    addVhdlRootShapeDeclaration(
                        source, tokens, segment, colon, typeBegin,
                        segmentEnd, direction, *category);
                }
            }
            segment = cursor + 1;
        }
        token = close;
    }

    std::map<std::string, SignalState> shapes = std::move(variables);
    const auto overlay = [&shapes](const auto& category) {
        for (const auto& [name, shape] : category) shapes.insert_or_assign(name, shape);
    };
    overlay(signals);
    overlay(generics);
    overlay(ports);
    overlay(localparams);
    return shapes;
}

std::map<std::string, SignalState> vhdlEntityGenericShapes(
    const core::SourceFile& source, const std::vector<HdlToken>& tokens,
    std::size_t architecture, std::string_view entityName) {
    std::map<std::string, SignalState> generics;
    for (std::size_t entity = 0; entity + 2 < architecture; ++entity) {
        if (!tokenIs(tokens[entity], "entity") ||
            !equalInsensitive(tokens[entity + 1].text, entityName) ||
            !tokenIs(tokens[entity + 2], "is")) continue;
        std::size_t entityEnd = entity + 3;
        while (entityEnd < architecture &&
               !(tokenIs(tokens[entityEnd], "end") &&
                 entityEnd + 1 < architecture &&
                 (tokenIs(tokens[entityEnd + 1], "entity") ||
                  equalInsensitive(tokens[entityEnd + 1].text, entityName) ||
                  tokens[entityEnd + 1].text == ";"))) ++entityEnd;
        for (std::size_t token = entity + 3; token + 1 < entityEnd; ++token) {
            if (!tokenIs(tokens[token], "generic") ||
                tokens[token + 1].text != "(") continue;
            const auto after = skipBalanced(tokens, token + 1, entityEnd, "(", ")");
            if (!after) break;
            const auto close = *after - 1;
            std::size_t declaration = token + 2;
            int depth = 0;
            for (std::size_t cursor = declaration; cursor <= close; ++cursor) {
                if (cursor < close && tokens[cursor].text == "(") ++depth;
                else if (cursor < close && tokens[cursor].text == ")") --depth;
                if (cursor < close && !(tokens[cursor].text == ";" && depth == 0)) continue;
                const auto declarationEnd = cursor;
                std::size_t colon = declaration;
                while (colon < declarationEnd && tokens[colon].text != ":") ++colon;
                if (colon < declarationEnd) {
                    addVhdlRootShapeDeclaration(
                        source, tokens, declaration, colon, colon + 1,
                        declarationEnd, Direction::Input, generics);
                }
                declaration = cursor + 1;
            }
            break;
        }
        break;
    }
    return generics;
}

void appendVhdlPackageBodyCaseOthersIssues(
    const core::SourceFile& source, const std::vector<HdlToken>& tokens,
    std::vector<SemanticIssue>& issues) {
    for (const auto& package : vhdlPackageBodyRanges(tokens)) {
        const auto shapes = vhdlRootShapes(
            source, tokens, package.begin, package.end);
        appendVhdlDuplicateCaseIssues(
            source, tokens, package.begin, package.end,
            shapes, issues, package.name, true, true);
    }
}

bool verilogCombinationalProcess(const std::vector<HdlToken>& tokens,
                                 const ProcessRange& process, std::size_t limit) {
    if (tokenIs(tokens[process.keyword], "always_comb")) return true;
    if (!tokenIs(tokens[process.keyword], "always")) return false;
    std::size_t sensitivity = process.keyword + 1;
    if (sensitivity < limit && tokens[sensitivity].text == "@") ++sensitivity;
    if (sensitivity < limit && tokens[sensitivity].text == "*") return true;
    if (sensitivity >= limit || tokens[sensitivity].text != "(") return false;
    const auto after = skipBalanced(tokens, sensitivity, limit, "(", ")");
    if (!after) return false;
    for (std::size_t i = sensitivity + 1; i + 1 < *after; ++i) {
        if (tokenIs(tokens[i], "posedge") || tokenIs(tokens[i], "negedge")) return false;
    }
    return true;
}

bool verilogSequentialProcess(const std::vector<HdlToken>& tokens,
                              const ProcessRange& process, std::size_t limit) {
    if (tokenIs(tokens[process.keyword], "always_ff")) return true;
    if (!tokenIs(tokens[process.keyword], "always")) return false;
    std::size_t sensitivity = process.keyword + 1;
    if (sensitivity < limit && tokens[sensitivity].text == "@") ++sensitivity;
    if (sensitivity >= limit || tokens[sensitivity].text != "(") return false;
    const auto after = skipBalanced(tokens, sensitivity, limit, "(", ")");
    if (!after) return false;
    return std::ranges::any_of(
        tokens.begin() + static_cast<std::ptrdiff_t>(sensitivity + 1),
        tokens.begin() + static_cast<std::ptrdiff_t>(*after),
        [](const HdlToken& token) {
            return tokenIs(token, "posedge") || tokenIs(token, "negedge");
        });
}

std::optional<std::size_t> simpleIdentifierToken(
    const std::vector<HdlToken>& tokens, std::size_t begin, std::size_t end) {
    while (begin < end && tokens[begin].text == "(") {
        const auto after = skipBalanced(tokens, begin, end, "(", ")");
        if (!after || *after != end) break;
        ++begin;
        --end;
    }
    if (begin + 1 == end && identifier(tokens[begin])) return begin;
    return std::nullopt;
}

bool simpleIdentifierExpression(const std::vector<HdlToken>& tokens,
                                std::size_t begin, std::size_t end) {
    return simpleIdentifierToken(tokens, begin, end).has_value();
}

void appendVerilogComplexEdgeEventIssues(
    const core::SourceFile& source, const std::vector<HdlToken>& tokens,
    const std::vector<ProcessRange>& processes, std::size_t limit,
    std::vector<SemanticIssue>& issues, const std::string& unitName) {
    for (const auto& process : processes) {
        for (std::size_t token = process.keyword + 1;
             token + 1 < process.end && token + 1 < limit; ++token) {
            if (!tokenIs(tokens[token], "posedge") &&
                !tokenIs(tokens[token], "negedge")) continue;
            const auto operand = token + 1;
            bool simple = false;
            if (tokens[operand].text == "(") {
                const auto after = skipBalanced(
                    tokens, operand, std::min(process.end, limit), "(", ")");
                simple = after && simpleIdentifierExpression(
                    tokens, operand + 1, *after - 1);
            } else if (identifier(tokens[operand])) {
                const auto following = operand + 1;
                simple = following >= process.end || following >= limit ||
                         tokens[following].text == ")" ||
                         tokens[following].text == "," ||
                         tokenIs(tokens[following], "or");
            }
            if (simple) continue;
            issues.push_back({"RTL-COMPLEX-EDGE-EVENT",
                "Edge event uses a complex expression",
                "A clock or asynchronous control edge is derived from an expression, select, or other non-scalar event source and can introduce glitches or non-portable synthesis behavior.",
                "Use a direct scalar clock/reset signal and move qualification into synchronous logic or a supported dedicated clock-control primitive.",
                location(source, tokens[token]), unitName, std::nullopt});
        }
    }
}

void appendVhdlComplexEdgeEventIssues(
    const core::SourceFile& source, const std::vector<HdlToken>& tokens,
    const std::vector<ProcessRange>& processes, std::size_t limit,
    std::vector<SemanticIssue>& issues, const std::string& unitName) {
    for (const auto& process : processes) {
        for (std::size_t token = process.begin;
             token + 1 < process.end && token + 1 < limit; ++token) {
            if (!tokenIs(tokens[token], "rising_edge") &&
                !tokenIs(tokens[token], "falling_edge")) continue;
            if (tokens[token + 1].text != "(") continue;
            const auto after = skipBalanced(
                tokens, token + 1, std::min(process.end, limit), "(", ")");
            if (!after || simpleIdentifierExpression(
                    tokens, token + 2, *after - 1)) continue;
            issues.push_back({"RTL-COMPLEX-EDGE-EVENT",
                "Edge function uses a complex expression",
                "A clock edge function is applied to an expression or select instead of one scalar clock signal, which can introduce glitches or non-portable synthesis behavior.",
                "Use a direct scalar clock signal and move qualification into synchronous logic or a supported dedicated clock-control primitive.",
                location(source, tokens[token]), unitName, std::nullopt});
        }
    }
}

struct ConditionRange {
    std::size_t begin;
    std::size_t end;
    std::size_t after;
};

std::string conditionKey(const std::vector<HdlToken>& tokens,
                         std::size_t begin, std::size_t end, bool vhdl) {
    while (begin < end && tokens[begin].text == "(") {
        const auto after = skipBalanced(tokens, begin, end, "(", ")");
        if (!after || *after != end) break;
        ++begin;
        --end;
    }
    std::string key;
    for (std::size_t token = begin; token < end; ++token) {
        if (!key.empty()) key.push_back('\x1f');
        key += vhdl ? lower(tokens[token].text) : tokens[token].text;
    }
    return key;
}

std::optional<ConditionRange> verilogConditionAt(
    const std::vector<HdlToken>& tokens, std::size_t ifToken, std::size_t limit) {
    if (ifToken + 1 >= limit || !tokenIs(tokens[ifToken], "if") ||
        tokens[ifToken + 1].text != "(") return std::nullopt;
    const auto after = skipBalanced(tokens, ifToken + 1, limit, "(", ")");
    if (!after) return std::nullopt;
    return ConditionRange{ifToken + 2, *after - 1, *after};
}

void appendVerilogDuplicateConditionIssues(
    const core::SourceFile& source, const std::vector<HdlToken>& tokens,
    const std::vector<ProcessRange>& processes, std::size_t limit,
    std::vector<SemanticIssue>& issues, const std::string& unitName) {
    for (const auto& process : processes) {
        for (std::size_t token = process.begin;
             token < process.end && token < limit; ++token) {
            if (!tokenIs(tokens[token], "if") ||
                (token > process.begin && tokenIs(tokens[token - 1], "else"))) continue;
            std::set<std::string> seen;
            std::size_t branch = token;
            while (branch < process.end) {
                const auto condition = verilogConditionAt(
                    tokens, branch, std::min(process.end, limit));
                if (!condition) break;
                const auto key = conditionKey(
                    tokens, condition->begin, condition->end, false);
                if (!key.empty() && !seen.insert(key).second) {
                    issues.push_back({"RTL-DUPLICATE-CONDITION",
                        "If/else-if chain repeats an earlier condition",
                        "An exact condition appears more than once in the same if/else-if chain, so the later branch cannot be selected.",
                        "Remove the duplicate branch or correct it to test the intended distinct condition.",
                        location(source, tokens[condition->begin]), unitName, std::nullopt});
                }
                const auto afterThen = verilogStatementEnd(
                    tokens, condition->after, std::min(process.end, limit));
                if (afterThen + 1 >= process.end ||
                    !tokenIs(tokens[afterThen], "else") ||
                    !tokenIs(tokens[afterThen + 1], "if")) break;
                branch = afterThen + 1;
            }
        }
    }
}

std::optional<ConditionRange> vhdlConditionAt(
    const std::vector<HdlToken>& tokens, std::size_t keyword, std::size_t limit) {
    if (keyword + 1 >= limit ||
        (!tokenIs(tokens[keyword], "if") && !tokenIs(tokens[keyword], "elsif"))) {
        return std::nullopt;
    }
    std::size_t thenToken = keyword + 1;
    while (thenToken < limit && !tokenIs(tokens[thenToken], "then")) ++thenToken;
    if (thenToken >= limit) return std::nullopt;
    return ConditionRange{keyword + 1, thenToken, thenToken + 1};
}

std::optional<CaseSelectorShape> comparisonOperandShape(
    const std::vector<HdlToken>& tokens, std::size_t begin, std::size_t end,
    const std::map<std::string, SignalState>& signals, bool vhdl) {
    if (const auto selector = vhdl ? vhdlCaseSelectorShape(
                                         tokens, begin, end, signals)
                                   : verilogCaseSelectorShape(
                                         tokens, begin, end, signals)) {
        return selector;
    }
    if (const auto concatenation = vhdl ? vhdlConcatenationShape(
                                              tokens, begin, end, signals)
                                        : verilogConcatenationShape(
                                              tokens, begin, end, signals)) {
        return concatenation;
    }
    if (vhdl) {
        if (const auto conversion = vhdlNativeConversionShape(
                tokens, begin, end, signals)) return conversion;
    }
    while (begin < end && tokens[begin].text == "(") {
        const auto after = skipBalanced(tokens, begin, end, "(", ")");
        if (!after || *after != end) break;
        ++begin;
        --end;
    }
    if (vhdl) {
        if (begin + 1 == end) {
            const auto width = vhdlStringLiteralWidth(tokens[begin]);
            if (width) return CaseSelectorShape{*width, tokens[begin].text};
        }
        if (const auto width = vhdlLogicCharacterLiteralWidth(tokens, begin, end)) {
            return CaseSelectorShape{*width, expressionText(tokens, begin, end)};
        }
        return std::nullopt;
    }
    if (begin + 1 != end || tokens[begin].kind != HdlTokenKind::Number) {
        return std::nullopt;
    }
    const auto width = verilogLiteralDeclaredWidth(tokens[begin].text);
    return width ? std::optional<CaseSelectorShape>(
                       CaseSelectorShape{*width, tokens[begin].text})
                 : std::nullopt;
}

std::optional<CaseSelectorShape> vhdlNativeConversionShape(
    const std::vector<HdlToken>& tokens, std::size_t begin, std::size_t end,
    const std::map<std::string, SignalState>& signals,
    const VhdlStaticIntegerMap* staticIntegerValues) {
    const auto originalBegin = begin;
    const auto originalEnd = end;
    while (begin < end && tokens[begin].text == "(") {
        const auto after = skipBalanced(tokens, begin, end, "(", ")");
        if (!after || *after != end) break;
        ++begin;
        --end;
    }
    if (begin + 4 >= end || !identifier(tokens[begin]) ||
        tokens[begin + 1].text != "(") return std::nullopt;
    const auto after = skipBalanced(tokens, begin + 1, end, "(", ")");
    if (!after || *after != end) return std::nullopt;

    static const std::unordered_set<std::string> nativeWidthFunctions{
        "to_unsigned", "to_signed", "conv_std_logic_vector",
        "to_stdlogicvector", "to_bitvector"};
    if (!nativeWidthFunctions.contains(lower(tokens[begin].text))) {
        return std::nullopt;
    }

    std::optional<std::size_t> comma;
    int depth = 0;
    for (std::size_t token = begin + 2; token + 1 < end; ++token) {
        if (tokens[token].text == "(") ++depth;
        else if (tokens[token].text == ")") --depth;
        else if (tokens[token].text == "," && depth == 0) {
            if (comma) return std::nullopt;
            comma = token;
        }
    }
    if (!comma || *comma == begin + 2 || *comma + 1 >= end - 1) {
        return std::nullopt;
    }

    std::size_t widthBegin = *comma + 1;
    std::size_t widthEnd = end - 1;
    while (widthBegin < widthEnd && tokens[widthBegin].text == "(") {
        const auto widthAfter = skipBalanced(
            tokens, widthBegin, widthEnd, "(", ")");
        if (!widthAfter || *widthAfter != widthEnd) break;
        ++widthBegin;
        --widthEnd;
    }

    std::optional<std::size_t> width;
    if (widthBegin + 1 == widthEnd &&
        tokens[widthBegin].kind == HdlTokenKind::Number) {
        width = decimalValue(tokens[widthBegin].text);
    } else if (widthBegin + 1 == widthEnd &&
               identifier(tokens[widthBegin]) && staticIntegerValues) {
        const auto value = staticIntegerValues->find(
            lower(tokens[widthBegin].text));
        if (value != staticIntegerValues->end() && value->second > 0 &&
            value->second < (1 << 24)) {
            width = static_cast<std::size_t>(value->second);
        }
    } else if (widthBegin + 3 == widthEnd &&
               identifier(tokens[widthBegin]) &&
               tokens[widthBegin + 1].text == "'" &&
               tokenIs(tokens[widthBegin + 2], "length")) {
        const auto object = signals.find(lower(tokens[widthBegin].text));
        if (object != signals.end()) width = object->second.width;
    }
    if (!width || *width == 0 || *width >= (1U << 24)) return std::nullopt;
    return CaseSelectorShape{*width,
        expressionText(tokens, originalBegin, originalEnd)};
}

std::optional<CaseSelectorShape> verilogNativeFlagZeroExpressionShape(
    const std::vector<HdlToken>& tokens, std::size_t begin, std::size_t end,
    const std::map<std::string, SignalState>& signals) {
    const auto stripParentheses = [&](std::size_t& first, std::size_t& last) {
        while (first < last && tokens[first].text == "(") {
            const auto after = skipBalanced(tokens, first, last, "(", ")");
            if (!after || *after != last) break;
            ++first;
            --last;
        }
    };
    struct Root {
        std::size_t token;
        std::size_t tokenCount;
        std::string_view operation;
    };
    const auto splitArithmetic = [&](std::size_t first, std::size_t last)
            -> std::optional<Root> {
        stripParentheses(first, last);
        const auto binarySign = [&](std::size_t token) {
            if (token == first) return false;
            const auto& previous = tokens[token - 1].text;
            return previous != "+" && previous != "-" && previous != "*" &&
                previous != "/" && previous != "%" && previous != "(" &&
                previous != "{" && previous != "," && previous != "?" &&
                previous != ":";
        };
        std::optional<Root> additive;
        std::optional<Root> multiplicative;
        std::optional<Root> power;
        int parentheses = 0;
        int brackets = 0;
        int braces = 0;
        for (std::size_t token = first; token < last; ++token) {
            const auto& text = tokens[token].text;
            if (text == "(") ++parentheses;
            else if (text == ")") --parentheses;
            else if (text == "[") ++brackets;
            else if (text == "]") --brackets;
            else if (text == "{") ++braces;
            else if (text == "}") --braces;
            else if (parentheses == 0 && brackets == 0 && braces == 0) {
                if ((text == "+" || text == "-") && binarySign(token)) {
                    additive = Root{token, 1, text};
                } else if (text == "/" || text == "%") {
                    multiplicative = Root{token, 1, text};
                } else if (text == "*") {
                    const bool previousStar = token > first &&
                        tokens[token - 1].text == "*";
                    const bool nextStar = token + 1 < last &&
                        tokens[token + 1].text == "*";
                    if (!previousStar && !nextStar) {
                        multiplicative = Root{token, 1, "*"};
                    } else if (nextStar && !power) {
                        power = Root{token, 2, "**"};
                    }
                }
            }
        }
        if (additive) return additive;
        if (multiplicative) return multiplicative;
        return power;
    };

    const auto originalBegin = begin;
    const auto originalEnd = end;
    const auto widthOf = [&](auto&& self, std::size_t first, std::size_t last)
            -> std::optional<std::size_t> {
        stripParentheses(first, last);
        if (const auto shape = comparisonOperandShape(
                tokens, first, last, signals, false)) {
            return shape->width < (1U << 24)
                ? std::optional<std::size_t>(shape->width) : std::nullopt;
        }
        if (first < last &&
            (tokens[first].text == "+" || tokens[first].text == "-")) {
            return self(self, first + 1, last);
        }
        const auto root = splitArithmetic(first, last);
        if (!root || root->token == first ||
            root->token + root->tokenCount >= last) return std::nullopt;
        const auto left = self(self, first, root->token);
        const auto right = self(self, root->token + root->tokenCount, last);
        if (!left || !right) return std::nullopt;
        // FUN_1808f46c0 flag-zero mode takes max width for ADD/SUB/MUL/DIV/MOD
        // and propagates the first child for POW.
        return root->operation == "**" ? left : std::max(*left, *right);
    };
    const auto width = widthOf(widthOf, begin, end);
    if (!width) return std::nullopt;
    return CaseSelectorShape{*width,
        expressionText(tokens, originalBegin, originalEnd)};
}

void appendVerilogBitwiseWidthIssues(
    const core::SourceFile& source, const std::vector<HdlToken>& tokens,
    const std::vector<ProcessRange>& processes, std::size_t body, std::size_t limit,
    const std::map<std::string, SignalState>& signals,
    std::vector<SemanticIssue>& issues, const std::string& unitName) {
    const auto assignmentBefore = [&](std::size_t bitwise) {
        std::size_t statementBegin = body;
        for (std::size_t cursor = bitwise; cursor > body; --cursor) {
            if (tokens[cursor - 1].text == ";") {
                statementBegin = cursor;
                break;
            }
        }
        std::size_t statementEnd = bitwise;
        while (statementEnd < limit && tokens[statementEnd].text != ";") {
            ++statementEnd;
        }
        bool continuous = false;
        for (std::size_t token = statementBegin; token < bitwise; ++token) {
            if (tokenIs(tokens[token], "assign")) continuous = true;
            if (!identifier(tokens[token])) continue;
            const auto op = continuous
                ? assignmentOperator(tokens, token, statementEnd)
                : verilogAssignmentOperator(
                      tokens, token, statementBegin, statementEnd);
            if (op && *op < bitwise) return true;
        }
        return false;
    };
    const auto matchingOpen = [&](std::size_t close, std::string_view left,
                                  std::string_view right) -> std::optional<std::size_t> {
        int depth = 0;
        for (std::size_t cursor = close + 1; cursor > body; ) {
            --cursor;
            if (tokens[cursor].text == right) ++depth;
            else if (tokens[cursor].text == left && --depth == 0) return cursor;
        }
        return std::nullopt;
    };
    const auto leftRange = [&](std::size_t op)
        -> std::optional<std::pair<std::size_t, std::size_t>> {
        if (op <= body) return std::nullopt;
        const auto last = op - 1;
        if (tokens[last].text == "]") {
            const auto open = matchingOpen(last, "[", "]");
            if (!open || *open == body || !identifier(tokens[*open - 1])) return std::nullopt;
            return std::pair{*open - 1, op};
        }
        if (tokens[last].text == ")" || tokens[last].text == "}") {
            const auto open = tokens[last].text == ")"
                ? matchingOpen(last, "(", ")") : matchingOpen(last, "{", "}");
            if (!open) return std::nullopt;
            return std::pair{*open, op};
        }
        if (!identifier(tokens[last]) && tokens[last].kind != HdlTokenKind::Number)
            return std::nullopt;
        return std::pair{last, op};
    };
    const auto rightRange = [&](std::size_t begin)
        -> std::optional<std::pair<std::size_t, std::size_t>> {
        if (begin >= limit) return std::nullopt;
        if (tokens[begin].text == "(" || tokens[begin].text == "{") {
            const auto after = skipBalanced(tokens, begin, limit,
                tokens[begin].text, tokens[begin].text == "(" ? ")" : "}");
            if (!after) return std::nullopt;
            return std::pair{begin, *after};
        }
        if (!identifier(tokens[begin]) && tokens[begin].kind != HdlTokenKind::Number)
            return std::nullopt;
        auto end = begin + 1;
        while (end < limit && tokens[end].text == "[") {
            const auto after = skipBalanced(tokens, end, limit, "[", "]");
            if (!after) return std::nullopt;
            end = *after;
        }
        return std::pair{begin, end};
    };

    for (std::size_t token = body; token < limit; ++token) {
        std::size_t operatorEnd = token + 1;
        const auto& op = tokens[token].text;
        if (op == "&" || op == "|") {
            if ((token > body && tokens[token - 1].text == op) ||
                (token + 1 < limit && tokens[token + 1].text == op)) continue;
        } else if (op == "^") {
            if (token > body && tokens[token - 1].text == "~") continue;
            if (token + 1 < limit && tokens[token + 1].text == "~") operatorEnd = token + 2;
        } else if (op == "~") {
            if (token + 1 >= limit || tokens[token + 1].text != "^") continue;
            operatorEnd = token + 2;
        } else {
            continue;
        }
        const bool inProcess = std::ranges::any_of(processes,
            [&](const auto& process) {
                return token >= process.begin && token < process.end;
            });
        if (!inProcess && !assignmentBefore(token)) continue;
        const auto leftTokens = leftRange(token);
        const auto rightTokens = rightRange(operatorEnd);
        if (!leftTokens || !rightTokens) continue;
        // GJB10157.8.2 calls FUN_1808f4650 for both children with the
        // recovered fifth argument set to zero. Reuse that exact bounded
        // expression-width model instead of limiting the rule to leaf shapes.
        const auto left = verilogNativeFlagZeroExpressionShape(
            tokens, leftTokens->first, leftTokens->second, signals);
        const auto right = verilogNativeFlagZeroExpressionShape(
            tokens, rightTokens->first, rightTokens->second, signals);
        if (!left || !right || left->width == right->width) continue;
        issues.push_back({"RTL-BITWISE-WIDTH-MISMATCH",
            "Bitwise operands have different widths: " + left->display + " is " +
                std::to_string(left->width) + " bits and " + right->display + " is " +
                std::to_string(right->width) + " bits",
            "A binary bitwise operator uses statically sized operands of different widths, so one operand is implicitly extended before the operation.",
            "Resize or slice both bitwise operands to the intended common width.",
            location(source, tokens[token]), unitName, left->display});
    }
}

void appendVhdlBitwiseWidthIssues(
    const core::SourceFile& source, const std::vector<HdlToken>& tokens,
    std::size_t body, std::size_t limit,
    const std::map<std::string, SignalState>& signals,
    const VhdlStaticIntegerMap& staticIntegerValues,
    std::vector<SemanticIssue>& issues, const std::string& unitName) {
    const auto matchingOpen = [&](std::size_t close) -> std::optional<std::size_t> {
        int depth = 0;
        for (std::size_t cursor = close + 1; cursor > body; ) {
            --cursor;
            if (tokens[cursor].text == ")") ++depth;
            else if (tokens[cursor].text == "(" && --depth == 0) return cursor;
        }
        return std::nullopt;
    };
    const auto leftRange = [&](std::size_t op)
        -> std::optional<std::pair<std::size_t, std::size_t>> {
        if (op <= body) return std::nullopt;
        const auto last = op - 1;
        if (tokens[last].text == ")") {
            const auto open = matchingOpen(last);
            if (!open) return std::nullopt;
            // A VHDL selected name is represented as IDENTIFIER + RANGE by the
            // native width resolver. Keep the identifier with its parentheses;
            // otherwise retain the whole parenthesized expression.
            if (*open > body && identifier(tokens[*open - 1])) {
                return std::pair{*open - 1, op};
            }
            return std::pair{*open, op};
        }
        return std::pair{last, op};
    };
    const auto rightRange = [&](std::size_t begin)
        -> std::optional<std::pair<std::size_t, std::size_t>> {
        if (begin >= limit) return std::nullopt;
        if (tokens[begin].text == "(") {
            const auto after = skipBalanced(tokens, begin, limit, "(", ")");
            return after ? std::optional(std::pair{begin, *after}) : std::nullopt;
        }
        auto end = begin + 1;
        if (identifier(tokens[begin]) && end < limit && tokens[end].text == "(") {
            const auto after = skipBalanced(tokens, end, limit, "(", ")");
            if (!after) return std::nullopt;
            end = *after;
        }
        return std::pair{begin, end};
    };
    for (std::size_t token = body + 1; token + 1 < limit; ++token) {
        if (!tokenIs(tokens[token], "and") && !tokenIs(tokens[token], "or") &&
            !tokenIs(tokens[token], "xor") && !tokenIs(tokens[token], "xnor")) continue;
        const auto leftTokens = leftRange(token);
        const auto rightTokens = rightRange(token + 1);
        if (!leftTokens || !rightTokens) continue;
        auto left = comparisonOperandShape(
            tokens, leftTokens->first, leftTokens->second, signals, true);
        auto right = comparisonOperandShape(
            tokens, rightTokens->first, rightTokens->second, signals, true);
        if (!left) left = vhdlNativeConversionShape(
            tokens, leftTokens->first, leftTokens->second, signals,
            &staticIntegerValues);
        if (!right) right = vhdlNativeConversionShape(
            tokens, rightTokens->first, rightTokens->second, signals,
            &staticIntegerValues);
        if (!left || !right || left->width == right->width) continue;
        issues.push_back({"RTL-BITWISE-WIDTH-MISMATCH",
            "Bitwise operands have different widths: " + left->display + " is " +
                std::to_string(left->width) + " bits and " + right->display + " is " +
                std::to_string(right->width) + " bits",
            "A native GJB9765.4.6.b.1 operation node has two statically known operand widths that are not equal.",
            "Resize or slice both operands to the intended common width.",
            location(source, tokens[token]), unitName, left->display});
    }
}

void appendVerilogMixedSignednessArithmeticIssues(
    const core::SourceFile& source, const std::vector<HdlToken>& tokens,
    const std::vector<ProcessRange>& processes, std::size_t body, std::size_t limit,
    const std::map<std::string, SignalState>& signals,
    std::vector<SemanticIssue>& issues, const std::string& unitName) {
    const auto assignmentBefore = [&](std::size_t arithmetic) {
        std::size_t statementBegin = body;
        for (std::size_t cursor = arithmetic; cursor > body; --cursor) {
            if (tokens[cursor - 1].text == ";") {
                statementBegin = cursor;
                break;
            }
        }
        std::size_t statementEnd = arithmetic;
        while (statementEnd < limit && tokens[statementEnd].text != ";") ++statementEnd;
        bool continuous = false;
        for (std::size_t token = statementBegin; token < arithmetic; ++token) {
            if (tokenIs(tokens[token], "assign")) continuous = true;
            if (!identifier(tokens[token])) continue;
            const auto op = continuous
                ? assignmentOperator(tokens, token, statementEnd)
                : verilogAssignmentOperator(tokens, token, statementBegin, statementEnd);
            if (op && *op < arithmetic) return true;
        }
        return false;
    };
    const auto stripParentheses = [&](std::size_t& first, std::size_t& last) {
        while (first < last && tokens[first].text == "(") {
            const auto after = skipBalanced(tokens, first, last, "(", ")");
            if (!after || *after != last) break;
            ++first;
            --last;
        }
    };
    struct ArithmeticRoot {
        std::size_t token;
        std::size_t tokenCount;
    };
    const auto splitArithmetic = [&](std::size_t first, std::size_t last)
            -> std::optional<ArithmeticRoot> {
        stripParentheses(first, last);
        const auto binarySign = [&](std::size_t token) {
            if (token == first) return false;
            const auto& previous = tokens[token - 1].text;
            return previous != "+" && previous != "-" && previous != "*" &&
                previous != "/" && previous != "%" && previous != "(" &&
                previous != "{" && previous != "," && previous != "?" &&
                previous != ":";
        };
        std::optional<ArithmeticRoot> additive;
        std::optional<ArithmeticRoot> multiplicative;
        std::optional<ArithmeticRoot> power;
        int parentheses = 0;
        int brackets = 0;
        int braces = 0;
        for (std::size_t token = first; token < last; ++token) {
            const auto& text = tokens[token].text;
            if (text == "(") ++parentheses;
            else if (text == ")") --parentheses;
            else if (text == "[") ++brackets;
            else if (text == "]") --brackets;
            else if (text == "{") ++braces;
            else if (text == "}") --braces;
            else if (parentheses == 0 && brackets == 0 && braces == 0) {
                if ((text == "+" || text == "-") && binarySign(token)) {
                    additive = ArithmeticRoot{token, 1};
                } else if (text == "/" || text == "%") {
                    multiplicative = ArithmeticRoot{token, 1};
                } else if (text == "*") {
                    const bool previousStar = token > first &&
                        tokens[token - 1].text == "*";
                    const bool nextStar = token + 1 < last &&
                        tokens[token + 1].text == "*";
                    if (!previousStar && !nextStar) {
                        multiplicative = ArithmeticRoot{token, 1};
                    } else if (nextStar && !power) {
                        power = ArithmeticRoot{token, 2};
                    }
                }
            }
        }
        if (additive) return additive;       // rightmost, left associative
        if (multiplicative) return multiplicative;
        return power;                       // leftmost, right associative
    };
    const auto collectSignedness = [&](std::size_t begin, std::size_t end,
                                       bool& sawSigned, bool& sawUnsigned) {
        for (std::size_t token = begin; token < end; ++token) {
            if (!identifier(tokens[token])) continue;
            const auto signal = signals.find(tokens[token].text);
            if (signal == signals.end()) continue;
            if (signal->second.verilogSignedness == VerilogSignedness::Signed)
                sawSigned = true;
            else if (signal->second.verilogSignedness == VerilogSignedness::Unsigned)
                sawUnsigned = true;
        }
    };

    const auto arithmeticIsland = [&](std::size_t op)
            -> std::pair<std::size_t, std::size_t> {
        const auto allowedAtTopLevel = [&](std::size_t token) {
            const auto& text = tokens[token].text;
            return identifier(tokens[token]) ||
                tokens[token].kind == HdlTokenKind::Number ||
                text == "+" || text == "-" || text == "*" ||
                text == "/" || text == "%" || text == "(" ||
                text == ")" || text == "[" || text == "]" ||
                text == "{" || text == "}";
        };
        std::size_t first = op;
        int parentheses = 0;
        int brackets = 0;
        int braces = 0;
        while (first > body) {
            const auto candidate = first - 1;
            const auto& text = tokens[candidate].text;
            if (text == ")") ++parentheses;
            else if (text == "]") ++brackets;
            else if (text == "}") ++braces;
            else if (text == "(") {
                if (parentheses == 0) break;
                --parentheses;
            } else if (text == "[") {
                if (brackets == 0) break;
                --brackets;
            } else if (text == "{") {
                if (braces == 0) break;
                --braces;
            } else if (parentheses == 0 && brackets == 0 && braces == 0 &&
                       !allowedAtTopLevel(candidate)) {
                break;
            }
            first = candidate;
        }

        std::size_t last = op + 1;
        parentheses = 0;
        brackets = 0;
        braces = 0;
        while (last < limit) {
            const auto& text = tokens[last].text;
            if (text == "(") ++parentheses;
            else if (text == "[") ++brackets;
            else if (text == "{") ++braces;
            else if (text == ")") {
                if (parentheses == 0) break;
                --parentheses;
            } else if (text == "]") {
                if (brackets == 0) break;
                --brackets;
            } else if (text == "}") {
                if (braces == 0) break;
                --braces;
            } else if (parentheses == 0 && brackets == 0 && braces == 0 &&
                       !allowedAtTopLevel(last)) {
                break;
            }
            ++last;
        }
        return {first, last};
    };

    std::set<std::size_t> reportedOperators;
    const auto analyzeTree = [&](auto&& self, std::size_t first,
                                 std::size_t last) -> void {
        stripParentheses(first, last);
        const auto root = splitArithmetic(first, last);
        if (!root || root->token == first ||
            root->token + root->tokenCount >= last) return;
        bool sawSigned = false;
        bool sawUnsigned = false;
        collectSignedness(first, last, sawSigned, sawUnsigned);
        if (sawSigned && sawUnsigned && reportedOperators.insert(root->token).second) {
            issues.push_back({"RTL-MIXED-SIGNEDNESS-ARITHMETIC",
                "Arithmetic expression mixes signed and unsigned signals",
                "A Verilog arithmetic AST node contains identifier descendants whose resolved declarations do not share one signedness.",
                "Convert operands explicitly with $signed/$unsigned or use declarations with consistent signedness.",
                location(source, tokens[root->token]), unitName, std::nullopt});
        }
        self(self, first, root->token);
        self(self, root->token + root->tokenCount, last);
    };

    std::set<std::pair<std::size_t, std::size_t>> analyzedIslands;
    for (std::size_t token = body; token < limit; ++token) {
        const auto& op = tokens[token].text;
        if (op == "*") {
            if (token > body && tokens[token - 1].text == "*") continue;
            if (token + 1 < limit && tokens[token + 1].text == "*") ++token;
        } else if (op != "+" && op != "-" && op != "/" && op != "%") {
            continue;
        }
        const bool inProcess = std::ranges::any_of(processes, [&](const auto& process) {
            return token >= process.begin && token < process.end;
        });
        if (!inProcess && !assignmentBefore(token)) continue;
        const auto island = arithmeticIsland(token);
        if (analyzedIslands.insert(island).second) {
            analyzeTree(analyzeTree, island.first, island.second);
        }
    }
}

void appendComparisonWidthMismatchInCondition(
    const core::SourceFile& source, const std::vector<HdlToken>& tokens,
    std::size_t begin, std::size_t end,
    const std::map<std::string, SignalState>& signals, bool vhdl,
    std::vector<SemanticIssue>& issues, const std::string& unitName) {
    std::function<void(std::size_t, std::size_t)> analyze;
    analyze = [&](std::size_t first, std::size_t last) {
        while (first < last && tokens[first].text == "(") {
            const auto after = skipBalanced(tokens, first, last, "(", ")");
            if (!after || *after != last) break;
            ++first;
            --last;
        }
        if (first >= last) return;
        const bool negated = (!vhdl && tokens[first].text == "!") ||
                             (vhdl && tokenIs(tokens[first], "not"));
        if (negated && first + 1 < last && tokens[first + 1].text == "(") {
            const auto after = skipBalanced(tokens, first + 1, last, "(", ")");
            if (after && *after == last) {
                analyze(first + 1, last);
                return;
            }
        }

        int depth = 0;
        for (std::size_t token = first; token < last; ++token) {
            if (tokens[token].text == "(" || tokens[token].text == "[" ||
                tokens[token].text == "{") {
                ++depth;
                continue;
            }
            if (tokens[token].text == ")" || tokens[token].text == "]" ||
                tokens[token].text == "}") {
                --depth;
                continue;
            }
            if (depth != 0) continue;
            const bool verilogConnector = !vhdl && token + 1 < last &&
                ((tokens[token].text == "&" && tokens[token + 1].text == "&") ||
                 (tokens[token].text == "|" && tokens[token + 1].text == "|"));
            const bool vhdlConnector = vhdl &&
                (tokenIs(tokens[token], "and") || tokenIs(tokens[token], "or") ||
                 tokenIs(tokens[token], "nand") || tokenIs(tokens[token], "nor") ||
                 tokenIs(tokens[token], "xor") || tokenIs(tokens[token], "xnor"));
            if (!verilogConnector && !vhdlConnector) continue;
            const auto connectorWidth = verilogConnector ? 2U : 1U;
            analyze(first, token);
            analyze(token + connectorWidth, last);
            return;
        }

        std::optional<std::size_t> operatorToken;
        std::size_t operatorEnd = 0;
        depth = 0;
        for (std::size_t token = first; token < last; ++token) {
            if (tokens[token].text == "(" || tokens[token].text == "[" ||
                tokens[token].text == "{") {
                ++depth;
                continue;
            }
            if (tokens[token].text == ")" || tokens[token].text == "]" ||
                tokens[token].text == "}") {
                --depth;
                continue;
            }
            if (depth != 0) continue;
            std::size_t candidateEnd = 0;
            if (!vhdl && token + 1 < last &&
                (tokens[token].text == "=" || tokens[token].text == "!") &&
                tokens[token + 1].text == "=") {
                candidateEnd = token + 2;
                if (candidateEnd < last && tokens[candidateEnd].text == "=") {
                    ++candidateEnd;
                } else if (candidateEnd < last && tokens[candidateEnd].text == "?") {
                    ++candidateEnd;
                }
            } else if (!vhdl &&
                       (tokens[token].text == "<" || tokens[token].text == ">") &&
                       !(token + 1 < last &&
                         tokens[token + 1].text == tokens[token].text) &&
                       !(token > first &&
                         tokens[token - 1].text == tokens[token].text)) {
                candidateEnd = token + 1;
                if (candidateEnd < last && tokens[candidateEnd].text == "=") {
                    ++candidateEnd;
                }
            } else if (vhdl && tokens[token].text == "=" &&
                       (token == first || (tokens[token - 1].text != "<" &&
                                           tokens[token - 1].text != ">" &&
                                           tokens[token - 1].text != ":")) &&
                       (token + 1 >= last || tokens[token + 1].text != ">")) {
                candidateEnd = token + 1;
            } else if (vhdl && token + 1 < last && tokens[token].text == "/" &&
                       tokens[token + 1].text == "=") {
                candidateEnd = token + 2;
            } else if (vhdl &&
                       (tokens[token].text == "<" || tokens[token].text == ">")) {
                candidateEnd = token + 1;
                if (candidateEnd < last && tokens[candidateEnd].text == "=") {
                    ++candidateEnd;
                }
            }
            if (candidateEnd == 0) continue;
            if (operatorToken) return;
            operatorToken = token;
            operatorEnd = candidateEnd;
            token = candidateEnd - 1;
        }
        if (!operatorToken || first >= *operatorToken || operatorEnd >= last) return;
        const auto left = vhdl
            ? comparisonOperandShape(tokens, first, *operatorToken, signals, true)
            : verilogNativeFlagZeroExpressionShape(
                  tokens, first, *operatorToken, signals);
        const auto right = vhdl
            ? comparisonOperandShape(tokens, operatorEnd, last, signals, true)
            : verilogNativeFlagZeroExpressionShape(
                  tokens, operatorEnd, last, signals);
        if (!left || !right || left->width == right->width) return;
        issues.push_back({"RTL-COMPARISON-WIDTH-MISMATCH",
            "Comparison operands have different widths: " + left->display +
                " is " + std::to_string(left->width) + " bits and " +
                right->display + " is " + std::to_string(right->width) + " bits",
            "A procedural equality or relational comparison uses statically sized operands of different widths, so implicit extension can hide a selection or declaration mistake.",
            "Resize or slice both operands to the intended common width before comparing them.",
            location(source, tokens[*operatorToken]), unitName, left->display});
    };
    analyze(begin, end);
}

void appendVerilogComparisonWidthIssues(
    const core::SourceFile& source, const std::vector<HdlToken>& tokens,
    const std::vector<ProcessRange>& processes, std::size_t body, std::size_t limit,
    const std::map<std::string, SignalState>& signals,
    std::vector<SemanticIssue>& issues, const std::string& unitName) {
    for (const auto& process : processes) {
        for (std::size_t token = process.begin;
             token + 1 < process.end && token + 1 < limit; ++token) {
            if (tokens[token + 1].text != "(") continue;
            if (tokenIs(tokens[token], "for")) {
                const auto after = skipBalanced(
                    tokens, token + 1, std::min(process.end, limit), "(", ")");
                if (!after) continue;
                std::vector<std::size_t> separators;
                int depth = 0;
                for (std::size_t part = token + 2; part + 1 < *after; ++part) {
                    if (tokens[part].text == "(" || tokens[part].text == "[" ||
                        tokens[part].text == "{") ++depth;
                    else if (tokens[part].text == ")" || tokens[part].text == "]" ||
                             tokens[part].text == "}") --depth;
                    else if (tokens[part].text == ";" && depth == 0) {
                        separators.push_back(part);
                    }
                }
                if (separators.size() == 2 && separators[0] + 1 < separators[1]) {
                    appendComparisonWidthMismatchInCondition(
                        source, tokens, separators[0] + 1, separators[1],
                        signals, false, issues, unitName);
                }
                continue;
            }
            if (!tokenIs(tokens[token], "if") &&
                !tokenIs(tokens[token], "while")) continue;
            const auto after = skipBalanced(
                tokens, token + 1, std::min(process.end, limit), "(", ")");
            if (!after) continue;
            appendComparisonWidthMismatchInCondition(
                source, tokens, token + 2, *after - 1, signals, false,
                issues, unitName);
        }
    }

    // Native slot 0 recursively collects comparison descendants from both
    // AST_ALWAYS and continuous AST_ASSIGN roots. Conditions above cover the
    // control-flow paths; this pass covers a comparison used directly as an
    // assignment RHS without admitting declaration initializers or cell ports.
    std::set<std::size_t> scannedAssignments;
    for (std::size_t token = body; token < limit; ++token) {
        if (!identifier(tokens[token]) || !signals.contains(tokens[token].text)) continue;
        const auto process = std::ranges::find_if(processes, [&](const auto& candidate) {
            return token >= candidate.begin && token < candidate.end;
        });
        std::size_t statementBegin = process == processes.end() ? body : process->begin;
        for (std::size_t cursor = token; cursor > statementBegin; --cursor) {
            if (tokens[cursor - 1].text == ";") {
                statementBegin = cursor;
                break;
            }
        }
        std::size_t statementEnd = token;
        while (statementEnd < limit && tokens[statementEnd].text != ";") ++statementEnd;
        if (statementEnd >= limit) continue;

        std::optional<std::size_t> op;
        if (process != processes.end()) {
            op = verilogAssignmentOperator(
                tokens, token, statementBegin, statementEnd);
        } else {
            const bool continuous = std::ranges::any_of(
                tokens.begin() + static_cast<std::ptrdiff_t>(statementBegin),
                tokens.begin() + static_cast<std::ptrdiff_t>(token),
                [](const HdlToken& candidate) { return tokenIs(candidate, "assign"); });
            if (continuous) op = assignmentOperator(tokens, token, statementEnd);
        }
        if (!op || !scannedAssignments.insert(*op).second) continue;
        std::size_t rhsBegin = *op + 1;
        if (tokens[*op].text == "<" && rhsBegin < statementEnd &&
            tokens[rhsBegin].text == "=") ++rhsBegin;
        if (rhsBegin >= statementEnd) continue;
        appendComparisonWidthMismatchInCondition(
            source, tokens, rhsBegin, statementEnd, signals, false,
            issues, unitName);
    }
}

void appendVhdlComparisonWidthIssues(
    const core::SourceFile& source, const std::vector<HdlToken>& tokens,
    const std::vector<ProcessRange>& processes, std::size_t limit,
    const std::map<std::string, SignalState>& signals,
    std::vector<SemanticIssue>& issues, const std::string& unitName) {
    for (const auto& process : processes) {
        for (std::size_t token = process.begin;
             token < process.end && token < limit; ++token) {
            if (tokenIs(tokens[token], "while")) {
                std::size_t loopToken = token + 1;
                int depth = 0;
                for (; loopToken < process.end && loopToken < limit; ++loopToken) {
                    if (tokens[loopToken].text == "(") ++depth;
                    else if (tokens[loopToken].text == ")") --depth;
                    else if (depth == 0 && tokenIs(tokens[loopToken], "loop")) break;
                }
                if (loopToken < process.end && loopToken < limit) {
                    appendComparisonWidthMismatchInCondition(
                        source, tokens, token + 1, loopToken, signals, true,
                        issues, unitName);
                }
                continue;
            }
            if ((!tokenIs(tokens[token], "if") && !tokenIs(tokens[token], "elsif")) ||
                (token > process.begin && tokenIs(tokens[token - 1], "end"))) continue;
            const auto condition = vhdlConditionAt(
                tokens, token, std::min(process.end, limit));
            if (!condition) continue;
            appendComparisonWidthMismatchInCondition(
                source, tokens, condition->begin, condition->end, signals, true,
                issues, unitName);
        }
    }
}

std::vector<ConditionRange> verilogConditionalExpressionConditions(
    const std::vector<HdlToken>& tokens, std::size_t body, std::size_t limit) {
    std::vector<ConditionRange> conditions;
    for (std::size_t token = body; token < limit; ++token) {
        if (tokens[token].text != "?" || token == 0 ||
            tokens[token - 1].text == "=") continue;
        std::optional<std::pair<std::size_t, std::size_t>> condition;
        if (tokens[token - 1].text == ")") {
            int depth = 0;
            for (std::size_t cursor = token; cursor > body; ) {
                --cursor;
                if (tokens[cursor].text == ")") ++depth;
                else if (tokens[cursor].text == "(" && --depth == 0) {
                    if (cursor + 1 < token - 1) {
                        condition = std::pair{cursor + 1, token - 1};
                    }
                    break;
                }
            }
        } else {
            std::size_t statementBegin = body;
            for (std::size_t cursor = token; cursor > body; --cursor) {
                if (tokens[cursor - 1].text == ";") {
                    statementBegin = cursor;
                    break;
                }
            }
            std::optional<std::size_t> expressionBegin;
            for (std::size_t candidate = statementBegin; candidate < token;
                 ++candidate) {
                if (tokenIs(tokens[candidate], "assign")) {
                    statementBegin = candidate + 1;
                    continue;
                }
                if (!identifier(tokens[candidate])) continue;
                const auto op = verilogAssignmentOperator(
                    tokens, candidate, statementBegin, token);
                if (!op) continue;
                std::size_t afterOperator = *op + 1;
                if (tokens[*op].text == "<" && afterOperator < token &&
                    tokens[afterOperator].text == "=") {
                    ++afterOperator;
                }
                if (afterOperator < token) {
                    expressionBegin = afterOperator;
                }
            }
            if (!expressionBegin) {
                int depth = 0;
                for (std::size_t cursor = token; cursor > statementBegin; ) {
                    --cursor;
                    if (tokens[cursor].text == ")") {
                        ++depth;
                    } else if (tokens[cursor].text == "(") {
                        if (depth == 0) {
                            expressionBegin = cursor + 1;
                            break;
                        }
                        --depth;
                    }
                }
            }
            if (expressionBegin) {
                int depth = 0;
                std::size_t nestedBegin = *expressionBegin;
                for (std::size_t cursor = *expressionBegin; cursor < token; ++cursor) {
                    if (tokens[cursor].text == "(" || tokens[cursor].text == "[" ||
                        tokens[cursor].text == "{") {
                        ++depth;
                        continue;
                    }
                    if (tokens[cursor].text == ")" || tokens[cursor].text == "]" ||
                        tokens[cursor].text == "}") {
                        --depth;
                        continue;
                    }
                    if (depth != 0) continue;
                    const bool priorTernary = tokens[cursor].text == "?" &&
                        (cursor == 0 || tokens[cursor - 1].text != "=");
                    const bool branchSeparator = tokens[cursor].text == ":" &&
                        (cursor == 0 || tokens[cursor - 1].text != ":") &&
                        (cursor + 1 >= token || tokens[cursor + 1].text != ":");
                    if (priorTernary || branchSeparator) nestedBegin = cursor + 1;
                }
                if (nestedBegin < token) condition = std::pair{nestedBegin, token};
            }
        }
        if (condition) {
            conditions.push_back({condition->first, condition->second, token + 1});
        }
    }
    return conditions;
}

void appendVerilogConditionalExpressionComparisonWidthIssues(
    const core::SourceFile& source, const std::vector<HdlToken>& tokens,
    std::size_t body, std::size_t limit,
    const std::map<std::string, SignalState>& signals,
    std::vector<SemanticIssue>& issues, const std::string& unitName) {
    for (const auto& condition :
         verilogConditionalExpressionConditions(tokens, body, limit)) {
        appendComparisonWidthMismatchInCondition(
            source, tokens, condition.begin, condition.end, signals, false,
            issues, unitName);
    }
}

std::vector<ConditionRange> vhdlConditionalExpressionConditions(
    const std::vector<HdlToken>& tokens, std::size_t body, std::size_t limit) {
    std::vector<ConditionRange> conditions;
    for (std::size_t token = body; token < limit; ++token) {
        if (!tokenIs(tokens[token], "when")) continue;
        int depth = 0;
        std::size_t elseToken = token + 1;
        for (; elseToken < limit && tokens[elseToken].text != ";"; ++elseToken) {
            if (tokens[elseToken].text == "(") ++depth;
            else if (tokens[elseToken].text == ")") --depth;
            else if (depth == 0 && tokenIs(tokens[elseToken], "else")) break;
        }
        if (elseToken >= limit || !tokenIs(tokens[elseToken], "else") ||
            token + 1 >= elseToken) continue;
        conditions.push_back({token + 1, elseToken, elseToken + 1});
    }
    return conditions;
}

void appendVhdlConditionalExpressionComparisonWidthIssues(
    const core::SourceFile& source, const std::vector<HdlToken>& tokens,
    std::size_t body, std::size_t limit,
    const std::map<std::string, SignalState>& signals,
    std::vector<SemanticIssue>& issues, const std::string& unitName) {
    for (const auto& condition :
         vhdlConditionalExpressionConditions(tokens, body, limit)) {
        appendComparisonWidthMismatchInCondition(
            source, tokens, condition.begin, condition.end, signals, true,
            issues, unitName);
    }
}

void appendVhdlPackageBodyWidthIssues(
    const core::SourceFile& source, const std::vector<HdlToken>& tokens,
    std::vector<SemanticIssue>& issues) {
    for (const auto& package : vhdlPackageBodyRanges(tokens)) {
        const auto shapes = vhdlRootShapes(
            source, tokens, package.begin, package.end);
        VhdlStaticIntegerMap staticIntegerValues;
        appendVhdlNativeStaticIntegerDeclarations(
            tokens, package.begin, package.end, staticIntegerValues);

        appendVhdlBitwiseWidthIssues(
            source, tokens, package.begin, package.end, shapes,
            staticIntegerValues, issues, package.name);
        const std::vector<ProcessRange> rootRange{
            ProcessRange{package.begin, package.end, 1, false, package.root}};
        appendVhdlComparisonWidthIssues(
            source, tokens, rootRange, package.end, shapes, issues, package.name);
        appendVhdlConditionalExpressionComparisonWidthIssues(
            source, tokens, package.begin, package.end,
            shapes, issues, package.name);

        for (std::size_t token = package.begin;
             token + 2 < package.end; ++token) {
            if (!identifier(tokens[token])) continue;
            const auto target = shapes.find(lower(tokens[token].text));
            if (target == shapes.end()) continue;
            // FUN_1805fe650 admits AST_ASSIGN_LE(0x56), but not the distinct
            // AST_VARASGN(0xa2) produced for VHDL := variable assignments.
            const auto op = vhdlAssignmentOperator(tokens, token, package.end);
            if (!op) continue;
            const auto targetShape = vhdlCaseSelectorShape(
                tokens, token, *op, shapes);
            if (!targetShape) continue;
            const auto rhs = *op + 2;
            std::size_t statementEnd = rhs;
            while (statementEnd < package.end &&
                   tokens[statementEnd].text != ";") ++statementEnd;
            if (rhs >= statementEnd) continue;
            if (const auto sourceShape = vhdlCaseSelectorShape(
                    tokens, rhs, statementEnd, shapes)) {
                appendVhdlAssignmentWidthMismatch(
                    issues, package.name, targetShape->display, targetShape->width,
                    sourceShape->display, sourceShape->width,
                    location(source, tokens[token]));
                token = statementEnd < package.end ? statementEnd : token;
                continue;
            }
            if (const auto conversion = vhdlNativeConversionShape(
                    tokens, rhs, statementEnd, shapes, &staticIntegerValues)) {
                appendVhdlAssignmentWidthMismatch(
                    issues, package.name, targetShape->display, targetShape->width,
                    conversion->display, conversion->width,
                    location(source, tokens[rhs]));
                token = statementEnd < package.end ? statementEnd : token;
                continue;
            }
            if (const auto sourceToken = singleExpressionToken(
                    tokens, rhs, statementEnd)) {
                if (const auto literalWidth = vhdlStringLiteralWidth(
                        tokens[*sourceToken])) {
                    appendVhdlAssignmentWidthMismatch(
                        issues, package.name, targetShape->display,
                        targetShape->width, tokens[*sourceToken].text,
                        *literalWidth, location(source, tokens[*sourceToken]));
                }
                token = statementEnd < package.end ? statementEnd : token;
                continue;
            }
            if (const auto concatenation = vhdlConcatenationShape(
                    tokens, rhs, statementEnd, shapes)) {
                appendVhdlAssignmentWidthMismatch(
                    issues, package.name, targetShape->display, targetShape->width,
                    concatenation->display, concatenation->width,
                    location(source, tokens[rhs]));
            }
            token = statementEnd < package.end ? statementEnd : token;
        }
    }
}

void appendVhdlDuplicateConditionIssues(
    const core::SourceFile& source, const std::vector<HdlToken>& tokens,
    const std::vector<ProcessRange>& processes, std::size_t limit,
    std::vector<SemanticIssue>& issues, const std::string& unitName) {
    for (const auto& process : processes) {
        for (std::size_t token = process.begin;
             token < process.end && token < limit; ++token) {
            if (!tokenIs(tokens[token], "if") ||
                (token > process.begin && tokenIs(tokens[token - 1], "end"))) continue;
            const auto root = vhdlConditionAt(
                tokens, token, std::min(process.end, limit));
            if (!root) continue;
            std::set<std::string> seen;
            const auto rootKey = conditionKey(tokens, root->begin, root->end, true);
            if (!rootKey.empty()) seen.insert(rootKey);
            int depth = 1;
            for (std::size_t branch = root->after;
                 branch + 1 < process.end && branch + 1 < limit; ++branch) {
                if (tokenIs(tokens[branch], "if") &&
                    (branch == 0 || !tokenIs(tokens[branch - 1], "end"))) {
                    ++depth;
                    continue;
                }
                if (tokenIs(tokens[branch], "end") &&
                    tokenIs(tokens[branch + 1], "if")) {
                    --depth;
                    if (depth == 0) break;
                    ++branch;
                    continue;
                }
                if (depth != 1 || !tokenIs(tokens[branch], "elsif")) continue;
                const auto condition = vhdlConditionAt(
                    tokens, branch, std::min(process.end, limit));
                if (!condition) continue;
                const auto key = conditionKey(
                    tokens, condition->begin, condition->end, true);
                if (!key.empty() && !seen.insert(key).second) {
                    issues.push_back({"RTL-DUPLICATE-CONDITION",
                        "If/elsif chain repeats an earlier condition",
                        "An exact condition appears more than once in the same if/elsif chain, so the later branch cannot be selected.",
                        "Remove the duplicate branch or correct it to test the intended distinct condition.",
                        location(source, tokens[condition->begin]), unitName, std::nullopt});
                }
                branch = condition->after - 1;
            }
        }
    }
}

void stripWholeParentheses(const std::vector<HdlToken>& tokens,
                           std::size_t& begin, std::size_t& end) {
    while (begin < end && tokens[begin].text == "(") {
        const auto after = skipBalanced(tokens, begin, end, "(", ")");
        if (!after || *after != end) break;
        ++begin;
        --end;
    }
}

std::optional<std::size_t> topLevelOperator(
    const std::vector<HdlToken>& tokens, std::size_t begin, std::size_t end,
    std::string_view first, std::string_view second = {}) {
    int parentheses = 0;
    for (std::size_t token = begin; token < end; ++token) {
        if (tokens[token].text == "(") {
            ++parentheses;
            continue;
        }
        if (tokens[token].text == ")") {
            --parentheses;
            continue;
        }
        if (parentheses != 0 || tokens[token].text != first) continue;
        if (second.empty() ||
            (token + 1 < end && tokens[token + 1].text == second)) return token;
    }
    return std::nullopt;
}

std::optional<bool> verilogConstantBoolean(
    const std::vector<HdlToken>& tokens, std::size_t begin, std::size_t end) {
    stripWholeParentheses(tokens, begin, end);
    if (begin >= end) return std::nullopt;

    if (const auto op = topLevelOperator(tokens, begin, end, "|", "|")) {
        const auto left = verilogConstantBoolean(tokens, begin, *op);
        const auto right = verilogConstantBoolean(tokens, *op + 2, end);
        if ((left && *left) || (right && *right)) return true;
        if (left && right) return *left || *right;
        return std::nullopt;
    }
    if (const auto op = topLevelOperator(tokens, begin, end, "&", "&")) {
        const auto left = verilogConstantBoolean(tokens, begin, *op);
        const auto right = verilogConstantBoolean(tokens, *op + 2, end);
        if ((left && !*left) || (right && !*right)) return false;
        if (left && right) return *left && *right;
        return std::nullopt;
    }
    if (tokens[begin].text == "!") {
        const auto nested = verilogConstantBoolean(tokens, begin + 1, end);
        return nested ? std::optional<bool>(!*nested) : std::nullopt;
    }
    if (begin + 1 == end && tokens[begin].kind == HdlTokenKind::Number) {
        const auto value = verilogIntegerValue(tokens[begin].text);
        return value ? std::optional<bool>(*value != 0) : std::nullopt;
    }

    if (tokens[begin].kind == HdlTokenKind::Number &&
        tokens[end - 1].kind == HdlTokenKind::Number && begin + 2 < end) {
        std::string comparison;
        for (std::size_t token = begin + 1; token + 1 < end; ++token) {
            comparison += tokens[token].text;
        }
        const auto left = verilogIntegerValue(tokens[begin].text);
        const auto right = verilogIntegerValue(tokens[end - 1].text);
        if (!left || !right) return std::nullopt;
        if (comparison == "==" || comparison == "===") return *left == *right;
        if (comparison == "!=" || comparison == "!==") return *left != *right;
        if (comparison == "<=") return *left <= *right;
        if (comparison == ">=") return *left >= *right;
        if (comparison == "<") return *left < *right;
        if (comparison == ">") return *left > *right;
    }
    return std::nullopt;
}

std::optional<std::size_t> verilogConditionSideEffect(
    const std::vector<HdlToken>& tokens, std::size_t begin, std::size_t end) {
    for (std::size_t token = begin; token < end; ++token) {
        if ((tokens[token].text == "+" || tokens[token].text == "-") &&
            token + 1 < end && tokens[token + 1].text == tokens[token].text) {
            return token;
        }
        if (tokens[token].text != "=") continue;
        if (token + 1 < end && tokens[token + 1].text == "=") continue;
        if (token > begin && (tokens[token - 1].text == "=" ||
                              tokens[token - 1].text == "!")) continue;
        if (token > begin && (tokens[token - 1].text == "<" ||
                              tokens[token - 1].text == ">")) {
            if (token >= begin + 2 &&
                tokens[token - 2].text == tokens[token - 1].text) {
                return token - 2;
            }
            continue;
        }
        if (token > begin &&
            (tokens[token - 1].text == "+" || tokens[token - 1].text == "-" ||
             tokens[token - 1].text == "*" || tokens[token - 1].text == "/" ||
             tokens[token - 1].text == "%" || tokens[token - 1].text == "&" ||
             tokens[token - 1].text == "|" || tokens[token - 1].text == "^")) {
            return token - 1;
        }
        return token;
    }
    return std::nullopt;
}

void appendVerilogConditionSideEffectIssues(
    const core::SourceFile& source, const std::vector<HdlToken>& tokens,
    const std::vector<ProcessRange>& processes, std::size_t limit,
    std::vector<SemanticIssue>& issues, const std::string& unitName) {
    std::set<std::size_t> reportedOffsets;
    const auto appendIssue = [&](std::size_t sideEffect) {
        if (!reportedOffsets.insert(tokens[sideEffect].offset).second) return;
        issues.push_back({"VL-SIDE-EFFECT-CONDITION",
            "Condition expression modifies state",
            "A runtime control condition contains an assignment, increment, or decrement operator, so evaluating the condition also changes design state and may hide a mistaken comparison.",
            "Move the state update to a separate statement and keep the condition side-effect free; verify that assignment was not intended to be equality comparison.",
            location(source, tokens[sideEffect]), unitName, std::nullopt});
    };
    for (const auto& process : processes) {
        for (std::size_t token = process.begin;
             token + 1 < process.end && token + 1 < limit; ++token) {
            if ((!tokenIs(tokens[token], "if") && !tokenIs(tokens[token], "while")) ||
                tokens[token + 1].text != "(") continue;
            const auto after = skipBalanced(
                tokens, token + 1, std::min(process.end, limit), "(", ")");
            if (!after) continue;
            const auto sideEffect = verilogConditionSideEffect(
                tokens, token + 2, *after - 1);
            if (sideEffect) appendIssue(*sideEffect);
            token = *after - 1;
        }
        for (const auto& condition : verilogConditionalExpressionConditions(
                 tokens, process.begin, std::min(process.end, limit))) {
            const auto sideEffect = verilogConditionSideEffect(
                tokens, condition.begin, condition.end);
            if (sideEffect) appendIssue(*sideEffect);
        }
    }
}

std::optional<std::size_t> verilogEmptyBranchToken(
    const std::vector<HdlToken>& tokens, std::size_t statement,
    std::size_t limit) {
    if (statement >= limit) return std::nullopt;
    if (tokens[statement].text == ";") return statement;
    if (!tokenIs(tokens[statement], "begin")) return std::nullopt;

    int depth = 0;
    bool executable = false;
    for (std::size_t token = statement; token < limit; ++token) {
        if (tokenIs(tokens[token], "begin")) {
            ++depth;
            if (token + 2 < limit && tokens[token + 1].text == ":" &&
                identifier(tokens[token + 2])) token += 2;
            continue;
        }
        if (tokenIs(tokens[token], "end")) {
            if (--depth == 0) return executable ? std::nullopt
                                                : std::optional<std::size_t>(statement);
            continue;
        }
        if (tokens[token].text != ";") executable = true;
    }
    return std::nullopt;
}

std::size_t verilogBranchEnd(const std::vector<HdlToken>& tokens,
                             std::size_t statement, std::size_t limit) {
    if (statement < limit && tokens[statement].text == ";") return statement + 1;
    return verilogStatementEnd(tokens, statement, limit);
}

void appendVerilogEmptyConditionalBranchIssues(
    const core::SourceFile& source, const std::vector<HdlToken>& tokens,
    const std::vector<ProcessRange>& processes, std::size_t limit,
    std::vector<SemanticIssue>& issues, const std::string& unitName) {
    for (const auto& process : processes) {
        for (std::size_t token = process.begin;
             token + 1 < process.end && token + 1 < limit; ++token) {
            if (!tokenIs(tokens[token], "if") || tokens[token + 1].text != "(") continue;
            const auto afterCondition = skipBalanced(
                tokens, token + 1, std::min(process.end, limit), "(", ")");
            if (!afterCondition) continue;
            if (const auto empty = verilogEmptyBranchToken(
                    tokens, *afterCondition, std::min(process.end, limit))) {
                issues.push_back({"VL-EMPTY-CONDITIONAL-BRANCH",
                    "Conditional branch has no executable statement",
                    "An if/else branch is an empty statement or structurally empty begin/end block, which often indicates a stray semicolon or missing RTL behavior.",
                    "Remove an unintended semicolon or implement the branch; if the empty branch is intentional, document and locally suppress this rule.",
                    location(source, tokens[*empty]), unitName, std::nullopt});
            }
            const auto afterThen = verilogBranchEnd(
                tokens, *afterCondition, std::min(process.end, limit));
            if (afterThen < process.end && afterThen < limit &&
                tokenIs(tokens[afterThen], "else") && afterThen + 1 < process.end &&
                !tokenIs(tokens[afterThen + 1], "if")) {
                if (const auto empty = verilogEmptyBranchToken(
                        tokens, afterThen + 1, std::min(process.end, limit))) {
                    issues.push_back({"VL-EMPTY-CONDITIONAL-BRANCH",
                        "Conditional branch has no executable statement",
                        "An if/else branch is an empty statement or structurally empty begin/end block, which often indicates a stray semicolon or missing RTL behavior.",
                        "Remove an unintended semicolon or implement the branch; if the empty branch is intentional, document and locally suppress this rule.",
                        location(source, tokens[*empty]), unitName, std::nullopt});
                }
            }
            token = *afterCondition - 1;
        }
    }
}

void appendVerilogConstantConditionIssues(
    const core::SourceFile& source, const std::vector<HdlToken>& tokens,
    const std::vector<ProcessRange>& processes, std::size_t body, std::size_t limit,
    std::vector<SemanticIssue>& issues, const std::string& unitName) {
    for (const auto& process : processes) {
        for (std::size_t token = process.begin;
            token + 2 < process.end && token + 2 < limit; ++token) {
            if ((!tokenIs(tokens[token], "if") && !tokenIs(tokens[token], "while")) ||
                tokens[token + 1].text != "(") continue;
            const auto after = skipBalanced(
                tokens, token + 1, process.end, "(", ")");
            if (!after) continue;
            const auto value = verilogConstantBoolean(
                tokens, token + 2, *after - 1);
            if (!value) continue;
            const bool condition = *value;
            issues.push_back({"RTL-CONSTANT-CONDITION",
                std::string("Procedural condition is always ") +
                    (condition ? "true" : "false"),
                "A procedural if/while condition can be evaluated as a constant, so one control-flow path is unreachable or unconditional.",
                "Remove the dead branch or replace the constant expression with the intended runtime condition.",
                location(source, tokens[token + 1]), unitName, std::nullopt});
            token = *after - 1;
        }
    }
    for (const auto& condition :
         verilogConditionalExpressionConditions(tokens, body, limit)) {
        bool runtime = std::ranges::any_of(
            processes, [&](const ProcessRange& process) {
                return condition.end >= process.begin && condition.end < process.end;
            });
        if (!runtime) {
            std::size_t statementBegin = body;
            for (std::size_t cursor = condition.end; cursor > body; --cursor) {
                if (tokens[cursor - 1].text == ";") {
                    statementBegin = cursor;
                    break;
                }
            }
            for (std::size_t token = statementBegin; token < condition.end; ++token) {
                if (tokenIs(tokens[token], "assign")) {
                    runtime = true;
                    break;
                }
            }
        }
        if (!runtime) continue;
        const auto value = verilogConstantBoolean(
            tokens, condition.begin, condition.end);
        if (!value) continue;
        issues.push_back({"RTL-CONSTANT-CONDITION",
            std::string("Conditional expression guard is always ") +
                (*value ? "true" : "false"),
            "A runtime ternary guard can be evaluated as a constant, so one result arm is unreachable.",
            "Remove the unreachable arm or replace the constant expression with the intended runtime condition.",
            location(source, tokens[condition.begin]), unitName, std::nullopt});
    }
}

std::optional<bool> verilogConditionPolarity(
    const std::vector<HdlToken>& tokens, std::size_t begin, std::size_t end,
    std::string_view signal) {
    while (begin < end && tokens[begin].text == "(") {
        const auto after = skipBalanced(tokens, begin, end, "(", ")");
        if (!after || *after != end) break;
        ++begin;
        --end;
    }
    if (begin >= end) return std::nullopt;
    if ((tokens[begin].text == "!" || tokens[begin].text == "~") &&
        begin + 1 < end && tokens[begin + 1].text != "=") {
        const auto nested = verilogConditionPolarity(tokens, begin + 1, end, signal);
        return nested ? std::optional<bool>(!*nested) : std::nullopt;
    }
    if (begin + 1 == end && identifier(tokens[begin]) &&
        tokens[begin].text == signal) return true;

    const auto comparison = [&](std::size_t signalToken, std::size_t valueToken,
                                std::size_t operatorBegin,
                                std::size_t operatorEnd) -> std::optional<bool> {
        if (signalToken >= end || valueToken >= end ||
            !identifier(tokens[signalToken]) || tokens[signalToken].text != signal ||
            tokens[valueToken].kind != HdlTokenKind::Number) return std::nullopt;
        std::string op;
        for (std::size_t token = operatorBegin; token < operatorEnd; ++token) {
            op += tokens[token].text;
        }
        if (op != "==" && op != "===" && op != "!=" && op != "!==") {
            return std::nullopt;
        }
        const auto value = verilogIntegerValue(tokens[valueToken].text);
        if (!value || *value > 1) return std::nullopt;
        const bool equal = op == "==" || op == "===";
        const bool high = *value == 1;
        return equal ? high : !high;
    };
    if (identifier(tokens[begin]) && tokens[begin].text == signal &&
        tokens[end - 1].kind == HdlTokenKind::Number) {
        return comparison(begin, end - 1, begin + 1, end - 1);
    }
    if (tokens[begin].kind == HdlTokenKind::Number &&
        identifier(tokens[end - 1]) && tokens[end - 1].text == signal) {
        return comparison(end - 1, begin, begin + 1, end - 1);
    }
    return std::nullopt;
}

bool verilogProceduralDeclarationKeyword(const HdlToken& token) {
    return tokenIs(token, "logic") || tokenIs(token, "reg") ||
           tokenIs(token, "bit") || tokenIs(token, "byte") ||
           tokenIs(token, "shortint") || tokenIs(token, "int") ||
           tokenIs(token, "integer") || tokenIs(token, "longint") ||
           tokenIs(token, "time") || tokenIs(token, "real") ||
           tokenIs(token, "realtime") || tokenIs(token, "shortreal") ||
           tokenIs(token, "string") || tokenIs(token, "event") ||
           tokenIs(token, "chandle") || tokenIs(token, "var") ||
           tokenIs(token, "automatic") || tokenIs(token, "static") ||
           tokenIs(token, "const");
}

void skipVerilogProceduralDeclarations(const std::vector<HdlToken>& tokens,
                                       std::size_t limit,
                                       std::size_t& statement) {
    while (statement < limit &&
           verilogProceduralDeclarationKeyword(tokens[statement])) {
        while (statement < limit && tokens[statement].text != ";") ++statement;
        if (statement < limit) ++statement;
        while (statement < limit && tokens[statement].text == ";") ++statement;
    }
}

bool verilogResetIfHasTrailingLogic(const std::vector<HdlToken>& tokens,
                                    std::size_t statement,
                                    std::size_t limit) {
    const auto afterIf = verilogStatementEnd(tokens, statement, limit);
    for (std::size_t token = afterIf; token < limit; ++token) {
        if (!identifier(tokens[token])) continue;
        if (verilogAssignmentOperator(tokens, token, afterIf, limit)) return true;
    }
    return false;
}

void appendVerilogAsyncResetPolarityIssues(
    const core::SourceFile& source, const std::vector<HdlToken>& tokens,
    const std::vector<ProcessRange>& processes, std::size_t limit,
    std::vector<SemanticIssue>& issues, const std::string& unitName) {
    for (const auto& process : processes) {
        if (!tokenIs(tokens[process.keyword], "always") &&
            !tokenIs(tokens[process.keyword], "always_ff")) continue;
        std::size_t sensitivity = process.keyword + 1;
        if (sensitivity >= limit || tokens[sensitivity].text != "@") continue;
        ++sensitivity;
        if (sensitivity >= limit || tokens[sensitivity].text != "(") continue;
        const auto afterSensitivity = skipBalanced(
            tokens, sensitivity, limit, "(", ")");
        if (!afterSensitivity) continue;

        struct EdgeEvent {
            std::size_t signal;
            bool rising;
        };
        std::vector<EdgeEvent> events;
        for (std::size_t token = sensitivity + 1;
             token + 1 < *afterSensitivity; ++token) {
            if (!tokenIs(tokens[token], "posedge") &&
                !tokenIs(tokens[token], "negedge")) continue;
            std::optional<std::size_t> signal;
            if (identifier(tokens[token + 1])) {
                signal = token + 1;
            } else if (tokens[token + 1].text == "(") {
                const auto afterOperand = skipBalanced(
                    tokens, token + 1, *afterSensitivity, "(", ")");
                if (afterOperand) signal = simpleIdentifierToken(
                    tokens, token + 2, *afterOperand - 1);
            }
            if (signal) events.push_back(
                {*signal, tokenIs(tokens[token], "posedge")});
        }
        if (events.size() < 2) continue;

        std::size_t statement = process.begin;
        if (statement < process.end && tokenIs(tokens[statement], "begin")) {
            ++statement;
            if (statement + 1 < process.end && tokens[statement].text == ":" &&
                identifier(tokens[statement + 1])) statement += 2;
        }
        while (statement < process.end && tokens[statement].text == ";") ++statement;
        skipVerilogProceduralDeclarations(tokens, process.end, statement);
        std::optional<std::size_t> afterCondition;
        if (statement + 1 < process.end && tokenIs(tokens[statement], "if") &&
            tokens[statement + 1].text == "(") {
            afterCondition = skipBalanced(
                tokens, statement + 1, process.end, "(", ")");
        }

        for (const auto& event : events) {
            const auto& signal = tokens[event.signal];
            if (!asynchronousControlLikeName(signal.text)) continue;
            const auto activeHigh = afterCondition ? verilogConditionPolarity(
                tokens, statement + 2, *afterCondition - 1, signal.text) : std::nullopt;
            if (!activeHigh) {
                issues.push_back({"RTL-ASYNC-RESET-NOT-FIRST",
                    "Asynchronous reset is not the isolated leading branch: " + signal.text,
                    "An asynchronous reset event can enter the process while another condition is evaluated first, so reset behavior can be masked or become simulation/synthesis dependent.",
                    "Test the asynchronous reset alone in the first if branch, before clock-enable or functional conditions.",
                    location(source, signal), unitName, signal.text});
                continue;
            }
            if (*activeHigh != event.rising) {
                issues.push_back({"RTL-ASYNC-RESET-POLARITY-MISMATCH",
                    "Asynchronous reset event and branch polarity disagree: " + signal.text,
                    "The reset edge in the event control does not match the active level tested by the leading reset branch.",
                    "Use posedge with an active-high condition or negedge with an active-low condition.",
                    location(source, signal), unitName, signal.text});
                continue;
            }
            if (verilogResetIfHasTrailingLogic(tokens, statement, process.end)) {
                issues.push_back({"RTL-ASYNC-RESET-UNGUARDED-LOGIC",
                    "Logic executes outside the asynchronous reset if/else chain: " + signal.text,
                    "Statements after the leading reset if/else chain also execute on the asynchronous reset event and can override or bypass reset assignments.",
                    "Move all sequential behavior into the reset branch or its else chain so the asynchronous reset guards the complete process body.",
                    location(source, signal), unitName, signal.text});
            }
        }
    }
}

bool vhdlLiteral(const std::vector<HdlToken>& tokens,
                 std::size_t begin, std::size_t end) {
    if (begin + 1 == end && identifier(tokens[begin]) &&
        (tokenIs(tokens[begin], "true") || tokenIs(tokens[begin], "false"))) return true;
    if (begin + 2 == end && tokens[begin].text == "'" &&
        tokens[begin + 1].kind == HdlTokenKind::Number &&
        (tokens[begin + 1].text == "0'" || tokens[begin + 1].text == "1'")) return true;
    return begin + 3 == end && tokens[begin].text == "'" &&
           tokens[begin + 1].kind == HdlTokenKind::Number &&
           (tokens[begin + 1].text == "0" || tokens[begin + 1].text == "1") &&
           tokens[begin + 2].text == "'";
}

bool vhdlLeadingResetCondition(const std::vector<HdlToken>& tokens,
                               std::size_t begin, std::size_t end,
                               std::string_view signal) {
    while (begin < end && tokens[begin].text == "(") {
        const auto after = skipBalanced(tokens, begin, end, "(", ")");
        if (!after || *after != end) break;
        ++begin;
        --end;
    }
    if (begin >= end) return false;
    if (begin + 1 == end && identifier(tokens[begin]) &&
        equalInsensitive(tokens[begin].text, signal)) return true;
    if (begin + 2 == end && tokenIs(tokens[begin], "not") &&
        identifier(tokens[begin + 1]) &&
        equalInsensitive(tokens[begin + 1].text, signal)) return true;

    const auto comparisonEnd = [&](std::size_t operatorBegin) {
        if (operatorBegin < end && tokens[operatorBegin].text == "=") {
            return operatorBegin + 1;
        }
        if (operatorBegin + 1 < end && tokens[operatorBegin].text == "/" &&
            tokens[operatorBegin + 1].text == "=") return operatorBegin + 2;
        return std::size_t{0};
    };
    if (identifier(tokens[begin]) && equalInsensitive(tokens[begin].text, signal)) {
        const auto literalBegin = comparisonEnd(begin + 1);
        if (literalBegin != 0 && vhdlLiteral(tokens, literalBegin, end)) return true;
    }
    if (identifier(tokens[end - 1]) && equalInsensitive(tokens[end - 1].text, signal)) {
        for (std::size_t op = begin; op < end - 1; ++op) {
            const auto afterOp = comparisonEnd(op);
            if (afterOp == end - 1 && vhdlLiteral(tokens, begin, op)) return true;
        }
    }
    return false;
}

bool vhdlResetIfHasTrailingLogic(const std::vector<HdlToken>& tokens,
                                 std::size_t statement,
                                 std::size_t limit) {
    int depth = 0;
    std::optional<std::size_t> afterIf;
    for (std::size_t token = statement; token + 1 < limit; ++token) {
        if (tokenIs(tokens[token], "if") &&
            (token == statement || !tokenIs(tokens[token - 1], "end"))) {
            ++depth;
            continue;
        }
        if (!tokenIs(tokens[token], "end") || !tokenIs(tokens[token + 1], "if")) {
            continue;
        }
        if (depth > 0) --depth;
        if (depth == 0) {
            afterIf = token + 2;
            break;
        }
    }
    if (!afterIf) return false;
    for (std::size_t token = *afterIf; token < limit; ++token) {
        if (tokenIs(tokens[token], "end") && token + 1 < limit &&
            tokenIs(tokens[token + 1], "process")) break;
        if (!identifier(tokens[token]) ||
            !vhdlAssignmentOperator(tokens, token, limit)) continue;
        if (token == *afterIf || tokens[token - 1].text == ";" ||
            tokenIs(tokens[token - 1], "then") || tokenIs(tokens[token - 1], "else") ||
            tokenIs(tokens[token - 1], "begin") || tokenIs(tokens[token - 1], "loop") ||
            tokens[token - 1].text == ">") return true;
    }
    return false;
}

void appendVhdlAsyncResetOrderIssues(
    const core::SourceFile& source, const std::vector<HdlToken>& tokens,
    const std::vector<ProcessRange>& processes, std::size_t limit,
    std::vector<SemanticIssue>& issues, const std::string& unitName) {
    for (const auto& process : processes) {
        std::size_t sensitivity = process.keyword + 1;
        if (sensitivity >= process.end || tokens[sensitivity].text != "(") continue;
        const auto afterSensitivity = skipBalanced(
            tokens, sensitivity, process.end, "(", ")");
        if (!afterSensitivity) continue;
        std::vector<std::size_t> resets;
        for (std::size_t token = sensitivity + 1; token + 1 < *afterSensitivity; ++token) {
            if (!identifier(tokens[token]) ||
                !asynchronousControlLikeName(tokens[token].text)) continue;
            const auto duplicate = std::ranges::any_of(resets, [&](std::size_t existing) {
                return equalInsensitive(tokens[existing].text, tokens[token].text);
            });
            if (!duplicate) resets.push_back(token);
        }
        if (resets.empty()) continue;
        const bool clocked = std::ranges::any_of(
            tokens.begin() + static_cast<std::ptrdiff_t>(*afterSensitivity),
            tokens.begin() + static_cast<std::ptrdiff_t>(std::min(process.end, limit)),
            [](const HdlToken& token) {
                return tokenIs(token, "rising_edge") || tokenIs(token, "falling_edge") ||
                       tokenIs(token, "event");
            });
        if (!clocked) continue;

        std::size_t body = *afterSensitivity;
        int subprogramDepth = 0;
        while (body < process.end && body < limit) {
            if (tokenIs(tokens[body], "procedure") || tokenIs(tokens[body], "function")) {
                bool hasBody = false;
                for (std::size_t look = body + 1; look < process.end; ++look) {
                    if (tokenIs(tokens[look], "is")) { hasBody = true; break; }
                    if (tokens[look].text == ";") break;
                }
                if (hasBody) ++subprogramDepth;
            } else if (tokenIs(tokens[body], "end") && body + 1 < process.end &&
                       (tokenIs(tokens[body + 1], "procedure") ||
                        tokenIs(tokens[body + 1], "function"))) {
                if (subprogramDepth > 0) --subprogramDepth;
                ++body;
            } else if (tokenIs(tokens[body], "begin") && subprogramDepth == 0) {
                break;
            }
            ++body;
        }
        std::size_t statement = body < process.end ? body + 1 : process.end;
        while (statement < process.end && tokens[statement].text == ";") ++statement;
        std::optional<std::size_t> thenToken;
        if (statement < process.end && tokenIs(tokens[statement], "if")) {
            std::size_t found = statement + 1;
            while (found < process.end && !tokenIs(tokens[found], "then")) ++found;
            if (found < process.end) thenToken = found;
        }
        for (const auto reset : resets) {
            const bool leadingReset = thenToken && vhdlLeadingResetCondition(
                tokens, statement + 1, *thenToken, tokens[reset].text);
            if (!leadingReset) {
                issues.push_back({"RTL-ASYNC-RESET-NOT-FIRST",
                    "Asynchronous reset is not the isolated leading branch: " + tokens[reset].text,
                    "A sensitivity-list reset can wake the process while another condition is evaluated first, so reset behavior can be masked or become simulation/synthesis dependent.",
                    "Test the asynchronous reset alone in the first if branch, before clock-enable or functional conditions.",
                    location(source, tokens[reset]), unitName, tokens[reset].text});
                continue;
            }
            if (vhdlResetIfHasTrailingLogic(tokens, statement, process.end)) {
                issues.push_back({"RTL-ASYNC-RESET-UNGUARDED-LOGIC",
                    "Logic executes outside the asynchronous reset if/elsif/else chain: " +
                        tokens[reset].text,
                    "Statements after the leading reset if/elsif/else chain also execute on a sensitivity-list reset event and can override or bypass reset assignments.",
                    "Move all sequential behavior into the reset branch or its elsif/else chain so the asynchronous reset guards the complete process body.",
                    location(source, tokens[reset]), unitName, tokens[reset].text});
            }
        }
    }
}

std::optional<std::size_t> topLevelWordOperator(
    const std::vector<HdlToken>& tokens, std::size_t begin, std::size_t end,
    std::string_view word) {
    int parentheses = 0;
    for (std::size_t token = begin; token < end; ++token) {
        if (tokens[token].text == "(") {
            ++parentheses;
        } else if (tokens[token].text == ")") {
            --parentheses;
        } else if (parentheses == 0 && tokenIs(tokens[token], word)) {
            return token;
        }
    }
    return std::nullopt;
}

std::optional<bool> vhdlConstantBoolean(
    const std::vector<HdlToken>& tokens, std::size_t begin, std::size_t end,
    const std::map<std::string, SignalState>& signals) {
    stripWholeParentheses(tokens, begin, end);
    if (begin >= end) return std::nullopt;

    if (const auto op = topLevelWordOperator(tokens, begin, end, "or")) {
        const auto left = vhdlConstantBoolean(tokens, begin, *op, signals);
        const auto right = vhdlConstantBoolean(tokens, *op + 1, end, signals);
        if ((left && *left) || (right && *right)) return true;
        if (left && right) return *left || *right;
        return std::nullopt;
    }
    if (const auto op = topLevelWordOperator(tokens, begin, end, "and")) {
        const auto left = vhdlConstantBoolean(tokens, begin, *op, signals);
        const auto right = vhdlConstantBoolean(tokens, *op + 1, end, signals);
        if ((left && !*left) || (right && !*right)) return false;
        if (left && right) return *left && *right;
        return std::nullopt;
    }
    if (tokenIs(tokens[begin], "not")) {
        const auto nested = vhdlConstantBoolean(tokens, begin + 1, end, signals);
        return nested ? std::optional<bool>(!*nested) : std::nullopt;
    }
    if (begin + 1 == end &&
        (tokenIs(tokens[begin], "true") || tokenIs(tokens[begin], "false"))) {
        return tokenIs(tokens[begin], "true");
    }

    const auto comparisonDomain = [&](std::size_t valueBegin,
                                      std::size_t valueEnd)
        -> std::optional<std::pair<std::size_t, std::size_t>> {
        if (valueBegin + 1 == valueEnd &&
            tokens[valueBegin].kind == HdlTokenKind::Number) {
            if (const auto value = decimalValue(tokens[valueBegin].text))
                return std::pair{*value, *value};
        }
        if (valueBegin + 1 == valueEnd && tokenIs(tokens[valueBegin], "false"))
            return std::pair<std::size_t, std::size_t>{0, 0};
        if (valueBegin + 1 == valueEnd && tokenIs(tokens[valueBegin], "true"))
            return std::pair<std::size_t, std::size_t>{1, 1};
        if (valueBegin + 1 == valueEnd && identifier(tokens[valueBegin])) {
            const auto signal = signals.find(lower(tokens[valueBegin].text));
            if (signal != signals.end() && signal->second.vhdlUnsignedNumeric &&
                signal->second.width && *signal->second.width > 0 &&
                *signal->second.width < std::numeric_limits<std::size_t>::digits) {
                return std::pair<std::size_t, std::size_t>{
                    0, (std::size_t{1} << *signal->second.width) - 1};
            }
        }
        return std::nullopt;
    };
    int parentheses = 0;
    for (std::size_t op = begin; op < end; ++op) {
        if (tokens[op].text == "(") { ++parentheses; continue; }
        if (tokens[op].text == ")") { --parentheses; continue; }
        if (parentheses != 0) continue;
        std::size_t width = 0;
        enum class Operation { Equal, NotEqual, Less, LessEqual, Greater, GreaterEqual };
        std::optional<Operation> operation;
        if (tokens[op].text == "=") {
            width = 1;
            operation = Operation::Equal;
        } else if (tokens[op].text == "/" && op + 1 < end &&
                   tokens[op + 1].text == "=") {
            width = 2;
            operation = Operation::NotEqual;
        } else if (tokens[op].text == "<") {
            width = op + 1 < end && tokens[op + 1].text == "=" ? 2 : 1;
            operation = width == 2 ? Operation::LessEqual : Operation::Less;
        } else if (tokens[op].text == ">") {
            width = op + 1 < end && tokens[op + 1].text == "=" ? 2 : 1;
            operation = width == 2 ? Operation::GreaterEqual : Operation::Greater;
        }
        if (!operation) continue;
        const auto left = comparisonDomain(begin, op);
        const auto right = comparisonDomain(op + width, end);
        if (!left || !right) return std::nullopt;
        switch (*operation) {
        case Operation::Equal:
            if (left->second < right->first || right->second < left->first) return false;
            if (left->first == left->second && right->first == right->second)
                return left->first == right->first;
            break;
        case Operation::NotEqual:
            if (left->second < right->first || right->second < left->first) return true;
            if (left->first == left->second && right->first == right->second)
                return left->first != right->first;
            break;
        case Operation::Less:
            if (left->second < right->first) return true;
            if (left->first >= right->second) return false;
            break;
        case Operation::LessEqual:
            if (left->second <= right->first) return true;
            if (left->first > right->second) return false;
            break;
        case Operation::Greater:
            if (left->first > right->second) return true;
            if (left->second <= right->first) return false;
            break;
        case Operation::GreaterEqual:
            if (left->first >= right->second) return true;
            if (left->second < right->first) return false;
            break;
        }
        return std::nullopt;
    }
    return std::nullopt;
}

void appendVhdlConstantConditionIssues(
    const core::SourceFile& source, const std::vector<HdlToken>& tokens,
    const std::vector<ProcessRange>& processes, std::size_t body, std::size_t limit,
    const std::map<std::string, SignalState>& signals,
    std::vector<SemanticIssue>& issues, const std::string& unitName) {
    for (const auto& process : processes) {
        for (std::size_t token = process.begin;
             token + 1 < process.end && token + 1 < limit; ++token) {
            if ((!tokenIs(tokens[token], "if") && !tokenIs(tokens[token], "elsif") &&
                 !tokenIs(tokens[token], "while")) ||
                (token > process.begin && tokenIs(tokens[token - 1], "end"))) continue;
            const bool loop = tokenIs(tokens[token], "while");
            std::size_t terminator = token + 1;
            while (terminator < process.end && terminator < limit &&
                   !tokenIs(tokens[terminator], loop ? "loop" : "then")) ++terminator;
            if (terminator >= process.end || terminator >= limit) continue;
            const auto value = vhdlConstantBoolean(tokens, token + 1, terminator, signals);
            if (!value) continue;
            issues.push_back({"RTL-CONSTANT-CONDITION",
                std::string("Procedural condition is always ") +
                    (*value ? "true" : "false"),
                "A procedural if/elsif/while condition can be evaluated as a constant, so one control-flow path is unreachable or unconditional.",
                "Remove the dead branch or replace the constant expression with the intended runtime condition.",
                location(source, tokens[token]), unitName, std::nullopt});
            token = terminator;
        }
    }
    for (const auto& condition :
         vhdlConditionalExpressionConditions(tokens, body, limit)) {
        bool runtime = std::ranges::any_of(
            processes, [&](const ProcessRange& process) {
                return condition.begin > process.begin && condition.begin < process.end;
            });
        if (!runtime) {
            const auto whenToken = condition.begin - 1;
            std::size_t statementBegin = body;
            for (std::size_t cursor = whenToken; cursor > body; --cursor) {
                if (tokens[cursor - 1].text == ";") {
                    statementBegin = cursor;
                    break;
                }
            }
            for (std::size_t token = statementBegin; token < whenToken; ++token) {
                if (!identifier(tokens[token])) continue;
                const auto op = vhdlAssignmentOperator(
                    tokens, token, whenToken);
                if (op && *op < whenToken) {
                    runtime = true;
                    break;
                }
            }
        }
        if (!runtime) continue;
        const auto value = vhdlConstantBoolean(
            tokens, condition.begin, condition.end, signals);
        if (!value) continue;
        issues.push_back({"RTL-CONSTANT-CONDITION",
            std::string("Conditional expression guard is always ") +
                (*value ? "true" : "false"),
            "A runtime when-else guard can be evaluated as a constant, so one result arm is unreachable.",
            "Remove the unreachable arm or replace the constant expression with the intended runtime condition.",
            location(source, tokens[condition.begin]), unitName, std::nullopt});
    }
}

void appendVerilogDuplicateSensitivityIssues(
    const core::SourceFile& source, const std::vector<HdlToken>& tokens,
    const std::vector<ProcessRange>& processes, std::size_t limit,
    const std::map<std::string, SignalState>& signals,
    std::vector<SemanticIssue>& issues, const std::string& unitName) {
    for (const auto& process : processes) {
        if (!tokenIs(tokens[process.keyword], "always")) continue;
        std::size_t sensitivity = process.keyword + 1;
        if (sensitivity >= limit || tokens[sensitivity].text != "@") continue;
        ++sensitivity;
        if (sensitivity >= limit || tokens[sensitivity].text != "(") continue;
        const auto after = skipBalanced(tokens, sensitivity, limit, "(", ")");
        if (!after) continue;
        std::set<std::string> seen;
        const auto reportSegment = [&](std::size_t begin, std::size_t segmentEnd) {
            std::size_t signalToken = begin;
            std::string event;
            if (begin + 2 == segmentEnd &&
                (tokenIs(tokens[begin], "posedge") ||
                 tokenIs(tokens[begin], "negedge"))) {
                event = lower(tokens[begin].text) + ":";
                signalToken = begin + 1;
            } else if (begin + 1 != segmentEnd) {
                return;
            }
            if (!identifier(tokens[signalToken])) return;
            const auto signal = signals.find(tokens[signalToken].text);
            if (signal == signals.end()) return;
            const auto key = event + signal->first;
            if (seen.insert(key).second) return;
            issues.push_back({"RTL-DUPLICATE-SENSITIVITY",
                "Sensitivity list repeats " +
                    (event.empty() ? signal->first : event + signal->first),
                "The same signal or exact edge event appears more than once in this explicit sensitivity list.",
                "Remove the duplicate entry or use always @(*) / always_comb for combinational logic.",
                location(source, tokens[signalToken]), unitName, signal->first});
        };
        std::size_t segment = sensitivity + 1;
        int depth = 0;
        for (std::size_t token = segment; token + 1 < *after; ++token) {
            if (tokens[token].text == "(" || tokens[token].text == "[" ||
                tokens[token].text == "{") {
                ++depth;
            } else if (tokens[token].text == ")" || tokens[token].text == "]" ||
                       tokens[token].text == "}") {
                --depth;
            }
            if (depth != 0 || (tokens[token].text != "," &&
                               !tokenIs(tokens[token], "or"))) continue;
            reportSegment(segment, token);
            segment = token + 1;
        }
        reportSegment(segment, *after - 1);
    }
}

void appendVhdlDuplicateSensitivityIssues(
    const core::SourceFile& source, const std::vector<HdlToken>& tokens,
    const std::vector<ProcessRange>& processes, std::size_t limit,
    const std::map<std::string, SignalState>& signals,
    std::vector<SemanticIssue>& issues, const std::string& unitName) {
    for (const auto& process : processes) {
        const std::size_t sensitivity = process.keyword + 1;
        if (sensitivity >= limit || tokens[sensitivity].text != "(") continue;
        const auto after = skipBalanced(tokens, sensitivity, limit, "(", ")");
        if (!after) continue;
        std::set<std::string> seen;
        std::size_t segment = sensitivity + 1;
        for (std::size_t token = segment; token < *after; ++token) {
            const bool closing = token + 1 == *after;
            if (!closing && tokens[token].text != ",") continue;
            const auto segmentEnd = token;
            if (segment + 1 == segmentEnd && identifier(tokens[segment])) {
                const auto signal = signals.find(lower(tokens[segment].text));
                if (signal != signals.end() && !seen.insert(signal->first).second) {
                    issues.push_back({"RTL-DUPLICATE-SENSITIVITY",
                        "Sensitivity list repeats " + signal->first,
                        "The same signal appears more than once in this explicit sensitivity list.",
                        "Remove the duplicate entry or use process(all) with VHDL-2008 for combinational logic.",
                        location(source, tokens[segment]), unitName, signal->first});
                }
            }
            segment = token + 1;
        }
    }
}

void appendVerilogSelfAssignmentIssues(
    const core::SourceFile& source, const std::vector<HdlToken>& tokens,
    const std::vector<ProcessRange>& processes, std::size_t limit,
    const std::map<std::string, SignalState>& signals,
    std::vector<SemanticIssue>& issues, const std::string& unitName) {
    for (const auto& process : processes) {
        for (std::size_t token = process.begin;
             token < process.end && token < limit; ++token) {
            if (!identifier(tokens[token]) ||
                (token > process.begin && tokens[token - 1].text == ".")) continue;
            const auto signal = signals.find(tokens[token].text);
            if (signal == signals.end()) continue;
            const auto op = verilogAssignmentOperator(
                tokens, token, process.begin, process.end);
            if (!op || *op != token + 1) continue;
            const auto rhs = tokens[*op].text == "<" ? *op + 2 : *op + 1;
            std::size_t statementEnd = rhs;
            while (statementEnd < process.end && tokens[statementEnd].text != ";") {
                ++statementEnd;
            }
            if (rhs + 1 != statementEnd || !identifier(tokens[rhs]) ||
                tokens[rhs].text != signal->first) continue;
            issues.push_back({"RTL-SELF-ASSIGNMENT",
                "Procedural assignment writes " + signal->first + " to itself",
                "The assignment has the same direct signal on both sides and leaves its value unchanged.",
                "Remove the redundant assignment or replace it with the intended state update.",
                location(source, tokens[token]), unitName, signal->first});
            token = statementEnd;
        }
    }
}

void appendVhdlSelfAssignmentIssues(
    const core::SourceFile& source, const std::vector<HdlToken>& tokens,
    const std::vector<ProcessRange>& processes, std::size_t limit,
    const std::map<std::string, SignalState>& signals,
    std::vector<SemanticIssue>& issues, const std::string& unitName) {
    for (const auto& process : processes) {
        for (std::size_t token = process.begin;
             token < process.end && token < limit; ++token) {
            if (!identifier(tokens[token])) continue;
            const auto signal = signals.find(lower(tokens[token].text));
            if (signal == signals.end()) continue;
            const auto op = vhdlAssignmentOperator(tokens, token, process.end);
            if (!op || *op != token + 1) continue;
            const auto rhs = *op + 2;
            std::size_t statementEnd = rhs;
            while (statementEnd < process.end && tokens[statementEnd].text != ";") {
                ++statementEnd;
            }
            if (rhs + 1 != statementEnd || !identifier(tokens[rhs]) ||
                lower(tokens[rhs].text) != signal->first) continue;
            issues.push_back({"RTL-SELF-ASSIGNMENT",
                "Procedural assignment writes " + signal->first + " to itself",
                "The assignment has the same direct signal on both sides and leaves its value unchanged.",
                "Remove the redundant assignment or replace it with the intended state update.",
                location(source, tokens[token]), unitName, signal->first});
            token = statementEnd;
        }
    }
}

void appendVerilogEmptyProcessIssues(
    const core::SourceFile& source, const std::vector<HdlToken>& tokens,
    const std::vector<ProcessRange>& processes, std::size_t limit,
    std::vector<SemanticIssue>& issues, const std::string& unitName) {
    for (const auto& process : processes) {
        bool executable = false;
        for (std::size_t token = process.begin;
             token < process.end && token < limit; ++token) {
            if (tokenIs(tokens[token], "begin") && token + 2 < process.end &&
                tokens[token + 1].text == ":" && identifier(tokens[token + 2])) {
                token += 2;
                continue;
            }
            if (tokenIs(tokens[token], "begin") || tokenIs(tokens[token], "end") ||
                tokenIs(tokens[token], "fork") || tokenIs(tokens[token], "join") ||
                tokenIs(tokens[token], "join_any") || tokenIs(tokens[token], "join_none") ||
                tokens[token].text == ";") continue;
            executable = true;
            break;
        }
        if (executable) continue;
        issues.push_back({"RTL-EMPTY-PROCESS",
            "Process has no executable behavior",
            "The procedural body contains only an empty block and cannot affect the design.",
            "Remove the empty process or implement the intended procedural behavior.",
            location(source, tokens[process.keyword]), unitName, std::nullopt});
    }
}

void appendVerilogAlwaysNoSuspensionIssues(
    const core::SourceFile& source, const std::vector<HdlToken>& tokens,
    const std::vector<ProcessRange>& processes, std::size_t limit,
    std::vector<SemanticIssue>& issues, const std::string& unitName) {
    for (const auto& process : processes) {
        if (!tokenIs(tokens[process.keyword], "always")) continue;
        bool suspension = false;
        for (std::size_t token = process.keyword + 1;
             token < process.end && token < limit; ++token) {
            if (tokens[token].text == "@" || tokens[token].text == "#" ||
                tokenIs(tokens[token], "wait")) {
                suspension = true;
                break;
            }
        }
        if (suspension) continue;
        issues.push_back({"VL-ALWAYS-NO-SUSPENSION",
            "Always process has no suspension point",
            "A plain always process without an event control, delay control, or wait statement repeats forever without advancing simulation time.",
            "Add the intended event control or wait condition; use always_comb for combinational RTL.",
            location(source, tokens[process.keyword]), unitName, std::nullopt});
    }
}

void appendVhdlEmptyProcessIssues(
    const core::SourceFile& source, const std::vector<HdlToken>& tokens,
    const std::vector<ProcessRange>& processes, std::size_t limit,
    std::vector<SemanticIssue>& issues, const std::string& unitName) {
    for (const auto& process : processes) {
        std::size_t body = process.keyword + 1;
        int subprogramDepth = 0;
        while (body < process.end && body < limit) {
            if (tokenIs(tokens[body], "procedure") ||
                tokenIs(tokens[body], "function")) {
                bool hasBody = false;
                for (std::size_t look = body + 1; look < process.end; ++look) {
                    if (tokenIs(tokens[look], "is")) { hasBody = true; break; }
                    if (tokens[look].text == ";") break;
                }
                if (hasBody) ++subprogramDepth;
            } else if (tokenIs(tokens[body], "end") && body + 1 < process.end &&
                       (tokenIs(tokens[body + 1], "procedure") ||
                        tokenIs(tokens[body + 1], "function"))) {
                if (subprogramDepth > 0) --subprogramDepth;
                ++body;
            } else if (tokenIs(tokens[body], "begin") && subprogramDepth == 0) {
                break;
            }
            ++body;
        }
        if (body >= process.end || body >= limit) continue;
        bool executable = false;
        for (std::size_t token = body + 1;
             token < process.end && token < limit; ++token) {
            if (tokenIs(tokens[token], "end") && token + 1 < process.end &&
                tokenIs(tokens[token + 1], "process")) break;
            if (tokenIs(tokens[token], "null") || tokens[token].text == ";") continue;
            executable = true;
            break;
        }
        if (executable) continue;
        issues.push_back({"RTL-EMPTY-PROCESS",
            "Process has no executable behavior",
            "The process body contains only an explicit null statement and cannot affect the design.",
            "Remove the empty process or implement the intended procedural behavior.",
            location(source, tokens[process.keyword]), unitName, std::nullopt});
    }
}

void appendVerilogUnusedParameterIssues(
    const core::SourceFile& source, const std::vector<HdlToken>& tokens,
    std::size_t begin, std::size_t end, std::vector<SemanticIssue>& issues,
    const std::string& unitName) {
    std::map<std::string, const HdlToken*> declarations;
    std::set<std::size_t> declarationOffsets;
    for (std::size_t token = begin; token < end; ++token) {
        if (!tokenIs(tokens[token], "parameter") &&
            !tokenIs(tokens[token], "localparam")) continue;
        for (const auto* name : declarationNames(tokens, token + 1, end, false)) {
            declarationOffsets.insert(name->offset);
            declarations.try_emplace(name->text, name);
        }
    }
    for (const auto& [name, declaration] : declarations) {
        bool used = false;
        for (std::size_t token = begin; token < end; ++token) {
            if (tokens[token].text != name ||
                declarationOffsets.contains(tokens[token].offset) ||
                (token > begin && tokens[token - 1].text == ".")) continue;
            used = true;
            break;
        }
        if (used) continue;
        issues.push_back({"RTL-UNUSED-PARAMETER",
            "Parameter is never referenced: " + name,
            "The parameter or local parameter does not affect this design unit interface or implementation.",
            "Remove the unused declaration or connect it to the intended width, type, generate condition, or behavior.",
            location(source, *declaration), unitName, name});
    }
}

std::vector<SemanticIssue> analyzeUnusedVhdlGenericsImpl(
    const std::vector<core::SourceFile>& sources) {
    struct IndexedSource {
        std::vector<HdlToken> tokens;
    };
    struct EntityRecord {
        DesignUnitInfo unit;
        std::size_t source;
    };
    struct TokenRange {
        std::size_t source;
        std::size_t begin;
        std::size_t end;
    };
    std::vector<IndexedSource> indexed;
    std::map<std::string, std::vector<EntityRecord>> entities;
    for (const auto& source : sources) {
        if (source.language() != core::HdlLanguage::Vhdl) continue;
        const auto sourceIndex = indexed.size();
        indexed.push_back({tokenize(source)});
        for (auto unit : parseDesignUnits(source)) {
            if (unit.kind == DesignUnitKind::VhdlEntity && !unit.parameters.empty()) {
                entities[lower(unit.name)].push_back({std::move(unit), sourceIndex});
            }
        }
    }
    std::vector<SemanticIssue> issues;
    for (const auto& [entityName, definitions] : entities) {
        if (definitions.size() != 1) continue;
        const auto& definition = definitions.front();
        std::vector<TokenRange> ranges;
        const auto& entityTokens = indexed[definition.source].tokens;
        for (std::size_t token = 1; token < entityTokens.size(); ++token) {
            if (entityTokens[token].offset != definition.unit.location.byteOffset ||
                !tokenIs(entityTokens[token - 1], "entity")) continue;
            std::size_t rangeEnd = token + 1;
            while (rangeEnd + 1 < entityTokens.size() &&
                   !(tokenIs(entityTokens[rangeEnd], "end") &&
                     (tokenIs(entityTokens[rangeEnd + 1], "entity") ||
                      tokenIs(entityTokens[rangeEnd + 1], definition.unit.name) ||
                      entityTokens[rangeEnd + 1].text == ";"))) ++rangeEnd;
            ranges.push_back({definition.source, token - 1,
                              std::min(rangeEnd + 2, entityTokens.size())});
            break;
        }
        bool hasArchitecture = false;
        for (std::size_t sourceIndex = 0; sourceIndex < indexed.size(); ++sourceIndex) {
            const auto& tokens = indexed[sourceIndex].tokens;
            for (std::size_t token = 0; token + 3 < tokens.size(); ++token) {
                if (!tokenIs(tokens[token], "architecture") ||
                    !tokenIs(tokens[token + 2], "of") ||
                    !tokenIs(tokens[token + 3], entityName)) continue;
                std::size_t rangeEnd = token + 4;
                while (rangeEnd + 1 < tokens.size() &&
                       !(tokenIs(tokens[rangeEnd], "end") &&
                         (tokenIs(tokens[rangeEnd + 1], "architecture") ||
                          tokenIs(tokens[rangeEnd + 1], tokens[token + 1].text)))) ++rangeEnd;
                ranges.push_back({sourceIndex, token,
                                  std::min(rangeEnd + 2, tokens.size())});
                hasArchitecture = true;
                token = rangeEnd;
            }
        }
        if (!hasArchitecture) continue;
        std::set<std::size_t> declarationOffsets;
        for (const auto& parameter : definition.unit.parameters) {
            declarationOffsets.insert(parameter.location.byteOffset);
        }
        for (const auto& parameter : definition.unit.parameters) {
            const auto name = lower(parameter.name);
            bool used = false;
            for (const auto& range : ranges) {
                const auto& tokens = indexed[range.source].tokens;
                for (std::size_t token = range.begin; token < range.end; ++token) {
                    const bool declaration = range.source == definition.source &&
                        declarationOffsets.contains(tokens[token].offset);
                    if (!tokenIs(tokens[token], name) || declaration) continue;
                    if (token + 2 < range.end && tokens[token + 1].text == "=" &&
                        tokens[token + 2].text == ">") continue;
                    used = true;
                    break;
                }
                if (used) break;
            }
            if (used) continue;
            issues.push_back({"RTL-UNUSED-PARAMETER",
                "Generic is never referenced: " + parameter.name,
                "The generic does not affect this entity interface or any analyzed architecture.",
                "Remove the unused generic or connect it to the intended width, type, generate condition, or behavior.",
                parameter.location, definition.unit.name, parameter.name});
        }
    }
    return issues;
}

std::vector<SemanticIssue> analyzeVerilog(const core::SourceFile& source) {
    const auto tokens = tokenize(source);
    std::vector<SemanticIssue> issues;

    // FUN_180533200 resolves a cell type when it is available and distinguishes
    // output formals (drivers of the actual) from input/inout formals (loads of
    // the actual).  Build the same bounded, same-source interface index before
    // analyzing individual modules.  Duplicate module names deliberately stay
    // unresolved so that ambiguous project context cannot be guessed.
    std::map<std::string, std::vector<VerilogModuleInterface>> moduleInterfaces;
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

        VerilogModuleInterface interface;
        for (std::size_t token = module + 2; token < end; ++token) {
            if (!directionKeyword(tokens[token])) continue;
            const auto direction = directionOf(tokens[token]);
            for (const auto* name : declarationNames(tokens, token + 1, end, true))
                interface.directions.insert_or_assign(name->text, direction);
        }
        std::set<std::string> ordered;
        for (std::size_t token = module + 2; token < body; ++token) {
            if (!identifier(tokens[token]) ||
                !interface.directions.contains(tokens[token].text) ||
                !ordered.insert(tokens[token].text).second) continue;
            interface.portOrder.push_back(tokens[token].text);
        }
        moduleInterfaces[tokens[module + 1].text].push_back(std::move(interface));
        module = end > 0 ? end - 1 : end;
    }

    for (std::size_t module = 0; module + 1 < tokens.size(); ++module) {
        if (!tokenIs(tokens[module], "module") || !identifier(tokens[module + 1])) continue;
        const auto moduleName = tokens[module + 1].text;
        std::size_t end = module + 2;
        while (end < tokens.size() && !tokenIs(tokens[end], "endmodule")) ++end;
        appendVerilogUnusedParameterIssues(
            source, tokens, module + 2, end, issues, moduleName);
        std::size_t body = module + 2;
        int parentheses = 0;
        for (; body < end; ++body) {
            if (tokens[body].text == "(") ++parentheses;
            else if (tokens[body].text == ")") --parentheses;
            else if (tokens[body].text == ";" && parentheses == 0) { ++body; break; }
        }
        const auto processes = verilogProcesses(tokens, body, end);
        const auto parameterDefaults = verilogModuleParameterDefaults(
            tokens, module + 2, body, end, processes);
        for (std::size_t token = body; token < end; ++token) {
            if (!tokenIs(tokens[token], "defparam")) continue;
            issues.push_back({"VL-DEFPARAM", "defparam overrides an instance parameter",
                "GJB10157.5.2.16 rejects defparam because the override is separated from the instance and is fragile under hierarchy changes.",
                "Use a named parameter override in the instance #(...) list.",
                location(source, tokens[token]), moduleName, std::nullopt});
        }
        appendVerilogComplexEdgeEventIssues(
            source, tokens, processes, end, issues, moduleName);
        appendVerilogDuplicateConditionIssues(
            source, tokens, processes, end, issues, moduleName);
        appendVerilogConditionSideEffectIssues(
            source, tokens, processes, end, issues, moduleName);
        appendVerilogEmptyConditionalBranchIssues(
            source, tokens, processes, end, issues, moduleName);
        appendVerilogEmptyProcessIssues(
            source, tokens, processes, end, issues, moduleName);
        appendVerilogAlwaysNoSuspensionIssues(
            source, tokens, processes, end, issues, moduleName);
        appendVerilogConstantConditionIssues(
            source, tokens, processes, body, end, issues, moduleName);
        appendVerilogAsyncResetPolarityIssues(
            source, tokens, processes, end, issues, moduleName);
        if (source.language() == core::HdlLanguage::SystemVerilog) {
            for (const auto& process : processes) {
                std::vector<std::size_t> eventControls;
                for (std::size_t token = process.keyword + 1;
                     token < process.end && token < end; ++token) {
                    if (tokens[token].text == "@") eventControls.push_back(token);
                }
                if (tokenIs(tokens[process.keyword], "always_comb") &&
                    !eventControls.empty()) {
                    const auto event = eventControls.front();
                    issues.push_back({"SV-ALWAYS-COMB-EVENT-CONTROL",
                        "always_comb contains an explicit event control",
                        "always_comb derives its sensitivity automatically and cannot contain an explicit event control.",
                        "Remove the event control and retain only the combinational procedure body.",
                        location(source, tokens[event]), moduleName, std::nullopt});
                }
                if (tokenIs(tokens[process.keyword], "always_latch") &&
                    !eventControls.empty()) {
                    const auto event = eventControls.front();
                    issues.push_back({"SV-ALWAYS-LATCH-EVENT-CONTROL",
                        "always_latch contains an explicit event control",
                        "always_latch derives its sensitivity automatically and cannot contain an explicit event control.",
                        "Remove the event control and retain only the level-sensitive procedure body.",
                        location(source, tokens[event]), moduleName, std::nullopt});
                }
                if (!tokenIs(tokens[process.keyword], "always_ff")) continue;
                if (eventControls.size() != 1) {
                    const auto locationToken = eventControls.size() > 1 ?
                        eventControls[1] : process.keyword;
                    const auto message = eventControls.empty() ?
                        "always_ff has no event control" :
                        "always_ff contains more than one event control";
                    issues.push_back({"SV-ALWAYS-FF-EVENT-CONTROL", message,
                        "always_ff must contain exactly one event control for its sequential behavior.",
                        "Use one clock/reset event control or split the logic into separate processes.",
                        location(source, tokens[locationToken]), moduleName, std::nullopt});
                    continue;
                }
                const auto event = eventControls.front();
                std::size_t eventEnd = std::min(process.begin, process.end);
                if (event + 1 < process.end && tokens[event + 1].text == "(") {
                    const auto after = skipBalanced(
                        tokens, event + 1, process.end, "(", ")");
                    if (after) eventEnd = *after;
                }
                const bool hasEdge = std::ranges::any_of(
                    tokens.begin() + static_cast<std::ptrdiff_t>(event + 1),
                    tokens.begin() + static_cast<std::ptrdiff_t>(eventEnd),
                    [](const HdlToken& token) {
                        return tokenIs(token, "posedge") || tokenIs(token, "negedge");
                    });
                if (hasEdge) continue;
                issues.push_back({"SV-ALWAYS-FF-NONEDGE-EVENT",
                    "always_ff event control has no edge event",
                    "An always_ff procedure triggered by a level/change event does not describe ordinary edge-triggered register behavior.",
                    "Use posedge or negedge on the intended clock, with optional asynchronous reset/set edges.",
                    location(source, tokens[event]), moduleName, std::nullopt});
            }
        }
        appendVerilogDelayIssues(source, tokens, body, end, processes, issues, moduleName);
        appendVerilogDivideByZeroIssues(
            source, tokens, body, end, issues, moduleName);
        std::map<std::string, SignalState> signals;
        std::set<std::string> ansiPorts;

        for (std::size_t i = module + 2; i < end; ++i) {
            if (!directionKeyword(tokens[i])) continue;
            const auto direction = directionOf(tokens[i]);
            for (const auto* name : declarationNames(tokens, i + 1, end, true)) {
                const auto index = static_cast<std::size_t>(name - tokens.data());
                if (index < body) ansiPorts.insert(name->text);
                auto& signal = signals[name->text];
                signal.direction = direction;
                signal.declaration = location(source, *name);
                signal.memory = index + 1 < end && tokens[index + 1].text == "[";
                signal.width = verilogDeclarationWidth(tokens, i, index, parameterDefaults);
                signal.range = verilogDeclarationRange(tokens, i, index, parameterDefaults);
                signal.verilogSignedness = verilogDeclarationSignedness(tokens, i, index);
                signal.unpackedRanges =
                    verilogUnpackedDeclarationRanges(tokens, index, end, parameterDefaults);
            }
        }
        int declarationDriverId = 50000;
        std::set<std::string> internalDeclarations;
        for (std::size_t i = body; i < end; ++i) {
            if (inProcess(i, processes) || !typeKeyword(tokens[i])) continue;
            std::size_t statementBegin = i;
            while (statementBegin > body && tokens[statementBegin - 1].text != ";") {
                --statementBegin;
            }
            const bool earlierTypeKeyword = std::ranges::any_of(
                tokens.begin() + static_cast<std::ptrdiff_t>(statementBegin),
                tokens.begin() + static_cast<std::ptrdiff_t>(i),
                [](const HdlToken& token) { return typeKeyword(token); });
            if (earlierTypeKeyword) continue;
            const bool portDeclaration = std::ranges::any_of(
                tokens.begin() + static_cast<std::ptrdiff_t>(statementBegin),
                tokens.begin() + static_cast<std::ptrdiff_t>(i),
                [](const HdlToken& token) { return directionKeyword(token); });
            for (const auto* name : declarationNames(tokens, i + 1, end, false)) {
                const auto index = static_cast<std::size_t>(name - tokens.data());
                const bool duplicateInternal = !portDeclaration &&
                    (!internalDeclarations.insert(name->text).second ||
                     ansiPorts.contains(name->text));
                if (duplicateInternal) {
                    issues.push_back({"STRUCT-DUPLICATE-SIGNAL-DECLARATION",
                        "Signal is declared more than once: " + name->text,
                        "An internal signal declaration repeats an existing name in the same module scope.",
                        "Remove or rename the later declaration so every signal name is unique within the module.",
                        location(source, *name), moduleName, name->text});
                    continue;
                }
                if (!signals.contains(name->text)) {
                    signals[name->text] = {Direction::Internal, location(source, *name), {},
                                           index + 1 < end && tokens[index + 1].text == "[",
                                           false, false,
                                           verilogDeclarationWidth(tokens, i, index, parameterDefaults),
                                           verilogDeclarationRange(tokens, i, index, parameterDefaults),
                                           false,
                                           verilogDeclarationSignedness(tokens, i, index),
                                           std::nullopt,
                                           verilogUnpackedDeclarationRanges(
                                               tokens, index, end, parameterDefaults)};
                }
                std::size_t afterName = index + 1;
                while (afterName < end && tokens[afterName].text == "[") {
                    const auto afterRange = skipBalanced(tokens, afterName, end, "[", "]");
                    if (!afterRange) break;
                    afterName = *afterRange;
                }
                if (afterName < end && tokens[afterName].text == "=") {
                    addDriver(signals[name->text], declarationDriverId++, location(source, *name), true);
                }
            }
        }
        appendVerilogCaseIssues(
            source, tokens, body, end, signals, issues, moduleName);
        appendVerilogComparisonWidthIssues(
            source, tokens, processes, body, end, signals, issues, moduleName);
        appendVerilogBitwiseWidthIssues(
            source, tokens, processes, body, end, signals, issues, moduleName);
        appendVerilogMixedSignednessArithmeticIssues(
            source, tokens, processes, body, end, signals, issues, moduleName);
        appendVerilogConditionalExpressionComparisonWidthIssues(
            source, tokens, body, end, signals, issues, moduleName);
        appendVerilogShiftAmountIssues(
            source, tokens, body, end, signals, issues, moduleName);
        appendVerilogNegativeShiftIssues(
            source, tokens, body, end, issues, moduleName);
        appendConstantSelectIssues(source, tokens, body, end, signals, false,
                                   issues, moduleName);
        appendVerilogDuplicateSensitivityIssues(
            source, tokens, processes, end, signals, issues, moduleName);
        appendVerilogSelfAssignmentIssues(
            source, tokens, processes, end, signals, issues, moduleName);

        std::set<std::string> valueIdentifiers;
        for (std::size_t i = module + 2; i < end; ++i) {
            if (!tokenIs(tokens[i], "parameter") && !tokenIs(tokens[i], "localparam") &&
                !tokenIs(tokens[i], "genvar")) continue;
            for (const auto* name : declarationNames(tokens, i + 1, end, false)) {
                valueIdentifiers.insert(name->text);
            }
        }
        std::set<std::string> reportedImplicitNets;
        const auto reportImplicitNet = [&](const HdlToken& token) {
            if (source.language() != core::HdlLanguage::Verilog ||
                signals.contains(token.text) || valueIdentifiers.contains(token.text) ||
                !reportedImplicitNets.insert(token.text).second) return;
            issues.push_back({"VL-IMPLICIT-NET",
                "Undeclared identifier creates an implicit net: " + token.text,
                "A simple continuous-assignment endpoint or module-instance port actual is used without a matching net or port declaration.",
                "Declare the net explicitly and enable the Verilog default-nettype-none directive to make future typos compile errors.",
                location(source, token), moduleName, token.text});
        };

        int continuousId = 100000;
        std::set<std::size_t> assignmentTargets;
        for (std::size_t i = body; i + 1 < end; ++i) {
            if (tokenIs(tokens[i], "assign") && identifier(tokens[i + 1])) {
                const auto found = signals.find(tokens[i + 1].text);
                if (found != signals.end()) {
                    addDriver(found->second, continuousId++, location(source, tokens[i + 1]));
                    assignmentTargets.insert(tokens[i + 1].offset);
                } else if (!valueIdentifiers.contains(tokens[i + 1].text)) {
                    reportImplicitNet(tokens[i + 1]);
                }
                const auto op = assignmentOperator(tokens, i + 1, end);
                if (op) {
                    const auto rhs = tokens[*op].text == "<" ? *op + 2 : *op + 1;
                    std::size_t statementEnd = rhs;
                    while (statementEnd < end && tokens[statementEnd].text != ";") {
                        ++statementEnd;
                    }
                    if (rhs + 1 == statementEnd && identifier(tokens[rhs])) {
                        reportImplicitNet(tokens[rhs]);
                    }
                }
            }
        }
        for (const auto& process : processes) {
            for (std::size_t i = process.begin; i < process.end && i < end; ++i) {
                if (!identifier(tokens[i]) || (i > process.begin && tokens[i - 1].text == ".")) continue;
                const auto op = verilogAssignmentOperator(
                    tokens, i, process.begin, process.end);
                if (!op) continue;
                const auto found = signals.find(tokens[i].text);
                if (found != signals.end()) {
                    addDriver(found->second, process.driverId, location(source, tokens[i]),
                              process.initialization);
                    assignmentTargets.insert(tokens[i].offset);
                }
            }
        }
        for (const auto& process : processes) {
            const bool sequential = verilogSequentialProcess(tokens, process, end);
            const bool combinational = verilogCombinationalProcess(tokens, process, end);
            std::set<std::size_t> loopHeaderTokens;
            for (std::size_t token = process.begin; token + 1 < process.end; ++token) {
                if (!tokenIs(tokens[token], "for") || tokens[token + 1].text != "(") continue;
                const auto after = skipBalanced(
                    tokens, token + 1, process.end, "(", ")");
                if (!after) continue;
                for (std::size_t header = token; header < *after; ++header) {
                    loopHeaderTokens.insert(header);
                }
                token = *after - 1;
            }
            std::map<std::string, bool> firstNonblocking;
            std::set<std::string> reported;
            for (std::size_t token = process.begin;
                 token < process.end && token < end; ++token) {
                if (!identifier(tokens[token]) || loopHeaderTokens.contains(token) ||
                    (token > process.begin && tokens[token - 1].text == ".")) continue;
                const auto op = verilogAssignmentOperator(
                    tokens, token, process.begin, process.end);
                if (!op) continue;
                const bool nonblocking = tokens[*op].text == "<";
                if (sequential && !nonblocking) {
                    issues.push_back({"VL-SEQ-BLOCKING",
                        "Blocking assignment in sequential process",
                        "Blocking assignments in edge-triggered logic can create simulation and ordering dependencies.",
                        "Use a nonblocking assignment (<=) for registers.",
                        location(source, tokens[token]), moduleName, tokens[token].text});
                } else if (combinational && nonblocking) {
                    issues.push_back({"VL-COMB-NONBLOCKING",
                        "Nonblocking assignment in combinational process",
                        "Nonblocking scheduling can make combinational simulation differ from hardware intent.",
                        "Use a blocking assignment (=) in combinational logic.",
                        location(source, tokens[token]), moduleName, tokens[token].text});
                }
                const auto signal = signals.find(tokens[token].text);
                if (signal == signals.end()) continue;
                const auto [first, inserted] =
                    firstNonblocking.try_emplace(signal->first, nonblocking);
                if (inserted || first->second == nonblocking ||
                    !reported.insert(signal->first).second) continue;
                issues.push_back({"VL-MIXED-ASSIGNMENT-STYLE",
                    "Signal uses both blocking and nonblocking assignments in one process: " +
                        tokens[token].text,
                    "Mixing procedural assignment scheduling for the same signal makes simulation ordering and hardware intent difficult to reason about.",
                    "Use one assignment style consistently for this signal within the process.",
                    location(source, tokens[token]), moduleName, tokens[token].text});
            }
        }

        std::map<std::string, std::string> clockSignals;
        for (const auto& process : processes) {
            std::size_t sensitivity = process.keyword + 1;
            if (sensitivity < end && tokens[sensitivity].text == "@") ++sensitivity;
            if (sensitivity >= end || tokens[sensitivity].text != "(") continue;
            const auto after = skipBalanced(tokens, sensitivity, end, "(", ")");
            if (!after) continue;
            for (std::size_t token = sensitivity + 1; token + 1 < *after; ++token) {
                if ((!tokenIs(tokens[token], "posedge") &&
                     !tokenIs(tokens[token], "negedge")) || !identifier(tokens[token + 1]) ||
                    resetLikeName(tokens[token + 1].text)) continue;
                clockSignals.try_emplace(tokens[token + 1].text, tokens[token + 1].text);
            }
        }
        appendDerivedClockIssues(signals, clockSignals, issues, moduleName);

        std::set<std::string> knownSignals;
        for (const auto& [name, signal] : signals) {
            static_cast<void>(signal);
            knownSignals.insert(name);
        }
        for (std::size_t i = body; i < end; ++i) {
            if (!identifier(tokens[i]) ||
                (i > body && tokens[i - 1].text == ".")) continue;
            const auto target = signals.find(tokens[i].text);
            if (target == signals.end() || target->second.memory) continue;
            std::optional<std::size_t> op;
            const auto owner = std::ranges::find_if(processes, [&](const ProcessRange& process) {
                return i >= process.begin && i < process.end;
            });
            if (owner != processes.end()) {
                op = verilogAssignmentOperator(tokens, i, owner->begin, owner->end);
            } else if (i > body && tokenIs(tokens[i - 1], "assign")) {
                op = assignmentOperator(tokens, i, end);
            }
            if (!op) continue;
            const auto targetShape = verilogCaseSelectorShape(
                tokens, i, *op, signals);
            if (!targetShape) continue;
            auto effectiveTarget = target->second;
            effectiveTarget.width = targetShape->width;
            const auto rhs = tokens[*op].text == "<" ? *op + 2 : *op + 1;
            std::size_t statementEnd = rhs;
            while (statementEnd < end && tokens[statementEnd].text != ";") ++statementEnd;
            appendVerilogArithmeticResultWidthRisk(
                source, tokens, rhs, statementEnd, targetShape->width,
                targetShape->display, signals, issues, moduleName);
            const auto sourceShape = verilogCaseSelectorShape(
                tokens, rhs, statementEnd, signals);
            if (sourceShape) {
                SignalState effectiveSource;
                effectiveSource.width = sourceShape->width;
                appendWidthTruncation(issues, moduleName, targetShape->display,
                                      effectiveTarget,
                                      sourceShape->display, effectiveSource,
                                      location(source, tokens[i]));
                continue;
            }
            const auto sourceToken = singleExpressionToken(tokens, rhs, statementEnd);
            if (sourceToken &&
                tokens[*sourceToken].kind == HdlTokenKind::Number) {
                const auto literalWidth = verilogLiteralDeclaredWidth(
                    tokens[*sourceToken].text);
                if (literalWidth) appendLiteralWidthTruncation(
                    issues, moduleName, targetShape->display, effectiveTarget,
                    *literalWidth, tokens[*sourceToken], source);
                continue;
            }
            if (const auto concatenation = verilogConcatenationShape(
                    tokens, rhs, statementEnd, signals)) {
                appendCompositeWidthTruncation(
                    issues, moduleName, targetShape->display, effectiveTarget,
                    concatenation->width, concatenation->display,
                    location(source, tokens[rhs]));
            }
        }
        CombinationalGraph combinationalGraph;
        for (std::size_t i = body; i + 1 < end; ++i) {
            if (!tokenIs(tokens[i], "assign") || !identifier(tokens[i + 1])) continue;
            const auto target = signals.find(tokens[i + 1].text);
            if (target == signals.end() || target->second.memory) continue;
            const auto op = assignmentOperator(tokens, i + 1, end);
            if (!op) continue;
            const auto targetNodes = verilogDependencyNodes(
                tokens, i + 1, *op, target->second);
            const auto rhs = tokens[*op].text == "<" ? *op + 2 : *op + 1;
            std::size_t statementEnd = rhs;
            while (statementEnd < end && tokens[statementEnd].text != ";") ++statementEnd;
            for (std::size_t sourceToken = rhs; sourceToken < statementEnd; ++sourceToken) {
                if (!identifier(tokens[sourceToken])) continue;
                const auto dependency = signals.find(tokens[sourceToken].text);
                if (dependency == signals.end() || dependency->second.memory) continue;
                const auto sourceNodes = verilogDependencyNodes(
                    tokens, sourceToken, statementEnd, dependency->second);
                for (const auto& targetNode : targetNodes)
                    for (const auto& sourceNode : sourceNodes)
                        addCombinationalDependency(combinationalGraph, targetNode, sourceNode,
                            location(source, tokens[i + 1]), tokens[i + 1].text,
                            tokens[sourceToken].text);
            }
            i = statementEnd < end ? statementEnd : i;
        }
        for (const auto& process : processes) {
            if (!verilogCombinationalProcess(tokens, process, end)) continue;
            std::set<std::size_t> loopHeaderTokens;
            std::set<std::string> assignedPreviously;
            for (std::size_t i = process.begin; i + 1 < process.end; ++i) {
                if (!tokenIs(tokens[i], "for") || tokens[i + 1].text != "(") continue;
                const auto after = skipBalanced(tokens, i + 1, process.end, "(", ")");
                if (!after) continue;
                for (std::size_t token = i; token < *after; ++token) loopHeaderTokens.insert(token);
                i = *after - 1;
            }
            for (std::size_t i = process.begin; i < process.end; ++i) {
                if (!identifier(tokens[i]) || loopHeaderTokens.contains(i) ||
                    (i > process.begin && tokens[i - 1].text == ".")) continue;
                const auto target = signals.find(tokens[i].text);
                if (target == signals.end() || target->second.memory) continue;
                const auto op = verilogAssignmentOperator(
                    tokens, i, process.begin, process.end);
                if (!op) continue;
                const auto targetNodes = verilogDependencyNodes(
                    tokens, i, *op, target->second);
                const auto rhs = tokens[*op].text == "<" ? *op + 2 : *op + 1;
                std::size_t statementEnd = rhs;
                while (statementEnd < process.end && tokens[statementEnd].text != ";") {
                    ++statementEnd;
                }
                for (std::size_t sourceToken = rhs; sourceToken < statementEnd; ++sourceToken) {
                    if (!identifier(tokens[sourceToken])) continue;
                    const auto dependency = signals.find(tokens[sourceToken].text);
                    if (dependency == signals.end() || dependency->second.memory) continue;
                    if (dependency->first == target->first && tokens[*op].text == "=" &&
                        assignedPreviously.contains(target->first)) continue;
                    const auto sourceNodes = verilogDependencyNodes(
                        tokens, sourceToken, statementEnd, dependency->second);
                    for (const auto& targetNode : targetNodes)
                        for (const auto& sourceNode : sourceNodes)
                            addCombinationalDependency(combinationalGraph, targetNode, sourceNode,
                                location(source, tokens[i]), tokens[i].text,
                                tokens[sourceToken].text);
                }
                assignedPreviously.insert(target->first);
                i = statementEnd < process.end ? statementEnd : i;
            }
        }
        appendCombinationalLoopIssues(combinationalGraph, issues, moduleName);
        for (const auto& process : processes) {
            bool combinational = tokenIs(tokens[process.keyword], "always_comb");
            if (tokenIs(tokens[process.keyword], "always")) {
                std::size_t sensitivity = process.keyword + 1;
                if (sensitivity < end && tokens[sensitivity].text == "@") ++sensitivity;
                bool edgeTriggered = false;
                if (sensitivity < end && tokens[sensitivity].text == "*") {
                    combinational = true;
                } else if (sensitivity < end && tokens[sensitivity].text == "(") {
                    const auto after = skipBalanced(tokens, sensitivity, end, "(", ")");
                    if (after) {
                        for (std::size_t i = sensitivity + 1; i + 1 < *after; ++i) {
                            if (tokenIs(tokens[i], "posedge") || tokenIs(tokens[i], "negedge")) {
                                edgeTriggered = true;
                            }
                        }
                        combinational = !edgeTriggered;
                    }
                }
            }
            if (!combinational) continue;
            const auto flow = parseVerilogFlow(tokens, process.begin, process.end, knownSignals);
            for (const auto& name : flow.maybe) {
                const auto signal = signals.find(name);
                if (flow.definite.contains(name) || signal == signals.end() || signal->second.memory ||
                    signal->second.direction == Direction::Input ||
                    signal->second.direction == Direction::Inout) continue;
                issues.push_back({"RTL-LATCH-INFERRED",
                    "Combinational process may retain the previous value of: " + name,
                    "The signal is assigned on some, but not all, control-flow paths in a combinational process.",
                    "Assign a default before the conditional logic or cover every if/case branch.",
                    location(source, tokens[process.keyword]), moduleName, name});
            }
        }
        if (source.language() == core::HdlLanguage::SystemVerilog) {
            for (const auto& process : processes) {
                if (!tokenIs(tokens[process.keyword], "always_latch")) continue;
                const auto flow = parseVerilogFlow(
                    tokens, process.begin, process.end, knownSignals);
                const bool retainsValue = std::ranges::any_of(
                    flow.maybe, [&](const std::string& name) {
                        const auto signal = signals.find(name);
                        return !flow.definite.contains(name) && signal != signals.end() &&
                            !signal->second.memory &&
                            signal->second.direction != Direction::Input;
                    });
                if (retainsValue) continue;
                issues.push_back({"SV-ALWAYS-LATCH-NO-LATCH",
                    "always_latch does not infer level-sensitive storage",
                    "Every recognized assignment in this always_latch is complete on all control-flow paths, or the process assigns no declared signal.",
                    "Use always_comb for complete combinational logic or leave an intentional assignment path uncovered to model a latch.",
                    location(source, tokens[process.keyword]), moduleName, std::nullopt});
            }
        }

        std::set<std::string> explicitSensitivityNames;
        const auto simpleSensitivityName = [&](std::size_t token, std::size_t open,
                                               std::size_t close) {
            if (token <= open || token + 1 >= close || !identifier(tokens[token])) return false;
            const auto& before = tokens[token - 1];
            const auto& afterToken = tokens[token + 1];
            return (before.text == "(" || before.text == "," || tokenIs(before, "or")) &&
                (afterToken.text == ")" || afterToken.text == "," || tokenIs(afterToken, "or"));
        };
        if (source.language() == core::HdlLanguage::Verilog) {
            for (const auto& process : processes) {
                if (!tokenIs(tokens[process.keyword], "always")) continue;
                const auto at = process.keyword + 1;
                if (at + 1 >= end || tokens[at].text != "@" || tokens[at + 1].text != "(")
                    continue;
                const auto close = skipBalanced(tokens, at + 1, end, "(", ")");
                if (!close) continue;
                for (std::size_t i = at + 2; i + 1 < *close; ++i)
                    if (simpleSensitivityName(i, at + 1, *close))
                        explicitSensitivityNames.insert(tokens[i].text);
            }
        }
        for (const auto& process : processes) {
            if (!tokenIs(tokens[process.keyword], "always")) continue;
            std::size_t sensitivity = process.keyword + 1;
            if (sensitivity >= end || tokens[sensitivity].text != "@") continue;
            ++sensitivity;
            if (sensitivity >= end || tokens[sensitivity].text != "(") continue;
            const auto after = skipBalanced(tokens, sensitivity, end, "(", ")");
            if (!after) continue;

            bool automaticOrClocked = false;
            std::map<std::string, std::size_t> listed;
            for (std::size_t i = sensitivity + 1; i + 1 < *after; ++i) {
                if (tokens[i].text == "*" || tokenIs(tokens[i], "posedge") ||
                    tokenIs(tokens[i], "negedge")) automaticOrClocked = true;
                const auto found = signals.find(tokens[i].text);
                if (found != signals.end()) listed.try_emplace(found->first, i);
                else if (source.language() == core::HdlLanguage::Verilog &&
                         simpleSensitivityName(i, sensitivity, *after))
                    listed.try_emplace(tokens[i].text, i);
            }
            if (automaticOrClocked) continue;

            std::map<std::string, std::size_t> reads;
            for (std::size_t i = process.begin; i < process.end && i < end; ++i) {
                if (!identifier(tokens[i]) || (i > process.begin && tokens[i - 1].text == ".") ||
                    verilogAssignmentOperator(tokens, i, process.begin, process.end)) continue;
                const auto found = signals.find(tokens[i].text);
                if (found != signals.end()) reads.try_emplace(found->first, i);
                else if (source.language() == core::HdlLanguage::Verilog &&
                         explicitSensitivityNames.contains(tokens[i].text))
                    reads.try_emplace(tokens[i].text, i);
            }
            for (const auto& [name, token] : reads) {
                if (listed.contains(name)) continue;
                issues.push_back({"RTL-INCOMPLETE-SENSITIVITY",
                    "组合always块的敏感列表不完整，缺少信号" + name,
                    "组合 always 块读取了显式敏感列表中缺少的信号。",
                    "将该信号加入敏感列表，或改用 always @(*) / always_comb。",
                    location(source, tokens[token]), moduleName, name});
            }
            for (const auto& [name, token] : listed) {
                if (reads.contains(name)) continue;
                issues.push_back({"RTL-REDUNDANT-SENSITIVITY",
                    "组合always敏感列表中信号" + name + "是冗余的",
                    "组合 always 块的显式敏感列表包含正文未读取的信号。",
                    "删除该冗余信号，或改用 always @(*) / always_comb。",
                    location(source, tokens[token]), moduleName, name});
            }
        }

        // Native 5.2.18 treats identifiers below a resolved output formal as
        // driven, not read.  Unresolved or ambiguous cell types remain
        // conservative and count every actual as connected.
        std::set<std::size_t> instanceActualOffsets;
        int instanceDriverId = 70000;
        for (std::size_t i = body; i + 2 < end; ++i) {
            if (inProcess(i, processes) || !identifier(tokens[i]) || signals.contains(tokens[i].text)) continue;
            const auto target = moduleInterfaces.find(tokens[i].text);
            const VerilogModuleInterface* interface =
                target != moduleInterfaces.end() && target->second.size() == 1 ?
                    &target->second.front() : nullptr;
            std::size_t instance = i + 1;
            if (tokens[instance].text == "#") {
                const auto after = skipBalanced(tokens, instance + 1, end, "(", ")");
                if (!after) continue;
                instance = *after;
            }
            if (instance + 1 >= end || !identifier(tokens[instance]) ||
                tokens[instance + 1].text != "(") continue;
            const auto after = skipBalanced(tokens, instance + 1, end, "(", ")");
            if (!after) continue;
            const auto reportSimpleConnection = [&](std::size_t begin, std::size_t finish) {
                if (finish == begin + 1 && identifier(tokens[begin])) {
                    reportImplicitNet(tokens[begin]);
                } else if (finish == begin + 5 && tokens[begin].text == "." &&
                           identifier(tokens[begin + 1]) && tokens[begin + 2].text == "(" &&
                           identifier(tokens[begin + 3]) && tokens[begin + 4].text == ")") {
                    reportImplicitNet(tokens[begin + 3]);
                }
            };
            const auto classifyActual = [&](std::size_t begin, std::size_t finish,
                                            std::size_t position) {
                if (begin >= finish) return;
                std::optional<std::string> formal;
                std::size_t actualBegin = begin;
                std::size_t actualEnd = finish;
                if (begin + 1 < finish && tokens[begin].text == "." &&
                    identifier(tokens[begin + 1])) {
                    formal = tokens[begin + 1].text;
                    if (begin + 3 < finish && tokens[begin + 2].text == "(" &&
                        tokens[finish - 1].text == ")") {
                        actualBegin = begin + 3;
                        actualEnd = finish - 1;
                    } else {
                        // SystemVerilog shorthand .name connects the same name.
                        actualBegin = begin + 1;
                        actualEnd = begin + 2;
                    }
                }

                std::optional<Direction> direction;
                if (interface) {
                    if (formal) {
                        const auto found = interface->directions.find(*formal);
                        if (found != interface->directions.end()) direction = found->second;
                    } else if (position < interface->portOrder.size()) {
                        const auto found = interface->directions.find(
                            interface->portOrder[position]);
                        if (found != interface->directions.end()) direction = found->second;
                    }
                }
                for (std::size_t actual = actualBegin; actual < actualEnd; ++actual) {
                    if (!identifier(tokens[actual]) ||
                        (actual > actualBegin && tokens[actual - 1].text == ".")) continue;
                    const auto signal = signals.find(tokens[actual].text);
                    if (signal == signals.end()) continue;
                    instanceActualOffsets.insert(tokens[actual].offset);
                    if (direction == Direction::Output) {
                        signal->second.drivers.push_back({instanceDriverId++,
                            location(source, tokens[actual]), false});
                    } else {
                        signal->second.connected = true;
                    }
                }
            };

            std::size_t segment = instance + 2;
            std::size_t position = 0;
            int connectionDepth = 0;
            for (std::size_t token = segment; token < *after; ++token) {
                const bool closingOuter = token + 1 == *after;
                if ((tokens[token].text == "," && connectionDepth == 0) || closingOuter) {
                    reportSimpleConnection(segment, token);
                    classifyActual(segment, token, position++);
                    segment = token + 1;
                    continue;
                }
                if (tokens[token].text == "(" || tokens[token].text == "[" ||
                    tokens[token].text == "{") ++connectionDepth;
                else if (tokens[token].text == ")" || tokens[token].text == "]" ||
                         tokens[token].text == "}") --connectionDepth;
            }
            i = *after - 1;
        }

        for (std::size_t i = body; i < end; ++i) {
            if (!identifier(tokens[i]) || assignmentTargets.contains(tokens[i].offset) ||
                instanceActualOffsets.contains(tokens[i].offset) ||
                (i > 0 && tokens[i - 1].text == ".")) continue;
            const auto found = signals.find(tokens[i].text);
            if (found != signals.end() && tokens[i].offset != found->second.declaration.byteOffset) {
                found->second.read = true;
                if (!found->second.firstRead)
                    found->second.firstRead = location(source, tokens[i]);
            }
        }

        for (const auto& [name, signal] : signals) {
            const auto runtimeDrivers = std::ranges::count_if(signal.drivers, [](const Driver& driver) {
                return !driver.initialization;
            });
            if (runtimeDrivers > 1 && !signal.memory) {
                const Driver* conflictingDriver = nullptr;
                int runtimeIndex = 0;
                for (const auto& driver : signal.drivers) {
                    if (!driver.initialization && ++runtimeIndex == 2) {
                        conflictingDriver = &driver;
                        break;
                    }
                }
                issues.push_back({"RTL-MULTIPLE-DRIVERS", "Signal has multiple independent drivers: " + name,
                    "The signal is assigned by more than one always block or continuous assignment.",
                    "Use a single owning process/assignment or add explicit arbitration.",
                    conflictingDriver->location, moduleName, name});
            }
            if (signal.direction == Direction::Input && !signal.drivers.empty()) {
                issues.push_back({"RTL-INPUT-DRIVEN", "Input port is assigned internally: " + name,
                    "An input port should be driven by the parent design, not by this module.",
                    "Remove the internal assignment or change the port direction.",
                    signal.drivers.front().location, moduleName, name});
            }
            if (signal.direction == Direction::Output && signal.drivers.empty() && !signal.connected) {
                issues.push_back({"RTL-UNDRIVEN-OUTPUT", "Output port has no local driver: " + name,
                    "No procedural or continuous assignment to this output was found.",
                    "Drive the output explicitly or remove it from the interface.",
                    signal.declaration, moduleName, name});
            }
            if (signal.direction == Direction::Input && signal.drivers.empty() &&
                !signal.read && !signal.connected) {
                issues.push_back({"RTL-UNUSED-INPUT-PORT", "Input port is never used: " + name,
                    "The input is not read by local logic or forwarded through an instance connection.",
                    "Remove the unused input, connect it to the intended logic, or suppress the rule for an intentionally reserved interface pin.",
                    signal.declaration, moduleName, name});
            }
            if (signal.direction == Direction::Internal && signal.drivers.empty() &&
                signal.read && !signal.connected) {
                issues.push_back({"RTL-UNDRIVEN-SIGNAL", "Signal is read but has no driver: " + name,
                    "The internal signal is consumed but no procedural or continuous driver was found.",
                    "Add the missing assignment or connect the intended source.",
                    signal.firstRead.value_or(signal.declaration), moduleName, name});
            }
            if (signal.direction == Direction::Internal && !signal.read && !signal.connected) {
                const bool driven = !signal.drivers.empty();
                issues.push_back({driven ? "RTL-WRITE-ONLY-SIGNAL" : "RTL-UNUSED-SIGNAL",
                    driven ? "Driven signal has no load: " + name :
                             "Declared signal is never read: " + name,
                    driven ?
                        "The internal signal has a recognized driver but is never read or connected as an instance actual." :
                        "The internal signal does not contribute to an expression or instance connection.",
                    driven ?
                        "Remove the unused driver or connect the signal to its intended load." :
                        "Remove the signal or connect it to the intended logic.",
                    signal.declaration, moduleName, name});
            }
        }
        module = end > 0 ? end - 1 : end;
    }
    return issues;
}

std::map<std::string, SignalState> vhdlPorts(const core::SourceFile& source,
                                             const std::vector<HdlToken>& tokens,
                                             std::size_t entity, std::size_t limit,
                                             bool stdLogicUnsignedImported) {
    std::map<std::string, SignalState> ports;
    std::size_t port = entity;
    while (port < limit && !tokenIs(tokens[port], "port")) ++port;
    if (port + 1 >= limit || tokens[port + 1].text != "(") return ports;
    const auto after = skipBalanced(tokens, port + 1, limit, "(", ")");
    if (!after) return ports;
    const auto end = *after - 1;
    std::size_t segment = port + 2;
    while (segment < end) {
        std::vector<const HdlToken*> names;
        std::size_t colon = segment;
        for (; colon < end && tokens[colon].text != ":" && tokens[colon].text != ";"; ++colon) {
            if (identifier(tokens[colon])) names.push_back(&tokens[colon]);
        }
        if (colon >= end || tokens[colon].text != ":" || colon + 1 >= end) {
            while (segment < end && tokens[segment].text != ";") ++segment;
            ++segment;
            continue;
        }
        Direction direction = Direction::Internal;
        if (tokenIs(tokens[colon + 1], "in")) direction = Direction::Input;
        else if (tokenIs(tokens[colon + 1], "out") || tokenIs(tokens[colon + 1], "buffer")) {
            direction = Direction::Output;
        } else if (tokenIs(tokens[colon + 1], "inout")) direction = Direction::Inout;
        std::size_t typeEnd = colon + 1;
        while (typeEnd < end && tokens[typeEnd].text != ";") ++typeEnd;
        const auto width = vhdlTypeWidth(tokens, colon + 2, typeEnd);
        const auto range = vhdlTypeRange(tokens, colon + 2, typeEnd);
        const bool unsignedNumeric = colon + 2 < typeEnd &&
            (tokenIs(tokens[colon + 2], "unsigned") ||
             (stdLogicUnsignedImported &&
              tokenIs(tokens[colon + 2], "std_logic_vector")));
        for (const auto* name : names) {
            ports[lower(name->text)] = {direction, location(source, *name), {}, false, false,
                                        false, width, range, unsignedNumeric,
                                        VerilogSignedness::Unknown, std::nullopt, {}};
        }
        segment = typeEnd;
        if (segment < end) ++segment;
    }
    return ports;
}

std::vector<ProcessRange> vhdlProcesses(const std::vector<HdlToken>& tokens,
                                        std::size_t body, std::size_t end) {
    std::vector<ProcessRange> ranges;
    int nextId = 1;
    for (std::size_t i = body; i < end; ++i) {
        if (!tokenIs(tokens[i], "process")) continue;
        std::size_t finish = i + 1;
        while (finish + 1 < end &&
               !(tokenIs(tokens[finish], "end") && tokenIs(tokens[finish + 1], "process"))) {
            ++finish;
        }
        if (finish + 1 < end) finish += 2;
        ranges.push_back({i, finish, nextId++, false, i});
        i = finish > 0 ? finish - 1 : i;
    }
    return ranges;
}

bool vhdlFlowStop(const std::vector<HdlToken>& tokens, std::size_t position,
                  std::size_t limit) {
    if (position >= limit) return true;
    if (tokenIs(tokens[position], "elsif") || tokenIs(tokens[position], "else") ||
        tokenIs(tokens[position], "when")) return true;
    return tokenIs(tokens[position], "end") && position + 1 < limit &&
           (tokenIs(tokens[position + 1], "if") || tokenIs(tokens[position + 1], "case") ||
            tokenIs(tokens[position + 1], "loop") || tokenIs(tokens[position + 1], "process"));
}

AssignmentFlow parseVhdlFlow(const std::vector<HdlToken>& tokens, std::size_t position,
                             std::size_t limit, const std::set<std::string>& known);

AssignmentFlow parseVhdlSequence(const std::vector<HdlToken>& tokens, std::size_t position,
                                 std::size_t limit, const std::set<std::string>& known) {
    AssignmentFlow result{{}, {}, position};
    while (result.next < limit && !vhdlFlowStop(tokens, result.next, limit)) {
        const auto child = parseVhdlFlow(tokens, result.next, limit, known);
        unionInto(result.maybe, child.maybe);
        unionInto(result.definite, child.definite);
        if (child.next <= result.next) ++result.next;
        else result.next = child.next;
    }
    return result;
}

AssignmentFlow parseVhdlIf(const std::vector<HdlToken>& tokens, std::size_t position,
                           std::size_t limit, const std::set<std::string>& known) {
    std::size_t thenToken = position + 1;
    while (thenToken < limit && !tokenIs(tokens[thenToken], "then")) ++thenToken;
    if (thenToken >= limit) return {{}, {}, position + 1};

    std::vector<AssignmentFlow> branches;
    auto branch = parseVhdlSequence(tokens, thenToken + 1, limit, known);
    branches.push_back(branch);
    std::size_t cursor = branch.next;
    while (cursor < limit && tokenIs(tokens[cursor], "elsif")) {
        thenToken = cursor + 1;
        while (thenToken < limit && !tokenIs(tokens[thenToken], "then")) ++thenToken;
        if (thenToken >= limit) break;
        branch = parseVhdlSequence(tokens, thenToken + 1, limit, known);
        branches.push_back(branch);
        cursor = branch.next;
    }
    bool hasElse = false;
    if (cursor < limit && tokenIs(tokens[cursor], "else")) {
        hasElse = true;
        branch = parseVhdlSequence(tokens, cursor + 1, limit, known);
        branches.push_back(branch);
        cursor = branch.next;
    }
    if (cursor + 1 < limit && tokenIs(tokens[cursor], "end") &&
        tokenIs(tokens[cursor + 1], "if")) cursor += 2;
    if (cursor < limit && tokens[cursor].text == ";") ++cursor;

    AssignmentFlow result{{}, {}, cursor};
    for (const auto& item : branches) unionInto(result.maybe, item.maybe);
    if (hasElse && !branches.empty()) {
        result.definite = branches.front().definite;
        for (std::size_t i = 1; i < branches.size(); ++i) {
            result.definite = intersectionOf(result.definite, branches[i].definite);
        }
    }
    return result;
}

AssignmentFlow parseVhdlCase(const std::vector<HdlToken>& tokens, std::size_t position,
                             std::size_t limit, const std::set<std::string>& known) {
    std::size_t cursor = position + 1;
    while (cursor < limit && !tokenIs(tokens[cursor], "is")) ++cursor;
    if (cursor < limit) ++cursor;
    std::vector<AssignmentFlow> branches;
    bool hasOthers = false;
    while (cursor < limit && !(tokenIs(tokens[cursor], "end") && cursor + 1 < limit &&
                               tokenIs(tokens[cursor + 1], "case"))) {
        if (!tokenIs(tokens[cursor], "when")) { ++cursor; continue; }
        const auto labelBegin = ++cursor;
        while (cursor + 1 < limit &&
               !(tokens[cursor].text == "=" && tokens[cursor + 1].text == ">")) ++cursor;
        for (std::size_t i = labelBegin; i < cursor; ++i) {
            if (tokenIs(tokens[i], "others")) hasOthers = true;
        }
        if (cursor + 1 >= limit) break;
        auto branch = parseVhdlSequence(tokens, cursor + 2, limit, known);
        branches.push_back(branch);
        cursor = branch.next;
    }
    if (cursor + 1 < limit && tokenIs(tokens[cursor], "end") &&
        tokenIs(tokens[cursor + 1], "case")) cursor += 2;
    if (cursor < limit && tokens[cursor].text == ";") ++cursor;

    AssignmentFlow result{{}, {}, cursor};
    for (const auto& branch : branches) unionInto(result.maybe, branch.maybe);
    if (hasOthers && !branches.empty()) {
        result.definite = branches.front().definite;
        for (std::size_t i = 1; i < branches.size(); ++i) {
            result.definite = intersectionOf(result.definite, branches[i].definite);
        }
    }
    return result;
}

AssignmentFlow parseVhdlFlow(const std::vector<HdlToken>& tokens, std::size_t position,
                             std::size_t limit, const std::set<std::string>& known) {
    while (position < limit && tokens[position].text == ";") ++position;
    if (position >= limit) return {{}, {}, position};
    if (tokenIs(tokens[position], "if")) return parseVhdlIf(tokens, position, limit, known);
    if (tokenIs(tokens[position], "case")) return parseVhdlCase(tokens, position, limit, known);
    if (tokenIs(tokens[position], "for") || tokenIs(tokens[position], "while")) {
        std::size_t loop = position + 1;
        while (loop < limit && !tokenIs(tokens[loop], "loop")) ++loop;
        auto body = parseVhdlSequence(tokens, loop < limit ? loop + 1 : loop, limit, known);
        body.definite.clear();
        if (body.next + 1 < limit && tokenIs(tokens[body.next], "end") &&
            tokenIs(tokens[body.next + 1], "loop")) body.next += 2;
        if (body.next < limit && tokens[body.next].text == ";") ++body.next;
        return body;
    }

    AssignmentFlow result{{}, {}, position};
    while (result.next < limit && tokens[result.next].text != ";" &&
           !vhdlFlowStop(tokens, result.next, limit)) {
        if (identifier(tokens[result.next]) && known.contains(lower(tokens[result.next].text)) &&
            result.next + 2 < limit && tokens[result.next + 1].text == "<" &&
            tokens[result.next + 2].text == "=") {
            const auto name = lower(tokens[result.next].text);
            result.maybe.insert(name);
            result.definite.insert(name);
        }
        ++result.next;
    }
    if (result.next < limit && tokens[result.next].text == ";") ++result.next;
    return result;
}

void appendVhdlUnreachableComparisons(
    const core::SourceFile& source, const std::vector<HdlToken>& tokens,
    std::size_t begin, std::size_t end, const std::map<std::string, SignalState>& signals,
    std::vector<SemanticIssue>& issues, const std::string& unitName) {
    // Native 0x1805d1d00 admits only a bare left identifier. 0x1805d0e00
    // reports EQ/GE when C > 2^W-1 and GT when C >= 2^W-1; LT/LE records
    // do not reach a report branch. Do not normalize reversed operands.
    // Defer architectures with local name/type scopes not represented here.
    for (auto at = begin; at < end; ++at)
        if (tokenIs(tokens[at], "variable") || tokenIs(tokens[at], "function") ||
            tokenIs(tokens[at], "procedure") || tokenIs(tokens[at], "alias")) return;
    for (auto at = begin; at + 2 < end; ++at) {
        if (!identifier(tokens[at])) continue;
        if (at > begin) {
            const auto& before = tokens[at - 1].text;
            if (before == "+" || before == "-" || before == "*" || before == "/" ||
                before == "." || before == "&" || tokenIs(tokens[at - 1], "not")) continue;
        }
        const auto signal = signals.find(lower(tokens[at].text));
        if (signal == signals.end() || !signal->second.vhdlUnsignedNumeric ||
            !signal->second.width || *signal->second.width == 0 ||
            *signal->second.width > 31) continue;
        auto rhs = at + 2;
        const bool equal = tokens[at + 1].text == "=";
        const bool greater = tokens[at + 1].text == ">";
        if (!equal && !greater) continue;
        const bool greaterEqual = greater && tokens[rhs].text == "=";
        if (greaterEqual) ++rhs;
        if (rhs >= end || tokens[rhs].kind != HdlTokenKind::Number) continue;
        const auto value = decimalValue(tokens[rhs].text);
        if (!value || *value > std::numeric_limits<std::uint32_t>::max()) continue;
        if (rhs + 1 < end) {
            const auto& after = tokens[rhs + 1].text;
            if (after != ")" && after != ";" && after != "," &&
                !tokenIs(tokens[rhs + 1], "then") && !tokenIs(tokens[rhs + 1], "loop") &&
                !tokenIs(tokens[rhs + 1], "else") && !tokenIs(tokens[rhs + 1], "and") &&
                !tokenIs(tokens[rhs + 1], "or") && !tokenIs(tokens[rhs + 1], "xor")) continue;
        }
        const auto maximum = (std::size_t{1} << *signal->second.width) - 1;
        if ((greater && !greaterEqual) ? *value < maximum : *value <= maximum) continue;
        issues.push_back({"RTL-UNREACHABLE-RANGE-COMPARISON",
            "Unsigned comparison cannot be true: " + tokens[at].text,
            "The direct comparison constant is outside the signal's bounded unsigned range.",
            "Correct the comparison value or the declared signal width.",
            location(source, tokens[at + 1]), unitName, tokens[at].text});
    }
}

std::vector<SemanticIssue> analyzeVhdl(const core::SourceFile& source,
                                       bool unreachableComparisonsOnly = false) {
    const auto tokens = tokenize(source);
    std::vector<SemanticIssue> issues;
    std::set<std::size_t> reportedSharedVariables;
    bool stdLogicUnsignedImported = false;
    for (std::size_t token = 0; token < tokens.size(); ++token) {
        if (!tokenIs(tokens[token], "use")) continue;
        std::size_t clauseEnd = token + 1;
        while (clauseEnd < tokens.size() && tokens[clauseEnd].text != ";") ++clauseEnd;
        for (std::size_t name = token + 1; name < clauseEnd; ++name) {
            if (tokenIs(tokens[name], "std_logic_unsigned"))
                stdLogicUnsignedImported = true;
            if (!tokenIs(tokens[name], "std_logic_arith") &&
                !tokenIs(tokens[name], "std_logic_unsigned") &&
                !tokenIs(tokens[name], "std_logic_signed")) continue;
            issues.push_back({"VHDL-NONSTD-NUMERIC",
                "Non-standard VHDL arithmetic package used: " + tokens[name].text,
                "Vendor arithmetic packages are non-standard and can produce ambiguous overloads.",
                "Use ieee.numeric_std and explicit signed/unsigned conversions.",
                location(source, tokens[name]), std::nullopt, std::nullopt});
        }
        token = clauseEnd;
    }
    std::map<std::string, std::map<std::string, SignalState>> entityPorts;
    for (std::size_t i = 0; i + 2 < tokens.size(); ++i) {
        if (!tokenIs(tokens[i], "entity") || !identifier(tokens[i + 1]) ||
            !tokenIs(tokens[i + 2], "is")) continue;
        std::size_t end = i + 3;
        while (end < tokens.size() && !tokenIs(tokens[end], "architecture") &&
               !(tokenIs(tokens[end], "end") && end + 1 < tokens.size() &&
                 (tokenIs(tokens[end + 1], "entity") || tokenIs(tokens[end + 1], tokens[i + 1].text) ||
                  tokens[end + 1].text == ";"))) ++end;
        // T-Linter's native 4.12.c collector stops at the VHDL entity node and
        // copies only children of its direct port container.  It therefore does
        // not recurse into component declarations in the entity declarative
        // region.  Mirror that bounded scope instead of scanning to `end entity`.
        // See docs/evidence/native_vhdl_buffer_port_4_12_c_findings.md.
        std::size_t entityPortOpen = end;
        for (std::size_t token = i + 3; token + 1 < end; ++token) {
            if (tokenIs(tokens[token], "port") && tokens[token + 1].text == "(") {
                entityPortOpen = token + 1;
                break;
            }
        }
        if (entityPortOpen < end) {
            std::size_t depth = 1;
            std::size_t entityPortClose = entityPortOpen + 1;
            for (; entityPortClose < end && depth != 0; ++entityPortClose) {
                if (tokens[entityPortClose].text == "(") ++depth;
                else if (tokens[entityPortClose].text == ")") --depth;
            }
            const auto portLimit = depth == 0 ? entityPortClose - 1 : end;
            for (std::size_t token = entityPortOpen + 1; token < portLimit; ++token) {
                if (!tokenIs(tokens[token], "buffer")) continue;
                issues.push_back({"VHDL-BUFFER-PORT", "Entity port uses buffer mode",
                    "GJB9765.4.12.c rejects buffer ports because reading an output through buffer mode reduces interface portability and complicates hierarchy reuse.",
                    "Use mode out with an internal signal, or use inout only for a genuinely bidirectional interface.",
                    location(source, tokens[token]), tokens[i + 1].text, std::nullopt});
            }
        }
        entityPorts[lower(tokens[i + 1].text)] = vhdlPorts(
            source, tokens, i + 3, end, stdLogicUnsignedImported);
    }
    appendVhdlPackageBodyCaseOthersIssues(source, tokens, issues);
    if (!unreachableComparisonsOnly) {
        appendVhdlPackageBodyWidthIssues(source, tokens, issues);
    }
    for (std::size_t architecture = 0; architecture + 4 < tokens.size(); ++architecture) {
        if (!tokenIs(tokens[architecture], "architecture") ||
            !tokenIs(tokens[architecture + 2], "of") ||
            !identifier(tokens[architecture + 3])) continue;
        const auto unitName = tokens[architecture + 3].text;
        const auto ports = entityPorts[lower(unitName)];
        auto signals = ports;
        std::size_t body = architecture + 4;
        while (body < tokens.size() && !tokenIs(tokens[body], "begin")) {
            if (tokenIs(tokens[body], "shared") && body + 1 < tokens.size() &&
                tokenIs(tokens[body + 1], "variable")) {
                reportedSharedVariables.insert(tokens[body].offset);
                issues.push_back({"VHDL-SHARED-VARIABLE",
                    "Shared variable requires careful synchronization",
                    "An unprotected shared variable can cause simulation races and poor synthesis portability.",
                    "Prefer signals or a protected type with a documented synchronization scheme.",
                    location(source, tokens[body]), unitName, std::nullopt});
            }
            if (tokenIs(tokens[body], "signal")) {
                std::size_t colon = body + 1;
                while (colon < tokens.size() && tokens[colon].text != ":" &&
                       tokens[colon].text != ";") ++colon;
                std::size_t declarationEnd = colon;
                while (declarationEnd < tokens.size() && tokens[declarationEnd].text != ";") {
                    ++declarationEnd;
                }
                const auto width = vhdlTypeWidth(tokens, colon + 1, declarationEnd);
                const auto range = vhdlTypeRange(tokens, colon + 1, declarationEnd);
                const bool unsignedNumeric = colon + 1 < declarationEnd &&
                    (tokenIs(tokens[colon + 1], "unsigned") ||
                     (stdLogicUnsignedImported &&
                      tokenIs(tokens[colon + 1], "std_logic_vector")));
                for (std::size_t name = body + 1; name < colon; ++name) {
                    if (identifier(tokens[name])) {
                        const auto key = lower(tokens[name].text);
                        const auto [signal, inserted] = signals.try_emplace(key,
                            SignalState{Direction::Internal, location(source, tokens[name]), {},
                                        false, false, false, width, range, unsignedNumeric,
                                        VerilogSignedness::Unknown, std::nullopt, {}});
                        static_cast<void>(signal);
                        if (!inserted) {
                            issues.push_back({"STRUCT-DUPLICATE-SIGNAL-DECLARATION",
                                "Signal is declared more than once: " + tokens[name].text,
                                "A VHDL signal declaration repeats an existing signal or port name in the same architecture scope.",
                                "Remove or rename the later declaration so every signal name is unique within the architecture.",
                                location(source, tokens[name]), unitName, tokens[name].text});
                        }
                    }
                }
            }
            ++body;
        }
        std::size_t end = body;
        while (end + 1 < tokens.size() &&
               !(tokenIs(tokens[end], "end") &&
                 (tokenIs(tokens[end + 1], "architecture") ||
                  tokenIs(tokens[end + 1], tokens[architecture + 1].text)))) ++end;
        if (unreachableComparisonsOnly) {
            const bool localScope = std::any_of(tokens.begin() + architecture,
                tokens.begin() + end, [](const auto& token) {
                    return tokenIs(token, "variable") || tokenIs(token, "function") ||
                        tokenIs(token, "procedure") || tokenIs(token, "alias");
                });
            if (!localScope)
                appendVhdlUnreachableComparisons(source, tokens, body, end, signals, issues, unitName);
            architecture = end;
            continue;
        }
        const auto processes = vhdlProcesses(tokens, body, end);
        const auto genericShapes = vhdlEntityGenericShapes(
            source, tokens, architecture, unitName);
        const auto widthShapes = vhdlRootShapes(
            source, tokens, architecture + 4, end,
            signals, ports, genericShapes);
        appendConstantSelectIssues(source, tokens, body, end, signals, true,
                                   issues, unitName);
        appendVhdlDuplicateCaseIssues(
            source, tokens, body, end, widthShapes, issues, unitName);
        appendVhdlShiftAmountIssues(
            source, tokens, body, end, signals, issues, unitName);
        appendVhdlNegativeShiftIssues(
            source, tokens, body, end, issues, unitName);
        const std::vector<ProcessRange> widthRoot{
            ProcessRange{architecture + 4, end, 1, false, architecture}};
        appendVhdlComparisonWidthIssues(
            source, tokens, widthRoot, end, widthShapes, issues, unitName);
        const auto staticIntegerValues = vhdlNativeStaticIntegerValues(
            tokens, architecture, end, unitName);
        appendVhdlBitwiseWidthIssues(
            source, tokens, architecture + 4, end, widthShapes,
            staticIntegerValues, issues, unitName);
        appendVhdlConditionalExpressionComparisonWidthIssues(
            source, tokens, architecture + 4, end,
            widthShapes, issues, unitName);
        appendVhdlComplexEdgeEventIssues(
            source, tokens, processes, end, issues, unitName);
        appendVhdlDuplicateConditionIssues(
            source, tokens, processes, end, issues, unitName);
        appendVhdlAsyncResetOrderIssues(
            source, tokens, processes, end, issues, unitName);
        appendVhdlEmptyProcessIssues(
            source, tokens, processes, end, issues, unitName);
        appendVhdlConstantConditionIssues(
            source, tokens, processes, body, end, signals, issues, unitName);
        appendVhdlDuplicateSensitivityIssues(
            source, tokens, processes, end, signals, issues, unitName);
        appendVhdlSelfAssignmentIssues(
            source, tokens, processes, end, signals, issues, unitName);
        std::set<std::size_t> sensitivityWaitOffsets;
        for (const auto& process : processes) {
            const bool hasSensitivity = process.keyword + 1 < process.end &&
                                        tokens[process.keyword + 1].text == "(";
            std::vector<std::size_t> waits;
            bool anyWait = false;
            int subprogramDepth = 0;
            for (std::size_t token = process.keyword + 1; token < process.end; ++token) {
                if (tokenIs(tokens[token], "procedure") || tokenIs(tokens[token], "function")) {
                    bool hasBody = false;
                    for (std::size_t look = token + 1; look < process.end; ++look) {
                        if (tokenIs(tokens[look], "is")) { hasBody = true; break; }
                        if (tokens[look].text == ";") break;
                    }
                    if (hasBody) ++subprogramDepth;
                    continue;
                }
                if (tokenIs(tokens[token], "end") && token + 1 < process.end &&
                    (tokenIs(tokens[token + 1], "procedure") ||
                     tokenIs(tokens[token + 1], "function"))) {
                    if (subprogramDepth > 0) --subprogramDepth;
                    ++token;
                    continue;
                }
                if (tokenIs(tokens[token], "wait")) {
                    anyWait = true;
                    if (subprogramDepth == 0) waits.push_back(token);
                }
            }
            if (hasSensitivity) {
                for (const auto wait : waits) {
                    sensitivityWaitOffsets.insert(tokens[wait].offset);
                    issues.push_back({"VHDL-WAIT-WITH-SENSITIVITY",
                        "Sensitivity-list process contains an explicit wait statement",
                        "VHDL does not allow an explicit wait in a process that already has a sensitivity list.",
                        "Remove the wait or rewrite the process without a sensitivity list.",
                        location(source, tokens[wait]), unitName, std::nullopt});
                }
                continue;
            }
            const bool hasWait = anyWait;
            if (hasWait) continue;
            issues.push_back({"VHDL-PROCESS-NO-SUSPENSION",
                "Process has neither a sensitivity list nor a wait statement",
                "The process repeats forever without suspending and can lock simulation in zero time.",
                "Add the intended sensitivity list or an explicit wait statement.",
                location(source, tokens[process.keyword]), unitName, std::nullopt});
        }
        for (std::size_t token = body; token < end; ++token) {
            if (!tokenIs(tokens[token], "wait") ||
                sensitivityWaitOffsets.contains(tokens[token].offset)) continue;
            issues.push_back({"VHDL-WAIT",
                "Wait statement may be non-synthesizable",
                "Wait statements are tool- and context-dependent in synthesizable RTL.",
                "Use a clocked or combinational process for portable RTL.",
                location(source, tokens[token]), unitName, std::nullopt});
        }
        appendVhdlDelayIssues(source, tokens, body, end, issues, unitName);
        appendVhdlDivideByZeroIssues(
            source, tokens, body, end, issues, unitName);
        int concurrentId = 100000;
        std::set<std::size_t> assignmentTargets;
        for (std::size_t i = body; i + 2 < end; ++i) {
            if (!identifier(tokens[i]) ||
                !vhdlAssignmentOperator(tokens, i, end)) continue;
            const auto found = signals.find(lower(tokens[i].text));
            if (found == signals.end()) continue;
            int driverId = concurrentId++;
            for (const auto& process : processes) {
                if (i >= process.begin && i < process.end) { driverId = process.driverId; break; }
            }
            addDriver(found->second, driverId, location(source, tokens[i]));
            assignmentTargets.insert(tokens[i].offset);
        }
        std::map<std::string, std::string> clockSignals;
        for (const auto& process : processes) {
            for (std::size_t token = process.begin; token + 2 < process.end && token + 2 < end;
                 ++token) {
                if ((!tokenIs(tokens[token], "rising_edge") &&
                     !tokenIs(tokens[token], "falling_edge")) ||
                    tokens[token + 1].text != "(" || !identifier(tokens[token + 2]) ||
                    resetLikeName(tokens[token + 2].text)) continue;
                clockSignals.try_emplace(lower(tokens[token + 2].text), tokens[token + 2].text);
            }
        }
        appendDerivedClockIssues(signals, clockSignals, issues, unitName);
        std::set<std::string> knownSignals;
        for (const auto& [name, signal] : signals) {
            static_cast<void>(signal);
            knownSignals.insert(name);
        }
        CombinationalGraph combinationalGraph;
        const auto clockedProcess = [&](const ProcessRange& process) {
            for (std::size_t token = process.begin; token < process.end && token < end; ++token) {
                if (tokenIs(tokens[token], "rising_edge") ||
                    tokenIs(tokens[token], "falling_edge") || tokenIs(tokens[token], "event")) {
                    return true;
                }
            }
            return false;
        };
        for (std::size_t i = body; i + 2 < end; ++i) {
            if (!identifier(tokens[i])) continue;
            const auto target = signals.find(lower(tokens[i].text));
            if (target == signals.end()) continue;
            const auto op = vhdlAssignmentOperator(tokens, i, end);
            if (!op) continue;
            const auto targetShape = vhdlCaseSelectorShape(
                tokens, i, *op, widthShapes);
            if (!targetShape) continue;
            const auto rhs = *op + 2;
            std::size_t statementEnd = rhs;
            while (statementEnd < end && tokens[statementEnd].text != ";") ++statementEnd;
            const auto sourceShape = vhdlCaseSelectorShape(
                tokens, rhs, statementEnd, widthShapes);
            if (sourceShape) {
                appendVhdlAssignmentWidthMismatch(
                    issues, unitName, targetShape->display, targetShape->width,
                    sourceShape->display, sourceShape->width,
                    location(source, tokens[i]));
                continue;
            }
            if (const auto conversion = vhdlNativeConversionShape(
                    tokens, rhs, statementEnd, widthShapes,
                    &staticIntegerValues)) {
                appendVhdlAssignmentWidthMismatch(
                    issues, unitName, targetShape->display, targetShape->width,
                    conversion->display, conversion->width,
                    location(source, tokens[rhs]));
                continue;
            }
            const auto sourceToken = singleExpressionToken(tokens, rhs, statementEnd);
            if (sourceToken) {
                const auto literalWidth = vhdlStringLiteralWidth(tokens[*sourceToken]);
                if (!literalWidth) continue;
                appendVhdlAssignmentWidthMismatch(
                    issues, unitName, targetShape->display, targetShape->width,
                    tokens[*sourceToken].text, *literalWidth,
                    location(source, tokens[*sourceToken]));
                continue;
            }
            if (const auto concatenation = vhdlConcatenationShape(
                    tokens, rhs, statementEnd, widthShapes)) {
                appendVhdlAssignmentWidthMismatch(
                    issues, unitName, targetShape->display, targetShape->width,
                    concatenation->display, concatenation->width,
                    location(source, tokens[rhs]));
            }
        }
        for (std::size_t i = body; i + 2 < end; ++i) {
            if (!identifier(tokens[i])) continue;
            const auto op = vhdlAssignmentOperator(tokens, i, end);
            if (!op) continue;
            const auto target = signals.find(lower(tokens[i].text));
            if (target == signals.end()) continue;
            const auto owner = std::ranges::find_if(processes, [&](const ProcessRange& process) {
                return i >= process.begin && i < process.end;
            });
            if (owner != processes.end() && clockedProcess(*owner)) continue;
            const auto rhs = *op + 2;
            std::size_t statementEnd = rhs;
            while (statementEnd < end && tokens[statementEnd].text != ";") ++statementEnd;
            for (std::size_t sourceToken = rhs; sourceToken < statementEnd; ++sourceToken) {
                if (!identifier(tokens[sourceToken])) continue;
                const auto dependency = signals.find(lower(tokens[sourceToken].text));
                if (dependency == signals.end()) continue;
                addCombinationalDependency(combinationalGraph, target->first, dependency->first,
                    location(source, tokens[i]), tokens[i].text, tokens[sourceToken].text);
            }
            i = statementEnd < end ? statementEnd : i;
        }
        appendCombinationalLoopIssues(combinationalGraph, issues, unitName);
        for (const auto& process : processes) {
            std::size_t sensitivity = process.keyword + 1;
            if (sensitivity >= end || tokens[sensitivity].text != "(") continue;
            const auto after = skipBalanced(tokens, sensitivity, end, "(", ")");
            if (!after) continue;
            bool clocked = false;
            for (std::size_t i = *after; i < process.end && i < end; ++i) {
                if (tokenIs(tokens[i], "rising_edge") || tokenIs(tokens[i], "falling_edge") ||
                    tokenIs(tokens[i], "event")) clocked = true;
            }
            if (clocked) continue;
            std::size_t processBody = *after;
            while (processBody < process.end && !tokenIs(tokens[processBody], "begin")) ++processBody;
            if (processBody >= process.end) continue;
            const auto flow = parseVhdlSequence(tokens, processBody + 1, process.end, knownSignals);
            for (const auto& name : flow.maybe) {
                const auto signal = signals.find(name);
                if (flow.definite.contains(name) || signal == signals.end() ||
                    signal->second.direction == Direction::Input ||
                    signal->second.direction == Direction::Inout) continue;
                issues.push_back({"RTL-LATCH-INFERRED",
                    "Combinational process may retain the previous value of: " + name,
                    "The signal is assigned on some, but not all, control-flow paths in a combinational process.",
                    "Assign a default before the conditional logic or cover every if/case branch.",
                    location(source, tokens[process.keyword]), unitName, name});
            }
        }
        for (const auto& process : processes) {
            std::size_t sensitivity = process.keyword + 1;
            if (sensitivity >= end || tokens[sensitivity].text != "(") continue;
            const auto after = skipBalanced(tokens, sensitivity, end, "(", ")");
            if (!after) continue;

            bool allOrClocked = false;
            std::map<std::string, std::size_t> listed;
            for (std::size_t i = sensitivity + 1; i + 1 < *after; ++i) {
                if (tokenIs(tokens[i], "all")) allOrClocked = true;
                const auto found = signals.find(lower(tokens[i].text));
                if (found != signals.end()) listed.try_emplace(found->first, i);
            }
            for (std::size_t i = *after; i < process.end && i < end; ++i) {
                if (tokenIs(tokens[i], "rising_edge") || tokenIs(tokens[i], "falling_edge") ||
                    tokenIs(tokens[i], "event")) allOrClocked = true;
            }
            if (allOrClocked) continue;

            std::map<std::string, std::size_t> reads;
            for (std::size_t i = *after; i < process.end && i < end; ++i) {
                if (!identifier(tokens[i]) ||
                    vhdlAssignmentOperator(tokens, i, process.end)) {
                    continue;
                }
                const auto found = signals.find(lower(tokens[i].text));
                if (found != signals.end()) reads.try_emplace(found->first, i);
            }
            for (const auto& [name, token] : reads) {
                if (listed.contains(name)) continue;
                issues.push_back({"RTL-INCOMPLETE-SENSITIVITY",
                    "Combinational sensitivity list is missing: " + name,
                    "The process reads declared signals that are absent from its sensitivity list.",
                    "Add the missing signals or use process(all) with VHDL-2008.",
                    location(source, tokens[token]), unitName, name});
            }
            for (const auto& [name, token] : listed) {
                if (reads.contains(name)) continue;
                issues.push_back({"RTL-REDUNDANT-SENSITIVITY",
                    "Combinational sensitivity list contains unread signals: " + name,
                    "The process lists declared signals that its body never reads.",
                    "Remove the redundant signals or use process(all) with VHDL-2008.",
                    location(source, tokens[token]), unitName, name});
            }
        }
        for (std::size_t i = body; i + 2 < end; ++i) {
            if (!tokenIs(tokens[i], "port") || !tokenIs(tokens[i + 1], "map") ||
                tokens[i + 2].text != "(") continue;
            const auto after = skipBalanced(tokens, i + 2, end, "(", ")");
            if (!after) continue;
            for (std::size_t actual = i + 3; actual < *after; ++actual) {
                if (!identifier(tokens[actual])) continue;
                if (actual + 2 < *after && tokens[actual + 1].text == "=" &&
                    tokens[actual + 2].text == ">") continue;
                const auto found = signals.find(lower(tokens[actual].text));
                if (found != signals.end()) found->second.connected = true;
            }
            i = *after - 1;
        }
        for (std::size_t i = body; i < end; ++i) {
            if (!identifier(tokens[i]) || assignmentTargets.contains(tokens[i].offset)) continue;
            const auto found = signals.find(lower(tokens[i].text));
            if (found != signals.end() && tokens[i].offset != found->second.declaration.byteOffset) {
                found->second.read = true;
                if (!found->second.firstRead)
                    found->second.firstRead = location(source, tokens[i]);
            }
        }
        for (const auto& [name, signal] : signals) {
            if (signal.drivers.size() > 1) {
                issues.push_back({"RTL-MULTIPLE-DRIVERS", "Signal has multiple independent drivers: " + name,
                    "The signal is assigned by more than one process or concurrent assignment.",
                    "Use a single owning process/assignment or add explicit resolved-bus intent.",
                    signal.drivers[1].location, unitName, name});
            }
            if (signal.direction == Direction::Input && !signal.drivers.empty()) {
                issues.push_back({"RTL-INPUT-DRIVEN", "Input port is assigned internally: " + name,
                    "An input port should be driven by the parent design, not by this architecture.",
                    "Remove the internal assignment or change the port mode.",
                    signal.drivers.front().location, unitName, name});
            }
            if (signal.direction == Direction::Output && signal.drivers.empty() && !signal.connected) {
                issues.push_back({"RTL-UNDRIVEN-OUTPUT", "Output port has no local driver: " + name,
                    "No process or concurrent assignment to this output was found.",
                    "Drive the output explicitly or remove it from the entity interface.",
                    signal.declaration, unitName, name});
            }
            if (signal.direction == Direction::Input && signal.drivers.empty() &&
                !signal.read && !signal.connected) {
                issues.push_back({"RTL-UNUSED-INPUT-PORT", "Input port is never used: " + name,
                    "The input is not read by local logic or forwarded through a port map.",
                    "Remove the unused input, connect it to the intended logic, or suppress the rule for an intentionally reserved interface pin.",
                    signal.declaration, unitName, name});
            }
            if (signal.direction == Direction::Internal && signal.drivers.empty() &&
                signal.read && !signal.connected) {
                issues.push_back({"RTL-UNDRIVEN-SIGNAL", "Signal is read but has no driver: " + name,
                    "The internal signal is consumed but no process or concurrent driver was found.",
                    "Add the missing assignment or connect the intended source.",
                    signal.firstRead.value_or(signal.declaration), unitName, name});
            }
            if (signal.direction == Direction::Internal && !signal.read && !signal.connected) {
                issues.push_back({"RTL-UNUSED-SIGNAL", "Declared signal is never read: " + name,
                    "The internal signal does not contribute to an expression or port map.",
                    "Remove the signal or connect it to the intended logic.",
                    signal.declaration, unitName, name});
            }
        }
        architecture = end;
    }
    for (std::size_t token = 0; token + 1 < tokens.size(); ++token) {
        if (!tokenIs(tokens[token], "shared") ||
            !tokenIs(tokens[token + 1], "variable") ||
            reportedSharedVariables.contains(tokens[token].offset)) continue;
        issues.push_back({"VHDL-SHARED-VARIABLE",
            "Shared variable requires careful synchronization",
            "An unprotected shared variable can cause simulation races and poor synthesis portability.",
            "Prefer signals or a protected type with a documented synchronization scheme.",
            location(source, tokens[token]), std::nullopt, std::nullopt});
    }
    return issues;
}

} // namespace

std::vector<SemanticIssue> analyzeSemantics(const core::SourceFile& source) {
    if (source.language() == core::HdlLanguage::Verilog ||
        source.language() == core::HdlLanguage::SystemVerilog) return analyzeVerilog(source);
    if (source.language() == core::HdlLanguage::Vhdl) return analyzeVhdl(source);
    return {};
}

std::vector<SemanticIssue> analyzeVhdlUnreachableComparisons(const core::SourceFile& source) {
    if (source.language() != core::HdlLanguage::Vhdl) return {};
    auto issues = analyzeVhdl(source, true);
    std::erase_if(issues, [](const auto& issue) {
        return issue.ruleId != "RTL-UNREACHABLE-RANGE-COMPARISON";
    });
    return issues;
}

std::vector<SemanticIssue>
analyzeUnusedVhdlGenerics(const std::vector<core::SourceFile>& sources) {
    return analyzeUnusedVhdlGenericsImpl(sources);
}

} // namespace fpga::parser
