#pragma once
#include <string_view>

namespace fpga {
// Shared application metadata. The core has no Qt or GUI dependency.
struct ApplicationInfo {
    std::string_view name;
    std::string_view version;
    std::string_view stage;
};
[[nodiscard]] ApplicationInfo application_info() noexcept;
} // namespace fpga
