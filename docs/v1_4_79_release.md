# V1.4.79：R-13-6 task 形参与实参位宽深化

本轮只深化规则判断，不修改界面或表格布局。公开编号继续使用官方三段式 `R-13-6`。

## 原生恢复

- 在完整 DLL 中定位到 `FUN_180914500` 的 `$task$` 与 `$para_assign` 生产路径。
- 前端为每个带方向的 task 形参创建一个 `AST_ASSIGN_EQ(0x55)` 绑定：输入 child 顺序为形参/实参，输出为实参/形参。
- 新绑定命名为 `$para_assign`，源码位置元数据从当前实参 AST 复制；原生规则随后逐绑定计算两侧宽度并报告。
- 详细证据见 [R-13-6 原生证据](evidence/native_verilog_task_argument_width_9_4_6_findings.md)。

## 产品接入

- 在原 ANSI task 基础上增加 legacy task 声明与 legacy module 端口声明。
- 增加 module-scope 固定宽度 wire/reg/logic 裸实参。
- 增加显式定宽字面量、固定 bit/part select、静态拼接/重复、一元与嵌套算术/位运算的有界宽度传播。
- 保持原生输入/输出宽度参数顺序、方向后缀、逐绑定报告粒度和 `Warning` 级别。
- 参数化范围、动态选择、调用、完整 signedness/type 传播与 inout 仍保持人工复核，不以无报警判合规。

## 验证

- 自动化测试：`135/135` 通过。
- 209 条规则审计：共探测 446 个附件示例；`Automated:matched` 435、`ProjectContext:matched` 6、`context-required` 4、原生与文档冲突 1，与 V1.4.78 基线一致。
- 台账状态：`R-13-6` 为 `evidence-supported-core-predicate; integrated-source-adapter`；完整厂商算法等价仍为 `0/209`。
- 发布包：`releases/1.4.79/FPGA-HDL-Static-Analyzer-1.4.79-Darwin-arm64.zip`。
- SHA-256：`d90c9fbd3415da3685a38d93e85635ce402c9a1467696102aa31ae15b460b731`。
