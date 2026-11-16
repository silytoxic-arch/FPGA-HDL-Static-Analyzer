# V1.4.65 规则恢复说明

本版继续只处理规则集、逆向证据与验证台账，没有调整界面或表格。

## GJB9765.4.8.d 运算符函数与嵌套子程序

V1.4.64 已由 `FUN_1801c7b80` 和 `FUN_18077c840` 证明：规则接纳带来源标志/句柄的 VHDL IF，不要求祖先为 process。V1.4.65 补齐产品源码适配器中仍会漏掉的两种合法子程序结构：

- function designator 是字符串形式的运算符符号，例如 `function "+"` 或 `function "and"`；
- 外层子程序声明区包含内层子程序体，内层 `BEGIN` 不再被误当成外层主体，内外两层分别扫描。

新增回归覆盖：

- 运算符函数缺少最终 ELSE：一报告；
- 运算符函数带最终 ELSE 且以字符串 designator 命名结束：零报告；
- 嵌套函数与外层函数各有一个缺少 ELSE 的 IF：两条独立报告。

## 证据与限制

- 目标 `T-Linter.dll` SHA-256：`60e6b52875f4d7c3fbdf494d869b3a4bcc3fdc8ee63d8620ea9614da5d6599b0`；
- 来源 IF 元数据生产者仍引用 `reverse-tools/evidence/remaining_rules/field_183_writers_and_metadata.c`，SHA-256 `f83b1a90df4df99fdf7241cda6f59af8a212f5f067340bf57d6a4044d25838e5`；
- 错误恢复、所有扩展标识符结束形式、嵌套 package/generic subprogram 特殊形态和原生报告顺序仍保持未知；无告警不用于证明这些路径合规。

## 发布验证

- Release 配置完整构建通过，CTest 135/135 全部通过；
- 附件 446 个文档样例审计保持稳定：435 个 `Automated:matched`、6 个 `ProjectContext:matched`、4 个 `context-required`、1 个 `native-documentation-conflict`，无普通不匹配；
- 可重复生成的规则台账保持 209/209 条核心判断已接入、完整原生算法等价 0/209；
- 最终安装包：`releases/1.4.65/FPGA-HDL-Static-Analyzer-1.4.65-Darwin-arm64.zip`；
- 最终包体积与 SHA-256 记录在发布目录的外部 `SHA256SUMS`，避免把 ZIP 自身哈希写入包内文档形成自引用；
- 最终 ZIP 的独立验证覆盖 CLI 版本、包内说明占位清理和 `codesign --verify --deep --strict`。
