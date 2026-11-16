# FPGA HDL Static Analyzer V1.4.23

本版继续只处理规则集，不调整界面。

## GJB9765.4.14.b.1、b.2 原生读取块算法修正

DLL 静态证据确认两条规则分别由 `FUN_1805bda30` 与 `FUN_1805be220` 执行，
且都通过 `fopen("r")`、`fgets(..., 0x200, ...)` 读取源码。V1.4.23 已接入：

- 每个读取块最多 511 个数据字节，遇 LF 提前结束；
- 每个读取块递增一次诊断序号，超长物理行可能占用多个序号；
- b.1 对每块独立执行原生双字节集合和“先遇 Tab”判断；
- b.2 只识别块末尾的 `空格 + LF` 或 `空格 + CR + LF`；
- b.2 不会跨块拼接尾部空格与下一块 LF；
- 行尾 Tab 和未终止的最后一块仍不触发 b.2。

新增 511 字节边界、超长物理行第二块及诊断序号测试。产品按目标 Windows PE
和文本模式推断进行常规 CRLF→LF 转换；NUL、Ctrl-Z、代码页、流错误、重复源记录
与文件变化仍缺少运行态验证，因此无告警继续保持 `ManualReviewRequired`。

静态观察证据分别为
`ev_18d4943dfaaee8f0aa7b3c890ddda29c8b39c5fe4fd31f7481114ce8ba9ee4f6`
和 `ev_85d543f2dad139b11c46626c705dbcaade605b36564317949dfe2ab259432a27`；
详细记录见 [b.1](evidence/native_vhdl_leading_tab_4_14_b_1_findings.md) 与
[b.2](evidence/native_vhdl_trailing_space_4_14_b_2_findings.md)。

## 验证与附件审计

- 全量自动化测试：135/135 通过；
- 标准规则：209 条；附件示例探测：446 个；
- Automated：435 个吻合、1 个差异；唯一差异仍是已记录的 5.2.15 HTML 与 DLL 冲突；
- ProjectContext：4 个可判定示例吻合；6 个缺少工程上下文的示例保持
  `context-required`；
- 台账继续记录 `fullAlgorithmsVerified: 0`，不把核心谓词部分接入表述为完整等价。
