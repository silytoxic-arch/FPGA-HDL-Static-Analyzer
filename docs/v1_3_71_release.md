# FPGA HDL Static Analyzer V1.3.71

本版继续只处理规则逆向、判断逻辑与验证，未修改表格或界面布局。

## GJB9765.4.6.c

原生 slot 0 构建已认可常量和拒绝常量集合：字面量直接认可；标识符或复合表达式中的全部标识符必须引用已认可常量；拒绝依赖会传播到后续常量。产品接入架构声明区的可靠源码子集，识别实体端口、signal、variable 和已判定动态常量依赖，同时接受字面量、绑定 generic、枚举文字及先前静态常量。

包限定常量、用户函数、跨设计单元名称、deferred constant 和无法完成绑定的表达式不猜测为合规。无报警时该规则仍为 `ManualReviewRequired`。详见 [恢复记录](evidence/native_vhdl_constant_static_initializer_4_6_c_findings.md)。

## 覆盖与验证

- 209 条规则中，181 条已接入证据支持的核心判断；28 条仍待解释。完整原生算法等价数仍为 0。
- 完整 CTest 131/131 通过；附件 446 个示例复测，4.6.c 正反例均吻合。ManualReview 示例总体由 32 个吻合提升到 33 个，差异由 16 个降至 15 个。
- 当前沙盒中未完成桌面交互启动验收；Windows ZIP 需在 Windows 构建环境中生成和验证。
