# V1.4.114 R-6-4 完整 architecture 根常量静态性

本版本只推进规则逆向、判断逻辑和验证，不调整界面或表格布局。对外规则编号继续严格采用官方三段式 `R-X-X` / `A-X-X`。

## 原生证据闭环

- `GJB9765_VHDL_4_6_c` slot 0 `FUN_1806016c0` 按设计根递归收集 `AST_LOCALPARAM(0x0a)`，不是只读取 architecture 顶层声明区。
- 直接常量或前端已折叠子节点进入认可集合；直接名称及复合表达式中的全部名称必须在先前认可集合中，否则当前常量进入拒绝集合并向后传播。
- 原生函数另行收集带标记的 `AST_SIGNAL(0x78)` / `AST_VARIABLE(0x79)`，证明 signal/variable 是本规则的运行期名称来源。
- 这些结论来自附件 `T-Linter.dll` 的离线静态反编译，不代表运行了附件软件。

证据：`docs/evidence/native_vhdl_constant_static_initializer_4_6_c_findings.md`；保留证据 ID `ev_5f591cf433b08c94fd59aae3b050b90a666c6db29bb7ea7f776975767cec926c`。

## 产品修正

- `R-6-4` 的 constant、signal、variable 和枚举文字收集范围从 architecture 声明区扩展到完整 architecture 设计根。
- 新增 process、block 和 function/procedure 声明区内局部常量的检查；依赖局部 signal/variable 时报告，依赖先前静态常量时认可。
- architecture 范围优先由 `end architecture` 或 `end <architecture-name>` 终止符确定，每个 architecture 独立维护认可/拒绝状态，避免相邻设计单元串扰。
- 包限定常量、用户函数、跨设计单元名称和其他无法完成静态绑定的表达式仍不猜测违规，也不判定合规；无诊断保持 `ManualReviewRequired`。

## 新增验证

- process 局部 variable 驱动局部 constant：报告一次。
- architecture 内 procedure 局部 variable 驱动局部 constant：报告一次。
- block 局部 signal 驱动局部 constant：报告一次。
- process 内先前静态 constant 驱动后续 constant：不报告。
- 前一 architecture 的动态常量与后一 architecture 的静态常量：只报告前者，证明根状态隔离。

## 仍未知

节点状态字节的正式字段名、前端常量折叠边界、无初始化 constant/formal 的专有分类、嵌套同名对象的作用域遮蔽、package body、纯函数/用户函数、包限定和跨单元绑定、重载、物理类型、生成域、错误恢复及原生诊断顺序仍未知。产品不以无报告宣称合规，完整厂商算法等价仍为 0/209。

## 发布验证

- 完整测试：135/135 通过；规则目录、标准准确性、规则说明和版本号定点复验通过。
- 附件审计：544 条配置/文档记录、537 个随包页面、209 条标准规则、446 个正反例；其中 435 个自动规则示例吻合、6 个工程上下文示例吻合、4 个示例明确保持上下文不足、1 个附件标注与 DLL 固定默认谓词冲突。
- 原生实现台账：209/209 条均为官方三段式 `R-X-X` / `A-X-X`，209 条核心谓词均有接入记录，完整厂商算法等价仍为 0/209。
- 从全新解压目录运行 `--version` 返回 `1.4.114`；归档内应用深度签名校验通过。
- 全新解压运行夹具：process 局部 variable 依赖产生 1 条官方编号 `R-6-4`；双 architecture 夹具只报告前一根的动态常量、后一根静态常量不报告；两者均无分析错误。
- 发布包路径：`releases/1.4.114/FPGA-HDL-Static-Analyzer-1.4.114-Darwin-arm64.zip`。SHA-256 在最终归档生成后记录于工作区发布说明，不写入归档内副本，以避免归档哈希自引用变化。
- 工作区发布归档 SHA-256：`60dc3f172cbf455eedc87ed2abf0a5f254ba8ac69a4458440b6d95b7fb273c3c`。
