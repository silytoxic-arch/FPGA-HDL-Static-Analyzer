# FPGA HDL Static Analyzer V1.4.217

## R-6-1 原生判据深化

- 软件规则编号严格使用官方三段式 `R-6-1`。
- 恢复 IF 降低树最终 default/else 与此前全部分支的赋值目标表比较。
- 恢复空复位分支仍逐个报告最终 else 全部目标的原生边界。
- 恢复 `赋值物理行 <= 最终 default 行` 的原生截止，同一行的最终 else 赋值不误报。
- 恢复逐缺失目标的报告重数、精确 `Error` 正文、赋值物理行及无原生列定位。
- 支持有界 IF/ELSE-IF 链，并增加多分支、空复位分支、同行截止及嵌套同步数据条件测试。
- 未恢复的综合关系生产、位级目标归一化和工程上下文继续要求人工复核。

Windows 安装包只由 GitHub Actions 构建和保存；本地不生成、下载或保留 Windows 包。

## 验证

- 本地无 GUI 验证构建 CTest `133/133` 全部通过，其中定向
  `standard_accuracy_unit` 通过。
- 附件审计覆盖 544 条安装规则、537 个页面、209 条 GJB 标准规则和 446 个示例：
  自动规则 434 个吻合、工程上下文规则 6 个吻合、4 个明确需要上下文、2 个已记录的
  原生实现与附件说明冲突，无未解释差异。
- 审计 JSON SHA-256：`18684f39c9559dafa9eae2e4ee440b4d9954840c555a5971abcfcd9e81a06b3b`。
- 原生规则台账 SHA-256：`80e833dde7df40c59cfc78335cc5f11b44dca22e79c48161c2d59a5c8b258819`。
- 第八十一轮静态证据 SHA-256：`cd0814abe5239538f913b1cebdb8d3af3a5c66f17dbb74daf7923e0e3e293ef7`。
- Windows x64 成品由本提交推送后的 GitHub Actions 构建；本地不生成或下载 Windows 包。
- 209 条规则继续区分已观察事实、产品映射和未知边界；完整专有前端算法等价仍为
  `0/209`。

详细证据见 [第八十一轮逆向记录](native_rule_recovery_pass81.md) 与
[R-6-1 原生证据](evidence/native_verilog_flipflop_reset_coverage_7_1_1_7_1_2_findings.md)。
