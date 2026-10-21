[CmdletBinding()]
param([ValidateSet('arm64', 'x64')][string]$Architecture = 'arm64', [string]$QtRoot)
. "$PSScriptRoot/common.ps1"
Assert-Windows
$QtRoot = Get-QtRoot $QtRoot
Enter-MsvcEnvironment $Architecture
$env:PATH = (Join-Path $QtRoot 'bin') + ';' + $env:PATH
$exe = Join-Path $RepoRoot "build/windows-$Architecture/fomawrite.exe"
if (-not (Test-Path -LiteralPath $exe)) { throw 'Run build.ps1 first.' }
$stage = Join-Path $RepoRoot "dist/windows-$Architecture/Fomawrite"
if (Test-Path -LiteralPath $stage) { Remove-Item -LiteralPath $stage -Recurse -Force }
New-Item -ItemType Directory -Force -Path $stage | Out-Null
Copy-Item -LiteralPath $exe -Destination (Join-Path $stage 'Fomawrite.exe')
Invoke-Checked (Join-Path $QtRoot 'bin/windeployqt.exe') @('--release', '--compiler-runtime', '--qmldir', (Join-Path $RepoRoot 'src'), '--dir', $stage, (Join-Path $stage 'Fomawrite.exe'))
Invoke-Checked python @("$PSScriptRoot/package-manifest.py", '--architecture', $Architecture, '--qt-root', $QtRoot, '--stage', $stage)
