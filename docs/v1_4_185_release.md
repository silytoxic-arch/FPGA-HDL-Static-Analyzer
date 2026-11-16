# FPGA HDL Static Analyzer V1.4.185

## A-1-5 原生判据深化

- 公开编号严格使用官方三段式 `A-1-5`。
- 恢复时钟类别 2/3、复位类别 0/1、三种端口方向和固定分组顺序。
- 恢复重复端口名导致整模块跳过，以及每模块只报告首个错位的行为。
- 恢复 Error 严重度、物理行、第 1 列及固定正文。
- 多时钟/多复位的专有关系生产顺序、宏、层次和 interface/modport 路径继续保持 `ManualReviewRequired`。

## 验证

- 新增固定诊断、重复名称和同行定位断言；CTest `135/135` 全部通过。
- 附件审计覆盖544条安装规则、537个页面、209条 GJB 标准规则和446个示例：自动规则434个吻合、工程上下文规则6个吻合、4个明确需要上下文、2个原生实现与附件说明冲突，无未解释差异。
- 集成源码适配器由32条增至33条，剩余176条保持部分接入；完整原生等价仍为 `0/209`。
- 审计 JSON SHA-256：`48456b22ba25a531b7fd8bf806566cf578bdc33269d42be9d57a799bff73491c`。
- 原生规则台账 SHA-256：`a13b29b1d91a31e2e0e8c4149d474c969251be36cfeed13e1848a40492597f2d`。
- 发布包：`releases/1.4.185/FPGA-HDL-Static-Analyzer-1.4.185-Darwin-arm64.zip`，SHA-256 `9c2e4e5dcb60e0ab60530c8fc13def6a51f18b4e11199d672574fd202c62fbaf`。
- 解包后 CLI 返回 `FPGA HDL Static Analyzer 1.4.185`；运行探针输出官方编号 `A-1-5`、Error、第3行第1列及固定正文；应用短版本和构建版本均为 `1.4.185`，并通过 `codesign --verify --deep --strict`。

详细证据见 [第四十九轮逆向记录](native_rule_recovery_pass49.md) 与 [A-1-5 原生证据](evidence/native_verilog_port_order_5_1_5_findings.md)。
