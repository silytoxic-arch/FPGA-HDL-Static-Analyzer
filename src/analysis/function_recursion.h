#pragma once

#include "parser/design_parser.h"

#include <algorithm>
#include <cstddef>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace fpga::analysis::detail {

struct FunctionCallSite { std::string callee; std::size_t token; };
struct ScopedFunction {
    std::size_t moduleToken;
    std::string moduleName;
    std::string name;
    std::size_t nameToken;
    std::size_t bodyBegin;
    std::size_t bodyEnd;
    std::vector<FunctionCallSite> calls;
};
struct FunctionRecursionCycle {
    std::vector<std::size_t> functions; // path order; indices into functions
};
struct FunctionRecursionResult {
    std::vector<ScopedFunction> functions;
    std::vector<FunctionRecursionCycle> cycles;
    bool incomplete{false};
};

// Native reconstruction: admit only FUNCTION nodes that contain an FCALL,
// build one case-sensitive name graph for the run, start a DFS from every stored
// caller/callee edge, and deduplicate closed paths by their member-name set.
// This does not execute HDL, and deliberately does not resolve
// class/package/hierarchical calls in the bounded source adapter.
// Native function+FCALL collector and name graph:
// ev_e13dfc70635c3e47a0883fc2b46110f1803b213fe8676233046f6b00ef3aa40f
// ev_bb439465834fb0b8f08945cbaf0d4a18e1d519768494816ed04d3b6641d60df9
// Path-based closure helper:
// ev_514f2733ff6dc76663e9353c6881576bb4fc7b96552160f85f89e84e9fc6075f
[[nodiscard]] inline FunctionRecursionResult functionRecursion(
    const core::SourceFile& source,
    const std::vector<parser::HdlToken>& tokens) {
    FunctionRecursionResult result;
    const auto keyword = [&](std::size_t i) {
        return tokens[i].offset < source.content().size() &&
            source.content()[tokens[i].offset] != '\\';
    };
    const auto identifier = [&](std::size_t i) {
        return i < tokens.size() && tokens[i].kind == parser::HdlTokenKind::Identifier;
    };
    const auto seek = [&](std::size_t i, std::size_t limit, const std::string& text) {
        while (i < limit && (tokens[i].text != text || !keyword(i))) ++i;
        return i;
    };
    for (std::size_t module = 0; module < tokens.size(); ++module) {
        if (tokens[module].text != "module" || !keyword(module)) continue;
        const auto moduleEnd = seek(module + 1, tokens.size(), "endmodule");
        if (moduleEnd == tokens.size()) { result.incomplete = true; break; }
        std::string moduleName;
        for (auto j = module + 1; j < moduleEnd; ++j) {
            if (identifier(j)) { moduleName = tokens[j].text; break; }
            if (tokens[j].text == ";" || tokens[j].text == "(") break;
        }
        if (!moduleName.empty() && moduleName.front() == '\\')
            moduleName.erase(moduleName.begin());
        for (auto i = module + 1; i < moduleEnd; ++i) {
            if (tokens[i].text == "class" && keyword(i)) {
                const auto endClass = seek(i + 1, moduleEnd, "endclass");
                result.incomplete = true;
                i = endClass;
                continue;
            }
            if (tokens[i].text != "function" || !keyword(i)) continue;
            if (i > 0 && (tokens[i - 1].text == "extern" ||
                (tokens[i - 1].text == "virtual" && i > 1 && tokens[i - 2].text == "pure"))) {
                result.incomplete = true;
                i = seek(i + 1, moduleEnd, ";");
                continue;
            }
            const auto endFunction = seek(i + 1, moduleEnd, "endfunction");
            if (endFunction == moduleEnd) { result.incomplete = true; break; }
            auto headerEnd = i + 1;
            int parens = 0;
            for (; headerEnd < endFunction; ++headerEnd) {
                if (tokens[headerEnd].text == "(") ++parens;
                else if (tokens[headerEnd].text == ")") --parens;
                if (tokens[headerEnd].text == ";" && parens == 0) break;
            }
            if (headerEnd == endFunction || parens != 0) {
                result.incomplete = true;
                i = endFunction;
                continue;
            }
            auto nameToken = headerEnd;
            for (auto j = i + 1; j < headerEnd; ++j) {
                if (tokens[j].text == "(" && j > i + 1 && identifier(j - 1)) {
                    nameToken = j - 1;
                    break;
                }
                if (identifier(j)) nameToken = j;
            }
            if (nameToken == headerEnd || tokens[nameToken].text == "void" ||
                tokens[nameToken].text == "automatic" || tokens[nameToken].text == "static") {
                result.incomplete = true;
                i = endFunction;
                continue;
            }
            ScopedFunction function{module, moduleName, tokens[nameToken].text, nameToken,
                headerEnd + 1, endFunction, {}};
            std::set<std::string> localNames;
            const auto declarationKeyword = [](const std::string& word) {
                return word == "input" || word == "output" || word == "inout" ||
                    word == "reg" || word == "wire" || word == "logic" || word == "bit" ||
                    word == "integer" || word == "int" || word == "byte" || word == "shortint" ||
                    word == "longint" || word == "real" || word == "string" ||
                    word == "parameter" || word == "localparam";
            };
            // Conservatively exclude names declared as parameters/locals from
            // function-call resolution. Typedef/class-specific declarations
            // remain outside this deliberately bounded source parser.
            for (auto j = nameToken + 1; j < function.bodyEnd; ++j) {
                if (!declarationKeyword(tokens[j].text)) continue;
                for (++j; j < function.bodyEnd && tokens[j].text != ";" &&
                     tokens[j].text != ")"; ++j) {
                    if (tokens[j].text == "=") {
                        while (j < function.bodyEnd && tokens[j].text != ";" &&
                            tokens[j].text != ",") ++j;
                        if (j == function.bodyEnd || tokens[j].text == ";") break;
                    } else if (identifier(j) && !declarationKeyword(tokens[j].text) &&
                        tokens[j].text != "signed" && tokens[j].text != "unsigned")
                        localNames.insert(tokens[j].text);
                }
            }
            for (auto j = function.bodyBegin; j + 1 < function.bodyEnd; ++j) {
                if (!identifier(j) || tokens[j + 1].text != "(") continue;
                const auto& name = tokens[j].text;
                if (name == "if" || name == "case" || name == "casex" || name == "casez" ||
                    name == "for" || name == "while" || name == "repeat" || name == "return") continue;
                if (localNames.contains(name)) { result.incomplete = true; continue; }
                if (j > 0 && (tokens[j - 1].text == "`" || tokens[j - 1].text == "." || tokens[j - 1].text == "::" ||
                    (j > 1 && tokens[j - 1].text == ":" && tokens[j - 2].text == ":"))) {
                    result.incomplete = true;
                    continue;
                }
                function.calls.push_back({name, j});
            }
            result.functions.push_back(std::move(function));
            i = endFunction;
        }
        module = moduleEnd;
    }

    // The native std::map assignment overwrites the report node for duplicate
    // names, while calls gathered from all same-name candidates are merged.
    // FUN_180588f70 compares only the function-name string at AST+0x48; neither
    // the candidate map nor adjacency key contains a module identity.
    std::map<std::string, std::size_t> candidates;
    std::set<std::string> declaredNames;
    for (std::size_t i = 0; i < result.functions.size(); ++i) {
        const auto& function = result.functions[i];
        declaredNames.insert(function.name);
        if (!function.calls.empty())
            candidates[function.name] = i;
    }
    std::map<std::string, std::vector<std::string>> adjacency;
    for (std::size_t i = 0; i < result.functions.size(); ++i) {
        const auto& function = result.functions[i];
        if (function.calls.empty()) continue;
        for (const auto& call : function.calls) {
            const auto found = candidates.find(call.callee);
            if (found == candidates.end()) {
                // Native graph construction ignores names absent from its
                // admitted FUNCTION map. Keep unsupported source resolution
                // review-required rather than claiming whole-rule compliance.
                if (!call.callee.starts_with("$") &&
                    !declaredNames.contains(call.callee))
                    result.incomplete = true;
                continue;
            }
            adjacency[function.name].push_back(call.callee);
        }
    }
    if (adjacency.size() > 4096) { result.incomplete = true; return result; }

    std::set<std::set<std::string>> reported;
    for (const auto& [caller, callees] : adjacency) {
        for (const auto& callee : callees) {
            std::vector<std::string> path{caller};
            std::set<std::string> active{caller};
            const auto visit = [&](const auto& self, const std::string& current) -> void {
                if (active.contains(current)) {
                    if (current == path.front()) {
                        if (reported.insert(active).second) {
                            FunctionRecursionCycle cycle;
                            for (const auto& name : path) {
                                const auto found = candidates.find(name);
                                if (found != candidates.end()) cycle.functions.push_back(found->second);
                            }
                            if (!cycle.functions.empty()) result.cycles.push_back(std::move(cycle));
                        }
                    }
                    return;
                }
                if (std::ranges::find(path, current) != path.end() ||
                    path.size() > adjacency.size()) return;
                active.insert(current);
                path.push_back(current);
                const auto edges = adjacency.find(current);
                if (edges != adjacency.end()) {
                    for (const auto& next : edges->second) self(self, next);
                }
                path.pop_back();
                active.erase(current);
            };
            visit(visit, callee);
        }
    }
    return result;
}

} // namespace fpga::analysis::detail
