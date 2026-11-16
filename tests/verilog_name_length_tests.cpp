#include "analysis/verilog_name_length.h"
#include <iostream>
#include <string>

int main() {
    int failures = 0;
    const auto check = [&](bool ok, const char* name) {
        if (!ok) { ++failures; std::cerr << "FAIL " << name << '\n'; }
    };
    using fpga::analysis::checkVerilogNameLength;
    check(checkVerilogNameLength("") == false, "empty native name skipped");
    check(checkVerilogNameLength("a") == true, "one byte below native minimum");
    check(checkVerilogNameLength("ab") == false, "inclusive minimum");
    check(checkVerilogNameLength(std::string(32, 'a')) == false, "inclusive maximum");
    check(checkVerilogNameLength(std::string(33, 'a')) == true, "one above maximum");
    check(checkVerilogNameLength("a", {1, 1}) == false, "explicit configured limits");
    check(checkVerilogNameLength("ab", {-1, 32}) == true,
          "negative minimum sign-extends to native unsigned maximum");
    check(checkVerilogNameLength("ab", {2, -1}) == false,
          "negative maximum sign-extends to native unsigned maximum");
    check(checkVerilogNameLength("ab", {32, 2}) == true,
          "inverted bounds reject every nonempty integer length");
    check(checkVerilogNameLength("ab", {-1, -1}) == true,
          "two negative bounds preserve native first comparison");
    return failures ? 1 : 0;
}
