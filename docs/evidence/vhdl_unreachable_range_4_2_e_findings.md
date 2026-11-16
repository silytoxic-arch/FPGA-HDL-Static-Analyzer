# GJB9765.4.2.e：固定无符号范围比较的有界实现

附件身份：`T-Linter.dll` SHA-256 为 `60e6b52875f4d7c3fbdf494d869b3a4bcc3fdc8ee63d8620ea9614da5d6599b0`。本轮没有执行厂商软件。

## 观察与证据

- 随包 `GJB9765_VHDL.4.2.e.html` SHA-256 为 `efe271b1c9ddecf0f1174d03ef46987d8f21214ad8363ff14b63f8f740b862c9`。其反例把 4 位 `std_logic_vector` 与十进制 16 比较，正例改为 10。这仅证明随包文档使用这个例子，不证明厂商完整算法。
- 既有 vtable 提取将该规则构造函数关联到 `0x180026bc0`、slot 0 到 `0x1805d0e00`，见 `native_rule_implementation_ledger.json` 的 `ev_4051123ce85adb566e857498f80313a640d8bdfd8838ae2922533615021ab708`、`ev_d236cbf52997863fb759b99d869228693c121892a1f9b423611c9f5fbc8ec527`。这只是入口定位，并非判据恢复。
- 本轮对该入口的 `analyze_function`、`procedure_pseudo_code` 和 `read_function_instructions` 均遇到服务端超时，未取得新的可解释原生伪代码。会话断连时 `close_binary` 不再可用；未据此宣称已完成原生清理。

## 本软件实现边界

在固定宽度、已知无符号解释的 `unsigned`，或导入 `std_logic_unsigned` 后的 `std_logic_vector` 上，仅对直接十进制常量的比较作区间推理。证明比较恒真或恒假时报告 `RTL-CONSTANT-CONDITION`，映射至 GJB9765.4.2.e。未知宽度、签名/重载不明、算术表达式、参数展开及其他不可达代码均不作合规结论。

随包反例还有 `S_f1ag` 拼写问题以及小写 `'l'` 时钟字面量；测试仅使用其位宽与 16/10 的可验证差异。`tests/standard_accuracy_tests.cpp` 增加 16、10、15、反向比较、signed、未导入包、算术表达式边界。附件审计 215/220，整套 CTest 130/130。原生算法仍为待恢复状态。
