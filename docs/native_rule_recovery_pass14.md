# 第十四轮：GJB10157.5.2.9

> 历史记录更新：本轮把 `lVar3` 泛称为 AST 根，并据此采用逐设计单元近似；第六十二轮进一步恢复虚表槽 1 的 `AST_CELL(0x4f)` 准入，已修正为逐实例报告。当前结论见 `native_rule_recovery_pass62.md`。

附件 T-Linter.dll（SHA-256 `60e6b52875f4d7c3fbdf494d869b3a4bcc3fdc8ee63d8620ea9614da5d6599b0`）的静态证据显示：规则构造函数 `0x180021b80` 最终安装 vtable `0x180eef1d8`，run 入口 `0x1806d30d0`。其递归收集 AST_ARGUMENT(13) 节点，检查节点名称字符串长度（`+0x58`）是否为零；在每个 AST 根中命中第一个后发出 Warning 并停止当前根的搜索。证据 `ev_6659c1428a97450292d463e44400d3a98c8a36293bdf30761102a33f30cffc76`、`ev_90de243667744365d1adc00c2a5571260bb3e2e2df08f1c6761bebb0f92a38e8`、`ev_2148b633a698afcb26c5e94d994d52969dc623bbcd931daae04a4f0fa1f1b67c`、`ev_d02a3a7b6216e84a9e2e87d6b439ba09f213ce9520a3e520c4538722f755f236`。完整原生记录见同工作区 `reverse-tools/evidence/t_linter_pass3/positional_ports_5_2_9_findings.md`。

软件以已解析的实例位置端口连接作为有界适配：每个 Verilog 设计单元只输出第一个位置连接诊断，而非每个实例重复输出。为方便修改，诊断定位仍是首个实例，区别于原生根节点位置。VHDL 规则不受此改动影响。

尚不能证明 AST_ARGUMENT 在原生前端仅用于实例端口，也未恢复宏、生成结构、所有 AST 根分组及完整诊断等价。无告警保持“需人工复核”。

209条台账：27条核心谓词部分接入，2条模型谓词已观察但未接入，182条核心谓词仍待解释，完整算法等价验证0条。
