# GJB9765.4.2.c：内部三态信号

目标附件 `T-Linter.dll` 的 SHA-256 为 `60e6b52875f4d7c3fbdf494d869b3a4bcc3fdc8ee63d8620ea9614da5d6599b0`。静态反编译证据保存在 `rea_vhdl_internal_tristate_4_2_c_bundle.json`；此处不宣称运行了厂商检查器。

## 原生观察

运行入口 `FUN_18078ae30` 对每个设计根先清空名称和关系集合，再调用 `FUN_18078bbc0` 收集声明/端口等 AST 节点及类型 `0x56` 的赋值节点，随后调用 `FUN_18078c370` 构造目标名、三态字面量与关联信号的记录，按集合条件报告。证据 `ev_0e2820bc56c108c74cbf9288e64d63c58742b213335097c583bdc2205f1af7ab`、`ev_bab82d7ffc9b587caf647c21c1f812b7a9b987bc893d0b6468e92689d22a6208`、`ev_af5e7a852961241391732336b1e1511cf33e73275aef08013c217669a51b0ba9`。

`FUN_18078c370` 的直接分支只接纳三子节点 `AST_ASSIGN_LE(0x56)`：左侧必须是
`AST_IDENTIFIER(0x13)`；第三子节点必须是 `0x10/0xb2/0xb3` 字面量且其值包含三态字符；
第二子节点必须是双子节点 `AST_WHEN(0xb6)`，其数据项是直接标识符，条件项是单子节点
`AST_COND(0x58)`。COND 中的关系根只接纳 `AST_LT/LE/EQ/EQX/GE/GT`
（`0x34/0x35/0x36/0x38/0x3a/0x3b`）、明确排除 `AST_NE/NEX(0x37/0x39)`，
恰有两个操作数，并从声明名称映射中选择其中一个操作数作为控制名。因此单独出现 Z
字面量不足以推导本规则告警。

继续读取同一函数的复杂分支后，`0xaa/0x58/0xac` 路径也已恢复到可描述的结构边界：

- 根必须是恰有两个子节点的 `AST_IF(0xaa)`，第一子节点为 `AST_COND(0x58)`，第二子节点
  为最终 `AST_ELSE(0xac)`；`0xaa/0xab/0xac` 到 IF/ELSIF/ELSE 的映射由同 DLL 的相邻
  VHDL 规则及控制流交叉印证；
- IF 条件仍只从上述六种双操作数关系根中提取声明名称映射内的控制对象；
- IF/THEN 侧只记录直接 `AST_BLOCK(0x54)` 内、恰有两个子节点的
  `AST_ASSIGN_LE(0x56)`，建立“目标名 → THEN 侧 RHS 节点”的关系；
- ELSE 侧扫描直接双子节点 `AST_ASSIGN_LE(0x56)`，仅在 RHS 为
  `0x10/0xb2/0xb3` 字面量且包含三态字符、并且同一目标已在 THEN 侧关系表出现时，填出
  目标、THEN 数据源和条件控制三个报告字段；
- 主函数随后仍应用“所选顶层 INOUT 才豁免”的边界。由此可排除“任意进程中出现一次
  Z 赋值就等于原生告警”的旧假设。

V1.4.48 继续恢复了顶层边界：

- `FUN_18078bbc0` 在类型 `0x76` 记录设计名称；相邻规则已把它确认到 VHDL entity；
- 遇到 entity 的直接端口容器 `0xa4` 时，收集子类型 6 的端口声明，并只把字节
  `+0x104 != 0` 的端口名加入豁免集合；相邻 GJB9765.4.5.f.24 的 DLL 分支和附件正反例
  已把 `+0x104` 确认为 `INOUT` 模式；
- 主函数的两个候选循环都先要求 `FUN_18078c370` 填出目标、三态和关系字段，然后把当前
  entity 名称与 `DAT_18135b808` 中的 `setTopModule` 名称作完整字符串比较；
- 当前 entity 不是所选顶层时直接报告。只有当前 entity 是所选顶层且赋值目标位于上述
  `INOUT` 豁免集合时跳过；顶层 `OUT` 不在豁免集合，非顶层 `INOUT` 也会报告；
- 第二循环在 `0x18078b6b1..0x18078b6f0` 调用字符串比较辅助器，栈上第五参数明确装载当前
  entity 名称长度，因此是完整相等比较，不是前缀搜索。

这些结论来自
`/Users/pinger/Desktop/Codex-test/reverse-tools/evidence/vhdl_4_2_c/top_and_helpers.c` 与
`/Users/pinger/Desktop/Codex-test/reverse-tools/evidence/vhdl_4_2_c/compare_call_block.txt`
的只读静态反编译/指令证据；目标摘要与本页一致。

V1.4.52 又收窄了关系条件可用的声明类别。`FUN_18078bbc0` 写入
`DAT_181374408` 名称→节点表的入口只有：

- `AST_PORT(0xa4)` 的直接 `AST_WIRE(6)` 子节点；
- `AST_GENERIC(0xa3)` 的直接 `AST_IDENTIFIER(0x13)` 子节点；
- `AST_SIGNAL(0x78)` 节点自身。

`FUN_18078c370` 对关系的左右操作数分别查找该表，任一操作数命中即可成为控制对象。
类型名函数同时确认 `0x79=AST_VARIABLE`、`0xa0=AST_ALIAS`，而常量声明为
`AST_CONSTANT(0x10)`；这些类别都没有名称表写入分支。因此产品条件控制集合现精确包含端口、
entity generic 和 signal，排除 constant、process variable 与 alias。这里是静态控制流观察，
不表示执行过厂商程序。

V1.4.53 进一步对齐 IF 分支的直接性。原生函数要求 IF 根恰有 `AST_COND` 与最终
`AST_ELSE` 两个子节点；THEN 侧只从该 COND 的直接 `AST_BLOCK` 子节点提取单个
`AST_ASSIGN_LE`。因此同层 `AST_ELSIF` 不属于该两臂候选，嵌套 IF 内的赋值也不能提升为
外层 THEN 的目标。产品源码适配现扫描完整 THEN 区域：只接纳嵌套深度为零的赋值，并在
同层出现 ELSIF 时否决候选。新增两个负例先复现旧误报，修正后均保持无 4.2.c 诊断。

V1.4.54 把同一直接性边界扩展到其他顺序控制节点。原生只读取 COND 的直接
`AST_BLOCK→AST_ASSIGN_LE`，所以嵌套 CASE 或 LOOP 内的赋值同样不能进入外层 THEN 目标表。
产品现在统一跟踪 IF、CASE、LOOP 嵌套深度；新增 CASE 和 WHILE-LOOP 两个负例均先复现旧
误报，修正后无 4.2.c 诊断。

V1.4.55 对齐 `AST_COND` 的单一关系根。原生要求 COND 恰有一个子节点，该节点必须是
`AST_LT/LE/EQ/EQX/GE/GT` 且恰有两个操作数；以 `AST_LOGIC_AND/OR/NOT` 等逻辑节点为根、
内部再包含比较关系的复合条件不属于候选。产品现在去除完整包围条件的括号后，要求顶层恰有
一个受支持关系运算符，并拒绝顶层 `and/or/xor/xnor/nand/nor/not`。并发 WHEN 与进程 IF
两个 AND 条件负例均先复现旧误报，修正后无 4.2.c 诊断。

V1.4.56 继续对齐关系操作数的直接性。`FUN_18078c370` 取得关系根的两个直接子节点，分别
使用各自 `+0x48` 名称查询 `DAT_181374408`；它不会递归搜索操作数子树中的声明名。因此
`sel + 1 = 2` 的左操作数是算术根，不能仅因其内部含 `sel` 而成为控制对象。产品现在只允许
关系运算符左、右两侧的直接声明名称（含同一标识符节点的选择后缀）命中控制表。并发 WHEN
和进程 IF 两个算术操作数负例均先复现旧误报、修正后通过；`2 = sel` 右操作数正例保持报告。
此结论仍来自只读静态反编译，没有执行厂商检查器。

## 产品接入与边界

旧实现全文件发现 `'Z'`/`"ZZZZ"` 即报，误报了附件遵循例中的外部 `inout C` 端口。
产品现按 architecture/entity 隔离声明和端口：内部 `signal`、顶层非 `INOUT` 端口以及所有
非顶层端口的明确三态候选会报告；只有所选顶层的 `INOUT` 边界豁免。多根回归证明切换显式
顶层后，豁免随所选 entity 转移；另有顶层 `OUT` 三态边界测试。V1.4.49 又把源码适配收窄
到上面的直接 `WHEN ... ELSE Z` 和进程 `IF relation THEN target <= data ELSE target <= Z`
关系形态；裸 Z、没有 ELSE 的单臂 IF、以及 THEN 为 Z/ELSE 为数据的反向分支均不报告，
并保持 `ManualReviewRequired`。

类型名函数 `FUN_1808ea310` 进一步把 `0xaa/0xab/0xac` 明确命名为
`AST_IF/AST_ELSIF/AST_ELSE`，把 `0xb2/0xb3` 明确命名为
`AST_STRING_LITERAL/AST_CHARACTER_LITERAL`。完整反编译保存在
`/Users/pinger/Desktop/Codex-test/reverse-tools/evidence/vhdl_4_2_c/type2str.c`。

别名/跨实例传播、配置与 generate、端口实际连接解析、所有选择/聚合目标、异常语法恢复及
精确诊断重数仍未知。
没有本规则诊断时仍为 `ManualReviewRequired`，不是自动合规。
