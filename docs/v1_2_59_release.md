# V1.2.59 GJB9765.4.1.b / 4.1.c 组件端口检查

本轮继续只处理规则集，没有调整表格格式。对 VHDL architecture 内声明的 `component`，软件现在可以把端口方向与实例 `port map` 关联：必需输入遗漏或显式 `open` 触发 4.1.b；输出遗漏触发 4.1.c；显式 `open` 的未用输出不触发 4.1.c。已解析的实体也采用相同的输出 `open` 判别。

附件两条规则的违反例各检出一处，遵循例均为零。446 个附件文档示例中 Automated 类别吻合 210/220（前版 208/220）。209 条 GJB 规则中 64 条核心判据部分接入、145 条仍需解释，0 条宣称完整等价。完整测试 130/130 通过；macOS 软件包及源码包均通过 ZIP 完整性验证，应用签名验证通过。

证据：[原生恢复](evidence/native_vhdl_component_ports_4_1_bc_findings.md)、[示例审计](evidence/t_linter_after_1_2_59.md)、[规则台账](evidence/native_rule_implementation_ledger.json)。
