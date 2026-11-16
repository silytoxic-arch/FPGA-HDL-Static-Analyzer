# FPGA HDL Static Analyzer V1.4.22

本版继续只处理规则集，不调整界面。

## GJB9765.4.14.d.1、d.2 原生读取块算法修正

DLL 静态证据确认两条规则共享 `fopen("r")`、`fgets(..., 0xff, ...)` 风格的原始
文本扫描，而不是词法注释或 HDL 语句分析。V1.4.22 已接入：

- Windows 文本模式的常规 CRLF→LF 处理；
- 每个读取块最多 254 个数据字节，块序号作为诊断行号；
- 首个字面 `--` 截断，字符串中的双连字符同样生效；
- 跨块的两个 `-` 不会被合并为注释；
- d.1 只排除 ASCII Tab、LF、空格，超过 72 个其他字节报告；
- d.2 统计注释前所有分号，包含字符串内分号，超过一个报告。

新增 72/73 字节、字符串、注释及 254 字节分块边界测试。NUL、Ctrl-Z、代码页、I/O
错误和重复源记录仍缺少运行态验证，因此无告警继续保持 `ManualReviewRequired`。
证据见 [d.1 逆向记录](evidence/native_vhdl_line_length_4_14_d_1_findings.md) 和
[d.2 逆向记录](evidence/native_vhdl_statement_count_4_14_d_2_findings.md)。

## 验证与附件审计

- 全量自动化测试：135/135 通过；
- 标准规则：209 条；附件示例探测：446 个；
- Automated：435 个吻合、1 个差异；唯一差异仍是已记录的 5.2.15 HTML 与 DLL 冲突；
- ProjectContext：4 个可判定示例吻合；6 个缺少工程上下文的示例保持
  `context-required`；
- 台账继续记录 `fullAlgorithmsVerified: 0`。
