# V1.2.56 GJB9765.4.5.f.1 信号名称长度规则

附件原生构造默认长度范围为 1–20 字节，运行时对选定 AST 声明节点按上下界分别报告。软件修正为检查明确的端口与内部 `signal` 声明，删除把所有设计单元名称一律作为信号名称的旧判断。

附件违反例检出长端口和长内部信号；遵循例的长 entity 名不再误报。446 个文档示例中 Automated 吻合 206、不吻合 14。台账共 209 条规则：60 条核心判据部分接入，149 条仍需解释，0 条宣称完整等价。无本规则诊断时仍为 `ManualReviewRequired`。

证据：[原生恢复](evidence/native_vhdl_signal_name_length_4_5_f_1_findings.md)、[示例审计](evidence/t_linter_after_1_2_56.md)。可配置阈值与其他原生 AST 类别尚未映射。
