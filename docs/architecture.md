# 分层与接口

## V1.0 的实际分析链路

```text
file/directory inputs or recursive HDL file lists
        │
        ├── .fpga-lint.conf / CLI options ── file-list / excludes / rules / black boxes / -I / -D
        │
        ▼
analysis::Analyzer ── source discovery / language detection / built-in rules
        │
        ├── analysis::ruleCatalog ── canonical IDs / severities / languages / guidance
        ├── parser::preprocess       ── conditionals / defines / include resolution
        ├── parser::checkSyntax      ── lexical and structural sanity diagnostics
        ├── parser::parseDesignUnits ── units / ports / parameters + generics / instances / actual state
        │                                      │
        │                                      └── hierarchy / unique declarations / association rules
        ├── parser::analyzeSemantics ── reads / drivers / sensitivity / definite-assignment flow
        │                                      ├── combinational dependency graph / SCC loops
        │                                      ├── declaration/literal + selected/concatenated source and target widths / truncation checks
        │                                      ├── Verilog implicit nets at simple assignment/port endpoints
        │                                      ├── clock-event signals / local-driver correlation
        │                                      ├── scalar edge-source validation / safe parenthesis unwrapping
        │                                      ├── procedural / continuous simulation delays
        │                                      ├── scoped case choices / duplicate + wildcard overlap
        │                                      ├── scoped if/elsif chain duplicate conditions
        │                                      ├── conservative runtime branch/loop/conditional-expression constant evaluation
        │                                      ├── branch/loop/conditional-expression comparison width checks
        │                                      ├── side-effecting Verilog/SV branch/loop/ternary condition checks
        │                                      ├── empty Verilog/SV conditional branch checks
        │                                      ├── empty Verilog/SV case-item checks
        │                                      ├── duplicate Verilog default / VHDL when-others checks
        │                                      ├── direct/indexed/sliced case selector width checks
        │                                      ├── unreachable unsized Verilog case value checks
        │                                      ├── constant shift amount versus operand width checks
        │                                      ├── negative literal shift amount checks
        │                                      ├── literal divide/modulo/remainder-by-zero checks
        │                                      ├── plain Verilog always suspension-point checks
        │                                      ├── explicit sensitivity uniqueness / coverage analysis
        │                                      ├── asynchronous reset edge / branch polarity agreement
        │                                      ├── asynchronous reset leading-branch isolation
        │                                      ├── asynchronous reset full-body guarding
        │                                      ├── always_ff event-count + edge-presence validation
        │                                      ├── direct procedural self-assignment checks
        │                                      ├── empty procedural body checks
        │                                      └── scoped Verilog parameter use checks
        ├── project-level VHDL generic use ── entity + all analyzed architecture files
        ├── parser::extractTimingInfo ── all process clock events / resets / sequential read-write sets
        │                                      ├── VHDL edge functions + signal'event/level idioms
        │                                      ├── project merge for split VHDL entity / architecture files
        │                                      ├── cross-process mode / edge / polarity consistency
        │                                      ├── sensitivity reset use consistency
        │                                      ├── multiple clocks in one sequential process
        │                                      ├── intra-unit direct CDC + proven scalar synchronizer recognition
        │                                      └── intra-unit asynchronous reset multi-domain analysis
        │
        ▼
analysis::AnalysisResult ── hierarchy + certified CDC snapshot + source-mapped/suppressible Violation records
        │
        ├── severity policy ── ordered per-rule overrides / effective levels / CI threshold
        ├── analysis::applyBaseline ── counted stable fingerprints / new findings only
        │
        ├── report::writeText  ── compiler-style terminal/IDE diagnostics
        ├── report::writeJson  ── native versioned data / hierarchy / deterministic summaries
        ├── report::writeSarif ── SARIF 2.1.0 + run severity/rule aggregation
        └── report::writeHtml  ── offline dashboard / severity cards / rule summary
```

桌面端通过 QtConcurrent 在后台调用同一 Analyzer；QFutureWatcher 完成信号回到 GUI 线程，
再填充层次树、问题表、详情和源码视图。GUI 不直接执行规则，也不复制报告逻辑。

CLI 只负责参数、报告格式和退出阈值，不包含规则逻辑。结构提取器不依赖 UI，并保留
设计单元和实例的来源位置。报告层只消费结果，不重复执行分析。`fpga_core` 仍保持纯
C++20，Qt 只存在于可选的 GUI 目标中。

## 基础工程依赖

```text
fpga_analyzer -> fpga_gui -> fpga_core
                       -> Qt6::Widgets / Qt6::Concurrent
fpga_analyzer_cli ------> fpga_core
```

`fpga_core` 暴露数据模型、分析器与报告接口；CLI 参数处理通过传入流和参数视图独立
测试。GUI 使用 Qt 父子对象管理生命周期，并通过后台任务连接共享分析器及报告导出接口。

## 完整语法/语义分析阶段的接口方向

先定义来源位置、诊断和输入契约，再引入解析后端；不要让 UI 控件、QString 或 QWidget 出现在核心公共接口中。

| 边界 | 建议契约 | 需要先确定的问题 |
|---|---|---|
| project → parser | SourceFile / ParseOptions | 文件稳定 ID、语言与标准版本、include 目录、宏、VHDL library |
| core | SourceLocation / SourceRange / Diagnostic | 行列从 1 起、范围末端不含、字节偏移与 Unicode 显示列区分 |
| parser → database | ParseResult | 不可变语法/设计数据、解析诊断、部分失败状态、来源映射 |
| database → analysis | DesignSnapshot | 对象 ID 稳定性、快照所有权、跨文件引用与未解析符号 |
| analysis → rules | RuleContext / RuleResult | 规则输入能力、适用语言、确定性排序、只读访问 |
| analysis → CLI/GUI | AnalysisRequest / AnalysisResult | 取消、阶段进度、未运行/失败/完成状态区分 |
| report | ReportWriter | 消费统一诊断，不重复执行分析 |

建议以 `std::filesystem::path` 表达文件路径，在 Windows 边界处理宽字符与 UTF-8；避免用本地代码页损坏中文路径。源码缓冲区必须拥有数据，诊断和设计节点不能保存悬空 string_view。

当前 token 语义层已用小型 Verilog/VHDL fixture 验证来源位置、端口/内部信号读写和驱动
所有权，GUI 也已在工作线程执行分析并由主线程更新控件。后续完整 parser 适配仍应沿用上述
边界，并补充取消、阶段进度、语言标准与 library 映射契约。
