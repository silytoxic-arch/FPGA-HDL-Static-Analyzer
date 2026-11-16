#pragma once

#include "parser/design_parser.h"

#include <algorithm>
#include <cstddef>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace fpga::analysis {

// Conservative lexical constant classification, NOT an HDL elaborator or value
// evaluator. Native evidence for GJB10157.9.2.2 selects an AST_CONSTANT/REALVALUE
// case control (ev_3a2e24112630365fcae32ee1e22794a35ebca760e7d8249ca16fa8cc55f319b2).
// Parameter references need symbol resolution before such a classification can
// be approximated. Unknown names, cycles, calls, types and hierarchical names
// deliberately remain unproved. Parameter override values are not evaluated.
class VerilogConstantExpressions {
public:
    explicit VerilogConstantExpressions(const std::vector<parser::HdlToken>& tokens)
        : tokens_(tokens), tokenScopes_(tokens.size(), 0), scopes_(1) {
        indexScopes();
        indexDeclarations();
    }

    // Half-open token range [begin, end); use the same comment-free, normalized
    // token vector used to construct this object. Scope is resolved at begin.
    [[nodiscard]] bool isConstant(std::size_t begin, std::size_t end) const {
        if (begin >= end || end > tokens_.size()) return false;
        std::vector<std::pair<std::size_t, std::string>> active;
        std::unordered_map<std::string, bool> resolved;
        return expression(begin, end, tokenScopes_[begin], active, resolved);
    }

    // Partial-rule support: unlike !isConstant(), this only proves a dependency
    // on a declared non-parameter variable. Unresolved symbols are not variables.
    [[nodiscard]] bool hasRuntimeDependency(std::size_t begin, std::size_t end) const {
        if (begin >= end || end > tokens_.size()) return false;
        std::vector<std::pair<std::size_t, std::string>> active;
        std::unordered_map<std::string, bool> resolved;
        return runtimeDependency(begin, end, tokenScopes_[begin], active, resolved, {});
    }

    // Same proof, but treats one resolved source identifier as a compile-time
    // substitution supplied by the caller.  The recovered FOR frontend clones
    // its condition after replacing the loop counter with the current constant
    // iteration value; examining isolated identifier tokens would lose system-
    // function and member-access context around the other names.
    [[nodiscard]] bool hasRuntimeDependencyExcept(
        std::size_t begin, std::size_t end, std::string_view ignoredIdentifier) const {
        if (begin >= end || end > tokens_.size()) return false;
        std::vector<std::pair<std::size_t, std::string>> active;
        std::unordered_map<std::string, bool> resolved;
        return runtimeDependency(begin, end, tokenScopes_[begin], active, resolved,
                                 ignoredIdentifier);
    }

    // True only when this identifier token resolves to a declared, unambiguous
    // non-parameter object in its lexical scope. This intentionally does not
    // inspect parameter initializers: native GJB10157.9.3.6 compares each
    // repeat-count AST_IDENTIFIER with its parameter/localparam name table.
    [[nodiscard]] bool isDeclaredRuntimeIdentifier(std::size_t index) const {
        if (index >= tokens_.size() ||
            tokens_[index].kind != parser::HdlTokenKind::Identifier) return false;
        auto scope = tokenScopes_[index];
        for (;;) {
            if (const auto it = scopes_[scope].names.find(tokens_[index].text);
                it != scopes_[scope].names.end())
                return !it->second.parameter && !it->second.ambiguous;
            if (scope == 0) return false;
            scope = scopes_[scope].parent;
        }
    }

private:
    struct Binding {
        bool parameter{false};
        bool ambiguous{false};
        std::size_t begin{0};
        std::size_t end{0};
    };
    struct Scope {
        std::size_t parent{0};
        std::string closing;
        std::unordered_map<std::string, Binding> names;
    };
    const std::vector<parser::HdlToken>& tokens_;
    std::vector<std::size_t> tokenScopes_;
    std::vector<Scope> scopes_;

    static bool isTypeWord(std::string_view word) {
        return word == "signed" || word == "unsigned" || word == "wire" ||
            word == "reg" || word == "logic" || word == "bit" || word == "byte" ||
            word == "shortint" || word == "int" || word == "integer" ||
            word == "longint" || word == "time" || word == "real" ||
            word == "shortreal" || word == "realtime" || word == "var" ||
            word == "automatic" || word == "static" || word == "input" ||
            word == "output" || word == "inout" || word == "ref" ||
            word == "parameter" || word == "localparam" || word == "const" ||
            word == "type";
    }
    static bool isDeclaration(std::string_view word) {
        return word == "parameter" || word == "localparam" || word == "wire" ||
            word == "reg" || word == "logic" || word == "bit" || word == "byte" ||
            word == "shortint" || word == "int" || word == "integer" ||
            word == "longint" || word == "time" || word == "real" ||
            word == "shortreal" || word == "realtime" || word == "input" ||
            word == "output" || word == "inout" || word == "ref" || word == "genvar";
    }

    void indexScopes() {
        std::vector<std::size_t> stack{0};
        for (std::size_t i = 0; i < tokens_.size(); ++i) {
            const auto& word = tokens_[i].text;
            std::string closing;
            if (word == "module") closing = "endmodule";
            else if (word == "interface") closing = "endinterface";
            else if (word == "program") closing = "endprogram";
            else if (word == "package") closing = "endpackage";
            else if (word == "class") closing = "endclass";
            else if (word == "function") closing = "endfunction";
            else if (word == "task") closing = "endtask";
            else if (word == "begin") closing = "end";
            else if (word == "generate") closing = "endgenerate";
            else if (word == "fork") closing = "join";
            if (!closing.empty()) {
                const auto id = scopes_.size();
                scopes_.push_back({stack.back(), std::move(closing), {}});
                stack.push_back(id);
            }
            tokenScopes_[i] = stack.back();
            auto close = word;
            if (word == "join_any" || word == "join_none") close = "join";
            for (std::size_t n = stack.size(); n > 1; --n) {
                if (scopes_[stack[n - 1]].closing == close) {
                    stack.resize(n - 1);
                    break;
                }
            }
        }
    }

    void bind(std::size_t scope, const std::string& name, Binding value) {
        const auto [it, inserted] = scopes_[scope].names.emplace(name, value);
        if (!inserted) {
            // Re-reading the same declaration via its type keyword is harmless;
            // conflicting declarations are not a sound basis for a constant.
            if (it->second.parameter != value.parameter ||
                (value.parameter && (it->second.begin != value.begin || it->second.end != value.end)))
                it->second.ambiguous = true;
        }
    }

    void segment(std::size_t scope, std::size_t begin, std::size_t end, bool parameter) {
        std::size_t name = tokens_.size(), equals = end;
        int brackets = 0;
        bool typeParameter = false;
        for (auto i = begin; i < end; ++i) {
            const auto& token = tokens_[i];
            if (token.text == "[") ++brackets;
            else if (token.text == "]") --brackets;
            else if (brackets == 0 && token.text == "=") { equals = i; break; }
            else if (brackets == 0 && token.text == "type") typeParameter = true;
            else if (brackets == 0 && token.kind == parser::HdlTokenKind::Identifier &&
                     !isTypeWord(token.text) && token.text != "genvar") name = i;
        }
        if (name != tokens_.size())
            bind(scope, tokens_[name].text, {parameter && !typeParameter, typeParameter, equals + 1, end});
    }

    void indexDeclarations() {
        // A locally declared typedef may shadow an outer parameter through a
        // variable of that type. Type aliases themselves are never constants.
        std::vector<std::string> declaredTypes;
        for (std::size_t i = 0; i < tokens_.size(); ++i) {
            if (tokens_[i].text != "typedef") continue;
            std::size_t name = tokens_.size();
            int brackets = 0, braces = 0;
            for (auto j = i + 1; j < tokens_.size() && tokens_[j].text != ";"; ++j) {
                const auto& text = tokens_[j].text;
                if (text == "[") ++brackets;
                else if (text == "]") --brackets;
                else if (text == "{") ++braces;
                else if (text == "}") --braces;
                else if (brackets == 0 && braces == 0 &&
                         tokens_[j].kind == parser::HdlTokenKind::Identifier && !isTypeWord(text)) name = j;
            }
            if (name != tokens_.size()) declaredTypes.push_back(tokens_[name].text);
        }
        for (std::size_t i = 0; i < tokens_.size(); ++i) {
            const auto& word = tokens_[i].text;
            if (word == "function" || word == "task") {
                // The implicit function return variable is a signal too.
                std::size_t name = tokens_.size();
                for (auto j = i + 1; j < tokens_.size() && tokens_[j].text != "(" &&
                     tokens_[j].text != ";"; ++j)
                    if (tokens_[j].kind == parser::HdlTokenKind::Identifier &&
                        !isTypeWord(tokens_[j].text)) name = j;
                if (name != tokens_.size()) bind(tokenScopes_[i], tokens_[name].text, {});
            }
            if (!isDeclaration(word) &&
                std::find(declaredTypes.begin(), declaredTypes.end(), word) == declaredTypes.end()) continue;
            const bool parameter = word == "parameter" || word == "localparam";
            // A type keyword within a parameter declaration must not rebind its
            // name as an ordinary variable (e.g. parameter integer P = 1).
            if (!parameter) {
                bool inParameter = false;
                for (auto j = i; j > 0;) {
                    --j;
                    if (tokenScopes_[j] != tokenScopes_[i] || tokens_[j].text == ";" ||
                        tokens_[j].text == "(" || tokens_[j].text == ")") break;
                    if (tokens_[j].text == "parameter" || tokens_[j].text == "localparam") {
                        inParameter = true; break;
                    }
                }
                if (inParameter) continue;
            }
            std::size_t start = i + 1;
            int parens = 0, brackets = 0, braces = 0;
            for (auto j = start; j < tokens_.size(); ++j) {
                const auto& text = tokens_[j].text;
                const bool flat = parens == 0 && brackets == 0 && braces == 0;
                if (tokenScopes_[j] != tokenScopes_[i] ||
                    (flat && (text == ";" || text == ")" || text == "end"))) {
                    segment(tokenScopes_[i], start, j, parameter); break;
                }
                if (flat && text == ",") {
                    segment(tokenScopes_[i], start, j, parameter);
                    start = j + 1;
                    // A following declaration kind starts a fresh declaration.
                    if (start < tokens_.size() && isDeclaration(tokens_[start].text)) break;
                    continue;
                }
                if (text == "(") ++parens;
                else if (text == ")") --parens;
                else if (text == "[") ++brackets;
                else if (text == "]") --brackets;
                else if (text == "{") ++braces;
                else if (text == "}") --braces;
            }
        }
    }

    bool identifier(const std::string& name, std::size_t scope,
                    std::vector<std::pair<std::size_t, std::string>>& active,
                    std::unordered_map<std::string, bool>& resolved) const {
        for (;;) {
            if (const auto it = scopes_[scope].names.find(name); it != scopes_[scope].names.end()) {
                const auto& binding = it->second;
                if (!binding.parameter || binding.ambiguous || binding.begin >= binding.end) return false;
                const auto key = std::pair{scope, name};
                const auto cacheKey = std::to_string(scope) + ":" + name;
                if (const auto known = resolved.find(cacheKey); known != resolved.end()) return known->second;
                if (active.size() >= 256 || std::find(active.begin(), active.end(), key) != active.end())
                    return false;
                active.push_back(key);
                const auto result = expression(binding.begin, binding.end, scope, active, resolved);
                active.pop_back();
                resolved.emplace(cacheKey, result);
                return result;
            }
            if (scope == 0) return false;
            scope = scopes_[scope].parent;
        }
    }

    bool expression(std::size_t begin, std::size_t end, std::size_t scope,
                    std::vector<std::pair<std::size_t, std::string>>& active,
                    std::unordered_map<std::string, bool>& resolved) const {
        if (begin >= end || end > tokens_.size()) return false;
        bool operand = false;
        for (auto i = begin; i < end; ++i) {
            const auto& token = tokens_[i];
            if (token.kind == parser::HdlTokenKind::Number) operand = true;
            else if (token.kind == parser::HdlTokenKind::Identifier) {
                // Only a tightly bounded known pure numeric system function is
                // accepted; user functions/$bits(type or signal) need elaboration.
                if (token.text == "$clog2" && i + 1 < end && tokens_[i + 1].text == "(") continue;
                if (!identifier(token.text, scope, active, resolved)) return false;
                // P(...) is a call, not a reference to a numeric parameter.
                if (i + 1 < end && tokens_[i + 1].text == "(") return false;
                operand = true;
            } else if (token.kind == parser::HdlTokenKind::String) return false;
            else if (token.text != "+" && token.text != "-" && token.text != "*" &&
                     token.text != "/" && token.text != "%" && token.text != "**" &&
                     token.text != "!" && token.text != "~" && token.text != "&" &&
                     token.text != "|" && token.text != "^" && token.text != "&&" &&
                     token.text != "||" && token.text != "~^" && token.text != "^~" &&
                     token.text != "<" && token.text != ">" && token.text != "<=" &&
                     token.text != ">=" && token.text != "==" && token.text != "!=" &&
                     token.text != "===" && token.text != "!==" && token.text != "<<" &&
                     token.text != ">>" && token.text != "<<<" && token.text != ">>>" &&
                     token.text != "(" && token.text != ")" && token.text != "[" &&
                     token.text != "]" && token.text != "{" && token.text != "}" &&
                     token.text != "," && token.text != "?" && token.text != ":") return false;
        }
        return operand;
    }

    bool runtimeDependency(std::size_t begin, std::size_t end, std::size_t scope,
                           std::vector<std::pair<std::size_t, std::string>>& active,
                           std::unordered_map<std::string, bool>& resolved,
                           std::string_view ignoredIdentifier) const {
        // A macro can replace the entire following token sequence. Without its
        // expansion, even an apparent variable name is not established.
        for (auto i = begin; i < end; ++i) if (tokens_[i].text == "`") return false;
        for (auto i = begin; i < end; ++i) {
            if (tokens_[i].kind != parser::HdlTokenKind::Identifier) continue;
            if (!ignoredIdentifier.empty() && tokens_[i].text == ignoredIdentifier)
                continue;
            // $bits/$size and other elaboration queries can use a signal name
            // yet yield a compile-time value. Their arguments cannot be treated
            // as runtime dependencies without function semantics.
            if (!tokens_[i].text.empty() && tokens_[i].text.front() == '$' &&
                tokens_[i].text != "$clog2" && i + 1 < end && tokens_[i + 1].text == "(") {
                int depth = 0;
                for (++i; i < end; ++i) {
                    if (tokens_[i].text == "(") ++depth;
                    else if (tokens_[i].text == ")" && --depth == 0) break;
                }
                continue;
            }
            // A call name is not the implicit function-return variable. Its
            // ordinary arguments are still inspected independently below.
            if (i + 1 < end && tokens_[i + 1].text == "(") continue;
            // Hierarchical/class/package member binding requires an elaborator.
            if ((i > begin && (tokens_[i - 1].text == "." || tokens_[i - 1].text == "::")) ||
                (i + 1 < end && (tokens_[i + 1].text == "." || tokens_[i + 1].text == "::")) ||
                (i > begin + 1 && tokens_[i - 1].text == ":" && tokens_[i - 2].text == ":") ||
                (i + 2 < end && tokens_[i + 1].text == ":" && tokens_[i + 2].text == ":")) continue;
            auto current = scope;
            for (;;) {
                if (const auto it = scopes_[current].names.find(tokens_[i].text);
                    it != scopes_[current].names.end()) {
                    const auto& binding = it->second;
                    if (binding.ambiguous) break;
                    if (!binding.parameter) return true;
                    const auto key = std::pair{current, tokens_[i].text};
                    const auto cacheKey = std::to_string(current) + ":" + tokens_[i].text;
                    if (const auto known = resolved.find(cacheKey); known != resolved.end()) {
                        if (known->second) return true;
                        break;
                    }
                    if (binding.begin >= binding.end || active.size() >= 256 ||
                        std::find(active.begin(), active.end(), key) != active.end()) break;
                    active.push_back(key);
                    const auto depends = runtimeDependency(
                        binding.begin, binding.end, current, active, resolved,
                        ignoredIdentifier);
                    active.pop_back();
                    resolved.emplace(cacheKey, depends);
                    if (depends) return true;
                    break;
                }
                if (current == 0) break;
                current = scopes_[current].parent;
            }
        }
        return false;
    }
};

} // namespace fpga::analysis
