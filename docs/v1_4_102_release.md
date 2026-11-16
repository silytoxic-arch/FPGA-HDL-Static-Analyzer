# V1.4.102 A-5-31 跨文件设计根绑定

本版本只推进规则逆向、判断逻辑和验证，不调整界面或表格布局。

## 新恢复的原生边界

重新静态反编译 `FUN_1804ca4c0` 后确认：

- 输入是一个已经绑定的当前设计根，而不是一个物理源文件。
- 根的 `AST_PORT(0xa4)` 直接子节点中，类型 6 声明的名称会被转为小写并写入候选树。
- mode 为 1 时，同根 `AST_ARCHITECTURE(0x75)` 的类型 `0x78` 直接声明也写入同一候选树。
- 每个树项同时保存小写名称和原声明指针。
- 函数没有源文件路径比较或同文件限制。

结合 V1.4.101 已恢复的逐根清空逻辑，可确定原生规则能够在一个已绑定根内联合处理分属不同文件的 entity 端口与 architecture 声明。

证据：`reverse-tools/evidence/a5_31_root_port_architecture_collector.txt`，SHA-256 `b93fab8ab2b02b0830c806f64ed0cef59ea0effe4c4d12d499d28b3eef9b67fc`。

## 产品修正

当 architecture 所属 entity 位于另一个 VHDL 文件时：

- 工程中折叠名称恰好匹配一个 entity，组合该 entity 端口和 architecture 本地声明、复位种子及直接赋值关系；
- 端口命名问题定位回 entity 声明文件，而不是错误定位到 architecture 文件；
- 工程中存在两个同名 entity 时，不按文件顺序猜测绑定，不生成推测性违规，规则状态保持人工复核。

## 回归覆盖

- entity 与 architecture 分属两个文件，复位别名从 `rst_alias` 向上游端口 `bad_source` 传播并报告；
- 同一跨文件根中的非复位 `rst_payload` 独立报告；
- 两条端口诊断均定位到 entity 文件；
- 两个同名 entity 加一个 architecture 时不猜测绑定，并保持 `ManualReviewRequired`。

## 官方编号

本规则对外编号为 `A-5-31`。全部标准规则继续采用官方三段式 `R-X-X` / `A-X-X`；内部 GJB 条款仅用于绑定和追溯。

## 仍未知

原生库限定与 binder、多个 architecture 的 configuration 选择、重复库单元解析、生成结构、表达式/调用/端口/层级别名、原生配置词实际绑定值和精确诊断顺序仍未完全恢复。未绑定或绑定不唯一的路径不判合规，完整厂商算法等价保持 0/209。

## 发布验证

- 完整测试为 135/135，通过 `standard_accuracy_unit`、GUI 工作流和其余回归。
- 附件审计保持稳定：544 条产品规则页、537 个已打包页面、209 条标准规则、446 个例程；其中 435 个自动匹配、1 个原生文档冲突、6 个工程上下文匹配、4 个必须依赖工程上下文。
- 209/209 条标准规则的公开编号均符合官方三段式 `R-X-X` / `A-X-X`。
- 发布包：`releases/1.4.102/FPGA-HDL-Static-Analyzer-1.4.102-Darwin-arm64.zip`。
