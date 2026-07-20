#Requires -Version 5.1
<#
  Build helper for FateHacks, called by .vscode/tasks.json.

    .\build.ps1    # unload old DLL -> compile -> inject new DLL

  Every build runs the project's Pre/PostBuild events (DLLInjector.exe against
  fate.exe): the old DLL is unloaded first (freeing the file lock so the linker
  can overwrite it), then the freshly built DLL is injected. Runs at normal
  (non-elevated) integrity -- no UAC prompt. If FATE isn't running the
  unload/inject steps are harmless no-ops and the build still succeeds. If the
  injector reports "Access is denied", launch VS Code as administrator.
#>
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$sln  = Join-Path $root 'FateHacks.sln'

# Locate MSBuild via vswhere so this isn't tied to a specific VS install path.
$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
if (-not (Test-Path $vswhere)) { throw "vswhere not found -- is Visual Studio / Build Tools installed?" }
$msbuild = & $vswhere -latest -requires Microsoft.Component.MSBuild -find 'MSBuild\**\Bin\MSBuild.exe' | Select-Object -First 1
if (-not $msbuild) { throw "MSBuild not found via vswhere." }

if (-not (Get-Process -Name fate -ErrorAction SilentlyContinue)) {
    Write-Warning "fate.exe is not running -- compiling only; unload/inject will be no-ops."
}

# Full build WITH the project's Pre/PostBuild events: unload -> compile -> load.
& $msbuild $sln /p:Configuration=Debug /p:Platform=x86 /nologo /v:minimal
exit $LASTEXITCODE
