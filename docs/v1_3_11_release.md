# V1.3.11 规则集进展

本版只推进规则准确性，没有调整问题列表、代码规模明细或其他表格格式。

## 原生判据升级

- `GJB10157.9.4.3`：原生槽函数只接纳 AST 类型 `4` 的 function 根节点。
- 执行函数在每个 function 根下递归收集 AST 类型 `0x56` 的非阻塞赋值，并逐个报告。
- 产品专用适配器覆盖完整 function 范围、条件分支、循环体和选择目标，同时排除关系比较、task、always、原型、注释与字符串。
- 具体违规由自动检测器报告；无告警仍保持 `ManualReviewRequired`，不把未知宏展开、损坏语法或工程上下文伪判合规。

## 验证

- 专用正例、反例及边界测试通过。
- 全量 `ctest`：131/131 通过。
- 附件 446 个正反例审计结果见 `docs/evidence/t_linter_after_1_3_11.md`。

详细逆向证据见 `docs/evidence/native_verilog_function_nonblocking_9_4_3_findings.md`。
