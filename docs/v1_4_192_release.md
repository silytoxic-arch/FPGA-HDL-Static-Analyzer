# FPGA HDL Static Analyzer V1.4.192

## R-2-4 原生判据深化

- 公开编号严格使用官方三段式 `R-2-4`。
- 恢复唯一模块组边界和六类 AST 声明集合。
- 恢复五个固定名称的 C `tolower` 精确等长比较、每声明节点多重性、固定 Error 和精确正文。
- 原生报告无列字段；产品以声明物理行、第 1 列承载，不夸大尚未完整恢复的专有 AST 位置字段生产过程。
- 非 ASCII 区域设置、跨文件重复模块及完整宏/include/generated 前端继续保持人工复核。

## 验证

- 增加固定正文、名称大小写保留、严重度、位置、同名同位置多重性、重复模块组、不同唯一模块组及函数后代断言；CTest `135/135` 全部通过。
- 附件审计覆盖 544 条安装规则、537 个页面、209 条 GJB 标准规则和 446 个示例：自动规则 434 个吻合、工程上下文规则 6 个吻合、4 个明确需要上下文、2 个原生实现与附件说明冲突，无未解释差异。
- 集成源码适配器由 39 条增至 40 条，剩余 169 条保持部分接入；完整原生等价仍为 `0/209`。
- 审计 JSON SHA-256：`ea999d56dfaefcb9d133e5b603d27dc80e1bba338de92cd9266d1a7d73cdf8d4`。
- 原生规则台账 SHA-256：`938e12bf6f86f47e36433638b64c75881ec3cea1facab2d9111795d05d8c58f3`。
- 发布包：`releases/1.4.192/FPGA-HDL-Static-Analyzer-1.4.192-Darwin-arm64.zip`；校验值见同目录 `SHA256SUMS.txt`，避免把归档自身摘要写回归档造成自引用。
- 解包后 CLI 返回 `FPGA HDL Static Analyzer 1.4.192`；探针输出三条官方编号 `R-2-4`、Error 和第 1 列，其中同一行两个 `VDD` 保留两条精确正文，混合大小写 `vCc` 保留原始拼写；应用短版本和构建版本均为 `1.4.192`，并通过 `codesign --verify --deep --strict`。

详细证据见 [第五十六轮逆向记录](native_rule_recovery_pass56.md) 与 [R-2-4 原生证据](evidence/native_verilog_power_names_5_2_4_findings.md)。
