# R-2-4（内部 GJB10157.5.2.4）原生恢复记录

公开界面与报告使用官方三段式编号 `R-2-4`；内部条款键只用于实现、测试和证据关联。

## Recovered predicate

- 构造函数 `FUN_180021820` 注册内部规则并安装虚表 `0x180eeda08`，活动槽为 `FUN_180539910`。
- 活动函数遍历对象 `+0x68` 的有序模块名映射；仅当同名组恰有一个非空 `AST_MODULE(2)` 根时继续。
- 递归筛选 `MODULE(2)`、`WIRE(6)`、`PARAMETER(9)`、`LOCALPARAM(10)`、`CELL(79)`、`TASK(3)`；排除 `FUNCTION(4)` 本身，但函数内部被选中的声明后代仍可进入。
- 固定集合为 `VDD`、`VSS`、`VCC`、`GND`、`VREF`。`FUN_1804d77f0` 对集合项逐字节调用 C `tolower` 后插入；`FUN_1804c5730` 对候选副本做相同转换；`FUN_1804bef20` 使用最短长度 `memcmp` 加长度相等判断。因此是折叠大小写后的精确匹配，不是前缀或子串匹配。
- 每个命中节点独立输出固定 `Error`，报文精确为 `禁止使用<原始归一名称>命名`。名称比较使用副本，报文保留原始归一名称的大小写。
- 源文件来自唯一模块根，报告整数取自候选节点 `+0x210`，不写列字段。

## 产品适配边界

产品对已证明的 ASCII 范围实现确定比较；非 ASCII 名称继续返回未知，不用未恢复的 Windows C 区域设置行为伪判。源码适配器采用声明物理行并以第 1 列承载原生“无列”定位；不宣称专有 AST `+0x210` 字段生产过程已完整等价。

## 证据与完整性

- `T-Linter.dll` SHA-256：`60e6b52875f4d7c3fbdf494d869b3a4bcc3fdc8ee63d8620ea9614da5d6599b0`
- 构造函数与活动函数：`reverse-tools/evidence/t_linter_pass56/r2_4_decompile.txt`，SHA-256 `8f264bd1817e5dd4a05bc64d4b73f08372e246f74f65ffeded6a6e34d0de099c`
- 名称归一与候选比较：`reverse-tools/evidence/t_linter_pass56/r2_4_helpers.txt`，SHA-256 `4bc26b098fd8a4290f9a4e7e7eb0f1d79b70d4e0434fa91bd6381f0b5207bc34`
- 集合项归一与等长比较：`reverse-tools/evidence/t_linter_pass56/r2_4_list_helper.txt`，SHA-256 `060779d17104509a7041914cbbd8618c46a5447f00e1a89a4526358c99738422`
- 固定名称和 GB18030 报文片段：`reverse-tools/evidence/t_linter_pass56/r2_4_strings.txt`，SHA-256 `ad315b5cef2a0aa99f53a5c464e8d5ddb8ffc6a5715811d8ce48083b509bda83`

## 尚未闭合

跨文件重复模块组、完整宏/include/generated AST 构造、转义及非 ASCII 区域设置、不透明类型、复杂 cell、语法恢复、专有位置字段生产和全局报告顺序仍未证明。无报警继续标记 `ManualReviewRequired`，完整原生等价不作声明。
