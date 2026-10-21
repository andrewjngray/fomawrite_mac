[CmdletBinding()]
param([ValidateSet('arm64', 'x64')][string]$Architecture = 'arm64', [string]$Iscc)
. "$PSScriptRoot/common.ps1"
Assert-Windows
$stage = Join-Path $RepoRoot "dist/windows-$Architecture/Fomawrite"
$manifest = Get-Content -LiteralPath (Join-Path $stage 'manifest.json') -Raw | ConvertFrom-Json
if ($manifest.architecture -ne $Architecture) { throw 'Package architecture mismatch.' }
Invoke-Checked python @("$PSScriptRoot/check-document-views.py", '--package', $stage, '--verify-only')
$evidence = Join-Path $RepoRoot "build-tests/windows-$Architecture/deployed-acceptance/package-verification.json"
$verification = Get-Content -LiteralPath $evidence -Raw | ConvertFrom-Json
$manifestHash = (Get-FileHash -LiteralPath (Join-Path $stage 'manifest.json') -Algorithm SHA256).Hash.ToLowerInvariant()
if (-not $verification.passed -or $verification.executableSha256 -ne $manifest.executableSha256 -or
    $verification.manifestSha256 -ne $manifestHash -or $verification.commit -ne $manifest.commit) {
    throw 'Run deployed acceptance on this portable package before compiling its installer.'
}
if (-not $Iscc) {
    $command = Get-Command ISCC.exe -ErrorAction SilentlyContinue
    if ($command) { $Iscc = $command.Source }
    else { $Iscc = Join-Path ${env:ProgramFiles(x86)} 'Inno Setup 6/ISCC.exe' }
}
if (-not (Test-Path -LiteralPath $Iscc)) { throw 'Inno Setup 6.3 or later is required.' }
$output = Join-Path $RepoRoot "dist/windows-$Architecture"
if ($manifest.applicationVersion -notmatch '^(\d+)\.(\d+)\.(\d+)') { throw 'Invalid application version.' }
$numericVersion = "$($Matches[1]).$($Matches[2]).$($Matches[3]).0"
$commit = $manifest.commit.Substring(0, 12)
Invoke-Checked $Iscc @("/DStageDir=$stage", "/DOutputDir=$output", "/DAppVersion=$($manifest.applicationVersion)", "/DAppNumericVersion=$numericVersion", "/DBuildCommit=$commit", "/DArchitecture=$Architecture", (Join-Path $RepoRoot 'windows/Fomawrite.iss'))
$installer = Join-Path $output "Fomawrite-$($manifest.applicationVersion)-windows-$Architecture-$commit-setup.exe"
$hash = (Get-FileHash -LiteralPath $installer -Algorithm SHA256).Hash.ToLowerInvariant()
"$hash  $([IO.Path]::GetFileName($installer))" | Set-Content -LiteralPath "$installer.sha256" -Encoding utf8NoBOM
Write-Host "Compiled $installer; install, genuine version upgrade and uninstall remain separate acceptance gates."
