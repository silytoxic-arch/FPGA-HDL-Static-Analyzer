# FPGA HDL Static Analyzer V1.4.18

本版继续只处理规则集，不调整界面。

## GJB10157.5.2.7 原生逐行判据修正

DLL 静态证据确认该规则不是通用绝对路径解析器：它逐物理行查找精确 `` `include`` 字节串，
再检查同一行任意位置是否包含冒号。V1.4.18 按此恢复核心判断：

- `C:/...` 或其他含冒号的 include 行报告；
- 冒号不必位于引号路径内部；
- 注释、字符串、畸形指令和未激活宏分支仍按原生物理行扫描报告；
- 纯 `/absolute/path` 不再被错误归入该原生规则；
- 每个匹配物理行报告一次并定位到 `` `include``。

删除了原产品挂在该 GJB 条款下但缺少 DLL 依据的 Unix 斜杠路径扩展。新增九类正反例及边界
测试。证据与剩余未知项见
[5.2.7 逆向记录](evidence/native_verilog_include_path_5_2_7_findings.md)。

## 附件审计

- 标准规则：209 条；附件示例探测：446 个；
- Automated：435 个吻合、1 个差异；唯一差异仍是已记录的 5.2.15 HTML 与 DLL 冲突；
- ProjectContext：4 个可判定示例吻合；6 个缺少工程上下文的示例保持
  `context-required`；
- 台账继续记录 `fullAlgorithmsVerified: 0`。
