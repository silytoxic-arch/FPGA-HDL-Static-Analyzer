#pragma once

#include <cstddef>
#include <filesystem>
#include <optional>
#include <string>

namespace fpga::analysis {

enum class Severity { Critical, Error, Warning, Info };

struct Violation {
    std::string ruleId;
    Severity severity{Severity::Warning};
    std::string message;
    std::string description;
    std::string recommendation;
    std::filesystem::path file;
    std::size_t line{1};
    std::size_t column{1};
    std::optional<std::string> module;
    std::optional<std::string> signal;
};

} // namespace fpga::analysis
