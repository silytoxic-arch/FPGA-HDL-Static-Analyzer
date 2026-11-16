# GJB10157.9.2.11 组合过程 if/else 判据（V1.4.40 校准）

## 证据边界

- 目标：附件 `T-Linter.dll`，SHA-256 `60e6b52875f4d7c3fbdf494d869b3a4bcc3fdc8ee63d8620ea9614da5d6599b0`。
- 本记录只使用静态反编译、静态反汇编和附件随包示例，没有执行厂商 DLL。
- 原生主判定函数：`FUN_180580740`，证据 `ev_bb7fc67da6a764411eeddbfe03fc0357342ce0db1d93094802847cb0a8f8e33e`。
- 虚表 slot 2：`FUN_180581410`，证据 `ev_e2b67ac7b02f48f4b34bc3782eb523f6980bd6e8ef31a443ab6aeb9eb6a5fbcb`；其辅助器反汇编见 `native_helper_1804e66e0_disassembly.txt`。
- AST 枚举名：`FUN_1808ea310`，证据 `ev_512c915b90a165536016fafbb5a5ded6cfba13c0084ad97b3576b7906ee5547b`。

## 已观察的原生判据

1. slot 2 先调用 `FUN_1804e66e0`。V1.4.40 的定点反汇编恢复了完整辅助判据：根必须是 `AST_ALWAYS(0x52)`；初始结果为真；只遍历根的直接子节点；遇到 `AST_POSEDGE(0x65)` 或 `AST_NEGEDGE(0x66)` 即返回假；空 always 或所有直接子节点均非这两类时返回真。`AST_EDGE(0x67)` 不在排除范围。辅助器返回假时，slot 2 仍额外接纳 `AST_TASK(3)` 和 `AST_FUNCTION(4)`。
2. `FUN_180580740` 先把已接纳的 TASK/FUNCTION 定义按节点名称写入查找表。处理 `AST_ALWAYS(0x52)` 时，它收集其后代 `AST_FCALL(0x1c)` 和 `AST_TCALL(0x4c)`，按调用名称查找定义，把命中的定义节点加入集合，并从查找表删除该名称。因此同一个定义至多展开一次。
3. 主函数对 always 根调用回调 `0x1805813e0`，随后对已解析的每个直接调用定义再调用同一回调。没有观察到在被调定义中重新收集 FCALL/TCALL 的路径，所以恢复为一层展开，不递归猜测间接调用。
4. 静态反汇编在 `0x1805813e0` 观察到：节点类型必须为 `0x57`，节点字节 `+0x183` 必须等于 1，且子节点数必须非零。它们对应被前端标记的 source if/else 降低 CASE；普通 source case 不进入本条。关键指令为 `cmp [rcx+4],0x57`、`cmp byte ptr [rcx+0x183],1` 和对子节点区间非空的检查。
5. 对每个收集节点，主函数遍历直接子节点；只要存在 `AST_COND(0x58)`，且该 COND 非空并以 `AST_DEFAULT(0x5b)` 开头，即认为存在 else。否则在该条件节点位置发出一条 `Warning`。

## 产品映射

V1.4.40 按恢复后的辅助器处理条件预处理源码：`always @*`、无 `posedge/negedge` 的 `always @(...)`、无事件控制的 bare `always`、`always_comb`、`always_latch`，以及未被两个边沿类型排除的命名 `AST_EDGE`。`always_ff` 也按其实际事件子节点判定，含直接 posedge/negedge 时排除。它按 HDL 语句边界逐个检查 source if，不把普通 case 缺 default、`initial` 或带上述边沿的时序 always 误标成本条。

本版新增同文件 TASK/FUNCTION 索引。组合 always 中直接出现的有括号函数/任务调用以及无括号任务调用，会按名称展开其定义体；同一名称只展开一次，未调用定义、仅从 `initial`/时序过程调用的定义和二级间接调用不会被推断为已覆盖。

## 回归覆盖

- 附件违规例中的两个组合 if 分别报告，遵循例保持零报告。
- 完整 else、默认赋值后无 else、普通 case、同步 always 和未激活条件编译分支。
- 组合 always 直接调用的 task、直接调用的 function、未调用 task、重复调用去重、二级 task 调用边界，以及仅由 initial 调用。
- bare always、always_latch、带 posedge 的 always_ff，以及不属于 POSEDGE/NEGEDGE 的命名事件。
- 完整 CTest 135/135 通过；附件 446 个示例审计保持 435 个自动匹配、1 个已知差异、4 个工程上下文匹配及 6 个缺少上下文。

## 仍未知

- `+0x183` 字段在附件所用 Yosys 分支中的正式成员名，以及生成/简化节点对它的传播方式。
- 宏或复杂事件表达式从源码到 AST 直接子节点的完整降低，以及错误恢复事件节点。
- 跨文件、层次化或作用域重名的 TASK/FUNCTION 解析，递归与间接调用，宏生成调用和错误恢复 AST。
- 原生消息正文与异常语法下的精确诊断数量。

因此本条仍是证据支持的核心判据与有界一层调用展开；无告警结果继续为 `ManualReviewRequired`，不宣称完整原生等价。
