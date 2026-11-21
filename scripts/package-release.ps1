param(
    [Parameter(Mandatory = $true)]
    [string]$Version
)

# Builds the Windows release zip from build/ (Visual Studio Release build).
#
# Usage: powershell -ExecutionPolicy Bypass -File .\scripts\package-release.ps1 -Version 1.0.1

$ErrorActionPreference = 'Stop'

$root = Split-Path -Parent $PSScriptRoot
$build = Join-Path $root 'build\IRLoader_artefacts\Release'
$stage = Join-Path $root "dist\Simple IR VST $Version"
$zip = Join-Path $root "dist\Simple-IR-VST-$Version-Windows-x64.zip"

$vst3 = Join-Path $build 'VST3\IR Loader.vst3'
$exe = Join-Path $build 'Standalone\IR Loader.exe'

if (-not (Test-Path $vst3) -or -not (Test-Path $exe)) {
    throw "Missing Windows build output in $build. Run the release build first."
}

Remove-Item -Recurse -Force (Join-Path $root 'dist') -ErrorAction SilentlyContinue
New-Item -ItemType Directory -Force -Path $stage | Out-Null

Copy-Item -Recurse -Path $vst3 -Destination $stage
Copy-Item -Path $exe -Destination $stage
Copy-Item -Path (Join-Path $root 'Packaging\Windows\Install.ps1') -Destination $stage
Copy-Item -Path (Join-Path $root 'Packaging\Windows\README.txt') -Destination $stage

# Zip the containing folder so extraction produces one tidy directory.
Compress-Archive -Path $stage -DestinationPath $zip -Force

Write-Host "Created $zip"
Get-FileHash -Path $zip -Algorithm SHA256 | Format-List
