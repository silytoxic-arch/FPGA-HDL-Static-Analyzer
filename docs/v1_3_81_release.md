# FPGA HDL Static Analyzer V1.3.81

本版继续只处理规则逆向、判断逻辑与验证，未修改表格或界面布局。

## GJB9765.4.11.d

已恢复并接入“建议使用三段式状态机”的证据支持核心判断。产品在可证明状态机上检查三个独立部分：状态寄存器进程、只写 next-state 的组合转换进程，以及独立的输出译码进程。一段式、转换和输出混合、缺少输出译码均会报告。

并发输出、寄存输出、复杂过程调用和跨文件绑定继续为 `ManualReviewRequired`。完整证据与限制见 [恢复记录](evidence/native_vhdl_fsm_three_stage_4_11_d_findings.md)。

## 覆盖与验证

- 209 条规则中，191 条已接入证据支持的核心判断；18 条仍待解释。完整原生算法等价数仍为 0。
- 新增 4 项边界回归：转换输出混合、缺少输出译码、完整三段式和一段式状态机；Release 构建的 131 项 CTest 全部通过。
- 附件 446 个示例复测；4.11.d 正反例均吻合。ManualReview 示例总体由 42 个吻合提升到 43 个，差异由 6 个降至 5 个。
- 附件审计详见 `evidence/t_linter_after_1_3_81.json` 与同名 Markdown 摘要；逐规则状态见 `evidence/native_rule_implementation_ledger.json`。
- 本次提供经签名校验和 ZIP 完整性校验的 macOS arm64 包及 SHA-256；完整原生等价仍未宣称。
- Windows ZIP 仍需在 Windows 构建环境中生成和验证。
