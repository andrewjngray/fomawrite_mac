# Run from START-FOMAWRITE.cmd; Windows PowerShell 5.1, no policy changes.
[CmdletBinding()]
param([string]$Package = $env:FOMAWRITE_LAUNCHER_DIR, [switch]$StageOnly)
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
function Get-Sha256([string]$Path) {
    (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash.ToLowerInvariant()
}
function Get-PackagePath([string]$Root, [string]$Name) {
    if ([string]::IsNullOrWhiteSpace($Name) -or [IO.Path]::IsPathRooted($Name) -or $Name.Contains(':')) {
        throw "Unsafe manifest path: $Name"
    }
    foreach ($part in ($Name -split '[\\/]')) {
        if ($part -eq '' -or $part -eq '.' -or $part -eq '..' -or $part -match '[. ]$' -or
            $part -match '^(CON|PRN|AUX|NUL|COM[1-9]|LPT[1-9])(\.|$)') { throw "Unsafe manifest path: $Name" }
    }
    $path = [IO.Path]::GetFullPath((Join-Path $Root $Name))
    if (-not $path.StartsWith($Root + '\', [StringComparison]::OrdinalIgnoreCase)) { throw "Manifest path escapes package: $Name" }
    return $path
}
function Assert-NoPackageLinks([string]$Root, [string]$Path) {
    # The package root itself may be a Mac shared-folder alias; entries below
    # that root must be ordinary files/directories, never escaping junctions.
    $current = $Path
    while ($current.Length -gt $Root.Length) {
        $item = Get-Item -LiteralPath $current -Force
        if (($item.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) { throw "Package contains a link: $current" }
        $current = [IO.Path]::GetDirectoryName($current)
    }
}
function Assert-Package([string]$Root, $Manifest, [string]$ManifestHash, [switch]$ExactFiles) {
    $manifestFile = Join-Path $Root 'manifest.json'
    if ((Get-Sha256 $manifestFile) -ne $ManifestHash) { throw 'Package manifest changed during staging or differs from the source.' }
    $allowed = @{'manifest.json' = $true}
    foreach ($entry in $Manifest.files.PSObject.Properties) {
        $path = Get-PackagePath $Root $entry.Name
        if ($entry.Name -ieq 'manifest.json' -or $entry.Value -cnotmatch '^[0-9a-f]{64}$') { throw "Invalid manifest entry: $($entry.Name)" }
        Assert-NoPackageLinks $Root $path
        if (-not (Test-Path -LiteralPath $path -PathType Leaf) -or (Get-Sha256 $path) -ne $entry.Value) {
            throw "Package file hash mismatch: $($entry.Name)"
        }
        $relative = $path.Substring($Root.Length + 1)
        if ($allowed.ContainsKey($relative)) { throw "Duplicate manifest path: $($entry.Name)" }
        $allowed[$relative] = $true
    }
    $exeEntry = $Manifest.files.PSObject.Properties['Fomawrite.exe']
    if (-not $exeEntry -or $exeEntry.Value -ne $Manifest.executableSha256) { throw 'Executable identity is absent from or inconsistent with the manifest.' }
    if ($ExactFiles) {
        $directories = New-Object 'System.Collections.Generic.Queue[string]'
        $directories.Enqueue($Root)
        while ($directories.Count -gt 0) {
            foreach ($item in Get-ChildItem -LiteralPath $directories.Dequeue() -Force) {
                if (($item.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) { throw "Unexpected link in local package cache: $($item.FullName)" }
                if ($item.PSIsContainer) { $directories.Enqueue($item.FullName) }
                elseif (-not $allowed.ContainsKey($item.FullName.Substring($Root.Length + 1))) { throw "Unexpected file in local package cache: $($item.FullName)" }
            }
        }
    }
    if ((Get-Sha256 $manifestFile) -ne $ManifestHash) { throw 'Source manifest changed while verifying files.' }
}
function Invoke-LocalLaunch {
    if ($env:OS -ne 'Windows_NT') { throw 'This starter requires Windows.' }
    if ([string]::IsNullOrWhiteSpace($Package) -or [string]::IsNullOrWhiteSpace($env:LOCALAPPDATA)) { throw 'Package and LOCALAPPDATA must be available.' }
    $resolvedPackage = Resolve-Path -LiteralPath $Package
    if ($resolvedPackage.Provider.Name -ne 'FileSystem') { throw 'Package must use the Windows filesystem provider.' }
    $source = [IO.Path]::GetFullPath($resolvedPackage.ProviderPath).TrimEnd('\')
    $manifestPath = Join-Path $source 'manifest.json'
    $manifestHash = Get-Sha256 $manifestPath
    $manifest = Get-Content -LiteralPath $manifestPath -Raw | ConvertFrom-Json
    if ($manifest.architecture -notin @('arm64', 'x64') -or $manifest.commit -cnotmatch '^[0-9a-f]{40}$') { throw 'Manifest architecture or full commit identity is invalid.' }
    Assert-Package $source $manifest $manifestHash
    $cacheRoot = Join-Path $env:LOCALAPPDATA 'Fomawrite\Portable'
    New-Item -ItemType Directory -Path $cacheRoot -Force | Out-Null
    $cacheRoot = [IO.Path]::GetFullPath($cacheRoot).TrimEnd('\')
    $version = $manifest.architecture + '-' + $manifest.commit.Substring(0, 12) + '-' + $manifestHash.Substring(0, 16)
    $destination = Join-Path $cacheRoot $version
    $lock = $null
    $temporary = $null
    $reused = $false
    $deadline = [DateTime]::UtcNow.AddSeconds(120)
    try {
        do {
            try { $lock = [IO.File]::Open((Join-Path $cacheRoot '.stage-lock'), [IO.FileMode]::OpenOrCreate, [IO.FileAccess]::ReadWrite, [IO.FileShare]::None) }
            catch [IO.IOException] {
                if ([DateTime]::UtcNow -ge $deadline) { throw 'Another starter is still staging files; timed out waiting for the local cache lock.' }
                Start-Sleep -Milliseconds 200
            }
        } while ($null -eq $lock)
        if (Test-Path -LiteralPath $destination) {
            Assert-NoPackageLinks $cacheRoot $destination
            Assert-Package $destination $manifest $manifestHash -ExactFiles
            $reused = $true
        } else {
            $temporary = Join-Path $cacheRoot ($version + '.staging-' + [Guid]::NewGuid().ToString('N'))
            New-Item -ItemType Directory -Path $temporary | Out-Null
            foreach ($entry in $manifest.files.PSObject.Properties) {
                $from = Get-PackagePath $source $entry.Name
                $to = Get-PackagePath $temporary $entry.Name
                New-Item -ItemType Directory -Path ([IO.Path]::GetDirectoryName($to)) -Force | Out-Null
                Copy-Item -LiteralPath $from -Destination $to
            }
            Copy-Item -LiteralPath $manifestPath -Destination (Join-Path $temporary 'manifest.json')
            Assert-Package $temporary $manifest $manifestHash -ExactFiles
            # Same-parent directory rename publishes a completely verified tree.
            # Existing versions are never replaced, including running versions.
            [IO.Directory]::Move($temporary, $destination)
            $temporary = $null
        }
        Assert-Package $destination $manifest $manifestHash -ExactFiles
    } finally {
        if ($temporary -and (Test-Path -LiteralPath $temporary)) { Remove-Item -LiteralPath $temporary -Recurse -Force }
        if ($lock) { $lock.Dispose() }
    }
    $localExecutable = Join-Path $destination 'Fomawrite.exe'
    $receipt = [ordered]@{
        localExecutable = $localExecutable; sourcePackage = $source; localPackage = $destination
        architecture = $manifest.architecture; commit = $manifest.commit; manifestSha256 = $manifestHash
        executableSha256 = Get-Sha256 $localExecutable; reusedCache = $reused; stageOnly = [bool]$StageOnly
    }
    if (-not $StageOnly) {
        $process = Start-Process -FilePath $localExecutable -WorkingDirectory $destination -PassThru
        $receipt.processId = $process.Id
    }
    $receipt | ConvertTo-Json -Compress
}
try { Invoke-LocalLaunch }
catch {
    if (-not $StageOnly) {
        Add-Type -AssemblyName System.Windows.Forms
        [Windows.Forms.MessageBox]::Show(('Fomawrite could not start: ' + $_.Exception.Message), 'Fomawrite', 'OK', 'Error') | Out-Null
    }
    throw
}
