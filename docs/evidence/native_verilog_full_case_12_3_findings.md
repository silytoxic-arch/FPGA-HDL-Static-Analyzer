# GJB10157.12.3 Verilog full_case 属性规则恢复记录

## 目标与二进制身份

- 目标规则：`GJB10157.12.3`
- 附件版本：T-Linter V3.1.4.25120520251205115359
- `T-Linter.dll` SHA-256：`60e6b52875f4d7c3fbdf494d869b3a4bcc3fdc8ee63d8620ea9614da5d6599b0`
- 构造函数：`0x180020b10`
- slot 0：`FUN_1806eeba0`（`0x1806eeba0`）
- slot 1：`FUN_1806ef640`（`0x1806ef640`）
- 本记录来自静态反编译和 PE 字节检查，不代表观察了原软件运行。
- 核心函数证据：`reverse-tools/evidence/a16_1/native_core_functions.txt`，SHA-256 `7586a6687b654b8778aaa72f42b43e2835fb4c9d329012ccec2dd2efc52d9d`。
- 消息拼接辅助函数：`reverse-tools/evidence/a16_1/native_message_helpers.txt`，SHA-256 `a246ecbac3c7a4f1fd766bc03efb81c4bc56efff8566735557bd1054414dd90c`。
- 名称清理辅助函数：`reverse-tools/evidence/a16_1/native_name_helper.txt`，SHA-256 `aefec5928e6b1bd5c9573b39c2feafcec0f68a548b37fc012be40f4bb4d0d07c`。
- Verilog 词法器：`reverse-tools/evidence/a16_1/native_verilog_lexer.txt`，SHA-256 `029139d1fdf4ecc25c9c1fc73841093407b2a6c12f48c5ce9e8b92244ed1554b`。

## Recovered predicate

- slot 1 只在传入 AST 节点类型等于 `0x57` 时返回真；已恢复枚举关系及规则示例将该类型确定为 `AST_CASE`。
- slot 0 遍历每个准入 case 节点的属性表，通过全局字符串表还原属性名。
- 比较逻辑先取属性名长度与 10 的较小值做字节比较，随后再次要求长度恰好为 10，因此不是前缀或子串匹配。
- 目标地址 `0x180f12b10` 对应的十个字节是 `\full_case`。只有属性键与它完全相等时才产生告警。
- 附件违反例 `case (sel) //synopsys full_case` 表明前端把该 hot comment 规范化为 case 节点的 `\full_case` 属性；无属性的相同 case 为遵循例。
- 原生 Verilog 词法器 `FUN_180138530` 的动作 `case 0x81` 对 legacy `full_case` hot comment 输出一次 Yosys 警告，并返回 token `0x154`；相邻动作 `case 0x82` 是 `parallel_case`。附件实例闭合了 `//synopsys full_case` 到本规则 AST 属性的可观察路径。
- 报告器固定写入严重级别 `Warning`。消息字节片段为 `Synopsys full case用于模块`、`第`、`行.`；它先去除 case 节点名称开头的冒号，再拼接节点行号，得到 `Synopsys full case用于模块<名称>第<行号>行.`。

## 产品接入

- 在条件编译预处理后检查 `case`、`casex`、`casez`，非活动分支不会报警。
- 接受紧邻 case 之前、可连续出现的显式属性块；只检查每个 `attr_spec` 的键，不把属性值字符串误认为键。
- 显式属性键必须精确且区分大小写；属性值不参与 slot 0 的键比较。
- 接受附件证明的 case 选择表达式后同行 Synopsys/Yosys hot comment 路径；不再把属于另一条“工具特定编译指令”扫描规则的 `pragma/exemplar` 正则错误套用到本规则。
- hot-comment 词法动作在识别 `full_case` 前缀后进入专用状态，余下内容由该状态忽略；显式 AST 属性仍要求精确键。
- 恢复 Warning 和动态原生消息格式。
- 同时将产品规则说明中错误的 `fall_case` 修正为附件和二进制共同证明的 `full_case`。

## 已知边界

- 附件前端的完整 lexer DFA 模式、宏/include 后附着的属性、生成 AST 来源、节点名称构造、语法恢复路径及重复诊断尚未完整恢复。
- 当前适配器将 hot comment 限定在 case 头物理行，不对未证明的跨行写法作推断。
- 未检出仍保持 `ManualReviewRequired`，不宣称完整原生等价或自动合规。

## 回归覆盖

`tests/standard_accuracy_tests.cpp` 覆盖附件违反例、Warning 与原生动态消息、显式属性及连续属性块、属性值字符串隔离、精确属性键、hot-comment 前缀动作、选择表达式前错误位置、无证据生产者、`casez`、未附着注释、字符串内容以及非活动条件编译分支。

