# FPGA HDL Static Analyzer V1.4.0

本版不调整界面，继续收紧规则恢复与验证边界。

## 附件示例验证修正

T-Linter 的 VHDL 文件名规则示例没有携带原始源文件名。旧审计统一使用 `example.vhd`，会人为制造 4.5.f.5、4.5.f.9 的合规样例误报，并把只提供 `use work...` 或 package declaration 的 4.5.f.9/.10 片段错误当成可独立验证。

审计器现按已恢复谓词合成有证据约束的文件名环境：

- 4.5.f.5：从 entity 名恢复 `<entity>.vhd`；
- 4.5.f.9：从 package declaration 恢复 `<package>.vhd`；
- 4.5.f.10：从 package body 恢复 `<package>_body.vhd`；
- 仅有 USE/declaration 而缺少目标源文件的片段标为“需要工程上下文”，不判通过或失败。

新增 8 个文件名环境边界断言，并接入 CTest。

校正后的附件审计结果为：380 个 Automated 示例全部匹配；45 个 ManualReview 示例匹配、1 个保留已记录的文档/原生逻辑冲突；11 个 ProjectContext 示例匹配、5 个仍需工程上下文；另有 4 个片段因缺少被引用源文件而明确记为 context-required。

## 规则恢复状态

209 条规则均保留证据支持的核心谓词部分接入。此数字不表示完整商业前端或完整厂商算法等价；完整算法验证仍为 0 条。缺少跨文件绑定、配置、宏展开、层次传播或专有数据模型的结果继续保留人工复核或工程上下文状态。

完整 CTest：135/135 通过。

macOS Apple Silicon 发布包已完成压缩数据、应用版本和代码签名校验；SHA-256：`8b76bd4359962c02e6d03adbe955e1a0bbc2d3de56ac896ab0a3c7d7d518f503`。
