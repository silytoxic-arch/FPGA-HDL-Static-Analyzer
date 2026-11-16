# GJB10157.6.1.3 原生判据恢复

## Recovered predicate

`GJB10157_VLOG_6_1_3` 的 slot 1 `FUN_18054ed80` 接受 `AST_ASSIGN(0x4d)` 和
`AST_ALWAYS(0x52)`，但当 `AST_ALWAYS` 含有 `AST_POSEDGE(0x65)` 或
`AST_NEGEDGE(0x66)` 子节点时排除。

slot 0 `FUN_18054de80` 在候选组合过程中收集 `AST_ASSIGN_EQ(0x55)` 和
`AST_ASSIGN_LE(0x56)`，并将赋值目标名称与展开后的时钟关系比较。连续赋值
的右值是直接标识符时被排除，因此普通时钟别名不属于“组合逻辑输出”。

## Evidence

- DLL SHA-256: `60e6b52875f4d7c3fbdf494d869b3a4bcc3fdc8ee63d8620ea9614da5d6599b0`
- 主函数：`0x18054de80`
- 候选过滤：`0x18054ed80`
- 证据：`ev_a2c625015f0d0a597841603ab30ba60956e1da25c812a0993e3bd7b033990930`、
  `ev_728be2738a7301c4d1b1fcc211bbd2f86f712a45cdf20651ab93666ae09aebc3`

## Boundary

产品当前覆盖有界的连续表达式和组合过程赋值。完整层次展开、生成块、
宏、复合左值及别名网络传播仍未完全恢复。未发现问题仍为
`ManualReviewRequired`，不宣称与原生工具完全等价。
