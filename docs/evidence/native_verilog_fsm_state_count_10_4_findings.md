# A-14-1 Verilog 状态机状态数量原生恢复记录

## 证据边界

- 目标：附件 `T-Linter.dll`，SHA-256 `60e6b52875f4d7c3fbdf494d869b3a4bcc3fdc8ee63d8620ea9614da5d6599b0`。
- 官方公开编号：`A-14-1`；内部条款仅保留在实现台账中。
- 原生入口：构造函数 `FUN_180020700`，vtable `0x180ee93b0`，slot 0 `FUN_180512df0`。
- 候选验证/遍历：`FUN_180515d70`；关系构造：`FUN_1805173e0`；报告器：`FUN_1805138b0`。
- 工具：Ghidra 12.1.4，独立工程 `T_Linter_A14_1`，OpenJDK 21.0.12.1；静态分析，不执行目标 DLL。
- 原始证据：
  - `reverse-tools/evidence/a14_1/native_core_functions.txt`，SHA-256 `487cdc2300597bc73434790bc4673c6e2e9e3da2e284f526fa071ea772061d64`；
  - `reverse-tools/evidence/a14_1/native_relation_helpers.txt`，SHA-256 `a8e4518924c4752e700350168d0f75e5ee98471322d1bdcdf7aebd977bc9c1ef`；
  - `reverse-tools/evidence/a14_1/native_strings.txt`，SHA-256 `a1aaf9f3805b04710d495f3746ff14b6716f497df8078890c5fc5e887cd89f64`。
- 附件没有本规则的随包正反例；验证使用从原生分支导出的定向样例，不宣称完整厂商前端等价。

## Recovered predicate

`FUN_180512df0` 构建设计关系并把候选交给 `FUN_180515d70` 验证和遍历。最终 `FUN_1805138b0` 遍历候选关系键，明确执行以下步骤：

1. 精确比较并排除 ASCII 键 `init`；
2. 精确比较并排除 ASCII 键 `default`；
3. 将其余键插入去重树；
4. 仅在去重树大小满足 `0x28 < size` 时进入报告分支。

因此阈值是严格大于十进制 40：40 不报告，41 报告。计数对象是状态机关系中的去重状态键，不包含合成初始化关系和 default 关系。

`FUN_1805173e0` 对支持的 `AST_CONSTANT (0x10)` case 标签先调用前端数值转换，再以转换后的字符串作为关系键；具有直接名称的节点使用其规范化名称。这意味着等值数值编码应共享一个状态键，例如 `6'd1` 与 `6'b000001` 不应被重复计数。

报告级别来自 GB18030 字符串 `警告`。完整消息由下列片段拼接：

```text
状态机状态数不应超过40个,当前 + decimal(size) + 个
```

## 产品接入边界

产品只自动接纳可由源码证明的一进程直接状态更新，或具有直接 `state <= next_state` 传递的二进程状态机。状态 case 必须使用普通标识符控制，标签限于简单标识符或支持的数值原子。

已接入：

- 排除 default；合成 init 不进入源状态集合；
- 标识符保持 Verilog 大小写敏感；
- 支持的二/八/十/十六进制、位宽和 signed 数值标签按数值规范化后去重；
- 严格 `> 40` 阈值；
- Warning 级别和完整原生消息。

参数/常量标识符折叠、逗号标签的完整前端降低、复合标签、X/Z 通配语义、宏、generate、多驱动、层次与跨文件绑定仍不外推。无诊断的未支持路径保持 `ManualReviewRequired`。

## 验证

- 40 个去重状态：0 条告警；
- 41 个去重状态：1 条 Warning，完整消息为 `状态机状态数不应超过40个,当前41个`；
- 一进程、二进程和 `always_ff` 候选均覆盖；
- 41 个分支但只有 40 个不同标识符状态：不报告；
- `6'd1` 与 `6'b000001` 等值标签：去重，不报告；
- 41 路普通译码器但无状态更新：不作为状态机报告。

实现位于 `src/analysis/verilog_fsm_state_count.h` 与 `src/analysis/standard_evaluator.cpp`，回归位于 `tests/standard_accuracy_tests.cpp`。
