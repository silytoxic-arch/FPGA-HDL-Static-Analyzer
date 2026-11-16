# GJB 10157 `R-11-2` 原生判断逻辑恢复记录

## 目标

- 公开编号：`R-11-2`
- 内部条款键：`GJB10157.9.2.2`
- 附件类名：`GJB10157_VLOG_9_2_2`
- DLL SHA-256：`60e6b52875f4d7c3fbdf494d869b3a4bcc3fdc8ee63d8620ea9614da5d6599b0`

## 已观察事实

构造函数 `FUN_180023c20` 安装虚表 `0x180ef4cd8`。候选选择器 `FUN_180581b90` 和执行函数 `FUN_180581600` 均重复验证以下条件：

1. 当前节点是 `AST_CASE(0x57)`；
2. 节点字节 `+0x183 == 0`；
3. CASE 至少存在一个直接子节点；
4. 第一个直接子节点为 `AST_CONSTANT(0x10)` 或 `AST_REALVALUE(0x11)`。

匹配后，每个 CASE 根输出一条，文件和行号取自 CASE 节点本身；分支标签与分支体不参与判断。原生固定消息为“禁止在case语句的控制表达式中使用固定值.”，原生内部严重级为 Warning。产品标准配置中的 `R-11-2` 属于强制规则，因此公开结果按配置提升为 Error。

## 产品修正

旧实现已经支持常量、参数和 localparam selector，但仅凭闭合括号即可输出；对于缺少 `endcase` 的不完整源码，会产生原生 AST 不可能进入的告警。本轮新增完整 CASE 门槛：

- 完整 `case/casex/casez` 常量 selector 各输出一条；
- 完整空 case 仍输出，因为 selector 是 CASE 的第一个子节点；
- 缺少 `endcase` 时不输出，并保持 `ManualReviewRequired`；
- 嵌套常量 CASE 按两个 CASE 根输出两条；
- 消息替换为 DLL 中恢复出的固定文本。

新增边界测试覆盖空 case、不完整 case、casex/casez、嵌套结构、精确消息、参数/localparam 和条件预处理。

## 仍未知

规则运行相对 Yosys 简化和常量折叠的精确阶段、`+0x183` 字段名与全部生产路径、参数覆盖、包/类、常量函数、完整宏/include、生成与错误恢复节点、精确列号和遍历顺序仍未闭合。未知表达式不判合规，完整原生等价未成立。

核心地址和判断摘录见 `native_verilog_constant_case_selector_9_2_2_core.txt`。
