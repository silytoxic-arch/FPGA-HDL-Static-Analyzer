# V1.4.55 规则恢复说明

本版继续只处理规则集及其验证台账，没有调整问题列表、代码规模明细或其他表格界面。

## GJB9765.4.2.c 单一关系根

`T-Linter.dll` 的 `FUN_18078c370` 要求 `AST_COND` 恰有一个子节点，该节点本身必须是
`AST_LT/LE/EQ/EQX/GE/GT` 且恰有两个操作数。以逻辑 AND、OR、NOT 等节点为根、内部再
包含比较关系的复合条件不属于原生候选。

产品旧实现只检查条件文本中是否出现任一比较符和已声明名称，因此并发 WHEN 和进程 IF 的
复合 AND 条件都会误报。本版在去除完整包围括号后，要求条件顶层恰有一个受支持关系运算符，
并拒绝顶层 `and/or/xor/xnor/nand/nor/not`。两个新增负例均在修正前失败、修正后通过。
该结论来自静态反编译，没有运行厂商检查器。

## 尚未恢复

别名/跨实例传播、configuration/generate、端口实际连接解析、聚合及复杂选择目标、异常语法
恢复和原生诊断重数仍未知。无诊断路径继续保持 `ManualReviewRequired`。

## 发布验证

- 新增并发 WHEN 与进程 IF 两个复合条件误报回归并通过；
- Release 配置完整构建通过，CTest 135/135 全部通过；
- 附件 446 个文档样例审计保持稳定：435 个 `Automated:matched`、6 个
  `ProjectContext:matched`、4 个 `context-required`、1 个
  `native-documentation-conflict`，无普通不匹配；
- 规则台账覆盖 209/209 条核心判断，未把 4 个缺少真实文件环境的样例伪判为合规；
- 完整原生算法等价验证仍为 0/209；这表示尚未取得足以证明逐条完全等价的证据，
  不影响已恢复核心判断和现有边界测试的可追溯性；
- 最终安装包：
  `releases/1.4.55/FPGA-HDL-Static-Analyzer-1.4.55-Darwin-arm64.zip`；
- 包体积：92,714,266 字节；SHA-256：
  `199cb29a972db6aba2ec926e4b9c044a62379a20f1df0e14c7993993e5c37137`；
- 独立临时目录解包后，CLI 报告 `FPGA HDL Static Analyzer 1.4.55`，应用包通过
  `codesign --verify --deep --strict`。
