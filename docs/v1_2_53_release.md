# V1.2.53 GJB9765.4.14.b.2 行尾空格规则

## 本轮恢复与修正

- 附件原生函数检查 `空格 + LF` 和 `空格 + CRLF` 两种块尾形式。
- 标准规则改用独立判断；通用 `STYLE-001` 对 Tab 和未换行文件末尾的检测不再映射为该 GJB 条款。
- 长于 511 字节的物理行仍需人工复核。

## 验证

- 附件违反例第 10 行有本规则诊断，遵循例无诊断。
- 新增 LF、CRLF、Tab、注释、空白行及无末尾换行的边界测试。
- 附件 446 个示例中，Automated 吻合 203，不吻合 17。
- 台账：209 条规则，57 条核心判据部分接入，152 条仍需解释，0 条宣称完整等价。

## 证据

- [原生恢复](evidence/native_vhdl_trailing_space_4_14_b_2_findings.md)
- REA Evidence bundle：`evidence/rea_vhdl_trailing_space_4_14_b_2_bundle.json`
- 示例审计：`evidence/t_linter_after_1_2_53.md`

无本规则诊断时仍显示 `ManualReviewRequired`。
