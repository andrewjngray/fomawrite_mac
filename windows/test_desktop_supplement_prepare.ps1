# Preparation/identity guards only. No application launch or input API calls.
[CmdletBinding()]
param([string]$Output = '')
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
if ($env:OS -ne 'Windows_NT' -or $PSVersionTable.PSVersion.Major -ne 5) { throw 'Windows PowerShell 5.1 required' }
$source = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\bin\windows'))
$root = Join-Path $env:TEMP ('Fomawrite-supplement-prepare-' + [Guid]::NewGuid().ToString('N'))
$checks = New-Object 'Collections.Generic.List[string]'
function Assert([bool]$Condition, [string]$Message) { if (-not $Condition) { throw $Message } }
function Hash([string]$Path) { (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash.ToLowerInvariant() }
function Reject([scriptblock]$Action, [string]$Reason) {
    $failure = $null
    try { & $Action | Out-Null } catch { $failure = $_.Exception.Message }
    Assert ($null -ne $failure -and $failure -match $Reason) ('Wrong rejection: ' + $failure)
}
function Parse([string]$Path) {
    $tokens = $null; $errors = $null
    $null = [Management.Automation.Language.Parser]::ParseFile($Path, [ref]$tokens, [ref]$errors)
    Assert ($errors.Count -eq 0) ('Native parsing failed: ' + $Path + "`n" + ($errors | Out-String))
}
try {
    $null = New-Item -ItemType Directory -Path $root
    foreach ($name in @('run-desktop-supplement-guest.ps1','run-normal-launch-guest.ps1','desktop-supplement-steps.ps1')) {
        Parse (Join-Path $source $name)
    }
    $checks.Add('native-powershell-5.1-source-parse')
    $package = Join-Path $root 'package with spaces'
    $null = New-Item -ItemType Directory -Path $package
    $exe = Join-Path $package 'Fomawrite.exe'
    [IO.File]::WriteAllText($exe, 'SYNTHETIC NONEXECUTABLE FIXTURE - NEVER LAUNCH')
    $commit = 'a' * 40; $backend = 'b' * 64; $menu = 'c' * 64
    $manifestPath = Join-Path $package 'manifest.json'
    $manifest = [ordered]@{
        commit=$commit; architecture='arm64'; trackedSourceDirty=$false; executableSha256=(Hash $exe)
        nativeBuild=[ordered]@{commit=$commit; sourceFiles=[ordered]@{'src/backend.cpp'=$backend;'src/WindowsMenuBar.qml'=$menu}}
    }
    $manifest | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath $manifestPath -Encoding UTF8
    $receiptPath = Join-Path $root 'stage-receipt.json'
    $receipt = [ordered]@{stageOnly=$true;commit=$commit;architecture='arm64';manifestSha256=(Hash $manifestPath);executableSha256=(Hash $exe);localPackage=$package}
    $receipt | ConvertTo-Json | Set-Content -LiteralPath $receiptPath -Encoding UTF8
    $receiptBytes = [IO.File]::ReadAllBytes($receiptPath)
    $bindings = @{Package=$package;LocalStageReceipt=$receiptPath;Architecture='arm64';ExpectedCommit=$commit;ExpectedManifestSha256=(Hash $manifestPath);ExpectedExecutableSha256=(Hash $exe);ExpectedBackendSourceSha256=$backend;ExpectedMenuSourceSha256=$menu;DriverDirectory=$source;PrepareOnly=$true}
    $driver = [ScriptBlock]::Create([IO.File]::ReadAllText((Join-Path $source 'run-desktop-supplement-guest.ps1')))
    $prepared = Join-Path $root 'prepared'
    & $driver @bindings -Output $prepared | Out-Null
    $generated = Join-Path $prepared 'driver'
    Parse (Join-Path $generated 'run-normal-launch-guest.ps1')
    $composition = Get-Content (Join-Path $generated 'driver-composition.json') -Raw | ConvertFrom-Json
    Assert ($composition.preparedOnly -eq $true -and $composition.nativeParsePassed -eq $true -and $composition.helperCompilePending -eq $true) 'Preparation falsely claims helper execution/qualification'
    Assert (-not (Test-Path (Join-Path $prepared 'evidence'))) 'PrepareOnly entered ordinary workflow'
    Assert ($composition.localStageReceipt -ceq 'local-stage-receipt.json' -and $composition.localStageReceiptSha256 -ceq (Hash $receiptPath) -and (Hash (Join-Path $generated $composition.localStageReceipt)) -ceq (Hash $receiptPath)) 'Stage receipt copy lost exact bytes'
    Assert ($composition.generatedDriverSha256 -ceq (Hash (Join-Path $generated 'run-normal-launch-guest.ps1')) -and $composition.generatedInputSha256 -ceq (Hash (Join-Path $generated 'normal-launch-input.cs'))) 'Generated source hashes differ'
    foreach ($property in $composition.sourceSha256.PSObject.Properties) { Assert ($property.Value -ceq (Hash (Join-Path $source $property.Name))) ('Source hash differs: '+$property.Name) }
    $checks.Add('prepare-only-composes-and-binds-without-application-launch')

    Add-Type -AssemblyName UIAutomationClient, UIAutomationTypes, WindowsBase, System.Drawing, System.Windows.Forms
    Add-Type -Path (Join-Path $generated 'normal-launch-input.cs') -ReferencedAssemblies @([Windows.Automation.AutomationElement].Assembly.Location, [Windows.Automation.ControlType].Assembly.Location, [Windows.Rect].Assembly.Location)
    Assert ($null -ne ('DesktopSupplementInput' -as [type]) -and $null -ne ('NormalLaunchInput' -as [type]) -and $null -ne ('OwnedMenuNavigation' -as [type])) 'Combined helper types did not compile'
    $checks.Add('native-combined-csharp-helper-compiles-without-invoking-input')

    Reject { & $driver @bindings -Output $prepared } 'Fresh output outside'
    $checks.Add('existing-output-rejected')
    Reject { & $driver @bindings -Output (Join-Path $package 'inside-output') } 'Fresh output outside'
    $checks.Add('package-contained-output-rejected')
    $altered = $bindings.Clone(); $altered.ExpectedManifestSha256 = 'd' * 64
    Reject { & $driver @altered -Output (Join-Path $root 'wrong-manifest') } 'Exact manifest differs'
    $checks.Add('wrong-manifest-rejected')
    $altered = $bindings.Clone(); $altered.ExpectedBackendSourceSha256 = 'd' * 64
    Reject { & $driver @altered -Output (Join-Path $root 'wrong-source') } 'Exact clean candidate/source/executable binding differs'
    $checks.Add('wrong-source-rejected')
    [IO.File]::WriteAllText($exe, 'CORRUPT NONEXECUTABLE')
    Reject { & $driver @bindings -Output (Join-Path $root 'wrong-exe') } 'Exact clean candidate/source/executable binding differs'
    [IO.File]::WriteAllText($exe, 'SYNTHETIC NONEXECUTABLE FIXTURE - NEVER LAUNCH')
    $checks.Add('modified-executable-rejected')
    $receipt.stageOnly = $false
    $receipt | ConvertTo-Json | Set-Content -LiteralPath $receiptPath -Encoding UTF8
    Reject { & $driver @bindings -Output (Join-Path $root 'wrong-receipt') } 'Verified StageOnly receipt'
    [IO.File]::WriteAllBytes($receiptPath, $receiptBytes)
    $checks.Add('non-stage-only-receipt-rejected')

    $changedSource = Join-Path $root 'changed-source'
    $null = New-Item -ItemType Directory -Path $changedSource
    foreach ($property in $composition.sourceSha256.PSObject.Properties) { Copy-Item (Join-Path $source $property.Name) (Join-Path $changedSource $property.Name) }
    $basePath = Join-Path $changedSource 'run-normal-launch-guest.ps1'
    $base = [IO.File]::ReadAllText($basePath)
    [IO.File]::WriteAllText($basePath, $base.Replace("    Capture 'live-mode-rendered'", "    Capture 'changed-hook'"))
    $altered = $bindings.Clone(); $altered.DriverDirectory = $changedSource
    Reject { & $driver @altered -Output (Join-Path $root 'wrong-hook') } 'Maintained composition anchor differs'
    $checks.Add('missing-composition-hook-rejected')

    $result = [ordered]@{schemaVersion=1;passed=$true;count=$checks.Count;checks=@($checks.ToArray());applicationStarted=$false;inputSent=$false;desktopQualification=$false;sourceSha256=$composition.sourceSha256;generatedDriverSha256=$composition.generatedDriverSha256;generatedInputSha256=$composition.generatedInputSha256;nativePowerShell=$PSVersionTable.PSVersion.ToString();observedUtc=[DateTime]::UtcNow.ToString('o')}
    $json = $result | ConvertTo-Json -Depth 6
    if ($Output) {
        $outputPath = [IO.Path]::GetFullPath($Output)
        $null = New-Item -ItemType Directory -Force -Path ([IO.Path]::GetDirectoryName($outputPath))
        [IO.File]::WriteAllText($outputPath, $json, (New-Object Text.UTF8Encoding($false)))
    }
    Write-Output $json
} finally {
    if (Test-Path -LiteralPath $root) { Remove-Item -LiteralPath $root -Recurse -Force }
}
