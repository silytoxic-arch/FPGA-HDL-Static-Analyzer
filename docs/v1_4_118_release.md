# V1.4.118 R-6-4 常量来源标志与逐节点诊断

本版本只推进规则逆向、判断逻辑和验证，不调整界面或表格布局。对外规则编号继续严格采用官方三段式 `R-X-X` / `A-X-X`。

## 原生证据闭环

- `FUN_1806016c0` 只处理 `+0x178 != 0` 的 `AST_LOCALPARAM(0x0a)`。
- VHDL 语法动作 `FUN_1801b2770` 为同一名称创建 `AST_LOCALPARAM(0x0a)` 和可选的 `AST_VARASGN(0xa2)`，无条件把前者 `+0x178` 置 1；没有初始化时销毁赋值节点。
- 因此 `+0x178` 是该 VHDL constant 产生式的来源标志，不是 deferred constant 独有标志；零子节点常量会被原生规则直接加入认可集合。
- 通用递归收集器 `FUN_1804f3870` 深度优先访问每个子节点，对每个匹配 AST 类型分别追加指针，没有名称去重。
- `R-6-4` 赋值分支逐项遍历该收集结果并报告。因此同一个拒绝常量在 RHS 出现两次，会产生两条位置和消息相同的赋值诊断。

证据：`docs/evidence/native_vhdl_constant_static_initializer_4_6_c_findings.md`、`reverse-tools/evidence/r6_4_field_178_writers.txt`、`reverse-tools/evidence/r6_4_recursive_collector.txt`；附件 DLL SHA-256 仍为 `60e6b52875f4d7c3fbdf494d869b3a4bcc3fdc8ee63d8620ea9614da5d6599b0`。

## 产品修正

- 没有 `:=` 初始化的 VHDL constant 从 Unknown 改为与原生零子节点分支一致的认可状态。
- package specification 的 deferred constant 可作为同一根后续常量的认可依赖。
- package body 中的完成声明仍在独立根重新判断；不会把 package specification 状态跨根复制。
- `R-6-4` 加入全局诊断去重例外，保留赋值 RHS 每个拒绝标识符 AST 节点的原生报告重数。
- 未解析名称、用户 subtype、跨 package 绑定和用户函数继续保持 `ManualReviewRequired`，不因本次修正伪判合规。

## 新增验证

- package specification 的 deferred constant 后跟依赖它的 constant：0 条问题。
- integer signal 的 `<=` RHS 使用认可的 deferred constant：0 条问题。
- package specification 声明 deferred constant、package body 用运行期 signal 完成它：package-body 根报告 1 条。
- 动态常量声明后，integer signal 的 RHS 出现两次同名拒绝常量：声明 1 条、赋值 2 条，共 3 条。

## 仍未知

`+0x178` / `+0x109` / `+0x10e` 的正式字段名、`+0x10e` 常量折叠的完整生产路径、用户 subtype 的解析后整数标志、嵌套同名作用域、跨 package 可见性、纯函数/用户函数、包限定名称、重载、配置/生成结构及错误恢复仍未完全恢复。产品不以无报告宣称合规，完整厂商算法等价仍为 0/209。

## 发布验证

- 完整测试：`135/135` 通过（含本轮 4 个 `R-6-4` 正反例与重数边界用例）。
- 附件审计：544 条厂商记录、537 个规则页、209 条标准规则、446 个示例；435 个 `Automated:matched`、6 个 `ProjectContext:matched`、4 个 `context-required`、1 个 `native-documentation-conflict`。
- 实施台账：209 条核心判据均已接入；完整厂商算法等价验证仍为 `0/209`，未作超证据声明。
- 公开编号：209/209 条规则记录均匹配官方三段式 `^[RA]-[0-9]+-[0-9]+$`；同一官方索引可对应多条原始记录，因此不以唯一编号数代替记录数。
- 从发布 ZIP 全新解压后，CLI 版本确认为 `1.4.118`，应用包通过 macOS 深度严格签名校验。
- 全新解压样例：deferred constant 为 0 条 `R-6-4`，重复拒绝标识符为 3 条，package-body 动态完成为 1 条；三组均为 0 个分析错误，全部输出编号均为三段式。
- 发布包：`releases/1.4.118/FPGA-HDL-Static-Analyzer-1.4.118-Darwin-arm64.zip`
- SHA-256：`b3ac5aa8624b3140c14d53608990a6f1774a9d266167a3afa32afb1a6636b25b`
