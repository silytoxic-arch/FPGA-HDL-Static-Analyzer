# V1.4.115 R-6-4 裸结束设计根隔离

本版本只推进规则逆向、判断逻辑和验证，不调整界面或表格布局。对外规则编号继续严格采用官方三段式 `R-X-X` / `A-X-X`。

## 原生证据复核

- `GJB9765_VHDL_4_6_c` slot 0 `FUN_1806016c0` 在每个输入设计根开始时重新建立认可常量、拒绝常量、运行期对象和赋值记录容器，证明不同设计根的名称状态不能互相污染。
- `AST_LOCALPARAM(0x0a)` 候选先受节点 `+0x178` 标志门控；无子节点、直接 `AST_CONSTANT(0x10)`、前端折叠子节点、直接名称和复合名称依赖随后进入不同分支。由于 `+0x178` 的正式生产语义尚未恢复，本版不把所有无初始化源码对象擅自视为原生认可常量。
- 这些结论来自附件 `T-Linter.dll` 的离线静态反编译，不代表运行了附件软件。

证据：`docs/evidence/native_vhdl_constant_static_initializer_4_6_c_findings.md`；保留证据 ID `ev_5f591cf433b08c94fd59aae3b050b90a666c6db29bb7ea7f776975767cec926c`；完整保存的 Ghidra 伪代码位于工作区 `reverse-tools/Library/ghidra/home/Library/ghidra/ghidra_12.1.4_PUBLIC/application.log`。

## 产品修正

- architecture 根范围现在同时支持 `end architecture;`、`end architecture <name>;`、`end <name>;` 和裸 `end;`。
- 范围扫描维护嵌套 function/procedure 名称栈，合法的 `end function/procedure`、`end <subprogram-name>` 和子程序裸 `end;` 不会提前截断 architecture。
- 同时维护本地 package 名称栈，并排除 `package ... is new ...;` 实例化，避免本地 package 裸结束与 architecture 裸结束混淆。
- architecture 主 `begin` 查找使用相同的子程序结束语义，修复此前裸结束子程序会让整根被误判为没有主体的问题。

## 新增验证

- 裸结束的第一 architecture 后跟带显式 `signal` 类端口的第二 entity：第二 entity 名称不污染第一根，未知依赖保持人工复核且不误报。
- procedure 使用裸 `end;`，其后 architecture 也使用裸 `end;`：局部 variable 驱动的 constant 仍报告一次。
- 子程序名称与 architecture 同名并使用 `end <name>;`：子程序结束不提前终止根，后续动态常量仍报告。
- architecture 内本地 package 使用裸 `end;`：package 结束不提前终止根，后续动态常量仍报告。

## 仍未知

节点状态字节 `+0x178` / `+0x109` / `+0x10e` 的正式字段与全部生产者、前端常量折叠边界、deferred constant、嵌套同名对象的完整词法作用域、独立 package-body 根、纯函数/用户函数、包限定和跨单元绑定、重载、物理类型、生成域、错误恢复及原生诊断顺序仍未知。产品不以无报告宣称合规，完整厂商算法等价仍为 0/209。

## 发布验证

- 完整测试：135/135 通过；新增 4 组 `R-6-4` 根终止边界测试通过。
- 附件审计：544 条配置/文档记录、537 个随包页面、209 条标准规则、446 个正反例；其中 435 个自动规则示例吻合、6 个工程上下文示例吻合、4 个示例明确保持上下文不足、1 个附件标注与 DLL 固定默认谓词冲突。
- 原生实现台账：209/209 条均为官方三段式 `R-X-X` / `A-X-X`，209 条核心谓词均有接入记录，完整厂商算法等价仍为 0/209。
- 从全新解压目录运行 `--version` 返回 `1.4.115`；归档内应用深度签名校验通过。
- 全新解压运行夹具：裸结束 architecture 后的第二 entity 接口不产生 `R-6-4`；裸结束 procedure 内的动态常量产生 1 条官方编号 `R-6-4`；两者均无分析错误。
- 发布包路径：`releases/1.4.115/FPGA-HDL-Static-Analyzer-1.4.115-Darwin-arm64.zip`。SHA-256 在最终归档生成后记录于工作区发布说明，不写入归档内副本，以避免归档哈希自引用变化。
- 工作区发布归档 SHA-256：`0fa41cbd2c50ad005492bdd438f0a4a13872c3b5a1503bb208198fced6b543bf`。
