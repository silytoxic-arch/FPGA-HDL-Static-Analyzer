# V1.4.113 R-6-3 CASE 根级宽度名称表

本版本只推进规则逆向、判断逻辑和验证，不调整界面或表格布局。对外规则编号继续严格采用官方三段式 `R-X-X` / `A-X-X`。

## 原生证据闭环

- `FUN_180600da0` 对每个保存的设计根先清空六组宽度/名称表，再由 `FUN_180600050` 递归收集完整根，最后调用 `FUN_180601490` 检查 CASE。
- CASE selector 只接受直接 `AST_IDENTIFIER(0x13)`；choice 只接受直接名称，或由 `0x0e` 包装的直接常量节点。两侧宽度都已知且不等才报告。
- 共享查询器 `FUN_1805fee80` 的五类宽度表固定顺序为 `constant > port > generic > signal > variable`，因此 `R-6-3` 与 `R-6-2` 应使用相同名称解析语义。
- 这些结论来自附件 `T-Linter.dll` 的离线静态反编译，不代表运行了附件软件。

证据：`docs/evidence/native_vhdl_case_width_4_6_b_2_findings.md`；共享反编译证据 `reverse-tools/evidence/r6_2_width_entry_collector_resolver.c`，SHA-256 `a7f1be023c877cc9906fbdd9a7eb6f96bb894321b3c10975f7702fea5985537f`。

## 产品修正

- architecture 的 CASE 宽度检查改用 V1.4.112 已恢复的根级五类宽度视图，不再使用 architecture 声明区专用常量表。
- 接入所属 entity 的固定宽度 vector generic，以及 architecture/process/subprogram 根内的 constant、signal、variable 和形式参数。
- package body 的子程序 CASE 现在除 `when others` 外也执行 `R-6-3` 直接名称宽度比较；每个 package body 与 architecture 仍独立建表。
- selector 切片/索引、复杂 choice、未知宽度、别名/子类型/重载仍失败关闭，不猜测违规；无诊断保持 `ManualReviewRequired`。

## 新增验证

- 8 位所属 entity vector generic selector 对 4 位命名 constant：报告一次。
- 8 位 process variable selector 对 4 位局部 constant：报告一次。
- package-body procedure 中 8 位 variable selector 对 4 位 constant：报告一次。
- 8 位 port 与同名 4 位 process variable selector：port 优先，不报告。
- 4 位 process constant 与同名 8 位 port selector、8 位 choice：constant 优先，报告一次。
- 此前直接 selector、切片/索引排除、命名常量、字符串字面量与附件正反例继续执行。

## 仍未知

case 状态字节的全部前端生产语义、缺省形式参数的专有节点类别、名称作用域修饰、同名嵌套子程序、package specification 可见性、别名/子类型/枚举/qualified expression/aggregate/重载、生成域、错误恢复及原生诊断顺序仍未知。产品不以无报告宣称合规，完整厂商算法等价仍为 0/209。

## 发布验证

- 完整测试：135/135 通过。
- 附件审计：544 条配置/文档记录、537 个随包页面、209 条标准规则、446 个正反例；其中 435 个自动规则示例吻合、6 个工程上下文示例吻合、4 个示例明确保持上下文不足、1 个附件标注与 DLL 固定默认谓词冲突。
- 原生实现台账：209/209 条均为官方三段式 `R-X-X` / `A-X-X`，209 条核心谓词均有接入记录，完整厂商算法等价仍为 0/209。
- 从全新解压目录运行 `--version` 返回 `1.4.113`；归档内应用深度签名校验通过。
- 全新解压运行夹具：package-body CASE 宽度冲突产生 1 条官方编号 `R-6-3`；port/variable 重名 selector 产生 0 条 `R-6-3` 并保持 `ManualReviewRequired`；两者均无分析错误。
- 发布包路径：`releases/1.4.113/FPGA-HDL-Static-Analyzer-1.4.113-Darwin-arm64.zip`。SHA-256 在最终归档生成后记录于工作区发布说明，不写入归档内副本，以避免归档哈希自引用变化。
- 工作区发布归档 SHA-256：`c6a4fe5dc3b0de26d68420b495bc1a03cdbbc0694a00bcbd8c9e2891374c05dd`。
