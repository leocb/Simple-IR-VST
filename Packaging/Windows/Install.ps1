# Installs IR Loader (VST3) on Windows.
#
#   powershell -ExecutionPolicy Bypass -File .\Install.ps1          # current user
#   powershell -ExecutionPolicy Bypass -File .\Install.ps1 -System  # all users (admin)

param(
    [switch]$System
)

$ErrorActionPreference = 'Stop'

$here = Split-Path -Parent $MyInvocation.MyCommand.Path
$source = Join-Path $here 'IR Loader.vst3'

if (-not (Test-Path $source)) {
    Write-Host "Could not find 'IR Loader.vst3' next to this script." -ForegroundColor Red
    Read-Host "Press Enter to close"
    exit 1
}

if ($System) {
    $vst3Root = Join-Path $env:ProgramFiles 'Common Files\VST3'
} else {
    $vst3Root = Join-Path $env:LOCALAPPDATA 'Programs\Common\VST3'
}

$destination = Join-Path $vst3Root 'IR Loader.vst3'

Write-Host "Installing IR Loader..."
Write-Host "  $destination"

try {
    New-Item -ItemType Directory -Force -Path $vst3Root | Out-Null

    if (Test-Path $destination) {
        Remove-Item -Recurse -Force $destination
    }

    Copy-Item -Recurse -Force $source $destination
} catch {
    Write-Host ""
    Write-Host "Install failed: $($_.Exception.Message)" -ForegroundColor Red
    Write-Host ""
    Write-Host "For the all-users install, run PowerShell as administrator, or omit"
    Write-Host "-System to install into your own user folder."
    Read-Host "Press Enter to close"
    exit 1
}

Write-Host ""
Write-Host "Installed VST3:" -ForegroundColor Green
Write-Host "  $destination"
Write-Host ""
Write-Host "The standalone app is 'IR Loader.exe' in this folder."
Write-Host "Restart your DAW and rescan plugins if it does not appear."
Write-Host ""

Read-Host "Press Enter to close"
