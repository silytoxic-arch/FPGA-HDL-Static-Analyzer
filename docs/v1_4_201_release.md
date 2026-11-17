# FPGA HDL Static Analyzer V1.4.201

## R-3-2 原生判据深化

- 软件规则编号严格采用官方三段式 `R-3-2`。
- 恢复顶层 `AST_MODULE`、方向端口和十二个非 wire/reg 类型标志的组合边界。
- 恢复每个违规端口一条诊断，同一行相同正文不再被通用去重合并。
- 恢复固定正文 `顶层模块的输入输出端口必须只使用reg型和wire型数据`、Warning、端口物理行和第 1 列占位。
- `logic` 未命中十二个原生标志，保持不报告。

## 验证

- CTest `135/135` 全部通过。
- 附件审计覆盖 544 条安装规则、537 个页面、209 条 GJB 标准规则和 446 个示例：自动规则 434 个吻合、工程上下文规则 6 个吻合、4 个明确需要上下文、2 个原生实现与附件说明冲突，无未解释差异。
- 审计 JSON SHA-256：`0f1ee60b89ca7ecb7716e15725e49b181f9d59fdd7e6e5402411c31f560cf3e0`。
- 原生规则台账 SHA-256：`6ca429da8b351fedaa3778a9ccb6372eabc296d9adb353a8d35b19d3c6a15afe`。
- 209 条规则继续保留证据支持的核心判据和有界源码适配器；完整专有前端算法等价仍为 `0/209`。
- Windows 安装包只由 GitHub Actions 构建和保存；本地不下载、不保留安装包。

详细证据见 [第六十五轮逆向记录](native_rule_recovery_pass65.md) 与 [R-3-2 原生证据](evidence/native_verilog_top_port_types_5_3_2_findings.md)。
