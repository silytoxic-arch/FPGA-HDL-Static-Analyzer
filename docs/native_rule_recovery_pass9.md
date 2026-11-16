# 第九轮：六类关键字表与显式策略

目标 SHA-256：`60e6b52875f4d7c3fbdf494d869b3a4bcc3fdc8ee63d8620ea9614da5d6599b0`。
静态只读逆向，未执行厂商 DLL，未修改附件或用户 RTL。

GJB10157.5.2.3 run `0x180535e60` 的保留 Evidence：
`ev_e1ff709dbf6ffe9075286c99ef5e824912ce9dc45827378aaf7d79cd5afdfe26`。
七类声明包含 FUNCTION 本体；标准化名称后 tolower，再查类别开关与完整名称表。
完整控制流确认“第一个启用且命中的类别”优先；启用但未命中时继续下一类别。
不可误解为只检查第一个启用类别，也不是名称含有关键字子串即违规。

六个连续 string 初始化范围及析构计数：VHDL115、Verilog126（trireg重复，唯一125）、
SystemVerilog117、SDF/EDIF1、PSL67、Windows22，共448个初始化项。
按保留伪代码构造参数与 SHA 绑定的不可变 PE 文件字节逐项恢复；135个DATA引用记录VA/RVA/
section/fileOffset/原字节，其他项保留伪代码字面量来源，并未伪造为原始字节证据。
完整记录见 [表项观察](evidence/verilog_keyword_table_observations.json)。

构造辅助 `0x18004a000`、赋值辅助 `0x1800d9010` 伪代码省略copy长度参数，本轮用汇编补核：
`ev_341ab1f176062981a28a9b11897565a8ca07464ec296fb35b7def47a825a20cc`、
`ev_f99dbfc8d4c931a228a31a2641cc1b147d87cf787a11931b13b8188e628eba92`
均通过R8传strlen（空串0）至`0x180040680`，没有分词/逗号拆分。
先前表节点copy证据`ev_e1297c18f51d2cc9b7fecbc64009446b553e2a77709a5091e4e8de912db21e25`、
`ev_8c1fb3bcfa6481b3630b2df37c62ca49f48f3768c0f7e33d9ad58e2d3722c4e6`继续复用。
等值区间查询及计数`ev_483f65e2ba14138beb01e591fac3e028922ea4bdb4d13870289f471114a40aa2`、
`ev_d50d0f55c0a322755661ca996b9a031b07b3c3dc844be33156ad1b4adfb49e3a`支持全名等长匹配。

独立软件将这些表接入显式 `keyword-policy` / `keyword-category` 工程配置。
厂商外部默认开关仍未知，未指定时要求上下文，不替用户默认全开。
SDF/EDIF确实只有一整段逗号串；忠实保留，不宣称启用此类别即完成全部SDF/EDIF关键词安全检查。
Windows只匹配22项，不擅自扩展为操作系统保留路径/设备检测。
ASCII之外、escaped、宏、复杂类型/实例、完整AST/诊断等价仍未知；无报警仍复核。

真实源码回归发现旧前端对Verilog结构关键字错误地转小写，合法MODULE/BEGIN等被误判为语法结构。
修正结构检查与Verilog模块/端口/参数解析的关键字大小写，保留VHDL原有不敏感路径。
新增大写标识符、escaped结构关键字、参数/端口保持与真实关键字声明的正反例。
此为独立产品前端修复，不声称恢复完整厂商前端。

纯判断3609断言（448初始化项×七声明类型、全名边界与策略控制）通过。
目录台账209条，23条核心谓词部分接入，2条模型型观察尚未接入，完整算法等价验证0条。
静态session证据 checkpoint9：1265条记录、21624621字节。目标持续，不是全部规则完成。
