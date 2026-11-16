# V1.2.51 GJB9765.4.14.b.1 行首 Tab 规则

## 本轮恢复与接入

- 读取附件 `T-Linter.dll` 的规则入口 `FUN_1805bda30`，恢复逐块源码扫描、首字符放行、Tab 优先放行和缺少 Tab 的告警分支。
- 产品按原生两组字节掩码检查短物理行，包含仅有空格的行及行首空格后的注释行。
- 超过 511 字节的物理行仍需人工复核；原生 `fgets(512)` 的拆块行号尚未证明与产品物理行号等价。
- 包装规则文字提及 Tab 宽度，但本次函数证据没有宽度比较，不据此扩展检测。

## 验证

- 附件违反例产生本规则诊断，遵循例无本规则诊断。
- 新增空格、Tab、空白行、注释行和 Tab 优先边界测试；规则专项测试通过。
- 446 个附件示例审计中，Automated 吻合 201，不吻合 17。
- 原生规则台账：209 条，55 条核心判据部分接入，154 条仍需解释，0 条宣称完整等价。

## 证据与限制

- 原生恢复：[native_vhdl_leading_tab_4_14_b_1_findings.md](evidence/native_vhdl_leading_tab_4_14_b_1_findings.md)
- REA Evidence bundle：`evidence/rea_vhdl_leading_tab_4_14_b_1_bundle.json`
- 附件示例审计：`evidence/t_linter_after_1_2_51.md`

无本规则诊断时继续显示 `ManualReviewRequired`，不将未证实的长行、源码准入及其他规则链路判为合规。
