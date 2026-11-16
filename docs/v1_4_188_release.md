# FPGA HDL Static Analyzer V1.4.188

## A-1-3 原生判据深化

- 公开编号严格使用官方三段式 `A-1-3`。
- 恢复端口/内部声明独立固定基准、Tab 展开和端口续行 `uint32` 回绕列。
- 恢复 Error、物理行、第 1 列映射及“<声明名>定义左端未对齐”正文。
- 同物理行多 AST_WIRE 节点、宏/include/generated 归属和专有名称规范化继续保持 `ManualReviewRequired`。

## 验证

- 新增端口/内部声明固定诊断和端口续行回绕断言；CTest `135/135` 全部通过。
- 附件审计覆盖544条安装规则、537个页面、209条 GJB 标准规则和446个示例：自动规则434个吻合、工程上下文规则6个吻合、4个明确需要上下文、2个原生实现与附件说明冲突，无未解释差异。
- 集成源码适配器由35条增至36条，剩余173条保持部分接入；完整原生等价仍为 `0/209`。
- 审计 JSON SHA-256：`3093d67d1e6a22d18c5fc5b2ac452a76ff5a41c8c9f779ef00789a13100a3f7d`。
- 原生规则台账 SHA-256：`aa17a883f0abfd5afbb2f19db782c45b26060d4fe257de4464bc648e57ad3b19`。
- 发布包：`releases/1.4.188/FPGA-HDL-Static-Analyzer-1.4.188-Darwin-arm64.zip`，SHA-256 `fab84b62a5c062a35ac19ac7db0e310da3df39270a7c501f423f2665cc276475`。
- 解包后 CLI 返回 `FPGA HDL Static Analyzer 1.4.188`；运行探针输出官方编号 `A-1-3`、Error、第4行第1列及正文“Y定义左端未对齐”；应用短版本和构建版本均为 `1.4.188`，并通过 `codesign --verify --deep --strict`。

详细证据见 [第五十二轮逆向记录](native_rule_recovery_pass52.md) 与 [A-1-3 原生证据](evidence/native_verilog_declaration_alignment_5_1_3_findings.md)。
