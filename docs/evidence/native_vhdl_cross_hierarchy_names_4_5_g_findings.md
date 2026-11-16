# GJB9765.4.5.g 跨层级信号命名一致性

目标附件 `T-Linter.dll` SHA-256 `60e6b52875f4d7c3fbdf494d869b3a4bcc3fdc8ee63d8620ea9614da5d6599b0`。这里的原生行为依据是 Ghidra 12.1.4 静态反编译，不是执行厂商程序。

## 原生观察

- vtable slot 0 `FUN_1806bc5c0` 保留证据 `ev_c07c84a3affb6c7af8d84a772f30dc92e3527de2c0bd71d29c5052e66c7d0ee0`。其入口调用 `FUN_1804d0140` 构建层级连接记录，再遍历记录中的两侧名称；名称均非空时分别做小写转换与字节比较，不相等才走诊断分支。
- `FUN_1804d0140` 的工作区只读定点反编译在 `reverse-tools/Library/ghidra/home/Library/ghidra/ghidra_12.1.4_PUBLIC/application.log` 中，可按 `FUNCTION 1804d0140` 查找。它显式区分 AST_INSTANCE_ENTITY(0x93) 和 AST_INSTANCE_COMPONENT(0x94)，并收集关联及名称；其完整层级展开语义尚未恢复。
- `FUN_1806bc5c0` 按对象 `+0x78` 的设计根逐项执行，并把当前根连同对象 `+0x68/+0x78` 的两个工程容器传给 `FUN_1804d0140`。构造器不是按源文件调用，因而可以用工程解析状态为当前根绑定另一个物理文件中的 entity 形式端口。
- V1.4.94 重新导出的完整入口与连接构造器保存在 `reverse-tools/evidence/a5_32_entry_connection_builder.txt`（SHA-256 `6c6fdeb30c6ff9a139033adb061cdafca36a12eac883c082e31cc724b4a51c03`）。连接构造器对类型 `0x8b` 的具名关联从首子节点提取形式端口名，再处理实际端；对直接 `AST_IDENTIFIER(0x13)` 的位置实参按当前关联序号从已解析形式端口列表取名。两条分支最终生成同一种连接记录。
- 规则入口逐个读取连接记录的首字符串和 `+0x48` 字符串；仅当两者都非空时逐字节 `tolower`，随后执行完整长度敏感比较。名称不相等时每条连接记录产生一个 Warning，没有“只处理具名关联”的入口过滤。
- 附件 `examples_html/GJB9765_VHDL.4.5.g.html` 的错误例具名端口映射有 5 处实参信号名与形式端口名不同；正确例的对应名称一致。

## 有界产品判断

对已解析实体或当前架构声明的 component，检查具名 `port map` 中“一个形式端口 => 一个直接标识符实参”的连接，以及按声明顺序可绑定的直接标识符位置实参；比较不区分大小写，名字不同逐连接报告。唯一 entity 目标允许位于另一个工程文件中。附件错误例定位 5 处，正确例 0 处。另有具名/位置大小写一致、位置名称不一致、跨文件具名/位置解析、重名目标失败关闭、表达式和 `open` 的边界测试。

表达式实参、别名、库限定绑定、配置/生成层级、隐式且无本地声明的 component 绑定、混合非法关联的错误恢复及原生精确诊断位置仍未知。目标 entity 重名时不猜测绑定；产品未命中时保持人工复核含义，不宣称整条规则的原生算法完全等价。

新增证据：`reverse-tools/evidence/a5_32_bound_root_project_resolution.txt`，SHA-256 `c3c12da777faaf2d7e1d7c9eab5d269d3c292ccb97535343f787cb4c840a160f`。
