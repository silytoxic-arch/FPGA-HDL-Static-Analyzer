# GJB10157.12.2 原生状态机 safe 模式判定恢复记录

## 目标与边界

- 目标二进制：`T-Linter.dll`
- SHA-256：`60e6b52875f4d7c3fbdf494d869b3a4bcc3fdc8ee63d8620ea9614da5d6599b0`
- 原生类：`GJB10157_VLOG_12_2`
- vtable slot 0：`FUN_1805215d0`（`0x1805215d0`）
- 主要候选处理函数：`FUN_180524530`（`0x180524530`）
- 注释/属性诊断函数：`FUN_180522090`（`0x180522090`）
- 工具：Ghidra 12.1.4 保存工程 `T_Linter_A14_1`，OpenJDK 21.0.12.1；仅离线反编译，未执行厂商程序。
- 本轮原生核心反编译：`reverse-tools/evidence/r16_2/native_core_functions.txt`，SHA-256 `2a4f31704758db7f2e03f8278053feb8636f7a69815bef29ab5f57faa87811b2`。
- 正则与字符串查找辅助函数：`reverse-tools/evidence/r16_2/native_regex_helpers.txt`，SHA-256 `4788fdd1614475c66ae0342c4ef233c01beaefa630b8bd94b2cda00098597371`。

## 原生观察

`FUN_1805215d0` 先为每个设计构造驱动、使用和连接关系，再反向遍历候选 wire 并调用 `FUN_180524530`。入口不是对源文件中 `case` 或 `safe` 的全局字符串搜索。

`FUN_180524530` 的前半段与 Yosys `fsm_detect` 的候选门控一致：

1. 读取 wire 的 `fsm_encoding` 属性；值为精确的 `none` 时返回；
2. wire 宽度小于等于 1 时不继续检测；
3. 只沿支持的触发器、MUX 树和状态使用关系证明 FSM 候选；
4. 候选成立后进入 `FUN_180522090` 检查源属性或综合注释。

该结构还由 DLL 中的原生字符串 `Removing fsm_encoding attribute from 1-bit net: %s.%s`、`none` 和同一函数的控制流交叉印证。上游 Yosys `passes/fsm/fsm_detect.cc` 仅作为开源结构对照，不替代附件 DLL 的反编译证据。

`FUN_180522090` 将候选 wire 的源位置拆为文件名和行号，检查状态变量的属性值是否包含小写 `safe`；若未满足，再读取候选行及前一行关联的 `//` 或 `/* ... */` 注释，并使用 DLL 内嵌的两个正则：

```text
\s*synthesis\s*((syn_encoding)?)\s*=\s*".*safe.*"\s*
\s*((synthesis)?)\s*fsm_safe_state\s*=\s*".*state.*"\s*
```

两个正则均以原生 `basic_regex` 选项值 `1` 构造；MSVC `syntax_option_type` 中该位为 `icase`，因此注释形式不区分大小写。属性路径则调用普通字节串查找，在 `fsm_encoding` 属性值中搜索精确小写 `safe`，不接受大写 `SAFE`。

若属性和两个注释形式均未命中，函数从 `0x180eeaab8` 复制 22 字节 GB18030 文本 `状态机未设置为safe模式`，并从 `0x180ee83b0` 复制 4 字节 `警告`。因此原生严重级别为 Warning，消息不附加状态变量名。

## Recovered predicate

产品接入下列有证据且可在源码层严格证明的子集：

1. 复用已验证的一进程状态更新或直接二进程 `state <= next_state` FSM 适配器；
2. 状态变量必须有唯一的 `reg`、`logic`、`bit` 或 `integer` 声明，并能证明宽度大于 1；
3. 精确的 `fsm_encoding="none"` 保留原生排除语义；
4. 接受 `fsm_encoding` 值包含精确小写 `safe`，以及上述两种与状态变量声明位于同一行或前一行、且不区分大小写的综合注释；
5. 对已证明候选且没有这些 safe 证据的状态变量报告 Warning：`状态机未设置为safe模式`。

## 验证

`tests/standard_accuracy_tests.cpp` 覆盖：

- 一进程和直接二进程多位 FSM 缺少 safe 模式时报告；
- `fsm_encoding="safe"`、`synthesis syn_encoding="safe"`、`synthesis fsm_safe_state="state"` 三种正例；
- 两种注释的大写形式与原生可选 `syn_encoding` 组边界；
- 大写 `fsm_encoding="SAFE"` 不满足属性字节串查找；
- Warning 严重级别与不附加变量名的原生完整消息；
- `fsm_encoding="onehot"` 仍报告；
- `fsm_encoding="none"` 和一位状态寄存器不报告；
- 距状态声明两行以上的无关 safe 注释不能抑制告警。

## 未恢复部分

- 原生网表候选识别的全部 FF/MUX/别名、常量传播和使用关系；
- typedef/enum、参数化位宽、宏、generate、层次及跨文件属性绑定；
- 工程级综合器配置是否可在源属性之外满足条款；
- 正则区域设置对非 ASCII 字符的细节、诊断数量和顺序。

因此该实现仍标记为人工复核模式：命中的受限子谓词可报告，但未覆盖的无告警路径不自动判定合规，也不宣称完整原生等价。
