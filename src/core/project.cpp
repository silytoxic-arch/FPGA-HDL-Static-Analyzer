#include "core/project.h"

#include <algorithm>
#include <utility>

namespace fpga::core {

SourceFile::SourceFile(std::filesystem::path path, HdlLanguage language,
                       std::string content)
    : path_(std::move(path)), language_(language), content_(std::move(content)) {}

const std::filesystem::path& SourceFile::path() const noexcept { return path_; }
HdlLanguage SourceFile::language() const noexcept { return language_; }
const std::string& SourceFile::content() const noexcept { return content_; }

Project::Project(std::string name, std::filesystem::path rootDirectory)
    : name_(std::move(name)), rootDirectory_(std::move(rootDirectory)) {}

const std::string& Project::name() const noexcept { return name_; }
const std::filesystem::path& Project::rootDirectory() const noexcept {
    return rootDirectory_;
}

bool Project::addSourceFile(SourceFile sourceFile) {
    if (findSourceFile(sourceFile.path())) {
        return false;
    }
    sourceFiles_.push_back(std::move(sourceFile));
    return true;
}

std::optional<std::reference_wrapper<const SourceFile>>
Project::findSourceFile(const std::filesystem::path& path) const noexcept {
    const auto normalized = path.lexically_normal();
    const auto found = std::find_if(sourceFiles_.begin(), sourceFiles_.end(),
        [&normalized](const SourceFile& source) {
            return source.path().lexically_normal() == normalized;
        });
    if (found == sourceFiles_.end()) {
        return std::nullopt;
    }
    return std::cref(*found);
}

const std::vector<SourceFile>& Project::sourceFiles() const noexcept {
    return sourceFiles_;
}

} // namespace fpga::core
