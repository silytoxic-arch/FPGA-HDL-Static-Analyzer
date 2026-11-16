# V1.4.59 规则恢复说明

本版继续只处理规则集与验证台账，没有调整界面或表格。

## GJB9765.4.8.b architecture 结束形式

原生 `FUN_18065f460` 按 AST 设计根分别重建静态名称集合，源码选择哪一种合法 architecture
结束写法不会改变根边界。产品 V1.4.58 的源码适配只识别 `end architecture`，因此使用
`end rtl;` 或 `end;` 时可能把后续 architecture 吞入第一根，导致运行时 choice 漏报。

范围扫描现支持完整、命名和裸三种结束形式。裸 `end;` 会保留为候选，直到遇到下一个设计
单元或文件末尾才确定为根终点，以免直接把内部结构的裸结束误认为 architecture 结束。
两个新增跨根用例均在修复前漏报、修复后各产生一条 4.8.b 诊断。

## 证据与限制

- 目标 `T-Linter.dll` SHA-256：
  `60e6b52875f4d7c3fbdf494d869b3a4bcc3fdc8ee63d8620ea9614da5d6599b0`；
- 原生按设计根重建集合的证据：
  `ev_26210e473bab6346944094e210e7f99d4c7da3f98c8211927cc35c04e9d689b1`；
- 合法结束形式来自 VHDL architecture 语法边界，未声称执行过厂商程序；configuration、嵌套
  子程序裸结束、错误恢复及完整前端根构造仍保留人工复核。

## 发布验证

- 新增命名结束与裸结束两个跨设计根回归并通过；
- Release 配置完整构建通过，CTest 135/135 全部通过；
- 附件 446 个文档样例审计保持稳定：435 个 `Automated:matched`、6 个
  `ProjectContext:matched`、4 个 `context-required`、1 个
  `native-documentation-conflict`，无普通不匹配；
- 规则台账覆盖 209/209 条核心判断；完整原生算法等价仍为 0/209，configuration、异常语法
  和缺少工程上下文的路径没有被伪判为合规；
- 最终安装包：
  `releases/1.4.59/FPGA-HDL-Static-Analyzer-1.4.59-Darwin-arm64.zip`；
- 包体积：93,815,104 字节；SHA-256：
  `4d3443de343a1ea3d6831a83624cebab6a190c28fb9cd14466ce1b73f45ef373`；
- 独立临时目录解包后，CLI 报告 `FPGA HDL Static Analyzer 1.4.59`，应用包通过
  `codesign --verify --deep --strict`。
