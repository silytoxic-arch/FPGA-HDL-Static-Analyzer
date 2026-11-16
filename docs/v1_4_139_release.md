# V1.4.139 R-9-4 条件边沿先序恢复

本版本继续推进规则逆向和判断准确性，不调整界面或表格。对外规则编号继续严格使用官方三段式 `R-X-X` / `A-X-X`。

## 原生证据

对用户附件 `T-Linter.dll` 的 `FUN_1804c4430`、`FUN_1804f3870` 与 `FUN_1804d4cc0` 进行联合静态恢复，确认：

- 条件收集过滤表为 `AST_POSEDGE(0x65)`、`AST_NEGEDGE(0x66)`、`AST_LOGIC_AND(0x45)`、`AST_BIT_AND(0x25)`；
- 收集器先记录当前匹配节点，再按子节点向量从左到右递归，即 AST 先序；
- 识别入口按该顺序扫描，并在第一个成功的 edge/AND 候选处停止；
- `0x25/0x45` 均为 AND，OR 类型 `0x26/0x46` 不会直接进入旧式边沿帮助函数；
- 旧式 event 帮助函数扫描 AND 的全部直接子节点，不要求 event 与等式的左右顺序；
- event 标识符与等式标识符必须按 VHDL 大小写不敏感规则同名，等式常量必须是精确二进制字符值。

上述结论来自离线反编译，不代表运行了附件程序。DLL SHA-256 为 `60e6b52875f4d7c3fbdf494d869b3a4bcc3fdc8ee63d8620ea9614da5d6599b0`。

## 产品接入

- 新增 `findFirstVhdlEdgeCondition`，按先序重建复合条件中的第一条原生可识别边沿；
- 直接 AND 同时支持 `clk'event and clk='1'` 与 `clk='1' and clk'event`；
- 支持操作数外层括号、逻辑父节点和普通调用参数中的后代边沿；
- 左结合顺序保持不变：合法左子树优先于后续 edge 调用；更早的显式 edge 仍优先；
- 不把 `enable and clk'event and clk='1'` 的尾部相邻项伪造为独立子树，显式 `enable and (clk'event and clk='1')` 才识别右子树；
- `R-9-4` 类别 1/2 统一消费该有界识别器，其他类别分流、静态动作和顶层来源 IF 边界不变。

## 验证

- 新增 helper 正反例与顺序边界：左右交换、括号、左/右子树、显式 edge 冲突、普通调用后代、NOT/OR 父节点和非子树尾部片段；
- 新增 `R-9-4` 集成边界：交换旧式操作数、左旧式子树先于后续 edge、非子树尾部排除、括号右子树接纳；
- 持久化证据：`docs/evidence/native_vhdl_edge_condition_preorder.txt`，SHA-256 `dd2803727a7c598cbd6a937e62518398d2b633703b43ce0994fdf642c89e8ede`；
- 实施台账保持 209/209 条，完整厂商算法等价验证仍为 `0/209`；别名、generate、包限定/扩展名称、错误恢复和精确诊断顺序仍保留为未恢复边界。
- 完整测试：135/135 通过；
- 附件回归审计：544 条附件记录、537 个打包页面、209 条标准规则、446 个示例；结果为 435 个 `Automated:matched`、6 个 `ProjectContext:matched`、4 个 `context-required`、1 个 `native-documentation-conflict`，相对 V1.4.138 无回退；
- 全新目录解压验证：CLI 版本为 `1.4.139`，macOS 深度严格签名校验通过；独立运行 `R-9-4` 正例得到 1 条违规，旧式交换操作数上升沿/下降沿均被识别；
- 对外 JSON 中违规与标准评估的规则编号全部满足 `R-X-X` / `A-X-X`，未输出内部 `GJB9765.x.x` 条款编号。

## 发行包

- 文件：`releases/1.4.139/FPGA-HDL-Static-Analyzer-1.4.139-Darwin-arm64.zip`
- SHA-256：`5c0bf7236e9700bb81fe9d7613e771855796dfeb8c7c2c91e5d473f12180583d`
- 台账 SHA-256：`c169506a3e6407c15d810365099d5c8258284a11ef913ec6e4a3a8589374264c`
