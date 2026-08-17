param(
    [string]$Executable = "",
    [string]$OutputDirectory = ""
)

$ErrorActionPreference = "Stop"

$repositoryRoot = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot ".."))
if ([string]::IsNullOrWhiteSpace($Executable)) {
    $Executable = Join-Path $repositoryRoot "out\build\msvc\Release\DentalViz.exe"
}
if ([string]::IsNullOrWhiteSpace($OutputDirectory)) {
    $OutputDirectory = Join-Path $repositoryRoot "out\verification\commit-18"
}

$Executable = [System.IO.Path]::GetFullPath($Executable)
$OutputDirectory = [System.IO.Path]::GetFullPath($OutputDirectory)
if (-not (Test-Path -LiteralPath $Executable -PathType Leaf)) {
    throw "DentalViz executable was not found: $Executable"
}
[void](New-Item -ItemType Directory -Path $OutputDirectory -Force)

Add-Type -AssemblyName System.Drawing
Add-Type -AssemblyName System.Windows.Forms
Add-Type -TypeDefinition @'
using System;
using System.Runtime.InteropServices;

public static class MiniShaderVerificationNative
{
    [StructLayout(LayoutKind.Sequential)]
    public struct Point
    {
        public int X;
        public int Y;
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct Rect
    {
        public int Left;
        public int Top;
        public int Right;
        public int Bottom;
    }

    [DllImport("user32.dll")]
    public static extern bool ClientToScreen(IntPtr window, ref Point point);

    [DllImport("user32.dll")]
    public static extern bool GetWindowRect(IntPtr window, out Rect rect);

    [DllImport("user32.dll")]
    public static extern bool PrintWindow(IntPtr window, IntPtr deviceContext, uint flags);

    [DllImport("user32.dll")]
    public static extern IntPtr SendMessage(
        IntPtr window,
        uint message,
        UIntPtr wordParameter,
        IntPtr longParameter);

    [DllImport("user32.dll")]
    public static extern bool SetCursorPos(int x, int y);

    [DllImport("user32.dll")]
    public static extern bool SetForegroundWindow(IntPtr window);
}
'@

function Get-LongParameter {
    param(
        [Parameter(Mandatory = $true)]
        [int]$ClientX,

        [Parameter(Mandatory = $true)]
        [int]$ClientY
    )

    return [IntPtr](($ClientY -shl 16) -bor ($ClientX -band 0xFFFF))
}

function Send-ClientClick {
    param(
        [Parameter(Mandatory = $true)]
        [IntPtr]$Window,

        [Parameter(Mandatory = $true)]
        [int]$ClientX,

        [Parameter(Mandatory = $true)]
        [int]$ClientY
    )

    $screenPoint = New-Object MiniShaderVerificationNative+Point
    $screenPoint.X = $ClientX
    $screenPoint.Y = $ClientY
    if (-not [MiniShaderVerificationNative]::ClientToScreen(
            $Window, [ref]$screenPoint)) {
        throw "Could not convert DentalViz client coordinates."
    }
    [void][MiniShaderVerificationNative]::SetForegroundWindow($Window)
    Start-Sleep -Milliseconds 120
    [void][MiniShaderVerificationNative]::SetCursorPos($screenPoint.X, $screenPoint.Y)
    $position = Get-LongParameter -ClientX $ClientX -ClientY $ClientY
    [void][MiniShaderVerificationNative]::SendMessage(
        $Window, 0x0200, [UIntPtr]::Zero, $position)
    [void][MiniShaderVerificationNative]::SendMessage(
        $Window, 0x0201, [UIntPtr]1, $position)
    Start-Sleep -Milliseconds 80
    [void][MiniShaderVerificationNative]::SendMessage(
        $Window, 0x0202, [UIntPtr]::Zero, $position)
    Start-Sleep -Milliseconds 350
}

function Set-EditorText {
    param(
        [Parameter(Mandatory = $true)]
        [IntPtr]$Window,

        [Parameter(Mandatory = $true)]
        [string]$Text
    )

    Send-ClientClick -Window $Window -ClientX 170 -ClientY 645
    [void][MiniShaderVerificationNative]::SetForegroundWindow($Window)
    [System.Windows.Forms.SendKeys]::SendWait("^a")
    foreach ($character in $Text.ToCharArray()) {
        $key = switch ($character) {
            '+' { '{+}' }
            '^' { '{^}' }
            '%' { '{%}' }
            '~' { '{~}' }
            '(' { '{(}' }
            ')' { '{)}' }
            '[' { '{[}' }
            ']' { '{]}' }
            '{' { '{{}' }
            '}' { '{}}' }
            default { [string]$character }
        }
        [System.Windows.Forms.SendKeys]::SendWait($key)
        Start-Sleep -Milliseconds 10
    }
    Start-Sleep -Milliseconds 500
}

function Save-WindowAndViewerCapture {
    param(
        [Parameter(Mandatory = $true)]
        [IntPtr]$Window,

        [Parameter(Mandatory = $true)]
        [string]$Name
    )

    $windowRectangle = New-Object MiniShaderVerificationNative+Rect
    if (-not [MiniShaderVerificationNative]::GetWindowRect(
            $Window, [ref]$windowRectangle)) {
        throw "Could not read the DentalViz window rectangle."
    }

    $width = $windowRectangle.Right - $windowRectangle.Left
    $height = $windowRectangle.Bottom - $windowRectangle.Top
    $bitmap = New-Object System.Drawing.Bitmap $width, $height
    $graphics = [System.Drawing.Graphics]::FromImage($bitmap)
    $deviceContext = $graphics.GetHdc()
    try {
        if (-not [MiniShaderVerificationNative]::PrintWindow(
                $Window, $deviceContext, 2)) {
            throw "Windows could not capture the DentalViz window."
        }
    } finally {
        $graphics.ReleaseHdc($deviceContext)
        $graphics.Dispose()
    }

    try {
        $windowPath = Join-Path $OutputDirectory "$Name.png"
        $bitmap.Save($windowPath, [System.Drawing.Imaging.ImageFormat]::Png)

        $viewerRectangle = New-Object System.Drawing.Rectangle 420, 40, ($width - 430), ($height - 50)
        $viewerBitmap = $bitmap.Clone(
            $viewerRectangle,
            [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
        try {
            $viewerPath = Join-Path $OutputDirectory "$Name-viewer.png"
            $viewerBitmap.Save($viewerPath, [System.Drawing.Imaging.ImageFormat]::Png)
        } finally {
            $viewerBitmap.Dispose()
        }
    } finally {
        $bitmap.Dispose()
    }
}

$standardOutputPath = Join-Path $OutputDirectory "DentalViz.stdout.txt"
$standardErrorPath = Join-Path $OutputDirectory "DentalViz.stderr.txt"
$viewerProcess = Start-Process `
    -FilePath $Executable `
    -ArgumentList @("--smoke-seconds", "120") `
    -WorkingDirectory $repositoryRoot `
    -WindowStyle Normal `
    -RedirectStandardOutput $standardOutputPath `
    -RedirectStandardError $standardErrorPath `
    -PassThru

try {
    $windowHandle = [IntPtr]::Zero
    for ($attempt = 0; $attempt -lt 100; $attempt++) {
        Start-Sleep -Milliseconds 100
        $viewerProcess.Refresh()
        $windowHandle = $viewerProcess.MainWindowHandle
        if ($windowHandle -ne [IntPtr]::Zero) {
            break
        }
    }
    if ($windowHandle -eq [IntPtr]::Zero) {
        throw "DentalViz did not create a window within 10 seconds."
    }

    Start-Sleep -Seconds 1
    Send-ClientClick -Window $windowHandle -ClientX 245 -ClientY 480
    Send-ClientClick -Window $windowHandle -ClientX 110 -ClientY 570
    Start-Sleep -Seconds 1
    Save-WindowAndViewerCapture -Window $windowHandle -Name "01-default-applied"

    $modifiedSource = "material Dental { let n = normalize(normal); let l = normalize(lightDir); let diffuse = max(dot(n, l), 0.0); let intensity = 0.5 + diffuse; output = baseColor * intensity; }"
    Set-EditorText -Window $windowHandle -Text $modifiedSource
    Send-ClientClick -Window $windowHandle -ClientX 110 -ClientY 570
    Start-Sleep -Seconds 1
    Save-WindowAndViewerCapture -Window $windowHandle -Name "02-modified-applied"

    Set-EditorText -Window $windowHandle -Text "material Broken { output = missing; }"
    Send-ClientClick -Window $windowHandle -ClientX 110 -ClientY 570
    Start-Sleep -Seconds 1
    Save-WindowAndViewerCapture -Window $windowHandle -Name "03-invalid-last-known-good"
} finally {
    if (-not $viewerProcess.HasExited) {
        [void][MiniShaderVerificationNative]::SendMessage(
            $viewerProcess.MainWindowHandle,
            0x0010,
            [UIntPtr]::Zero,
            [IntPtr]::Zero)
        if (-not $viewerProcess.WaitForExit(5000)) {
            $viewerProcess.Kill()
            $viewerProcess.WaitForExit()
        }
    }
    $viewerProcess.Dispose()
}

$standardOutput = Get-Content -LiteralPath $standardOutputPath -Raw
$standardError = Get-Content -LiteralPath $standardErrorPath -Raw
$successCount = ([regex]::Matches($standardOutput, "MiniShader 개정 [0-9]+ 적용됨")).Count
if ($successCount -ne 2) {
    throw "Expected two successful runtime applies, but found $successCount."
}
if (-not $standardError.Contains("알 수 없는 식별자: missing.")) {
    throw "The invalid source did not produce the expected semantic diagnostic."
}

$defaultHash = (Get-FileHash -Algorithm SHA256 -LiteralPath `
    (Join-Path $OutputDirectory "01-default-applied-viewer.png")).Hash
$modifiedHash = (Get-FileHash -Algorithm SHA256 -LiteralPath `
    (Join-Path $OutputDirectory "02-modified-applied-viewer.png")).Hash
$lastKnownGoodHash = (Get-FileHash -Algorithm SHA256 -LiteralPath `
    (Join-Path $OutputDirectory "03-invalid-last-known-good-viewer.png")).Hash

if ($defaultHash -eq $modifiedHash) {
    throw "Changing intensity from 0.2 to 0.5 did not change the viewer capture."
}
if ($modifiedHash -ne $lastKnownGoodHash) {
    throw "The viewer changed after invalid source; Last Known Good was not preserved."
}

Write-Output "MiniShader runtime verification: PASS"
Write-Output "Successful applies: $successCount"
Write-Output "Default viewer SHA-256:  $defaultHash"
Write-Output "Modified viewer SHA-256: $modifiedHash"
Write-Output "LKG viewer SHA-256:      $lastKnownGoodHash"
