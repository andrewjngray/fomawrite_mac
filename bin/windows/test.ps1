[CmdletBinding()]
param([ValidateSet('arm64', 'x64')][string]$Architecture = 'arm64', [string]$QtRoot)
. "$PSScriptRoot/common.ps1"
Assert-Windows
$QtRoot = Get-QtRoot $QtRoot
Enter-MsvcEnvironment $Architecture
$env:PATH = (Join-Path $QtRoot 'bin') + ';' + $env:PATH
$build = Join-Path $RepoRoot "build-tests/windows-$Architecture"
New-Item -ItemType Directory -Force -Path $build | Out-Null
Push-Location $build
try {
    Invoke-Checked (Join-Path $QtRoot 'bin/qmake.exe') @((Join-Path $RepoRoot 'tests/tests.pro'), 'CONFIG+=release', 'CONFIG-=debug_and_release', 'CONFIG-=debug')
    Invoke-Checked nmake @('/NOLOGO')
    $env:QT_QPA_PLATFORM = 'offscreen'
    Invoke-Checked (Join-Path $build 'tst_fomawrite.exe') @('-o', 'test.xml,junitxml', '-o', 'test.log,txt')
} finally { Pop-Location; Remove-Item Env:QT_QPA_PLATFORM -ErrorAction SilentlyContinue }
