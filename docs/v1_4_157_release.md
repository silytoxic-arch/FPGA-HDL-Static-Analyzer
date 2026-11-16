# FPGA HDL Static Analyzer V1.4.157

## 本轮目标

继续依据附件 `T-Linter.dll` 的原生静态反编译证据收敛 GJB9765 `A-9-6`（内部兼容键 `GJB9765.4.9.j`），恢复同一时钟关系内反相器/缓冲器结果的位图合并和诊断重数。本版不调整界面或表格。

## 新恢复的原生行为

- `FUN_18060f280` 命中组件缓冲路径时返回位 2，`FUN_18060fc80` 命中反相路径时返回位 1。
- `FUN_18060d980` 以直接反相结果初始化当前时钟的位图，遍历全部 component 候选并把 `FUN_180610ad0` 的结果逐次按位或入同一变量。
- 一个时钟关系处理完后只执行一次诊断：值 1 对应反相器，值 2 对应缓冲器，值 3 对应两者同时存在。
- 因此同一时钟路径上的多个同类命中只报告一次；两个独立时钟关系仍分别报告。
- 证据保存在 `docs/evidence/native_vhdl_clock_path_inverter_4_9_j_main.txt`，结论与边界保存在 `docs/evidence/native_vhdl_clock_path_inverter_4_9_j_findings.md`。

## 产品与回归

- 产品按时钟根保存直接依赖反向遍历的可达信号集，普通别名本身不违规，但可把上游组件输出纳入该时钟路径。
- 直接反相与已解析用户组件的一层缓冲/反相结果按原生位值合并，每个时钟最多产生一条 `A-9-6` 诊断。
- 新增三类边界回归：同一路径两个反相器只报告一次；组件缓冲加直接反相只报告一条“两者同时存在”；两个独立反相时钟分别报告两条。
- 多层组件链、配置绑定、复杂实参和外部库仍保持人工复核，不因无告警伪判合规。
- 对外规则编号继续严格使用官方三段式 `R-X-X` / `A-X-X`。

## 验证与发布

- 完整回归：`135/135` 通过。
- 附件复审：544 条附件记录、537 个页面、209 条标准规则、446 个可探测样例；结果为 435 个 `Automated:matched`、6 个 `ProjectContext:matched`、4 个 `context-required`、1 个 `native-documentation-conflict`。
- 209 个公开编号全部通过三段式校验。
- 干净解压后的 CLI 版本为 `1.4.157`。
- 解压应用通过 `codesign --verify --deep --strict`，框架符号链接保留 15 个。

发布物：

- `releases/1.4.157/FPGA-HDL-Static-Analyzer-1.4.157-Darwin-arm64.zip`
  - SHA-256：`5a7437485a1fb48b1240f80f2f371208c1ff0e43a770155f15711ca724b8b1ea`
- `docs/evidence/t_linter_after_1_4_157.json`
  - SHA-256：`492bb1802432b1b3fcc9d60bf30d8fc2d11d3faa1af3716622e2a16f8c88beb3`
- `docs/evidence/native_rule_implementation_ledger.json`
  - SHA-256：`e750d794ac88e948e685f085fb58af4c9320c7582d4703893b567875a321b488`
- `docs/evidence/native_vhdl_clock_path_inverter_4_9_j_findings.md`
  - SHA-256：`30323477c88b2756cc51de38cf70b3587b815d0ee9f09000a8039e5212e2e440`

## 边界

当前已恢复每时钟的位图聚合和三种诊断分支，但原生源位置选择、不同独立时钟间的排序、多层递归组件链、configuration binding、复杂端口表达式、厂商库变体及专有前端错误恢复仍未闭合。台账的 `fullAlgorithmsVerified` 继续为 `0`，不宣称与原厂 209 条规则完整算法等价。
