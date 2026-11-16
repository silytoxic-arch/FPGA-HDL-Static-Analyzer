# Windows 版本构建与打包

Windows 版本使用 Visual Studio 2022、CMake 3.24+ 和 Qt 6.2+ 构建。发布包包含中文 GUI、
命令行程序，以及 `windeployqt` 收集的 Qt DLL、平台插件和翻译文件。

## 环境要求

- Windows 10/11 x64
- Visual Studio 2022，安装“使用 C++ 的桌面开发”工作负载
- CMake 3.24 或更高版本
- Qt 6 x64 MSVC 2022，例如 `C:\Qt\6.8.3\msvc2022_64`

在项目根目录的 PowerShell 中执行：

```powershell
.\scripts\package_windows.ps1 -QtRoot C:\Qt\6.8.3\msvc2022_64
```

脚本会依次配置 Release 工程、编译 CLI 与 GUI、运行全部测试、创建安装树，并生成 CPack
ZIP。主要输出位置：

- `build\windows-install\bin\fpga_analyzer.exe`
- `build\windows-install\bin\fpga_analyzer_cli.exe`
- `build\windows-release-package\FPGA-HDL-Static-Analyzer-<版本>-Windows-AMD64.zip`

仓库内置的 `.github/workflows/windows-build.yml` 会在 GitHub 托管的 Windows 2025、
Visual Studio 2022 和 Qt 6.8.3 环境中执行同一脚本。每次推送到 `main`、提交针对
`main` 的拉取请求，或在 Actions 页面手动触发工作流时，都会重新编译、运行测试并上传
名为 `FPGA-HDL-Static-Analyzer-Windows-x64` 的 ZIP 工件及其 SHA-256 校验文件。

若 Qt 安装目录或处理器名称不同，ZIP 文件名中的架构字段可能不同。正式发布前应在干净的、
未安装 Qt 的 Windows 主机上解压并启动 GUI，以确认 DLL 与平台插件完整。
