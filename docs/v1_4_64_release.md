# V1.4.64 规则恢复说明

本版继续只处理规则集、逆向证据与验证台账，没有调整界面或表格。

## GJB9765.4.8.d 子程序 IF 准入

对全 DLL 的 `+0x183` 位移扫描只得到 61 个指令命中。置位者 `FUN_1801c7b80` 对原始 `AST_IF(0xaa)` 调用 `FUN_1808db990`，把返回的来源句柄写入 `+0x188`，随后置 `+0x183=1`、把节点改写为 `AST_CASE(0x57)`，并整理 `AST_ELSIF(0xab)` 与 `AST_ELSE(0xac)`。AST 构造器将这些字段初始化为零，复制器完整保留它们。

`FUN_18077c840` 对任何非 `AST_ALWAYS(0x52)` 节点，只要 `+0x183` 与 `+0x188` 同时非零就调用 IF 检查器；该路径不要求祖先是 process。这证明 architecture 声明区以及 package body 的 procedure/function 顺序 IF 也属于 4.8.d，而不仅是组合 process。

产品新增完整子程序体扫描，并保持失败关闭：只有识别到 `IS ... BEGIN ... END` 的完整 function/procedure 才检查，支持显式、命名与裸结束；无法配对、嵌套子程序声明及运算符符号函数仍不猜测。

新增回归覆盖：

- architecture 声明区 procedure 缺少最终 ELSE：一报告；
- package body function 使用命名结束且缺少最终 ELSE：一报告；
- package body function 使用裸结束且具有最终 ELSE：零报告。

## 证据与限制

- 目标 `T-Linter.dll` SHA-256：`60e6b52875f4d7c3fbdf494d869b3a4bcc3fdc8ee63d8620ea9614da5d6599b0`；
- `+0x183` 指令清单：`reverse-tools/evidence/remaining_rules/field_183_all.txt`，SHA-256 `1f101ca6079ed8156dfd39a801f16de2b5a22be9db6d48894c9ca982ad9552a2`；
- 元数据生产者：`reverse-tools/evidence/remaining_rules/field_183_writers_and_metadata.c`，SHA-256 `f83b1a90df4df99fdf7241cda6f59af8a212f5f067340bf57d6a4044d25838e5`；
- 原始 C++ 字段名、错误恢复、嵌套子程序声明、运算符符号函数、扩展标识符结束形式与原生报告顺序仍保持未知；无告警不用于证明这些路径合规。

## 发布验证

- Release 配置完整构建通过，CTest 135/135 全部通过；
- 附件 446 个文档样例审计保持稳定：435 个 `Automated:matched`、6 个 `ProjectContext:matched`、4 个 `context-required`、1 个 `native-documentation-conflict`，无普通不匹配；
- 可重复生成的规则台账保持 209/209 条核心判断已接入、完整原生算法等价 0/209；
- 最终安装包：`releases/1.4.64/FPGA-HDL-Static-Analyzer-1.4.64-Darwin-arm64.zip`；
- 最终包体积与 SHA-256 记录在发布目录的外部 `SHA256SUMS`，避免把 ZIP 自身哈希写入包内文档形成自引用；
- 最终 ZIP 的独立验证覆盖 CLI 版本、包内说明占位清理和 `codesign --verify --deep --strict`。
