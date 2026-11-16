# FPGA HDL Static Analyzer V1.4.182

## A-1-7 原生判据深化

- 公开编号严格使用官方三段式 `A-1-7`。
- 恢复 `inputport`、`inoutport`、`outputport` 三类原生记录生产者及统一 `0xb8` 字节记录布局。
- 保持同一文件、同一物理行全部端口记录对的 `N*(N-1)/2` 诊断重数，包括同文件跨模块记录。
- 恢复原生 `Warning` 严重度及固定正文“一行中存在多个端口声明”，不再附加无原生依据的端口名称。
- 宏展开位置、畸形恢复、转义标识符和异常重复声明顺序继续保持 `ManualReviewRequired`。

## 验证

- 新增并调整三类方向、非 ANSI 声明、固定消息、Warning 级别及诊断重数断言；CTest `135/135` 全部通过。
- 附件审计覆盖544条安装规则、537个页面、209条 GJB 标准规则和446个示例：自动规则434个吻合、工程上下文规则6个吻合、4个明确需要上下文、2个原生实现与附件说明冲突，无未解释差异。
- 集成源码适配器由29条增至30条，剩余179条保持部分接入；完整原生等价仍为 `0/209`。
- 审计 JSON SHA-256：`4a1b050df54f4580afaf721cfcc553381331d682f934555e2928e962b867bd5d`。
- 原生规则台账 SHA-256：`ec96df317102aa9fa28bf71ff8a091326f9a2acc9b09d7e45445879c69101fe6`。
- 发布包：`releases/1.4.182/FPGA-HDL-Static-Analyzer-1.4.182-Darwin-arm64.zip`，SHA-256 `778a7d82c5bea08a39501cd1d4c7146785dd24c6abc4d35b74c7ef8f2604f252`。
- 解包后 CLI 返回 `FPGA HDL Static Analyzer 1.4.182`；三端口运行探针输出官方编号 `A-1-7`、3条 Warning 和固定正文“一行中存在多个端口声明”；应用版本为 `1.4.182`，并通过 `codesign --verify --deep --strict`。

详细证据见 [第四十六轮逆向记录](native_rule_recovery_pass46.md) 与 [A-1-7 原生证据](evidence/native_verilog_port_line_5_1_7_findings.md)。
