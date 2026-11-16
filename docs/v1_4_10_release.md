# FPGA HDL Static Analyzer V1.4.10

本版继续只处理规则集，不调整界面。

## GJB10157.5.2.15 条件编译命名检查

原生 `FUN_1805306e0` 对前端解析后的 `AST_PARAMETER(9)`、`AST_LOCALPARAM(10)` 和
`AST_WIRE(6)` 执行名称大小写判断。产品此前在原始 token 上执行该条，并把条件编译块整体
跳过，导致工程宏启用分支中的确定违规漏检。

本版改为使用工程 `defines` 预处理后的活动声明：

- 活动分支中的 parameter/localparam 小写字符按 DLL 固定默认位报告；
- 活动分支中的 wire 类名称继续要求首字符为 ASCII 字母，且首字符之后不能含大写字母；
- 非活动分支不产生误报；
- 报告保留声明的原文件物理行列；
- 预处理失败时不猜测声明集合，无告警仍为 `ManualReviewRequired`。

新增活动违规分支、非活动违规分支和源位置边界测试。附件 `parameter Data` 的 HTML 遵循标记
与 DLL 固定判据冲突，仍作为上游文档差异保留，不修改二进制证据支持的判断。

## 附件审计

- 标准规则：209 条；附件示例探测：446 个；
- Automated：435 个吻合、1 个差异；唯一差异仍为上述 5.2.15 HTML 与 DLL 冲突；
- ProjectContext：4 个可判定示例吻合；6 个缺少工程上下文的示例保持
  `context-required`；
- 台账继续记录 `fullAlgorithmsVerified: 0`，不把本次 AST 输入边界修正描述成完整厂商前端恢复。
