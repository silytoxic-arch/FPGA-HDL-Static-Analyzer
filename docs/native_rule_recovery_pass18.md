# 第十八轮：GJB10157.5.2.16

附件 T-Linter.dll（SHA-256 `60e6b52875f4d7c3fbdf494d869b3a4bcc3fdc8ee63d8620ea9614da5d6599b0`）的规则 run 函数 `0x180531530` 遍历 AST，遇类型 `0xb` 即 `AST_DEFPARAM` 时逐个发出 `error`，以命中节点位置报告。依据 `ev_d0fe98bf9cceac0b2bea0ad6192686f386dad685df3dc554f8d4e17b74d9c39d`、`ev_512c915b90a165536016fafbb5a5ded6cfba13c0084ad97b3576b7906ee5547b`。详见同工作区 `reverse-tools/evidence/t_linter_pass3/defparam_5_2_16_findings.md`。

产品已有 Verilog `defparam` 词法/语义检测，本轮将标准级别修正为错误，并验证单条、多条、命名参数覆盖、注释和字符串边界。附件错误代码注释误把 Verilog 文件标成 `.vhd`；旧审计据此错误地按 VHDL 分析。本轮审计保留原标注，同时按规则语言改用 `.v` 作为测试输入。该规则错误/正确样例现均匹配；另有三条规则的错误样例也因相同测试输入修正而匹配，此增量不代表新算法恢复。

原生 AST 根选择、预处理/生成结构及完整诊断等价未知。209 条台账现为 31 条核心判据部分接入、2 条模型判据已观察未接入、178 条核心判据待解释、完整算法等价验证 0 条。无报警仍需人工复核。
