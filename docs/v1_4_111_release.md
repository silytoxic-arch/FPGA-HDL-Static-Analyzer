# V1.4.111 R-6-2 变量类别与完整 architecture 根

本版本只推进规则逆向、判断逻辑和验证，不调整界面或表格布局。对外规则编号继续严格采用官方三段式 `R-X-X` / `A-X-X`。

## 新确认的原生边界

- 附件类型表把 `0x56` 标为信号赋值节点，把 `0xa2` 明确标为 `AST_VARASGN`；`FUN_1805fe650` 的赋值分支只接纳 `0x56`，没有 `0xa2` 分支。
- `FUN_180600050` 则分别处理 `AST_SIGNAL(0x78)` 与 `AST_VARIABLE(0x79)`，把两者宽度写入不同名称表；宽度查询器 `FUN_1805fee80` 会读取两张表。
- 因此 variable 可作为六类关系节点和 `AST_BIT_AND/OR/XOR/XNOR` 的宽度来源，但 `:=` variable assignment 本身不属于该规则的原生赋值分支。
- `FUN_1805fe650` 对任意根递归收集关系与位运算，不限于 process；architecture 内 procedure/function 以及 process 局部声明都应参与根级宽度解析。
- 以上结论来自附件 DLL 的离线静态反编译，不代表运行了附件软件。

证据：`reverse-tools/evidence/r6_2_width_entry_collector_resolver.c`，SHA-256 `a7f1be023c877cc9906fbdd9a7eb6f96bb894321b3c10975f7702fea5985537f`；AST 类型表 `reverse-tools/evidence/a5_29_ast_enum_mapper.txt`，SHA-256 `78d56f0b3146c837c180974a5279ef43a718bc17c2507e45d1e7597d8b9bd7b0`。

## 产品修正

- 撤销 V1.4.110 对 package body `:=` variable assignment 的 `R-6-2` 报告；`vhdlAssignmentOperator` 继续只接纳 `<=`。
- 把根内形状收集器泛化为 architecture/package-body 共用实现，收集 signal、variable、vector constant 和 procedure/function 形式参数。
- architecture 的关系检查从仅遍历已识别 process 扩展为遍历完整 architecture 根，覆盖 architecture 子程序与 process 局部变量。
- architecture 的位运算、条件表达式和 `<=` 赋值宽度解析改用根内形状表；其他驱动、敏感表和结构规则仍使用原有语义信号表，避免扩大无关规则。
- 无法解析的类型、别名、子类型、重载及跨根可见性继续失败关闭；没有诊断仍为 `ManualReviewRequired`。

## 回归覆盖

- package body 中 8 位 variable `:=` 4 位转换：不得产生 `R-6-2`，并保持人工复核；
- package body 中 8 位与 4 位局部 variable 关系比较：报告一次；
- architecture process 中 8 位与 4 位局部 variable 关系比较：报告一次；
- architecture process 中 8 位与 4 位局部 variable 位运算：报告一次；
- architecture procedure 的 8 位输出 formal `<=` 4 位输入 formal：报告一次；
- architecture process 中 8 位 variable `:=` 4 位 variable：不得产生 `R-6-2`；
- V1.4.110 及此前全部规则测试继续执行。

## 仍未知

原生五张宽度名称表在同名跨类别对象间的完整优先级虽已观察到固定查询顺序，但 VHDL 前端对缺省形式参数类别、作用域修饰和重名嵌套子程序的名称生成仍未完全恢复。package specification 可见性、别名/子类型/重载、动态范围、生成域、错误恢复及原生诊断顺序仍未知。产品不以无报告宣称合规，完整厂商算法等价仍为 0/209。

## 发布验证

- 完整测试：135/135 通过。
- 附件审计：544 条配置/文档记录、537 个随包页面、209 条标准规则、446 个正反例；其中 435 个自动规则示例吻合、6 个工程上下文示例吻合、4 个示例明确保持上下文不足、1 个附件标注与 DLL 固定默认谓词冲突。
- 对外编号门禁：附件审计台账与原生实现台账均为 209/209 条官方三段式 `R-X-X` / `A-X-X`。
- 原生实现台账：209 条核心谓词均有接入记录，完整厂商算法等价仍为 0/209。
- 从全新解压目录运行 `--version` 返回 `1.4.111`；归档内应用深度签名校验通过；运行夹具确认 package-body `AST_VARASGN` 产生 0 条 `R-6-2`，architecture variable 关系产生 1 条官方编号 `R-6-2`，两者均无分析错误。
- 发布包路径：`releases/1.4.111/FPGA-HDL-Static-Analyzer-1.4.111-Darwin-arm64.zip`。SHA-256 在归档生成后记录于工作区发布说明，不写入归档内副本，以避免归档哈希自引用变化。
- 工作区发布归档 SHA-256：`6fad060b55727ae99c37a0042b5033f5a94bb65299d6927f5105e61ee404450e`。
