param(
    [ValidateSet("Debug", "Release")]
    [string]$Configuration = "Release",

    [switch]$SkipBuild
)

$ErrorActionPreference = "Stop"

$repositoryRoot = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot ".."))
$outputRoot = [System.IO.Path]::GetFullPath((Join-Path $repositoryRoot "out"))
$packageName = "DentalViz-v1.0-submission-windows-x64"
$stagingRoot = [System.IO.Path]::GetFullPath((Join-Path $outputRoot "package"))
$packageDirectory = [System.IO.Path]::GetFullPath((Join-Path $stagingRoot $packageName))
$archivePath = [System.IO.Path]::GetFullPath((Join-Path $outputRoot "$packageName.zip"))

function Assert-PathInsideOutput {
    param(
        [Parameter(Mandatory = $true)]
        [string]$PathToCheck
    )

    $outputPrefix = $outputRoot.TrimEnd([System.IO.Path]::DirectorySeparatorChar) +
        [System.IO.Path]::DirectorySeparatorChar
    if (-not $PathToCheck.StartsWith(
            $outputPrefix,
            [System.StringComparison]::OrdinalIgnoreCase)) {
        throw "Refusing to modify a path outside the repository output directory: $PathToCheck"
    }
}

Assert-PathInsideOutput -PathToCheck $stagingRoot
Assert-PathInsideOutput -PathToCheck $packageDirectory
Assert-PathInsideOutput -PathToCheck $archivePath

if (-not $SkipBuild) {
    & (Join-Path $PSScriptRoot "build.ps1") -Configuration $Configuration
    if ($LASTEXITCODE -ne 0) {
        exit $LASTEXITCODE
    }
}

$executablePath = Join-Path $repositoryRoot "out\build\msvc\$Configuration\DentalViz.exe"
$shaderSource = Join-Path $repositoryRoot "assets\shaders"
$vcpkgShare = Join-Path $repositoryRoot `
    "out\build\msvc\vcpkg_installed\x64-windows-static-md-gl33\share"

foreach ($requiredPath in @(
        $executablePath,
        $shaderSource,
        $vcpkgShare,
        (Join-Path $repositoryRoot "packaging\README.txt"),
        (Join-Path $repositoryRoot "LICENSE"))) {
    if (-not (Test-Path -LiteralPath $requiredPath)) {
        throw "Required package input was not found: $requiredPath"
    }
}

if (Test-Path -LiteralPath $packageDirectory) {
    Remove-Item -LiteralPath $packageDirectory -Recurse -Force
}
if (Test-Path -LiteralPath $archivePath) {
    Remove-Item -LiteralPath $archivePath -Force
}

$shaderDestination = Join-Path $packageDirectory "assets\shaders"
$licenseDestination = Join-Path $packageDirectory "third-party-licenses"
[void](New-Item -ItemType Directory -Path $shaderDestination -Force)
[void](New-Item -ItemType Directory -Path $licenseDestination -Force)

Copy-Item -LiteralPath $executablePath -Destination $packageDirectory
Copy-Item -Path (Join-Path $shaderSource "*") -Destination $shaderDestination
Copy-Item -LiteralPath (Join-Path $repositoryRoot "packaging\README.txt") `
    -Destination (Join-Path $packageDirectory "README.txt")
Copy-Item -LiteralPath (Join-Path $repositoryRoot "LICENSE") -Destination $packageDirectory
Copy-Item -LiteralPath (Join-Path $repositoryRoot "assets\models\LICENSE.txt") `
    -Destination (Join-Path $packageDirectory "MODEL-ASSET-NOTICE.txt")

$copyrightFiles = Get-ChildItem -LiteralPath $vcpkgShare -Directory |
    ForEach-Object {
        $copyrightPath = Join-Path $_.FullName "copyright"
        if (Test-Path -LiteralPath $copyrightPath) {
            [PSCustomObject]@{
                PackageName = $_.Name
                CopyrightPath = $copyrightPath
            }
        }
    }

foreach ($copyrightFile in $copyrightFiles) {
    Copy-Item -LiteralPath $copyrightFile.CopyrightPath `
        -Destination (Join-Path $licenseDestination "$($copyrightFile.PackageName).txt")
}

Compress-Archive -LiteralPath $packageDirectory `
    -DestinationPath $archivePath `
    -CompressionLevel Optimal

$archiveHash = Get-FileHash -LiteralPath $archivePath -Algorithm SHA256
Write-Output "Package directory: $packageDirectory"
Write-Output "Archive: $archivePath"
Write-Output "SHA256: $($archiveHash.Hash)"
