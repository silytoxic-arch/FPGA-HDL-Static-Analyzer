# parser

V1.0 的 `preprocessor` 支持条件编译、配置宏和 include 查找；`design_parser` 提供
注释/字符串感知 token 化、module/entity 定义、端口、parameter/generic 声明提取，
Verilog 参数化实例和 VHDL direct entity/component 实例及 override 形状提取，并保留来源位置。后续仍需
补充预处理、完整语法树、generate elaboration 和外部库绑定。`checkSyntax` 已覆盖括号、
注释、字符串和主要 Verilog/SystemVerilog 结构边界。

`semantic_analyzer` 为 Verilog/SV always/continuous assignment 和 VHDL process/concurrent
assignment 建立保守驱动模型，并对过程分支/循环及运行时三目/when-else guard 中的文字、括号、
逻辑连接和文字比较执行可证明的布尔常量求值，同时检查 Verilog/SV 分支、循环及三目 guard
中的赋值/自增/自减副作用、空条件分支、空 case
选择项、两个 HDL 的重复 catch-all、直接/常量索引/切片 selector 的 case 项位宽与 Verilog
无尺寸 choice 值域、相同形状操作数的常量移位范围与负 literal 移位量、除法/余数运算中的 literal 零分母，以及普通
`always` 的直接挂起点；只对已声明端口/信号产生多驱动、输入被驱动和输出未驱动诊断。
直接赋值位宽检查同时覆盖已知宽度信号、显式宽 literal、常量 selected source/target，以及
成员宽度均可求值的 Verilog/SystemVerilog 与 VHDL 简单拼接，并支持常量次数的 Verilog
复制拼接以及 VHDL 字符串/标量逻辑 literal 成员；动态/参数化复制次数、动态选择及复杂表达式
仍等待类型系统扩展。
过程条件的比较位宽检查复用静态 selector、literal 与拼接形状模型，覆盖两个 HDL 的
等值、不等值和关系运算；只有两侧表达式都能完整求宽时才报告，Verilog 移位符号明确排除。
条件范围覆盖分支、两个 HDL 的 while guard、Verilog/SystemVerilog for 的中间表达式、
括号包围、过程/连续赋值右侧、链式嵌套或外层控制表达式内的三目 guard，以及 VHDL
when-else 条件表达式。
完整括号条件外的逻辑取反和 SystemVerilog wildcard equality 也保留同一宽度分析语义。
`timing_analyzer` 同时识别 VHDL edge 函数与 `signal'event` + `'1'/'0'` 电平模板；其 VHDL
architecture 摘要由 analysis 层跨文件合并回唯一 entity，再统一执行
时钟/复位一致性、CDC/RDC 与同步器认证。
