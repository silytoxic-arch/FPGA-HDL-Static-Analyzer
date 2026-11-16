# FPGA HDL Static Analyzer V1.4.179

## A-14-1 原生判据深化

- 规则公开编号继续严格使用官方三段式 `A-14-1`。
- 恢复并接入原生状态关系键计数：精确排除 `init/default`，其余状态键去重后仅在严格大于40时报告。
- 支持等值数值 case 标签的前端值规范化，避免 `6'd1` 与 `6'b000001` 被错误计为两个状态。
- 严重级别保持原生 Warning，诊断恢复为 `状态机状态数不应超过40个,当前<数量>个`。
- 参数折叠、逗号标签完整降低、X/Z 通配、宏、generate、层次和跨文件绑定继续保持 `ManualReviewRequired`。

## 验证

- `A-14-1` 共7组定向正反例和边界断言通过；CTest `135/135` 全部通过。
- 附件审计覆盖544条安装规则、537个页面、209条 GJB 标准规则和446个示例：自动规则434个吻合、工程上下文规则6个吻合、4个明确需要上下文、2个原生实现与附件说明冲突，无未解释差异。
- 集成源码适配器由26条增至27条，剩余182条保持部分接入；完整原生等价仍为 `0/209`。
- 审计 JSON SHA-256：`41934dab188a4c9f7008d890f1c12f46f05f3879cc12b73667ff91a91ede3a74`。
- 原生规则台账 SHA-256：`e282f1e632d0b5d0fec258064967b704b81ab7c9e95040d009f5d4abee5d9f5c`。
- 发布包：`releases/1.4.179/FPGA-HDL-Static-Analyzer-1.4.179-Darwin-arm64.zip`，SHA-256 `e4050599a6fbaa9c09552d31a5ebb75c8b793ff0684f9c2eabadf9622c2d6d8a`。

详细证据见 [第四十三轮逆向记录](native_rule_recovery_pass43.md) 与 [A-14-1 原生证据](evidence/native_verilog_fsm_state_count_10_4_findings.md)。
