# 第九十九轮：R-6-3 主表达式转交与事件形态

目标 SHA-256：`60e6b52875f4d7c3fbdf494d869b3a4bcc3fdc8ee63d8620ea9614da5d6599b0`。
静态分析，未执行 T-Linter。原始摘录见 `reverse-tools/evidence/t_linter_pass99/`，
末尾空白已规范化。

## 原生观察

- FUN_1800c4060 → FUN_18006aec0 返回 PrimaryContext；FUN_1800d0040 →
  FUN_18006ce40 返回 Unary_operatorContext，RTTI 转换确认其类型。
- 第九十四轮实际 Expression exit FUN_1800fc0d0：存在 Primary、没有 Unary 时
  直接转交已有 AST 指针；有 Unary 时才构造对应操作节点。普通括号不创建运算节点。
- 实际 Primary exit FUN_18011a640：FUN_1800c09f0 → FUN_18006a200 返回 Number；
  FUN_1800b58f0 → FUN_1800681c0 返回 Hierarchical_identifier；
  FUN_1800ba010 → FUN_180069180 返回 Mintypmax_expression。这些类型均由 RTTI 证实。
  Mintypmax 路径直接转交已有指针，没有重写其源行。
- Mintypmax_expressionContext 虚表 0x180e1db08 的 exit → listener +0x1440 →
  FUN_18010d130。FUN_1800ad310 / FUN_18006dcb0 收集 ExpressionContext 指针。
  数量为 1 时转交第一项，为 3 时转交第二项（typical），其余数量不建立映射。
- 第九十三轮 R-6-3 事件路径要求边沿节点恰有一个 AST_IDENTIFIER(0x13) 子节点。
  因此 `posedge RST & EN` 的根操作节点不能凭首个名字 RST 入选。
- 第九十四轮 Primary 生产者为带选择范围的标识符保留/新建 0x13，并复制原始
  名称 +0x48；R-6-3 只比较此名称，不比较选择范围。不能把范围名义上不同推断为
  本条规则必然区分不同位。其它位级安全性仍需独立规则判断。

## 产品接入

在完整括号范围内转交表达式，min:typ:max 选择中项；区分三元条件中的冒号，
不将其误当 min/typ/max 分隔。比较左右与 ! 操作数都使用该映射，保留实际常量
或 ! 的源行。事件按完整表达式判断，只接入对应标识符节点，复合事件不再凭
首名字入选。带连续选择范围的标识符按原生名称进行该规则比较。

新增 23 个条件形态、13 个事件形态及跨行定位断言，包括正例、反例、三元表达式
边界、选择范围和复合事件排除。192 个数值比较场景继续保留。

完整综合复位关系、层次名称、参数/表达式折叠、宏与其它前端降级仍未知。
无诊断保持人工复核，完整算法等价仍为 0/209。

后续修正：保留 AST 基础名称不等于通过复位目录准入。第一百一十二轮确认宽线
片段的格式化名称带范围后缀，与基础名称精确查找不匹配。相关集成矩阵已补齐
独立标量目录前提，并将多比特切片另作负例；本轮 AST 转交观察仍保留。
