# GJB9765.4.6.d 隐式数值类型转换判断恢复

目标为用户附件 `T-Linter.dll`，SHA-256 `60e6b52875f4d7c3fbdf494d869b3a4bcc3fdc8ee63d8620ea9614da5d6599b0`。2026-10-20 使用工作区 Ghidra 12.1.4 保存项目，只读定点反编译 `FUN_180603930`、`FUN_180604790`、`FUN_1806050c0` 和名称表查询 `FUN_180676aa0`；未执行厂商 Windows 程序。主入口保留证据为 `ev_7f608e30d131fb45d96138de9db5bfeb18062745bb058c8c680617e43ab2b3fa`，完整新伪代码保留于 `reverse-tools/Library/ghidra/home/Library/ghidra/ghidra_12.1.4_PUBLIC/application.log`。

## Recovered predicate

- `FUN_180604790` 对 `AST_SIGNAL(0x78)`、`AST_VARIABLE(0x79)`、类型 6 和 `AST_LOCALPARAM(0x0a)` 读取四个类型标志并按名称登记：`+0x109`、`+0x10d`、`+0x10c`、`+0xa4`。
- `.rdata` 中诊断标签依次为 `整数`、`实数`、`无符号数`、`有符号数`。结合 `FUN_180603930` 十个有向匹配分支，可确定上述字段分别是 integer、real、unsigned、signed。
- 收集器只处理恰有两个直接子节点的 `AST_ASSIGN_LE(0x56)` 与 `AST_VARASGN(0xa2)`。第二个子节点是 RHS；`FUN_1806050c0` 递归收集其中 `AST_IDENTIFIER(0x13)` 及带 signed/unsigned/real 标志的节点。
- `FUN_180603930` 对每个输入记录取首个 AST 根，清空五组内部容器后无条件调用 `FUN_180604790(root)`；入口没有 `AST_ARCHITECTURE` 类型门控。因此独立 package specification、package body 及其子程序声明/赋值与 architecture 一样进入递归收集。完整伪代码保存在 `reverse-tools/evidence/r6_5_root_and_assignment_walkers.txt`。
- VHDL constant 产生器 `FUN_1801b2770` 为带初始化的声明创建 `AST_VARASGN(0xa2)`，与 `FUN_180604790` 对 `AST_VARASGN` 的双子节点分支相互印证：声明初始化式和过程内普通 `:=` 都属于本规则赋值集合，而不仅是 `<=`。
- 主函数将赋值首子节点名称作为一侧，对每个 RHS 名称查询类型表；类型组合命中时报告。明确存在的十个有向组合是 integer↔real、integer↔unsigned、real↔unsigned、signed↔integer、signed↔unsigned；没有 signed↔real 分支。
- 诊断由 `不应在`、两个类型标签和两个名称、`之间使用隐式转换` 拼接，证明比较粒度是赋值两侧的类型化名称，而不是仅搜索类型关键字。

附件 HTML `GJB9765_VHDL.4.6.d.html` 的 SHA-256 为 `8f6a7b15f28b7537caaad5074fbb7eca2b6c86f28dde861d74c7cbe97a8741cf`。违反例中 `a1` 标为 unsigned、`a2` 标为 integer，并执行 `a1 <= a2`；遵循例使用同类型 `std_logic_vector`。

## Product mapping and unknowns

产品检查同一实体/架构及独立 package specification/package body 根内可直接解析的端口、signal、variable、constant 名称，覆盖简单 `<=` / `:=`、signal/variable/constant 声明初始化式及 package-body 子程序中的赋值，严格采用上述十种类型组合。architecture 的 `end architecture;`、`end <architecture-name>;` 和裸 `end;` 均终止当前根；内部 function/procedure 和局部 package 的具名或裸终止符优先配对，不会误切 architecture。后续 entity、architecture、package/package body 重新建立类型表，与原生入口逐输入记录清空容器的生命周期一致。相同类型和 native 未列出的 signed↔real 不报告。package 扩展只作用于本规则，不据此推断相邻 `4.6.e` 的 package 运算树范围。

显式转换的专有 AST 降级、用户 subtype、package-qualified 类型、函数返回类型、重载、数组元素/记录字段、跨文件与层级绑定、别名、错误恢复和原生诊断数量仍未恢复。完整显式转换调用不自动展开；未知类型或名称不判合规，规则无报警时仍保持 `ManualReviewRequired`。
