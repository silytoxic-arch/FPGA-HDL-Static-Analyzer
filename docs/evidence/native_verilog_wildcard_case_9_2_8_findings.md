# GJB 10157 `A-11-2` 原生判断逻辑恢复记录

## 目标与身份

- 公开规则编号：`A-11-2`
- 内部条款键：`GJB10157.9.2.8`
- 附件类名：`GJB10157_VLOG_9_2_8`
- 目标：`T-Linter.dll`
- SHA-256：`60e6b52875f4d7c3fbdf494d869b3a4bcc3fdc8ee63d8620ea9614da5d6599b0`
- 分析器：Ghidra 12.1.4，复用已完成默认全量分析的数据库

## 已观察事实

构造函数 `FUN_180024010` 注册精确规则字符串，并安装位于 `0x180ef5458` 的规则虚表。虚表中的核心入口为：

- `FUN_180716130`：只接收类型 `0x57`、字节 `+0x183 == 0` 且子节点非空的节点；既有枚举证据将 `0x57` 映射为 `AST_CASE`。
- `FUN_1805843e0`：逐个扫描获准 CASE 的直接子节点，只接受类型 `0x59` 或 `0x5a`，即 `AST_CONDX/AST_CONDZ`。
- 命中第一个相应子节点后，使用 CASE 节点自身的文件路径 `+0x1e0` 和行号 `+0x210` 生成一条 Warning，随后立即结束该 CASE 的子节点循环。
- 固定 GB18030 消息为“避免使用casex与casez语句.”。

因此，原生诊断粒度是“每个含通配 case item 的 `AST_CASE` 最多一条”，不是每个 item 一条，也不是看见源码关键字就无条件报告。

## 源码映射

Yosys 官方解析器的 `case_item` 产生逻辑与二进制类型完全吻合：当前 case 类型为 `casex` 时创建 `AST_CONDX`，为 `casez` 时创建 `AST_CONDZ`；`case_body` 允许为空。这一公开源码只用于解释已观察到的二进制枚举，不替代附件 DLL 证据。

由此得到可安全实现的源码判据：

1. 必须是完整闭合的 `casex/casez (selector) ... endcase`；
2. 必须至少存在一个可识别的 case item，包括 default-only 形式；
3. 同一 case 无论有多少 item，只报告一次；
4. 嵌套 case 按各自 AST_CASE 分别报告；
5. 空 case、不完整源码、注释或字符串中的关键字不报告；无告警仍保持 `ManualReviewRequired`。

## 产品修正与验证

产品原实现只要看到活动源码中的 `casex/casez` 关键字就报告，会对空 case 与不完整 case 产生超出原生判据的告警。本轮已改成闭合结构和 item 双重准入，并新增以下边界回归：

- 普通 `casex` 与 `casez` 各一条；
- 空 `casex/casez` 均为零条；
- default-only 与含三元表达式的 item 均各一条；
- 不完整 case 为零条且评估保持人工复核；
- 嵌套 `casex/casez` 按两个 AST_CASE 输出两条。

原始核心摘录见 `native_verilog_wildcard_case_9_2_8_core.txt`。

## 仍未知

`+0x183` 的正式字段名及所有生产路径、附件所用 Yosys 精确版本、完整宏/include 来源映射、错误恢复与生成/简化节点、原生列号和遍历次序、豁免机制仍未闭合，因此不声明完整算法等价。
