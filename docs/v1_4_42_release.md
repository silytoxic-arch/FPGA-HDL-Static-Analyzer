# FPGA HDL Static Analyzer V1.4.42

本版继续只处理规则集，未调整任何表格或界面。

GJB10157.9.1.6 的原生标志生产者已经恢复。完整 DLL 中只有 `0x180958c8c` 会把 AST 节点字节 `+0x29b` 置为 1；该动作先确认 AST 指针向量非空，再标记尾节点。通过解析 Bison 动作跳转表、`yyr1/yyr2` 和 LR 状态表，目标动作精确对应规则 545：`behavioral_stmt -> non_opt_delay behavioral_stmt`。slot 0/2 对该字节的读取、克隆路径和唯一真实写入点形成了闭环。

同时确认附件内部存在值得保留的差异：附件错误实例要求拦截 `E = #5 A`、`E <= #5 D`，但这两类分别走赋值产生式 518/522，其动作块并不写 `+0x29b`。产品没有复制这一原生覆盖缺口：继续按附件实例拦截赋值内延时，并把连续赋值延时作为规则文本支持的产品扩展；台账会明确标识这些路径，不再把它们误写成已由原生标志证明。

本轮把确认的行为语句延时扩展到 task 体，因为 task 使用同一 `behavioral_stmt` 产生式，而既有 `verilogProcesses()` 只覆盖 always/initial。诊断详情现在区分行为语句、过程赋值、子程序和连续赋值延时。新增回归覆盖 task 前置延时、括号宏延时和纯事件控制负例；模块参数声明、实例参数覆盖继续保持无告警。

详细证据见 [native_verilog_delay_9_1_6_findings.md](evidence/native_verilog_delay_9_1_6_findings.md)。

## 验证结果

- 完整 CTest：135/135 通过。
- 附件规则总数：544；其中 GJB10157/GJB9765 标准规则：209。
- 附件示例探测：446；`Automated:matched` 435、已知 `Automated:mismatch` 1、`ProjectContext:matched` 4、`context-required` 6。
- 原生规则实现台账：209/209 条核心判据已接入；完整算法等价验证仍为 0，未把静态证据之外的未知分支标记为已验证。
- 已对 ZIP 解包后的 CLI 执行 `--version`，返回 `FPGA HDL Static Analyzer 1.4.42`；包内含 GUI、CLI、本版发布记录、9.1.6 证据与更新后的 209 条台账。
- 最终发布包 SHA-256 在打包完成后单独提供，避免把压缩包自身校验值写入包内形成循环依赖。
- 错误恢复、生成节点、宏展开后的原生位置和确切诊断次数继续保留为未知。
