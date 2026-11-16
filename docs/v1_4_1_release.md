# FPGA HDL Static Analyzer V1.4.1

本版继续只修正规则逻辑，不调整界面。

## GJB9765.4.11.f.b/.c 修正

原生入口表明 f.b 直接检查 FSM 编码是否为顺序编码 `0`，f.c 直接检查是否为 onehot `1`。附件 GJB9765 与组合规则配置同时启用两条规则，原生运行入口没有读取面积/速度优先级。

移除产品此前额外增加、但没有原生证据支持的 `--fsm-priority` 检测门控。现在：

- onehot 违反 f.b；
- 顺序/默认编码违反 f.c；
- gray 同时违反 f.b 与 f.c；
- 无法证明 FSM 或编码的源码仍保持人工复核。

面积/速度选项为兼容已有工程配置继续保留，但不再改变这两个独立规则的判断结果。

恢复证据与边界见 [4.11.f.b/.c 状态机编码判据记录](evidence/native_vhdl_fsm_encoding_4_11_f_bc_findings.md)。

## 验证

- 新增 `syn_encoding` 无本地声明、有合法声明、错误声明、别名冲突、`safe, onehot`、`safe, sequential` 及同文件 entity/architecture 上下文边界测试。
- 附件审计：384 个 Automated 示例全部匹配；45 个 ManualReview 示例匹配、1 个保留已记录的文档/原生逻辑冲突；9 个 ProjectContext 示例匹配、3 个仍需工程上下文；4 个缺少被引用源文件的片段保持 context-required。
- 完整 CTest：135/135 通过。
- 用户 RTL 回归：34 个文件、203 个设计单元、34,383 条诊断、0 个分析错误，与上一版 GJB10157 扫描基线一致。
