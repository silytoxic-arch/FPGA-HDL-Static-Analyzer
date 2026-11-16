# FPGA HDL Static Analyzer V1.4.193

## R-2-5 原生判据深化

- 软件规则编号严格采用官方三段式 `R-2-5`；内部条款键不出现在界面及文本、HTML、JSON、SARIF 报告中。
- 恢复按源文件精确分组、重复模块名不去重和每文件单条聚合告警。
- 恢复固定 Warning、模块名完整列表、ASCII 逗号及末尾句点的精确正文。
- 普通正物理行映射到最早模块行；原生无列字段由产品第 1 列承载。
- 全局对象枚举顺序、`+0x2f3` 语义和完整宏/include/generated 前端继续人工复核。

## 验证

- 增加同名、异名、三个模块、最早物理行、严重度、精确正文和跨文件隔离断言。
- 规则目录具备编译期三段编号格式门禁，并验证界面与全部报告输出只使用 `R-X-X / A-X-X`。
- CTest `135/135` 全部通过。
- 附件审计覆盖 544 条安装规则、537 个页面、209 条 GJB 标准规则和 446 个示例：自动规则 434 个吻合、工程上下文规则 6 个吻合、4 个明确需要上下文、2 个原生实现与附件说明冲突，无未解释差异。
- 集成源码适配器由 40 条增至 41 条，剩余 168 条保持部分接入；完整原生等价仍为 `0/209`。
- 审计 JSON SHA-256：`b72dddf9fab7581ccd90b65cceddb98ef7afb0d067948adad8bcd3a1b437b79f`。
- 原生规则台账 SHA-256：`e74b95cdd78c48903e6b42ecc3dd45d935226f0041a8c503f44f4696c0ad401c`。
- 发布包：`releases/1.4.193/FPGA-HDL-Static-Analyzer-1.4.193-Darwin-arm64.zip`，SHA-256 `c78c70cef6ab634f681bb1468fc1c3c557f5779703073ed13e4cf3f3dc1e6fac`。
- 解包后 CLI 返回 `FPGA HDL Static Analyzer 1.4.193`；探针输出 `R-2-5`、Warning、第 1 行第 1 列及精确正文 `在源文件中找到多个模块:first,second.`，JSON 中不存在内部条款键。两套规则清单共 209 条，全部匹配 `R-X-X / A-X-X`，应用短版本为 `1.4.193` 并通过 `codesign --verify --deep --strict`。

详细证据见 [第五十七轮逆向记录](native_rule_recovery_pass57.md) 与 [R-2-5 原生证据](evidence/native_verilog_module_per_file_5_2_5_findings.md)。
