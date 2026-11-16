# V1.4.110 R-6-2 package body 独立设计根

> 更正：后续对附件类型表与收集器逐分支复核确认，V1.4.110 错把独立的 `AST_VARASGN(0xa2)` 纳入了只接纳 `AST_ASSIGN_LE(0x56)` 的原生赋值分支。该版本不再推荐使用；V1.4.111 已撤销 `:=` 报告，并保留 variable 作为关系/位运算操作数的原生宽度来源。

本版本只推进规则逆向、判断逻辑和验证，不调整界面或表格布局。对外规则编号继续严格采用官方三段式 `R-X-X` / `A-X-X`。

## 新恢复的原生边界

- `FUN_1805fd880` 对每个输入记录取根向量首节点，先清空六组名称/宽度表，再调用 `FUN_180600050` 递归准备完整设计根，最后调用 `FUN_1805fe650` 收集二元记录；入口没有 architecture 类型过滤。
- `FUN_180600050` 处理 signal、variable、generic 和 `AST_LOCALPARAM` 后仍无条件递归所有子节点；`FUN_1805fe650` 同样递归整根并收集 `AST_ASSIGN_LE(0x56)`、六类关系节点及 `AST_BIT_AND/OR/XOR/XNOR(0x25..0x28)`。
- 因此，package body 被输入为独立设计根时，其中 procedure/function 的形式参数、局部变量、局部常量、赋值、比较和位运算都参与本规则；进入下一 package/architecture 根前名称表重新清空。
- 以上结论来自附件 DLL 的离线静态反编译，不代表运行了附件软件。

证据：`reverse-tools/evidence/r6_2_width_entry_collector_resolver.c`，SHA-256 `a7f1be023c877cc9906fbdd9a7eb6f96bb894321b3c10975f7702fea5985537f`；常量求值器 `reverse-tools/evidence/r6_2_constant_evaluator.c`，SHA-256 `4bc000795b51fc2a0ab9807e01c1216970a50277bcbae41835a5fb3efd3b6181`；AST 类型表 `reverse-tools/evidence/a5_29_ast_enum_mapper.txt`，SHA-256 `78d56f0b3146c837c180974a5279ef43a718bc17c2507e45d1e7597d8b9bd7b0`。

## 产品修正

- 复用完整、命名结束和裸结束的 package body 范围识别，避免吞并后续设计根；
- 为每个 package body 单独建立根内形状表，接入子程序形式参数、局部 signal/variable/vector constant；
- 根内 integer constant 使用已恢复的常量求值器，支持前序名称、括号、单目正负和 `+ - * /`；
- V1.4.110 曾检查 package body 子程序中的 signal `<=` 与 variable `:=`；其中 `:=` 后经复核确认超出原生收集器边界，已由 V1.4.111 撤销。六类关系比较和四类位运算继续保留；
- package body、package specification 与 architecture 不共享名称/常量状态；未知类型、别名、子类型及跨根可见性不猜测，未报告时继续保持 `ManualReviewRequired`。

## 回归覆盖

- package body procedure 中 8 位目标与 `to_unsigned(..., 4)`：报告一次；
- 同一形态使用宽度 8：不报告，但不宣称完全合规；
- 8 位局部 variable 使用 `:= to_unsigned(..., 4)`：V1.4.110 曾报告一次，V1.4.111 起按 `AST_VARASGN` 排除；
- 8 位与 4 位形式参数执行 `and`：报告一次；
- 8 位与 4 位形式参数执行关系比较：报告一次；
- package body 的 `WIDTH := 4` 与后续 architecture 的 `WIDTH := 8`：仅 package body 报告，证明根状态隔离；
- V1.4.109 及此前全部规则测试继续执行。

## 仍未知

原生名称是否携带作用域修饰、多个同名嵌套子程序的覆盖顺序、package specification 到 body 的语义可见性、别名/子类型/重载、生成域、异常/错误恢复树、`to_integer` 的二子节点形态、32 位异常边界与原生诊断顺序仍未完全恢复。产品只对直接可证明形态产生诊断；无报告不宣称完整厂商算法等价，完整算法等价仍为 0/209。

## 发布验证

- 完整测试：135/135 通过。
- 附件审计：544 条配置/文档记录、537 个随包页面、209 条标准规则、446 个正反例；其中 435 个自动规则示例吻合、6 个工程上下文示例吻合、4 个示例明确保持上下文不足、1 个附件标注与 DLL 固定默认谓词冲突。
- 对外编号门禁：附件审计台账与原生实现台账均为 209/209 条官方三段式 `R-X-X` / `A-X-X`。
- 原生实现台账：209 条核心谓词均有接入记录，完整厂商算法等价仍为 0/209。
- 从全新解压目录运行 `--version` 返回 `1.4.110`；package body 夹具输出官方编号 `R-6-2`，识别 `to_unsigned(3,WIDTH)` 为 4 位且无分析错误；归档内应用深度签名校验通过。
- 发布包：`releases/1.4.110/FPGA-HDL-Static-Analyzer-1.4.110-Darwin-arm64.zip`，SHA-256 `33165d41cc5f9c10fed59c2124d05346e0ae7005ebd72d504031acfbc8c7029f`。
