# GJB10157.8.7 POW 常量静态观察

- 目标：`T-Linter.dll` V3.1.4.25120520251205115359
- SHA-256：`60e6b52875f4d7c3fbdf494d869b3a4bcc3fdc8ee63d8620ea9614da5d6599b0`
- 工具：Ghidra 12.1.4，`ReadScalarValues.java`
- 证据类型：静态内存读取，不是运行时观察

观察结果：

- `DAT_180ef3ea8` / `0x180ef3ea8`：原始 64 位值
  `0x4000000000000000`，IEEE-754 double 为 `2.0`；
- `DAT_180e836b8` / `0x180e836b8`：原始 64 位值
  `0x3ff0000000000000`，IEEE-754 double 为 `1.0`。

结合 `FUN_1805790d0` 的 Ghidra 伪代码，POW 分支计算
`(int)(pow(2.0, exponent_width) - 1.0) * base_width - 1`，随后在目标位宽
严格小于该值时返回违规。超出浮点到有符号整数转换范围、以及后续有符号乘减
溢出的运行时结果没有由本静态观察证明。
