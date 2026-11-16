# V1.4.63 规则恢复说明

本版继续只处理规则集、逆向证据与验证台账，没有调整界面或表格。

## GJB9765.4.8.d generate 递归边界

从保存的 Ghidra 工程重新导出了 `FUN_18077ad60`、`FUN_18077c840` 与 `FUN_18077b8e0` 的完整静态反编译。`FUN_18077c840` 只把 `AST_ALWAYS(0x52)` 作为过程候选；对所有其他节点，它在完成本节点处理后无条件递归全部直接子节点，没有针对 `AST_GENIF`、`AST_GENCASE` 或其他 generate 容器的停止分支。

因此本版明确并回归验证：

- IF-generate 根本身不是组合过程，不因没有 ELSE 被本条报告；
- generate 分支内的组合 process 仍被递归到达，其中缺少最终 ELSE 的顺序 IF 仍报告；
- 时钟/事件过程仍由原生分类分支整体排除。

现有产品扫描本来已经具备这两个行为，本版增加证据约束、正反边界测试与台账记录，不扩大到证据不足的子程序 IF。

## 证据与限制

- 目标 `T-Linter.dll` SHA-256：`60e6b52875f4d7c3fbdf494d869b3a4bcc3fdc8ee63d8620ea9614da5d6599b0`；
- 完整入口/遍历器/IF 检查器反编译：`reverse-tools/evidence/remaining_rules/vhdl_4_8_d_entry_traversal_and_if.c`，SHA-256 `c41b6c032dcd6ca0b1ecabc7a0ebd2320ba39f6db3ac3dfca663c19ba3223d9b`；
- `+0x188` 位移原始扫描：`reverse-tools/evidence/remaining_rules/field_188_all.txt`，SHA-256 `838cbfdffdf573d6fcc829849ea897dd8fb4c17d8c8436f7053b6a21be1e6bb4`；该清单含大量栈和无关对象命中，不能单独证明字段含义；
- `+0x183/+0x188` 精确字段语义、受其影响的子程序 IF、错误恢复、原生诊断顺序和非默认运行时选项仍保持未知，不以无告警证明合规。

## 发布验证

- Release 配置完整构建通过，CTest 135/135 全部通过；
- 附件 446 个文档样例审计保持稳定：435 个 `Automated:matched`、6 个 `ProjectContext:matched`、4 个 `context-required`、1 个 `native-documentation-conflict`，无普通不匹配；
- 可重复生成的规则台账保持 209/209 条核心判断已接入、完整原生算法等价 0/209；
- 最终安装包：`releases/1.4.63/FPGA-HDL-Static-Analyzer-1.4.63-Darwin-arm64.zip`；
- 最终包体积与 SHA-256 记录在发布目录的外部 `SHA256SUMS`，避免把 ZIP 自身哈希写入包内文档形成自引用；
- 最终 ZIP 的独立验证覆盖 CLI 版本、包内说明占位清理和 `codesign --verify --deep --strict`。
