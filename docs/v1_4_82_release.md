# V1.4.82：R-13-1 task 时序控制判据闭环

本轮只深化规则判断，不修改界面或表格布局。公开编号继续使用官方三段式 `R-13-1`。

## 原生恢复

- 原生规则入口只接受 `AST_TASK(3)`，并递归检查任务体；赋值节点只有在专用标志字节非零时进入报告分支。
- 随包解析器表显示 `task_func_body` 通过规则 236 接收 `behavioral_stmt`；规则 539～567 的 `behavioral_stmt` 集合包含前缀时序控制，但不包含规则 460～482 的 `always_stmt` 家族。
- 规则 545 `behavioral_stmt -> non_opt_delay behavioral_stmt` 是该标志字节唯一已确认的实际设置路径；赋值内部延时对应的其他产生式不设置该标志。
- 因此原生代码中虽然存在 `AST_ALWAYS` 防御分支，但它对语法正常的 task 不可达；非法嵌套 `always` 不能被当作该规则的有效反例。Yosys 对应版本源码仅作为对随包二进制表的交叉印证。
- 详细证据见 [R-13-1 原生证据](evidence/native_verilog_task_timing_9_4_1_findings.md)。

## 产品接入

- 报告 task 主体中的前缀 `#` 时序控制，并保留准确源码位置。
- 明确排除赋值内部延时、事件控制 `@`、`wait` 和非法嵌套 `always` 文本，避免把相邻但不属于原生谓词的语法误报为 `R-13-1`。
- 无告警结果仍为 `ManualReviewRequired`；宏展开、包含文件位置归属、错误恢复及完整厂商前端等价尚未恢复。

## 验证

- 新增非法 task 内嵌 `always` 边界回归，并保留前缀延时、赋值延时、事件控制与 `wait` 的正反例。
- 自动化测试：`135/135` 通过。
- 209 条附件规则审计共探测 446 个示例：`Automated:matched` 435、`ProjectContext:matched` 6、`context-required` 4、原生与文档冲突 1，与 V1.4.81 基线一致。
- `R-13-1` 台账提升为 `evidence-supported-core-predicate; integrated-source-adapter`；此类适配器现为 13 条，完整厂商算法等价仍为 `0/209`。
- 209 条公开编号全部通过 `R-X-X/A-X-X` 三段格式校验。
- 发布包：`releases/1.4.82/FPGA-HDL-Static-Analyzer-1.4.82-Darwin-arm64.zip`。
- SHA-256：见发布目录中的同名 `.sha256` 校验文件。
