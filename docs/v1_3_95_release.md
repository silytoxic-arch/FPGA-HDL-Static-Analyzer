# FPGA HDL Static Analyzer V1.3.95

本版接入 GJB10157.6.2.5 上电初始化的证据支持核心判据。

## 检测与配置

明确指定非 SRAM 器件后，检查无歧义单边沿直线过程中的寄存器是否缺少可接纳的上电赋值。已恢复前端对 DLL initial 接纳开关的设置路径，支持显式选择接纳、忽略或未指定策略。类型或策略未知的路径继续复核。

CLI 示例：

```sh
fpga_analyzer_cli --standard gjb10157 --fpga-technology non-sram --initial-blocks accept --format json rtl
```

工程配置文件可使用：

```text
standard gjb10157
fpga-technology non-sram
initial-blocks accept
```

命令行参数可覆盖配置文件。JSON 报告新增 `configuration.powerOnPolicy`，记录器件类型和 initial 策略；未指定策略输出 null。GUI 在没有明确器件上下文时保持人工复核。

## 恢复与边界

原生 DLL 检测入口、初始化目标收集、selector 写入 API，以及 `runCheck.exe` 将配置字段空/非空转换为 0/1 的机器指令均有证据记录，见 [6.2.5 恢复记录](evidence/native_verilog_power_initialization_6_2_5_findings.md)。

复杂复位/default 路径、声明初始化、固定 RHS 解析、函数、分片和层次绑定仍未覆盖；无告警不自动判合规。

当前 205/209 条规则已有原生核心谓词部分接入；剩余 GJB10157.5.1.7、5.2.8、6.2.10 和 GJB9765.4.2.e 四条继续恢复。完整算法等价尚未宣称完成。

## 验证

- 完整 CTest：134/134 通过，包括 3 项新增 CLI 端到端检查、规则正反例/边界及配置校验。
- 附件 446 个示例：435 个匹配、11 个既有差异，无回退；6.2.5 没有随包正反例，不据此宣称厂商运行等价。
