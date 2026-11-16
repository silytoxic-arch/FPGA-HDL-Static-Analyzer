# V1.4.112 R-6-2 五类名称宽度优先级

本版本只推进规则逆向、判断逻辑和验证，不调整界面或表格布局。对外规则编号继续严格采用官方三段式 `R-X-X` / `A-X-X`。

## 新确认并接入的原生逻辑

- `FUN_1805fee80` 对名称宽度依次查询 `+0x168`、`+0x188`、`+0x198`、`+0x158`、`+0x178`，全部未命中才返回未知。
- `FUN_180600050` 的生产分支分别把 localparam/constant、entity port、entity generic、signal、variable 写入上述表，因此固定优先级为 `constant > port > generic > signal > variable`。
- 该结论来自附件 `T-Linter.dll` 的离线静态反编译，不代表运行了附件软件。

证据：`reverse-tools/evidence/r6_2_width_entry_collector_resolver.c`，SHA-256 `a7f1be023c877cc9906fbdd9a7eb6f96bb894321b3c10975f7702fea5985537f`。

## 产品修正

- `vhdlRootShapes` 不再使用一个源码顺序平面表，而是在完整 design root 内分别收集 constant、port、generic、signal、variable，最后按原生优先级合并。
- architecture 的 `R-6-2` 宽度解析新增所属 entity 的固定宽度 vector generic；动态或无法求宽的 generic 继续失败关闭。
- 显式 `constant/signal/variable` 形式参数进入对应类别；缺省 `in` 与 `out/inout/buffer` 形式参数分别按 VHDL 缺省 constant/variable 类处理。专有前端的精确缺省类别降级仍未恢复，因此该部分明确标为产品适配，不宣称厂商前端等价。
- file 参数没有已恢复的宽度表生产分支，不参与推断。
- `AST_VARASGN (:=)` 仍明确排除；无诊断仍为 `ManualReviewRequired`。

## 新增验证

- 所属 entity 的 8 位 vector generic 与 4 位 signal 比较：报告一次。
- 8 位 port 与同名 4 位 process variable：使用 port 宽度，不报告。
- 4 位 process constant 与同名 8 位 port：constant 优先，报告一次。
- 4 位 generic 与同名 8 位 architecture signal：generic 优先，报告一次。
- 8 位 architecture signal 与同名 4 位 process variable：signal 优先，不报告。
- 此前 `R-6-2` 正反例与边界测试继续执行。

## 仍未知

专有前端的缺省形式参数节点类别、名称作用域修饰、同名嵌套子程序、package specification 可见性、别名/子类型/重载、动态范围、生成域、错误恢复及原生诊断顺序仍未知。产品保留这些限制，不以无报告宣称合规，完整厂商算法等价仍为 0/209。

## 发布验证

- 完整测试：135/135 通过。
- 附件审计：544 条配置/文档记录、537 个随包页面、209 条标准规则、446 个正反例；其中 435 个自动规则示例吻合、6 个工程上下文示例吻合、4 个示例明确保持上下文不足、1 个附件标注与 DLL 固定默认谓词冲突。
- 原生实现台账：209/209 条均为官方三段式 `R-X-X` / `A-X-X`，209 条核心谓词均有接入记录，完整厂商算法等价仍为 0/209。
- 从全新解压目录运行 `--version` 返回 `1.4.112`；归档内应用深度签名校验通过。
- 全新解压运行夹具：所属 entity vector generic 宽度冲突产生 1 条官方编号 `R-6-2`；port/variable 重名边界产生 0 条 `R-6-2` 并保持 `ManualReviewRequired`；两者均无分析错误。
- 发布包路径：`releases/1.4.112/FPGA-HDL-Static-Analyzer-1.4.112-Darwin-arm64.zip`。SHA-256 在归档生成后记录于工作区发布说明，不写入归档内副本，以避免归档哈希自引用变化。
- 工作区发布归档 SHA-256：`ed7ed68f6bbb593beda57ef627a3ac90628e72e433b2c0c6527adbf19a4869aa`。
