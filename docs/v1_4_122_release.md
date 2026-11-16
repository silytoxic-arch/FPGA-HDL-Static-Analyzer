# V1.4.122 R-6-6 package 根与状态隔离

本版本只推进规则逆向、判断逻辑和验证，不调整界面或表格布局。对外规则编号继续严格采用官方三段式 `R-X-X` / `A-X-X`。

## 原生证据闭环

- `FUN_180605b20` 遍历规则对象 `+0x78` 的每个输入记录，从记录 `+8` 的指针向量取得首个 AST 根。
- 每轮在调用递归构建器前清空候选运算节点、signed 名称和 unsigned 名称容器。
- 入口直接调用 `FUN_1806067a0(root)`，调用前没有 `AST_ARCHITECTURE` 类型判断。
- 因此原生状态生命周期是每个输入 AST 根独立；package specification 和 package body 不应因产品的 architecture-only 适配而被排除。
- 已恢复的运算节点集合保持不变：位/逻辑、六类关系和 `+`、`-`、`*`、`/`；不扩展到 `mod`、`rem`、移位、连接或幂。

证据：`reverse-tools/evidence/r6_6_root_lifecycle.txt`（SHA-256 `630e82c68fbfc85495b7577261b846142ff90226b855ec591dde6436362011e0`）、`docs/evidence/native_vhdl_mixed_signedness_4_6_e_findings.md`；附件 DLL SHA-256 为 `60e6b52875f4d7c3fbdf494d869b3a4bcc3fdc8ee63d8620ea9614da5d6599b0`。

## 产品修正

- 将 `R-6-6` 运算树扫描器改为显式接收单个 AST 根对应的 token 范围和独立声明表。
- architecture 继续绑定对应 entity 端口，再扫描本根声明和表达式。
- 独立 package specification/package body 现在建立自己的 signed/unsigned 名称表并执行相同运算树判断。
- package body function/procedure 中的赋值和控制表达式随所属 package 根递归检查。
- 相邻 package、entity 和 architecture 根之间不共享声明表，避免同名对象类型泄漏。
- 不改变未知 subtype、包限定类型、函数返回类型、重载和跨文件绑定的人工复核状态。

## 新增验证

- package specification 中 `signed + unsigned` 的 constant 初始化：1 条 `R-6-6`。
- package body procedure 中 `signed and unsigned`：1 条。
- package 中同名对象为 signed、后续 architecture 中同名对象及其运算双方均为 unsigned：0 条，证明根隔离有效。

## 仍未知

专有 AST 运算符名称的完整映射、用户 subtype、包限定与函数返回类型、重载解析、显式转换降低、selected/indexed/record 名称、跨文件与层级绑定、错误恢复及原生诊断顺序仍未完全恢复。无报告不宣称完整厂商算法等价，完整算法等价仍为 0/209。

## 发布验证

- 完整测试：`135/135` 通过（含 3 个新增 `R-6-6` package/根隔离用例）。
- 附件审计：544 条厂商记录、537 个规则页、209 条标准规则、446 个示例；435 个 `Automated:matched`、6 个 `ProjectContext:matched`、4 个 `context-required`、1 个 `native-documentation-conflict`。
- 实施台账：209 条核心判据均有接入记录；完整厂商算法等价验证仍为 `0/209`，未作超证据声明。
- 公开编号：209/209 条规则记录均匹配官方三段式 `^[RA]-[0-9]+-[0-9]+$`。
- 从发布 ZIP 全新解压后，CLI 版本确认为 `1.4.122`，应用包通过 macOS 深度严格签名校验。
- 全新解压样例：package constant 的 `signed + unsigned` 为 1 条 `R-6-6`；package-body 子程序的 `signed and unsigned` 为 1 条；package/architecture 同名对象隔离样例为 0 条。三组均为 0 个分析错误，全部输出编号均为官方三段式。
- 发布包：`releases/1.4.122/FPGA-HDL-Static-Analyzer-1.4.122-Darwin-arm64.zip`
- SHA-256：`6fe51944ffffd69d8d3fd483f1579107ee23ec1f8bd9b87417a2404bf14899d0`
