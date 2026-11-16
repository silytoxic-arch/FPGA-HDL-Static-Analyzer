# FPGA HDL Static Analyzer V1.4.151

## 本轮目标

继续依据附件 `T-Linter.dll` 的原生静态反编译证据收敛 GJB9765 `R-9-4`（内部兼容键 `GJB9765.4.9.d`），不调整界面或表格。

## 原生证据与实现

- `FUN_18065a0c0` 通过 `FUN_1804c5890` 获取待分析进程。
- `FUN_1804c5890 @ 0x1804c5890` 的完整定点反编译证明它是非递归两级收集器：只接受根的直接 `AST_ARCHITECTURE(0x75)` 子节点及 architecture 的直接 `AST_ALWAYS/process(0x52)` 子节点。
- 产品据此把 `R-9-4` 的逐进程时钟关系比较限定到 architecture 直接 process；generate、block、子程序及其他并发节点下的嵌套 process 不参与。
- 原生证据：`docs/evidence/native_vhdl_direct_process_collector_4c5890.txt`；规则恢复记录同步更新于 `docs/evidence/native_vhdl_single_clock_process_4_9_d_findings.md`。

## 回归边界

- 修正前，generate 内含两个时钟关系的 process 会错误产生 `R-9-4`；修正后问题数为 0。
- 新增 generate 内嵌 process 排除测试。
- 新增 block 内嵌 process 排除测试。
- architecture 直接 process 的多时钟、同名相反沿、类别 1/2 等既有正反例继续执行。

## 验证与发布

- 完整回归：`135/135` 通过。
- 附件复审：544 条附件记录、537 个页面、209 条标准规则、446 个可探测样例；结果为 435 个 `Automated:matched`、6 个 `ProjectContext:matched`、4 个 `context-required`、1 个 `native-documentation-conflict`。
- 干净解压后的 CLI 版本为 `1.4.151`；generate 与 block 两个嵌套 process 输入的 `R-9-4` 问题数均为 0。
- 解压应用通过 `codesign --verify --deep --strict`，框架符号链接保留 15 个。

发布物：

- `releases/1.4.151/FPGA-HDL-Static-Analyzer-1.4.151-Darwin-arm64.zip`
  - SHA-256：`9de49dde249409abca3e324f5fca640a39180ed03c24d66f4ef7da1724cb0c7f`
- `docs/evidence/t_linter_after_1_4_151.json`
  - SHA-256：`067a82ed068e8c1ee18bea50a2280cca02a0b9c48be27804e3c3e5d46c3e0042`
- `docs/evidence/native_rule_implementation_ledger.json`
  - SHA-256：`8be41db019038143d63178fac04982ff1700162494f2db92aefbbffaa8ef5d7b`
- `docs/evidence/native_vhdl_single_clock_process_4_9_d_findings.md`
  - SHA-256：`c857be25bb0822d8bad78a6b936b23de8d0fa23f2ef1a431b25cc41cb0e5baca`

## 边界

该版本恢复并实现的是有静态证据支持的子集。别名、包限定或扩展名称、异常 frontend lowering、错误恢复、专有诊断顺序及格式仍属未知；无告警的未知路径继续保持人工复核，不宣称与原厂 209 条规则完整算法等价。
