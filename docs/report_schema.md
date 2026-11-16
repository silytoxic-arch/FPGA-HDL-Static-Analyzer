# JSON 报告协议

当前输出 `schemaVersion: 4`。V1.2.5 将标准评估的 `passed` 改为 `no-violation-found`，
明确自动检查未检出不等于标准合规；这是状态值兼容性变更。其他状态仍为 `violated`、
`project-context-required`、`manual-review-required`。V1.2.12的完整算法等价验证为0条，因此所有未报警的自动检测条款均需复核；`no-violation-found` 暂不输出，留待完整恢复并验证的规则使用。
协议顶层字段如下：

| 字段 | 含义 |
|---|---|
| `summary` | 文件、设计单元、活动诊断、顶层、实例、时钟/复位、时序过程、认证 CDC 同步器、suppression 及 baseline 命中数 |
| `lineStatistics` | 原始源码物理行数统计，包含 `total`、`languages[]` 和 `files[]` |
| `configuration.severityOverrides` | 本次分析按顺序应用的规则级别覆盖 |
| `design.units` | module/entity 定义、来源位置、顶层标记、时钟/复位/时序过程摘要及其直接实例 |
| `violations` | 统一规则诊断，包含规则、级别、说明、建议和来源位置 |
| `errors` | 输入读取或分析级错误；非空时 CLI 返回 2 |

`summary.severityCounts` 包含 `critical`、`error`、`warning`、`info` 四个活动诊断计数；
`summary.ruleCounts[]` 按规则 ID 排序，包含 `ruleId` 与 `count`。两者都基于 severity override、
suppression 和 baseline 处理后的最终活动诊断，因此各 severity 数之和等于 `summary.violations`。
SARIF run properties 发布同构统计，便于 CI 无需遍历 results 即可生成门禁摘要。

V1.1 在 schema 3 中新增可选顶层 `lineStatistics`。`total` 及每个 `lines` 对象均含
`total`、`code`、`comment`、`blank` 整数计数；`languages[]` 包含 `language`、`files`、`lines`，
`files[]` 包含 `file`、`language`、`lines`。SARIF `runs[].properties.lineStatistics` 发布工程
总计。完整口径见 [行数统计](line_statistics.md)。

`design.units[].instances[].resolved` 表示目标是否能在本次输入的设计单元集合中唯一解析。
未解析目标可能是缺失源码，也可能是有意作为外部 IP/库单元使用；对应规则默认为 info。
声明为 black box 的实例同时具有 `resolved: true` 和 `external: true`，但不会作为内部层次边
参与递归检查。

`design.units[].ports[]` 包含 Verilog/SystemVerilog module 或 VHDL entity 的端口名称、
`input|output|inout` 规范化方向、`hasDefault` 和声明位置；VHDL `buffer` 规范化为 output。
`instances[].namedConnections[]` 包含 module named connection 或 VHDL `port map` named
association 的 formal、`actualConnected` 及其连接位置。它们用于对已解析目标执行重复 formal、未知 formal 和必需输入连接
检查；VHDL 名称按大小写不敏感语义比较。每个 instance 还包含 `portConnectionCount`、
`hasPortConnectionList`、`connectionStyle`（`none|named|positional|mixed`）和
`hasWildcardConnection`，用于数量、风格及 Verilog `.*` 保守排除
检查。`positionalConnections[]` 按声明映射顺序公开每个 positional actual 的来源位置和
`actualConnected`；空 actual 与 VHDL `open` 为 false。
实例还包含 `invalidPortAssociationOrder` 和 `invalidParameterAssociationOrder`；当 VHDL map
在 named association 后再次出现 positional association 时对应字段为 true。

`design.units[].parameters[]` 包含 Verilog/SystemVerilog module parameter 或 VHDL entity
generic 的名称、`hasDefault` 和声明位置。实例的 `namedParameterOverrides[]` 保留 formal 与
来源位置，`parameterOverrideCount` 和 `parameterOverrideStyle`（`none|named|positional|mixed`）
描述 override 形状；VHDL generic 名称按大小写不敏感语义检查，Verilog/SystemVerilog
parameter 名称保持大小写敏感。positional override 目前只记录数量和风格，不展开逐项映射。

`design.units[].clocks[]` 包含时钟信号、`rising|falling` 边沿和来源位置；
`design.units[].resets[]` 包含复位信号、`high|low` 有效电平、`asynchronous` 属性和来源位置。
这些字段是基于时序过程事件与常见复位条件提取的结构摘要，不等同于 CDC/RDC 签核结果。
reset 项还包含 `synchronousObserved`、`asynchronousObserved`、`activeHighObserved` 和
`activeLowObserved`，因此混合模式或极性冲突不会被折叠后的单一属性掩盖。

`design.units[].sequentialProcesses[]` 包含过程的主 `clock`、`edge`、来源位置，以及确定性排序的
`reads`、`writes` 和 `indexedSignals` 集合、过程自己的 `resets[]` 及全部 `clockEvents[]`。
每个 clock event 包含信号、边沿和来源位置；过程 reset 包含信号、有效电平、同步/异步属性
和来源位置。`summary.sequentialProcesses` 是这些过程的总数。
VHDL sensitivity list 中具有 reset/rst/clear/preset 命名特征的控制信号也会作为异步过程 reset
保留，即使过程体遗漏了对应分支，以便 `RTL-UNUSED-ASYNC-RESET` 提供可审计诊断。
该模型用于单设计单元内直接 CDC 和异步复位跨多时钟域检查；它不是完整的跨层次 CDC/RDC
传播或签核结果。VHDL entity 与 architecture 可来自不同输入文件；项目分析会在输出报告前
按唯一 entity 名称合并其时序过程和时钟/复位摘要。

`summary.cdcSynchronizers` 是已认证两级标量同步链总数；
`design.units[].cdcSynchronizers[]` 包含 `sourceSignal`、`firstStage`、`secondStage`、
`destinationClock`、`edge` 和目的过程来源位置。该数组表达工具实际用于排除
`CDC-DIRECT-CROSSING` 的可审计证据，并不代表所有 CDC 结构或整项 CDC 签核已经完成。

`summary.baselinedViolations` 是被诊断基线匹配并从活动结果中移除的数量；它与源码/配置
豁免产生的 `suppressedViolations` 分开统计，避免历史债务不可见。

每个活动 violation 的 `severity` 已是覆盖后的最终级别。SARIF run properties 与 HTML/text
同样记录覆盖项，便于归档结果复现；SARIF 规则目录中的 defaultConfiguration 仍表示产品默认值。

消费者必须先检查 `schemaVersion`。新增可选字段可在同一主版本内添加；删除字段、改变类型
或改变已有字段语义时必须提升 schema 版本。
