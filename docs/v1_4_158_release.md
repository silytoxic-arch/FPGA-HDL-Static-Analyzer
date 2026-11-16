# FPGA HDL Static Analyzer V1.4.158

## 本轮目标

继续依据附件 `T-Linter.dll` 的原生静态反编译证据收敛 GJB9765 `A-9-6`（内部兼容键 `GJB9765.4.9.j`），接入用户组件/实体的递归层次检查。本版不调整界面或表格。

## 新恢复的原生行为

- `FUN_180610ad0` 在当前设计根没有直接命中时，按解析模式构造类型 `0x4f` 或 `0x94` 过滤器。
- 它通过 `FUN_1806124a0` 收集下一层候选，对每个候选递归调用自身，并把第一个非零返回值原样向上传播。
- 该证据证明组件检查不是固定一层；原生绑定容器的全部语义仍未恢复，因此产品只接入可由源码完整证明的递归子集。

## 产品实现

- 端口关联仅在完整实际值为一个普通标识符时保存递归绑定名称；表达式、选择/索引、函数调用、`open` 均不进入证明图。
- 对唯一解析的 VHDL 实体，在每层构建直接并发 `target <= source` 与 `target <= not source` 关系图。
- 已解析子实体的输入输出关系通过命名或位置端口关联映射到父实体，可跨多个源文件和多个组件层级继续传播。
- 使用活动实体集合终止循环层次；遇到循环、歧义、外部实体或无法解析的实际值时不猜测关系。
- 已证明路径继续使用原生位值：反相器为 1，缓冲器为 2；最终按时钟合并为一条 `A-9-6` 诊断。

## 新增回归

- 两层用户组件反相路径：命名内层、位置外层，报告一条仅反相诊断。
- 两层用户组件缓冲路径：位置内层、命名外层并经过内部别名，报告一条缓冲诊断。
- 内层表达式实参 `not i` 不进入递归证明图，无违规时仍保持整条标准规则人工复核状态。
- 两个实体互相实例化的循环层次安全终止，不产生伪造路径。
- 完整回归：`135/135` 通过。

## 审计与发布

- 附件复审：544 条附件记录、537 个页面、209 条标准规则、446 个可探测样例。
- 结果保持为 435 个 `Automated:matched`、6 个 `ProjectContext:matched`、4 个 `context-required`、1 个 `native-documentation-conflict`。
- 209 个公开编号全部通过官方 `R-X-X` / `A-X-X` 三段式校验。
- 台账 `fullAlgorithmsVerified` 继续为 `0`；不把尚未闭合的规则宣称为完全等价或合规。

发布物：

- `releases/1.4.158/FPGA-HDL-Static-Analyzer-1.4.158-Darwin-arm64.zip`
  - SHA-256：`1add88056ef945c98d88ed436beba3fdb0c05c4d38448b81e285375b0917c6ec`
- `docs/evidence/t_linter_after_1_4_158.json`
  - SHA-256：`f351d3b5c939d3e1483ff30ff125acc4af53488aef17d2dc1ddf977882de8d9d`
- `docs/evidence/native_rule_implementation_ledger.json`
  - SHA-256：`c295c92bf09830dcbd376439ad19c7890f0941a0131b6afb7c595b7e688e7824`
- `docs/evidence/native_vhdl_clock_path_inverter_4_9_j_findings.md`
  - SHA-256：`ea940894619e3941dd0885023d00e127fb99e1183da35d59d8713fe067ec4f1e`

## 仍未闭合的边界

Configuration binding、generate/block 内组件关系、表达式或选择实参、歧义/外部实体绑定、厂商库变体、专有前端错误恢复、原生源位置选择及独立时钟间排序仍未知。这些构造的干净结果继续是 `manual-review-required`，不表示合规。
