param(
    [Parameter(Mandatory = $true)]
    [string]$QtRoot
)

$ErrorActionPreference = "Stop"
$env:QT_ROOT = (Resolve-Path $QtRoot).Path

function Invoke-Checked {
    param([string]$Command, [string[]]$Arguments)
    & $Command @Arguments
    if ($LASTEXITCODE -ne 0) {
        throw "$Command 执行失败，退出码：$LASTEXITCODE"
    }
}

Push-Location (Join-Path $PSScriptRoot "..")
try {
    Invoke-Checked "cmake" @("--preset", "windows-vs2022")
    Invoke-Checked "cmake" @("--build", "--preset", "windows-release", "--parallel")
    Invoke-Checked "ctest" @("--preset", "windows-release")
    Invoke-Checked "cmake" @("--install", "build/windows-vs2022", "--config", "Release", "--prefix", "build/windows-install")
    Invoke-Checked "cpack" @("-C", "Release", "--config", "build/windows-vs2022/CPackConfig.cmake", "-B", "build/windows-release-package")
    Write-Host "Windows Release 构建、测试、安装和 ZIP 打包已完成：build/windows-release-package"
} finally {
    Pop-Location
}
