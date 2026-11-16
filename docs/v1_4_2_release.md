# FPGA HDL Static Analyzer V1.4.2

本版继续只修正规则逻辑，不调整界面。

## GJB10157.5.2.3 关键字命名规则修正

附件 DLL 的规则入口按固定顺序读取六类关键字表：VHDL、Verilog、SystemVerilog、SDF/EDIF、PSL、Windows。附件 `runCheck.exe` 对本规则只调用通用规则开关，没有提供类别配置，因此不能把厂商 GUI 的隐藏默认值当作已恢复事实。

附件规则页同时明确规定“禁止使用 VHDL、Verilog、SystemVerilog、SDF 和 EDIF 关键字”。本版据此加入标准文本默认策略：

- 默认启用 VHDL、Verilog、SystemVerilog、SDF/EDIF 四类；
- 默认不扩展到规则文本未提及的 PSL 与 Windows 类别；
- 工程显式类别配置优先，包括显式全部关闭；
- 语法异常、宏和重复模块根等未证明输入不制造诊断；
- 未命中仍保持人工复核，不将局部检测冒充整条规则合规。

详细恢复证据和剩余未知项见 [GJB10157.5.2.3 关键字命名规则证据](evidence/native_verilog_keyword_5_2_3_findings.md)。

## 验证

- 新增默认四类命中、PSL/Windows 默认排除、显式全关、显式六类和异常源码边界测试。
- 附件审计：386 个 Automated 示例全部匹配；45 个 ManualReview 示例匹配、1 个保留已记录的文档/原生逻辑冲突；8 个 ProjectContext 示例匹配、2 个仍需工程上下文；4 个缺少被引用源文件的片段保持 context-required。
- GJB10157.5.2.3 附件正反例均匹配，原 ProjectContext 反例不匹配已消除。
- 完整 CTest：135/135 通过。
- 用户 RTL 回归：34 个文件、203 个设计单元、34,383 条诊断、0 个分析错误；本次规则没有在该工程中制造新增误报。
