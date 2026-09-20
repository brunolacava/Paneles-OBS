<#
.SYNOPSIS
  Copies a build of Stream Panels into the classic OBS Studio folder layout:

      <OBS>\obs-plugins\64bit\stream-panels.dll
      <OBS>\data\obs-plugins\stream-panels\locale\*.ini

.DESCRIPTION
  This is the "manual copy" alternative to `cmake --install`, which installs to
  %ALLUSERSPROFILE%\obs-studio\plugins\stream-panels\{bin\64bit,data}.
  Run it from an elevated PowerShell when OBS lives under "Program Files".

.EXAMPLE
  .\scripts\install-legacy-windows.ps1 -Config Release
  .\scripts\install-legacy-windows.ps1 -Config RelWithDebInfo -ObsDir "D:\Apps\obs-studio"
#>
param(
  [ValidateSet("Debug", "RelWithDebInfo", "Release")]
  [string]$Config = "RelWithDebInfo",
  [string]$ObsDir = "C:\Program Files\obs-studio"
)

$ErrorActionPreference = "Stop"
$repo   = Resolve-Path (Join-Path $PSScriptRoot "..")
$rundir = Join-Path $repo "build_x64\rundir\$Config"
$dll    = Join-Path $rundir "stream-panels.dll"

if (-not (Test-Path $dll))            { throw "Build not found: $dll. Build the '$Config' configuration first." }
if (-not (Test-Path "$ObsDir\bin\64bit\obs64.exe")) { throw "OBS Studio not found in '$ObsDir'. Use -ObsDir." }
if (Get-Process obs64 -ErrorAction SilentlyContinue) { throw "Close OBS Studio first (it locks the plugin DLL)." }

$pluginDir = Join-Path $ObsDir "obs-plugins\64bit"
$dataDir   = Join-Path $ObsDir "data\obs-plugins\stream-panels"

New-Item -ItemType Directory -Force -Path $pluginDir, $dataDir | Out-Null
Copy-Item $dll $pluginDir -Force
$pdb = Join-Path $rundir "stream-panels.pdb"
if (Test-Path $pdb) { Copy-Item $pdb $pluginDir -Force }
Copy-Item (Join-Path $rundir "stream-panels\*") $dataDir -Recurse -Force

Write-Host "Installed:"
Write-Host "  $pluginDir\stream-panels.dll"
Write-Host "  $dataDir\locale\*.ini"
