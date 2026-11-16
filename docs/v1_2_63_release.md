# V1.2.63 GJB10157.8.7 算术结果位宽风险修正

本轮只处理规则集，没有调整表格格式。DLL 静态伪代码显示，本规则从赋值和 cell 相关路径检查算术表达式的结果位宽，分别处理加、减、乘、除、取模、幂等节点。本软件据此新增独立的 `RTL-ARITHMETIC-RESULT-WIDTH-RISK`，只对固定宽度双信号加减及不足以容纳可能进位/借位的目标发出诊断；`A-A` 的恒零情况排除。其他运算分支仍未恢复为可可靠接入的源代码判据。见 [原生恢复记录](evidence/native_verilog_arithmetic_overflow_8_7_findings.md)。

附件 446 个文档示例中 Automated 类别吻合 214/220（前版 213/220）；完整测试 130/130 通过。209 条 GJB 规则中 67 条核心判据部分接入、142 条仍需解释，0 条宣称完整厂商等价。未知逻辑和缺少工程上下文的规则不伪判合规。

证据：[示例审计](evidence/t_linter_after_1_2_63.md)、[规则台账](evidence/native_rule_implementation_ledger.json)、[原生证据包](evidence/rea_verilog_arithmetic_overflow_8_7_complete_bundle.json)。
