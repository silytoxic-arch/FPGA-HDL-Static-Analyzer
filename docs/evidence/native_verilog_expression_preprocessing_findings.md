# GJB10157 Verilog 表达式规则条件编译边界

## 范围

本记录覆盖 `GJB10157.8.4`、`.8.5`、`.9.2.2`、`.9.2.4`、`.9.2.8` 和
`.9.2.9`。附件 `T-Linter.dll` SHA-256：
`60e6b52875f4d7c3fbdf494d869b3a4bcc3fdc8ee63d8620ea9614da5d6599b0`。

## 二进制直接观察

- `.8.4` 仅接纳 AST_LT/LE/GE/GT，再递归检查关系子树中的 X/Z 常量；证据
  `ev_5e17a6c6e87602b5fa6286d7a26085bc1b82d4462303d398c15d665cadd70b2d`。
- `.8.5` 从 AST_ALWAYS/AST_ASSIGN 根收集六类算术节点，再检查其常量后代；证据
  `ev_869231e3e00357939a5cde478f93d051e876601f110406d8c9083277445d45eb`。
- `.9.2.2` 的 case 控制表达式首节点为 AST_CONSTANT/REALVALUE 时报告；证据
  `ev_3a2e24112630365fcae32ee1e22794a35ebca760e7d8249ca16fa8cc55f319b2`。
- `.9.2.4` 在普通 case 控制子树中搜索含 X/Z 编码的 AST_CONSTANT，并跳过
  AST_CONDX/CONDZ；证据
  `ev_4a20a076c4f9078f61bbc0534bfbea9293c29c92b085f8633a6ce8fa77c39631`。
- `.9.2.8` 由 AST_CONDX/CONDZ case 节点触发；证据
  `ev_e69cc1c5a34d9cccdde7754f61613b89544679ed77ae679a1ee94a84b9dff82c`。
- `.9.2.9` 只检查 default 分支赋值子树内的 X，不把控制条件或任务实参中的 X
  当作违规；证据
  `ev_8874f9a800dc5a78dd52b5e1ca936091c78aa6f519187a241c03fd329b381181`。

这些入口均消费前端生成的表达式或 case 节点，而不是未经条件选择的源文本。

## V1.4.16 产品适配

此前六个有界表达式适配器在原始 token 流上执行，导致未激活的 `ifdef/else` 分支也可能
触发。V1.4.16 改为仅在工程 `defines` 预处理成功后，对活动 token 流执行：

- 活动分支中的六种确定违规分别正常报告；
- 未激活分支中的相同文本不产生报告；
- 诊断继续保留原始物理行号；
- VHDL 的 `GJB9765.4.4.c` 表达式检查路径不受本次 Verilog 修正影响；
- 预处理失败时不猜测条件分支。

## 仍未知

- 厂商完整宏、include、generate、elaboration 和常量折叠；
- X/Z 经参数、函数或标识符传播后的完整语义；
- 前端错误恢复、waiver、诊断排序及全部 SystemVerilog 表达式语法。

因此确定匹配可以自动报告；无匹配继续保持 `ManualReviewRequired`，不声明完整厂商算法等价。
