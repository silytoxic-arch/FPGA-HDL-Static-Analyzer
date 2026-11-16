# V1.4.48 规则恢复说明

本版继续只处理规则集，没有调整问题列表、代码规模明细或其他表格界面。

## GJB9765.4.2.c 顶层三态边界

对附件 `T-Linter.dll`（SHA-256
`60e6b52875f4d7c3fbdf494d869b3a4bcc3fdc8ee63d8620ea9614da5d6599b0`）的只读
Ghidra 反编译确认，4.2.c 并不是简单地只检查顶层：

- 递归收集器记录当前 VHDL entity 名称，并从直接端口容器收集 `+0x104 != 0` 的端口；
- 相邻 4.5.f.24 的原生分支和附件正反例已把 `+0x104` 确认到 `INOUT` 模式；
- 两个三态候选循环都比较当前 entity 与 `setTopModule` 的完整名称；
- 非顶层候选直接报告；顶层候选只有目标是 `INOUT` 时豁免。因此顶层 `OUT` 和非顶层
  `INOUT` 都属于本条告警范围。

产品现按 architecture/entity 隔离声明和端口集合，对有界直接、条件、进程内及索引 Z
赋值应用上述边界。没有显式顶层时保留推断根兼容模式；无诊断继续为
`ManualReviewRequired`。

## 回归覆盖

- 多根 VHDL：配置 `TOP_A` 时只豁免 `top_a` 的 INOUT，报告 `top_b`；改选 `top_b` 后
  报告位置随之切换；
- 所选顶层 `OUT` 端口被赋 Z 时报告；
- 既有内部 signal、索引目标、进程内赋值、条件谓词 Z 排除及顶层 INOUT 遵循例继续覆盖。

当前台账仍为 209/209 条核心谓词已接入、完整供应商算法等价验证 0 条。复杂关系候选生产、
别名/跨实例传播、配置/generate 和完整前端降低仍未恢复，未知路径不判合规。

## 发布验证

- 完整重配置、构建及 CTest：135/135 通过；
- 附件样例重新审计：446 个样例，其中 `Automated:matched` 435、
  `Automated:mismatch` 1、`ProjectContext:matched` 4、`context-required` 6，较 V1.4.47
  没有新增差异；
- 台账重新生成：209/209 核心谓词已接入，4.2.c 已关联本轮原生证据；
- 解包后的 CLI 报告 `FPGA HDL Static Analyzer 1.4.48`；
- 解包后的 macOS 应用通过 `codesign --verify --deep --strict`；
- 发布包：`releases/1.4.48/FPGA-HDL-Static-Analyzer-1.4.48-Darwin-arm64.zip`，
  90,799,369 字节，SHA-256
  `12387487a368b823f8f9dcab2f1964438021096cdc4cd6a41f79cfdd8ad72fd3`。
