# V1.4.54 规则恢复说明

本版继续只处理规则集及其验证台账，没有调整问题列表、代码规模明细或其他表格界面。

## GJB9765.4.2.c 嵌套 CASE/LOOP 边界

`T-Linter.dll` 的 `FUN_18078c370` 只从 IF 条件节点的直接
`AST_BLOCK→AST_ASSIGN_LE` 路径建立 THEN 目标表。由此可确定：嵌套 CASE 或 LOOP 内部的
赋值不能提升为外层 IF 的直接 THEN 赋值。

产品 V1.4.53 已排除嵌套 IF，本版进一步统一跟踪 IF、CASE、LOOP 的嵌套深度。新增两个
负例分别在外层 THEN 中放置 CASE 和 WHILE-LOOP 赋值、在外层 ELSE 中赋 Z；旧实现均误报，
修正后均无 4.2.c 诊断。该结论来自静态反编译，没有运行厂商检查器。

## 尚未恢复

别名/跨实例传播、configuration/generate、端口实际连接解析、聚合及复杂选择目标、异常语法
恢复和原生诊断重数仍未知。无诊断路径继续保持 `ManualReviewRequired`。

## 发布验证

- 新增嵌套 CASE 与 LOOP 两个误报边界回归并通过；
- 完整重配置、构建及 CTest：135/135 通过；
- 附件样例审计：446 个样例，其中 `Automated:matched` 435、
  `native-documentation-conflict` 1、`ProjectContext:matched` 6、
  `context-required` 4；没有未解释的普通 mismatch；
- 台账重新生成：209/209 核心谓词已接入，完整供应商算法等价验证仍为 0；
- 解包后的 CLI 报告 `FPGA HDL Static Analyzer 1.4.54`；
- 解包后的 macOS 应用通过 `codesign --verify --deep --strict`；
- 发布包为 `releases/1.4.54/FPGA-HDL-Static-Analyzer-1.4.54-Darwin-arm64.zip`；
  92,441,091 字节，SHA-256
  `489723951438f79aa96cc26afad3b07f9022047b76065a27086b058c26ebafd2`。
