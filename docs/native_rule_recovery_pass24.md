# 第二十四轮：GJB10157.9.4.1

保存的 T-Linter.dll 静态证据表明：slot 2 仅选择 AST_TASK(3)，run 递归收集 task 后代中的 AST_ALWAYS(0x52) 并逐个报警；另收集 AST_ASSIGN_EQ/LE(0x55/56)，只对 `+0x29b != 0` 的节点报警。证据 `ev_9a12ca9f702b1999aa1ef7e39b23adad3bd6fa78676f156ac98d9b3bfc5fa0cb`、`ev_35d0750cf26166e2c36c2588f3bfb112cbf55220f35db8e59eb9b9a0d80a66c0`、`ev_64f1059ebe9591553a1ca8ce04e68a11ec13f174085f30430d2355624bf9696d`；先前记录见同工作区 `reverse-tools/evidence/t_linter_pass3/function-task-predicates.md`。

产品以附件明确的 `task t2; #10 b=...; endtask` 正反例为范围，接入 task 内独立数值 `#` 延时检查，报告 Warning。附件一份错误例命中、一份正确例不命中；边界测试包括多 task、注释、task 外延时、事件控制和赋值内延时。后两类因字段语义未恢复，不强行自动判定。

原生 `+0x29b` 字段写入、AST_ALWAYS lowering、宏/生成结构、其他时序控制和原生消息位置未完全恢复。本条仅算部分接入；无告警仍需人工复核。209 条台账现为 37 条核心谓词部分接入、2 条模型判据已观察未接入、172 条待解释、完整等价验证 0 条。
