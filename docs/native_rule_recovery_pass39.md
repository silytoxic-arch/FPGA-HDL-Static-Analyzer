# 第三十九轮：GJB10157 A-12-1 repeat 平面参数名称表

原生 `T-Linter.dll`（SHA-256 `60e6b52875f4d7c3fbdf494d869b3a4bcc3fdc8ee63d8620ea9614da5d6599b0`）中内部条款 `GJB10157.9.3.6` 的官方编号是 `A-12-1`。`FUN_18058aa70` 对每个非空 `AST_REPEAT(0x5e)` 的第一个 child 递归收集全部 `AST_IDENTIFIER(0x13)`，再用 `FUN_180529750` 与同组 parameter/localparam 名称向量做区分大小写的完整字符串比较。

进一步核对确认该参数向量只保存名称，没有词法作用域或声明身份；消费者也没有先证明 identifier 已绑定为运行时对象。因此直接未知名称 `repeat(unknown_count)` 会报告，而 module 参数 `N` 与块内运行时变量同名时，`repeat(N)` 仍因名称命中平面参数表而不报告。

产品原先采用更严格的词法解析，恰好与原生结果相反：漏报未知名称，并误报参数同名遮蔽。本轮改为每个 owning module 建立平面 parameter/localparam 名称表，保留逐 identifier 出现的诊断重数，并新增未知名称、块遮蔽、function 参数遮蔽及嵌套 localparam 边界测试。

厂商分组键、宏/include/generate 完整展开、限定名称、系统查询和调用的 AST 降级仍未完全恢复；产品对这些未证明形态保持人工复核，无告警不判定合规。
