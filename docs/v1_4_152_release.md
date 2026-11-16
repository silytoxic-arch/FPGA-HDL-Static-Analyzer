# FPGA HDL Static Analyzer V1.4.152

## 本轮目标

继续依据附件 `T-Linter.dll` 的原生静态反编译证据收敛 GJB9765 `A-9-3`（内部兼容键 `GJB9765.4.9.g`），不调整界面或表格。

## 原生证据与实现

- `FUN_180684a40` 通过 `FUN_1804c5890` 获取进程向量，再对该向量构造时钟关系并递归收集赋值。
- `FUN_1804c5890 @ 0x1804c5890` 是非递归两级收集器，只接受根的直接 architecture 和 architecture 的直接 process。
- 产品现仅从 architecture 直接时序进程建立时钟关系名称集合，也只从同一直接进程集合提取寄存器赋值目标。
- generate、block、子程序及其他并发节点下的嵌套 process 不参与交集任一侧。
- 相关证据记录：`docs/evidence/native_vhdl_direct_process_collector_4c5890.txt`、`docs/evidence/native_vhdl_ripple_clock_4_9_g_findings.md`。

## 回归边界

- 修正前：直接进程产生寄存器时钟、generate 内进程消费时，会错误产生 1 条 `A-9-3`；修正后为 0。
- 修正前：generate 内进程产生寄存器时钟、直接进程消费时，会错误产生 1 条 `A-9-3`；修正后为 0。
- 附件自反相分频正例、任意寄存器输出再作为直接后级时钟正例、普通寄存器反例继续执行。

## 验证与发布

- 完整回归：`135/135` 通过。
- 附件复审：544 条附件记录、537 个页面、209 条标准规则、446 个可探测样例；结果为 435 个 `Automated:matched`、6 个 `ProjectContext:matched`、4 个 `context-required`、1 个 `native-documentation-conflict`。
- 干净解压后的 CLI 版本为 `1.4.152`；嵌套消费侧和嵌套产生侧输入的 `A-9-3` 问题数均为 0，两个 architecture 直接进程构成的正例问题数为 1。
- 解压应用通过 `codesign --verify --deep --strict`，框架符号链接保留 15 个。

发布物：

- `releases/1.4.152/FPGA-HDL-Static-Analyzer-1.4.152-Darwin-arm64.zip`
  - SHA-256：`acc8757a81aa8c4e3947a0ab6815751b86c9493191c3ed70ec13d7c0d1f4376f`
- `docs/evidence/t_linter_after_1_4_152.json`
  - SHA-256：`e446c0ce119591f869df9b9ab2b924e9a5c6701856c34f438d9e27deb3d4d615`
- `docs/evidence/native_rule_implementation_ledger.json`
  - SHA-256：`8789b91bfe97131e436fdf57af46da68f9cb4448758191f75d4790277fb25742`
- `docs/evidence/native_vhdl_ripple_clock_4_9_g_findings.md`
  - SHA-256：`772daa103acf164f0572636777965613c2375f098b3c531e9d2d11ea868543d8`

## 边界

直接 architecture process 边界已经恢复。关系类别的专有名称及完整接纳范围、变量赋值、过程调用内间接赋值、configuration、组件/原语输出、别名与重载、错误恢复和诊断顺序仍属未知；无告警的未知路径继续保持人工复核，不宣称与原厂 209 条规则完整算法等价。
