# V1.4.121 R-6-5 设计单元根隔离

本版本只推进规则判断逻辑与验证，不调整界面或表格布局。对外规则编号继续严格采用官方三段式 `R-X-X` / `A-X-X`。

## 原生依据

`R-6-5` 对应的原生入口 `FUN_180603930` 对每条输入记录取首个 AST 根，在调用递归收集器 `FUN_180604790` 前清空五组内部容器。因此，不同 design unit 根的类型声明和赋值记录不得相互污染。

证据：`reverse-tools/evidence/r6_5_root_and_assignment_walkers.txt`、`docs/evidence/native_vhdl_implicit_numeric_conversion_4_6_d_findings.md`；附件 DLL SHA-256 为 `60e6b52875f4d7c3fbdf494d869b3a4bcc3fdc8ee63d8620ea9614da5d6599b0`。

## 产品修正

- architecture 在 `end architecture;`、`end <architecture-name>;` 或裸 `end;` 处结束，不再默认延伸至文件末尾或下一个 architecture。
- function/procedure 与局部 package 的完整、具名或裸终止符优先配对，不会被误识别为 architecture 终止符。
- 后续 entity、architecture、package specification 与 package body 均作为新根建立独立类型表。
- 对不完整输入增加基于完整 design-unit 起始语法的恢复边界；直接 entity 实例化和 package 实例化不会被误识别为新根。
- 相邻 `R-6-6` 仍只在 architecture 根内执行，没有把 `R-6-5` 的 package 证据外推到其他规则。

## 新增验证

- `end architecture;` 后紧跟 package，package 中 unsigned→integer 初始化只报告 1 条 `R-6-5`。
- architecture 内子程序以裸 `end;` 结束、architecture 以 `end rtl;` 结束，后续 package body 的 unsigned→integer 赋值只报告 1 条。
- 两组 entity/architecture 交错且使用同名对象，第二根同类型赋值为 0 条，证明前一根的 unsigned 类型不会泄漏。

## 仍未知

显式转换的专有 AST 降级、用户 subtype、包限定类型、函数返回类型、重载、数组元素/记录字段、跨文件与层级绑定、别名、错误恢复及原生诊断重数仍未完全恢复。产品不以无报告宣称合规，完整厂商算法等价仍为 0/209。

## 发布验证

- 完整测试：`135/135` 通过（含 3 个新增 `R-6-5` 设计单元根边界用例）。
- 附件审计：544 条厂商记录、537 个规则页、209 条标准规则、446 个示例；435 个 `Automated:matched`、6 个 `ProjectContext:matched`、4 个 `context-required`、1 个 `native-documentation-conflict`。
- 实施台账：209 条核心判据均已接入；完整厂商算法等价验证仍为 `0/209`，未作超证据声明。
- 公开编号：209/209 条规则记录均匹配官方三段式 `^[RA]-[0-9]+-[0-9]+$`。
- 从发布 ZIP 全新解压后，CLI 版本确认为 `1.4.121`，应用包通过 macOS 深度严格签名校验。
- 全新解压样例：architecture 后 package 转换为 1 条 `R-6-5`；具名 architecture 终止符和内部子程序裸终止符样例为 1 条；交错 entity/architecture 同名对象隔离样例为 0 条。三组均为 0 个分析错误，全部输出编号均为官方三段式。
- 发布包：`releases/1.4.121/FPGA-HDL-Static-Analyzer-1.4.121-Darwin-arm64.zip`
- SHA-256：`727c912554dac3565babb32efdf7e345cbfec88caa3c5d22ea27a1d538052f6f`
