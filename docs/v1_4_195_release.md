# FPGA HDL Static Analyzer V1.4.195

## R-2-8 原生判据深化

- 软件规则编号严格采用官方三段式 `R-2-8`；内部条款键不出现在用户界面及报告中。
- 恢复十三类 wire 标志、`localparam`/普通 `parameter` 分界和逐对象诊断重数。
- 恢复固定 Error、wire/localparam 两类精确正文、对象物理行及原生无列定位。
- 产品将原生无列稳定映射为第 1 列，并保留直接模块声明的证据边界。

## 验证

- 增加固定正文、严重度与位置断言；保留十三类、逗号继承、ANSI、条件编译和作用域边界覆盖。
- CTest `135/135` 全部通过。
- 附件审计覆盖 544 条安装规则、537 个页面、209 条 GJB 标准规则和 446 个示例：自动规则 434 个吻合、工程上下文规则 6 个吻合、4 个明确需要上下文、2 个原生实现与附件说明冲突，无未解释差异。
- 集成源码适配器由 42 条增至 43 条，剩余 166 条保持部分接入；完整原生等价仍为 `0/209`。
- 审计 JSON SHA-256：`91d788c6fe6341fcacfc5d90d715e3097faa7308a18a708fbfb94be6d6eeed21`。
- 原生规则台账 SHA-256：`931f8ef41384b7027ad6c66cdc04d7411619c20695a5bd8845f69191511d1367`。
- 发布包：`releases/1.4.195/FPGA-HDL-Static-Analyzer-1.4.195-Darwin-arm64.zip`；归档校验值以同目录 `SHA256SUMS` 为准。
- 解包后 CLI 返回 `FPGA HDL Static Analyzer 1.4.195`；探针输出两条官方编号 `R-2-8`、Error、第 2/3 行第 1 列及精确 wire/localparam 正文，JSON 中不存在内部条款键。应用短版本为 `1.4.195`，并通过 `codesign --verify --deep --strict` 与归档 SHA-256 校验。

详细证据见 [第五十九轮逆向记录](native_rule_recovery_pass59.md) 与 [R-2-8 原生证据](evidence/native_parameter_types_5_2_8_findings.md)。
