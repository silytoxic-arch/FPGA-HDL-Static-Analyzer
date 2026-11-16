# R-2-8 数据类型原生判据恢复

## 证据边界

附件 `T-Linter.dll` SHA-256：`60e6b52875f4d7c3fbdf494d869b3a4bcc3fdc8ee63d8620ea9614da5d6599b0`。使用 Ghidra 12.1.4 对构造函数 `FUN_180021af0` 和 vtable slot 0 活动入口 `FUN_18053f190` 做只读静态分析；本轮完整导出保存于 `reverse-tools/evidence/t_linter_pass59/r2_8_decompile.txt`，逐地址字符串保存于同目录 `r2_8_strings.txt`。未执行或修改 DLL。

附件 `examples_html/GJB10157_VLOG.5.2.8.html` 的 SHA-256 为 `4cdd5cdaefe302db91524ea941884c40be3dc7ec2ebc0fdaade6891775e0acf4`。

## 恢复判据

`FUN_18053f190` 遍历每个模块记录 `+0xa8` 容器中的直接子节点，并按子节点类型分支：

- 类型 6（由既有交叉规则枚举证据对应 `AST_WIRE`）读取十三个布尔标志。每个分支引用的独立诊断对象已经从 GB18030 字符串恢复，因此标志与类型可直接对应：
  - `+0x29f`：`time`
  - `+0x2a0`：`realtime`
  - `+0x2a4`：`tri0`
  - `+0x2a5`：`tri1`
  - `+0x2a2`：`triand`
  - `+0x2a3`：`trior`
  - `+0xae`：`trireg`
  - `+0x10d`：`real`
  - `+0x29d`：`supply0`
  - `+0x29e`：`supply1`
  - `+0x2a6`：`uwire`
  - `+0xa7`：`wand`
  - `+0xa8`：`wor`
- 类型 10 无条件构造 `localparam` 诊断。
- 类型 9 明确跳过，不进入报告路径；结合诊断正文，这建立了普通 `parameter` 的允许边界。
- 同一个声明节点若 `time` 标志成立，入口先报告 `time`，随后仍进入其余标志链；其余十二个标志采用互斥的嵌套分支。正常前端应只为一个节点设置对应类型标志。

十三个 wire 类诊断正文逐字为 `禁止使用除reg、wire、integer、tri和parameter之外的数据类型，信号%s是<type>类型违反.`；`localparam` 独立正文为 `禁止使用除reg、wire、integer、tri和parameter之外的数据类型，%s是localparam类型违反.`。两类均固定使用小写原生严重度 `error`（产品映射为 Error），候选名称来自节点 `+0x48`，文件来自 `+0x1e0`，物理行来自 `+0x210`，原生记录没有列字段。每个已命中的直接子对象各自构造一条诊断。

由此恢复的原生禁用集合是十三个 wire 类类型加 `localparam`，并非根据标题扩展推测出的所有 SystemVerilog 类型。正文中的紧凑英文枚举、中文逗号、`之外`、wire 分支中的 `信号`、localparam 分支不含 `信号`，以及末尾 ASCII 句点均属于固定原生文本。

## 产品接入

V1.4.195 按声明对象逐一检查上述精确集合，覆盖条件编译后的活动源码中的模块直接声明、ANSI 参数列表和 ANSI 端口列表，并支持逗号列表的类型继承。`localparam` 每个对象报告一次；普通 `parameter`、`wire`、`reg`、`integer`、`tri` 不报告。函数、任务、过程块局部声明、包、`typedef` 和 `nettype` 不被错误并入该“模块直接子节点”判据。产品输出固定 Error、对象物理行、第 1 列和逐字恢复正文；用户界面与报告公开编号仅为 `R-2-8`。

回归覆盖十三种 wire 类类型、`localparam`/`parameter` 分界、逗号继承、ANSI 端口、模块体、条件编译、注释、包、类型参数、typedef、函数、任务和过程局部声明，并新增 wire/localparam 两条固定正文、Error、物理行与第 1 列断言。

## 保留限制

专有 AST 字节字段的源码名称、所有 Verilog/SystemVerilog 方言结构的前端降低、接口与 modport、属性、生成层级、类型别名传播、由宏片段拼接的声明、错误恢复及原生诊断顺序仍未完全重建。产品只宣称接入已恢复的直接模块声明核心，不宣称与专有前端逐字节等价；这些未证明的无报警路径继续要求人工复核。
