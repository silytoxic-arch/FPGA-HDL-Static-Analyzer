# FPGA HDL Static Analyzer V1.4.150

## 本轮目标

继续依据附件 `T-Linter.dll` 的原生静态反编译证据收敛 GJB9765 `R-9-2`（内部兼容键 `GJB9765.4.9.b`），不调整界面或表格。

## 原生证据与实现

- `FUN_180699180` 明确通过 `FUN_1804c5890` 获取进程集合。
- `FUN_1804c5890 @ 0x1804c5890` 是非递归两级收集器：只接受根的直接 `AST_ARCHITECTURE(0x75)` 子节点及 architecture 的直接 `AST_ALWAYS/process(0x52)` 子节点。
- 产品据此同时限制 `R-9-2` 的类别 1/2 时钟关系生产端和 IF/ELSIF/CASE 候选端；generate、block、子程序等嵌套范围内的 process 不参与这两条路径。
- 条件信号赋值的独立 `AST_ASSIGN_LE -> AST_WHEN` 递归路径没有被无证据地收窄。
- 原生证据：`docs/evidence/native_vhdl_direct_process_collector_4c5890.txt`；规则恢复记录同步更新于 `docs/evidence/native_vhdl_clock_mux_control_4_9_b_findings.md`。

## 验证

- 新增“generate 内进程建立的时钟关系不得外溢到直接进程”边界测试。
- 新增“直接进程建立时钟关系后，generate 内进程的控制候选不得报告”边界测试。
- 完整回归：`135/135` 通过。
- 附件复审：544 条附件记录、537 个页面、209 条标准规则、446 个可探测样例；结果为 435 个 `Automated:matched`、6 个 `ProjectContext:matched`、4 个 `context-required`、1 个 `native-documentation-conflict`。
- 干净解压后的 CLI 版本为 `1.4.150`；两组新增边界输入均未产生 `R-9-2` 问题，规则状态保持 `manual-review-required`。
- 解压应用通过 `codesign --verify --deep --strict`，框架符号链接保留。

## 发布物与校验

- `releases/1.4.150/FPGA-HDL-Static-Analyzer-1.4.150-Darwin-arm64.zip`
  - SHA-256：`b7223b5dd369d3a45f59dcf795d9406a2320f3890246269f9c7a81a4f695b4dc`
- `docs/evidence/t_linter_after_1_4_150.json`
  - SHA-256：`3aafa8eefc605d06abd89089840ba919205eeb1a527294d36c612131ec33831f`
- `docs/evidence/native_rule_implementation_ledger.json`
  - SHA-256：`d152c2ca6a9c131a60a457c95678a719855250c8fe48fa022854e4cf35b28185`
- `docs/evidence/native_vhdl_clock_mux_control_4_9_b_findings.md`
  - SHA-256：`2513411308485eee1fa7b78a6e78fe6c9042d953c9cba351a07aa446d612bf17`

## 边界

该版本恢复并实现的是有静态证据支持的子集。异常 frontend 降级、完整层次/组件传播、全部条件赋值 lowering、错误恢复与专有诊断顺序仍属未知；无告警的未知路径继续保持人工复核，不宣称与原厂 209 条规则完整算法等价。
