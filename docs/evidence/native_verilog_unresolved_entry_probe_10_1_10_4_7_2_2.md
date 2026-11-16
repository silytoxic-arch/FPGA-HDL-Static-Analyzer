# 三条 Verilog 未恢复规则的定点入口观察

历史记录更新：10.1、10.4 和 7.2.2 的后续核心谓词恢复分别见 `native_verilog_fsm_deadlock_10_1_findings.md`、`native_verilog_fsm_state_count_10_4_findings.md`、`native_verilog_gate_latch_7_2_2_findings.md`。下面保留当时入口级观察，不代表当前接入状态；当前状态以 `native_rule_implementation_ledger.json` 为准。

目标：附件 `T-Linter.dll`，SHA256 `60e6b52875f4d7c3fbdf494d869b3a4bcc3fdc8ee63d8620ea9614da5d6599b0`。2026-10-19 使用工作区 Ghidra 12.1.4 保存的 PE x86-64 项目，以只读模式运行 `DecompileRuleHelper.java`；未执行厂商程序。完整输出在工作区 `reverse-tools/ghidra/ghidra_12.1.4_PUBLIC/application.log`，可按函数地址检索。

- GJB10157.10.1：vtable slot 0 `FUN_1804fe030` 遍历 `0x181370038` 起的对象表，从对象字段提取关系记录，并调用 `FUN_18025ab40`、`FUN_180472960`、`FUN_180502c50`。入口同时有多条分支，单凭入口尚不能确定死锁状态与不可达状态各自的判定和排除条件。
- GJB10157.10.4：vtable slot 0 `FUN_180512df0` 同样读取对象和关系记录；末段逐条调用 `FUN_180515d70`。该辅助函数前 220/402 行含模型字段查询、字符串比较及报告调用，但尚未确立“40 个状态”的来源、比较方向和诊断位置。
- GJB10157.7.2.2：vtable slot 0 `FUN_1805698f0` 共 818 行；已查看前 220 行，其中包含对象/索引初始化与图关系数据的准备。基础逻辑门锁存器的最终报告分支仍未隔离。

这三条规则的台账状态继续为“入口已提取、算法未解释”，软件不把上述入口观察转换为告警或合规判断。下一步需沿调用图解开关系数据结构与报告分支，再构造可对应的正反例。
