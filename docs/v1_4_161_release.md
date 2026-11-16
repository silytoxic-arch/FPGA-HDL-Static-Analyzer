# FPGA HDL Static Analyzer V1.4.161

## 本轮目标

继续恢复 GJB10157 `R-5-3` 与 `A-5-2`（内部兼容键 `GJB10157.6.2.3 / 6.2.7`）共享原生函数 `FUN_180758410` 的别名传播深度。本版不调整界面或表格。

## 新恢复的原生行为

- 当前关系名对应连续赋值若为直接标识符或精确一级逻辑/位 NOT，则其源名称进入下一来源集合。
- 每轮结束后 `FUN_18075a210` 清空当前名称树。
- `FUN_180676aa0` 对访问树执行匹配项计数；计数为零的来源才由 `FUN_180579cf0` 插入下一轮。
- 主循环持续到当前工作集为空，因此原生行为是多级、去重且循环安全的反向别名遍历，不是单层豁免。
- 达到非别名连续表达式或组合过程赋值时进入诊断构造。

## 产品实现

- 先为同一模块建立连续赋值与无边沿组合过程驱动表。
- 从解析器确认的异步复位/置位名称开始，沿直接或单目反相别名反向遍历全部可达来源。
- 在每个可达的非别名组合驱动处分别生成 `R-5-3` 与 `A-5-2` 诊断，定位上游真实驱动目标。
- 使用名称访问集合终止直接/反相别名循环；无诊断结果仍保持 `ManualReviewRequired`。

## 新增回归

- `alias_rst2 <- ~alias_rst1 <- logic_rst <- A & B`：追溯并定位 `logic_rst`。
- 异步复位别名指向组合 `always @*` 过程目标：追溯后报告过程驱动。
- 两个直接/反相别名形成循环：安全终止、不伪造违规，并保持人工复核状态。
- V1.4.160 的直接标识符、`!`、`~`、选择、双重 NOT、调用和归约边界继续保留。

## 审计与发布

- 完整回归：`135/135` 通过；新增多级连续别名、组合过程上游及循环终止边界均通过。
- 附件复审：544 条附件记录、537 个页面、209 条标准规则、446 个可探测样例。
- 结果保持为 435 个 `Automated:matched`、6 个 `ProjectContext:matched`、4 个 `context-required`、1 个 `native-documentation-conflict`。
- 209 个公开编号全部通过官方 `R-X-X` / `A-X-X` 三段式校验。
- 台账 `fullAlgorithmsVerified` 继续为 `0`；未把同模块别名工作集等同于完整层次算法。
- 从 ZIP 全新解压后验证：CLI 版本 `1.4.161`，`R-5-3`、`A-5-2` 官方编号查询正常，应用包签名有效，`CFBundleShortVersionString=1.4.161`，15 个 Qt 框架符号链接完整。

发布物：

- `releases/1.4.161/FPGA-HDL-Static-Analyzer-1.4.161-Darwin-arm64.zip`
  - SHA-256：`b0f8b9247ae5a42d57de3204d745c3cf569c35928fe17667013ccc33860216df`
- `docs/evidence/t_linter_after_1_4_161.json`
  - SHA-256：`51105ce7f3f977110000ed17f68c84dbaa5bab36a037b06f7341fa3f76cd8e6c`
- `docs/evidence/native_rule_implementation_ledger.json`
  - SHA-256：`b4bc15d38e03bd9be21f274bd6851cb099cbcefe88aafff1c6262ea4e5ec86b2`
- `docs/evidence/native_verilog_combinational_reset_6_2_3_6_2_7_findings.md`
  - SHA-256：`43401930feab3c1d4ec97de1a46427a155e03b14d53597dc71d165f79a43c600`
- `docs/evidence/native_verilog_reset_alias_worklist_6_2_3_6_2_7.txt`
  - SHA-256：`fdb63acb38fc1a2d3cdfcb2348d56ca869720007cfd207d5e7ab39b04866f189`

## 仍未闭合的边界

异步复位关系图的完整生产过程、跨层次传播、原语/库单元复位端口、宏、generate、复合左值、分层标识符的专有前端降低方式和原生诊断顺序/次数仍未知。未报告这些构造不表示合规。
