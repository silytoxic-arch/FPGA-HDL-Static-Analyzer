# GJB9765.4.5.a 原生判据恢复记录

## 目标与样本

- 样本：`T-Linter.dll` V3.1.4.25120520251205115359
- SHA-256：`60e6b52875f4d7c3fbdf494d869b3a4bcc3fdc8ee63d8620ea9614da5d6599b0`
- 规则执行函数：`FUN_1805d9230`（`0x1805d9230`）
- 名称收集函数：`FUN_1805dcab0`（`0x1805dcab0`）
- 原生伪代码证据：`ev_faf4f1e6949f0529c0d54a039b2625f9fd1b347006560b3b9a6250d92b0d11e7`

## Recovered predicate

名称收集函数递归访问 VHDL AST，并把以下对象的名称加入精确名称表：

- `0x77`：类型；
- `0x78`：信号；
- `0x79`：变量；
- `0x0a`：常量；
- `0x9f`：子类型；
- `0xa4` 的直接子节点：端口；
- `0xa3` 的直接子节点：泛型。

实体和体系结构名称不属于本规则的短标识符范围。执行函数按 VHDL、Verilog、SystemVerilog、SDF/EDIF、PSL、Windows 的顺序检查六套独立词表；每套检查均先查询规则对象 `+0x148` 的类别布尔配置，再查询名称是否存在于对应词表，命中后输出该类别专用告警并结束当前名称的后续类别检查。

附件规则说明把 `4.5.a` 定义为 IEEE Std 1076-2008 关键字约束，但附件 `.lprp` 仅保存规则启用状态与严重度，没有保存 `+0x148` 的六个类别布尔值。产品因此自动执行规范明确要求的 VHDL 词表，不擅自启用其他语言词表，以免把合法 VHDL 名称误报为违规。

原生 VHDL 词表为：

`abs access after alias all and architecture array assert assume assume_guarantee attribute begin block body buffer bus case component configuration constant context cover default disconnect downto else elsif end entity exit fairness file for force function generate generic group guarded if impure in inertial inout is label library linkage literal loop map mod nand new next nor not null of on open or others out package parameter port postponed procedure process property protected pure range record register reject release rem report restrict restrict_guarantee return rol ror select sequence severity signal shared sla sll sra srl strong subtype then to transport type unaffected units until use variable vmode vprop vunit wait when while with xnor xor`

产品在查询前折叠标识符大小写，符合 VHDL 标识符与保留字不区分大小写的语义。

## 已恢复但未默认启用的词表证据

- SDF/EDIF 原生字符串位于 RVA `0xeed430`，共 486 字节；
- PSL 原生词表包含 `abort`、`always`、`before`、`next_event`、`onehot`、`rose`、`stable` 等；
- Windows 原生词表为 `aux con com1..com9 lpt1..lpt9 prn nul`；
- Verilog 和 SystemVerilog 分别构造独立的标准关键字集合。

这些分支证明原工具支持可选的跨语言名称策略，但不能证明附件配置默认开启它们。

## 产品映射和验证边界

产品对完整的词法声明与接口形态执行同一七类对象范围，并增加以下自动验证：

- 信号、变量、常量、类型、子类型、端口、泛型各自命中；
- 大写 `LABEL` 仍命中 VHDL `label`；
- `wire`、`always_ff`、`delayfile`、`abort`、`con` 不因未证实的可选词表而误报；
- 附件错误示例中的 `signal label` 命中，正确示例保持无告警。

扩展标识符、语法损坏后的厂商前端降级、外部类别配置界面、跨类别命中优先级的运行态复现及原生诊断去重细节仍未完全验证。因此本规则的无告警结果不声明与原生工具完全等价。
