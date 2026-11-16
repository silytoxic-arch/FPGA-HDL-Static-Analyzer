# A-1-8 工程代码注释率原生恢复记录

公开规则编号严格采用官方三段式 `A-1-8`。内部条款键 `GJB10157.5.1.8` 仅用于源码、附件和逆向证据关联，不作为界面规则编号。

## 恢复对象

- 附件版本：T-Linter V3.1.4.251205
- 目标文件：`T-Linter.dll`
- SHA-256：`60e6b52875f4d7c3fbdf494d869b3a4bcc3fdc8ee63d8620ea9614da5d6599b0`
- 构造函数：`FUN_180021000`
- 活动入口：`FUN_18052b5d0`
- 逐文件扫描器：`FUN_18052bd10`

## 恢复判据

1. 活动入口先清零一个工程级累计器，而非为每个源码分别作出结论。
2. `+0xa8` 映射中的 Verilog 条目只有在向量恰有一个 `AST_MODULE(2)` 根时才参与统计；同名模块导致向量长度不为 1 时，该键整体跳过。
3. `+0x78` 映射由另一前端路径填充，活动入口对每个非空条目取第一个根的源码路径。生产链表明该路径覆盖 VHDL/另一前端的设计根。
4. 同一源码若拥有多个被接纳的设计根，会按根数重复累计。
5. 扫描器逐物理行统计，空行也进入分母；每行只删除空格和 Tab。
6. 扫描器按原始字节查找 `//`、`--`、`/*`、`*/`，不排除字符串中的标记。块注释持续期间的每个物理行均计为注释行，每行最多计一次。
7. 使用单精度除法计算 `commentLines / totalLines`，再与双精度常量 `0.2` 比较；小于 20% 时只产生一条诊断。总行数为零时得到 NaN，比较不成立，因此仍产生诊断。
8. 原生严重度字符串为小写 `error`；产品映射为 Error。诊断位置为空文件路径、第 1 行，固定 GB18030 正文为“该工程代码注释率小于20%”。

## 映射生产链

- `FUN_18062cf50` 根据根节点前端类别分派。
- Verilog 路径经 `FUN_18062d430` / `FUN_18062d640` 调用规则虚表 slot 2；本规则 slot 2 为 `FUN_1807238a0`，只接纳 `AST_MODULE(2)`，再由 `FUN_18062af10` 写入 `+0xa8`。
- 另一前端路径经 `FUN_18062d850`，由 `FUN_18062b8a0` 写入 `+0x78`。

## 产品接入

- 在标准级后处理阶段聚合全部被接纳设计根，只执行一次阈值判断。
- Verilog 同名模块按原生 `size == 1` 门控跳过；每个唯一模块根按其源码重复权重统计。
- VHDL 设计根参与同一工程累计。
- 重复 VHDL 实体键的原生首根选择顺序尚未恢复，产品直接停止该规则判断；Verilog 预处理出现问题时同样停止，避免把未知路径伪判为合规。
- 报告层继续把内部条款键映射成公开编号 `A-1-8`。

## 未解决边界

- 重复 VHDL/另一前端映射键的比较器与首根所有权顺序。
- 专有前端的编码转换、错误恢复、虚拟文件和 include 来源归属。
- 宏展开与 generate 产生额外设计根时的完整路径等价性。
- 与其他规则同时运行时的全局诊断排序。

因此本规则记为“证据支持的核心判据；集成源码适配器”，但安静结果仍为 `ManualReviewRequired`，不声明完整原生等价。

## 原始证据及哈希

| 证据 | SHA-256 |
| --- | --- |
| `reverse-tools/evidence/a1_8/native_core_functions.txt` | `588f036c2fe5555b1fec0f855a3f8aef7bafa482a7e9c3f3b860a0d7437f1bec` |
| `reverse-tools/evidence/a1_8/native_driver_functions.txt` | `17eb69f2367669ec525447e2576ce0afe2da4b3ab324731c86f17b5c4e64900e` |
| `reverse-tools/evidence/a1_8/native_run_dispatch.txt` | `ffea803a61033334a7e5a73bf6446cdf2c44a8fa2c09ad3c35b58eca417c0548` |
| `reverse-tools/evidence/a1_8/native_rule_prepare.txt` | `007872bebb172b33c6e7de6e56bf6037d9e51395bb95b1dc05be272ea79cb222` |
| `reverse-tools/evidence/a1_8/native_map_binding.txt` | `fb8979084bf203022ba6ecf82582cecd1d8afb7f15ea3a5e626d35326596c904` |
| `reverse-tools/evidence/a1_8/native_map_insertions.txt` | `53df8ffbb6f5c3c5cd27c66e1104b260ec9dff07452a882fa194777c8f05e77e` |
| `reverse-tools/evidence/a1_8/native_alt_map_insertions.txt` | `54bbeaeda66014aa2a1fd18b481f7d964858f650da8c47dd76912c64229d7caf` |

