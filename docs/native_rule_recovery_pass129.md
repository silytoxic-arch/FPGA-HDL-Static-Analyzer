# 第一百二十九轮：A-10-4 的类型声明边界

目标 DLL SHA-256：`60e6b52875f4d7c3fbdf494d869b3a4bcc3fdc8ee63d8620ea9614da5d6599b0`。
静态分析同一目标，未执行 DLL。原始输出保留于
`reverse-tools/evidence/t_linter_pass129/`，逐文件字节数和摘要见 manifest.json。

## 已观察事实

- `Type_declarationContext` vtable `180e6afc0` 的 enter/exit 委派至 listener
  `+fa8/+fb0`，对应 `1801c4430/1801cb4e0`。enter 建立 type `0x77` 节点，
  从 identifier context 取名称，写入名称/源行，并加入当前树与声明表。
- `Subtype_declarationContext` vtable `180e6a7f0` 的 enter/exit 委派至
  `+f08/+f10`，对应 `1801c23f0/1801c9d70`。建立 type `0x9f` 的独立节点，
  保存 identifier 名称和源行，进入/退出 AST 栈。
- `Enumeration_type_definitionContext` vtable `180e63900` 的 `+608/+610`
  回调均是 `1800e7160` 空函数。这仅说明该上下文自身没有额外动作；
  不表示枚举字面量、type-declaration exit 或其下游没有处理。
- `Subtype_indicationContext` vtable `180e6a8b8` 经 `+f18` 进入 `1801c2660`。
  函数取得类型名并转小写；在特定父节点情况下把名字写入 `+d0`，另一分支创建
  `0x83` 节点并保存名字及 `+158` 字符串。该事实证明类型指示不是简单删除的文本，
  但不足以证明别名经所有后续阶段会如何规范化成时钟关系。

这些是节点生产者的证据，规则入口仍沿用先前恢复的 `18068aaf0` 名称配对。
未把声明回调当成完整类型解析器，也未用示例推测补齐别名算法。

## 本版接入

之前 architecture 在 BEGIN 前出现任意 TYPE/SUBTYPE 都放弃整个区域。
现在使用与 block/generate 相同的有序声明解析，支持标识符枚举和无约束的
简单/选择式 subtype mark。无关声明不再丢弃已有的明确 IEEE 标量时钟关系。

类型名及枚举标识符在声明点隐藏外层同名对象，进入子区域复制绑定，退出后恢复。
只有明确的 std_logic/std_ulogic 信号建立本轮可检查的绑定。
subtype 声明可以独立存在，但使用 subtype 作为信号类型不会被猜为 IEEE 标量；
尤其不根据 `work.pkg.std_logic` 的最后一段名字推定类型。

保留未知边界：entity 内声明、约束/resolution indication、字符枚举、record/physical/
array/access 等复杂 type、子程序与导入、完整别名归一化及不同 owner 的同名关系。
未知局部区域仍隔离，不删除已证明的外层诊断。该适配器始终 Partial；无诊断需人工复核。

## 验证

新增 34 组模型边界及 4 组端到端回归：普通枚举、无关 subtype、名称隐藏、
源顺序、块/生成区域、复杂/残缺声明、别名链不猜测、跨 architecture、
外层有效定位保留和无诊断复核。
回归结果核对的是独立实现及恢复的来源约束，未冒充原工具运行对照。
完整算法验证保持 0/209。
