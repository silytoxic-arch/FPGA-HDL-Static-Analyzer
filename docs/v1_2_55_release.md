# V1.2.55 GJB9765.4.8.e 空语句规则

从附件 `T-Linter.dll` 恢复了递归收集显式空节点、零子节点语句块和一种单子节点结构的核心分支。软件保守实现顺序进程内 `NULL;`、空 `if` 分支及空进程体的检查，不再把通用 `RTL-EMPTY-PROCESS` 直接映射为该条款。

附件违反例和遵循例均吻合。446 个文档示例中 Automated 吻合 205、不吻合 15。台账共 209 条规则：59 条核心判据部分接入，150 条仍需解释，0 条宣称完整等价。无本规则诊断时保持 `ManualReviewRequired`。

证据：[原生恢复](evidence/native_vhdl_empty_statement_4_8_e_findings.md)、[示例审计](evidence/t_linter_after_1_2_55.md)。未知 AST 枚举名与完整语法覆盖仍未证实。
