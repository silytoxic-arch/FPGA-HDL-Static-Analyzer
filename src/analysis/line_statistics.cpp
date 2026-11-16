#include "analysis/line_statistics.h"
#include <algorithm>
#include <cctype>

namespace fpga::analysis {
LineCounts& LineCounts::operator+=(const LineCounts& other) {
    total += other.total; code += other.code; comment += other.comment; blank += other.blank;
    return *this;
}
void LineStatistics::append(const FileLineStatistics& file) {
    files.push_back(file);
    total += file.lines;
    auto found = std::find_if(languages.begin(), languages.end(), [&](const auto& entry) {
        return entry.language == file.language;
    });
    if (found == languages.end()) {
        languages.push_back({file.language, 1, file.lines});
    } else {
        ++found->files;
        found->lines += file.lines;
    }
    std::sort(languages.begin(), languages.end(), [](const auto& a, const auto& b) {
        return a.language < b.language;
    });
}
std::string_view lineStatisticsLanguageName(core::HdlLanguage language) {
    switch (language) {
    case core::HdlLanguage::Verilog: return "Verilog";
    case core::HdlLanguage::SystemVerilog: return "SystemVerilog";
    case core::HdlLanguage::Vhdl: return "VHDL";
    case core::HdlLanguage::Unknown: return "未知";
    }
    return "未知";
}
FileLineStatistics countSourceLines(const core::SourceFile& source) {
    FileLineStatistics result{source.path(), source.language(), {}};
    const auto& text = source.content();
    const bool vhdl = source.language() == core::HdlLanguage::Vhdl;
    bool blockComment = false;
    bool string = false;
    for (std::size_t start = 0; start < text.size();) {
        auto end = text.find('\n', start);
        if (end == std::string::npos) end = text.size();
        bool code = false;
        bool comment = blockComment;
        for (auto i = start; i < end; ++i) {
            if (i == 0 && text.size() >= 3 && text.compare(0, 3, "\xEF\xBB\xBF") == 0) {
                i = 2;
                continue;
            }
            const char c = text[i];
            const char next = i + 1 < end ? text[i + 1] : '\0';
            if (blockComment) {
                comment = true;
                if (c == '*' && next == '/') { blockComment = false; ++i; }
            } else if (string) {
                code = true;
                if (!vhdl && c == '\\' && i + 1 < end) ++i;
                else if (c == '"') {
                    if (vhdl && next == '"') ++i;
                    else string = false;
                }
            } else if ((!vhdl && c == '/' && next == '/') ||
                       (vhdl && c == '-' && next == '-')) {
                comment = true;
                break;
            } else if (c == '/' && next == '*') {
                // VHDL-2008 also supports delimited comments.
                comment = true; blockComment = true; ++i;
            } else if (vhdl && c == '\'' && i + 2 < end && text[i + 2] == '\'') {
                code = true;
                i += 2;
            } else if (c == '"') {
                code = true; string = true;
            } else if (!vhdl && c == '\\') {
                // Escaped HDL identifiers may contain comment delimiters.
                code = true;
                while (i + 1 < end && !std::isspace(static_cast<unsigned char>(text[i + 1]))) ++i;
            } else if (!std::isspace(static_cast<unsigned char>(c))) {
                code = true;
            }
        }
        ++result.lines.total;
        if (code) ++result.lines.code;
        else if (comment) ++result.lines.comment;
        else ++result.lines.blank;
        // Strings cannot span physical lines, except SV backslash continuation.
        const auto contentEnd = end > start && text[end - 1] == '\r' ? end - 1 : end;
        if (vhdl || contentEnd == start || text[contentEnd - 1] != '\\') string = false;
        start = end == text.size() ? end : end + 1;
    }
    return result;
}
} // namespace fpga::analysis
