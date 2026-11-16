# GJB10157.5.2.12（公开编号 A-2-2）名称长度原生判断恢复

## 证据边界

目标为附件 `T-Linter.dll`，SHA-256：
`60e6b52875f4d7c3fbdf494d869b3a4bcc3fdc8ee63d8620ea9614da5d6599b0`。
以下结论来自静态反编译，不代表厂商源码，也不声称完整前端等价。

原始证据：

- `reverse-tools/evidence/a2_2_name_length_entry.txt`，SHA-256 `7e2ee731f5c8c2757a42434c68e98e5e9f6909c55336bc99d55eaa27336cfe41`
- `reverse-tools/evidence/a2_2_name_length_helpers.txt`，SHA-256 `8ab3c5344a53cfb3bc4a052a5452408c3d77a6225380e9469533ae26b892217f`
- `reverse-tools/evidence/a2_2_name_length_strings.txt`，SHA-256 `925ac6f3c2f0725ba574a23a54750c8b7f5da423247a4e3f309a5655586748a7`
- `reverse-tools/evidence/a2_2_name_length_xrefs.txt`，SHA-256 `ff3cacad872f34946ee34e5ee300ae7ba2bcf6bf6e7bd4285651d364aa11b9ee`
- `reverse-tools/evidence/a2_2_runcheck_config_dispatch.txt`，SHA-256 `aeb1c9e12f7d0385da200019983697f7c35c7609e33f9e48d698a0d685b03ade`

## 恢复的判断逻辑

- 构造函数 `FUN_1800212e0` 将默认最小值和最大值分别设为 `2`、`32`。
- 运行入口 `FUN_18052d8a0` 遍历模块名映射。只有一个模块名恰好对应一个非空 `AST_MODULE(2)` 时才检查该模块；同名模块组包含两个或更多根时整组跳过。
- 递归选择器固定为 `AST_MODULE(2)`、`AST_WIRE(6)`、`AST_PARAMETER(9)`、`AST_LOCALPARAM(10)`、`AST_CELL(79)`、`AST_TASK(3)`；`AST_FUNCTION(4)` 不在集合内。
- 名称先经 `FUN_1804df040` 规范化，空名称跳过；比较对象为规范化名称的字节长度。
- 判断为 `length > maximum || length < minimum`，上下限均包含。32 位有符号配置值先符号扩展，再参与 64 位无符号比较。因此负数与倒置上下限也有确定的机器语义，而不是“未知”。
- 告警级别固定为 `error`；位置使用候选节点物理行，不提供列。
- 原生消息由三个片段拼接为：`<名称>:建议命名长度在<最小值>到<最大值>个字符之间`。

## 配置入口结论

- `runCheck.exe`（SHA-256 `0ee30b930fe018acca0998ef6bfcf7949c081c2b13eeb494b5b83dc6238d8d27`）为 `GJB10157_VLOG_5_2_12_NUM` 建立的三个扩展配置字符串均为空。
- 规则分发函数对它仅执行通用 `CheckRule::setRule(rule, true)`；它不属于 `setMaxStatesNum`、`setRuleNameCase` 或 `setRuleNameConvention` 的任何特殊分支。
- 附件内所有 PE64 文件的导入/导出表中不存在名称长度 setter。只有 `runCheck.exe` 导入上述通用和相邻规则 setter；DLL 导出表也没有其他相关接口。
- `setMaxStatesNum` 的静态实现只按规则名查找对象并写入 `+0x140`，不会写入本规则的 `+0x158/+0x15c`。

因此，`+0x158/+0x15c` 是规则对象中保存固定构造默认值的字段，不能仅因其位于实例中就推断为附件产品可配置策略。对于所提供版本，名称长度边界固定为 2..32。

## 产品接入

V1.4.90 将默认 2..32 闭区间、六类选择器、函数排除、重复同名模块整组跳过、负数/倒置边界的无符号比较、Error 级别、列 1 适配和原生中文消息接入活动预处理分支。V1.4.91 进一步闭合附件调用层，确认所提供版本没有覆盖 2..32 的配置入口。公开显示仍使用官方三段式编号 `A-2-2`。

## 尚未恢复

宏/include 生成 AST、全部转义名称、复杂实例和异常语法的厂商前端构造仍未等价恢复；附件外另行编写、直接操作 DLL 内部对象的未知客户端也不在证据范围内。因此无告警仍为人工复核，完整算法等价继续记为 `0/209`。
