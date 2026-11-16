# GJB9765.4.6.e 原生判断恢复记录

## 证据边界

- 目标：用户附件 `T-Linter.dll`，SHA-256 `60e6b52875f4d7c3fbdf494d869b3a4bcc3fdc8ee63d8620ea9614da5d6599b0`。
- 规则页面：`GJB9765_VHDL.4.6.e.html`，SHA-256 `6041b58a24eeb00651b92454e8ca4e3b97be04510dd077312085eaefaf808cce`。错误例在 `a1 + a2` 中混用 `unsigned` 与 `signed`，正确例没有这种运算式。
- 2026-10-21 使用工作区 Ghidra 12.1.4 保存项目，只读、`-noanalysis` 定点反编译 `FUN_180605b20`、`FUN_1806067a0` 和 `FUN_180607060`；没有执行厂商程序。完整伪代码保留在 `reverse-tools/Library/ghidra/home/Library/ghidra/ghidra_12.1.4_PUBLIC/application.log`。

## Recovered predicate

1. 入口 `FUN_180605b20` 遍历规则对象 `+0x78` 的每个输入记录，从记录 `+8` 的指针向量取得首个 AST 根；每轮先清空候选运算节点和 signed/unsigned 名称容器，再无条件调用递归构建器 `FUN_1806067a0(root)`。调用前不存在 `AST_ARCHITECTURE` 类型门控，因此原生状态生命周期是“每个输入 AST 根独立”，而不是 architecture-only。聚焦证据保存在 `reverse-tools/evidence/r6_6_root_lifecycle.txt`，SHA-256 `630e82c68fbfc85495b7577261b846142ff90226b855ec591dde6436362011e0`。
2. 构建器只从声明类 AST `0x78/0x79/0x06/0x0a` 登记类型名称；节点偏移 `+0xa4` 的名称进入 signed 映射，偏移 `+0x10c` 的名称进入 unsigned 映射。该字段映射与相邻 4.6.d 的诊断标签和十个分支交叉一致。
3. 候选运算节点必须恰有两个直接子节点，类型位掩码为 `0xe0fcf001f`，即 `0x24..0x28`、`0x34..0x37`、`0x3a..0x3f`、`0x45..0x47`。独立枚举证据把其中的二元家族限定为位/逻辑运算、六类关系运算及 `+`、`-`、`*`、`/`；`mod`、`rem`、移位、连接和幂不在该集合中。
4. 对候选节点的两个子树，`FUN_180607060` 递归收集 `AST_IDENTIFIER(0x13)` 以及自身带 signed/unsigned 标志的后代，并按名称去重。
5. 入口逐个候选节点检查收集名称：只要该运算子树至少有一个名称命中 signed 映射，同时至少有一个名称命中 unsigned 映射，就为该运算节点构造一次诊断。因此嵌套表达式可能在内层和外层分别报告。

## 产品实现和未决项

产品对每个独立 architecture、package specification 和 package body 根内可直接解析的端口、signal、variable、constant 声明建立 signed/unsigned 名称表，并在赋值、初始化、IF/ELSIF、WHILE、ASSERT、RETURN 和 CASE 控制表达式中按二元运算树检查。package body 子程序中的表达式随所属根递归检查；每个根重新建立名称表，避免同一源码文件内相邻设计单元的同名对象相互污染。条件赋值的各分支独立处理，避免把互斥分支中的不同类型错误合并。

用户 subtype、包限定类型、重载解析、函数返回类型、显式转换的专有 AST 降低方式、selected/indexed/record 名称、跨文件和跨层次绑定、错误恢复树及原生诊断顺序仍未恢复。没有命中的规则状态继续为 `ManualReviewRequired`，不声明完整原生等价。
