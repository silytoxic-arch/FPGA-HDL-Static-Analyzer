# FPGA HDL Static Analyzer V1.4.184

## A-1-4 原生判据深化

- 公开编号严格使用官方三段式 `A-1-4`。
- 恢复唯一模块根门控、同文件多模块重复扫描、重复同名模块跳过和无模块源码排除。
- 恢复完整物理行、Windows ANSI 字节长度及达到 80 即报告的边界。
- 恢复 Error 严重度、第 1 列和固定正文“建议每行字符数小于80个”。
- 非 Windows 非 ASCII、非法编码、虚拟/生成源码归属及完整专有前端路径继续保持 `ManualReviewRequired`。

## 验证

- 新增 79/80/81 字节边界、尾随空格、模块映射准入和诊断重复数量断言；CTest `135/135` 全部通过。
- 附件审计覆盖544条安装规则、537个页面、209条 GJB 标准规则和446个示例：自动规则434个吻合、工程上下文规则6个吻合、4个明确需要上下文、2个原生实现与附件说明冲突，无未解释差异。
- 集成源码适配器由31条增至32条，剩余177条保持部分接入；完整原生等价仍为 `0/209`。
- 审计 JSON SHA-256：`b66128e0041a8eb6852a642e3415cf7753464f16c29db5f8e13a1bec1dcdf346`。
- 原生规则台账 SHA-256：`4cb0e220209fdb7cfa895d726f694ac69f7094ca654fb77e601b9e2ea5bd23e1`。
- 发布包：`releases/1.4.184/FPGA-HDL-Static-Analyzer-1.4.184-Darwin-arm64.zip`，SHA-256 `79da8651539015ba565eacad3e128cd6c31c967bf57de26d9b72a53610eec145`。
- 解包后 CLI 返回 `FPGA HDL Static Analyzer 1.4.184`；运行探针输出两条官方编号 `A-1-4`、Error、第2行第1列及固定正文“建议每行字符数小于80个”；应用短版本和构建版本均为 `1.4.184`，并通过 `codesign --verify --deep --strict`。

详细证据见 [第四十八轮逆向记录](native_rule_recovery_pass48.md) 与 [A-1-4 原生证据](evidence/native_verilog_line_length_5_1_4_findings.md)。
