# V1.3.3 规则集进展

本版只推进规则准确性，未调整问题列表或代码规模明细的表格格式。

- 保留 V1.3.2 已恢复的 GJB9765.4.12.c 实体直接端口 buffer 判据与 component 假阳性修复。
- 定点反编译 GJB9765.4.2.a 运行入口 `FUN_18064e7e0` 和诊断写入器 `FUN_18064f100`。
- 确认规则包含两个独立分支：实体直接端口容器必须非空，以及内嵌 VLR-010073 路径逐个报告无端口 component。
- 增加有端口 entity + 无端口 component、无端口 entity + 无端口 component、正常 component 的正反例与组合回归。

原生 `0x75`/`0x7b` AST 枚举名、package/configuration 接口和前端错误恢复仍未完整恢复；无告警不表示整条规则已获原生等价验证。
