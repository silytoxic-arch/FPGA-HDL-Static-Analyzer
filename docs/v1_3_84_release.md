# FPGA HDL Static Analyzer V1.3.84

本版本只推进规则准确性，不调整表格或其他界面格式。

## GJB9765.4.12.a

对附件 `T-Linter.dll` 的 slot 0 入口 `FUN_1805b9590` 和递归收集器 `FUN_1805ba200` 完成只读反编译，恢复出三个原生分支：

- entity `generic` 的直接子节点在两个整数类型标志均为空时报告“非整型generic不可综合”；产品检查可无歧义识别的标准非整数类型，用户 subtype/package 类型继续人工复核。
- VHDL `constant` 的完整直接初值精确为 `""` 时报告“空字符串常量不可综合”；非空、组合表达式和普通字符串表达式不报告。
- 仅 `wait for` 报告“wait for语句不可综合”；`wait until`、`wait on` 和裸 `wait;` 均不属于该原生分支。

旧版本把通用 `RTL-NONSYNTHESIZABLE-DELAY` 和 `VHDL-PROCESS-NO-SUSPENSION` 映射到本条款。原生收集器没有这些泛化分支，本版本已撤销，避免将 `after` 延时或无悬挂进程错误归入 4.12.a。

详细证据见 [原生恢复记录](evidence/native_vhdl_unsynthesizable_4_12_a_findings.md)。

## 验证

- 新增 integer/natural、标准非整数类型、用户 subtype、空字符串直接/组合初值及四类 wait 语句边界测试。
- 完整 CTest：131/131 通过。
- 附件审计：446 个文档示例中 430 个匹配、16 个保持既有不匹配；相对 V1.3.83 无回退。
- 原生台账：209 条规则中 194 条具有已接入的证据支持核心谓词，尚有 15 条需要继续解释；完整算法等价验证仍为 0 条。

## 当前边界

用户 subtype/package 类型传播、厂商 `+0x110` 字段正式含义、常量折叠、错误语法恢复及精确原生遍历顺序仍未恢复。上述未覆盖路径不视为已证明合规，完整厂商算法等价数仍为 0。
