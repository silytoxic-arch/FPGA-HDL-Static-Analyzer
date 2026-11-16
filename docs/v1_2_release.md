# 1.2.0 最终交付版（2026-10-10）

本发布整合最新 GJB 规则实现、中文高级简约桌面界面与 HDL 代码行数统计。
GUI 可选规则集严格只有 GJB10157-2021（Verilog）和 GJB9765-2020（VHDL）。

| 规则集 | 条款 | 自动检测 | 工程上下文 | 人工复核 |
|---|---:|---:|---:|---:|
| GJB10157-2021 | 103 | 38 | 6 | 59 |
| GJB9765-2020 | 106 | 39 | 5 | 62 |

209 条均可追踪，不等于 209 条均能仅凭 HDL 自动判定，不是标准符合性认证。
正反例目前是可查询的判据，不冒充完整标准示例代码测试集。
CLI 保留原有无标准模式兼容性；执行标准检查时应显式指定 `--standard`。

## 使用

桌面端打开工程目录，在“规则集”中选择标准后运行分析。切换标准自动重新分析；
分析期间锁定选择框，避免所选标准与结果不一致。可查看问题、源码、设计层次和代码行数，
并导出 HTML、SARIF 或统计 CSV。逐条评估状态可在 JSON 报告中审查。

```sh
fpga_analyzer_cli --version
fpga_analyzer_cli --list-standards
fpga_analyzer_cli --standard gjb10157 --format json --fail-on none ./rtl
fpga_analyzer_cli --standard gjb9765 --format json --fail-on none ./rtl
fpga_analyzer_cli --describe-rule GJB10157.5.2.16
```

macOS arm64 包包含独立 GUI、CLI 与 Qt 运行依赖。应用为 ad-hoc 签名，未公证。
Windows 10/11 x64 需使用 Visual Studio 2022 和 Qt MSVC x64 在 Windows 原生编译，
见 [Windows 构建说明](windows_build.md)及源码包的 `scripts/package_windows.ps1`。
当前主机未发现已注册 Windows 虚拟机；不把 macOS 二进制当作 Windows 成品。

标准实现边界见 [实施矩阵](gjb_rule_matrix.md)与 [交叉依据](gjb_reverse_engineering.md)。
