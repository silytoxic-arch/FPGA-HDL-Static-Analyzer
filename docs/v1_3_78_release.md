# FPGA HDL Static Analyzer V1.3.78

本版继续只处理规则逆向、判断逻辑与验证，未修改表格或界面布局。

## GJB9765.4.11.e

已恢复并接入“建议使用枚举类型声明状态机”的证据支持核心判断。产品只对源码可证明的一进程状态更新或二进程直接 next-state 传递建立候选，候选状态信号未使用本地枚举类型时报告；普通组合译码不会被误报。

包类型、subtype、别名、复合状态目标、跨文件类型绑定及未完成的状态机识别继续为 `ManualReviewRequired`。完整证据与限制见 [恢复记录](evidence/native_vhdl_fsm_enumeration_4_11_e_findings.md)。

## 覆盖与验证

- 209 条规则中，188 条已接入证据支持的核心判断；21 条仍待解释。完整原生算法等价数仍为 0。
- 新增 4 项边界回归：非枚举一进程、枚举一进程、非枚举二进程及普通组合译码排除；Release 构建的 131 项 CTest 全部通过。
- 附件 446 个示例复测；4.11.e 正反例均吻合。ManualReview 示例总体由 39 个吻合提升到 40 个，差异由 9 个降至 8 个。
- 附件审计详见 `evidence/t_linter_after_1_3_78.json` 与同名 Markdown 摘要；逐规则状态见 `evidence/native_rule_implementation_ledger.json`。
- 本次提供经签名校验和 ZIP 完整性校验的 macOS arm64 包及 SHA-256；完整原生等价仍未宣称。
- Windows ZIP 仍需在 Windows 构建环境中生成和验证。
