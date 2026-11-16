# GJB10157.7.1.3 复位触发边沿与进入条件极性原生判据

## Recovered predicate

目标为用户附件中的 `T-Linter.dll`，SHA-256 为
`60e6b52875f4d7c3fbdf494d869b3a4bcc3fdc8ee63d8620ea9614da5d6599b0`。

静态逆向观察到：

- `GJB10157_VLOG_7_1_3` 构造函数位于 `0x180022a90`；
- slot 0 为 `FUN_1805629e0`（`0x1805629e0`），证据
  `ev_5227f8580d7594cfd1ce8c4870778518f68bfc4c2bd6d9667b892eb70eb9cacb`；
- slot 1 为 `FUN_180564050`，它直接委托给边沿触发 `always` 过滤器
  `FUN_1804e6df0`，证据
  `ev_16b894aa8c8c4e639d06ec49e60914d47df3cf0a8dc74fe48f5c0a9f0c8c24a3`；
- 主函数遍历 `AST_POSEDGE/AST_NEGEDGE(0x65/0x66)`，以事件信号名称为键记录
  上升沿/下降沿极性；
- `FUN_180562930` 复制关系名称，并保留其极性字节；
- 主函数随后遍历 `IF(0x57)` 节点，对直接标识符、前缀取反标识符以及标识符与
  一位常量的相等比较进行名称和极性比对；
- 条件表达的有效电平与事件边沿极性相反时进入诊断构造路径。

附件错误例中的 `posedge RST` 配合 `RST==1'b0`、`negedge RST` 配合
`RST==1'b1` 均被标记；正确例分别使用高有效和低有效的匹配条件。

## Product integration boundary

产品在条件预处理后的 Verilog/SystemVerilog 源码中检查边沿触发 `always`，为每个
简单事件标识符建立极性，并检查其作用域内以下确定条件：

- `if (RST)`；
- `if (!RST)` 与 `if (~RST)`；
- `if (RST == 1'b0/1'b1)` 及常量位于左侧的等价写法；
- 无尺寸 `0/1` 及一位二/八/十/十六进制 `0/1` 常量。

复合布尔条件、`!=`/case equality、别名传播、宏生成的事件、generate、层次关系、
完整原生前端降级方式及诊断次数仍未恢复。未知条件不报告；无诊断保持
`ManualReviewRequired`，不声明完整厂商算法等价。
