# FPGA HDL Static Analyzer V1.4.187

## A-1-2 原生判据深化

- 公开编号严格使用官方三段式 `A-1-2`。
- 恢复 IF/ELSE 与 CASE 两类 AST 收集路径、固定首项列基准和逐错位记录多重性。
- 恢复 Error、违规物理行、第 1 列映射及固定正文。
- 专有前端列规范化、宏/include/generated 节点归属和畸形 AST 恢复继续保持 `ManualReviewRequired`。

## 验证

- 新增固定诊断和同一 CASE 项多表达式断言；CTest `135/135` 全部通过。
- 附件审计覆盖544条安装规则、537个页面、209条 GJB 标准规则和446个示例：自动规则434个吻合、工程上下文规则6个吻合、4个明确需要上下文、2个原生实现与附件说明冲突，无未解释差异。
- 集成源码适配器由34条增至35条，剩余174条保持部分接入；完整原生等价仍为 `0/209`。
- 审计 JSON SHA-256：`f46dc0707e9f2fa20bf09ccf1259cfeee384346861739b28185c28ac8cbc28d3`。
- 原生规则台账 SHA-256：`8b9adc36dd5df220bb11b199e4e2a8f7d6b9e9586460834156d322167d75eb32`。
- 发布包：`releases/1.4.187/FPGA-HDL-Static-Analyzer-1.4.187-Darwin-arm64.zip`，SHA-256 `41eb06e5fe8b1230b4c15e4696bfd361e1ade8e747d8dfb94fb49351c4fbc69f`。
- 解包后 CLI 返回 `FPGA HDL Static Analyzer 1.4.187`；运行探针输出官方编号 `A-1-2`、Error、第5行第1列及固定正文；应用短版本和构建版本均为 `1.4.187`，并通过 `codesign --verify --deep --strict`。

详细证据见 [第五十一轮逆向记录](native_rule_recovery_pass51.md) 与 [A-1-2 原生证据](evidence/native_verilog_statement_alignment_5_1_2_findings.md)。
