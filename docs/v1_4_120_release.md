# V1.4.120 R-6-5 package 根与声明初始化路径

本版本只推进规则逆向、判断逻辑和验证，不调整界面或表格布局。对外规则编号继续严格采用官方三段式 `R-X-X` / `A-X-X`。

## 原生证据闭环

- `FUN_180603930` 对每个输入记录取首个 AST 根，清空内部类型/赋值容器后直接调用 `FUN_180604790(root)`，没有 architecture 类型门控。
- `FUN_180604790` 递归访问所有子节点，登记 `AST_SIGNAL(0x78)`、`AST_VARIABLE(0x79)`、类型 6、`AST_LOCALPARAM(0x0a)` 的 integer/real/unsigned/signed 类型标志。
- 同一递归器收集恰有两个子节点的 `AST_ASSIGN_LE(0x56)` 和 `AST_VARASGN(0xa2)`。
- VHDL constant 产生器 `FUN_1801b2770` 为带初始化声明创建 `AST_VARASGN`，证明声明初始化式与过程内普通 `:=` 都属于原生赋值集合。
- 因此独立 package specification、package body、其中的声明初始化和子程序赋值均应参与 `R-6-5`，不能只分析 architecture 中的 `<=`。

证据：`reverse-tools/evidence/r6_5_root_and_assignment_walkers.txt`、`docs/evidence/native_vhdl_implicit_numeric_conversion_4_6_d_findings.md`；附件 DLL SHA-256 为 `60e6b52875f4d7c3fbdf494d869b3a4bcc3fdc8ee63d8620ea9614da5d6599b0`。

## 产品修正

- 把 architecture 中 constant/signal/variable 的 `:=` 初始化式纳入隐式数值转换检查。
- 新增独立 package specification 和 package body 根，每个根单独建立直接可解析的类型表。
- package-body function/procedure 内的直接变量 `:=` 赋值进入同一十种原生有向类型组合判据。
- 完整外层 `integer(...)`、`real(...)`、`unsigned(...)`、`signed(...)` 转换继续保持不展开；未知 subtype、包限定类型和函数返回类型仍为 `ManualReviewRequired`。
- package 根扩展只应用于 `R-6-5`，不把该入口证据外推给相邻 `R-6-6` 的运算树范围。

## 新增验证

- architecture 内 `constant Cint : integer := U`，其中 `U` 为 unsigned：1 条 `R-6-5`。
- package specification 中同类 constant 初始化：1 条。
- package body 子程序中 `i := u`：1 条。
- package 初始化式使用完整 `integer(U)` 显式转换：0 条，规则保持人工复核状态。

## 仍未知

显式转换的专有 AST 降级、用户 subtype、包限定类型、函数返回类型、重载、数组元素/记录字段、跨文件与层级绑定、别名、同一源码文件内交错设计单元的完整根边界、错误恢复及原生诊断重数仍未完全恢复。产品不以无报告宣称合规，完整厂商算法等价仍为 0/209。

## 发布验证

- 完整测试：`135/135` 通过（含本轮 4 个 `R-6-5` package/初始化正反例与边界用例）。
- 附件审计：544 条厂商记录、537 个规则页、209 条标准规则、446 个示例；435 个 `Automated:matched`、6 个 `ProjectContext:matched`、4 个 `context-required`、1 个 `native-documentation-conflict`。
- 实施台账：209 条核心判据均已接入；完整厂商算法等价验证仍为 `0/209`，未作超证据声明。
- 公开编号：209/209 条规则记录均匹配官方三段式 `^[RA]-[0-9]+-[0-9]+$`。
- 从发布 ZIP 全新解压后，CLI 版本确认为 `1.4.120`，应用包通过 macOS 深度严格签名校验。
- 全新解压样例：package constant 隐式转换为 1 条 `R-6-5`，package-body 子程序赋值为 1 条，完整显式转换为 0 条且状态保持 `manual-review-required`；三组均为 0 个分析错误，全部输出编号均为三段式。
- 发布包：`releases/1.4.120/FPGA-HDL-Static-Analyzer-1.4.120-Darwin-arm64.zip`
- SHA-256：`bd6353de579d562433a0f04d4d8a2ec4c6adf77d0265a60f2f557f4c42fb74fa`
