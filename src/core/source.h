#pragma once

#include <cstddef>
#include <filesystem>
#include <string>

namespace fpga::core {

enum class HdlLanguage {
    Verilog,
    SystemVerilog,
    Vhdl,
    Unknown
};

// A source file owns its path and optional in-memory text. Parsing and rule
// execution remain outside this value type.
class SourceFile {
public:
    SourceFile(std::filesystem::path path, HdlLanguage language,
               std::string content = {});

    [[nodiscard]] const std::filesystem::path& path() const noexcept;
    [[nodiscard]] HdlLanguage language() const noexcept;
    [[nodiscard]] const std::string& content() const noexcept;

private:
    std::filesystem::path path_;
    HdlLanguage language_{HdlLanguage::Unknown};
    std::string content_;
};

// Lines and columns are one-based. A byte offset is kept separately because a
// displayed Unicode column is not necessarily a byte offset.
struct SourceLocation {
    std::filesystem::path file;
    std::size_t line{1};
    std::size_t column{1};
    std::size_t byteOffset{0};

    [[nodiscard]] bool valid() const noexcept {
        return !file.empty() && line > 0 && column > 0;
    }

    friend bool operator==(const SourceLocation&, const SourceLocation&) = default;
};

} // namespace fpga::core
