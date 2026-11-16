# GJB9765.4.8.c VHDL case others 规则恢复记录

## 目标与二进制身份

- 目标规则：`GJB9765.4.8.c`
- 附件版本：T-Linter V3.1.4.25120520251205115359
- `T-Linter.dll` SHA-256：`60e6b52875f4d7c3fbdf494d869b3a4bcc3fdc8ee63d8620ea9614da5d6599b0`
- 构造函数：`0x180029750`
- slot 0：`FUN_1806326d0`（`0x1806326d0`）
- case 收集器：`FUN_180632fc0`（`0x180632fc0`）
- choice 检查器：`FUN_180633070`（`0x180633070`）
- 本记录来自 Ghidra 12.1.4 对用户附件的离线静态反编译，不代表观察了原软件运行。

## Recovered predicate

- `FUN_1806326d0` 对每个设计根节点清空一棵按 case 节点地址排序的映射，调用 `FUN_180632fc0` 递归收集，然后对映射中标志仍为零的条目逐个报告。
- 入口实际遍历规则对象 `+0x78` 的有序输入记录，从每条记录的根向量取得首个非空根后直接调用 `FUN_180632fc0`，调用前没有 architecture 类型过滤。附件类型名函数同时明确包含 `AST_PACKAGE_BODY(0xa8)`。由此可确定收集器本身对设计根类型无偏好；只要 package body 根被上游输入记录接纳，其子程序 CASE 就沿同一递归路径进入本条。上游记录对所有异常 package 形态的准入仍是未知边界。
- `FUN_180632fc0` 遇到类型 `0x57` 且偏移 `+0x183/+0x184` 两个状态字节都为零的节点时，为该 case 插入标志零的条目，并调用 `FUN_180633070`；随后仍递归遍历所有子节点，因此嵌套 case 各自形成独立候选。
- 附件类型名函数把源码顺序 CASE 标为 `AST_CASE(0x57)`，把 case-generate 标为独立的 `AST_GENCASE(0x62)`。收集器只比较 `0x57`，所以 case-generate 根本身即使没有 `when others` 也不属于本条；递归遍历不会在该节点停止，generate 分支内的普通 `AST_CASE(0x57)` 仍会被检查。
- 收集器在插入 `AST_CASE(0x57)` 条目前没有“至少一个 choice/子节点”的数量条件。因此前端只要形成了成对的空 CASE 节点，它仍会因没有直接 `AST_OTHERS(0x89)` 而报告。严重语法错误若没有形成 CASE AST，是否进入规则输入仍取决于未恢复的前端错误恢复路径。
- 全 DLL 字段访问扫描得到 67 个立即数命中；排除栈偏移、32 位计数器和对象构造/复制后，`FUN_1801ad560` 是 `+0x184` 的关键 AST 生产者。它把 `AST_ELSIF(0xab)` 原地改写为 `AST_CASE(0x57)` 并立即写入 `+0x184=1`，随后把 `AST_ELSE(0xac)` 整理成该条件树的默认分支。由此可以确定该标志用于识别由 VHDL `if/elsif/else` 降低生成的 CASE 形态，而不是源码 case；`FUN_180632fc0` 要求它为零正是在排除这种伪 CASE。
- 相邻的 `+0x183` 已由 Verilog 条件 lowering 证据确认用于排除 Verilog `if/else` 生成的 `AST_CASE`。两个字节共同保证本条只检查源码 CASE，而不检查条件语句的内部统一表示。
- `FUN_180633070` 只遍历当前 case 的直接子节点：对于类型 `0x58` 的 choice 组，再检查其所有直接子节点；出现类型 `0x89` 时把当前 case 的标志置一。
- 由附件语法与诊断可将类型 `0x89` 确认为 `when others` 对应的 default 节点。内嵌 GB18030 诊断为“Case语句中缺少‘others’分支”。
- 原生算法没有统计显式 choice 数量、selector 位宽或枚举取值，也没有任何“已穷举则免除 others”的路径。因此即使 bit 的 `'0'` 和 `'1'` 都已列出，只要没有直接 `when others` 仍报告。
- 一个嵌套 case 的 `when others` 不会满足外层 case，反之亦然；报告位置取保存的 case 节点位置。

## 产品接入

- 现有 VHDL token 结构已经按 case 嵌套深度分别寻找直接 `when others`，与恢复的核心谓词一致。
- 源码级扫描只接纳显式 `case ... end case`，因此天然排除 VHDL `if/elsif/else`；新增无最终 ELSE 的 IF/ELSIF 反误报回归，验证不会把它误当成缺少 `when others` 的 case。
- package body 现在作为独立设计根扫描其子程序 CASE；完整 `end package body`、命名 `end <package>;` 和裸 `end;` 三种结束形式均有边界回归，裸结束不会吞入后续 architecture。
- case-generate 根明确排除；其分支内的普通顺序 CASE 仍检查。成对形成的空 CASE 仍按缺少 `when others` 报告。
- 保留“每个缺失 others 的 case 报告一次”，定位到 case 关键字。
- 不增加基于 bit、枚举或显式 choice 穷举的豁免。
- 注释中的 `when others` 不计入；内外层 case 互不借用 default。

## 已知边界

- `+0x183/+0x184` 已分别闭合到 Verilog 条件 lowering 与 VHDL IF/ELSIF lowering 的排除用途；字段在全部异常恢复路径中的写入仍未穷尽。
- 产品已经用回归确认 architecture 声明区 procedure、package body 子程序、case-generate 排除、generate 分支内普通 CASE，以及成对空 CASE。嵌套 package、未形成 AST 的严重语法错误、上游 package 根准入及原生遍历/诊断顺序仍可能与原生前端不同。
- 无报告仍不用于证明未覆盖的复杂前端形态合规；不宣称完整 AST 等价。

## 回归覆盖

`tests/standard_accuracy_tests.cpp` 覆盖 bit 取值显式穷举但缺少 others、嵌套 case 独立判断、注释伪分支排除、直接 others 遵循例、无 ELSE 的 IF/ELSIF 排除、architecture 声明区 procedure、package body 子程序的违反/遵循与三种结束形式、case-generate 排除、generate 分支内普通 CASE，以及成对空 CASE；附件正反例继续分别一报/零报。

## 新增原始证据

- `reverse-tools/evidence/remaining_rules/field_184_all.txt`：全 DLL `0x184` 指令命中清单，SHA-256 `ed3bdf3d65ecdf3155b8c2c025334bf95615e1819ff6a29e8e866a17db3b71ef`。
- `reverse-tools/evidence/remaining_rules/field_184_writers.c`：构造、setter、lowering 与复制候选反编译，SHA-256 `cdd861f7358b36ec3ea441586eaef3fd95b47977e178db2a3f40ca8ee2cc7b8e`。
- `reverse-tools/evidence/remaining_rules/vhdl_4_8_c_entry_and_collector.c`：入口、递归收集器和 choice 检查器完整反编译，SHA-256 `94e07b44bb10161eb32b5eebd6d5ea8ff7296ccbaa4634d5bd6012d8d49fff38`。
