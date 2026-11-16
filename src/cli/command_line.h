#pragma once
#include <iosfwd>
#include <span>
#include <string_view>

namespace fpga::cli {
// Arguments exclude argv[0]. Returns 0 for a clean analysis, 1 when lint
// violations are found, and 2 for invalid input or analysis errors.
int run(std::span<const std::string_view> arguments, std::ostream& out, std::ostream& err);
}
