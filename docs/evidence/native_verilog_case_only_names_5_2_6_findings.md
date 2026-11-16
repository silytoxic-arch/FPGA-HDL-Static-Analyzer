# R-2-6（内部 GJB10157.5.2.6）原生恢复记录

公开界面与所有报告只使用官方三段式编号 `R-2-6`；内部条款键只用于实现、测试和证据关联。

## Recovered predicate

- 构造函数 `FUN_1800219d0` 注册内部规则并安装虚表 `0x180eedc88`，活动槽为 `FUN_18053a3c0`。
- 活动函数遍历对象 `+0x68` 的有序模块名映射；仅当同名组恰有一个非空 `AST_MODULE(2)` 根时继续。
- 递归筛选 `MODULE(2)`、`WIRE(6)`、`PARAMETER(9)`、`LOCALPARAM(10)`、`CELL(79)` 和 `TASK(3)`。`FUNCTION(4)` 本身不在集合中，但函数内部被筛选类型的声明后代仍进入。
- 候选名称先经过专有归一；空名称跳过。每个后出现候选依次和先前候选比较：原始字节与长度完全相同则不报告；否则对两个副本逐字节调用 C `tolower`，折叠后等长字节相同即报告，并立即离开该后候选的先前候选循环。
- 因此每个后候选最多一条，但 `data`、`Data`、`DATA` 会在后两个候选处产生两条。不同模块组相互隔离。
- 固定严重度为 `Error`。源文件来自唯一模块根，位置取后候选节点 `+0x210`，不写列字段。精确正文为 `禁止仅使用大小写区分命名<后候选>和<先候选>`。

## 产品适配边界

产品在配置宏选择后的活动 token 流上建立有界声明集合，执行唯一模块组准入并以第 1 列承载原生“无列”定位。ASCII 名称遵循已证明比较；非 ASCII 的 Windows C 区域设置行为未恢复，不据此声明完整等价。

## 证据与完整性

- `T-Linter.dll` SHA-256：`60e6b52875f4d7c3fbdf494d869b3a4bcc3fdc8ee63d8620ea9614da5d6599b0`
- 构造函数、活动函数、集合、比较、诊断和位置：`reverse-tools/evidence/t_linter_pass58/r2_6_decompile.txt`，SHA-256 `141615a5c0ea6934247a60df32ebaa31a10330dd8971cbc0e3f081d719da624e`
- 三个字符串拼接 helper：`reverse-tools/evidence/t_linter_pass58/r2_6_helpers.txt`，SHA-256 `5072a6e9d06a3943aa8b1e66ed1a092ad17e0db86e2b1f4a8781524a611cf5d4`
- GB18030 固定正文、连接词与严重度：`reverse-tools/evidence/t_linter_pass58/r2_6_strings.txt`，SHA-256 `3955939dadd802c2bcff3da309a867d12ba4c033311fc4bab015fac94d491f61`

## 尚未闭合

完整宏/include/generated AST 构造、转义名称归一、非 ASCII 区域设置、不透明类型、复杂 cell、语法恢复、专有位置字段生产和全局报告顺序仍未证明。无报警继续标记 `ManualReviewRequired`，完整原生等价不作声明。
