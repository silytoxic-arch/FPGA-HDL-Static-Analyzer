#include "project/file_list.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <set>
#include <string_view>

namespace fpga::project {
namespace {

std::filesystem::path resolvedFrom(const std::filesystem::path& parent,
                                   std::string_view value) {
    auto path = std::filesystem::path(value);
    if (path.is_relative()) path = parent / path;
    return path.lexically_normal();
}

std::vector<std::string> splitPlus(std::string_view value) {
    std::vector<std::string> values;
    std::size_t begin = 0;
    while (begin <= value.size()) {
        const auto end = value.find('+', begin);
        const auto item = value.substr(begin, end == std::string_view::npos ?
            value.size() - begin : end - begin);
        if (!item.empty()) values.emplace_back(item);
        if (end == std::string_view::npos) break;
        begin = end + 1;
    }
    return values;
}

std::vector<std::string> tokenizeLine(std::string_view line, std::string& error) {
    std::vector<std::string> tokens;
    std::string token;
    char quote = 0;
    bool escaping = false;
    const auto flush = [&] {
        if (!token.empty()) {
            tokens.push_back(std::move(token));
            token.clear();
        }
    };
    for (std::size_t index = 0; index < line.size(); ++index) {
        const char character = line[index];
        if (escaping) {
            token.push_back(character);
            escaping = false;
            continue;
        }
        if (character == '\\') {
            const bool escapable = index + 1 < line.size() &&
                (line[index + 1] == '\\' || line[index + 1] == '\'' ||
                 line[index + 1] == '"' || line[index + 1] == '#' ||
                 std::isspace(static_cast<unsigned char>(line[index + 1])));
            if (escapable) escaping = true;
            else token.push_back(character);
            continue;
        }
        if (quote != 0) {
            if (character == quote) quote = 0;
            else token.push_back(character);
            continue;
        }
        if (character == '\'' || character == '"') {
            quote = character;
            continue;
        }
        const bool commentBoundary = index == 0 ||
            std::isspace(static_cast<unsigned char>(line[index - 1]));
        if (character == '#' && (token.empty() || commentBoundary)) break;
        if (character == '/' && index + 1 < line.size() && line[index + 1] == '/' &&
            (token.empty() || commentBoundary)) break;
        if (std::isspace(static_cast<unsigned char>(character))) {
            flush();
            continue;
        }
        token.push_back(character);
    }
    if (escaping) token.push_back('\\');
    if (quote != 0) error = "Unterminated quoted value";
    flush();
    return tokens;
}

std::filesystem::path cycleKey(const std::filesystem::path& path) {
    std::error_code error;
    auto canonical = std::filesystem::weakly_canonical(path, error);
    if (!error) return canonical;
    error.clear();
    auto absolute = std::filesystem::absolute(path, error);
    if (error) absolute = path;
    return absolute.lexically_normal();
}

void loadRecursive(const std::filesystem::path& path, FileListLoadResult& result,
                   std::set<std::filesystem::path>& active) {
    const auto key = cycleKey(path);
    if (!active.insert(key).second) {
        result.errors.push_back("Recursive file-list inclusion: " + path.string());
        return;
    }
    std::ifstream stream(path);
    if (!stream) {
        result.errors.push_back("Cannot read HDL file list: " + path.string());
        active.erase(key);
        return;
    }
    const auto parent = path.parent_path();
    std::string line;
    std::size_t lineNumber = 0;
    while (std::getline(stream, line)) {
        ++lineNumber;
        std::string tokenError;
        auto tokens = tokenizeLine(line, tokenError);
        const auto error = [&](std::string message) {
            result.errors.push_back(path.string() + ':' + std::to_string(lineNumber) +
                                    ": " + std::move(message));
        };
        if (!tokenError.empty()) {
            error(std::move(tokenError));
            continue;
        }
        for (std::size_t index = 0; index < tokens.size(); ++index) {
            const auto& token = tokens[index];
            if (token == "-f" || token == "-F") {
                if (++index == tokens.size()) {
                    error("Missing nested file list after " + token);
                    break;
                }
                const auto nested = resolvedFrom(parent, tokens[index]);
                if (active.contains(cycleKey(nested))) {
                    error("Recursive file-list inclusion: " + nested.string());
                } else {
                    loadRecursive(nested, result, active);
                }
            } else if ((token.starts_with("-f") || token.starts_with("-F")) &&
                       token.size() > 2) {
                const auto nested = resolvedFrom(parent, std::string_view(token).substr(2));
                if (active.contains(cycleKey(nested))) {
                    error("Recursive file-list inclusion: " + nested.string());
                } else {
                    loadRecursive(nested, result, active);
                }
            } else if (token.starts_with("+incdir+")) {
                const auto values = splitPlus(std::string_view(token).substr(8));
                if (values.empty()) error("+incdir+ requires at least one directory");
                for (const auto& value : values) {
                    result.includeDirectories.push_back(resolvedFrom(parent, value));
                }
            } else if (token.starts_with("+define+")) {
                const auto values = splitPlus(std::string_view(token).substr(8));
                if (values.empty()) error("+define+ requires at least one macro");
                result.defines.insert(result.defines.end(), values.begin(), values.end());
            } else if (token == "-I" || token == "-D") {
                if (++index == tokens.size()) {
                    error("Missing value after " + token);
                    break;
                }
                if (token == "-I") {
                    result.includeDirectories.push_back(resolvedFrom(parent, tokens[index]));
                } else {
                    result.defines.push_back(tokens[index]);
                }
            } else if (token.starts_with("-I") && token.size() > 2) {
                result.includeDirectories.push_back(
                    resolvedFrom(parent, std::string_view(token).substr(2)));
            } else if (token.starts_with("-D") && token.size() > 2) {
                result.defines.push_back(token.substr(2));
            } else if (!token.empty() && (token.front() == '-' || token.front() == '+')) {
                error("Unsupported file-list option: " + token);
            } else {
                result.inputs.push_back(resolvedFrom(parent, token));
            }
        }
    }
    active.erase(key);
}

} // namespace

FileListLoadResult loadFileList(const std::filesystem::path& path) {
    FileListLoadResult result;
    std::set<std::filesystem::path> active;
    loadRecursive(path.lexically_normal(), result, active);
    return result;
}

} // namespace fpga::project
