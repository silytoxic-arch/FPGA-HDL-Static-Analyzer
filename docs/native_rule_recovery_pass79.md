# 第七十九轮：A-5-3 内部时序信号作为异步复位

本轮只深化规则判断逻辑，不调整界面和表格。软件对外编号继续使用官方三段式
`A-5-3`，内部兼容键为 `GJB10157.6.2.8`。

## 已观察的原生事实

- 构造函数 `0x180022900` 注册规则并清零对象 `+0x158`；slot 0 为
  `FUN_1806f8890`，slot 1 包装函数 `FUN_180564050` 调用 `FUN_1804e6df0`。
- selector 仅接纳 `AST_ALWAYS(0x52)`，且直接子节点必须存在
  `AST_POSEDGE/AST_NEGEDGE(0x65/0x66)`。
- 主函数递归收集 `AST_ASSIGN_EQ/LE(0x55/0x56)`，将归一化目标名与
  异步复位关系名逐一比较；每个匹配赋值节点独立构造报告，不按信号名去重。
- 默认正文精确为 `异步复位<归一化目标名>由时序逻辑驱动`，级别为 `Warning`。
- 定位使用匹配赋值 AST 的源文件和物理行，原生报告不含列号。

## 产品接入

- 在同一 Verilog/SystemVerilog 模块内，使用解析器已确认的异步复位/置位关系，
  与带边沿 `always/always_ff` 中的简单赋值目标逐节点匹配。
- 匹配赋值逐节点报告，支持 blocking 和 nonblocking 赋值，输出精确原生正文、
  `Warning`、物理行和第 1 列映射。
- 括号表达式中的 `<=` 不按非阻塞赋值处理，避免把比较式误报为
  `AST_ASSIGN_LE`。
- 附件时序内部复位正例命中；组合生成复位、外部复位及仅比较不写复位的边界例不命中。

## 未恢复边界

异步复位关系的完整原生生产图及关系重数尚未恢复。附件文字提到锁存器，但已恢复
selector 不接纳无边沿 `always`，因此未猜测锁存器路径。层次传播、库原语、宏/
generate、别名、复合左值、转义名归一化和全局顺序仍为未知。清洁结果继续保持
`ManualReviewRequired`，完整专有前端等价仍为 `0/209`。

静态摘录见 `reverse-tools/evidence/t_linter_pass79/a5_3_internal_reset.txt`，详细判据见
`docs/evidence/native_verilog_internal_sequential_reset_6_2_8_findings.md`。
