# V1.4.119 R-6-4 boolean 类型门控恢复

本版本只推进规则逆向、判断逻辑和验证，不调整界面或表格布局。对外规则编号继续严格采用官方三段式 `R-X-X` / `A-X-X`。

## 原生证据闭环

- 全映像指令扫描找到 15 处立即数/字段引用，其中只有 `FUN_1801c2660` 对节点 `+0x10e` 写入实值 1，`FUN_1808d9a90` 只复制该字段，其余位置均为读取或无关立即数。
- `FUN_1801c2660` 将类型名转为小写并逐项匹配；`boolean` 命中时设置 `+0x10e = 1`。同函数相邻字段明确匹配 `integer`、`bit_vector`、`std_logic_vector`、`unsigned`、`signed`、`real`、`character`、`string`、`natural` 和 `positive`。
- 因此 `+0x10e` 是 boolean 类型标志，不是此前台账中暂定的“常量折叠标志”。
- `FUN_1806016c0` 对单子节点常量先接纳 `AST_CONSTANT(0x10)`，再接纳 `child[+0x10e] == 1`，之后才进入 `AST_IDENTIFIER(0x13)` 和复合表达式的依赖检查。
- 结果是 boolean 类型的初始化式不受本条“固定整型值”依赖判断约束，即使它读取运行期 boolean 对象或关系表达式中的运行期整数。

证据：`reverse-tools/evidence/r6_4_boolean_flag_producers.txt`、`docs/evidence/native_vhdl_constant_static_initializer_4_6_c_findings.md`；附件 DLL SHA-256 为 `60e6b52875f4d7c3fbdf494d869b3a4bcc3fdc8ee63d8620ea9614da5d6599b0`。

## 产品修正

- 直接声明为 `boolean` 的 VHDL constant 在 `R-6-4` 状态表中按原生分支加入认可集合，不再扫描其初始化式中的运行期依赖。
- 只接入直接 `boolean` 类型拼写。用户 subtype、别名、包限定类型和重载后的类型标志传播仍保持 `ManualReviewRequired`，不把未知路径伪判为已验证。
- integer 常量的动态依赖、deferred constant、拒绝状态传播和整数 `<=` 赋值二次定位保持不变。

## 新增验证

- `constant Cbool : boolean := runtime_bool`：0 条 `R-6-4`。
- `constant Crel : boolean := runtime_value > 0`：0 条 `R-6-4`。
- 既有 `constant Cint : integer := runtime_value + 1` 仍为 1 条 `R-6-4`，防止类型门控放宽到整数路径。

## 仍未知

`+0x178` / `+0x109` / `+0x10e` 的正式字段名，boolean/integer 标志通过用户 subtype、别名、包限定类型和重载解析后的完整传播路径，嵌套同名作用域、跨 package 可见性、纯函数/用户函数、配置/生成结构及错误恢复仍未完全恢复。产品不以无报告宣称合规，完整厂商算法等价仍为 0/209。

## 发布验证

- 完整测试：`135/135` 通过（含本轮 2 个 boolean 类型门控边界用例和既有 integer 反例）。
- 附件审计：544 条厂商记录、537 个规则页、209 条标准规则、446 个示例；435 个 `Automated:matched`、6 个 `ProjectContext:matched`、4 个 `context-required`、1 个 `native-documentation-conflict`。
- 实施台账：209 条核心判据均已接入；完整厂商算法等价验证仍为 `0/209`，未作超证据声明。
- 公开编号：209/209 条规则记录均匹配官方三段式 `^[RA]-[0-9]+-[0-9]+$`。
- 从发布 ZIP 全新解压后，CLI 版本确认为 `1.4.119`，应用包通过 macOS 深度严格签名校验。
- 全新解压样例：运行期 boolean 标识符为 0 条 `R-6-4`，boolean 关系表达式为 0 条，运行期 integer 表达式仍为 1 条；三组均为 0 个分析错误。两个无报警 boolean 样例的规则状态仍为 `manual-review-required`，未伪判合规；全部输出编号均为三段式。
- 发布包：`releases/1.4.119/FPGA-HDL-Static-Analyzer-1.4.119-Darwin-arm64.zip`
- SHA-256：`5332a09b94f6622d19f353634a4ee01bb2efd01a5aee992f9acd4aa48494590e`
