# HDL 代码行数统计

V1.1 新增类似 LineCount 的代码规模统计功能，适用于 Verilog、SystemVerilog、VHDL 工程。

## 桌面端

打开工程并运行分析，切换底部的“代码行数统计”面板。表格包含工程总计、按语言汇总、逐文件
明细，支持点击表头排序（行数按数字排序），双击文件行打开源码。通过“文件 → 导出行数统计
CSV”保存统计，CSV 使用带 BOM 的 UTF-8 编码，可在 Windows Excel 中直接查看中文。
HTML 报告也包含统计卡片、语言汇总和逐文件表格。

## 命令行

只统计行数，适合尚未完整编写或无法通过语法检查的工程：

```sh
fpga_analyzer_cli --line-count ./rtl
fpga_analyzer_cli --line-count --format json -o line-count.json ./rtl
fpga_analyzer_cli --line-count --format html -o line-count.html ./rtl
fpga_analyzer_cli --line-count --format csv -o line-count.csv ./rtl
fpga_analyzer_cli --line-count -f sources.f --exclude "*/generated/*" --format csv -o line-count.csv
```

普通 Lint 分析也自动附带行数统计。`--line-count` 会跳过解析、预处理和规则检查，不执行诊断
基线操作，不能与显式 `--baseline`、`--write-baseline` 同时使用。统计模式返回 0 表示输入读取
成功；输入错误或无可分析 HDL 文件返回 2。空文件仍属于一个成功读取的文件，行数为零。

## 统计口径

- 总行数：原始源码物理行数。空文件为 0；末尾换行不增加虚构的空白行；支持 LF、CRLF。
- 代码行：去除注释后仍含非空白代码；代码与注释混合的行归为代码行。
- 注释行：没有代码、包含行注释或块注释的行；块注释内部的空行也归为注释行。
- 空白行：既没有代码也不处于注释中的空行或仅含空白的行。

始终满足 `总行数 = 代码行 + 注释行 + 空白行`。字符串中的注释标记不会被视为注释，
支持 Verilog/SV 转义字符串和转义标识符、VHDL 双引号转义与字符字面量、块注释跨行状态。
文件开头的 UTF-8 BOM 不视为代码。

统计复用工程的 HDL 输入发现、文件清单、排除规则和路径去重；统计原始文件，包含未激活的
条件编译分支。include 引用不会自动展开增加行数，只有它作为目录/文件清单的实际输入时
才计入统计。这里只统计 HDL 文件，不包含 Tcl、XDC、C/C++ 或二进制资源；结果属于物理行数，
不代表逻辑语句数、设计复杂度或综合后资源数量。
