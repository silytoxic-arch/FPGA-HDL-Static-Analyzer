# V1.0 规则参考

所有规则输出稳定的规则 ID、严重级别、消息、详细说明、修改建议、文件、行和列；
已识别设计单元时还会附带 module/entity 名称。

`analysis/rule_catalog.cpp` 是默认级别、适用语言和帮助文本的权威目录。使用
`fpga_analyzer_cli --list-rules` 查看完整列表，或用 `--describe-rule RULE` 查询单条规则。
配置及命令行中的未知 `disable` 规则会作为错误拒绝，防止拼写错误被静默忽略。
可通过配置 `severity RULE LEVEL` 或 CLI `--severity RULE=LEVEL` 覆盖默认级别；同一规则
最后一次设置生效。关闭规则仍使用 `disable`。

| 规则 ID | 默认级别 | 语言 | 检查内容 |
|---|---|---|---|
| `STYLE-001` | info | 全部 | 行尾空白 |
| `STYLE-002` | info | 全部 | 行宽超过 120 列 |
| `SV-ALWAYS-COMB-EVENT-CONTROL` | error | SystemVerilog | always_comb 内出现显式事件控制，破坏自动敏感语义 |
| `SV-ALWAYS-FF-EVENT-CONTROL` | error | SystemVerilog | always_ff 没有 event control 或包含多个 event control |
| `SV-ALWAYS-FF-NONEDGE-EVENT` | error | SystemVerilog | always_ff 的唯一 event control 不包含 posedge/negedge，不能表达普通边沿触发寄存器 |
| `SV-ALWAYS-LATCH-EVENT-CONTROL` | error | SystemVerilog | always_latch 内出现显式事件控制，破坏自动敏感语义 |
| `SV-ALWAYS-LATCH-NO-LATCH` | warning | SystemVerilog | always_latch 没有任何已识别赋值需要在未覆盖控制流路径上保持原值 |
| `CDC-DIRECT-CROSSING` | warning | 全部 | 同一设计单元内寄存器信号直接跨时钟域；索引 RAM/数组与可证明的两级标量同步链排除 |
| `RDC-MULTI-CLOCK-ASYNC-RESET` | warning | 全部 | 同一异步复位直接控制单元内多个时钟边沿域，复位释放可能分别违反 recovery/removal 时序 |
| `VL-SEQ-BLOCKING` | warning | Verilog/SV | 边沿触发过程中的阻塞赋值；排除 `for` 循环控制语句 |
| `VL-COMB-NONBLOCKING` | warning | Verilog/SV | 组合过程中的非阻塞赋值 |
| `VL-ALWAYS-NO-SUSPENSION` | error | Verilog/SV | 普通 always 不含事件、延时或 wait 挂起点，会在零仿真时间无限重复 |
| `VL-EMPTY-CASE-ITEM` | warning | Verilog/SV | case 选择项为空语句或结构性空 begin/end，选择该项时不执行任何 RTL 行为 |
| `VL-EMPTY-CONDITIONAL-BRANCH` | warning | Verilog/SV | if/else 分支为空语句或结构性空 begin/end，可能存在多余分号或遗漏逻辑 |
| `VL-IMPLICIT-NET` | warning | Verilog | 简单 continuous assignment 端点或 module 实例端口实际信号未声明，可能创建隐式网络 |
| `VL-MIXED-ASSIGNMENT-STYLE` | warning | Verilog/SV | 同一过程内对同一信号混用阻塞与非阻塞赋值，导致调度方式和硬件意图不一致 |
| `VL-SIDE-EFFECT-CONDITION` | warning | Verilog/SV | 分支、循环或运行时三目 guard 包含赋值、复合赋值、移位赋值或自增/自减，求值同时修改状态 |
| `VL-CASEX` | warning | Verilog/SV | 使用可能隐藏 X/Z 问题的 `casex` |
| `VL-OVERLAPPING-WILDCARD-CASE` | warning | Verilog/SV | casez/casex 常量通配模式存在交集，分支选择依赖源码顺序 |
| `VL-CASE-DEFAULT` | warning | Verilog/SV | `case`/`casez`/`casex` 缺少 `default`；嵌套 case 独立跟踪 |
| `VL-CASE-ITEM-VALUE-OUT-OF-RANGE` | warning | Verilog/SV | 无尺寸非负整数 choice 超出固定宽度 selector 的全部可表示 bit pattern，因而永远无法匹配 |
| `VHDL-NONSTD-NUMERIC` | warning | VHDL | 使用 `std_logic_arith`、`std_logic_unsigned` 或 `std_logic_signed` |
| `VHDL-PROCESS-NO-SUSPENSION` | error | VHDL | process 既无 sensitivity list，过程体也无 wait，形成无限 delta-cycle 循环 |
| `VHDL-SHARED-VARIABLE` | warning | VHDL | 使用需要同步保护的 shared variable |
| `VHDL-WAIT` | warning | VHDL | RTL 中使用可综合性依赖上下文/工具的 `wait` |
| `VHDL-WAIT-WITH-SENSITIVITY` | error | VHDL | 带 sensitivity list 的 process 内出现显式 wait，违反 VHDL 进程语义 |
| `STRUCT-DUPLICATE-UNIT` | error | 全部 | 同一项目中存在重名 module/entity 定义 |
| `STRUCT-DUPLICATE-NAMED-PARAMETER` | error | 全部 | 同一实例重复覆盖同一 named parameter/generic |
| `STRUCT-DUPLICATE-NAMED-PORT` | error | 全部 | 同一个实例重复连接同一 named formal 端口 |
| `STRUCT-DUPLICATE-PARAMETER-DECLARATION` | error | 全部 | module/entity 接口重复声明 parameter/generic |
| `STRUCT-DUPLICATE-PORT-DECLARATION` | error | 全部 | module/entity 接口重复声明同名 port |
| `STRUCT-DUPLICATE-SIGNAL-DECLARATION` | error | 全部 | 同一 module/architecture 作用域重复声明内部信号，或 VHDL signal 与 port 同名冲突 |
| `STRUCT-MISSING-REQUIRED-PARAMETER` | error | 全部 | 无默认值的 parameter/generic 未被实例覆盖 |
| `STRUCT-MIXED-PARAMETER-OVERRIDE-STYLE` | error | Verilog/SV | 参数覆盖列表混用 positional 与 named assignments |
| `STRUCT-UNRESOLVED-INSTANCE` | info | 全部 | 实例目标不在本次分析输入中，可能是缺失源码或外部 IP |
| `STRUCT-UNKNOWN-NAMED-PORT` | error | 全部 | 已解析实例的 named formal 不存在于目标 module/entity 接口中 |
| `STRUCT-UNKNOWN-NAMED-PARAMETER` | error | 全部 | 已解析实例的 named parameter/generic 不存在于目标接口中 |
| `STRUCT-UNCONNECTED-INPUT-PORT` | warning | 全部 | 已解析实例遗漏必需 input，或以空 actual / `open` 显式悬空 |
| `STRUCT-UNCONNECTED-OUTPUT-PORT` | info | 全部 | 已解析实例遗漏 output/inout，或以空 actual / `open` 显式悬空 |
| `STRUCT-VHDL-ASSOCIATION-ORDER` | error | VHDL | port/generic map 在 named association 后再次出现 positional association |
| `STRUCT-MIXED-PORT-CONNECTION-STYLE` | error | Verilog/SV | 同一实例混用 positional 与 named connections |
| `STRUCT-POSITIONAL-PORT-COUNT` | warning | 全部 | positional connection 数量超过已解析目标的端口数量 |
| `STRUCT-POSITIONAL-PARAMETER-COUNT` | error | 全部 | positional parameter/generic override 数量超过目标声明数量 |
| `STRUCT-RECURSIVE-INSTANCE` | error | 全部 | module/entity 实例关系形成递归环路 |
| `SYNTAX-UNTERMINATED-COMMENT` | error | Verilog/SV | 块注释到文件结尾仍未闭合 |
| `SYNTAX-UNTERMINATED-STRING` | error | 全部 | 字符串在行尾或文件尾仍未闭合 |
| `SYNTAX-UNMATCHED-DELIMITER` | error | 全部 | 出现无匹配开符号的右括号 |
| `SYNTAX-UNCLOSED-DELIMITER` | error | 全部 | 左括号到文件结尾仍未闭合 |
| `SYNTAX-UNEXPECTED-CLOSER` | error | Verilog/SV | `endmodule`、`endcase` 等没有对应起始结构 |
| `SYNTAX-UNCLOSED-CONSTRUCT` | error | Verilog/SV | module、begin、case、function、task、generate、interface、package、class 或 fork 未闭合 |
| `PP-INCLUDE-NOT-FOUND` | error | Verilog/SV | include 文件在源码目录及配置 include 目录中均不存在 |
| `PP-UNEXPECTED-DIRECTIVE` | error | Verilog/SV | elsif/else/endif 没有对应条件编译起点 |
| `PP-INVALID-DIRECTIVE` | error | Verilog/SV | define/include 指令缺少合法参数 |
| `PP-UNCLOSED-CONDITIONAL` | error | Verilog/SV | ifdef/ifndef 没有对应 endif |
| `RTL-ASYNC-RESET-POLARITY-MISMATCH` | error | Verilog/SV | 异步 reset/clear/preset 事件边沿与首个复位分支测试的有效电平不一致 |
| `RTL-ASYNC-RESET-NOT-FIRST` | error | 全部 | 敏感列表中的异步 reset/clear/preset 未作为过程首个独立 if 条件，可能被功能条件屏蔽 |
| `RTL-ASYNC-RESET-UNGUARDED-LOGIC` | error | 全部 | 首个异步复位条件链之后仍有独立赋值，会在复位事件上覆盖或绕过复位行为 |
| `RTL-CASE-ITEM-WIDTH-MISMATCH` | warning | 全部 | 直接、常量位选/索引或切片 selector 与显式 sized Verilog case 项或 VHDL 字符串 choice 位宽不一致 |
| `RTL-MULTIPLE-DRIVERS` | error | 全部 | 已声明信号由多个独立 always/process/并发赋值驱动；Verilog unpacked memory 按多端口 RAM 模式排除 |
| `RTL-NEGATIVE-SHIFT-AMOUNT` | warning | 全部 | 移位量是可直接证明的负整数字面量，可能反转方向、变成超大计数或违反参数约束 |
| `RTL-MULTIPLE-CASE-DEFAULT` | error | 全部 | 同一 case 含多个 Verilog default 或 VHDL when others，后续 catch-all 分支非法且不可达 |
| `RTL-COMBINATIONAL-LOOP` | error | 全部 | 连续赋值或组合过程的信号依赖形成无时序单元断开的强连通环 |
| `RTL-COMPARISON-WIDTH-MISMATCH` | warning | 全部 | 分支、循环或条件表达式中的比较使用两个已知但不同位宽的信号、literal、选择或拼接操作数 |
| `RTL-COMPLEX-EDGE-EVENT` | warning | 全部 | 边沿事件/函数作用于组合表达式、位选或索引，而不是直接标量时钟或异步控制信号 |
| `RTL-CONSTANT-CONDITION` | warning | 全部 | 对过程分支、循环及运行时三目/when-else guard 做保守常量求值；generate/elaboration 条件排除 |
| `RTL-DERIVED-CLOCK` | warning | 全部 | 由本地 RTL 驱动的信号被用作边沿时钟，可能产生毛刺、偏斜或非专用时钟布线 |
| `RTL-DIVIDE-BY-ZERO` | error | 全部 | 除法、取模、余数或 Verilog 复合算术操作使用 literal 零分母 |
| `RTL-DUPLICATE-CASE-ITEM` | warning | 全部 | 同一个 case 内存在完全重复的选择项，后出现的分支不可达 |
| `RTL-DUPLICATE-CONDITION` | warning | 全部 | 同一 if/else-if 或 if/elsif 链重复完全相同的条件，后出现的分支不可达 |
| `RTL-DUPLICATE-SENSITIVITY` | info | 全部 | 显式敏感列表重复同一信号或完全相同的边沿事件；VHDL 大小写不敏感，Verilog 相反边沿不合并 |
| `RTL-EMPTY-PROCESS` | info | 全部 | `always`/`initial` 仅含空结构，或 VHDL process 主体仅含 `null;`，因而不影响设计行为 |
| `RTL-NONSYNTHESIZABLE-DELAY` | warning | 全部 | Verilog/SV 过程或 continuous assign 延时，以及 VHDL `after` 仿真延时 |
| `RTL-OUT-OF-RANGE-SELECT` | error | 全部 | 常量位索引或切片边界超出信号声明的真实上下界 |
| `RTL-INPUT-DRIVEN` | error | 全部 | input/in 端口在设计单元内部被赋值 |
| `RTL-UNDRIVEN-OUTPUT` | warning | 全部 | output/out 端口没有过程或连续/并发赋值驱动 |
| `RTL-UNDRIVEN-SIGNAL` | warning | 全部 | 被表达式读取的内部信号没有本地驱动或保守识别到的实例连接 |
| `RTL-UNUSED-ASYNC-RESET` | error | 全部 | 异步 reset-like 信号位于时序过程敏感列表中，但过程体未读取该信号 |
| `RTL-UNUSED-INPUT-PORT` | info | 全部 | input/in 端口既未被本地逻辑读取，也未转发到实例连接 |
| `RTL-UNUSED-PARAMETER` | info | 全部 | parameter/localparam/generic 在所属设计单元接口和实现中从未被引用；VHDL 跨文件关联 entity 与 architecture，named override formal 排除 |
| `RTL-UNUSED-SIGNAL` | info | 全部 | 内部信号声明后从未被读取；只写信号也会命中 |
| `RTL-WIDTH-TRUNCATION` | warning | 全部 | 静态可求宽源信号、literal 或简单拼接宽于已知宽度目标，导致截断或违反 VHDL 长度约束 |
| `RTL-INCOMPLETE-SENSITIVITY` | warning | 全部 | 显式组合 `always`/`process` 读取了敏感列表中缺少的已声明信号；自动列表和时钟过程排除 |
| `RTL-REDUNDANT-SENSITIVITY` | info | 全部 | 显式组合敏感列表包含过程体从未读取的已声明信号；自动列表和时钟过程排除 |
| `RTL-LATCH-INFERRED` | warning | 全部 | 组合过程中信号仅在部分控制流路径赋值；`always_latch` 视为显式设计意图 |
| `RTL-MIXED-CLOCK-EDGE` | warning | 全部 | 同一设计单元内同一时钟信号同时用于上升沿和下降沿过程 |
| `RTL-MIXED-RESET-MODE` | warning | 全部 | 同一设计单元内同一复位信号同时作为同步和异步复位使用 |
| `RTL-RESET-POLARITY-CONFLICT` | error | 全部 | 同一复位信号被观察为高有效和低有效两种相互冲突的用法 |
| `RTL-SELF-ASSIGNMENT` | info | 全部 | 过程内标量直接赋值的左右两侧为同一信号；切片、索引、连续赋值和复合表达式排除 |
| `RTL-SHIFT-AMOUNT-OUT-OF-RANGE` | warning | 全部 | 常量移位量大于等于直接/索引/切片操作数位宽，所有原始位都会被移出 |
| `RTL-MULTIPLE-CLOCKS-PER-PROCESS` | error | 全部 | 同一个时序过程包含两个或更多独立时钟边沿事件；异步 reset/clear/preset 事件排除 |

## 已知边界

V1.0 已使用预处理及注释/字符串感知 token 层提取设计单元、实例、时钟/复位摘要、时序过程域、驱动源、信号读写、
组合敏感列表、过程内控制流和高置信度语法错误，并提供项目级禁用及源码局部
豁免，但规则主体仍以快速源码扫描
为主。它还不是完整 HDL parser，因此宏展开、generate/elaboration、复杂语法、configuration
和外部库绑定可能限制精度。实例端口方向未知时会保守地把实际信号视为已连接；Verilog
声明初始化和 `initial` 块可证明信号有初始化来源，但不计作并行运行时驱动冲突。后续引入完整
语法树适配层时应保持统一 `Violation`、结构模型
和报告接口，避免规则或 UI 绑定某个解析后端。

`RTL-DIVIDE-BY-ZERO` 在设计单元实现范围内扫描 Verilog/SystemVerilog 的 `/`、`%`、
`/=`、`%=` 和 VHDL 的 `/`、`mod`、`rem`，仅在分母是可直接证明为零的整数 literal 时报告；
完整包围 literal 的多层括号会被剥离，Verilog based literal 也会求值。VHDL `/=` 明确按不等
比较排除。参数、常量标识符、算术表达式、函数结果以及带一元符号的零当前不做常量传播，
由后续 AST/elaboration 阶段扩展。

`VL-ALWAYS-NO-SUSPENSION` 仅检查普通 `always`，在完整过程范围内接受任意直接 `@` 事件控制、
`#` 延时控制或 `wait` 语句作为挂起点；`always_comb`、`always_ff`、`always_latch` 和 `initial`
不适用。当前不跨 task 调用边界解析内部 timing control，因此将 task 封装挂起行为的仿真代码
应显式局部豁免；面向综合的 RTL 应优先把事件控制直接写在过程入口。

named-port 结构检查提取 Verilog/SystemVerilog ANSI、module-scope legacy non-ANSI 端口和
VHDL entity port，保留规范化方向和来源；实例连接记录 `.formal(actual)` 或 `formal => actual`
中的 formal，并区分非空 actual 与 `.formal()` / `formal => open`；SystemVerilog `.formal`
shorthand 视为已连接。VHDL formal 以大小写不敏感方式去重和查找。重复 formal
不依赖目标解析即可报告；未知 formal 仅在目标 unit 在本次输入中唯一解析且端口目录非空时
报告，因此 black box、未解析 IP 和无可用接口模型的目标不会产生猜测。必需输入检查仅对目标
唯一解析、接口可用的纯 named 实例执行；input 默认值、output/inout、Verilog `.*` 以及含
positional 前缀的 VHDL association list 排除。interface/modport、configuration binding 和
component 声明接口一致性留待后续。

参数接口检查提取 module header 的 value/type parameter 和 entity generic，保留声明顺序、默认值
状态及来源；实例 `#(...)` 与 `generic map(...)` 记录 override 数量、风格和 named formal。
重复 override 不依赖目标解析即可报告；未知 override 仅要求目标在本次输入中唯一解析，并能
正确检查参数列表为空的目标。VHDL 名称不区分大小写，Verilog/SystemVerilog 区分大小写。
必需参数检查将 positional 前缀按声明顺序映射，并以 named formal 覆盖其余参数；带默认值的
参数无需实例覆盖。positional 数量超过声明数量按 elaboration error 报告。Verilog/SV 禁止
在同一 override list 混用两种风格；VHDL 允许 positional associations 后接 named
associations。当前不求值 override 表达式，也不比较参数类型或取值范围。

VHDL 未使用 generic 检查是项目级分析：按大小写不敏感名称将唯一 entity 与本次输入中的全部
architecture 关联，再扫描 entity 接口及各 architecture 的显式范围。若输入中没有该 entity 的
architecture，或 entity 定义不唯一，规则会保守跳过，避免对不完整工程输入产生误报。

连接形状模型会按顶层逗号拆分端口 actual，忽略表达式、函数调用和 aggregate 内部逗号；空
连接列表计为 0。positional actual 按目标端口声明顺序映射，空段和 VHDL `open` 保留为未连接
状态；必需 input 缺失时复用 `STRUCT-UNCONNECTED-INPUT-PORT`。只有 actual 超过接口端口数
才产生 count warning，因此尾部省略 output 不产生数量告警，而由独立的未连接输出 info 表达；
带默认值 input 不报。Verilog/SV 同一实例
混用两种风格按语法错误报告；VHDL 允许 positional associations 后接 named associations，
并继续组合两种映射检查必需 input。

已解析目标的 output/inout 使用同一 named、positional 与 VHDL mixed 映射模型检查连接状态；
遗漏、空 actual 或 `open` 产生 info，并优先定位显式悬空的 association。Verilog `.*` 因实际
连接依赖父作用域名称而保守排除；无接口模型、未解析 IP 与 black box 不做猜测。该规则用于
发现可能被意外丢弃的状态和握手输出，明确有意不用的输出可局部豁免。

VHDL mixed association 进一步保留源码顺序：合法形式必须先列出所有 positional associations，
再列 named associations。named 后出现 positional 会触发独立顺序错误；若 positional 前缀已按
声明顺序覆盖某 formal，后续 named association 再次指定同一 formal，则复用 duplicate port /
parameter 规则报告。未知 formal 仍由对应 unknown 规则独立表达。

接口声明唯一性检查在保留首个有效 port/parameter/generic 模型的同时记录后续重复声明来源，
因此不会用重复节点污染实例映射，却能在第二次声明位置产生 error。Verilog/SystemVerilog
名称区分大小写；VHDL port 与 generic 名称按大小写不敏感语义比较。legacy non-ANSI module
header 与其正常 module-scope 方向声明仍视为同一个端口建模过程，不会被当作重复接口。

内部信号唯一性检查定位后续重复声明：Verilog/SystemVerilog 对名称大小写敏感，并将同一声明
中的复合类型关键字视为一个声明起点；ANSI port 与后续内部声明同名会报告，而合法的 legacy
port 方向声明与后续 net 类型补充不会误报。
VHDL 名称按大小写不敏感比较，architecture signal 与可见 entity port 同名也会报告。当前检查
设计单元顶层声明，不深入 function/task、named block 或 VHDL subprogram 的局部作用域。

当前 CDC 规则只证明同一 module/entity 内的直接时序写入到时序读取关系。它不会穿越实例端口，
不会自动认定两级同步器、握手或异步 FIFO 的正确性；索引访问被视为潜在 RAM/
数组通道并保守排除。因此该规则适合作为早期风险筛查，不能替代 CDC 签核。

当前 RDC 规则只检查同一 module/entity 内同一个异步复位是否直接关联到多个不同时钟边沿域。
它不追踪跨实例复位网络，也不认证异步置位/同步释放电路，因此同样不能替代 recovery/removal
时序检查或完整 RDC 签核。

未使用输入端口检查基于同一语义读写模型：端口只要被表达式、过程条件、敏感列表或实例 actual
引用，就视为已使用。由于单文件语义阶段并不知道下级实例端口方向，任何实例连接均按接口透传
保守接受；未连接、未被内部非法驱动且完全未读的 input/in 才产生 info，避免与更高优先级的
`RTL-INPUT-DRIVEN` 形成级联噪声。预留引脚可使用局部规则豁免保留设计意图。

组合敏感列表检查只面向显式、非边沿触发的 Verilog/SystemVerilog `always @(...)` 与 VHDL
`process(...)`。它以过程体真正读取的已声明信号集合为准，同时报告缺失项和冗余项；同一信号
在过程内既写又读时，右值读取仍会被保留。`@*`、`@(*)`、`always_comb`、`process(all)` 以及
时钟过程不需要人工维护列表，因此不运行这两条规则。当前 token 模型尚不展开被调用
function/procedure 的隐式读取，复杂调用可改用自动敏感构造或局部豁免。

常量条件求值覆盖过程分支/循环和运行时条件表达式。Verilog/SystemVerilog 支持整数文字的真假值、完整外层
括号、逻辑 `!`、`&&`、`||` 和两个整数文字间的 `==`/`!=`/`===`/`!==`/关系比较；VHDL
支持 boolean/十进制文字、括号、`not`/`and`/`or` 及文字比较。`false && runtime`、
`true || runtime` 一类结果可证明的恒等式会报告，`runtime || false` 等仍依赖运行时值的
表达式不会报告。当前不求值参数、generic、常量声明、算术表达式或函数调用，也不检查
generate/elaboration 条件。三目和 VHDL when-else 复用比较位宽检查的结构化 guard 范围；
只接受过程范围、continuous assignment 或并发 signal assignment，parameter/generic 等
elaboration 表达式不报告。

比较位宽检查在运行时 `if`/`elsif`、两个 HDL 的 `while`、Verilog/SystemVerilog `for` 中间
条件、括号包围、位于过程/连续赋值右侧、链式嵌套或处于 `if` 等包围表达式中的三目运算
guard，以及 VHDL
`value when condition else value` 条件中分析
Verilog/SystemVerilog `==`、`!=`、`===`、`!==`、`==?`、`!=?`、
`<`、`<=`、`>`、`>=` 和 VHDL 对应比较运算。两侧必须都能完整化简为已声明且宽度已知的
直接信号、常量 bit/index/slice、
显式宽 Verilog literal、VHDL 字符串/逻辑字符，或成员宽度均可求值的静态拼接；`&&`/`||`
与 VHDL 布尔连接会拆成独立子条件。等宽比较不报，算术、调用、动态选择和无尺寸 literal
保持排除，避免只截取复杂表达式的一部分产生误报；Verilog `<<`/`>>`/`<<<`/`>>>`
会按移位符号排除，不会拆成关系比较。
完整包围比较的 Verilog/SystemVerilog `!(...)` 与 VHDL `not (...)` 会剥离后分析内部条件；
未包围完整余下表达式的前缀取反不会改写，避免改变运算符优先级语义。
无括号三目 guard 优先在可确认的 `=`/`<=` 赋值运算符之后提取，也可在明确的外层圆括号内
保守提取；非阻塞赋值与 guard 内的 `<=` 关系比较分别判定，SystemVerilog `==?`/`!=?`
也不会被误当成三目起点。
链式三目会在同级前序 `?` 或匹配分支 `:` 之后重新确定当前 guard 起点；括号、选择和拼接
内部的分隔符不参与切分。没有赋值起点时，仅使用可确认的外层圆括号作为保守边界。

Verilog/SystemVerilog 条件副作用检查只扫描运行时 `always`/`always_*`/`initial` 中的
`if`、`while` 与三目 guard。它识别普通赋值、算术/位复合赋值、`<<=`/`>>=` 以及前缀或后缀
`++`/`--`；同一副作用 token 被外层条件和内部三目同时覆盖时只报告一次。`==`、`===`、
`!=`、`!==`、`<=`、`>=` 和其他纯比较不会命中。函数调用是否有隐藏副作用需要过程间语义，
因此当前不猜测。

空条件分支检查覆盖运行时 `if`、`else if` 与最终 `else`。单独的 `;`、空 `begin/end`、
空命名块以及只含分号的块会报告，并定位到分号或块起点；带赋值、调用、嵌套条件或其他
token 的分支视为非空。显式空分支若用于表达设计意图，可添加说明并局部豁免该规则。

空 case 项复用相同的空语句/块判定，覆盖 `case`、`casez`、`casex`、`default`、逗号组合
choice 和嵌套 case。case 项后的单个 `;` 会作为该项自己的完整语句消费，不再被流程模型
跳过并错误吸收下一项；这同时保持 latch 完整赋值分析和 default 识别的分支边界准确。

重复 catch-all 检查在每个 case 作用域内独立维护状态：Verilog/SystemVerilog 的第二个及后续
`default`、VHDL 的第二个及后续 `when others` 均报告 error，诊断定位后出现的 catch-all。
嵌套 case 各自允许一个 catch-all，不会跨作用域合并；普通 choice 的重复仍由
`RTL-DUPLICATE-CASE-ITEM` 单独表达。

case 位宽检查在 selector 可化简为已声明、宽度已知且非 memory 的直接信号、常量位选/索引
或常量切片时运行。Verilog/SystemVerilog 仅比较带显式尺寸的整数 literal，unsized literal 及
参数/表达式 choice 保守跳过；VHDL 仅比较直接字符串 choice。动态索引、拼接、函数调用和其他
复杂表达式暂不推断结果宽度。诊断同时给出完整 selector 表示、推导宽度和 choice 宽度。

Verilog case choice 值域检查复用相同 selector 形状，只对单 token、无显式尺寸、可求值且位于
非负 `int` 范围的整数 literal 运行。当值大于等于 `2^selector_width` 时，该 choice 不对应
selector 的任何 bit pattern，因而按 warning 报告。负数、parameter/localparam、带显式尺寸
的 literal、含 X/Z 的值和复杂表达式保持排除；31 bit 及以上 selector 也保守跳过，避免在
当前尚未保存 signedness 的模型中猜测无尺寸整数的符号扩展语义。

常量移位范围检查复用同一直接/常量索引/切片宽度模型。Verilog/SystemVerilog 覆盖逻辑与
算术左右移位以及 `<<=`/`>>=`/`<<<=`/`>>>=`；VHDL 覆盖 `sll/srl/sla/sra` 和直接
`shift_left/shift_right(operand, literal)`。只有右操作数是可解析非负整数 literal 时运行；
动态移位、参数移位、旋转和复杂操作数保守跳过。移位量等于位宽也会报告，因为所有原始位
已经离开结果范围。

负移位量检查覆盖相同的 Verilog/SystemVerilog 二/三字符移位运算符及复合赋值、VHDL
`sll/srl/sla/sra`，以及 `shift_left/shift_right(operand, amount)` 调用。仅当剥离完整外层
括号后恰好剩下 `-` 与非零整数 literal 时报告；`-0`、动态值、参数以及 `(0 - 1)`、
`(-1 + 2)` 等尚未常量折叠的表达式保守跳过。扫描器一次消费完整移位运算符，避免 `>>>=`
内部重叠 token 重复报告。

单进程多时钟规则检查 Verilog/SystemVerilog sensitivity list 中的多个边沿事件，以及 VHDL
process 内出现的多个 rising_edge/falling_edge 调用或经典 `signal'event` + 电平比较。常见名称的异步 reset、clear 和 preset
只在多边沿敏感列表中作为控制事件排除；普通过程体中的 clear/busy/done 等信号不会被扩展为
复位。同一信号的双边沿由 `RTL-MIXED-CLOCK-EDGE` 表达，不重复报为多个时钟。该规则不替代
综合器对专用原语或非常规模板的合法性判断，必要时可按规则局部豁免。

未使用异步复位规则比较每个过程自己的异步 reset 关联与过程体 reads。Verilog/SystemVerilog
从多边沿 sensitivity event 中识别 reset/rst 以及常见 clear/preset；VHDL 从含 edge 函数的
时序 process sensitivity list 中识别相同命名特征。信号只出现在敏感列表、过程体没有任何读取
即报告；通过条件、过程调用参数或表达式读取都会避免命中。自定义且不带这些命名特征的异步
控制需要后续属性/约束机制或显式命名规则支持。

CDC 同步器识别基于目的时钟域内的直接寄存器传递图：异步源必须只直接进入内部第一级，
第一级只能由目的域写入、不能参与复杂表达式，并且至少直接进入一个同域第二级。异步源在
目的域存在组合表达式读取、任一第一级没有后继级、或第一级暴露为接口端口时仍报告 crossing。
当前只认证标量直接赋值链；packed shift-register、属性驱动、跨层次及握手/FIFO 结构仍保守报告。
认证成功的链不会静默丢弃：设计模型和报告保留源信号、两级寄存器、目的时钟/边沿及来源位置，
便于审核自动排除结果。

VHDL 时序规则在项目级按大小写不敏感 entity 名称合并所有已分析 architecture 的时钟、复位、
过程访问和标量传递信息；同文件已提取的过程按来源位置去重。entity 定义不唯一或 architecture
目标不在本次输入时保守跳过关联，避免把时序信息绑定到错误设计单元。

异步复位极性一致性规则只检查带至少两个边沿事件的 Verilog/SystemVerilog `always`/`always_ff`，
并要求过程首个可执行语句是直接复位 `if`。规则支持裸信号、逻辑/按位取反、以及与常量 0/1 的
`==`、`===`、`!=`、`!==` 比较；只有纯复位条件才进一步比较事件边沿与有效电平。

异步复位首分支规则复用同一纯条件判定，并扩展到 VHDL edge process。若首个可执行语句不是
`if`，或首个条件是 enable/功能条件、包含复位的复合布尔表达式、或测试了另一个异步控制，
则在敏感列表/事件控制中的复位信号位置报告 error。VHDL 支持裸布尔复位、`not reset` 以及
与 `'0'`/`'1'`、`true`/`false` 的 `=`/`/=` 比较；自定义异步控制仍受 reset/clear/preset
命名识别边界约束。SystemVerilog 块开头的内建类型局部声明会在寻找首个可执行语句时跳过。

异步复位守卫完整性规则只在首条件已被证明为纯复位条件且极性与事件边沿一致后运行，因此不会
与首分支或极性错误级联。它定位完整外层 reset `if/else`（VHDL 为 `if/elsif/else`）链的结尾，
若过程结束前仍有独立过程赋值则报告；嵌套条件和 begin/end 会保持各自边界。只出现 assertion、
过程调用或 `null` 而没有可证明赋值时保守跳过，避免把未知副作用误判为复位值覆盖。

隐式网络检查当前仅面向 Verilog：报告 `assign undeclared = declared`、右值恰好为一个未声明
标识符的直接赋值，以及 named/positional module 连接中恰好为一个未声明标识符的实际信号。
参数、localparam 和 genvar 不作为网络报告，复杂表达式、拼接、位选、宏展开后复杂形式和
SystemVerilog 用户类型暂不猜测。建议工程统一使用显式 wire 声明，并在允许时启用
`` `default_nettype none``；完整符号表落地后再扩展到表达式内部和 SystemVerilog。

组合环分析连接连续赋值和组合过程赋值目标到右值中的已声明信号，并按强连通分量去重报告；
时序过程不会进入组合图。Verilog 组合过程中的阻塞赋值遵循过程内顺序：已有赋值后的自引用
可作为临时累加器，`for(...)` 控制变量也被排除。当前尚不展开 function/task，也不把控制条件
依赖完整纳入图，因此复杂过程间或跨实例组合环仍需后续完整 AST/elaboration 支持。

位宽截断规则检查右值恰好为单个已声明信号，且两侧宽度都能由十进制常量范围精确求值的
赋值；也接受剥离完整冗余括号后为单一 Verilog 显式 sized integer literal 或 VHDL 字符串
literal 的 RHS。literal 宽于目标时，Verilog 会丢弃高位，VHDL 则可能违反数组长度约束。
目标宽度可来自直接信号、Verilog 常量 bit/slice 或 VHDL 常量 index/slice；诊断展示完整
selected target。信号 RHS 使用相同形状模型，可将 `data[7:4]` 或 `data(7 downto 4)` 的选择
宽度与直接/selected target 比较，并在诊断中保留完整来源表示。成员均为上述静态形状或显式
宽 literal 时，还会累加 Verilog/SystemVerilog 简单与嵌套 `{...}`、常量 `{N{expr}}` 复制及
VHDL `&` 拼接的宽度；VHDL 成员支持字符串和标准逻辑标量字符 literal。动态选择、
动态/参数化复制次数、算术、一元符号、类型转换、参数表达式、
无尺寸填充值及无法识别的用户类型不会被猜测；当前也不判断
符号扩展。该保守边界减少噪声，后续表达式类型系统可扩大覆盖范围。

越界选择规则为单维、常量声明范围保留最小和最大合法索引，不把 `[15:8]` 错当成 `[7:0]`；
Verilog/SystemVerilog 支持十进制及无 X/Z 的 binary/octal/decimal/hex based integer 选择，
VHDL 支持十进制 index 和 `to`/`downto` slice。动态索引、参数表达式范围、多维 packed array、
unpacked memory 以及用户定义数组类型暂不猜测。只要常量位索引或任一切片端点越界即报告一次。

派生时钟规则将边沿事件中的已声明时钟信号与本地运行时 driver 关联；input/inout 以及只有
实例连接、没有本地赋值的信号不报。它能发现常见 `assign gated_clk = clk & enable` 和本地
分频寄存器时钟。内联 `posedge (clk & enable)`、`posedge bus[0]` 及 VHDL 对应形式由独立
复杂边沿规则报告；多层括号包裹的单一标识符仍会解包进入时序、复位及 CDC 模型。复杂表达式
不会被伪装成其第一个标识符对应的时钟域。当前仍不识别每种厂商时钟 primitive 的合法性；
合法的专用 primitive 应通过实例输出连接时钟网络。

VHDL 时序域提取同时支持 `rising_edge`/`falling_edge` 与传统
`clock'event and clock = '1'/'0'`。传统模板要求同一个直接标量信号同时参与 `event` 属性和
电平相等比较，允许两部分交换顺序并按大小写不敏感名称匹配；缺少 `and`、信号名不一致或
电平不是 bit 0/1 时保守跳过，不猜测时钟域。

仿真延时检查只在 Verilog/SV 的过程范围或 `assign ...;` 范围内识别 `#`，因此 module/instance
参数覆盖 `#(...)` 不会命中；VHDL 则检查 architecture 中的 `after`。这些延时通常只适用于
testbench 或仿真模型，综合 RTL 应改用时钟计数、状态机或时序约束。VHDL `wait` 继续由独立
`VHDL-WAIT` 规则覆盖。

VHDL process suspension 检查按 architecture 内独立 process 范围执行：`process(...)` 与
`process(all)` 具有隐式挂起点，不报；无 sensitivity list 时，只要过程范围内存在显式 `wait`
也不报。两者都不存在时，process 会在 `end process` 后立即重新开始并形成无限 delta-cycle，
因此按 error 报告。当前对 process 内局部 subprogram 中出现的 wait 采取保守接受策略。

带 sensitivity list 的 process 已由列表隐式定义挂起点，VHDL 不允许其 statement part 再出现
显式 `wait`。检查按 process token 范围定位直接 wait，并跳过局部 procedure/function body，
避免把局部声明中的语句归到外层进程；命中时只报告 `VHDL-WAIT-WITH-SENSITIVITY` error。
通用 `VHDL-WAIT` 在同一 token 层扫描 architecture，除去已被 legality error 占用的 token，
因此跨行 `wait until/on/for`、裸 `wait;` 及局部 subprogram 内 wait 均能稳定表达综合可移植性风险。

VHDL 非标准数值包检查限定在 token 化 `use ...;` clause 内，仅匹配
`std_logic_arith/std_logic_unsigned/std_logic_signed` package 标识符；换行和点号布局不影响识别。
`shared variable` 以相邻关键字 token 识别，architecture 声明区内保留 entity 上下文，package
等其他声明区仍会报告但不猜测设计单元。注释与字符串不会进入这两类规则。

SystemVerilog 专用过程控制检查使用已分界的 procedure token 范围，而不是逐行正则：
`always_comb` 与 `always_latch` 范围内发现任何显式 `@` 即分别报告；`always_ff` 对整个过程统计 event control，
0 个定位过程关键字，多个定位第二个 `@`；恰好 1 个时再检查控制范围内至少包含一个
`posedge`/`negedge`，因此 `@(clk)` 与 `@(*)` 由独立 error 表达。规则仅对 SystemVerilog 文件启用，
不会把 `.v` 中的兼容性/拼写问题误分类为 SystemVerilog 语义诊断；它也不替代完整 LRM 对
过程内其他 blocking timing control 的检查。

`always_latch` 意图检查复用组合过程的确定赋值模型：只要至少一个可赋值信号在某条控制流
路径上保留原值，就接受为锁存器；若所有赋值均为 definite，或过程未赋值任何已声明信号，
则提示改用 `always_comb` 或修正锁存逻辑。memory 与 input 赋值不作为证明锁存器成立的依据，
task/function 内部赋值仍受当前非过程间分析边界限制。

混合赋值风格检查在每个 Verilog/SystemVerilog 过程内独立跟踪已声明信号的 `=` 与 `<=`，
首次遇到另一种调度方式时报告，同一信号同一过程最多报告一次；位选和切片按其基信号归并。
不同过程各自采用一致但不同的风格不会命中本规则，而由多驱动等独立规则表达跨过程风险；
`for` 控制头赋值作为迭代语法排除，function/task 的独立过程作用域仍待完整 AST 支持。

顺序阻塞赋值与组合非阻塞赋值规则已迁移到同一 token 化过程模型：赋值目标、运算符和右值
可以跨行，嵌套 `if/else`、case、循环及 begin/end 仍保持正确过程归属。左值必须位于合法语句
起点，因此条件或右值中的关系运算 `<=` 不会被误识别为非阻塞赋值；`for` 控制头继续排除。
当前不展开 task/function 调用内部的赋值效果。

Verilog/SV 的 `casex` 使用、缺失 `default`、重复 choice 与通配模式相交均共享同一 token 化
case 作用域；多行 selector、同一行完整 case 及嵌套 case 均独立分析，不会让内层 `default`
错误满足外层。choice 分析对 Verilog/SV 的逗号选择和 VHDL 的 `|` choices 分别拆分并比较
规范化 token 序列；`default`/`others` 不作为普通值比较。完全相同的 choice 由 duplicate
规则处理；不同 choice 的 wildcard overlap 目前只对
宽度一致、可完整解析的 sized binary/hex 常量执行位级求交。`casez` 将 `z/?` 视为通配，
`casex` 将 `x/z/?` 视为通配；`casez` 中包含字面 `x` 的模式保守跳过。不求值参数、枚举别名
或任意表达式。

重复条件规则只比较同一过程、同一 `if/else if` 或 `if/elsif` 链中的精确 token 序列，并移除
包住整个条件的冗余括号。嵌套链会建立自己的比较集合，相邻独立 if 不共享状态；VHDL token
比较统一转为小写，Verilog/SystemVerilog 保持大小写敏感。规则不尝试证明德摩根等价、交换律、
常量折叠或参数值等语义等价，从而避免对只是逻辑上相似的条件产生猜测性告警。

锁存分析覆盖常见 `begin/end`、if/elsif/else、case/default/when others 和循环结构；循环体
按可能零次执行处理。当前不做跨 task/function 的过程间数据流，也不证明两个独立条件在逻辑上
互补，复杂写法应使用过程开头默认赋值或完整分支以获得稳定结果。
