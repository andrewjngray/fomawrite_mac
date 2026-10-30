# Repository-root Launch.cmd entry point. Windows PowerShell 5.1; no policy changes.
[CmdletBinding()]
param(
    [string]$RepositoryRoot = '',
    [switch]$StageOnly,
    [ValidateSet('arm64', 'x64')][string]$Architecture = '',
    [string]$DocumentPath = ''
)
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
function Get-CurrentHash([string]$Path) {
    (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash.ToLowerInvariant()
}
function Get-NativeArchitecture {
    # A 32-bit/x64 PowerShell process on ARM64 must still select native ARM64.
    $native = $env:PROCESSOR_ARCHITEW6432
    if ([string]::IsNullOrWhiteSpace($native)) { $native = $env:PROCESSOR_ARCHITECTURE }
    if ([string]::IsNullOrWhiteSpace($native)) {
        try { $native = [Runtime.InteropServices.RuntimeInformation]::OSArchitecture.ToString() }
        catch { throw 'Cannot determine the native Windows architecture.' }
    }
    switch ($native.ToUpperInvariant()) {
        'ARM64' { return 'arm64' }
        'AMD64' { return 'x64' }
        'X64' { return 'x64' }
        default { throw "Unsupported native Windows architecture: $native" }
    }
}
function Assert-CurrentLinks([string]$Root, [string]$Path) {
    # The repository itself may be a shared-folder alias; children must not escape it.
    $current = $Path
    while ($current.Length -gt $Root.Length) {
        $item = Get-Item -LiteralPath $current -Force
        if (($item.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) { throw "Current channel contains a link: $current" }
        $current = [IO.Path]::GetDirectoryName($current)
    }
}
function Get-WorkspacePath([string]$Executable) {
    if ([string]::IsNullOrWhiteSpace($env:USERPROFILE) -or [string]::IsNullOrWhiteSpace($env:APPDATA)) {
        throw 'USERPROFILE and APPDATA must be available to preserve the normal workspace.'
    }
    # Match QDir::homePath() + QCoreApplication::applicationFilePath(), with Qt slashes.
    $homePath = [IO.Path]::GetFullPath($env:USERPROFILE).Replace('\', '/').TrimEnd('/')
    $inputBytes = [Text.Encoding]::UTF8.GetBytes($homePath + $Executable.Replace('\', '/'))
    $sha = [Security.Cryptography.SHA256]::Create()
    try { $identity = ([BitConverter]::ToString($sha.ComputeHash($inputBytes))).Replace('-', '').ToLowerInvariant().Substring(0, 20) }
    finally { $sha.Dispose() }
    Join-Path (Join-Path $env:APPDATA 'AndrewGray\fomawrite') ('workspace-' + $identity + '.json')
}
function Read-LastLaunch([string]$CacheRoot, [string]$Path) {
    if (-not (Test-Path -LiteralPath $Path)) { return $null }
    try {
        Assert-CurrentLinks $CacheRoot $Path
        if ((Get-Item -LiteralPath $Path).Length -gt 16384) { throw 'Receipt is too large.' }
        $previous = [IO.File]::ReadAllText($Path) | ConvertFrom-Json
        if (-not ($previous.schemaVersion -is [int] -or $previous.schemaVersion -is [long]) -or $previous.schemaVersion -ne 1 -or
            $previous.localExecutable -isnot [string]) { throw 'Invalid receipt schema.' }
        $prefix = $CacheRoot + '\'
        if (-not $previous.localExecutable.StartsWith($prefix, [StringComparison]::OrdinalIgnoreCase) -or
            $previous.localExecutable.Substring($prefix.Length) -cnotmatch '^(arm64|x64)-[0-9a-f]{12}-[0-9a-f]{16}\\Fomawrite\.exe$') {
            throw 'Receipt executable is outside the managed version cache.'
        }
        return $previous
    } catch { throw ('The last-launch receipt is invalid; workspace transfer was stopped: ' + $_.Exception.Message) }
}
function Copy-PreviousWorkspace([string]$PreviousExecutable, [string]$Executable) {
    $destination = Get-WorkspacePath $Executable
    if (Test-Path -LiteralPath $destination) { return $false }
    $source = Get-WorkspacePath $PreviousExecutable
    if (-not (Test-Path -LiteralPath $source -PathType Leaf)) { return $false }
    $item = Get-Item -LiteralPath $source -Force
    if (($item.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0 -or $item.Length -gt 1048576) { throw 'Previous workspace is a link or exceeds the supported size.' }
    $bytes = [IO.File]::ReadAllBytes($source)
    $utf8 = New-Object Text.UTF8Encoding($false, $true)
    $state = $utf8.GetString($bytes).TrimStart([char]0xfeff) | ConvertFrom-Json
    if (-not ($state.version -is [int] -or $state.version -is [long]) -or $state.version -ne 1 -or
        $state.windows -isnot [array] -or $state.windows.Count -gt 100) { throw 'Previous workspace has an unsupported version or windows list.' }
    foreach ($window in $state.windows) {
        if ($null -eq $window -or -not $window.PSObject.Properties['url'] -or $window.url -isnot [string]) { throw 'Previous workspace has an invalid window URL.' }
    }
    $directory = [IO.Path]::GetDirectoryName($destination)
    New-Item -ItemType Directory -Path $directory -Force | Out-Null
    $temporary = $destination + '.transfer-' + [Guid]::NewGuid().ToString('N')
    try {
        [IO.File]::WriteAllBytes($temporary, $bytes)
        # Never overwrite an existing destination, including a concurrent publication.
        try { [IO.File]::Move($temporary, $destination) }
        catch [IO.IOException] { if (-not (Test-Path -LiteralPath $destination)) { throw }; return $false }
    } finally { if (Test-Path -LiteralPath $temporary) { Remove-Item -LiteralPath $temporary -Force } }
    return $true
}
function Save-LastLaunch([string]$Path, [string]$Executable) {
    $temporary = $Path + '.writing-' + [Guid]::NewGuid().ToString('N')
    try {
        $json = [ordered]@{ schemaVersion = 1; localExecutable = $Executable } | ConvertTo-Json -Compress
        [IO.File]::WriteAllText($temporary, $json, (New-Object Text.UTF8Encoding($false)))
        # PowerShell 5.1 binds $null to an empty string for this .NET overload.
        if (Test-Path -LiteralPath $Path) { [IO.File]::Replace($temporary, $Path, [NullString]::Value) }
        else { [IO.File]::Move($temporary, $Path) }
    } finally { if (Test-Path -LiteralPath $temporary) { Remove-Item -LiteralPath $temporary -Force } }
}
function Invoke-CurrentLaunch {
    if ($env:OS -ne 'Windows_NT') { throw 'This launcher requires Windows.' }
    if ($Architecture -and -not $StageOnly) { throw 'Architecture override is only available with -StageOnly for testing.' }
    if ([string]::IsNullOrWhiteSpace($RepositoryRoot)) {
        if ([string]::IsNullOrWhiteSpace($PSScriptRoot)) { throw 'RepositoryRoot is required when invoking this launcher as a ScriptBlock.' }
        $RepositoryRoot = Join-Path $PSScriptRoot '..\..'
    }
    $resolved = Resolve-Path -LiteralPath $RepositoryRoot
    if ($resolved.Provider.Name -ne 'FileSystem') { throw 'RepositoryRoot must use the Windows filesystem provider.' }
    $root = [IO.Path]::GetFullPath($resolved.ProviderPath).TrimEnd('\')
    $channelPath = Join-Path $root 'dist\windows-testing\current.json'
    if (-not (Test-Path -LiteralPath $channelPath -PathType Leaf)) {
        throw 'No current downloaded Windows build is available. Run Update.command on the Mac, then click Launch again.'
    }
    Assert-CurrentLinks $root $channelPath
    $channel = Get-Content -LiteralPath $channelPath -Raw | ConvertFrom-Json
    if (-not ($channel.schemaVersion -is [int] -or $channel.schemaVersion -is [long]) -or $channel.schemaVersion -ne 1 -or
        -not ($channel.runId -is [int] -or $channel.runId -is [long]) -or $channel.runId -le 0 -or
        $channel.commit -isnot [string] -or $channel.commit -cnotmatch '^[0-9a-f]{40}$') {
        throw 'Current channel schema, run ID or full commit identity is invalid. Run Update.command on the Mac.'
    }
    if (-not $Architecture) { $Architecture = Get-NativeArchitecture }
    $selected = $channel.packages.PSObject.Properties[$Architecture]
    if (-not $selected) { throw "Current channel has no $Architecture package. Run Update.command on the Mac." }
    $entry = $selected.Value
    $expectedPath = 'dist/windows-testing/downloads/' + $channel.runId + '/' + $Architecture + '/Fomawrite'
    if ($entry.path -isnot [string] -or $entry.path -cne $expectedPath) { throw 'Unsafe or inconsistent current channel package path.' }
    if ($entry.manifestSha256 -isnot [string] -or $entry.manifestSha256 -cnotmatch '^[0-9a-f]{64}$') { throw 'Current channel manifest hash is invalid.' }
    $package = Join-Path $root $entry.path
    $manifestPath = Join-Path $package 'manifest.json'
    if (-not (Test-Path -LiteralPath $manifestPath -PathType Leaf)) { throw 'Current package is missing. Run Update.command on the Mac.' }
    Assert-CurrentLinks $root $manifestPath
    if ((Get-CurrentHash $manifestPath) -cne $entry.manifestSha256) { throw 'Current channel package manifest hash mismatch. Run Update.command on the Mac.' }
    $manifest = Get-Content -LiteralPath $manifestPath -Raw | ConvertFrom-Json
    if ($manifest.architecture -cne $Architecture -or $manifest.commit -cne $channel.commit) { throw 'Current channel and package architecture or commit do not match.' }
    $document = ''
    if (-not [string]::IsNullOrWhiteSpace($DocumentPath)) {
        $documentItem = Get-Item -LiteralPath $DocumentPath
        if ($documentItem.PSIsContainer -or $documentItem.PSProvider.Name -ne 'FileSystem') { throw 'Supply an existing document file.' }
        $document = $documentItem.FullName
        if ($document.Contains('"')) { throw 'The document path contains an invalid quote.' }
    }
    $version = if ($manifest.PSObject.Properties['applicationVersion']) { $manifest.applicationVersion } else { $channel.commit.Substring(0, 12) }
    Write-Host ('Fomawrite ' + $version + ' (' + $Architecture + '). Checking the downloaded build; no sign-in needed.')
    $stager = [ScriptBlock]::Create([IO.File]::ReadAllText((Join-Path $root 'bin\windows\launch-local.ps1')))
    $receipt = (& $stager -Package $package -StageOnly) | ConvertFrom-Json
    # Check the stager's actual identity too, detecting a source change between reads.
    if ($receipt.manifestSha256 -cne $entry.manifestSha256 -or $receipt.architecture -cne $Architecture -or $receipt.commit -cne $channel.commit) {
        throw 'Current package changed while staging; click Launch again after Update finishes.'
    }
    $receipt.stageOnly = [bool]$StageOnly
    $receipt | Add-Member -NotePropertyName channelRunId -NotePropertyValue $channel.runId
    if (-not $StageOnly) {
        $cacheRoot = [IO.Path]::GetDirectoryName($receipt.localPackage)
        $lastLaunchPath = Join-Path $cacheRoot 'last-launch.json'
        $previous = Read-LastLaunch $cacheRoot $lastLaunchPath
        $running = @(Get-Process -Name Fomawrite -ErrorAction SilentlyContinue)
        $currentProcesses = @($running | Where-Object {
            try { $_.Path -and [string]::Equals($_.Path, $receipt.localExecutable, [StringComparison]::OrdinalIgnoreCase) } catch { $false }
        })
        $olderProcesses = @($running | Where-Object {
            try { $_.Path -and $_.Path.StartsWith($cacheRoot + '\', [StringComparison]::OrdinalIgnoreCase) -and
                -not [string]::Equals($_.Path, $receipt.localExecutable, [StringComparison]::OrdinalIgnoreCase) } catch { $false }
        })
        if ($olderProcesses.Count -gt 0) {
            throw 'An older Fomawrite build is still running. Save your writing and close that build normally, then click Launch again to load the updated build and its workspace.'
        }
        $copied = $false
        if ($previous -and -not [string]::Equals($previous.localExecutable, $receipt.localExecutable, [StringComparison]::OrdinalIgnoreCase)) {
            $copied = Copy-PreviousWorkspace $previous.localExecutable $receipt.localExecutable
        }
        $receipt | Add-Member -NotePropertyName workspaceCopied -NotePropertyValue $copied
        $options = @{ FilePath = $receipt.localExecutable; WorkingDirectory = $receipt.localPackage; PassThru = $true }
        # Relaunching this exact executable forwards through InstanceBroker, restoring
        # and activating its existing window rather than creating a second workspace.
        if ($document) { $options.ArgumentList = '"' + $document + '"' }
        $process = Start-Process @options
        Save-LastLaunch $lastLaunchPath $receipt.localExecutable
        $receipt | Add-Member -NotePropertyName processId -NotePropertyValue $(if ($currentProcesses.Count -gt 0) { $currentProcesses[0].Id } else { $process.Id })
        $receipt | Add-Member -NotePropertyName alreadyRunning -NotePropertyValue ($currentProcesses.Count -gt 0)
        if ($currentProcesses.Count -gt 0 -and -not $document) { Write-Host 'The running build has been asked to bring your window forward.' }
    }
    $receipt | ConvertTo-Json -Compress
}
Invoke-CurrentLaunch
