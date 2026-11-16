# V1.2.96 规则集进展

本版本仅推进规则集，没有进行表格或其他界面格式调整。

## GJB10157.8.5

- 重新反编译 `FUN_1805748e0` 与递归助手 `FUN_1805757d0`。
- 确认检查根为 AST_ALWAYS、AST_ASSIGN，运算节点为 ADD、SUB、MUL、DIV、MOD、POW。
- 恢复 X、Z 两类独立诊断，而不是合并成一条“X 或 Z”提示。
- 恢复嵌套算术节点逐层诊断；一个同时含 X/Z 的内层节点及其算术祖先各产生两条。
- 同一算术节点内多个同类未知常量只产生该类别一条诊断。
- 新增正例、反例、嵌套和 X/Z 双状态边界回归测试。

详细原生证据见
[`native_unknown_arithmetic_8_5_findings.md`](evidence/native_unknown_arithmetic_8_5_findings.md)。

## 保守边界

未恢复的常量折叠、宏来源、生成层次、完整 SystemVerilog 表达式语法及豁免行为
继续保持人工复核；无问题不等于证明完全符合。
