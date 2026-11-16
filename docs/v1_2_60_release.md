# V1.2.60 GJB9765.4.5.e 时钟命名修正

本轮只处理规则集，没有调整表格格式。时钟候选名从宽泛的 `clk` 前缀收紧为原生证据支持的、不区分大小写的 `clk_` 前缀；附件明确标注的时钟相关输出端口也作有界检查。违反例检出 `CLK`、`C1_clk`，遵循例无本条诊断。

446 个附件文档示例中 Automated 类别吻合 211/220（前版 210/220）。完整测试 130/130 通过。209 条 GJB 规则中 65 条核心判据部分接入、144 条仍需解释，0 条宣称完整等价。未知候选集合及工程上下文不伪判合规。

证据：[原生恢复](evidence/native_vhdl_clock_name_4_5_e_findings.md)、[示例审计](evidence/t_linter_after_1_2_60.md)、[规则台账](evidence/native_rule_implementation_ledger.json)。
