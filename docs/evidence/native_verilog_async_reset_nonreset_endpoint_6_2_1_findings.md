# GJB10157.6.2.1 异步复位连接到非复位端原生判据

## Recovered predicate

目标为用户附件中的 `T-Linter.dll`，SHA-256 为
`60e6b52875f4d7c3fbdf494d869b3a4bcc3fdc8ee63d8620ea9614da5d6599b0`。

静态逆向观察到：

- `GJB10157_VLOG_6_2_1` 构造函数位于 `0x1800224c0`；
- slot 0 为 `FUN_1806e8d60`（`0x1806e8d60`），证据
  `ev_2be3a67f908304b85d468cb0368d9c237a2416337bc8bfe6a3136677de8e53e6`；
- slot 1 为模块过滤器 `FUN_1807238a0`，证据
  `ev_aaa493aac258e6c8e9f4d04fcd2ebeb575f37e5d55e363a21c32a358a3f3965b`；
- 主函数建立异步复位/置位关系名集合，遍历 `AST_ALWAYS(0x52)`、
  `AST_ASSIGN_EQ/AST_ASSIGN_LE(0x55/0x56)`、`AST_CELL(0x4f)` 和
  `AST_IDENTIFIER(0x13)`；
- 过程赋值和组合赋值的数据表达式中出现已知异步复位名时进入候选集合；
- 单元连接分支会查询端口元数据，排除复位/置位端口，并对其他实际端口上的异步复位
  标识符构造诊断；
- 地址 `0x180f126e0` 的 GBK 常量解码为“连接到非复位/置位端口”，与附件条款和
  黑盒 `.DI(RST)` 违规例一致。

附件违规例覆盖三种确定路径：时钟沿过程中的 `D <= RST`、黑盒数据端口
`.DI(RST)`、组合逻辑 `assign my_logic = RST & COND[0]`。正确例只在异步复位
事件/条件和名称明确的 `.RST(RST)` 端口使用复位信号。

## Product integration boundary

产品在同一 Verilog 模块内使用解析器确认的异步复位/置位集合，检查：

- 连续赋值右侧使用异步复位，而赋值目标不是另一条已确认的复位网络；
- `always` / `always_comb` / `always_latch` 赋值右侧使用异步复位，而目标不是复位网络；
- 命名实例端口的实际表达式使用异步复位，且形式端口名明确不含常见
  reset/rst/set/preset/clear/clr 标记。

复位事件、复位条件、复位网络之间的传播和名称明确的复位/置位形式端口不报告。
位置映射端口、语义不明的自定义端口、完整库模型元数据、跨层次关系、宏、generate、
别名传播、复杂左值及原生诊断次数仍未恢复。无诊断保持 `ManualReviewRequired`，不声明
完整厂商算法等价。
