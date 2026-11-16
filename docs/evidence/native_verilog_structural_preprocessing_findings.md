# GJB10157 Verilog 结构规则条件编译边界

## 范围

本记录覆盖 `GJB10157.9.1.5`、`.9.2.5`、`.9.2.6`、`.9.3.5`、`.9.3.6`、
`.9.4.3` 和 `.9.4.10`。附件 `T-Linter.dll` SHA-256：
`60e6b52875f4d7c3fbdf494d869b3a4bcc3fdc8ee63d8620ea9614da5d6599b0`。

## 二进制直接观察

- `.9.1.5` 从 AST_MODULE 派生的赋值模型读取 `is_override_assign` 属性并逐记录报告；证据
  `ev_082bb18f097147f33d9c4ea8bfbc4fd286a2532293060392b59ec6f1102d1292`。
- `.9.2.5` 收集 AST_ALWAYS，并在事件/边沿子树包含两种 OR 运算节点时每个 always 报告一次；
  证据 `ev_165fc19c9ef8d627b96bd5d6a92697bc84caaaae45cfcbaa1c0f8d3e2d47a00e`。
- `.9.2.6` 检查 case/casex/casez 分支的标签节点类型；产品只自动报告可证明依赖运行时变量
  的标签。证据 `ev_95102f809deb39475a14e001a1aa069a17fbeafdfff5d1337a06af1641c9cb40`。
- `.9.3.5` 建立 FUNCTION/FCALL 名称图并检测闭环；证据
  `ev_bb439465834fb0b8f08945cbaf0d4a18e1d519768494816ed04d3b6641d60df9`。
- `.9.3.6` 从 repeat 计数子树收集标识符，并与参数/localparam 表比较；证据
  `ev_cc1cc2326f3fb89f862a679ec1a028a505bd07da523173fd2d6fa1f2caa9d538`。
  后续已恢复逐 identifier 粒度、精确名称匹配、固定消息与严重度，详见
  `docs/evidence/native_verilog_repeat_count_9_3_6_findings.md`。
- `.9.4.3` 只接纳 FUNCTION 根，并递归报告其 AST_ASSIGN_LE 后代；证据
  `ev_6228d5daa98a640bf83594b507d1ca5a22af12a6d3e31f0474d77867446620e9`。
- `.9.4.10` 对 MEMRD/MEMWR 索引根限制可接纳的索引节点类型；产品报告表达式位置中可证明的
  运算或调用索引。证据 `ev_b8626bfc2750926780bcaa2a281cd2e99f4662b6069f347b81a555de2957b066`。

这些观察共同表明规则输入是前端解析后的结构，不是未经条件选择的原始文本。

## V1.4.14 产品适配

产品此前在预处理流程之后又对原始 token 运行这七个有界适配器，会产生两个偏差：活动宏
分支可能被适配器跳过，非活动分支中的结构也可能进入判断。

V1.4.14 改为仅在工程 `defines` 预处理成功后，对活动 token 流运行七个适配器：

- 活动分支中的确定违规正常报告；
- 只存在于非活动分支的结构不产生报告；
- 每条诊断仍映射到原始物理源文件行列；
- 预处理失败时不猜测结构。

## 仍未知

- 完整参数宏、include、generate 与 elaboration 后生成的节点；
- 厂商赋值模型、常量折叠、符号解析、错误恢复和诊断排序；
- 各规则现有有界源码适配器之外的完整 SystemVerilog 语法。

因此匹配结果可判定为违规；无匹配仍保持 `ManualReviewRequired`。
