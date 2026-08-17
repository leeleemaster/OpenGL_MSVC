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
    $OutputDirectory = Join-Path $repositoryRoot "docs\screenshots"
}

$Executable = [System.IO.Path]::GetFullPath($Executable)
$OutputDirectory = [System.IO.Path]::GetFullPath($OutputDirectory)
if (-not (Test-Path -LiteralPath $Executable)) {
    throw "DentalViz executable was not found: $Executable"
}

[void](New-Item -ItemType Directory -Path $OutputDirectory -Force)

Add-Type -AssemblyName System.Drawing
Add-Type -TypeDefinition @'
using System;
using System.Runtime.InteropServices;

public static class DentalVizScreenshotNative
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

    $screenPoint = New-Object DentalVizScreenshotNative+Point
    $screenPoint.X = $ClientX
    $screenPoint.Y = $ClientY
    if (-not [DentalVizScreenshotNative]::ClientToScreen($Window, [ref]$screenPoint)) {
        throw "Could not convert DentalViz client coordinates."
    }
    [void][DentalVizScreenshotNative]::SetForegroundWindow($Window)
    Start-Sleep -Milliseconds 120
    [void][DentalVizScreenshotNative]::SetCursorPos($screenPoint.X, $screenPoint.Y)
    $position = Get-LongParameter -ClientX $ClientX -ClientY $ClientY
    [void][DentalVizScreenshotNative]::SendMessage($Window, 0x0200, [UIntPtr]::Zero, $position)
    [void][DentalVizScreenshotNative]::SendMessage($Window, 0x0201, [UIntPtr]1, $position)
    Start-Sleep -Milliseconds 100
    [void][DentalVizScreenshotNative]::SendMessage($Window, 0x0202, [UIntPtr]::Zero, $position)
    Start-Sleep -Milliseconds 350
}

function Send-KeyPress {
    param(
        [Parameter(Mandatory = $true)]
        [IntPtr]$Window,

        [Parameter(Mandatory = $true)]
        [int]$VirtualKey
    )

    [void][DentalVizScreenshotNative]::SendMessage(
        $Window, 0x0100, [UIntPtr]$VirtualKey, [IntPtr]::Zero)
    Start-Sleep -Milliseconds 150
    [void][DentalVizScreenshotNative]::SendMessage(
        $Window, 0x0101, [UIntPtr]$VirtualKey, [IntPtr]::Zero)
    Start-Sleep -Milliseconds 350
}

function Save-WindowScreenshot {
    param(
        [Parameter(Mandatory = $true)]
        [IntPtr]$Window,

        [Parameter(Mandatory = $true)]
        [string]$Path
    )

    $windowRectangle = New-Object DentalVizScreenshotNative+Rect
    if (-not [DentalVizScreenshotNative]::GetWindowRect($Window, [ref]$windowRectangle)) {
        throw "Could not read the DentalViz window rectangle."
    }

    $width = $windowRectangle.Right - $windowRectangle.Left
    $height = $windowRectangle.Bottom - $windowRectangle.Top
    $bitmap = New-Object System.Drawing.Bitmap $width, $height
    $graphics = [System.Drawing.Graphics]::FromImage($bitmap)
    $deviceContext = $graphics.GetHdc()
    try {
        if (-not [DentalVizScreenshotNative]::PrintWindow($Window, $deviceContext, 2)) {
            throw "Windows could not capture the DentalViz window."
        }
    } finally {
        $graphics.ReleaseHdc($deviceContext)
    }

    try {
        $bitmap.Save($Path, [System.Drawing.Imaging.ImageFormat]::Png)
    } finally {
        $graphics.Dispose()
        $bitmap.Dispose()
    }
    Write-Output "Captured: $Path"
}

$viewerProcess = Start-Process `
    -FilePath $Executable `
    -ArgumentList @("--smoke-seconds", "120") `
    -WorkingDirectory $repositoryRoot `
    -WindowStyle Normal `
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
    Save-WindowScreenshot -Window $windowHandle `
        -Path (Join-Path $OutputDirectory "01_overview.png")

    Send-KeyPress -Window $windowHandle -VirtualKey 0x32
    Save-WindowScreenshot -Window $windowHandle `
        -Path (Join-Path $OutputDirectory "02_wireframe.png")

    Send-KeyPress -Window $windowHandle -VirtualKey 0x31
    Send-ClientClick -Window $windowHandle -ClientX 155 -ClientY 485
    Send-ClientClick -Window $windowHandle -ClientX 31 -ClientY 517
    Send-ClientClick -Window $windowHandle -ClientX 255 -ClientY 581
    Save-WindowScreenshot -Window $windowHandle `
        -Path (Join-Path $OutputDirectory "05_clipping.png")

    Send-ClientClick -Window $windowHandle -ClientX 31 -ClientY 517
    Send-ClientClick -Window $windowHandle -ClientX 70 -ClientY 484
    Send-ClientClick -Window $windowHandle -ClientX 70 -ClientY 484
    Send-ClientClick -Window $windowHandle -ClientX 830 -ClientY 250
    Save-WindowScreenshot -Window $windowHandle `
        -Path (Join-Path $OutputDirectory "03_picking.png")

    Send-ClientClick -Window $windowHandle -ClientX 860 -ClientY 450
    Save-WindowScreenshot -Window $windowHandle `
        -Path (Join-Path $OutputDirectory "04_measurement.png")
} finally {
    if (-not $viewerProcess.HasExited) {
        [void][DentalVizScreenshotNative]::SendMessage(
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
