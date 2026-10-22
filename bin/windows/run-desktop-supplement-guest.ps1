# Maintained physical-input supplement. Native validation and independent visual review required.
[CmdletBinding()]
param(
 [Parameter(Mandatory=$true)][string]$Package,
 [Parameter(Mandatory=$true)][string]$Output,
 [Parameter(Mandatory=$true)][string]$LocalStageReceipt,
 [Parameter(Mandatory=$true)][ValidateSet('arm64','x64')][string]$Architecture,
 [Parameter(Mandatory=$true)][ValidatePattern('^[0-9a-f]{40}$')][string]$ExpectedCommit,
 [Parameter(Mandatory=$true)][ValidatePattern('^[0-9a-f]{64}$')][string]$ExpectedManifestSha256,
 [Parameter(Mandatory=$true)][ValidatePattern('^[0-9a-f]{64}$')][string]$ExpectedExecutableSha256,
 [Parameter(Mandatory=$true)][ValidatePattern('^[0-9a-f]{64}$')][string]$ExpectedBackendSourceSha256,
 [Parameter(Mandatory=$true)][ValidatePattern('^[0-9a-f]{64}$')][string]$ExpectedMenuSourceSha256,
 [string]$DriverDirectory=$PSScriptRoot,
 [switch]$PrepareOnly
)
Set-StrictMode -Version Latest
$ErrorActionPreference='Stop'
if($env:OS-ne'Windows_NT'-or$PSVersionTable.PSVersion.Major-ne5){throw 'Fresh hidden Windows PowerShell5.1 required'}
if(Get-Process Fomawrite -ErrorAction SilentlyContinue){throw 'Existing Fomawrite: no attachment/input permitted'}
function Hash([string]$p){(Get-FileHash -LiteralPath $p -Algorithm SHA256).Hash.ToLowerInvariant()}
function Folder([string]$p){$v=Resolve-Path -LiteralPath $p;if($v.Provider.Name-ne'FileSystem'-or-not(Test-Path -LiteralPath $v.ProviderPath -PathType Container)){throw 'Filesystem directory required'};[IO.Path]::GetFullPath($v.ProviderPath).TrimEnd('\','/')}
if([string]::IsNullOrWhiteSpace($DriverDirectory)){throw 'ScriptBlock invocation requires -DriverDirectory'}
$root=Folder $DriverDirectory;$packageRoot=Folder $Package;$out=[IO.Path]::GetFullPath($Output)
if((Test-Path -LiteralPath $out)-or$out.StartsWith($packageRoot+'\',[StringComparison]::OrdinalIgnoreCase)){throw 'Fresh output outside the candidate package required'}
$sources=@('run-desktop-supplement-guest.ps1','run-normal-launch-guest.ps1','normal-launch-input.cs','desktop-supplement-steps.ps1','desktop-supplement-input.cs')
$index=[ordered]@{};foreach($name in $sources){$index[$name]=Hash (Join-Path $root $name)}
$manifestPath=Join-Path $packageRoot 'manifest.json'
if((Hash $manifestPath)-cne$ExpectedManifestSha256){throw 'Exact manifest differs'}
$m=Get-Content -LiteralPath $manifestPath -Raw|ConvertFrom-Json
if($m.commit-cne$ExpectedCommit-or$m.nativeBuild.commit-cne$ExpectedCommit-or$m.architecture-cne$Architecture-or$m.trackedSourceDirty-ne$false-or$m.nativeBuild.sourceFiles.'src/backend.cpp'-cne$ExpectedBackendSourceSha256-or$m.nativeBuild.sourceFiles.'src/WindowsMenuBar.qml'-cne$ExpectedMenuSourceSha256-or$m.executableSha256-cne$ExpectedExecutableSha256-or(Hash (Join-Path $packageRoot 'Fomawrite.exe'))-cne$ExpectedExecutableSha256){throw 'Exact clean candidate/source/executable binding differs'}
$localStageReceiptSha256=Hash $LocalStageReceipt
$receipt=Get-Content -LiteralPath $LocalStageReceipt -Raw|ConvertFrom-Json
if(-not$receipt.stageOnly-or$receipt.commit-cne$ExpectedCommit-or$receipt.architecture-cne$Architecture-or$receipt.manifestSha256-cne$ExpectedManifestSha256-or$receipt.executableSha256-cne$ExpectedExecutableSha256-or(Folder $receipt.localPackage)-ine$packageRoot){throw 'Verified StageOnly receipt must bind this exact local candidate'}
$drive=New-Object IO.DriveInfo([IO.Path]::GetPathRoot($packageRoot));if($drive.DriveFormat-cne'NTFS'){throw 'Ordinary app must run from verified local NTFS package'}
$basePath=Join-Path $root 'run-normal-launch-guest.ps1';$inputPath=Join-Path $root 'normal-launch-input.cs'
$base=[IO.File]::ReadAllText($basePath)
$config="`$ErrorActionPreference = 'Stop'";$hook="    Capture 'live-mode-rendered'";$report='        viewObservations = $viewObservations';$gate="gate = 'windows-normal-launch'";$passed='passed = ($null -eq $failure)'
foreach($anchor in @($config,$hook,$report,$gate,$passed)){if(($base.Split(@($anchor),[StringSplitOptions]::None).Count-1)-ne1){throw ('Maintained composition anchor differs: '+$anchor)}}
$body=$base.Replace($config,$config+"`n`$script:DesktopSupplementObservation=[ordered]@{completed=`$false;qualified=`$false;nativeVisualReviewPending=`$true}`n")
$startupAnchor="    Passed `$stage 'Deployed executable launched without command-line arguments and acknowledged isolated profile.'"
if(($body.Split(@($startupAnchor),[StringSplitOptions]::None).Count-1)-ne1){throw 'Ordinary startup setup anchor differs'}
$parts=[IO.File]::ReadAllText((Join-Path $root 'desktop-supplement-steps.ps1')).Split(@('# DESKTOP-SUPPLEMENT-WORKFLOW'),[StringSplitOptions]::None)
if($parts.Count-ne2){throw 'Supplement startup/workflow marker differs'}
$body=$body.Replace($startupAnchor,$parts[0]+"`n"+$startupAnchor)
$body=$body.Replace($hook,$hook+"`n"+$parts[1])
$body=$body.Replace($report,$report+"`n        desktopSupplementObservations = `$script:DesktopSupplementObservation")
$body=$body.Replace($gate,"gate = 'windows-desktop-supplement'").Replace('normal-launch-report.json','desktop-supplement-report.json')
$body=$body.Replace($passed,'passed = $false; workflowPassed = ($null -eq $failure -and $script:DesktopSupplementObservation.completed); qualified = $false; qualificationPending = $true')
$body=$body.Replace('Normal-launch Windows input gate passed; independently verify','Desktop supplemental workflows completed; independently review')
$tokens=$null;$errors=$null;$null=[Management.Automation.Language.Parser]::ParseInput($body,[ref]$tokens,[ref]$errors)
if($errors.Count){throw ('Composed native PowerShell parse failed: '+($errors|Out-String))}
$driverRoot=Join-Path $out 'driver';$null=New-Item -ItemType Directory -Path $driverRoot -Force;$utf8=New-Object Text.UTF8Encoding($false)
$driverPath=Join-Path $driverRoot 'run-normal-launch-guest.ps1';$helperPath=Join-Path $driverRoot 'normal-launch-input.cs'
[IO.File]::WriteAllText($driverPath,$body,$utf8)
$helper=[IO.File]::ReadAllText($inputPath)+"`n"+[IO.File]::ReadAllText((Join-Path $root 'desktop-supplement-input.cs'))
[IO.File]::WriteAllText($helperPath,$helper,$utf8)
$receiptCopy=Join-Path $driverRoot 'local-stage-receipt.json'
Copy-Item -LiteralPath $LocalStageReceipt -Destination $receiptCopy
if((Hash $receiptCopy)-cne$localStageReceiptSha256){throw 'StageOnly receipt changed during preparation'}
[ordered]@{schemaVersion=1;preparedOnly=[bool]$PrepareOnly;commit=$ExpectedCommit;architecture=$Architecture;manifestSha256=$ExpectedManifestSha256;executableSha256=$ExpectedExecutableSha256;backendSourceSha256=$ExpectedBackendSourceSha256;menuSourceSha256=$ExpectedMenuSourceSha256;localStageReceipt='local-stage-receipt.json';localStageReceiptSha256=$localStageReceiptSha256;sourceSha256=$index;generatedDriverSha256=Hash $driverPath;generatedInputSha256=Hash $helperPath;nativeParsePassed=$true;helperCompilePending=$true;clipboardTouched=$false;originalClipboardRestoration='unknown-r3-unresolved-by-this-run';observedUtc=[DateTime]::UtcNow.ToString('o')}|ConvertTo-Json -Depth 6|Set-Content (Join-Path $driverRoot 'driver-composition.json') -Encoding UTF8
if($PrepareOnly){Write-Output 'Prepared only; generated driver/input require compile-only validation before separate fresh-output execution';return}
$child="& ([ScriptBlock]::Create([IO.File]::ReadAllText('"+$driverPath.Replace("'","''")+"'))) -Package '"+$packageRoot.Replace("'","''")+"' -Output '"+(Join-Path $out 'evidence').Replace("'","''")+"' -DriverDirectory '"+$driverRoot.Replace("'","''")+"'"
$info=New-Object Diagnostics.ProcessStartInfo;$info.FileName=Join-Path $PSHOME 'powershell.exe';$info.Arguments='-NoProfile -NonInteractive -WindowStyle Hidden -EncodedCommand '+[Convert]::ToBase64String([Text.Encoding]::Unicode.GetBytes($child))
$info.UseShellExecute=$false;$info.CreateNoWindow=$true;$info.RedirectStandardOutput=$true;$info.RedirectStandardError=$true
$worker=New-Object Diagnostics.Process;$worker.StartInfo=$info;$null=$worker.Start();$stdout=$worker.StandardOutput.ReadToEndAsync();$stderr=$worker.StandardError.ReadToEndAsync();$worker.WaitForExit()
[IO.File]::WriteAllText((Join-Path $out 'driver-stdout.log'),$stdout.Result,$utf8);[IO.File]::WriteAllText((Join-Path $out 'driver-stderr.log'),$stderr.Result,$utf8);$exitCode=$worker.ExitCode;$worker.Dispose()
$reportPath=Join-Path $out 'evidence\desktop-supplement-report.json'
if($exitCode-ne0-or-not(Test-Path -LiteralPath $reportPath)){throw 'Desktop supplemental native workflow failed; preserve every first-failure output'}
$result=Get-Content -LiteralPath $reportPath -Raw|ConvertFrom-Json
if($result.workflowPassed-ne$true-or$result.desktopSupplementObservations.completed-ne$true-or$result.desktopSupplementObservations.checks.Count-ne6){throw 'Six desktop workflows were not proved'}
Write-Output ('Six workflows completed; independent identity/capture review required; no unconditional qualification: '+$reportPath)
