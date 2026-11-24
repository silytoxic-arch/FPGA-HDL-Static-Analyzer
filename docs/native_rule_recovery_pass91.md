# 第九十一轮：显式顺序块的 AST_BLOCK 生产链

沿用同一静态 T-Linter.dll 映像，未运行附件。

## 已绑定的 parser/listener 路径

- Seq_blockContext 虚表为 0x180e173e8，其 +0x80/+0x88 分别为
  FUN_1800a2d70、FUN_1800ab2e0。
- 两函数将 listener 动态转换到 Verilog2005ParserListener，分别派发
  虚表 +0xbf8/+0xc00。
- 实际 AST listener 虚表为 0x180e3dd90。该地址由构造器 FUN_1800d7890
  和析构/重置路径 FUN_1800d8870 的引用确认，不再按邻接函数猜测。
- 0x180e3e988（基址+0xbf8）为 FUN_1800e7170，下一槽为 FUN_180121af0。
- FUN_1800e7170 创建 AST_BLOCK(0x54)，将其挂到当前 AST 栈顶节点的 children，
  再压入 AST 栈；同一函数处理块命名和声明信息。
- FUN_180121af0 将 AST 栈尾指针减 8，恰好弹出一层。
- Always_constructContext 的 listener 派发 +0xb68/+0xb70 与同一表中的
  FUN_1800dcc60/FUN_180106db0 对应，确认了第八十七轮 always 生产者的绑定。

因此一个已识别 Seq_blockContext 对应一层显式顺序 AST_BLOCK。

## 剩余判定

尚需检查 procedural_timing_control_statement 对事件控制后的主体是否引入额外
AST_BLOCK。已找到其虚表 0x180e17c80，下一步从 +0x80/+0x88 回调及实际 listener
槽追踪生产者。未核实这一层之前，不把源码 begin 数量当作完整原生祖先深度，
也不直接给报告乘二。

原始块回调、实际生产者和 listener 引用见
`reverse-tools/evidence/t_linter_pass91/`。这是原生证据进展，既有未发布的多边沿
适配仍保持部分接入，完整前端等价为 0/209。
