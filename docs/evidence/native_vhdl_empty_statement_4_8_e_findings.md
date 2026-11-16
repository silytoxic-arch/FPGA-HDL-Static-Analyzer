# GJB9765 4.8.e 原生空语句判断恢复

## 目标与样本边界

- 目标：用户附件 `T-Linter.dll`。
- SHA-256：`60e6b52875f4d7c3fbdf494d869b3a4bcc3fdc8ee63d8620ea9614da5d6599b0`。
- 方法：Ghidra 12.1.4 只读静态反编译，不执行 Windows 目标。

## 已观察事实

### 规则入口 `FUN_1806090d0`

证据：`reverse-tools/evidence/remaining_rules/vhdl_4_8_e_entry.c`，SHA-256 `cc7d198c1dfae47b702e9d056afb482b9371971bb58e4e1e4258b405b4b98ece`。

1. 入口遍历每个输入文件的 AST 根。
2. 对每个根调用 `FUN_180609b30`，随后按全局向量中保存的节点逐项构造一条 Warning。
3. 报警循环没有按源码位置或节点地址去重，因此原生报警次数等于收集到的节点数。

### AST_ALWAYS 收集器 `FUN_180609b30`

证据：`reverse-tools/evidence/remaining_rules/vhdl_4_8_e_helpers.c`，SHA-256 `3f15f5cd445834aacef093eb3be4d18986a5d57a4ae6a87122c2613cac0c268d`。

1. 仅当当前节点类型为 `0x52` 且至少有一个子节点时进入规则专用检查；跨规则类型表把 `0x52` 标识为 `AST_ALWAYS`。
2. 它首先把该根交给 `FUN_1806097d0` 递归扫描。
3. 若 `AST_ALWAYS` 的第一个子节点为 `0x54`、该子节点恰有一个子节点且该孙节点类型为 `0x67`，则额外收集这个 `AST_ALWAYS` 根。
4. 跨规则类型表把 `0x54` 标识为 `AST_BLOCK`，`0x67` 位于 `AST_POSEDGE(0x65)`、`AST_NEGEDGE(0x66)` 之后并用于事件占位；该形状与带敏感/事件节点但没有实际顺序语句的空进程一致。
5. 函数随后递归访问全部子节点，因此嵌套顺序结构不会被跳过。

### 空节点递归器 `FUN_1806097d0`

证据：`reverse-tools/evidence/remaining_rules/vhdl_4_8_e_helpers2.c`，SHA-256 `45a44f1c43fecd33476317bcbe43c38aaf8339376dff4226f22c63202d783dbc`。

1. 类型 `0xb0` 的每个节点都加入报警向量；附件样例与当前类型映射把它标识为显式 `AST_NULL`。
2. 类型 `0x54` 且子节点数为零的每个节点都加入报警向量，即任意空顺序语句块。
3. 无论当前节点是否命中，都会递归访问它的所有子节点。

## 产品集成

产品在 VHDL process 范围内报告以下确定源码形态，每个确定空节点报告一次：

- `null;`；
- 空 process 主体；
- 空 `if`、`elsif` 或 `else` 分支；
- 空 `case` alternative；
- 空 `loop` 循环体。

新增回归覆盖空 case alternative 和空 loop；原有回归继续覆盖显式 NULL、空 IF 分支、空 process 以及完整非空分支。

## 推断与未知项

- `0x67` 的“事件占位”语义由相邻已命名枚举、AST 形状和空 process 行为共同支持，但没有原厂调试符号直接给出专有 VHDL 前端名称。
- 宏/预处理生成结构、错误恢复生成的空块、子程序内联方式、所有 VHDL-2008 顺序结构和原厂诊断文本仍未完全恢复。
- 因此本规则仍声明“核心谓词已恢复并集成”，不声明完整原生前端等价。
