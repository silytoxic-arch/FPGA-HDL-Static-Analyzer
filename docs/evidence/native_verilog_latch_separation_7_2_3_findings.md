# GJB10157.7.2.3 锁存器与组合逻辑分离原生判据

## Recovered predicate

目标为用户附件中的 `T-Linter.dll`，SHA-256 为
`60e6b52875f4d7c3fbdf494d869b3a4bcc3fdc8ee63d8620ea9614da5d6599b0`。

- 构造函数为 `0x180022c30`；slot 0 为 `FUN_18056e500`，静态反编译证据为
  `ev_ef7a798da6d1f5a123bf8eacbd11ee8ac475347f5231741fde070b34055cd06d`；
- slot 1 为模块过滤器 `FUN_1807238a0`，只接纳 `AST_MODULE(2)`，证据为
  `ev_aaa493aac258e6c8e9f4d04fcd2ebeb575f37e5d55e363a21c32a358a3f3965b`；
- 主函数收集 `AST_ALWAYS(0x52)`，存在 `AST_POSEDGE/AST_NEGEDGE(0x65/0x66)` 的
  时序过程被排除；
- `FUN_180560070` 递归遍历 `AST_CASE(0x57)`、`AST_COND(0x58)`、
  `AST_DEFAULT(0x5b)` 和 `AST_BLOCK(0x54)`，定位 if/else 降低后的默认分支；
- `FUN_18056f800` 处理 `AST_ASSIGN_EQ/AST_ASSIGN_LE(0x55/0x56)`，归一化第一个
  `AST_IDENTIFIER(0x13)` 赋值目标并收集右值标识符关系；
- 附件错误例的锁存关系为 `C = C`，同一无边沿过程还包含独立组合目标
  `D = B | A`；正确例将锁存器放入独立子模块，当前模块不再混合描述。

上述内容来自离线静态反编译和附件示例对照，不是运行时跟踪。

## Product integration boundary

产品在条件预处理后的 Verilog/SystemVerilog 源码中检查以下确定子集：

- 只检查 `always`/`always_comb` 无边沿过程；
- 识别简单赋值目标在自身右值中出现的显式保持关系；
- 同时复用语义分析器已由不完整控制路径证明的 `RTL-LATCH-INFERRED` 目标；
- 当同一过程中还存在不同的简单赋值目标时，对这些混合组合逻辑目标报告
  `GJB10157.7.2.3`；
- 仅包含锁存目标的过程、完整 if/else 组合过程和边沿触发过程不报告。

原生记录比较的所有分支、位/范围目标归一化、case 锁存器、复杂嵌套条件、综合网表推断、
别名、宏和 generate 以及精确诊断次数仍未恢复。无诊断保持 `ManualReviewRequired`，不声明
完整厂商算法等价。
