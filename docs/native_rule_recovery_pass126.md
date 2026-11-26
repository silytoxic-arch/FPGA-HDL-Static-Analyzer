# 第一百二十六轮：A-8-1 的真实 VHDL 节点来源

目标 T-Linter.dll，SHA-256
`60e6b52875f4d7c3fbdf494d869b3a4bcc3fdc8ee63d8620ea9614da5d6599b0`。
只读静态分析，不运行目标。完整输出、分析脚本和逐文件摘要保留在
`reverse-tools/evidence/t_linter_pass126/manifest.json`。

## 从语法上下文闭合到收集谓词

FindListenerContextOffsets.java 枚举 257 个具名 VHDL Context 的 vtable
+0x80/+0x88 回调，以直接指令位移匹配定位监听器槽；这是有界静态扫描，
不代表所有间接回调均已覆盖。随后 TraceListenerContext 逐项反编译委托和实际回调。

| 语法上下文 | Context vtable | listener 槽 | 实际进入回调 |
|---|---|---|---|
| Literal | 0x180e65db8 | +0x908 | 0x1801baf20 |
| Subprogram_body | 0x180e6a0e8 | +0xe78 | 0x1801c1de0 |
| Subprogram_declaration | 0x180e6a1b0 | +0xe88 | 0x1801c20d0 |
| Sequential_statement | 0x180e69080 | +0xd28 | 0x1801bfbf0（上一轮留档） |
| Sensitivity_list | 0x180e68ef0 | +0xd08 | 0x1801bfa20 |
| Loop_statement | 0x180e660d8 | +0x948 | 0x1801bb5c0 |
| Case_statement | 0x180e60e08 | +0x298 | 0x1801b0440 |
| Case_statement_alternative | 0x180e60ed0 | +0x2a8 | 0x1801b0600 |
| Condition | 0x180e618f8 | +0x378 | 0x1801b1d20 |

Literal 进入回调的 NULL 路径创建 type 0xb0。固定名称取 0x180e81424 的
`4e554c4c`，即 `NULL`；PE 读取前验证目标摘要，字节记录在 literal_name.json。
这与子程序构造时暂用 0xb0、随后可重标类型的路径不同。

顺序语句解析器 FUN_18018e770 的分支 0xc 直接匹配可选标签、NULL 和分号，
没有进入 Literal Context。因此显式 `null;` 留下的是空的 Sequential BLOCK；
表达式、初始化和条件中的 NULL 则可创建 0xb0。两者都由规则收集，但来源不同。

## 所有权与位置修复

Process 解析器 FUN_18018bee0 先处理可选标签及 postponed，再匹配 process。
进入回调 FUN_1801bde50 为 ALWAYS/BLOCK 使用 Context 起始行。
Sequential 回调也使用 Context 起始行，所以跨行 `label: null;` 应定位标签行。
空进程定位标签/postponed/process 起点，不能统一定位 begin。

新适配器按完整的 `end [postponed] process [label];` 限定范围；不再把结束关键字
中的 process 当成新进程。声明区属于这个范围：NULL 字面值即使在 begin 前也要收集。
空根判断只在可选敏感表及 is 后直接出现 begin/end 时成立；已有声明不能忽略。
Sensitivity 回调可追加一个 type 0x67 包含节点；规则特殊分支覆盖只有该节点的空根。

## 撤回先前空 CASE/LOOP/ELSE 外观推断

CASE 进入生成 0x57；alternative 进入生成 0x58；退出均只弹栈。
Alternative 解析器 FUN_180173320 调用通用 Sequence_of_statements
FUN_18018e600，其 Context 构造器 FUN_180150030 对应已恢复的通用语句序列；
该序列监听器是空操作。空 alternative 不是自动生成一个空 0x54 块。

Loop 解析器 FUN_180188510 同样调用通用序列。进入创建循环类别节点，
退出不构造 BLOCK。外层 Sequential BLOCK 仍含循环子节点，因此不是空块。
循环中真实存在的 `null;` 则仍留下独立空 Sequential BLOCK，应当报告。

普通 ELSE 的 0xac 容器也不因自身为空就满足 0x54 谓词。不能把标准的文字意图
或相邻关键字，直接当作原工具实际的节点构造。移除这些无依据的源码外观报警；
无诊断继续 ManualReviewRequired，不转为合规。

## IF 重建与原生未知位置

Condition 回调创建 0x58 并记录条件起始行。IF 退出 FUN_1801c7b80 把初始条件
改为表达式节点，将直接 Sequential BLOCK/赋值子节点移入新建的真分支 BLOCK，
再把该 BLOCK 行号设为初始 Condition 行。因此空初始真分支采用条件起点，
不采用 if 或 then 的固定位置。

ELSIF 重建 FUN_1801ad560 给新真分支 BLOCK 赋行号的语句位于“搬移实际语句”
分支内。真分支为空时保留构造器 FUN_1808d8dc0 的 +0x214 初值 0。
复制函数 FUN_1808d9a90 明确执行 `param_1[0x85] = param_2[0x85]`，深克隆
FUN_1808db990 调用该复制函数，所以不会用全局行号覆盖这个 0。
产品保留 0 表示原生未知行；固定列号缺省值仍为 1，不制造猜测位置。
多层复杂 ELSIF/ELSE 重建仍未完整模拟，不能推广为全部前端等价。

## 检查使用的是保留的 VHDL 树

VHDL 模块构造 FUN_1808e81d0 先调用 FUN_180623ac0 收集名称映射，再克隆树。
普通 param_3 == 0 分支调用 FUN_1809144f0 设置当前 design；其实现仅赋全局指针。
该普通路径没有执行 Verilog 简化器，随后把克隆保存到模块 +0x2e0。
preclone_normalization、retained_vhdl_tree、current_design_setter 和 clone 输出
一起保留，避免把 Verilog 的折叠/降级行为错误套用到这些 VHDL 节点。

## 接入与验证边界

新增 src/analysis/vhdl_empty_statement.h，将本规则从全文件状态扫描替换为
完整结束符限定的进程范围；接入 NULL 字面值、标签和已恢复的节点位置。
新增 20 组节点来源/所有权回归，更新 CASE/LOOP 的负例及普通 ELSE 控制，
覆盖重复计数、声明区、聚合、条件、局部子程序、多个进程、注释/字符串及未闭合进程。
这些测试验证产品接入，并非运行原工具的差分 oracle。

完整算法验证仍为 0/209。错误恢复、simultaneous 语句、完整类型/表达式绑定、
多层重建及全部工程来源仍未闭环；没有诊断继续人工复核。
Windows 软件仅在 GitHub Actions 构建、验证和保存，本地无 Windows 安装包。

## 原生未知位置的可用输出

行号 0 保留在分析结果及 JSON 中；界面显示“行号待确认”，不把第一行高亮成命中位置。
SARIF 对未知行仅保留文件位置，不输出虚构 region；已知行但未知列也不输出 0 列。
新增两个报告回归验证诊断和文件仍保留、已知行定位仍有效。
正整数行/列要求依据 [OASIS SARIF 2.1.0 §3.30.5–6](https://docs.oasis-open.org/sarif/sarif/v2.1.0/os/sarif-v2.1.0-os.html)。
