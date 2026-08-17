param(
    [string]$Archive = ""
)

$ErrorActionPreference = "Stop"

$repositoryRoot = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot ".."))
if ([string]::IsNullOrWhiteSpace($Archive)) {
    $Archive = Join-Path $repositoryRoot `
        "output\release\DentalViz-v1.0.1-korean-windows-x64.zip"
}
$Archive = [System.IO.Path]::GetFullPath($Archive)
if (-not (Test-Path -LiteralPath $Archive -PathType Leaf)) {
    throw "검증할 ZIP 파일이 없습니다: $Archive"
}

$temporaryRoot = [System.IO.Path]::GetFullPath([System.IO.Path]::GetTempPath())
$verificationRoot = [System.IO.Path]::GetFullPath(
    (Join-Path $temporaryRoot ("DentalViz-package-" + [guid]::NewGuid().ToString("N"))))
$temporaryPrefix = $temporaryRoot.TrimEnd([System.IO.Path]::DirectorySeparatorChar) +
    [System.IO.Path]::DirectorySeparatorChar
if (-not $verificationRoot.StartsWith(
        $temporaryPrefix,
        [System.StringComparison]::OrdinalIgnoreCase)) {
    throw "임시 폴더 밖의 경로는 사용할 수 없습니다: $verificationRoot"
}

[void](New-Item -ItemType Directory -Path $verificationRoot)
try {
    Expand-Archive -LiteralPath $Archive -DestinationPath $verificationRoot
    $executable = Get-ChildItem -LiteralPath $verificationRoot -Filter DentalViz.exe -Recurse |
        Select-Object -First 1
    if ($null -eq $executable) {
        throw "압축을 푼 패키지에서 DentalViz.exe를 찾지 못했습니다."
    }

    $standardOutput = Join-Path $verificationRoot "DentalViz.stdout.txt"
    $standardError = Join-Path $verificationRoot "DentalViz.stderr.txt"
    $process = Start-Process `
        -FilePath $executable.FullName `
        -ArgumentList @("--smoke-seconds", "5") `
        -WorkingDirectory $executable.DirectoryName `
        -WindowStyle Hidden `
        -RedirectStandardOutput $standardOutput `
        -RedirectStandardError $standardError `
        -PassThru
    try {
        if (-not $process.WaitForExit(15000)) {
            $process.Kill()
            $process.WaitForExit()
            throw "패키지 실행이 제한 시간 안에 끝나지 않았습니다."
        }
        if ($process.ExitCode -ne 0) {
            throw "패키지 실행이 종료 코드 $($process.ExitCode)로 실패했습니다."
        }
    } finally {
        $process.Dispose()
    }

    $outputText = Get-Content -LiteralPath $standardOutput -Raw -Encoding utf8
    if (-not $outputText.Contains("DentalViz 1.0.1")) {
        throw "패키지 실행 파일의 버전이 1.0.1이 아닙니다."
    }
    if (-not $outputText.Contains("Application loop exited cleanly.")) {
        throw "패키지 실행 파일의 정상 종료 기록이 없습니다."
    }

    Write-Output "Windows 패키지 스모크 테스트: 통과"
    Write-Output "버전: DentalViz 1.0.1"
    Write-Output "ZIP SHA-256: $((Get-FileHash -LiteralPath $Archive -Algorithm SHA256).Hash)"
} finally {
    $resolvedVerificationRoot = [System.IO.Path]::GetFullPath($verificationRoot)
    if ($resolvedVerificationRoot.StartsWith(
            $temporaryPrefix,
            [System.StringComparison]::OrdinalIgnoreCase) -and
        (Test-Path -LiteralPath $resolvedVerificationRoot)) {
        Remove-Item -LiteralPath $resolvedVerificationRoot -Recurse -Force
    }
}
