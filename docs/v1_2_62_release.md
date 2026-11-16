# V1.2.62 GJB10157.8.1 参数化比较位宽修正

本轮只处理规则集，没有调整表格格式。DLL 静态伪代码显示本规则从 `AST_ALWAYS/AST_ASSIGN` 根收集八类比较节点，对恰有两个操作数的节点分别计算位宽，两侧不等时报告。软件现在对模块参数默认值及受限的非负加减表达式求值，用于推导 Verilog 声明区间；附件 16 bit 对 32 bit 的违反例能检出，等宽遵循例无该条诊断。原生细节与未恢复范围见 [恢复记录](evidence/native_verilog_comparison_width_8_1_findings.md)。

附件 446 个文档示例中，Automated 类别吻合 213/220（前版 212/220）；完整测试 130/130 通过。209 条 GJB 规则中 66 条核心判据部分接入、143 条仍需解释，0 条宣称完整厂商等价。参数覆盖、复杂表达式及未知位宽不伪判合规，仍显示待人工复核。

证据：[示例审计](evidence/t_linter_after_1_2_62.md)、[规则台账](evidence/native_rule_implementation_ledger.json)、[原生证据包](evidence/rea_verilog_comparison_width_8_1_bundle.json)。
