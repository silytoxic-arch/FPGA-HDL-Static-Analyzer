# GJB10157.6.1.4 原生判据恢复

## Recovered predicate

`FUN_18054f6c0` 初始化综合后连接模型，`FUN_180552630` 遍历相关时钟端点。
每个端点贡献上升沿或下降沿位，位掩码达到 `3` 时报告。

附件示例确认三类关系：同名时钟的上下沿、时钟生成器输出的上下沿，以及
`inv = ~CLK` 后 `posedge inv` 与 `posedge CLK` 的有效双边沿冲突。产品对直接别名
和一元取反别名进行有界归一化。

## Evidence

- DLL SHA-256: `60e6b52875f4d7c3fbdf494d869b3a4bcc3fdc8ee63d8620ea9614da5d6599b0`
- 主入口：`0x18054f6c0`
- 网络遍历：`0x180552630`
- 证据：`ev_f71f67d1bdf1d3f4bb0d6437bf70ea0394d40debcbb7e9945b8639a38cb929d2`

## Boundary

完整综合网表、原语/单元库反相语义、跨层次传播、复合表达式和生成结构仍未
完全恢复。未发现问题仍为 `ManualReviewRequired`，不宣称与原生工具完全等价。
