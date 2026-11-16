# FPGA HDL Static Analyzer V1.4.190

## R-2-1 原生判据深化

- 公开编号严格使用官方三段式 `R-2-1`。
- 恢复唯一模块组边界和七类 AST 声明集合。
- 恢复每声明节点多重性、固定 Error 与“<名称>命名必须以字母开始”正文。
- 恢复声明物理行、原生无列字段的定位，产品映射为第 1 列。
- 跨文件重复模块、完整宏/include/generated 前端及转义/非 ASCII 规范化继续保持人工复核。

## 验证

- 增加固定正文、严重度、位置、同名同位置多重性、重复模块组和不同唯一模块组断言；CTest `135/135` 全部通过。
- 附件审计覆盖 544 条安装规则、537 个页面、209 条 GJB 标准规则和 446 个示例：自动规则 434 个吻合、工程上下文规则 6 个吻合、4 个明确需要上下文、2 个原生实现与附件说明冲突，无未解释差异。
- 集成源码适配器由 37 条增至 38 条，剩余 171 条保持部分接入；完整原生等价仍为 `0/209`。
- 审计 JSON SHA-256：`e8a813423043f665e4ab19249c731c2f19e5e135cd912e55e8dc71030f74dfd3`。
- 原生规则台账 SHA-256：`44cfeb63bedf2259efab06cd41e99911ede7e9dfbad8af35a5783db4d4ad6677`。
- 发布包：`releases/1.4.190/FPGA-HDL-Static-Analyzer-1.4.190-Darwin-arm64.zip`，SHA-256 `62fdf6309086a8e811e01302a7dc5c253f6e1a1769dfd515ac22e27b486da142`。
- 解包后 CLI 返回 `FPGA HDL Static Analyzer 1.4.190`；同一行两个同名声明探针输出两条官方编号 `R-2-1`、Error、第 2 行第 1 列及正文“_same命名必须以字母开始”；应用短版本和构建版本均为 `1.4.190`，并通过 `codesign --verify --deep --strict`。

详细证据见 [第五十四轮逆向记录](native_rule_recovery_pass54.md) 与 [R-2-1 原生证据](evidence/native_verilog_first_character_5_2_1_findings.md)。
