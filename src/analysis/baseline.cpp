#include "analysis/baseline.h"

#include <fstream>
#include <map>
#include <sstream>

namespace fpga::analysis {
namespace {

constexpr std::string_view Header = "fpga-lint-baseline\t1";

std::string escape(std::string_view value) {
    std::string result;
    for (const char character : value) {
        switch (character) {
        case '\\': result += "\\\\"; break;
        case '\t': result += "\\t"; break;
        case '\n': result += "\\n"; break;
        case '\r': result += "\\r"; break;
        default: result += character;
        }
    }
    return result;
}

bool unescape(std::string_view value, std::string& result) {
    result.clear();
    for (std::size_t i = 0; i < value.size(); ++i) {
        if (value[i] != '\\') { result += value[i]; continue; }
        if (++i == value.size()) return false;
        switch (value[i]) {
        case '\\': result += '\\'; break;
        case 't': result += '\t'; break;
        case 'n': result += '\n'; break;
        case 'r': result += '\r'; break;
        default: return false;
        }
    }
    return true;
}

std::vector<std::string_view> fields(std::string_view line) {
    std::vector<std::string_view> result;
    std::size_t begin = 0;
    while (true) {
        const auto separator = line.find('\t', begin);
        result.push_back(line.substr(begin, separator == std::string_view::npos ?
                                           line.size() - begin : separator - begin));
        if (separator == std::string_view::npos) break;
        begin = separator + 1;
    }
    return result;
}

std::filesystem::path absoluteNormalized(const std::filesystem::path& path) {
    std::error_code error;
    auto absolute = std::filesystem::absolute(path, error);
    return (error ? path : absolute).lexically_normal();
}

std::string fingerprint(std::string_view ruleId, const std::filesystem::path& file,
                        std::string_view message, std::string_view designUnit,
                        std::string_view signal) {
    std::ostringstream value;
    value << ruleId << '\n' << absoluteNormalized(file).generic_string() << '\n'
          << message << '\n' << designUnit << '\n' << signal;
    return value.str();
}

std::filesystem::path storedPath(const std::filesystem::path& file,
                                 const std::filesystem::path& baseline) {
    const auto absoluteFile = absoluteNormalized(file);
    const auto directory = absoluteNormalized(baseline.parent_path().empty() ?
                                               std::filesystem::path(".") : baseline.parent_path());
    const auto relative = absoluteFile.lexically_relative(directory);
    if (!relative.empty() && *relative.begin() != "..") return relative;
    return absoluteFile;
}

} // namespace

BaselineLoadResult loadBaseline(const std::filesystem::path& path) {
    BaselineLoadResult result;
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        result.errors.push_back("Cannot read baseline file: " + path.string());
        return result;
    }
    std::string line;
    if (!std::getline(input, line) || line != Header) {
        result.errors.push_back(path.string() + ":1: Invalid or unsupported baseline header");
        return result;
    }
    const auto directory = path.parent_path().empty() ? std::filesystem::path(".") : path.parent_path();
    std::size_t lineNumber = 1;
    while (std::getline(input, line)) {
        ++lineNumber;
        if (line.empty()) continue;
        const auto raw = fields(line);
        if (raw.size() != 6) {
            result.errors.push_back(path.string() + ':' + std::to_string(lineNumber) +
                                    ": Expected 6 tab-separated fields");
            continue;
        }
        std::string decoded[6];
        bool valid = true;
        for (std::size_t i = 0; i < raw.size(); ++i) valid = unescape(raw[i], decoded[i]) && valid;
        if (!valid || decoded[0].empty() || decoded[2].empty()) {
            result.errors.push_back(path.string() + ':' + std::to_string(lineNumber) +
                                    ": Invalid escaped field or missing rule/path");
            continue;
        }
        auto file = std::filesystem::path(decoded[2]);
        if (file.is_relative()) file = directory / file;
        result.entries.push_back({std::move(decoded[0]), std::move(decoded[1]),
                                  absoluteNormalized(file), std::move(decoded[3]),
                                  std::move(decoded[4]), std::move(decoded[5])});
    }
    return result;
}

std::size_t applyBaseline(AnalysisResult& result, const std::vector<BaselineEntry>& entries) {
    std::map<std::string, std::size_t> remaining;
    for (const auto& entry : entries) {
        ++remaining[fingerprint(entry.ruleId, entry.file, entry.message,
                                entry.designUnit, entry.signal)];
    }
    const auto before = result.violations.size();
    std::erase_if(result.violations, [&](const Violation& violation) {
        const auto key = fingerprint(violation.ruleId, violation.file, violation.message,
                                     violation.module.value_or(""), violation.signal.value_or(""));
        const auto found = remaining.find(key);
        if (found == remaining.end() || found->second == 0) return false;
        --found->second;
        return true;
    });
    result.baselinedViolations += before - result.violations.size();
    return before - result.violations.size();
}

bool writeBaseline(const AnalysisResult& result, const std::filesystem::path& path,
                   std::string& error) {
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    if (!output) {
        error = "Cannot write baseline file: " + path.string();
        return false;
    }
    output << Header << '\n';
    for (const auto& violation : result.violations) {
        output << escape(violation.ruleId) << '\t'
               << escape(severityName(violation.severity)) << '\t'
               << escape(storedPath(violation.file, path).generic_string()) << '\t'
               << escape(violation.message) << '\t'
               << escape(violation.module.value_or("")) << '\t'
               << escape(violation.signal.value_or("")) << '\n';
    }
    output.flush();
    if (!output) {
        error = "Failed while writing baseline file: " + path.string();
        return false;
    }
    return true;
}

} // namespace fpga::analysis
