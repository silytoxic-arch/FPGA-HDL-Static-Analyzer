# GJB9765.4.5.f.1：信号名称长度

目标附件 `T-Linter.dll` 的 SHA-256 为 `60e6b52875f4d7c3fbdf494d869b3a4bcc3fdc8ee63d8620ea9614da5d6599b0`。反编译证据保存在 `rea_vhdl_signal_name_length_4_5_f_1_bundle.json`；它是静态观察，不是厂商源码或运行验证。

## 原生观察

构造函数把两个长度参数默认设为 `1` 和 `0x14`（20）。运行入口把可选配置字符串解析为整数，再由辅助函数递归遍历 AST，把名称字节长度大于上界或小于下界的候选节点分开收集并逐个报告。证据 `ev_3601a6511a5ae0ab2ab5774e646c4d539c09e490e215d0787a1eb795993a79bf`、`ev_54bd8bcc694146b085e51d2c31448b76c63c8f43f3966e56a2d8d7a86fe9be49`、`ev_ee40a7de917d054ad0f2ab65f470653ad85425410a04a4c35e2e9de587250563`。

重新建立 Ghidra 12.1.4 工程并交叉查询枚举名称后，确认辅助函数并非只从 `0x75`、`0x54`、`0xa3`、`0xa4` 起点遍历：`FUN_1805e5770` 在函数末尾无条件递归所有子节点。上述四类当前节点只是额外检查其直接声明子节点。候选枚举已经确认为 `AST_FUNCTION(0x04)`、`AST_WIRE(0x06)`、`AST_LOCALPARAM(0x0a)`、`AST_IDENTIFIER(0x13)`、`AST_ALWAYS(0x52)`、`AST_BLOCK(0x54)`、`AST_ARCHITECTURE(0x75)`、`AST_ENTITY(0x76)`、`AST_TYPE(0x77)`、`AST_SIGNAL(0x78)`、`AST_VARIABLE(0x79)`、`AST_PROCEDURE(0x8e)`、`AST_SUBTYPE(0x9f)`、`AST_GENERIC(0xa3)` 和 `AST_PORT(0xa4)`。完整聚焦证据见 `native_vhdl_signal_name_length_4_5_f_1_candidate_set.txt`。

`AST_ALWAYS` 的内部占位名 `process`/`PROCESS` 被排除。`AST_BLOCK` 还有一个反直觉的原生边界：名称必须先长于 `0x16`（22）字节，且不能是 `enterProcess_statement`/`enterSequential_statement` 内部容器；因此 21、22 字节的块标号不会进入候选，23 字节非内部块标号才会报告。

## 产品修正与边界

旧实现仅检查端口和 `signal`，并曾把“长 entity 名不报告”写成原生边界；新的调用点和枚举证据推翻了这一结论。产品现按默认最大 20 字节覆盖可从源码可靠绑定的实体、结构体、类型、子类型、信号、变量、常量、端口、泛型、函数、过程、进程标号和块标号，同时保留 `AST_BLOCK` 的 22 字节专有准入门槛。新增测试覆盖各类别、同 token 去重和 21/22/23 字节块标号边界。

附件外部客户端对最小/最大阈值的配置、无法安全绑定用途的直接 `AST_IDENTIFIER` 子节点、生成/错误恢复节点、扩展标识符及原生遍历顺序仍未接入或证实。无诊断时维持 `ManualReviewRequired`，不表示自动合规。
