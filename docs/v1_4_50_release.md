# V1.4.50 规则恢复说明

本版继续只处理规则集及其验证台账，没有调整问题列表、代码规模明细或其他表格界面。

## GJB9765.4.2.c 比较类型精化

只读反编译 DLL 的 `FUN_1808ea310` 类型名函数，补齐了 V1.4.49 尚未命名的原生 AST：

- `0xaa/0xab/0xac` 为 `AST_IF/AST_ELSIF/AST_ELSE`；
- `0xb2/0xb3` 为 `AST_STRING_LITERAL/AST_CHARACTER_LITERAL`；
- `FUN_18078c370` 的关系类型位掩码接纳 `AST_LT/LE/EQ/EQX/GE/GT`，明确排除
  `AST_NE/NEX`。

产品因此把 4.2.c 条件适配改为接纳 `=、?=、<、<=、>、>=`，不再把 VHDL `/=` 当作
原生候选。新增 `/=` 反例和 `?=` 正例；未知路径仍为 `ManualReviewRequired`。

## 附件样例冲突分类

GJB10157.5.2.15 的附件 HTML 把 `parameter Data` 标为正确，但 DLL 构造函数固定启用
parameter/localparam ASCII 小写拒绝，专用标志没有配置覆盖。审计现将这一项单列为
`native-documentation-conflict`，不再混入产品 `Automated:mismatch`；产品规则逻辑未为迎合
HTML 标签而放宽。

## 尚未恢复

4.2.c 条件操作数的全部声明类别、别名与跨实例传播、configuration/generate、聚合及复杂
选择目标、异常语法恢复和原生诊断重数仍未知。台账继续保留这些边界，不宣称完整供应商算法
等价。

## 发布验证

- 完整重配置、构建及 CTest：135/135 通过；
- 附件样例重新审计：446 个样例，其中 `Automated:matched` 435、
  `native-documentation-conflict` 1、`ProjectContext:matched` 4、
  `context-required` 6；没有未解释的 `Automated:mismatch`；
- 台账重新生成：209/209 核心谓词已接入，完整供应商算法等价验证仍为 0；
- 解包后的 CLI 报告 `FPGA HDL Static Analyzer 1.4.50`；
- 解包后的 macOS 应用通过 `codesign --verify --deep --strict`；
- 发布包：`releases/1.4.50/FPGA-HDL-Static-Analyzer-1.4.50-Darwin-arm64.zip`，
  91,349,039 字节，SHA-256
  `11fc293a4a3371d9b3aa9edb45010f634a406808904361719a2037c758e051c4`。
