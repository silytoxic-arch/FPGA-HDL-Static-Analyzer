# GJB10157.9.4.2：显式 `initial` 构造判据

## 目标与静态证据

- 目标：附件 `T-Linter.dll`，SHA-256 `60e6b52875f4d7c3fbdf494d869b3a4bcc3fdc8ee63d8620ea9614da5d6599b0`。
- 原生收集器 `FUN_18058cff0` 只接受 `AST_INITIAL(0x53)` 且节点字节 `+0x2a9 == 1`；运行入口 `FUN_18058cad0` 对每个收集节点按源码位置报告一次 Warning。
- 在完整分析数据库的 `.text` 范围搜索数值字段偏移 `0x2a9` 共得到 7 处指令访问：唯一把常量 1 写入 AST 字节的生产者是 `FUN_1800e1200`，其在 `0x1800e15fc` 执行 `MOV byte ptr [RDI + 0x2a9], 1`；`FUN_1808d9a90` 仅复制该字节，`FUN_18058cff0`、`FUN_1805437e0` 和 `FUN_180357360` 仅读取它。
- `FUN_1800e1200` 调用 AST 构造器创建类型 `0x53`，立即写入 `+0x2a9=1`，随后挂接该上下文的语句内容，并创建 `AST_BLOCK(0x54)`。
- 同一函数族接收 `Verilog2005Parser` 上下文并生成 Yosys AST；二进制保留 `Verilog2005Parser.g4`、语法规则名 `initial_construct`，以及 `antlrcpptest::Verilog2005Parser::Initial_constructContext` RTTI。相邻 RTTI 为 `Always_constructContext`，与模块项语法次序一致。
- `FUN_180170ee0` 中出现的数值 `0x2a9` 由 `FUN_1807e8b10` 写入解析器对象 `+0x78`，属于解析状态值，不是 AST 节点 `+0x2a9` 写入，已从规则证据中排除。
- 入口 `FUN_18058cad0` 的诊断文本位于 `0x180ef6268`；原始字节按 GB18030 解码为“禁止使用initial语句”，产品已使用相同文本。

## 结论与产品映射

观察可以确定：规则并非选择所有可能由后续变换生成的 `AST_INITIAL`，而是选择由显式 `initial_construct` 前端 lowering 标记的根节点。产品因此在成功的条件编译预处理结果上，逐个选择模块、macromodule、interface 或 program 设计单元中的显式模块项 `initial`，并按关键字物理行列报告一次 Warning。任务/函数内部的非法同名 token 和设计单元外文本不会形成该 AST 节点，因此不再报警；generate 区域中的 `initial` 仍属于模块项并保持可达。

该映射同时覆盖单语句、`begin/end`、命名块、属性前缀与 `fork/join` 形式；声明初始化不是 `initial_construct`，不会命中。关闭的条件编译分支不会产生节点，预处理失败时不猜测结果。

## 验证

- 附件两份违反示例各命中一次，两份正确示例均不命中。
- 回归覆盖单个及多个显式 initial、属性、命名块、fork/join、声明初始化、注释、字符串、活动/非活动条件编译分支、设计单元外 token、任务/函数内非法 token，以及 generate 内合法模块项。

## 剩余未知

静态分析没有恢复 `FUN_1800e1200` 在完整派生 visitor 虚表中的具名槽位，也没有执行 Windows 目标；宏/include 的全部解析恢复行为、损坏语法容错、interface/program 在厂商前端中的全部扩展语义及跨文件诊断排序仍未完全验证。因此这是证据支持的核心谓词与源码适配，不宣称整个商业前端逐字节等价；无告警仍按现有策略保留人工复核。
