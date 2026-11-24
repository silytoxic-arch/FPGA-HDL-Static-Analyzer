# 第九十四轮：R-6-3 行号回调及表达式生产者

目标 SHA-256：`60e6b52875f4d7c3fbdf494d869b3a4bcc3fdc8ee63d8620ea9614da5d6599b0`。
本轮只静态分析，没有执行 T-Linter。原始 Ghidra 摘录在
`reverse-tools/evidence/t_linter_pass94/`；末尾空白已规范化。

## 已观察的链路

- AST 构造器 FUN_1808d8dc0 调用全局函数指针 DAT_18137cbb0，结果写入 +0x210。
- Verilog 前端 FUN_18094b1e0 将该指针设为 FUN_180138520，后者返回
  DAT_18131e000；VHDL 前端及另外的转换入口使用返回 0 的 FUN_180184800。
  因此前端不同，不能将这个字段的语义直接推广到全部语言和入口。
- 实际监听器虚表 0x180e3dd90 的 ExpressionContext exit +0x1420 绑定
  FUN_1800fc0d0。其结束路径显式将上下文 getStart()->getLine() 写入新表达式
  +0x210；0x36 相等节点和 0x47 逻辑取反节点均走这条写入路径。
- PrimaryContext exit +0x14c0 绑定 FUN_18011a640，
  Number 等子节点直接转交到主表达式映射；层次标识符路径由后续证据补齐。
- Conditional_statementContext 的入口构造 AST_CASE 0x57 及 IF 标记。

ANTLR getStart/getLine 的虚表映射沿用第八十七轮已恢复证据。以上建立的是
实际静态派发链，不是根据函数名推测执行行为。

R-6-3 原生相等分支读取常量节点 +0x210，直接标识符和逻辑取反读取条件节点
+0x210。下一轮核对 Number 和标识符生产者后，接入源代码位置适配。

宏展开、generate、合成节点、其它前端入口以及原生整体输出顺序仍未知。
