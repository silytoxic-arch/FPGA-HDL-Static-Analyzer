# V1.4.128 R-9-3 门控时钟候选根类型

本版本只推进规则逆向、判断逻辑和验证，不调整界面或表格布局。对外规则编号继续严格采用官方三段式 `R-X-X` / `A-X-X`。

## 新恢复的原生判断

- 从原始 DLL 重新建立临时 Ghidra 12.1.4 数据库，定点反编译 `FUN_1806841f0`。
- 收集器只检查 `AST_ARCHITECTURE(0x75)` 的直接 `AST_ASSIGN_LE(0x56)` 子节点，赋值左侧必须是 `AST_IDENTIFIER(0x13)`。
- 广义依赖关系表接受较宽的 RHS 根集合以及 `AST_WHEN(0xb6)`，但真正的门控候选表使用精确条件 `((rhsType - 0x25) & 0xffffffde) == 0`。
- 该条件只允许 `0x25/0x26/0x45/0x46`，即位/逻辑 `AND` 与 `OR`。`XOR/XNOR/NAND/NOR/WHEN` 和函数调用根不属于直接门控候选。

证据：原生入口 `ev_29c219f05a7448393aa834253a168ef537297d0f9fee1930afd84c280ce8d6c4`；聚焦恢复记录 `docs/evidence/native_vhdl_gated_clock_4_9_c_collector_excerpt.md`；本轮函数导出 SHA-256 `96af6f8dcc87b1a52d99a13a7613b016e03c71b89af922cd8a3ba3f479beb6a2`；附件 DLL SHA-256 `60e6b52875f4d7c3fbdf494d869b3a4bcc3fdc8ee63d8620ea9614da5d6599b0`。

## 产品修正

- `R-9-3` 只在去除完整外层括号后的 RHS 根层识别 `and/or`。
- 不再把根 `xor/xnor/nand/nor`、条件 `when` 或函数调用参数中的 `and/or` 当作门控候选。
- 广义时钟依赖追踪和相邻 `A-9-1` 组合时钟检查保持独立，不把候选表与关系表混为一谈。

## 新增验证

- 根 `OR` 派生时钟：1 条 `R-9-3`。
- 根 `XOR` 派生时钟：0 条 `R-9-3`。
- `wrap(clk and en)`：0 条 `R-9-3`。
- `clk when en='1' else '0'`：0 条 `R-9-3`。
- 附件根 `AND` 正例继续报告。

## 仍未知

component/厂商原语门控、跨层级关系、generate/configuration、完整重载与错误恢复、重复设计根和原生诊断重数仍未完全恢复。无报告不宣称完整厂商算法等价，完整算法等价仍为 0/209。

## 发布验证

- 完整测试：`135/135` 通过，包含本轮四个候选根类型边界。
- 附件审计：544 条厂商记录、537 个规则页、209 条标准规则、446 个示例；435 个 `Automated:matched`、6 个 `ProjectContext:matched`、4 个 `context-required`、1 个 `native-documentation-conflict`，相对 V1.4.127 无回退。
- 实施台账：209 条核心判据均有接入记录；`R-9-3` 已记录关系表与候选表、精确类型条件和限制；完整厂商算法等价验证仍为 `0/209`。
- 公开编号：209/209 条规则记录均匹配官方三段式 `^[RA]-[0-9]+-[0-9]+$`。
- 本地 CLI：根 OR、根 XOR、嵌套 AND、WHEN 的 `R-9-3` 数量分别为 1、0、0、0；相邻 `A-9-1` 分别为 1、1、0、1，证明两条规则保持独立；四组均为 0 个分析错误。
- 从发布 ZIP 全新解压后，CLI 版本确认为 `1.4.128`，应用包通过 macOS 深度严格签名校验。
- 全新解压样例：根 OR、根 XOR、嵌套 AND、WHEN 的 `R-9-3` 数量为 1、0、0、0，相邻 `A-9-1` 为 1、1、0、1；四组均为 0 个分析错误且全部公开编号为官方三段式。
- 发布包：`releases/1.4.128/FPGA-HDL-Static-Analyzer-1.4.128-Darwin-arm64.zip`
- SHA-256：`cdab7f35a9928f4240f0545348dce4bc71b2ba72d46b946d23747c0351ce47fa`
