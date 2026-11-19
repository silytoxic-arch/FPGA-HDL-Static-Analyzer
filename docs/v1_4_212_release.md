# FPGA HDL Static Analyzer V1.4.212

## R-5-2 原生判据深化

- 软件规则编号严格采用官方三段式 `R-5-2`。
- 恢复恰好一个边沿事件的过程筛选，以及只检查首个布尔化条件的行为。
- 条件中的每个异步控制标识符独立报告，同一物理行的重复标识符不再被折叠。
- 恢复精确原生正文、`Error` 和匹配标识符物理行无列定位。
- 跨层次复位关系、库原语端口和非 `if` 条件降低尚未完整恢复，无告警继续为
  `ManualReviewRequired`。

Windows 安装包只由 GitHub Actions 构建和保存；本地不下载、不保留 Windows 安装包。

## 验证

- CTest `135/135` 全部通过。
- 附件审计覆盖 544 条安装规则、537 个页面、209 条 GJB 标准规则和 446 个示例：
  自动规则 434 个吻合、工程上下文规则 6 个吻合、4 个明确需要上下文、2 个原生
  实现与附件说明冲突，无未解释差异。
- 审计 JSON SHA-256：`bff1e80ecaa0fa61d54827b7fec4868e736dbd51cad8206c59fb2284dcc47115`。
- 原生规则台账 SHA-256：`c400d8be3a83eb30396dc2a64bcbc84d984a56878820822da76269f3f1c115ac`。
- 第七十六轮静态证据 SHA-256：`e4eab9d0ca8aaac201f97273e4a60424aedd16b5b7693dd3fb870bed882db1c0`。
- 209 条规则保留证据支持的核心判据和有界源码适配器；完整专有前端算法等价
  仍为 `0/209`。

详细证据见 [第七十六轮逆向记录](native_rule_recovery_pass76.md) 与
[R-5-2 原生证据](evidence/native_verilog_mixed_reset_mode_6_2_2_findings.md)。
