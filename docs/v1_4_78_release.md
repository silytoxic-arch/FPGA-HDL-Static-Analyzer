# V1.4.78：R-13-5 function 返回赋值位宽深化

本轮只深化规则判断，不修改界面或表格布局。公开规则编号继续使用官方三段式 `R-X-X` / `A-X-X`。

## 原生恢复

- `GJB10157.9.4.5` 候选同时包含阻塞与非阻塞赋值，第一 child 的内部名称须包含 `$func$` 且以 `$result` 结尾。
- 原生运行函数分别计算返回端和输入表达式宽度，宽度不等时按每个赋值独立输出 `Warning`。
- 精确诊断模板为：`在function语句中，返回值的位宽%d与输入信号的位宽%d不一致`，两个参数依次为返回值宽度和输入表达式宽度。
- 详细证据见 [R-13-5 原生证据](evidence/native_verilog_function_return_width_9_4_5_findings.md)。

## 产品接入

- 支持固定十进制范围的非 ANSI function 返回值和 input。
- 支持阻塞及非阻塞 function 返回赋值。
- 支持直接 input、固定 bit-select/part-select、括号、一元 `+/-/~`。
- 支持嵌套 `+ - * / %`，按已恢复证据采用直接 child 最大宽度传播。
- ANSI、typed、参数化类型以及 concat/replicate、调用等未证明表达式仍保持复核，不以无报警判定合规。

## 验证

- 自动化测试：`135/135` 通过。
- 209 条规则审计：共探测 446 个附件示例；`Automated:matched` 435、`ProjectContext:matched` 6、`context-required` 4、原生与文档冲突 1，与 V1.4.77 基线一致。
- 台账状态：`R-13-5` 为 `evidence-supported-core-predicate; integrated-source-adapter`；完整厂商算法等价仍为 `0/209`。
- 发布包：`releases/1.4.78/FPGA-HDL-Static-Analyzer-1.4.78-Darwin-arm64.zip`。
- SHA-256：`e267515adc73fa733b5c96329dce254dda0f9c593dd3ccccf7de9a4c7e77d6d9`。
