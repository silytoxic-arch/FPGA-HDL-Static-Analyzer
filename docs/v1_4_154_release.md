# FPGA HDL Static Analyzer V1.4.154

## 本轮目标

继续依据附件 `T-Linter.dll` 的原生静态反编译证据收敛 GJB9765 `A-9-5`（内部兼容键 `GJB9765.4.9.i`），不调整界面或表格。

## 新恢复的原生边界

- 对 `FUN_180694590 @ 0x180694590` 进行完整定点反编译。
- 该函数先调用非递归 `FUN_1804c5890` 收集进程，仅 architecture 的直接 process 子节点建立类别 1/2 时钟关系。
- generate、block、子程序等节点内嵌的 process 不进入本规则的时钟沿映射。
- 后续赋值检查具有不同边界：从设计根递归收集 `AST_ASSIGN_LE (0x56)`，再递归收集一元 `AST_NOT (0x24)` 下的 `AST_IDENTIFIER (0x13)`。
- 因此原生语义是“直接进程时钟关系 + 递归赋值扫描”，不能按相邻规则推断成全部进程时钟。
- 完整反编译证据保存在 `docs/evidence/native_vhdl_mixed_clock_edges_4_9_i_main.txt`，恢复记录同步更新于 `docs/evidence/native_vhdl_mixed_clock_edges_4_9_i_findings.md`。

## 产品修正与回归

- `A-9-5` 的上升沿/下降沿映射现在只接纳 architecture 直接 process 中解析器确认的时钟事件。
- 简单 `target <= not source;` 赋值仍按设计根范围扫描，但只有赋值两端都存在于直接进程时钟关系映射时才报告。
- 修正了一个直接 rising-edge 进程与 generate/block 内嵌 falling-edge 进程被误报为混合边沿的问题。
- 新增 generate 与 block 两个排除边界；直接进程之间的真实混合边沿正例仍报告 1 条 `A-9-5`。
- 对外规则编号继续严格使用官方三段式 `R-X-X` / `A-X-X`，内部兼容键不出现在用户报告中。

## 验证与发布

- 完整回归：`135/135` 通过。
- 附件复审：544 条附件记录、537 个页面、209 条标准规则、446 个可探测样例；结果为 435 个 `Automated:matched`、6 个 `ProjectContext:matched`、4 个 `context-required`、1 个 `native-documentation-conflict`。
- 干净解压后的 CLI 版本为 `1.4.154`。
- 干净包验证：generate 内嵌相反沿不产生 `A-9-5`；两个直接进程的相反沿产生 1 条公开编号 `A-9-5`。
- 解压应用通过 `codesign --verify --deep --strict`，框架符号链接保留 15 个。

发布物：

- `releases/1.4.154/FPGA-HDL-Static-Analyzer-1.4.154-Darwin-arm64.zip`
  - SHA-256：`6b92e067fda3bea8ee87484e083f88790f0131f8c47bafbc5d7b5c3deddc2934`
- `docs/evidence/t_linter_after_1_4_154.json`
  - SHA-256：`93cb57183b06d9ea9c88b6307ba7236ae9cfe11bc99e0558d595854411223858`
- `docs/evidence/native_rule_implementation_ledger.json`
  - SHA-256：`c4cd80f53b47a4f6ed5c4f2f5292ca24a9c09c6e9b8e73594af1bf4dc9f101a4`
- `docs/evidence/native_vhdl_mixed_clock_edges_4_9_i_findings.md`
  - SHA-256：`498371017c91efa881319fbe0535926d1fa769ea71e7a52823a334ea07b5f5b7`
- `docs/evidence/native_vhdl_mixed_clock_edges_4_9_i_main.txt`
  - SHA-256：`58c22aebaa89c2512f0b47766dbed899e3321cd3b47d47ebdd7fa7ec9c668562`

## 边界

直接进程关系边界与递归赋值扫描已经恢复。关系类别的正式名称及完整归一化规则、多级反相链、组件/原语反相、configuration、错误恢复和专有诊断顺序仍属未知；未配置完整工程上下文时不把无告警伪判为合规，也不宣称与原厂 209 条规则完整算法等价。
