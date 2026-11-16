# V1.4.52 规则恢复说明

本版继续只处理规则集及其验证台账，没有调整问题列表、代码规模明细或其他表格界面。

## GJB9765.4.2.c 条件控制对象类别

只读反编译 `T-Linter.dll` 的 `FUN_18078bbc0` 与类型名函数 `FUN_1808ea310`，确认原生
关系条件名称表只从以下节点建立：

- `AST_PORT(0xa4)` 下的直接 `AST_WIRE(6)` 端口；
- `AST_GENERIC(0xa3)` 下的直接 `AST_IDENTIFIER(0x13)`；
- `AST_SIGNAL(0x78)`。

`FUN_18078c370` 对关系两侧分别查找该表，任一操作数命中即可。`AST_CONSTANT(0x10)`、
`AST_VARIABLE(0x79)` 和 `AST_ALIAS(0xa0)` 没有名称表写入路径，因此不是该候选中的控制对象。
这些是静态反编译观察，不表示运行了厂商检查器。

产品此前把端口与 signal 同时用作“赋值目标集合”和“条件控制集合”，因此遗漏了 generic。
本版拆分两个集合：generic 只加入条件控制集合，端口与 signal 同时加入两者。新增回归验证：

- generic 关系控制会形成 4.2.c 候选；
- constant、process variable、alias 关系控制均不会形成候选。

## 尚未恢复

别名/跨实例传播、configuration/generate、端口实际连接解析、聚合及复杂选择目标、异常语法
恢复和原生诊断重数仍未知。无诊断路径继续保持 `ManualReviewRequired`，不判定自动合规。

## 发布验证

- GJB9765.4.2.c 新增四类声明边界回归并通过；
- 完整重配置、构建及 CTest：135/135 通过；
- 附件样例审计：446 个样例，其中 `Automated:matched` 435、
  `native-documentation-conflict` 1、`ProjectContext:matched` 6、
  `context-required` 4；没有未解释的普通 mismatch；
- 台账重新生成：209/209 核心谓词已接入，完整供应商算法等价验证仍为 0；
- 解包后的 CLI 报告 `FPGA HDL Static Analyzer 1.4.52`；
- 解包后的 macOS 应用通过 `codesign --verify --deep --strict`；
- 发布包为 `releases/1.4.52/FPGA-HDL-Static-Analyzer-1.4.52-Darwin-arm64.zip`；
  91,895,693 字节，SHA-256
  `b1ac204e6f311d5fc9c0816c768e7d82098b213813860973264a3f4c1667a67f`。
