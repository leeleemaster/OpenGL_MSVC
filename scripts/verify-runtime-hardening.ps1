$ErrorActionPreference = "Stop"

$repositoryRoot = Split-Path -Parent $PSScriptRoot
& (Join-Path $PSScriptRoot "build.ps1") -Configuration Release
if ($LASTEXITCODE -ne 0) {
    exit $LASTEXITCODE
}

$checkExecutable = Join-Path $repositoryRoot "out\build\msvc\Release\dentalviz_benchmark.exe"
if (-not (Test-Path -LiteralPath $checkExecutable)) {
    throw "Runtime-check executable was not found: $checkExecutable"
}

& $checkExecutable --self-check
exit $LASTEXITCODE
