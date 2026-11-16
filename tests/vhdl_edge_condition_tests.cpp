#include "analysis/vhdl_edge_condition.h"

#include <iostream>
#include <string>

int main() {
    using namespace fpga;
    using namespace analysis;
    int failures = 0;
    const auto check = [&](std::string text, VhdlEdgeConditionStatus status,
                           VhdlEdgeDirection direction, std::string clock, const char* label,
                           bool merged = false) {
        core::SourceFile source("probe.vhd", core::HdlLanguage::Vhdl, std::move(text));
        auto tokens = parser::tokenize(source);
        if (merged) {
            std::vector<parser::HdlToken> joined;
            for (std::size_t i = 0; i < tokens.size(); ++i) {
                auto token = tokens[i];
                if (i + 1 < tokens.size() && token.text == "'" && tokens[i + 1].kind == parser::HdlTokenKind::Number &&
                    tokens[i + 1].text.size() == 2 && tokens[i + 1].text.back() == '\'' &&
                    tokens[i + 1].offset == token.offset + 1) {
                    token.text += tokens[++i].text;
                    token.kind = parser::HdlTokenKind::Number;
                } else if (i + 2 < tokens.size() && token.text == "'" && tokens[i + 2].text == "'" &&
                    tokens[i + 1].text.size() == 1 && tokens[i + 1].offset == token.offset + 1 &&
                    tokens[i + 2].offset == token.offset + 2) {
                    token.text += tokens[i + 1].text + "'";
                    token.kind = parser::HdlTokenKind::Number; i += 2;
                }
                joined.push_back(std::move(token));
            }
            tokens = std::move(joined);
        }
        const auto result = parseVhdlEdgeCondition(source, tokens, 0, tokens.size());
        if (result.status != status || result.direction != direction || result.clockName != clock ||
            (status == VhdlEdgeConditionStatus::Matched &&
             (result.clockToken >= tokens.size() || tokens[result.clockToken].kind != parser::HdlTokenKind::Identifier))) {
            ++failures; std::cerr << "FAIL: " << label << '\n';
        }
    };
    constexpr auto matched = VhdlEdgeConditionStatus::Matched;
    constexpr auto incomplete = VhdlEdgeConditionStatus::Incomplete;
    constexpr auto unsupported = VhdlEdgeConditionStatus::Unsupported;
    constexpr auto rising = VhdlEdgeDirection::Rising;
    constexpr auto falling = VhdlEdgeDirection::Falling;
    constexpr auto unknown = VhdlEdgeDirection::Unknown;
    check("rising_edge(clk)", matched,rising,"clk","simple rising");
    check("falling_edge(clk)", matched,falling,"clk","simple falling");
    check("RISING_EDGE(CLK_2)", matched,rising,"clk_2","case-insensitive simple name");
    check("rising_edge(\n -- hidden signal fake\n clk)", matched,rising,"clk","comments excluded by public lexer");
    check("clk'event and clk='1'",matched,rising,"clk","event left binary equality right raw tokens");
    check("clk'event and clk='0'",matched,falling,"clk","old falling raw tokens");
    check("CLK'EVENT AND cLk='1'",matched,rising,"clk","event identifier semantic lower casing");
    check("clk'event and '1'=clk",matched,rising,"clk","equality operands may be reversed");
    check("clk'event and '0'=clk",matched,falling,"clk","reversed falling equality");
    check("clk'event and clk='1'",matched,rising,"clk","merged character literal",true);
    check("clk'event and '0'=clk",matched,falling,"clk","merged reversed literal",true);
    check("clk='1' and clk'event",matched,rising,"clk","AND operands may swap");
    check("clk'event and other='1'",incomplete,unknown,"","mismatched clock names remain incomplete");
    check("clk'event and clk=1",incomplete,unknown,"","integer one isn't exact character literal");
    check("clk'event and clk='H'",incomplete,unknown,"","meta state not a binary edge");
    check("clk'event and clk=\"1\"",incomplete,unknown,"","string isn't character literal");
    check("clk'event and clk=' 1 '",incomplete,unknown,"","whitespace inside quoted literal rejected");
    check("clk'event and clk='01'",incomplete,unknown,"","multiple character literal rejected");
    check("clk'event and clk='1' and enable",incomplete,unknown,"","extra conjunction not folded");
    check("(clk'event) and (clk='1')",matched,rising,"clk","operand wrappers disappear in AST");
    check("clk'event and (clk='1' and enable)",incomplete,unknown,"","nested AND unsupported");
    check("rising_edge(clks(0))",incomplete,unknown,"","indexed clock not simple identifier");
    check("rising_edge(rec.clk)",incomplete,unknown,"","selected name not simple identifier");
    check("std_logic_1164.rising_edge(clk)",incomplete,unknown,"","qualified edge function unsupported");
    check("rising_edge(\\clk\\)",incomplete,unknown,"","extended identifier unsupported");
    check("rising_edge(clk,other)",incomplete,unknown,"","multiple arguments unsupported");
    check("rising_edge(clk$)",incomplete,unknown,"","non-VHDL basic identifier excluded");
    check("rising_edge(clk__2)",incomplete,unknown,"","consecutive underscore not basic identifier");
    check("rising_edge(clk_)",incomplete,unknown,"","trailing underscore not basic identifier");
    check("rising_edge(null)",incomplete,unknown,"","reserved keyword cannot be basic clock identifier");
    check("clk(0)'event and clk(0)='1'",incomplete,unknown,"","selected event/EQ identifiers unsupported");
    check("clk'event or clk='1'",incomplete,unknown,"","OR isn't supported old edge");
    check("clk'event and clk/='1'",incomplete,unknown,"","inequality isn't supported equality");
    check("not rising_edge(clk)",incomplete,unknown,"","negated edge incomplete");
    check("enable = true",unsupported,unknown,"","ordinary condition isn't an edge candidate");
    check("\"rising_edge(clk)\"",unsupported,unknown,"","string edge text ignored");
    check("-- rising_edge(clk)\n",incomplete,unknown,"","empty range cannot establish a condition");
    check("rising_edge(",incomplete,unknown,"","truncated edge expression incomplete");
    // A caller-supplied subrange must preserve absolute token indices.
    core::SourceFile source("probe.vhd", core::HdlLanguage::Vhdl, "if rising_edge(CLK) then null; end if;");
    const auto tokens = parser::tokenize(source);
    const auto subrange = parseVhdlEdgeCondition(source,tokens,1,5);
    if (subrange.status != matched || subrange.clockToken != 3 || subrange.clockName != "clk") ++failures;
    if (parseVhdlEdgeCondition(source,tokens,0,tokens.size()+1).status != incomplete) ++failures;
    core::SourceFile wrongLanguage("probe.v", core::HdlLanguage::Verilog, "rising_edge(clk)");
    if (parseVhdlEdgeCondition(wrongLanguage,parser::tokenize(wrongLanguage),0,4).status != unsupported) ++failures;
    // Older/general lexer variants can produce three tokens for a character.
    core::SourceFile tripleSource("probe.vhd",core::HdlLanguage::Vhdl,"clk'event and clk='1'");
    const auto raw = parser::tokenize(tripleSource);
    std::vector<parser::HdlToken> triple;
    for (const auto& token : raw) {
        if (token.kind == parser::HdlTokenKind::Number && token.text == "1'") {
            auto number = token;
            number.text = "1";
            triple.push_back(number);
            triple.push_back({parser::HdlTokenKind::Symbol,"'",token.line,token.column+1,token.offset+1});
        } else triple.push_back(token);
    }
    if (parseVhdlEdgeCondition(tripleSource,triple,0,triple.size()).status != matched) ++failures;

    const auto checkFirst = [&](std::string text, VhdlEdgeConditionStatus status,
                                VhdlEdgeDirection direction, std::string clock,
                                const char* label) {
        core::SourceFile nested("probe.vhd", core::HdlLanguage::Vhdl, std::move(text));
        const auto nestedTokens = parser::tokenize(nested);
        const auto found = findFirstVhdlEdgeCondition(nested, nestedTokens, 0, nestedTokens.size());
        if (found.status != status || found.direction != direction || found.clockName != clock) {
            ++failures;
            std::cerr << "FAIL: " << label << " (clock=" << found.clockName
                      << ", status=" << static_cast<int>(found.status) << ")\n";
        }
    };
    checkFirst("clk'event and clk='1' and enable", matched, rising, "clk",
               "left-associated legacy subtree precedes trailing enable");
    checkFirst("enable and clk'event and clk='1'", incomplete, unknown, "",
               "left association does not invent a trailing legacy subtree");
    checkFirst("enable and (clk'event and clk='1')", matched, rising, "clk",
               "parenthesized right legacy subtree is visited");
    checkFirst("clk'event and clk='1' and falling_edge(rst)", matched, rising, "clk",
               "preorder selects left legacy subtree before later edge call");
    checkFirst("falling_edge(rst) and clk'event and clk='1'", matched, falling, "rst",
               "preorder selects earlier explicit edge before incomplete legacy shape");
    checkFirst("wrapper(clk'event and clk='0')", matched, falling, "clk",
               "collector descends through non-edge call arguments");
    checkFirst("not rising_edge(clk)", matched, rising, "clk",
               "collector descends through unary logical nodes");
    checkFirst("enable or (clk'event and clk='1')", matched, rising, "clk",
               "OR is not an edge idiom but its AND child remains discoverable");
    return failures == 0 ? 0 : 1;
}
