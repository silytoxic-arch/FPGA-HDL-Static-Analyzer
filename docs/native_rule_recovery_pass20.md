# 第二十轮：GJB10157.5.1.4

依据附件原生运行函数 `0x180528fc0` 与转换函数 `0x1804df420` 的静态证据，修正 Verilog 行长规则为整条物理行达到阈值即报错，不剔除注释或行尾空格。证据 ID：`ev_2436427369f32440ad6e1ae1dbadeffc0db7e58ea7a93c45b4df49db11ec02a8`、`ev_630d95ea5bfb21f6bf8afbde4450ed38ec1652fdb24df59cb1258a1475b7e48e`。使用附件说明中的 80 字符阈值，测试覆盖 79/80/81 边界、注释、行尾空格、VHDL 隔离。详细记录见同工作区 `reverse-tools/evidence/t_linter_pass3/verilog_line_length_5_1_4_findings.md`。

209 条台账：33 条核心判据部分接入，2 条模型判据已观察未接入，176 条核心判据仍待解释，完整算法等价验证 0 条。Windows 字符编码转换、CRLF、配置阈值与 AST 准入尚未等价恢复；无报警仍需人工复核。
