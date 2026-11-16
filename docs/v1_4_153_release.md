# FPGA HDL Static Analyzer V1.4.153

## 本轮目标

继续依据附件 `T-Linter.dll` 的原生静态反编译证据收敛 GJB9765 `A-9-4`（内部兼容键 `GJB9765.4.9.h`），不调整界面或表格。

## 新恢复的原生边界

- 对 `FUN_18060c5c0 @ 0x18060c5c0` 进行完整定点反编译。
- 该函数构造类型 `0x52`（AST_ALWAYS/process）的过滤器，调用递归 `FUN_1804f3870` 从设计根收集进程。
- 它没有调用相邻规则使用的非递归 `FUN_1804c5890`。
- 因而 generate、block 等嵌套节点内的 process 时钟关系必须计入设计根的唯一时钟域名称集合。
- 完整证据保存在 `docs/evidence/native_vhdl_clock_domains_4_9_h_collector.txt`，恢复记录同步更新于 `docs/evidence/native_vhdl_clock_domains_4_9_h_findings.md`。

## 产品与回归

- 当前产品按设计单元使用解析器确认的全部进程时钟，方向与原生递归边界一致，无需错误套用直接进程过滤。
- 新增“一个直接进程使用 `clk1`、generate 内进程使用 `clk2`”正向边界，必须报告 1 条 `A-9-4`。
- 新增对应 block 内嵌进程正向边界，必须报告 1 条 `A-9-4`。
- 无告警仍保持 `ProjectContextRequired`，因为层次连接图分支尚未完整恢复。

## 验证与发布

- 完整回归：`135/135` 通过。
- 附件复审：544 条附件记录、537 个页面、209 条标准规则、446 个可探测样例；结果为 435 个 `Automated:matched`、6 个 `ProjectContext:matched`、4 个 `context-required`、1 个 `native-documentation-conflict`。
- 干净解压后的 CLI 版本为 `1.4.153`；generate 与 block 内嵌进程的双时钟域输入均报告 1 条 `A-9-4`。
- 解压应用通过 `codesign --verify --deep --strict`，框架符号链接保留 15 个。

发布物：

- `releases/1.4.153/FPGA-HDL-Static-Analyzer-1.4.153-Darwin-arm64.zip`
  - SHA-256：`7729c795459ff028381c544cfe2dae8a406224220fbd136d2730a3ed3556552d`
- `docs/evidence/t_linter_after_1_4_153.json`
  - SHA-256：`2bc574dc4be7a494b2d7ed4874536cf54d94aef378e4f435306752862339fa69`
- `docs/evidence/native_rule_implementation_ledger.json`
  - SHA-256：`3a69595de731d6f4022f8e8ec451e563bf5a8708cea79839b29cb9d2db6acf7c`
- `docs/evidence/native_vhdl_clock_domains_4_9_h_findings.md`
  - SHA-256：`a0a6057556634cb74f81039f856a4e428c771da1f9ee629af7ad147c4a9633b2`
- `docs/evidence/native_vhdl_clock_domains_4_9_h_collector.txt`
  - SHA-256：`2f01c5fe7f98824b9ff179cadf62dd1c3ecd82df3ab44b661018520cbef29c6a`

## 边界

递归进程收集边界已经恢复。关系类别的正式名称及完整接纳范围、层次连接图、组件和时钟原语、条件别名、configuration、错误恢复和专有诊断顺序仍属未知；未配置完整工程上下文时不把无告警伪判为合规，也不宣称与原厂 209 条规则完整算法等价。
