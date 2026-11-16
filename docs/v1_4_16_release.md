# FPGA HDL Static Analyzer V1.4.16

本版继续只处理规则集，不调整界面。

## 六条表达式规则条件编译修正

修正 `GJB10157.8.4`、`.8.5`、`.9.2.2`、`.9.2.4`、`.9.2.8` 和 `.9.2.9`
的条件编译输入边界。这些规则分别检查关系/算术表达式中的 X/Z、固定 case 控制量、控制条件
X/Z、casex/casez 以及 default 分支 X 赋值；原生实现消费前端生成的 AST 表达式或 case 节点。

本版改为仅在工程 `defines` 预处理成功后对活动 token 流执行 Verilog 有界适配器：

- 活动分支中的确定违规正常报告；
- 非活动分支中的相同表达式和 case 结构不再误报；
- 六条规则的诊断位置保持原始物理行号；
- VHDL 表达式检查路径保持不变；
- 预处理失败时不猜测结构。

新增六条规则的活动分支正例、非活动分支反例、精确重数和逐条位置测试。完整厂商宏/
include/generate 展开、常量传播和错误恢复仍未恢复，无告警继续保持人工复核。证据边界见
[表达式规则条件编译记录](evidence/native_verilog_expression_preprocessing_findings.md)。

## 附件审计

- 标准规则：209 条；附件示例探测：446 个；
- Automated：435 个吻合、1 个差异；唯一差异仍是已记录的 5.2.15 HTML 与 DLL 冲突；
- ProjectContext：4 个可判定示例吻合；6 个缺少工程上下文的示例保持
  `context-required`；
- 台账继续记录 `fullAlgorithmsVerified: 0`。
