# GJB10157 9.4.4（R-13-4）原生判断恢复记录

## 边界与目标身份

- 目标：附件 `T-Linter.dll`，SHA-256 `60e6b52875f4d7c3fbdf494d869b3a4bcc3fdc8ee63d8620ea9614da5d6599b0`。
- 方法：只读 Ghidra 12.1.4 静态反编译和附件 HTML 示例检查；未执行 Windows 目标。
- 构造函数：`0x1800246f0`，写入规则名 `GJB10157_VLOG.9.4.4` 并安装 vtable `0x180ef6490`。
- 执行函数：`0x18058d850`；模块根筛选槽：`0x1807238a0`。
- 本文中的地址、AST 类型和伪代码均为静态分析观察，不是原始源码或运行时轨迹。

## 已观察的原生谓词

1. 槽函数 `0x1807238a0` 只在节点 `+4 == 2` 时返回真；这与同一附件中的 `AST_MODULE(2)` 类型表一致。
2. 对每个模块根，执行函数递归收集三组后代：`FUNCTION(4)`、`TASK(3)` 和 `WIRE(6)`。
3. 只有字节字段 `WIRE + 0xa2` 非零的节点名称会进入全局候选集合。
4. 附件解析器中的多条写入路径会设置 `+0xa2`。函数 `0x1800ece10` 还在 `+0xa2 != 0 && input && !output` 时输出英文错误 `Input port '%s' is declared as register.`。因此该字段的“寄存器声明”语义已有直接代码佐证；原始 C++ 成员名仍未知。
5. 原始候选集合被复制为两份：
   - 所有 function 的每个直接子节点名称都会从 function 候选集合删除；
   - 所有 task 的每个直接子节点名称都会从 task 候选集合删除。
   这两个删除集合彼此独立，但同类子程序之间是累计删除，而不是逐个作用域重新计算。
6. 每个 function/task 内部都会递归收集 `AST_ASSIGN_EQ(0x55)`；没有收集 `AST_ASSIGN_LE(0x56)`。
7. 对每个 `AST_ASSIGN_EQ`，原生代码取其第一个子节点的名称，与对应的剩余候选名称逐一比较。每个匹配赋值独立报告，因此同一信号的两次阻塞赋值产生两条告警。
8. 文件路径和行号来自赋值第一个子节点的 `+0x1e0`、`+0x210` 字段，而不是赋值根节点。
9. 两条报告路径的严重级均为 `Warning`。静态字符串和拼接顺序给出精确文案：
   - function：`在function语句中，禁止对全局信号` + 规范化名称 + `进行赋值`
   - task：`在task语句中，禁止对全局信号` + 规范化名称 + `进行赋值`
10. 名称辅助函数 `0x18058ec50` 会对普通 Yosys 转义标识符移除首个反斜杠；以 `$`、反斜杠或数字开始的特殊内部名称走其他保留/复制分支。

## 附件示例交叉验证

附件 `examples_html/GJB10157_VLOG.9.4.4.html`（SHA-256 `faaff47ab63c8e34af6400c6da5b647dc6bfa6018be4a8c51638f725da0010f4`）的错误例在 function 内对模块级 `reg my_g_sig` 阻塞赋值两次；正确例只写 function 本地 `reg my_ng_sig`。这与上述候选、直接子节点删除和逐赋值报告路径一致。

## 产品接入

- `src/analysis/verilog_global_assignment.h` 复现独立的 function/task 候选集合和同类子程序累计直接子节点排除。
- 明确纳入普通模块 `reg`、带初始化的模块 `reg` 和 ANSI `output reg`；保留非阻塞赋值排除。
- 标识符位选仍保留第一个子节点名称，因此 `global_sig[0] = ...` 会按原生名称比较路径报告。
- `src/analysis/standard_evaluator.cpp` 按 function/task 输出原生文案，并把定位放在左值标识符。
- `src/analysis/standard_rule_inventory.inc` 将该规则严重级从 `Error` 修正为原生 `Warning`。
- 正反例和边界回归覆盖：逐赋值重数、task 路径、本地遮蔽、同类累计遮蔽、function/task 集合隔离、跨模块隔离、非阻塞排除、位选、初始化声明和 ANSI 输出寄存器。

## 证据文件

- `18058d850.c`：SHA-256 `ea795f827cedd80f9b307b1b992a3221b89a9fe99cb62ceb23ecf28d564ef184`
- `1807238a0.c`：SHA-256 `9f7f82468df0cf30d498dc6e925b41ff504e046211854d71980e9e23fb3b00a4`
- `18058ec50.c`：SHA-256 `85f74292ce87c14894b58c166af19bfd7badffa4f0e663c6d91402598f509e33`
- `1800ece10.c`：SHA-256 `48d8d9dbfdd8567eda262a6fba3843b2964e661d55196a970e455c70d7c7cedb`
- 完整 `.text` `+0xa2` 读写扫描 `field_a2_all.txt`：SHA-256 `56a1a34de84ec5ec43379b46e2471bdc585f16746985fd9ff4541c066e3982b0`

上述文件位于本地 `reverse-tools/evidence/remaining_rules/`，是可复核的静态反编译产物，不是厂商源码。

## 仍未知或未声称等价的部分

- `+0xa2` 原始 C++ 成员名、所有语法产生式和所有复制/重写路径仍未恢复为厂商源码级定义。
- SystemVerilog `logic`、全局 `integer`、存储器、转义/内部名称的全部 AST 降低形态尚未逐项证明，因此产品没有把这些形式猜测为已覆盖。
- 宏、include、generate、畸形语法恢复、复杂拼接/层次左值、完整源位置所有权和跨异常 AST 的原生报告顺序仍未完全验证。
- 本次完成的是证据支持的核心谓词与有界源码适配器，不声称完整厂商前端等价；无告警结果仍按现有策略保留人工复核状态。
