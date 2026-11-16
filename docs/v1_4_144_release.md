# FPGA HDL Static Analyzer V1.4.144

本版只推进规则判断逻辑，不调整界面或表格。

## R-9-3 原生边界恢复

- `FUN_1806841f0` 只从 `AST_ARCHITECTURE` 的直接 `AST_ASSIGN_LE` 子节点建立关系和门控候选。
- 进程、子程序、block 和 generate 内的赋值不进入本规则的两张原生表。
- 关系传播根严格为直接 NOT、AND、OR、XOR、XNOR、NAND、NOR、二元 ADD/SUB/MUL/DIV 与 WHEN。
- 裸标识符别名、函数调用、移位和一元正负号不传递 `R-9-3` 关系。
- 只有直接 AND/OR 根写入门控候选表；其他允许根只能传播关系。
- 两张表均按规范化目标名索引，后续同名合格赋值覆盖先前 AST 位置，因此产品只保留最后位置。

## 实现与测试

- 分离 `R-9-3` 的原生关系遍历与相邻时钟规则使用的较宽依赖遍历，避免互相借用判据。
- 新增 XOR、WHEN 传播正例。
- 新增裸别名、函数调用、进程赋值和 generate 赋值排除例。
- 新增同名多次门控赋值只报告最后候选位置的诊断重数测试。
- 既有附件 AND、根 OR、根 XOR 排除、调用内嵌 AND 排除及交叉规则测试继续保留。
- 完整测试 `135/135` 通过。
- 附件审计保持 544 条附件记录、537 个页面、209 条标准规则和 446 个示例；435 个 `Automated:matched`、6 个 `ProjectContext:matched`、4 个 `context-required`、1 个 `native-documentation-conflict`，相对 V1.4.143 无回退。
- 全新目录解压验证：CLI 版本为 `1.4.144`，macOS 深度严格签名通过；直接并发 AND 产生 1 条 `R-9-3`，裸别名后的 AND 和进程内 AND 均未误报，无分析错误。
- 解压报告的违规与评估编号全部满足官方 `R-X-X` / `A-X-X`，未外露内部条款编号。

## 证据与限制

- 原生边界证据：`docs/evidence/native_vhdl_gated_clock_4_9_c_relation_boundary.txt`
- 发现记录：`docs/evidence/native_vhdl_gated_clock_4_9_c_findings.md`
- 实现台账：`docs/evidence/native_rule_implementation_ledger.json`
- 台账 SHA-256：`e4446a0853894de4488da884c9b9852378f17217f75d28da476e4876513fdc58`

元件/原语门控、层次与 configuration 展开、selected/extended name、完整重载解析、畸形前端树和重复设计根的精确顺序仍未闭合；未知路径继续人工复核，完整厂商算法等价仍为 `0/209`。

## 发行包

- 文件：`releases/1.4.144/FPGA-HDL-Static-Analyzer-1.4.144-Darwin-arm64.zip`
- SHA-256：`5daa600f19aef4bb9afe753207ee117a3f7ff3897c94283e9116f5559ad9a280`
