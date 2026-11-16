# GJB10157 9.3.6（A-12-1）repeat 计数变量原生证据

## 范围与身份

- 目标：用户附件 `T-Linter.dll`
- SHA-256：`60e6b52875f4d7c3fbdf494d869b3a4bcc3fdc8ee63d8620ea9614da5d6599b0`
- 规则构造函数：`0x180024420`
- 规则运行函数：`0x18058aa70`
- 候选过滤函数：`0x18058b4e0`
- 名称表精确匹配 helper：`0x180529750`
- 本轮反编译：`reverse-tools/evidence/a12_1/native_entry_functions.txt`
- 本轮原始字符串：`reverse-tools/evidence/a12_1/native_strings.txt`

本记录仅描述离线静态观察和独立产品适配；未执行附件程序，未修改附件或授权机制。

## 已观察事实

1. 构造函数登记 `GJB10157_VLOG.9.3.6`。
2. 候选过滤函数接纳类型 `0x5e` 的非空节点，以及类型 9、10；结合交叉规则映射，分别为 `AST_REPEAT`、`AST_PARAMETER`、`AST_LOCALPARAM`。
3. 运行函数在每个内部字符串分组中分别收集 repeat 节点和 parameter/localparam 名称。参数表是该组的平面字符串向量，元素只有名称，不携带词法作用域或声明节点身份。
4. 对每个非空 repeat，只从第一个 child 开始递归收集所有 `AST_IDENTIFIER`；递归 helper 在命中 identifier 后仍继续遍历后代。
5. `0x180529750` 对参数名称表逐项做长度和 `memcmp` 精确匹配。名称不存在时返回零；Verilog 名称比较因此是区分大小写的完整相等，而不是前缀或子串匹配。消费者没有“先证明 identifier 是已声明运行时对象”的步骤，任何不在表中的直接 `AST_IDENTIFIER` 都报告。
6. 报告位于 identifier 循环内部，并直接使用当前 identifier 节点的源码位置。因此一个 repeat 中两个不同变量或同一变量的两个 AST 出现会分别报告，不按 repeat 合并。
7. 参数表只保存参数/localparam 名称；本规则消费者不会递归验证参数初始化表达式。非法参数初始化属于其他语义诊断，不应被本条间接改写为 repeat 变量违规。
8. 严重度固定为 `Warning`。
9. `0x180ef5ef8` 的 33 字节 GB18030 字符串精确解码为 `避免在repeat语句表达式中使用变量.`。

附件示例的 `repeat(B1)` / `repeat(data_in)` 违反例和 `repeat(5)` /
`parameter XX=10; repeat(XX)` 合规例与上述消费者逻辑一致。

## 产品接入

条件预处理成功后，产品在活动 token 流中：

- 按 owning module 组建立平面 parameter/localparam 名称表，不应用词法遮蔽；
- 对 repeat 计数中每个不在该名称表内的直接 identifier 分别输出 `Warning`，包括无法解析声明的普通名称；
- 使用当前 identifier 的原始物理行列和恢复出的固定消息；
- 接受已声明参数/localparam 名称，不递归把其初始化式问题映射到本规则；
- 跨 module 名称表相互隔离；同一 module 内块或 function/task 的运行时同名遮蔽不会覆盖参数名称豁免；
- 宏、未知名称、包/类/层次限定名称和未证明的系统查询不会被猜成违规或合规。

对外编号保持官方三段式 `A-12-1`；`GJB10157.9.3.6` 仅作为内部追溯键。

## 未知边界

- 原生运行函数的字符串分组键如何由前端形成尚未完全恢复；产品以 owning module 近似；
- 完整宏/include/generate 展开、错误恢复、调用/系统函数 AST 降级及参数覆盖仍未知；
- 产品以源码词法作用域保守近似厂商前端分组，不能证明完整前端等价；
- 对无法绑定的名称，无告警仍保持 `ManualReviewRequired`。

因此本轮状态为“证据支持的核心谓词 + 已接入源适配器”，完整厂商算法等价仍不成立。
