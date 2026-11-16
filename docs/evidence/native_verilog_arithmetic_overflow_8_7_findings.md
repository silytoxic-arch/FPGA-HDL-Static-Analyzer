# GJB10157.8.7 算术结果位宽：静态恢复记录

目标：附件 `T-Linter.dll`，SHA-256 `60e6b52875f4d7c3fbdf494d869b3a4bcc3fdc8ee63d8620ea9614da5d6599b0`。以下为 Ghidra 静态伪代码观察，不是原厂源代码，未执行 Windows 软件。原始记录见 [本规则 REA 证据包](rea_verilog_arithmetic_overflow_8_7_complete_bundle.json)；AST 枚举名称对照见 [比较位宽 REA 证据包](rea_verilog_comparison_width_8_1_bundle.json)。

## 观察与推断

- 规则入口 `0x180577640` 遍历模块相关 AST，在赋值路径获取赋值目标和右侧表达式的位宽信息，并发出 Warning。该入口还检查 `AST_CELL` 的参数/端口绑定，不能把规则缩写为“只查 always 中的加减”。证据 `ev_2d39647f5db8d6528c086519184668d7bd758d868c50723656f573df76be7be0`。
- 子过程 `0x1805790d0` 按 RHS 节点类型分支：`AST_ADD(0x3c)`、`AST_SUB(0x3d)` 共用一路，另有 `AST_MUL(0x3e)`、`AST_DIV(0x3f)`、`AST_MOD(0x40)`、`AST_POW(0x42)`。它多次调用原生位宽推导入口，并将结果同目标宽度比较。类型名由 `0x1808ea310` 的枚举名称映射核对。证据 `ev_965de2ff6bdd12c2e267427e014a9916d9a02e3f5092ba6edc9c48b5a6b30171`、`ev_512c915b90a165536016fafbb5a5ded6cfba13c0084ad97b3576b7906ee5547b`。
- 附件违反例把两个 3 位输入的加、减结果写回 3 位目标；遵循例把目标扩为 4 位。该正反例支持一个独立、受限的源代码级风险判据：两个固定宽度信号相加或相减时，若目标不比最宽输入多一位，则可能丢失进位/借位；恒等式 `A-A=0` 被排除。这是本软件的保守接入选择，不是已证明与原厂所有位模式分支等价。

## 本软件边界

实现见 `src/parser/semantic_analyzer.cpp` 的 `RTL-ARITHMETIC-RESULT-WIDTH-RISK`，由 `src/analysis/standard_evaluator.cpp` 映射至本条。附件同型正反例和字面量排除边界由 `tests/standard_accuracy_tests.cpp` 验证。乘除取模幂、单目运算、字面量/拼接/嵌套表达式、参数覆盖、复杂 cell 绑定及原生位模式优化均未接入。无诊断继续是 `ManualReviewRequired`，不是合规证明。
