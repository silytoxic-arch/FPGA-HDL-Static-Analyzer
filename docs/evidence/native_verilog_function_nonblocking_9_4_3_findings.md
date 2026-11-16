# GJB10157.9.4.3 原生判据恢复记录

## 目标与样本

- 样本：`T-Linter.dll` V3.1.4.25120520251205115359
- SHA-256：`60e6b52875f4d7c3fbdf494d869b3a4bcc3fdc8ee63d8620ea9614da5d6599b0`
- 执行函数：`FUN_18058d060`（`0x18058d060`）
- 根节点接纳函数：`FUN_18058d740`（`0x18058d740`）
- 构造函数：`FUN_180024660`（注册 `GJB10157_VLOG.9.4.3`，vtable `0x180ef6328`）
- 执行函数证据：`ev_479db7b1c96fbb20ca9f2d8e00fe26f5b1fa2aa94d3fe2b8beaaf441093a18c1`
- 接纳函数证据：`ev_6228d5daa98a640bf83594b507d1ca5a22af12a6d3e31f0474d77867446620e9`

## Recovered predicate

根节点接纳函数只有一条条件：

```text
return node.type == 4
```

结合规则语义、附件示例及执行函数使用方式，类型 `4` 为 function 根节点。执行函数遍历规则对象保存的每个已接纳根节点，对其调用递归 AST 查询，查询键固定为 `0x56`。相邻赋值规则的交叉证据和附件示例共同确定 `0x56` 为 `ASSIGN_LE`，即 Verilog/SystemVerilog 非阻塞赋值。查询得到的每个节点都会独立生成一条告警。报告从命中节点 `+0x1e0` 读取源文件、从 `+0x210` 读取行号，严重级别字面量为 `Warning`。`0x180ef6370` 的 GB18030 文本为 `禁止在function语句中使用非阻塞赋值.`。

因此核心判据是：**function 语句范围内出现任意非阻塞赋值即违规，每个非阻塞赋值节点分别报告。** 原生函数没有附加目标名称、位宽、时钟、组合/时序上下文或右值表达式限制。

## 产品映射

产品使用独立的 function 词法范围适配器：

- 只进入完整的 `function ... endfunction`；
- 排除 task、always 和 function 外部赋值；
- 区分非阻塞赋值 `<=` 与关系比较 `<=`；
- 支持条件分支和循环体中的多个赋值并逐个报告；
- 支持 class method 与 package function 的完整函数体；
- 排除注释、字符串、return 比较、for 条件和阻塞赋值右值中的比较；
- extern 与 pure virtual function 原型没有可执行函数体，不产生告警。

附件错误示例和正确示例均与预期一致。额外边界测试覆盖三次嵌套命中、一个赋值右值中的关系比较只计一次、class/package 函数、两分支多命中、for 条件、选择目标、task/always 排除、注释/字符串和未闭合 function。

## 未验证边界

宏生成的 function、include 来源映射、class/package 的完整厂商前端展开、语法损坏后的恢复方式和生产者排序仍未完全验证。已恢复的消费者逐节点报告且没有去重分支；产品仅把具体命中自动报告为违规，没有告警时仍输出 `ManualReviewRequired`，不声明完全合规。
