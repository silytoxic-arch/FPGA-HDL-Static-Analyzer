# V1.2.71 case 位宽与 X/Z 条件更新

本版只处理规则集，未调整表格。GJB10157.9.2.3 的原生分支仅接纳 AST_COND（普通 `case`）选择项；产品现限制该 GJB 映射，避免 `casez/casex` 的通用位宽诊断误映射。GJB10157.9.2.4 的原生分支在 AST_CONDX/Z 存在时跳过，产品相应不再把 `casez/casex` 条件中的 X/Z 直接报到这条规则。普通 `case` 和源级 `if` 的已知字面量仍检查。见 `evidence/native_verilog_case_item_width_9_2_3_findings.md` 与 `evidence/native_verilog_unknown_condition_9_2_4_findings.md`。

新增两规则的正反和边界回归，完整 CTest 130/130 通过。附件 446 个文档示例中 Automated 类别吻合 219/224。209 条规则中 74 条原生核心判据部分接入，135 条仍需解释；0 条宣称完整厂商等价。无告警不判合规。本版沿用 SHA 绑定的静态反编译证据，没有新增 DLL 运行时校准。
