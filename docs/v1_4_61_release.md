# V1.4.61 规则恢复说明

本版继续只处理规则集、逆向证据与验证台账，没有调整界面或表格。

## GJB9765.4.8.c package body 设计根

保存的入口反编译表明，`FUN_1806326d0` 遍历规则输入记录并取得每条记录的非空 AST 根，随后不检查根类型就调用递归收集器 `FUN_180632fc0`。同一附件的类型名函数明确包含 `AST_PACKAGE_BODY(0xa8)`。这是收集器覆盖 package body 子树的直接结构依据；上游输入记录对异常包体的全部准入条件仍不作推测。

产品此前只在 architecture 循环中调用 CASE 检查，package body 内 procedure/function 的显式 CASE 会漏报。本版增加独立 package body 根扫描，并保持规则范围收窄：该新路径只接入 `GJB9765.4.8.c` 的 `when others` 判据，不顺带扩大其他 CASE 规则。

同时修正该诊断原因文字中遗留的错误条款号：由 `GJB9765.4.11.a` 改为实际映射的 `GJB9765.4.8.c`；规则 ID 和判断逻辑此前未受此文字错误影响。

范围解析支持：

- `end package body;`；
- `end <package-name>;`；
- 裸 `end;`，并以后一设计根为边界，避免吞入 architecture；
- package body 子程序中直接 `when others` 的遵循例。

修复前新增 package body 违反例为零报告，修复后为一报告；其余边界回归通过。

## 证据与限制

- 目标 `T-Linter.dll` SHA-256：`60e6b52875f4d7c3fbdf494d869b3a4bcc3fdc8ee63d8620ea9614da5d6599b0`；
- 入口及收集器反编译：`reverse-tools/evidence/remaining_rules/vhdl_4_8_c_entry_and_collector.c`，SHA-256 `94e07b44bb10161eb32b5eebd6d5ea8ff7296ccbaa4634d5bd6012d8d49fff38`；
- 嵌套 package、错误恢复、生成作用域、上游根记录准入及原生诊断顺序仍未完全恢复；这些路径无告警不判定为合规。

## 发布验证

- Release 配置完整构建通过，CTest 135/135 全部通过；
- 附件 446 个文档样例审计保持稳定：435 个 `Automated:matched`、6 个 `ProjectContext:matched`、4 个 `context-required`、1 个 `native-documentation-conflict`，无普通不匹配；
- 可重复生成的规则台账保持 209/209 条核心判断已接入、完整原生算法等价 0/209；
- 最终安装包：`releases/1.4.61/FPGA-HDL-Static-Analyzer-1.4.61-Darwin-arm64.zip`；
- 包体积：94,364,176 字节；SHA-256：`6a2758af4e54838d9a3ce778554dc7a50aa39b621a1ae729d22692d871f24730`；
- 独立临时目录解包后，CLI 报告 `FPGA HDL Static Analyzer 1.4.61`，应用包通过 `codesign --verify --deep --strict`。
