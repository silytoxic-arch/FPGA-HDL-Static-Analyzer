# FPGA HDL Static Analyzer V1.4.143

本版只推进规则判断逻辑，不调整界面或表格。

## R-9-2 恢复结果

- 从附件 `T-Linter.dll` 恢复 `FUN_1804c0de0` 与 `FUN_18069a710`。
- 前者把候选 AST 指针和规范化名称构造成 `0x28` 字节记录；后者逐条追加记录，不比较名称、AST 指针或源码行。
- 因此原生去重仅发生在单个 IF/CASE 条件的名称树中，不跨不同条件去重。
- 条件赋值的每个直接 `AST_COND` 子节点也独立保留候选。
- 产品继续在单个条件内部去重，但会保留不同 IF/CASE 条件以及不同 WHEN 条件中的同名时钟诊断。

## 验证

- 新增“同一时钟出现在两个独立 IF 条件”边界测试，预期两条 `R-9-2`。
- 新增“同一时钟出现在条件赋值两个 WHEN 条件”边界测试，预期两条 `R-9-2`。
- 既有“同一条件内重复时钟名”测试仍预期一条。
- 所有公开编号继续限制为官方三段式 `R-X-X` / `A-X-X`。
- 完整测试为 `135/135` 通过。
- 附件审计保持 544 条附件记录、537 个页面、209 条标准规则和 446 个示例；结果为 435 个 `Automated:matched`、6 个 `ProjectContext:matched`、4 个 `context-required`、1 个 `native-documentation-conflict`，无回退。
- 全新目录解压验证：CLI 版本为 `1.4.143`，macOS 深度严格签名通过；验证样例准确输出 4 条 `R-9-2`（两个独立 IF 加两个 WHEN 条件），无分析错误。
- 解压报告的全部违规与评估编号均满足 `R-X-X` / `A-X-X`，未外露内部 `GJB9765.x.x` 条款编号。

## 证据与边界

- 原生候选追加证据：`docs/evidence/native_vhdl_clock_mux_control_4_9_b_candidate_append.txt`
- 规则发现记录：`docs/evidence/native_vhdl_clock_mux_control_4_9_b_findings.md`
- 实现台账：`docs/evidence/native_rule_implementation_ledger.json`
- 台账 SHA-256：`353c8aa3c3df8271a2d2a6601d612aa0d2c204f23cf87ab48808cef1bae0793f`

组件 MUX、层次传播、generate/configuration、包限定名、异常前端降级及畸形恢复仍未闭合；这些路径不伪判为合规，干净结果继续保持人工复核状态。完整原生等价仍为 `0/209`。

## 发行包

- 文件：`releases/1.4.143/FPGA-HDL-Static-Analyzer-1.4.143-Darwin-arm64.zip`
- SHA-256：`849f341270813e6908cb01733b4acd184c269e0427183d3d5f159b0369711040`
