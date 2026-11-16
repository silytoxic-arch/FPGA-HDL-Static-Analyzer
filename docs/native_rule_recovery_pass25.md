# 第二十五轮：GJB10157.9.4.4

静态逆向证据：slot 1 选择 AST_MODULE(2)；run 收集 FUNCTION(4)、TASK(3)、WIRE(6) 子节点，WIRE 仅在 `+0xa2 != 0` 时进入候选；函数和任务的直属子节点名称从相应候选集合中剔除；随后仅对 AST_ASSIGN_EQ(0x55) 的首子节点名称与剩余候选相等者报警，AST_ASSIGN_LE(0x56) 不属于该检查。依据 `ev_aaa493aac258e6c8e9f4d04fcd2ebeb575f37e5d55e363a21c32a358a3f3965b`、`ev_e9b91922eba7ad4818588c41bea1f1caf91c07a8afbea44c38d4d821c54b1117` 和 `ev_d02a3a7b6216e84a9e2e87d6b439ba09f213ce9520a3e520c4538722f755f236`；完整先前记录见同工作区 `reverse-tools/evidence/t_linter_pass3/function-task-predicates.md`。

产品的独立有界适配仅选择 module 正文中简单、未初始化的显式 `reg` 声明，在同一 module 的 function/task 中检出对其直接名称的阻塞赋值；局部同名声明和函数返回名排除。附件错误例的两次写入均定位，正确例不报警。单元测试另覆盖 task、局部遮蔽、跨 module、非阻塞、位选择左值及初始化声明边界。

`+0xa2` 标志的语义及写入、复杂左值 lowering、宏/生成结构、原生跨子程序累计排除行为及诊断等价性尚不明确。此规则只算核心判据部分接入，未命中仍需人工复核。209 条台账现为 38 条核心谓词部分接入、2 条模型判据观察未接入、171 条待解释、完整算法等价验证 0 条。
