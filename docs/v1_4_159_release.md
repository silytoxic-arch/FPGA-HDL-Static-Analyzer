# FPGA HDL Static Analyzer V1.4.159

## 本轮目标

继续依据附件 `T-Linter.dll` 的原生静态反编译证据收敛 GJB9765 `A-9-6`（内部兼容键 `GJB9765.4.9.j`），闭合此前未知的两个单目 AST 类型并强化直接反相边界。本版不调整界面或表格。

## 新恢复的原生行为

- AST 名称分派表把 `0x24` 精确映射为 `AST_BIT_NOT`，把 `0x47` 精确映射为 `AST_LOGIC_NOT`；相邻 `0x45/0x46` 是 `AST_LOGIC_AND/AST_LOGIC_OR`。
- 宽度辅助函数 `FUN_1808f46c0` 对 `AST_BIT_NOT` 递归继承子节点宽度，对 `AST_LOGIC_NOT` 则与逻辑 AND/OR 一样把结果宽度固定为 1。
- `A-9-6` 的 `FUN_18060fc80` 接受上述两种 NOT 根，但共同要求恰有一个直接 `AST_IDENTIFIER(0x13)` 子节点。
- 该证据也修正了共享复位别名规则 `GJB10157 / R-5-3` 与 `A-5-2` 台账中“0x24/0x47 含义未知”的旧限制。

## 产品实现与回归

- VHDL 源码适配继续使用共同结构 `not <单个标识符>`，但代码注释和台账现明确对应两种原生枚举。
- 增加 `std_logic` 直接 NOT 时钟路径正例。
- 增加索引选择 `not bus(0)`、函数调用 `not pass(clk)`、双重 `not not clk` 和一元负号四类排除边界。
- 双重 NOT 的无诊断结果仍验证为 `ManualReviewRequired`，不把未支持结构伪判合规。
- VHDL 前端对预定义或用户自定义 `not` 重载的精确类型选择尚未恢复，继续列为未知。

## 审计与发布

- 完整回归：`135/135` 通过；其中新增 A-9-6 正反例和人工复核状态断言均通过。
- 附件复审：544 条附件记录、537 个页面、209 条标准规则、446 个可探测样例。
- 结果保持为 435 个 `Automated:matched`、6 个 `ProjectContext:matched`、4 个 `context-required`、1 个 `native-documentation-conflict`。
- 209 个公开编号全部通过官方 `R-X-X` / `A-X-X` 三段式校验。
- 台账 `fullAlgorithmsVerified` 继续为 `0`；不把尚未闭合的规则宣称为完全等价或合规。
- 从 ZIP 全新解压后验证：CLI 版本 `1.4.159`、`A-9-6` 官方三段编号与规则说明、应用包签名、`CFBundleShortVersionString=1.4.159` 及 15 个 Qt 框架符号链接均正常。

发布物：

- `releases/1.4.159/FPGA-HDL-Static-Analyzer-1.4.159-Darwin-arm64.zip`
  - SHA-256：`782ad6c2c41544729c06e34b7352dd861ecc2f10de96274421733c4d5ca33f3f`
- `docs/evidence/t_linter_after_1_4_159.json`
  - SHA-256：`f57c8d3a9f0cc0d275c145d4bc52f7f2825ba46825235dac7399cb35294e91b3`
- `docs/evidence/native_rule_implementation_ledger.json`
  - SHA-256：`5138d8b9ca73551f46a877d50bd94cd0fdcc4c0fcac91c7a4f157e7c482d618e`
- `docs/evidence/native_vhdl_clock_path_inverter_4_9_j_findings.md`
  - SHA-256：`298985ee0d45f20a9a81ba901ddbd39935b176f32e88b47b68abdc5cece37d0d`
- `docs/evidence/native_ast_not_types_0x24_0x47.txt`
  - SHA-256：`12a2f6b1597e5affaa96f195060becee903b99028e01fd26a1956b75af12fd20`

## 仍未闭合的边界

Configuration binding、generate/block 内组件关系、表达式或选择实参、歧义/外部实体绑定、厂商库变体、VHDL 前端重载选择、专有错误恢复、原生源位置选择及独立时钟间排序仍未知。这些构造的干净结果继续是 `manual-review-required`，不表示合规。
