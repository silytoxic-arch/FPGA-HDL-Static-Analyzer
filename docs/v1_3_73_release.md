# FPGA HDL Static Analyzer V1.3.73

本版继续只处理规则逆向、判断逻辑与验证，未修改表格或界面布局。

## GJB9765.4.6.e

原生入口、构建器和操作数收集器已经交叉恢复。厂商逻辑为每个恰有两个子节点的候选运算建立名称集合；当同一运算子树同时解析到 signed 与 unsigned 声明时报告。候选位掩码覆盖位/逻辑、六类关系以及 `+`、`-`、`*`、`/`，不包含 `mod`、`rem`、移位、连接和幂。

产品接入同一 entity/architecture 内可直接解析的端口、signal、variable、constant，并检查赋值、初始化及常见控制表达式。条件赋值各分支独立分析。用户 subtype、包类型、重载、函数返回类型、显式转换降低方式和跨单元绑定仍保留 `ManualReviewRequired`。详见 [恢复记录](evidence/native_vhdl_mixed_signedness_4_6_e_findings.md)。

## 覆盖与验证

- 209 条规则中，183 条已接入证据支持的核心判断；26 条仍待解释。完整原生算法等价数仍为 0。
- 完整 CTest 回归测试 131/131 通过。
- 附件 446 个示例复测；4.6.e 正反例均吻合。ManualReview 示例总体由 34 个吻合提升到 35 个，差异由 14 个降至 13 个。
- Windows ZIP 仍需在 Windows 构建环境中生成和验证。
