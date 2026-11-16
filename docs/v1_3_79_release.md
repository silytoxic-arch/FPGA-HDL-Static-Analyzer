# FPGA HDL Static Analyzer V1.3.79

本版继续只处理规则逆向、判断逻辑与验证，未修改表格或界面布局。

## GJB9765.4.11.c

已恢复并接入“状态机应初始化为合法状态”的证据支持核心判断。产品在可证明的一进程或二进程状态机上，检查直接复位赋值和声明初始化是否存在、是否属于已解析枚举或 `case` 合法状态集合；二进程状态传递不被误当作初始化。

复杂初始化表达式、包类型、subtype、别名、复合状态目标和跨文件绑定继续为 `ManualReviewRequired`。完整证据与限制见 [恢复记录](evidence/native_vhdl_fsm_legal_initialization_4_11_c_findings.md)。

## 覆盖与验证

- 209 条规则中，189 条已接入证据支持的核心判断；20 条仍待解释。完整原生算法等价数仍为 0。
- 新增 5 项边界回归：缺少初始化、合法复位值、非法复位值、合法声明初始化和二进程缺少初始化；Release 构建的 131 项 CTest 全部通过。
- 附件 446 个示例复测；4.11.c 正反例均吻合。ManualReview 示例总体由 40 个吻合提升到 41 个，差异由 8 个降至 7 个。
- 附件审计详见 `evidence/t_linter_after_1_3_79.json` 与同名 Markdown 摘要；逐规则状态见 `evidence/native_rule_implementation_ledger.json`。
- 本次提供经签名校验和 ZIP 完整性校验的 macOS arm64 包及 SHA-256；完整原生等价仍未宣称。
- Windows ZIP 仍需在 Windows 构建环境中生成和验证。
