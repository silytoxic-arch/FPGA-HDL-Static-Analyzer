# FPGA HDL Static Analyzer V1.4.37

本版继续只处理规则集，未调整任何表格或界面。

GJB10157.9.2.7 已由通用重复/通配诊断映射升级为条款专用实现。依据附件 `T-Linter.dll` 的保存静态反编译证据，检查器现在对 case/casez/casex 的每一对标签分别判断：常量状态 `0x04` 作为通配；不同位宽先比较共同低位，较长值的额外高位出现 `1` 时才排除重叠；简单非定值标签按名称精确比较。每一组重叠对均在后出现标签处报告。

正反例及边界回归新增覆盖：普通重复与互异、casex/casez 通配语义、casez 中 X 非通配、不同位宽零扩展、较长值额外高位 1、同名标签、三标签三组重叠对、嵌套 case 隔离及条件预处理禁用分支隔离。

尚未恢复的参数折叠、负数/超大十进制、完整有符号扩展、复杂表达式名称字段和生成位置均明确保留为未知；无告警结果仍为 `ManualReviewRequired`，不伪判合规。详细证据见 [native_verilog_case_overlap_9_2_7_findings.md](evidence/native_verilog_case_overlap_9_2_7_findings.md)。
