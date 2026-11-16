# GJB9765.4.6.b.1 VHDL 两侧位宽一致性规则恢复记录

## 目标与二进制身份

- 目标规则：`GJB9765.4.6.b.1`
- 附件版本：T-Linter V3.1.4.25120520251205115359
- `T-Linter.dll` SHA-256：`60e6b52875f4d7c3fbdf494d869b3a4bcc3fdc8ee63d8620ea9614da5d6599b0`
- 构造函数：`0x1800288a0`
- slot 0：`FUN_1805fd880`（`0x1805fd880`）
- 记录收集器：`FUN_1805fe650`（`0x1805fe650`）
- 宽度推导器：`FUN_1805ff100`（`0x1805ff100`）
- 本记录来自 Ghidra 12.1.4 对用户附件的离线静态反编译，不代表观察了原软件运行。

## Recovered predicate

- `FUN_1805fd880` 对每个保存的设计根节点清空并重建六组声明/名称宽度映射，调用 `FUN_1805fe650` 递归构造比较记录，然后逐记录、逐右侧条目比较宽度。
- 最终报告条件非常明确：左、右宽度均不为 `-1`，且两者不相等。判断是对称的，不区分“源比目标宽”还是“目标比源宽”。附件中的 9←8、7←8、12←8 三种赋值都属于违反。
- `FUN_1805fe650` 存在三类显式收集分支：
  - 类型 `0x56`、恰好两个子节点的赋值；
  - 类型 `0x34..0x37`、`0x3a..0x3b`、恰好两个子节点的六类关系运算；
  - 类型 `0x25..0x28`、恰好两个子节点的四类运算操作。附件自身的类型名函数 `FUN_1808ea310` 已将其逐一确认为 `AST_BIT_AND`、`AST_BIT_OR`、`AST_BIT_XOR`、`AST_BIT_XNOR`。
- 同一类型名函数把 `0xa2` 明确标为 `AST_VARASGN`，而收集器没有 `0xa2` 分支；因此 VHDL `:=` variable assignment 不属于本规则的赋值记录。类型 `0x78/0x79` 分别为 `AST_SIGNAL/AST_VARIABLE`，`FUN_180600050` 将两者宽度分别写入 `+0x158/+0x178`，所以 variable 仍可作为关系或位运算操作数参与宽度比较。
- 内嵌 GB18030 字符串依次为“赋值语句”“比较操作符”“运算操作符”“右边信号位宽为”“左边信号位宽为”“位宽不匹配，”“左右信号位宽应该相等，”，与三个分支和最终比较一致。
- `FUN_1805ff100` 处理直接常量、64 位常量、解析后的名称、选择/切片、函数调用、拼接、一元运算、四类位运算、关系/逻辑结果、加减乘除模及幂等节点。它使用最大子宽度、拼接求和、乘法求和等策略，并含若干加减、常量 0/1、函数/属性特例。
- 函数调用分支逐名比较 `to_unsigned`、`to_integer`、`to_signed`、`conv_std_logic_vector`、`to_stdlogicvector`、`to_bitvector`，只在 AST 恰有两个子节点时继续，并从第二子节点读取数值常量、已登记名称值或对象 `length` 属性作为结果宽度。
- `FUN_180600050` 是已登记名称值的生产路径：处理匹配实体的 generic 容器时，对可求值的默认项调用 `FUN_1804b78d0` 并以 generic 名称写入对象 `+0x1a8` 的名称→整数表；递归遇到类型 `0x0a`（附件类型表中的 `AST_LOCALPARAM`）且可求值时，也将名称和值写入同一表。`FUN_1805fff90` 随后按名称读取，未命中返回 `-1`。
- 名称宽度查询器 `FUN_1805fee80` 依次查找对象内偏移 `+0x168`、`+0x188`、`+0x198`、`+0x158`、`+0x178`，全部未命中才返回未知。结合 `FUN_180600050` 的生产分支可分别确认这些表为 localparam/constant、entity port、entity generic、signal、variable；因此固定优先级为 `constant > port > generic > signal > variable`，而不是源码中最后出现的同名声明覆盖所有类别。
- `FUN_1804b78d0` 的完整分派已反编译：类型 `0x10` 为带有效标志的常量，`0x13` 为名称表读取，`0x43/0x44` 为 `AST_POS/AST_NEG`，`0x3c..0x3f` 依次为 `AST_ADD/AST_SUB/AST_MUL/AST_DIV`。其他类型直接返回失败；二元节点递归求两侧后执行对应整数运算。单目正负的零值分支不返回成功，除法路径未观察到本函数内的除零保护。
- `FUN_1805fd880` 对每个输入记录只取其根向量首节点，先调用 `FUN_1805fefc0` 清空本规则的宽度/名称表，再以该根调用 `FUN_180600050`，之后才调用 `FUN_1805fe650` 收集比较记录。入口不按 architecture 类型过滤根节点；`FUN_180600050` 在处理 entity/port/generic、signal、variable 和 `AST_LOCALPARAM` 后，无条件按子节点向量继续递归。因此同一 architecture 根下 process、block、function/procedure 等声明区中的可求值 constant 都会写入 `+0x1a8`；package body 作为另一棵完整设计根时，其子程序形式参数、变量、常量以及赋值/关系/位运算节点也会由同一组递归器处理，而每个独立 package/architecture 根之间都会在入口处重新清表。
- 对赋值右侧的加/减节点还存在额外排除路径：当推导宽度与左侧相等并满足名称/常量形态条件时可能不构造普通不等记录。该路径的所有前端语义尚未完全恢复。

## 产品接入

- VHDL 直接赋值由“只在源更宽时报告截断”改为“两侧已知且不相等即报告”，因此同时覆盖扩展与截断，并覆盖并发和进程内赋值。
- 已知字符串字面量、直接名称、选择/切片和可静态求宽的拼接沿用同一对称判断；未知宽度不猜测。
- 六类 VHDL 关系运算继续通过有界过程条件检查映射到本条款。
- `and/or/xor/xnor` 两侧已知宽度不相等检查映射到同一条款；本轮把操作数范围从单 token 扩展到静态选择/切片与括号内拼接，避免原生可求宽场景漏报。
- 转换函数只在恢复出的白名单内求宽；产品支持直接正整数、已知信号/端口的 `'length`、同一实体的 integer generic 默认值，以及同一 architecture 根内 architecture/process/block/subprogram 声明区的 integer constant。命名值表达式严格限定为已恢复的常量、前序名称、括号、单目正负和 `+ - * /`；后遍历的同名 constant 覆盖先前 generic/constant，结果同时用于赋值与位运算两侧比较。
- package body 现在按独立设计根建立名称表，收集子程序形式参数、局部 signal/variable/vector constant 与可求值整数 constant；在该根内检查 `<=`、六类关系和 `and/or/xor/xnor`，明确排除 `:=`。package body 的名称和值不向后续 architecture 传播。
- architecture 使用同一根内形状收集器补齐 process/subprogram 局部 variable 与形式参数；关系和位运算按完整 architecture 根遍历，`<=` 赋值仍使用原生支持的信号赋值形态。
- 根内形状收集器现保留 constant、port、generic、signal、variable 五类独立名称表，并按原生固定优先级合并；所属 entity 的固定宽度 vector generic 也进入宽度解析。显式形式参数类进入对应表，缺省 `in`/`out|inout|buffer` 参数分别按 VHDL 的 constant/variable 缺省类处理，file 参数因没有已恢复的宽度生产分支而失败关闭。
- 附件违反样例现分别定位 9←8、7←8 和时序中的 12←8 三处；遵循样例保持零报告。

## 已知边界

- 白名单函数名、名称值表的两类生产节点以及 `FUN_1804b78d0` 的运算类型集合已恢复；其中 `to_integer` 的原生二子节点前端形态、原生 32 位溢出/异常边界、其他属性例外、加减常量排除和若干宽度辅助函数尚未完整解释。
- 产品的位运算分支当前覆盖直接命名、直接字符串、静态选择/切片、括号内拼接和已恢复白名单转换；architecture/package body 已按独立根覆盖直接声明和子程序形式参数，并复现已观察的跨类别固定查询顺序。原生名称是否携带作用域修饰、同名嵌套子程序、缺省形式参数的精确前端节点类别、跨 design-unit/package specification 可见性、动态范围和嵌套位运算的精确结合语义仍进入人工复核；`mod`、`rem`、幂、除零、超出有符号 32 位结果和前向未定义名称不猜测。
- subtype、别名、包常量、重载、生成域、错误恢复、诊断顺序与完整 AST 等价仍未恢复。
- 无报告仍保持 `ManualReviewRequired`，不宣称完整原生算法等价。

## 回归覆盖

`tests/standard_accuracy_tests.cpp` 覆盖宽目标、窄目标、时序赋值、等宽赋值、关系运算、直接位运算、不等宽/等宽静态切片、括号内拼接、转换函数数值宽度、`'length` 宽度、generic/architecture/process/block/subprogram constant、整根覆盖顺序、独立根隔离、package-body/architecture process/subprogram 的 signal/variable/formal 关系与位运算，以及 `AST_VARASGN` 排除、所属 entity vector generic、`port > variable`、`constant > port`、`generic > signal`、`signal > variable`、前序名称、单目负值、四则运算、优先级、`mod`/除零/前向引用失败关闭和非白名单函数；`tests/analyzer_tests.cpp` 明确验证 VHDL 扩展与截断均报告。入口与宽度解析证据为 `reverse-tools/evidence/r6_2_width_entry_collector_resolver.c`（SHA-256 `a7f1be023c877cc9906fbdd9a7eb6f96bb894321b3c10975f7702fea5985537f`），AST 类型表证据为 `reverse-tools/evidence/a5_29_ast_enum_mapper.txt`（SHA-256 `78d56f0b3146c837c180974a5279ef43a718bc17c2507e45d1e7597d8b9bd7b0`），常量求值器证据为 `reverse-tools/evidence/r6_2_constant_evaluator.c`（SHA-256 `4bc000795b51fc2a0ab9807e01c1216970a50277bcbae41835a5fb3efd3b6181`）。
