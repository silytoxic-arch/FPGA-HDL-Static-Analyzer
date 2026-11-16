# V1.4.135 R-9-4 静态异步动作判定

本版本只推进规则逆向、判断逻辑和验证，不调整界面或表格布局。对外规则编号继续严格采用官方三段式 `R-X-X` / `A-X-X`。

## 新恢复并接入的原生判断

- `FUN_1804d20e0` 递归进入 `AST_BLOCK(0x54)`，只检查恰有两个子节点的 `AST_ASSIGN_LE(0x56)` / `AST_VARASGN(0xa2)`。
- RHS 为直接 `AST_CONSTANT(0x10)` 时，异步动作可证明为静态。
- RHS 为 `AST_CONCAT(0x22)` 时，首个直接子节点是 `AST_OTHERS(0x89)`，或全部直接子节点均为 `AST_CONSTANT(0x10)`，均可证明为静态。
- RHS 为单一 `AST_IDENTIFIER(0x13)` 时，必须已绑定且名称存在于每个设计根重新建立的大小写折叠命名值树。
- `FUN_1804cbad0` 证明该树只接纳所属实体泛型、当前 architecture 的直接常量与直接类型枚举文字；普通信号、process 局部常量、局部 package 常量及其他 architecture 声明不在集合中。
- 产品按这一边界扩展 `R-9-4` 类别 1 的接纳范围，并继续把不满足条件的形态留在原生类别 4，不参与本规则多时钟比较。

附件 DLL SHA-256：`60e6b52875f4d7c3fbdf494d869b3a4bcc3fdc8ee63d8620ea9614da5d6599b0`。

持久化证据：`docs/evidence/native_vhdl_single_clock_process_4_9_d_constant_helpers.txt`（SHA-256 `59b2822f588faa3947df178b0f90489b2bde7f75f6c8b1268f6731bb86ea0e06`）、`native_vhdl_single_clock_process_4_9_d_constant_set_builder.txt`（`9db66f4cca7e21edf1972317ebed35bed37f560b02be4bc4d60ad18b700dcef3`）、`native_vhdl_single_clock_process_4_9_d_constant_set_references.txt`（`23efc2497c6ecfc0db97314ba077c22eb1f8e84fbfe25e2b7aa72fa97cade4b7`）。

## 新增验证

- 所属实体泛型作为异步动作值：接纳为类别 1，并报告后续第二个时钟。
- 当前 architecture 直接常量：接纳。
- 当前 architecture 直接类型的枚举文字：接纳。
- 全字面量拼接：接纳。
- 首项 `others` 聚合：按原生短路分支接纳。
- process 局部常量：排除，保持类别 4。
- 局部 package 内常量：排除，保持类别 4。
- 普通信号动作、赋值 RHS 边沿调用、独立进程、同名同沿与同名异沿既有边界继续回归。

## 仍未知

类别 2 的全部 linked-source-IF 内部记录、复合 event 变体、别名传播、generate 进程、包限定名称、错误恢复树、精确遍历顺序和原生诊断格式仍未完全恢复。产品只对有证据的有界源码形态作肯定报告；其他语法有效的无告警结果保持 `ManualReviewRequired`，完整厂商算法等价仍为 `0/209`。

## 发布验证

- 完整测试：`135/135` 通过。
- 附件审计：544 条厂商记录、537 个规则页、209 条标准规则、446 个示例；435 个 `Automated:matched`、6 个 `ProjectContext:matched`、4 个 `context-required`、1 个 `native-documentation-conflict`，相对 V1.4.134 无回退。
- 实施台账：209 条核心判据均有接入记录；`R-9-4` 已补充静态动作辅助函数、命名值树来源与作用域排除；完整厂商算法等价验证仍为 `0/209`。
- 公开编号：209/209 条规则记录均匹配官方三段式 `^[RA]-[0-9]+-[0-9]+$`。
- 从发布 ZIP 全新解压后，CLI 版本确认为 `1.4.135`，应用包通过 macOS 深度严格签名校验。
- 全新解压样例：附件违反例输出 2 条 `R-9-4`，遵循例为 0 条并保持 `manual-review-required`；所属实体泛型、architecture 常量、枚举文字、全字面量拼接和首项 `others` 聚合各输出 1 条；process 局部常量与局部 package 常量均为 0 条并保持人工复核。全部样例无分析错误，问题列表公开编号均为官方三段式。
- 发布包：`releases/1.4.135/FPGA-HDL-Static-Analyzer-1.4.135-Darwin-arm64.zip`
- SHA-256：`2aedceb5eaeffb7e3293ee937ee04a31db42d2dcc20ecabd3f48c50e81248a3e`
