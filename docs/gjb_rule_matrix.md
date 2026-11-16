# GJB10157/GJB9765 规则实现矩阵

本表将 T-Linter 逆向得到的 209 条规则标识、严重级与强制属性，同两份标准 PDF 中的规则语义和正例/反例判据交叉核对。
`Automated` 表示存在可执行检测器；`ProjectContext` 表示需要器件、约束或综合工程信息；`ManualReview` 表示需要设计意图/人工复核。后两类会明确输出待复核状态，绝不静默判定为通过。

当前目录：GJB10157共103条（87可执行检测器、3工程上下文、13人工复核）；GJB9765共106条（85可执行检测器、5工程上下文、16人工复核）。累计206条规则已有证据支持的原生核心谓词部分接入；该数字与可执行检测器数量不同。完整算法等价验证仍为0条，因此检测未报警仍须复核。

每个条款均由 `standardRuleExamples()` 生成并暴露一对“正确案例判据/违反案例判据”；可用 `--describe-rule 条款号` 查询。

| 标准 | 条款 | 严重级 | 强制 | 规则摘要 | 评估方式 |
|---|---|---|---:|---|---|
| GJB10157-2021 | R-14-1 | error | 是 | 状态机禁止出现死锁状态和不可达状态。 | ManualReview |
| GJB10157-2021 | R-14-2 | warning | 是 | 状态机在 default 状态时，必须把状态机中所有寄存器重置为确定状态；按原生实现，仅在 default 键存在且简单右值经赋值依赖过滤后为空时报告一次。 | ManualReview |
| GJB10157-2021 | R-14-3 | warning | 是 | 状态机必须设置初始状态；原生实现检查内部关系模型是否生成精确 init 键：显式 initial 赋值、非状态控制 if 的非默认分支及非状态控制 case 的非 default 分支可生成该键。复杂表达式、default/else 单独赋值、声明初始化、宏和跨文件绑定仍需人工复核。 | ManualReview |
| GJB10157-2021 | A-14-1 | warning | 否 | 建议单个状态机的状态数不超过40个；原生报告器排除 init/default 关系键，对其余前端规范化状态键去重，并仅在数量严格大于40时输出警告。 | ManualReview |
| GJB10157-2021 | R-16-1 | error | 是 | 禁止在 // 注释式综合指令中使用 async_set_reset 或 sync_set_reset 改变代码的复位属性。 | Automated |
| GJB10157-2021 | R-16-2 | error | 是 | 状态机编译模式必须选择 safe 模式。 | ProjectContext |
| GJB10157-2021 | A-16-1 | warning | 否 | 避免使用“full_case”属性。 | Automated |
| GJB10157-2021 | A-1-1 | warning | 否 | 建议采用标准文件头。 | Automated |
| GJB10157-2021 | A-1-2 | error | 否 | 建议同一层次的所有语句左端对齐，case 和 if-else 等嵌套语句应逐层缩进对齐。 | ManualReview |
| GJB10157-2021 | A-1-3 | error | 否 | 建议端口、信号和变量的定义左端对齐。 | ManualReview |
| GJB10157-2021 | A-1-4 | warning | 否 | 建议每行字符数小于80个。 | Automated |
| GJB10157-2021 | A-1-5 | warning | 否 | 建议端口列表按照“时钟、复位、输入、输出、双向端口”的顺序描述。 | ManualReview |
| GJB10157-2021 | A-1-6 | warning | 否 | 避免在同一行中描述多个赋值语句。 | Automated |
| GJB10157-2021 | A-1-7 | warning | 否 | 建议一行只说明一个 I/O 端口 | ManualReview |
| GJB10157-2021 | A-1-8 | warning | 否 | 建议代码注释率不小于 20% | Automated |
| GJB10157-2021 | R-2-10 | error | 是 | 禁止内部信号有驱动但没有负载或任何使用。 | Automated |
| GJB10157-2021 | A-2-1 | warning | 否 | 建议文件名与 module 名相同。 | Automated |
| GJB10157-2021 | A-2-2 | error | 否 | 建议 module 名、端口名、instance名、信号名、变量名、define 名和函数名长度在2到32个字符 之间。 | Automated |
| GJB10157-2021 | A-2-3 | error | 否 | 时钟信号命名宜采用 clk_x 或 Clk_X | Automated |
| GJB10157-2021 | A-2-4 | error | 否 | 建议复位信号命名采用“Rst_X”、“Reset X”、“rst_x”或者“reset_x”，其中“x”为“p” 时代表 高电平复位，“x”“n”时代表低电平复位。 | Automated |
| GJB10157-2021 | A-2-5 | error | 否 | 建议常量的命名中字母全部采用大写，变量的命名中字母全部采用小写或者仅首字母大写。已按 DLL 固定默认位检查，附件 `parameter Data` 正例与二进制判据冲突。 | Automated |
| GJB10157-2021 | A-2-6 | warning | 否 | 避免使用 defparam 语句 | Automated |
| GJB10157-2021 | A-2-7 | warning | 否 | 避免在不同 module 中使用相同的参数名。 | ManualReview |
| GJB10157-2021 | A-2-8 | warning | 否 | 避免冗余端口或内部信号。已自动检查未使用输入端口、完全未使用内部声明及有驱动无负载内部信号。 | Automated |
| GJB10157-2021 | R-2-1 | error | 是 | 命名必须以字母开始，应采用字母、数字、下划线或者其组合。 | Automated |
| GJB10157-2021 | R-2-2 | error | 是 | 禁止使用连续的“_”命名，且禁止在末尾使用 “_”。 | Automated |
| GJB10157-2021 | R-2-3 | error | 是 | 命名禁止与 Verilog、SystemVerilog、VHDL、SDF 和 EDIF 的关键字同名。 | ManualReview |
| GJB10157-2021 | R-2-4 | error | 是 | 禁止使用“VDD”、“VSS”，“VCC”“GND”或者“VREF”命名。 | Automated |
| GJB10157-2021 | R-2-5 | error | 是 | 每个文件中只能有一个 module。 | Automated |
| GJB10157-2021 | R-2-6 | error | 是 | 禁止仅使用字母的大小写来区分命名。 | Automated |
| GJB10157-2021 | R-2-7 | error | 是 | 文件引用时，必须使用相对路径而不是绝对路径。 | Automated |
| GJB10157-2021 | R-2-8 | error | 是 | 禁止使用除 reg、wire、 integer、tri 和 parameter 之外的数据类型。 | ManualReview |
| GJB10157-2021 | R-2-9 | error | 是 | 在模块例化时，必须采用端口名称映射，禁止位置映射。 | Automated |
| GJB10157-2021 | R-3-1 | error | 是 | 端口例化时禁止使用表达式。 | ManualReview |
| GJB10157-2021 | R-3-2 | error | 是 | 顶层模块的输入输出端口必须只使用reg 型和wire 型数据。 | ManualReview |
| GJB10157-2021 | A-3-1 | warning | 否 | 顶层模块建议仅包含时钟产生模块、复位产生模块、RAM、ROM、V/O单元及其他例化的子模块， 避免使用胶合逻辑。 | ProjectContext |
| GJB10157-2021 | A-3-2 | warning | 否 | 除顶层模块外，上层模块建议仅包含子模块的例化。 | ProjectContext |
| GJB10157-2021 | A-3-3 | warning | 否 | 建议模块使用组合逻辑输入，寄存器输出。 | Automated |
| GJB10157-2021 | R-4-1 | error | 是 | 禁止将寄存器的输出直接连接到其他寄存器的时钟管脚。 | Automated |
| GJB10157-2021 | R-4-2 | error | 是 | 禁止将时钟信号连接在除寄存器时钟管脚之外的其他信号管脚。 | Automated |
| GJB10157-2021 | R-4-3 | error | 是 | 禁止将组合逻辑输出作为时钟信号 | Automated |
| GJB10157-2021 | A-4-1 | warning | 否 | 避免同时使用时钟的上升沿和下降沿。 | Automated |
| GJB10157-2021 | A-4-3 | warning | 是 | 避免使用门控时钟。 | Automated |
| GJB10157-2021 | A-4-4 | warning | 否 | 建议仅使用一个时钟域。 6.2 复位 | ProjectContext |
| GJB10157-2021 | A-5-5 | warning | 否 | 建议在同一个 always 语句中只使用同一个复位信号。 7 触发器、锁存器与缓冲器 7.1 触发器 | ManualReview |
| GJB10157-2021 | R-5-1 | error | 是 | 禁止将异步的置位/复位信号连接到非置位/复位端。 | Automated |
| GJB10157-2021 | R-5-2 | error | 是 | 禁止在同一条复位线路中同时使用异步复位和同步复位。 | Automated |
| GJB10157-2021 | R-5-3 | error | 是 | 禁止使用有竞争冒险情况的组合逻辑输出作为复位信号。 | Automated |
| GJB10157-2021 | R-5-5 | error | 是 | 非SRAM型FPGA，必须对寄存器进行上电初始化，赋固定值。 | ManualReview |
| GJB10157-2021 | A-5-2 | warning | 否 | 避免在异步复位线路中插入逻辑操作运算。 | Automated |
| GJB10157-2021 | A-5-3 | warning | 否 | 避免使用内部生成信号作为寄存器的异步复位信号。 | Automated |
| GJB10157-2021 | A-5-4 | warning | 否 | 避免同时使用异步置位和异步复位信号。 | ManualReview |
| GJB10157-2021 | R-6-1 | error | 是 | 禁止在同一个 always 语句中混合使用有复位和无复位的触发器。已自动检查简单异步复位 if/else 中确定的赋值目标集合。 | Automated |
| GJB10157-2021 | R-6-2 | error | 是 | 在复位或者初始化时，必须对触发器赋初值。已自动检查单边沿时序过程及简单异步复位 if/else 的确定目标。 | Automated |
| GJB10157-2021 | R-6-3 | error | 是 | 复位信号的触发条件与进入条件必须一致。已自动检查边沿事件与简单 if 条件的确定极性；复合条件仍需人工复核。 | Automated |
| GJB10157-2021 | R-7-1 | error | 是 | 禁止出现含有锁存器的组合逻辑环路。 | Automated |
| GJB10157-2021 | R-7-2 | error | 是 | 禁止使用基本逻辑门来描述锁存器；已接入标量连续布尔表达式和基本门实例反馈环检查，保留原生超过两个指定门则跳过的边界。 | ManualReview |
| GJB10157-2021 | A-7-1 | warning | 否 | 建议将锁存器与其他组合逻辑电路分开描述。已自动检查同一组合过程中可证明的锁存目标与其他赋值目标混合。 | Automated |
| GJB10157-2021 | R-8-1 | error | 是 | 禁止组合逻辑电路的输出作为三态缓冲器的使能端。已自动检查连续组合表达式生成并用于条件三态使能的简单信号。 | Automated |
| GJB10157-2021 | R-8-2 | error | 是 | 禁止使用内部三态。已自动检查连续赋值中明确驱动 Z 的内部简单目标。 | Automated |
| GJB10157-2021 | A-8-1 | warning | 否 | 避免将双向端口直接连接输入端口或输出端口。已自动检查工程内唯一解析模块的简单端口连接；未解析库单元仍需工程上下文。 | ProjectContext |
| GJB10157-2021 | R-9-1 | error | 是 | 关系操作符左边与右边的数据位宽必须一致。 | Automated |
| GJB10157-2021 | R-9-2 | error | 是 | 位操作符左边与右边的数据位宽必须一致。 | ManualReview |
| GJB10157-2021 | R-9-3 | error | 是 | 禁止在算术表达式中同时出现无符号数和有符号数。 | ManualReview |
| GJB10157-2021 | R-9-4 | error | 是 | 禁止在关系表达式中使用包含“x”或者“z”的值。 | Automated |
| GJB10157-2021 | R-9-5 | error | 是 | 禁止在算术操作时使用包含“x”或者“z”的值。 | Automated |
| GJB10157-2021 | R-9-6 | error | 是 | 在数值<位宽><进制><数字>表示中，数值位宽必须与数值的实际位宽一致。 | ManualReview |
| GJB10157-2021 | R-9-7 | error | 是 | 禁止算术运算结果出现位宽溢出 | Automated |
| GJB10157-2021 | A-9-1 | warning | 否 | 避免在向量运算中使用逻辑运算符。 | ManualReview |
| GJB10157-2021 | A-9-2 | warning | 否 | 建议为 parameter 数据和常数指定进制（d,b,h,o）和位宽。 | ManualReview |
| GJB10157-2021 | R-10-1 | error | 是 | 模块端口的数据位宽与实例化信号的数据位宽必须一致。 | ManualReview |
| GJB10157-2021 | R-10-2 | error | 是 | 赋值表达式左值与右值的位宽必须一致。 | Automated |
| GJB10157-2021 | R-10-3 | error | 是 | 组合电路必须使用阻塞赋值，禁止使用非阻塞赋值。 | Automated |
| GJB10157-2021 | R-10-4 | error | 是 | 时序电路必须使用非阻塞赋值，禁止使用阻塞赋值。 | Automated |
| GJB10157-2021 | R-10-5 | error | 是 | 禁止在同一条件下对同一个信号进行重复赋值。 | ManualReview |
| GJB10157-2021 | R-10-6 | error | 是 | 禁止使用延时语句。 | Automated |
| GJB10157-2021 | A-11-4 | warning | 否 | if 语句和条件运算符的条件表达式避免使用多位向量 | ManualReview |
| GJB10157-2021 | A-11-5 | warning | 否 | 建议组合逻辑电路中 if语句以 else语句结尾。 | Automated |
| GJB10157-2021 | R-11-1 | error | 是 | 在 case 语句中必须使用 default 分支。 | Automated |
| GJB10157-2021 | R-11-2 | error | 是 | 禁止在case 语句的控制表达式中使用固定值。 | Automated |
| GJB10157-2021 | R-11-3 | error | 是 | case语句的所有分支表达式必须在控制表达式的范围之内，并且数据位宽一致。 | Automated |
| GJB10157-2021 | R-11-4 | error | 是 | if 条件表达式与 case 控制表达式中禁止出现 X 或 Z | Automated |
| GJB10157-2021 | R-11-5 | error | 是 | 敏感列表禁止使用位运算符连接敏感信号 | ManualReview |
| GJB10157-2021 | R-11-6 | error | 是 | case 语句的分支语句必须使用常量，禁止使用变量或者逻辑运算。 | ManualReview |
| GJB10157-2021 | A-11-1 | warning | 否 | case 语句中的分支语句避免重叠。 | Automated |
| GJB10157-2021 | A-11-2 | warning | 否 | 避免使用 casex 与casez 语句。 | Automated |
| GJB10157-2021 | A-11-3 | warning | 否 | default 分支中避免使用包含 X 的赋值 | Automated |
| GJB10157-2021 | R-12-1 | error | 是 | for 语句的初始值和条件必须为常数，且禁止改变循环变量。 | ManualReview |
| GJB10157-2021 | R-12-2 | error | 是 | 禁止在 for 语句中对循环变量之外的变量进行算术运算。 | ManualReview |
| GJB10157-2021 | R-12-3 | error | 是 | 复位部分和逻辑部分禁止出现在同一个 for 循环中。 | ManualReview |
| GJB10157-2021 | R-12-4 | error | 是 | 循环语句必须至少满足一次循环条件。 | ManualReview |
| GJB10157-2021 | R-12-5 | error | 是 | 禁止调用递归函数。 | ManualReview |
| GJB10157-2021 | A-12-1 | warning | 否 | 避免在 repeat语句表达式中使用变量。 | ManualReview |
| GJB10157-2021 | A-13-1 | warning | 否 | 建议数组索引为简单的信号，避免使用表达式。 | ManualReview |
| GJB10157-2021 | R-13-1 | error | 是 | 禁止在 task 语句中使用时序控制语句。 | ManualReview |
| GJB10157-2021 | R-13-2 | error | 是 | 禁止使用 initial 语句。 | ManualReview |
| GJB10157-2021 | R-13-3 | warning | 是 | 禁止在 function 语句中使用非阻塞赋值。 | Automated（R-13-3） |
| GJB10157-2021 | R-13-4 | error | 是 | 在 function 和 task 语句中，禁止对全局信号进行赋值。 | ManualReview |
| GJB10157-2021 | R-13-5 | warning | 是 | function 返回值位宽必须与输入信号位宽一致 | Automated（有界源码适配；复杂位宽传播仍需复核） |
| GJB10157-2021 | R-13-6 | warning | 是 | task 输入信号位宽必须与输出信号位宽一致 | Automated（有界源码适配；复杂绑定仍需复核） |
| GJB10157-2021 | R-13-7 | warning | 是 | 禁止在组合逻辑电路中使用反馈。 | Automated（R-13-7；原生图结果消息与级别已恢复） |
| GJB10157-2021 | R-13-8 | warning | 是 | 禁止数组索引越界。 | Automated（R-13-8；原生多维范围与诊断表面部分恢复） |
| GJB10157-2021 | R-13-9 | error | 是 | 敏感信号列表中的敏感信号必须齐全，无遗漏，无冗余。 | Automated |
| GJB9765-2020 | R-10-1 | error | 是 | 禁止将寄存器输出反馈到异步复位或置位端 | ManualReview |
| GJB9765-2020 | R-10-2 | error | 是 | 复位与置位信号不应来自同一信号 | ManualReview |
| GJB9765-2020 | R-10-3 | error | 是 | 异步复位释放应采用同步释放 | ManualReview |
| GJB9765-2020 | R-10-4 | error | 是 | 寄存器复位端口禁止悬空 | ManualReview |
| GJB9765-2020 | A-10-1 | warning | 否 | 宜使用复位操作初始化寄存器 | ManualReview |
| GJB9765-2020 | A-10-2 | warning | 否 | 同一时刻宜使用同一复位域复位所有模块；已对声明为单一时钟输入且具有多个异步复位输入的 VHDL 实例比较直接复位信号映射。自定义端口语义、表达式、层次传播和复杂绑定仍需人工复核。 | ManualReview |
| GJB9765-2020 | A-10-4 | warning | 否 | 不宜使用时钟信号执行复位或置位 | Automated |
| GJB9765-2020 | A-10-5 | warning | 否 | 同步复位时外部复位宜使用两级级联寄存器同步；已自动识别 VHDL 时序进程中直接作为同步复位条件的实体输入端口。内部信号的完整两级来源、别名、层次及原语仍需人工复核。 | ManualReview |
| GJB9765-2020 | A-10-6 | warning | 否 | 同一复位或置位信号宜使用唯一有效电平 | Automated |
| GJB9765-2020 | A-10-7 | warning | 否 | 组合逻辑产生的复位信号应同步处理 | ManualReview |
| GJB9765-2020 | R-11-1 | error | 是 | 状态机无效状态应得到合理处理 | ManualReview |
| GJB9765-2020 | R-11-3 | error | 是 | 状态机应初始化到合法状态 | ManualReview |
| GJB9765-2020 | A-11-1 | warning | 否 | 建议使用三段式状态机 | ManualReview |
| GJB9765-2020 | A-11-2 | warning | 否 | 建议使用枚举类型声明状态机 | ManualReview |
| GJB9765-2020 | A-11-3 | warning | 否 | 单个状态机的状态数量一般应少于 40 | Automated |
| GJB9765-2020 | A-11-4 | warning | 否 | 面积优先时宜采用二进制状态编码 | ProjectContext |
| GJB9765-2020 | A-11-5 | warning | 否 | 速度优先时宜采用独热状态编码 | ProjectContext |
| GJB9765-2020 | R-12-1 | error | 是 | 避免非整数 generic、空字符串常量和 wait for 语句 | Automated |
| GJB9765-2020 | R-12-3 | error | 是 | 避免使用 buffer 端口模式 | Automated |
| GJB9765-2020 | A-13-1 | warning | 是 | 文件开头应添加规定内容的英文注释 | ProjectContext |
| GJB9765-2020 | A-13-2 | warning | 否 | 输入输出端口定义附近应添加注释 | Automated |
| GJB9765-2020 | A-13-3 | warning | 否 | 内部信号和变量定义附近应添加注释 | Automated |
| GJB9765-2020 | A-14-1 | warning | 否 | 代码各节之间应使用空行分隔 | ManualReview |
| GJB9765-2020 | A-14-3 | warning | 否 | 行首宜使用统一宽度的 Tab 对齐和缩进 | ManualReview |
| GJB9765-2020 | A-14-4 | warning | 否 | 行尾不宜存在多余空格 | Automated |
| GJB9765-2020 | A-14-5 | warning | 否 | 同层语句应左对齐，不同层次应逐级缩进 | ManualReview |
| GJB9765-2020 | A-14-6 | warning | 否 | 嵌套语句宜逐层缩进对齐 | ManualReview |
| GJB9765-2020 | A-14-7 | warning | 否 | 端口、信号和变量定义应对齐 | ManualReview |
| GJB9765-2020 | A-14-8 | warning | 否 | 模块例化时端口映射应对齐 | Automated |
| GJB9765-2020 | A-14-9 | warning | 否 | 变量、符号和括号之间宜保留适当空格 | Automated |
| GJB9765-2020 | A-14-10 | warning | 否 | 赋值符号两边宜保留空格 | Automated |
| GJB9765-2020 | A-14-11 | warning | 否 | 运算符两边宜保留空格 | Automated |
| GJB9765-2020 | A-14-12 | warning | 否 | 一行代码不宜超过 72 个字符 | Automated |
| GJB9765-2020 | A-14-13 | warning | 否 | 同一行不宜包含多个 HDL 语句 | Automated |
| GJB9765-2020 | A-14-14 | warning | 否 | 端口映射时一行宜只映射一个信号 | ManualReview |
| GJB9765-2020 | A-14-15 | warning | 否 | 文件引用应使用相对路径 | Automated |
| GJB9765-2020 | R-1-1 | error | 是 | 端口例化应采用名称映射，避免位置映射 | Automated |
| GJB9765-2020 | R-1-2 | error | 是 | 例化时禁止输入端口悬空 | Automated |
| GJB9765-2020 | A-1-1 | warning | 否 | 例化时未使用的输出端口宜使用 open | Automated |
| GJB9765-2020 | R-2-1 | error | 是 | 每个模块或单元应至少包含一个输入或输出端口 | Automated |
| GJB9765-2020 | R-2-2 | error | 是 | 除双向端口外，输出信号禁止直接驱动同模块内部逻辑 | ManualReview |
| GJB9765-2020 | R-2-3 | error | 是 | 内部信号应避免使用三态逻辑 | Automated |
| GJB9765-2020 | A-2-1 | warning | 否 | 顶层宜只进行模块例化 | ManualReview |
| GJB9765-2020 | A-2-2 | warning | 否 | 设计中不宜存在多余代码或不可达分支 | Automated |
| GJB9765-2020 | A-2-3 | warning | 否 | 设计中不宜产生非预期锁存器 | Automated |
| GJB9765-2020 | A-2-4 | warning | 是 | 每个 VHDL 文件最多包含一个实体 | Automated |
| GJB9765-2020 | R-3-1 | error | 是 | 进程敏感信号列表应完整且正确 | Automated |
| GJB9765-2020 | R-4-1 | error | 是 | 多位信号和变量定义时必须设置位宽 | ManualReview |
| GJB9765-2020 | A-4-1 | warning | 否 | 避免出现未使用的多余信号 | Automated |
| GJB9765-2020 | R-4-2 | error | 是 | 条件判断表达式中禁止使用问号、X、Z 等状态 | Automated |
| GJB9765-2020 | R-4-3 | error | 是 | 同一进程中不应使用多个 event control 段 | Automated |
| GJB9765-2020 | R-5-1 | error | 是 | 自定义短标识符禁止与 VHDL 关键字重复 | ManualReview |
| GJB9765-2020 | R-5-2 | error | 是 | 不同类型的自定义短标识符禁止采用相同名称 | Automated |
| GJB9765-2020 | R-5-3 | error | 是 | 标识符只能含字母数字和下划线，并满足首尾及连续下划线要求 | Automated |
| GJB9765-2020 | R-5-4 | error | 是 | 禁止仅用英文字母大小写区分不同命名 | Automated |
| GJB9765-2020 | A-5-1 | warning | 否 | 时钟命名宜与普通端口区别，并以 clk 开始 | Automated |
| GJB9765-2020 | A-5-11 | warning | 否 | 包体文件名宜采用包名称加 _body.vhd | Automated |
| GJB9765-2020 | A-5-15 | warning | 否 | 类型名称宜以 T 开始 | Automated |
| GJB9765-2020 | A-5-16 | warning | 否 | 子类型名称宜以 ST 开始 | Automated |
| GJB9765-2020 | A-5-17 | warning | 否 | 函数名称宜以 F 开始 | Automated |
| GJB9765-2020 | A-5-19 | warning | 否 | 块标号名称宜以 _block 结束 | Automated |
| GJB9765-2020 | A-5-20 | warning | 否 | generate 标号名称宜以 _gen 结束 | Automated |
| GJB9765-2020 | A-5-2 | warning | 否 | 信号名称不宜超过 20 个字符 | Automated |
| GJB9765-2020 | A-5-21 | warning | 否 | 进程标号名称宜以 _proc 结束 | Automated |
| GJB9765-2020 | A-5-23 | warning | 否 | 输入端口名称宜以 _in 结束 | Automated |
| GJB9765-2020 | A-5-24 | warning | 否 | 输出端口名称宜以 _out 结束 | Automated |
| GJB9765-2020 | A-5-25 | warning | 否 | 双向端口名称宜以 _inout 结束 | Automated |
| GJB9765-2020 | A-5-27 | warning | 否 | 时钟端口名称宜以 clk_ 开始 | Automated |
| GJB9765-2020 | A-5-29 | warning | 否 | 低有效信号名称宜以 _n 结束 | Automated |
| GJB9765-2020 | A-5-30 | warning | 否 | 反相或缓冲后的信号名称宜仅改变后缀 | Automated |
| GJB9765-2020 | A-5-3 | warning | 否 | 变量名称宜以 V 开始 | Automated |
| GJB9765-2020 | A-5-31 | warning | 否 | 复位端口宜以 rst 开始，非复位信号不宜包含 rst | Automated |
| GJB9765-2020 | A-5-4 | warning | 否 | 常量名称宜以 C 开始 | Automated |
| GJB9765-2020 | A-5-5 | warning | 否 | 实体名称宜以 _ent 结束 | Automated |
| GJB9765-2020 | A-5-6 | warning | 否 | 实体声明文件名宜采用实体名称加 .vhd | Automated |
| GJB9765-2020 | A-5-7 | warning | 否 | 结构体名称宜以 _arc 结束 | Automated |
| GJB9765-2020 | A-5-9 | warning | 否 | 包名称宜以 _pack 结束 | Automated |
| GJB9765-2020 | A-5-10 | warning | 否 | 包声明文件基名宜与包名称一致 | Automated |
| GJB9765-2020 | A-5-32 | warning | 否 | 跨越多个模块层次的信号宜使用相同命名 | ManualReview |
| GJB9765-2020 | R-6-1 | error | 是 | 信号应在赋值后使用 | Automated |
| GJB9765-2020 | R-6-2 | error | 是 | 赋值、比较和运算语句两侧位宽应相等 | Automated |
| GJB9765-2020 | R-6-3 | error | 是 | case 表达式与选择条件的位宽应相等 | Automated |
| GJB9765-2020 | R-6-4 | error | 是 | 常量赋值右侧应为固定整数或可计算的固定整数 | ManualReview |
| GJB9765-2020 | R-6-5 | error | 是 | 整数、实数和无符号整数之间不应使用隐式转换 | ManualReview |
| GJB9765-2020 | R-6-6 | error | 是 | 有符号数与无符号数禁止混用于同一运算式 | ManualReview |
| GJB9765-2020 | R-6-7 | error | 是 | 同一模块中的单向信号不应有多重驱动 | Automated |
| GJB9765-2020 | R-6-8 | error | 是 | 同一信号源应避免驱动同一寄存器的多个异步控制端 | ManualReview |
| GJB9765-2020 | A-6-1 | warning | 否 | 三态节点的使能端和输入端不宜由同一信号源驱动 | ManualReview |
| GJB9765-2020 | A-6-2 | warning | 否 | 不宜使用组合逻辑输出控制三态门 | ManualReview |
| GJB9765-2020 | A-7-1 | error | 是 | IF/ELSIF/CASE 控制表达式不得为常量；顺序 FOR 与 FOR generate 的直接名称边界必须解析为常量 | Automated |
| GJB9765-2020 | R-7-1 | error | 是 | 禁止在 for 循环体内使用 := 修改当前或外层循环变量 | Automated |
| GJB9765-2020 | R-7-2 | error | 是 | WHILE 条件禁止使用直接字面量、已解析命名常量或两侧均为常量的直接关系表达式；动态名称和普通 IF 不属于该谓词 | Automated |
| GJB9765-2020 | R-8-1 | error | 是 | case 控制表达式不应为固定常量 | Automated |
| GJB9765-2020 | R-8-2 | error | 是 | case/with-select 分支选择不应使用已解析为端口、信号或变量的运行时名称；常量、泛型和枚举字面量允许 | Automated |
| GJB9765-2020 | R-8-3 | error | 是 | 未完全覆盖取值时 case 必须包含 when others | Automated |
| GJB9765-2020 | R-8-4 | error | 是 | 组合逻辑中的 if 语句应包含 else 分支 | Automated |
| GJB9765-2020 | A-8-1 | warning | 否 | 进程中应避免空语句 | Automated |
| GJB9765-2020 | R-9-1 | error | 是 | 禁止时钟信号经不同路径后再汇聚 | ManualReview |
| GJB9765-2020 | R-9-2 | error | 是 | 禁止时钟信号连接到多路选择器控制端 | ManualReview |
| GJB9765-2020 | R-9-3 | error | 是 | 避免使用门控时钟 | Automated |
| GJB9765-2020 | R-9-4 | error | 是 | 同一进程中应只使用一个时钟 | Automated |
| GJB9765-2020 | A-9-1 | warning | 否 | 附件示例与既有目录语义冲突，需核对规则版本 | ManualReview |
| GJB9765-2020 | A-9-3 | warning | 否 | 不宜使用组合逻辑产生时钟 | Automated |
| GJB9765-2020 | A-9-4 | warning | 否 | 设计中应尽量减少时钟域 | ProjectContext |
| GJB9765-2020 | A-9-5 | warning | 否 | 同一时钟源不宜驱动不同边沿触发的寄存器 | Automated |
| GJB9765-2020 | A-9-6 | warning | 否 | 时钟路径上应避免插入反相器或缓冲器 | ManualReview |
