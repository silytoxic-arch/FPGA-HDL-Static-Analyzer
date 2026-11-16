# V1.2.50 GJB9765.4.14.b.3 对齐规则

## 本轮恢复与接入

- 从附件 `T-Linter.dll` 的规则入口和构造辅助函数恢复两类原生分组：顺序 `if/elsif/else` 关键字组，以及顺序 `case/when` 关键字组。
- 显示列按原生算法以 4 列制表位展开 Tab。
- `elsif`、`else` 必须与对应起始 `if` 同列。
- 每个 `when` 必须严格位于 `case` 右侧；后续 `when` 还必须与首个 `when` 同列。
- 同一个过浅且错列的后续 `when` 会保留两条不同原因的诊断，不合并为一条。
- `if generate`、`case generate` 和条件信号赋值不纳入该顺序语句适配器。

## 验证

- 附件违反例产生三条本规则诊断，遵循例不产生诊断。
- 新增 Tab/空格等显示列、过浅 WHEN、双原因 WHEN、非精确一 Tab 深度、generate 排除和嵌套 CASE 作用域测试。
- 附件 446 个公开示例审计中，Automated 吻合由 197 增至 199，Automated 不吻合仍为 17。
- 原生规则台账更新为：209 条规则，54 条核心谓词部分接入，155 条仍需解释，0 条宣称完整等价。

## 证据与限制

- 逆向发现：[native_vhdl_same_level_alignment_4_14_b_3_findings.md](evidence/native_vhdl_same_level_alignment_4_14_b_3_findings.md)
- REA Evidence bundle：`evidence/rea_vhdl_same_level_alignment_4_14_b_3_bundle.json`
- 公开示例审计：`evidence/t_linter_after_1_2_50.md`

专有 AST 枚举名称、畸形语法、其他 AST 形态、原生同名子串歧义和重复抑制仍未完全验证；无诊断继续显示为 `ManualReviewRequired`，不将未知逻辑伪判为合规。
