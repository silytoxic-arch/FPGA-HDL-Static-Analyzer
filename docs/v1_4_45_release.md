# V1.4.45 规则恢复说明

本版继续只处理规则集，没有调整表格或其他界面。

GJB10157.5.3.4 的两个关键生产者已进一步恢复：

- `setTopModule(0x1806298c0)` 将普通名称规范化为 Yosys `\\name`，验证模块存在后写入 `DAT_18135b808`；规则对模块内部名称做完整长度精确比较并跳过配置顶层；
- `FUN_1800e1200` 创建显式 `initial_construct` 的 `AST_INITIAL(0x53)` 并写入 `+0x2a9=1`，确认规则只把前端显式 initial 根节点计为功能逻辑；
- 产品增加带属性/命名块 initial 的非顶层中间模块测试，并排除损坏 task/function 内不可能形成 AST_INITIAL 的伪关键字。

产品仍以实例图中的全部根模块替代原生单一显式顶层配置，因此多根工程的无告警结果继续保留人工复核。

证据与边界详见 [GJB10157.5.3.4 原生判据恢复记录](evidence/native_verilog_hierarchy_only_5_3_4_findings.md)。

## 验证结果

- 完整 CTest：135/135 通过；新增边界测试曾发现并修复 task/function 内损坏源码赋值被误算为模块逻辑的问题。
- 附件规则总数：544；其中 GJB10157/GJB9765 标准规则 209 条。
- 附件示例探测：446；`Automated:matched` 435、既有 `Automated:mismatch` 1、`ProjectContext:matched` 4、`context-required` 6。
- 原生规则实现台账：209/209 条核心谓词已接入；2 条已提升为 `integrated-source-adapter`，207 条仍为 `partial-integration`，不把未知分支标记为原生等价。
