# GJB9765.4.8.d VHDL 组合 IF/ELSE 规则恢复记录

## 目标与二进制身份

- 目标规则：`GJB9765.4.8.d`
- 附件版本：T-Linter V3.1.4.25120520251205115359
- `T-Linter.dll` SHA-256：`60e6b52875f4d7c3fbdf494d869b3a4bcc3fdc8ee63d8620ea9614da5d6599b0`
- 构造函数：`FUN_180029870`（`0x180029870`）
- slot 0：`FUN_18077ad60`（`0x18077ad60`）
- 过程遍历器：`FUN_18077c840`（`0x18077c840`）
- IF 分支检查器：`FUN_18077b8e0`（`0x18077b8e0`）
- 本记录来自 Ghidra 12.1.4 对用户附件的离线静态反编译，不代表观察了原软件运行。

## Recovered predicate

- 构造函数先把 `+0x158..+0x15e` 七个选项字节初始化为一，随后以四字节写把 `+0x158` 保留为一、`+0x159..+0x15b` 清零，并把 `+0x15c..+0x15e` 清零。因此该规则默认只启用 IF 缺少最终 ELSE 的路径。
- `FUN_18077ad60` 遍历规则保存的设计根；`FUN_18077c840` 遇到类型 `0x52` 的过程节点时调用过程分类器。分类记录为 1、2 或 4 时设置事件状态并停止分析该过程；没有这些记录时才递归检查过程体。附件的时钟进程因此不进入本规则，而异步组合进程进入。
- `FUN_18077c840` 对所有非 `AST_ALWAYS(0x52)` 节点都在完成本节点处理后无条件递归全部直接子节点，没有针对 `AST_GENIF`、`AST_GENCASE` 或其他 generate 容器的停止分支。因此 generate 根本身不是本条候选，但其子树中的组合 process 仍会到达并按相同过程分类器检查。
- `FUN_18077b8e0` 识别直接分支类型 `0x58`、`0xab`、`0xac`。由控制流与附件语法可分别对应当前 IF 分支、ELSIF 分支和最终 ELSE 分支。
- 全 DLL `+0x183` 位移扫描只有 61 个指令命中。置位者 `FUN_1801c7b80` 先对原始 `AST_IF(0xaa)` 调用 `FUN_1808db990` 并把返回的来源句柄写入 `+0x188`，随后置 `+0x183=1`、把节点改写为 `AST_CASE(0x57)`，再整理其 `AST_ELSIF(0xab)` 与 `AST_ELSE(0xac)` 子树。构造器 `FUN_1808d8dc0` 将 `+0x183/+0x184` 和 `+0x188` 初始化为零，复制器 `FUN_1808d9a90` 保留这些字段。
- `FUN_18077c840` 在所有非 `AST_ALWAYS(0x52)` 节点上检查 `+0x183 != 0 && +0x188 != 0`，满足时调用 `FUN_18077b8e0`；该路径不要求祖先是 process。因此由同一前端 lowering 形成的 architecture/package-body procedure/function 顺序 IF 也进入规则，而不是仅检查 process 内 IF。
- 遍历到 `0xac` 时记录“存在最终 ELSE”；完成当前 IF 后，如果未观察到 `0xac` 且选项 `+0x158` 为一，就在该 IF 节点报告。
- `0x180f1d698` 的 GB18030 文本为“if语句没有else分支，可能产生锁存器”。同一区域还有逐分支缺少赋值、CASE 缺少 OTHERS、条件赋值缺少 ELSE 等诊断，但它们由 `+0x159..+0x15e` 控制，在本规则默认构造结果中关闭。
- 因此核心谓词是“无事件控制的组合进程内，每个顺序 IF/ELSIF 链必须有最终 ELSE”，不是“所有推断锁存器”，也不因 IF 前已有默认赋值而豁免。

## 产品接入

- 移除 `RTL-LATCH-INFERRED -> GJB9765.4.8.d` 的宽泛映射，避免把缺少 OTHERS 的 CASE、其他不完全赋值路径误报成该条款。
- 新增 VHDL 组合进程专用检查：为每个可配对的顺序 IF 单独跟踪最终 ELSE，嵌套 IF 独立判断。
- 进程中出现 `rising_edge`、`falling_edge` 或 `'event` 时，整个进程排除；时钟使能 IF 不映射为本条款。
- 条件信号赋值中的 `when ... else` 不会满足外层 IF 的最终 ELSE。
- 报告定位到缺少 ELSE 的 IF 关键字，而不是泛化到过程起始位置或每个可能保持的信号。
- IF-generate 根明确排除；generate 分支内的组合 process 仍检查，分支内缺少最终 ELSE 的顺序 IF 仍报告。
- architecture 声明区 procedure 和 package body procedure/function 的完整顺序 IF 也按最终 ELSE 判断；支持显式 `end procedure/function`、命名结束和裸 `end;`，包括字符串 designator 的运算符函数以及声明区嵌套子程序体。

## 已知边界

- 原生过程分类记录 1/2/4 的精确枚举名仍未恢复；`+0x183` 已闭合为来源 IF lowering 标志，`+0x188` 已闭合为 lowering 前通过 `FUN_1808db990` 取得的来源句柄，但原始 C++ 成员名仍未知。
- 严重语法错误、前端错误恢复节点、所有扩展标识符结束形式、嵌套 package/generic subprogram 特殊形态以及原生报告顺序尚未完整恢复。
- 产品仅对可配对的顺序 IF/ELSIF/ELSE token 结构作肯定判断；无法配对的结构不报告，不伪判合规。
- 非默认运行时选项若重新开启逐分支、CASE 或条件赋值检查，不在本次等价范围内。

## 回归覆盖

`tests/standard_accuracy_tests.cpp` 覆盖：先默认赋值但 IF 缺 ELSE、嵌套 IF 独立判断、条件赋值 ELSE 不冒充 IF ELSE、时钟进程排除、完整 IF/ELSIF/ELSE 遵循例、CASE 缺 OTHERS 不误映射到本条款、IF-generate 根排除、generate 分支内组合 process、architecture 声明区 procedure、package body function 的违反/遵循和命名/裸结束形式、字符串 designator 运算符函数，以及内外层子程序 IF 独立报告。附件正反例继续分别命中和保持无报告。

## 新增原始证据

- `reverse-tools/evidence/remaining_rules/vhdl_4_8_d_entry_traversal_and_if.c`：入口、递归过程遍历器与 IF 检查器的完整静态反编译，SHA-256 `c41b6c032dcd6ca0b1ecabc7a0ebd2320ba39f6db3ac3dfca663c19ba3223d9b`。
- `reverse-tools/evidence/remaining_rules/field_188_all.txt`：全 DLL 位移 `0x188` 的 2,043 个立即数指令命中原始清单，SHA-256 `838cbfdffdf573d6fcc829849ea897dd8fb4c17d8c8436f7053b6a21be1e6bb4`。其中包含大量栈偏移及无关对象访问，尚不足以给 AST `+0x188` 字段命名，不作为扩大规则范围的依据。
- `reverse-tools/evidence/remaining_rules/field_183_all.txt`：全 DLL 位移 `0x183` 的 61 个指令命中，SHA-256 `1f101ca6079ed8156dfd39a801f16de2b5a22be9db6d48894c9ca982ad9552a2`。
- `reverse-tools/evidence/remaining_rules/field_183_writers_and_metadata.c`：`+0x183` 置位/初始化/复制函数及与 `+0x188` 相邻生产逻辑的完整反编译，SHA-256 `f83b1a90df4df99fdf7241cda6f59af8a212f5f067340bf57d6a4044d25838e5`。
