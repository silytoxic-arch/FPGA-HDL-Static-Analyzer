# GJB9765.4.2.d 原生判断恢复

## 目标与身份

- 规则：`GJB9765_VHDL.4.2.d`，顶层宜只进行模块例化。
- 二进制：`T-Linter.dll`，SHA256 `60e6b52875f4d7c3fbdf494d869b3a4bcc3fdc8ee63d8620ea9614da5d6599b0`。
- 构造函数证据：`ev_e827820d401af751c4d921e0e8b0a69f8e02df96562782c1ccc5cec52f55aee0`。
- AST 枚举名称证据：`ev_512c915b90a165536016fafbb5a5ded6cfba13c0084ad97b3576b7906ee5547b`。

以下内容是 Ghidra 12.1.4 的反编译观察和有界推断，不是原始供应商源码。

## 恢复的核心判断

入口 `FUN_180782710` 先按配置名称解析顶层设计记录，再调用 `FUN_180783220`。该收集器只在节点类型为 `AST_ARCHITECTURE(0x75)` 时把其直接子节点加入全局候选容器，随后递归访问这些子节点；因此规则检查的是顶层架构的直接子项，而不是任意深度的所有节点。

对每个直接子项执行以下分支：

1. `AST_ASSIGN_CASE(0x8c)`、`AST_VARASGN(0xa2)`、`AST_ALWAYS(0x52)` 直接报告。
2. `AST_ASSIGN(0x4d)`、`AST_ASSIGN_EQ(0x55)`、`AST_ASSIGN_LE(0x56)` 调用递归辅助函数 `FUN_180782640`，命中后报告；报告位置取赋值节点本身。
3. `AST_INSTANCE_ENTITY(0x93)`、`AST_INSTANCE_COMPONENT(0x94)` 同样调用 `FUN_180782640`，命中后报告；报告位置取命中的运算符节点。

`FUN_180782640` 的命中集合由位掩码 `0xe00003dff` 和额外类型 `0x85/0x86/0xb6` 构成。结合枚举名称，精确集合为：

- `AST_BIT_NOT/BIT_AND/BIT_OR/BIT_XOR/BIT_XNOR`；
- `AST_REDUCE_AND/OR/XOR/XNOR`，但不含 `AST_REDUCE_BOOL(0x2d)`；
- `AST_SHIFT_LEFT/RIGHT/SLEFT/SRIGHT`，但不含 `AST_SHIFTX(0x32)`；
- `AST_ROL/ROR`；
- `AST_LOGIC_AND/OR/NOT`；
- `AST_WHEN`。

此集合不包含算术、比较、连接、标识符和常量节点。因此原生谓词允许顶层纯直连缓冲 `OUT4 <= IN1;`，对 `OUT1 <= IN1 or IN2;` 报告；顶层进程直接报告；例化端口映射中的直接信号连接允许，而包含命中运算符的端口实际表达式报告。

报告文本地址 `0x180f1dfe0` 的 44 字节按 GB18030 解码为“顶层只能包含模块实例，此级别禁止任何粘合逻辑”。

## 附件验证

- 违反例 `utest_GJB_4_2_d_ecode`：两个直接例化不报告；`OUT1 <= IN1 or IN2;` 和 `my_proc` 进程各报告一次。
- 正确例 `utest_GJB_4_2_d_ccode`：两个直接例化和 `OUT4 <= IN1;` 均不报告。
- 新增边界测试：位逻辑、`when`、选择赋值、复合端口实际表达式均报告；纯标识符直连、直连例化、算术赋值以及 generate 内的进程不会被提升为架构直接子项。
- 多根 VHDL 工程配置 `top-module TOP_B` 后只检查 `top_b` 的 architecture；名称按 VHDL 语义大小写不敏感解析。缺失或重复名称时失败关闭，不回退到自动根。

## 未决范围

完整 VHDL 前端降低、重载运算符、所有赋值形式、配置、别名、函数/过程体、宏预处理、错误恢复、重复设计记录、扩展标识符、位置顺序和诊断文本仍不等价于供应商原生产品。显式单一顶层已经接入；大小写不敏感解析是依据 VHDL 语言语义的产品适配，原生规范化函数名仍未知。当前实现只声明已恢复的直接子项谓词和有界源码扫描，不宣称完整前端等价。
