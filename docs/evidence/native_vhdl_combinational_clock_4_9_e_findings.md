# GJB9765.4.9.e 组合逻辑生成时钟

目标附件 `T-Linter.dll` SHA-256 `60e6b52875f4d7c3fbdf494d869b3a4bcc3fdc8ee63d8620ea9614da5d6599b0`。以下依据 Ghidra 12.1.4 静态反编译及附件示例，不声称已运行厂商 DLL。

## Recovered predicate

- 原生 slot 0 `FUN_18060a000` 的保留证据为 `ev_2614f6621bf4803ebbf8c32cf6c0e058febc1c3f5c5b373ecb8376bd87e1075a`。它收集 `AST_ALWAYS(0x52)`，用 `FUN_1804d56f0` 建立时钟关系，再由 `FUN_18060ae10` 对赋值记录与时钟关系名称求交；架构下的直接赋值也进入该检查。
- `FUN_18060ae10` 与 `FUN_1804d3050` 的定点反编译留在 `reverse-tools/Library/ghidra/home/Library/ghidra/ghidra_12.1.4_PUBLIC/application.log`，可按 `FUNCTION 18060ae10` 和 `FUNCTION 1804d3050` 定位。后者的 `AST_ASSIGN_LE(0x56)` 分支在参数 0 时要求 RHS **根节点**类型为 `0x24..0x2c`、`0x3c..0x3f`、`0x45..0x47` 或 `0xb6`；单纯标识符及函数调用等其他根类型不进入此分支。相邻原生规则已把 `0x3c..0x3f` 交叉确认成二元 `AST_ADD/AST_SUB/AST_MUL/AST_DIV`，一元 `AST_POS/AST_NEG` 是集合外的 `0x43/0x44`。
- 附件 `examples_html/GJB9765_VHDL.4.9.e.html` 的错误例使用 `clk_g1 <= CLK and CLK_EN` 并在 `rising_edge(clk_g1)` 触发；正确例直接使用外部 `CLK`。附件标题与另一规则目录对该条款的描述存在版本/语义差异，仍需标准文本核对。

## 有界产品判断

仅在解析器确认 VHDL 时钟后，沿当前实体架构中的直接赋值依赖反向追踪；可达赋值的 RHS 根为明确逻辑、反相、移位、二元算术或条件选择运算时报告 4.9.e。产品在去除完整外层括号后只扫描根层运算符：根一元 `+/-` 以及函数调用根下嵌套的算术不报告，二元根 `+/-` 保持报告。单纯时钟别名以及与时钟无关的普通数据组合逻辑也不报告。附件正反例、别名、反相、二元算术、一元负号、函数调用嵌套算术和普通数据逻辑均有回归测试。

函数返回值的跨调用传播、元件输出、复杂层级及生成语句、剩余 AST 枚举的完整源级映射、原生诊断次数和标准版本差异未恢复。未报告不代表整条规则合规；台账保留人工复核要求。
