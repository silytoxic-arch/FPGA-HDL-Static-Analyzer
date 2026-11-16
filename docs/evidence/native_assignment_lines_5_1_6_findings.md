# A-1-6 原生判据恢复记录

公开规则编号为官方三段式 `A-1-6`。`GJB10157.5.1.6` 只作为产品内部条款键和证据关联键，不在问题列表中代替官方编号。

## 目标与入口

- 目标：附件 `T-Linter.dll`
- SHA-256：`60e6b52875f4d7c3fbdf494d869b3a4bcc3fdc8ee63d8620ea9614da5d6599b0`
- 构造函数：`FUN_180020e20`
- 原生别名：`VLR-110018`
- 虚表：`Yosys::GJB10157_VLOG_5_1_6::vftable`，活动入口 `FUN_1806de620`，候选过滤槽 `FUN_1806df2f0`

原始证据：

- `reverse-tools/evidence/a1_6/native_core_functions.txt`，SHA-256 `c21687992434bfb72707f58930d9df0a0b5ff15db55e7a339a2db167463870cb`
- `reverse-tools/evidence/a1_6/native_grouping_helpers.txt`，SHA-256 `ecf52b365efe0912a68ca03e4a8f03b9ff1a5a11a969bbc4ee65c6de9056cc57`
- `reverse-tools/evidence/a1_6/native_record_dedup_helpers.txt`，SHA-256 `2df71fc59643d7c59210d95cb01386103a8adffbe3f65b90c287f3ac301a7f7f`
- `reverse-tools/evidence/a1_6/native_vtable_and_strings.txt`，SHA-256 `cce57b0d8ece353c53f3610a36d8d155abd6a96d8af34d9769889374955a645a`

## FUN_1806df2f0 候选类型

过滤函数对 `type - 0x4d` 使用位掩码 `0x88301`，准入五种类型：

- `0x4d`：`AST_ASSIGN`，连续赋值；
- `0x55`：`AST_ASSIGN_EQ`，阻塞赋值；
- `0x56`：`AST_ASSIGN_LE`，非阻塞赋值；
- `0x5c`：`AST_FOR`；
- `0x60`：`AST_GENFOR`。

`0x5c/0x60` 的名称由独立 FOR/GENFOR 证据交叉确认，不再把 `0x60` 记为未知类型。

## FUN_1806de620 核心判据

1. 从规则对象 `+0xa8` 的映射中复制候选向量。
2. 对每个 `AST_FOR/AST_GENFOR` 根检查直接子节点；只收集直接的 `AST_ASSIGN/AST_ASSIGN_EQ/AST_ASSIGN_LE`。
3. 从一般候选向量中移除所有 FOR/GENFOR 根及上述直接赋值子节点。因此裸循环体赋值被排除，位于 `BLOCK/IF` 等包装节点之下的赋值并非直接子节点，仍可参与配对。
4. 对剩余候选执行所有 `i < j` 配对。仅当两节点物理行 `+0x210` 相同、较早节点有子节点且其首子节点类型为 `AST_IDENTIFIER(0x13)` 时生成记录。
5. 记录包含较早节点的源路径 `+0x1e0` 和物理行。`FUN_1806de100`、`FUN_1806de440`、`FUN_1806df310` 按“源路径+行”做相邻等值压缩，因此同一文件同一行出现三条以上赋值时仍只报告一次。

这证明原生判据不是物理行分号计数，也不是简单的每行赋值数量统计；较早候选的左值 AST 形态和 FOR/GENFOR 直接父子关系都会改变结果。

## 固定诊断

- 严重度：`Warning`（原生字符串“警告”）
- 正文：`一行中不止一个信号赋值语句`
- 位置：记录中的物理行；原生记录未恢复到列，产品映射为第 1 列
- 多重性：每个源文件物理行至多一条

## 产品源码适配器

V1.4.186 在条件预处理成功后只收集有界的连续、阻塞和非阻塞赋值候选；排除声明、条件比较、FOR 头及 FOR/GENFOR 的裸直接赋值体。左值从赋值运算符向左识别直接标识符及静态/动态位选、片选，复合左值不能作为原生配对中的较早候选。同行候选满足配对后只发出一条固定 Warning。

## 未恢复边界

规则对象 `+0xa8` 映射的完整生产者及分组所有权、宏/include/generated 节点归属、声明初始化的前端降级、畸形 AST 恢复、候选遍历顺序和跨源文件同号行的分组行为仍未闭合。产品使用词法/结构化源码适配器近似 AST 候选顺序，不宣称完整原生等价；无告警结果继续标为 `ManualReviewRequired`。
