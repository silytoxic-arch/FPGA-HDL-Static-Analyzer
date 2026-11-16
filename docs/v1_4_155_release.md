# FPGA HDL Static Analyzer V1.4.155

## 本轮目标

继续依据附件 `T-Linter.dll` 的原生静态反编译证据收敛 GJB9765 `A-9-6`（内部兼容键 `GJB9765.4.9.j`），不调整界面或表格。本版同时包含 V1.4.154 对 `A-9-5` 的直接进程边界修正。

## 新恢复的原生边界

- 对 `FUN_18060d980 @ 0x18060d980` 及其收集器 `FUN_1806124a0 @ 0x1806124a0` 进行定点反编译。
- 主函数构造类型 `0x52`（`AST_ALWAYS/process`）过滤器，由 `FUN_1806124a0` 从设计根递归收集匹配节点。
- 收集器在访问当前节点后递归遍历所有子节点，仅对类型 `0x7b`（component declaration）停止下探。
- generate、block 内嵌 process 因而必须进入 `A-9-6` 的时钟路径关系集合；这与 `A-9-5` 使用非递归 `FUN_1804c5890` 的边界不同。
- 完整证据保存在 `docs/evidence/native_vhdl_clock_path_inverter_4_9_j_main.txt` 和 `docs/evidence/native_vhdl_clock_path_inverter_4_9_j_collector.txt`。

## 产品与回归

- 产品原有的全部解析器时钟集合与原生递归进程边界一致，无需错误套用直接进程过滤。
- 新增 generate 内嵌进程使用反相时钟正例，必须报告 1 条 `A-9-6`。
- 新增 block 内嵌进程使用反相时钟正例，必须报告 1 条 `A-9-6`。
- 普通别名、非时钟数据逻辑和未被时钟使用的反相表达式继续不误报。
- 对外规则编号继续严格使用官方三段式 `R-X-X` / `A-X-X`。

## 验证与发布

- 完整回归：`135/135` 通过。
- 附件复审：544 条附件记录、537 个页面、209 条标准规则、446 个可探测样例；结果为 435 个 `Automated:matched`、6 个 `ProjectContext:matched`、4 个 `context-required`、1 个 `native-documentation-conflict`。
- 干净解压后的 CLI 版本为 `1.4.155`。
- 干净包的 generate 内嵌进程反相时钟输入报告 1 条公开编号 `A-9-6`。
- 解压应用通过 `codesign --verify --deep --strict`，框架符号链接保留 15 个。

发布物：

- `releases/1.4.155/FPGA-HDL-Static-Analyzer-1.4.155-Darwin-arm64.zip`
  - SHA-256：`b1bb2aa44b9971c4976e2b888c1c367ecb3e6a42ca464056c6c8efcea4df60e0`
- `docs/evidence/t_linter_after_1_4_155.json`
  - SHA-256：`4a5af4bc8072639f6727cecb9e98c1795f6ca60e48480ea5dcb314f5a801ef30`
- `docs/evidence/native_rule_implementation_ledger.json`
  - SHA-256：`0d23f517df016441150b65efc3858dd0444146d05c66fb380b527563aa137395`
- `docs/evidence/native_vhdl_clock_path_inverter_4_9_j_findings.md`
  - SHA-256：`4c9a726016cb1af3f149dc9a32dbab75fc219e389db608d0128949460ec73546`
- `docs/evidence/native_vhdl_clock_path_inverter_4_9_j_main.txt`
  - SHA-256：`e95eca76de5a51c0ad1ce9f08ea87c546f0f9438b79e1acc7cac85136a5ab8f8`
- `docs/evidence/native_vhdl_clock_path_inverter_4_9_j_collector.txt`
  - SHA-256：`afd04b702dc48ad249e0d98e919b47994a12e591e7bb9952fe756e7c3a5d2620`

## 边界

递归进程收集和 component declaration 剪枝已经恢复。component 缓冲器、厂商时钟原语、完整层次/配置传播、两个原生单目 AST 类型的精确区别、错误恢复和专有诊断次数仍属未知；未覆盖构造的无告警结果继续保持人工复核，不宣称与原厂 209 条规则完整算法等价。
