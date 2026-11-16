# V1.4.109 R-6-2 同一设计根常量作用域

本版本只推进规则逆向、判断逻辑和验证，不调整界面或表格布局。对外规则编号继续严格采用官方三段式 `R-X-X` / `A-X-X`。

## 新恢复的原生边界

- `FUN_1805fd880` 对每个输入记录只取根向量首节点，先清空本规则的六组宽度/名称表，再调用 `FUN_180600050` 遍历整棵根，最后才调用 `FUN_1805fe650` 收集赋值、关系和位运算记录。
- `FUN_180600050` 特别处理 entity 的 port/generic、signal、variable 和 `AST_LOCALPARAM`，然后无条件按子节点向量递归。
- 因此，同一 architecture 根下 architecture、process、block、function/procedure 声明区中的可求值 VHDL constant 都会在比较记录生成前写入 `+0x1a8` 名称值表。
- 后遍历的同名 constant 会覆盖先前写入的 generic/constant。独立 package 或另一 architecture 属于另一输入根，入口会重新清表。
- 以上结论来自附件 DLL 的离线静态反编译，不代表运行了附件软件。

证据：`reverse-tools/evidence/r6_2_width_entry_collector_resolver.c`，SHA-256 `a7f1be023c877cc9906fbdd9a7eb6f96bb894321b3c10975f7702fea5985537f`；AST 类型表 `reverse-tools/evidence/a5_29_ast_enum_mapper.txt`，SHA-256 `78d56f0b3146c837c180974a5279ef43a718bc17c2507e45d1e7597d8b9bd7b0`。

## 产品修正

- `R-6-2` 命名宽度表从仅扫描 architecture 声明区扩展为扫描完整 architecture 根；
- process、block 和 architecture 内部 procedure/function 的可求值 constant 可用于转换函数宽度；
- 采用源码前序近似原生子节点向量的深度优先顺序，后出现的同名常量覆盖前值；
- 独立 package/architecture 根不会跨根传播；无法在当前 architecture 根证明的名称仍保持未知；
- 未求得宽度不产生 `R-6-2` 结论，标准状态继续保持 `ManualReviewRequired`。

## 回归覆盖

- process 声明区 `WIDTH := 4`：8 位目标必须报告；
- block 声明区 `WIDTH := 4`：8 位目标必须报告；
- architecture 内 procedure 声明区 `WIDTH := 4`：procedure 中赋值必须报告；
- generic `WIDTH := 4` 后出现 process-local `WIDTH := 8`：按整根写表顺序覆盖后不得报告；
- 独立 package 中 `WIDTH := 4`：不得泄漏到 architecture，且无报告仍为人工复核；
- V1.4.108 及此前全部规则测试继续执行。

## 仍未知

原生 AST 名称是否携带作用域修饰、异常/错误恢复树的实际子节点次序、package body 内子程序赋值、跨 design-unit 可见性、`to_integer` 的二子节点前端形态、32 位溢出和除法异常、别名/子类型/重载、生成域与原生诊断顺序仍未完全恢复。无报告不宣称完整厂商算法等价，完整算法等价仍为 0/209。

## 发布验证

- 完整测试：135/135 通过。
- 附件审计：544 条配置/文档记录、537 个随包页面、209 条标准规则、446 个正反例；其中 435 个自动规则示例吻合、6 个工程上下文示例吻合、4 个示例明确保持上下文不足、1 个附件标注与 DLL 固定默认谓词冲突。
- 对外编号门禁：附件审计台账与原生实现台账均为 209/209 条官方三段式 `R-X-X` / `A-X-X`。
- 原生实现台账：209 条核心谓词均有接入记录，完整厂商算法等价仍为 0/209。
- 从全新解压目录运行 `--version` 返回 `1.4.109`；process 声明区 `WIDTH := 4` 夹具输出官方编号 `R-6-2`，并识别 `to_unsigned(3,WIDTH)` 为 4 位；归档内应用深度签名校验通过。
- 发布包：`releases/1.4.109/FPGA-HDL-Static-Analyzer-1.4.109-Darwin-arm64.zip`，SHA-256 `2cfb6279600db82632cb53c543dc411db64e9b22d8d9f89d9c3fa00e9e482d57`。
