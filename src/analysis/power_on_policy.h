#pragma once
#include <optional>
#include <string_view>

namespace fpga::analysis {
enum class FpgaTechnology { Unspecified, Sram, NonSram };
inline std::string_view fpgaTechnologyName(FpgaTechnology value) {
    switch (value) {
    case FpgaTechnology::Sram: return "sram";
    case FpgaTechnology::NonSram: return "non-sram";
    default: return "unspecified";
    }
}
inline std::optional<FpgaTechnology> parseFpgaTechnology(std::string_view value) {
    if (value == "sram") return FpgaTechnology::Sram;
    if (value == "non-sram") return FpgaTechnology::NonSram;
    if (value == "unspecified") return FpgaTechnology::Unspecified;
    return std::nullopt;
}
struct PowerOnPolicy {
    FpgaTechnology technology{FpgaTechnology::Unspecified};
    // Unknown vendor default is distinct from either configured setting.
    std::optional<bool> acceptInitialBlocks;
};
} // namespace fpga::analysis
