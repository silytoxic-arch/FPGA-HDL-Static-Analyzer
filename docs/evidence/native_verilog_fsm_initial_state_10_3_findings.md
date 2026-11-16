# R-14-3 Verilog 状态机初始状态原生恢复记录

## 证据边界

- 目标：附件 `T-Linter.dll`，SHA-256 `60e6b52875f4d7c3fbdf494d869b3a4bcc3fdc8ee63d8620ea9614da5d6599b0`。
- 官方公开编号：`R-14-3`；内部映射条款仅保留在实现台账中。
- 原生入口：slot 0 `FUN_18050cc20`；候选分派 `FUN_18050fb00`；报告器 `FUN_18050d6e0`；关系构造器 `FUN_180511170`。
- 工具：Ghidra 12.1.4，只读工程 `T_Linter_Targeted`，OpenJDK 21.0.13。
- 原始反编译证据：
  - `reverse-tools/evidence/r14_3/native_core_functions.txt`，SHA-256 `7a7941df371923f4c7f6209f138b9c5cf982f93b83b9ceecf81a8ed20a7931a8`；
  - `reverse-tools/evidence/r14_3/native_helpers.txt`，SHA-256 `8d82e4f124d05620f303ca7a02ba101e8657de8d9ffdef3b56977ed9aac1b0d2`；
  - `reverse-tools/evidence/r14_3/native_strings.txt`，SHA-256 `ae354218ca6699562645af8ea88867baa1ce896af4c50567afc9417db3c462f3`。

## Recovered predicate

`FUN_18050d6e0` 在候选关系树中精确查找 ASCII 键 `init`。键不存在时输出 `Warning`，完整消息为 `状态机变量<name>未设置初始状态`。因此源代码状态名恰好为 `INIT` 并不能满足规则。

`FUN_180511170` 与辅助函数恢复出以下生产路径：

1. `AST_INITIAL (0x53)` 下受支持的相关赋值会生成 `init`；
2. 对控制表达式不包含候选状态变量的 `AST_COND (0x58)`，非默认分支中收集到的相关赋值会生成 `init`；
3. 非状态控制 `case` 的非 `default` 分支同样进入该生产路径；
4. 状态变量控制的 `case` 属于状态转换关系，不作为初始化生产者；
5. 普通 `else/default` 分支只有在尚未完全恢复的依赖图标志成立时才可能插入，不能单凭该分支赋值推定存在 `init`。

这说明原生判据是关系模型键是否存在，而不是“是否语义识别出复位”。例如普通 `if (en) state <= next_state` 的非默认分支也可能生成该键；反过来，只有 `else` 分支执行复位赋值时不能直接推定该键存在。

## 产品接入边界

产品复用已验证的有界状态机候选准入，并在同一模块内实现上述已证实生产者：

- 显式 `initial` 对状态变量或已绑定 next-state 变量的简单原子/简单三目赋值；
- 非状态控制 `if` 的 then 分支中的同类赋值，不要求存在 else；
- 非状态控制 `case` 的非 default 分支中的同类赋值；
- 扫描时跳过候选状态控制 case，防止把普通状态转换误作初始化。

仅出现在 `else/default` 的赋值、无条件普通赋值、声明初始化、复合或函数派生表达式、宏、generate、别名、多驱动、层次和跨文件绑定均不外推。未支持的无告警路径继续标记 `ManualReviewRequired`，不宣称完整原生前端等价。

`R-14-1` 的可达性起点仍使用更保守的可证明初始状态值，不直接把所有 `R-14-3` 合成 `init` 生产者当作可达性证明。

## 验证

- 无 `init` 关系：1 条 Warning，消息精确匹配原生文本；
- 源状态名为 `INIT`：仍报告；
- then 分支复位赋值：不报告；
- 两个 if/else 分支都只传递 next-state：按原生非默认分支生产路径不报告；
- 只有 else 分支复位：报告；
- if 无 else、非默认分支直接赋值：不报告；
- 非状态 case 的非 default 分支赋值：不报告；仅 default 分支赋值：报告；
- 显式 initial 的原子赋值或简单三目赋值：不报告；
- 无条件赋值和声明初始化：仍报告。

实现位于 `src/analysis/verilog_fsm_state_count.h` 与 `src/analysis/standard_evaluator.cpp`，回归位于 `tests/standard_accuracy_tests.cpp`。
