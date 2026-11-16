# V1.3.10 规则集进展

本版只推进规则准确性，没有调整问题列表、代码规模明细或其他表格格式。

## 新增自动规则

- `GJB9765.4.5.a`：恢复原生短标识符收集范围，覆盖类型、子类型、信号、变量、常量、端口和泛型。
- 使用从 `T-Linter.dll` 恢复的完整 IEEE 1076-2008 VHDL 关键字词表。
- VHDL 名称按大小写不敏感方式比较。
- 原生二进制中的 Verilog、SystemVerilog、SDF/EDIF、PSL、Windows 可选词表未在缺少附件配置证据时默认开启，避免跨语言误报。

## 验证

- 七类对象正例、合法反例、大小写边界和跨语言词表边界均加入自动测试。
- 全量 `ctest`：131/131 通过。
- 附件 446 个正反例审计结果见 `docs/evidence/t_linter_after_1_3_10.md`。

详细逆向证据见 `docs/evidence/native_vhdl_keyword_collision_4_5_a_findings.md`。
