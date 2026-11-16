# FPGA HDL Static Analyzer V1.4.41

本版继续只处理规则集，未调整任何表格或界面。

GJB10157.9.2.5 的原生判据得到进一步校准。虚表 slot 2 仅接纳 `AST_ALWAYS(0x52)`；主函数只检查其直接 `AST_POSEDGE/AST_NEGEDGE/AST_EDGE(0x65..0x67)` 子节点，并在每个事件子树中递归搜索 `AST_BIT_OR(0x26)` 与 `AST_LOGIC_OR(0x46)`。任一搜索非空时在 always 根报告一次，因此同一灵敏度表出现多个 `|`/`||` 仍只有一条本规则诊断；过程体中的 OR、事件分隔关键字 `or` 和逗号均不触发。

Yosys 官方解析器源码补证 `always` 与 `always_ff` 都降低为 `AST_ALWAYS`，区别仅为属性。产品据此把现有显式 `always @(…)` 检查扩展到 `always_ff @(…)`，同时保持条件预处理、括号边界和每 always 去重。

新增正反例与边界测试覆盖 `always_ff` 边沿表达式内的位 OR、过程体 OR 排除、三元事件中的嵌套逻辑 OR、多个 OR 去重、普通 `or`/逗号分隔、注释、`@*` 和条件编译。附件所用具体前端版本、错误恢复 AST、宏/属性极端变换和原生消息正文仍保持未知，无告警不伪判为完整合规。

详细证据见 [native_verilog_sensitivity_or_9_2_5_findings.md](evidence/native_verilog_sensitivity_or_9_2_5_findings.md)。

## 验证结果

- 完整 CTest：135/135 通过。
- 附件规则总数：544；其中 GJB10157/GJB9765 标准规则：209。
- 附件示例探测：446；`Automated:matched` 435、已知 `Automated:mismatch` 1、`ProjectContext:matched` 4、`context-required` 6。
- 原生规则实现台账：209/209 条核心判据已接入；完整算法等价验证仍为 0，未把静态证据之外的未知分支标记为已验证。
- 最终发布包 SHA-256 在打包完成后单独提供，避免把压缩包自身校验值写入包内形成循环依赖。
