# V1.4.46 规则恢复说明

本版继续只处理规则集，没有调整问题列表、代码规模明细或其他表格界面。

## 单一顶层配置

只读反编译确认，导出函数 `Yosys::CheckRule::setTopModule(0x1806298c0)` 对普通 Verilog 名称添加 Yosys 前导反斜杠、验证设计记录存在并写入共享字符串 `DAT_18135b808`。GJB10157.5.3.2、5.3.3、5.3.4 使用该名称选择或排除唯一顶层。VHDL GJB9765.4.2.d 的 `FUN_180782710` 也从同一共享名称解析设计记录，成功后才收集 architecture 直接子项。

产品新增：

- CLI：`--top-module NAME`；
- 配置文件：`top-module NAME`；
- Verilog 名称保持大小写敏感，并接受普通名或带 Yosys 前导反斜杠的名称；
- VHDL entity 按语言语义大小写不敏感解析；
- 显式名称缺失或重复时产生分析错误并失败关闭，不回退到自动推断根；
- 未显式配置时保留实例图根兼容模式，无告警继续保持 `ManualReviewRequired`。

## 准确性回归

新增多根 Verilog 工程测试，证明配置 `top_a` 后 5.3.2/5.3.3 只检查该模块，而 5.3.4 会把另一个实例图根 `top_b` 当作非顶层。新增 VHDL 大小写、多根选择、缺失顶层和规范反斜杠名称边界测试。

当前台账仍为 209/209 条核心谓词已接入、完整供应商算法等价验证 0 条。5.3.2、5.3.3 和 4.2.d 仍因前端降级及复杂语法边界保持 `partial-integration`；5.3.4 保持 `integrated-source-adapter`。未知分支不被判定为合规。

证据详见 [单一顶层配置恢复记录](evidence/native_configured_top_selection_findings.md)。

## 验证结果

- 完整 CTest：135/135 通过。
- 附件规则总数：544；其中 GJB10157/GJB9765 标准规则 209 条。
- 附件示例探测：446；`Automated:matched` 435、既有 `Automated:mismatch` 1、`ProjectContext:matched` 4、`context-required` 6，与 V1.4.45 基线无新增退化。
