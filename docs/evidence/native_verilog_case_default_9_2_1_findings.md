# GJB10157.9.2.1 case 缺省分支判据（V1.4.38 专用适配器）

附件 T-Linter.dll SHA-256：`60e6b52875f4d7c3fbdf494d869b3a4bcc3fdc8ee63d8620ea9614da5d6599b0`。原生收集器证据 `ev_e69cc1c5a34d9cccdde7754f61613b89544679ed77ae679a1ee94a84b9dff82c` 只接纳类型 `0x57`、节点状态字节 `+0x183==0` 且子节点非空的节点；附件的相邻控制流证据将 `+0x183!=0` 的 `AST_CASE` 关联到 IF/ELSIF/ELSE 降低路径，因此本条不把源码 if 当作 case 检查。原生 slot 0 证据 `ev_8e1f2c6a9a8140cca8d40a38d0ea81fab5d70826fe87a63eb8e495f0b446e2ae` 检查 `0x58..0x5a` 子节点：首个后代为 `0x5b`，或唯一后代为 `0x54` 时视为存在回退分支；否则在 `0x57` 处输出一次 Warning。类型表证据 `ev_512c915b90a165536016fafbb5a5ded6cfba13c0084ad97b3576b7906ee5547b` 将它们标为 `AST_CASE`、`AST_COND/CONDX/CONDZ`、`AST_DEFAULT` 和 `AST_BLOCK`。

## AST 到源码形态的补证

附件含完整 Yosys 命名空间和前端字符串。Yosys 官方 `frontends/verilog/verilog_parser.y` 的 case 产生式显示：普通 `default:` 经 `case_expr_list TOK_COL` 生成 `AST_DEFAULT`，随后追加 `AST_BLOCK`；Verilog 允许的省略冒号形态 `default <statement>` 走独立 `TOK_DEFAULT` 分支，不生成 `AST_DEFAULT`，只剩随后追加的 `AST_BLOCK`。这与附件二进制的两个接受条件逐项对应。官方源码仅作为源码形态补证，附件指令仍是判据权威来源：<https://github.com/YosysHQ/yosys/blob/main/frontends/verilog/verilog_parser.y>。

## 产品接入

V1.4.38 撤销 `VL-CASE-DEFAULT` 到本条款的通用诊断映射，改由条件预处理后的专用适配器直接检查 case/casez/casex。标准 `default:` 和省略冒号的 `default <statement>` 都满足回退；空 case、标签全集但无 default、普通与通配 case 均按节点分别报告一次；嵌套 case 各自独立，源码 if 不进入本条。回归还覆盖未激活预处理分支中的 default 不得满足活动 case。

## 未恢复边界

`+0x183` 字段的正式结构成员名、Yosys 分支版本差异、生成/简化过程新建的 case、错误恢复 AST、宏展开产生的非直接位置和原生消息文本仍未完全恢复。产品在预处理失败时不猜测；无告警保持 `ManualReviewRequired`，不宣称完整原生等价或合规。
