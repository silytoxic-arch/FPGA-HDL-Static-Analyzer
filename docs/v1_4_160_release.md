# FPGA HDL Static Analyzer V1.4.160

## 本轮目标

把 V1.4.159 已闭合的 `AST_LOGIC_NOT(0x47)` / `AST_BIT_NOT(0x24)` 枚举证据继续落实到 GJB10157 复位组合逻辑共享核心。对应官方公开编号为 `R-5-3` 与 `A-5-2`；内部兼容键分别为 `GJB10157.6.2.3` 与 `GJB10157.6.2.7`。本版不调整界面或表格。

## 原生证据与产品边界

- `FUN_180758410` 对连续赋值右侧接受直接 `AST_IDENTIFIER(0x13)`。
- 一元别名只接受 `AST_LOGIC_NOT(0x47)` 或 `AST_BIT_NOT(0x24)`，且根节点必须恰有一个直接 `AST_IDENTIFIER(0x13)` 子节点。
- 产品据此接受直接标识符、标识符选择及精确一级 `!`/`~` 标识符结构。
- 双重 NOT、函数调用和归约运算不属于该别名形态；若其目标被解析器确认用于异步复位/置位，则按组合逻辑驱动分别产生 `R-5-3` 与 `A-5-2` 诊断。
- 组合过程赋值仍按已恢复原生分支报告，不借用仅属于连续赋值的别名豁免。

## 新增回归

- `!(ARST)`：逻辑 NOT 单级别名，不报告。
- `~ARST`：位 NOT 单级别名，不报告。
- `~RST_BUS[0]`：直接子节点仍为标识符选择，不报告。
- `!!ARST`、`!pass(ARST)`、`&RST_BUS`：分别验证嵌套 NOT、函数调用和归约根，每个目标对两条规则各报告一次。
- 所有无诊断样例仍由标准评估状态断言保持 `ManualReviewRequired`，不声明整条规则合规。

## 审计与发布

- 完整回归：`135/135` 通过；新增的六类复位别名正反边界全部通过。
- 附件复审：544 条附件记录、537 个页面、209 条标准规则、446 个可探测样例。
- 结果保持为 435 个 `Automated:matched`、6 个 `ProjectContext:matched`、4 个 `context-required`、1 个 `native-documentation-conflict`。
- 209 个公开编号全部通过官方 `R-X-X` / `A-X-X` 三段式校验。
- 台账 `fullAlgorithmsVerified` 继续为 `0`；未把局部直接子节点判据声明为完整算法等价。
- 从 ZIP 全新解压后验证：CLI 版本 `1.4.160`，`R-5-3`、`A-5-2` 均可按官方编号查询，应用包签名有效，`CFBundleShortVersionString=1.4.160`，15 个 Qt 框架符号链接完整。

发布物：

- `releases/1.4.160/FPGA-HDL-Static-Analyzer-1.4.160-Darwin-arm64.zip`
  - SHA-256：`983f37ba42504f3434eb2ac898d2ac90521e9d4366ba6e40209eedd94d6b5ae1`
- `docs/evidence/t_linter_after_1_4_160.json`
  - SHA-256：`89b8cc786b735d2eeabc1d2a75702b047118caa3d1583806cab9cac42bc28848`
- `docs/evidence/native_rule_implementation_ledger.json`
  - SHA-256：`4c3775f72337834f15e5d9a47d25016565c8e22dc001697b5f69fad4e7a117c9`
- `docs/evidence/native_verilog_combinational_reset_6_2_3_6_2_7_findings.md`
  - SHA-256：`57f6a815b17d223cb0378d46528a6086f94b6ec02d1c7b4bd109f089a634dcaf`

## 仍未闭合的边界

异步复位关系图的完整生产过程、多级别名、跨层次传播、原语/库单元复位端口、宏、generate、复合左值、分层标识符的专有前端降低方式和原生诊断顺序/次数仍未知。未报告这些构造不表示合规。
