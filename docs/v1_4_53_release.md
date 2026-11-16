# V1.4.53 规则恢复说明

本版继续只处理规则集及其验证台账，没有调整问题列表、代码规模明细或其他表格界面。

## GJB9765.4.2.c 两臂 IF 直接赋值边界

`T-Linter.dll` 的 `FUN_18078c370` 只接纳恰有 `AST_COND` 和最终 `AST_ELSE` 两个子节点
的 `AST_IF`。THEN 侧目标表只从 COND 的直接 `AST_BLOCK` 子节点提取单个
`AST_ASSIGN_LE`：

- 同层存在 `AST_ELSIF` 时，不是该两臂 IF 候选；
- 嵌套 IF 内的赋值不能作为外层 IF 的直接 THEN 赋值；
- 最终 ELSE-Z 仍必须与直接 THEN 目标相同。

产品旧实现会扫描 THEN 到 ELSE 之间的全部赋值，两个新增负例分别复现了嵌套 IF 和 ELSIF
误报。本版按 IF 嵌套深度只接纳同层赋值，并在同层 ELSIF 出现时否决候选；两个负例修正后
均无 4.2.c 诊断。该结论来自静态反编译，没有执行厂商检查器。

## 尚未恢复

别名/跨实例传播、configuration/generate、端口实际连接解析、聚合及复杂选择目标、异常语法
恢复和原生诊断重数仍未知。无诊断路径继续保持 `ManualReviewRequired`。

## 发布验证

- 新增嵌套 IF 与 ELSIF 两个误报边界回归并通过；
- 完整重配置、构建及 CTest：135/135 通过；
- 附件样例审计：446 个样例，其中 `Automated:matched` 435、
  `native-documentation-conflict` 1、`ProjectContext:matched` 6、
  `context-required` 4；没有未解释的普通 mismatch；
- 台账重新生成：209/209 核心谓词已接入，完整供应商算法等价验证仍为 0；
- 解包后的 CLI 报告 `FPGA HDL Static Analyzer 1.4.53`；
- 解包后的 macOS 应用通过 `codesign --verify --deep --strict`；
- 发布包为 `releases/1.4.53/FPGA-HDL-Static-Analyzer-1.4.53-Darwin-arm64.zip`；
  92,169,194 字节，SHA-256
  `34e09e603c433eadc9c19fba876de6c374ce0436aab7906af8b685b06dcf656d`。
