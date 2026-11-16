# FPGA HDL Static Analyzer V1.4.6

本版继续只处理规则集，不调整界面。

## GJB9765.4.14.d.4 工程上下文门槛纠正

离线反编译已贯通本规则的入口、递归收集器和报告分支。原生实现递归查找大小写无关的
`file_open` 调用，只检查其直接字符串字面量子节点；字面量包含 `:/` 或 `:\` 时逐条报告
Warning。常量字节和中文诊断文本均已从 DLL 常量区复核。整个判据不读取目标文件，也不查询
工程层次关系，因此此前的 `ProjectContext` 分类过严。

本版将该规则登记为 `Automated`，并增加协议前缀、Windows 路径、Unix 根路径、冒号非斜杠、
注释、无关字符串和嵌套参数边界测试。无告警仍输出 `ManualReviewRequired`，不构成完整合规证明。
详细证据见 [FILE_OPEN 路径判据恢复记录](evidence/native_vhdl_file_open_path_4_14_d_4_findings.md)。

## 209 条规则状态

- 206 条：存在确定违规自动检测器；
- 3 条：确实需要层次、子模块端口定义或更完整工程时钟关系，保持 `ProjectContext`；
- 0 条：仅有 `ManualReview` 登记。

完整算法等价验证仍为 0 条；所有部分检测器的无告警结果继续保留人工复核状态。

附件审计共执行 446 个样例：Automated 匹配 435、保留 1 个已记录的 5.2.15 文档冲突；
ProjectContext 匹配 5、1 个因缺少子模块定义保持不匹配；另有 4 个缺失被引用源文件的片段明确
记为 `context-required`。
