# Non-UI fixture: synthetic executable bytes are staged, never launched.
[CmdletBinding()]
param([string]$DriverPath = '')
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
if ([string]::IsNullOrWhiteSpace($DriverPath)) { $driverPath = Join-Path $PSScriptRoot '..\bin\windows\launch-local.ps1' }
$driver = [ScriptBlock]::Create([IO.File]::ReadAllText($driverPath))
$root = Join-Path $env:TEMP ('Fomawrite-launcher-fixture-' + [Guid]::NewGuid().ToString('N'))
$package = Join-Path $root 'package with spaces'
$cacheHome = Join-Path $root 'local-app-data'
$previousLocalAppData = $env:LOCALAPPDATA
$checks = New-Object 'System.Collections.Generic.List[string]'
$heldLock = $null
$blockingJob = $null
function Hash([string]$Path) { (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash.ToLowerInvariant() }
function Assert([bool]$Condition, [string]$Message) { if (-not $Condition) { throw $Message } }
function Reject([scriptblock]$Action, [string]$Message, [string]$ExpectedReason) {
    $rejected = $false
    $reason = ''
    try { & $Action | Out-Null } catch { $rejected = $true; $reason = $_.Exception.Message }
    Assert $rejected $Message
    Assert ($reason -match $ExpectedReason) ("Wrong rejection reason: " + $reason)
}
function Get-JobDiagnostic($Job) {
    $children = @($Job.ChildJobs | ForEach-Object {
        [ordered]@{ state = [string]$_.State; reason = [string]$_.JobStateInfo.Reason;
            errors = @($_.Error | ForEach-Object { $_.ToString() }); output = @($_.Output | ForEach-Object { $_.ToString() }) }
    })
    [ordered]@{ state = [string]$Job.State; reason = [string]$Job.JobStateInfo.Reason; children = $children } | ConvertTo-Json -Depth 7 -Compress
}
function Write-Manifest($Files) {
    $manifest = [ordered]@{ architecture = 'arm64'; commit = ('a' * 40); executableSha256 = Hash (Join-Path $package 'Fomawrite.exe'); files = $Files }
    $manifest | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $package 'manifest.json') -Encoding UTF8
}
try {
    New-Item -ItemType Directory -Path (Join-Path $package 'nested'), $cacheHome -Force | Out-Null
    [IO.File]::WriteAllText((Join-Path $package 'Fomawrite.exe'), 'synthetic executable - NEVER RUN')
    [IO.File]::WriteAllText((Join-Path $package 'nested\dependency.dll'), 'synthetic library')
    [IO.File]::WriteAllText((Join-Path $package 'unlisted-private.md'), 'must not be staged')
    $files = [ordered]@{ 'Fomawrite.exe' = Hash (Join-Path $package 'Fomawrite.exe'); 'nested/dependency.dll' = Hash (Join-Path $package 'nested\dependency.dll') }
    Write-Manifest $files
    $env:LOCALAPPDATA = $cacheHome
    $first = (& $driver -Package $package -StageOnly) | ConvertFrom-Json
    Assert ($first.stageOnly -eq $true -and $first.reusedCache -eq $false) 'First stage did not create a new cache.'
    Assert (Test-Path -LiteralPath $first.localExecutable -PathType Leaf) 'Staged executable is missing.'
    Assert (-not (Test-Path -LiteralPath (Join-Path $first.localPackage 'unlisted-private.md'))) 'Unlisted writing was copied.'
    Assert ((Hash $first.localExecutable) -eq $files['Fomawrite.exe']) 'Staged bytes differ.'
    $checks.Add('new-cache-stages-only-manifest-files')

    $modified = (Get-Item -LiteralPath $first.localExecutable).LastWriteTimeUtc.Ticks
    $second = (& $driver -Package $package -StageOnly) | ConvertFrom-Json
    Assert ($second.reusedCache -eq $true -and $second.localExecutable -eq $first.localExecutable) 'Verified cache was not reused.'
    Assert ((Get-Item -LiteralPath $first.localExecutable).LastWriteTimeUtc.Ticks -eq $modified) 'Reuse rewrote a cached executable.'
    $checks.Add('verified-cache-reuse-does-not-rewrite')

    $lockPath = Join-Path $cacheHome 'Fomawrite\Portable\.stage-lock'
    $marker = Join-Path $root 'waiting-starter-ready.txt'
    $heldLock = [IO.File]::Open($lockPath, [IO.FileMode]::OpenOrCreate, [IO.FileAccess]::ReadWrite, [IO.FileShare]::None)
    $blockingJob = Start-Job -ArgumentList $driverPath, $package, $marker, $cacheHome -ScriptBlock {
        param($DriverPath, $Package, $Marker, $CacheHome)
        $env:LOCALAPPDATA = $CacheHome
        [IO.File]::WriteAllText($Marker, 'ready')
        & ([ScriptBlock]::Create([IO.File]::ReadAllText($DriverPath))) -Package $Package -StageOnly
    }
    $readyDeadline = [DateTime]::UtcNow.AddSeconds(15)
    while (-not (Test-Path -LiteralPath $marker) -and [DateTime]::UtcNow -lt $readyDeadline) { Start-Sleep -Milliseconds 100 }
    if (-not (Test-Path -LiteralPath $marker)) { throw ('Concurrent starter never reached its launch point. ' + (Get-JobDiagnostic $blockingJob)) }
    Start-Sleep -Milliseconds 750
    if ($blockingJob.State -ne 'Running') { throw ('Concurrent starter did not wait for the cache lock. ' + (Get-JobDiagnostic $blockingJob)) }
    $heldLock.Dispose(); $heldLock = $null
    if ($blockingJob.State -eq 'Running') { Wait-Job $blockingJob -Timeout 60 | Out-Null }
    if ($blockingJob.State -ne 'Completed') { throw ('Concurrent starter did not complete after lock release. ' + (Get-JobDiagnostic $blockingJob)) }
    $serialized = (Receive-Job $blockingJob) | ConvertFrom-Json
    Remove-Job $blockingJob; $blockingJob = $null
    Assert ($serialized.reusedCache -eq $true -and $serialized.localPackage -eq $first.localPackage) 'Concurrent starter did not reverify/reuse the published cache.'
    $checks.Add('concurrent-starter-waits-for-serialized-cache-lock')

    $sourceExe = Join-Path $package 'Fomawrite.exe'
    $sourceBytes = [IO.File]::ReadAllBytes($sourceExe)
    [IO.File]::WriteAllText($sourceExe, 'corrupted source')
    Reject { & $driver -Package $package -StageOnly } 'Corrupted source was accepted.' 'Package file hash mismatch'
    [IO.File]::WriteAllBytes($sourceExe, $sourceBytes)
    $checks.Add('source-corruption-is-rejected-before-cache-use')

    [IO.File]::WriteAllText($first.localExecutable, 'corrupted cache')
    Reject { & $driver -Package $package -StageOnly } 'Corrupted cache was accepted.' 'Package file hash mismatch'
    Assert ([IO.File]::ReadAllText($first.localExecutable) -eq 'corrupted cache') 'Corrupted existing version was overwritten.'
    [IO.File]::WriteAllBytes($first.localExecutable, $sourceBytes)
    $checks.Add('corrupt-cache-is-rejected-without-overwrite')

    [IO.File]::WriteAllText((Join-Path $first.localPackage 'unexpected.dll'), 'unexpected')
    Reject { & $driver -Package $package -StageOnly } 'Unexpected cached executable content was accepted.' 'Unexpected file in local package cache'
    Remove-Item -LiteralPath (Join-Path $first.localPackage 'unexpected.dll')
    $checks.Add('unexpected-cache-files-are-rejected')

    foreach ($name in @('../outside.txt', '..\outside.txt', 'nested/../../outside.txt', 'C:\outside.txt', '\\server\share\outside.txt', 'nested/file.dll:stream', 'nested/./alias.dll', 'nested/trailing.')) {
        $unsafe = [ordered]@{ 'Fomawrite.exe' = $files['Fomawrite.exe']; 'nested/dependency.dll' = $files['nested/dependency.dll'] }
        $unsafe[$name] = ('0' * 64)
        Write-Manifest $unsafe
        Reject { & $driver -Package $package -StageOnly } "Unsafe manifest path was accepted: $name" 'Unsafe manifest path|Manifest path escapes'
    }
    Write-Manifest $files
    $checks.Add('traversal-rooted-ads-and-ambiguous-paths-are-rejected')

    $changed = [ordered]@{ 'Fomawrite.exe' = $files['Fomawrite.exe']; 'nested/dependency.dll' = $files['nested/dependency.dll']; 'new.dll' = '' }
    [IO.File]::WriteAllText((Join-Path $package 'new.dll'), 'new dependency')
    $changed['new.dll'] = Hash (Join-Path $package 'new.dll')
    Write-Manifest $changed
    $third = (& $driver -Package $package -StageOnly) | ConvertFrom-Json
    Assert ($third.reusedCache -eq $false -and $third.localPackage -ne $first.localPackage) 'Different manifest reused an old version directory.'
    Assert ((Hash $first.localExecutable) -eq $files['Fomawrite.exe']) 'Staging a different manifest altered the previous version.'
    $checks.Add('manifest-version-staging-preserves-previous-cache')

    Assert (-not @(Get-ChildItem -LiteralPath (Join-Path $cacheHome 'Fomawrite\Portable') -Directory | Where-Object Name -Like '*.staging-*').Count) 'Temporary staging directories remained.'
    $checks.Add('temporary-staging-is-cleaned')
    [ordered]@{ passed = $true; checks = @($checks.ToArray()); count = $checks.Count; applicationStarted = $false } | ConvertTo-Json -Depth 5
} finally {
    if ($heldLock) { $heldLock.Dispose() }
    if ($blockingJob) { Stop-Job $blockingJob; Remove-Job $blockingJob }
    $env:LOCALAPPDATA = $previousLocalAppData
    if (Test-Path -LiteralPath $root) { Remove-Item -LiteralPath $root -Recurse -Force }
}
