# V1.4.116 R-6-4 package 设计根覆盖

本版本只推进规则逆向、判断逻辑和验证，不调整界面或表格布局。对外规则编号继续严格采用官方三段式 `R-X-X` / `A-X-X`。

## 原生证据闭环

- `GJB9765_VHDL_4_6_c` slot 0 `FUN_1806016c0` 对每条输入记录直接取得首个 AST 根，随后以该根调用递归收集器；入口没有 `AST_ARCHITECTURE` 类型判断。
- 每个输入根重新创建认可常量、拒绝常量、运行期对象和赋值记录容器。因此 package specification、package body 与 architecture 必须作为独立根处理，名称状态不能跨根传播。
- `AST_LOCALPARAM(0x0a)` 仍受 `+0x178` 节点标志门控。本版没有把源码中所有无初始化 constant 或形式常量擅自映射成该标志。
- 这些结论来自附件 `T-Linter.dll` 的离线静态反编译，不代表运行了附件软件。

证据：`docs/evidence/native_vhdl_constant_static_initializer_4_6_c_findings.md`；保留证据 ID `ev_5f591cf433b08c94fd59aae3b050b90a666c6db29bb7ea7f776975767cec926c`；完整保存的 Ghidra 伪代码位于工作区 `reverse-tools/Library/ghidra/home/Library/ghidra/ghidra_12.1.4_PUBLIC/application.log`。

## 产品修正

- `R-6-4` 的根集合从 architecture 扩展到顶层 package specification 与 package body。
- package body 内 function/procedure 的局部 signal/variable、constant、枚举文字和前置静态/动态常量使用与 architecture 相同的有界依赖分析。
- package specification/body 支持 `end package...`、`end <package-name>;` 和裸 `end;`，并维护嵌套子程序、本地 package 的结束栈。
- `package ... is new ...;` 实例化不被误作含声明的 package 根；architecture 内本地 package 也不会重复作为独立根执行。
- 每个 package/architecture 根重新建立状态表；包限定、跨根、用户函数或其他未绑定名称仍保持未知且无诊断结果为 `ManualReviewRequired`。

## 新增验证

- package specification 中 signal 驱动 constant：报告一次。
- package body procedure 中局部 variable 驱动 constant：报告一次。
- 命名结束 package body 中两级静态 constant 链：不报告。
- package specification 的动态常量后跟 architecture 中同名未知依赖：只报告 package 根，证明拒绝集合不会跨根传播。

## 仍未知

节点状态字节 `+0x178` / `+0x109` / `+0x10e` 的正式字段与全部生产者、deferred constant 的具体标志、前端常量折叠边界、嵌套同名对象的完整词法作用域、package specification/body 跨单元可见性、纯函数/用户函数、包限定名称、重载、物理类型、生成域、错误恢复及原生诊断顺序仍未知。产品不以无报告宣称合规，完整厂商算法等价仍为 0/209。

## 发布验证

- 完整测试：135/135 通过；新增 4 组 package 根正反例与隔离测试通过。
- 附件审计：544 条配置/文档记录、537 个随包页面、209 条标准规则、446 个正反例；其中 435 个自动规则示例吻合、6 个工程上下文示例吻合、4 个示例明确保持上下文不足、1 个附件标注与 DLL 固定默认谓词冲突。
- 原生实现台账：209/209 条均为官方三段式 `R-X-X` / `A-X-X`，209 条核心谓词均有接入记录，完整厂商算法等价仍为 0/209。
- 从全新解压目录运行 `--version` 返回 `1.4.116`；归档内应用深度签名校验通过。
- 全新解压运行夹具：package-body procedure 局部 variable 依赖产生 1 条官方编号 `R-6-4`；package specification 动态常量后跟 architecture 未绑定同名依赖时总计仍为 1 条，证明根状态隔离；两者均无分析错误。
- 发布包路径：`releases/1.4.116/FPGA-HDL-Static-Analyzer-1.4.116-Darwin-arm64.zip`。SHA-256 在最终归档生成后记录于工作区发布说明，不写入归档内副本，以避免归档哈希自引用变化。
- 工作区发布归档 SHA-256：`8d4b08e9e34b815d8ae9024f099a8d3287b8cb98a0133d2739efac43b1a260cb`。
