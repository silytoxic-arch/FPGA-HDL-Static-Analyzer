#pragma once

#include "analysis/vhdl_edge_condition.h"
#include <algorithm>
#include <map>
#include <optional>
#include <set>

namespace fpga::analysis {

struct VhdlClockControlModel {
    std::size_t architectureToken{}, processToken{};
    unsigned nativeClassCandidate{}; // Bounded source reconstruction, not a vendor run.
    VhdlEdgeCondition edge;
    std::vector<std::size_t> equalityIdentifierTokens;
    // Lexical owners bound by this source adapter. Native normalization for
    // equal-spelling objects in distinct local scopes remains unverified.
    std::size_t clockBindingOwner{};
    std::vector<std::size_t> equalityBindingOwners;
};
struct VhdlClockControlOverlap {
    std::size_t architectureToken{}, clockToken{}, equalityIdentifierToken{};
    std::string name;
};
struct VhdlClockControlScan {
    std::vector<VhdlClockControlModel> models;
    std::vector<VhdlClockControlOverlap> overlaps;
    // This rule remains Partial even when all input fits this subset. Empty
    // overlaps never prove compliance. Unknown/unsupported syntax sets this.
    bool incomplete{false};
    bool partial{true};
};

namespace vhdl_clock_detail {
inline std::string lower(std::string s) {
    for (auto& c:s) if (c>='A' && c<='Z') c=static_cast<char>(c-'A'+'a');
    return s;
}
inline bool basic(const parser::HdlToken& t) {
    if (t.kind!=parser::HdlTokenKind::Identifier || t.text.empty()) return false;
    const auto letter=[](char c){return (c>='a'&&c<='z')||(c>='A'&&c<='Z');};
    if (!letter(t.text.front())) return false;
    bool underscore=false;
    for (char c:t.text) {
        if (!letter(c) && !(c>='0'&&c<='9') && c!='_') return false;
        if (c=='_' && underscore) return false;
        underscore=c=='_';
    }
    return !underscore;
}
struct Branch { std::size_t conditionBegin{},conditionEnd{},bodyBegin{},bodyEnd{}; };
struct If { std::size_t begin{},end{}; std::vector<Branch> branches; };
class Scanner {
    const core::SourceFile& source;
    const std::vector<parser::HdlToken>& t;
    VhdlClockControlScan result;
    using Names=std::set<std::string>;
    using Bindings=std::map<std::string,std::size_t>;
    using ScalarTypes=std::set<std::string>; // Source visibility, not vendor type IDs.
    bool word(std::size_t i,std::string_view w) const {
        return i<t.size() && t[i].kind==parser::HdlTokenKind::Identifier && lower(t[i].text)==w;
    }
    bool scalar(std::size_t i) const { return word(i,"std_logic")||word(i,"std_ulogic"); }
    struct StatementEnd { std::size_t end{}, keyword{}, after{}; };
    bool closingKeyword(std::size_t i) const {
        return (i>0 && word(i-1,"end")) ||
            (i>1 && word(i-1,"postponed") && word(i-2,"end"));
    }
    std::optional<std::size_t> generateControl(std::size_t generate) const {
        for (auto i=generate;i>0;) {
            --i;
            if (word(i,"if") || word(i,"for") || word(i,"case")) {
                if (i>=2 && t[i-1].text==":" && basic(t[i-2])) return i;
                return std::nullopt;
            }
            if (t[i].text==";" || word(i,"generate") || word(i,"begin") ||
                word(i,"end") || word(i,"elsif") || word(i,"else")) break;
        }
        return std::nullopt;
    }
    std::optional<StatementEnd> statementEnd(std::size_t opening,
                                            std::string_view kind,std::size_t limit) const {
        unsigned depth=1;
        for (auto i=opening+1;i<limit;++i) {
            if (word(i,"end")) {
                auto keyword=i+1;
                if (kind=="process" && word(keyword,"postponed")) ++keyword;
                if (word(keyword,kind)) {
                    if (--depth==0) {
                        auto semi=keyword+1;
                        if (semi<limit && basic(t[semi])) ++semi;
                        if (semi>=limit || t[semi].text!=";") return std::nullopt;
                        return StatementEnd{i,keyword,semi+1};
                    }
                    i=keyword;
                    continue;
                }
            }
            if (word(i,kind) && !closingKeyword(i) &&
                (kind!="generate" || generateControl(i))) ++depth;
        }
        return std::nullopt;
    }
    std::size_t unitEnd(std::size_t start,std::string_view kind,const std::string& name) const {
        for (auto i=start;i+1<t.size();++i) {
            if (kind=="architecture" && !closingKeyword(i)) {
                const auto nested=word(i,"process")?"process":word(i,"block")?"block":
                    (word(i,"generate") && generateControl(i))?"generate":"";
                if (*nested) {
                    const auto end=statementEnd(i,nested,t.size());
                    if (!end) return t.size();
                    i=end->after-1;
                    continue;
                }
            }
            if (word(i,"end") && (word(i+1,kind)||word(i+1,name)||t[i+1].text==";")) return i;
        }
        return t.size();
    }
    bool primitiveShadow(const std::string& name) const {
        return name=="rising_edge" || name=="falling_edge" ||
            name=="std_logic" || name=="std_ulogic";
    }
    // Source binding for unconstrained, unqualified IEEE subtype chains. The
    // recovered edge/condition helpers consume identifier nodes, not type marks.
    // This is not a reconstruction of the vendor's complete type engine.
    std::optional<std::size_t> typeDeclaration(std::size_t start,std::size_t limit,
                                               Names& names,Bindings& bindings,ScalarTypes& types) const {
        if (start+3>=limit || !basic(t[start+1]) || !word(start+2,"is")) return std::nullopt;
        const auto name=lower(t[start+1].text);
        if (primitiveShadow(name)) return std::nullopt;
        std::vector<std::string> hidden{name};
        auto i=start+3;
        bool scalarAlias=false;
        if (word(start,"subtype")) {
            // Resolve the previously visible bare mark only. Selected package
            // names, constraints and resolution functions are not guessed.
            if (!basic(t[i])) return std::nullopt;
            const auto base=lower(t[i].text);
            scalarAlias=base!=name && types.contains(base);
            ++i;
            while (i<limit && t[i].text==".") {
                scalarAlias=false;
                if (++i>=limit || !basic(t[i])) return std::nullopt;
                ++i;
            }
        } else {
            if (t[i].text!="(") return std::nullopt;
            ++i;
            std::set<std::string> literals;
            for (;;) {
                if (i>=limit || !basic(t[i]) || word(i,"begin") || word(i,"end")) return std::nullopt;
                const auto literal=lower(t[i].text);
                if (primitiveShadow(literal) || literal==name || !literals.insert(literal).second)
                    return std::nullopt;
                hidden.push_back(literal);
                if (++i>=limit) return std::nullopt;
                if (t[i].text==")") { ++i; break; }
                if (t[i].text!=",") return std::nullopt;
                ++i;
            }
        }
        if (i>=limit || t[i].text!=";") return std::nullopt;
        for (const auto& identifier:hidden) {
            names.erase(identifier); bindings.erase(identifier); types.erase(identifier);
        }
        if (scalarAlias) types.insert(name);
        return i+1;
    }
    // Declaration names hide outer objects before their type is considered.
    // Non-scalar objects and constants never inherit an outer scalar binding.
    std::optional<std::size_t> declarationBody(std::size_t start,std::size_t limit,
                                              Names& names,Bindings& bindings,ScalarTypes& types,
                                              bool beginRequired) const {
        auto i=start;
        bool sawDeclaration=false;
        Names declaredHere;
        while (i<limit) {
            if (word(i,"begin")) return i+1;
            if (word(i,"type") || word(i,"subtype")) {
                if (i+1>=limit || !declaredHere.insert(lower(t[i+1].text)).second) return std::nullopt;
                const auto next=typeDeclaration(i,limit,names,bindings,types);
                if (!next) return std::nullopt;
                sawDeclaration=true;
                i=*next;
                continue;
            }
            if (!word(i,"signal") && !word(i,"constant") && !word(i,"variable") && !word(i,"file")) {
                if (beginRequired || sawDeclaration || word(i,"type") || word(i,"subtype") ||
                    word(i,"function") || word(i,"procedure") || word(i,"alias") ||
                    word(i,"package") || word(i,"use") || word(i,"attribute") ||
                    word(i,"generic") || word(i,"port")) return std::nullopt;
                return i; // Optional declaration part omitted in GENERATE.
            }
            sawDeclaration=true;
            const bool signal=word(i,"signal");
            ++i;
            std::vector<std::string> declared;
            while (i<limit && basic(t[i])) {
                const auto name=lower(t[i].text);
                if (primitiveShadow(name) || !declaredHere.insert(name).second) return std::nullopt;
                declared.push_back(name);
                ++i;
                if (i>=limit || t[i].text!=",") break;
                ++i;
            }
            if (declared.empty() || i>=limit || t[i].text!=":" || i+1>=limit)
                return std::nullopt;
            ++i;
            const bool scalarType=basic(t[i]) && types.contains(lower(t[i].text));
            auto tail=i+1;
            if (word(tail,"register") || word(tail,"bus")) ++tail;
            const bool initializer=tail+1<limit && t[tail].text==":" && t[tail+1].text=="=" &&
                t[tail+1].offset==t[tail].offset+1;
            const bool boundScalar=signal && scalarType && tail<limit &&
                (t[tail].text==";" || initializer);
            for (const auto& name:declared) {
                names.erase(name);
                bindings.erase(name);
                types.erase(name);
                if (boundScalar) { names.insert(name); bindings[name]=i; }
            }
            unsigned parentheses=0;
            for (;i<limit;++i) {
                if (t[i].text=="(") ++parentheses;
                else if (t[i].text==")") {
                    if (!parentheses) return std::nullopt;
                    --parentheses;
                } else if (t[i].text==";" && !parentheses) break;
                if (word(i,"begin") || word(i,"end")) return std::nullopt;
            }
            if (i==limit) return std::nullopt;
            ++i;
        }
        if (!beginRequired && !sawDeclaration) return limit;
        return std::nullopt;
    }
    bool supportedGenerateBody(std::size_t start,std::size_t limit) const {
        for (auto i=start;i<limit;++i) {
            const auto nested=word(i,"process")?"process":word(i,"block")?"block":
                (word(i,"generate") && generateControl(i))?"generate":"";
            if (*nested && !closingKeyword(i)) {
                const auto end=statementEnd(i,nested,limit);
                if (!end) return false;
                i=end->after-1;
            } else if (word(i,"generate") && !closingKeyword(i)) {
                return false; // Alternative-generate grammar not established.
            }
        }
        return true;
    }
    void concurrent(std::size_t start,std::size_t limit,std::size_t architecture,
                    const Names& names,const Bindings& bindings,const ScalarTypes& types) {
        for (auto i=start;i<limit;++i) {
            if (closingKeyword(i)) continue;
            if (word(i,"process")) {
                const auto end=statementEnd(i,"process",limit);
                if (!end) { result.incomplete=true; return; }
                process(i,end->end,architecture,names,bindings);
                i=end->after-1;
            } else if (word(i,"block") || word(i,"generate")) {
                const bool block=word(i,"block");
                const auto end=statementEnd(i,block?"block":"generate",limit);
                if (!end) { result.incomplete=true; return; }
                auto local=names;
                auto localBindings=bindings;
                auto localTypes=types;
                auto body=i+1;
                bool admitted=true;
                if (block) {
                    admitted=i>=2 && t[i-1].text==":" && basic(t[i-2]);
                    if (body<end->end && t[body].text=="(") {
                        unsigned depth=1;
                        while (++body<end->end && depth) {
                            if (t[body].text=="(") ++depth;
                            else if (t[body].text==")") --depth;
                        }
                        if (depth) admitted=false;
                    }
                    if (word(body,"is")) ++body;
                } else {
                    const auto control=generateControl(i);
                    admitted=control && *control+1<i &&
                        (word(*control,"if") || word(*control,"for")) &&
                        supportedGenerateBody(body,end->end);
                    if (admitted && word(*control,"for")) {
                        const auto index=*control+1;
                        if (index+2>=i || !basic(t[index]) || !word(index+1,"in") ||
                            primitiveShadow(lower(t[index].text))) admitted=false;
                        else { // A generate iterator cannot inherit an outer signal binding.
                            local.erase(lower(t[index].text));
                            localBindings.erase(lower(t[index].text));
                            localTypes.erase(lower(t[index].text));
                        }
                    }
                }
                const auto begin=admitted?declarationBody(body,end->end,local,localBindings,localTypes,block):std::nullopt;
                if (begin) concurrent(*begin,end->end,architecture,local,localBindings,localTypes);
                else result.incomplete=true;
                i=end->after-1;
            } else if (word(i,"signal") || word(i,"constant") || word(i,"type") ||
                       word(i,"subtype") || word(i,"function") || word(i,"procedure") ||
                       word(i,"alias") || word(i,"package") || word(i,"use")) {
                result.incomplete=true;
                return;
            }
        }
    }
    bool bound(std::size_t i,const Names& names) const {
        return i<t.size() && basic(t[i]) && names.contains(lower(t[i].text));
    }
    bool ieee(std::size_t a,std::size_t b) const {
        bool found=false;
        for (auto i=a;i<b;++i)
            if (word(i,"use")) {
                if (i+6>=b) return false;
                // Additional IEEE packages in the packaged example do not
                // introduce a user-defined rising_edge. Imports from another
                // library remain unsupported because they can shadow it.
                if (!(word(i+1,"ieee") && t[i+2].text=="." && basic(t[i+3]) &&
                    t[i+4].text=="." && word(i+5,"all") && t[i+6].text==";"))
                    return false;
                if (word(i+3,"std_logic_1164")) found=true;
            }
        return found;
    }
    // Only literals whose text is present contiguously in source. No constant
    // evaluation, parameter names, aggregates or function calls are inferred.
    bool literal(std::size_t a,std::size_t b) const {
        if (a>=b) return false;
        const auto start=t[a].offset, stop=t[b-1].offset+t[b-1].text.size();
        if (stop>source.content().size()) return false;
        const auto text=source.content().substr(start,stop-start);
        if (text.size()==3 && text.front()=='\'' && text.back()=='\'' &&
            std::string_view("01xXzZuUwWlLhH-").find(text[1])!=std::string_view::npos) return true;
        if (text.size()>=2 && text.front()=='"' && text.back()=='"') {
            for (std::size_t i=1;i+1<text.size();++i)
                if (std::string_view("01xXzZuUwWlLhH-").find(text[i])==std::string_view::npos) return false;
            return true;
        }
        if (b-a!=1 || t[a].kind!=parser::HdlTokenKind::Number) return false;
        return std::all_of(text.begin(),text.end(),[](char c){return (c>='0'&&c<='9')||c=='_';});
    }
    bool eq(std::size_t a,std::size_t b,const Names& names,std::size_t& chosen) const {
        // Exact direct EQ operands. No logical wrappers, selected names or
        // descendant comparisons; native first identifier operand wins.
        for (auto i=a;i<b;++i) if (t[i].text=="=") {
            if (i==a+1 && bound(a,names) &&
                ((i+2==b && bound(i+1,names)) || literal(i+1,b))) { chosen=a; return true; }
            if (i+2==b && bound(i+1,names) && literal(a,i)) { chosen=i+1; return true; }
            return false;
        }
        return false;
    }
    bool condition(std::size_t a,std::size_t b,const Names& names) const {
        std::size_t unused{};
        return (b==a+1 && bound(a,names)) ||
            (b==a+2 && word(a,"not") && bound(a+1,names)) || eq(a,b,names,unused);
    }
    bool readIf(std::size_t a,std::size_t limit,If& out) const {
        if (!word(a,"if")) return false;
        out.begin=a;
        auto cond=a+1;
        auto then=cond;
        for (;then<limit && !word(then,"then");++then)
            if (word(then,"if")||word(then,"end")||t[then].text==";") return false;
        if (then==limit) return false;
        Branch branch{cond,then,then+1,0};
        unsigned depth=0;
        bool seenElse=false;
        for (auto i=then+1;i<limit;++i) {
            if (word(i,"end") && word(i+1,"if")) {
                if (depth) { --depth; ++i; continue; }
                branch.bodyEnd=i; out.branches.push_back(branch);
                if (i+2>=limit || t[i+2].text!=";") return false;
                out.end=i+3; return true;
            }
            if (word(i,"if")) { ++depth; continue; }
            if (depth) continue;
            if (word(i,"elsif")||word(i,"else")) {
                if (seenElse) return false;
                branch.bodyEnd=i; out.branches.push_back(branch);
                if (word(i,"else")) { seenElse=true; branch={i,i,i+1,0}; continue; }
                cond=i+1; then=cond;
                for (;then<limit && !word(then,"then");++then)
                    if (word(then,"end")||t[then].text==";") return false;
                if (then==limit) return false;
                branch={cond,then,then+1,0}; i=then;
            }
        }
        return false;
    }
    bool assignments(std::size_t a,std::size_t b,const Names& names,bool constants) const {
        bool assigned=false;
        while (a<b) {
            if (word(a,"null") && a+1<b && t[a+1].text==";") { a+=2; continue; }
            if (!bound(a,names) || a+1>=b) return false;
            auto rhs=a+2;
            if (t[a+1].text!="<=") {
                if (a+2>=b || t[a+1].text!="<" || t[a+2].text!="=" ||
                    t[a+2].offset!=t[a+1].offset+1) return false;
                rhs=a+3;
            }
            auto end=rhs;
            while (end<b && t[end].text!=";") ++end;
            if (end==b || (!literal(rhs,end) &&
                (constants || end!=rhs+1 || !bound(rhs,names)))) return false;
            assigned=true; a=end+1;
        }
        return !constants || assigned;
    }
    bool controlIf(const If& control,const Names& names,bool requireConstant,
                   std::vector<std::size_t>& equalities) const {
        for (const auto& br:control.branches) {
            if (br.conditionBegin!=br.conditionEnd) {
                if (!condition(br.conditionBegin,br.conditionEnd,names)) return false;
                std::size_t index{};
                if (eq(br.conditionBegin,br.conditionEnd,names,index)) equalities.push_back(index);
            }
            if (!assignments(br.bodyBegin,br.bodyEnd,names,requireConstant)) return false;
        }
        return true;
    }
    bool edgeBody(const Branch& br,const Names& names,bool classOne,
                  std::vector<std::size_t>& equalities) const {
        if (br.bodyBegin==br.bodyEnd) return false; // VHDL requires a statement (e.g. null).
        if (!word(br.bodyBegin,"if"))
            return assignments(br.bodyBegin,br.bodyEnd,names,false);
        If control;
        if (!readIf(br.bodyBegin,br.bodyEnd,control) || control.end!=br.bodyEnd) return false;
        if (!classOne) return controlIf(control,names,false,equalities);
        // 68c2c0: first BLOCK's direct IF, only its own first COND. ELSIF
        // isn't scanned here. Restrict to one constant then body (+else).
        if (control.branches.size()>2 ||
            (control.branches.size()==2 && control.branches[1].conditionBegin!=control.branches[1].conditionEnd)) return false;
        const auto& first=control.branches.front();
        if (!condition(first.conditionBegin,first.conditionEnd,names) ||
            !assignments(first.bodyBegin,first.bodyEnd,names,true)) return false;
        std::size_t index{};
        if (eq(first.conditionBegin,first.conditionEnd,names,index)) {
            // single lower name requirement: identifier=identifier with two
            // different names is NOT the reset-like single-name subset.
            Names used;
            for (auto i=first.conditionBegin;i<first.conditionEnd;++i)
                if (bound(i,names)) used.insert(lower(t[i].text));
            if (used.size()==1) equalities.push_back(index);
        }
        return control.branches.size()==1 || assignments(control.branches[1].bodyBegin,
            control.branches[1].bodyEnd,names,false);
    }
    void process(std::size_t p,std::size_t stop,std::size_t architecture,const Names& names,
                 const Bindings& bindings) {
        auto begin=p+1;
        if (begin<stop && t[begin].text=="(") {
            ++begin;
            while (begin<stop && t[begin].text!=")") {
                if (!bound(begin,names) && t[begin].text!="," && !word(begin,"all")) {
                    result.incomplete=true; return;
                }
                ++begin;
            }
            if (begin==stop) { result.incomplete=true; return; }
            ++begin;
        }
        if (word(begin,"is")) ++begin;
        if (!word(begin,"begin")) { result.incomplete=true; return; }
        ++begin;
        If outer;
        if (!readIf(begin,stop,outer) || outer.end!=stop) { result.incomplete=true; return; }
        VhdlClockControlModel model; model.architectureToken=architecture; model.processToken=p;
        const auto& first=outer.branches.front();
        model.edge=parseVhdlEdgeCondition(source,t,first.conditionBegin,first.conditionEnd);
        if (model.edge.status==VhdlEdgeConditionStatus::Matched && bound(model.edge.clockToken,names)) {
            // Flat class2 only: no outer ELSIF/ELSE. This avoids silently
            // accepting collector branch-map paths outside the known subset.
            if (outer.branches.size()!=1 || !edgeBody(first,names,false,model.equalityIdentifierTokens)) {
                result.incomplete=true; return;
            }
            model.nativeClassCandidate=2;
        } else {
            if (outer.branches.size()!=2 ||
                !condition(first.conditionBegin,first.conditionEnd,names) ||
                !assignments(first.bodyBegin,first.bodyEnd,names,true)) { result.incomplete=true; return; }
            const auto& next=outer.branches[1];
            model.edge=parseVhdlEdgeCondition(source,t,next.conditionBegin,next.conditionEnd);
            if (model.edge.status!=VhdlEdgeConditionStatus::Matched || !bound(model.edge.clockToken,names) ||
                !edgeBody(next,names,true,model.equalityIdentifierTokens)) { result.incomplete=true; return; }
            std::size_t index{};
            if (eq(first.conditionBegin,first.conditionEnd,names,index)) model.equalityIdentifierTokens.push_back(index);
            model.nativeClassCandidate=1;
        }
        const auto clockOwner=bindings.find(model.edge.clockName);
        if (clockOwner==bindings.end()) { result.incomplete=true; return; }
        model.clockBindingOwner=clockOwner->second;
        for (const auto token:model.equalityIdentifierTokens) {
            const auto owner=bindings.find(lower(t[token].text));
            if (owner==bindings.end()) { result.incomplete=true; return; }
            model.equalityBindingOwners.push_back(owner->second);
        }
        result.models.push_back(std::move(model));
    }
    void declaredNames(std::size_t a,std::size_t b,Names& names,bool ports) const {
        for (auto i=a;i<b;++i) {
            const bool signal=ports || word(i,"signal");
            if (!ports && !signal && !word(i,"constant") && !word(i,"file") &&
                !word(i,"variable")) continue;
            auto start=ports?i:i+1;
            if (start>=b || !basic(t[start])) continue;
            auto colon=start+1;
            while (colon<b && t[colon].text==",") colon+=2;
            if (colon>=b || t[colon].text!=":") continue;
            // Any declaration hides a visible object, even if its new type
            // is outside this bounded IEEE scalar model.
            for (auto n=start;n<colon;n+=2)
                if (basic(t[n])) names.erase(lower(t[n].text));
            auto type=colon+1;
            if (ports && (word(type,"in")||word(type,"out")||word(type,"inout")||word(type,"buffer"))) ++type;
            if (!signal || type>=b || !scalar(type)) continue;
            auto tail=type+1;
            if (word(tail,"register") || word(tail,"bus")) ++tail;
            const bool initializer=tail+1<b && t[tail].text==":" && t[tail+1].text=="=" &&
                t[tail+1].offset==t[tail].offset+1;
            if (tail<b && t[tail].text!=";" && t[tail].text!=")" && !initializer) continue;
            for (auto n=start;n<colon;n+=2) if (basic(t[n])) names.insert(lower(t[n].text));
        }
    }

public:
    Scanner(const core::SourceFile& s,const std::vector<parser::HdlToken>& tokens):source(s),t(tokens){}
    VhdlClockControlScan run() {
        if (source.language()!=core::HdlLanguage::Vhdl) { result.incomplete=true; return result; }
        std::map<std::string,Names> entities;
        std::set<std::string> ieeeEntities;
        std::size_t context=0;
        for (std::size_t i=0;i<t.size();++i) {
            if (word(i,"entity") && i+2<t.size() && basic(t[i+1]) && word(i+2,"is")) {
                const auto end=unitEnd(i+3,"entity",lower(t[i+1].text));
                if (end==t.size()) { result.incomplete=true; break; }
                Names names;
                bool shadow=false;
                for (auto j=i+3;j<end;++j)
                    if (word(j,"function")||word(j,"procedure")||word(j,"alias")||word(j,"type")||
                        word(j,"subtype")||word(j,"package")||word(j,"use")) shadow=true;
                const auto entityIeee=ieee(context,i);
                if (entityIeee && !shadow) {
                    for (auto p=i+3;p<end;++p) if (word(p,"port") && p+1<end && t[p+1].text=="(") {
                        auto stop=p+2;
                        unsigned depth=1;
                        for (;stop<end;++stop) {
                            if (t[stop].text=="(") ++depth;
                            else if (t[stop].text==")" && --depth==0) break;
                        }
                        if (stop==end) { result.incomplete=true; break; }
                        declaredNames(p+2,stop,names,true); break;
                    }
                }
                const auto entityName=lower(t[i+1].text);
                entities[entityName]=std::move(names);
                if (entityIeee && !shadow) ieeeEntities.insert(entityName);
                auto semi=end+1; while (semi<t.size()&&t[semi].text!=";") ++semi;
                i=semi; context=std::min(i+1,t.size());
            } else if (word(i,"architecture") && i+4<t.size() && basic(t[i+1]) &&
                       word(i+2,"of") && basic(t[i+3]) && word(i+4,"is")) {
                const auto arch=i;
                const auto end=unitEnd(i+5,"architecture",lower(t[i+1].text));
                if (end==t.size()) { result.incomplete=true; break; }
                const auto entity=entities.find(lower(t[i+3].text));
                bool hasInterveningUse=false;
                for (auto j=context;j<i;++j) if (word(j,"use")) hasInterveningUse=true;
                const auto entityName=lower(t[i+3].text);
                const bool boundIeee=ieee(context,i) ||
                    (!hasInterveningUse && ieeeEntities.contains(entityName));
                if (!boundIeee || entity==entities.end()) {
                    result.incomplete=true;
                } else {
                    auto names=entity->second;
                    Bindings bindings;
                    for (const auto& name:names) bindings[name]=arch;
                    // Consume declarations in visibility order. Searching for the
                    // first BEGIN would mistake a subprogram body for this unit.
                    ScalarTypes types{"std_logic","std_ulogic"};
                    const auto begin=declarationBody(i+5,end,names,bindings,types,true);
                    if (begin) concurrent(*begin,end,arch,names,bindings,types);
                    else result.incomplete=true;
                }
                auto semi=end+1; while (semi<t.size()&&t[semi].text!=";") ++semi;
                i=semi; context=std::min(i+1,t.size());
            }
        }
        for (const auto& clock:result.models) for (const auto& control:result.models) {
            if (clock.architectureToken!=control.architectureToken) continue;
            for (std::size_t n=0;n<control.equalityIdentifierTokens.size();++n) {
                const auto index=control.equalityIdentifierTokens[n];
                if (lower(t[index].text)==clock.edge.clockName) {
                    if (clock.clockBindingOwner!=control.equalityBindingOwners[n]) {
                        result.incomplete=true; // Distinct local objects: native qualification unresolved.
                        continue;
                    }
                    // Native 68aaf0 appends every matching clock/control pair.
                    // Repeated nodes remain distinct, including equal locations.
                    result.overlaps.push_back({clock.architectureToken,clock.edge.clockToken,index,clock.edge.clockName});
                }
            }
        }
        if (result.models.empty()) result.incomplete=true;
        return std::move(result);
    }
};
} // namespace vhdl_clock_detail

// Native run ev_8db32876e0c5e664daa8d39b05e03aafd03e235a87160ab608419944718e1add;
// classifier assembly ev_78ded7eaead89d9cd328278a0fc9eba39dcbc0365a11f5e2ba70c9a45bed2898.
// Actual bounded source parsing and declaration binding; no metadata-only claim.
// Source constraints remain explicit IEEE scalar binding and architecture
// isolation. Local owners are tracked; unverified cross-owner qualification
// and unsupported syntax stay incomplete/ManualReview.
[[nodiscard]] inline VhdlClockControlScan scanVhdlClockControl(
    const core::SourceFile& source,const std::vector<parser::HdlToken>& tokens) {
    return vhdl_clock_detail::Scanner(source,tokens).run();
}
} // namespace fpga::analysis
