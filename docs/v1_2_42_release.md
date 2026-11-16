# V1.2.42 VHDL 赋值符号空格规则

新增 GJB9765.4.14.c.2 的有界检测：VHDL `<=`、`:=` 两侧缺少空白时逐处报告 Warning。附件的违反与遵循例均匹配，另有单侧缺失、两侧缺失、Tab、注释与字符串边界测试。原生证据和产品安全差异见 [恢复记录](evidence/native_vhdl_assignment_spacing_4_14_c_2_findings.md)。

公开样例审计为 446 例：本条正反例均由 ManualReview 类转入 Automated 类并匹配；Automated 类匹配数由 185 增至 187，失配维持 17。逐条台账现为 209 条中 48 条核心谓词部分接入、161 条待解释、0 条完整等价验证；无告警仍需人工复核。
