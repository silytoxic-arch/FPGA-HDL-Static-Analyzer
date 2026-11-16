# 第二十二轮：GJB10157.5.3.1

附件 T-Linter.dll 原生函数 `0x180542990` 对 AST_ARGUMENT 首子节点检查：AST_CONSTANT(16)、AST_IDENTIFIER(19)、AST_PREFIX(20) 不报，其他类型报 Warning。依据 `ev_efbd0c9b79d5a3d8a287fcabac46eb74e69b3fbc052895287945719c4057ea1a` 和类型映射 `ev_512c915b90a165536016fafbb5a5ded6cfba13c0084ad97b3576b7906ee5547b`。产品通过解析器有界实参形态接入，附件错误例报 3 处、正确例无报警；边界测试覆盖常量、标识符、选择、空连接、一元/二元/拼接/三目及括号。详细记录见同工作区 `reverse-tools/evidence/t_linter_pass3/port_actual_expression_5_3_1_findings.md`。

209 条台账：35 条核心判据部分接入，2 条模型判据已观察未接入，174 条核心判据待解释，完整算法等价验证 0 条。未知语法、原生 AST 准入与折叠行为仍保留人工复核。
