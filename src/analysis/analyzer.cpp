#include "analysis/analyzer.h"
#include "analysis/rule_catalog.h"
#include "analysis/standard_evaluator.h"
#include "analysis/standard_profiles.h"
#include "parser/frontend_preprocessor.h"
#include "parser/frontend_source_view.h"
#include "core/source_text.h"
#include <array>
#include "parser/preprocessor.h"
#include "parser/semantic_analyzer.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <functional>
#include <map>
#include <set>
#include <sstream>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace fpga::analysis {
namespace {

struct Line {
    std::string text;
    std::size_t number;
};

std::string lower(std::string value) {
    std::ranges::transform(value, value.begin(),
                           [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return value;
}

bool equalInsensitive(std::string_view left, std::string_view right) {
    return lower(std::string(left)) == lower(std::string(right));
}

bool wildcardMatch(std::string_view pattern, std::string_view value) {
    std::size_t patternIndex = 0;
    std::size_t valueIndex = 0;
    std::size_t star = std::string_view::npos;
    std::size_t retry = 0;
    while (valueIndex < value.size()) {
        if (patternIndex < pattern.size() &&
            (pattern[patternIndex] == '?' || pattern[patternIndex] == value[valueIndex])) {
            ++patternIndex;
            ++valueIndex;
        } else if (patternIndex < pattern.size() && pattern[patternIndex] == '*') {
            star = patternIndex++;
            retry = valueIndex;
        } else if (star != std::string_view::npos) {
            patternIndex = star + 1;
            valueIndex = ++retry;
        } else {
            return false;
        }
    }
    while (patternIndex < pattern.size() && pattern[patternIndex] == '*') ++patternIndex;
    return patternIndex == pattern.size();
}

bool matchesExclude(const std::filesystem::path& path, const AnalysisOptions& options) {
    const auto generic = path.lexically_normal().generic_string();
    const auto filename = path.filename().generic_string();
    for (const auto& pattern : options.excludePatterns) {
        if (wildcardMatch(pattern, generic) || wildcardMatch(pattern, filename)) return true;
    }
    return false;
}

std::vector<Line> splitLines(const std::string& content) {
    std::vector<Line> lines;
    std::istringstream stream(content);
    std::string line;
    std::size_t number = 1;
    while (std::getline(stream, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        lines.push_back({std::move(line), number++});
    }
    if (lines.empty() && !content.empty()) {
        lines.push_back({content, 1});
    }
    return lines;
}

void addViolation(AnalysisResult& result, std::string rule, Severity severity,
                  std::string message, std::string description, std::string recommendation,
                  const std::filesystem::path& file, std::size_t line, std::size_t column,
                  std::optional<std::string> unit = std::nullopt,
                  std::optional<std::string> signal = std::nullopt) {
    result.violations.push_back({std::move(rule), severity, std::move(message),
        std::move(description), std::move(recommendation), file, line, column,
        std::move(unit), std::move(signal)});
}

bool directiveMatches(const std::string& line, std::string_view directive,
                      std::string_view rule) {
    const auto directivePosition = line.find(directive);
    if (directivePosition == std::string::npos) return false;
    const auto slash = line.find("//");
    const auto dash = line.find("--");
    const auto block = line.find("/*");
    const auto commentBefore = [&](std::size_t position) {
        return position != std::string::npos && position < directivePosition;
    };
    if (!commentBefore(slash) && !commentBefore(dash) && !commentBefore(block)) return false;
    auto values = line.substr(directivePosition + directive.size());
    std::ranges::replace(values, ',', ' ');
    std::istringstream parser(values);
    std::string value;
    while (parser >> value) {
        if (equalInsensitive(value, "all") || equalInsensitive(value, rule)) return true;
    }
    return false;
}

bool globallyDisabled(std::string_view rule, const AnalysisOptions& options) {
    return std::ranges::any_of(options.disabledRules, [&](const std::string& disabled) {
        return equalInsensitive(disabled, "all") || equalInsensitive(disabled, rule);
    });
}

void applySeverityOverrides(AnalysisResult& result, const AnalysisOptions& options) {
    for (const auto& override : options.severityOverrides) {
        for (auto& violation : result.violations) {
            if (equalInsensitive(violation.ruleId, override.ruleId)) {
                violation.severity = override.severity;
            }
        }
    }
}

void filterViolations(AnalysisResult& result, std::span<const core::SourceFile* const> sources,
                      const AnalysisOptions& options) {
    // Splitting a large generated netlist once per diagnostic made waiver
    // filtering proportional to source-size * findings. Cache within this
    // call only, using the proven source text and existing waiver rules.
    struct WaiverSource {
        std::vector<Line> lines;
        std::vector<std::size_t> fileDirectiveLines;
    };
    std::unordered_map<const core::SourceFile*, WaiverSource> waiverSources;
    const auto suppressed = [&](const Violation& violation) {
        if (globallyDisabled(violation.ruleId, options)) return true;
        const auto source = std::ranges::find_if(sources, [&](const core::SourceFile* candidate) {
            return candidate->path().lexically_normal() == violation.file.lexically_normal();
        });
        if (source == sources.end()) return false;
        auto cached = waiverSources.find(*source);
        if (cached == waiverSources.end()) {
            WaiverSource entry;
            entry.lines = splitLines((*source)->content());
            for (std::size_t i = 0; i < entry.lines.size(); ++i)
                if (entry.lines[i].text.find("fpga-lint-disable-file") != std::string::npos)
                    entry.fileDirectiveLines.push_back(i);
            cached = waiverSources.emplace(*source, std::move(entry)).first;
        }
        const auto& lines = cached->second.lines;
        if (std::ranges::any_of(cached->second.fileDirectiveLines, [&](std::size_t line) {
                return directiveMatches(lines[line].text, "fpga-lint-disable-file", violation.ruleId);
            })) return true;
        if (violation.line > 0 && violation.line <= lines.size() &&
            directiveMatches(lines[violation.line - 1].text, "fpga-lint-disable-line", violation.ruleId)) {
            return true;
        }
        return violation.line > 1 && violation.line - 1 <= lines.size() &&
               directiveMatches(lines[violation.line - 2].text,
                                "fpga-lint-disable-next-line", violation.ruleId);
    };
    const auto before = result.violations.size();
    std::erase_if(result.violations, suppressed);
    result.suppressedViolations += before - result.violations.size();
}

// Match waivers against immutable consumed text. Ambiguous same-path snapshots
// cannot prove a suppression; never reread a potentially changed include file.
std::vector<const core::SourceFile*> waiverInputViews(std::span<const core::SourceFile> roots,
    std::span<const parser::FrontendPreprocessorResult* const> frontends) {
    std::map<std::filesystem::path,const core::SourceFile*> views;
    std::set<std::filesystem::path> ambiguous;
    const auto add=[&](const core::SourceFile& source) {
        const auto key=source.path().lexically_normal();
        if(ambiguous.contains(key)) return;
        const auto [found,inserted]=views.emplace(key,&source);
        if(!inserted && !core::sameLineEndingText(found->second->content(),source.content())) {
            views.erase(found);ambiguous.insert(key);
        }
    };
    for(const auto& root:roots) add(root);
    for(const auto* frontend:frontends) if(frontend) {
        for(const auto& source:frontend->includedSources) add(source);
        for(const auto& path:frontend->ambiguousInputFiles) {
            const auto key=path.lexically_normal();views.erase(key);ambiguous.insert(key);
        }
    }
    std::vector<const core::SourceFile*> result;
    for(const auto& [path,source]:views) {static_cast<void>(path);result.push_back(source);}
    return result;
}

void analyzeCommon(const core::SourceFile& source, const std::vector<Line>& lines,
                   AnalysisResult& result) {
    for (const auto& line : lines) {
        if (!line.text.empty() && (line.text.back() == ' ' || line.text.back() == '\t')) {
            addViolation(result, "STYLE-001", Severity::Info, "Trailing whitespace",
                         "The line ends with whitespace, which creates noisy diffs.",
                         "Remove whitespace at the end of the line.", source.path(), line.number,
                         line.text.size());
        }
        if (line.text.size() > 120) {
            addViolation(result, "STYLE-002", Severity::Info, "Line exceeds 120 columns",
                         "Long HDL lines are harder to review and maintain.",
                         "Wrap the expression or declaration across multiple lines.",
                         source.path(), line.number, 121);
        }
    }
}

std::string unitKey(const parser::DesignUnitInfo& unit) {
    if (unit.kind == parser::DesignUnitKind::VhdlEntity) return "h:" + lower(unit.name);
    return "v:" + unit.name;
}

std::vector<std::string> targetKeys(const parser::DesignUnitInfo& parent,
                                    const parser::InstanceInfo& instance) {
    if (parent.kind == parser::DesignUnitKind::VhdlEntity) {
        return {"h:" + lower(instance.target), "v:" + instance.target};
    }
    return {"v:" + instance.target, "h:" + lower(instance.target)};
}

bool declaredBlackBox(const parser::DesignUnitInfo& parent,
                      const parser::InstanceInfo& instance,
                      const AnalysisOptions& options) {
    return std::ranges::any_of(options.blackBoxes, [&](const std::string& blackBox) {
        if (parent.kind == parser::DesignUnitKind::VhdlEntity) {
            return equalInsensitive(blackBox, instance.target);
        }
        return blackBox == instance.target;
    });
}

void analyzeTimingConsistency(AnalysisResult& result) {
    for (const auto& unit : result.structure) {
        std::unordered_map<std::string, bool> clockEdges;
        std::unordered_map<std::string, bool> reportedClocks;
        for (const auto& clock : unit.clocks) {
            const auto key = lower(clock.signal);
            const auto existing = clockEdges.find(key);
            if (existing == clockEdges.end()) {
                clockEdges.emplace(key, clock.risingEdge);
            } else if (existing->second != clock.risingEdge && !reportedClocks[key]) {
                const auto definition = findRule("RTL-MIXED-CLOCK-EDGE");
                addViolation(result, "RTL-MIXED-CLOCK-EDGE", definition->severity,
                    "Clock is used on both rising and falling edges: " + clock.signal,
                    std::string(definition->description), std::string(definition->recommendation),
                    clock.location.file, clock.location.line, clock.location.column,
                    unit.name, clock.signal);
                reportedClocks[key] = true;
            }
        }
        for (const auto& reset : unit.resets) {
            if (reset.synchronousObserved && reset.asynchronousObserved) {
                const auto definition = findRule("RTL-MIXED-RESET-MODE");
                addViolation(result, "RTL-MIXED-RESET-MODE", definition->severity,
                    "Reset is used synchronously and asynchronously: " + reset.signal,
                    std::string(definition->description), std::string(definition->recommendation),
                    reset.location.file, reset.location.line, reset.location.column,
                    unit.name, reset.signal);
            }
            if (reset.activeHighObserved && reset.activeLowObserved) {
                const auto definition = findRule("RTL-RESET-POLARITY-CONFLICT");
                addViolation(result, "RTL-RESET-POLARITY-CONFLICT", definition->severity,
                    "Reset has conflicting active levels: " + reset.signal,
                    std::string(definition->description), std::string(definition->recommendation),
                    reset.location.file, reset.location.line, reset.location.column,
                    unit.name, reset.signal);
            }
        }
    }
}

void analyzeMultipleClocksPerProcess(AnalysisResult& result) {
    for (const auto& unit : result.structure) {
        const auto canonical = [&](const std::string& name) {
            return unit.kind == parser::DesignUnitKind::VhdlEntity ? lower(name) : name;
        };
        for (const auto& process : unit.sequentialProcesses) {
            std::vector<const parser::ClockInfo*> distinctClocks;
            std::unordered_set<std::string> seen;
            for (const auto& clock : process.clockEvents) {
                if (seen.insert(canonical(clock.signal)).second) {
                    distinctClocks.push_back(&clock);
                }
            }
            if (distinctClocks.size() < 2) continue;
            std::string events;
            for (const auto* clock : distinctClocks) {
                if (!events.empty()) events += ", ";
                events += clock->signal + (clock->risingEdge ? " rising" : " falling");
            }
            const auto& conflict = *distinctClocks[1];
            const auto definition = findRule("RTL-MULTIPLE-CLOCKS-PER-PROCESS");
            addViolation(result, "RTL-MULTIPLE-CLOCKS-PER-PROCESS", definition->severity,
                "Sequential process uses multiple clock events: " + events,
                std::string(definition->description), std::string(definition->recommendation),
                conflict.location.file, conflict.location.line, conflict.location.column,
                unit.name, conflict.signal);
        }
    }
}

void analyzeUnusedAsynchronousResets(AnalysisResult& result) {
    for (const auto& unit : result.structure) {
        const auto canonical = [&](const std::string& name) {
            return unit.kind == parser::DesignUnitKind::VhdlEntity ? lower(name) : name;
        };
        for (const auto& process : unit.sequentialProcesses) {
            for (const auto& reset : process.resets) {
                if (!reset.asynchronous) continue;
                const auto key = canonical(reset.signal);
                const bool readInBody = std::ranges::any_of(process.reads,
                    [&](const std::string& signal) { return canonical(signal) == key; });
                if (readInBody) continue;
                const auto definition = findRule("RTL-UNUSED-ASYNC-RESET");
                addViolation(result, "RTL-UNUSED-ASYNC-RESET", definition->severity,
                    "Asynchronous reset is not read in the sequential process body: " +
                        reset.signal,
                    std::string(definition->description),
                    std::string(definition->recommendation),
                    reset.location.file, reset.location.line, reset.location.column,
                    unit.name, reset.signal);
            }
        }
    }
}

void analyzeDirectClockDomainCrossings(AnalysisResult& result) {
    for (auto& unit : result.structure) {
        unit.cdcSynchronizers.clear();
        const auto canonical = [&](const std::string& name) {
            return unit.kind == parser::DesignUnitKind::VhdlEntity ? lower(name) : name;
        };
        const auto domainKey = [&](const parser::SequentialProcessInfo& process) {
            return canonical(process.clock) + (process.risingEdge ? ":rising" : ":falling");
        };
        std::unordered_map<std::string, std::vector<const parser::SequentialProcessInfo*>> writers;
        std::unordered_map<std::string, std::string> displayNames;
        std::unordered_set<std::string> timingSignals;
        for (const auto& clock : unit.clocks) timingSignals.insert(canonical(clock.signal));
        for (const auto& reset : unit.resets) timingSignals.insert(canonical(reset.signal));
        for (const auto& process : unit.sequentialProcesses) {
            for (const auto& signal : process.writes) {
                const auto key = canonical(signal);
                if (timingSignals.contains(key)) continue;
                writers[key].push_back(&process);
                displayNames.try_emplace(key, signal);
            }
        }
        std::unordered_set<std::string> interfaceSignals;
        for (const auto& port : unit.ports) {
            interfaceSignals.insert(canonical(port.name));
        }
        std::unordered_set<std::string> scalarSignals;
        for (const auto& signal : unit.scalarSignals) {
            scalarSignals.insert(canonical(signal));
        }
        const auto indexedInDomain = [&](const std::string& signal,
                                         const std::string& domain) {
            return std::ranges::any_of(unit.sequentialProcesses,
                [&](const parser::SequentialProcessInfo& process) {
                    if (domainKey(process) != domain) return false;
                    return std::ranges::any_of(process.indexedSignals,
                        [&](const std::string& indexed) {
                            return canonical(indexed) == signal;
                        });
                });
        };
        const auto writtenOnlyInDomain = [&](const std::string& signal,
                                             const std::string& domain) {
            const auto found = writers.find(signal);
            return found != writers.end() && !found->second.empty() &&
                std::ranges::all_of(found->second, [&](const auto* writer) {
                    return domainKey(*writer) == domain;
                });
        };
        const auto hasComplexReadInDomain = [&](const std::string& signal,
                                                const std::string& domain) {
            return std::ranges::any_of(unit.sequentialProcesses,
                [&](const parser::SequentialProcessInfo& process) {
                    if (domainKey(process) != domain) return false;
                    return std::ranges::any_of(process.complexReads,
                        [&](const std::string& read) {
                            return canonical(read) == signal;
                        });
                });
        };
        const auto transferDestinations = [&](const std::string& signal,
                                              const std::string& domain) {
            std::set<std::string> destinations;
            for (const auto& process : unit.sequentialProcesses) {
                if (domainKey(process) != domain) continue;
                for (const auto& transfer : process.directTransfers) {
                    if (canonical(transfer.source) == signal) {
                        destinations.insert(canonical(transfer.destination));
                    }
                }
            }
            return destinations;
        };
        const auto recognizedSynchronizers = [&](const std::string& signal,
                                                 const std::string& domain) {
            std::vector<parser::CdcSynchronizerInfo> synchronizers;
            if (hasComplexReadInDomain(signal, domain)) return synchronizers;
            const auto firstStages = transferDestinations(signal, domain);
            if (firstStages.empty()) return synchronizers;
            for (const auto& firstStage : firstStages) {
                if (firstStage == signal || interfaceSignals.contains(firstStage) ||
                    !scalarSignals.contains(firstStage) ||
                    indexedInDomain(firstStage, domain) ||
                    !writtenOnlyInDomain(firstStage, domain) ||
                    hasComplexReadInDomain(firstStage, domain)) {
                    return std::vector<parser::CdcSynchronizerInfo>{};
                }
                const auto secondStages = transferDestinations(firstStage, domain);
                const auto secondStage = std::ranges::find_if(
                    secondStages, [&](const std::string& candidate) {
                        return candidate != signal && candidate != firstStage &&
                               !indexedInDomain(candidate, domain) &&
                               writtenOnlyInDomain(candidate, domain);
                    });
                if (secondStage == secondStages.end()) {
                    return std::vector<parser::CdcSynchronizerInfo>{};
                }
                const auto firstWriter = writers.find(firstStage);
                if (firstWriter == writers.end() || firstWriter->second.empty()) {
                    return std::vector<parser::CdcSynchronizerInfo>{};
                }
                const auto* process = firstWriter->second.front();
                synchronizers.push_back({displayNames[signal], displayNames[firstStage],
                    displayNames[*secondStage], process->clock, process->risingEdge,
                    process->location});
            }
            return synchronizers;
        };
        std::set<std::pair<std::string, std::string>> reported;
        std::set<std::pair<std::string, std::string>> recordedSynchronizers;
        for (const auto& reader : unit.sequentialProcesses) {
            const auto destination = domainKey(reader);
            for (const auto& signal : reader.reads) {
                const auto key = canonical(signal);
                if (timingSignals.contains(key)) continue;
                if (std::ranges::any_of(reader.indexedSignals,
                        [&](const std::string& indexed) { return canonical(indexed) == key; })) continue;
                const auto found = writers.find(key);
                if (found == writers.end()) continue;
                std::set<std::string> sourceDomains;
                for (const auto* writer : found->second) {
                    if (std::ranges::any_of(writer->indexedSignals,
                            [&](const std::string& indexed) { return canonical(indexed) == key; })) {
                        sourceDomains.clear();
                        break;
                    }
                    if (domainKey(*writer) != destination) {
                        sourceDomains.insert(writer->clock +
                            (writer->risingEdge ? " rising" : " falling"));
                    }
                }
                if (sourceDomains.empty()) continue;
                const auto synchronizers = recognizedSynchronizers(key, destination);
                if (!synchronizers.empty()) {
                    if (recordedSynchronizers.emplace(key, destination).second) {
                        unit.cdcSynchronizers.insert(unit.cdcSynchronizers.end(),
                            synchronizers.begin(), synchronizers.end());
                    }
                    continue;
                }
                if (!reported.emplace(key, destination).second) continue;
                std::string sources;
                for (const auto& sourceDomain : sourceDomains) {
                    if (!sources.empty()) sources += ", ";
                    sources += sourceDomain;
                }
                const auto definition = findRule("CDC-DIRECT-CROSSING");
                addViolation(result, "CDC-DIRECT-CROSSING", definition->severity,
                    "Signal " + displayNames[key] + " crosses directly from " + sources +
                        " to " + reader.clock + (reader.risingEdge ? " rising" : " falling"),
                    std::string(definition->description), std::string(definition->recommendation),
                    reader.location.file, reader.location.line, reader.location.column,
                    unit.name, displayNames[key]);
            }
        }
    }
}

void analyzeResetDomainCrossings(AnalysisResult& result) {
    for (const auto& unit : result.structure) {
        const auto canonical = [&](const std::string& name) {
            return unit.kind == parser::DesignUnitKind::VhdlEntity ? lower(name) : name;
        };
        struct ResetUsage {
            std::string displayName;
            core::SourceLocation location;
            std::map<std::string, std::string> domains;
        };
        std::map<std::string, ResetUsage> usages;
        for (const auto& process : unit.sequentialProcesses) {
            const auto domainKey = canonical(process.clock) +
                                   (process.risingEdge ? ":rising" : ":falling");
            const auto domainName = process.clock +
                                    (process.risingEdge ? " rising" : " falling");
            for (const auto& reset : process.resets) {
                if (!reset.asynchronous) continue;
                const auto key = canonical(reset.signal);
                auto [usage, inserted] = usages.try_emplace(
                    key, ResetUsage{reset.signal, reset.location, {}});
                static_cast<void>(inserted);
                usage->second.domains.try_emplace(domainKey, domainName);
            }
        }
        for (const auto& [key, usage] : usages) {
            static_cast<void>(key);
            if (usage.domains.size() < 2) continue;
            std::string domains;
            for (const auto& [domainKey, domainName] : usage.domains) {
                static_cast<void>(domainKey);
                if (!domains.empty()) domains += ", ";
                domains += domainName;
            }
            const auto definition = findRule("RDC-MULTI-CLOCK-ASYNC-RESET");
            addViolation(result, "RDC-MULTI-CLOCK-ASYNC-RESET", definition->severity,
                "Asynchronous reset " + usage.displayName +
                    " directly controls multiple clock domains: " + domains,
                std::string(definition->description), std::string(definition->recommendation),
                usage.location.file, usage.location.line, usage.location.column,
                unit.name, usage.displayName);
        }
    }
}

void analyzeStructure(AnalysisResult& result, const AnalysisOptions& options) {
    std::unordered_map<std::string, std::vector<std::size_t>> definitions;
    for (std::size_t i = 0; i < result.structure.size(); ++i) {
        result.structure[i].topLevel = true;
        definitions[unitKey(result.structure[i])].push_back(i);
    }
    for (const auto& [key, indices] : definitions) {
        static_cast<void>(key);
        if (indices.size() < 2) continue;
        for (const auto index : indices) {
            const auto& unit = result.structure[index];
            addViolation(result, "STRUCT-DUPLICATE-UNIT", Severity::Error,
                         "Duplicate design unit: " + unit.name,
                         "More than one source defines the same module or entity name.",
                         "Rename or remove the duplicate definition so elaboration is deterministic.",
                         unit.location.file, unit.location.line, unit.location.column, unit.name);
        }
    }
    for (const auto& unit : result.structure) {
        for (const auto& duplicate : unit.duplicatePortDeclarations) {
            const auto definition = findRule("STRUCT-DUPLICATE-PORT-DECLARATION");
            addViolation(result, "STRUCT-DUPLICATE-PORT-DECLARATION",
                definition->severity,
                "Design unit " + unit.name + " declares port more than once: " +
                    duplicate.name,
                std::string(definition->description),
                std::string(definition->recommendation),
                duplicate.location.file, duplicate.location.line, duplicate.location.column,
                unit.name, duplicate.name);
        }
        for (const auto& duplicate : unit.duplicateParameterDeclarations) {
            const auto definition = findRule("STRUCT-DUPLICATE-PARAMETER-DECLARATION");
            addViolation(result, "STRUCT-DUPLICATE-PARAMETER-DECLARATION",
                definition->severity,
                "Design unit " + unit.name + " declares parameter or generic more than once: " +
                    duplicate.name,
                std::string(definition->description),
                std::string(definition->recommendation),
                duplicate.location.file, duplicate.location.line, duplicate.location.column,
                unit.name, duplicate.name);
        }
    }

    struct Edge { std::size_t target; const parser::InstanceInfo* instance; };
    std::vector<std::vector<Edge>> graph(result.structure.size());
    for (std::size_t parentIndex = 0; parentIndex < result.structure.size(); ++parentIndex) {
        auto& parent = result.structure[parentIndex];
        for (auto& instance : parent.instances) {
            const auto formalKey = [&](const std::string& name) {
                return parent.kind == parser::DesignUnitKind::VhdlEntity ? lower(name) : name;
            };
            std::set<std::string> connectedFormals;
            std::set<std::string> overriddenParameters;
            for (const auto& override : instance.namedParameterOverrides) {
                if (overriddenParameters.insert(formalKey(override.formal)).second) continue;
                const auto definition = findRule("STRUCT-DUPLICATE-NAMED-PARAMETER");
                addViolation(result, "STRUCT-DUPLICATE-NAMED-PARAMETER",
                    definition->severity,
                    "Instance " + instance.name +
                        " overrides named parameter more than once: " + override.formal,
                    std::string(definition->description),
                    std::string(definition->recommendation),
                    override.location.file, override.location.line, override.location.column,
                    parent.name, override.formal);
            }
            for (const auto& connection : instance.namedConnections) {
                if (connectedFormals.insert(formalKey(connection.formal)).second) continue;
                const auto definition = findRule("STRUCT-DUPLICATE-NAMED-PORT");
                addViolation(result, "STRUCT-DUPLICATE-NAMED-PORT", definition->severity,
                    "Instance " + instance.name + " connects named port more than once: " +
                        connection.formal,
                    std::string(definition->description),
                    std::string(definition->recommendation),
                    connection.location.file, connection.location.line,
                    connection.location.column, parent.name, connection.formal);
            }
            if (parent.kind == parser::DesignUnitKind::VerilogModule &&
                instance.hasNamedConnections && instance.hasPositionalConnections) {
                const auto definition = findRule("STRUCT-MIXED-PORT-CONNECTION-STYLE");
                addViolation(result, "STRUCT-MIXED-PORT-CONNECTION-STYLE",
                    definition->severity,
                    "Instance mixes positional and named port connections: " + instance.name,
                    std::string(definition->description),
                    std::string(definition->recommendation),
                    instance.location.file, instance.location.line,
                    instance.location.column, parent.name);
            }
            if (parent.kind == parser::DesignUnitKind::VerilogModule &&
                instance.hasNamedParameterOverrides &&
                instance.hasPositionalParameterOverrides) {
                const auto definition = findRule("STRUCT-MIXED-PARAMETER-OVERRIDE-STYLE");
                addViolation(result, "STRUCT-MIXED-PARAMETER-OVERRIDE-STYLE",
                    definition->severity,
                    "Instance mixes positional and named parameter overrides: " + instance.name,
                    std::string(definition->description),
                    std::string(definition->recommendation),
                    instance.location.file, instance.location.line,
                    instance.location.column, parent.name);
            }
            if (parent.kind == parser::DesignUnitKind::VhdlEntity &&
                instance.invalidParameterAssociationOrder) {
                const auto definition = findRule("STRUCT-VHDL-ASSOCIATION-ORDER");
                addViolation(result, "STRUCT-VHDL-ASSOCIATION-ORDER",
                    definition->severity,
                    "VHDL generic map has a positional association after a named association: " +
                        instance.name,
                    std::string(definition->description),
                    std::string(definition->recommendation),
                    instance.location.file, instance.location.line,
                    instance.location.column, parent.name);
            }
            if (parent.kind == parser::DesignUnitKind::VhdlEntity &&
                instance.invalidPortAssociationOrder) {
                const auto definition = findRule("STRUCT-VHDL-ASSOCIATION-ORDER");
                addViolation(result, "STRUCT-VHDL-ASSOCIATION-ORDER",
                    definition->severity,
                    "VHDL port map has a positional association after a named association: " +
                        instance.name,
                    std::string(definition->description),
                    std::string(definition->recommendation),
                    instance.location.file, instance.location.line,
                    instance.location.column, parent.name);
            }
            std::optional<std::size_t> resolvedIndex;
            for (const auto& key : targetKeys(parent, instance)) {
                const auto found = definitions.find(key);
                if (found != definitions.end() && found->second.size() == 1) {
                    resolvedIndex = found->second.front();
                    break;
                }
            }
            if (!resolvedIndex) {
                if (declaredBlackBox(parent, instance, options)) {
                    instance.resolved = true;
                    instance.external = true;
                    continue;
                }
                addViolation(result, "STRUCT-UNRESOLVED-INSTANCE", Severity::Info,
                             "Unresolved instance target: " + instance.target,
                             "The instantiated design unit was not found in the analyzed inputs.",
                             "Add the defining HDL file or configure the target as a library/black box.",
                             instance.location.file, instance.location.line,
                             instance.location.column, parent.name);
                continue;
            }
            instance.resolved = true;
            auto& target = result.structure[*resolvedIndex];
            target.topLevel = false;
            std::set<std::string> checkedParameters;
            for (const auto& override : instance.namedParameterOverrides) {
                const auto key = target.kind == parser::DesignUnitKind::VhdlEntity ?
                    lower(override.formal) : override.formal;
                if (!checkedParameters.insert(key).second) continue;
                const bool exists = std::ranges::any_of(target.parameters,
                    [&](const parser::ParameterInfo& parameter) {
                        return target.kind == parser::DesignUnitKind::VhdlEntity ?
                            lower(parameter.name) == key : parameter.name == key;
                    });
                if (exists) continue;
                const auto definition = findRule("STRUCT-UNKNOWN-NAMED-PARAMETER");
                addViolation(result, "STRUCT-UNKNOWN-NAMED-PARAMETER",
                    definition->severity,
                    "Instance " + instance.name + " overrides unknown parameter " +
                        override.formal + " on " + target.name,
                    std::string(definition->description),
                    std::string(definition->recommendation),
                    override.location.file, override.location.line, override.location.column,
                    parent.name, override.formal);
            }
            const auto parameterKey = [&](const std::string& name) {
                return target.kind == parser::DesignUnitKind::VhdlEntity ? lower(name) : name;
            };
            const auto positionalParameterCount = instance.hasPositionalParameterOverrides ?
                instance.parameterOverrideCount - instance.namedParameterOverrides.size() : 0;
            if (parent.kind == parser::DesignUnitKind::VhdlEntity &&
                positionalParameterCount > 0) {
                std::set<std::string> checkedOverlap;
                for (const auto& override : instance.namedParameterOverrides) {
                    const auto key = parameterKey(override.formal);
                    if (!checkedOverlap.insert(key).second) continue;
                    const auto found = std::ranges::find_if(target.parameters,
                        [&](const parser::ParameterInfo& parameter) {
                            return parameterKey(parameter.name) == key;
                        });
                    if (found == target.parameters.end() ||
                        static_cast<std::size_t>(found - target.parameters.begin()) >=
                            positionalParameterCount) continue;
                    const auto definition = findRule("STRUCT-DUPLICATE-NAMED-PARAMETER");
                    addViolation(result, "STRUCT-DUPLICATE-NAMED-PARAMETER",
                        definition->severity,
                        "Instance " + instance.name + " associates parameter both positionally "
                            "and by name: " + override.formal,
                        std::string(definition->description),
                        std::string(definition->recommendation),
                        override.location.file, override.location.line,
                        override.location.column, parent.name, override.formal);
                }
            }
            if (positionalParameterCount > target.parameters.size()) {
                const auto definition = findRule("STRUCT-POSITIONAL-PARAMETER-COUNT");
                addViolation(result, "STRUCT-POSITIONAL-PARAMETER-COUNT",
                    definition->severity,
                    "Instance " + instance.name + " has " +
                        std::to_string(positionalParameterCount) +
                        " positional parameter override(s), but " + target.name + " declares " +
                        std::to_string(target.parameters.size()) + " parameter(s)",
                    std::string(definition->description),
                    std::string(definition->recommendation),
                    instance.location.file, instance.location.line,
                    instance.location.column, parent.name);
            }
            for (std::size_t parameterIndex = 0;
                 parameterIndex < target.parameters.size(); ++parameterIndex) {
                const auto& parameter = target.parameters[parameterIndex];
                if (parameter.hasDefault || parameterIndex < positionalParameterCount) continue;
                const auto key = parameterKey(parameter.name);
                const bool named = std::ranges::any_of(instance.namedParameterOverrides,
                    [&](const parser::NamedParameterOverrideInfo& override) {
                        return parameterKey(override.formal) == key;
                    });
                if (named) continue;
                const auto definition = findRule("STRUCT-MISSING-REQUIRED-PARAMETER");
                addViolation(result, "STRUCT-MISSING-REQUIRED-PARAMETER",
                    definition->severity,
                    "Instance " + instance.name + " does not override required parameter " +
                        parameter.name + " on " + target.name,
                    std::string(definition->description),
                    std::string(definition->recommendation),
                    instance.location.file, instance.location.line,
                    instance.location.column, parent.name, parameter.name);
            }
            std::set<std::string> checkedFormals;
            for (const auto& connection : instance.namedConnections) {
                const auto key = target.kind == parser::DesignUnitKind::VhdlEntity ?
                    lower(connection.formal) : connection.formal;
                if (!checkedFormals.insert(key).second) continue;
                const bool exists = std::ranges::any_of(target.ports,
                    [&](const parser::PortInfo& port) {
                        return target.kind == parser::DesignUnitKind::VhdlEntity ?
                            lower(port.name) == key : port.name == key;
                    });
                if (exists || target.ports.empty()) continue;
                const auto definition = findRule("STRUCT-UNKNOWN-NAMED-PORT");
                addViolation(result, "STRUCT-UNKNOWN-NAMED-PORT", definition->severity,
                    "Instance " + instance.name + " connects unknown port " +
                        connection.formal + " on " + target.name,
                    std::string(definition->description),
                    std::string(definition->recommendation),
                    connection.location.file, connection.location.line,
                    connection.location.column, parent.name, connection.formal);
            }
            if (parent.kind == parser::DesignUnitKind::VhdlEntity &&
                !instance.positionalConnections.empty()) {
                std::set<std::string> checkedOverlap;
                for (const auto& connection : instance.namedConnections) {
                    const auto key = target.kind == parser::DesignUnitKind::VhdlEntity ?
                        lower(connection.formal) : connection.formal;
                    if (!checkedOverlap.insert(key).second) continue;
                    const auto found = std::ranges::find_if(target.ports,
                        [&](const parser::PortInfo& port) {
                            return target.kind == parser::DesignUnitKind::VhdlEntity ?
                                lower(port.name) == key : port.name == key;
                        });
                    if (found == target.ports.end() ||
                        static_cast<std::size_t>(found - target.ports.begin()) >=
                            instance.positionalConnections.size()) continue;
                    const auto definition = findRule("STRUCT-DUPLICATE-NAMED-PORT");
                    addViolation(result, "STRUCT-DUPLICATE-NAMED-PORT",
                        definition->severity,
                        "Instance " + instance.name + " associates port both positionally "
                            "and by name: " + connection.formal,
                        std::string(definition->description),
                        std::string(definition->recommendation),
                        connection.location.file, connection.location.line,
                        connection.location.column, parent.name, connection.formal);
                }
            }
            if (instance.hasPortConnectionList && !instance.hasWildcardConnection &&
                !target.ports.empty()) {
                const auto portKey = [&](const std::string& name) {
                    return target.kind == parser::DesignUnitKind::VhdlEntity ? lower(name) : name;
                };
                for (std::size_t portIndex = 0; portIndex < target.ports.size(); ++portIndex) {
                    const auto& port = target.ports[portIndex];
                    const auto key = portKey(port.name);
                    bool connected = false;
                    const core::SourceLocation* disconnected = nullptr;
                    if (portIndex < instance.positionalConnections.size()) {
                        const auto& positional = instance.positionalConnections[portIndex];
                        connected = positional.actualConnected;
                        if (!connected) disconnected = &positional.location;
                    }
                    for (const auto& connection : instance.namedConnections) {
                        if (portKey(connection.formal) != key) continue;
                        if (connection.actualConnected) connected = true;
                        else if (!disconnected) disconnected = &connection.location;
                    }
                    if (connected) continue;
                    const auto& issueLocation = disconnected ? *disconnected : instance.location;
                    if (port.direction == parser::PortDirection::Input && !port.hasDefault) {
                        const auto definition = findRule("STRUCT-UNCONNECTED-INPUT-PORT");
                        addViolation(result, "STRUCT-UNCONNECTED-INPUT-PORT", definition->severity,
                            "Instance " + instance.name + " leaves required input unconnected: " +
                                port.name,
                            std::string(definition->description),
                            std::string(definition->recommendation),
                            issueLocation.file, issueLocation.line, issueLocation.column,
                            parent.name, port.name);
                    } else if (port.direction == parser::PortDirection::Output ||
                               port.direction == parser::PortDirection::Inout) {
                        const auto definition = findRule("STRUCT-UNCONNECTED-OUTPUT-PORT");
                        addViolation(result, "STRUCT-UNCONNECTED-OUTPUT-PORT", definition->severity,
                            "Instance " + instance.name + " leaves output or inout unconnected: " +
                                port.name,
                            std::string(definition->description),
                            std::string(definition->recommendation),
                            issueLocation.file, issueLocation.line, issueLocation.column,
                            parent.name, port.name);
                    }
                }
            }
            if (instance.hasPortConnectionList && !target.ports.empty() &&
                instance.positionalConnections.size() > target.ports.size()) {
                const auto definition = findRule("STRUCT-POSITIONAL-PORT-COUNT");
                addViolation(result, "STRUCT-POSITIONAL-PORT-COUNT", definition->severity,
                    "Instance " + instance.name + " has " +
                        std::to_string(instance.positionalConnections.size()) +
                        " positional connection(s), but " + target.name + " declares " +
                        std::to_string(target.ports.size()) + " port(s)",
                    std::string(definition->description),
                    std::string(definition->recommendation),
                    instance.location.file, instance.location.line,
                    instance.location.column, parent.name);
            }
            graph[parentIndex].push_back({*resolvedIndex, &instance});
        }
    }

    std::vector<int> color(result.structure.size(), 0);
    std::function<void(std::size_t)> visit = [&](std::size_t node) {
        color[node] = 1;
        for (const auto& edge : graph[node]) {
            if (color[edge.target] == 0) {
                visit(edge.target);
            } else if (color[edge.target] == 1) {
                const auto& parent = result.structure[node];
                addViolation(result, "STRUCT-RECURSIVE-INSTANCE", Severity::Error,
                             "Recursive instance hierarchy: " + edge.instance->target,
                             "The instance creates a cycle in the design-unit hierarchy.",
                             "Break the recursive module/entity instantiation chain.",
                             edge.instance->location.file, edge.instance->location.line,
                             edge.instance->location.column, parent.name);
            }
        }
        color[node] = 2;
    };
    for (std::size_t i = 0; i < result.structure.size(); ++i) {
        if (color[i] == 0) visit(i);
    }
}

void mergeTimingInfo(AnalysisResult& result, parser::UnitTimingInfo timing) {
    std::vector<parser::DesignUnitInfo*> matches;
    for (auto& candidate : result.structure) {
        if (candidate.kind != timing.kind) continue;
        const bool sameName = candidate.kind == parser::DesignUnitKind::VhdlEntity ?
            equalInsensitive(candidate.name, timing.designUnit) :
            candidate.name == timing.designUnit;
        if (sameName) matches.push_back(&candidate);
    }
    if (matches.size() != 1) return;
    auto& unit = *matches.front();
    const auto sameSignal = [&](std::string_view left, std::string_view right) {
        return unit.kind == parser::DesignUnitKind::VhdlEntity ?
            equalInsensitive(left, right) : left == right;
    };
    for (auto& signal : timing.scalarSignals) {
        const auto duplicate = std::ranges::any_of(
            unit.scalarSignals, [&](const std::string& existing) {
                return sameSignal(existing, signal);
            });
        if (!duplicate) unit.scalarSignals.push_back(std::move(signal));
    }
    for (auto& clock : timing.clocks) {
        const auto duplicate = std::ranges::any_of(
            unit.clocks, [&](const parser::ClockInfo& existing) {
                return sameSignal(existing.signal, clock.signal) &&
                       existing.risingEdge == clock.risingEdge;
            });
        if (!duplicate) unit.clocks.push_back(std::move(clock));
    }
    for (auto& reset : timing.resets) {
        const auto existing = std::ranges::find_if(
            unit.resets, [&](const parser::ResetInfo& value) {
                return sameSignal(value.signal, reset.signal);
            });
        if (existing == unit.resets.end()) {
            unit.resets.push_back(std::move(reset));
            continue;
        }
        const bool polarityConflict =
            (reset.activeHighObserved && existing->activeLowObserved) ||
            (reset.activeLowObserved && existing->activeHighObserved);
        existing->synchronousObserved = existing->synchronousObserved ||
                                        reset.synchronousObserved;
        existing->asynchronousObserved = existing->asynchronousObserved ||
                                         reset.asynchronousObserved;
        existing->activeHighObserved = existing->activeHighObserved ||
                                       reset.activeHighObserved;
        existing->activeLowObserved = existing->activeLowObserved ||
                                      reset.activeLowObserved;
        if (reset.asynchronous) {
            existing->asynchronous = true;
            existing->activeHigh = reset.activeHigh;
        }
        if (reset.asynchronous || polarityConflict) {
            existing->location = std::move(reset.location);
        }
    }
    for (auto& process : timing.sequentialProcesses) {
        const auto duplicate = std::ranges::any_of(
            unit.sequentialProcesses,
            [&](const parser::SequentialProcessInfo& existing) {
                return sameSignal(existing.clock, process.clock) &&
                       existing.risingEdge == process.risingEdge &&
                       existing.location.file.lexically_normal() ==
                           process.location.file.lexically_normal() &&
                       existing.location.line == process.location.line &&
                       existing.location.column == process.location.column;
            });
        if (!duplicate) unit.sequentialProcesses.push_back(std::move(process));
    }
}

// Expand Verilog once in the caller's source order, for generic and standard analysis.
// Every later consumer receives the same immutable per-root result.
std::vector<std::optional<parser::FrontendPreprocessorResult>> prepareFrontendInputs(
    std::span<const core::SourceFile> sources,const AnalysisOptions& options) {
    std::vector<std::optional<parser::FrontendPreprocessorResult>> inputs(sources.size());
    if(options.countOnly) return inputs;
    parser::FrontendPreprocessorSession session;
    for(std::size_t i=0;i<sources.size();++i) {
        if(sources[i].language()!=core::HdlLanguage::Verilog &&
           sources[i].language()!=core::HdlLanguage::SystemVerilog) continue;
        inputs[i]=session.expand(sources[i],{options.includeDirectories,options.defines});
    }
    return inputs;
}

} // namespace

Analyzer::Analyzer(AnalysisOptions options) : options_(std::move(options)) {}

std::string_view severityName(Severity severity) noexcept {
    switch (severity) {
    case Severity::Critical: return "critical";
    case Severity::Error: return "error";
    case Severity::Warning: return "warning";
    case Severity::Info: return "info";
    }
    return "unknown";
}

core::HdlLanguage Analyzer::languageFromPath(const std::filesystem::path& path) noexcept {
    const auto extension = lower(path.extension().string());
    if (extension == ".sv" || extension == ".svh") return core::HdlLanguage::SystemVerilog;
    if (extension == ".v" || extension == ".vh") return core::HdlLanguage::Verilog;
    if (extension == ".vhd" || extension == ".vhdl") return core::HdlLanguage::Vhdl;
    return core::HdlLanguage::Unknown;
}

AnalysisResult Analyzer::analyzeSource(const core::SourceFile& source) const {
    const auto inputs=prepareFrontendInputs(std::span<const core::SourceFile>(&source,1),options_);
    return analyzeSourceImpl(source,true,inputs[0] ? &*inputs[0] : nullptr);
}

AnalysisResult Analyzer::analyzeSourceImpl(const core::SourceFile& source,
                                           bool runTimingRules,
                                           const parser::FrontendPreprocessorResult* frontend,
                                           std::vector<parser::DesignUnitInfo>* sourceBoundStructure) const {
    AnalysisResult result;
    result.severityOverrides = options_.severityOverrides;
    result.standardId = options_.standardId;
    result.verilogHeaderPolicy = options_.verilogHeaderPolicy;
    result.verilogKeywordPolicy = options_.verilogKeywordPolicy;
    result.powerOnPolicy = options_.powerOnPolicy;
    if (source.language() == core::HdlLanguage::Unknown) {
        result.errors.push_back("Unsupported HDL file extension: " + source.path().string());
        return result;
    }
    result.filesAnalyzed = 1;
    result.lineStatistics.append(countSourceLines(source));
    if (options_.countOnly) return result;
    const auto preprocessed = parser::preprocess(
        source, {options_.includeDirectories, options_.defines},frontend);
    const auto profile=findStandardProfile(options_.standardId);
    const bool needsSourceBound=profile && profile->id=="GJB10157-2021";
    AnalysisResult boundModel;
    if(!frontend || needsSourceBound) {
        boundModel.structure=parser::parseDesignUnits(preprocessed.source);
        for(auto& timing:parser::extractTimingInfo(preprocessed.source)) mergeTimingInfo(boundModel,std::move(timing));
    }
    std::vector<parser::DesignUnitInfo> boundUnits;
    std::optional<parser::FrontendSourceView> sourceView;
    const core::SourceFile* modelInput=&preprocessed.source;
    bool modelAdmitted=preprocessed.issues.empty();
    if(frontend) {
        sourceView.emplace(source,*frontend);
        modelInput=&sourceView->input();
        modelAdmitted=sourceView->admitted();
        if(modelAdmitted) {
            // Merge while positions are still virtual: separate expanded
            // processes can later report on the same physical invocation line.
            result.structure=parser::parseDesignUnits(*modelInput);
            for(auto& timing:parser::extractTimingInfo(*modelInput)) mergeTimingInfo(result,std::move(timing));
            for(auto& unit:result.structure) unit=sourceView->report(std::move(unit));
        }
        boundUnits=std::move(boundModel.structure);
    } else {
        result.structure=std::move(boundModel.structure);
        if(needsSourceBound) boundUnits=result.structure;
    }
    if(sourceBoundStructure)
        sourceBoundStructure->insert(sourceBoundStructure->end(),
            std::make_move_iterator(boundUnits.begin()),std::make_move_iterator(boundUnits.end()));
    if (runTimingRules) {
        analyzeTimingConsistency(result);
        analyzeUnusedAsynchronousResets(result);
        analyzeMultipleClocksPerProcess(result);
        analyzeDirectClockDomainCrossings(result);
        analyzeResetDomainCrossings(result);
    }
    result.designUnits = result.structure.size();
    const auto& preprocessingIssues=frontend && !needsSourceBound && frontend->issues.empty()
        ? frontend->issues : preprocessed.issues;
    for (const auto& issue : preprocessingIssues) {
        addViolation(result, issue.ruleId, Severity::Error, issue.message,
                     "The HDL preprocessor could not resolve the source configuration reliably.",
                     "Correct the directive or configure the required macro/include directory.",
                     issue.location.file, issue.location.line, issue.location.column);
    }
    const bool physicalSyntaxFallback=frontend && !modelAdmitted &&
        std::ranges::none_of(parser::tokenize(source),[](const auto& token){return token.text=="`";});
    const auto syntaxIssues=modelAdmitted ? parser::checkSyntax(*modelInput) :
        physicalSyntaxFallback ? parser::checkSyntax(source) : std::vector<parser::SyntaxIssue>{};
    for (const auto& issue : syntaxIssues) {
        const auto at=sourceView && !physicalSyntaxFallback ? sourceView->report(issue.location) : issue.location;
        addViolation(result, issue.ruleId, Severity::Error, issue.message,
                     "The HDL source is structurally malformed, so later lint results may be incomplete.",
                     "Correct the source syntax at this location and run the analyzer again.",
                     at.file, at.line, at.column);
    }
    const auto semanticIssues=modelAdmitted ? parser::analyzeSemantics(*modelInput) : std::vector<parser::SemanticIssue>{};
    for (const auto& issue : semanticIssues) {
        const auto at=sourceView ? sourceView->report(issue.location) : issue.location;
        const auto definition = findRule(issue.ruleId);
        const auto severity = definition ? definition->severity : Severity::Warning;
        addViolation(result, issue.ruleId, severity, issue.message,
                     issue.description, issue.recommendation, at.file,
                     at.line, at.column, issue.designUnit, issue.signal);
    }
    const auto rawLines = splitLines(source.content());
    analyzeCommon(source, rawLines, result);
    applySeverityOverrides(result, options_);
    const std::array<const parser::FrontendPreprocessorResult*,1> inputs{frontend};
    const auto waiverViews=waiverInputViews(std::span<const core::SourceFile>(&source,1),inputs);
    filterViolations(result,waiverViews,options_);
    if (runTimingRules) {
        applyStandardProfile(result,std::span<const core::SourceFile>(&source,1),options_,inputs,std::move(boundUnits));
    }
    std::ranges::sort(result.violations, [](const Violation& a, const Violation& b) {
        return std::tie(a.file, a.line, a.column, a.ruleId) <
               std::tie(b.file, b.line, b.column, b.ruleId);
    });
    return result;
}

AnalysisResult Analyzer::analyzePaths(const std::vector<std::filesystem::path>& inputs) const {
    std::vector<std::string> inputErrors;
    std::vector<std::filesystem::path> files;
    std::set<std::filesystem::path> seenFiles;
    const auto appendFile=[&](const std::filesystem::path& file) {
        if(seenFiles.insert(file).second) files.push_back(file);
    };
    for (const auto& input : inputs) {
        std::error_code error;
        if (std::filesystem::is_regular_file(input, error)) {
            if (languageFromPath(input) != core::HdlLanguage::Unknown && !matchesExclude(input, options_)) {
                appendFile(input);
            }
        } else if (std::filesystem::is_directory(input, error)) {
            std::vector<std::filesystem::path> directoryFiles;
            std::filesystem::recursive_directory_iterator iterator(
                input, std::filesystem::directory_options::skip_permission_denied, error);
            for (const auto& entry : iterator) {
                if (entry.is_regular_file(error) &&
                    languageFromPath(entry.path()) != core::HdlLanguage::Unknown &&
                    !matchesExclude(entry.path(), options_)) {
                    directoryFiles.push_back(entry.path());
                }
            }
            // Directory enumeration is deterministic; explicit roots/file-list
            // order must remain intact because source macros persist by order.
            std::ranges::sort(directoryFiles);
            for(const auto& file:directoryFiles) appendFile(file);
        } else {
            inputErrors.push_back("Input does not exist or is not readable: " + input.string());
        }
    }
    std::vector<core::SourceFile> sources;
    sources.reserve(files.size());
    for (const auto& file : files) {
        std::ifstream stream(file, std::ios::binary);
        if (!stream) {
            inputErrors.push_back("Cannot read HDL file: " + file.string());
            continue;
        }
        std::ostringstream buffer;
        buffer << stream.rdbuf();
        sources.emplace_back(file, languageFromPath(file), buffer.str());
    }
    auto combined = analyzeSources(sources);
    combined.errors.insert(combined.errors.end(),
                           std::make_move_iterator(inputErrors.begin()),
                           std::make_move_iterator(inputErrors.end()));
    return combined;
}

AnalysisResult Analyzer::analyzeSources(const std::vector<core::SourceFile>& sources) const {
    AnalysisResult combined;
    combined.severityOverrides = options_.severityOverrides;
    combined.verilogHeaderPolicy = options_.verilogHeaderPolicy;
    combined.verilogKeywordPolicy = options_.verilogKeywordPolicy;
    combined.powerOnPolicy = options_.powerOnPolicy;
    combined.standardId = options_.standardId;
    const auto frontendInputs=prepareFrontendInputs(sources,options_);
    std::vector<const parser::FrontendPreprocessorResult*> frontendViews;
    frontendViews.reserve(sources.size());
    for(const auto& input:frontendInputs) frontendViews.push_back(input ? &*input : nullptr);
    std::vector<parser::DesignUnitInfo> sourceBoundStructure;
    std::vector<core::SourceFile> preprocessedVhdlSources;
    for (std::size_t index=0;index<sources.size();++index) {
        const auto& source=sources[index];
        auto result = analyzeSourceImpl(source,false,frontendViews[index],&sourceBoundStructure);
        combined.filesAnalyzed += result.filesAnalyzed;
        for (const auto& file : result.lineStatistics.files) combined.lineStatistics.append(file);
        combined.designUnits += result.designUnits;
        combined.structure.insert(combined.structure.end(),
                                  std::make_move_iterator(result.structure.begin()),
                                  std::make_move_iterator(result.structure.end()));
        combined.violations.insert(combined.violations.end(),
                                   std::make_move_iterator(result.violations.begin()),
                                   std::make_move_iterator(result.violations.end()));
        combined.errors.insert(combined.errors.end(),
                               std::make_move_iterator(result.errors.begin()),
                               std::make_move_iterator(result.errors.end()));
        combined.suppressedViolations += result.suppressedViolations;
        if (!options_.countOnly && source.language() == core::HdlLanguage::Vhdl) {
            preprocessedVhdlSources.push_back(parser::preprocess(
                source, {options_.includeDirectories, options_.defines}).source);
        }
    }
    if (options_.countOnly) return combined;
    for (const auto& source : preprocessedVhdlSources) {
        for (auto& timing : parser::extractTimingInfo(source)) {
            mergeTimingInfo(combined, std::move(timing));
        }
    }
    analyzeTimingConsistency(combined);
    analyzeUnusedAsynchronousResets(combined);
    analyzeMultipleClocksPerProcess(combined);
    analyzeDirectClockDomainCrossings(combined);
    analyzeResetDomainCrossings(combined);
    for (const auto& issue :
         parser::analyzeUnusedVhdlGenerics(preprocessedVhdlSources)) {
        const auto definition = findRule(issue.ruleId);
        const auto severity = definition ? definition->severity : Severity::Warning;
        addViolation(combined, issue.ruleId, severity, issue.message,
                     issue.description, issue.recommendation, issue.location.file,
                     issue.location.line, issue.location.column, issue.designUnit,
                     issue.signal);
    }
    combined.designUnits = combined.structure.size();
    analyzeStructure(combined, options_);
    // Resolve the physical-source adapter model in the same project scope;
    // these binding facts are separate from its discarded generic findings.
    AnalysisResult boundProject;
    boundProject.structure=std::move(sourceBoundStructure);
    analyzeStructure(boundProject,options_);
    applySeverityOverrides(combined, options_);
    const auto waiverViews=waiverInputViews(sources,frontendViews);
    filterViolations(combined,waiverViews,options_);
    applyStandardProfile(combined,sources,options_,frontendViews,std::move(boundProject.structure));
    std::ranges::sort(combined.violations, [](const Violation& a, const Violation& b) {
        return std::tie(a.file, a.line, a.column, a.ruleId) <
               std::tie(b.file, b.line, b.column, b.ruleId);
    });
    return combined;
}

} // namespace fpga::analysis
