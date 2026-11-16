# V1.4.58 规则恢复说明

本版继续只处理规则集与验证台账，没有调整界面或表格。

## GJB9765.4.8.b 设计根隔离

原生 `FUN_18065f460` 对每个设计根先清空、再重建允许作为 CASE/SELECT choice 的静态
名称集合。旧产品按整文件合并名称：当 entity A 声明 generic `CHOICE`，entity B 的同名
运行时端口会被全局静态集合遮蔽，造成 B 的 `when CHOICE` 漏报。

产品现在为每个 architecture 按其所属 entity 分别建立运行时名和静态名：端口与 generic
来自对应 entity，signal、variable、constant 与枚举字面量来自该 architecture。新增跨根
同名负例先复现漏报，修复后产生一条 4.8.b 诊断；既有同根正反例保持通过。

## 证据与限制

- 目标 `T-Linter.dll` SHA-256：
  `60e6b52875f4d7c3fbdf494d869b3a4bcc3fdc8ee63d8620ea9614da5d6599b0`；
- 原生证据：`ev_26210e473bab6346944094e210e7f99d4c7da3f98c8211927cc35c04e9d689b1`
  与 `ev_23d2e820cfe6e62123a15b6baaff6e4a88228c0b704e4e96b16291ea83a7671a`；
- 没有执行厂商程序。`0x77` 的完整派生结构、package/subtype 可见性、alias、纯函数、属性、
  configuration 绑定和异常语法恢复仍未知；未知名称继续人工复核。

## 发布验证

- 新增跨 entity 同名隔离回归并通过；
- Release 配置完整构建通过，CTest 135/135 全部通过；
- 附件 446 个文档样例审计保持稳定：435 个 `Automated:matched`、6 个
  `ProjectContext:matched`、4 个 `context-required`、1 个
  `native-documentation-conflict`，无普通不匹配；
- 规则台账覆盖 209/209 条核心判断；完整原生算法等价仍为 0/209，未知名称、配置和工程
  上下文继续显式保留为人工复核；
- 最终安装包：
  `releases/1.4.58/FPGA-HDL-Static-Analyzer-1.4.58-Darwin-arm64.zip`；
- 包体积：93,542,217 字节；SHA-256：
  `2354cbdce9779379bec3335db0c172442ad9bbcb6a14fdea83b67e16ba0bd73e`；
- 独立临时目录解包后，CLI 报告 `FPGA HDL Static Analyzer 1.4.58`，应用包通过
  `codesign --verify --deep --strict`。
