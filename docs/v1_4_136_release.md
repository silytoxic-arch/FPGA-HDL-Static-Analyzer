# V1.4.136 R-9-4 类别 2 来源 IF 所有权

## 本轮恢复

对用户附件 `T-Linter.dll` 的 `FUN_1804c5070` 与 `FUN_1804d68a0` 进行联合静态恢复，确认：

- 来源 IF 收集器只接受设计根下固定的两个 `AST_BLOCK(0x54)` 层级中的 source-linked `AST_CASE(0x57)`，对应进程顺序语句块的直接顶层 IF；
- 类别 2 生产器只从来源 IF 的直接 `AST_COND(0x58)` 主条件取得时钟；
- 分支块首项链接的来源 IF 只生成内部关系记录和标志，不向 `R-9-4` 增加时钟名称；
- 同一进程内多个独立顶层 IF 会分别生成关系，仍应进行时钟名称与边沿比较。

上述结论来自离线反编译，不代表运行了附件程序。DLL SHA-256 为 `60e6b52875f4d7c3fbdf494d869b3a4bcc3fdc8ee63d8620ea9614da5d6599b0`。

## 产品接入

- `R-9-4` 不再要求进程体首条语句必须是 IF，而是枚举进程顺序语句块中的独立顶层 IF；
- 类别 2 只记录外层 IF 主条件的第一个可识别边沿；
- 排除同一 IF 的 ELSIF、分支嵌套 IF，以及 CASE/LOOP 内的 IF 作为独立类别 2 时钟；
- 多个独立顶层 IF 继续在同一进程的时钟集合中比较；
- 类别 1 的静态异步动作、类别 3 无边沿、类别 4 排除及大小写折叠逻辑保持独立。

## 新增验证

- 主条件 `rising_edge(clk1)` 加 `elsif rising_edge(clk2)`：0 条 `R-9-4`；
- 主条件 `rising_edge(clk1)` 的分支中嵌套 `rising_edge(clk2)`：0 条；
- 普通顺序语句后两个独立顶层 IF 分别使用 `clk1`、`clk2`：1 条；
- 顶层 `clk1` IF 加 LOOP 内 `clk2` IF：0 条；
- 既有类别 1 三时钟违反样例仍输出 2 条，证明未破坏异步复位 IF/ELSIF 链。

## 证据与边界

- 类别 2 生产器：`docs/evidence/native_vhdl_single_clock_process_4_9_d_category2.txt`，SHA-256 `6e09ba7698fe3d3092df8e3bfaf599e088d4413a6c40897ed2928449b9be6d9b`；
- 来源 IF 收集器：`docs/evidence/native_vhdl_single_clock_process_4_9_d_source_if_collector.txt`，SHA-256 `0da5ff5ef5b14872f54587a2e44c158cc0454611e05a3594d425765cd34951bc`；
- 详细结论：`docs/evidence/native_vhdl_single_clock_process_4_9_d_findings.md`。

复合 event 变体、别名传播、generate 过程、包限定名称、错误恢复树、类别 1 的全部嵌套构造、精确遍历顺序和厂商诊断格式仍未完全恢复。无证据支持的干净结果继续保持人工复核，不声明完整厂商等价。

## 发布验证

- 完整测试：`135/135` 通过。
- 附件审计：544 条厂商记录、537 个规则页、209 条标准规则、446 个示例；435 个 `Automated:matched`、6 个 `ProjectContext:matched`、4 个 `context-required`、1 个 `native-documentation-conflict`，相对 V1.4.135 无回退。
- 实施台账：209 条核心判据均有接入记录；`R-9-4` 已补充类别 2 来源 IF 固定层级、外层时钟所有权与链接 IF 内部记录用途；完整厂商算法等价验证仍为 `0/209`。
- 公开编号：209/209 条规则记录均匹配官方三段式 `^[RA]-[0-9]+-[0-9]+$`。
- 全新解压 CLI 版本为 `1.4.136`，应用包通过 macOS 深度严格签名校验。
- 全新解压边界样例：类别 2 ELSIF、嵌套 IF、LOOP 内 IF 的 `R-9-4` 数量均为 0；普通语句后的两个独立顶层 IF 为 1；四组均无分析错误，所有公开规则编号均为官方三段式。
- 发布包：`releases/1.4.136/FPGA-HDL-Static-Analyzer-1.4.136-Darwin-arm64.zip`。
