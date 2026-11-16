# FPGA HDL Static Analyzer V1.4.180

## R-16-2 原生判据深化

- 规则公开编号继续严格使用官方三段式 `R-16-2`。
- 原生严重级别恢复为 Warning，诊断恢复为不附加变量名的完整文本 `状态机未设置为safe模式`。
- 两个综合注释正则恢复不区分大小写语义，并保留第一条正则中可选 `syn_encoding` 组。
- `fsm_encoding` 属性值仍按原生字节串查找，只接受小写 `safe`；精确 `none` 和一位状态寄存器继续排除。
- 完整 FF/MUX/别名/常量传播网表识别、typedef/enum、参数位宽、宏、generate、层次、跨文件属性和工程级综合设置继续保持 `ManualReviewRequired`。

## 验证

- 定向测试覆盖一/二进程缺失 safe、三种常用 safe 形式、大写注释、可选组、属性大小写、onehot、none、一位状态寄存器、无关注释隔离及原生 Warning/消息。
- CTest `135/135` 全部通过。
- 附件审计覆盖544条安装规则、537个页面、209条 GJB 标准规则和446个示例：自动规则434个吻合、工程上下文规则6个吻合、4个明确需要上下文、2个原生实现与附件说明冲突，无未解释差异。
- 集成源码适配器由27条增至28条，剩余181条保持部分接入；完整原生等价仍为 `0/209`。
- 审计 JSON SHA-256：`dc9c9bd9270ccc560d3dbbe0baf27c1e0ec6774c84bcca02ce52ff087b3653d1`。
- 原生规则台账 SHA-256：`e4d72427a00ec57f721a0bf39debc0386074e917659f87aed3379b1018751c2e`。
- 发布包：`releases/1.4.180/FPGA-HDL-Static-Analyzer-1.4.180-Darwin-arm64.zip`，SHA-256 `62433773b76e46b8a09c59e2602e5144f67fb70c37b2662eff6bbd0856538132`。
- 解包后 CLI 返回 `FPGA HDL Static Analyzer 1.4.180`；`R-16-2` 运行时输出 Warning 和精确消息 `状态机未设置为safe模式`；应用通过 `codesign --verify --deep --strict`。

详细证据见 [第四十四轮逆向记录](native_rule_recovery_pass44.md) 与 [R-16-2 原生证据](evidence/native_verilog_fsm_safe_mode_12_2_findings.md)。
