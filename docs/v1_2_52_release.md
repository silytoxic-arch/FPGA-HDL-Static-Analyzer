# V1.2.52 GJB9765.4.14.a.1 分节空行规则

## 本轮恢复与接入

- 原生入口先收集指定 AST 节点的源码行号，再检查节点前一行是否含有 `--` 注释前的非空白字节。
- 产品在可确认的顺序 `process` 边界接入该判断；允许标签和 `postponed` 修饰。
- 其他 AST 类型的权威名称及长行拆块行号仍未确认。

## 验证

- 附件违反例在第二个 `process` 的第 42 行产生本规则诊断，遵循例无诊断。
- 新增相邻进程、空白行、独立注释行、行尾注释和 `postponed process` 边界测试。
- 446 个附件示例审计中，Automated 吻合 203，不吻合 17。
- 规则台账：209 条，56 条核心判据部分接入，153 条仍需解释，0 条宣称完整等价。

## 证据与限制

- [原生恢复](evidence/native_vhdl_blank_section_4_14_a_1_findings.md)
- REA Evidence bundle：`evidence/rea_vhdl_blank_section_4_14_a_1_bundle.json`
- 附件示例审计：`evidence/t_linter_after_1_2_52.md`

无本规则诊断时仍显示 `ManualReviewRequired`，不能据此认定整条规则已合规。
