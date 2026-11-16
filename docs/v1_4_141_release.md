# V1.4.141 R-9-2 类别 1 静态动作

本版本继续推进规则逆向和判断准确性，不调整界面或表格。对外规则编号继续严格使用官方三段式 `R-X-X` / `A-X-X`。

## 原生证据

`R-9-2` 与 `R-9-4` 共用 `FUN_1804d56f0` 关系构建器。类别 1/4 生产器 `FUN_1804d24a0` 通过 `FUN_1804d20e0` 判断时钟边沿之前的异步动作是否静态：

- 递归进入 `AST_BLOCK(0x54)`，只检查恰有两个子节点的 `AST_ASSIGN_LE(0x56)` / `AST_VARASGN(0xa2)`；
- RHS 为直接 `AST_CONSTANT(0x10)` 时接纳；
- RHS 为 `AST_CONCAT(0x22)` 时，首个直接子节点是 `AST_OTHERS(0x89)`，或全部直接子节点都是常量时接纳；
- RHS 为单一已绑定 `AST_IDENTIFIER(0x13)` 时，名称必须存在于每个设计根重新建立的大小写折叠命名值树；
- `FUN_1804cbad0` 只用所属实体泛型、当前 architecture 的直接常量和直接类型枚举文字建立该树；普通信号、process 局部常量和局部 package 常量不属于该集合。

这些结论来自附件 `T-Linter.dll` 的离线静态反编译，不代表运行附件程序。DLL SHA-256 为 `60e6b52875f4d7c3fbdf494d869b3a4bcc3fdc8ee63d8620ea9614da5d6599b0`。持久化证据为：

- `docs/evidence/native_vhdl_single_clock_process_4_9_d_constant_helpers.txt`；
- `docs/evidence/native_vhdl_single_clock_process_4_9_d_constant_set_builder.txt`；
- `docs/evidence/native_vhdl_single_clock_process_4_9_d_constant_set_references.txt`。

## 产品接入

- `R-9-2` 类别 1 的直接 ELSIF 与 ELSE 链接 IF 路径改用完整的已恢复静态动作子集；
- 新增 architecture 作用域的命名静态值收集，并跳过局部 package、function 和 procedure 声明体；
- 泛型、architecture 直接常量、枚举文字、全字面量拼接及首项 `others` 聚合可建立类别 1 时钟关系；
- process 局部常量、局部 package 常量、普通动态信号仍归入不被本规则消费的类别 4；
- V1.4.140 的条件边沿先序、类别 3/4 排除、关系行排除、控制候选去重和 WHEN 分支保持不变。

## 新增验证

- 所属实体泛型：类别 1 正例；
- architecture 直接常量：类别 1 正例；
- architecture 直接枚举文字：类别 1 正例；
- 全字面量拼接：类别 1 正例；
- 首项 `others` 聚合：按原生短路分支接纳；
- process 局部常量：排除，保持类别 4；
- 局部 package 常量：排除，保持类别 4；
- ELSE 链接 IF 配合泛型静态动作：类别 1 正例。

别名、generate/configuration、包限定或扩展名称、其他嵌套动作、错误恢复和完整层次传播仍未恢复；相关无告警继续保持人工复核，不声明完整原生等价。

## 发布验证

- 完整测试：135/135 通过；
- 附件回归审计：544 条附件记录、537 个打包页面、209 条标准规则、446 个示例；结果为 435 个 `Automated:matched`、6 个 `ProjectContext:matched`、4 个 `context-required`、1 个 `native-documentation-conflict`，相对 V1.4.140 无回退；
- 实施台账：209/209 条核心谓词有接入记录，完整厂商算法等价仍为 `0/209`；台账 SHA-256 为 `708d337108dc83215347efab3c4cae624ac13a4bddacf2d4828c21b258eda2ac`；
- 全新目录解压验证：CLI 版本为 `1.4.141`，macOS 深度严格签名校验通过；所属实体泛型静态动作样例得到 1 条 `R-9-2`，无分析错误；
- 解压报告中的违规和标准评估编号全部满足 `R-X-X` / `A-X-X`，未输出内部 `GJB9765.x.x` 条款编号。

## 发行包

- 文件：`releases/1.4.141/FPGA-HDL-Static-Analyzer-1.4.141-Darwin-arm64.zip`
- SHA-256：`37119b04de7a29b143349682b9913927ad66e1c26107ea2438e045d245d43b11`
