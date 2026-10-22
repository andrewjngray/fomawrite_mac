# Supplemental real Windows input gate. Run in a fresh hidden PowerShell 5.1 process.
[CmdletBinding()]
param([Parameter(Mandatory=$true)][string]$Package,
      [Parameter(Mandatory=$true)][string]$Output,
      [ValidateSet('all','settled','immediate','50ms','100ms','150ms')][string]$Case='all',
      [string]$DriverDirectory=$PSScriptRoot)
Set-StrictMode -Version Latest
$ErrorActionPreference='Stop'
if ($env:OS -ne 'Windows_NT' -or $PSVersionTable.PSVersion.Major -ne 5) { throw 'Use a fresh Windows PowerShell 5.1 process in the interactive guest.' }
if ([string]::IsNullOrWhiteSpace($DriverDirectory)) { throw 'ScriptBlock invocation requires -DriverDirectory.' }
$root=[IO.Path]::GetFullPath($DriverDirectory)
$out=[IO.Path]::GetFullPath($Output)
if (Test-Path -LiteralPath $out) { throw 'Use a new supplemental output directory.' }
if (Get-Process -Name Fomawrite -ErrorAction SilentlyContinue) { throw 'Close existing Fomawrite normally before this input gate.' }
$base=[IO.File]::ReadAllText((Join-Path $root 'run-normal-launch-guest.ps1'))
$helper=[IO.File]::ReadAllText((Join-Path $root 'normal-launch-input.cs'))
$steps=[IO.File]::ReadAllText((Join-Path $root 'live-edit-steps.ps1'))
$extra=[IO.File]::ReadAllText((Join-Path $root 'live-edit-input.cs'))
$hook="    Capture 'live-mode-rendered'"
$configuration="`$ErrorActionPreference = 'Stop'"
$failureAnchor='    $failure = $_.Exception.Message'
$failureDiagnostic=$failureAnchor+"`n    `$_.Exception.ToString() | Set-Content -LiteralPath (Join-Path `$outputRoot 'failure-exception.txt') -Encoding UTF8"
foreach ($anchor in @($hook,$configuration,$failureAnchor,"gate = 'windows-normal-launch'")) {
    if (($base.Split(@($anchor),[StringSplitOptions]::None).Count-1) -ne 1) { throw "Normal driver composition anchor changed: $anchor" }
}
New-Item -ItemType Directory -Path $out | Out-Null
$utf8=New-Object Text.UTF8Encoding($false)
$cases=[ordered]@{ settled=-1; immediate=0; '50ms'=50; '100ms'=100; '150ms'=150 }
$results=New-Object 'System.Collections.Generic.List[object]'
function Hash([string]$path) { (Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash.ToLowerInvariant() }
foreach ($name in $cases.Keys) {
    if ($Case -ne 'all' -and $Case -ne $name) { continue }
    $run=Join-Path $out $name
    $drivers=Join-Path $run 'driver'
    New-Item -ItemType Directory -Path $drivers -Force | Out-Null
    $body=$base.Replace($configuration,$configuration+"`n`$script:LiveEditCase = '$name'`n`$script:LiveEditGapMs = "+$cases[$name])
    $body=$body.Replace($hook,$hook+"`n"+$steps).Replace($failureAnchor,$failureDiagnostic)
    $body=$body.Replace("gate = 'windows-normal-launch'","gate = 'windows-live-edit'").Replace('normal-launch-report.json','live-edit-report.json')
    [IO.File]::WriteAllText((Join-Path $drivers 'run-normal-launch-guest.ps1'),$body,$utf8)
    [IO.File]::WriteAllText((Join-Path $drivers 'normal-launch-input.cs'),$helper+"`n"+$extra,$utf8)
    $composition=[ordered]@{ schemaVersion=1; case=$name; requestedGapMs=$cases[$name]; sourceSha256=[ordered]@{} }
    foreach ($file in @('run-live-edit-guest.ps1','run-normal-launch-guest.ps1','normal-launch-input.cs','live-edit-steps.ps1','live-edit-input.cs')) {
        $composition.sourceSha256[$file]=Hash (Join-Path $root $file)
    }
    $composition | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $drivers 'driver-composition.json') -Encoding UTF8
    $errorText=$null
    # Each case gets ordinary startup and its own app-written isolated profile.
    # Keep the parent shell free of Add-Type identities; use a fresh PS process.
    $child="& ([ScriptBlock]::Create([IO.File]::ReadAllText('"+(Join-Path $drivers 'run-normal-launch-guest.ps1').Replace("'","''")+"'))) -Package '"+$Package.Replace("'","''")+"' -Output '"+(Join-Path $run 'evidence').Replace("'","''")+"' -DriverDirectory '"+$drivers.Replace("'","''")+"'"
    $encoded=[Convert]::ToBase64String([Text.Encoding]::Unicode.GetBytes($child))
    $info=New-Object Diagnostics.ProcessStartInfo
    $info.FileName=Join-Path $PSHOME 'powershell.exe'
    $info.Arguments='-NoProfile -NonInteractive -WindowStyle Hidden -EncodedCommand '+$encoded
    $info.UseShellExecute=$false; $info.CreateNoWindow=$true
    $info.RedirectStandardOutput=$true; $info.RedirectStandardError=$true
    $process=New-Object Diagnostics.Process; $process.StartInfo=$info
    $null=$process.Start()
    $stdout=$process.StandardOutput.ReadToEndAsync(); $stderr=$process.StandardError.ReadToEndAsync()
    # The underlying driver bounds UI discovery and waits. Never detach it.
    $process.WaitForExit()
    [IO.File]::WriteAllText((Join-Path $run 'driver-stdout.log'),$stdout.Result,$utf8)
    [IO.File]::WriteAllText((Join-Path $run 'driver-stderr.log'),$stderr.Result,$utf8)
    $exit=$process.ExitCode; $process.Dispose()
    $report=Join-Path $run 'evidence\live-edit-report.json'
    $passed=$false
    if (Test-Path -LiteralPath $report) { $passed=($exit -eq 0 -and (Get-Content -LiteralPath $report -Raw | ConvertFrom-Json).passed -eq $true) }
    $results.Add([ordered]@{ case=$name; requestedGapMs=$cases[$name]; passed=$passed; exitCode=$exit;
        report=($name+'/evidence/live-edit-report.json'); reportSha256=if(Test-Path -LiteralPath $report){Hash $report}else{$null} })
}
$passed=@($results | Where-Object { -not $_.passed }).Count -eq 0
[ordered]@{ schemaVersion=1; gate='windows-live-edit-series'; passed=$passed; allCases=($Case -eq 'all');
    cases=@($results.ToArray()) } | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath (Join-Path $out 'live-edit-series.json') -Encoding UTF8
if (-not $passed) { throw "Live input acceptance failed; preserve and inspect $out/live-edit-series.json." }
Write-Output "Live input cases passed; independently verify $out/live-edit-series.json."
