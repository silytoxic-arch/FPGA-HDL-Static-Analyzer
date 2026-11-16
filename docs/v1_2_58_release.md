# V1.2.58 GJB9765.4.6.f 多驱动规则修正

本轮只修改规则判断、测试和证据台账，没有调整任何表格格式。该规则不再把通用多驱动诊断原样映射到 GJB 条款；附件允许的显式条件三态驱动不再误报，同时检出组件输出与本地赋值共同驱动。附件违反例检出 `Z1`、`Z2`、`Z3`、`int4` 四处，遵循例无该规则诊断。

446 个附件文档示例中，Automated 类别吻合 208、不吻合 12（前版为 207/13）。209 条 GJB 规则中，62 条核心判据部分接入、147 条仍需解释；没有规则宣称已完整等价。无诊断的未证实情形继续保留人工复核状态。

证据：[原生恢复](evidence/native_vhdl_multiple_drivers_4_6_f_findings.md)、[示例审计](evidence/t_linter_after_1_2_58.md)、[实现台账](evidence/native_rule_implementation_ledger.json)。
