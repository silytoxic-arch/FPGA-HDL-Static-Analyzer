# V1.4.100 A-5-29 设计根命名值恢复

## 新恢复的原生边界

- `FUN_1804cbad0` 在每个设计根分析前建立不区分大小写的命名值树。
- 生产来源包括直接 `AST_GENERIC` 中的已绑定标识符、`AST_ARCHITECTURE` 的直接 `AST_LOCALPARAM`，以及 architecture 本地 `AST_TYPE` 中的枚举标识符。
- `FUN_1804d20e0` 只在赋值 RHS 是单一、已绑定的 `AST_IDENTIFIER` 且名称存在于该树中时，把命名 RHS 视为支持的常量赋值。
- process 局部常量不属于该生产器的直接遍历范围；每个设计根开始前集合会清空，因此其他 architecture 的声明不会泄漏。

## 产品修正

在 V1.4.99 已闭合的类别 2 结构内，新增以下 RHS：

- 当前 entity 的 generic；
- 当前 architecture 声明区的 constant；
- 当前 architecture 声明的枚举类型文字。

普通信号、process 声明区常量、其他 architecture 的常量均不接纳。直接字面量同时收窄为完整的单一 RHS，复合表达式首项不再被误当作常量。

## 回归覆盖

- generic、architecture constant、architecture enum literal 三个正例；
- 普通信号、process 局部常量、跨 architecture 常量三个反例；
- V1.4.99 的裸下降沿、非首项嵌套 IF、ELSIF 借用、分支外赋值和普通数据传送边界继续保留。

## 官方编号

本规则对外编号为 `A-5-29`。全部 209 条规则继续采用官方三段式 `R-X-X` / `A-X-X`；内部 GJB 条款只用于绑定和追溯。

## 仍未知

多个分支块的全部前端来源、所有聚合命名值、别名/层级传播、生成结构、外部后缀配置和原生诊断次数仍未完全恢复。完整厂商算法等价保持 0/209。

## 发布验证

- 规则准确性专项测试通过，完整测试为 135/135。
- 附件审计保持稳定：544 条产品规则页、537 个已打包页面、209 条标准规则、446 个例程；其中 435 个自动匹配、1 个原生文档冲突、6 个工程上下文匹配、4 个必须依赖工程上下文。
- 209/209 条标准规则的公开编号均符合官方三段式 `R-X-X` / `A-X-X`。
- 发布包：`releases/1.4.100/FPGA-HDL-Static-Analyzer-1.4.100-Darwin-arm64.zip`。
