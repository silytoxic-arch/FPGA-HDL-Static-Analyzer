# GJB9765.4.6.b.2 VHDL case 位宽规则恢复记录

## 目标与二进制身份

- 目标规则：`GJB9765.4.6.b.2`
- 附件版本：T-Linter V3.1.4.25120520251205115359
- `T-Linter.dll` SHA-256：`60e6b52875f4d7c3fbdf494d869b3a4bcc3fdc8ee63d8620ea9614da5d6599b0`
- 构造函数：`0x1800289b0`
- slot 0：`FUN_180600da0`（`0x180600da0`）
- 宽度收集器：`FUN_180600050`（`0x180600050`）
- case 比较器：`FUN_180601490`（`0x180601490`）
- 本记录来自 Ghidra 12.1.4 对用户附件的离线静态反编译，不代表观察了原软件运行。

## Recovered predicate

- `FUN_180600da0` 遍历规则对象保存的设计根节点；每个根节点先由 `FUN_1805fefc0` 清空六组名称/位宽映射，再由 `FUN_180600050` 递归收集声明与常量宽度，最后调用 `FUN_180601490` 比较 case 节点。
- `FUN_180600050` 对多个声明 AST 类别计算静态宽度；名称统一转为小写。它还记录名称是否精确等于 `signed`，证明映射条目除宽度外还保存类型属性，但本规则比较分支只读取宽度。
- 名称宽度最终由共享查询器 `FUN_1805fee80` 读取；结合 `FUN_180600050` 的生产分支，五类表的固定顺序已经确认为 `localparam/constant > entity port > entity generic > signal > variable`。这不是源码中最后出现的同名声明覆盖全部类别。
- `FUN_180601490` 只处理类型 `0x57`、偏移 `+0x183/+0x184` 两个状态字节均为零且直接子节点数大于一的 case 节点；否则递归检查子节点。
- case 选择表达式只在直接子节点类型为 `0x13` 时调用名称宽度查询，即只接受可解析的简单名。
- 对每个类型 `0x58` 且至少有两个子节点的 choice 组，只检查第一个子节点：
  - 直接类型 `0x13`：按名称查询宽度；
  - 类型 `0x0e` 包装且其第一个子节点为类型 `0x10`、状态字节 `+0x109` 为零：直接用该常量节点的范围差计算宽度；
  - 其他形态保持未知。
- 只有 selector 与 choice 两侧宽度均不是 `-1` 且互不相等时，才把当前 choice 记录加入诊断集合。未知宽度、等宽、切片/索引或复杂表达式不由该分支报告。

## 产品接入

- VHDL case 宽度检查限定为直接简单名 selector；允许前端会消去的完整包围括号，如 `case (A) is`，但排除 `A(0)` 与 `A(7 downto 0)`。
- 直接 choice 支持字符串字面量以及根级五类宽度表中可解析的直接名称；architecture 绑定所属 entity 的固定宽度 vector generic，完整根内的局部 constant/signal/variable 与子程序形式参数参与解析。
- package body 作为独立设计根建立自己的五类宽度视图，其子程序 CASE 与后续 architecture 相互隔离。
- 同名跨类别对象按原生固定顺序解析，而不是按源码遮蔽顺序猜测。
- 两侧宽度都已知且不等时逐 choice 报告；未知宽度不猜测。
- 附件违反样例现可分别报告两个 boolean 命名常量和两个 3 位字符串，共四个具体位置；附件遵循样例保持零报告。

## 已知边界

- `0x0e/0x10/0x13/0x57/0x58` 的完整枚举名与 case 状态字节的全部前端生产语义尚未恢复；五类名称宽度表的生产类别和固定查询顺序已由共享收集器/查询器交叉确认。
- 产品尚未完整解析 subtype、枚举、qualified expression、aggregate、别名、包常量、生成域和重载；这些情况保持未知或人工复核。
- 产品是有界源级适配器，不宣称完整前端 AST、诊断顺序或原生算法等价。

## 回归覆盖

`tests/standard_accuracy_tests.cpp` 覆盖不等宽命名 boolean 常量、不等宽字符串、等宽命名 vector 常量、切片 selector 排除、逐 choice 报告、所属 entity vector generic selector、process 局部 variable/constant、package-body 子程序 CASE、`port > variable` 和 `constant > port`；`tests/analyzer_tests.cpp` 与 CLI fixture 覆盖直接 selector、切片和索引边界。附件 446 个正反例继续由版本审计台账逐项验证。
