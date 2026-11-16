# V1.4.43 规则恢复说明

本版继续只处理规则集，没有调整表格或其他界面。

GJB10157.9.4.1 从“附件数值示例子集”提升为“原生前缀延时标志生产者驱动”的有界实现：

- 复用已恢复的 Bison 第 545 条产生式 `behavioral_stmt -> non_opt_delay behavioral_stmt`；
- task 内前置 `#` 延时覆盖直接数值、宏值及括号形式；
- 明确排除由独立产生式解析、且不写 `+0x29b` 的赋值内延时；
- `@` 事件控制与 `wait` 的原生 lowering 尚未恢复，继续保留人工复核而不猜测合规。

证据与边界详见 [GJB10157.9.4.1 task 时序控制判据](evidence/native_verilog_task_timing_9_4_1_findings.md)。

## 验证结果

- 完整 CTest：135/135 通过。
- 附件规则总数：544；其中 GJB10157/GJB9765 标准规则 209 条。
- 附件示例探测：446；`Automated:matched` 435、既有 `Automated:mismatch` 1、`ProjectContext:matched` 4、`context-required` 6。
- 原生规则实现台账：209/209 条核心谓词已接入；完整算法等价验证仍为 0，不把未恢复分支标记为已验证。
