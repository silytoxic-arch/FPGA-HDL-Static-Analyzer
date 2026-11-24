# 第一百轮：R-6-3 复位候选目录来源

目标 SHA-256：`60e6b52875f4d7c3fbdf494d869b3a4bcc3fdc8ee63d8620ea9614da5d6599b0`。
静态分析，未执行目标。摘录在 `reverse-tools/evidence/t_linter_pass100/`，保存时
规范化末尾空白。

## 原生证据

- DAT_18136fd70/78 是规则读取的模块指针向量。交叉引用将生产者定位到
  runRulesPrepare(0x180627340) 与 runRulesPhase2Prepare(0x1806260e0)。
- runRulesPrepare 依次调用 proc_clean、proc_rmdead、proc_prune、proc_init、
  proc_arst。之后按选定模块名称取对象，读取模块 +0x2f3 标志，调用虚方法 +0x58，
  将返回指针追加到目录。此处传给 proc_arst 的字符串没有 global_arst 选项。
- RTTI 标记 Yosys::RTLIL::Module 的实际虚表为 0x180f952b0，+0x58 绑定
  FUN_1809b7fd0。后者分配对象、调用构造器，复制 +0x130 名称字段，再调用
  FUN_1809b8500 复制模块内容。这建立了第一阶段模块快照来源。
  基类目标不是全部动态派发的证明；派生 AstModule 的覆盖入口见第一百零六轮修正。
- 第二阶段准备函数将模块指针直接追加到同一目录；不能将两个阶段都描述为深拷贝。
- R-6-3 按模块名匹配目录，再遍历过程记录 +0x100 的同步记录向量。
  FUN_1804dee00 仅接受同步记录首字节 <2 的项，再格式化其 +8 信号并建立名称表。
  AST 中的事件名称必须先存在于该表，才写入本过程的极性映射。
- ProcArstPass 实际虚表 +0x18 绑定 FUN_180288920；它对过程调用
  FUN_18028e760，后续第一百零一轮检查该函数的单边沿与多边沿分支。

## 解释边界

公开 [Yosys 0.9 RTLIL 定义](https://github.com/YosysHQ/yosys/blob/yosys-0.9/kernel/rtlil.h)
给出低/高电平同步值 0/1、正/负边沿值 2/3；这是类型语义的辅助对应。
[公开 proc_arst](https://github.com/YosysHQ/yosys/blob/yosys-0.9/passes/proc/proc_arst.cc)
说明异步复位转换用途。目标中的实际实现更复杂，不能以此公开版本替代原生代码，
也不能宣称二者完整算法等价。

模块目录生成时点、虚派发、值 <2 的规则入口已恢复。完整名称格式、别名、信号
选择范围、层次和转换后的全部过程关系仍待恢复。
