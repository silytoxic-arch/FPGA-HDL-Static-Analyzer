# GJB9765.4.14.b.4 嵌套 IF 缩进判据（部分恢复）

目标附件：`T-Linter.dll`，SHA-256 `60e6b52875f4d7c3fbdf494d869b3a4bcc3fdc8ee63d8620ea9614da5d6599b0`。

## 原生静态观察

- `FUN_1805c32c0` 遍历带条件结构标记的 AST，建立包含 `if`、`elsif`、`else` 关键字及源码行的分支树。证据 `ev_37e19937fe90d98240c2159037ca786ce936b9e7211afb729ee6b0873675f7d8`。
- `FUN_1805c4af0` 在一个分支的后代中递归寻找嵌套条件结构，找到首个后写入父记录的单一子指针并停止；同一分支不会继续链接第二个兄弟嵌套 IF。证据 `ev_d7422141e14ebceda8bcf2a88688e89dbcdea92477af9ab085c8902f049d508c`。
- 规则入口 `FUN_1805c3e60` 逐行读取源码、截断 `--` 注释并定位记录的关键字。若已链接子 IF 的显示列小于或等于父分支关键字列，报告 Warning。判据是“子级列必须严格更大”，并不要求恰好增加一个 Tab。证据 `ev_4bcab576fca1868e7a0d2e63b68be85dcb6153133d132da8a59c89be50b57e0d`。
- `FUN_1805c4920` 使用 `strstr` 定位关键字，Tab 推进到下一个 4 列制表位并返回零基显示偏移。证据 `ev_016bfbc44bffe9b8849d844cb85929b760af4d72d169ba3b57927984afafb671`。

完整本轮 REA Evidence bundle 保存于 `docs/evidence/rea_vhdl_nested_if_4_14_b_4_bundle.json`。

## 产品接入及限制

产品为顺序 VHDL `if`/`elsif`/`else` 建立分支栈，每个分支只比较首个嵌套 IF；显示列同样按 4 列制表位换算。附件违反例在未缩进的第二层 IF 处产生一条告警，遵循例无告警。测试另覆盖 elsif/else、单列加深、第二兄弟 IF 和 if-generate 边界。

原生条件标记及 AST 类型 `0xab`、`0xac`、`0x58` 的权威枚举名称尚未确认；原生同名子串定位、异常 AST、其他条件构造和重复抑制细节也未完全恢复。因此只标记核心谓词部分接入；无告警仍为 `ManualReviewRequired`，不宣称完整算法等价。
