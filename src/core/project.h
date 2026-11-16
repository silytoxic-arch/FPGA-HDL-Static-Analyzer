#pragma once

#include "core/source.h"

#include <filesystem>
#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace fpga::core {

// Project owns its source-file records. Duplicate normalized paths are rejected.
class Project {
public:
    Project(std::string name, std::filesystem::path rootDirectory);

    [[nodiscard]] const std::string& name() const noexcept;
    [[nodiscard]] const std::filesystem::path& rootDirectory() const noexcept;
    [[nodiscard]] bool addSourceFile(SourceFile sourceFile);
    [[nodiscard]] std::optional<std::reference_wrapper<const SourceFile>>
    findSourceFile(const std::filesystem::path& path) const noexcept;
    [[nodiscard]] const std::vector<SourceFile>& sourceFiles() const noexcept;

private:
    std::string name_;
    std::filesystem::path rootDirectory_;
    std::vector<SourceFile> sourceFiles_;
};

} // namespace fpga::core
