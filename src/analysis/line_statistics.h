#pragma once

#include "core/source.h"
#include <vector>
#include <string_view>

namespace fpga::analysis {
struct LineCounts {
    std::size_t total{0};
    std::size_t code{0};
    std::size_t comment{0};
    std::size_t blank{0};
    LineCounts& operator+=(const LineCounts& other);
};
struct FileLineStatistics {
    std::filesystem::path file;
    core::HdlLanguage language{core::HdlLanguage::Unknown};
    LineCounts lines;
};
struct LanguageLineStatistics {
    core::HdlLanguage language;
    std::size_t files{0};
    LineCounts lines;
};
struct LineStatistics {
    LineCounts total;
    std::vector<FileLineStatistics> files;
    std::vector<LanguageLineStatistics> languages;
    void append(const FileLineStatistics& file);
};
[[nodiscard]] FileLineStatistics countSourceLines(const core::SourceFile& source);
[[nodiscard]] std::string_view lineStatisticsLanguageName(core::HdlLanguage language);
} // namespace fpga::analysis
