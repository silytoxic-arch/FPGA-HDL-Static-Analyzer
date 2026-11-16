# 验证记录

## V1.2.1（2026-10-10）

- 源码文件选择修复后，Release CTest 98/98、独立 CLI 回归 89/89 通过。
- GUI 测试覆盖两个页面真实按钮打开文件选择窗口、取消、Verilog/VHDL 多选、单文件、
  带空格文件名、取消后保留结果，以及切换标准后仍仅分析选中文件。
- 桌面实测从文件选择窗口打开 FLASH RTL 中的 `MultiBoot.v`，得到 1 文件、1 设计单元、
  5 实例和 7 条 warning。未修改原 RTL；不将此记录视为该目录全部源码的验收。

## V1.2.0（2026-10-10）

- 干净的 macOS arm64 Release 构建通过 CTest 98/98 和独立 CLI 回归 89/89。
- 整合 GJB10157-2021 的 103 条与 GJB9765-2020 的 106 条：77 条自动检测、11 条工程上下文、
  121 条人工复核；元数据与实际检测器注册一致性纳入测试。未自动判定的条款不标记为通过。
- GUI 工作流测试确认仅有两套标准、分别返回 103/106 条评估，后台分析期间锁定标准选择，
  同时验证问题定位、设计层次与行数统计页面。
- Windows 打包脚本修复工作目录依赖，并在原生命令返回非零时立即停止；本机未执行 Windows
  编译与验收，不将其列为已验证二进制。

## Figma 界面优化（2026-10-09）

- 第二轮高级简约优化：暖白/石墨灰配色、无边框指标、浅色源码区、低饱和状态色、精简提示和
  细滚动条；Figma 与 Qt 实际窗口均检查完成，Release CTest 保持 93/93 通过。

- 创建可编辑的中文分析/统计界面及指标卡片、导航组件、基础/语义变量。
- 将设计落实至 Qt 原生控件，保留真实数据、异步分析、问题定位和三种离线导出。
- Release CTest 93/93 通过，包含新增的真实工程 GUI 工作流测试。
- 离屏实际窗口截图已与设计对照检查；问题列宽、相对路径和长内容提示完成调整。
- Windows 源码兼容路径保持，未在本轮执行 Windows 二进制构建或界面验收。

## V1.1.0（2026-10-09）

- 新增原始 HDL 源码物理行数统计：工程总计、语言汇总和逐文件明细，分类为代码行、纯注释行、
  空白行；混合行归为代码行。复用文件清单、目录递归、排除规则和路径去重。
- 中文桌面端增加行数统计面板、数字排序、双击文件定位、CSV 导出；text/JSON/HTML 报告
  增加统计内容，SARIF 增加工程总计。CSV 包含 UTF-8 BOM，支持 Windows Excel 中文显示。
- `--line-count` 提供免解析的独立统计模式，支持未完成/语法错误 HDL，跳过预处理和规则检查。
- Release CTest 92/92、独立 CLI 回归 84/84 通过。新增专项验证覆盖空文件、末尾换行、CRLF、
  多行块注释、混合行、字符串/标识符中的注释标记、VHDL 字符字面量、UTF-8 BOM、聚合和 CSV 转义。
- Windows 仍使用已有 VS2022/Qt 原生构建流程；当前 macOS 主机不具备 Windows 二进制验证条件。

## V1.0.1（2026-10-09）

- 桌面端菜单、工具栏、面板、表头、状态栏、对话框、层次属性、严重级和问题详情统一改为
  简体中文；90 条规则均提供中文 GUI 摘要，规则 ID、HDL 关键字和机器可读报告字段保持稳定。
- 命令行帮助、纯文本报告和离线 HTML 报告的界面标签统一为简体中文；JSON/SARIF 的字段名、
  schema 和规则 ID 保持英文稳定标识，便于 CI 与第三方工具继续解析。
- 中文化后的 macOS arm64 Release 构建通过 CTest 90/90 和独立 CLI 回归 83/83；重新生成的
  1.0.1 ZIP 已验证深度签名、CLI 版本、中文帮助和随包 Windows 构建文档。
- 增加 Windows x64 / Visual Studio 2022 Release 测试 preset、PowerShell 一键打包脚本和
  `windeployqt` 安装阶段依赖收集，可输出包含中文 GUI、CLI、Qt DLL、平台插件和翻译文件的
  Windows ZIP。当前 macOS 主机没有 Windows 编译器/Qt SDK，因此不把 Windows 二进制列为
  本机已验证产物；必须在 Windows 主机或 Windows CI 执行并验收。

## V1.0（2026-09-23）

- 使用 CMake 4.4.3、AppleClang 15 和 Qt 6.11.2 完成 macOS arm64 Release 全功能构建；CLI、
  Qt 桌面端、六个核心测试程序与 GUI smoke 均成功链接。
- CTest 全矩阵 90/90 通过：六个核心单元/组件测试、83 个 CLI 集成回归和一个离屏 GUI
  smoke。独立 `tests/run_cli_regressions.py` 也可在无 CMake 测试驱动时复用同一份测试声明。
- Homebrew LLVM 20.1.8 的 UBSan 错误即停构建通过 89/89 个非 GUI 测试。当前 macOS 26.6.2
  环境中 AppleClang 15 ASan 在运行时初始化阶段崩溃，LLVM 20 ASan 则在初始化后挂起，连
  `--version` 都无法进入；因此 ASan 未列为通过项，属于已记录的主机运行时限制。
- 安装规则已验证可部署 CLI、自带 Qt Framework/平台插件的 macOS `.app`、README、全部文档
  与配置示例；从最终 CPack ZIP 解压后的 `.app` 通过 ad-hoc 深度签名校验并可离屏启动，CLI
  报告 1.0.0。该包尚未做 Developer ID 签名或 Apple notarization。
- 对真实 CLAHE RTL 工程扫描得到 12 个文件、12 个设计单元、11 个实例和 105 条诊断
  （2 warning、103 info），与阶段 99 基线一致；第二个 Zynq/CLAHE RTL 工程扫描得到 1 个
  文件、1 个设计单元和 12 条 info，未出现分析错误。
- 发布规则目录保持 90 条，支持 Verilog、SystemVerilog、VHDL，报告格式为 text、JSON
  schema 3、SARIF 2.1.0 与离线 HTML。正式范围和非目标见 [V1.0 发布说明](v1_release.md)。

## 阶段 99（2026-09-22）

- 扩展 `VL-SIDE-EFFECT-CONDITION` 到过程内运行时三目 guard；普通/复合/移位赋值及前后缀
  `++`/`--` 继续复用现有副作用分类器，纯比较保持排除。
- 三目范围复用统一结构提取器；外层 `if/while` 与内部三目覆盖同一副作用 token 时按源码
  offset 去重。SV 单元测试和 CLI fixture 从 4 条扩展为精确 7 条 warning，覆盖普通赋值、
  自增、嵌套复合赋值和纯比较负例；规则目录保持 90 条。
- AppleClang C++20 严格警告下 CLI 与六个核心测试目标全部编译通过，六个核心测试全部通过；
  最终 0.99.0 SARIF 发布 90 条规则和 7 条 warning result。常量条件保持 8/8、比较位宽保持
  14/10，递归 file-list 保持 2/2/0；CLAHE 保持 12 单元、11 实例和 105 条诊断（2 warning、
  103 info），三目副作用扩展未增加真实项目噪声。

## 阶段 98（2026-09-22）

- 扩展 `RTL-CONSTANT-CONDITION` 到运行时条件表达式：检查 Verilog/SystemVerilog 过程/连续
  赋值的三目 guard，以及 VHDL 过程/并发 signal assignment 的 when-else guard。
- 三目与 when-else 条件范围由比较位宽规则共用的结构提取器提供，避免两套边界逐步分叉；
  parameter/generic、generate/elaboration 条件和运行时依赖表达式保持排除。
- SV/VHDL constant-expression 单元测试及 CLI fixture 均从原有过程条件扩展为精确 8 条 warning，
  覆盖 false/true、嵌套、continuous/concurrent 和运行时负例；规则目录保持 90 条。
- AppleClang C++20 严格警告下 CLI 与六个核心测试目标全部编译通过，六个核心测试全部通过；
  最终 0.98.0 SARIF 发布 90 条规则和 8 条 warning result。比较位宽 fixture 保持 14/10，递归
  file-list 保持 2/2/0；CLAHE 保持 12 单元、11 实例和 105 条诊断（2 warning、103 info），
  常量条件表达式扩展未增加真实项目噪声。

## 阶段 97（2026-09-21）

- 扩展 Verilog/SystemVerilog 三目 guard 的结构边界：支持链式 false/true arm 中的嵌套条件，
  以及 `if (a < b ? en : 1'b0)` 等没有直接赋值起点、但存在明确外层圆括号的上下文。
- 同级前序 `?`/`:` 用于定位当前 guard；括号、选择和拼接内部的分隔符排除，`==?`/`!=?`
  继续按 wildcard equality 处理。SV fixture 新增两组 mismatch/等宽对照后精确产生 14 条
  warning；VHDL fixture 保持 10 条，规则目录保持 90 条。
- AppleClang C++20 严格警告下 CLI 与六个核心测试目标全部编译通过，六个核心测试全部通过；
  最终 0.97.0 SARIF 发布 90 条规则和 14 条 warning result。递归 file-list 保持 2/2/0；CLAHE
  保持 12 单元、11 实例和 105 条诊断（2 warning、103 info），链式三目扩展未增加噪声。

## 阶段 96（2026-09-21）

- 扩展 `RTL-COMPARISON-WIDTH-MISMATCH` 的 Verilog/SystemVerilog 三目条件覆盖：除括号 guard
  外，新增过程阻塞/非阻塞赋值及 continuous assignment 右侧的无括号 guard，例如
  `q = a < b ? x : y`。
- 赋值边界通过既有语义赋值识别器确认；`q <= a <= c ? x : y` 中首个 `<=` 作为非阻塞赋值，
  第二个作为关系比较。等宽关系和 `==?`/`!=?` wildcard equality 不产生额外告警。
- SV fixture 新增 mismatch/等宽无括号条件对照后精确产生 12 条 warning；独立 continuous
  assignment 单元测试精确产生 1 条 mismatch。VHDL fixture 保持 10 条 warning，规则目录
  保持 90 条。
- AppleClang C++20 严格警告下 CLI 与六个核心测试目标全部编译通过，六个核心测试全部通过；
  最终 0.96.0 SARIF 发布 90 条规则和 12 条 warning result。递归 file-list 保持 2/2/0；CLAHE
  保持 12 单元、11 实例和 105 条诊断（2 warning、103 info），无括号三目扩展未增加噪声。

## 阶段 95（2026-09-21）

- 扩展 `RTL-COMPARISON-WIDTH-MISMATCH` 的条件表达式覆盖：分析括号包围的
  Verilog/SystemVerilog 三目运算 guard，以及 VHDL `value when condition else value`
  条件；结果分支不参与比较扫描。
- SV fixture 新增 mismatch/等宽三目条件对照后精确产生 11 条 warning；VHDL fixture 新增
  mismatch/等宽 when-else 条件对照后精确产生 10 条 warning，且关系运算不会误触发
  `RTL-INPUT-DRIVEN`；规则目录保持 90 条。
- 同步修正 VHDL 组合依赖图的赋值识别：关系表达式 `a <= c` 不再被原始 token 模式误当成
  对 `a` 的赋值，消除了由此虚构的 `a`/`q` 组合环；单元测试增加无
  `RTL-COMBINATIONAL-LOOP` 回归断言。
- AppleClang C++20 严格警告下 CLI 与六个核心测试目标全部编译通过，六个核心测试全部通过；
  最终 0.95.0 SARIF 发布 90 条规则和 11 条 warning result。递归 file-list 保持 2/2/0；CLAHE
  保持 12 单元、11 实例和 105 条诊断（2 warning、103 info），条件表达式扩展未增加噪声。

## 阶段 94（2026-09-21）

- 扩展 `RTL-COMPARISON-WIDTH-MISMATCH` 的条件规范化与运算符覆盖：完整包围比较的
  Verilog/SystemVerilog `!(...)`、VHDL `not (...)` 会递归分析内部比较；SystemVerilog
  `==?`/`!=?` wildcard equality 纳入静态位宽检查。
- 只有取反后的括号覆盖完整余下条件时才剥离，`!a == b` 等优先级不同的表达式保持排除。
  SV fixture 新增 negated 与 wildcard mismatch/等宽对照后精确产生 10 条 warning；VHDL
  fixture 新增 not 包围的 mismatch/等宽对照后精确产生 9 条 warning；规则目录保持 90 条。
- AppleClang C++20 严格警告下 CLI 与六个核心测试目标全部编译通过，六个核心测试全部通过；
  最终 0.94.0 SARIF 发布 90 条规则和 10 条 warning result。递归 file-list 保持 2/2/0；CLAHE
  保持 12 单元、11 实例和 105 条诊断（2 warning、103 info），新增条件形态未增加噪声。

## 阶段 93（2026-09-21）

- 扩展 `RTL-COMPARISON-WIDTH-MISMATCH` 的过程范围：新增 VHDL `while ... loop` guard，并解析
  Verilog/SystemVerilog `for(init; condition; update)` 的顶层分号，仅将中间控制条件送入比较
  宽度分析；Verilog `while` 同步加入明确回归覆盖。
- SV/VHDL 单元测试及 CLI fixture 分别新增直接和 selected loop-condition mismatch，初始化、
  更新表达式及等宽控制条件不会误报；两个 fixture 均精确产生 8 条 warning，规则目录保持
  90 条。
- AppleClang C++20 严格警告下 CLI 与六个核心测试目标全部编译通过，六个核心测试全部通过；
  最终 0.93.0 SARIF 发布 90 条规则和 8 条 warning result。递归 file-list 保持 2/2/0；CLAHE
  保持 12 单元、11 实例和 105 条诊断（2 warning、103 info），循环条件扩展未增加额外问题。

## 阶段 92（2026-09-21）

- 扩展 `RTL-COMPARISON-WIDTH-MISMATCH` 的运算符覆盖：新增 Verilog/SystemVerilog 与 VHDL
  `<`、`<=`、`>`、`>=` 关系比较，继续复用信号、常量选择、显式宽 literal 与静态拼接的完整
  操作数宽度模型。
- Verilog 移位符号会被明确识别并排除，复杂移位结果不会被误当成直接操作数。SV/VHDL 单元测试
  和 CLI fixture 均新增直接及 selected 关系 mismatch，并保留等宽和移位/拼接等负例；两个
  fixture 均精确产生 6 条 warning，规则目录保持 90 条。
- 回归中修正 VHDL 驱动提取：条件中的 `a <= c` 关系运算不再被误识别为信号赋值，赋值目标
  现在必须出现在合法语句起点；fixture 恢复为纯 6 条 warning，不再级联输入被驱动 error。
- AppleClang C++20 严格警告下 CLI 与六个核心测试目标全部编译通过，六个核心测试全部通过；
  最终 0.92.0 SARIF 发布 90 条规则和 6 条 warning result。递归 file-list 保持 2/2/0；CLAHE
  增至 105 条诊断（2 warning、103 info），新增项为 9 位 `bin_cnt` 与 8 位
  `excess_remainder` 的真实关系比较位宽风险，定位在 `clahe_clipper_cdf.v:374`。

## 阶段 91（2026-09-21）

- 扩展 `RTL-COMPARISON-WIDTH-MISMATCH` 的操作数形状：除直接信号和常量选择外，新增
  Verilog/SystemVerilog 显式宽 literal、VHDL 字符串/标量逻辑 literal，以及成员宽度均可求值的
  两种 HDL 静态拼接。
- SV/VHDL 单元测试及 CLI fixture 均新增“窄信号与宽 literal”“窄信号与宽拼接”正例，并保留
  等宽 literal、等宽拼接和算术表达式负例；两个 fixture 均精确产生 4 条 warning，规则目录保持
  90 条。
- AppleClang C++20 严格警告下 CLI 与六个核心测试目标全部编译通过，六个核心测试全部通过；
  最终 0.91.0 SARIF 发布 90 条规则和 4 条 warning result。递归 file-list 保持 2/2/0；CLAHE
  保持 12 单元、11 实例和 104 条诊断（1 warning、103 info），扩展操作数未增加噪声。

## 阶段 90（2026-09-21）

- 新增 `RTL-COMPARISON-WIDTH-MISMATCH`（warning）：检查 Verilog/SystemVerilog 过程条件中的
  `==`/`!=`/`===`/`!==` 与 VHDL `=`/`/=`，当两侧直接信号或常量 bit/index/slice 的静态
  位宽不一致时报告隐式扩展风险。
- 逻辑连接会递归拆分为独立子条件；等宽比较不报，算术、拼接、函数调用、literal、动态选择和
  关系比较保守排除。SV/VHDL 单元测试与 CLI fixture 各覆盖直接和 selected mismatch、等宽及
  复杂表达式负例，均精确产生 2 条 warning；规则目录增至 90 条。
- AppleClang C++20 严格警告下 CLI 与六个核心测试目标全部编译通过，六个核心测试全部通过；
  最终 0.90.0 SARIF 发布 90 条规则和 2 条 warning result。递归 file-list 保持 2/2/0；CLAHE
  保持 12 单元、11 实例和 104 条诊断（1 warning、103 info），比较位宽检查未增加噪声。

## 阶段 89（2026-09-21）

- 扩展 `RTL-WIDTH-TRUNCATION` 的 VHDL 拼接成员宽度推导：除向量信号、常量 index/slice 与
  字符串 literal 外，`'0'`、`'1'` 及 `std_ulogic` 其余标准逻辑字符现在均按 1 位参与 `&`
  拼接宽度计算；属性 apostrophe 和非逻辑字符不会误判为 literal。
- 单元测试及 VHDL CLI fixture 新增 3 位 `slice & '0'`、`slice & 'X'` 写入 2 位目标的正例，
  分别覆盖数字与字母字符 token 形态，并保留 4 位等宽负例；VHDL fixture 精确产生 4 条
  warning，SV fixture 保持 3 条 warning，规则目录保持 89 条。
- AppleClang C++20 严格警告下 CLI 与六个核心测试目标全部编译通过，六个核心测试全部通过；
  最终 0.89.0 SARIF 发布 89 条规则和 4 条 warning result。递归 file-list 保持 2/2/0；CLAHE
  保持 12 单元、11 实例和 104 条诊断（1 warning、103 info），VHDL 标量拼接未影响 Verilog 工程。

## 阶段 88（2026-09-21）

- 扩展 `RTL-WIDTH-TRUNCATION` 的 Verilog/SystemVerilog 拼接宽度推导：常量复制拼接
  `{N{expr}}` 现在会将静态可求的成员宽度乘以复制次数，并继续支持复制体内的简单或嵌套拼接。
- 等宽复制不报；零次、动态/参数化复制次数、动态选择成员及乘法溢出均保守排除。单元测试与
  CLI fixture 新增 6 位复制结果写入 4 位目标的正例，同时保留等宽复制负例；SV fixture 精确
  产生 3 条 warning，VHDL fixture 保持 2 条 warning，规则目录保持 89 条。
- AppleClang C++20 严格警告下 CLI 与六个核心测试目标全部编译通过，六个核心测试全部通过；
  最终 0.88.0 SARIF 发布 89 条规则和 3 条 warning result。递归 file-list 保持 2/2/0；CLAHE
  保持 12 单元、11 实例和 104 条诊断（1 warning、103 info），复制拼接分析未增加噪声。

## 阶段 87（2026-09-21）

- 扩展 `RTL-WIDTH-TRUNCATION` 的表达式宽度推导：Verilog/SystemVerilog 简单及嵌套 `{...}`
  拼接与 VHDL `&` 拼接现在会累加静态可求宽成员，并与直接或 selected target 的真实宽度比较；
  诊断保留完整拼接表达式。
- 等宽拼接不报；Verilog 复制拼接、动态选择成员、算术/调用等复杂表达式保守排除。SV/VHDL
  单元测试和 CLI fixture 均覆盖宽拼接到窄目标、等宽与边界负例，两个 fixture 均精确产生
  2 条 warning；规则目录保持 89 条。
- AppleClang C++20 严格警告下 CLI 与六个核心测试目标全部编译通过，六个核心测试全部通过；
  最终 0.87.0 SARIF 发布 89 条规则和 2 条 warning result。递归 file-list 保持 2/2/0；CLAHE
  保持 12 单元、11 实例和 104 条诊断（1 warning、103 info），静态拼接分析未增加噪声。

## 阶段 86（2026-09-21）

- 扩展 `RTL-WIDTH-TRUNCATION` 的来源宽度推导：Verilog/SystemVerilog 常量 bit/slice 与
  VHDL 常量 index/slice RHS 现在使用选择后的真实宽度，并可与直接或 selected target 组合；
  诊断同时保留 `data[7:4]`、`data(7 downto 4)` 等完整来源表示。
- 等宽选择不报，动态索引/part-select、参数化或复杂 source 保守排除。SV/VHDL 单元测试和
  CLI fixture 均覆盖“宽 slice→窄直接目标”“宽 slice→窄 selected target”以及等宽/动态负例，
  两个 fixture 均精确产生 2 条 warning；规则目录保持 89 条。
- AppleClang C++20 严格警告下 CLI 与六个核心测试目标全部编译通过，六个核心测试全部通过；
  最终 0.86.0 SARIF 发布 89 条规则和 2 条 warning result。递归 file-list 保持 2/2/0；CLAHE
  保持 12 单元、11 实例和 104 条诊断（1 warning、103 info），selected source 分析未增加噪声。

## 阶段 85（2026-09-21）

- 扩展 `RTL-WIDTH-TRUNCATION` 的目标宽度推导：除直接信号外，新增 Verilog/SystemVerilog
  常量 bit/slice 与 VHDL 常量 index/slice 左值；诊断保留 `q[3:0]`、`q(3 downto 0)` 等完整
  selected target，并以选择后的真实宽度比较直接信号或显式宽 literal。
- 动态选择、memory、多维/复杂目标继续保守排除。同步修正 VHDL driver 提取，使 selected
  signal assignment 能驱动其所属基信号，避免新增位宽诊断同时级联错误的未驱动 output 告警。
  SV/VHDL CLI fixture 均精确产生 2 条 warning，规则目录保持 89 条。
- AppleClang C++20 严格警告下 CLI 与六个核心测试目标全部编译通过，六个核心测试全部通过；
  最终 0.85.0 SARIF 发布 89 条规则和 2 条 warning result。递归 file-list 保持 2/2/0；CLAHE
  保持 12 单元、11 实例和 104 条诊断（1 warning、103 info），selected target 分析未增加噪声。

## 阶段 84（2026-09-21）

- 扩展 `RTL-WIDTH-TRUNCATION`：除直接宽信号赋给窄目标外，新增 Verilog/SystemVerilog
  显式 sized integer literal 与 VHDL 字符串 literal 的直接赋值宽度检查；支持过程、连续/并发
  赋值和完整冗余括号，诊断公开 literal 宽度、目标宽度与目标信号。
- 仅在 RHS 剥离完整外层括号后恰好为单一 literal 且目标具有常量宽度时运行；等宽 literal、
  Verilog `'0` 等无尺寸填充值、一元符号表达式、拼接、切片和复杂表达式保守排除。SV/VHDL
  CLI fixture 分别精确产生 3 条和 2 条 warning；规则目录保持 89 条。
- AppleClang C++20 严格警告下 CLI 与六个核心测试目标全部编译通过，六个核心测试全部通过；
  最终 0.84.0 SARIF 发布 89 条规则和 3 条 warning result。递归 file-list 保持 2/2/0；CLAHE
  保持 12 单元、11 实例和 104 条诊断（1 warning、103 info），literal 检查未增加真实工程噪声。

## 阶段 83（2026-09-21）

- 新增 `VL-CASE-ITEM-VALUE-OUT-OF-RANGE`（warning）：对固定宽度的 Verilog/SystemVerilog
  直接信号、常量位选或切片 case selector，检查单 token 无尺寸非负整数 choice 是否大于等于
  `2^selector_width`；这类值不对应 selector 的任何 bit pattern，分支永远无法匹配。
- 负数、参数、显式 sized literal、X/Z 值、复杂表达式及 31 bit 以上 selector 保守排除，避免
  在尚未保存 signedness 时猜测符号扩展。单元测试覆盖直接/bit/slice selector、逗号多 choice
  与全部负例；CLI fixture 精确产生 4 条 warning，没有混入既有 case 位宽诊断。
- AppleClang C++20 严格警告下 CLI 与六个核心测试目标全部编译通过，六个核心测试全部通过；
  最终 0.83.0 SARIF 发布 89 条规则和 4 条 warning result。递归 file-list 保持 2/2/0；CLAHE
  保持 12 单元、11 实例和 104 条诊断（1 warning、103 info），值域检查未给真实工程增加噪声。

## 阶段 82（2026-09-21）

- 新增 `RTL-NEGATIVE-SHIFT-AMOUNT`（warning），覆盖 Verilog/SystemVerilog 的
  `<<`/`>>`/`<<<`/`>>>` 与复合赋值、VHDL 的 `sll/srl/sla/sra`，以及直接
  `shift_left/shift_right` 调用；负计数可能反转方向、成为超大无符号计数或违反参数约束。
- 仅对剥离完整外层括号后恰好为负非零整数 literal 的移位量报告；`-0`、正数、动态值及
  `(0 - 1)`、`(-1 + 2)` 等表达式保守排除。修正三字符复合移位内部 token 重叠，确保
  `>>>= -3` 只产生一条诊断。SV/VHDL CLI fixture 分别精确产生 4 条和 3 条 warning。
- AppleClang C++20 严格警告下 CLI 与六个核心测试目标全部编译通过，六个核心测试全部通过；
  最终 0.82.0 SARIF 发布 88 条规则和 4 条 warning result。递归 file-list 保持 2/2/0；CLAHE
  保持 12 单元、11 实例和 104 条诊断（1 warning、103 info），新检查未给真实工程增加噪声。

## 阶段 81（2026-09-21）

- 新增 `VL-ALWAYS-NO-SUSPENSION`（error）：普通 Verilog/SystemVerilog `always` 若完整过程
  范围内没有直接 `@` 事件控制、`#` 延时控制或 `wait` 语句，会在零仿真时间无限重复并可能
  卡死仿真，因此定位到 `always` 关键字报告。
- 单元测试精确命中块语句与单语句两种缺失挂起点写法，并验证含事件、延时、`wait` 的普通
  `always`，以及 `always_comb`、`always_ff`、`initial` 均排除。CLI fixture 精确产生 2 条
  error；跨 task 调用的内部 timing control 尚不解析，已在规则边界中明确说明。
- AppleClang C++20 严格警告下 CLI 与六个核心测试目标全部编译通过，六个核心测试全部通过；
  最终 0.81.0 SARIF 发布 87 条规则和 2 条 error result。递归 file-list 保持 2/2/0；CLAHE
  保持 12 单元、11 实例和 104 条诊断（1 warning、103 info），新检查未给真实工程增加噪声。

## 阶段 80（2026-09-21）

- 新增 `RTL-DIVIDE-BY-ZERO`（error），检查 Verilog/SystemVerilog 的 `/`、`%`、`/=`、`%=`
  以及 VHDL 的 `/`、`mod`、`rem` 是否使用整数 literal 零分母；支持完整包围分母的多层括号
  和 Verilog based literal，并明确排除 VHDL `/=` 不等比较。
- 当前规则只报告可直接证明的 literal 零；参数、常量标识符、复合表达式、函数结果和带一元
  符号的零保持保守边界，等待完整 AST 与常量传播阶段扩展。SystemVerilog/VHDL CLI fixture
  分别精确产生 4 条和 3 条 error，单元测试同时覆盖合法非零分母及 `/=` 负例。
- AppleClang C++20 严格警告下 CLI 与六个核心测试目标全部编译通过，六个核心测试全部通过；
  最终 0.80.0 SARIF 发布 86 条规则和 4 条 error result。递归 file-list 保持 2/2/0；CLAHE
  保持 12 单元、11 实例和 104 条诊断（1 warning、103 info），零分母检查未给真实工程增加噪声。

## 阶段 79（2026-09-21）

- 新增 `RTL-SHIFT-AMOUNT-OUT-OF-RANGE`（warning），当直接、常量索引或常量切片操作数被
  非负整数 literal 移位，且移位量大于等于操作数宽度时报告，因为所有原始位都会离开结果范围。
- Verilog/SystemVerilog 覆盖 `<<`、`>>`、`<<<`、`>>>` 及其复合赋值形式；VHDL 覆盖
  `sll/srl/sla/sra` 和直接 `shift_left/shift_right(operand, literal)`。动态/参数移位、旋转、
  memory 和复杂操作数保守排除。
- 单元测试覆盖逻辑/算术、直接/切片/单 bit、复合赋值、VHDL infix 与函数形式，并验证小于
  位宽的负例；SV/VHDL CLI fixture 分别精确产生 4 条和 5 条 warning。规则目录增至 85 条。
- AppleClang C++20 严格警告下六个核心测试全部通过；最终 0.79.0 SARIF 发布 85 条规则和
  4 条对应 result。递归 file-list 保持 2/2/0；CLAHE 保持 12 单元、11 实例和 104 条诊断
  （1 warning、103 info），移位检查未给真实工程增加噪声。

## 阶段 78（2026-09-21）

- 扩展 `RTL-CASE-ITEM-WIDTH-MISMATCH` 的 selector 宽度推导：除直接固定宽度信号外，新增
  Verilog/SystemVerilog 常量位选与 `[left:right]` 切片，以及 VHDL 常量索引与
  `(left downto/to right)` slice；诊断显示完整 selector 表示和推导宽度。
- 动态索引、memory、拼接、函数调用和其他复杂表达式继续保守排除；常量 bit/index 统一推导为
  1 bit，常量 slice 使用端点绝对差加一，并继续复用越界选择规则独立检查端点合法性。
- 单元测试和两个 CLI fixture 均覆盖直接、slice、bit/index selector 的宽度不匹配及同宽负例；
  两个 fixture 分别精确产生 4 条 warning。本阶段规则目录保持 84 条。
- AppleClang C++20 严格警告下六个核心测试全部通过；最终 0.78.0 SARIF 发布 84 条规则和
  4 条对应 result。递归 file-list 保持 2/2/0；CLAHE 保持 12 单元、11 实例和 104 条诊断
  （1 warning、103 info），扩展 selector 覆盖未改变真实工程基线。

## 阶段 77（2026-09-21）

- 新增 `RTL-CASE-ITEM-WIDTH-MISMATCH`（warning）：当 case selector 是一个已声明、固定宽度、
  非 memory 的直接信号时，检查 Verilog/SystemVerilog 显式 sized integer choice 与 VHDL
  字符串 choice 的位宽，报告扩展、截断或无法按预期匹配的风险。
- 诊断同时公开 selector 名称、声明宽度与 choice 宽度；unsized Verilog 常量、参数/表达式
  choice，以及位选、切片、拼接、调用等复杂 selector 保守排除，避免猜测表达式宽度。
- 单元测试覆盖两个 HDL 的短/长 choice、同宽负例，并覆盖 Verilog unsized literal 与复杂
  selector 边界；两个 CLI fixture 均精确产生 2 条 warning。规则目录增至 84 条。
- AppleClang C++20 严格警告下六个核心测试全部通过；最终 0.77.0 SARIF 发布 84 条规则和
  2 条对应 result。递归 file-list 保持 2/2/0；CLAHE 保持 12 单元、11 实例和 104 条诊断
  （1 warning、103 info），case 位宽检查未给真实工程增加噪声。

## 阶段 76（2026-09-21）

- 新增 `RTL-MULTIPLE-CASE-DEFAULT`（error），检查同一 Verilog/SystemVerilog case 中第二个及
  后续 `default`，以及同一 VHDL case 中第二个及后续 `when others`；诊断定位后出现的
  catch-all，明确表达不可达且非法的结构。
- 每个嵌套 case 独立维护 catch-all 状态；普通重复 choice 仍由 `RTL-DUPLICATE-CASE-ITEM`
  表达，Verilog 缺少 default 仍由 `VL-CASE-DEFAULT` 表达，避免规则职责混淆。
- 单元测试覆盖两个 HDL 的重复 catch-all、精确行号和 Verilog 嵌套作用域负例；两个 CLI
  fixture 均精确产生 1 条 error，规则目录增至 83 条。
- AppleClang C++20 严格警告下六个核心测试全部通过；最终 0.76.0 SARIF 发布 83 条规则和
  1 条 error result。递归 file-list 保持 2/2/0；CLAHE 保持 12 单元、11 实例和 104 条诊断
  （1 warning、103 info），新增 case 合法性检查未改变真实工程基线。

## 阶段 75（2026-09-21）

- 新增 `VL-EMPTY-CASE-ITEM`（warning），检查 Verilog/SystemVerilog `case/casez/casex`
  choice 与 default 对应的空语句、空 `begin/end`、空命名块和只含分号的块；嵌套 case
  分别建模并定位实际空语句或块起点。
- 修复通用 Verilog 流程解析器跳过语句起始分号的错误：`label:;` 现在作为该 case 项自己的
  完整空语句消费，不会再吸收下一项。这也恢复了后续 default 识别和 latch 分支集合的准确性。
- 单元测试覆盖普通 choice、default、命名/未命名空块、单行 case、嵌套 case 作用域与 populated
  负例；独立 CLI fixture 精确产生 5 条 warning，没有级联 `VL-CASE-DEFAULT`。规则目录增至 82 条。
- AppleClang C++20 严格警告下六个核心测试全部通过；最终 0.75.0 SARIF 发布 82 条规则和
  5 条对应 result。递归 file-list 保持 2/2/0；CLAHE 保持 12 单元、11 实例和 104 条诊断
  （1 warning、103 info），底层流程修复未改变真实工程基线。

## 阶段 74（2026-09-21）

- 新增 `VL-EMPTY-CONDITIONAL-BRANCH`（warning），检查 Verilog/SystemVerilog 运行时
  `if/else if/else` 中的空语句、空 `begin/end`、空命名块和只含分号的块，重点定位
  `if (condition);` 这类多余分号造成的静默控制流缺陷。
- 分支边界使用平衡条件括号和单语句/块结束位置；最终 else 与嵌套 if 独立检查，合法的
  populated branch 和 else-if 连接不报告，每个实际空分支保留自己的精确位置。
- 单元测试覆盖空 then、空 else、命名/未命名空块、嵌套空分支及 populated/else-if 负例；
  独立 CLI fixture 精确产生 5 条 warning，没有其他诊断。规则目录增至 81 条。
- AppleClang C++20 严格警告下六个核心测试全部通过；最终 0.74.0 SARIF 发布 81 条规则和
  5 条对应 result。递归 file-list 保持 2/2/0；CLAHE 保持 12 单元、11 实例和 104 条诊断
  （1 warning、103 info），新分支检查未改变真实工程基线。

## 阶段 73（2026-09-21）

- 新增 `VL-SIDE-EFFECT-CONDITION`（warning），检查 Verilog/SystemVerilog 运行时 `if/while`
  条件中的普通赋值、算术/位复合赋值、移位赋值及前后缀自增/自减，避免 `=` 误写为 `==`
  或条件求值意外修改状态。
- token 级操作符判定排除 `==`、`===`、`!=`、`!==`、`<=`、`>=` 等比较；每个条件最多报告
  一次，函数调用的潜在过程间副作用和 generate/elaboration 表达式按保守边界排除。
- 单元测试覆盖 `=`、`+=`、`<<=`、`++` 四种正例以及 equality、inequality、关系比较负例；
  独立 CLI fixture 精确产生 4 条 warning，没有其他诊断。规则目录增至 80 条。
- AppleClang C++20 严格警告下六个核心测试全部通过；最终 0.73.0 SARIF 发布 80 条规则和
  4 条对应 result。递归 file-list 保持 2/2/0；CLAHE 保持 12 单元、11 实例和 104 条诊断
  （1 warning、103 info），新规则未改变真实回归工程结果。

## 阶段 72（2026-09-20）

- 扩展 `RTL-CONSTANT-CONDITION` 的保守求值能力：Verilog/SystemVerilog 支持冗余括号、逻辑
  取反、`&&`/`||` 短路恒等式和整数文字比较（含 case equality）；VHDL 支持大小写不敏感的
  `not`、`and`、`or`、布尔/十进制文字比较。只有能证明结果的表达式才报告。
- 修复 Verilog 扫描器把紧跟块结尾 `end` 的下一条 `if` 错当成 VHDL `end if` 后继而跳过的
  漏检。generate/elaboration 条件仍不进入过程规则，包含未知运行时值且无法由布尔恒等式
  化简的条件继续保守排除。
- 单元测试覆盖两个 HDL 的取反、比较、短路恒真/恒假、未知运行时负例、块后首条 if 和
  generate 负例；CLI fixture 分别精确产生 5 条与 6 条 `RTL-CONSTANT-CONDITION` warning。
- AppleClang C++20 严格警告下六个核心测试全部通过；最终 0.72.0 SARIF 继续发布 79 条规则，
  SV fixture 产生 5 条对应 result。递归 file-list 保持 2/2/0；CLAHE 保持 12 单元、11 实例和
  104 条诊断（1 warning、103 info），证明扩展未给真实回归工程增加噪声。

## 阶段 71（2026-09-20）

- 新增 `RTL-DUPLICATE-CONDITION`（warning），检查同一 Verilog/SystemVerilog `if/else if`
  或 VHDL `if/elsif` 链中完全重复、导致后续分支不可达的条件。
- 条件键移除完整外层冗余括号；嵌套链和相邻独立链各自隔离，VHDL 按大小写不敏感 token
  比较，Verilog/SystemVerilog 保持大小写敏感，不猜测代数或参数求值后的逻辑等价。
- 单元测试覆盖两个 HDL 的外层重复、嵌套重复、独立链负例、括号规范化和 VHDL 大小写；两个
  CLI fixture 均精确产生 1 条 warning，规则目录增至 79 条。
- AppleClang C++20 严格警告下六个核心测试全部通过；最终 0.71.0 SARIF 发布 79 条规则和
  1 条对应 warning result。递归 file-list 保持 2/2/0，CLAHE 保持 12 单元、11 实例及 104 条
  诊断（1 warning、103 info），新增控制流检查未改变既有工程结果。

## 阶段 70（2026-09-20）

- 新增 `SV-ALWAYS-FF-NONEDGE-EVENT`（error）：当 `always_ff` 恰好有一个 event control 时，
  继续验证其中至少存在一个 `posedge` 或 `negedge`，拒绝 `@(clk)` 与 `@(*)` 电平变化模板。
- 与 `SV-ALWAYS-FF-EVENT-CONTROL` 分工：0 个或多个 `@` 仍只报告数量错误，恰好一个但无边沿
  才报告本规则，避免同一过程产生级联根因。
- 单元测试覆盖 level-change、wildcard、合法单边沿和带异步控制的多边沿列表；CLI fixture 精确
  产生 1 条 error，并保留 1 个合法时序过程。本阶段规则目录增至 78 条。
- AppleClang C++20 严格警告下六个核心测试全部通过；最终 0.70.0 SARIF 发布 78 条规则和
  1 条对应 error result。递归 file-list 保持 2/2/0，传统 VHDL fixture 保持 2 时钟、2 过程和
  1 条 CDC warning；CLAHE 保持 104 条诊断（1 warning、103 info）。

## 阶段 69（2026-09-20）

- VHDL timing 提取新增经典 `clock'event and clock = '1'/'0'` 模板，电平比较可以位于 `event`
  前后，信号名按 VHDL 大小写不敏感语义匹配，并分别建模为上升沿或下降沿。
- 传统模板现会进入 clocks、sequentialProcesses、CDC/RDC、复位一致性和报告链路，而不再仅由
  语义层粗略视为时序过程；edge function 行为保持不变。
- 单元测试使用一升一降两个传统时钟域验证 2 时钟、2 时序过程及 1 条直接 CDC warning；CLI
  fixture 精确发布同样的结构与诊断。本阶段不增加规则数量。
- AppleClang C++20 严格警告下六个核心测试全部通过；最终 0.69.0 CLI 继续发布 77 条规则，
  SARIF 发布 77 条规则和 1 条 CDC result。递归 file-list 保持 2/2/0，跨文件 VHDL 安全工程
  保持 2 时钟、2 过程、1 条认证同步链和 0 诊断；CLAHE 保持 104 条诊断（1 warning、103 info）。

## 阶段 68（2026-09-20）

- 新增 `RTL-COMPLEX-EDGE-EVENT`（warning），检查 Verilog/SystemVerilog 的
  `posedge`/`negedge` 与 VHDL 的 `rising_edge`/`falling_edge` 是否直接使用标量标识符。
- 组合表达式、位选和索引边沿源会报告；直接标识符及任意层无害括号保持接受。时序提取器同步
  解包合法括号操作数，并排除复杂操作数，避免把表达式首个标识符伪装成可审计时钟域。
- 单元测试覆盖两个 HDL 的组合边沿、选择边沿、直接/括号标量负例，以及括号异步复位继续进入
  复位首分支分析的模型一致性；两个 CLI fixture 均精确产生 1 条 warning。
- AppleClang C++20 严格警告下六个核心测试全部通过；最终 0.68.0 CLI 发布 77 条规则。SARIF
  发布 77 条规则和 1 条对应 warning result；递归 file-list 保持 2/2/0，CLAHE 保持 12 单元、
  11 实例与 104 条诊断（1 warning、103 info），新增检查未改变既有工程结果。

## 阶段 67（2026-09-20）

- 新增 `RTL-ASYNC-RESET-UNGUARDED-LOGIC`（error），检查首个纯异步复位条件链结束后是否仍有
  独立过程赋值；这些赋值会在复位事件触发进程时继续执行，并可能覆盖或绕过复位值。
- Verilog/SystemVerilog 使用完整 statement 边界定位外层 `if/else`，VHDL 跟踪嵌套
  `if/end if` 深度；只有可证明的后续赋值才报告，assertion、调用和 `null` 保守跳过。
- 新规则只在首复位条件有效且 Verilog 事件边沿/极性一致后运行，不与 Stage 62/66 的极性或
  首分支根因级联；单元测试覆盖两个 HDL 的错误模板、完整守卫模板和精确来源上下文。
- AppleClang C++20 严格警告下六个核心测试全部通过；最终 0.67.0 CLI 发布 76 条规则。两个
  CLI fixture 均精确产生 1 条 error，SARIF 发布 76 条规则和 1 条对应 result；递归 file-list
  保持 2/2/0，CLAHE 保持 12 单元、11 实例与 104 条诊断（1 warning、103 info）。

## 阶段 66（2026-09-20）

- 新增 `RTL-ASYNC-RESET-NOT-FIRST`（error），检查 Verilog/SystemVerilog 与 VHDL 时序过程的
  异步 reset/clear/preset 是否由首个独立 `if` 条件优先处理，发现 enable 或功能条件屏蔽复位。
- Verilog/SystemVerilog 复用现有极性表达式解析，复杂复位条件改由本规则报告而不误归类为
  极性错误，并跳过块开头的内建类型局部声明；VHDL 支持裸布尔、`not` 及与 bit/boolean
  常量的等于和不等于模板。
- 单元测试覆盖非首分支、复合条件、正确首分支、两个 HDL 的精确来源上下文；两个 CLI fixture
  均精确产生 1 条 error，且合法模板保持零命中。
- AppleClang C++20 严格警告下六个核心测试全部通过；最终 0.66.0 CLI 发布 75 条规则。SARIF
  精确发布 75 条规则和 1 条 error result；递归 file-list 保持 2/2/0，CLAHE 保持 12 单元、
  11 实例与 104 条诊断（1 warning、103 info），新增规则未对现有复位模板产生噪声。

## 阶段 65（2026-09-20）

- 将时序结构规则统一为项目级执行：`analyzeSources` 不再逐文件提前产生 timing finding，而是在
  合并设计单元后重新关联 VHDL architecture 摘要并一次性运行 timing/CDC/RDC 检查。
- 抽取统一 timing merge，按唯一 module/entity 名称关联，并对时钟、复位、标量声明和时序过程
  去重；公共 `analyzeSource` 仍保留原有单文件行为。
- 单元测试覆盖 entity/architecture 分文件的 2 时钟、2 过程、认证同步链及直接 CDC 反例；
  两个目录 CLI fixture 分别精确产生 1 条认证链/0 诊断和 1 条 CDC warning。
- AppleClang C++20 严格警告下六个核心测试全部通过；最终 0.65.0 CLI 继续发布 74 条规则，
  本阶段补齐跨文件项目语义，不增加规则数量。
- 安全 fixture 的 JSON 精确发布 2 文件、1 单元、2 时钟、2 时序过程、1 条认证同步链及 0 诊断；
  不安全 fixture 发布 0 条认证链及 1 条 `CDC-DIRECT-CROSSING` warning。SARIF 发布 74 条规则、
  0 results 和 1 条认证计数；递归 file-list 保持 2/2/0，CLAHE 保持 12 单元、11 实例与 104 条
  诊断（1 warning、103 info）。

## 阶段 64（2026-09-20）

- 将 Stage 63 识别的两级标量 CDC 同步器提升为一等设计结构，记录异步源、第一级、第二级、
  目的时钟/边沿与可跳转来源位置，不再把自动排除结果静默隐藏。
- text 和 SARIF run properties 发布认证总数；JSON summary 与 unit 结构发布总数及完整链；HTML
  dashboard 增加数量卡并在层次表展示链路；Qt 设计树增加可跳转同步器节点。
- 单元测试验证结构字段、JSON 链、text/SARIF 计数和 HTML 转义后链路；安全/旁路 CLI fixture
  继续分别精确产生 0 条及 1 条 CDC warning。
- AppleClang C++20 严格警告下六个核心测试全部通过；最终 0.64.0 CLI 继续发布 74 条规则，
  本阶段增强结果可审计性，不增加规则数量。
- 安全 fixture 的 JSON 精确发布 1 条认证链且 0 诊断，旁路 fixture 发布 0 条认证链及 1 条 CDC
  warning；SARIF 发布 74 条规则、0 results 和 1 条认证计数。递归 file-list 保持 2/2/0，CLAHE
  保持 12 单元、11 实例与 104 条诊断（1 warning、103 info）。

## 阶段 63（2026-09-20）

- 加固 `CDC-DIRECT-CROSSING`：时序过程模型新增直接寄存器传递与复杂读取集合，可证明的两级
  标量目的域同步链不再产生 warning，降低真实 FPGA 工程中的同步器噪声。
- 只有异步源仅直接写入内部第一级、第一级仅由目的域写入且不参与复杂表达式、并直接进入
  同域第二级时才抑制；存在组合旁路、接口第一级或不完整链时继续报告。
- 单元测试覆盖 Verilog/SystemVerilog、VHDL 正确两级链、带组合旁路的反例和不得认证的整条
  多位总线流水线；CLI fixture 分别精确产生 0 条及 1 条 `CDC-DIRECT-CROSSING` warning。
- AppleClang C++20 严格警告下六个核心测试全部通过；最终 0.63.0 CLI 继续发布 74 条规则，
  本阶段专注 CDC 精度，不增加规则数量。
- SARIF 目录保持 74 条规则，递归 file-list 保持 2 文件、2 单元、0 诊断；12 文件 CLAHE 工程
  保持 12 单元、11 实例和 104 条诊断（1 warning、103 info）。

## 阶段 62（2026-09-20）

- 新增 `RTL-ASYNC-RESET-POLARITY-MISMATCH`（error），检查 Verilog/SystemVerilog 时序过程的
  异步 reset/clear/preset 事件边沿是否与首个复位分支测试的有效电平一致。
- 支持 `if (reset)`、`if (!reset_n)`、额外括号、按位取反，以及与 Verilog 0/1 常量进行
  `==`、`===`、`!=`、`!==` 比较；复杂布尔表达式和非首分支模板保守跳过。
- 单元测试覆盖 active-low/active-high 错误模板、正确取反模板、case equality、clear 命名及
  精确来源上下文；CLI fixture 精确产生 1 条 error。
- AppleClang C++20 严格警告下六个核心测试全部通过；最终 0.62.0 CLI 发布 74 条规则，SARIF
  目录为 74 条，递归 file-list 回归保持 2 文件、2 单元、0 诊断。
- 12 文件 CLAHE 工程保持 12 单元、11 实例和 104 条诊断（1 warning、103 info），新增规则
  未对其现有复位模板产生噪声。

## 阶段 61（2026-09-20）

- 将 VHDL `RTL-UNUSED-PARAMETER` 从单文件语义扫描提升为项目级扫描，按大小写不敏感名称将
  entity 与全部已分析 architecture 文件关联；generic 在分离 architecture 中的使用不再误报。
- 只有 entity 而未提供 architecture、或存在重复 entity 定义时保守跳过 generic 诊断，避免
  不完整工程输入产生假阳性；Verilog/SystemVerilog parameter 检查保持单元级行为。
- 新增 entity/architecture 分文件单元回归与目录 CLI fixture，精确验证 2 文件、1 单元、1 条
  `RTL-UNUSED-PARAMETER` info，并验证 entity-only 输入为 0 条该规则诊断。
- AppleClang C++20 严格警告下六个核心测试全部通过；最终 0.61.0 CLI 继续发布 73 条规则，
  本阶段不增加规则数量，只加固跨文件语义完整性。
- 12 文件 CLAHE 工程保持 12 单元、11 实例和 104 条诊断（1 warning、103 info），证明本次
  VHDL 项目级改造未改变既有 Verilog 项目结果；递归 file-list 仍为 2 文件、2 单元、0 诊断，
  SARIF 目录仍完整发布 73 条规则。

## 阶段 60（2026-09-20）

- 新增 `RTL-UNUSED-PARAMETER`（info），检查 Verilog/SystemVerilog value/type parameter、
  localparam 与 VHDL generic 是否在所属接口或实现中真正被引用。
- Verilog 声明 token 和 `.FORMAL(...)` 的 named override formal 不计作使用；VHDL 使用 entity
  加其全部 architecture 的显式范围，按大小写不敏感匹配，并排除 `FORMAL => actual` 的 formal，
  防止其他 entity 或实例映射名称掩盖未使用 generic。
- 单元测试覆盖 value/type/local parameter、端口宽度、类型位置、表达式使用、named formal、
  多 entity 作用域与 VHDL generic map；两种语言的 CLI fixture 均精确产生 1 条 info。
- AppleClang C++20 严格警告下六个核心测试全部通过；最终 0.60.0 CLI 发布 73 条规则，SARIF
  目录为 73 条，递归 file-list 回归仍为 2 文件、2 单元、0 诊断。
- CLAHE 新识别 12 项未引用 parameter/localparam，总计 104 条诊断（1 warning、103 info）；旧
  85 项兼容基线继续全部命中，并保留 19 条活动 info，其中 12 条来自本阶段规则。

## 阶段 59（2026-09-20）

- 新增 `RTL-EMPTY-PROCESS`（info），检查只含结构 token 的 Verilog/SystemVerilog
  `always`/`initial` 空块，以及主体只含显式 `null;` 的 VHDL process。
- Verilog 命名空块会跳过 `begin : label` 标签；VHDL 查找主 process `begin` 时跟踪局部
  procedure/function 主体，避免把声明区子程序的实现误当作外层过程行为。
- 单元测试覆盖命名 always、空 initial、有效赋值过程、VHDL 简单 null process、带局部 function
  的 null process 和有效 VHDL process；两种语言的 CLI fixture 均精确产生 1 条 info。
- AppleClang C++20 严格警告下六个核心测试全部通过；最终 0.59.0 CLI 发布 72 条规则，SARIF
  目录为 72 条，递归 file-list 回归仍为 2 文件、2 单元、0 诊断。
- CLAHE 保持 12 文件、12 单元、11 实例和 92 条诊断（1 warning、91 info），兼容基线重放后
  仍为 7 条新增、85 条命中。

## 阶段 58（2026-09-20）

- 新增 `RTL-SELF-ASSIGNMENT`（info），检查 Verilog/SystemVerilog 与 VHDL 过程内标量直接
  自赋值；Verilog 阻塞/非阻塞形式均支持，VHDL 信号名按大小写不敏感比较。
- 规则复用现有过程边界与赋值运算符识别，只在左右侧均为同一已声明直接信号且 RHS 到分号间
  无其他 token 时报告；连续赋值、切片/索引、延时及包含运算的状态更新保守排除。
- 单元测试覆盖两种 Verilog 赋值方式、VHDL 大小写、切片/索引和复合表达式负例；两种语言的
  CLI fixture 均精确产生 1 条 info。AppleClang C++20 严格警告下六个核心测试全部通过。
- 最终 0.58.0 CLI 发布 71 条规则，SARIF 目录为 71 条；递归 file-list 回归仍为 2 文件、2 单元、
  0 诊断。CLAHE 保持 12 文件、12 单元、11 实例和 92 条诊断（1 warning、91 info），兼容基线
  重放后仍为 7 条新增、85 条命中。

## 阶段 57（2026-09-20）

- 新增 `RTL-DUPLICATE-SENSITIVITY`（info），检查 Verilog/SystemVerilog 显式 event list 与
  VHDL process sensitivity list 中重复的简单信号项，并将诊断定位到后续重复 token。
- Verilog 对 `posedge`/`negedge` 分别建键，避免把同一时钟的相反边沿当成完全重复事件；只接受
  顶层单信号或 edge + signal 项，复合 event expression 保守跳过。VHDL 名称按大小写不敏感匹配，
  复杂 selected/indexed name 同样保守跳过。
- 单元测试覆盖组合列表重复、相同边沿重复、相反边沿、复合表达式和 VHDL 大小写语义；两种
  语言的 CLI fixture 均精确产生 1 条 info。AppleClang C++20 严格警告下六个核心测试通过。
- 最终 0.57.0 CLI 发布 70 条规则，SARIF 目录为 70 条；递归 file-list 回归仍为 2 文件、2 单元、
  0 诊断。CLAHE 保持 12 文件、12 单元、11 实例和 92 条诊断（1 warning、91 info），兼容基线
  重放后仍为 7 条新增、85 条命中。

## 阶段 56（2026-09-20）

- 新增 `RTL-CONSTANT-CONDITION`（warning）：在 Verilog/SystemVerilog 过程范围内检查字面量
  `if/while`，在 VHDL process 范围内检查字面量布尔 `if/elsif/while`；generate/elaboration
  条件不进入运行时规则。
- 单元测试覆盖 Verilog `if (1'b0)`、`while (1)`、普通信号条件、generate 排除，以及 VHDL
  `if false`、`elsif false`、带括号的 `while (true)` 和非字面量时钟条件；两种语言的 CLI
  fixture 分别精确产生 1 条和 2 条 warning。
- AppleClang C++20 严格警告下六个核心测试全部重新编译并通过；最终 0.56.0 CLI 发布 69 条
  规则，SARIF 目录为 69 条，递归 file-list 回归仍为 2 文件、2 单元、0 诊断。
- CLAHE 保持 12 文件、12 单元、11 实例和 92 条诊断（1 warning、91 info），证明新增规则未
  扩大该工程的现有噪声；用同一 85 项兼容基线重放后仍为 7 条新增、85 条命中。

## 阶段 55（2026-09-16）

- 将 `VHDL-NONSTD-NUMERIC` 与 `VHDL-SHARED-VARIABLE` 从逐行正则迁移到统一 tokenizer；非标准
  package 仅在 `use ...;` clause 内匹配，shared variable 由相邻关键字 token 识别。
- 单元测试覆盖跨行 package 名、跨行 `shared variable`、architecture/entity 上下文，以及字符串和
  注释中的伪命中排除；CLI fixture 精确产生 2 条 warning。
- 删除核心 Analyzer 中已无调用者的 HDL 注释剥离、列搜索和 VHDL 行规则路径；语言规则现在统一
  消费预处理后的 token/语义结果，只有通用行尾空白与行宽规则继续合理地读取原始行文本。
- AppleClang C++20 严格警告下六个核心测试全部通过；最终 0.55.0 CLI 仍发布 68 条规则，SARIF
  目录为 68 条，递归 file-list 回归仍为 2 文件、2 单元、0 诊断。
- CLAHE 保持 12 文件、12 单元、11 实例和 92 条诊断（1 warning、91 info），旧 baseline 后
  仍为 7 条新增、85 条命中。

## 阶段 54（2026-09-16）

- 将通用 `VHDL-WAIT` 从逐行正则迁移到 architecture/process token 模型；跨行 `wait until`、
  `wait on/for` 与裸 `wait;` 均在关键字位置产生稳定 warning。
- 敏感列表 process 中的直接 wait 以 token offset 与 `VHDL-WAIT-WITH-SENSITIVITY` 去重，只保留
  更具体的 error；局部 procedure/function 内 wait 不误归为外层 legality error，但仍保留通用
  综合可移植性 warning。
- 单元测试覆盖直接非法 wait、wait-on、bare-wait、跨行 wait-until 与局部 procedure wait；CLI
  fixture 精确产生 1 条 warning。AppleClang C++20 严格警告下六个核心测试全部通过。
- 最终 0.54.0 CLI 仍发布 68 条规则，SARIF 目录为 68 条；递归 file-list 回归仍为 2 文件、2 单元、
  0 诊断。CLAHE 保持 12 文件、12 单元、11 实例和 92 条诊断（1 warning、91 info），旧 baseline
  后仍为 7 条新增、85 条命中。

## 阶段 53（2026-09-16）

- 将 `VL-CASEX` 与 `VL-CASE-DEFAULT` 从逐行正则迁移到已有 token 化 case 作用域分析，与重复
  choice 和通配模式相交检查共享 condition、branch 与嵌套 case 边界。
- 单元测试覆盖多行 selector、嵌套内外层独立 default、同一行完整 case，以及 casex 的真实关键字
  定位；注释/字符串仍由 tokenizer 排除。CLI fixture 精确产生 2 条 warning。
- CLI 校准暴露结构检查器把共享 `endcase` 固定匹配到 `case`、从而误报合法 `casex` 的旧缺陷；
  closer 现在会在 `case/casez/casex` 候选中关闭源码位置最近的未闭合 opener，并由回归锁定。
- AppleClang C++20 严格警告下六个核心测试全部通过；最终 0.53.0 CLI 仍发布 68 条规则，SARIF
  目录为 68 条，递归 file-list 回归仍为 2 文件、2 单元、0 诊断。
- CLAHE 保持 12 文件、12 单元、11 实例和 92 条诊断（1 warning、91 info），旧 baseline 后
  仍为 7 条新增、85 条命中；既有 `VL-CASE-DEFAULT` 校准结果保持稳定。

## 阶段 52（2026-09-16）

- 将 `VL-SEQ-BLOCKING` 与 `VL-COMB-NONBLOCKING` 从逐行正则迁移到注释/字符串感知 token 和
  完整过程范围模型，支持赋值目标与运算符跨行、嵌套控制流、位选/切片及准确设计单元上下文。
- 新增语句左值位置判定，区分非阻塞赋值 `<=` 与条件/右值中的关系比较 `<=`；同一判定同时用于
  驱动、确定赋值、位宽截断、组合依赖、敏感列表读取及赋值风格分析，避免子系统语义分叉。
- 针对性测试验证跨行两类赋值各命中一次，关系比较不产生输入驱动或位宽误报，嵌套 `end` 后的
  后续赋值仍识别为 driver。CLI fixture 精确产生 2 条 warning。
- 首次 CLAHE 校准暴露了 `end` 后合法赋值未被视为语句起点的边界并产生 4 条未驱动输出；补齐
  `end/endcase/join*` 边界后重新校准，恢复 12 文件、12 单元、11 实例和 92 条诊断
  （1 warning、91 info），旧 baseline 后仍为 7 条新增、85 条命中。
- AppleClang C++20 严格警告下六个核心测试全部通过；最终 0.52.0 CLI 仍发布 68 条规则，SARIF
  目录为 68 条，递归 file-list 回归仍为 2 文件、2 单元、0 诊断。

## 阶段 51（2026-09-16）

- 新增 `VL-MIXED-ASSIGNMENT-STYLE`（warning），在每个 Verilog/SystemVerilog 过程内按已声明
  基信号跟踪阻塞与非阻塞赋值，首次出现冲突风格时定位并每信号去重。
- 单元测试验证位选目标归并、同一过程混用命中、不同过程风格不合并，以及 severity、设计单元、
  signal 和冲突行上下文；`for` 循环控制头从赋值风格统计中排除。
- CLI fixture 使用既有行级 waiver 屏蔽泛化的 `VL-SEQ-BLOCKING`，最终精确保留 1 条新 warning，
  同时证明 suppression 计数为 1。AppleClang C++20 严格警告下六个核心测试全部通过。
- 最终 0.51.0 CLI 发布 68 条规则，SARIF 目录为 68 条；递归 file-list 回归仍为 2 文件、2 单元、
  0 诊断。CLAHE 保持 12 文件、12 单元、11 实例和 92 条诊断（1 warning、91 info），旧 baseline
  后仍为 7 条新增、85 条命中。

## 阶段 50（2026-09-16）

- 新增 `SV-ALWAYS-LATCH-NO-LATCH`（warning），复用确定赋值控制流分析，检查 `always_latch`
  是否至少有一个可赋值信号在未覆盖路径上保留原值；完整赋值过程提示改用 `always_comb`。
- 修正 Verilog/SystemVerilog 单语句过程范围：不再在 `if` 首个分号处截断，而是完整覆盖
  `if/else`、循环、case、begin/end 与嵌套 event control。这一底层修复同步提升过程驱动、锁存、
  延时及 SystemVerilog 专用规则的范围准确性。
- 单元测试覆盖真实锁存路径、完整 `if/else`、非法事件控制与 `.v` 语言门控；CLI fixture 精确
  产生 5 条诊断（4 error、1 warning）。AppleClang C++20 严格警告下六个核心测试全部通过。
- 最终 0.50.0 CLI 发布 67 条规则，SARIF 目录为 67 条；递归 file-list 回归仍为 2 文件、2 单元、
  0 诊断。CLAHE 保持 12 文件、12 单元、11 实例和 92 条诊断（1 warning、91 info），旧 baseline
  后仍为 7 条新增、85 条命中。

## 阶段 49（2026-09-16）

- 新增 `SV-ALWAYS-LATCH-EVENT-CONTROL`（error），在 `always_latch` 已分界的完整过程范围内发现
  显式 `@` 时定位 event control，防止自动敏感锁存过程被写成混合语义。
- 单元测试同时覆盖非法显式事件、合法自动敏感锁存过程，以及 `.v` 文件语言门控；与既有
  `always_comb`、`always_ff` 用例组合后，CLI fixture 精确产生 4 条 error。
- AppleClang C++20 严格警告下六个核心测试程序全部通过；最终 0.49.0 CLI 发布 66 条规则，
  SARIF 目录为 66 条，递归 file-list 回归仍为 2 文件、2 单元、0 诊断。
- CLAHE 保持 12 文件、12 单元、11 实例和 92 条诊断（1 warning、91 info）；旧 baseline 后
  仍为 7 条新增、85 条命中，符合该纯 Verilog 工程不运行 SystemVerilog 专用规则的边界。

## 阶段 48（2026-09-16）

- 新增 `RTL-REDUNDANT-SENSITIVITY`（info），检查显式组合敏感列表中已声明、但过程体从未读取的
  信号；覆盖 Verilog/SystemVerilog `always @(...)` 与 VHDL `process(...)`。
- 复用同一过程体读取集合双向计算缺失项和冗余项，并修正“信号在过程内既写又读”时被全局
  assigned 集合错误排除，以及 VHDL 索引/切片赋值目标被误算为读取的边界；自动敏感列表和
  边沿触发过程继续排除。
- 单元测试覆盖两种 HDL 的冗余项、完整列表、自动列表、时钟过程及读写同一信号回归；CLI fixture
  精确产生 1 条 info。AppleClang C++20 严格警告下六个核心测试程序全部通过。
- 最终 0.48.0 CLI 发布 65 条规则，SARIF 目录为 65 条；递归 file-list 回归仍分析 2 文件、2 设计
  单元且为 0 诊断。CLAHE 保持 12 文件、12 单元、11 实例和 92 条诊断（1 warning、91 info），
  旧 baseline 后保持 7 条新增、85 条命中，未引入新噪声。

## 阶段 47（2026-09-16）

- 新增 `SV-ALWAYS-COMB-EVENT-CONTROL`（error），在 `always_comb` 范围内发现显式 `@` 时定位
  event control，防止自动敏感过程被写成混合语义。
- 新增 `SV-ALWAYS-FF-EVENT-CONTROL`（error），要求 `always_ff` 整个过程范围恰有一个 event
  control；缺失时定位关键字，多个时定位第二个 `@`。
- 单元测试同一 module 覆盖 comb 非法事件、ff 缺失、ff 多事件、ff 单事件四种模板，并验证
  `.v` 文件不运行 SV 专用规则；CLI fixture 精确产生 3 条 error，无级联诊断。
- AppleClang C++20 严格警告下六个核心测试程序全部通过；最终 0.47.0 CLI 发布 64 条规则，
  SARIF 目录为 64 条。CLAHE 为 Verilog 工程，回归保持 92 条诊断和 7 条 baseline 后新增问题。

## 阶段 46（2026-09-16）

- 新增 `VHDL-WAIT-WITH-SENSITIVITY`（error），在显式 wait 位置报告带 sensitivity list 的
  process 违反 VHDL 进程挂起语义。
- 检查按 process token 范围执行并跳过局部 procedure/function body；无 sensitivity process
  中的 wait 保持合法结构，仅由既有 `VHDL-WAIT` 提示综合风险。
- 报告去重：同一行命中特定 legality error 后不再发射通用 wait warning；通用规则同时扩展到
  裸 `wait;`。单元测试验证非法、wait-on 与 bare-wait 进程，结果为 1 error + 2 独立 warning；CLI fixture
  仅产生 1 条 legality error。
- AppleClang C++20 严格警告下六个核心测试程序全部通过；最终 0.46.0 CLI 发布 62 条规则，
  SARIF 目录为 62 条。CLAHE 为纯 Verilog 工程，回归保持 92 条诊断和 7 条 baseline 后新增问题。

## 阶段 45（2026-09-16）

- 新增 `VHDL-PROCESS-NO-SUSPENSION`（error），检查 architecture process 既无 sensitivity list，
  整个过程范围也没有显式 `wait`，从而在零仿真时间无限重复的错误模板。
- token 范围按每个 process 独立判断，注释/字符串不会参与；`process(a)`、`process(all)` 与含
  `wait on/until/for` 的无 sensitivity process 均作为反例，不产生该诊断。
- 单元测试同一 entity 覆盖四种模板，仅无挂起点进程精确产生 1 条 error；CLI fixture 同样只
  产生该规则，并验证实体、行列和严重级上下文。
- AppleClang C++20 严格警告下六个核心测试程序全部通过；最终 0.45.0 CLI 发布 61 条规则，
  SARIF 目录为 61 条。CLAHE 为纯 Verilog 工程，回归保持 92 条诊断和 7 条 baseline 后新增问题。

## 阶段 44（2026-09-16）

- 新增 dependency-free `project::loadFileList`，支持 HDL 文件/目录、递归 `-f/-F`、`+incdir+`、
  `+define+`、`-I/-D`、引号路径及注释；所有相对路径以包含它们的清单目录解析。
- CLI 新增 `-f FILE`、`-fFILE`、`--file-list FILE`，项目配置新增 `file-list PATH`；清单 include
  目录与宏直接进入现有预处理链路，不复制源码发现或分析逻辑。
- fixture 覆盖两层嵌套清单、SystemVerilog/VHDL 混合输入、include 解析、条件宏、plus options、
  短选项、引号、循环引用、未知选项及未闭合引号。CLI 与配置入口均分析 2 文件、2 设计单元，
  结果为 0 条诊断；循环引用精确定位到嵌套清单行。
- AppleClang C++20 严格警告下六个核心测试程序全部通过；最终 0.44.0 CLI 仍发布 60 条规则，
  SARIF 目录保持 60 条。CLAHE 直接目录回归保持 92 条诊断，旧基线仍留下 7 条新增活动问题。

## 阶段 43（2026-09-16）

- 新增 `STRUCT-UNCONNECTED-OUTPUT-PORT`（info），对唯一解析且接口可用的实例检查遗漏、空 actual
  或 VHDL `open` 的 output/inout；显式悬空定位 association，遗漏项定位实例。
- 规则复用现有 named、positional 和 VHDL mixed 映射模型；完整连接不报，Verilog `.*`、未解析
  IP、black box 及无接口模型目标保持保守排除。
- SystemVerilog 与 VHDL 单元测试各覆盖 named omission、显式 open、尾部 positional omission、
  完整连接；两种语言均精确产生 4 条 info，CLI fixture 同样验证 rule count 为 4。
- CLAHE 新发现 2 条有效 info：`clahe_ram_64tiles_sim` 中 `ram_a_inst` 与 `ram_b_inst` 的 `douta`
  输出未连接；总诊断由 90 增至 92（1 warning、91 info）。
- AppleClang C++20 严格警告下六个核心测试程序全部通过；最终 0.43.0 CLI 发布 60 条规则，
  SARIF 目录为 60 条。旧 CLAHE 基线匹配 85 条历史问题，并保留 7 条后续新增活动问题。

## 阶段 42（2026-09-16）

- 新增 `STRUCT-DUPLICATE-SIGNAL-DECLARATION`（error），在第二次声明位置报告同一设计单元
  作用域中的内部信号重名。
- Verilog/SystemVerilog 覆盖跨声明重复、同一 grouped declaration 内重复和 ANSI port 同名冲突，
  并排除复合类型关键字被重复扫描；合法 legacy port/net 重声明保持兼容。VHDL 按大小写不敏感
  语义检查 signal 重复，并识别 architecture signal 与 entity port 的同名冲突。
- SystemVerilog 单元测试及 CLI fixture 精确产生 3 条 error，VHDL 精确产生 2 条；诊断保留
  designUnit、signal、行列定位。CLAHE 无重复信号声明，总诊断保持 90 条（1 warning、89 info）。
- AppleClang C++20 严格警告下六个核心测试程序全部通过；最终 0.42.0 CLI 发布 59 条规则，
  SARIF 目录为 59 条。旧 CLAHE 基线仍匹配 85 条历史问题，并保留阶段 41 的 5 条新增活动问题。

## 阶段 41（2026-09-16）

- 新增 `RTL-UNUSED-INPUT-PORT`（info），覆盖 Verilog、SystemVerilog 与 VHDL，定位声明后既未被
  本地逻辑读取、也未作为实例 actual/port map actual 转发的 input/in 端口。
- 复用注释/字符串感知的语义 token 模型，不用原始文本猜测；敏感列表、条件表达式、普通表达式
  和实例连接均视为有效使用，VHDL 名称按大小写不敏感语义处理。
- SystemVerilog 与 VHDL 单元测试均验证“使用、转发、未使用”三类输入，CLI fixture 验证诊断的
  rule ID 与 signal 上下文；被内部非法驱动的输入只保留 `RTL-INPUT-DRIVEN`，不产生级联提示。
- CLAHE 实测新增 5 条有效 info：两个版本直方图模块中未使用的 `ping_pong_flag`，以及映射模块
  中未使用的 `pixel_x`、`pixel_y`、`cdf_ready`；总诊断由 85 增至 90（1 warning、89 info）。
- AppleClang C++20 严格警告下六个核心测试程序全部通过；最终 0.41.0 CLI 发布 58 条规则，
  SARIF 目录为 58 条。旧 CLAHE 基线回读后精确保留 5 条新增活动问题，并匹配 85 条历史问题。

## 阶段 40（2026-09-16）

- 报告层新增统一聚合器，按 override 后的最终有效级别计算 critical/error/warning/info，并按
  rule ID 确定性汇总活动诊断；suppressed/baselined 项不会混入活动统计。
- text 增加 severity/rule totals；JSON summary 与 SARIF run properties 新增
  `severityCounts`、`ruleCounts`；HTML 增加四类严重级指标卡及 Rule summary 表。
- GUI 状态栏同步显示 C/E/W/I、suppression 与 baseline 数量；报告聚合仍只消费统一
  AnalysisResult，不复制规则执行逻辑。
- 单元测试构造四种严重级和重复 rule ID，验证四种报告格式；真实 CLI JSON/SARIF 均可由
  jq 解析。CLAHE 的 85 条活动诊断聚合为 1 warning、84 info，规则计数合计为 85。
- AppleClang C++20 严格警告下六个核心测试程序全部通过；最终 0.40.0 CLI 仍发布 57 条规则，
  SARIF 目录为 57 条。CLAHE 基线仍为 86 行，回读后为 0 条活动问题和 85 条 baselined 问题。

## 阶段 39（2026-09-16）

- DesignUnitInfo 新增重复 port 与 parameter/generic 声明位置集合；接口模型继续保留首个声明，
  避免重复节点影响实例映射和数量检查。
- 新增 `STRUCT-DUPLICATE-PORT-DECLARATION` 与
  `STRUCT-DUPLICATE-PARAMETER-DECLARATION`（均为 error），诊断定位第二次声明并附 unit/signal。
- SystemVerilog fixture 覆盖重复 ANSI port 和 parameter；VHDL fixture 验证 port/generic 名称按
  大小写不敏感语义判重。CLI fixture 精确产生 2 条 error。
- 真实 CLAHE 保持 12 文件、12 单元、197 个端口、19 个 parameter 和 85 条既有诊断，两条
  新规则无新增误报。
- AppleClang C++20 严格警告下六个核心测试程序全部通过；最终 0.39.0 CLI 发布 57 条规则，
  实际发射集合与目录为 57/57，SARIF 目录为 57 条。CLAHE 基线仍为 86 行，回读后为
  0 条活动问题和 85 条 baselined 问题。

## 阶段 38（2026-09-16）

- ConnectionShape 新增 association 顺序状态，InstanceInfo 分别保留 port map 与 generic map
  的 invalid-order 标志；JSON、HTML 和 GUI 同步公开该状态。
- 新增 `STRUCT-VHDL-ASSOCIATION-ORDER`（error），检查 named association 后再次出现
  positional association 的非法 VHDL map。
- 对合法 mixed map 建立 positional 前缀到目标声明的映射；后续 named formal 若再次覆盖已由
  positional 关联的 port/generic，复用 duplicate named port/parameter 规则报告。
- VHDL fixture 精确产生 1 条 duplicate generic、1 条 duplicate port 和 2 条 association-order
  error，同时既有合法 mixed fixture 保持无误报。
- 真实 CLAHE 不含 VHDL association，仍保持 85 条既有诊断，无结构回归。
- AppleClang C++20 严格警告下六个核心测试程序全部通过；最终 0.38.0 CLI 发布 55 条规则，
  实际发射集合与目录为 55/55，SARIF 目录为 55 条。CLAHE 基线仍为 86 行，回读后为
  0 条活动问题和 85 条 baselined 问题。

## 阶段 37（2026-09-16）

- InstanceInfo 新增按映射顺序保存的 positional actual 状态，区分非空连接、空 actual 与
  VHDL `open`；JSON 新增 `positionalConnections[]` 及其来源和 `actualConnected`。
- `STRUCT-UNCONNECTED-INPUT-PORT` 从纯 named 扩展至 positional 及 VHDL 合法 mixed map，
  按目标声明顺序检查无默认值 input；省略 output 和带默认值 input 保持合法。
- `STRUCT-POSITIONAL-PORT-COUNT` 收紧为只报告 actual 超过目标端口数，消除“少于端口总数”
  对合法省略场景的笼统告警。
- SystemVerilog fixture 精确产生 2 条 required-input warning 和 1 条 too-many warning；VHDL
  fixture 覆盖 missing、open、mixed missing、mixed valid 与 too-many。
- 真实 CLAHE 的 11 个实例均为 named style，仍保持 85 条既有诊断，新语义无新增误报。
- AppleClang C++20 严格警告下六个核心测试程序全部通过；最终 0.37.0 CLI 仍发布 54 条规则，
  实际发射集合与目录为 54/54，SARIF 目录为 54 条。CLAHE 基线仍为 86 行，回读后为
  0 条活动问题和 85 条 baselined 问题。

## 阶段 36（2026-09-16）

- 新增 `STRUCT-MISSING-REQUIRED-PARAMETER`，按 parameter/generic 默认值以及 positional 前缀、
  named formal 的覆盖关系检查实例是否遗漏必需参数。
- 新增 `STRUCT-POSITIONAL-PARAMETER-COUNT`，拒绝 override 数量超过目标声明；新增
  `STRUCT-MIXED-PARAMETER-OVERRIDE-STYLE`，拒绝 Verilog/SystemVerilog 混合覆盖风格，
  同时保留 VHDL positional 后 named 的合法语义。
- SystemVerilog 与 VHDL fixture 覆盖缺失、部分 named、完整 named、完整 positional、过多
  positional 和 mixed 反例；CLI fixture 精确产生 4 条 error。
- 真实 CLAHE 仍提取 19 个 parameter、15 个 named override，保持 12 文件、12 单元、11 实例、
  85 条既有诊断，三条新规则无新增误报。
- AppleClang C++20 严格警告下六个核心测试程序全部通过；最终 0.36.0 CLI 发布 54 条规则，
  实际发射集合与目录为 54/54，SARIF 目录为 54 条。CLAHE 基线仍为 86 行，回读后为
  0 条活动问题和 85 条 baselined 问题。

## 阶段 35（2026-09-16）

- DesignUnitInfo 新增 parameter/generic 声明，InstanceInfo 新增 named override、override 总数及
  named/positional 风格；JSON、HTML 和 GUI 同步展示接口与覆盖形状。
- 新增 `STRUCT-DUPLICATE-NAMED-PARAMETER` 和 `STRUCT-UNKNOWN-NAMED-PARAMETER`（均为
  error），覆盖 SystemVerilog value/type parameter 与 VHDL generic，后者按大小写不敏感比较。
- fixture 覆盖重复、未知、无参数目标上的非法 override、positional 反例、type parameter、
  VHDL 分组 generic 声明及报告字段；CLI fixture 产生 3 条预期 error。
- 真实 CLAHE 提取 19 个 parameter 和 15 个 named override，仍保持 12 文件、12 单元、11 实例、
  197 个端口、147 个 named port formal 和 85 条既有诊断，两条新规则无新增误报。
- AppleClang C++20 严格警告下六个核心测试程序全部通过；最终 0.35.0 CLI 发布 51 条规则，
  实际发射集合与目录为 51/51，SARIF 目录为 51 条。CLAHE 基线仍为 86 行，回读后为
  0 条活动问题和 85 条 baselined 问题。

## 阶段 34（2026-09-15）

- PortInfo 新增 `hasDefault`，named connection 新增 `actualConnected`，InstanceInfo 新增
  `hasWildcardConnection`；JSON、HTML 和 GUI 同步公开默认端口、空/open actual 与通配状态。
- 新增 `STRUCT-UNCONNECTED-INPUT-PORT`（warning），检查已解析目标的纯 named 实例是否遗漏
  必需 input，或通过 `.formal()` / `formal => open` 显式悬空。
- SystemVerilog 与 VHDL fixture 均覆盖 omitted、empty/open、带默认值 input、output、Verilog
  `.*`、SystemVerilog `.port` shorthand 和 VHDL positional-prefix mixed map；两种语言各产生
  2 条预期诊断。
- CLI fixture 产生 2 条 warning；真实 CLAHE 仍为 12 文件、12 单元、11 实例、197 个端口、
  147 个 named formal 和 85 条既有诊断，新规则无新增误报。
- AppleClang C++20 严格警告下六个核心测试程序全部通过；最终 0.34.0 CLI 发布 49 条规则，
  实际发射集合与目录为 49/49，SARIF 目录为 49 条。CLAHE 基线仍为 86 行，回读后为
  0 条活动问题和 85 条 baselined 问题。

## 阶段 33（2026-09-15）

- InstanceInfo 新增 port connection 总数、是否存在连接列表及 named/positional 风格标志；
  JSON 输出 `portConnectionCount`、`hasPortConnectionList`、`connectionStyle`，HTML/GUI
  同步展示连接数量和风格。
- 新增 `STRUCT-POSITIONAL-PORT-COUNT`（warning），对已解析 module/entity 比较纯 positional
  actual 数与目标 ports；新增 `STRUCT-MIXED-PORT-CONNECTION-STYLE`（error），检查
  Verilog/SystemVerilog 实例混用 ordered 和 named 连接。
- fixture 覆盖少连、多连、正确数量、Verilog mixed style、VHDL positional 数量不一致，以及
  VHDL 合法“positional 后 named”反例；诊断级别、父单元上下文和 JSON 风格字段均验证。
- CLI fixture 产生 2 条 count warning 和 1 条 mixed-style error；CLAHE 的 11 个实例全部
  分类为 named style，继续保持 197 个端口、147 个 named formal 和 85 条既有诊断。
- AppleClang C++20 严格警告下六个核心测试程序全部通过；最终 0.33.0 CLI 发布 48 条规则，
  实际发射集合与目录为 48/48，SARIF 目录为 48 条。CLAHE 基线仍为 86 行，回读后为
  0 条活动问题和 85 条 baselined 问题。

## 阶段 32（2026-09-15）

- 将 Stage 31 的端口结构模型扩展到 VHDL：提取 entity `port(...)` 名称、in/out/inout/buffer
  方向和来源，并从 direct entity/component instance 的 `port map` 中保留 named formal。
- `STRUCT-DUPLICATE-NAMED-PORT` 与 `STRUCT-UNKNOWN-NAMED-PORT` 现适用于全部 HDL；VHDL
  formal 按大小写不敏感语义去重并与已解析 entity 端口比较。
- fixture 验证 `a`/`A` 重复、`Missing` 未知端口、端口顺序/方向、association 顺序和诊断
  entity/signal 上下文；CLI fixture 同样产生两条预期结构诊断。
- 工作区真实 VHDL counter 提取 4 个 entity ports、1 个时钟、1 个复位、1 个时序过程且
  0 诊断；CLAHE 仍提取 197 个 module ports、147 个 named connections 和 85 条既有诊断。
- AppleClang C++20 严格警告下六个核心测试程序全部通过；最终 0.32.0 CLI 仍发布 46 条规则，
  实际发射集合与目录为 46/46，SARIF 目录为 46 条。CLAHE 基线仍为 86 行，回读后为
  0 条活动问题和 85 条 baselined 问题。

## 阶段 31（2026-09-15）

- 设计结构模型新增 Verilog/SystemVerilog module `ports[]`（名称、方向、来源）和实例
  `namedConnections[]`（formal、来源）；JSON、HTML 和 GUI 层次同步展示端口与连接计数。
- 新增 `STRUCT-DUPLICATE-NAMED-PORT` 与 `STRUCT-UNKNOWN-NAMED-PORT`（error）：前者定位
  同一实例重复 formal，后者在跨文件目标唯一解析后校验 formal 是否存在。
- fixture 覆盖 ANSI 端口方向与顺序、legacy non-ANSI 声明、重复/未知 named connection、
  positional connection 反例，以及 JSON 模型字段和诊断 designUnit/signal/位置上下文。
- 真实 CLAHE RTL 提取 197 个 module 端口、147 个 named connections，仍为 85 条既有诊断、
  11 个实例，两条新规则均无误报。
- AppleClang C++20 严格警告下六个核心测试程序全部通过；最终 0.31.0 CLI 发布 46 条规则，
  实际发射集合与目录为 46/46，SARIF 目录为 46 条。CLAHE 基线仍为 86 行，回读后为
  0 条活动问题和 85 条 baselined 问题。

## 阶段 30（2026-09-15）

- 新增 `RTL-UNUSED-ASYNC-RESET`（error）：比较时序过程的异步 reset 关联与过程体 reads，
  定位 reset/clear/preset 已进入敏感列表却完全没有复位分支或其他读取的模板遗漏。
- VHDL timing extraction 现在先保留时序 process sensitivity list 中的 reset-like 控制，即使
  过程体没有引用；fixture 覆盖 Verilog/SV reset event、VHDL clear sensitivity，以及两种
  正常复位分支反例，并验证 error、designUnit、signal 和事件来源。
- 真实 CLAHE RTL 回归仍为 85 条既有诊断、14 clocks、9 resets、33 个时序过程，新增规则
  无误报且既有时序统计未改变。
- AppleClang C++20 严格警告下六个核心测试程序全部通过；最终 0.30.0 CLI 发布 44 条规则，
  实际发射集合与目录为 44/44，SARIF 目录为 44 条。CLAHE 基线仍为 86 行，回读后为
  0 条活动问题和 85 条 baselined 问题。

## 阶段 29（2026-09-15）

- 新增 `RTL-OUT-OF-RANGE-SELECT`（error）：信号语义模型在宽度之外保留单维常量声明的真实
  low/high，检查 Verilog/SystemVerilog `[]` 和 VHDL `()` 的常量位选及切片端点。
- fixture 覆盖 Verilog based integer 索引、`[15:8]` 非零边界、VHDL `8 to 15` 升序范围，
  并验证 error、designUnit、signal 和首个越界端点来源；合法切片、动态索引和参数化未知范围
  均不会命中。
- 真实 CLAHE RTL 回归仍为 85 条既有诊断、14 clocks、9 resets、33 个时序过程，新增规则
  无误报。
- AppleClang C++20 严格警告下六个核心测试程序全部通过；最终 0.29.0 CLI 发布 43 条规则，
  实际发射集合与目录为 43/43，SARIF 目录为 43 条。CLAHE 基线仍为 86 行，回读后为
  0 条活动问题和 85 条 baselined 问题。

## 阶段 28（2026-09-15）

- 新增 `VL-IMPLICIT-NET`（warning）：在 Verilog 中检查简单 continuous assignment 左端、
  单标识符右端，以及 module 实例 named/positional 端口的单标识符实际信号；每个未声明名称
  在单 module 内只报告首次出现，并保留 designUnit/signal/精确来源。
- fixture 覆盖三个入口，同时证明 parameter 常量不会误报；SystemVerilog 用户自定义类型采用
  明确反例，当前 Verilog 专用规则不会在不完整类型系统上猜测。
- 真实 CLAHE RTL 回归仍为 85 条既有诊断、14 clocks、9 resets、33 个时序过程，新增规则
  无误报。
- AppleClang C++20 严格警告下六个核心测试程序全部通过；最终 0.28.0 CLI 发布 42 条规则，
  实际发射集合与目录为 42/42，SARIF 目录为 42 条。CLAHE 基线仍为 86 行，回读后为
  0 条活动问题和 85 条 baselined 问题。

## 阶段 27（2026-09-15）

- 新增 `RTL-MULTIPLE-CLOCKS-PER-PROCESS`（error）：时序模型保留每个过程的全部时钟边沿
  事件，定位由两个或更多独立时钟边沿驱动的单一过程，并在 JSON、HTML 与 GUI 摘要中暴露
  clock event 数量和来源。
- fixture 覆盖 Verilog/SystemVerilog 的双边沿敏感列表及 VHDL 的两个 edge 调用，验证 error、
  designUnit、第二时钟 signal 和精确来源；异步 `clear_n` 对照证明控制事件不会误计为第二时钟，
  同一信号的双边沿对照证明其只属于既有 mixed-edge 规则。
- 首轮真实项目校准发现过宽的 clear/clr 名称匹配会污染同步复位统计；收紧为只在多边沿
  Verilog 敏感列表中识别异步 clear/preset 后，CLAHE 恢复为 85 条既有诊断、14 clocks、
  9 resets、33 个时序过程，新规则无误报。
- AppleClang C++20 严格警告下六个核心测试程序全部通过；最终 0.27.0 CLI 发布 41 条规则，
  实际发射集合与目录为 41/41，SARIF 目录为 41 条。CLAHE 基线仍为 86 行，回读后为
  0 条活动问题和 85 条 baselined 问题。

## 阶段 26（2026-09-15）

- 新增 `VL-OVERLAPPING-WILDCARD-CASE`（warning）：解析 sized binary/hex choice 为位模式，
  对 casez 的 z/? 和 casex 的 x/z/? 执行相交检查，定位后出现的重叠分支。
- 数字 lexer 仅在 based literal 内接收 `?`；fixture 覆盖 binary casez、casex、hex wildcard
  三类重叠，并证明 casez 中无法按二态精确解释的字面 x 会保守跳过。
- 与完全相同 token 的 duplicate 规则分工，避免同一问题双报；诊断验证 warning 和
  designUnit 上下文。CLAHE 仍为 85 条既有诊断，无 wildcard overlap 新增误报。
- AppleClang C++20 严格警告下六个核心测试程序全部通过；最终 0.26.0 CLI 发布 40 条规则，
  实际发射集合与目录为 40/40，SARIF 目录为 40 条。CLAHE 基线仍为 86 行，回读后为
  0 条活动问题和 85 条 baselined 问题。

## 阶段 25（2026-09-15）

- 新增 `RTL-DUPLICATE-CASE-ITEM`（warning）：按 case 作用域记录规范化 choice，展开
  Verilog/SystemVerilog 逗号列表和 VHDL `|` 列表，定位后出现的完全重复分支。
- fixture 覆盖两种 HDL 的多选项重复；Verilog 还包含嵌套 case，证明内外层相同值不会串扰。
  诊断验证 warning、后续分支行和 designUnit 上下文。
- CLAHE 仍为 85 条既有诊断、33 个时序过程、14 clocks、9 resets，无重复 case 新增误报。
- AppleClang C++20 严格警告下六个核心测试程序全部通过；最终 0.25.0 CLI 发布 39 条规则，
  实际发射集合与目录为 39/39，SARIF 目录为 39 条。CLAHE 基线仍为 86 行，回读后为
  0 条活动问题和 85 条 baselined 问题。

## 阶段 24（2026-09-15）

- 新增 `RTL-NONSYNTHESIZABLE-DELAY`（warning）：检查 Verilog/SystemVerilog 过程 `#delay`、
  continuous assign delay，以及 VHDL signal assignment 的 `after` 子句。
- fixture 覆盖两种 Verilog 延时和 VHDL after，并验证 warning/designUnit 上下文；module 与
  instance 的 `#(.PARAM(...))` 参数覆盖具有明确反例，不会被当成仿真延时。
- CLAHE 仍为 85 条既有诊断、33 个时序过程、14 clocks、9 resets，无延时新增误报。
- AppleClang C++20 严格警告下六个核心测试程序全部通过；最终 0.24.0 CLI 发布 38 条规则，
  实际发射集合与目录为 38/38，SARIF 目录为 38 条。CLAHE 基线仍为 86 行，回读后为
  0 条活动问题和 85 条 baselined 问题。

## 阶段 23（2026-09-15）

- 新增 `RTL-DERIVED-CLOCK`（warning）：关联 Verilog/SystemVerilog posedge/negedge 与 VHDL
  rising_edge/falling_edge 信号的本地运行时 driver，定位由 RTL 生成后又作为时钟使用的信号。
- fixture 覆盖 Verilog continuous gated clock、VHDL concurrent gated clock，并验证 warning、
  driver 行、designUnit/signal 上下文；输入时钟和仅通过 vendor clock 实例输出连接的时钟不报。
- CLAHE 仍为 85 条既有诊断、33 个时序过程、14 clocks、9 resets，无派生时钟新增误报。
- AppleClang C++20 严格警告下六个核心测试程序全部通过；最终 0.23.0 CLI 发布 37 条规则，
  实际发射集合与目录为 37/37，SARIF 目录为 37 条。CLAHE 基线仍为 86 行，回读后为
  0 条活动问题和 85 条 baselined 问题。

## 阶段 22（2026-09-15）

- 新增 `RTL-WIDTH-TRUNCATION`（warning）：解析 Verilog/SystemVerilog 十进制常量 packed
  range 与 VHDL `std_logic_vector`/`bit_vector`/`signed`/`unsigned` 常量范围，只在直接
  单信号赋值且源宽度大于目标宽度时报告。
- fixture 覆盖 Verilog 16→8、VHDL 16→8 截断及诊断尺寸、warning、designUnit/signal
  上下文；扩宽赋值、显式 `[7:0]` 切片和参数化未知宽度均不报。
- CLAHE 仍为 85 条既有诊断、33 个时序过程、14 clocks、9 resets，无位宽新增误报。
- AppleClang C++20 严格警告下六个核心测试程序全部通过；最终 0.22.0 CLI 发布 36 条规则，
  实际发射集合与目录为 36/36，SARIF 目录为 36 条。CLAHE 基线仍为 86 行，回读后为
  0 条活动问题和 85 条 baselined 问题。

## 阶段 21（2026-09-15）

- 新增 `RTL-COMBINATIONAL-LOOP`（error）：为 Verilog/SystemVerilog 连续赋值与组合过程、
  VHDL 并发赋值与组合 process 建立依赖图，以 Tarjan 强连通分量对每个环去重报告。
- fixture 覆盖跨 continuous/always_comb 的双节点环、VHDL 并发双节点环、合法时序自反馈，
  并验证诊断的 error、designUnit 和 signal 上下文。
- 首轮定向测试发现带默认值的阻塞赋值累加器会形成表面自边；图构建加入过程内赋值顺序及
  `for(...)` 控制变量排除后，XOR reduction 类组合循环不再误报，未初始化自引用仍可命中。
- CLAHE 仍为 85 条既有诊断、33 个时序过程、14 clocks、9 resets，无组合环新增误报。
- AppleClang C++20 严格警告下六个核心测试程序全部通过；最终 0.21.0 CLI 发布 35 条规则，
  实际发射集合与目录为 35/35，SARIF 目录为 35 条。CLAHE 基线仍为 86 行，回读后为
  0 条活动问题和 85 条 baselined 问题。

## 阶段 20（2026-09-15）

- 新增 `RDC-MULTI-CLOCK-ASYNC-RESET`（warning）：将每个时序过程与其复位、有效电平、
  同步/异步属性关联，仅在同一异步复位直接控制两个或更多时钟边沿域时报告。
- Verilog/SystemVerilog 与 VHDL fixture 均覆盖双时钟异步复位；对照 fixture 证明一个异步
  使用加一个同步使用不会命中。诊断保留 designUnit、reset signal 和全部时钟域文本。
- JSON 在每个 sequential process 下输出 resets 数组；HTML 和 GUI 过程节点显示复位计数。
- CLAHE 仍为 85 条既有诊断、33 个时序过程、14 clocks、9 resets，无新增 RDC 误报。
- AppleClang C++20 严格警告下六个核心测试程序全部通过；最终 0.20.0 CLI 发布 34 条规则，
  SARIF 目录为 34 条。CLAHE 提取 27 个过程级复位关联，完整基线仍为 86 行，回读后为
  0 条活动问题和 85 条 baselined 问题。

## 阶段 19（2026-09-15）

- 新增 `CDC-DIRECT-CROSSING`（warning）：为 Verilog/SystemVerilog 与 VHDL 时序过程记录
  时钟、边沿、来源位置及排序后的读写集合，并检查单设计单元内的直接寄存器跨域。
- fixture 覆盖双语言 clk_a→clk_b 跨域、诊断 designUnit/signal/目标过程定位、JSON 过程模型，
  同时证明同域流水线不报。索引访问单独记录，双时钟 RAM fixture 不按普通寄存器跨域处理。
- 首轮 CLAHE 校准发现 3 条均来自双口 RAM 的 `ram[...]` 访问；加入索引存储排除后回归恢复
  85 条既有诊断，无 CDC 新增误报，并提取 33 个时序过程。
- AppleClang C++20 严格警告下六个核心测试程序全部通过；最终 0.19.0 CLI 发布 33 条规则，
  SARIF 目录为 33 条，HTML 展示 33 个过程。CLAHE 完整基线仍为 86 行，回读结果为 0 条
  活动问题、85 条 baselined 问题，时钟、复位及过程统计保持一致。

## 阶段 18（2026-09-15）

- 新增跨过程 `RTL-MIXED-CLOCK-EDGE`、`RTL-MIXED-RESET-MODE`（warning）和
  `RTL-RESET-POLARITY-CONFLICT`（error），同时覆盖 Verilog/SystemVerilog 与 VHDL。
- ResetInfo 保留同步/异步、高/低有效四个 observed 标志；跨 VHDL architecture 合并时不会
  丢失观察状态。JSON 输出全部标志，HTML/GUI 对冲突显示 mixed mode/polarity。
- fixture 验证同一 clk 的 posedge/negedge、同一 reset 的 async/sync 及高低有效冲突；极性
  诊断保留 designUnit/signal 上下文和 error 严重级。两种 HDL 的等价结构均命中三条规则。
- 真实 CLAHE RTL 仍为 85 条诊断、14 clocks、9 resets，三条新规则均无误报；双口 RAM 的
  两个不同命名时钟不触发 mixed edge。实际发射集合与中央目录重新核对为 32/32。
- AppleClang C++20 严格警告下六个核心测试程序全部通过；最终 0.18.0 CLI 的版本、帮助、
  32 条规则查询、配置 fixture 与 SARIF 完整目录均通过。CLAHE 基线文件为 86 行（header
  加 85 条记录），回读后为 0 条活动问题和 85 条 baselined 问题，结构统计保持不变。

## 阶段 17（2026-09-15）

- 新增项目 `severity RULE LEVEL` 与 CLI `--severity RULE=LEVEL`，支持 info、warning、error、
  critical；列表按顺序应用，同一规则最后一次覆盖生效，关闭规则仍由 disable 表达。
- 规则与级别均通过中央目录校验；缺值、错误格式、未知规则和非法级别在 CLI/config 两侧均
  返回 2。配置 fixture 覆盖全部错误类别及相对路径设置共存。
- Analyzer 在单文件语义诊断及项目级结构诊断完成后统一应用策略，报告与 `--fail-on` 消费
  最终级别。AnalysisResult 保留覆盖列表，text、JSON、SARIF run properties 和 HTML 均展示。
- 生命周期验证：VL-CASEX 默认 warning 在 fail-on error 下返回 0，提升 error 后返回 1，
  后续覆盖 info 后恢复 0；提升为 critical 仍可匹配原基线的 5 条记录，证明严重级不参与指纹。

## 阶段 16（2026-09-15）

- 新增中央 `rule_catalog`，29 条内建规则均有唯一 ID、默认严重级、适用语言、稳定标题、
  说明和修复建议；目录按 ID 确定性排序并支持不区分大小写查询。
- CLI 新增 `--list-rules` 和 `--describe-rule RULE`，无需 HDL 输入即可查看目录；未知查询、
  CLI `--disable` 及配置 `disable` 均返回明确错误，避免拼写错误静默生效。
- 语义分析严重级从中央目录读取，不再维护独立 if/else 映射；SARIF tool.driver.rules 始终
  输出完整 29 条目录，即使本次分析未命中对应规则。
- 独立目录测试验证数量、唯一性、排序、字段完整性、大小写查询及未知项；源码扫描对照证明
  29 个实际发射的规则 ID 与目录集合完全一致，无 missing/unused 条目。
- AppleClang C++20 严格警告下六个核心测试程序及 8 个 CLI/报告场景通过；CLAHE 保持
  85 条原始诊断、14 clocks、9 resets，完整基线回读后仍为 0 活动/85 baselined。

## 阶段 15（2026-09-15）

- 新增版本化诊断基线格式及 `--write-baseline`、`--baseline` CLI；项目配置支持相对
  `baseline` 路径，GUI 后台分析使用同一加载与过滤实现。
- 稳定指纹包含规则、规范化路径、消息、设计单元和信号，不含行列号；重复指纹采用计数消费。
  单元测试证明行号移动仍匹配、两个历史重复项不会隐藏第三个新增项，tab 转义可往返。
- 无效 header/转义/字段、缺失文件、参数缺值、同时读写基线以及报告与基线使用同一路径均
  返回 2。基线生成成功后固定返回 0，便于首次接入已有项目。
- text、JSON、SARIF、HTML 和 GUI 将 `baselinedViolations` 与源码/配置 suppression 分开
  统计；JSON schema 3 通过兼容新增字段扩展。
- CLI 生命周期验证：为 risky fixture 生成 3 条基线后，同一输入剩余 0 条活动问题并返回 0；
  加入新 RTL 后保留 5 条新增问题并返回 1。真实 CLAHE 的 85 条诊断生成 86 行版本化文件，
  回读后活动问题为 0、`baselinedViolations` 为 85，14 clocks/9 resets 结构保持不变。

## 阶段 14（2026-09-15）

- 新增独立 `timing_analyzer`，为每个 Verilog/SystemVerilog module 和 VHDL entity 提取
  时钟信号、上升/下降沿、复位信号、有效电平以及同步/异步属性，并保留来源位置。
- 异步 Verilog 复位以 sensitivity event 的边沿确定有效电平；同步复位与 VHDL 复位同时
  结合条件表达式和命名约定。测试使用不带 `_n` 的 `negedge reset`、`reset_async = '0'`
  证明并非只靠名称猜测。
- 多 architecture 的 VHDL 时序摘要合并到同一 entity；同一信号/边沿去重，异步属性优先于
  同步观察。双口 RAM 的两个独立时钟保持为两个事件。
- text 报告增加时序结构计数；JSON schema 3 以兼容新增字段输出 clocks/resets 及来源；HTML
  和 Qt 设计树展示时钟边沿、复位有效电平和 sync/async 属性。
- 真实 CLAHE RTL 提取出 14 个单元级时钟事件和 9 个异步低有效复位域；人工抽样确认 pclk、
  rst_n 及双口 RAM 的 clka/clkb、clk_a/clk_b 分类正确。原有 85 条诊断保持不变。

## 阶段 13（2026-09-15）

- 新增 `RTL-LATCH-INFERRED`（warning），为 Verilog/SystemVerilog 和 VHDL 组合过程构建
  “可能赋值/所有路径确定赋值”数据流，而不是仅通过文本搜索判断锁存风险。
- 控制流覆盖顺序语句、嵌套 begin/end、if/else、VHDL elsif、case 分支、default/
  when others 和可能零次执行的循环；过程开头默认赋值会正确覆盖后续不完整分支。
- `always_comb`、`always @(*)`、`always @*`、显式组合敏感列表和 VHDL process 均支持；
  时钟过程与表达明确意图的 `always_latch` 排除。SystemVerilog `unique/unique0/priority`
  if/case 修饰符不会阻断路径分析。
- fixture 验证缺失 else、空的 others 分支及缺失 default 会按具体信号报告；完整分支、默认
  赋值、时钟过程和 intentional latch 不报。
- 真实 CLAHE RTL 回归仍为 12 文件、12 单元、11 实例、5 顶层和 85 条诊断；组合状态机的
  默认赋值被正确识别，无新增锁存误报。

## 阶段 12（2026-09-15）

- 新增 `RTL-INCOMPLETE-SENSITIVITY`（warning），对传统 Verilog/SystemVerilog
  `always @(...)` 和 VHDL `process(...)` 比较显式敏感列表与过程体读取的已声明信号。
- 自动敏感列表 `@(*)`、`always_comb`、VHDL-2008 `process(all)` 以及包含 posedge/negedge、
  rising_edge/falling_edge/`event` 的时钟过程均明确排除；同一过程中被赋值的目标也不作为
  敏感列表依赖，避免循环变量和输出自引用造成噪声。
- fixture 覆盖两种 HDL 的真实遗漏、完整显式列表、自动列表和时钟过程；诊断定位到 always/
  process 起点并列出缺失信号。
- 真实 CLAHE RTL 的 12 文件回归无显式组合敏感列表命中，仍保持 12 单元、11 实例、5 顶层
  和 85 条诊断，证明新规则没有扰动既有项目结果。

## 阶段 11（2026-09-15）

- 语义分析器在已有驱动所有权基础上增加内部信号 read/write 使用追踪，新增
  `RTL-UNDRIVEN-SIGNAL`（warning）和 `RTL-UNUSED-SIGNAL`（info），覆盖 Verilog、
  SystemVerilog 与 VHDL。
- Verilog 声明初始化、`initial` 块和 VHDL 并发/过程赋值均计入“存在来源”；初始化来源与
  运行时驱动分开建模，不把一次初始化和一个 always 块误判为两个并行驱动。
- 实例端口方向在完整 elaboration 前未知，因此连接实际信号采用保守策略，避免把子模块输出
  误报为父模块内部/输出未驱动。单元测试覆盖未驱动、未使用、只写、实例连接及初始化所有权。
- 真实 CLAHE RTL 回归分析 12 文件、12 单元、11 实例、5 顶层：消除声明初始化与 `initial`
  memory 写入产生的 14 条未驱动误报，最终为 85 条诊断；新增 20 条均为未读取或只写信号，
  其余既有 65 条保持稳定，且无新增多驱动、输入驱动或输出未驱动问题。
- AppleClang 15 / C++20 严格警告参数下，新增分析测试和 CLI 已手工编译通过；GUI 仍受本机
  缺少 macOS Qt SDK 限制，保留 Windows x64 构建验收门槛。

## 阶段 10（2026-09-14）

- Qt MainWindow 从静态骨架升级为实际工作流：目录选择、自动加载项目配置、后台分析、层次
  树、可排序问题表、规则详情、源码行定位及 HTML/SARIF 原子导出。
- 采用 QtConcurrent::run + QFutureWatcher，耗时分析不在 GUI 线程执行；任务闭包不捕获窗口
  对象，完成后由 watcher 信号在 GUI 线程更新状态。
- GUI smoke 断言已更新为 4 个菜单、3 列层次树、6 列问题表、Open/Run 动作与初始禁用状态。
- 核心/CLI 的 Qt 独立构建与全部测试继续验证。当前本机 macOS Qt SDK 已清理，仅留 Windows
  MSVC Qt 文件，无法在本机可信编译或运行新 GUI；该项明确保留为 Windows x64 验收门槛。

## 阶段 9（2026-09-14）

- 新增共享 token 驱动的 RTL 语义分析器，为 Verilog/SV always 与 continuous assign、VHDL
  process 与 concurrent assignment 建立独立驱动源模型。
- 新增 `RTL-MULTIPLE-DRIVERS`、`RTL-INPUT-DRIVEN`、`RTL-UNDRIVEN-OUTPUT`；诊断携带
  designUnit 和 signal 上下文，JSON/SARIF 也保留该字段。
- 测试证明两个过程驱动标量会报 error、内部赋值 input 会报 error、无驱动 output 会报
  warning；同一过程的条件分支赋值只计一个驱动。Verilog/SV 与 VHDL fixture 均覆盖。
- 首轮真实项目试跑识别出两个双口 RAM memory 被不同端口过程写入；根据 unpacked memory
  声明模式加入 RAM 推断例外后，两条不适用 error 消除，标量/向量规则保持有效。
- 真实 CLAHE RTL 最终回归无新增 RTL 误报，既有 12 文件、12 单元、11 实例、5 顶层、
  65 条诊断保持稳定。

## 阶段 8（2026-09-14）

- 新增保持原始字节长度与行列位置的 Verilog/SystemVerilog 预处理层，支持 `define`、
  `undef`、`ifdef`、`ifndef`、`elsif`、`else`、`endif` 和 include 文件查找。
- inactive 分支不进入结构提取、结构语法检查或 HDL 规则；配置宏切换测试确认只出现所选
  module，隐藏分支中的 `casex` 不产生诊断。
- 新增 `PP-INCLUDE-NOT-FOUND`、`PP-UNEXPECTED-DIRECTIVE`、`PP-INVALID-DIRECTIVE` 和
  `PP-UNCLOSED-CONDITIONAL`，均提供文件、行、列及 error 严重级。
- `.fpga-lint.conf` 新增 `include-dir` 和 `define`；相对 include 目录以配置文件目录解析。
  CLI 支持 `-I dir`、`-Idir`、`-D NAME=value`、`-DNAME=value` 及对应长选项。
- 配置、分离及紧凑 CLI 形式均完成集成验证；真实 CLAHE RTL 未产生 PP/SYNTAX 新误报，
  12 文件、12 单元、11 实例和 65 条既有诊断保持稳定。

## 阶段 7（2026-09-14）

- 新增内建 `checkSyntax`，在规则执行前检查未终止块注释/字符串、`()`/`[]`/`{}` 配对，
  以及 module/begin/case/function/task/generate/interface/package/class/fork 结构边界。
- 语法错误统一进入 `Violation`、阈值、suppression 和全部报告格式，严重级别为 error，并
  定位到未闭合起点或意外闭合符号。
- 测试覆盖未闭合 module、case、括号、块注释、VHDL 字符串及意外 endmodule。
- malformed fixture 同时被本工具和本机 Icarus Verilog 判为错误；本工具稳定输出 2 条高
  置信度结构错误，Icarus 额外给出完整语法解析错误，符合当前“健全性检查而非完整语法树”边界。
- 真实 CLAHE RTL 回归未产生任何 SYNTAX 误报，既有 12 文件、12 单元、11 实例和 65 条
  诊断结果保持稳定。

## 阶段 6（2026-09-14）

- 新增 SARIF 2.1.0 + Errata 01 报告，包含 tool/driver、去重规则元数据、result level、
  physicalLocation、invocation 状态和分析统计；输出经 JSON 解析并逐项检查关键结构。
- 新增响应式单文件 HTML 报告，覆盖分析指标、问题明细、修改建议、设计单元和直接实例；
  不包含 JavaScript 或外部资源，HTML parser 验证通过。
- HTML 安全测试确认消息、说明、建议、文件路径及结构名称进行实体转义，不把源码内容解释
  为标签或脚本。
- CLI 新增 `sarif`、`html` 格式及 `-o/--output`，报告文件打开、写入或 flush 失败均返回 2。
- text、JSON schema 3、配置、suppression 和真实 CLAHE 项目回归保持兼容。

## 阶段 5（2026-09-14）

- 新增无第三方依赖的 `.fpga-lint.conf` 解析器，支持规则禁用、外部 IP black box、路径
  wildcard 排除和 CI fail-on 阈值；错误配置逐行给出文件与行号。
- CLI 支持显式配置及运行目录自动发现；命令行列表项追加到配置，fail-on 覆盖配置值。
- 新增 `disable-line`、`disable-next-line`、`disable-file` 三种注释内 suppression，支持特定
  规则或 `all`，并在文本/JSON 报告中统计被抑制诊断。
- JSON 协议升级为 schema 3，实例增加 external 标志，summary 增加 suppressedViolations。
- 配置集成验证确认生成目录被排除、vendor IP 被识别为 resolved external、未产生 unresolved
  噪声，且配置的 error 阈值使仅 warning/info 的分析正常返回 0。

## 阶段 4（2026-09-14）

- 新增注释/字符串感知 token 层和设计结构提取器，覆盖 Verilog/SV module、参数化实例、
  VHDL entity、direct entity/component 实例及精确来源位置。
- 项目结构测试覆盖跨单元解析、顶层标记、外部目标未解析、重复 module、递归例化环路，
  以及 VHDL 不区分大小写的 entity 目标解析。
- JSON 协议升级为 schema 2，增加设计单元、直接实例、resolved 和 topLevel 信息。
- 真实 CLAHE RTL 复测：12 个文件、12 个设计单元、11 个实例、5 个顶层；所有 11 个实例
  均在输入集中解析，没有新增未解析实例噪声。原有 65 条规则诊断保持稳定。

## 阶段 3（2026-09-14）

- AppleClang 15 / C++20 严格警告参数下，核心、CLI、数据库测试和新增分析/报告测试
  均完成手工编译并通过；本机当前没有可调用的 CMake，因此未伪称完成本轮 CTest。
- 分析测试覆盖大小写扩展名识别、Verilog/SV module 和 VHDL entity 统计、注释排除、
  时序/组合赋值类型、`casex`、缺失 default、VHDL 非标准数值包、wait、shared variable、
  行列定位，以及文本/JSON 报告。
- CLI 集成 fixture：递归分析 2 个文件、2 个设计单元、4 个 warning；默认阈值返回 1，
  `--fail-on none` 返回 0，并生成 `schemaVersion: 1` 的 JSON。
- 真实 CLAHE RTL 项目试跑：递归分析 12 个文件和 12 个设计单元，产生 65 条诊断。
  其中 64 条为 info 风格项；唯一 warning 定位到 `clahe_clipper_cdf.v:282:13` 的状态机
  case 缺失 default。人工核查至对应 `endcase`，确认该告警与源码一致。
- 首轮试跑将时序块中 `for (i = ...; i = i + 1)` 的循环控制变量识别为阻塞赋值；规则
  随后增加针对 for 控制语句的排除并复测，诊断从 66 条降至 65 条。

以下内容保留早期基础工程验证历史。

验证日期：2026-09-08。

## 已完成

- 本机：macOS / AppleClang 15，C++20，临时 CMake 4.4.3。
- 无 Qt 模式：CMake 配置、生成、编译成功。
- CTest：core_and_cli_unit、cli_help、cli_version 全部通过（3/3）。
- 核心/CLI 单元测试覆盖无参数、帮助别名、版本、未知参数、HDL 文件参数和多余参数；检查退出码、输出内容以及 stdout/stderr 路由。
- Qt 6.8.3 模式：CMake 配置、生成成功，GUI、CLI、单元测试及 GUI 冒烟程序全部编译成功。
- GUI 冒烟测试通过：窗口显示、事件循环、三个 Dock、菜单、工具栏、编辑器、空 Problems 和状态栏均满足检查；窗口正常关闭。与先前通过的三项测试合计 4/4 通过。
- GUI 测试首次在沙箱中因 Qt 的 NEON CPU 特性探测失败而退出；同一二进制在沙箱外复测通过，未修改应用代码来规避检测。
- CMake 成功读取 Windows VS2022 x64 配置、Debug/Release 构建与 Debug 测试预设。

## Windows 虚拟机后续验证

在 Parallels 虚拟机中完成了后续验证。来宾系统实际为 Windows 10 Insider Preview 10.0.21277 ARM64，并非产品要求的 Windows 10/11 x64。

已安装或部署到虚拟机：

- Visual Studio 2022 Build Tools：`C:\BuildTools2022`，包含 MSVC 19.44、MSBuild 17.14、Windows SDK 10.0.26100 和 CMake 3.31.6。
- Qt 6.8.3 MSVC 2022 x64：`C:\Qt\6.8.3\msvc2022_64`。
- Windows 构建目录：`C:\FPGAAnalyzerBuild`。

Visual Studio 2022 Community IDE 安装器拒绝安装。安装日志明确记录当前 ARM64 OS build 21277 低于要求的 build 22000；Visual Studio 2022 ARM64 IDE 需要 Windows 11。随后安装 Visual Studio 2022 Build Tools 成功，并用其 Visual Studio 17 2022 x64 生成器完成配置。

Windows 配置和编译结果：

- CMake 成功识别 MSVC 19.44、Windows SDK 10.0.26100 和 Qt 6.8.3。
- Debug 与 Release 均成功生成 `fpga_analyzer.exe`、`fpga_analyzer_cli.exe` 和测试程序。
- 从 Parallels 的大小写敏感共享盘 `Z:` 直接构建时，MSBuild 对转成小写的依赖路径发出增量构建警告；完整构建仍成功。正式 Windows 开发时应把源码放在本地 NTFS 路径中。
- Release 数据库测试可以运行并通过，证明基本 MSVC x64 程序能在兼容层运行。
- CLI、核心/CLI 测试、Qt 的 `windeployqt.exe` 与 GUI 冒烟测试在运行时稳定触发 `0xc0000005`；GUI 未部署时还会返回 `0xc0000135`。`windeployqt` 的 Windows Application Error 事件同样记录异常码 `0xc0000005` 和 unknown faulting module。
- 一个独立的单翻译单元 MSVC C++20 iostream 程序可正常运行，但调用跨翻译单元应用元数据接口的最小程序会触发同一异常。两种接口写法均复现，未把针对旧仿真层且无效的实验修改保留在交付代码中。

结论：阶段 1 已在 Windows 来宾中完成 CMake 配置和 MSVC x64 编译验证，但该虚拟机不能完成可信的 Windows x64 运行验证。原因是它是已不受 Visual Studio 2022 支持的 Windows 10 ARM64 预览版，且其旧 x64 仿真层会使本工程和 Qt 官方 x64 工具发生访问冲突。完成产品目标平台验收需要 Windows 10/11 x64 环境；若使用 Apple Silicon Parallels，则至少升级为受支持的 Windows 11 ARM64，再把结果明确标记为 ARM64 主机上的 x64 交叉构建/仿真验证。

## 本机复现命令

工具与 Qt 仅临时安装到 `/tmp`，不是项目交付依赖；清理临时目录后请使用自己的 CMake/Qt 安装。

```sh
/tmp/fpga-analyzer-tools/bin/cmake -S . -B build/qt-debug -DCMAKE_PREFIX_PATH=/tmp/fpga-analyzer-qt/6.8.3/macos -DCMAKE_BUILD_TYPE=Debug
/tmp/fpga-analyzer-tools/bin/cmake --build build/qt-debug -j 4
/tmp/fpga-analyzer-tools/bin/ctest --test-dir build/qt-debug --output-on-failure
```

本机产物是 macOS 可执行文件。Windows 虚拟机已经生成 x64 `.exe`，但由于上述来宾平台限制，不能把它们认定为通过运行验收的交付二进制。
