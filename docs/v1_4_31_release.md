# FPGA HDL Static Analyzer V1.4.31

本版继续只处理规则集，不调整表格或其他界面。

## GJB10157.8.3 混合符号算术

原生入口会分别收集每个 `AST_ADD/SUB/MUL/DIV/MOD/POW` 节点，再由
`FUN_180573af0` 递归遍历该节点的完整子树，只对可解析的 `AST_IDENTIFIER`
读取声明对象符号分类。分类中同时存在 0 和 1 才报告。

产品此前只收集运算符附近的叶子或括号操作数。本版按 Verilog 算术优先级与
结合性重建有界子树，并逐节点递归收集声明符号属性，因而可正确处理
`signed_a * 2 + unsigned_b`、`(signed_a + unsigned_b) * signed_a` 和
`signed_a ** (unsigned_b + unsigned_b)` 等父子表达式。

新增回归同时验证混合父节点、混合内层节点、纯 signed/unsigned 嵌套树以及
不贡献分类的字面量。强制转换、函数、宏、参数/生成展开和原生诊断重数仍保持
未知/人工复核。

## 验证与附件审计

- 全量自动化测试：135/135 通过；
- 标准规则：209 条；附件示例探测：446 个；
- Automated：435 个吻合、1 个差异；唯一差异仍是已记录的 5.2.15 HTML 与
  DLL 配置证据冲突；
- ProjectContext：4 个可判定示例吻合；6 个缺少工程上下文的示例保持
  `context-required`；
- 台账继续记录 `fullAlgorithmsVerified: 0`，未知边界不以无告警判定合规。
