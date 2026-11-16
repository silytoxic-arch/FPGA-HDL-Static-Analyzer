# GJB10157 Verilog 子程序规则条件编译边界

## 范围

本记录覆盖 `GJB10157.9.4.1`、`.9.4.2`、`.9.4.4`、`.9.4.5` 和 `.9.4.6`。
附件 `T-Linter.dll` SHA-256：
`60e6b52875f4d7c3fbdf494d869b3a4bcc3fdc8ee63d8620ea9614da5d6599b0`。

## 二进制直接观察

- `.9.4.1` 只接纳 AST_TASK 根，再收集其过程及带特定前端标志的赋值后代；产品现有确定子集是
  task 内独立数值语句延时。证据：
  `ev_9a12ca9f702b1999aa1ef7e39b23adad3bd6fa78676f156ac98d9b3bfc5fa0cb`。
- `.9.4.2` 收集带 `+0x2a9` 标志的 AST_INITIAL 节点并逐节点报告；产品只自动报告显式
  initial 根。证据：`ev_b433931bb0da8d65d2a7ef81e810377c6eaab9d239e0985dd106560ff457f050`。
- `.9.4.4` 在 AST_MODULE 内收集 FUNCTION、TASK、WIRE 和 AST_ASSIGN_EQ，检查子程序对模块级
  候选对象的阻塞赋值，排除非阻塞赋值。证据：
  `ev_e9b91922eba7ad4818588c41bea1f1caf91c07a8afbea44c38d4d821c54b1117`。
- `.9.4.5` 对内部 `$func$...$result` 赋值的两侧调用前端位宽推导并在不等时报告。证据：
  `ev_0a5c5f8e4a601c41857969d2485a32e4f7eea3c4f2ffea1f5443da08b26e39c0`。
  后续已恢复阻塞/非阻塞候选、逐赋值粒度、精确格式和高置信位宽传播，详见
  `docs/evidence/native_verilog_function_return_width_9_4_5_findings.md`。
- `.9.4.6` 对内部 `$para_assign` task 参数绑定的两侧调用同一位宽推导并在不等时报告。证据：
  `ev_99bc8498ab2e03c2678324195c94fd9c9ddc22b01bdbd4068275f9c34d1c6fb8`。
  后续已恢复 `$task$`/`$para_assign` 生产路径、方向 child 顺序与实参位置复制，详见
  `docs/evidence/native_verilog_task_argument_width_9_4_6_findings.md`。

这些规则的直接输入均为解析或 lowering 后的 AST/内部节点，而不是未经条件选择的原始文本。

## V1.4.15 产品适配

产品此前在原始 token 上执行五个有界适配器，非活动宏分支中的 task、function 或 initial 仍可能
触发误报。V1.4.15 改为只在工程 `defines` 预处理成功后分析活动 token 流：

- 活动分支中的五类确定违规正常报告；
- 非活动分支不产生报告；
- 诊断行列保持原始物理源位置；
- 预处理失败时不猜测子程序结构。

## 仍未知

- 原生 `+0x29b`、`+0x2a9` 和 WIRE `+0xa2` 标志的完整写入条件；
- 完整参数宏、include、generate、内部名称构造及 Yosys 位宽推导；
- legacy/named/复杂 task 参数、类型化或参数化 function 返回值、错误恢复和诊断排序。

因此命中可以自动判定违规；无命中继续保持 `ManualReviewRequired`。
