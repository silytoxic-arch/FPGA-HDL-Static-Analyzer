# FPGA HDL Static Analyzer V1.4.4

本版继续只修正规则逻辑与规则状态登记，不调整界面。

## 自动检测器注册表校正

27 条已有确定违规检测能力的规则此前仍标为 `ManualReview`。本版依据已有 DLL 恢复证据、可执行实现和正反例/边界测试，将它们登记为 `Automated`，覆盖：

- Verilog 状态机、safe 模式、上电初始化、基本门锁存器、case 标签、递归函数、repeat 变量和复杂索引；
- VHDL 输出端口回读、静态常量/类型混用、时钟再汇聚、复位网络和状态机结构。

这不是“自动合规认证”：检测器命中才是确定违规，未命中仍输出 `ManualReviewRequired`。完整规则清单和边界见 [V1.4.4 自动检测器注册表审计](evidence/automated_detector_registry_1_4_4.md)。

## 209 条规则状态

- 204 条：有确定违规自动检测器；
- 1 条：`GJB10157.5.2.15` 因附件合规示例与 DLL 默认逻辑冲突，保留 ManualReview；
- 4 条：需要时钟域、层次、外部文件或子模块端口定义，保留 ProjectContext。

## 附件审计

- 432 个 Automated 样例全部匹配；
- ManualReview：1 个匹配、1 个保留已记录冲突；
- ProjectContext：7 个匹配、1 个因附件缺少子模块定义而无法安全判断；
- 4 个缺少被引用源文件的片段保持 context-required。
- 完整 CTest：135/135 通过。
- 本版只校正规则检测器登记，不改变诊断判据或用户 RTL 的违规数量。
