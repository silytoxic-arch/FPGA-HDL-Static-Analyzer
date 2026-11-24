# FPGA HDL Static Analyzer V1.4.215

## A-5-3 原生判据深化

- 软件规则编号严格使用官方三段式 `A-5-3`。
- 恢复带边沿 `AST_ALWAYS` 筛选和 `AST_ASSIGN_EQ/LE` 目标匹配。
- 恢复精确原生正文 `异步复位<名称>由时序逻辑驱动`、`Warning` 及赋值物理行无列定位。
- 恢复逐匹配赋值节点的报告重数：同一复位信号两个独立赋值产生两条问题。
- 增加 blocking/nonblocking 双赋值、精确正文/级别/定位、`<=` 比较式、组合复位和外部复位回归。
- 未恢复的异步复位关系生产图、锁存器路径和专有前端边界仍保持人工复核。

Windows 安装包只由 GitHub Actions 构建和保存；本地不下载、不保留 Windows 安装包。

## 验证

- 本地无 GUI 验证构建 CTest `133/133` 全部通过，其中定向
  `standard_accuracy_unit` 通过。
- 附件审计稳定覆盖 544 条安装规则、537 个页面、209 条 GJB 标准规则和
  446 个示例：自动规则 434 个吻合、工程上下文规则 6 个吻合、4 个明确需要上下文、
  2 个已记录的原生实现与附件说明冲突，无未解释差异。
- 审计 JSON SHA-256：`d42cec46ca9b8b13c690b0099b6609f8087e5312e7516a3baafdefbd511eb020`。
- 原生规则台账 SHA-256：`3631b76ff1f0a3e484765b65d7a695565057f28f685efd24cec26b447dd4cf0d`。
- 第七十九轮静态证据 SHA-256：`1e78f6206751f9b2f571871c1be54331202a8b83e5d122bc470307b3dce05fc0`。
- Windows x64 成品由本提交推送后的 GitHub Actions 构建；本地不生成或下载 Windows 包。
- 209 条规则继续区分已观察事实、产品映射和未知边界；完整专有前端算法等价仍为 `0/209`。

详细证据见 [第七十九轮逆向记录](native_rule_recovery_pass79.md) 与
[A-5-3 原生证据](evidence/native_verilog_internal_sequential_reset_6_2_8_findings.md)。
