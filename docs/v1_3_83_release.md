# FPGA HDL Static Analyzer V1.3.83

本版继续只处理规则逆向、判断逻辑与验证，未修改表格或界面布局。

## GJB10157.10.3

已恢复原生规则“状态机必须设置初始状态”的核心判断：最终上报函数在状态机关系树中精确查找由前端合成的 `init` 关系，缺失时才上报。该键不是状态名称，因此源代码存在名为 `INIT` 的状态不能代替初始化。

产品现对可证明的一进程或直接二进程 Verilog 状态机，检查显式 `initial` 状态赋值，以及时序 `if/else` 中与正常状态更新相对的直接初始化赋值。声明初始化、复杂表达式、宏、generate、别名和跨文件绑定继续为 `ManualReviewRequired`。完整证据与限制见 [恢复记录](evidence/native_verilog_fsm_initial_state_10_3_findings.md)。

## 覆盖与验证

- 209 条规则中，193 条已接入证据支持的核心判断；16 条仍待解释。完整原生算法等价数仍为 0。
- 新增 7 项边界回归，覆盖缺失初始化、名称假阳性、单/双进程复位、双分支普通 next-state 传递排除、显式 `initial` 和声明初始化边界；Release 构建的 131 项 CTest 全部通过。
- 附件没有 GJB10157.10.3 的随包示例，因此附件 446 个示例复测不构成本规则的厂商一致性结论；总体仍为 430 个吻合、16 个差异，没有退化。
- 逐规则状态见 `evidence/native_rule_implementation_ledger.json`，附件审计见 `evidence/t_linter_after_1_3_83.json` 与同名 Markdown 摘要。
- Windows ZIP 仍需在 Windows 构建环境中生成和验证。
