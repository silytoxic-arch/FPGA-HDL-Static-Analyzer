# R-2-2（内部 GJB10157.5.2.2）原生恢复记录

公开界面与报告使用官方三段式编号 `R-2-2`；内部条款键只用于实现、测试和证据关联。

## Recovered predicate

- 构造函数 `FUN_180021700` 注册内部规则并安装虚表 `0x180eec8f0`，活动槽为 `FUN_180535370`。
- 活动函数遍历对象 `+0x68` 的有序模块名映射；仅当同名组恰有一个非空 `AST_MODULE(2)` 根时才继续。
- 递归筛选 `MODULE(2)`、`WIRE(6)`、`PARAMETER(9)`、`LOCALPARAM(10)`、`CELL(79)`、`TASK(3)`；排除 `FUNCTION(4)` 本身，但函数内部被选中的声明后代仍可进入。
- 非空归一名称先搜索 `__`。命中时只输出 `<名称>:禁止命名中使用连续的"__"`，不再执行末尾分支。
- 仅当没有 `__` 时，最后一个字节为 `_` 才输出 `<名称>:禁止命名末尾使用"_"`。
- 每个被选节点独立输出固定 `Error`。源文件来自唯一模块根；报告不写列字段。

## 位置边界

活动函数把候选节点 `+0x200` 的 32 位整数写入报告行字段，相邻前端代码还使用节点 `+0x210`，但现有附件证据不足以可靠命名两者各自语义。产品适配器因此采用已解析声明的物理行，并以第 1 列承载原生“无列”定位；不宣称该源码字段与专有 AST 字段完全等价。

## 证据与完整性

- `T-Linter.dll` SHA-256：`60e6b52875f4d7c3fbdf494d869b3a4bcc3fdc8ee63d8620ea9614da5d6599b0`
- 活动函数反编译：`reverse-tools/evidence/t_linter_pass55/r2_2_decompile.txt`，SHA-256 `ff6dc3164fc540598d0eddccebdb3dd701b75bdebd2d57cb9f7975ffa0dfb3f4`
- 固定字符串提取：`reverse-tools/evidence/t_linter_pass55/r2_2_strings.txt`，SHA-256 `fe61573848884b2c35299736aefb2faefa43cd4bcbc12269f2d5683341ecb597`
- 位置写入追踪：`reverse-tools/evidence/t_linter_pass55/r2_2_location_writers.txt`，SHA-256 `d299cbc1f175ee432ecaccf6595ec634ef20b2da182a9fb0848b41f947273c9b`；其结果只支持上述有界结论，不支持进一步命名 `+0x200`。

## 尚未闭合

跨文件重复模块组、完整宏/include/generated AST 构造、转义及非 ASCII 归一化、不透明类型、复杂 cell、语法恢复与全局报告顺序仍未证明。无报警继续标记 `ManualReviewRequired`，完整原生等价不作声明。
