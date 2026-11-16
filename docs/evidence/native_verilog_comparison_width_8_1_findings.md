# GJB10157.8.1 比较操作数位宽：静态恢复记录

目标：用户附件 `T-Linter.dll`，SHA-256 `60e6b52875f4d7c3fbdf494d869b3a4bcc3fdc8ee63d8620ea9614da5d6599b0`。以下是 Ghidra 对原生二进制的静态伪代码观察，不是厂商源代码，也没有执行 Windows 程序。完整记录见 [REA 证据包](rea_verilog_comparison_width_8_1_bundle.json)。

## 已观察到的判据

- 类 `GJB10157_VLOG_8_1` 的 slot 0 指向 `0x180570040`。该过程在模块集合中先从 `AST_ALWAYS(0x52)` 和 `AST_ASSIGN(0x4d)` 根递归筛选，再对子树收集八类比较节点：`AST_LT/LE/GE/GT/EQ/NE/EQX/NEX`。类型名由节点枚举名称函数交叉确认。证据 `ev_4b56231ed19a0a496798bd8721aae35b6e61063b9d5ff29c517a8328ea93f004`、`ev_64f1059ebe9591553a1ca8ce04e68a11ec13f174085f30430d2355624bf9696d`、`ev_512c915b90a165536016fafbb5a5ded6cfba13c0084ad97b3576b7906ee5547b`。
- 比较节点恰有两个子节点时，分别调用位宽检测入口 `0x1808f4650`。两侧返回位宽不相等才写入 Warning。该入口以 `-1` 初始化宽度并转调通用宽度计算函数；不能仅由它推出所有参数、宏或生成逻辑的最终位宽。证据 `ev_4b56231ed19a0a496798bd8721aae35b6e61063b9d5ff29c517a8328ea93f004`、`ev_9e78a36c6bd5d2da1500b37922980ffe87fb0858db5ab22fd036a378923f6eab`、`ev_b1d82b5ee521f5171cc14875eaf2c3f9447d01ade51e599d9288dffde4504178`。
- 指令级复核确认两次调用前的 `0x180570698`、`0x1805706b6` 均把第五参数栈槽 `[RSP+0x20]` 写为 0，调用位于 `0x1805706ad`、`0x1805706cc`。因此本规则使用通用 helper 的 flag-zero 路径：嵌套 ADD/SUB/MUL/DIV/MOD 取子项最大宽度，POW 与单目正负号传播第一个子项宽度，CONCAT 求和，REPLICATE 乘静态次数。完整指令范围和摘要保存在 `reverse-tools/evidence/t_linter_pass4/gjb10157_8_1_asm.txt`、`gjb10157_8_1_flag_zero.md`。

## 本软件接入边界

`src/parser/semantic_analyzer.cpp` 对模块作用域中可静态确定的 `parameter/localparam` 默认值及受限的非负加减表达式求值，再推导 Verilog 声明区间。现按 flag-zero 路径递归推导静态标识符/选择、显式定宽字面量、拼接/复制、括号、单目正负号及 ADD/SUB/MUL/DIV/MOD/POW 的比较操作数宽度；尤其比较中的嵌套 MUL 取最大子宽度，不能借用 8.7 最外层乘法的求和公式。

扫描范围同步补齐到原生证明的 `AST_ALWAYS` 与连续 `AST_ASSIGN` 根：除 if/while/for 和条件表达式外，比较结果直接作为连续或过程赋值 RHS 也会检查；声明初始化不会被提升成该根。附件的 `P_BUS2_SIZE=16` 对 `P_BUS1_SIZE=32` 两处比较均能报告，而相同宽度不报。独立正反例、嵌套表达式、赋值根、1 bit 边界和未知参数测试见 `tests/standard_accuracy_tests.cpp`；通用交叉回归见 `tests/analyzer_tests.cpp`。

其他 helper AST 运算符、函数调用、动态选择、AST 转换阶段、实例参数覆盖、完整宏/生成展开和精确诊断顺序/重数仍未恢复。未知位宽不出本条诊断，但标准评估仍为 `ManualReviewRequired`，绝不作为合规证明。
