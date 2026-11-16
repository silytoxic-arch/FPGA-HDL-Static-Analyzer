# V1.4.81：R-13-7 基本门网表反馈深化

本轮只深化规则判断，不修改界面或表格布局。公开编号继续使用官方三段式 `R-13-7`。

## 原生恢复

- 原生入口 `FUN_1806e2530` 通过 `FUN_1806e04f0` 构造 elaborated 模块连接、cell 端口和信号位图，再由 `FUN_180565f90`、`FUN_1806e3910` 返回强连通结果。
- 入口没有把图限制为源码 `assign/always`，也没有在进入 SCC 路径前仅保留少数 `$and/$or` cell；因此规则边界包含前端降低后的通用组合网表连接。
- 结果名称去除内部 `$` 后缀，以“、”连接，级别为 `Warning`，诊断前缀为“检测到组合回路”。
- 详细证据见 [R-13-7 原生证据](evidence/native_verilog_combinational_feedback_9_4_7_findings.md)。

## 产品接入

- 保留连续赋值、组合过程和常量位选择的语义 Tarjan 图。
- 将已有标量基本门/布尔表达式网表适配器的 primitive-only SCC 接入 `R-13-7`；支持有界的 `and/or/nand/nor/xor/xnor/not/buf` 原语集合。
- 已被语义图报告的连续赋值组件不重复输出；三个及以上原语闭环不套用 7.2.2 的门数上限。
- 动态选择、宽向量、层次路径、黑盒内部、未支持 cell 与需常量折叠才能判定的路径继续人工复核。

## 验证

- 新增连续赋值去重、双原语闭环、三原语闭环、原生中文消息与无环原语链边界测试。
- 自动化测试：`135/135` 通过。
- 209 条附件规则审计共探测 446 个示例：`Automated:matched` 435、`ProjectContext:matched` 6、`context-required` 4、原生与文档冲突 1，与 V1.4.80 基线一致。
- `R-13-7` 台账提升为 `evidence-supported-core-predicate; integrated-source-adapter`；此类适配器现为 12 条，完整厂商算法等价仍为 `0/209`。
- 209 条公开编号全部通过 `R-X-X/A-X-X` 三段格式校验。
- 发布包：`releases/1.4.81/FPGA-HDL-Static-Analyzer-1.4.81-Darwin-arm64.zip`。
- SHA-256：`718ecc36f7d93b4f6e5c7be928c2ae074dd0585c5c2973259628dc55bbc36888`。
