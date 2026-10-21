[CmdletBinding()]
param([ValidateSet('arm64', 'x64')][string]$Architecture = 'arm64', [string]$QtRoot)
. "$PSScriptRoot/common.ps1"
Assert-Windows
$QtRoot = Get-QtRoot $QtRoot
Enter-MsvcEnvironment $Architecture
$env:PATH = (Join-Path $QtRoot 'bin') + ';' + $env:PATH
Invoke-Checked python @("$PSScriptRoot/fetch-harper.py")
$build = Join-Path $RepoRoot "build/windows-$Architecture"
New-Item -ItemType Directory -Force -Path $build | Out-Null
Invoke-Checked python @("$PSScriptRoot/build_identity.py", 'begin', '--architecture', $Architecture, '--build', $build)
Push-Location $build
try {
    Invoke-Checked (Join-Path $QtRoot 'bin/qmake.exe') @((Join-Path $RepoRoot 'fomawrite.pro'), 'CONFIG+=release', 'CONFIG-=debug_and_release', 'CONFIG-=debug')
    Invoke-Checked nmake @('/NOLOGO')
} finally { Pop-Location }
if (-not (Test-Path -LiteralPath (Join-Path $build 'fomawrite.exe'))) { throw 'Build did not produce fomawrite.exe.' }
Invoke-Checked python @("$PSScriptRoot/build_identity.py", 'finish', '--architecture', $Architecture, '--build', $build)
Write-Host "Built $build/fomawrite.exe"
