# GJB10157.9.4.1 task 时序控制判据

附件 `T-Linter.dll` SHA-256：`60e6b52875f4d7c3fbdf494d869b3a4bcc3fdc8ee63d8620ea9614da5d6599b0`。

## 已确认的二进制事实

- 规则 slot 2 只接纳 `AST_TASK(3)`；run 递归收集 task 后代中的 `AST_ALWAYS(0x52)` 并逐个报告，另收集 `AST_ASSIGN_EQ/LE(0x55/0x56)`，只在节点 `+0x29b` 非零时报告。对应证据为 `ev_9a12ca9f702b1999aa1ef7e39b23adad3bd6fa78676f156ac98d9b3bfc5fa0cb` 与 `ev_35d0750cf26166e2c36c2588f3bfb112cbf55220f35db8e59eb9b9a0d80a66c0`。
- 对完整 DLL 的指令交叉引用恢复表明，`+0x29b` 唯一显式置一生产者位于 Verilog Bison 动作块。反向解析 `yyr1/yyr2/yystos/yypact/yytable/yycheck` 后，该动作精确对应第 545 条产生式：`behavioral_stmt -> non_opt_delay behavioral_stmt`。
- 动作先确认 AST 指针向量非空，再把尾节点 `+0x29b` 置一。因此 task 中 `#5 stmt`、`#(MACRO) stmt` 等前置延时均会把最终行为节点送入规则的赋值分支。
- 赋值内延时使用不同产生式：518 `simple_behavioral_stmt -> attr lvalue '=' delay expr`、522 `simple_behavioral_stmt -> attr lvalue OP_LE delay expr`；相应动作不写 `+0x29b`。不能把它们当作已恢复的 9.4.1 原生路径。
- 附件 DLL 的 Bison 表进一步证明：规则 236 的 `task_func_body` 只经 `behavioral_stmt` 接入 task 体；规则 539..567 是完整的 `behavioral_stmt` 产生式族，其中没有 `always_stmt`、`always_cond` 或 `always_event`。这些 always 相关语法位于互不相交的规则 460..482。因此，对该前端能够接受的 task 定义，`AST_ALWAYS` 后代分支不可达，它是原生规则中的防御性分支，不表示 `@(...)`、`@*` 或非法嵌套 `always` 必须由本规则报告。
- DLL 自报的 Yosys 基线提交 `6f9602b4cfedc6441f55272f6c025d64620cd24f` 与上述表一致；官方源码还显示简化器遇到 `AST_FUNCTION/AST_TASK` 会立即返回，不会在保存的 task 定义下生成 `AST_ALWAYS`。官方源码只作为解释性旁证，附件表和 DLL 指令仍是判据权威。完整记录见 `reverse-tools/evidence/r13_1_ast_always_reachability.txt`。

## 产品映射

产品在条件编译后的活动 token 流中跟踪 task 范围，报告其中每个非赋值内位置的前置 `#`。检测不再限定 `#` 后必须紧跟十进制数字，因此覆盖宏替换、括号表达式和附件中的直接数值形式。诊断定位到 `#`。

正反例及边界回归覆盖：附件 `#10` 错误例、无延时正确例、注释、task 外延时、括号宏延时、阻塞/非阻塞赋值内延时、`@` 事件控制，以及语法上非法的 task 内嵌套 `always` 不被词法误报。

## 仍未确认

- 非法/截断源码的 Bison 错误恢复、include 文件诊断排序、间接字段复制和确切报告顺序；
- 厂商文档所称“时序控制语句”与该 DLL 版本实际可达判据之间的版本差异。

因此，产品把 task 前置 `#` 作为已恢复且已接入的源码适配器；`@`、`wait`、赋值内延时和非法嵌套 `always` 不会被猜测为该版本的违规路径。无告警仍保持 `ManualReviewRequired`，不宣称完整厂商前端等价。
