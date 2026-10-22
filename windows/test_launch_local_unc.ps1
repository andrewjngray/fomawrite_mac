# Requires a real writable UNC share; stages synthetic bytes and never starts UI.
[CmdletBinding()]
param([Parameter(Mandatory = $true)][string]$SharedRoot)
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
if (-not $SharedRoot.StartsWith('\\')) { throw 'Supply a real UNC path such as \\Mac\Home\repo\projects\fomawrite_windows\windows.' }
$driver = [ScriptBlock]::Create([IO.File]::ReadAllText((Join-Path $PSScriptRoot '..\bin\windows\launch-local.ps1')))
$name = 'Fomawrite-UNC-fixture-' + [Guid]::NewGuid().ToString('N')
$source = Join-Path $SharedRoot $name
$local = Join-Path $env:TEMP $name
$savedLocalAppData = $env:LOCALAPPDATA
try {
    New-Item -ItemType Directory -Path $source, $local | Out-Null
    [IO.File]::WriteAllText((Join-Path $source 'Fomawrite.exe'), 'synthetic UNC executable - NEVER RUN')
    $hash = (Get-FileHash -LiteralPath (Join-Path $source 'Fomawrite.exe') -Algorithm SHA256).Hash.ToLowerInvariant()
    [ordered]@{ architecture = 'arm64'; commit = ('b' * 40); executableSha256 = $hash; files = @{'Fomawrite.exe' = $hash} } |
        ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $source 'manifest.json') -Encoding UTF8
    $env:LOCALAPPDATA = $local
    $first = (& $driver -Package $source -StageOnly) | ConvertFrom-Json
    if (-not $first.sourcePackage.StartsWith('\\') -or $first.sourcePackage.Contains('::') -or $first.reusedCache) { throw 'UNC source was not normalized to its filesystem provider path.' }
    if ((Get-FileHash -LiteralPath $first.localExecutable -Algorithm SHA256).Hash.ToLowerInvariant() -ne $hash) { throw 'UNC-staged bytes differ.' }
    $second = (& $driver -Package $source -StageOnly) | ConvertFrom-Json
    if (-not $second.reusedCache -or $second.localExecutable -ne $first.localExecutable) { throw 'UNC package cache was not reused.' }
    [ordered]@{ passed = $true; checks = @('real-unc-filesystem-path-stages', 'real-unc-verified-cache-reuses'); count = 2; applicationStarted = $false } | ConvertTo-Json
} finally {
    $env:LOCALAPPDATA = $savedLocalAppData
    if (Test-Path -LiteralPath $source) { Remove-Item -LiteralPath $source -Recurse -Force }
    if (Test-Path -LiteralPath $local) { Remove-Item -LiteralPath $local -Recurse -Force }
}
