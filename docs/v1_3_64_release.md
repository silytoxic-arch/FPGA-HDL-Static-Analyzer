# FPGA HDL Static Analyzer V1.3.64

本版只推进规则逆向、判断实现和验证，未调整表格或界面布局。

## GJB9765.4.2.d

恢复顶层架构直接子项判断：

- 直接报告进程、变量赋值和选择赋值；
- 普通赋值及实体/组件例化只报告命中的位逻辑、归约、移位、旋转、逻辑或 `when` 节点；
- 顶层纯标识符缓冲和直接端口连接保持通过；
- 复合端口实际表达式报告。

附件违反例命中赋值和进程两处，正确例零告警。判断记录见 [4.2.d 证据](evidence/native_vhdl_top_level_instantiation_4_2_d_findings.md)。

## GJB9765.4.4.a

恢复 `std_logic_vector` 子类型范围判断：原生节点类型名必须精确等于 `std_logic_vector`，且直接子节点少于 2 时报告。附件违反例活动代码命中 5 处，正确例零告警；泛型、端口、常量、信号、变量和数组元素子类型均有覆盖测试。判断记录见 [4.4.a 证据](evidence/native_vhdl_std_logic_vector_range_4_4_a_findings.md)。

## 台账与验证

- 209 条规则中，173 条已接入证据支持的核心判断，从 V1.3.63 增加 2 条。
- 仍需解释的核心判断为 36 条。
- 完整原生算法等价数仍为 0。
- 附件 446 个示例继续执行审计；除两条新接入规则外，其他规则结果保持基线。
- 全套 CTest 131/131 通过。
- 附件审计共探测 446 个示例；本轮只改变 4.2.d 与 4.4.a 的匹配状态。
- 解压发布包后，CLI 报告 `FPGA HDL Static Analyzer 1.3.64`，示例分析保持 3 条预期告警；
  macOS `.app` 的 `CFBundleShortVersionString`/`CFBundleVersion` 均为 1.3.64，并通过
  `codesign --verify --deep --strict`。
- 发布包：
  `releases/1.3.64/FPGA-HDL-Static-Analyzer-1.3.64-Darwin-arm64.zip`，同目录附 SHA256 文件。
