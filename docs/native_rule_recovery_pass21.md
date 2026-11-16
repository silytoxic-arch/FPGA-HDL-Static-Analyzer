# 第二十一轮：GJB10157.5.1.8

附件 T-Linter.dll 的 `0x18052b5d0` 和 `0x18052bd10` 表明注释率按“注释物理行 / 全部物理行”计算，空白行进入分母，行内注释与跨行块注释计入分子，低于 20% 输出 Error。产品按单文件文本扫描接入该核心判据。证据 ID：`ev_bdfe040db6eb919c319340d5452687913b2684241d2a84f17c6ea835ad1141ee`、`ev_c76fe2fae0af11952e589beeb91fcbf5c94925f226ee8c8b6616a4bad6d7e55e`、`ev_88387e624ecb9f3c68a8702cec44f2bcff8a6bceb53f05f87ffa6a64e8865a7f`、`ev_9f3fb61330ebae7329e2f12abe171e84c861c30ef0894ffd606b2e0598b0c870`。细节见同工作区 `reverse-tools/evidence/t_linter_pass3/verilog_comment_ratio_5_1_8_findings.md`。

209 条台账：34 条核心判据部分接入，2 条模型判据已观察未接入，175 条核心判据待解释，完整算法等价验证 0 条。原生跨文件累积和 AST 准入未恢复；无告警仍需人工复核。
