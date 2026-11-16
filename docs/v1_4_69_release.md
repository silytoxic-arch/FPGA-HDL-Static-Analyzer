# V1.4.69 R-13-4 原生判断深化说明

## 原生恢复

- 对附件 `T-Linter.dll`（SHA-256 `60e6b52875f4d7c3fbdf494d869b3a4bcc3fdc8ee63d8620ea9614da5d6599b0`）静态恢复 `R-13-4` 的构造函数、模块根筛选器、执行函数和名称格式化辅助器。
- 原生执行函数收集 `FUNCTION(4)`、`TASK(3)`、`WIRE(6)`；只把 `WIRE +0xa2 != 0` 的名称作为全局候选。解析器错误路径直接把 `+0xa2` 用作“declared as register”判断，闭合了此前未知的字段语义。
- function 与 task 分别持有候选集合；所有同类子程序的直接子节点名称会累计删除候选。两类集合彼此独立。
- 只递归收集阻塞赋值 `AST_ASSIGN_EQ(0x55)`，不收集非阻塞赋值 `AST_ASSIGN_LE(0x56)`；按赋值第一个子节点名称比较并逐节点报告。
- 文件与行号取自左值首子节点；严重级为 `Warning`。
- 原生诊断分别为 `在function语句中，禁止对全局信号<名称>进行赋值` 和 `在task语句中，禁止对全局信号<名称>进行赋值`。

## 产品修正

- `verilog_global_assignment.h` 改为 function/task 独立候选集和同类累计遮蔽模型，不再按每个子程序单独恢复候选。
- 模块候选增加初始化 `reg` 与 ANSI `output reg`；标识符位选保持首子节点名称并纳入判断。
- 非阻塞赋值、模块外写入、跨模块名称、本地返回名和同类直接子节点遮蔽继续排除。
- 规则严重级由 `Error` 修正为 `Warning`，诊断按原生 function/task 路径分别输出。
- 公开规则编号继续使用官方三段式 `R-13-4`；完整条款号仅用于内部追溯。

## 验证范围

- 回归覆盖 function 两次写入、task 写入、精确文案和 Warning 级别。
- 边界覆盖 function/task 集合隔离、同类累计遮蔽、局部初始化遮蔽、跨模块隔离、非阻塞排除、位选、初始化模块 reg、ANSI 输出寄存器。
- 完整静态证据、观察/推断/未知边界见 `docs/evidence/native_verilog_global_assignment_9_4_4_findings.md`。
- 完整厂商前端等价仍未声明；SystemVerilog logic、integer/memory、宏/include/generate、复杂左值和异常 AST 仍保持人工复核边界。
- 未修改任何表格或界面布局。
