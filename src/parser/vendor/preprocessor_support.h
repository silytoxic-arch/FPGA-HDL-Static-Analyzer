#pragma once
#include "core/source_text.h"
// Standalone host bridge for the ISC-licensed Yosys preprocessor. No RTLIL,
// target DLL, vendor runtime or global application state is linked here.
#include <algorithm>
#include <cstdarg>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <istream>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <stack>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace fpga::parser::vendorpp {
struct Failure : std::runtime_error {
    std::string file;
    explicit Failure(std::string text,std::string source):std::runtime_error(std::move(text)),file(std::move(source)){}
};
struct Budget {
    struct ConditionalEvent { std::size_t outputOffset; std::string directive; bool active; };
    std::size_t operations{}, inputBytes{}, outputBytes{}, maxOperations{256*1024*1024},
        maxOutputBytes{64*1024*1024}, maxInputBytes{64*1024*1024}, maxIncludeDepth{128}, maxMacroRestoreDepth{256};
    std::string current_file;
    std::vector<ConditionalEvent> conditionalEvents;
    std::map<std::string,std::string> inputSnapshots;
    std::set<std::string> unstableSnapshots;
};
inline thread_local Budget budget;
inline thread_local std::set<std::string> yosys_input_files;
inline std::string format(const char* pattern,va_list arguments) {
    va_list copy; va_copy(copy,arguments);
    const int count=std::vsnprintf(nullptr,0,pattern,copy); va_end(copy);
    if (count<0 || count>1024*1024) return "Preprocessor diagnostic could not be formatted.";
    std::vector<char> result(static_cast<std::size_t>(count)+1);
    std::vsnprintf(result.data(),result.size(),pattern,arguments);
    return std::string(result.data(),static_cast<std::size_t>(count));
}
inline std::string stringf(const char* pattern,...) {
    va_list args;va_start(args,pattern);auto text=format(pattern,args);va_end(args);return text;
}
[[noreturn]] inline void log_error(const char* pattern,...) {
    va_list args;va_start(args,pattern);auto text=format(pattern,args);va_end(args);
    throw Failure(std::move(text),budget.current_file);
}
[[noreturn]] inline void log_file_error(const std::string& file,int,const char* pattern,...) {
    va_list args;va_start(args,pattern);auto text=format(pattern,args);va_end(args);
    throw Failure(std::move(text),file);
}
inline void log(const char*,...) {} // Definition-list debug output is not used by this bridge.
template<class T> inline int GetSize(const T& value) { return static_cast<int>(value.size()); }
inline int readsome(std::istream& stream,char* data,int size) {
    stream.read(data,size);return static_cast<int>(stream.gcount());
}
inline void check_operation_budget() {
    if (++budget.operations>budget.maxOperations) log_error("Preprocessor expansion operation limit exceeded.");
}
inline void check_output_size(std::size_t current,std::size_t added) {
    if (added>budget.maxOutputBytes || current>budget.maxOutputBytes-added)
        log_error("Preprocessor expanded output size limit exceeded.");
}
inline void check_input_file(const std::string& name) {
    if (name.find_first_of("\n\r\"")!=std::string::npos) log_error("File name cannot be represented by the frontend file-stack protocol.");
}
inline void check_input_bytes(std::size_t bytes,const char* data) {
    if (std::memchr(data,0,bytes)) log_error("NUL byte in HDL input.");
    if (bytes>budget.maxInputBytes || budget.inputBytes>budget.maxInputBytes-bytes)
        log_error("Preprocessor input size limit exceeded.");
    budget.inputBytes+=bytes;
}
inline void check_include_depth(std::size_t depth) {
    if (depth>budget.maxIncludeDepth) log_error("Preprocessor include depth limit exceeded.");
}
inline void check_macro_depth(std::size_t depth) {
    if (depth>budget.maxMacroRestoreDepth) log_error("Preprocessor macro argument restoration depth limit exceeded.");
}
inline std::filesystem::path path_from_utf8(const std::string& value) {
    return std::filesystem::path(std::u8string(value.begin(),value.end()));
}
inline void open_native_file(std::ifstream& stream,const std::string& filename) {
    const auto path=path_from_utf8(filename);
    std::error_code error;
    if (!std::filesystem::is_regular_file(path,error)) { stream.setstate(std::ios::failbit);return; }
    stream.open(path); // Match the retained engine's host text-mode include stream.
}
} // namespace fpga::parser::vendorpp
#define log_assert(test) do { if (!(test)) ::fpga::parser::vendorpp::log_error("Preprocessor internal invariant: " #test); } while(false)
