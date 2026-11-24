# 第八十六轮：R-6-2 位置来源与精确单边沿正文

沿用同一 T-Linter.dll 映像，静态分析未执行目标。

## 新观察

- AST 构造器 `FUN_1808d8dc0` 的首参数为 uint 指针。它将全局
  `DAT_18133c1e8` 写入 `param_1[0x80]`，即节点字节偏移 +0x200；随后三个
  32 位全局位置值写入 +0x204/+0x208/+0x20c。
- +0x210 另由一个函数指针调用的返回值写入，和上述四元组不是同一生产路径。
- parser visitor 的多个位置生产者通过上下文的虚方法 +0xa8/+0xb0 取得两个对象，
  再通过它们的 +0x18/+0x20 虚方法构建上述全局四元组。仅凭这些偏移尚不能给
  所有虚方法赋予确定语义名称。
- 原生 RTTI 定位 Always_constructContext 虚表 0x180e16ce0。其 +0x28 方法
  FUN_180085710 动态转换到 Verilog2005ParserVisitor 后调用 visitor +0x5e0。
  上下文类型识别和派发偏移已建立；实际 AST always 生产者尚未绑定。
- 对 AST 构造器直接调用的窄窗口 type=0x52 搜索为空。这不是不存在生产者的证明，
  可能经过包装或动态类型参数。

## 本轮接入

单边沿 R-6-2 正文改为已恢复的原生固定拼接：
`触发器<名称>无异步复位或置位`。测试对 Q3 精确正文和 Error 严重度作断言。
产品位置继续采用既有赋值位置代理；尚不声明它与原生过程 +0x200 等价。

## 静态证据哈希

- /private/tmp/tlinter-r62-pass86-constructor.txt：
  242c7c69f4f35f275c2fc78905255a48c7fdc251df4e93bead1649b3bd377a9f
- /private/tmp/tlinter-r62-pass86-global-refs.txt：
  9aa20288902ba9656e33f55ce1a7b9e431684f09b93495efd677021d651898d6
- /private/tmp/tlinter-r62-pass86-more-producers.txt：
  bd8097c5043ddc88f6e81e90488ff95c3270aa264028524d7d9e3bfa2fe4f408

上下文派发和虚表原始摘录保存在同前缀的 context-functions、always-vtable 和
context-full-vtable 文本中。实际生产者、虚方法语义、多边沿状态和完整专有前端
仍未恢复；规则保持部分接入，完整算法等价仍为 0/209。
