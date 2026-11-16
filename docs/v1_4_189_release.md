# FPGA HDL Static Analyzer V1.4.189

## A-1-1 原生判据深化

- 公开编号严格使用官方三段式 `A-1-1`。
- 恢复外部有序关键字映射的缺项顺序与每模块一次的组合诊断。
- 恢复“文件缺少头部注释”和“文件头部注释中缺少\"…\"的注释声明”固定正文。
- 恢复模块物理行、无原生列字段的定位，产品映射为第 1 列。
- 厂商默认关键字集合仍未知，产品默认模板不冒充原生默认策略。

## 验证

- 增加无前置行、单缺项、多缺项排序及固定标点断言；CTest `135/135` 全部通过。
- 附件审计覆盖 544 条安装规则、537 个页面、209 条 GJB 标准规则和 446 个示例：自动规则 434 个吻合、工程上下文规则 6 个吻合、4 个明确需要上下文、2 个原生实现与附件说明冲突，无未解释差异。
- 集成源码适配器由 36 条增至 37 条，剩余 172 条保持部分接入；完整原生等价仍为 `0/209`。
- 审计 JSON SHA-256：`486d0f84bbcb05c7186e07ccb4e3a70c5bddc031a78bf2adda94aced059daa95`。
- 原生规则台账 SHA-256：`7c4640fa0bd9537f95ca4c82eedd604fd145970a52408ae7ef4d9dfe1a9c5570`。
- 发布包：`releases/1.4.189/FPGA-HDL-Static-Analyzer-1.4.189-Darwin-arm64.zip`，SHA-256 `ff59ac327b1c662a515c2ab29473c9c4bd1876b5f1105e7beb36af37a7aa51d5`。
- 解包后 CLI 返回 `FPGA HDL Static Analyzer 1.4.189`；显式策略探针输出官方编号 `A-1-1`、Warning、第 2 行第 1 列及正文“文件头部注释中缺少\"Author、Revision History\"的注释声明”；应用短版本和构建版本均为 `1.4.189`，并通过 `codesign --verify --deep --strict`。

详细证据见 [第五十三轮逆向记录](native_rule_recovery_pass53.md) 与 [A-1-1 原生证据](evidence/native_verilog_header_5_1_1_findings.md)。
