#include "parser/frontend_preprocessor.h"
#include "parser/design_parser.h"
#include "parser/vendor/yosys_preproc.h"
#include <cerrno>
#include <climits>
#include <cstdlib>
#include <mutex>
#include <regex>
#include <set>
#include <sstream>

namespace fpga::parser {
namespace {
std::string utf8(const std::filesystem::path& path) {
    const auto value=path.generic_u8string();
    return {reinterpret_cast<const char*>(value.data()),value.size()};
}
bool identifier(std::string_view value) {
    const auto letter=[](char c){return (c>='a'&&c<='z')||(c>='A'&&c<='Z')||c=='_';};
    if (value.empty() || !letter(value.front())) return false;
    return std::all_of(value.begin(),value.end(),[&](char c){return letter(c)||(c>='0'&&c<='9')||c=='$';});
}
std::string failureText(std::string_view text) {
    const auto between=[&](std::string_view start,std::string_view end) {
        const auto a=text.find(start);
        if(a==std::string_view::npos) return std::string{};
        const auto begin=a+start.size(), stop=text.find(end,begin);
        return std::string(text.substr(begin,stop==std::string_view::npos?text.size()-begin:stop-begin));
    };
    if(text.starts_with("Duplicate macro arguments")) return "宏形参重复："+between("`","'");
    if(text.starts_with("Cannot expand macro")) return "宏的实参数量不足，且缺少默认值："+between("`"," by");
    if(text.starts_with("Expected to find '('")) return "函数宏缺少实参左括号："+between("for '","'");
    if(text.starts_with("Mismatched brackets")) return "宏实参中的括号不匹配。";
    if(text.starts_with("Backslash in macro arguments")) return "宏实参中的反斜杠必须位于行末。";
    if(text.starts_with("Trailing contents")) return "宏形参后存在无效内容，应使用等号、逗号或右括号。";
    if(text.starts_with("Invalid name for macro definition")) return "宏定义名称无效："+between(">>","<<");
    if(text.starts_with("Invalid configured macro name")) return "配置中的宏名称无效。";
    if(text.starts_with("Found ")) return "条件编译指令缺少对应起点："+between("Found "," outside");
    if(text.starts_with("Unterminated preprocessor conditional")) return "条件编译块未闭合。";
    if(text.find("operation limit")!=std::string_view::npos) return "宏展开超过操作次数限制，可能存在循环展开。";
    if(text.find("expanded output size")!=std::string_view::npos) return "宏展开结果超过大小限制。";
    if(text.find("input size")!=std::string_view::npos) return "源文件及包含内容超过输入大小限制。";
    if(text.find("include depth")!=std::string_view::npos) return "包含文件嵌套过深，可能存在循环包含。";
    if(text.find("restoration depth")!=std::string_view::npos) return "函数宏递归展开或嵌套过深。";
    if(text.starts_with("NUL byte")) return "HDL 输入含有空字节，无法确认解析结果。";
    if(text.starts_with("File name cannot")) return "文件路径含有当前来源标记无法表示的引号或换行。";
    if(text.starts_with("Unexpected file_pop")) return "文件来源栈缺少可恢复的父文件。";
    return "宏或包含文件的语法、读取过程或展开状态无法可靠确认。";
}
struct Marker {
    enum Kind { Push,Pop,Line,Blank } kind;
    std::size_t begin{},end{};
    std::string file;
    std::size_t line{};
};
void mapExpanded(FrontendPreprocessorResult& result,std::string text) {
    const core::SourceFile raw(result.source.path(),result.source.language(),text);
    const auto tokens=tokenize(raw);
    std::vector<Marker> markers;
    const auto issue=[&](std::string id,std::string message,std::size_t line) {
        result.issues.push_back({std::move(id),std::move(message),{raw.path(),line,1,0}});
    };
    // This lexical pass excludes marker-looking text inside strings/comments.
    // Native lexer consumes file_push up to, but not including, its LF;
    // file_pop and line consume their LF before restoring/setting the line.
    for (std::size_t i=0;i<tokens.size();++i) {
        if (tokens[i].text!="`") continue;
        const auto start=tokens[i].offset;
        if (i+1==tokens.size()) { issue("PP-FRONTEND-DIRECTIVE","无法解析展开后残留的反引号。",tokens[i].line); break; }
        const auto& name=tokens[i+1].text;
        auto end=text.find('\n',start);if(end==std::string::npos) end=text.size();
        if (name=="file_push") {
            if (text.compare(start,11,"`file_push ")!=0 || i+2>=tokens.size() ||
                tokens[i+2].kind!=HdlTokenKind::String || tokens[i+2].line!=tokens[i].line) {
                issue("PP-FRONTEND-DIRECTIVE","文件来源栈标记不完整。",tokens[i].line);break;
            }
            auto path=tokens[i+2].text;
            if(path.size()<2 || path.front()!='"'||path.back()!='"') {
                issue("PP-FRONTEND-DIRECTIVE","文件来源栈路径不完整。",tokens[i].line);break;
            }
            markers.push_back({Marker::Push,start,end,path.substr(1,path.size()-2),0});
        } else if(name=="file_pop") {
            markers.push_back({Marker::Pop,start,end+(end<text.size()),{},0});
        } else if(name=="line") {
            static const std::regex pattern(R"pp(^`line[ \t]+([^ \t\r\n]+)[ \t]+"([^ \r\n]+)"[^\r\n]*\n)pp");
            std::match_results<std::string::const_iterator> match;
            if(!std::regex_search(text.cbegin()+static_cast<std::ptrdiff_t>(start),text.cend(),match,pattern)) {
                issue("PP-FRONTEND-DIRECTIVE","无法按原生词法规则解释 line 指令。",tokens[i].line);break;
            }
            errno=0;const auto number=std::strtol(match[1].str().c_str(),nullptr,10);
            if(errno==ERANGE || number<0 || number>INT_MAX) {
                issue("PP-FRONTEND-DIRECTIVE","line 指令的行号超出已确认范围。",tokens[i].line);break;
            }
            markers.push_back({Marker::Line,start,start+static_cast<std::size_t>(match.length()),match[2].str(),static_cast<std::size_t>(number)});
        } else if(name=="celldefine"||name=="endcelldefine"||name=="protect"||name=="endprotect") {
            markers.push_back({Marker::Blank,start,end,{},0});
        } else if(name=="default_nettype") {
            if(i+2>=tokens.size() || tokens[i+2].line!=tokens[i].line ||
               (tokens[i+2].text!="wire"&&tokens[i+2].text!="none")) {
                issue("PP-FRONTEND-DIRECTIVE","不支持或不完整的 default_nettype 指令。",tokens[i].line);break;
            }
            markers.push_back({Marker::Blank,start,tokens[i+2].offset+tokens[i+2].text.size(),{},0});
        } else if(name=="file_notfound") {
            issue("PP-INCLUDE-NOT-FOUND","展开时找不到包含文件："+text.substr(tokens[i+1].offset+name.size(),end-tokens[i+1].offset-name.size()),tokens[i].line);
            markers.push_back({Marker::Blank,start,end,{},0});
        } else {
            issue("PP-FRONTEND-DIRECTIVE","存在未定义宏或未支持的编译指令：`"+name,tokens[i].line);
            markers.push_back({Marker::Blank,start,end,{},0});
        }
        while(i+1<tokens.size() && tokens[i+1].offset<markers.back().end) ++i;
    }
    FrontendLineOrigin current{raw.path(),1};
    std::vector<FrontendLineOrigin> stack;
    result.origins.push_back(current);
    auto physical=current;
    std::vector<FrontendLineOrigin> physicalOrigins{physical},physicalStack;
    const auto newline=[&] {
        ++current.line;result.origins.push_back(current);
        ++physical.line;physicalOrigins.push_back(physical);
    };
    std::size_t marker=0, virtualLine=0;
    for(std::size_t pos=0;pos<text.size();) {
        if(marker<markers.size() && pos==markers[marker].begin) {
            const auto& item=markers[marker++];
            if(item.kind==Marker::Push) {
                stack.push_back(current);current={vendorpp::path_from_utf8(item.file),0};
                physicalStack.push_back(physical);physical={vendorpp::path_from_utf8(item.file),0};
            }
            for(;pos<item.end;++pos) {
                if(text[pos]=='\n') { newline();++virtualLine; }
                else if(text[pos]!='\r') text[pos]=' ';
            }
            if(item.kind==Marker::Pop) {
                if(stack.empty()) { issue("PP-FRONTEND-DIRECTIVE","展开后的文件来源栈无法恢复。",virtualLine+1);break; }
                current=stack.back();stack.pop_back();result.origins.back()=current;
                physical=physicalStack.back();physicalStack.pop_back();physicalOrigins.back()=physical;
            } else if(item.kind==Marker::Line) {
                current={vendorpp::path_from_utf8(item.file),item.line};result.origins.back()=current;
            }
            continue;
        }
        if(text[pos++]=='\n') { newline();++virtualLine; }
    }
    if(!stack.empty()) issue("PP-FRONTEND-DIRECTIVE","展开后的文件来源栈未闭合。",virtualLine+1);
    // Resolve issues recorded at virtual output lines after building the map.
    for(auto& entry:result.issues) {
        if(entry.location.line>0 && entry.location.line<=result.origins.size()) {
            const auto& at=result.origins[entry.location.line-1];entry.location.file=at.file;entry.location.line=at.line;
        }
    }
    std::vector<std::size_t> lineStarts{0};
    for(std::size_t pos=0;pos<text.size();++pos) if(text[pos]=='\n') lineStarts.push_back(pos+1);
    for(const auto& event:vendorpp::budget.conditionalEvents) {
        const auto line=static_cast<std::size_t>(std::upper_bound(lineStarts.begin(),lineStarts.end(),event.outputOffset)-lineStarts.begin()-1);
        if(line<physicalOrigins.size())
            result.conditionalDecisions.push_back({physicalOrigins[line],event.directive,event.active});
    }
    result.source=core::SourceFile(raw.path(),raw.language(),std::move(text));
}
} // namespace
struct FrontendPreprocessorSession::State {
    vendorpp::define_map_t cache;
    State() { cache.clear(); } // Match the explicit public clear boundary.
};
namespace {
std::mutex& engineMutex() {
    static std::mutex mutex;
    return mutex;
}
}
FrontendPreprocessorSession::FrontendPreprocessorSession():state_(std::make_unique<State>()) {}
FrontendPreprocessorSession::~FrontendPreprocessorSession()=default;
void FrontendPreprocessorSession::reset() {
    std::lock_guard guard(engineMutex());
    state_->cache.clear();
}
core::SourceLocation FrontendPreprocessorResult::origin(std::size_t virtualLine) const {
    if(virtualLine==0 || virtualLine>origins.size()) return {source.path(),0,1,0};
    const auto& at=origins[virtualLine-1];return {at.file,at.line,1,0};
}
FrontendPreprocessorResult FrontendPreprocessorSession::expand(
    const core::SourceFile& source,const PreprocessorOptions& options,const FrontendPreprocessorLimits& limits) {
    FrontendPreprocessorResult result{source,{},{},{},{},{}};
    if(source.language()==core::HdlLanguage::Vhdl || source.language()==core::HdlLanguage::Unknown) {
        result.issues.push_back({"PP-FRONTEND-LANGUAGE","该展开器仅接受 Verilog/SystemVerilog。",{source.path(),0,1,0}});return result;
    }
    // The retained upstream engine has static token queues. Its complete call
    // and cleanup run under this lock; independent analyzer threads never mix.
    std::lock_guard guard(engineMutex());
    vendorpp::budget={};
    vendorpp::budget.maxOperations=limits.maxOperations;
    vendorpp::budget.maxOutputBytes=limits.maxOutputBytes;
    vendorpp::budget.maxInputBytes=limits.maxInputBytes;
    vendorpp::budget.maxIncludeDepth=limits.maxIncludeDepth;
    vendorpp::budget.maxMacroRestoreDepth=limits.maxMacroRestoreDepth;
    vendorpp::budget.current_file=utf8(source.path());
    vendorpp::yosys_input_files.clear();
    try {
        vendorpp::define_map_t configured;
        for(const auto& definition:options.defines) {
            const auto equal=definition.find('=');const auto name=definition.substr(0,equal);
            if(!identifier(name)) throw vendorpp::Failure("Invalid configured macro name.",utf8(source.path()));
            configured.add(name,equal==std::string::npos?"1":definition.substr(equal+1));
        }
        // Default read_hdl emits read_verilog -sv without -nosynthesis or
        // -formal. The native frontend adds this after command-line defines.
        configured.add("SYNTHESIS","1");
        std::set<std::string> initialNames;
        for(const auto& entry:configured.defines) initialNames.insert(entry.first);
        for(const auto& entry:state_->cache.defines) initialNames.insert(entry.first);
        result.initialMacroNames.assign(initialNames.begin(),initialNames.end());
        std::list<std::string> directories;
        for(const auto& path:options.includeDirectories) directories.push_back(utf8(path));
        std::istringstream input(source.content());
        auto expanded=vendorpp::frontend_verilog_preproc(input,utf8(source.path()),configured,state_->cache,directories);
        for(const auto& file:vendorpp::yosys_input_files) result.includedFiles.push_back(vendorpp::path_from_utf8(file));
        mapExpanded(result,std::move(expanded));
    } catch(const vendorpp::Failure& error) {
        result.issues.push_back({"PP-FRONTEND-EXPANSION","宏或包含文件展开失败："+failureText(error.what()),
            {vendorpp::path_from_utf8(error.file),0,1,0}});
    } catch(const std::exception&) {
        result.issues.push_back({"PP-FRONTEND-EXPANSION","文件读取或展开过程异常，请检查包含路径的编码、权限和宏定义。",{source.path(),0,1,0}});
    }
    vendorpp::yosys_input_files.clear();return result;
}
FrontendPreprocessorResult expandFrontendSource(const core::SourceFile& source,const PreprocessorOptions& options,
                                               const FrontendPreprocessorLimits& limits) {
    FrontendPreprocessorSession session;
    return session.expand(source,options,limits);
}
} // namespace fpga::parser
