# FPGA HDL Static Analyzer V1.4.156

## 本轮目标

继续依据附件 `T-Linter.dll` 的原生静态反编译证据收敛 GJB9765 `A-9-6`（内部兼容键 `GJB9765.4.9.j`），接入组件/实体层次中的直接时钟缓冲和反相路径。本版不调整界面或表格。

## 新恢复的原生边界

- `FUN_18060cf40 @ 0x18060cf40` 递归收集组件实例及端口关联中的名称。
- `FUN_180610ad0 @ 0x180610ad0` 解析组件/实体端口关联并递归进入绑定设计根，分别处理缓冲关系与反相关系。
- `FUN_18060f280 @ 0x18060f280` 遍历输入、输出关联及赋值关系图，识别缓冲直通路径。
- 恢复专用时钟单元白名单：`IBUFG`、`IBUFGDS`、`BUFG`、`BUFGP`、`BUFGCE`、`BUFGMUX`、`BUFGDLL`、`DCM`、`INBUF`。
- 原生证据和边界说明保存在 `docs/evidence/native_vhdl_clock_path_inverter_4_9_j_*`。

## 产品与回归

- VHDL 解析器记录 architecture 直接并发 `target <= source;` 与 `target <= not source;`，不把 process、generate 或 block 内语句误当成组件直通关系。
- 对已解析的用户实体，支持命名端口和位置端口的一层跨文件直接缓冲/反相路径；仅当父层输出实参已被证明是时钟时报告。
- 厂商专用时钟单元白名单不产生误报；时序组件内部传递不冒充直接组合路径。
- GUI、CLI、文本、HTML、JSON 与 SARIF 的公开规则编号均来自显式官方索引表，严格采用 `R-X-X` / `A-X-X` 三段式，例如 `R-2-1`。完整 GJB 条款号仅保留为内部兼容和追溯键。
- 编译期与单元测试同时校验全部 209 条规则均为三段数字编号，并校验标准配置内唯一；两套标准可以各自拥有同名编号，查询时使用标准配置消歧。

## 验证与发布

- 完整回归：`135/135` 通过。
- 附件复审：544 条附件记录、537 个页面、209 条标准规则、446 个可探测样例；结果为 435 个 `Automated:matched`、6 个 `ProjectContext:matched`、4 个 `context-required`、1 个 `native-documentation-conflict`。
- 规则台账中的 209 个公开编号全部通过正则 `^[RA]-[0-9]+-[0-9]+$` 校验。
- 干净解压后的 CLI 版本为 `1.4.156`；跨文件组件时钟缓冲样例报告公开编号 `A-9-6`。
- 解压应用通过 `codesign --verify --deep --strict`，框架符号链接保留 15 个。

发布物：

- `releases/1.4.156/FPGA-HDL-Static-Analyzer-1.4.156-Darwin-arm64.zip`
  - SHA-256：`7eca0886d7484aaf6f3f961379a7add9c7f46fbcebb96d2b8d909d7d81051199`
- `docs/evidence/t_linter_after_1_4_156.json`
  - SHA-256：`0d699f2849ddade5121c9d8d07106b4da32d065d1c280a41b84d7d87b6c9aafe`
- `docs/evidence/native_rule_implementation_ledger.json`
  - SHA-256：`c0e29aefbe59e79a99ee7d6c1a07fede4a2f9c36cda9b1d5d4a0b4aa0b935da2`
- `docs/evidence/native_vhdl_clock_path_inverter_4_9_j_findings.md`
  - SHA-256：`dc018a8e1a136b20652f25c245ee2fcdb25e9f894ac60f1046b190b91626999f`
- `docs/evidence/native_vhdl_clock_path_inverter_4_9_j_component_names.txt`
  - SHA-256：`58ee910cd097d2837c2a90a7cc21462a4e97586f6ce04caca0e4f36b827d1f59`
- `docs/evidence/native_vhdl_clock_path_inverter_4_9_j_component_check.txt`
  - SHA-256：`2e4331120ac8ebcc4060f9c4fa16403fe1dc7301211dd1bc777e201f48723127`
- `docs/evidence/native_vhdl_clock_path_inverter_4_9_j_buffer_check.txt`
  - SHA-256：`b54624fa2f154362eb5d407d5bd1629ef80d591f888d2a1d2d69d6a581d7233b`

## 边界

当前产品只把源码可证明的已解析用户实体一层直接缓冲/反相路径判为违规。复杂多层关系图、configuration binding、表达式实参、未解析外部库、厂商库变体、专有前端错误恢复和精确诊断次数仍保持人工复核；`fullAlgorithmsVerified` 继续为 `0`，不宣称与原厂 209 条规则完整算法等价。
