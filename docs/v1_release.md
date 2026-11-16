# FPGA HDL Static Analyzer V1.0 发布说明

## 产品结论

V1.0 是一套可用于实际 FPGA 项目的 HDL Lint 基线：它能在本地开发和 CI 中完成源码发现、
预处理、结构提取、编码规范检查、RTL 风险检查、单元级 CDC/RDC 检查、问题定位和报告归档。
它不宣称替代 SpyGlass、ALINT-PRO、Questa Lint 或厂商时序签核工具。

## 支持范围

- 输入语言：Verilog、SystemVerilog、VHDL；识别 `.v/.vh/.sv/.svh/.vhd/.vhdl`。
- 工程入口：单文件、多个文件、递归目录、simulator 风格 file list 和项目配置文件。
- 预处理：Verilog/SystemVerilog 条件编译、宏定义与 include 路径；black box、路径排除、
  规则禁用、严重级覆盖和源码行级豁免。
- 结构分析：module/entity、端口、parameter/generic、实例关系、顶层单元、时钟、复位、
  时序过程、信号读写和已认证的两级 CDC 同步链。
- 规则：90 条内建规则，覆盖 syntax、style、structure、synthesizability、RTL correctness、
  width/range、clock/reset、CDC/RDC 等类别。完整目录见 [规则参考](rules.md)。
- 输出：终端 text、JSON schema 3、OASIS SARIF 2.1.0、离线 HTML 和 Qt 桌面 GUI；诊断包含
  文件、行列、设计单元、说明、建议、严重级与确定性规则统计。
- CI 工作流：`--fail-on` 阈值、可审查 baseline、稳定退出码、配置归档和 SARIF 集成。

## 发布验收

- macOS arm64 Release 构建通过，Qt 6.11.2 GUI 编译及离屏 smoke 通过。
- CTest 90/90：6 个核心测试、83 个 CLI 集成回归、1 个 GUI smoke。
- LLVM 20 UBSan 错误即停矩阵 89/89；ASan 因当前 macOS 主机运行时初始化不兼容未形成
  有效执行，已作为环境限制记录，后续应在 Linux CI 补跑。
- 安装树和 CPack ZIP 已验证包含 CLI、自带 Qt Framework/平台插件的 macOS `.app`、文档与
  示例配置。
- 真实 CLAHE 工程回归：12 文件、12 设计单元、11 实例、105 条诊断，无分析错误。

详细命令、版本和历史结果见 [验证记录](validation.md)。

## 已知边界与非目标

- 分析核心采用预处理后、注释/字符串感知的 token 与结构模型，并非完整 IEEE 语法树、类型
  系统或 elaborator；复杂语法会保守跳过而不是猜测。
- include 文件可以查找和验证，但当前不进行完整的文本注入与宏展开；宏函数、token paste、
  generate 展开和参数化 elaboration 不完整。
- signedness、用户自定义类型、复杂算术表达式和动态选择的精确位宽推导仍有限。
- CDC/RDC 以设计单元内可证明的直接路径和模板为主，不覆盖完整跨层次传播、约束意图、
  waiver 体系或签核级亚稳态/复位释放分析。
- 不读取 XDC/SDC，不执行形式验证、综合、布局布线、静态时序分析或时序收敛判断。
- GUI smoke 不替代各目标操作系统上的人工交互、可访问性和大工程性能验收。
- macOS `.app` 已做 ad-hoc 签名以保证 bundle 完整性，但未使用 Apple Developer ID 签名或
  notarization；面向互联网分发前仍需由发布方完成正式签名、公证和 Gatekeeper 验收。

因此，V1.0 适合作为提交前检查和 CI 质量门禁；涉及安全关键设计、复杂 CDC/RDC、约束与
时序闭合时，仍需结合仿真、综合实现、STA、形式工具和商业签核流程。
