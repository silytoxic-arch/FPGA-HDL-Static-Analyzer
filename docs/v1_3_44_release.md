# FPGA HDL Static Analyzer V1.3.44

本版只推进规则集，没有调整问题列表、代码规模明细或其他表格界面。

- 逆向并验证 `GJB9765.4.8.c` 的原生 case/default 判据。
- 原生算法逐 case 建立候选，只有直接 choice 下出现 `when others` 才清除违规标志。
- 明确确认：即使 bit 的 `'0'/'1'` 或枚举值已显式穷举，缺少 `when others` 仍会报告，不存在自动完备性豁免。
- 嵌套 case 独立判断，内层 default 不满足外层；注释中的伪 `when others` 不计入。
- 补充穷举无 default、嵌套、注释伪分支和直接 default 四组边界测试。
- 核心判断逻辑累计接入 153/209，仍有 56 条需要继续解释；完整原生算法验证仍为 0。
