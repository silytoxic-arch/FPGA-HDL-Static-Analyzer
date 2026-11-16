#include "analysis/verilog_power_names.h"
#include <iostream>

int main() {
    using namespace fpga::analysis;
    int failures = 0;
    const auto test = [&](bool ok, const char* label) {
        if (!ok) { ++failures; std::cerr << "FAIL " << label << '\n'; }
    };
    for (auto subject : {VerilogNamingShapeSubject::Module, VerilogNamingShapeSubject::Net,
                        VerilogNamingShapeSubject::Parameter, VerilogNamingShapeSubject::Localparam,
                        VerilogNamingShapeSubject::Cell, VerilogNamingShapeSubject::Task})
        for (auto name : {"VDD", "vss", "VcC", "gNd", "VREF"})
            test(checkVerilogPowerName(name, subject) == true, "six types, five exact ASCII-folded names");
    for (auto name : {"vdd_x", "xvdd", "vref1", "vdds", "", "good"})
        test(checkVerilogPowerName(name, VerilogNamingShapeSubject::Net) == false, "no substring/prefix ban");
    test(!checkVerilogPowerName("VDD", VerilogNamingShapeSubject::Function), "function definition excluded");
    test(!checkVerilogPowerName("VDD", VerilogNamingShapeSubject::Unsupported), "unknown category unproved");
    test(!checkVerilogPowerName("éVDD", VerilogNamingShapeSubject::Net), "nonASCII locale unproved");
    return failures ? 1 : 0;
}
