# A-1-2 原生判据恢复记录

公开规则编号为官方三段式 `A-1-2`。`GJB10157.5.1.2` 只作为内部条款键和证据关联键。

## 目标与样本

- 样本：`T-Linter.dll` V3.1.4.25120520251205115359
- SHA-256：`60e6b52875f4d7c3fbdf494d869b3a4bcc3fdc8ee63d8620ea9614da5d6599b0`
- 主执行函数：`FUN_180526b30`（`0x180526b30`）
- if/else 链收集函数：`FUN_180527490`（`0x180527490`）
- 根节点接纳函数：`FUN_1806ef640`（`0x1806ef640`）
- 主函数证据：`ev_f20266ff4ce6c3d1c5b4b07d7744332226e766b2aefb2c54b8e1cf9d7ec8b292`
- 接纳函数证据：`ev_e4a68e44f81363c13a1facdbbbc50f1ff77d7b2000c704f1f20e67e851c9cf64`
- 定点核心函数：`reverse-tools/evidence/a1_2_native_core.txt`，SHA-256 `f71325877e82f49d5905e534a076af5d9f9670a45ef8cd1236e048e78ed6a337`
- 虚表与诊断面：`reverse-tools/evidence/a1_2/native_surface.txt`，SHA-256 `eaa79ab1a73f6183138ffd2817a1970bae361ef98216fb4b34ad15d6c86b53a1`

## Recovered predicate

根节点接纳函数只有一条条件：节点类型必须为 `0x57`，即 `AST_CASE`。主函数根据第一个子节点分为两条路径。

### if/else 路径

第一个子节点为 `AST_REDUCE_BOOL (0x2d)` 时，原生前端把该根解释为 if 条件结构。辅助函数沿 `AST_DEFAULT (0x5b) -> AST_BLOCK (0x54) -> AST_CASE (0x57)` 形状继续收集同一条 else-if 链，并要求嵌套 case 与 block 位于同一源码行。根 if 与各个 else 分支的源码列字段 `+0x208` 都与第一项比较；不同即报告。

### case 路径

其他 `AST_CASE` 根按 case/casex/casez 处理。主函数从类型 `0x58..0x5a` 的条件分支节点中收集分支体 `AST_BLOCK` 之前的标签表达式，并连同 case 根一起比较源码列 `+0x208`。因此每个直接 case 标签必须与 case 关键字处于同一列；嵌套 case 使用自己的独立基准。

两条路径都是固定首项基准，并非与前一项滚动比较。附件违规示例中偏移的最终 `else` 和第二个 case 标签分别触发一次；合规示例两组列均一致。

## 构造、虚表与固定诊断

`FUN_180526a10` 注册内部键 `GJB10157_VLOG.5.1.2` 并安装虚表 `0x180eeaf80`。活动槽为 `FUN_180526b30`，槽 2 `FUN_1806ef640` 只准入 `AST_CASE(0x57)`。

主函数的 if/else 与 case 两条报告分支均构造小写 `error`，复制违规 AST 节点的源路径 `+0x1e0` 和物理行 `+0x210`，但不把用于比较的列字段 `+0x208` 写入诊断。固定 GB18030 正文解码为“建议同一层次的所有语句左端对齐”。产品因此映射为 Error、违规物理行、第 1 列，并保留每个偏移后续记录各一条的多重性。

## 产品映射

产品仅实现有原生证据的两类完整源码结构：

- 完整 `if/else-if/else` 链以首个 if 的列为基准，比较每个 else；
- `case/casex/casez` 以 case 关键字列为基准，比较其直接标签；
- 嵌套 if 和嵌套 case 分别建立独立基准；
- 注释与字符串不产生结构节点；
- 同行 else 仍按其真实源码列比较，不作视觉格式宽容处理。
- 每个不等于首项基准的后续记录独立报告，统一使用固定 Error 正文和第 1 列定位。

回归测试覆盖附件正反例、else-if、最终 else、嵌套 if、嵌套 case、同行 else、注释及字符串边界。

## 未验证边界

Tab 的专有前端列计数、转义标识符、宏与 generate 展开、属性节点、损坏语法恢复及共享嵌套 AST 根的重复报告顺序仍未完全验证。产品只自动报告明确命中的有界子集；没有告警仍为 `ManualReviewRequired`，不声明完全合规。
