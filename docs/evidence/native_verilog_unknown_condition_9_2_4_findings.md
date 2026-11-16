# GJB10157.9.2.4 X/Z 条件（有界接入）

附件 T-Linter.dll SHA-256：`60e6b52875f4d7c3fbdf494d869b3a4bcc3fdc8ee63d8620ea9614da5d6599b0`。原生 slot 0 证据 `ev_4a20a076c4f9078f61bbc0534bfbea9293c29c92b085f8633a6ce8fa77c39631` 对 AST_CASE(0x57) 的选择器子树搜索 AST_CONSTANT(0x10)，扫描常量编码中 `0x03` 或 `0x02` 字节并输出 Warning；若存在 AST_CONDX/Z(0x59/0x5a) 分支则跳过。slot 1 `ev_9533e82fa089a8346a022bb518139a5b0c6a71f674bc6ad31fe2bda35b38c8ce` 接纳 AST_CASE。类型名据 `ev_512c915b90a165536016fafbb5a5ded6cfba13c0084ad97b3576b7906ee5547b`；编码字节与 X、Z 的一一对应仍需核实。

产品保留源级 `if` 和普通 `case` 条件中的直接 X/Z 字面量检查，本版依据原生排除条件跳过 `casez/casex`，它们仍可由其他条款独立报告。新增 X、Z、if、case、casez、casex 正反边界回归。源级 if 如何降低为 AST_CASE、常量传播与宏展开未恢复；无告警不判合规。
