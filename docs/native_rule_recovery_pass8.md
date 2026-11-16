# 第八轮：文件头显式配置接入

目标仍为附件 T-Linter.dll，SHA-256：
`60e6b52875f4d7c3fbdf494d869b3a4bcc3fdc8ee63d8620ea9614da5d6599b0`。
仅静态只读分析，未执行厂商 DLL，也未修改附件。

GJB10157.5.1.1 的 run `0x1805258c0` 及逐关键字大小写分支，已有证据
`ev_351f2d01a7e2d8efb05ecb58c1a4ff21ebc46f16acca2df1b165c89b9c8bc30c`。
本轮读取调用者汇编 `ev_af5d79ef77ddca2d17f43bdd4311ebf0cdb99613f35752cb69d82c291e2d7b2f`：
`0x180525fb7 MOV R9,[RBP+0x150]`，随后 `XOR R8D,R8D`，
`0x180525fc8 CALL 0x1801a2d00`。该栈槽为复制出的关键字 string.size；
因此第四参数确为完整关键字长度，第三参数搜索起点为0。此前伪代码遗漏参数的缺口得到补足。
匹配 helper `ev_4ead1cd3df3fd24c1039b4d47a24d251880a61f562e6ccc0853e8c1dab4d9a85`
是 memchr/memcmp 字节子串匹配，不是正则、字段解析或整词比较。

此前直接导出 API 的证据继续复用：setHeaderComment
`ev_c2a2f9228d4b8abb364922693e1df5a80dac61114d0247578e399a2feb80b310`，
Rule map copy `ev_7b37ea997846cac845b54b7f69002f632c1e915ee64d9657c2046369c88ed821`，
bool 不反转的复制 `ev_6518abafe2526f0e6d88e15c4685588f7b0ad7c50e75905cf81c6525c2766168`。
这些证明运行期可配置，不证明外部 GUI 默认关键字已恢复。

独立产品实现增加 `header-policy` 与 `header-keyword` 工程配置，GUI/CLI 共用 AnalysisOptions。
显式 ASCII 策略查模块声明前全部非空物理行；空白行不 trim；空值字段和子串可满足条件；
每模块最多一条缺失诊断，列出所有缺少的关键字。空 map 禁用，不回退模板。
JSON 记录来源与关键字，产品默认模板明确标识 product-template，未冒充厂商默认值。

源到模块绑定采用独立保守门槛：宏、escaped、复杂容器、嵌套/未闭合/重复绑定不推断结果。
非 ASCII 前置行受 Windows ACP 转换影响，保留人工复核；诊断位置/顺序及完整前端仍未证明原生等价。
5.1.1 无告警统一人工复核，包含模板成功或显式禁用情况。

独立 helper 的23组边界断言继续使用；新增共享引擎、配置合法性与三个真实 CLI 配置回归。
台账22/209条核心谓词部分接入，2条模型型与1条配置型谓词观察但未接入；完整算法等价验证0条。
这是持续逆向检查点，不是全部209条完成。

文件头汇编阶段证据保存至pass3/session-evidence-checkpoint7.json：1259条记录，21596059字节。

继续追踪5.2.3的查表 helper：`0x180676aa0`
`ev_d50d0f55c0a322755661ca996b9a031b07b3c3dc844be33156ad1b4adfb49e3a`
遍历查询区间并计数；区间提供者`0x18093fd20`
`ev_483f65e2ba14138beb01e591fac3e028922ea4bdb4d13870289f471114a40aa2`
在有序树中使用memcmp及string长度比较进行双边界搜索。
由此支持完整字符串键查找，不支持“含有关键字子串即违规”的推断。
这是静态解释；类别开关默认值、完整表字节与源前端映射仍未恢复，5.2.3未接入。
本轮最终静态证据保存至pass3/session-evidence-checkpoint8.json：1261条记录，21611145字节。
