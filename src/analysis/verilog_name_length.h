#pragma once

#include <cstdint>
#include <optional>
#include <string_view>

namespace fpga::analysis {

// Default policy from ctor 0x1800212e0:
// ev_72031d097e80e28af266b1d999a091e98e505f631f13006bbb16c39c3b51e52b.
// Run 0x18052d8a0 compares nonempty normalized byte length inclusively:
// ev_18d1ea59101ccfacbaf11002bc798a90887dab8fc02865c01c8d2c786def1449.
// Only an independently established native-compatible declaration category may
// invoke this helper. FUNCTION declarations are excluded by this run's type set.
struct VerilogNameLengthPolicy { int minimum{2}, maximum{32}; };

[[nodiscard]] inline std::optional<bool> checkVerilogNameLength(
    std::string_view normalizedName, VerilogNameLengthPolicy policy = {}) {
    if (normalizedName.empty()) return false;
    // FUN_18052d8a0 sign-extends each 32-bit policy value and then compares it
    // with the unsigned 64-bit byte length.  Preserve that behavior even for
    // negative and inverted externally supplied bounds.
    const auto nativeUnsigned = [](int value) {
        return static_cast<std::uint64_t>(static_cast<std::int64_t>(value));
    };
    const auto length = static_cast<std::uint64_t>(normalizedName.size());
    return length < nativeUnsigned(policy.minimum) ||
        length > nativeUnsigned(policy.maximum);
}
} // namespace fpga::analysis
