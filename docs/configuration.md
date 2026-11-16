# 项目配置

`.fpga-lint.conf` 是逐行、无第三方依赖的项目配置。空行和 `#` 注释被忽略；相对路径以
配置文件所在目录解析。

```text
fail-on warning
include-dir rtl/include
define FPGA_FAMILY_7SERIES=1
disable STYLE-001
severity RTL-LATCH-INFERRED error
blackbox xpm_fifo_async
exclude **/generated/**
baseline .fpga-lint-baseline
file-list project/files.f
```

支持的指令：

- `disable RULE`：关闭规则；也可使用 `all`。
- `severity RULE LEVEL`：把规则调整为 `info`、`warning`、`error` 或 `critical`。
- `blackbox UNIT`：把未提供源码的 module/entity 作为外部 IP 解析。
- `exclude GLOB`：排除规范化路径；支持 `*` 和 `?`。
- `include-dir PATH`：增加 Verilog/SystemVerilog include 搜索目录。
- `define MACRO[=VALUE]`：设置条件编译宏。
- `baseline PATH`：加载诊断基线。
- `file-list PATH`：展开 simulator 风格 HDL 源文件清单；相对路径以配置文件目录解析。
- `fail-on LEVEL`：设置 CI 退出 1 的最低级别；额外支持 `none`。
- `fsm-priority area|speed|unspecified`：声明状态机的面积/速度优化目标；默认未指定。
  CLI `--fsm-priority` 可覆盖配置。仅对已可靠绑定的 VHDL 状态机执行对应编码判断；
  不指定目标时 GJB9765.4.11.f.b/c 仍要求工程上下文。该目标门槛是产品适用上下文，
  并非已恢复的原生 run 内部条件；部分检测未发现问题仍保留人工复核。
- `header-policy template|explicit|disabled`：选择 Verilog/SystemVerilog 文件头策略；
  每次设置均清空之前的关键字。未设置时使用软件自身的 `template` 模板，**不是已恢复的厂商默认值**。
  `explicit` 后可添加关键字；无关键字与 `disabled` 均不执行文件头检查，但不会判定条款合规。
- `header-keyword sensitive|insensitive "KEYWORD"`：设置独立大小写标志，允许多个不同关键字。
  含空格的关键字须加双引号；空字符串可显式设置，重复键报错。
  当前仅支持 ASCII，且配置文件会从首个 `#` 开始剥离注释，因此关键字不可包含 `#`。

例如启用已恢复的文件头关键字核心判断（`gjb10157` 对应 Verilog，
`gjb9765` 对应 VHDL）：

```text
standard gjb10157
header-policy explicit
header-keyword insensitive "Author"
header-keyword sensitive "Revision History"
```

VHDL 工程使用相同配置语法，例如：

```text
standard gjb9765
header-policy explicit
header-keyword insensitive "Author"
header-keyword sensitive "Revision history"
```

通过 `--config FILE` 或 GUI 工程配置加载。Verilog 关键字在每个已可靠识别模块之前，VHDL
关键字在文件首个 `library`、`use` 或 `entity` 节点之前的非空物理行中做完整子串匹配；不限于注释、
不要求冒号后有值，也不要求单词边界，首个上下文/设计单元节点及之后不参与。
前置空格行不是空行。带宏、复杂或重复模块绑定、非 ASCII 前置行等情况仍需人工复核。
不报警也不证明完整 GJB10157.5.1.1 或 GJB9765.4.13.a 合规。JSON 的
`configuration.verilogHeaderPolicy`（为兼容早期版本保留的字段名）记录两种 HDL 的策略来源与有效关键字。

GJB10157.5.2.3 命名关键字类别采用显式工程配置，不假定厂商默认启用项：

```text
standard gjb10157
keyword-policy explicit
keyword-category vhdl on
keyword-category verilog on
keyword-category systemverilog on
keyword-category sdf-edif off
keyword-category psl off
keyword-category windows off
```

`keyword-policy explicit|disabled|unspecified` 每次重置六个开关。`explicit` 的初始开关全部关闭，
随后用 `keyword-category vhdl|verilog|systemverilog|sdf-edif|psl|windows on|off` 设置；同类别最后一次设置生效。
`disabled` 是显式全关；`unspecified` 恢复未指定策略，5.2.3 要求工程上下文。JSON 记录有效开关。
配置未知时JSON的 enabled 为 null，而不是伪装为已关闭的 false。
判据对已识别的七类声明名称做 ASCII 大小写折叠和严格全名查表；类别按 VHDL→Verilog→SV→
SDF/EDIF→PSL→Windows 搜索，第一个启用且命中的类别产生诊断，前一类别未命中会继续搜索。
Windows 表仅有22个字面项，不是扩展的操作系统文件名检测；原生 SDF/EDIF 表为一整段逗号字符串，
当前忠实保留，不私自拆成单词。因此不得将启用它描述为完整 SDF/EDIF 关键字安全检查。
非 ASCII/escaped/宏/复杂或重复绑定仍需复核；显式全关或未发现问题均不会认证标准合规。

CLI 的列表设置追加到配置之后。`--severity RULE=LEVEL` 可重复使用，同一规则最后一次设置
生效；`--fail-on` 直接覆盖配置值。未知规则、非法级别、缺失值和未知指令都会定位到配置
文件行并使命令返回 2。

CLI 可通过 `-f FILE`、`-fFILE` 或 `--file-list FILE` 加载同一格式。清单中的源码、嵌套清单
和 include 目录以各自所在清单的目录解析，详细语法见 [File list](file_lists.md)。
