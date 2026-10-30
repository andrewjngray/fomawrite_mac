# Synthetic packages and mocked process calls: never starts application UI or uses a network.
[CmdletBinding()]
param([string]$DriverPath = '')
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
if ([string]::IsNullOrWhiteSpace($DriverPath)) { $DriverPath = Join-Path $PSScriptRoot '..\bin\windows\launch-current.ps1' }
$driver = [ScriptBlock]::Create([IO.File]::ReadAllText($DriverPath))
$stagerPath = Join-Path ([IO.Path]::GetDirectoryName($DriverPath)) 'launch-local.ps1'
$fixture = Join-Path $env:TEMP ('Fomawrite-current-fixture-' + [Guid]::NewGuid().ToString('N'))
$repository = Join-Path $fixture 'repository with spaces'
$cacheHome = Join-Path $fixture 'local app data'
$channelPath = Join-Path $repository 'dist\windows-testing\current.json'
$savedLocalAppData = $env:LOCALAPPDATA
$savedAppData = $env:APPDATA
$savedUserProfile = $env:USERPROFILE
$savedNativeArchitecture = $env:PROCESSOR_ARCHITEW6432
$savedProcessArchitecture = $env:PROCESSOR_ARCHITECTURE
$checks = New-Object 'System.Collections.Generic.List[string]'
$script:networkCalls = 0
$script:processCalls = New-Object 'System.Collections.Generic.List[object]'
$script:mockProcesses = @()
function Invoke-WebRequest { $script:networkCalls++; throw 'Network is unavailable in this fixture.' }
function Invoke-RestMethod { $script:networkCalls++; throw 'Network is unavailable in this fixture.' }
function Start-BitsTransfer { $script:networkCalls++; throw 'Network is unavailable in this fixture.' }
function Get-Process {
    [CmdletBinding()]param([string]$Name)
    $script:mockProcesses
}
function Start-Process {
    [CmdletBinding()]param([string]$FilePath, [string]$WorkingDirectory, [string]$ArgumentList = '', [switch]$PassThru)
    $script:processCalls.Add([pscustomobject]@{ FilePath = $FilePath; WorkingDirectory = $WorkingDirectory; ArgumentList = $ArgumentList })
    [pscustomobject]@{ Id = 4242 }
}
function Hash([string]$Path) { (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash.ToLowerInvariant() }
function Assert([bool]$Condition, [string]$Message) { if (-not $Condition) { throw $Message } }
function Reject([scriptblock]$Action, [string]$Message, [string]$ExpectedReason) {
    $rejected = $false
    $reason = ''
    try { & $Action | Out-Null } catch { $rejected = $true; $reason = $_.Exception.Message }
    Assert $rejected $Message
    Assert ($reason -match $ExpectedReason) ('Wrong rejection reason: ' + $reason)
}
function New-Build([int]$RunId, [string]$Commit) {
    $packages = [ordered]@{}
    foreach ($arch in @('arm64', 'x64')) {
        $relative = "dist/windows-testing/downloads/$RunId/$arch/Fomawrite"
        $package = Join-Path $repository $relative
        New-Item -ItemType Directory -Path $package -Force | Out-Null
        [IO.File]::WriteAllText((Join-Path $package 'Fomawrite.exe'), "Synthetic $arch executable $Commit - NEVER RUN")
        $hash = Hash (Join-Path $package 'Fomawrite.exe')
        [ordered]@{ architecture = $arch; commit = $Commit; executableSha256 = $hash; files = @{'Fomawrite.exe' = $hash} } |
            ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $package 'manifest.json') -Encoding UTF8
        $packages[$arch] = [ordered]@{ path = $relative; manifestSha256 = Hash (Join-Path $package 'manifest.json') }
    }
    [ordered]@{ schemaVersion = 1; runId = $RunId; commit = $Commit; packages = $packages }
}
function Write-Channel($Channel) { $Channel | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath $channelPath -Encoding UTF8 }
function Workspace-Path([string]$Executable) {
    $qtPath = [IO.Path]::GetFullPath($env:USERPROFILE).Replace('\', '/') + $Executable.Replace('\', '/')
    $hasher = [Security.Cryptography.SHA256]::Create()
    try { $identity = -join ($hasher.ComputeHash([Text.Encoding]::UTF8.GetBytes($qtPath)) | ForEach-Object { $_.ToString('x2') }) }
    finally { $hasher.Dispose() }
    Join-Path (Join-Path $env:APPDATA 'AndrewGray\fomawrite') ('workspace-' + $identity.Substring(0, 20) + '.json')
}
function Write-Last([string]$Path, [string]$Executable) {
    [ordered]@{ schemaVersion = 1; localExecutable = $Executable } | ConvertTo-Json -Compress | Set-Content -LiteralPath $Path -Encoding UTF8
}
try {
    New-Item -ItemType Directory -Path (Join-Path $repository 'bin\windows'), $cacheHome -Force | Out-Null
    Copy-Item -LiteralPath $stagerPath -Destination (Join-Path $repository 'bin\windows\launch-local.ps1')
    $env:LOCALAPPDATA = $cacheHome
    $env:APPDATA = Join-Path $fixture 'roaming app data'
    $env:USERPROFILE = Join-Path $fixture 'user profile'
    $env:PROCESSOR_ARCHITEW6432 = 'ARM64'
    $env:PROCESSOR_ARCHITECTURE = 'AMD64'
    Reject { & $driver -RepositoryRoot $repository -StageOnly } 'Missing channel was accepted.' 'Run Update.command on the Mac'
    $checks.Add('missing-channel-explains-mac-update')

    $channel = New-Build 100 ('a' * 40)
    Write-Channel $channel
    $first = (& $driver -RepositoryRoot $repository -StageOnly) | ConvertFrom-Json
    Assert ($first.architecture -eq 'arm64' -and $first.channelRunId -eq 100 -and -not $first.reusedCache -and $first.stageOnly) 'Native ARM64 did not override emulated x64 process architecture.'
    Assert ($first.commit -eq ('a' * 40) -and (Hash $first.localExecutable) -eq $first.executableSha256) 'Receipt identity or staged bytes differ.'
    $checks.Add('arm64-native-selection-and-exact-staging-receipt')

    $env:PROCESSOR_ARCHITEW6432 = ''
    $env:PROCESSOR_ARCHITECTURE = 'AMD64'
    $x64 = (& $driver -RepositoryRoot $repository -StageOnly) | ConvertFrom-Json
    Assert ($x64.architecture -eq 'x64' -and $x64.localPackage -ne $first.localPackage) 'Native x64 fallback was not selected.'
    $override = (& $driver -RepositoryRoot $repository -StageOnly -Architecture arm64) | ConvertFrom-Json
    Assert ($override.architecture -eq 'arm64' -and $override.reusedCache) 'Test architecture override failed.'
    Reject { & $driver -RepositoryRoot $repository -Architecture arm64 } 'Production architecture override was accepted.' 'only available with -StageOnly'
    $env:PROCESSOR_ARCHITEW6432 = 'ARM64'
    $checks.Add('x64-fallback-and-test-only-override')

    $timestamp = (Get-Item -LiteralPath $first.localExecutable).LastWriteTimeUtc.Ticks
    $reuse = (& $driver -RepositoryRoot $repository -StageOnly) | ConvertFrom-Json
    Assert ($reuse.reusedCache -and $reuse.localExecutable -eq $first.localExecutable -and (Get-Item -LiteralPath $first.localExecutable).LastWriteTimeUtc.Ticks -eq $timestamp) 'Same-version launch rewrote its verified cache.'
    $checks.Add('every-click-reverifies-and-reuses-cache')

    $channel2 = New-Build 101 ('b' * 40)
    Write-Channel $channel2
    $second = (& $driver -RepositoryRoot $repository -StageOnly) | ConvertFrom-Json
    Assert ($second.channelRunId -eq 101 -and $second.commit -eq ('b' * 40) -and $second.localPackage -ne $first.localPackage -and -not $second.reusedCache) 'Changed channel did not select a fresh immutable version.'
    Assert ((Hash $first.localExecutable) -eq $first.executableSha256) 'New version altered the old cached version.'
    $checks.Add('changed-channel-selects-new-version-preserving-old-cache')

    $manifestPath = Join-Path (Join-Path $repository $channel2.packages.arm64.path) 'manifest.json'
    $manifestBytes = [IO.File]::ReadAllBytes($manifestPath)
    [IO.File]::AppendAllText($manifestPath, ' ')
    Reject { & $driver -RepositoryRoot $repository -StageOnly } 'Changed manifest was accepted.' 'manifest hash mismatch'
    [IO.File]::WriteAllBytes($manifestPath, $manifestBytes)
    $checks.Add('channel-manifest-hash-is-required-even-with-cached-version')

    foreach ($badPath in @('../outside', 'C:\outside', '\\server\share\Fomawrite', 'dist/windows-testing/downloads/101/arm64/../Fomawrite', 'dist/windows-testing/downloads/100/arm64/Fomawrite', 'dist/windows-testing/downloads/101/x64/Fomawrite', 'dist/windows-testing/downloads/101/arm64/Fomawrite:stream')) {
        $channel2.packages.arm64.path = $badPath
        Write-Channel $channel2
        Reject { & $driver -RepositoryRoot $repository -StageOnly } "Unsafe channel path was accepted: $badPath" 'package path'
    }
    $channel2.packages.arm64.path = 'dist/windows-testing/downloads/101/arm64/Fomawrite'
    $checks.Add('traversal-rooted-ads-and-inconsistent-channel-paths-rejected')

    foreach ($field in @('architecture', 'commit')) {
        $manifest = [IO.File]::ReadAllText($manifestPath) | ConvertFrom-Json
        if ($field -eq 'architecture') { $manifest.architecture = 'x64' } else { $manifest.commit = ('c' * 40) }
        $manifest | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath $manifestPath -Encoding UTF8
        $channel2.packages.arm64.manifestSha256 = Hash $manifestPath
        Write-Channel $channel2
        Reject { & $driver -RepositoryRoot $repository -StageOnly } "Mismatched manifest $field was accepted." 'architecture or commit do not match'
        [IO.File]::WriteAllBytes($manifestPath, $manifestBytes)
    }
    $channel2.packages.arm64.manifestSha256 = Hash $manifestPath
    Write-Channel $channel2
    $checks.Add('manifest-architecture-and-commit-match-channel')

    foreach ($invalid in @(@{schemaVersion = '1'}, @{schemaVersion = 2}, @{runId = '101'}, @{runId = -1}, @{commit = 'short'})) {
        $badChannel = ($channel2 | ConvertTo-Json -Depth 8) | ConvertFrom-Json
        foreach ($key in $invalid.Keys) { $badChannel.$key = $invalid[$key] }
        Write-Channel $badChannel
        Reject { & $driver -RepositoryRoot $repository -StageOnly } 'Invalid channel schema was accepted.' 'schema, run ID or full commit identity is invalid'
    }
    Write-Channel $channel2
    Remove-Item -LiteralPath $channelPath
    Reject { & $driver -RepositoryRoot $repository -StageOnly } 'Missing channel fell back to a cached version.' 'Run Update.command on the Mac'
    Write-Channel $channel2
    $checks.Add('invalid-or-missing-current-channel-never-falls-back')

    $lastLaunch = Join-Path (Join-Path $cacheHome 'Fomawrite\Portable') 'last-launch.json'
    Assert (-not (Test-Path -LiteralPath $env:APPDATA) -and -not (Test-Path -LiteralPath $lastLaunch)) 'StageOnly touched the production workspace or launch receipt.'
    $checks.Add('stage-only-never-creates-production-state')

    $sourceWorkspace = Workspace-Path $first.localExecutable
    $destinationWorkspace = Workspace-Path $second.localExecutable
    New-Item -ItemType Directory -Path ([IO.Path]::GetDirectoryName($sourceWorkspace)) -Force | Out-Null
    $workspaceText = '{ "version": 1, "windows": [{"url":"file:///C:/writing/my%20draft.md","caret":17}], "untouched":"extra state" }'
    [IO.File]::WriteAllText($sourceWorkspace, $workspaceText, (New-Object Text.UTF8Encoding($false)))
    $sourceHash = Hash $sourceWorkspace
    $orphanRecovery = Join-Path ([IO.Path]::GetDirectoryName($sourceWorkspace)) 'recovery-orphan.json'
    [IO.File]::WriteAllText($orphanRecovery, 'orphan recovery must remain untouched')
    $orphanHash = Hash $orphanRecovery
    Write-Last $lastLaunch $first.localExecutable
    $lastHash = Hash $lastLaunch
    $stagedAgain = (& $driver -RepositoryRoot $repository -StageOnly) | ConvertFrom-Json
    Assert ((Hash $lastLaunch) -eq $lastHash -and -not (Test-Path -LiteralPath $destinationWorkspace) -and (Hash $sourceWorkspace) -eq $sourceHash) 'StageOnly migrated, remembered or altered production state.'
    $checks.Add('stage-only-never-reads-or-migrates-existing-workspace')

    foreach ($badReceipt in @('{broken', '{"schemaVersion":2,"localExecutable":"C:\\outside.exe"}', '{"schemaVersion":1,"localExecutable":"C:\\outside.exe"}')) {
        [IO.File]::WriteAllText($lastLaunch, $badReceipt)
        Reject { & $driver -RepositoryRoot $repository } 'Invalid last-launch receipt was accepted.' 'last-launch receipt is invalid'
        Assert (-not (Test-Path -LiteralPath $destinationWorkspace) -and $script:processCalls.Count -eq 0) 'Bad receipt changed workspace or started a process.'
        $ignoredBadReceipt = (& $driver -RepositoryRoot $repository -StageOnly) | ConvertFrom-Json
        Assert ($ignoredBadReceipt.stageOnly) 'StageOnly depended on a production launch receipt.'
    }
    Write-Last $lastLaunch $first.localExecutable
    $checks.Add('invalid-receipt-blocks-normal-transfer-but-not-stage-only')

    foreach ($badState in @('{"version":2,"windows":[]}', '{"version":1,"windows":{}}', '{"version":1,"windows":[{"url":12}]}')) {
        [IO.File]::WriteAllText($sourceWorkspace, $badState)
        Reject { & $driver -RepositoryRoot $repository } 'Invalid previous workspace was transferred.' 'Previous workspace'
        Assert (-not (Test-Path -LiteralPath $destinationWorkspace) -and $script:processCalls.Count -eq 0) 'Invalid workspace changed production state.'
    }
    [IO.File]::WriteAllText($sourceWorkspace, $workspaceText, (New-Object Text.UTF8Encoding($false)))
    $checks.Add('unsupported-workspace-schema-is-never-copied')

    $script:mockProcesses = @([pscustomobject]@{ Path = $first.localExecutable; Id = 2345 })
    Reject { & $driver -RepositoryRoot $repository } 'An update started while the old build was running.' 'close that build normally'
    Assert ($script:processCalls.Count -eq 0 -and -not (Test-Path -LiteralPath $destinationWorkspace) -and (Hash $lastLaunch) -eq $lastHash) 'Running old build was replaced or production state changed.'
    $checks.Add('running-old-version-requires-normal-close-before-update')

    $script:mockProcesses = @()
    $updated = (& $driver -RepositoryRoot $repository) | ConvertFrom-Json
    $remembered = [IO.File]::ReadAllText($lastLaunch) | ConvertFrom-Json
    Assert ($updated.workspaceCopied -and $script:processCalls.Count -eq 1 -and $remembered.localExecutable -eq $second.localExecutable) 'New build did not migrate and remember its previous workspace.'
    Assert ((Hash $sourceWorkspace) -eq $sourceHash -and (Hash $destinationWorkspace) -eq $sourceHash -and (Hash $orphanRecovery) -eq $orphanHash) 'Workspace was rewritten, moved or orphan recovery was changed.'
    $checks.Add('closed-old-version-workspace-copies-byte-exactly-once')

    $destinationText = '{"version":1,"windows":[{"url":"file:///C:/existing.md"}]}'
    [IO.File]::WriteAllText($destinationWorkspace, $destinationText)
    $destinationHash = Hash $destinationWorkspace
    Write-Last $lastLaunch $first.localExecutable
    $script:mockProcesses = @([pscustomobject]@{ Path = $second.localExecutable; Id = 1234 })
    $existing = (& $driver -RepositoryRoot $repository) | ConvertFrom-Json
    Assert (-not $existing.workspaceCopied -and (Hash $destinationWorkspace) -eq $destinationHash -and (Hash $sourceWorkspace) -eq $sourceHash) 'Existing destination workspace was overwritten.'
    Assert ($existing.alreadyRunning -and $existing.processId -eq 1234 -and $script:processCalls.Count -eq 2 -and $script:processCalls[1].FilePath -eq $second.localExecutable -and $script:processCalls[1].ArgumentList -eq '') 'Same-version relaunch did not forward an activation request to the existing app.'
    $checks.Add('existing-destination-is-preserved-and-running-app-is-foregrounded')

    $document = Join-Path $fixture 'draft & notes with spaces.md'
    [IO.File]::WriteAllText($document, 'ordinary document bytes')
    $before = Hash $document
    $explicit = (& $driver -RepositoryRoot $repository -DocumentPath $document) | ConvertFrom-Json
    # Windows may expand a short TEMP directory or normalize its casing when
    # Get-Item resolves the existing file; the launcher deliberately passes FullName.
    $expectedArgument = '"' + (Get-Item -LiteralPath $document).FullName + '"'
    $documentDiagnostic = [ordered]@{ count = $script:processCalls.Count; expected = $expectedArgument; calls = @($script:processCalls.ToArray()) } | ConvertTo-Json -Depth 4 -Compress
    Assert ($script:processCalls.Count -eq 3 -and $script:processCalls[2].ArgumentList -ceq $expectedArgument) ('Explicit document was not passed as one correctly quoted argument. ' + $documentDiagnostic)
    Assert ((Hash $document) -eq $before) 'Explicit document bytes were altered.'
    Assert ($env:LOCALAPPDATA -eq $cacheHome -and $script:processCalls[2].WorkingDirectory -eq $second.localPackage) 'Normal launch changed the user profile or local working directory.'
    $checks.Add('explicit-document-is-quoted-and-writing-is-untouched')

    Remove-Item -LiteralPath $lastLaunch
    $fresh = (& $driver -RepositoryRoot $repository) | ConvertFrom-Json
    Assert (-not $fresh.workspaceCopied -and (Test-Path -LiteralPath $lastLaunch) -and (Hash $destinationWorkspace) -eq $destinationHash) 'First ordinary launch without a receipt did not preserve existing state.'
    $checks.Add('first-normal-launch-needs-no-previous-receipt')

    Assert ($script:networkCalls -eq 0) 'Launcher attempted a network request.'
    $checks.Add('launch-is-local-with-no-authentication-or-network')
    [ordered]@{ passed = $true; checks = @($checks.ToArray()); count = $checks.Count; applicationStarted = $false; processLaunchesMocked = $script:processCalls.Count; networkCalls = $script:networkCalls } | ConvertTo-Json -Depth 5
} finally {
    $env:LOCALAPPDATA = $savedLocalAppData
    $env:APPDATA = $savedAppData
    $env:USERPROFILE = $savedUserProfile
    $env:PROCESSOR_ARCHITEW6432 = $savedNativeArchitecture
    $env:PROCESSOR_ARCHITECTURE = $savedProcessArchitecture
    if (Test-Path -LiteralPath $fixture) { Remove-Item -LiteralPath $fixture -Recurse -Force }
}
