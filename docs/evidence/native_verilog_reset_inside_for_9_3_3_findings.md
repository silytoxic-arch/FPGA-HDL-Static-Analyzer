# GJB10157 / R-12-3：for 循环中的复位与逻辑分支

- 目标：附件 `T-Linter.dll`，SHA-256 `60e6b52875f4d7c3fbdf494d869b3a4bcc3fdc8ee63d8620ea9614da5d6599b0`。仅执行静态反编译和只读数据提取，未运行厂商 DLL。
- 原生入口 `FUN_1805874a0` 要求当前记录恰有一个 `AST_MODULE(2)` 根，递归收集其中 `AST_ALWAYS(0x52)`，再把所有单 `AST_IDENTIFIER(0x13)` 子节点的 `AST_POSEDGE/AST_NEGEDGE(0x65/0x66)` 名称放入事件集合。该入口不区分哪个名称是时钟或复位，也不要求同时存在两类边沿。
- 入口检查过程块的直接 `AST_FOR(0x5c)`，以及循环块中的直接 `AST_CASE(0x57)`；过程块和循环块各允许一次首子节点 `AST_BLOCK(0x54)` 展开。CASE 的直接 `AST_REDUCE_BOOL(0x2d)` 只有在其唯一子节点为裸 `AST_IDENTIFIER`，或一层 `AST_LOGIC_NOT(0x47)` 包裹的标识符时才参与匹配。名称命中任一边沿事件名称即设置第一道门。
- 第二道门 `FUN_180560070` 遍历 `AST_COND(0x58)`，要求存在 `AST_DEFAULT(0x5b)`；若默认分支块包含同源码行的嵌套 CASE，则递归追踪该 CASE。这对应完整 if/else，或以最终 else 结束的 else-if 链。没有最终 else 时不报告。
- 原生逻辑完全不读取复位分支和逻辑分支的赋值目标或赋值值。因此现有产品中“低有效异步复位、必须同时存在正沿时钟、两分支写同一目标”的限制没有原生依据，已移除。
- 原生 GBK 消息为“for循环中同时出现复位部分和逻辑部分”。每个同时通过两道门的 CASE 独立报告，源码行取当前 FOR 节点。原始聚焦证据为 `reverse-tools/evidence/r12_3_for_reset_logic.txt`，SHA-256 `ccceaebac0503dbca1fade07f6377316e21d4a22f264d51d9995e109aae927b9`；主入口证据为 `ev_8f101f9b2d643f53865518245673a2851837d98e45fab8793e73277b4505e60a`。
- 产品现按直接语句结构建立所有边沿事件名称集合，检查直接 FOR/IF、裸名称或一层逻辑非、完整 else/default 和 else-if 终结分支；支持高/低有效、单边沿过程、不同分支目标、同一过程中的多个直接 FOR、一次嵌套块和括号化单标识符事件。多 module 源文件按原生单根门限不自动报告。
- 验证覆盖附件正反例，以及高有效复位、单 clock 名称匹配、不同目标、无 else、复合条件、多个 FOR、嵌套块、带/不带最终 else 的 else-if、括号化事件和多 module 根。专项与完整回归结果随 V1.4.172 发布记录固化。
- 未知：完整 Yosys `AST_BLOCK/AST_CASE` 降低、选择/转义名称规范化、宏和 generate 后的直接子节点关系、非法源码恢复、生成源码映射、全局诊断顺序及与其他规则的执行关系仍未恢复。所有无告警结果仍为 `ManualReviewRequired`，不宣称完整原生等价。
