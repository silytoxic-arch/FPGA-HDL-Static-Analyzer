# T-Linter GJB10157 / GJB9765 规则集逆向与落地说明

## 结论与边界

本次工作同时使用用户提供的 T-Linter 发布物和两份标准 PDF，恢复两个规则集的目录、严重级、
强制属性、规则语义与正反例判据，并将其接入 FPGAAnalyzer：

- GJB10157-2021（Verilog）：观察到 103 条规则，其中 65 条为强制规则；
- GJB9765-2020（VHDL）：观察到 106 条规则，其中 39 条为强制规则；
- 共导入 209 条条款目录和 104 条强制属性；
- 209 条全部具有唯一条款、严重级、强制属性、摘要、正确案例判据、违反案例判据和评估状态；
- 1.2.3：76 条接入自动检测（含部分覆盖），11 条要求工程上下文，122 条进入人工复核；
- 1.2.7：时钟/控制交集部分接入后，目录为76条自动（含部分）、11条工程上下文、122条人工复核；14条核心谓词部分接入，完整算法等价验证为0条；无报警不代表合规。
- 1.2.8：命名与FSM数量/编码部分接入后，目录79条可执行检测器、11条工程上下文、119条人工复核；20条核心谓词部分接入，完整算法等价验证0条。见 [本轮记录](native_rule_recovery_pass6.md)。
- 1.2.9：电源保留名称部分接入后，目录80条可执行检测器、11条工程上下文、118条人工复核；21条核心谓词部分接入，完整算法等价验证0条。见 [本轮记录](native_rule_recovery_pass7.md)。
- 1.2.12：目录80条可执行检测器、12条工程上下文、117条人工复核；24条核心谓词部分接入，完整算法等价验证0条。未报警条款统一要求复核。见 [本轮记录](native_rule_recovery_pass10.md)。
- 1.2.16：追溯5.2.9原生 AST_ARGUMENT 空名称及每根节点只报一次的谓词，收敛位置端口诊断；27条核心谓词部分接入，完整算法等价验证0条。见 [本轮记录](native_rule_recovery_pass14.md)。
- 1.2.15：追溯5.2.7原生逐行冒号判据，接入有界引号路径判断并保留独立的Unix绝对路径检查；26条核心谓词部分接入，完整算法等价验证0条。见 [第十三轮记录](native_rule_recovery_pass13.md)。
- 1.2.14：追溯5.2.6原生比较谓词、声明类型及逐模块分组，修正跨模块误报；25条核心谓词部分接入，完整算法等价验证0条。见 [第十二轮记录](native_rule_recovery_pass12.md)。
- 1.2.13：追溯5.2.5原生对象排除位和AST源文件键，改进多模块定位；24条核心谓词部分接入，完整算法等价验证0条。见 [第十一轮记录](native_rule_recovery_pass11.md)。
- 1.2.6：撤销4.11.a泛case/latch映射后，目录为75条自动（含部分）、11条工程上下文、123条人工复核；11条核心谓词部分接入，完整算法等价验证为0条；
- GUI 的规则集选择器只提供 GJB10157 与 GJB9765 两项。

209 条均进入结果目录，但不代表 209 条算法已实现。V1.2.5 只将自动检查未发现目标问题记为
`no-violation-found`，明确不等于标准合规；已确认仍为部分覆盖的原生条款、需要工程信息或
设计意图的条款保留 `manual-review-required` 或 `project-context-required`。V1.2.12的209条中没有
完整算法等价验证通过的规则，故所有未报警条款都要求复核。
逐条真实恢复/接入进度见 [实施台账](evidence/native_rule_implementation_ledger.json)。

## 证据来源

分析对象来自 `/Users/pinger/Desktop/新建文件夹/t_linter/`。为保证结果可复核，关键文件的
SHA-256 如下：

| 文件 | SHA-256 |
|---|---|
| `GJB10157.lprp` | `a5c6bd2b1d0817113795cc84f12a5559fd2841f5723004d75bf9228072ffcfdf` |
| `GJB_10157_COMPULSORY.lprp` | `b8d3fb2a1dc88d5ed4336636c2bfc81ac48410ce23bcce355b36fa8a65e9be11` |
| `GJB9765.lprp` | `05ecbfc21b8b8ee7153220d5ae5bdf058a3e52798a89f3235734876f8ed2cc70` |
| `COMPULSORY.lprp` | `40fc7d6e48dc77fd756358ace31ae8fac7b307e25807af7c84d5e51a1cccd06c` |
| `T-Linter.dll` | `60e6b52875f4d7c3fbdf494d869b3a4bcc3fdc8ee63d8620ea9614da5d6599b0` |
| `T-Linter-Console.exe` | `c8231eb0fdbcd609ae4c05a8df53d8a3bd2a218416758924414a5848dc67191b` |

`LinterDB.db` 也被检查：它只有通用规则分类字段，没有保存 GJB 条款的完整语义说明，因此不能用来
恢复标准原文。

## 逆向观察

以下是发布物直接支持的观察结果：

1. `.lprp` 是 UTF-8 XML 规则配置文件，保存规则标识、启用状态和 Error/Warning 级别；
2. `GJB10157.lprp` 中 103 条规则全部启用，64 条为 Error、39 条为 Warning；
3. `GJB9765.lprp` 中 106 条规则全部启用，40 条为 Error、66 条为 Warning；
4. `GJB_10157_COMPULSORY.lprp` 标出 65 条 GJB10157 强制规则；
5. `COMPULSORY.lprp` 同时包含 65 条 GJB10157 和 39 条 GJB9765 强制规则；
6. GJB10157 中 `GJB10157.6.1.6` 是强制规则但配置级别为 Warning；
7. GJB9765 中 `GJB9765.4.7.a` 配置级别为 Error，但不在强制规则集合中；
8. `T-Linter.dll` 包含各 GJB 规则实现类名，并在部分实现附近暴露了稳定的 HDL 关键字字符串。

据此推断的组件关系为：`.lprp` 负责选择规则和严重级别，`T-Linter.dll` 承载检测实现，
Console/GUI 负责加载配置和呈现结果。该组件关系是基于发布物结构的工程推断，不是厂商公开设计说明。

## 历史字符串线索与当前实现（不是算法恢复证据）

| 标准条款 | FPGAAnalyzer 规则 | 覆盖状态 | 直接证据与实现边界 |
|---|---|---|---|
| GJB10157.5.2.16 | `VL-DEFPARAM` | implemented | 对应实现附近出现 `defparam`；现已检测该语句 |
| GJB10157.9.2.8 | 直接 token 检查 | implemented | 结合附件规则页，检测 casex/casez；未恢复厂商函数算法 |
| GJB10157.9.2.9 | 直接 token 检查 | partial | 结合附件规则页，检测 default 分支内 X；已撤销“缺少 default”错误映射 |
| GJB9765.4.11.a | 人工复核 | 未接入 FSM 模型 | V1.2.6 原生模型证据否定泛case/latch足以证明该条，已撤销映射；通用风险仍单独检查 |
| GJB9765.4.12.a | `RTL-NONSYNTHESIZABLE-DELAY` | partial | 撤销所有 wait 一律违规的映射；仅覆盖部分非综合结构 |
| GJB9765.4.12.c | `VHDL-BUFFER-PORT` | implemented | 对应实现附近出现 `buffer`；现已检测 entity buffer 端口模式 |

字符串邻接不能证明控制流、AST 判据或函数归属；上述历史线索不能称为厂商内部算法。
`implemented` 仅表示 FPGAAnalyzer 已有对应检测器，不代表标准认证或与厂商行为完全一致。
GJB9765.4.9.e 的附件示例与既有目录语义冲突，1.2.3 已改为人工核对。

2026-10-10：已安装 REA 6.3.0、Ghidra 12.1.4、JDK 21，并在独立只读会话载入 DLL。
第二阶段已建立 209 条规则的同名类构造/vtable 候选/前四槽函数索引，并提取 247 个唯一函数的伪代码。
GJB10157.9.2.8 的节点枚举及运行诊断路径已取得直接支持，但定制标志位含义与全部规则语义仍未完成。
历史正反例台账中的 `nativeFunction-not-recovered` 是原生成时间状态，当前进度见
[原生恢复第二阶段](native_rule_recovery_pass2.md)；不能把函数提取数量当成算法等价性完成数量。
阶段原生证据保存在工作区 `reverse-tools/evidence/t_linter_pass2/`，没有把厂商反编译代码复制进软件。
详见 [1.2.3 验证报告](v1_2_3_release.md) 和 [逐规则台账](t_linter_rule_audit.md)。

## 证据边界

- T-Linter 每条规则的 AST/数据流算法及所有触发边界；
- 厂商内部测试向量、误报抑制策略和认证结论；
- 二进制中没有稳定邻接字符串的规则，不能宣称复刻了厂商的内部算法。

因此 FPGAAnalyzer 的规则语义以附件 PDF 为主，T-Linter 仅用于核对规则标识、严重级、强制属性和
可观察实现证据；工程上下文或人工判断项保持显式待复核。

## FPGAAnalyzer 中的落点

- `src/analysis/standard_rule_inventory.inc`：209 条标准条款、严重级别和强制属性；
- `src/analysis/standard_rule_details.inc`：209 条 PDF 语义与评估方式；
- `src/analysis/standard_profiles.cpp`：标准元数据、别名、正反例判据和二进制证据映射；
- `src/analysis/standard_evaluator.cpp`：标准规则翻译、直接检测器及 209 条逐条状态输出；
- `src/parser/semantic_analyzer.cpp`：新增 `VL-DEFPARAM`、`VHDL-CASE-OTHERS`、
  `VHDL-BUFFER-PORT` 检测；
- `src/analysis/rule_catalog.cpp`：新增规则目录项；
- `src/analysis/rule_localization.cpp`：新增中文说明；
- `src/cli/command_line.cpp`：标准选择、逐条查询、禁用及严重级覆盖；
- `src/gui/main_window.cpp`：仅包含 GJB10157 与 GJB9765 的规则集选择器；
- `tests/`：209 条目录/语义/正反例判据完整性门禁、自动检测器双向注册门禁及 CLI 回归测试。

查询标准覆盖情况：

```sh
./build/v1-release/fpga_analyzer_cli --list-standards
./build/v1-release/fpga_analyzer_cli --list-standard-rules gjb10157
./build/v1-release/fpga_analyzer_cli --list-standard-rules gjb9765
```

使用标准规则集分析：

```sh
./build/v1-release/fpga_analyzer_cli --standard gjb10157 ./rtl
./build/v1-release/fpga_analyzer_cli --standard gjb9765 ./rtl-vhdl
./build/v1-release/fpga_analyzer_cli --describe-rule GJB10157.9.2.2
```

未指定 `--standard` 时保留原有通用规则行为，以免破坏既有 CLI 工作流；GUI 始终要求从上述两个标准中选择。

## 完整性门禁

1. 规则目录、详情和正反例判据数量必须都等于 209，条款 ID 必须一一对应；
2. 标为 `Automated` 的条款必须存在可执行检测器；注册了检测器的条款也必须标为 `Automated`；
3. 非自动和已知部分覆盖条款必须输出工程上下文或人工复核状态；自动未检出也禁止当作合规证明；
4. 规则集只允许 `gjb10157` 或 `gjb9765`，并检查输入 HDL 语言与标准匹配；
5. 标准条款 ID 可用于禁用规则、严重级覆盖和单条规则说明。

