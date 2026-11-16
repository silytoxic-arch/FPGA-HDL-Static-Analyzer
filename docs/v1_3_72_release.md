# FPGA HDL Static Analyzer V1.3.72

本版继续只处理规则逆向、判断逻辑与验证，未修改表格或界面布局。

## GJB9765.4.6.d

原生类型字段和诊断标签已经交叉恢复：integer、real、unsigned、signed。厂商入口对赋值左端和 RHS 递归类型化名称进行比较，存在十个有向组合：integer↔real、integer↔unsigned、real↔unsigned、signed↔integer、signed↔unsigned；没有 signed↔real 分支。

产品接入同一架构内可直接解析的端口、signal、variable、constant 名称及简单 signal/variable 赋值。显式转换、用户 subtype、包限定类型、函数返回类型和跨单元绑定不猜测为合规；无报警时仍为 `ManualReviewRequired`。详见 [恢复记录](evidence/native_vhdl_implicit_numeric_conversion_4_6_d_findings.md)。

## 覆盖与验证

- 209 条规则中，182 条已接入证据支持的核心判断；27 条仍待解释。完整原生算法等价数仍为 0。
- 完整 CTest 回归测试 131/131 通过。
- 附件 446 个示例复测；4.6.d 正反例均吻合。ManualReview 示例总体由 33 个吻合提升到 34 个，差异由 15 个降至 14 个。
- Windows ZIP 仍需在 Windows 构建环境中生成和验证。
