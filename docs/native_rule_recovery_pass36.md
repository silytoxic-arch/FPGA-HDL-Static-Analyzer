# 第三十六轮：GJB10157 R-12-3 FOR 内复位与逻辑

原生 `T-Linter.dll`（SHA-256 `60e6b52875f4d7c3fbdf494d869b3a4bcc3fdc8ee63d8620ea9614da5d6599b0`）中 `R-12-3` 的主入口 `FUN_1805874a0` 与完整性 helper `FUN_180560070` 已闭合到可接入程度。

主入口收集 `AST_ALWAYS` 下所有单标识符正/负边沿事件名称，再检查直接 FOR 块内的 if 降低 CASE。条件只接受裸事件名称或一层逻辑非；helper 要求 if/else 的 default，且会沿同源码行的 else-if CASE 递归到最终 else。入口没有时钟/复位角色分类，也不比较两分支赋值目标。

产品据此移除原有“唯一 negedge 复位 + posedge 时钟 + 同目标赋值”猜测，改为事件名称集合、直接结构、完整 default 和逐 CASE 判定；同时恢复单 module 根门限、高有效形式、单边沿、多个直接 FOR、不同目标、括号化事件及 else-if 边界。原始证据 `reverse-tools/evidence/r12_3_for_reset_logic.txt` 的 SHA-256 为 `ccceaebac0503dbca1fade07f6377316e21d4a22f264d51d9995e109aae927b9`，详细发现见 `docs/evidence/native_verilog_reset_inside_for_9_3_3_findings.md`。

尚未恢复完整 Yosys 块/CASE 降低、宏与 generate 后的直接子节点关系、选择或转义名称规范化、非法源码恢复及全局诊断顺序。产品继续把未覆盖路径与所有无告警结果保留为人工复核，不判定合规。
