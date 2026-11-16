# 报告格式

CLI 通过 `--format text|json|sarif|html|csv` 选择格式，通过 `-o/--output FILE` 直接写入
文件；不指定 output 时写入标准输出。报告写入失败返回退出码 2。

## Text

开头包含原始 HDL 源码行数总计、语言汇总和逐文件明细。

编译器风格的 `file:line:column: severity rule: message`，适合终端、编辑器 problem
matcher 和快速日志查看，末尾包含分析、设计层次及已认证 CDC 同步器数量摘要。
摘要还包含最终有效 critical/error/warning/info 数量，以及按规则 ID 排序的命中次数。

## JSON

FPGA HDL Static Analyzer 的完整原生数据协议，包含诊断、设计单元、实例层次、时钟/复位、时序过程
摘要、已认证 CDC 同步器链、black box 状态、suppression 和 baseline 命中统计。字段契约见
[JSON 报告协议](report_schema.md)。
报告还记录按顺序生效的严重级覆盖，活动诊断的级别均为策略处理后的最终值；summary 提供
`severityCounts` 与确定性 `ruleCounts`。

## SARIF

输出 OASIS SARIF 2.1.0 + Errata 01，可供支持 SARIF 的 CI、安全扫描和 IDE 工具消费。
每条结果包含 ruleId、SARIF level、消息及物理文件/行/列；规则目录包含说明、建议和默认
级别，并始终登记全部内建规则，而不只登记本次命中的子集。官方规范及 schema：
run properties 同步发布 severity/rule 聚合，便于 CI dashboard 直接消费。

- https://docs.oasis-open.org/sarif/sarif/v2.1.0/sarif-v2.1.0.html
- https://docs.oasis-open.org/sarif/sarif/v2.1.0/errata01/os/schemas/sarif-schema-2.1.0.json

```sh
fpga_analyzer_cli --format sarif --output fpga-lint.sarif ./rtl
```

## HTML

生成 UTF-8 单文件静态报告，不引用网络资源、不执行 JavaScript，包含 severity 指标卡、
rule summary、问题明细、
时钟/复位、过程读写、已认证 CDC 链及直接实例层次表。所有来自源码和诊断的文本均进行 HTML 转义，可直接离线打开或归档。

```sh
fpga_analyzer_cli --format html -o fpga-lint-report.html ./rtl
```

## CSV

输出中文行数统计表，包含工程总计、语言汇总、逐文件明细，使用 UTF-8 BOM 和 CRLF，
支持 Excel 直接打开。完整口径见 [代码行数统计](line_statistics.md)。

```sh
fpga_analyzer_cli --line-count --format csv -o line-count.csv ./rtl
```
