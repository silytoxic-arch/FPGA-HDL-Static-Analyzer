# GJB9765.4.5.f.30 VHDL 复位信号命名规则恢复记录

## 目标与二进制身份

- 目标规则：`GJB9765.4.5.f.30`
- 附件版本：T-Linter V3.1.4.25120520251205115359
- `T-Linter.dll` SHA-256：`60e6b52875f4d7c3fbdf494d869b3a4bcc3fdc8ee63d8620ea9614da5d6599b0`
- 构造函数：`FUN_1805f4f60`（`0x1805f4f60`）
- slot 0：`FUN_1805f5080`（`0x1805f5080`）
- 本记录来自静态反编译和 PE 数据检查，不代表观察了原软件运行。

## Recovered predicate

- 主函数建立不区分大小写的 VHDL 信号名称集合，并区分“复位相关名称”和其他名称。
- `FUN_1805f6790` 只遍历 `AST_ARCHITECTURE`（`0x75`）的直接 `AST_ASSIGN_LE`（`0x56`）子节点；进程、子程序、block 和 generate 内的赋值不进入该关系集合。左侧必须无子节点且名称不含点号；右侧只接受无子节点的直接 `AST_IDENTIFIER`（`0x13`），或恰含一个标识符的 `AST_BIT_NOT`（`0x24`）。`0x24` 已由独立枚举函数与 A-5-30 入口交叉确认，不再只是附件形态推断。
- `FUN_1805f5080` 按规则对象 `+0x78` 的设计根逐项执行；每个非空根都会先清空 `+0x158` 名称树，并在根内重新初始化关系容器，再以同一根调用 `FUN_1805f6790` 和 `FUN_1805f6590`。因此同一物理文件中的多个设计根不会共享复位种子或赋值关系图。
- `FUN_1804ca4c0` 在一个当前根内同时收集 `AST_PORT(0xa4)` 的直接类型 6 端口声明，以及 mode=1 时 `AST_ARCHITECTURE(0x75)` 的直接类型 `0x78` 声明；两者都按小写名称连同原声明指针写入同一树。函数没有源路径过滤，因此绑定根可以跨物理文件组合 entity 端口与 architecture 声明。
- 插入调用点把关系保存为有向的“赋值目标 → RHS 来源”二元组。`FUN_1805f6590` 从每个语义复位种子出发，只沿该方向递归，因此复位属性向上游来源传播，而不会从 RHS 复位源反向扩散到下游赋值目标。
- `FUN_1804d6e80` 的 mode 0、1、2 分别为前缀、子串、后缀比较，并支持大小写折叠。本规则在两个告警分支分别使用：
  - 复位相关名称不以前缀 `rst` 开始时报警；
  - 非复位名称包含子串 `rst` 时报警。
- DLL 诊断片段包括“复位信号”“命名宜以”“起始”“非复位信号”“命名中不宜包含”。附件规则文字及正反例确定产品应实现的规则词为 `rst`；该值不是 DLL 构造函数中已恢复的原生默认值。

## 配置链边界

- 基类构造函数 `FUN_18062a230` 将规则对象 `+0xc0`、`+0xe0`、`+0x100`、`+0x120` 的四个字符串初始化为空；本规则构造函数 `FUN_1805f4f60` 不写入 `+0xc0`。
- 导出的 `Yosys::CheckRule::setRuleNameConvention` 经 `FUN_180628e70` 将调用方提供的四个字符串分别写入上述字段；这是当前恢复到的 `+0xc0` 配置入口。
- 随附 `runCheck.exe` 将内部编号 `VLR-200055` 映射到本规则，但其配置分发函数 `FUN_140042500` 没有对 `VLR-200055` 调用 `setRuleNameConvention`。四处同类调用均属于其他被显式比较的规则编号。
- 随附 `.lprp` 对本规则只保存启用状态和严重级，未提供命名词；其他随附 PE 也未发现第二条已绑定的配置入口。
- 因此，“比较模式和字段来源”已经恢复，而“随附客户端运行时实际绑定的词”仍未知。产品采用的 `rst` 来自附件规则文字和正反例，不能宣称为原生对象默认值，也不能据此宣称与随附客户端运行完全等价。

## 产品接入

- 候选范围为同源 VHDL 实体端口和显式 `signal` 声明，名称比较不区分大小写。
- 复位种子来自结构模型已确认的复位，以及规范异步形态 `if RESET = literal then ... elsif rising_edge/falling_edge(CLK) then ...`。
- 单实体源码内，只对 architecture 直接完整并发 `dst <= src;` 和 `dst <= not src;` 建立 `dst → src` 关系；从已识别复位目标向来源追溯。顺序、子程序、block 和 generate 赋值不建边。
- 传播后的复位名称要求以 `rst` 开始；其余候选名称不得包含 `rst`。
- 多实体同文件按 `architecture ... of entity` 分成独立设计根；每个根分别收集实体端口、architecture 直接 signal、复位种子和目标到来源关系，并独立执行非复位 `rst` 子串检查。
- entity 与 architecture 分处不同文件时，工程内实体名唯一即可组成同一源码适配根；端口告警保留 entity 文件位置。若同名 entity 不唯一则不按文件顺序猜测绑定，规则保持人工复核。

## 已知边界

- 规则对象 `+0xc0` 的通用写入机制已经恢复，但随附客户端没有为本规则绑定值；未检查的外部客户端或运行时写入仍可能覆盖该字段。产品按附件文字实现 `rst` 规则词，并明确把原生绑定值保留为未知。
- 原生复位种子构造、表达式/调用/端口/层级别名、多 architecture 配置选择、库限定、重复库单元绑定及精确告警次数仍未完全证明；`0x24`、赋值方向、architecture 直接子节点范围、逐设计根容器生命周期，以及同根端口/architecture 联合收集已经恢复。
- 同步复位、复合复位条件和无法绑定的工程关系不猜测；未报警仍保持 `ManualReviewRequired`，不宣称完整原生等价。

## 回归覆盖

`tests/standard_accuracy_tests.cpp` 覆盖附件形态的违反链和遵循链、直接复位、多级 `not` 上游传播、禁止向下游反向传播、嵌套 block 赋值排除、多设计根同文件的上游传播与隔离、唯一 entity 的跨文件绑定、重名 entity 的失败关闭、跨文件诊断位置、非复位名称包含 `rst`、相似但不含 `rst` 的名称，以及规则状态转换。

新增证据：

- `reverse-tools/evidence/a5_31_graph_helpers.txt`，SHA-256 `ed6ef91052bd9730a2845330a7e732515d8c85ede25def55e44089886130d66a`
- `reverse-tools/evidence/a5_31_graph_container_helpers.txt`，SHA-256 `0932dd62921111f1e121469c5d7cd3be341d44957f8d62edf3eaed20e6f20765`
- `reverse-tools/evidence/a5_31_graph_insert_disassembly.txt`，SHA-256 `bd9900a644f69139ac4946e028ac71061a9dcbb424284790f8c1a60de8e640cd`
- `reverse-tools/evidence/a5_31_per_design_root_scope.txt`，SHA-256 `78c5566febd23b4e26ca896b07f16c3d3639df2d6d51ef5c4dc9d1676e1d9d79`
- `reverse-tools/evidence/a5_31_root_port_architecture_collector.txt`，SHA-256 `b93fab8ab2b02b0830c806f64ed0cef59ea0effe4c4d12d499d28b3eef9b67fc`

