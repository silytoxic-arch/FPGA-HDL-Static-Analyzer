# FPGA HDL Static Analyzer V1.3.77

本版继续只处理规则逆向、判断逻辑与验证，未修改表格或界面布局。

## GJB9765.4.10.e

已恢复并接入“应使用复位操作对寄存器进行初始化”的证据支持核心判断。产品对解析器确认的 VHDL 时序进程建立全部直接寄存器目标集合和复位分支覆盖集合，只对两者差集逐个报告。

异步复位、同步复位以及同进程混合覆盖均按目标区分；无时钟关系的组合过程不报告。复杂目标或无法识别的复位控制树继续为 `ManualReviewRequired`。详见 [恢复记录](evidence/native_vhdl_register_reset_initialization_4_10_e_findings.md)。

## 覆盖与验证

- 209 条规则中，187 条已接入证据支持的核心判断；22 条仍待解释。完整原生算法等价数仍为 0。
- 附件 446 个示例复测；4.10.e 正反例均吻合。ManualReview 示例总体由 38 个吻合提升到 39 个，差异由 10 个降至 9 个。
- Release 构建的 131 项 CTest 全部通过，包含 5 项新增的 4.10.e 边界回归：未复位、异步复位、同步复位、同进程部分覆盖和组合进程排除。
- 附件审计详见 `evidence/t_linter_after_1_3_77.json` 与同名 Markdown 摘要；逐规则状态见 `evidence/native_rule_implementation_ledger.json`。
- 本次提供经签名校验和 ZIP 完整性校验的 macOS arm64 包及 SHA-256；完整原生等价仍未宣称。
- Windows ZIP 仍需在 Windows 构建环境中生成和验证。
