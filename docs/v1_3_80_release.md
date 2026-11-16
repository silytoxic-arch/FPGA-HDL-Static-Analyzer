# FPGA HDL Static Analyzer V1.3.80

本版继续只处理规则逆向、判断逻辑与验证，未修改表格或界面布局。

## GJB9765.4.11.a

已恢复并接入“状态机无效状态应得到合理处理”的证据支持核心判断。产品在可证明的一进程或二进程状态机上，比较普通状态分支与 `others` 分支的直接赋值目标；空分支或目标遗漏会报告。没有 `others` 时，对可解析的枚举或二进制编码容量检查是否存在未覆盖编码。

嵌套状态机、复杂目标、综合器实际编码和未知容量继续为 `ManualReviewRequired`。完整证据与限制见 [恢复记录](evidence/native_vhdl_fsm_invalid_state_4_11_a_findings.md)。

## 覆盖与验证

- 209 条规则中，190 条已接入证据支持的核心判断；19 条仍待解释。完整原生算法等价数仍为 0。
- 新增 5 项边界回归：遗漏目标、完整覆盖、空 `others`、未覆盖二进制编码和完整二进制编码；Release 构建的 131 项 CTest 全部通过。
- 附件 446 个示例复测；4.11.a 正反例均吻合。ManualReview 示例总体由 41 个吻合提升到 42 个，差异由 7 个降至 6 个。
- 附件审计详见 `evidence/t_linter_after_1_3_80.json` 与同名 Markdown 摘要；逐规则状态见 `evidence/native_rule_implementation_ledger.json`。
- 本次提供经签名校验和 ZIP 完整性校验的 macOS arm64 包及 SHA-256；完整原生等价仍未宣称。
- Windows ZIP 仍需在 Windows 构建环境中生成和验证。
