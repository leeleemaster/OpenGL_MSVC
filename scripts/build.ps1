param(
    [ValidateSet("Debug", "Release")]
    [string]$Configuration = "Debug"
)

$ErrorActionPreference = "Stop"

function Invoke-CleanProcess {
    param(
        [Parameter(Mandatory = $true)]
        [string]$FilePath,

        [Parameter(Mandatory = $true)]
        [string[]]$ArgumentList
    )

    $startInfo = [System.Diagnostics.ProcessStartInfo]::new()
    $startInfo.FileName = $FilePath
    $startInfo.UseShellExecute = $false

    if ($null -ne $startInfo.PSObject.Properties["ArgumentList"]) {
        foreach ($argument in $ArgumentList) {
            [void]$startInfo.ArgumentList.Add($argument)
        }
    } else {
        # Windows PowerShell 5.1 uses .NET Framework, where ProcessStartInfo
        # does not expose ArgumentList. These arguments are fixed CMake preset
        # switches without whitespace, so a joined command line is equivalent.
        $startInfo.Arguments = $ArgumentList -join " "
    }

    # Some agent shells expose both `Path` and `PATH`. MSBuild treats those as
    # duplicate case-insensitive keys when it launches cl.exe, so normalize the
    # child environment without changing the user's persistent environment.
    if ($startInfo.Environment.ContainsKey("Path") -and $startInfo.Environment.ContainsKey("PATH")) {
        [void]$startInfo.Environment.Remove("Path")
    }

    $process = [System.Diagnostics.Process]::Start($startInfo)
    $process.WaitForExit()
    return $process.ExitCode
}

$vswherePath = "C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe"
if (-not (Test-Path -LiteralPath $vswherePath)) {
    throw "Visual Studio Installer (vswhere.exe) was not found."
}

$installationPath = & $vswherePath `
    -latest `
    -products * `
    -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 `
    -property installationPath

if (-not $installationPath) {
    throw "Visual Studio with the MSVC x64 toolchain was not found."
}

$cmakePath = Join-Path $installationPath "Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
$ctestPath = Join-Path $installationPath "Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\ctest.exe"
$vcpkgRoot = Join-Path $installationPath "VC\vcpkg"
$vcpkgToolchain = Join-Path $vcpkgRoot "scripts\buildsystems\vcpkg.cmake"

foreach ($requiredPath in @($cmakePath, $ctestPath, $vcpkgToolchain)) {
    if (-not (Test-Path -LiteralPath $requiredPath)) {
        throw "Required build tool was not found: $requiredPath"
    }
}

$env:VCPKG_ROOT = $vcpkgRoot
$presetSuffix = $Configuration.ToLowerInvariant()

& $cmakePath --preset msvc --no-warn-unused-cli
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

$exitCode = Invoke-CleanProcess -FilePath $cmakePath -ArgumentList @("--build", "--preset", "msvc-$presetSuffix")
if ($exitCode -ne 0) { exit $exitCode }

$exitCode = Invoke-CleanProcess -FilePath $ctestPath -ArgumentList @("--preset", "msvc-$presetSuffix")
exit $exitCode
