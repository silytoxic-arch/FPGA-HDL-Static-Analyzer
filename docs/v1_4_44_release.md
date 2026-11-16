# V1.4.44 规则恢复说明

本版继续只处理规则集，没有调整表格或其他界面。

GJB10157.9.4.2 从“未知 AST 标志下的显式源码子集”提升为“前端标志生产者驱动”的有界实现：

- 确认 `FUN_1800e1200` 是 `AST_INITIAL(0x53)` 的唯一明确 `+0x2a9=1` 生产者；
- 结合 `initial_construct` 规则名、`Initial_constructContext` RTTI 和函数的 AST_INITIAL/AST_BLOCK 构造序列，确认该标志对应显式 initial 前端根节点；
- 排除解析器状态函数中同数值 `0x2a9` 的无关线索；
- 新增属性、命名块及 fork/join 正反边界测试，仍按每个显式 initial 根节点报告一次。

证据与边界详见 [GJB10157.9.4.2 initial 判据](evidence/native_verilog_initial_9_4_2_findings.md)。

## 验证结果

- 完整 CTest：135/135 通过。
- 附件规则总数：544；其中 GJB10157/GJB9765 标准规则 209 条。
- 附件示例探测：446；`Automated:matched` 435、既有 `Automated:mismatch` 1、`ProjectContext:matched` 4、`context-required` 6。
- 原生规则实现台账：209/209 条核心谓词已接入；完整算法等价验证仍为 0，不把未恢复分支标记为已验证。
