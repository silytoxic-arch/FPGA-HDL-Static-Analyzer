#pragma once

#include "analysis/verilog_naming_declarations.h"
#include <optional>
#include <set>

namespace fpga::analysis {

enum class VerilogNamingShapeSubject { Module, Net, Parameter, Localparam, Cell, Task, Function, Unsupported };
struct VerilogNamingShapeResult {
    std::optional<bool> firstByteViolation;
    std::optional<bool> underscoreViolation;
};

// Native 5.2.1 ev_8ed10f75820818d3d20d08887706e8cd7659c8e91158a8f12d34d1be83f68c3b
// tests exactly ASCII digit or '_' in first byte, NOT "not a letter".
// Native 5.2.2 ev_32bd8b91ae9488d76d830d21919304bee359a91224d19bde6409118acd1fab75
// tests substring '__' first, otherwise trailing '_'; FUNCTION(4) absent from
// its six-element collector, unlike 5.2.1's seven-element collector.
// Input is the established normalized AST name, not an arbitrary HDL use.
[[nodiscard]] inline VerilogNamingShapeResult checkVerilogNamingShape(
    std::string_view name,VerilogNamingShapeSubject subject) {
    if (subject==VerilogNamingShapeSubject::Unsupported) return {};
    const bool first=!name.empty() && ((name.front()>='0'&&name.front()<='9')||name.front()=='_');
    std::optional<bool> underscores;
    if (subject!=VerilogNamingShapeSubject::Function)
        underscores=!name.empty() && (name.find("__")!=std::string_view::npos || name.back()=='_');
    return {first,underscores};
}
struct VerilogNamingShapeDeclaration {
    std::size_t tokenIndex{};
    VerilogNamingShapeSubject subject{VerilogNamingShapeSubject::Unsupported};
    std::string normalizedName;
    std::string moduleName;
    VerilogNamingShapeResult predicates;
};
struct VerilogNamingShapeScan {
    std::vector<VerilogNamingShapeDeclaration> declarations;
    bool incomplete{false};
    bool partial{true}; // No full vendor frontend equivalence / negative compliance claim.
};

namespace verilog_shape_detail {
inline bool type(std::string_view s) {
    return s=="automatic"||s=="static"||s=="signed"||s=="unsigned"||s=="reg"||s=="wire"||
        s=="logic"||s=="bit"||s=="integer"||s=="int"||s=="byte"||s=="shortint"||s=="longint"||
        s=="time"||s=="real"||s=="realtime"||s=="shortreal"||s=="void";
}
inline bool simple(const parser::HdlToken& t) {
    if (t.kind!=parser::HdlTokenKind::Identifier||t.text.empty()) return false;
    const auto letter=[](char c){return (c>='A'&&c<='Z')||(c>='a'&&c<='z');};
    if (!letter(t.text.front()) && t.text.front()!='_') return false;
    for(char c:t.text) if(!letter(c)&&!(c>='0'&&c<='9')&&c!='_'&&c!='$') return false;
    return true;
}
// The lexer keeps a malformed name such as 1_B1 in one Number token. Admit it
// only at an unambiguous declaration-name position; literals remain excluded.
inline bool numericUnderscoreName(const parser::HdlToken& t) {
    if (t.kind!=parser::HdlTokenKind::Number || t.text.size()<3 ||
        t.text.front()<'0' || t.text.front()>'9' || t.text.find('_')==std::string::npos) return false;
    for (char c:t.text)
        if (!((c>='A'&&c<='Z')||(c>='a'&&c<='z')||(c>='0'&&c<='9')||c=='_'||c=='$')) return false;
    return true;
}
}

[[nodiscard]] inline VerilogNamingShapeScan scanVerilogNamingShape(
    const core::SourceFile& source,const std::vector<parser::HdlToken>& tokens) {
    VerilogNamingShapeScan out;
    if (source.language()!=core::HdlLanguage::Verilog && source.language()!=core::HdlLanguage::SystemVerilog) {
        out.incomplete=true; return out;
    }
    const auto escaped=[&](std::size_t i){return i<tokens.size()&&tokens[i].offset<source.content().size()&&
        source.content()[tokens[i].offset]=='\\';};
    const auto word=[&](std::size_t i,std::string_view w){return i<tokens.size()&&!escaped(i)&&
        tokens[i].kind==parser::HdlTokenKind::Identifier&&tokens[i].text==w;};
    const auto add=[&](std::size_t i,VerilogNamingShapeSubject subject,
                       std::string_view moduleName={},bool malformedNumeric=false){
        if (i>=tokens.size()||escaped(i)||
            !(verilog_shape_detail::simple(tokens[i]) ||
              (malformedNumeric && verilog_shape_detail::numericUnderscoreName(tokens[i])))) {
            out.incomplete=true;return;
        }
        for(const auto& decl:out.declarations) if(decl.tokenIndex==i) return;
        out.declarations.push_back({i,subject,tokens[i].text,std::string(moduleName),
                                    checkVerilogNamingShape(tokens[i].text,subject)});
    };
    // Existing declaration grammar reused; opt-in subprogram descendants is
    // necessary for these native collectors, without changing 5.2.15 defaults.
    for(const auto& declaration:VerilogNamingDeclarations(source,tokens,true).collect()) {
        const auto subject=declaration.subject==VerilogNamingSubject::Parameter?VerilogNamingShapeSubject::Parameter:
            declaration.subject==VerilogNamingSubject::Localparam?VerilogNamingShapeSubject::Localparam:VerilogNamingShapeSubject::Net;
        add(declaration.tokenIndex,subject,declaration.moduleName);
    }
    bool module=false;
    std::string currentModuleName;
    std::size_t conditional=0;
    std::vector<std::string> excluded;
    std::set<std::string> moduleNames;
    // First collect known module types for bounded CELL declaration recognition.
    for(std::size_t i=0;i+1<tokens.size();++i)
        if(word(i,"module")) {
            auto name=i+1;
            if(word(name,"automatic")||word(name,"static")) ++name;
            if(name<tokens.size()&&verilog_shape_detail::simple(tokens[name])) moduleNames.insert(tokens[name].text);
        }
    for(std::size_t i=0;i<tokens.size();++i) {
        if(tokens[i].text=="`") {
            out.incomplete=true;
            if(word(i+1,"ifdef")||word(i+1,"ifndef")) ++conditional;
            else if(word(i+1,"endif")&&conditional) --conditional;
            auto line=tokens[i].line;
            while(i+1<tokens.size()) {
                if(tokens[i+1].line>line) {
                    const auto offset=tokens[i].offset;
                    const auto newline=source.content().find('\n',offset);
                    auto end=newline==std::string::npos?source.content().size():newline;
                    while(end>offset&&(source.content()[end-1]==' '||source.content()[end-1]=='\t'||source.content()[end-1]=='\r')) --end;
                    if(!end||source.content()[end-1]!='\\') break;
                    line=tokens[i+1].line;
                }
                ++i;
            }
            continue;
        }
        if(conditional) continue;
        if(!excluded.empty()) {if(word(i,excluded.back())) excluded.pop_back();continue;}
        if(word(i,"module")) {
            module=true;
            auto name=i+1;
            if(word(name,"automatic")||word(name,"static")) ++name;
            currentModuleName = name < tokens.size() ? tokens[name].text : std::string{};
            add(name,VerilogNamingShapeSubject::Module,currentModuleName); i=name; continue;
        }
        if(word(i,"endmodule")) {module=false;currentModuleName.clear();continue;}
        if(!module) continue;
        if(word(i,"class")||word(i,"interface")||word(i,"package")||word(i,"program")) {
            out.incomplete=true;
            excluded.push_back(word(i,"class")?"endclass":word(i,"interface")?"endinterface":word(i,"package")?"endpackage":"endprogram");
            continue;
        }
        if(word(i,"typedef")||word(i,"struct")||word(i,"union")||word(i,"enum")) out.incomplete=true;
        if(word(i,"wire")||word(i,"reg")||word(i,"logic")||word(i,"input")||word(i,"output")||
           word(i,"inout")||word(i,"parameter")||word(i,"localparam")) {
            auto candidate=i+1;
            while(candidate<tokens.size() && !escaped(candidate) && verilog_shape_detail::type(tokens[candidate].text)) ++candidate;
            while(candidate<tokens.size() && tokens[candidate].text=="[") {
                unsigned depth=1;
                for(++candidate;candidate<tokens.size()&&depth;++candidate) {
                    if(tokens[candidate].text=="[") ++depth;
                    else if(tokens[candidate].text=="]") --depth;
                }
                if(depth) out.incomplete=true;
            }
            // Narrow recovery for a malformed port/net declaration. A bare
            // number or based literal is never promoted into an AST name.
            if(candidate+1<tokens.size() && verilog_shape_detail::numericUnderscoreName(tokens[candidate]) &&
                (tokens[candidate+1].text=="," || tokens[candidate+1].text==")" ||
                 tokens[candidate+1].text==";" || tokens[candidate+1].text=="=")) {
                const auto subject=word(i,"parameter")?VerilogNamingShapeSubject::Parameter:
                    word(i,"localparam")?VerilogNamingShapeSubject::Localparam:VerilogNamingShapeSubject::Net;
                add(candidate,subject,currentModuleName,true);
            }
            if(candidate<tokens.size() && tokens[candidate].kind==parser::HdlTokenKind::Identifier &&
                std::none_of(out.declarations.begin(),out.declarations.end(),[&](const auto& d){return d.tokenIndex==candidate;}))
                out.incomplete=true;
        }
        if(word(i,"function")||word(i,"task")) {
            if(i>0&&(word(i-1,"extern")||word(i-1,"pure")||word(i-1,"virtual"))) {out.incomplete=true;continue;}
            const auto subject=word(i,"function")?VerilogNamingShapeSubject::Function:VerilogNamingShapeSubject::Task;
            auto name=i+1;
            while(name<tokens.size()) {
                if(!escaped(name)&&verilog_shape_detail::type(tokens[name].text)) {++name;continue;}
                if(tokens[name].text=="[") {
                    unsigned depth=1;
                    for(++name;name<tokens.size()&&depth;++name) {
                        if(tokens[name].text=="[") ++depth;
                        else if(tokens[name].text=="]") --depth;
                    }
                    if(depth) out.incomplete=true;
                    continue;
                }
                break;
            }
            if(name+1<tokens.size()&&(tokens[name+1].text=="("||tokens[name+1].text==";"))
                add(name,subject,currentModuleName);
            else out.incomplete=true;
            continue;
        }
        // Bounded module instantiation: known same-source module type, no
        // parameter overrides/arrays. Never infer declarations from function
        // calls, hierarchy references, named connections or arbitrary uses.
        if(tokens[i].kind==parser::HdlTokenKind::Identifier && moduleNames.contains(tokens[i].text) &&
            i+1<tokens.size() && tokens[i+1].text=="#") out.incomplete=true;
        if(tokens[i].kind==parser::HdlTokenKind::Identifier && moduleNames.contains(tokens[i].text) &&
            i+2<tokens.size() && tokens[i+2].text=="(" &&
            verilog_shape_detail::simple(tokens[i+1])) {
            auto stop=i+3; unsigned depth=1;
            for(;stop<tokens.size()&&depth;++stop) {
                if(tokens[stop].text=="(") ++depth;
                else if(tokens[stop].text==")") --depth;
            }
            if(!depth&&stop<tokens.size()&&tokens[stop].text==";") {
                add(i+1,VerilogNamingShapeSubject::Cell,currentModuleName); i=stop;
            } else out.incomplete=true;
        }
        if(escaped(i)) out.incomplete=true;
    }
    std::sort(out.declarations.begin(),out.declarations.end(),[](const auto& a,const auto& b){return a.tokenIndex<b.tokenIndex;});
    return out;
}
} // namespace fpga::analysis
