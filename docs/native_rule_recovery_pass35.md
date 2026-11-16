# 第三十五轮：GJB10157 R-12-2 FOR 循环体运行时算术

原生 `T-Linter.dll`（SHA-256 `60e6b52875f4d7c3fbdf494d869b3a4bcc3fdc8ee63d8620ea9614da5d6599b0`）中 `R-12-2` 的核心入口确认为 `FUN_180586120`。该入口从 `AST_FOR(0x5c)` 的块体内收集 `AST_ADD/SUB/MUL/DIV/MOD/POW` 下的全部 `AST_IDENTIFIER`，排除循环计数器与模块参数/localparam 后报警。

本轮进一步反编译 `FUN_180580440`，确认候选集合以 AST 节点指针为键，而非变量名或运算符：嵌套算术导致同一节点被多次访问时去重，但两个独立源码出现位置分别报警。原生诊断定位到候选标识符，GBK 模板也确认两个占位依次为循环计数器和候选变量。

产品据此从“相邻操作数、按运算符报警”改为完整表达式范围递归：按标识符 token 出现位置去重并定位，支持嵌套表达式、双运行时操作数、索引内部运行时名称、条件表达式与内联 `integer` 计数器，同时排除嵌套 `for` 头部。原始证据 `reverse-tools/evidence/r12_2_for_body_runtime_arithmetic.txt` 的 SHA-256 为 `1b3a71a8621b5f8a0a6d13180ee71b121aabae5a7a0c48deb26f92b6c844df53`，详细发现见 `docs/evidence/native_verilog_for_body_arithmetic_9_3_2_findings.md`。

尚未恢复完整 Yosys `AST_CASE` 降低、宏/包/层次绑定、函数与任务调用、非法源码恢复、生成源码映射及全局诊断顺序。产品继续只对声明解析可证明的运行时对象报警，无告警仍为人工复核，不判定合规。
