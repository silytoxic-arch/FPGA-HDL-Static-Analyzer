# V1.4.80：R-13-8 动态写索引容量检查

本轮只深化规则判断，不修改界面或表格布局。公开编号继续使用官方三段式 `R-13-8`。

## 原生恢复

- 在完整 DLL 前端简化器 `FUN_180914500` 中定位到精确属性 `is_over_array_index` 的生产路径。
- 该路径处理 `AST_ASSIGN_EQ/LE(0x55/0x56)` 的直接左值选择，调用原生位宽推导后比较 `declaration_width < 2^index_width`；严格小于时写入属性，相等时不写入。
- 规则入口 `FUN_18071f0a0` 消费该属性、排除带 `$` 的内部名称，并以 `Warning` 输出“信号<名称>索引超出指定的数组范围”。
- 详细证据见 [R-13-8 原生证据](evidence/native_verilog_array_index_9_4_8_findings.md)。

## 产品接入

- 保留固定 packed/unpacked 多维范围的常量索引、片选端点和不可覆盖直接数值 `localparam` 检查。
- 新增有界动态写适配器：直接阻塞/非阻塞赋值左值、单一已声明索引标识符、单维固定范围采用原生 `selected_width < 2^index_width` 条件。
- 容量等于声明宽度不报告；动态读、复合表达式、多维动态写、参数覆盖和 elaboration 不越权扩张，继续人工复核。

## 验证

- 自动化测试：`135/135` 通过。
- 新增动态写容量大于声明宽度的反例、容量相等的边界正例，以及动态读不套用写路径的负向边界测试。
- `R-13-8` 台账提升为 `evidence-supported-core-predicate; integrated-source-adapter`；完整厂商算法等价仍为 `0/209`。
- 209 条附件规则审计共探测 446 个示例：`Automated:matched` 435、`ProjectContext:matched` 6、`context-required` 4、原生与文档冲突 1，与 V1.4.79 基线一致。
- 发布包：`releases/1.4.80/FPGA-HDL-Static-Analyzer-1.4.80-Darwin-arm64.zip`。
- SHA-256：`4577b019a76a176f3b6f534383f95ddd585ab7b7c4b9de20c87b4a886e9da6a3`。
