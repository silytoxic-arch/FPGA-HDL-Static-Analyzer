#include "analysis/baseline.h"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>

int main() {
    using namespace fpga;
    int failures = 0;
    const auto check = [&](bool condition, const char* message) {
        if (!condition) { std::cerr << "FAIL: " << message << '\n'; ++failures; }
    };
    const auto suffix = std::to_string(
        std::chrono::high_resolution_clock::now().time_since_epoch().count());
    const auto baselinePath = std::filesystem::temp_directory_path() /
                              ("fpga-analyzer-baseline-" + suffix + ".txt");
    const auto malformedPath = std::filesystem::temp_directory_path() /
                               ("fpga-analyzer-baseline-bad-" + suffix + ".txt");
    const auto source = std::filesystem::absolute("rtl/top.sv");

    analysis::AnalysisResult recorded;
    recorded.violations.push_back({"RULE-A", analysis::Severity::Warning,
        "Repeated\tmessage", "description", "recommendation", source, 10, 3,
        "top", "state"});
    recorded.violations.push_back({"RULE-A", analysis::Severity::Warning,
        "Repeated\tmessage", "description", "recommendation", source, 20, 3,
        "top", "state"});
    std::string error;
    check(analysis::writeBaseline(recorded, baselinePath, error),
          "baseline is written successfully");
    const auto loaded = analysis::loadBaseline(baselinePath);
    check(loaded.errors.empty() && loaded.entries.size() == 2,
          "baseline round trip preserves duplicate entries");
    check(!loaded.entries.empty() && loaded.entries[0].message == "Repeated\tmessage",
          "baseline escaping round trip preserves tabs");

    analysis::AnalysisResult current;
    current.violations.push_back({"RULE-A", analysis::Severity::Warning,
        "Repeated\tmessage", "description", "recommendation", source, 11, 8,
        "top", "state"});
    current.violations.push_back({"RULE-A", analysis::Severity::Warning,
        "Repeated\tmessage", "description", "recommendation", source, 21, 8,
        "top", "state"});
    current.violations.push_back({"RULE-A", analysis::Severity::Warning,
        "Repeated\tmessage", "description", "recommendation", source, 31, 8,
        "top", "state"});
    current.violations.push_back({"RULE-A", analysis::Severity::Warning,
        "New message", "description", "recommendation", source, 40, 8,
        "top", "state"});
    check(analysis::applyBaseline(current, loaded.entries) == 2,
          "baseline applies each duplicate entry only once");
    check(current.baselinedViolations == 2 && current.violations.size() == 2,
          "baseline count is separate and new findings remain active");
    check(current.violations[0].line == 31 && current.violations[1].message == "New message",
          "line movement is tolerated without hiding an extra or changed finding");

    {
        std::ofstream malformed(malformedPath);
        malformed << "not-a-baseline\n";
    }
    check(analysis::loadBaseline(malformedPath).errors.size() == 1,
          "invalid baseline header is rejected");
    std::error_code cleanupError;
    std::filesystem::remove(baselinePath, cleanupError);
    std::filesystem::remove(malformedPath, cleanupError);
    return failures == 0 ? 0 : 1;
}
