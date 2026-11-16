#include "database/design_database.h"

#include <algorithm>
#include <utility>

namespace fpga::design {
namespace {
template <typename T>
void sortByName(std::vector<std::reference_wrapper<const T>>& values) {
    std::ranges::sort(values, {}, [](const T& value) { return value.name; });
}

template <>
void sortByName(std::vector<std::reference_wrapper<const Module>>& values) {
    std::ranges::sort(values, {}, [](const Module& value) { return value.name(); });
}
} // namespace

bool DesignDatabase::addModule(Module module) {
    const auto name = module.name();
    if (name.empty() || modules_.contains(name)) {
        return false;
    }
    modules_.emplace(name, std::make_unique<Module>(std::move(module)));
    return true;
}

std::optional<std::reference_wrapper<const Module>>
DesignDatabase::findModule(std::string_view moduleName) const noexcept {
    const auto found = modules_.find(std::string(moduleName));
    if (found == modules_.end()) {
        return std::nullopt;
    }
    return std::cref(*found->second);
}

std::optional<std::reference_wrapper<const Signal>>
DesignDatabase::findSignal(std::string_view moduleName,
                           std::string_view signalName) const noexcept {
    const auto module = findModule(moduleName);
    if (!module) {
        return std::nullopt;
    }
    const auto& signals = module->get().signals();
    const auto found = std::ranges::find(signals, signalName, &Signal::name);
    if (found == signals.end()) {
        return std::nullopt;
    }
    return std::cref(*found);
}

std::vector<std::reference_wrapper<const Module>> DesignDatabase::getModules() const {
    std::vector<std::reference_wrapper<const Module>> result;
    result.reserve(modules_.size());
    for (const auto& [name, module] : modules_) {
        static_cast<void>(name);
        result.emplace_back(*module);
    }
    sortByName(result);
    return result;
}

std::vector<std::reference_wrapper<const Signal>> DesignDatabase::getSignals() const {
    std::vector<std::reference_wrapper<const Signal>> result;
    for (const auto& module : getModules()) {
        for (const auto& signal : module.get().signals()) {
            result.emplace_back(signal);
        }
    }
    return result;
}

std::vector<std::reference_wrapper<const Instance>> DesignDatabase::getInstances() const {
    std::vector<std::reference_wrapper<const Instance>> result;
    for (const auto& module : getModules()) {
        for (const auto& instance : module.get().instances()) {
            result.emplace_back(instance);
        }
    }
    return result;
}

std::vector<std::reference_wrapper<const Driver>> DesignDatabase::getDrivers() const {
    std::vector<std::reference_wrapper<const Driver>> result;
    for (const auto& signal : getSignals()) {
        for (const auto& driver : signal.get().drivers()) {
            result.emplace_back(driver);
        }
    }
    return result;
}

std::vector<std::reference_wrapper<const Load>> DesignDatabase::getLoads() const {
    std::vector<std::reference_wrapper<const Load>> result;
    for (const auto& signal : getSignals()) {
        for (const auto& load : signal.get().loads()) {
            result.emplace_back(load);
        }
    }
    return result;
}

} // namespace fpga::design
