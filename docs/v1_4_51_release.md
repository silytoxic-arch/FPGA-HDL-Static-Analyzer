# V1.4.51 规则恢复说明

本版继续只处理规则集及其验证台账，没有调整问题列表、代码规模明细或其他表格界面。

## GJB10157.7.3.3 跨页面工程上下文恢复

规则页面 `GJB10157_VLOG.7.3.3.html` 的两个父模块分别实例化 `my_module1` 和
`my_module2`，但页面自身没有给出子模块定义。先前审计因此正确保留为
`context-required`，没有猜测形式端口方向。

同一附件的 `TLR-110056.html` 精确包含这两个子模块定义：

- `my_module1.IN1` 为 input、`my_module1.OUT1` 为 output，对应错误示例；
- `my_module2.IN1/OUT1` 均为 inout，对应正确示例。

审计器现在只在该页面 SHA-256 等于
`4606db7ae726e11ac504f839cabba0ecfd923e7e598c4b61f279f8e7a552b823` 时提取完整模块，
并把它作为独立 Verilog 文件与原规则示例一起分析。报告保留模块名、来源路径、页面哈希和
探测文件名；页面缺失、哈希变化或定义缺失都会继续标记为需要上下文。

错误示例现在在原父模块第 24、25 行分别报告 INOUT1→input 和 INOUT2→output；正确示例
不报告 7.3.3。原示例文本和行号没有因补上下文而改变。

## 尚未恢复

GJB9765.4.5.f.9 的两个 USE-only 示例仍缺包声明源文件及其真实文件名；
GJB9765.4.5.f.10 的两个声明/USE-only 示例仍缺包体源文件及其真实文件名。这四项继续保持
`context-required`，不根据示例类别反推文件名。

完整供应商算法等价仍未证明。原生诊断重数、异常语法恢复以及各规则台账中列出的深层语义
边界仍需继续逆向。

## 发布验证

- 审计器边界测试覆盖正确恢复、哈希拒绝和缺定义拒绝；
- 完整重配置、构建及 CTest：135/135 通过；
- 附件样例审计：446 个样例，其中 `Automated:matched` 435、
  `native-documentation-conflict` 1、`ProjectContext:matched` 6、
  `context-required` 4；没有未解释的普通 mismatch；
- 台账重新生成：209/209 核心谓词已接入，完整供应商算法等价验证仍为 0；
- 解包后的 CLI 报告 `FPGA HDL Static Analyzer 1.4.51`；
- 解包后的 macOS 应用通过 `codesign --verify --deep --strict`；
- 发布包为 `releases/1.4.51/FPGA-HDL-Static-Analyzer-1.4.51-Darwin-arm64.zip`；
  91,621,348 字节，SHA-256
  `9885e8b40d859a227e67a50666b3211d9dbf939f3205e49e4c32b9fe83ae0eac`。
