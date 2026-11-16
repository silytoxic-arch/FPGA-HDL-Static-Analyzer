#pragma once

#include "analysis/verilog_naming_shape.h"

namespace fpga::analysis {

// 5.2.4 run ev_a9ff5e1f8c558a95613fe54cd4ca2bccb6312f68c4803bd832692c1ea973d5a0
// selects six declaration types, NOT FUNCTION itself; recursively selected
// function/task signal descendants still participate. Literal bytes and the
// lowercase + equal-length memcmp chain are recorded in pass3/104..107.
[[nodiscard]] inline std::optional<bool> checkVerilogPowerName(
    std::string_view name, VerilogNamingShapeSubject subject) {
    if (subject == VerilogNamingShapeSubject::Unsupported ||
        subject == VerilogNamingShapeSubject::Function) return std::nullopt;
    std::string ascii;
    for (unsigned char c : name) {
        if (c >= 128) return std::nullopt; // Native tolower locale unestablished.
        ascii += static_cast<char>(c >= 'A' && c <= 'Z' ? c + ('a' - 'A') : c);
    }
    return ascii == "vdd" || ascii == "vss" || ascii == "vcc" ||
           ascii == "gnd" || ascii == "vref";
}
} // namespace fpga::analysis
