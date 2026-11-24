# 第八十一轮：R-6-1 异步 IF 赋值覆盖

本轮只深化规则判断逻辑，不调整界面和表格。软件对外编号继续使用官方三段式
`R-6-1`，内部兼容键为 `GJB10157.7.1.1`。

## 已观察的原生事实

- 静态注册包装器 `0x180022a50` 构造规则对象；vtable slot 0 为
  `FUN_18055e770`，slot 2 只接纳 `AST_MODULE(2)`。
- 主函数遍历综合关系、`AST_ALWAYS(0x52)` 和
  `AST_POSEDGE/AST_NEGEDGE(0x65/0x66)`，命中过程关系后调用
  `FUN_18055e020`。
- `FUN_180560070` 定位 IF 降低树的最终 `AST_DEFAULT`，并沿同物理行的嵌套
  `AST_CASE(0x57)` 跟随 else-if 链。
- `FUN_18055fd10` 收集最终 default 分支内的直接标识符
  `AST_ASSIGN_EQ/LE(0x55/0x56)` 目标；`FUN_18055f9a0` 收集物理行不晚于最终
  default 行的此前分支目标。
- 每个存在于最终 default、但不存在于此前目标表的目标独立报告；此前目标表为空时
  仍对全部 default 目标报告。
- 原生级别固定为 `error`，正文精确为
  `always语句中混合使用有复位和无复位的触发器：触发器“<名称>”未进行复位`。
- 文件取当前设计记录，行号取缺失目标在最终 default 分支的赋值物理行，不含列号。

## 产品接入

- 以显式边沿事件和首层简单复位 IF 作为未恢复综合关系生产器的有界代理。
- 按源代码 IF/ELSE-IF 链收集最终 ELSE 前的目标表，再与最终 ELSE 目标表比较。
- 以 `else` 物理行作为原生 `AST_DEFAULT` 行代理；同行最终分支赋值也进入此前目标
  表，复现原生 `赋值行 <= default行` 截止行为。
- 复位分支为空时逐目标报告，不再错误跳过。
- 每个缺失简单目标输出精确原生 `Error` 正文、赋值物理行和产品第 1 列。
- 新增精确正文/级别/位置、多个 else-if 分支、空复位分支逐目标重数、同行截止以及
  嵌套同步数据条件完整覆盖回归。

## 未恢复边界

完整综合关系生产、位/范围目标归一化、别名与层次传播、库原语、宏、generate、复合
复位条件、专有异常 AST 降低和全局顺序仍未知。清洁结果继续保持
`ManualReviewRequired`，完整专有前端等价仍为 `0/209`。

静态摘录见
`reverse-tools/evidence/t_linter_pass81/r6_1_async_if_assignment_coverage.txt`，详细判据见
`docs/evidence/native_verilog_flipflop_reset_coverage_7_1_1_7_1_2_findings.md`。
