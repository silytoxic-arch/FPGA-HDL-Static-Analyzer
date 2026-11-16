# FPGA HDL Static Analyzer V1.4.183

## A-1-8 原生判据深化

- 公开编号严格使用官方三段式 `A-1-8`。
- 恢复工程级累计、唯一 Verilog 模块根门控、VHDL/另一前端根参与和同文件多根计权。
- 恢复物理行扫描、空行分母、原始 `//`、`--`、`/*...*/` 标记识别及 20% 阈值。
- 恢复原生 Error 严重度、空文件路径第 1 行和固定正文“该工程代码注释率小于20%”。
- 重复 VHDL 根选择、专有前端编码与错误恢复、虚拟文件及完整宏生成路径继续保持 `ManualReviewRequired`。

## 验证

- 新增工程聚合、模块根计权、字符串原始标记、混合 Verilog/VHDL 根和固定诊断断言；CTest `135/135` 全部通过。
- 附件审计覆盖544条安装规则、537个页面、209条 GJB 标准规则和446个示例：自动规则434个吻合、工程上下文规则6个吻合、4个明确需要上下文、2个原生实现与附件说明冲突，无未解释差异。
- 集成源码适配器由30条增至31条，剩余178条保持部分接入；完整原生等价仍为 `0/209`。
- 审计 JSON SHA-256：`ae658bfb148d28e8eda8d4a2aedd9cbc4da42136be781cee534d4831722fb46f`。
- 原生规则台账 SHA-256：`e6c640b962070abf25a019c91bc70129a21baa3d40978d812329227ac2c80924`。
- 发布包：`releases/1.4.183/FPGA-HDL-Static-Analyzer-1.4.183-Darwin-arm64.zip`，SHA-256 `36e2a3ee111612f98c3f8412c5b660b35cd9fd32d61fbd6509f206739a8514f6`。
- 解包后 CLI 返回 `FPGA HDL Static Analyzer 1.4.183`；运行探针输出官方编号 `A-1-8`、Error、空文件路径第1行及固定正文“该工程代码注释率小于20%”；应用短版本和构建版本均为 `1.4.183`，并通过 `codesign --verify --deep --strict`。

详细证据见 [第四十七轮逆向记录](native_rule_recovery_pass47.md) 与 [A-1-8 原生证据](evidence/native_verilog_comment_ratio_5_1_8_findings.md)。

