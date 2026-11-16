# V1.2.61 GJB10157.5.2.1 数字开头名称漏检修正

本轮只处理规则集，没有调整表格格式。原生证据确认本规则在 MODULE/WIRE/PARAMETER/LOCALPARAM/CELL/TASK/FUNCTION 的规范化名称首字节为 ASCII 数字或下划线时触发。附件违反例的 ANSI 端口 `input 1_B1` 被通用词法器归为 Number，过去未进入声明名称收集器；现在仅在明确的声明名称位置对这类含下划线的数字开头 token 做有界恢复。位宽数字、数值常量、注释和任意表达式不会被提升为声明名称。

446 个附件文档示例中 Automated 类别吻合 212/220（前版 211/220）；完整测试 130/130 通过。209 条 GJB 规则中 65 条核心判据部分接入、144 条仍需解释，0 条宣称与厂商完整等价。其他数字开头但不含下划线的非法名称、宏、转义名称和复杂语法仍未获得厂商 AST 等价证据；无告警仍需人工复核。

证据：[示例审计](evidence/t_linter_after_1_2_61.md)、[规则台账](evidence/native_rule_implementation_ledger.json)。
