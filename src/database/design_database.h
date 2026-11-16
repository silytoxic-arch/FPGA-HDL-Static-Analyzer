#pragma once

#include "database/design_model.h"

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace fpga::design {

// Owns modules with unique_ptr so references returned by queries remain stable
// while other modules are inserted. Removing modules is intentionally absent,
// which makes the reference lifetime equal to the database lifetime.
class DesignDatabase {
public:
    [[nodiscard]] bool addModule(Module module);

    [[nodiscard]] std::optional<std::reference_wrapper<const Module>>
    findModule(std::string_view moduleName) const noexcept;
    [[nodiscard]] std::optional<std::reference_wrapper<const Signal>>
    findSignal(std::string_view moduleName, std::string_view signalName) const noexcept;

    [[nodiscard]] std::vector<std::reference_wrapper<const Module>> getModules() const;
    [[nodiscard]] std::vector<std::reference_wrapper<const Signal>> getSignals() const;
    [[nodiscard]] std::vector<std::reference_wrapper<const Instance>> getInstances() const;
    [[nodiscard]] std::vector<std::reference_wrapper<const Driver>> getDrivers() const;
    [[nodiscard]] std::vector<std::reference_wrapper<const Load>> getLoads() const;

private:
    std::unordered_map<std::string, std::unique_ptr<Module>> modules_;
};

} // namespace fpga::design
