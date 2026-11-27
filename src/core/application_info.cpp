#include "core/application_info.h"

namespace fpga {
ApplicationInfo application_info() noexcept {
    return {"FPGA HDL Static Analyzer", "1.4.250",
            "GJB10157 / GJB9765 HDL 静态分析与编码规则检查平台"};
}
} // namespace fpga
