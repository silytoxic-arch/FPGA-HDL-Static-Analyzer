# V1.4.60 规则恢复说明

本版继续只处理规则集、逆向证据与验证台账，没有调整界面或表格。

## GJB9765.4.8.c VHDL 条件 lowering 排除

原生 `FUN_180632fc0` 只收集 `AST_CASE(0x57)` 且 `+0x183/+0x184` 都为零的节点。此前已确认 `+0x183` 排除 Verilog IF lowering；本轮对完整 DLL 扫描 `0x184` 的全部 67 个指令命中，并反编译真正的 AST 写入候选。

关键生产者 `FUN_1801ad560` 把 `AST_ELSIF(0xab)` 原地改写为 `AST_CASE(0x57)` 后立即写入 `+0x184=1`，随后将 `AST_ELSE(0xac)` 整理成默认分支。这闭合了第二个标志的过滤用途：它标记 VHDL `if/elsif/else` 降低生成的内部 CASE，`4.8.c` 只检查源码 CASE，不把没有最终 ELSE 的 VHDL 条件树误报为缺少 `when others`。

产品本来就按源码 `case ... end case` 配对，因此核心实现无需修改。本版增加两个边界验证：

- VHDL IF/ELSIF 没有最终 ELSE 时，`GJB9765.4.8.c` 必须为零；
- architecture 声明区 procedure 内显式 CASE 缺少 `when others` 时，必须报告一次。

## 证据与限制

- 目标 `T-Linter.dll` SHA-256：`60e6b52875f4d7c3fbdf494d869b3a4bcc3fdc8ee63d8620ea9614da5d6599b0`；
- 全字段命中清单：`reverse-tools/evidence/remaining_rules/field_184_all.txt`，SHA-256 `ed3bdf3d65ecdf3155b8c2c025334bf95615e1819ff6a29e8e866a17db3b71ef`；
- 写入候选反编译：`reverse-tools/evidence/remaining_rules/field_184_writers.c`，SHA-256 `cdd861f7358b36ec3ea441586eaef3fd95b47977e178db2a3f40ca8ee2cc7b8e`；
- 原始 C++ 字段名、全部错误恢复生产者、package body、生成作用域及原生诊断顺序仍未完全恢复；无告警不用于证明这些未知路径合规。

## 发布验证

- Release 配置完整构建通过，CTest 135/135 全部通过，其中新增 IF/ELSIF 排除与 procedure CASE 边界均通过；
- 附件 446 个文档样例审计保持稳定：435 个 `Automated:matched`、6 个 `ProjectContext:matched`、4 个 `context-required`、1 个 `native-documentation-conflict`，无普通不匹配；
- 可重复生成的规则台账保持 209/209 条核心判断已接入、完整原生算法等价 0/209，不把字段用途恢复夸大为全前端等价；
- 最终安装包：`releases/1.4.60/FPGA-HDL-Static-Analyzer-1.4.60-Darwin-arm64.zip`；
- 包体积：94,088,374 字节；SHA-256：`77bf59cc1add81b3d7de0b310a954b01fa5b17787c71013464fd4ffb951f85e2`；
- 独立临时目录解包后，CLI 报告 `FPGA HDL Static Analyzer 1.4.60`，应用包通过 `codesign --verify --deep --strict`。
