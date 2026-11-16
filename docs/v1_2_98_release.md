# V1.2.98 规则集进展

本版本只推进规则判断，不调整表格或其他界面格式。

## GJB10157.8.7

- 重新反编译主入口 `FUN_180577640`、准入槽 `FUN_1805790b0` 和算术 helper
  `FUN_1805790d0`。
- 恢复 ADD/SUB、MUL、DIV/MOD、POW 的独立原生分派。
- 确认 MUL 将全部直接子操作数的推断位宽相加，并与赋值目标宽度比较。
- 产品新增固定宽度、两个直接信号操作数的乘法结果溢出检查。
- 新增 3 位乘 3 位写入 5 位目标的反例，以及写入 6 位目标的边界正例。
- DIV/MOD/POW、完整 ADD/SUB 位模式和复杂表达式仍保持人工复核。

详细证据见
[`native_arithmetic_result_width_8_7_findings.md`](evidence/native_arithmetic_result_width_8_7_findings.md)。
