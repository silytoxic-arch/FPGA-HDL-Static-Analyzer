# V1.4.57 规则恢复说明

本版继续只处理规则集及验证台账，没有调整界面或表格。

## GJB9765.4.8.a 常量名称来源

`FUN_180608bc0` 遍历 `0x76 -> 0xa3 -> 0x13` 的直接层次；相邻规则已把它交叉确认成
entity、generic、identifier。原生主函数还递归收集带 `+0x178` 标志的 `0x0a` 声明。
因此固定 CASE 选择器的常量集合不只包含 architecture 声明区常量，也包含所属 entity 的
generic 和更深声明区中的常量。

产品现在把 entity generic 绑定到它的命名 architecture，并收集 architecture 与 process
声明区中的显式 constant。新增 generic 正例、process 局部 constant 正例和跨 entity 同名
generic 隔离负例；两个正例修复前漏报，修复后均报告，隔离负例保持无诊断。

## 证据与限制

- 目标 `T-Linter.dll` SHA-256：
  `60e6b52875f4d7c3fbdf494d869b3a4bcc3fdc8ee63d8620ea9614da5d6599b0`；
- 证据：`ev_0045125577774a59b95281d68c8a30bc680ec3ce1750c75bddd6a5789dc89c47`
  和 `ev_bc0f722c9b21f3f1bebb5edbc5e7316797c851c5d91e88e91a9096d6f7d98ca4`；
- 没有执行厂商程序。package 可见性、子程序声明、遮蔽、别名、函数求值及其余 `0x0a`
  标志生产路径仍未知；无诊断继续保持人工复核，不伪判为完整合规。

## 发布验证

- 三个新增作用域边界测试已通过；
- Release 配置完整构建通过，CTest 135/135 全部通过；
- 附件 446 个文档样例审计保持稳定：435 个 `Automated:matched`、6 个
  `ProjectContext:matched`、4 个 `context-required`、1 个
  `native-documentation-conflict`，无普通不匹配；
- 规则台账覆盖 209/209 条核心判断；完整原生算法等价仍为 0/209，未把未知作用域或缺少
  工程上下文的干净结果伪判为合规；
- 最终安装包：
  `releases/1.4.57/FPGA-HDL-Static-Analyzer-1.4.57-Darwin-arm64.zip`；
- 包体积：93,263,634 字节；SHA-256：
  `9e828792c369f1a936b633aac7f5d96d6aa7bdca82418fe4192a4c91768f428c`；
- 独立临时目录解包后，CLI 报告 `FPGA HDL Static Analyzer 1.4.57`，应用包通过
  `codesign --verify --deep --strict`。
