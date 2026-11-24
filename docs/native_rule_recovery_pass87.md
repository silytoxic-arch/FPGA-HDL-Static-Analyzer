# 第八十七轮：R-6-2 单边沿过程起始行

## 原生位置生产链

静态目标与第八十二轮相同，未运行附件。

1. `FUN_1800dcc60` 在准备上下文位置四元组后，调用
   `FUN_1808d8dc0(...,0x52,...)` 创建 AST_ALWAYS。
2. `Always_constructContext` 虚表 +0xa8/+0xb0 对应
   `FUN_1807c4b70`/`FUN_1807fad60`，分别返回上下文 +0x30/+0x38。
3. `CommonToken` 虚表 0x180f2c360 的 +0x18/+0x20 也是这两个访问器。
   上游 ANTLR 4.9.3 的 ParserRuleContext::getStart/getStop 与
   Token::getLine/getCharPositionInLine 顺序和访问形态为语义映射佐证。
4. `FUN_1800dcc60` 调用上下文 +0xa8，再调用返回 token 的 +0x18，写入
   DAT_18133c1e8。构造器将该值复制到 uint 参数对象的 [0x80]，即 +0x200。
5. R-6-2 单边沿报告从候选过程 +0x200 取行号，不包含原生列。

因此对成功解析的直接源码 always，证据支持 +0x200 为上下文起始 token 行。
这不推导 +0x210 的统一语义，也不证明所有宏展开/生成节点的位置等价。

## 产品接入

单边沿报告改用 always 关键字的物理行与产品第 1 列；精确正文延续第八十六轮。
新增 always 在第 2 行、赋值在第 4 行的回归，断言报告为第 2 行。
多边沿候选的具体节点归属仍未恢复，保留现有位置代理。

## 来源

- /private/tmp/tlinter-r62-pass87-candidate.txt SHA-256：
  3d7378fca32db5a90de688db094e6f62bd459e6073f529aafd5dee1d9deae393
- /private/tmp/tlinter-r62-pass87-token-accessors.txt SHA-256：
  6887773aed818ff66d36734d6859daddf1d19dffe03c3515b364d7c17ac18134
- /private/tmp/tlinter-r62-pass87-token-vtable.txt SHA-256：
  c4cd7e1e1ba9993de1af929f89550f01379a2e9f551c957bba4773efde9f903a
- 构造器摘录和哈希见第八十六轮。
- 上述 always 生产者、构造器、token 访问器和 token 虚表的可复核副本保存在
  `reverse-tools/evidence/t_linter_pass87/`（规范化文件末尾空白，地址及伪代码保留）。
- https://github.com/antlr/antlr4/blob/4.9.3/runtime/Cpp/runtime/src/ParserRuleContext.h
- https://github.com/antlr/antlr4/blob/4.9.3/runtime/Cpp/runtime/src/Token.h

完整多边沿状态、局部名称遮蔽、选择左值、专有前端降低仍需恢复。
规则继续部分接入；完整算法等价为 0/209。
