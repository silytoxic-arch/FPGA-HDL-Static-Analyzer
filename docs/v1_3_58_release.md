# FPGA HDL Static Analyzer V1.3.58

本版只调整规则集，没有修改问题列表、代码规模明细或任何表格界面。

## 规则准确性

- 恢复并接入 GJB9765.4.9.c 的 VHDL 门控时钟核心判断：从解析器确认的时钟反向追踪赋值依赖，在二元逻辑或条件选择驱动处报告。
- 移除 `RTL-DERIVED-CLOCK` 到 4.9.c 的宽泛映射，避免把任意本地时钟生成方式都当成门控逻辑。
- 移除同一通用诊断到 GJB9765.4.9.g 的映射。附件中的 4.9.g 是行波时钟，不等同于门控时钟；在原生算法恢复完成前，该条款保持人工复核，避免无报警即伪判合规。
- 新增附件正反例、直接别名传播、逻辑运算、单独反相、算术派生以及 4.9.c/4.9.g 条款隔离测试。

原生证据和边界见 [GJB9765.4.9.c 记录](evidence/native_vhdl_gated_clock_4_9_c_findings.md)。

## 验证与交付

- 规则台账：209 条规则中 167 条核心判据已接入，42 条仍需继续解释；未宣称任何一条已达到完整专有前端等价。
- 附件审计：4.9.c 正反例 2/2 匹配；总计 `Automated:matched=353`、`Automated:mismatch=5`、`ManualReview:matched=42`、`ManualReview:mismatch=28`、`ProjectContext:matched=11`、`ProjectContext:mismatch=7`。
- 测试：核心构建 129/129，通过发布构建 131/131（含 GUI smoke 和工程工作流）。
