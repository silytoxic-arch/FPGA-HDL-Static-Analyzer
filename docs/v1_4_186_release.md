# FPGA HDL Static Analyzer V1.4.186

## A-1-6 原生判据深化

- 公开编号严格使用官方三段式 `A-1-6`。
- 恢复五类 AST 候选、FOR/GENFOR 直接子赋值排除、同行两两配对和较早候选左值约束。
- 恢复按源路径与物理行去重，以及每个文件行最多一条诊断的行为。
- 恢复 Warning、物理行、第 1 列映射及固定正文。
- 专有前端分组、宏/include/generated 归属和候选顺序继续保持 `ManualReviewRequired`。

## 验证

- 新增固定诊断、裸循环体、复合左值和三候选去重断言；CTest `135/135` 全部通过。
- 附件审计覆盖544条安装规则、537个页面、209条 GJB 标准规则和446个示例：自动规则434个吻合、工程上下文规则6个吻合、4个明确需要上下文、2个原生实现与附件说明冲突，无未解释差异。
- 集成源码适配器由33条增至34条，剩余175条保持部分接入；完整原生等价仍为 `0/209`。
- 审计 JSON SHA-256：`36beb77ec9c61340c14e3dd3e9f679891fa63847bca7ed9dfc9d7968fb770e6e`。
- 原生规则台账 SHA-256：`2c74113dff6fb59281f85b2d3d42a4185ef87587c13adaf435043e153ae83169`。
- 发布包：`releases/1.4.186/FPGA-HDL-Static-Analyzer-1.4.186-Darwin-arm64.zip`，SHA-256 `e7b0e05446bad6dcdde7ff71d0e408b082edf3412290db0c0366d761ad081d8f`。
- 解包后 CLI 返回 `FPGA HDL Static Analyzer 1.4.186`；运行探针输出官方编号 `A-1-6`、Warning、第2行第1列及固定正文；应用短版本和构建版本均为 `1.4.186`，并通过 `codesign --verify --deep --strict`。

详细证据见 [第五十轮逆向记录](native_rule_recovery_pass50.md) 与 [A-1-6 原生证据](evidence/native_assignment_lines_5_1_6_findings.md)。
