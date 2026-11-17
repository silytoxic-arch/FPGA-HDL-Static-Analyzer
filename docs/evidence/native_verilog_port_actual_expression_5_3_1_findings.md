# GJB10157.5.3.1（R-3-1）端口实参表达式原生恢复

## 已观察事实

- 附件 `T-Linter.dll` SHA-256 为 `60e6b52875f4d7c3fbdf494d869b3a4bcc3fdc8ee63d8620ea9614da5d6599b0`。
- 规则构造函数为 `0x180021ca0`，运行入口为 `FUN_180542990`。
- 虚表选择器 `FUN_18071a060` 只准入 `AST_CELL(0x4f)`。
- 运行入口只检查每个 cell 的直接 `AST_ARGUMENT(13)` 子节点；空实参没有首子节点，不进入诊断。
- 对每个有首子节点的 argument，只允许首子节点类型 `AST_CONSTANT(16)`、`AST_IDENTIFIER(19)` 或 `AST_PREFIX(20)`；其他类型各自报告一条 Warning，没有首条终止或名称去重。
- 文件和物理行来自实参首子节点的 `+0x1e0`、`+0x210`；没有观察到列号。
- PE 原始字节按 GB18030 解码得到固定正文 `端口例化时禁止使用表达式`。

以上是离线静态分析事实，不是原工具运行观测。

## 产品接入

- 命名和位置端口关联都逐 argument 判断；同一实例同一物理行上的多个表达式保留多条相同正文诊断。
- 常量、普通标识符、层次标识符、索引/范围选择及空连接不报告。
- 已明确识别的一元、二元、拼接、三目及调用形态报告；不支持的源码形态不伪判。
- 诊断使用原生固定正文、Warning、实参首 token 物理行和第 1 列。

## 证据

- `ev_efbd0c9b79d5a3d8a287fcabac46eb74e69b3fbc052895287945719c4057ea1a`
- `ev_2931001d50d0a817c327e36f2110c5a6b8262d6ef85f7f36049a336d94f9a687`
- `ev_512c915b90a165536016fafbb5a5ded6cfba13c0084ad97b3576b7906ee5547b`
- `reverse-tools/evidence/t_linter_pass64/r3_1_port_actual.txt`，SHA-256 `482e63c3ff0f6a1f8a5a8e45a91e1b21c2775ee10ad4b06f8f141c4c7f75744b`

## 仍未知

- 宏、include、generate 和异常语法的完整专有 AST 构造；
- argument 建立前的完整常量折叠；
- 所有会降低为 `AST_PREFIX` 的专有源码形态；
- 跨模块键及生成 cell 的原生诊断全局顺序。

因此无告警路径继续保持 `ManualReviewRequired`，完整专有前端等价仍不声明完成。

