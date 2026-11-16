#include "analysis/verilog_naming_style.h"

#include <iostream>
#include <string>
#include <vector>

int main() {
    using namespace fpga::analysis;
    int failures = 0;
    const auto check = [&](bool result, const char* label) {
        if (!result) { ++failures; std::cerr << "FAIL: " << label << '\n'; }
    };
    for (const auto subject : {VerilogNamingSubject::Parameter, VerilogNamingSubject::Localparam}) {
        for (const auto* name : {"DATA", "DATA_1", "D2", "_DATA", "123", "D$T"})
            check(checkVerilogNamingStyle(name, subject) == false, "constant without ASCII lowercase");
        for (const auto* name : {"data", "Data", "dATA", "DATA_x", "1a"})
            check(checkVerilogNamingStyle(name, subject) == true, "constant with ASCII lowercase");
        check(checkVerilogNamingStyle("Data", subject, {false,true,false}) == false,
              "parameter policy disabled");
    }
    for (const auto* name : {"data", "Data", "D", "d", "Data_1", "d_2", "d$x"})
        check(checkVerilogNamingStyle(name, VerilogNamingSubject::WireLike) == false,
              "wire lowercase or only first letter uppercase");
    for (const auto* name : {"DATA", "RST", "OUT", "dataOut", "_data", "1data", "$data", "d_X"})
        check(checkVerilogNamingStyle(name, VerilogNamingSubject::WireLike) == true,
              "wire invalid first byte or later uppercase");
    for (const auto* name : {"data", "_data", "1data", "$data"})
        check(checkVerilogNamingStyle(name, VerilogNamingSubject::WireLike, {true,true,true}) == false,
              "all-lowercase alternative only rejects ASCII uppercase");
    check(checkVerilogNamingStyle("Data", VerilogNamingSubject::WireLike, {true,true,true}) == true,
          "all-lowercase alternative rejects first uppercase");
    check(checkVerilogNamingStyle("OUT", VerilogNamingSubject::WireLike, {true,false,false}) == false,
          "wire policy disabled");
    check(!checkVerilogNamingStyle("DATA", VerilogNamingSubject::Unsupported).has_value(),
          "unsupported subject remains unknown");
    check(checkVerilogNamingStyle("", VerilogNamingSubject::WireLike) == false,
          "empty names skipped");
    check(checkVerilogNamingStyle(std::string(100, 'A'), VerilogNamingSubject::Parameter) == false,
          "long uppercase parameter");
    check(checkVerilogNamingStyle("D" + std::string(100, 'a'), VerilogNamingSubject::WireLike) == false,
          "long first-uppercase variable");
    check(checkVerilogNamingStyle(std::string(100, 'a') + "X", VerilogNamingSubject::WireLike) == true,
          "late uppercase does not evade scan");
    const std::string nonAscii = "\xc3\xa9";
    check(checkVerilogNamingStyle(nonAscii, VerilogNamingSubject::Parameter) == false,
          "non-ASCII bytes are not ASCII lowercase");
    check(checkVerilogNamingStyle(nonAscii, VerilogNamingSubject::WireLike) == true,
          "wire initial non-ASCII byte rejected by default");
    check(checkVerilogNamingStyle("a" + nonAscii, VerilogNamingSubject::WireLike) == false,
          "later non-ASCII bytes ignored by ASCII-case predicate");
    return failures == 0 ? 0 : 1;
}
