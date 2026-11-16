#pragma once

#include "core/source.h"

#include <cstddef>
#include <filesystem>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace fpga::design {

enum class PortDirection { Input, Output, Inout };
enum class SignalType { Wire, Reg, Logic, Integer, Other };
enum class AlwaysKind { Combinational, Sequential, Latch, Unknown };

struct Driver {
    std::string kind;
    std::string sourceExpression;
    core::SourceLocation location;
};

struct Load {
    std::string kind;
    std::string destination;
    core::SourceLocation location;
};

struct Clock {
    std::string name;
    std::string signal;
    std::optional<double> periodNanoseconds;
    core::SourceLocation location;
};

struct Reset {
    std::string name;
    std::string signal;
    bool activeHigh{true};
    bool asynchronous{false};
    core::SourceLocation location;
};

class Signal {
public:
    Signal(std::string name, std::size_t width, bool isSigned, SignalType type,
           std::string module, core::SourceLocation location)
        : name_(std::move(name)), width_(width), signed_(isSigned), type_(type),
          module_(std::move(module)), location_(std::move(location)) {}

    [[nodiscard]] const std::string& name() const noexcept { return name_; }
    [[nodiscard]] std::size_t width() const noexcept { return width_; }
    [[nodiscard]] bool isSigned() const noexcept { return signed_; }
    [[nodiscard]] SignalType type() const noexcept { return type_; }
    [[nodiscard]] const std::string& module() const noexcept { return module_; }
    [[nodiscard]] const core::SourceLocation& location() const noexcept { return location_; }
    [[nodiscard]] const std::vector<Driver>& drivers() const noexcept { return drivers_; }
    [[nodiscard]] const std::vector<Load>& loads() const noexcept { return loads_; }
    [[nodiscard]] const std::optional<std::string>& clockDomain() const noexcept {
        return clockDomain_;
    }
    [[nodiscard]] const std::optional<std::string>& resetDomain() const noexcept {
        return resetDomain_;
    }

    void addDriver(Driver driver) { drivers_.push_back(std::move(driver)); }
    void addLoad(Load load) { loads_.push_back(std::move(load)); }
    void setClockDomain(std::optional<std::string> domain) { clockDomain_ = std::move(domain); }
    void setResetDomain(std::optional<std::string> domain) { resetDomain_ = std::move(domain); }

private:
    std::string name_;
    std::size_t width_{1};
    bool signed_{false};
    SignalType type_{SignalType::Wire};
    std::string module_;
    core::SourceLocation location_;
    std::vector<Driver> drivers_;
    std::vector<Load> loads_;
    std::optional<std::string> clockDomain_;
    std::optional<std::string> resetDomain_;
};

struct Port {
    std::string name;
    PortDirection direction{PortDirection::Input};
    std::size_t width{1};
    bool isSigned{false};
    std::string signal;
    core::SourceLocation location;
};

struct Parameter {
    std::string name;
    std::string value;
    std::optional<std::string> type;
    core::SourceLocation location;
};

struct Instance {
    std::string name;
    std::string moduleType;
    std::string parentModule;
    core::SourceLocation location;
};

struct Assignment {
    std::string target;
    std::string expression;
    bool continuous{true};
    core::SourceLocation location;
};

struct AlwaysBlock {
    AlwaysKind kind{AlwaysKind::Unknown};
    std::vector<std::string> sensitivityList;
    std::vector<Assignment> assignments;
    core::SourceLocation location;
};

class Module {
public:
    Module(std::string name, std::filesystem::path sourceFile,
           core::SourceLocation location = {})
        : name_(std::move(name)), sourceFile_(std::move(sourceFile)),
          location_(std::move(location)) {}

    [[nodiscard]] const std::string& name() const noexcept { return name_; }
    [[nodiscard]] const std::filesystem::path& sourceFile() const noexcept { return sourceFile_; }
    [[nodiscard]] const core::SourceLocation& location() const noexcept { return location_; }
    [[nodiscard]] const std::vector<Port>& ports() const noexcept { return ports_; }
    [[nodiscard]] const std::vector<Signal>& signals() const noexcept { return signals_; }
    [[nodiscard]] const std::vector<Parameter>& parameters() const noexcept { return parameters_; }
    [[nodiscard]] const std::vector<Instance>& instances() const noexcept { return instances_; }
    [[nodiscard]] const std::vector<Assignment>& assignments() const noexcept { return assignments_; }
    [[nodiscard]] const std::vector<AlwaysBlock>& alwaysBlocks() const noexcept { return alwaysBlocks_; }
    [[nodiscard]] const std::vector<Clock>& clocks() const noexcept { return clocks_; }
    [[nodiscard]] const std::vector<Reset>& resets() const noexcept { return resets_; }

    void addPort(Port value) { ports_.push_back(std::move(value)); }
    void addSignal(Signal value) { signals_.push_back(std::move(value)); }
    void addParameter(Parameter value) { parameters_.push_back(std::move(value)); }
    void addInstance(Instance value) { instances_.push_back(std::move(value)); }
    void addAssignment(Assignment value) { assignments_.push_back(std::move(value)); }
    void addAlwaysBlock(AlwaysBlock value) { alwaysBlocks_.push_back(std::move(value)); }
    void addClock(Clock value) { clocks_.push_back(std::move(value)); }
    void addReset(Reset value) { resets_.push_back(std::move(value)); }

private:
    std::string name_;
    std::filesystem::path sourceFile_;
    core::SourceLocation location_;
    std::vector<Port> ports_;
    std::vector<Signal> signals_;
    std::vector<Parameter> parameters_;
    std::vector<Instance> instances_;
    std::vector<Assignment> assignments_;
    std::vector<AlwaysBlock> alwaysBlocks_;
    std::vector<Clock> clocks_;
    std::vector<Reset> resets_;
};

} // namespace fpga::design
