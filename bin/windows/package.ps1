[CmdletBinding()]
param([ValidateSet('arm64', 'x64')][string]$Architecture = 'arm64', [string]$QtRoot)
. "$PSScriptRoot/common.ps1"
Assert-Windows
$QtRoot = Get-QtRoot $QtRoot
Enter-MsvcEnvironment $Architecture
$env:PATH = (Join-Path $QtRoot 'bin') + ';' + $env:PATH
$exe = Join-Path $RepoRoot "build/windows-$Architecture/fomawrite.exe"
if (-not (Test-Path -LiteralPath $exe)) { throw 'Run build.ps1 first.' }
Invoke-Checked python @("$PSScriptRoot/build_identity.py", 'verify', '--architecture', $Architecture, '--build', (Split-Path -Parent $exe))
$stage = Join-Path $RepoRoot "dist/windows-$Architecture/Fomawrite"
if (Test-Path -LiteralPath $stage) { Remove-Item -LiteralPath $stage -Recurse -Force }
New-Item -ItemType Directory -Force -Path $stage | Out-Null
Copy-Item -LiteralPath $exe -Destination (Join-Path $stage 'Fomawrite.exe')
Invoke-Checked (Join-Path $QtRoot 'bin/windeployqt.exe') @('--release', '--no-compiler-runtime', '--qmldir', (Join-Path $RepoRoot 'src'), '--dir', $stage, (Join-Path $stage 'Fomawrite.exe'))
# windeployqt may bundle a vc_redist installer rather than app-local DLLs. A
# portable ZIP must launch without elevation or an installed development kit.
# package-manifest inventories actual native CRT DLL versions and prefers the
# selected compiler's v143 runtime; compatible newer v14 runtimes are explicit.
Invoke-Checked python @("$PSScriptRoot/package-manifest.py", '--architecture', $Architecture, '--qt-root', $QtRoot, '--stage', $stage)
