# V1.4.56 规则恢复说明

本版继续只处理规则集及验证台账，没有调整任何界面或表格。

## GJB9765.4.2.c 直接关系操作数

`T-Linter.dll` 的 `FUN_18078c370` 只使用关系根两个直接子节点的名称查询声明表。旧产品
实现会扫描整个条件文本，因此 `sel + 1 = 2` 会因为算术子树中出现已声明的 `sel` 而误报。

本版把查找边界收窄到关系运算符两侧：只有直接声明名称才能命中，嵌在算术表达式中的名称
不能提升为候选。并发 WHEN 与进程 IF 两个负例均在修复前失败、修复后通过；另以
`2 = sel` 验证原生允许右侧直接操作数命中。

## 证据与限制

- 目标 `T-Linter.dll` SHA-256：
  `60e6b52875f4d7c3fbdf494d869b3a4bcc3fdc8ee63d8620ea9614da5d6599b0`；
- 静态反编译证据：`FUN_18078c370` 对两个直接子节点分别调用
  `FUN_180676aa0(&DAT_181374408, child + 0x48)`；
- 没有执行厂商程序；别名、跨实例传播、configuration/generate、复杂目标和原生诊断重数
  仍保持未知，干净结果不伪判为自动合规。

## 发布验证

- 三个新增关系操作数边界测试已通过；
- Release 配置完整构建通过，CTest 135/135 全部通过；
- 附件 446 个文档样例审计保持稳定：435 个 `Automated:matched`、6 个
  `ProjectContext:matched`、4 个 `context-required`、1 个
  `native-documentation-conflict`，无普通不匹配；
- 规则台账覆盖 209/209 条核心判断；完整原生算法等价仍为 0/209，未知分支继续显式保留，
  没有把缺少工程上下文的结果伪判为合规；
- 最终安装包：
  `releases/1.4.56/FPGA-HDL-Static-Analyzer-1.4.56-Darwin-arm64.zip`；
- 包体积：92,987,788 字节；SHA-256：
  `3a9aaabbee6c4fd824319d60572c893fddbf83793ceccb2d2c67440803c77aaa`；
- 独立临时目录解包后，CLI 报告 `FPGA HDL Static Analyzer 1.4.56`，应用包通过
  `codesign --verify --deep --strict`。
