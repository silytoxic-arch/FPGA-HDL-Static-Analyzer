# V1.2.70 case/default 规则证据更新

本版只处理规则集，未调整表格。GJB10157.9.2.1 的原生核心分支已与 AST 类型表交叉核对：`AST_CASE` 下的 `AST_COND/CONDX/CONDZ` 分支是否包含 `AST_DEFAULT`，另有单一 `AST_BLOCK` 回退形式。现有产品的 case/casex/casez 缺省检查通过附件正反例及嵌套、数值全集边界测试；原生单一 block 形式仍待解释，见 `evidence/native_verilog_case_default_9_2_1_findings.md`。同时修正 9.2.10 证据说明：AST 0x57 是 `AST_CASE`，不是直接的 if 节点。

完整 CTest 130/130 通过。附件 446 个文档示例中 Automated 类别吻合 219/224；9.2.1 错误例报告 2 处、正确例 0 处。209 条规则中 72 条原生核心判据部分接入，137 条仍需解释；0 条宣称完整厂商等价。无告警不判合规。本版沿用 SHA 绑定的静态反编译证据，没有新增 DLL 运行时校准。
