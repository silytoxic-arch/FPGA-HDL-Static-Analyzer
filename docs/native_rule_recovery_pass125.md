# 第一百二十五轮：VHDL 空语句报告与证据留存

目标 T-Linter.dll，SHA-256
`60e6b52875f4d7c3fbdf494d869b3a4bcc3fdc8ee63d8620ea9614da5d6599b0`。
仅静态分析，未执行原工具。原始输出位于
`reverse-tools/evidence/t_linter_pass125/`，文件中的目标摘要保留原样。

## A-8-1 已确认字段

FUN_1806090d0 按收集向量逐项输出，正文取 0x180f02d20 的 22 个 GB18030 字节：
`b1dcc3e2d4dabdf8b3ccd6d0b3f6cfd6bfd5d3efbee4`，即“避免在进程中出现空语句”。
级别 Warning，行号来自节点 +0x214，列号字符串为空；产品按缺省值映射为 1。
现有五类源码检测路径统一该正文和列号。六组报告回归覆盖两条 null 的重复计数。
回归发现通用诊断去重会把同一行的两个空节点合并；本轮按已恢复的原生逐节点追加行为，为 A-8-1 保留重复报告。
本轮未修改尚未恢复的空分支最终节点定位算法。

## 修正旧有节点推断

Sequential_statementContext vtable 0x180e69080 的进入委托经 listener +0xd28
到 FUN_1801bfbf0，为每条顺序语句创建 0x54 块，行号为上下文起始行；退出弹栈。
Sequence_of_statementsContext 的进入/退出均为空操作。
Process_statementContext 进入 FUN_1801bde50 创建 ALWAYS 与 BLOCK，
二者行号均来自上下文起点；这不能等同于 begin 的源码行。
IF 退出回调会重建 CASE/BLOCK（地址与正文见保存的实际输出）：
本轮保存完整 if_listener.txt，不以 then 关键字位置替代所有重建节点的行号。

对构造器 FUN_1808d8dc0 的有界直接调用扫描发现三个类型 0xb0 构造入口：
FUN_1801baf20、FUN_1801c1de0、FUN_1801c20d0；后两者可重标为 0x8e 或 4。
尚未完整归属这些上下文，不能继续把 0xb0 简称为源码显式 null。
原始收集谓词、明确的源码检测与尚未知的前端映射分别记录，不计为完整算法验证。

## 修复证据引用

此前 143 份被台账引用的静态文本只在仓库外的同级 reverse-tools 中存在。
本轮逐字节恢复约 4 MB 文本，所有已有 #sha256 声明一致，无摘要冲突。
清单 native_evidence_bundle_pass125.json 记录大小、真实摘要、既有声明是否匹配，
以及原始输出是否包含目标摘要。没有目标摘要的历史文件仍明确保留来源验证限制，
不能仅凭新计算的内容摘要宣称目标来源已经证明。

将 ../reverse-tools/evidence 引用修正为仓库内路径；原始证据禁用换行转换，
避免 Windows checkout 改变摘要。验证脚本同时检查台账全部文件引用、声明摘要、
恢复清单大小与目标摘要声明，并加入每次 Windows GitHub Actions 的构建前检查。
不上传目标 DLL，不在本地生成、下载或保存 Windows 安装包。

完整算法验证仍为 0/209；无诊断仍需人工复核。空分支重建、标签、子程序声明区、
敏感表和错误恢复等前端语义继续核验。
