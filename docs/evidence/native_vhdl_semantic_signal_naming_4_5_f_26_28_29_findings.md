# GJB9765 VHDL 语义信号命名规则恢复记录

## 目标与二进制身份

- 目标规则：`GJB9765.4.5.f.26`、`.28`、`.29`
- 附件版本：T-Linter V3.1.4.25120520251205115359
- `T-Linter.dll` SHA-256：`60e6b52875f4d7c3fbdf494d869b3a4bcc3fdc8ee63d8620ea9614da5d6599b0`
- 本记录来自静态反编译和 PE 指令检查，不代表观察了原软件运行。

## Recovered predicates

### 4.5.f.26

- 构造函数：`0x180028030`；slot 0：`FUN_1805f0300`（`0x1805f0300`）。
- `0x1805f09e0..0x1805f09f9` 明确把 `R8D` 置 0 后调用 `FUN_1804d6e80`，即前缀匹配；`R9D` 取规则对象 `+0x144` 的大小写配置。
- DLL 在相邻规则数据中包含默认字面量 `clk_`。函数先从语义关系中恢复实际时钟候选，再检查候选名称，不是扫描所有包含 `clk` 的标识符。
- 产品实现仅检查被解析为 `rising_edge`/`falling_edge` 时钟且能绑定到同一实体端口的名称，使用大小写不敏感的 `clk_` 前缀。

### 4.5.f.28

- 构造函数：`0x180028180`；slot 0：`FUN_1805f1190`（`0x1805f1190`）。
- 两个检查点 `0x1805f1540`、`0x1805f1b5b` 都在调用 `FUN_1804d6e80` 前把 `R8D` 置 2，即后缀匹配；`R9D` 取 `+0x145` 的大小写配置。
- 附件正反例确定默认后缀为 `_n`，并表明候选来自低有效复位/置位条件，而不是仅凭名称或任意与 `'0'` 的比较。
- 完整入口先调用共享生产器 `FUN_1804d56f0`。生产器按优先级把条件树分入类别 3、1/4、2；f.28 检查类别 1 的全部记录，以及类别 2 中显式字节标志为 1 的记录，并且两类都只在记录极性字段为 0 时执行 `_n` 后缀检查。
- 条件提取器 `FUN_1804c4430` 明确识别 `falling_edge(identifier)` 和 `rising_edge(identifier)`，分别写出极性 0 和 1；它也通过 `FUN_1804d4cc0` 提取比较条件。类别 1/4 分类器 `FUN_1804d24a0` 仅在条件名称提取成功时产出记录，并用 `FUN_1804d20e0` 的赋值形态结果把类别 1 与 4 分开。
- `FUN_1804d20e0` 的直接恢复分支把字面量赋值 RHS 视为支持形态。`FUN_1804c5070` 只沿严格的 `AST_BLOCK -> AST_BLOCK -> AST_CASE` 路径取得由 `+0x188` 链接的 `AST_IF`；`FUN_1804d1f90` 要求该 IF 含 `AST_ELSIF`，或 `AST_ELSE` 内含同类链接 IF。枚举映射函数 `FUN_1808ea310` 确认 `0xaa/0xab/0xac/0xad` 分别为 `AST_IF / AST_ELSIF / AST_ELSE / AST_ATTRIBUTE_DESIGNATOR`。
- 容器复制路径确认类别键位于树节点 `+0x20`，关系对象始于 `+0x28`；`FUN_1805f1190` 对类别 2 读取的节点 `+0x38`，就是 `FUN_1804d68a0` 写入关系对象 `+0x10` 的最后嵌套条件常量赋值标志，不是相邻字段猜测。
- 新的 IF 降低证据确认 `FUN_1801c7b80` 先保存源码 IF，再把当前节点改为 `AST_CASE`；原源码第一条件保持直接 `AST_COND`，`AST_ELSIF` 与 `AST_ELSE` 保持独立节点。`FUN_1804d24a0` 只有在 ELSIF 或 ELSE 内链接 IF 中找到边沿后才返回类别 1/4，因此裸 `if falling_edge(...) then literal_assignment` 不属于类别 1。
- `FUN_1804d68a0` 的类别 2 路径要求主 `AST_COND` 本身是边沿，并且只检查该分支块首节点链接的源码 IF；它只处理链接 IF 的直接 `AST_COND`，不会把 `AST_ELSIF` 分支的赋值借给主条件。产品据此撤销裸下降沿分支，只保留“主 IF 为 `falling_edge`、分支首项为链接嵌套 IF、嵌套主条件可建立原生关系且其直接主分支含字面量 RHS”的证据支持子集。
- `FUN_1804cbad0` 在每个设计根开始时构建不区分大小写的命名值树：直接 `AST_GENERIC` 中已绑定的标识符、`AST_ARCHITECTURE` 的直接 `AST_LOCALPARAM`，以及 architecture 本地 `AST_TYPE` 中的枚举标识符进入集合。初始化值可以是常量、布尔标识符、已存在命名值或前端支持的聚合值；`FUN_1804d20e0` 对单一标识符 RHS 只要求其已绑定并存在于该集合。产品接入同一实体的泛型、当前 architecture 的常量与枚举文字，并明确排除 process 局部常量和其他 architecture 的声明。
- 新证据：`reverse-tools/evidence/a5_29_entry_full.txt`（SHA-256 `35864579d2c94d27fa77759e8378cddb0524eb9f00b455bdadbeef4dc0a400dc`）、`a5_29_relation_producer.txt`（`c890837fabf89d09f358ab6ac32d3415be028e7ed92daf559bb71c32aa582216`）、`a5_29_relation_classifiers.txt`（`fe947c42cfa888e3b8e93c1cf62117091633e2ed145964ca41105882602147b2`）、`a5_29_condition_helpers.txt`（`24e8f30ecb4fc06aaec6f57f2092eb4c794dd51f46c75ff6be77e885147d8e90`）、`a5_29_category2_admission_and_layout.txt`（`0da5ff5ef5b14872f54587a2e44c158cc0454611e05a3594d425765cd34951bc`）、`a5_29_ast_enum_mapper.txt`（`78d56f0b3146c837c180974a5279ef43a718bc17c2507e45d1e7597d8b9bd7b0`）、`a5_29_condition_relation_and_if_lowering.txt`（`c28ee14e9a5e0bd25937a47498b7838194e1db24a06d00976350d268f70d91b1`）、`a5_29_named_constant_producer_and_lookup.txt`（`de02e8895b2c10e81fa31a3c8d9f03991a8156faf39c9282b68cb1b359f6da54`）。

### 4.5.f.29

- 构造函数：`0x180028210`；slot 0：`FUN_1805f21d0`（`0x1805f21d0`）。
- 入口对每个设计根递归收集类型 `0x56` 的节点。候选必须恰有两个子节点：左侧为 `AST_IDENTIFIER`（`0x13`），右侧严格为 `AST_BIT_NOT`（`0x24`），且该 NOT 节点必须恰有一个 `AST_IDENTIFIER` 子节点。`0x24` 由独立枚举函数和其他规则交叉确认；入口中没有“右侧直接标识符”的分支。
- 两端全名先以 `_stricmp` 比较。不同则分别查找第一个下划线：存在下划线时取其前段，不存在时使用完整名称；两个基名仍以不区分大小写方式比较，基名不同才报警。
- 原生入口的递归收集没有并发区、进程或子程序过滤。因此产品把完整 `dst <= not src;` 接入 architecture 的并发与顺序区域。
- 附件 HTML 反例同时把 `dst <= src;` 标为违反，这与 DLL 入口冲突。产品仍保留该标准文档判据，但明确作为“仅完整并发简单标识符”的文档扩展，不把它冒充为已恢复的原生分支。
- 新入口证据：`reverse-tools/evidence/a5_30_entry.txt`，SHA-256 `b6522272ce08624f0bed60593b08d85c79c1365a5ca540a5ef14726aaaa02b78`。

## 已知边界

- f.26 的原生关系构建、派生时钟和跨层级传播尚未完全命名；产品只接入同源实体端口与明确边沿事件。
- f.28 的类别 2 标志来源、关系记录复制布局、IF/ELSIF/ELSE 节点类型、主条件降低路径和设计根命名值集合已经恢复；但多个分支块的产生条件、所有前端聚合值、别名/层级传播和外部后缀配置仍未完全恢复。产品接入有明确极性证据的复位/置位，以及结构可证明的“主下降沿 IF + 首项嵌套 IF 主分支字面量或同根命名值赋值”子集。
- f.29 的 `0x56` 确切枚举名、设计根准入、package/subprogram 的源码降低、扩展标识符和重载 `not` 尚未证明；直接缓冲属于附件文档与 DLL 的明确冲突。
- 三条规则均保留“未发现问题 = 需要人工复核”的平台语义，不输出完全合规结论。

## 回归覆盖

`tests/standard_accuracy_tests.cpp` 覆盖：

- 附件违反例和遵循例；
- f.26 大小写边界与仅实际时钟端口范围；
- f.28 普通数据与 `'0'` 比较、裸下降沿常量赋值、分支外无关字面量、非首项嵌套 IF、ELSIF 分支借用及普通下降沿数据传送的排除边界；覆盖首项嵌套 IF 主分支的字面量、实体泛型、architecture 常量与枚举文字命中，并验证普通信号、process 局部常量和跨 architecture 名称不命中；
- f.29 无下划线名称作为完整基名、文档直接缓冲扩展、并发/顺序反相、复合表达式排除边界。
