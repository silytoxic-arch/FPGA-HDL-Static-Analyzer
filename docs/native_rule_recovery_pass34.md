# 第三十四轮：GJB10157 R-12-1 FOR 头部常量性

原生 `T-Linter.dll`（SHA-256 `60e6b52875f4d7c3fbdf494d869b3a4bcc3fdc8ee63d8620ea9614da5d6599b0`）中 `is_initVal_const` 与 `is_loop_condition_const` 的前端生产逻辑已定位到 `FUN_180914500`。两个属性都在克隆表达式经过宽度准备和反复简化后、结果不是 `AST_CONSTANT(0x10)` 时生成；属性名称的真实极性因此得到确认。

同一分支确认 procedural FOR 与 generate FOR 分别要求 `AST_WIRE(6)` / `AST_GENVAR(0x5f)` 左绑定，且第 1、3 个 `AST_ASSIGN_EQ(0x55)` 的 `+0x1d0` 绑定指针必须完全相同。产品据此扩展显式目标身份比较、选择左值以及内联 `integer` / `genvar` 计数器，并把运行时依赖判断从逐标识符改为完整表达式上下文，避免把 `$bits(d)` 的参数误判为运行时边界。

产品仍只对声明解析能够证明的运行时依赖或显式目标不一致报警。完整 Yosys 常量折叠、参数覆盖、宏、常量函数、类型转换、包/层次绑定和隐式步进仍为人工复核；无告警不判合规。证据见 `evidence/native_verilog_for_header_9_3_1_findings.md`，原始聚焦记录 SHA-256 为 `3fa8e1135ca6c32ea0130b8b0bdb4a0477b0f52f4d7b7e506e031629353ab35a`，台账见 `evidence/native_rule_implementation_ledger.json`。
