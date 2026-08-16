param(
    [string]$OutputDirectory
)

$ErrorActionPreference = "Stop"

$repositoryRoot = Split-Path -Parent $PSScriptRoot
if ([string]::IsNullOrWhiteSpace($OutputDirectory)) {
    $OutputDirectory = Join-Path $repositoryRoot "out\benchmark"
}

& (Join-Path $PSScriptRoot "build.ps1") -Configuration Release
if ($LASTEXITCODE -ne 0) {
    exit $LASTEXITCODE
}

$benchmarkExecutable = Join-Path $repositoryRoot "out\build\msvc\Release\dentalviz_benchmark.exe"
if (-not (Test-Path -LiteralPath $benchmarkExecutable)) {
    throw "Benchmark executable was not found: $benchmarkExecutable"
}

& $benchmarkExecutable --output $OutputDirectory
exit $LASTEXITCODE
