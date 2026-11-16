# FPGA HDL Static Analyzer V1.4.5

本版继续只处理规则集，不调整界面。

## GJB10157.5.2.15 配置链补证

离线反编译确认该规则的专用大小写控制位由 DLL 构造函数固定为 `0/1/1`，且未发现第二个直接写入点；
通用 `setRuleNameCase` 修改的是另一组字段，附件 `.lprp` 也只为本规则保存启用状态和严重级。
因此随包 HTML 把 `parameter Data` 标成正确代码，属于文档与二进制默认判据冲突，而不是未恢复的配置覆盖。

现有实现已经按 DLL 判据检查：

- parameter/localparam 名称含任一 ASCII 小写字母时报告；
- wire 类名称首字节必须是 ASCII 字母，第二字节以后不得出现 ASCII 大写字母；
- function/task 子树、内存数组、宏分支和不透明声明不据此推断合规。

本版将该确定违规检测器从 `ManualReview` 正式登记为 `Automated`，但未报警仍输出
`ManualReviewRequired`，不构成自动合规认证。详细证据见
[5.2.15 配置链补证](evidence/native_verilog_naming_case_5_2_15_configuration_findings.md)。

## 209 条规则状态

- 205 条：存在确定违规自动检测器；
- 4 条：需要时钟域、层次、外部文件或子模块端口定义，保持 `ProjectContext`；
- 0 条：仅有 `ManualReview` 登记。

附件审计保留 5.2.15 的一个文档冲突，不通过改写预期消除差异。
