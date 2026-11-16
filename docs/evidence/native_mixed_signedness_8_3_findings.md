# GJB10157.8.3 混合符号算术：静态恢复记录

目标：附件 `T-Linter.dll`，SHA-256 `60e6b52875f4d7c3fbdf494d869b3a4bcc3fdc8ee63d8620ea9614da5d6599b0`。以下是 Ghidra 静态反编译观察，不是原厂源代码，也未执行 Windows DLL。

## FUN_1805728e0

- 规则入口 `0x1805728e0` 从模块记录收集 `AST_ALWAYS(0x52)` 和 `AST_ASSIGN(0x4d)` 根，再收集 `AST_ADD(0x3c)`、`AST_SUB(0x3d)`、`AST_MUL(0x3e)`、`AST_DIV(0x3f)`、`AST_MOD(0x40)`、`AST_POW(0x42)`。常量地址 `0x180e79ff0..0x180e79ffc` 的只读值依次为 `0x3c..0x3f`，另两个立即量为 `0x40`、`0x42`。
- 对每个算术节点的全部子树调用 `FUN_180573af0`。该递归过程只在节点类型 `0x13`（`AST_IDENTIFIER`）处调用 `FUN_1808f4650` 解析声明对象；若解析成功，读取对象偏移 `+0xa4` 的布尔值并加入位向量。
- 主过程分别统计位向量中的 0 和 1。全部为 0 或全部为 1 时不报；两类同时存在时在该算术节点发出 Warning。结合规则标题、signed/unsigned 声明正反例，可以确定这个布尔分类承担声明符号属性；但缺少调试类型，偏移字段名仍是语义推断。
- 附件错误例中 `reg signed a` 与默认 unsigned 的 `reg b` 分别参与乘、加、减，三个点均应命中；正确例的 `a*a`、`b+b`、`b-MY_IN2` 每个算术节点只含一种声明符号属性。

## 产品边界

产品规则 `RTL-MIXED-SIGNEDNESS-ARITHMETIC` 覆盖连续赋值和过程中的六类原生算术运算符，解析显式 signed/unsigned、默认无符号的 net/reg/logic/端口以及默认有符号的整数类型。V1.4.31 按 `+/-`、`*//%`、`**` 的 Verilog 优先级与结合性重建有界算术子树，并像 `FUN_180573af0` 一样对每个算术节点递归收集声明可解析的标识符，因此支持无括号复合链和嵌套父子节点的独立判断。常量选择仍解析为其基声明，字面量不贡献符号分类。

强制类型转换、函数调用、宏、参数与生成展开、拼接中的完整前端降低、错误恢复和原生告警数量仍未证明。无告警继续标为 `ManualReviewRequired`，不作为合规证明。V1.4.31 的复核摘要保存在 `reverse-tools/evidence/t_linter_pass4/gjb10157_8_3_recursive_identifiers.md`。
