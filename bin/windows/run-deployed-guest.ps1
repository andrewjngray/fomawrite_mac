# Windows PowerShell 5.1 entry point; no Python, PowerShell 7 or Qt SDK needed.
# Run in the current desktop session. Copy the complete output directory to the
# host, then run check-document-views.py --verify-report <report.json>
# --package <copied-package> --expected-executable <this guest Fomawrite.exe>.
[CmdletBinding()]
param([Parameter(Mandatory = $true)][string]$Package,
      [Parameter(Mandatory = $true)][string]$Output)
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
if ($env:OS -ne 'Windows_NT') { throw 'Run on the Windows desktop.' }
$packageRoot = [IO.Path]::GetFullPath((Resolve-Path -LiteralPath $Package).Path).TrimEnd('\')
$executable = Join-Path $packageRoot 'Fomawrite.exe'
$manifestPath = Join-Path $packageRoot 'manifest.json'
$manifest = Get-Content -LiteralPath $manifestPath -Raw | ConvertFrom-Json
foreach ($entry in $manifest.files.PSObject.Properties) {
    $path = [IO.Path]::GetFullPath((Join-Path $packageRoot $entry.Name))
    if ([IO.Path]::IsPathRooted($entry.Name) -or
        -not $path.StartsWith($packageRoot + '\', [StringComparison]::OrdinalIgnoreCase)) {
        throw "Invalid manifest path: $($entry.Name)"
    }
    $actual = (Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash.ToLowerInvariant()
    if ($actual -ne $entry.Value) { throw "Deployed file hash mismatch: $($entry.Name)" }
}
$executableHash = (Get-FileHash -LiteralPath $executable -Algorithm SHA256).Hash.ToLowerInvariant()
if ($executableHash -ne $manifest.executableSha256) { throw 'Executable differs from package manifest.' }
$outputRoot = [IO.Path]::GetFullPath($Output)
if (Test-Path -LiteralPath $outputRoot) { throw 'Use a new output directory; old evidence cannot satisfy acceptance.' }
New-Item -ItemType Directory -Path $outputRoot -Force | Out-Null
$saved = @{}
foreach ($entry in Get-ChildItem Env:) {
    if ($entry.Name -match '^(QT_|QTWEBENGINE|QML_|FOMAWRITE_)' -or $entry.Name -eq 'PATH') {
        $saved[$entry.Name] = $entry.Value
    }
}
$started = [DateTime]::UtcNow
$exitCode = -1
try {
    foreach ($name in $saved.Keys) { [Environment]::SetEnvironmentVariable($name, $null, 'Process') }
    $env:PATH = $packageRoot + ';' + (Join-Path $env:SystemRoot 'System32') + ';' + $env:SystemRoot
    $env:QT_FORCE_STDERR_LOGGING = '1'
    $process = Start-Process -FilePath $executable -WorkingDirectory $packageRoot -PassThru `
        -ArgumentList @('--check-document-views', ('"{0}"' -f $outputRoot)) `
        -RedirectStandardOutput (Join-Path $outputRoot 'stdout.log') `
        -RedirectStandardError (Join-Path $outputRoot 'stderr.log')
    # Keep the handle open so Windows PowerShell 5.1 retains the exit code.
    $process.Handle | Out-Null
    if (-not $process.WaitForExit(360000)) {
        $process.Kill()
        $process.WaitForExit()
        throw 'Deployed acceptance timed out after 360 seconds.'
    }
    $process.WaitForExit()
    $exitCode = $process.ExitCode
} finally {
    Remove-Item Env:QT_FORCE_STDERR_LOGGING -ErrorAction SilentlyContinue
    Remove-Item Env:PATH -ErrorAction SilentlyContinue
    foreach ($name in $saved.Keys) { [Environment]::SetEnvironmentVariable($name, $saved[$name], 'Process') }
}
$reportPath = Join-Path $outputRoot 'report.json'
$fresh = (Test-Path -LiteralPath $reportPath) -and (Get-Item -LiteralPath $reportPath).LastWriteTimeUtc -ge $started
$reportHash = if ($fresh) { (Get-FileHash -LiteralPath $reportPath -Algorithm SHA256).Hash.ToLowerInvariant() } else { $null }
$receipt = [ordered]@{
    executable = $executable; executableSha256 = $executableHash
    manifestSha256 = (Get-FileHash -LiteralPath $manifestPath -Algorithm SHA256).Hash.ToLowerInvariant()
    reportSha256 = $reportHash; freshReport = [bool]$fresh; exitCode = $exitCode
    startedUtc = $started.ToString('o'); finishedUtc = [DateTime]::UtcNow.ToString('o')
    powershellVersion = $PSVersionTable.PSVersion.ToString()
}
$receipt | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath (Join-Path $outputRoot 'guest-run.json') -Encoding UTF8
if (-not $fresh) { throw 'No fresh report from deployed executable.' }
if ($exitCode -ne 0) { throw "Deployed executable failed with status $exitCode; inspect $outputRoot" }
Write-Output "Guest run captured at $outputRoot; strict host report verification is still required."
