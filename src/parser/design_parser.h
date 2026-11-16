#pragma once

#include "core/source.h"

#include <string>
#include <vector>

namespace fpga::parser {

enum class HdlTokenKind { Identifier, Number, Symbol, String };

struct HdlToken {
    HdlTokenKind kind;
    std::string text;
    std::size_t line;
    std::size_t column;
    std::size_t offset;
};

[[nodiscard]] std::vector<HdlToken> tokenize(const core::SourceFile& source);

enum class DesignUnitKind { VerilogModule, VhdlEntity };

enum class PortDirection { Input, Output, Inout, Unknown };

struct PortInfo {
    std::string name;
    PortDirection direction{PortDirection::Unknown};
    core::SourceLocation location;
    bool hasDefault{false};
};

struct NamedPortConnectionInfo {
    std::string formal;
    core::SourceLocation location;
    bool actualConnected{true};
    bool actualExpression{false};
    core::SourceLocation actualLocation{};
    // Populated only when the complete actual is one plain identifier.
    // Empty means open, expression, selected/indexed name, or unresolved.
    std::string actualIdentifier;
};

struct PositionalPortConnectionInfo {
    core::SourceLocation location;
    bool actualConnected{true};
    bool actualExpression{false};
    std::string actualIdentifier;
};

struct ParameterInfo {
    std::string name;
    core::SourceLocation location;
    bool hasDefault{false};
};

struct NamedParameterOverrideInfo {
    std::string formal;
    core::SourceLocation location;
};

struct DuplicateDeclarationInfo {
    std::string name;
    core::SourceLocation location;
};

struct InstanceInfo {
    std::string name;
    std::string target;
    core::SourceLocation location;
    bool resolved{false};
    bool external{false};
    std::vector<NamedPortConnectionInfo> namedConnections;
    std::vector<PositionalPortConnectionInfo> positionalConnections;
    std::size_t portConnectionCount{0};
    bool hasPortConnectionList{false};
    bool hasNamedConnections{false};
    bool hasPositionalConnections{false};
    bool hasWildcardConnection{false};
    std::vector<NamedParameterOverrideInfo> namedParameterOverrides;
    std::size_t parameterOverrideCount{0};
    bool hasNamedParameterOverrides{false};
    bool hasPositionalParameterOverrides{false};
    bool invalidPortAssociationOrder{false};
    bool invalidParameterAssociationOrder{false};
};

struct ClockInfo {
    std::string signal;
    bool risingEdge{true};
    core::SourceLocation location;
};

struct ResetInfo {
    std::string signal;
    bool activeHigh{true};
    bool asynchronous{false};
    core::SourceLocation location;
    bool synchronousObserved{false};
    bool asynchronousObserved{false};
    bool activeHighObserved{false};
    bool activeLowObserved{false};
};

struct ProcessResetInfo {
    std::string signal;
    bool activeHigh{true};
    bool asynchronous{false};
    core::SourceLocation location;
};

struct RegisterTransferInfo {
    std::string destination;
    std::string source;
};

struct CdcSynchronizerInfo {
    std::string sourceSignal;
    std::string firstStage;
    std::string secondStage;
    std::string destinationClock;
    bool risingEdge{true};
    core::SourceLocation location;
};

// A bounded VHDL concurrent signal transfer used by hierarchy-aware rules.
// Only direct architecture-level `target <= source;` and
// `target <= not source;` shapes are recorded; complex expressions and nested
// process/generate/block statements deliberately remain unresolved.
struct SimpleSignalTransferInfo {
    std::string target;
    std::string source;
    bool inverted{false};
    core::SourceLocation location;
};

struct SequentialProcessInfo {
    std::string clock;
    bool risingEdge{true};
    core::SourceLocation location;
    std::vector<std::string> reads;
    std::vector<std::string> writes;
    std::vector<std::string> indexedSignals;
    std::vector<ProcessResetInfo> resets;
    std::vector<ClockInfo> clockEvents;
    std::vector<RegisterTransferInfo> directTransfers;
    std::vector<std::string> complexReads;
};

struct DesignUnitInfo {
    std::string name;
    DesignUnitKind kind{DesignUnitKind::VerilogModule};
    core::SourceLocation location;
    std::vector<InstanceInfo> instances;
    std::vector<ClockInfo> clocks;
    std::vector<ResetInfo> resets;
    std::vector<SequentialProcessInfo> sequentialProcesses;
    std::vector<std::string> scalarSignals;
    std::vector<CdcSynchronizerInfo> cdcSynchronizers;
    bool topLevel{true};
    std::vector<PortInfo> ports;
    std::vector<ParameterInfo> parameters;
    std::vector<DuplicateDeclarationInfo> duplicatePortDeclarations;
    std::vector<DuplicateDeclarationInfo> duplicateParameterDeclarations;
    std::vector<SimpleSignalTransferInfo> simpleSignalTransfers;
};

struct SyntaxIssue {
    std::string ruleId;
    std::string message;
    core::SourceLocation location;
};

// Extracts the project structure needed by hierarchy checks. The lexer is
// comment/string aware and preserves one-based source positions. This is not
// yet a full language grammar or elaborator.
[[nodiscard]] std::vector<DesignUnitInfo>
parseDesignUnits(const core::SourceFile& source);

// Performs dependency-free lexical/structural sanity checks before lint rules
// run. It deliberately reports only high-confidence malformed constructs.
[[nodiscard]] std::vector<SyntaxIssue>
checkSyntax(const core::SourceFile& source);

[[nodiscard]] std::string_view designUnitKindName(DesignUnitKind kind) noexcept;
[[nodiscard]] std::string_view portDirectionName(PortDirection direction) noexcept;

} // namespace fpga::parser
