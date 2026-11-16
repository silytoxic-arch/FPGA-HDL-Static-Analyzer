# GJB9765.4.8.a：固定常量 CASE 选择表达式

目标为附件 `T-Linter.dll`（SHA-256 `60e6b52875f4d7c3fbdf494d869b3a4bcc3fdc8ee63d8620ea9614da5d6599b0`）。静态反编译证据保存在 `rea_vhdl_case_constant_4_8_a_bundle.json`；以下“观察”指反编译控制流，不是厂商源码或运行验证。

## 原生观察

- 规则入口 `FUN_180607cb0` 先填充名称集合，再遍历类型 `0x57` 的 AST 节点，跳过两个标志置位的节点，取首个子节点作为选择表达式。子节点为类型 `0x10` 时直接报告；为 `0x13` 时按名称集合查找；其他表达式则收集全部 `0x13` 后代，仅当名称均在集合内时报告。报告位置取选择表达式子节点的行号。证据 `ev_bc0f722c9b21f3f1bebb5edbc5e7316797c851c5d91e88e91a9096d6f7d98ca4`。
- 名称集合的一条来源是 `FUN_180608bc0` 对类型 `0x76` / `0xa3` / `0x13` 节点的遍历；后续规则恢复已把这条直接层次交叉确认成 entity / generic / identifier。主函数还递归收集类型 `0x0a`，仅把字节 `+0x178` 非零的声明名加入集合。名称查找经 `FUN_1804c5730` 转小写。证据 `ev_0045125577774a59b95281d68c8a30bc680ec3ce1750c75bddd6a5789dc89c47`、`ev_175fe37265fd337fda2ce81459024ad8d4cf613bc779baa1430a338955202284`、`ev_5798100ee296ea297f6d3f504ff53fde08e2db04adfedf884d5598860304f2b4`。

## 推断与接入范围

附件违反例的 `CFS_COND` 是 architecture 常量，遵循例选择输入信号 `A_in`；结合上述控制流，`0x13` 为标识符，`0x10` 为字面量，`0x57` 为顺序 CASE。`0x76/0xa3` 的 entity/generic 含义现已由相邻 VHDL 规则交叉确认；`0x0a` 的所有标志生产路径仍未完全命名。

产品据此对顺序 CASE 的字面量、所属 entity 的 generic、architecture/process 声明常量以及仅含这类常量的有界运算表达式报告该条款；generic 按 architecture 所属 entity 隔离，不把其他 entity 的同名 generic 泄漏进来。不把 `case generate` 误作顺序 CASE，也不把分支中的常量当选择器。未知函数调用、package 常量及不确定的子程序作用域仍交人工复核。报告定位在选择表达式首 token。

附件正反例现均吻合，另有大小写无关名称、字符字面量、复合表达式、信号依赖、跨行定位和 generate 排除的单元边界测试。审计见 `t_linter_after_1_2_54.json`。无诊断仍显示 `ManualReviewRequired`，不宣称完整等价。

V1.4.57 新增 entity generic 和 process 局部常量两个正例，以及同名 generic 不跨 entity 根
传播的负例。前两个正例在旧实现中均漏报，接入上述名称来源后通过；完整等价仍不宣称成立。
