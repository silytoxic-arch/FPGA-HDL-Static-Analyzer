# FPGA HDL Static Analyzer V1.4.181

## A-16-1 原生判据深化

- 公开编号严格使用官方三段式 `A-16-1`。
- 恢复 AST_CASE 上精确十字节属性键 `\full_case`，显式属性按键而非全文子串解析。
- 恢复 legacy full_case 词法动作、Warning 级别及 `Synopsys full case用于模块<名称>第<行号>行.` 消息格式。
- 移除属性值字符串、选择表达式前注释和无直接证据 `pragma/exemplar` 生产者导致的误报。
- 宏/include、完整 lexer DFA、生成 AST 名称和语法恢复路径继续保持 `ManualReviewRequired`。

## 验证

- 新增并调整定向正反例及边界断言；CTest `135/135` 全部通过。
- 附件审计覆盖544条安装规则、537个页面、209条 GJB 标准规则和446个示例：自动规则434个吻合、工程上下文规则6个吻合、4个明确需要上下文、2个原生实现与附件说明冲突，无未解释差异。
- 集成源码适配器由28条增至29条，剩余180条保持部分接入；完整原生等价仍为 `0/209`。
- 审计 JSON SHA-256：`c7ef6169839ab619437cd559c745da773e05c15b408f0d650fc1d75fc8cfd8cd`。
- 原生规则台账 SHA-256：`6c21428cc36377acc70ac84993902e17ac757270b6493e110ba490a5f4de8285`。
- 发布包：`releases/1.4.181/FPGA-HDL-Static-Analyzer-1.4.181-Darwin-arm64.zip`，SHA-256 `a4c458126ecd0e47170afb7834908839700ce958fef3c93183911e11579dfa9d`。
- 解包后 CLI 返回 `FPGA HDL Static Analyzer 1.4.181`；运行时 `A-16-1` 输出 Warning 和动态消息 `Synopsys full case用于模块probe第3行.`；应用通过 `codesign --verify --deep --strict`。

详细证据见 [第四十五轮逆向记录](native_rule_recovery_pass45.md) 与 [A-16-1 原生证据](evidence/native_verilog_full_case_12_3_findings.md)。
