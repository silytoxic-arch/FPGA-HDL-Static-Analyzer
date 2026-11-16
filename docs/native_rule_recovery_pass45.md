# 第四十五轮：A-16-1 full_case 属性键与词法边界

本轮按官方三段式编号深化 `A-16-1`，不修改表格或界面布局。

## 新恢复事实

- 原生 slot 1 只接收 AST_CASE；slot 0 只匹配十字节精确属性键 `\full_case`。
- 原生 Verilog 词法器的 legacy `full_case` 动作返回专用 token `0x154`，附件实例闭合了 `//synopsys full_case` 到 case 属性的路径。
- 原生报告级别为 Warning，消息格式为 `Synopsys full case用于模块<名称>第<行号>行.`。
- case 节点名称在消息拼接前会清除开头的冒号。

## 产品变更

- 显式属性改为按 `attr_spec` 键解析，不再把 `(* note="full_case" *)` 的字符串值误报为属性键；
- 支持连续紧邻的多个属性块，属性值为0仍按原生键存在语义报告；
- 显式 `full_case_extra` 继续排除，hot comment 保留词法器的 `full_case` 前缀动作；
- hot comment 必须位于 case 选择表达式之后，且不再误接入另一规则的 `pragma/exemplar` 生产者；
- 恢复 Warning 和原生动态消息格式；未命中仍保持 `ManualReviewRequired`。

## 验证

定向覆盖附件形态、精确消息、显式键、属性值字符串、连续属性块、前缀动作、错误位置、无证据生产者、非活动预处理分支及干净路径人工复核。CTest `135/135` 全部通过；附件446个示例审计保持无未解释差异。台账中集成源码适配器增至29条，完整原生等价仍为 `0/209`。

完整证据见 [A-16-1 原生恢复记录](evidence/native_verilog_full_case_12_3_findings.md)。
