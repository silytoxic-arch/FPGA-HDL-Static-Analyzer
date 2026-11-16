# GJB9765.4.2.a 原生判据恢复记录

目标二进制：`T-Linter.dll` V3.1.4，SHA-256
`60e6b52875f4d7c3fbdf494d869b3a4bcc3fdc8ee63d8620ea9614da5d6599b0`。

## Recovered predicate

规则构造函数 `0x180026800` 安装 vtable `0x180efd148` / `0x180efd190`，运行槽均为
`FUN_18064e7e0`（`0x18064e7e0`）。该函数对每个 VHDL 根执行两个独立分支：

1. 遇到直接子节点类型 `0xa4` 时，仅当其子对象向量非空才设置“已有端口”标志；遍历后标志仍为空则在设计单元位置发出 Warning。
2. 遇到类型 `0x75` 时遍历其子节点；每个类型为 `0x7b` 且子向量为空的节点都通过 `FUN_18064f100` 单独写入诊断，并使用内部规则标识 `VLR-010073`。

附件的 `GJB9765_VHDL.4.2.a` 示例证明第一条路径对应无端口 entity。随安装包提供的
`TLR-010073.html`（页面规则号为 TLR、二进制内部标识为 VLR）错误例则保持 entity 有端口，
但声明并例化了一个无端口 component；因此第二条路径对应无端口 component。

## 产品集成边界

产品分别报告解析到的无端口 VHDL entity，以及具有完整 `end component` 边界但没有任何
端口对象的本地 component 声明。两者同时存在时分别报告，不合并为一个文件级问题。

`0x75`、`0x7b` 的权威 AST 枚举名和前端节点构造仍未恢复。package/configuration 中的接口、
跨文件或生成结构、错误恢复以及最终诊断去重尚未证明。无告警保持
`ManualReviewRequired`，不声明完整原生等价。
