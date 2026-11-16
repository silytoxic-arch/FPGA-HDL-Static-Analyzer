#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace fpga::project {

struct FileListLoadResult {
    std::vector<std::filesystem::path> inputs;
    std::vector<std::filesystem::path> includeDirectories;
    std::vector<std::string> defines;
    std::vector<std::string> errors;
};

// Loads a simulator-style HDL file list. Relative paths are resolved from the
// file list that contains them; nested -f/-F lists are expanded recursively.
[[nodiscard]] FileListLoadResult loadFileList(const std::filesystem::path& path);

} // namespace fpga::project
