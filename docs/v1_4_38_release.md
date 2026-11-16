# FPGA HDL Static Analyzer V1.4.38

本版继续只处理规则集，未调整任何表格或界面。

GJB10157.9.2.1 已由通用 `VL-CASE-DEFAULT` 映射升级为条款专用实现。附件原生收集器只接纳真实 case 节点并排除 if 降低的 `AST_CASE`；判定函数接受两种回退结构：`AST_DEFAULT + AST_BLOCK`，以及唯一子节点为 `AST_BLOCK`。结合附件中的 Yosys 前端标识和官方解析器源码，后一结构对应 Verilog 合法的省略冒号写法 `default <statement>`。

专用检查器在条件预处理后的源码上分别检查 case/casez/casex，支持 `default:` 与省略冒号的 default；空 case、数值标签全集但无 default、嵌套 case 和多个 case 均按原生节点粒度处理。源码 if 不进入本条，未激活条件编译分支中的 default 也不会错误消除告警。

新增正反例与边界回归覆盖：标准 default、无冒号 default、空 case、普通/casez/casex、两个缺省 case 分别报告、if 无 else 不触发本条、内层 default 不替代外层，以及未激活预处理 default。预处理失败、生成/简化新建节点、错误恢复 AST、精确字段名和原生消息仍保持未知；无告警结果继续为 `ManualReviewRequired`。

详细证据见 [native_verilog_case_default_9_2_1_findings.md](evidence/native_verilog_case_default_9_2_1_findings.md)。
