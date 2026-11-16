# 第五轮：VHDL 时钟控制与行格式核心落地

目标 DLL SHA-256：`60e6b52875f4d7c3fbdf494d869b3a4bcc3fdc8ee63d8620ea9614da5d6599b0`。
静态证据通过 REA/Ghidra 读取，不执行供应商 DLL，不修改附件。

## 时钟控制

入口 `0x18068aaf0`，Evidence `ev_8db32876e0c5e664daa8d39b05e03aafd03e235a87160ab608419944718e1add`。
class1/2分类器经汇编确认原process入参，纠正反编译显示null的失真：
`ev_78ded7eaead89d9cd328278a0fc9eba39dcbc0365a11f5e2ba70c9a45bed2898`。
原生先收准入进程的边沿标识和筛选的直属EQ operand，遍历结束后求名称交集；
不是对所有比较或常量赋值报警。

独立实现 `vhdl_edge_condition.h`、`vhdl_clock_control.h`，由 `standard_evaluator.cpp` 调用。
信号位置来自真实源码token，支持同architecture跨process交集；声明/IEEE上下文/局部遮蔽检查限制误报。
安全差异、flat子集及未恢复前端详见台账。没有报警仍为ManualReview。
纯edge常量赋值的附件反例与原生核心观察冲突；不迎合文档标签伪造报警。

## 格式规则

- 4.14.d.1 / `0x1805ca910`：`ev_de05796012609d038f81de9e112f2310faa3e25feaf0bcca39a86639c4955f75`。非空白字节计数大于72；不是可见列宽或含缩进长度。
- 4.14.d.2 / `0x1805cb0d0`：`ev_1368fc8f429eba7e2bc3986daf88028e9d33f5cbf274f32757b2333040e50c6a`。分号计数大于1。

原生按固定fgets缓冲分段，且字节循环误把引号内分号当语句，遇引号内`--`也可能截断。
独立软件改用真实物理行及词法token。此处明确保留产品安全差异，未宣称完整等价。

## 待继续的复杂规则

已建立独立 `vhdl_fsm_model.h` 与测试，保存 source/destination 集合、may-transition和初始化出处。
它仍是部分模型 helper，尚未用于声称4.11全部恢复；不把只有enum/输出decoder当FSM。
模型的driver、作用域、前端 lowering、两个process聚合和编码属性仍需逐项验证。
文件头5.1.1原生依赖配置关键词map，构造map为空；不能把产品Author/History模板谎称原生默认。

总台账209条，14条核心谓词部分接入，5条FSM消费者已解释但未接入，完整算法等价验证0条。
本轮软件与源码包是检查点。继续按“证据→独立实现→正反例/边界→实际CLI/GUI共用引擎→台账”推进剩余规则。
