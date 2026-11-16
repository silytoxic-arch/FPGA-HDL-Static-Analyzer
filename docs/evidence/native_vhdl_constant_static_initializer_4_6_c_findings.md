# GJB9765.4.6.c 常量静态初值判断恢复

目标为用户附件 `T-Linter.dll`，SHA-256 `60e6b52875f4d7c3fbdf494d869b3a4bcc3fdc8ee63d8620ea9614da5d6599b0`。2026-10-20 使用工作区 Ghidra 12.1.4 保存的 PE x86-64 项目，对 vtable slot 0 `FUN_1806016c0` 做只读定点反编译；未执行厂商 Windows 程序。完整伪代码保留在 `reverse-tools/Library/ghidra/home/Library/ghidra/ghidra_12.1.4_PUBLIC/application.log`，现有保留证据为 `ev_5f591cf433b08c94fd59aae3b050b90a666c6db29bb7ea7f776975767cec926c`。

## Recovered predicate

- 函数按设计根处理类型 `0x0a`。附件二进制自身的枚举映射将它标识为 `AST_LOCALPARAM`，VHDL 常量在该前端路径中进入这一类；规则只接纳节点字节 `+0x178 != 0`。
- 生产者 `FUN_1801b2770` 为同一名称创建 `AST_LOCALPARAM(0x0a)` 和可选 `AST_VARASGN(0xa2)`，无条件把前者 `+0x178` 置 1；没有初始化时销毁赋值节点。因此 `+0x178` 是该 VHDL 常量产生式的来源标志，不是 deferred constant 专属状态。完整伪代码保存在 `reverse-tools/evidence/r6_4_field_178_writers.txt`。
- 无初始化子节点的记录进入已认可名称集合。一个初始化子节点为 `AST_CONSTANT(0x10)`，或子节点的 `+0x10e` 字节为 1，也进入已认可集合。
- 全映像指令扫描只发现 `FUN_1801c2660` 对 `+0x10e` 的实值写入，以及 `FUN_1808d9a90` 的字段复制。`FUN_1801c2660` 在直接类型名等于 `boolean` 时置 1，并对解析后的声明/赋值对象重复同一类型标记流程；相邻字段同函数内分别对应 `integer`、`bit_vector`、`std_logic_vector`、`unsigned`、`signed`、`real`、`character`、`string`、`natural` 和 `positive`。因此 `+0x10e` 是 boolean 类型标志，不是常量折叠标志。完整反编译保存在 `reverse-tools/evidence/r6_4_boolean_flag_producers.txt`。
- 初始化根为 `AST_IDENTIFIER(0x13)` 时，名称必须已经存在于已认可集合；否则该常量名进入拒绝集合并报告。
- 其他复合初始化式递归收集全部 `AST_IDENTIFIER(0x13)`。任一名称不在已认可集合时报告并把当前常量加入拒绝集合；全部名称均已认可时当前常量进入认可集合。因此动态依赖会继续传播给后续常量。
- 函数还收集带 `+0x109` 整数标记的 `AST_SIGNAL(0x78)` / `AST_VARIABLE(0x79)` 名称及其物理行，再扫描双子节点 `AST_ASSIGN_LE(0x56)`。它先排除与上述声明物理行相同的赋值；当剩余赋值目标属于整数对象集合时，逐个检查 RHS `AST_IDENTIFIER(0x13)` 是否不在认可集合、但在拒绝集合中，并在赋值位置报告目标与依赖名称。该分支不接纳 `AST_VARASGN(0xa2)`。
- 2026-11-02 从同一 DLL 映像只读恢复三个 GB18030 诊断片段：`0x180f018e0` 为“常量”、`0x180f018d0` 为“的赋值表达式中”、`0x180f018a8` 为“不是固定整型值或可计算的固定整型值”。三段在常量拒绝分支和赋值二次定位分支以相同顺序拼接。
- 递归收集器 `FUN_1804f3870` 深度优先访问每个子节点，对每个匹配类型节点分别追加指针，不按名称去重；赋值分支随后逐项调用报告器。因此同一个被拒绝常量在 RHS 出现两次会产生两条字节相同的赋值诊断。完整伪代码保存在 `reverse-tools/evidence/r6_4_recursive_collector.txt`。

附件 HTML `GJB9765_VHDL.4.6.c.html` 的 SHA-256 为 `1285e37f9b817418993efaab8f5c6854595f3b599ebadca01a5bec9d80a51503`。违反例 `constant Cint2 : integer := S + 1` 中 `S` 是信号；遵循例使用先前常量和固定值。

## Product mapping

产品只自动报告源码中能够证明的完整 architecture、package specification 和 package body 设计根依赖：实体端口，以及 architecture、package、process、block、function/procedure 声明区中的 signal/variable 和已经判为动态的前置常量。字面量、绑定到 architecture 所属 entity 的 generic、枚举文字和同一根内先前已认可常量作为静态来源。直接声明为 `boolean` 的常量按原生 `+0x10e` 分支在标识符依赖检查前接纳，即使初始化式读取运行期 boolean 对象或关系表达式；这反映厂商判据对“固定整型值”规则的类型门控，而不是宣称该表达式在语言语义上静态。动态常量依赖会继续传播；每个独立 package/architecture 根都重新建立名称状态。architecture 与 package 均支持完整、命名或裸结束，范围识别维护嵌套子程序与本地 package 名称栈。

零初始化子节点的 deferred constant 按原生分支加入已认可集合；若 package body 中另有完成声明，则在独立 package-body 根重新判断该完成声明，不跨根继承状态。

赋值二次定位只接入源码可证明子集：直接声明为 `integer` 的 signal/variable、目标为简单名称、运算符严格为 `<=`、RHS 直接词法标识符已在同一根被判为动态常量，且赋值不与整数对象声明处于同一物理行。未知常量、用户子类型和无法绑定名称不会被提升为拒绝项；同一被拒绝名称每次词法出现均保留一条赋值诊断。无报告仍为 `ManualReviewRequired`。

包限定常量、用户函数、跨设计单元名称、物理类型以及无法完成绑定的表达式不自动判合规，规则无报警时仍保持 `ManualReviewRequired`。无初始化的 deferred constant 已依据 `FUN_1801b2770` 对 `+0x178` 的无条件写入和零子节点分支纳入已恢复判据。

## Remaining unknowns

节点字节 `+0x178`、`+0x109`、`+0x10e` 的正式字段名，boolean/integer 标志通过用户 subtype、别名、包限定类型和重载解析后的完整传播路径、嵌套同名对象的作用域遮蔽、跨 package 可见性和函数静态性解析、配置/生成结构及错误恢复仍未恢复。当前实现只接入直接 `boolean` 类型拼写，不把未知 subtype 伪判为已验证；这是证据支持的有界子集，不宣称与厂商前端完全等价。
