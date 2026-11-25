# FPGA HDL Static Analyzer — V1.4.233

[![Windows Build](https://github.com/silytoxic-arch/FPGA-HDL-Static-Analyzer/actions/workflows/windows-build.yml/badge.svg)](https://github.com/silytoxic-arch/FPGA-HDL-Static-Analyzer/actions/workflows/windows-build.yml)

公开源码仓库使用 GitHub Actions 在 Windows Server 2022、Visual Studio 2022 和 Qt 6.8.3 环境中自动编译、运行全部测试并生成 Windows x64 ZIP。可在仓库的 **Actions → Windows Build** 页面手动触发，构建完成后从 `FPGA-HDL-Static-Analyzer-Windows-x64` 工件下载。

V1.4.233 深化官方三段式规则 `R-6-3`：复位名称表支持参数和整数常量表达式数据，并补齐直接标量与宽常量比较的准入；保留运算节点被位宽提示扩展时的未知边界。详见 [本轮记录](docs/v1_4_233_release.md)。

V1.4.214 深化官方三段式规则 `R-5-5`：恢复两条原生诊断正文、Error、赋值物理行无列定位，以及“目标名 + 物理行”的报告重数；确认启用 `initial` 时 DLL 仅登记赋值目标而不校验 RHS 固定性。复杂 default/reset 分支继续要求人工复核。详见 [本轮记录](docs/v1_4_214_release.md)。

V1.4.213 深化官方三段式规则 `R-5-3`（与 `A-5-2` 共用原生核心）：确认两个构造器均选择同一模式，恢复精确正文、Warning、赋值物理行无列定位和同一行多驱动重数；未恢复的专有复位关系生产过程继续要求人工复核。详见 [本轮记录](docs/v1_4_213_release.md)。

V1.4.212 深化官方三段式规则 `R-5-2`：恢复单边沿过程筛选、首个条件、逐标识符报告、精确原生正文及物理行无列定位；未恢复的跨层次复位关系继续要求人工复核。详见 [本轮记录](docs/v1_4_212_release.md)。

V1.4.197 深化官方三段式规则 `R-2-3`：恢复逐模块组准入、七类声明候选、六类关键字表顺序、固定 Error、六类精确正文、声明行及原生无列定位。详见 [本轮记录](docs/v1_4_197_release.md)。

V1.4.193 深化官方三段式规则 `R-2-5`：恢复按源文件聚合模块、重复名称不去重、每文件单条告警、固定 Warning、完整模块名列表、英文逗号与句点正文以及原生无列定位。软件界面及全部报告仍只显示官方 `R-X-X / A-X-X` 三段编号，内部 GJB 条款键仅用于实现追溯。详见 [本轮记录](docs/v1_4_193_release.md)。

V1.4.192 深化官方三段式规则 `R-2-4`：恢复唯一模块组边界、六类声明、五个电源保留名的精确折叠比较、每节点多重性、固定 Error 正文和原生无列定位；非 ASCII 区域设置及完整宏/include/generated 前端继续保持人工复核。详见 [本轮记录](docs/v1_4_192_release.md)。

V1.4.181 深化官方规则 `A-16-1`：恢复 `\full_case` 精确 AST 属性键、Yosys hot-comment 词法边界、Warning 级别和原生动态消息，并消除属性值字符串与无证据注释生产者造成的误报；未知前端附着路径继续人工复核。详见 [本轮记录](docs/v1_4_181_release.md)。

V1.4.178 深化官方规则 `R-14-3`：恢复内部 `init` 关系的真实生产路径、then/else 非对称边界、Warning 级别和原生完整消息；规则矩阵中的 209 条编号也统一为官方三段式 `R-X-X` / `A-X-X`。详见 [本轮记录](docs/v1_4_178_release.md)。

V1.4.172 恢复 `GJB10157 / R-12-3` 的双门判定：FOR 内 if 条件必须直接命中任一边沿事件名称，并具有完整 else/default；移除缺少原生依据的低有效、双边沿和同目标赋值限制。未知前端降低路径仍保持人工复核。详见 [本轮记录](docs/v1_4_172_release.md)。

V1.4.169 深化恢复 `GJB10157 / R-12-4`：确认 `is_count_zero` 来自 FOR 条件的首次前端常量化结果，并补齐参数默认值、定宽整数与对象数值宏边界；未知折叠路径仍保持人工复核。详见 [本轮记录](docs/v1_4_169_release.md)。

V1.4.166 深化恢复 `GJB10157 / R-11-2`：只对首个 AST 子节点为常量/实数常量的完整 `case/casex/casez` 报告，完整空 case 仍报告，不完整 case 不猜测；诊断改为原生固定消息，同时保留强制规则配置的 Error 严重度和未知路径人工复核。详见 [本轮记录](docs/v1_4_166_release.md)。

V1.4.165 深化恢复 `GJB10157 / R-11-6`：DLL 只检查每个 case item 的第一个 AST 子节点，逗号后的后续选择值不参与本规则决策；产品已消除“常量在前、变量在后”的过度报告，并保持未知常量折叠路径人工复核。详见 [本轮记录](docs/v1_4_165_release.md)。

V1.4.164 深化恢复 `GJB10157 / A-11-2`：原生规则按含 `AST_CONDX/AST_CONDZ` 子节点的 CASE 根报告，每个非空 `casex/casez` 最多一次；空 case 与不完整 case 不再仅因关键字出现而误报，且无告警仍保持人工复核。详见 [本轮记录](docs/v1_4_164_release.md)。

V1.4.163 重新反编译并校准 `GJB9765 / A-5-2` 的原生名称长度候选集：不再只检查端口和 `signal`，新增实体、结构体、类型、子类型、泛型、变量、常量、函数、过程、进程/块标号等证据支持类别，并保留 `AST_BLOCK` 对 21/22/23 字节的专有边界。公开编号继续严格采用官方三段式。详见 [本轮记录](docs/v1_4_163_release.md)。

V1.4.162 恢复 `GJB10157 / R-5-3` 与 `A-5-2` 的原生诊断重数：访问集合仅对关系名称去重，每个非别名赋值节点独立报告，同一物理行不合并；多条别名路径汇聚到同一上游名称时仍只遍历一次。公开编号继续严格采用官方三段式。详见 [本轮记录](docs/v1_4_162_release.md)。

V1.4.161 恢复 `GJB10157 / R-5-3` 与 `A-5-2` 的原生多级复位别名工作集：直接及单目反相别名按访问集合递归追溯至上游组合驱动，循环别名安全终止；新增连续/过程上游正例、混合反相链及循环边界，未知层次仍人工复核。公开编号继续严格采用官方三段式。详见 [本轮记录](docs/v1_4_161_release.md)。

V1.4.160 将已恢复的 `AST_LOGIC_NOT(0x47)` / `AST_BIT_NOT(0x24)` 直接子节点约束落实到 `GJB10157 / R-5-3` 与 `A-5-2` 的回归边界：单级 `!`、`~` 和标识符选择作为复位别名，双重 NOT、函数调用与归约根按组合驱动报告；无告警仍保持人工复核。公开编号继续严格采用官方三段式。详见 [本轮记录](docs/v1_4_160_release.md)。

V1.4.159 精确恢复原生 `AST_BIT_NOT(0x24)` 与 `AST_LOGIC_NOT(0x47)` 枚举及宽度类别，收敛 `GJB9765 / A-9-6` 的直接反相结构：只接受单一标识符子节点，新增 `std_logic` 正例以及索引、调用、双重 NOT、一元负号边界；未知 VHDL 重载选择继续人工复核。公开编号继续严格采用官方三段式 `R-X-X` / `A-X-X`。详见 [本轮记录](docs/v1_4_159_release.md)。

V1.4.158 接入 `GJB9765 / A-9-6` 的递归用户组件分支：对唯一解析实体、简单标识符命名/位置端口关联及直接并发直通/反相赋值构建跨文件多层关系图，循环层次安全终止；复杂实参、配置绑定和歧义/外部实体继续人工复核。公开编号继续严格采用 `R-X-X` / `A-X-X` 三段式。详见 [本轮记录](docs/v1_4_158_release.md)。

V1.4.157 恢复 `GJB9765 / A-9-6` 的原生每时钟诊断聚合：直接反相结果与全部 component 缓冲/反相结果先按位合并，同一时钟只报告一次，并区分仅反相、仅缓冲和两者同时存在；两个独立时钟仍分别报告。公开编号继续严格采用 `R-X-X` / `A-X-X` 三段式。详见 [本轮记录](docs/v1_4_157_release.md)。

V1.4.156 继续恢复 `GJB9765 / A-9-6` 的 component 层次分支：接入已解析用户实体的一层跨文件时钟缓冲/反相路径，恢复专用时钟单元白名单，并新增命名端口、位置端口及时序组件排除测试。公开编号继续严格采用 `R-X-X` / `A-X-X` 三段式。详见 [本轮记录](docs/v1_4_156_release.md)。

V1.4.152 将原生 `FUN_1804c5890` 的直接进程边界接入 `GJB9765 / A-9-3`：时钟关系名与寄存器赋值候选均只来自 architecture 直接时序进程，generate/block 内进程不再从消费侧或产生侧造成行波时钟误报。公开编号继续严格采用 `R-X-X` / `A-X-X` 三段式。详见 [本轮记录](docs/v1_4_152_release.md)。

V1.4.151 将原生 `FUN_1804c5890` 的“仅 architecture 直接 process”边界继续接入 `GJB9765 / R-9-4`：generate/block 内进程不再被错误纳入“同一进程只使用一个时钟”检查；新增两类嵌套边界测试，并保留未知路径人工复核。公开编号继续严格采用 `R-X-X` / `A-X-X` 三段式。详见 [本轮记录](docs/v1_4_151_release.md)。

V1.4.150 将原生 `FUN_1804c5890` 已确认的“仅 architecture 直接 process”边界继续接入 `GJB9765 / R-9-2`：generate/block/子程序内进程不再建立外溢时钟关系，也不再作为 IF/CASE 控制候选产生误报；新增关系端与候选端两组边界测试。公开编号继续严格采用 `R-X-X` / `A-X-X` 三段式。详见 [本轮记录](docs/v1_4_150_release.md)。

V1.4.149 继续收敛 `GJB9765 / R-9-1`：原生 `FUN_1804c5890` 被确认只收集架构直接进程，不递归进入 generate/block；产品据此排除内嵌进程对时钟再汇聚关系集的错误扩充，并增加对应边界测试。公开编号继续严格采用 `R-X-X` / `A-X-X` 三段式。详见 [本轮记录](docs/v1_4_149_release.md)。

V1.4.143 继续完善 `GJB9765 / R-9-2` 的原生诊断重数：候选追加器不跨条件去重，因此同一时钟出现在不同 IF/CASE 条件或条件赋值的不同 WHEN 条件中时分别报告；单个条件内部仍按名称去重。公开编号保持官方三段式 `R-X-X` / `A-X-X`，未知前端形态继续人工复核。详见 [本轮记录](docs/v1_4_143_release.md)。

V1.4.142 继续完善 `GJB9765 / R-9-2` 的共享关系所有权：枚举进程语句层的全部直接来源 IF，按顺序处理全部直接 ELSIF，并只采用 ELSE 中最后一个直接链接 IF 的主条件。类别1仍只用首个边沿名建立时钟集合，额外边沿用于分类与源码行排除。未知前端形态继续人工复核，公开编号保持官方三段式。详见 [本轮记录](docs/v1_4_142_release.md)。

V1.4.138 将所有用户可见 GJB 规则编号统一为官方三段式 `R-X-X` / `A-X-X`。问题列表、CLI、文本、HTML、JSON 与 SARIF 报告不再显示 `GJB9765.4.8.e` 一类内部条款键；规则判断逻辑和内部兼容键保持不变。详见 [本轮记录](docs/v1_4_138_release.md)。

V1.4.137 继续深化 `GJB9765 / R-9-4` 类别 1：恢复直接 ELSIF 的顺序状态、附加边沿记录以及 ELSE 中最后一个直接链接 IF 的选择规则。静态动作可在首边沿前或首边沿与第二边沿之间建立类别 1，但第二边沿之后的静态分支不再把类别 4 提升为类别 1；链接 IF 只采用主条件，不借用其 ELSIF。公开编号继续严格采用官方三段式。未调整界面或表格。详见 [本轮记录](docs/v1_4_137_release.md)。

V1.4.136 继续深化 `GJB9765 / R-9-4`：恢复类别 2 的来源 IF 固定层级、外层主条件时钟所有权和链接 IF 内部记录用途。产品现逐个枚举进程顶层 IF；ELSIF、分支嵌套 IF、CASE/LOOP 内 IF 不再冒充类别 2 新时钟，多个独立顶层 IF 仍会进行进程内比较。公开编号继续严格采用官方三段式 `R-X-X` / `A-X-X`。未调整界面或表格。详见 [本轮记录](docs/v1_4_136_release.md)。

V1.4.135 继续深化 `GJB9765 / R-9-4`：恢复类别 1 静态异步动作辅助函数，接入直接字面量、全字面量拼接、首项 `others` 聚合，以及所属实体泛型、architecture 常量和枚举文字；同时排除 process 局部常量与局部 package 常量。公开编号继续严格采用官方三段式 `R-X-X` / `A-X-X`。未调整界面或表格。详见 [本轮记录](docs/v1_4_135_release.md)。

V1.4.119 纠正 `GJB9765 / R-6-4` 的 `+0x10e` 语义：全映像写入者证明它是 `boolean` 类型标志，不是常量折叠标志；产品现按原生分支在标识符依赖检查前接纳直接 `boolean` 常量初始化式，消除运行期 boolean 对象和关系表达式造成的误报。公开编号继续严格采用官方三段式。未调整界面或表格。详见 [本轮记录](docs/v1_4_119_release.md)。

V1.4.118 继续深化 `GJB9765 / R-6-4`：恢复 VHDL 常量产生式对 `+0x178` 的写入，按原生零子节点分支认可 deferred constant；同时确认递归 AST 收集器不按名称去重，保留整数赋值 RHS 重复拒绝常量的逐节点诊断重数。公开编号继续严格采用官方三段式。未调整界面或表格。详见 [本轮记录](docs/v1_4_118_release.md)。

V1.4.117 继续深化 `GJB9765 / R-6-4`：恢复原生 `+0x109` 整数 signal/variable 与 `AST_ASSIGN_LE(0x56)` 二次定位分支；当整数目标的 `<=` 右侧引用同一设计根内已拒绝常量时，在赋值点追加问题定位，并复现声明同行过滤。公开编号继续严格采用官方三段式。未调整界面或表格。详见 [本轮记录](docs/v1_4_117_release.md)。

V1.4.116 继续深化 `GJB9765 / R-6-4`：依据原生入口无 architecture 类型门控的证据，把独立 package specification、package body 及其子程序常量纳入根级静态性检查；各 package/architecture 根独立建表，包可见性和无初始化节点标志仍保持人工复核。公开编号继续严格采用官方三段式。未调整界面或表格。详见 [本轮记录](docs/v1_4_116_release.md)。

V1.4.115 修复 `R-6-4` 的完整 architecture 根终止边界，支持 `end architecture`、`end <architecture-name>` 和裸 `end;`，并区分子程序与本地 package 的结束；详见 [先前记录](docs/v1_4_115_release.md)。

V1.4.114 将 `R-6-4` 从 architecture 声明区扩展到完整 architecture 设计根，覆盖 process、block 和 function/procedure 局部常量，并在显式 architecture 终止符处隔离状态；详见 [先前记录](docs/v1_4_114_release.md)。

V1.4.112 继续深化 `GJB9765 / R-6-2`：按 DLL 已恢复的固定顺序实现五类宽度名称查询 `constant → port → generic → signal → variable`，接入所属 entity 的固定宽度 vector generic，并增加跨类别重名边界验证。公开规则编号继续严格采用官方三段式 `R-X-X` / `A-X-X`。未调整界面或表格。详见 [本轮记录](docs/v1_4_112_release.md)。

V1.4.111 修正并继续深化 `GJB9765 / R-6-2`：依据类型表明确区分 `AST_ASSIGN_LE(0x56)` 与 `AST_VARASGN(0xa2)`，撤销无原生依据的 `:=` 赋值报告；同时把 architecture/process/subprogram 根内的局部 variable 与形式参数接入关系、位运算及 `<=` 宽度解析。对外规则编号严格采用官方三段式。未调整界面或表格。详见 [本轮记录](docs/v1_4_111_release.md)。

V1.4.110 把 package body 子程序中的形式参数、局部变量/常量、`<=` 赋值、关系比较和位运算纳入独立根检查；package body 与 architecture 的名称表严格隔离。详见 [先前记录](docs/v1_4_110_release.md)。

V1.4.109 依据入口的“清表→遍历整棵设计根→收集比较记录”顺序，把 process、block 和 architecture 内部子程序声明区的可求值 constant 接入同一命名宽度表；独立 package/architecture 根不跨根传播。详见 [先前记录](docs/v1_4_109_release.md)。

V1.4.103 深化 `A-5-32` 的工程绑定边界：`FUN_1806bc5c0` 按已绑定设计根调用 `FUN_1804d0140`，同时传入两个工程解析容器，因此形式端口解析不受物理文件限制。产品现支持父 architecture 的直接 entity 实例绑定另一个文件中的唯一 entity，并覆盖具名和位置实参；目标重名时不猜测，保持人工复核。表达式、`open` 和无本地声明的隐式 component 继续排除。未调整界面或表格。详见 [本轮记录](docs/v1_4_103_release.md)。

V1.4.102 继续深化 `A-5-31`：新反编译的 `FUN_1804ca4c0` 证明同一个原生设计根会联合收集 `AST_PORT` 端口和 `AST_ARCHITECTURE` 直接声明，且没有物理文件过滤。产品因此支持唯一 entity 与独立 architecture 文件的跨文件复位命名图，端口告警保留实体声明文件位置；重名 entity 绑定不唯一时失败关闭并保持人工复核。新增跨文件正例、位置和歧义反例测试，未调整界面或表格。公开编号继续为官方三段式。详见 [本轮记录](docs/v1_4_102_release.md)。

V1.4.101 深化 `A-5-31` 的逐设计根生命周期：恢复 `FUN_1805f5080` 在每个根内清空名称树、重建复位种子和赋值关系容器的边界。产品不再因一个 VHDL 文件包含多个 entity 而关闭别名传播，而是按 `architecture ... of entity` 隔离端口、直接 signal、复位种子及“目标→来源”关系图；跨根名称不会互相污染。新增同文件双设计根的正反边界回归，未调整界面或表格。公开编号继续为官方三段式。详见 [本轮记录](docs/v1_4_101_release.md)。

V1.4.100 深化 `A-5-29` 的命名赋值分支：恢复 `FUN_1804cbad0` 的当前设计根命名值集合，接入实体泛型、architecture 常量及 architecture 枚举文字 RHS；普通信号、process 局部常量和其他 architecture 的常量保持排除。直接字面量也收窄为完整单一 RHS，不再把复合表达式首项误当常量。公开编号继续为官方三段式。详见 [本轮记录](docs/v1_4_100_release.md)。

V1.4.99 修正 `A-5-29` 的下降沿候选归属：原生类别 1 是“主 IF 控制 + ELSIF/ELSE 边沿”，不是裸 `falling_edge`；类别 2 只从主边沿 IF 分支首项的链接嵌套 IF 读取直接主条件分支标志。产品撤销裸下降沿 + 常量赋值误报，并把适配器收窄到已恢复的嵌套 IF 子集。新增裸下降沿、非首项嵌套 IF、ELSIF 借用三个反向边界。公开规则编号仍为官方三段式 `R-X-X` / `A-X-X`。详见 [本轮记录](docs/v1_4_99_release.md)。

V1.4.98 继续深化 `A-5-29`：确认原生 `0xaa/0xab/0xac/0xad` 分别为 `AST_IF / AST_ELSIF / AST_ELSE / AST_ATTRIBUTE_DESIGNATOR`，并证明类别 2 标志来自条件关系对象自身。产品把下降沿适配从“同一进程任意字面量赋值”收窄为“对应 `falling_edge` 的 IF/ELSIF 分支内存在受支持字面量赋值”，消除分支外无关赋值造成的误报，同时保留嵌套 IF 分支。209 条规则编号继续严格采用官方三段式 `R-X-X` / `A-X-X`。详见 [本轮记录](docs/v1_4_98_release.md)。

V1.4.97 深化 `A-5-29`：恢复条件关系分类器及其极性提取辅助函数。DLL 把 `falling_edge(identifier)` 编码为极性 0、`rising_edge(identifier)` 编码为 1，并只对选定类别中的极性 0 记录执行 `_n` 后缀检查。产品在既有低有效复位/置位检查之外，新增受限的“下降沿条件树 + 字面量赋值”分支，同时保留普通下降沿数据传送的排除边界。公开规则编号继续为官方三段式。详见 [本轮记录](docs/v1_4_97_release.md)。

V1.4.95 确认 209/209 条公开规则编号均采用官方三段式 `R-X-X` / `A-X-X`，并从规则清单与规则说明的对外字段中移除内部章节键。规则准确性方面，深化 `A-5-30`：DLL 原生入口只接受一元 `not` 反相结构，且递归收集不限制并发区；产品现覆盖并发与顺序反相。附件 HTML 中的直接缓冲判据与 DLL 冲突，继续作为明确标注的并发文档扩展保留，不伪称原生等价。未调整表格布局。详见 [本轮记录](docs/v1_4_95_release.md)。

V1.4.94 深化 `A-5-32`（GJB9765 4.5.g）：恢复层级连接构造器对位置关联的实际分支，将“已解析目标 + 直接标识符位置实参”按形式端口序号接入跨层级同名检查；表达式、`open` 和未绑定目标继续保持未知。全部 209 条规则继续使用官方三段编号。未修改表格或界面布局。详见 [本轮记录](docs/v1_4_94_release.md)。

V1.4.93 深化 `A-5-31`（GJB9765 4.5.f.30）：恢复规则对象命名词的通用写入链，并确认随附客户端没有为该规则绑定原生值；产品继续严格按附件文字使用 `rst`，但不再把它误写成已恢复的 DLL 默认值。新增 `reset` 不得替代 `rst` 的边界回归。全部 209 条规则继续使用官方三段编号。未修改表格或界面布局。详见 [本轮记录](docs/v1_4_93_release.md)。

V1.4.81 深化 `R-13-7`（GJB10157 9.4.7）：恢复其通用 elaborated 模块连接图边界，并将 primitive-only 基本门强连通分量接入组合反馈检查；连续赋值、组合过程与常量位选择图保持不重复报告。全部 209 条规则继续使用官方三段编号。未修改表格或界面布局。详见 [本轮记录](docs/v1_4_81_release.md)。

V1.4.68 深化恢复 `R-13-3`（GJB10157 9.4.3）的 function 非阻塞赋值逐节点判据、原生 Warning 级别和中文诊断；同时把 GJB9765/10157 的公开规则编号统一为官方三段式数字索引（例如 `R-2-1`），不再显示 `R-8-e`、`R-5-f2` 这类机械派生编号。详见 [本轮记录](docs/v1_4_68_release.md)。

V1.4.64 恢复 GJB9765.4.8.d 的源 IF 元数据生产路径：`FUN_1801c7b80` 写入 `+0x188` 来源句柄、置 `+0x183` 并把 `AST_IF(0xaa)` 降低为 `AST_CASE(0x57)`；原生遍历器也会检查 procedure/function 下的这些节点。产品新增 architecture/package-body 子程序 IF 检查与三组回归；未修改界面。详见 [本轮记录](docs/v1_4_64_release.md)。

V1.4.63 收窄 GJB9765.4.8.d 的 generate 边界：原生遍历器对所有非 `AST_ALWAYS(0x52)` 节点无条件递归，因此 IF-generate 根不属于规则候选，但 generate 分支内的组合 process 仍检查最终 ELSE。新增两组边界回归；未修改界面。详见 [本轮记录](docs/v1_4_63_release.md)。

V1.4.62 收窄 GJB9765.4.8.c 的生成与空节点边界：原生类型表将普通 `AST_CASE(0x57)` 与 `AST_GENCASE(0x62)` 分开，case-generate 根不检查，但其分支内的普通顺序 CASE 仍检查；成对形成的空 CASE 也按原生收集器进入缺失 `when others` 判断。未修改界面。详见 [本轮记录](docs/v1_4_62_release.md)。

V1.4.61 将 GJB9765.4.8.c 的递归设计根覆盖扩展到 package body 子程序，支持 `end package body`、命名结束和裸结束，并保持后续设计根隔离。未修改界面。详见 [本轮记录](docs/v1_4_61_release.md)。

V1.4.60 恢复 GJB9765.4.8.c 的第二个 CASE 来源标志：附件会把 VHDL `elsif` 降低为带 `+0x184` 标记的内部 AST_CASE，原生规则明确排除该内部形态，只检查源码 CASE。产品新增 IF/ELSIF 排除与声明区 procedure CASE 回归；未修改界面。详见 [上一版记录](docs/v1_4_60_release.md)。

V1.4.59 完善 GJB9765.4.8.b 的 architecture 范围识别：除 `end architecture` 外，支持合法的命名结束 `end rtl;` 和裸结束 `end;`，避免后续设计根因范围吞并而漏报。未修改界面。详见 [上一版记录](docs/v1_4_59_release.md)。

V1.4.49 完成 GJB9765.4.2.c 的直接 WHEN 与 `0xaa/0x58/0xac` IF/ELSE 候选结构恢复，并把进程内检测收窄为“THEN 侧数据、最终 ELSE 侧同目标 Z”的原生关系形态；裸 Z、单臂 IF 和反向分支不再误报。未修改界面。详见 [本轮记录](docs/v1_4_49_release.md)。

V1.4.48 恢复 GJB9765.4.2.c 的顶层/端口边界：只豁免所选顶层的 INOUT 三态，非顶层 INOUT 与顶层 OUT 均按内部层次三态报告；同时按 architecture/entity 隔离名称作用域。未修改界面。详见 [本轮记录](docs/v1_4_48_release.md)。

V1.4.47 继续恢复共享顶层配置消费者：GJB10157.5.2.10、6.1.7 和 GJB9765.4.9.h 在显式配置时只分析所选顶层；4.2.c 因原生分支语义尚未闭合而保持原边界，避免误收窄。未修改界面。详见 [本轮记录](docs/v1_4_47_release.md)。

V1.4.46 接入 T-Linter 单一显式顶层配置，修正多根工程中 GJB10157.5.3.2、5.3.3、5.3.4 和 GJB9765.4.2.d 的选择边界；缺失或重复顶层失败关闭，不猜测合规。未修改界面。详见 [本轮记录](docs/v1_4_46_release.md)。

V1.4.45 恢复 GJB10157.5.3.4 的 `setTopModule` 名称生产者和显式 `initial_construct` 标志生产者；未修改界面。详见 [上一版记录](docs/v1_4_45_release.md)。

V1.4.43 继续只处理规则集：将已恢复的 Bison 延时标志生产者接入 GJB10157.9.4.1，task 内前置 `#` 延时现覆盖宏值与括号形式，并明确排除赋值内延时产生式；未修改界面。详见 [先前记录](docs/v1_4_43_release.md)。

V1.4.41 为 GJB10157.9.2.5 确认事件子树中的 AST_BIT_OR/AST_LOGIC_OR 精确类型与逐 always 诊断，并补入 always_ff 事件表达式；未修改界面。详见 [上一版记录](docs/v1_4_41_release.md)。

V1.4.40 为 GJB10157.9.2.11 完整恢复 always 根接纳辅助器，精确排除直接 POSEDGE/NEGEDGE，补入 bare always、always_latch 和命名事件边界；未修改界面。详见 [上一版记录](docs/v1_4_40_release.md)。

V1.4.39 为 GJB10157.9.2.11 恢复组合 always 对直接调用 task/function 定义体的一层展开、名称去重和原生 source-if 节点筛选；未修改界面。详见 [上一版记录](docs/v1_4_39_release.md)。

V1.4.38 将 GJB10157.9.2.1 改用条款专用适配器，恢复普通/通配 case 的两种原生 default AST 形态、节点级诊断和 if 降低节点排除；未修改界面。详见 [上一版记录](docs/v1_4_38_release.md)。

V1.4.34 恢复 GJB10157.9.1.2 普通赋值的 flag-zero/flag-one 双重位宽判据、乘法求和与常量 2 特例，以及三目结果分支的独立比较；未修改界面。详见 [上一版记录](docs/v1_4_34_release.md)。

V1.4.33 确认 GJB10157.9.2.10 两条原生路径均使用 flag-zero 位宽辅助器，接入算术、拼接、静态重复拼接、比较与逻辑条件，并支持平衡嵌套括号；未修改界面。详见 [上一版记录](docs/v1_4_33_release.md)。

V1.4.32 确认 GJB10157.8.8 使用 flag-zero 宽度辅助器，接入算术与重复连接操作数，并修正嵌套逻辑节点的直接子项判断；未修改界面。详见 [上一版记录](docs/v1_4_32_release.md)。

V1.4.28 恢复并接入 GJB10157.8.7 在 flag=0 路径下的嵌套算术表达式位宽推断，并保持最外层 ADD/SUB、MUL、DIV/MOD、POW 的原生阈值；未修改界面。详见 [上一版记录](docs/v1_4_28_release.md)。

V1.4.27 恢复并接入 GJB10157.8.7 的 ADD/SUB 逐操作数判据与拼接 bits 向量豁免；未修改界面。详见 [上一版记录](docs/v1_4_27_release.md)。

V1.4.23 按 DLL 证据修正 GJB9765.4.14.b.1、b.2：复现 `fgets(512)` 的 511 字节分块、逐块判定和诊断序号，补齐超长物理行及空格/LF 跨块边界；未修改界面。详见 [上一版记录](docs/v1_4_23_release.md)。

V1.4.22 按 DLL 证据修正 GJB9765.4.14.d.1、d.2：复现 `fgets(0xff)` 的 254 字节分块、首个字面 `--` 截断、原始非空白字节与分号计数，以及分块诊断序号；未修改界面。详见 [上一版记录](docs/v1_4_22_release.md)。

## 上一版本

V1.4.21 按 DLL 证据修正 GJB9765.4.14.c.1：接入原始物理行括号扫描、精确允许字符位图、字符串行为、首个字面 `--` 截断，以及 `(others` 清除同一行先前告警标志的顺序逻辑；未修改界面。详见 [本轮记录](docs/v1_4_21_release.md)。

V1.4.20 按 DLL 证据修正 GJB9765.4.14.c.3：改为原始物理行扫描，精确接入运算符集合、复合符号排除、字符串扫描、首个字面 `--` 截断和 ASCII 空格/Tab 判据；未修改界面。详见 [本轮记录](docs/v1_4_20_release.md)。

V1.4.19 按 DLL 证据修正 GJB9765.4.14.c.2：改为原始物理行扫描，复现首个字面 `--` 截断、字符串参与检查及仅允许 ASCII 空格/Tab 的相邻字节判据；未修改界面。详见 [本轮记录](docs/v1_4_19_release.md)。

V1.4.18 按 DLL 证据恢复 GJB10157.5.2.7 的原生物理行扫描：精确查找 `` `include`` 与同行任意冒号，覆盖注释、字符串、畸形和未激活文本，并移除无原生依据的 Unix 斜杠路径扩展；未修改界面。详见 [本轮记录](docs/v1_4_18_release.md)。

V1.4.17 将 GJB10157.5.1.2、5.1.3、5.1.6 三条格式结构规则切换到条件编译活动分支，修复未激活分支中的错位语句、声明及同一行赋值误报；未修改表格或界面布局。详见 [本轮记录](docs/v1_4_17_release.md)。

V1.4.16 将 GJB10157.8.4、8.5、9.2.2、9.2.4、9.2.8、9.2.9 六条表达式规则接入条件编译活动分支，修复未激活分支误报；未修改表格或界面布局。详见 [本轮记录](docs/v1_4_16_release.md)。

V1.3.87 恢复并接入 GJB9765.4.10.c“异步复位释放应采用同步释放”的原生关系图核心谓词，覆盖源码可证明的两级异步置位、同步释放链及原始复位直连风险。未修改表格或界面布局。详见 [本轮记录](docs/v1_3_87_release.md)。

V1.3.84 恢复并接入 GJB9765.4.12.a 的三个原生分支：非整数 `generic`、直接空字符串常量和 `wait for`；同时撤销无原生依据的通用延时/无悬挂进程映射。未修改表格或界面布局。详见 [本轮记录](docs/v1_3_84_release.md)。

V1.3.83 恢复并接入 GJB10157.10.3 的 Verilog 状态机初始状态判断：原生最终分支查找分析器合成的 `init` 关系，而不是源状态名称；产品接入显式 `initial` 和直接复位 `if/else` 子集，复杂绑定继续保留人工复核。未修改表格或界面布局。详见 [本轮记录](docs/v1_3_83_release.md)。

V1.3.69 恢复并接入 GJB9765.4.9.j 时钟路径直接单目 NOT 反相器判断；元件缓冲器和层级路径继续保留为未知，未修改表格或界面布局。详见 [本轮记录](docs/v1_3_69_release.md)。

V1.3.68 恢复并接入 GJB9765.4.9.e“组合逻辑生成时钟”的有界判断，区分时钟别名、普通数据组合逻辑及门控时钟；未修改表格或界面布局。详见 [本轮记录](docs/v1_3_68_release.md)。

V1.3.67 恢复 GJB9765.4.5.g 跨层级信号命名一致性的有界判断，并接入附件正反例；未修改任何表格或界面布局。详见 [本轮记录](docs/v1_3_67_release.md)。

V1.3.64 接入 GJB9765.4.2.d 顶层直接子项判断和 GJB9765.4.4.a 的 `std_logic_vector` 范围判断，见 [记录](docs/v1_3_64_release.md)。

V1.3.60 只推进规则集：恢复并接入 GJB9765.4.9.h“同一 VHDL 设计单元存在多个直接时钟域”的核心判据；层次关系仍保留工程上下文状态，未改动表格或界面格式，见 [本轮记录](docs/v1_3_60_release.md)。

V1.3.59 恢复并接入 GJB9765.4.9.g“触发器输出作为后级时钟”的行波时钟判据；未改动表格或界面格式，见 [本轮记录](docs/v1_3_59_release.md)。

V1.3.54 恢复 GJB9765.4.1.a 的 VHDL 具名端口关联判据和“每个违规实例最多一条”的诊断重数；未改动表格或界面格式，见 [本轮记录](docs/v1_3_54_release.md)。

V1.3.53 恢复 GJB9765.4.10.j 的 VHDL 复位/置位唯一有效电平判据，补齐 SET 控制并修复显式低有效电平被名称推断污染的问题；未改动表格或界面格式，见 [本轮记录](docs/v1_3_53_release.md)。

V1.3.4 恢复 GJB9765.4.5.f.22/.23/.24 的端口方向后缀判据，见 [V1.3.4 记录](docs/v1_3_4_release.md)。

V1.3.3 恢复 GJB9765.4.2.a 的实体与 component 双分支端口存在性检查，见 [V1.3.3 记录](docs/v1_3_3_release.md)。

V1.2.59 为 GJB9765.4.1.b / 4.1.c 接入本地组件与实体的端口关联检查，修复附件输入、输出遗漏例的漏检，并区分合法的输出 `open`。见 [V1.2.59 记录](docs/v1_2_59_release.md)。

V1.2.58 修正 GJB9765.4.6.f 对明确条件三态驱动的误报，并补充组件输出端口与本地赋值共同驱动的检查，见 [V1.2.58 记录](docs/v1_2_58_release.md)。

V1.2.52 根据附件 AST 行号收集及前一行注释截断/空白扫描证据，为 GJB9765.4.14.a.1 接入相邻顺序 `process` 的分节检查；附件正反例吻合，其他 AST 节点类型和长行边界继续人工复核。见 [本轮记录](docs/v1_2_52_release.md)。

V1.2.51 根据附件逐行 `fgets(512)` 扫描和双字节集合判据，为 GJB9765.4.14.b.1 接入行首 Tab 检查；附件正反例吻合，长行与完整源码准入边界仍保留人工复核。见 [本轮记录](docs/v1_2_51_release.md)。

V1.2.50 根据附件二进制的 IF/CASE 分组、4 列 Tab 显示列及两条原生告警分支证据，为 GJB9765.4.14.b.3 接入顺序 `if/elsif/else` 同层对齐和 `case/when` 层级/同层对齐检查；附件正反例吻合，未知 AST 边界继续保留人工复核。见 [本轮记录](docs/v1_2_50_release.md)。

V1.2.47 根据附件 AST 声明分组、源码名称定位和 4 列制表位证据，为 GJB9765.4.14.b.5 接入 VHDL 端口、连续信号及变量声明的对齐检查；附件正反例匹配，同一行和声明分组边界不扩大检测。见 [本轮记录](docs/v1_2_47_release.md)。

V1.2.46 重新实现问题列表与代码规模明细的列宽拖动：拖动期间以明确指示线跟随鼠标，松开后按同方向提交列宽，避免横向滚动范围变化造成反向视觉反馈；两张表改为连续闭合的单层框体。见 [本轮记录](docs/v1_2_46_release.md)。

V1.2.45 根据附件原生关联分组、名称定位与制表位换算证据，为 GJB9765.4.14.b.6 接入 VHDL 具名端口映射对齐检查；附件正反例均匹配。见 [本轮记录](docs/v1_2_45_release.md)。

V1.2.44 根据附件原生括号邻接判据，为 GJB9765.4.14.c.1 接入 VHDL 括号内侧分隔的有界检查；附件正反例均匹配。见 [本轮记录](docs/v1_2_44_release.md)。

V1.2.43 根据附件原生源码扫描分支，为 GJB9765.4.14.c.3 接入 VHDL 比较与算术运算符两侧空白的有界检查；附件正反例均匹配。见 [本轮记录](docs/v1_2_43_release.md)。

V1.2.42 根据附件逐行空格检查证据，为 GJB9765.4.14.c.2 接入 VHDL `<=`、`:=` 两侧空白的有界检查；附件正反例均匹配，注释与字符串误报被排除。见 [本轮记录](docs/v1_2_42_release.md)。

V1.2.41 根据附件 `file_open` 原生 AST 收集和路径字节匹配证据，为 GJB9765.4.14.d.4 接入 VHDL 直接字符串参数中 `:/`、`:\` 的有界检查；附件正反例均匹配，无告警仍需工程上下文复核。见 [本轮记录](docs/v1_2_41_release.md)。

V1.2.40 修正问题列表与代码规模明细在横向滚动后拖动列宽时的视口偏移，保持分隔线与指针同向；两张表增加独立内框、深色表头分隔线和持续可见的网格。见 [本轮记录](docs/v1_2_40_release.md)。

V1.2.39 根据附件原生相邻映射节点行号比较证据，为 GJB9765.4.14.d.3 接入 VHDL 具名端口映射“一行多个信号”的有界检查；附件正反例均匹配。见 [本轮记录](docs/v1_2_39_release.md)。

V1.2.38 根据附件 `is_override_assign` 模型证据，新增 GJB10157.9.1.5 的有界同路径覆盖赋值检查；附件正反例匹配，互斥分支不误报。完整位级过程降级仍待恢复。见 [本轮记录](docs/v1_2_38_release.md)。

V1.2.37 根据附件原生 AST 遍历证据，为 GJB10157.9.2.5 接入显式敏感列表 `|`/`||` 的有界检测；公开正反例均匹配，并新增注释、过程体和重复运算符测试。规则仍保持未完全等价的复核状态。见 [本轮记录](docs/v1_2_37_release.md)。

V1.2.36 修正两张结果表的列边界拖动坐标、补齐空白区网格线，并将标题和表格纳入清晰的单层外框；增加水平滚动后的拖动回归测试。见 [本轮记录](docs/v1_2_36_release.md)。

V1.2.35 深化 GJB10157.9.4.9 的敏感列表判据：保留简单 Verilog 显式列表中的隐式网络名称，补齐附件遗漏/冗余定位，并允许同一位置同时呈现两类问题。见 [本轮记录](docs/v1_2_35_release.md)。

V1.2.34 对 GJB10157.9.4.8 的标识符下标补充有界求值：模块级、直接数字赋值的 `localparam` 可参与越界检查；可覆盖参数、动态索引、重名遮蔽和宏相关代码仍需复核。见 [本轮记录](docs/v1_2_34_release.md)。

V1.2.33 基于 GJB10157.9.4.8 原生常量索引与上下界比较证据，补充负下标和负切片端点越界检查；公开样例无回退，未知索引仍需复核。见 [本轮记录](docs/v1_2_33_release.md)。

V1.2.32 将 GJB10157.9.4.7 的已知常量位选反馈分析接入按位依赖图，修正附件合规例误报；四个公开例子全匹配，原生完整图算法仍未恢复。见 [本轮记录](docs/v1_2_32_release.md)。

V1.2.31 修正问题列表与代码规模明细的列宽拖动坐标，向左缩窄、向右加宽；两表在空白区域持续显示列界线，并加深明确的表格外框。见 [本轮记录](docs/v1_2_31_release.md)。

V1.2.30 接入 GJB10157.9.4.6 的同 module task 位置实参已知切片位宽检查；附件错误例 2 次、正确例 0 次。见 [本轮记录](docs/v1_2_30_release.md)。

V1.2.29 接入 GJB10157.9.4.5 的已知位宽 function 返回赋值检查；附件正反例匹配，未知表达式保留复核。见 [本轮记录](docs/v1_2_29_release.md)。

V1.2.28 按附件 AST 证据接入 GJB10157.9.4.4 中“模块级 reg 在 function/task 内被阻塞写入”的有界检测；局部遮蔽和复杂表达式仍按部分恢复处理。见 [本轮记录](docs/v1_2_28_release.md)。

V1.2.27 按附件证据对 GJB10157.9.4.1 接入 task 中独立数值延时语句的有界检查；其他时序控制仍需复核。见 [本轮记录](docs/v1_2_27_release.md)。

V1.2.26 修正问题列表和代码规模明细表格的列宽拖动方向，并强化可见表格框体；同时接入 GJB10157.9.4.2 的有界 initial 检测。完整 209 条规则的原生等价恢复仍在进行中。见 [本轮记录](docs/v1_2_26_release.md)。

V1.2.25 按附件原生 AST 判据接入 GJB10157.5.3.1 的端口实参表达式检查，补充正反例与边界测试；完整 209 条规则的等价恢复仍在进行中。
24条原生核心谓词部分接入，完整算法等价验证仍为0条。见 [当前检查点](docs/v1_2_13_release.md)。

V1.2.12 继续恢复“一文件一模块”原生分组/计数谓词，并将所有尚未验证完整算法的无告警规则结果标记为需复核。
见 [V1.2.12记录](docs/v1_2_12_release.md)。

V1.2.11 将六类命名关键字查表判断接入显式工程配置，并修正 Verilog 大写标识符被当成小写关键字的前端误判。

V1.2.6 新增默认命名谓词、递归函数调用图和复杂索引的部分检测，撤销过宽的状态机条款映射。

V1.2.5 继续接入原生证据支持的 function、case 标签、repeat 检查，改进算术 X/Z 子树与 default 赋值检查。
规则状态“未检出”不代表标准合规；尚未完整覆盖的条款保留需复核。见 [本轮说明](docs/v1_2_5_release.md)。

V1.2.4 将原生逆向证据支持的 8.4 关系运算边界与 9.2.2 常量参数 case 检查接入软件，并修正非 case 的 default 误报。
这不是 209 条原生算法全部恢复的最终版；完整的逐条证据、实现与验证状态见
[原生规则实施台账](docs/evidence/native_rule_implementation_ledger.json)和
[本轮说明](docs/v1_2_4_release.md)。

面向实际 FPGA 项目的 HDL 静态分析与编码规则检查平台。当前版本以 C++20
实现了可用于本地开发和 CI 的 Lint 与设计结构分析链路，支持 Verilog、SystemVerilog
和 VHDL 源文件。

## 当前能力

- 新增类似 LineCount 的 [HDL 代码行数统计](docs/line_statistics.md)：总行数、代码行、注释行、
  空白行，工程/语言/文件三级汇总；中文桌面面板、CSV/HTML/JSON 导出与 `--line-count` 独立统计模式。
- 接受单个文件或目录；目录会递归发现 `.v`、`.vh`、`.sv`、`.svh`、`.vhd`、`.vhdl`。
- 支持 `-f/--file-list` 与配置中的 `file-list`，递归展开 simulator 风格 HDL 清单、相对源码路径、
  `+incdir+`、`+define+`、`-I/-D`；循环引用和非法选项带清单行号诊断。
- 自动识别 HDL 语言，统计 Verilog/SystemVerilog module 与 VHDL entity。
- 在执行 Lint 前检查注释、字符串、括号和主要 Verilog/SystemVerilog 结构是否闭合。
- 支持 Verilog/SV `define/undef`、ifdef/ifndef/elsif/else/endif 与 include 路径检查。
- 通过注释/字符串感知 token 层提取 module/entity、参数化实例和 VHDL entity/component 实例。
- 解析跨文件实例目标，识别顶层单元、重复定义、未解析目标和递归例化。
- 提取 Verilog/SystemVerilog ANSI/legacy non-ANSI module 端口及 VHDL entity ports，保留
  方向和来源；解析 module named connections 与 VHDL `port map` named associations，并对
  已解析目标检查重复 formal 和目标接口中不存在的端口名。
- 记录实例端口连接总数及 named/positional/mixed 风格，对已解析目标拒绝超出接口的
  positional actual，并拒绝 Verilog/SystemVerilog 中 positional 与 named connection 混用。
- 对纯 named 实例检查缺失或显式 open/空置的必需 input；支持 SystemVerilog `.port`
  shorthand，带默认值的输入、output、Verilog `.*` 以及 VHDL positional 后 named 的合法
  混合映射按保守边界排除。
- 对已解析实例检查遗漏、空 actual 或 VHDL `open` 的 output/inout，以 info 级别提示可能丢失的
  状态、握手或双向接口；完整连接和 Verilog `.*` 保守排除。
- 按端口声明顺序映射 positional actual，检查缺失、空 actual 或 VHDL `open` 的必需 input；
  尾部省略 output 或带默认值 input 不再产生笼统的数量告警。
- 验证 VHDL port/generic map 的 association 顺序，并检查 positional 前缀与后续 named formal
  是否重复覆盖同一端口或 generic。
- 检查 module/entity 接口内重复声明的 port 与 parameter/generic；VHDL 名称按大小写不敏感
  语义比较，并定位第二次声明位置。
- 检查同一 module/architecture 作用域内重复声明的内部 signal/net/variable；VHDL 同时识别
  内部 signal 与 entity port 的同名冲突，Verilog 保留合法 legacy port/net 重声明兼容性。
- 提取 Verilog/SystemVerilog module parameter 与 VHDL entity generic，并记录实例参数覆盖
  数量及 named/positional 风格；检查重复或目标接口中不存在的 named parameter/generic。
- 检查 Verilog/SystemVerilog value/type parameter、localparam 与 VHDL generic 是否真正影响
  当前设计单元接口或实现；VHDL 会跨输入文件关联 entity 与全部 architecture，named override
  formal、其他 entity 和声明 token 不计作本地使用；未提供 architecture 时保守跳过 generic 告警。
- 检查无默认值的必需 parameter/generic 是否被 named 或 positional override 覆盖，拒绝
  positional override 数量超过目标接口，并检查 Verilog/SystemVerilog 参数覆盖风格混用。
- 为 Verilog、SystemVerilog、VHDL 建立过程/并发赋值和读写使用模型，检查多驱动、输入被驱动、
  输出/内部信号未驱动，以及声明后从未读取的内部信号。
- 检查声明后既未被本地逻辑读取、也未转发到下级实例的 input/in 端口；保留实例方向未知时的
  保守连接边界，避免把合法接口透传误报为未使用。
- 双向检查传统 Verilog `always @(...)` 与 VHDL `process(...)` 的组合敏感列表：报告过程体
  读取但列表遗漏的信号，以及列表中存在但过程体从未读取的冗余信号；识别 `@(*)`、
  `always_comb`、`process(all)` 和边沿触发过程并排除。
- 检查显式敏感列表中的重复信号或完全相同边沿事件；VHDL 名称按大小写不敏感匹配，Verilog
  的 `posedge` 与 `negedge` 保持不同事件，诊断定位后续重复项。
- 检查没有任何可执行行为的 Verilog/SystemVerilog `always`/`initial` 空块，以及仅含 `null;`
  的 VHDL process；命名块与 VHDL 局部子程序声明不会被误当作过程行为。
- 对组合过程执行控制流确定赋值分析，定位缺失 `else`、分支赋值不对称或缺少完整
  `default`/`when others` 覆盖造成的锁存器推断风险。
- 为连续赋值和组合过程建立信号依赖图，通过强连通分量检测跨赋值及自反馈组合环；时序过程
  作为断点，并处理 Verilog 阻塞赋值临时累加和 `for` 控制变量语义。
- 解析 Verilog/SystemVerilog 常量 packed range 和 VHDL 常量 vector range，对已知宽度的
  直接信号赋值检查高位截断；同时检查直接赋值的 Verilog 显式 sized literal 与 VHDL 字符串
  literal。来源和目标均可为直接信号、常量 bit/index 或 slice；来源还支持成员宽度均可静态
  求值的 Verilog/SystemVerilog `{a, b}`、常量 `{N{expr}}` 复制与 VHDL `a & b` 拼接；VHDL
  拼接成员可包含字符串及 `'0'`/`'1'` 等标量逻辑 literal。动态复制
  次数、动态选择和无法求值的参数化宽度保守跳过。
- 保留单维常量向量声明的真实上下界，检查 Verilog/SystemVerilog 位选/切片及 VHDL
  index/slice 的常量边界越界；支持非零起始范围和 Verilog based integer 索引。
- 检查由本地连续/过程赋值生成后又用于边沿触发的派生或门控时钟；输入时钟和仅通过实例端口
  连接的 PLL/MMCM/时钟原语输出保守排除。
- 检查 Verilog/SystemVerilog `posedge`/`negedge` 与 VHDL `rising_edge`/`falling_edge`
  是否直接作用于标量信号；组合表达式、位选和索引边沿源会报告 warning，多层无害括号保留。
- 检查 Verilog/SystemVerilog 过程与 continuous assign 延时控制，以及 VHDL signal
  assignment 的 `after` 子句；实例/模块 `#(...)` 参数覆盖不会误判为延时。
- 检查没有 sensitivity list 且过程体不存在任何 `wait` 的 VHDL process，定位会在零仿真时间
  无限循环、阻塞仿真的缺失 suspension point。
- 检查普通 Verilog/SystemVerilog `always` 是否缺少 `@`、`#` 或 `wait` 挂起点，定位会在
  零仿真时间无限重复并卡死仿真的过程；自动敏感过程和 `initial` 保持排除。
- 检查带 sensitivity list 的 VHDL process 内非法出现的显式 `wait`；该 error 会替代同位置的
  通用 wait 可综合性 warning，避免重复报告同一根因。
- 通用 `VHDL-WAIT` 使用 architecture/process token 范围，支持跨行 `wait until/on/for` 与裸
  `wait;`；敏感列表 process 中的直接 wait 仅保留更具体的 legality error。
- VHDL 非标准算术包与 `shared variable` 使用 token 化 use/declaration 扫描，支持跨行写法并
  排除注释和字符串；architecture 内 shared variable 保留所属 entity 上下文。
- 对 Verilog/SystemVerilog 与 VHDL 过程内 `if/elsif/while` 及运行时三目/when-else guard
  执行保守常量求值，覆盖字面量、冗余括号、逻辑取反、布尔连接、短路恒等式及字面量比较；
  generate/elaboration 条件和不能完整证明的表达式按边界排除。
- 检查过程条件中等值、不等值及 `<`/`<=`/`>`/`>=` 关系比较两侧的静态位宽，覆盖直接信号、
  常量 bit/index/slice、显式宽 literal 和静态拼接；逻辑连接的子条件独立分析，算术、调用及
  动态选择保持保守排除，并区分 Verilog 移位运算符；覆盖 `if`、两个 HDL 的 `while` 及
  Verilog/SystemVerilog `for` 的中间控制条件、括号包围、过程/连续赋值右侧、链式嵌套及
  `if` 等包围表达式中的三目运算条件，以及 VHDL
  `value when condition else value` 条件，以及完整括号比较的 `!`/`not` 外层取反；
  SystemVerilog `==?`/`!=?` 通配相等同样支持。
- 检查 Verilog/SystemVerilog `if/while` 和运行时三目 guard 中的普通/复合/移位赋值及
  `++`/`--` 副作用，
  定位容易由 `=`/`==` 拼写错误或隐式状态更新造成的控制流风险；合法比较运算保持排除。
- 检查 Verilog/SystemVerilog 条件分支中的空语句和结构性空 `begin/end`，覆盖 then、else、
  嵌套分支及命名块，定位 `if (condition);` 这类多余分号或遗漏行为造成的静默缺陷。
- 检查 Verilog/SystemVerilog `case/casez/casex` 的空选择项，覆盖空语句、命名/未命名空块、
  `default` 和嵌套 case，并修正空语句不得吞并下一 case 项的控制流边界。
- 检查同一 case 内多个 Verilog/SystemVerilog `default` 或多个 VHDL `when others`，以 error
  定位后续非法且不可达的 catch-all 分支；嵌套 case 的 catch-all 状态分别维护。
- 对直接信号、Verilog/SystemVerilog 常量位选/切片及 VHDL 常量索引/slice selector 检查
  显式 sized case 常量或字符串 choice 的位宽，定位扩展、截断或永远无法按预期匹配的宽度
  不一致；动态索引、拼接、调用和无尺寸常量按保守边界排除。
- 对固定宽度的 Verilog/SystemVerilog 直接、常量位选或切片 case selector，检查无尺寸非负
  整数 choice 是否超出所有可表示 bit pattern，定位永远无法匹配的不可达分支。
- 检查直接、常量索引或切片操作数的常量移位量是否大于等于位宽，覆盖 Verilog/SV
  `<<`/`>>`/`<<<`/`>>>` 与复合移位赋值，以及 VHDL `sll/srl/sla/sra` 和
  `shift_left/shift_right` 调用。
- 检查上述 Verilog/SystemVerilog 与 VHDL 移位形式是否使用负整数字面量作为移位量，定位
  方向反转、超大无符号计数或违反 `natural` 参数约束等易混淆行为。
- 检查 Verilog/SystemVerilog 除法、取模及 `/=`/`%=` 复合赋值，以及 VHDL `/`、`mod`、
  `rem` 的 literal 零分母；支持完整冗余括号并排除 VHDL `/=` 不等比较。
- 检查 SystemVerilog `always_comb` 内非法的显式 `@`，以及 `always_ff` 缺少或包含多个
  event control；合法的单 event-control `always_ff` 和 Verilog `.v` 源码保持排除。
- 对恰好一个 event control 的 `always_ff` 继续要求至少包含一个 `posedge`/`negedge`；
  `@(clk)` 和 `@(*)` 会报告 error，避免把电平变化过程误当作触发器过程。
- 检查 SystemVerilog `always_latch` 内破坏自动敏感语义的显式 event control；合法的
  level-sensitive 自动敏感过程及 Verilog `.v` 源码保持排除。
- 对 `always_latch` 执行控制流确定赋值分析；若所有已识别赋值在全部路径都完成、实际不会
  推断锁存器，则提示过程类型与硬件意图不一致。
- 检查同一 Verilog/SystemVerilog 过程内对同一已声明信号混用阻塞与非阻塞赋值，支持位选/
  切片目标并排除 `for` 循环控制头，定位第二种冲突调度方式出现的位置。
- 检查 Verilog/SystemVerilog 与 VHDL 过程内标量直接自赋值，例如 `q <= q`；阻塞/非阻塞及
  VHDL 大小写语义均支持，切片、索引、连续赋值和包含运算的真实状态更新保守排除。
- `VL-SEQ-BLOCKING` 与 `VL-COMB-NONBLOCKING` 使用 token 化完整过程范围，不依赖赋值与过程
  关键字位于同一行；语句左值判定区分非阻塞赋值 `<=` 与关系比较 `<=`。
- `VL-CASEX` 与 `VL-CASE-DEFAULT` 复用 token 化 case 作用域模型，支持嵌套 case、多行 selector
  以及同一行完整 case，并与重复项和通配模式相交检查共享准确边界。
- 检查同一 Verilog/SystemVerilog 或 VHDL case 内完全重复的选择项，展开逗号/竖线多选项，
  并保持嵌套 case 的独立作用域，定位后续不可达分支。
- 检查同一 `if/else if` 或 VHDL `if/elsif` 链中的完全重复条件，规范化冗余外层括号；嵌套及
  相邻独立条件链分别处理，VHDL 条件按大小写不敏感语义比较。
- 对可完整解析的 Verilog/SystemVerilog `casez`/`casex` 二进制与十六进制常量模式执行
  位级相交检查，定位不同但能同时匹配至少一个值的顺序依赖分支。
- 检查 Verilog 简单连续赋值端点和 module 实例端口实际信号中的未声明标识符，定位因拼写
  错误产生的隐式网络；参数、localparam、genvar 和复杂端口表达式按保守边界处理。
- 从 Verilog/SystemVerilog 边沿事件、VHDL edge 函数及经典
  `clk'event and clk = '1'/'0'` 模板提取单元级时钟、边沿、同步/异步复位及有效电平，并在
  text、JSON、HTML 和 GUI 设计层次中展示。
- VHDL 时序摘要按项目输入关联 entity 与分离的 architecture 文件，再统一执行时钟/复位一致性、
  CDC、RDC、未使用异步复位及多时钟过程检查；同文件结构会按来源位置去重。
- 跨过程检查同一时钟的上升/下降沿混用、同一复位的同步/异步混用及有效电平冲突，并保留
  全部观察状态供报告审计。
- 为每个时序过程建立时钟边沿域与信号读写集合，检查同一设计单元内由一个时钟域写入、被
  另一个时钟域直接采样的寄存器信号；索引 RAM/数组通道保守排除。对能证明异步源仅进入
  目的域内部第一级、再直接进入同域第二级且不存在复杂旁路消费的标量同步链自动抑制告警。
- 将复位信号、极性及同步/异步属性关联到具体时序过程，检查同一异步复位直接控制多个时钟
  边沿域时可能存在的独立释放与 recovery/removal 风险。
- 检查 Verilog/SystemVerilog 异步 reset/clear/preset 的事件边沿与首个复位分支有效电平是否
  一致，覆盖直接、取反及 `==`/`===`/`!=`/`!==` 的 0/1 比较模板。
- 检查 Verilog/SystemVerilog 与 VHDL 异步 reset/clear/preset 是否作为过程第一个独立 `if`
  条件；在 enable、功能条件或复合表达式之后才测试复位会报告 error，避免异步复位被屏蔽。
- 检查首个异步复位 `if/else` 或 `if/elsif` 链之后是否仍存在独立赋值；这些赋值会在复位事件
  到来时继续执行并覆盖或绕过复位值，因此按 error 报告。
- 检查异步 reset/clear/preset 已列入时序过程敏感列表、但过程体完全未读取该控制信号的
  模板遗漏，覆盖 Verilog/SystemVerilog 边沿事件与 VHDL reset-like sensitivity 信号。
- 保留每个时序过程的全部边沿事件，并检查同一过程由两个或更多独立时钟驱动的通常不可综合
  写法；异步 reset/clear/preset 事件不会被误计为第二时钟。
- 忽略 Verilog 字符串、行注释和块注释中的规则关键字；忽略 VHDL 行注释。
- 输出确定性排序、精确到行列的诊断，并附问题说明和修改建议。
- 提供 text、原生 JSON、OASIS SARIF 2.1.0 和离线 HTML 报告，可直接写入指定文件。
- text、JSON、SARIF、HTML 和 GUI 统一展示最终有效严重级统计；JSON/SARIF 提供确定性
  `ruleCounts`，HTML 增加 severity 指标卡和 rule summary 表。
- text、JSON、SARIF、HTML 与 GUI 设计树公开已认证 CDC 同步器数量；JSON 和层次视图保留
  异步源、第一级、第二级、目的时钟/边沿及来源位置，避免安全结构被静默隐藏。
- `--fail-on` 可控制 CI 失败阈值；默认仅 warning 及以上令进程返回 1。
- 支持可移植、可审查的诊断基线，让既有项目在 CI 中只阻断新增问题，并单独统计基线命中。
- 93 条内建规则由中央目录统一定义级别、语言、说明和建议，可通过 CLI 查询；SARIF 始终
  发布完整规则元数据，不受本次是否命中影响。
- 提供 GJB10157-2021（103 条）与 GJB9765-2020（106 条）两套标准规则集：209 条均有
  条款、摘要、严重级、强制属性、正反例判据和评估状态；80 条具有可执行检测器，11 条工程复核，
  118 条人工复核。部分检测器无告警仍要求复核；目录数字不是原生算法恢复数量。详见
  [GJB 实施矩阵](docs/gjb_rule_matrix.md)及 [1.2.1 源码选择修复说明](docs/v1_2_1_release.md)。
- 支持项目及命令行按规则覆盖 info/warning/error/critical，最终级别同时控制报告、基线前
  结果和 CI 退出阈值，并记录在归档报告中。
- 支持项目配置、路径排除、外部 IP/black box 声明和全局/源码行级规则豁免。
- Qt 桌面端已接入后台项目分析、设计层次、问题筛选、详情、源码定位和 HTML/SARIF 导出。
- 核心和 CLI 不依赖 Qt；桌面 GUI 仍是可选构建。

当前规则目录见 [规则参考](docs/rules.md)，JSON 契约见
[报告格式](docs/reports.md)，桌面工作流见 [GUI 说明](docs/gui.md)，架构边界见
[架构说明](docs/architecture.md)，V1.0 发布范围与验收结论见
[V1.0 发布说明](docs/v1_release.md)，Windows 构建与 ZIP 打包见
[Windows 构建说明](docs/windows_build.md)。V1.0
仍不等同于完整语法/语义解析器；当前 CDC 仅覆盖单元内直接寄存器跨域，尚未覆盖跨层次
端口传播和向量/属性/跨层次同步器认证；当前 RDC 仅覆盖单元内异步复位跨多时钟域，尚未覆盖复位生成
链、跨层次传播、复位同步器认证、generate elaboration 和时序约束等深度签核能力。

## 快速开始

```sh
cmake -S . -B build/core-only -DFPGA_ANALYZER_BUILD_GUI=OFF -DBUILD_TESTING=ON
cmake --build build/core-only --config Debug
ctest --test-dir build/core-only -C Debug --output-on-failure
python3 tests/run_cli_regressions.py --cli ./build/core-only/fpga_analyzer_cli

./build/core-only/fpga_analyzer_cli ./rtl
./build/core-only/fpga_analyzer_cli -f project/files.f
./build/core-only/fpga_analyzer_cli --format json --fail-on error ./rtl ./ip/core.sv
./build/core-only/fpga_analyzer_cli --format sarif -o fpga-lint.sarif ./rtl
./build/core-only/fpga_analyzer_cli --format html -o fpga-lint-report.html ./rtl
./build/core-only/fpga_analyzer_cli --write-baseline .fpga-lint-baseline ./rtl
./build/core-only/fpga_analyzer_cli --baseline .fpga-lint-baseline ./rtl
./build/core-only/fpga_analyzer_cli --list-rules
./build/core-only/fpga_analyzer_cli --describe-rule RTL-LATCH-INFERRED
./build/core-only/fpga_analyzer_cli --list-standards
./build/core-only/fpga_analyzer_cli --list-standard-rules gjb10157
./build/core-only/fpga_analyzer_cli --list-standard-rules gjb9765
./build/core-only/fpga_analyzer_cli --severity VL-CASEX=error --fail-on error ./rtl
```

完整桌面版本、安装树和 ZIP 发布包可按下列方式生成：

```sh
cmake -S . -B build/release -DCMAKE_BUILD_TYPE=Release \
  -DFPGA_ANALYZER_BUILD_GUI=ON -DBUILD_TESTING=ON
cmake --build build/release --parallel
ctest --test-dir build/release --output-on-failure
cmake --install build/release --prefix build/install
cpack --config build/release/CPackConfig.cmake
```

## 项目配置与规则豁免

在运行目录放置 `.fpga-lint.conf` 会被自动加载，也可以用 `--config FILE` 显式指定。
完整示例见 [fpga-lint.conf.example](examples/fpga-lint.conf.example)：

```text
fail-on warning
include-dir rtl/include
define FPGA_FAMILY_7SERIES=1
disable STYLE-001
severity RTL-LATCH-INFERRED error
blackbox xpm_fifo_async
exclude **/generated/**
# file-list project/files.f
# baseline .fpga-lint-baseline
```

命令行可重复使用 `--disable RULE`、`--severity RULE=LEVEL`、`--blackbox UNIT` 和
`--exclude GLOB`；命令行 severity 设置追加在配置之后，同一规则最后一次覆盖生效；
`--fail-on` 覆盖配置文件的阈值。完整配置语义见 [项目配置](docs/configuration.md)，HDL 清单
格式见 [File list](docs/file_lists.md)。

已有工程可先用 `--write-baseline FILE` 记录当前诊断（成功写入后退出 0），再通过
`--baseline FILE` 或配置中的 `baseline FILE` 只保留新增问题。基线匹配不依赖行列号，重复
诊断按数量消费；格式、更新策略和 CI 示例见 [诊断基线](docs/baseline.md)。

预处理选项支持 `-I dir`、`-Idir`、`-D MACRO=value`、`-DMACRO=value`，也可以使用
长选项 `--include-dir` 和 `--define`。inactive 条件编译分支会保持原始行列布局但不参与
结构提取、语法结构检查或 Lint。

源码中的局部豁免必须写在 HDL 注释里：

```systemverilog
q = d; // fpga-lint-disable-line VL-SEQ-BLOCKING
// fpga-lint-disable-next-line VL-COMB-NONBLOCKING
q <= d;
```

文件级豁免使用 `// fpga-lint-disable-file RULE` 或 VHDL 的 `--` 注释形式。可使用
`all` 代替规则 ID。报告会记录 suppression 数量，避免豁免行为不可见。

退出码：

- `0`：分析完成，未达到失败阈值；
- `1`：发现达到 `--fail-on` 阈值的规则问题；
- `2`：参数错误、输入不可读，或没有发现可分析的 HDL 文件。

## Windows 10/11 x64

安装 Visual Studio 2022“使用 C++ 的桌面开发”、Windows SDK、CMake 3.24+，
以及 Qt 6 MSVC x64 组件。在 PowerShell 中：

```powershell
$env:QT_ROOT = "C:/Qt/6.8.3/msvc2022_64"
$env:PATH = "$env:QT_ROOT/bin;$env:PATH"
cmake --preset windows-vs2022
cmake --build --preset windows-debug
ctest --preset windows-debug
./build/windows-vs2022/Debug/fpga_analyzer_cli.exe --format text ./rtl
```

GUI 与 CLI 的构建依赖分离；只需要分析内核时使用无 Qt 构建即可。

## 工程结构

```text
src/core/       项目、源码位置与应用元数据
src/database/   RTL 设计对象模型与 DesignDatabase
src/parser/     预处理、token 化、语法/语义检查、设计层次及时钟/复位结构提取
src/analysis/   文件发现、语言识别、规则执行、层次检查与统一 Violation
src/report/     text、JSON、SARIF 和 HTML 报告
src/cli/        命令行入口、参数和 CI 退出码
src/gui/        Qt Widgets 桌面分析工作区
tests/          核心、数据库、分析规则、报告和 CLI 测试
```
