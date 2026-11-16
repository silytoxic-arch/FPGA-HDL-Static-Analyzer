# GJB10157.6.1.2 原生判据恢复

## Recovered predicate

原生过滤函数 `0x180730b90` 仅接受 `AST_ALWAYS(0x52)`，且第一子节点必须为
`AST_POSEDGE(0x65)` 或 `AST_NEGEDGE(0x66)`。主函数 `FUN_180729250` 从展开设计中
收集时钟关系，在这些边沿过程下递归收集 `AST_ASSIGN_EQ(0x55)` 和
`AST_ASSIGN_LE(0x56)`，赋值数据表达式中的标识符与时钟名一致时报告。

产品使用解析器确认的边沿时钟与时序过程正文读集合求交集。敏感列表
不在正文读集合中，因此时钟仅用于 `posedge/negedge` 不报告。

## Evidence

- DLL SHA-256: `60e6b52875f4d7c3fbdf494d869b3a4bcc3fdc8ee63d8620ea9614da5d6599b0`
- 主函数：`0x180729250`
- 过滤函数：`0x180730b90`
- 主证据：`ev_1be0b0252eccce0f64a73f9a4c64990cec83b41408076e27ea7a5a8c3db26f21`
- 过滤证据：`ev_370fe66f55ae6530e42c0fca58ceaab7cef4d4724a76601782814c548ed7bcf6`

## Boundary

黑盒/单元库端口语义、别名传播、跨模块层次和生成结构尚未完全恢复。
当前不通过 `clk`/`data` 等端口名称猜测语义，以避免误报。未发现问题仍为
`ManualReviewRequired`，不宣称与原生工具完全等价。
