# FPGA HDL Static Analyzer V1.4.40

本版继续只处理规则集，未调整任何表格或界面。

GJB10157.9.2.11 的 always 根接纳辅助器 `FUN_1804e66e0` 已通过定点静态反汇编完整恢复。它只接纳 `AST_ALWAYS(0x52)`，初始结果为真，并逐个检查直接子节点；出现 `AST_POSEDGE(0x65)` 或 `AST_NEGEDGE(0x66)` 即拒绝。空 always、普通 `AST_EDGE(0x67)` 和其他子节点均不构成拒绝条件。虚表 slot 2 在此之外继续显式接纳 `AST_TASK(3)` 与 `AST_FUNCTION(4)`。

产品适配器由原先的三个显式组合写法扩展为与原生入口一致的有界来源集合：`always @*`、无边沿的 `always @(...)`、无事件控制的 bare always、`always_comb`、`always_latch`、以及命名事件控制；`always_ff` 也按实际事件控制判定，含直接 posedge/negedge 时排除。上一版恢复的一层 task/function 直接调用展开保持不变。

新增边界测试覆盖 bare always、always_latch、带 posedge 的 always_ff 和命名事件；同步过程、initial、普通 case、未调用/重复/间接子程序及条件编译回归继续保留。复杂事件表达式的完整 AST 降低、`+0x183` 字段正式名称、跨文件定义与递归调用仍保持未知，无告警不伪判为完整合规。

完整 CTest 135/135 通过。附件 446 个示例审计保持 435 个自动匹配、1 个已知文档/二进制判据差异、4 个工程上下文匹配及 6 个缺少工程上下文；209 条核心规则保持接入，完整原生算法验证仍为 0 条。

详细证据见 [native_verilog_comb_if_else_9_2_11_findings.md](evidence/native_verilog_comb_if_else_9_2_11_findings.md) 和 [FUN_1804e66e0 静态反汇编记录](evidence/native_helper_1804e66e0_disassembly.txt)。
