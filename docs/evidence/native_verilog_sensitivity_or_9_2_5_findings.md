# GJB10157.9.2.5 灵敏度事件 OR 判据（V1.4.41 校准）

## 证据边界

- 目标附件 `T-Linter.dll` SHA-256：`60e6b52875f4d7c3fbdf494d869b3a4bcc3fdc8ee63d8620ea9614da5d6599b0`。
- 本记录只使用静态反编译、静态反汇编、附件示例和 Yosys 官方解析器源码作结构补证；没有运行厂商 DLL。
- 原生主函数 `FUN_180583540`：证据 `ev_165fc19c9ef8d627b96bd5d6a92697bc84caaaae45cfcbaa1c0f8d3e2d47a00e`。
- 虚表 slot 2 `FUN_180583c40`：证据 `ev_2a16e02acb25139a18a51d208245698c12f17a782f0668b01b45b260614ed9bc`。
- AST 类型映射 `FUN_1808ea310`：证据 `ev_512c915b90a165536016fafbb5a5ded6cfba13c0084ad97b3576b7906ee5547b`。

## 已观察的原生判据

1. slot 2 仅比较节点类型是否为 `0x52`，即只接纳 `AST_ALWAYS`。
2. 主函数对每个已接纳 always 的直接子节点逐一检查。子节点类型减 `0x65` 后小于 3 才进入下一步，因此只处理 `AST_POSEDGE(0x65)`、`AST_NEGEDGE(0x66)` 与 `AST_EDGE(0x67)`。
3. 对每个事件子节点调用递归收集器 `FUN_1804f3870`，目标类型数组固定为 `0x26` 和 `0x46`。独立枚举映射证明它们分别是 `AST_BIT_OR` 与 `AST_LOGIC_OR`。
4. 任一事件子树收集结果非空即设置本 always 的违规标志；扫描仍可继续，但最终只在 always 根位置发出一条 `Warning`。因此一个灵敏度表中多个 `|`/`||` 不会产生多条本规则诊断。
5. 普通事件分隔关键字 `or` 或逗号生成多个事件子节点，不是 `AST_BIT_OR/AST_LOGIC_OR`；过程体内的 OR 也不在三个事件子树内，均不触发本条。

Yosys 官方 `verilog_parser.y` 进一步说明 `always` 与 `always_ff` 都创建 `AST_ALWAYS`，差别仅为 `always_ff` 属性；其事件分别创建 `AST_POSEDGE`、`AST_NEGEDGE` 或 `AST_EDGE`。该源码只用于解释源语法到已观察枚举的对应，附件二进制仍是判据权威来源：<https://github.com/YosysHQ/yosys/blob/main/frontends/verilog/verilog_parser.y>。

## 产品映射

V1.4.41 在条件预处理后的活动源码中检查显式 `always @(…)` 和 `always_ff @(…)` 事件表达式。只在匹配的外层事件括号内部查找词法 `|` 或 `||`，按 always 根最多报告一次；注释、过程体运算符、`or`/逗号分隔和 `@*` 均不会误触发。

新增回归覆盖：

- `posedge (clk | enable)` 与普通 `a || b` 事件；
- 一个事件表含多个 OR 仍只报告一次；
- `always_ff` 的事件 OR 与过程体 OR 区分；
- 三元事件表达式内嵌 `||` 的递归后代；
- `or` 和逗号分隔、注释、`@*` 及条件编译边界。

## 仍未知

- 附件所用具体 Yosys 分支对宏生成、错误恢复及极端事件语法的精确 AST。
- 属性包围事件、跨 include 的完整预处理变换及原生消息正文。
- 源码解析失败时原生程序是否仍保留部分 AST；产品在预处理失败时不猜测。

因此本条已接入证据支持的核心事件子树判据，但无告警结果仍为 `ManualReviewRequired`，不宣称完整原生等价。

## V1.4.41 验证

- 新增的 9.2.5 专项正例、反例与边界测试通过。
- 完整 CTest 135/135 通过。
- 全量附件审计保持稳定：446 个示例中 435 个自动匹配、1 个已知自动差异、4 个工程上下文匹配、6 个需要工程上下文。
