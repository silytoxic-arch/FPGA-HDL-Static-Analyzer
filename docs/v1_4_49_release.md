# V1.4.49 规则恢复说明

本版继续只处理规则集，没有调整问题列表、代码规模明细或其他表格界面。

## GJB9765.4.2.c 候选结构收窄

在 V1.4.48 已恢复“只有所选顶层 INOUT 可豁免”的边界基础上，本轮继续读取附件
`T-Linter.dll` 中 `FUN_18078c370` 的完整候选构造控制流：

- 直接分支接纳 `target <= data when relation else Z`，要求直接名称/选择数据、双操作数关系
  条件和 inactive Z；
- `0xaa/0x58/0xac` 分支对应两臂 IF：THEN 侧记录目标及数据，最终 ELSE 侧只有在同一目标
  被赋 Z 时才构成候选；
- 两条路径均继续应用显式所选顶层 INOUT 豁免，非顶层 INOUT 和顶层 OUT 仍报告。

产品检测已收窄到上述有证据的源码形态。新增正反例覆盖配对 IF/ELSE、无 ELSE 单臂 IF、
裸 Z 及反向 THEN-Z/ELSE-data；后三类不再因为出现 Z 字面量而误报。未知语法仍保持
`ManualReviewRequired`。

## 尚未恢复

条件操作数的全部声明类别、厂商字面量类型 `0xb2/0xb3` 的正式名称、别名/跨实例传播、
configuration/generate、聚合与复杂选择目标、异常语法恢复及原生精确诊断重数仍未知，不宣称
完整供应商算法等价。

## 发布验证

- 完整重配置、构建及 CTest：135/135 通过；
- 附件样例重新审计：446 个样例，其中 `Automated:matched` 435、
  `Automated:mismatch` 1、`ProjectContext:matched` 4、`context-required` 6，较 V1.4.48
  没有新增差异；
- 台账重新生成：209/209 核心谓词已接入，完整供应商算法等价验证仍为 0；
- 解包后的 CLI 报告 `FPGA HDL Static Analyzer 1.4.49`；
- 解包后的 macOS 应用通过 `codesign --verify --deep --strict`；
- 发布包：`releases/1.4.49/FPGA-HDL-Static-Analyzer-1.4.49-Darwin-arm64.zip`，
  91,076,882 字节，SHA-256
  `afc36d8721ca2aac71f950ee836ba64d4858b85d5025b6aba1860e598e44cf08`。
