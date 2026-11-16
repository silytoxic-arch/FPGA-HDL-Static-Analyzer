# 1.2.2 图标预览版（2026-10-10）

沿用 1.2.1 的 RTL 源码单选/多选和 GJB 检查流程，加入 Figma 设计的芯片与检查标记图标。
macOS 应用包包含 ICNS 图标及 `CFBundleIconFile` 声明；Qt 窗口图标内嵌 PNG 资源，不依赖外部路径。

解压后双击 `fpga_analyzer.app`。该包适用于 Apple Silicon macOS，采用 ad-hoc 签名，未公证。
关闭旧版本后再启动新版；Dock 图标由实际运行的应用版本决定。
Windows 原生可执行文件仍须在 Windows 编译，本次预览包不是 Windows EXE。

验证：98/98 项 CTest、89/89 项 CLI 回归通过。GUI 测试覆盖图标资源加载、RTL 单选/多选及取消行为。
应用包的图标声明和 ad-hoc 签名完整性检查通过。

[Figma 图标设计](https://www.figma.com/design/QGY5w6p4GZ4ibG5Ek1oFFT?node-id=18-59)
