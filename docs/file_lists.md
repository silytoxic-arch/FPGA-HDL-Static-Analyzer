# HDL File list

CLI 使用 `-f FILE`、`-fFILE` 或 `--file-list FILE` 加载 simulator 风格 HDL 源文件清单；
`.fpga-lint.conf` 可用 `file-list FILE` 引用同一格式。

```text
# 注释和空行会被忽略
+incdir+rtl/include+ip/include
+define+FPGA_FAMILY=1+ENABLE_TRACE
-I generated/include
-D BUILD_ID=42
-f blocks/video.f
rtl/top.sv
rtl/control.vhd
"rtl/path with spaces/helper.sv"
```

支持内容：

- HDL 文件或目录路径；
- `-f FILE`、`-F FILE`、`-fFILE`、`-FFILE` 递归清单；
- `+incdir+DIR[+DIR...]` 与 `-I DIR`、`-IDIR`；
- `+define+MACRO[=VALUE][+MACRO...]` 与 `-D MACRO[=VALUE]`、`-DMACRO[=VALUE]`；
- 单引号、双引号和反斜杠转义的空白路径；
- `#` 及位于 token 边界的 `//` 行注释。

源码、include 目录和嵌套清单的相对路径均以包含它们的清单文件目录解析。展开后保留清单及显式输入顺序；每个目录内按路径排序，重复的同一路径仅处理首次出现。
GJB10157 分析中的源码宏按该顺序共享，定义文件须放在使用文件之前；不自动猜测宏依赖顺序。循环清单、缺少参数、未闭合引号、无法读取的
嵌套清单及不支持的 `-`/`+` 选项会返回退出码 2，并尽可能附带清单文件与行号；未知 simulator
选项不会被静默忽略，避免分析配置与实际编译配置悄然分叉。
