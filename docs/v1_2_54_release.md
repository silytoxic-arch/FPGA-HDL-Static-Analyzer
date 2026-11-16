# V1.2.54 GJB9765.4.8.a 固定常量 CASE 规则

从附件 `T-Linter.dll` 恢复该规则的核心 AST 判据：CASE 选择器为字面量、已收集常量名称，或复合表达式中所有标识符均为已收集常量时报告。软件保守实现 architecture 声明常量及有界表达式，并排除 `case generate`。选择器跨行时定位于选择器而非 `case` 关键字。

附件违反例与遵循例均吻合；446 个文档示例中 Automated 吻合 204、不吻合 16。台账共 209 条，其中 58 条核心判据部分接入，151 条仍需解释，0 条宣称完整等价。无本规则诊断时保持 `ManualReviewRequired`。

证据：[原生恢复](evidence/native_vhdl_case_constant_4_8_a_findings.md)、[示例审计](evidence/t_linter_after_1_2_54.md)。未知作用域、函数计算与完整 AST 类型映射尚未恢复。
