# FPGA HDL Static Analyzer V1.3.91

本版本只推进规则集准确性，不修改表格或其他界面格式。

## GJB10157.12.2

完成附件 `T-Linter.dll` 原生入口 `FUN_1805215d0`、FSM 候选处理函数 `FUN_180524530` 及 safe 模式诊断函数 `FUN_180522090` 的只读恢复。

产品现接入证据支持的受限判断：

- 复用严格的一进程和直接二进程 Verilog FSM 识别；
- 仅检查具有唯一声明、且可证明宽度大于 1 的状态变量；
- 缺少 safe 模式时报告 `状态机未设置为 safe 模式`；
- 接受 `fsm_encoding="safe"`、`synthesis syn_encoding="safe"` 和 `synthesis fsm_safe_state="state"`；
- 保留原生 `fsm_encoding="none"` 与一位状态寄存器排除语义；
- 无关位置的 safe 注释不能抑制告警。

详细证据见 [原生恢复记录](evidence/native_verilog_fsm_safe_mode_12_2_findings.md)。

## 验证

- 新增一/二进程违反样例、三种 safe 正例、onehot 反例、none/一位寄存器边界和无关注释隔离测试。
- 完整 CTest：131/131 通过。
- 附件审计：446 个文档样例中 435 个匹配、11 个不匹配；12.2 没有随包正反例，本次未改变既有样例结果。
- 原生台账：209 条规则中 201 条具有已接入的证据支持核心谓词，尚有 8 条需要继续解释；完整算法等价验证仍为 0 条。

## 当前边界

typedef/enum 和参数化位宽、宏与 generate、跨文件属性、完整原生网表识别及工程级综合设置仍需人工复核；未覆盖的无告警路径不自动判定合规，完整原生算法等价仍不宣称完成。
