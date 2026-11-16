# V1.4.62 规则恢复说明

本版继续只处理规则集、逆向证据与验证台账，没有调整界面或表格。

## GJB9765.4.8.c 生成与空 CASE 边界

保存的原生类型名表把普通顺序 CASE 标为 `AST_CASE(0x57)`，把 case-generate 标为独立的 `AST_GENCASE(0x62)`。递归收集器 `FUN_180632fc0` 只在节点类型等于 `0x57` 时创建规则候选，因此 case-generate 根本身即使没有 `when others` 也不触发本条。收集器仍无条件递归其子树，所以 generate 分支内的普通顺序 CASE 继续检查。

同一收集器在创建 `AST_CASE(0x57)` 候选前没有检查 choice 或子节点数量。由此可确定：只要前端形成了成对的空 CASE AST，该节点仍会进入映射，并因没有直接 `AST_OTHERS(0x89)` 而报告。严重错误导致前端完全不形成 CASE AST 的路径仍不推测。

新增三组边界回归：

- 没有 `when others` 的 case-generate 根为零报告；
- case-generate 分支内缺少 `when others` 的普通顺序 CASE 为一报告；
- `case ... is end case;` 成对空 CASE 为一报告。

## 证据与限制

- 目标 `T-Linter.dll` SHA-256：`60e6b52875f4d7c3fbdf494d869b3a4bcc3fdc8ee63d8620ea9614da5d6599b0`；
- 入口、递归收集器与 choice 检查器：`reverse-tools/evidence/remaining_rules/vhdl_4_8_c_entry_and_collector.c`，SHA-256 `94e07b44bb10161eb32b5eebd6d5ea8ff7296ccbaa4634d5bd6012d8d49fff38`；
- 类型表明确区分 `AST_CASE(0x57)` 与 `AST_GENCASE(0x62)`；
- 嵌套 package、未形成 AST 的严重错误、上游异常根准入以及原生遍历/诊断顺序仍未完全恢复；这些路径无告警不判定为合规。

## 发布验证

- Release 配置完整构建通过，CTest 135/135 全部通过；
- 附件 446 个文档样例审计保持稳定：435 个 `Automated:matched`、6 个 `ProjectContext:matched`、4 个 `context-required`、1 个 `native-documentation-conflict`，无普通不匹配；
- 可重复生成的规则台账保持 209/209 条核心判断已接入、完整原生算法等价 0/209；
- 最终安装包：`releases/1.4.62/FPGA-HDL-Static-Analyzer-1.4.62-Darwin-arm64.zip`；
- 最终包体积与 SHA-256 记录在发布目录的外部 `SHA256SUMS`，避免把 ZIP 自身哈希写入包内文档形成自引用；
- 独立临时目录解包后，CLI 报告 `FPGA HDL Static Analyzer 1.4.62`，应用包通过 `codesign --verify --deep --strict`。
