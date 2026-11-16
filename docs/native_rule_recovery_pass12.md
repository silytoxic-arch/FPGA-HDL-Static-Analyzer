# 第十二轮：GJB10157.5.2.6

逆向目标仍为用户附件 T-Linter.dll（SHA-256 `60e6b52875f4d7c3fbdf494d869b3a4bcc3fdc8ee63d8620ea9614da5d6599b0`）。仅对静态反编译证据作结构判断，未运行厂商程序。

已追溯规则构造函数 `0x1800219d0` 至 vtable `0x180eedc88`，再定位 run 过程 `0x18053a3c0`（证据 `ev_8e1b9d81e8edb95475bc59ecdec44a90c3adff6c5ba423efb03b5515ef57de9b`、`ev_558bcca5db66985cdb9428bff27ed1b8b21bbb980a13c89c0f556ac278bf6a10`、`ev_9f4b850ac513b5a9f63dffbc54f518d21028ea2923558edeee5753c9452398df`）。其候选为每个 AST_MODULE 中的 MODULE、WIRE、PARAMETER、LOCALPARAM、CELL、TASK；不含 FUNCTION（类型映射证据 `ev_512c915b90a165536016fafbb5a5ded6cfba13c0084ad97b3576b7906ee5547b`）。对非空规范化名称，原拼写不同且折叠大小写后相等时报告后一个候选；不同模块不互相比对。完整原生发现保存在同工作区 `reverse-tools/evidence/t_linter_pass3/case_only_5_2_6_findings.md`。

软件以有界源码声明扫描器接入此核心谓词，修正了跨模块误报，并新增正反例与边界回归。复杂宏、转义标识符、实例化、完整前端以及原生 locale/诊断顺序未验证，故本条仍是部分接入。无报警的规则不标为符合，继续显示“需人工复核”。

当前209条台账：25条核心谓词部分接入，2条模型谓词已观察但未接入，184条核心谓词仍待解释，完整算法等价验证0条。
