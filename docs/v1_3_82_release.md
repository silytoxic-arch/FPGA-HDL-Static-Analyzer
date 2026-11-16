# FPGA HDL Static Analyzer V1.3.82

本版继续只处理规则逆向、判断逻辑与验证，未修改表格或界面布局。

## GJB10157.10.4

已从原生最终上报函数恢复“单个状态机状态数不超过 40”的核心边界。`FUN_1805138b0` 对恢复出的去重状态集合执行严格比较 `0x28 < size`，所以 40 状态合规，41 状态开始告警。

产品现对可证明的一进程 Verilog 状态机，以及具有直接 `state <= next_state` 传递的二进程状态机，统计普通 `case` 的去重显式源状态。普通宽译码器、复杂标签和无法完成状态绑定的代码继续为 `ManualReviewRequired`。完整证据与限制见 [恢复记录](evidence/native_verilog_fsm_state_count_10_4_findings.md)。

## 覆盖与验证

- 209 条规则中，192 条已接入证据支持的核心判断；17 条仍待解释。完整原生算法等价数仍为 0。
- 新增 6 项边界回归：40/41 状态、一进程/二进程、`always_ff`、重复标签去重和宽译码器排除；Release 构建的 131 项 CTest 全部通过。
- 附件没有 GJB10157.10.4 的随包示例，因此附件 446 个示例复测不会产生该规则的直接正反例匹配结论；总体仍为 430 个吻合、16 个差异，没有退化。
- 附件审计详见 `evidence/t_linter_after_1_3_82.json` 与同名 Markdown 摘要；逐规则状态见 `evidence/native_rule_implementation_ledger.json`。
- 本次提供经签名校验和 ZIP 完整性校验的 macOS arm64 包及独立 SHA-256；完整原生等价仍未宣称。
- Windows ZIP 仍需在 Windows 构建环境中生成和验证。
