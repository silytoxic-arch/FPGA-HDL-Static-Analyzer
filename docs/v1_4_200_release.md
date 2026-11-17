# FPGA HDL Static Analyzer V1.4.200

## R-3-1 原生判据深化

- 软件规则编号严格采用官方三段式 `R-3-1`。
- 恢复 `AST_CELL` 根和直接 `AST_ARGUMENT` 父子边界。
- 恢复每个违规实参一条诊断，同一行相同正文不再被通用去重合并。
- 恢复固定正文 `端口例化时禁止使用表达式`、Warning、实参物理行和第 1 列。
- 保留常量、标识符、层次标识符、选择及空连接的原生允许边界。

## 验证

- CTest `135/135` 全部通过。
- 附件审计覆盖 544 条安装规则、537 个页面、209 条 GJB 标准规则和 446 个示例：自动规则 434 个吻合、工程上下文规则 6 个吻合、4 个明确需要上下文、2 个原生实现与附件说明冲突，无未解释差异。
- 审计 JSON SHA-256：`0e45d038392ae6edd16dc912a3828e04a25eb534d41f2c7a6d8088f51e0a3b88`。
- 原生规则台账 SHA-256：`c6d925e29cdb8c5fcbf9b622d2737246fc08aec01955dcf8da446eb1af36e1e4`。
- 209 条规则均已有证据支持的核心判据和有界源码适配器；完整专有前端算法等价仍为 `0/209`。
- Windows 安装包只由 GitHub Actions 构建和保存；本地不下载、不保留安装包。

详细证据见 [第六十四轮逆向记录](native_rule_recovery_pass64.md) 与 [R-3-1 原生证据](evidence/native_verilog_port_actual_expression_5_3_1_findings.md)。

