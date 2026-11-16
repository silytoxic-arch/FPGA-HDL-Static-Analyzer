#include "analysis/rule_catalog.h"
#include "analysis/standard_profiles.h"
#include "analysis/standard_evaluator.h"

#include <algorithm>
#include <cctype>
#include <iostream>
#include <set>
#include <string>

int main() {
    using namespace fpga::analysis;
    int failures = 0;
    const auto check = [&](bool condition, const char* message) {
        if (!condition) { std::cerr << "FAIL: " << message << '\n'; ++failures; }
    };
    const auto rules = ruleCatalog();
    check(rules.size() == 97, "all built-in rules are registered");
    std::set<std::string> identifiers;
    std::string previous;
    for (const auto& rule : rules) {
        check(!rule.id.empty() && !rule.languages.empty() && !rule.title.empty() &&
                  !rule.description.empty() && !rule.recommendation.empty(),
              "every rule has complete metadata");
        check(identifiers.insert(std::string(rule.id)).second, "rule IDs are unique");
        check(previous.empty() || previous < rule.id, "rule catalog has deterministic ID ordering");
        previous = rule.id;
    }
    const auto latch = findRule("rtl-latch-inferred");
    check(latch && latch->severity == Severity::Warning,
          "case-insensitive rule lookup returns canonical metadata");
    check(!findRule("NOT-A-RULE"), "unknown rule lookup is rejected");
    check(standardProfiles().size() == 2, "both GJB standard profiles are registered");
    check(standardRuleInventory().size() == 209,
          "reverse-engineered GJB inventory contains all observed rules");
    check(standardRuleDetails().size() == standardRuleInventory().size(),
          "every observed GJB rule has a PDF-derived implementation detail");
    check(standardRuleExamples().size() == standardRuleInventory().size(),
          "all 209 GJB rules have compliant and violating example criteria");
    const auto isOfficialThreePartId = [](const std::string& id) {
        if (id.size() < 5 || (id[0] != 'R' && id[0] != 'A') || id[1] != '-') return false;
        const auto secondDash = id.find('-', 2);
        if (secondDash == std::string::npos || secondDash == 2 || secondDash + 1 == id.size() ||
            id.find('-', secondDash + 1) != std::string::npos) return false;
        return std::all_of(id.begin() + 2, id.begin() + secondDash,
                   [](unsigned char value) { return std::isdigit(value) != 0; }) &&
               std::all_of(id.begin() + secondDash + 1, id.end(),
                   [](unsigned char value) { return std::isdigit(value) != 0; });
    };
    std::set<std::string> displayIdentifiers;
    for (const auto& entry : standardRuleInventory()) {
        const auto displayId = standardRuleDisplayId(entry.clauseId);
        check(isOfficialThreePartId(displayId) &&
                  displayIdentifiers.insert(std::string(entry.standardId) + ":" + displayId).second,
              "every GJB rule has a unique official three-part public identifier within its profile");
        const auto detail = findStandardRuleDetail(entry.clauseId);
        check(detail.has_value() && !detail->summary.empty(),
              "every observed GJB rule has a non-empty summary");
        check(!detail || detail->assessmentMode != StandardAssessmentMode::Automated ||
                  hasAutomatedStandardDetector(entry.clauseId),
              "an automated GJB rule must have an executable detector");
        const auto* example = findStandardRuleExample(entry.clauseId);
        check(example && !example->compliant.empty() && !example->violating.empty() &&
                  !example->evidenceBasis.empty(),
              "every GJB rule has complete positive/negative evidence criteria");
    }
    check(standardRuleDisplayId("GJB9765.4.2.a") == "R-2-1" &&
              standardRuleDisplayId("GJB9765.4.8.e") == "A-8-1" &&
              standardRuleDisplayId("GJB9765.4.5.f.2") == "A-5-3" &&
              standardRuleDisplayId("GJB9765.4.11.c") == "R-11-3" &&
              standardRuleDisplayId("GJB9765.4.14.b.1") == "A-14-3" &&
              standardRuleDisplayId("GJB10157.5.2.1") == "R-2-1" &&
              standardRuleDisplayId("GJB10157.5.2.18") == "A-2-8" &&
              standardRuleDisplayId("RTL-LATCH-INFERRED") == "RTL-LATCH-INFERRED",
          "public GJB IDs follow the official three-part numeric index and preserve gaps");
    check(findStandardRuleExample("R-2-1") == nullptr &&
              findStandardRuleDetail("GJB9765", "R-2-1")->clauseId == "GJB9765.4.2.a" &&
              findStandardRuleExample("GJB10157", "R-2-1") != nullptr &&
              findStandardRuleDetail("A-2-8")->clauseId == "GJB10157.5.2.18",
          "profile-qualified public IDs resolve while cross-profile ambiguity is rejected");
    for (const auto& detail : standardRuleDetails()) {
        check(detail.assessmentMode == StandardAssessmentMode::Automated ||
                  detail.assessmentMode == StandardAssessmentMode::ProjectContext ||
                  !hasAutomatedStandardDetector(detail.clauseId),
              "registered detectors must be automated or explicitly context-gated, never manual-only");
    }
    check(findStandardRuleDetail("GJB10157.5.2.3")->assessmentMode == StandardAssessmentMode::Automated &&
              hasAutomatedStandardDetector("GJB10157.5.2.3"),
          "documented GJB keyword families have an automated detector without claiming vendor GUI defaults");
    check(findStandardRuleDetail("GJB10157.5.2.15")->assessmentMode == StandardAssessmentMode::Automated &&
              hasAutomatedStandardDetector("GJB10157.5.2.15"),
          "native naming-case predicate is registered despite the contradictory packaged compliant example");
    check(findStandardRuleDetail("GJB9765.4.14.d.4")->assessmentMode == StandardAssessmentMode::Automated &&
              hasAutomatedStandardDetector("GJB9765.4.14.d.4"),
          "native FILE_OPEN path-literal predicate is registered without a false project-context gate");
    check(std::ranges::count_if(standardRuleInventory(), [](const auto& rule) {
              return rule.compulsory;
          }) == 104,
          "combined compulsory subset count matches T-Linter profiles");
    check(findStandardProfile("gjb10157") && findStandardProfile("GJB9765-2020"),
          "standard profile aliases resolve case-insensitively");
    for (const auto& binding : standardRuleBindings()) {
        check(findRule(binding.analyzerRuleId).has_value(),
              "every GJB binding references an implemented analyzer rule");
    }
    return failures == 0 ? 0 : 1;
}
