# GJB9765.4.5.f.2/.14/.15/.16/.18/.19/.20 命名规则证据

## 样本绑定

- 目标：附件 `T-Linter.dll`
- SHA-256：`60e6b52875f4d7c3fbdf494d869b3a4bcc3fdc8ee63d8620ea9614da5d6599b0`
- f.2：`FUN_1805ed330`，`ev_8e9d84f98ea3fb91a27094a2ecb42762455deef56556f5e79e8cba29d8a8ded6`
- f.14：`FUN_1805e75d0` / `FUN_1805e7f70`，`ev_a684b95a3cbf6a958bc8a2ae559fd5dbccad0e3fc2aa332ceb242e4c999c9668`
- f.15：`FUN_1805e9000` / `FUN_1805e99a0`，`ev_8c634e750133156ff685bbe5fbef90a7a951c47dbfe10d8d3c1b94131669c0f2`
- f.16：`FUN_1805eaab0` / `FUN_1805eb450`，`ev_0cbba0652066e66ef9657de0de0b72c39bf61633c675a50ace7f96657ee9b9a0`
- f.18：`FUN_1805ec170`，`ev_28277d2f384f70d5cfac1790caa3483d5580554ec6484f6b1a99242d685a6297`
- f.19：`FUN_1805eca50`，`ev_3ee5b852b714f98e0fa41fb1e6c5ca86960e156749b3de1b75d6c97b821ae9b9`
- f.20：`FUN_1805edc20`，`ev_78ccf26c417dd23e54ce4e91ad29bbab093cba851a5e295c4eddc91867d4d648`

## Recovered predicates

f.2 通过通用递归收集器选择 AST 类型 `0x79`，读取 `+0x48` 的非空名称，并把规则对象 `+0xc0` 的字符串传给 `FUN_1804d6e80` 模式 `0`。模式 `0` 比较名称开头，默认大小写标志会先折叠两侧字符。附件正反例将该对象类型和默认值分别确定为 `variable` 与 `V`。

f.14、f.15、f.16 使用同构递归命名引擎，分别选择 AST 类型 `0x77`、`0x9f`、`0x04`。引擎可从规则对象 `+0xc0`、`+0xe0`、`+0x100` 取得前缀、后缀和包含串，并使用 `+0x144..+0x146` 的选项控制比较。附件默认配置仅证明以下大小写无关前缀：

- f.14：`type` 使用 `T`；
- f.15：`subtype` 使用 `ST`；
- f.16：`function` 使用 `F`。

f.18 与 f.19 分别递归收集 AST 类型 `0xb4` 和 `0x63`，对名称执行 `FUN_1804d6e80` 模式 `2` 的末尾比较。附件示例将它们确定为 block 和 generate 标签，默认后缀分别是 `_block` 和 `_gen`。

f.20 在设计对象下先选择类型 `0x75` 的容器，再检查其直接类型 `0x52` 子节点；空名称和一个原生内部占位名被排除。调用点 `0x1805edf22` 将 `r8d` 设置为 `2`，并从规则对象 `+0x145` 读取大小写选项后调用 `FUN_1804d6e80`。附件示例确定这是明确标注的 process，默认后缀为 `_proc`。无标签 process 不应由产品虚构名称后报告。

## 产品边界

V1.3.5 接入完整、简单源码声明与明确标签：分组 `variable`、`type name is`、`subtype name is`、简单标识符函数声明，以及 block、for/if/case generate、process/postponed process 标签。比较不区分大小写。

外部规则配置的非默认前缀/后缀/包含串、扩展标识符、操作符函数、复杂前端归一化、错误恢复和原生诊断去重尚未完整恢复。因此未发现问题仍标记为人工复核，不声明完整原生等价。
