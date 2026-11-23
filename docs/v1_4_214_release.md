# FPGA HDL Static Analyzer V1.4.214

## R-5-5 原生判据深化

- 软件规则编号严格采用官方三段式 `R-5-5`。
- 恢复精确原生正文 `必须对寄存器<名称>进行上电初始化,赋固定值`、`Error` 和赋值
  目标物理行无列定位。
- 恢复“归一化目标名 + 物理行”的记录粒度：同名跨行逐条报告，同一行重复合并。
- 确认 selector 启用时 DLL 的 initial 收集器只登记赋值目标，不校验 RHS 是否固定值；
  产品不增加缺少原生证据的常量判断。
- 复杂 default/reset 分支、厂商器件分类和专有候选图仍保持人工复核。

Windows 安装包只由 GitHub Actions 构建和保存；本地不下载、不保留 Windows 安装包。

## 验证

- CTest `135/135` 全部通过，其中定向 `standard_accuracy_unit` 同样通过。
- 附件审计覆盖 544 条安装规则、537 个页面、209 条 GJB 标准规则和 446 个示例：
  自动规则 434 个吻合、工程上下文规则 6 个吻合、4 个明确需要上下文、2 个原生
  实现与附件说明冲突，无未解释差异。
- 审计 JSON SHA-256：`33f6b1629fd2ed91ee81383089ee0c6848bb404380e41d4746ccd8a9e1d90d2a`。
- 原生规则台账 SHA-256：`c7b43f2d0fd6cd28c9c9a824ad52db876a64f43fc4cf804f6ff31846987ff078`。
- 第七十八轮静态证据 SHA-256：`a64d64cb47e9799977825b9b6b6ec959e8458255a8978aea52603b4e2832ea47`。
- 209 条规则保留证据支持的核心判据和有界源码适配器；完整专有前端算法等价
  仍为 `0/209`。

详细证据见 [第七十八轮逆向记录](native_rule_recovery_pass78.md) 与
[R-5-5 原生证据](evidence/native_verilog_power_initialization_6_2_5_findings.md)。
