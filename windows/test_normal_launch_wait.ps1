# Non-UI regression: execute the real driver wait and picker selection with a stub.
# Run in a fresh Windows PowerShell 5.1 process; no application is started.
[CmdletBinding()]
param([string]$DriverPath = '', [string]$PublishingDriverPath = '')
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
if ([string]::IsNullOrWhiteSpace($DriverPath)) {
    $DriverPath = Join-Path $PSScriptRoot '..\bin\windows\run-normal-launch-guest.ps1'
}
if ([string]::IsNullOrWhiteSpace($PublishingDriverPath)) { $PublishingDriverPath=Join-Path (Split-Path -Parent $DriverPath) 'run-publishing-guest.ps1' }
$tokens = $null
$parseErrors = $null
$ast = [Management.Automation.Language.Parser]::ParseInput(
    [IO.File]::ReadAllText([IO.Path]::GetFullPath($DriverPath)), [ref]$tokens, [ref]$parseErrors)
if ($parseErrors.Count) { throw ('Driver parse failed: ' + ($parseErrors -join '; ')) }
$waits = @($ast.FindAll({ param($node)
    $node -is [Management.Automation.Language.FunctionDefinitionAst] -and $node.Name -eq 'Wait-For'
}, $true))
$selections = @($ast.FindAll({ param($node)
    $node -is [Management.Automation.Language.AssignmentStatementAst] -and
    $node.Left -is [Management.Automation.Language.VariableExpressionAst] -and
    $node.Left.VariablePath.UserPath -eq 'select'
}, $true))
if ($waits.Count -ne 1 -or $selections.Count -ne 1) {
    throw 'Expected exactly one driver Wait-For function and one picker $select assignment.'
}
# Keep the real function body, including process-exit detection and deadline logic.
# Only the public wrapper below shortens its deadline for this non-UI fixture.
$waitDefinition = $waits[0].Extent.Text -replace '^function\s+Wait-For\b', 'function Invoke-DriverWait'
. ([ScriptBlock]::Create($waitDefinition))
function Wait-For([scriptblock]$condition, [string]$reason, [int]$seconds = 20) {
    Invoke-DriverWait $condition $reason 1
}
$selection = $selections[0]
$selectionText = $selection.Extent.Text
# Also execute the old driver's adjacent null-result failure guard when present.
# This makes the same fixture a red test for the previous one-shot selection.
$next = @($selection.Parent.Statements | Where-Object {
    $_.Extent.StartOffset -ge $selection.Extent.EndOffset
} | Select-Object -First 1)
if ($next.Count -and $next[0] -is [Management.Automation.Language.IfStatementAst] -and
    $next[0].Extent.Text -match '^if\s*\(\s*-not\s+\$select\s*\)') {
    $selectionText += "`n" + $next[0].Extent.Text
}
$selectFolder = [ScriptBlock]::Create($selectionText + "`nreturn `$select")

# Find models the helper contract: a disabled/unavailable target returns null.
# Requiring repeated calls makes the old one-shot selection fail without mirroring
# the implementation of its replacement. No UIA, SendInput or live process is used.
Add-Type -TypeDefinition @'
using System;
public static class NormalLaunchInput {
    public static int Calls;
    public static int AvailableAfter;
    public static int ExitAfter;
    public static int TypeCalls,SelectAllCalls,EnterCalls,WheelCalls,ClickCalls;
    public static string LastTyped;
    public static void ResetInput(){TypeCalls=SelectAllCalls=EnterCalls=WheelCalls=ClickCalls=0;LastTyped=null;}
    public static void Chord(int modifier,int key){if(modifier!=0x11||key!=0x41)throw new InvalidOperationException("Unexpected fixture chord");SelectAllCalls++;}
    public static void Type(string value){TypeCalls++;LastTyped=value;NativeFilenameWitness.Typed=true;if(NativeFilenameWitness.Mode=="input-rejected")throw new InvalidOperationException("fixture rejected input");}
    public static void Press(int key){if(key!=0x0D)throw new InvalidOperationException("Unexpected fixture key");EnterCalls++;}
    public static string ScrollOwnedMenuDown(object proof,int pid,object bounds,long deadline){
        if(!Object.ReferenceEquals(proof,FixturePopup)||pid!=7||bounds.ToString()!="same-owned-popup")throw new InvalidOperationException("Wrong retained fixture popup");
        if(System.Diagnostics.Stopwatch.GetTimestamp()>=deadline)throw new InvalidOperationException("Late fixture wheel");WheelCalls++;return "one recorded physical-wheel call";
    }
    public static object FixturePopup;
    public static void ClickOwnedMenuPoint(object proof,int pid,int x,int y,object bounds,long captured){
        if(!Object.ReferenceEquals(proof,FixturePopup)||pid!=7||x!=40||y!=60)throw new InvalidOperationException("Wrong final retained-popup target");
        double age=(System.Diagnostics.Stopwatch.GetTimestamp()-captured)*1000.0/System.Diagnostics.Stopwatch.Frequency;
        if(age<0||age>750)throw new InvalidOperationException("Stale fixture click");ClickCalls++;
    }
    public static readonly object Target = new object();
    public static void Reset(int availableAfter, int exitAfter) {
        Calls = 0; AvailableAfter = availableAfter; ExitAfter = exitAfter;
    }
    public static object Find(object dialog, string pattern) {
        if (dialog == null || pattern != "^Select Folder$")
            throw new InvalidOperationException("Wrong folder-picker target.");
        ++Calls;
        return AvailableAfter > 0 && Calls >= AvailableAfter ? Target : null;
    }
}
public static class NativeFilenameWitness {
    public static string Mode;public static bool Typed;public static int AfterReads;
    public sealed class State{public bool ready;public string value;}
    public static void Reset(string mode){Mode=mode;Typed=false;AfterReads=0;}
    public static State Read(IntPtr dialog,int pid){
        if(dialog.ToInt64()!=42||pid!=7)throw new InvalidOperationException("Wrong exact filename fixture identity");
        if(!Typed)return new State{ready=Mode!="unavailable-before-type",value=""};
        AfterReads++;if(Mode=="lost-owner-after-type")throw new InvalidOperationException("fixture owner lost");
        string full=NormalLaunchInput.LastTyped;bool exact=Mode=="prefix-then-exact"&&AfterReads>=5;
        return new State{ready=true,value=exact?full:full.Substring(0,full.Length-3)};
    }
}
public sealed class NormalLaunchFixtureProcess {
    public int Id {get{return 7;}}
    public bool HasExited {
        get { return NormalLaunchInput.ExitAfter > 0 && NormalLaunchInput.Calls >= NormalLaunchInput.ExitAfter; }
    }
    public int ExitCode { get { return 17; } }
}
'@
$dialog = New-Object object
$script:process = New-Object NormalLaunchFixtureProcess
$checks = New-Object 'System.Collections.Generic.List[string]'
function Assert([bool]$condition, [string]$message) { if (-not $condition) { throw $message } }
function Reject([scriptblock]$action, [string]$expected) {
    $reason = ''
    try { & $action | Out-Null } catch { $reason = $_.Exception.Message }
    Assert ($reason -match $expected) ("Expected '$expected'; received '$reason'.")
}

[NormalLaunchInput]::Reset(4, 0)
$selected = & $selectFolder
Assert ([object]::ReferenceEquals($selected, [NormalLaunchInput]::Target)) 'Delayed enabled folder target was not selected.'
Assert ([NormalLaunchInput]::Calls -ge 4) 'Picker selection did not poll until the target became enabled.'
$checks.Add('delayed-enabled-folder-target-is-returned')

[NormalLaunchInput]::Reset(0, 0)
$timer = [Diagnostics.Stopwatch]::StartNew()
Reject { & $selectFolder } '^Timed out: .+'
$timer.Stop()
Assert ([NormalLaunchInput]::Calls -gt 1) 'Unavailable target was not polled.'
Assert ($timer.ElapsedMilliseconds -ge 900 -and $timer.ElapsedMilliseconds -lt 2500) 'Unavailable target did not respect the bounded fixture deadline.'
$checks.Add('never-available-folder-target-times-out')

[NormalLaunchInput]::Reset(0, 1)
Reject { & $selectFolder } '^Application exited before .+ \(status 17\)\.$'
Assert ([NormalLaunchInput]::Calls -eq 1) 'Picker search continued after application exit.'
$checks.Add('application-exit-aborts-picker-wait')

# Additional non-UI regressions execute the current driver function/callback.
# The stub records prospective input; it never calls UIA, user32 or SendInput.
$nativeFunctions=@($ast.FindAll({param($n) $n -is [Management.Automation.Language.FunctionDefinitionAst] -and $n.Name -eq 'Type-NativeFilePath'},$true))
$nativeCalls=@($ast.FindAll({param($n) $n -is [Management.Automation.Language.CommandAst] -and $n.GetCommandName() -eq 'Type-NativeFilePath'},$true))
$menus=@($ast.FindAll({param($n) $n -is [Management.Automation.Language.FunctionDefinitionAst] -and $n.Name -eq 'Open-Menu'},$true))
Assert ($nativeFunctions.Count-eq1-and$nativeCalls.Count-eq1-and$menus.Count-eq1) 'Expected one actual filename guard/caller and Open-Menu function.'
$call=$nativeCalls[0].Parent
$next=@($call.Parent.Statements|Where-Object{$_.Extent.StartOffset-ge$call.Extent.EndOffset}|Select-Object -First 1)
Assert ($next.Count-eq1-and$next[0].Extent.Text-match '^\[NormalLaunchInput\]::Press\(0x0D\)$') 'Actual filename guard must be followed by exactly the existing Enter.'
. ([ScriptBlock]::Create($nativeFunctions[0].Extent.Text))
$filenameCaller=[ScriptBlock]::Create($call.Extent.Text+"`n"+$next[0].Extent.Text)
function Load-ExitDriver([string]$path) {
    $tokens=$null;$errors=$null
    $exitAst=[Management.Automation.Language.Parser]::ParseInput([IO.File]::ReadAllText([IO.Path]::GetFullPath($path)),[ref]$tokens,[ref]$errors)
    Assert ($errors.Count-eq0) ('Exit driver parse failed: '+$path)
    $exitMenus=@($exitAst.FindAll({param($n) $n -is [Management.Automation.Language.FunctionDefinitionAst] -and $n.Name-eq'Open-Menu'},$true))
    Assert ($exitMenus.Count-eq1) 'Expected one actual Exit menu function.'
    $targets=@($exitMenus[0].Body.FindAll({param($n) $n -is [Management.Automation.Language.AssignmentStatementAst] -and $n.Left -is [Management.Automation.Language.VariableExpressionAst] -and $n.Left.VariablePath.UserPath-eq'target' -and $n.Right.Extent.Text-match '^Wait-For\s*\{'},$true))
    Assert ($targets.Count-eq1) 'Expected exactly the actual Exit Wait-For callback.'
    $blocks=@($targets[0].Right.FindAll({param($n) $n -is [Management.Automation.Language.ScriptBlockExpressionAst]},$true))
    Assert ($blocks.Count-eq1) 'Expected one Exit callback ScriptBlock.'
    $body=$blocks[0].ScriptBlock.Extent.Text
    $exitCallback=[ScriptBlock]::Create($body.Substring(1,$body.Length-2))
    $clicks=@($exitMenus[0].Body.FindAll({param($n) $n -is [Management.Automation.Language.InvokeMemberExpressionAst] -and $n.Member.Value-eq'ClickOwnedMenuPoint'},$true))
    $measures=@($exitMenus[0].Body.FindAll({param($n) $n -is [Management.Automation.Language.AssignmentStatementAst] -and $n.Left -is [Management.Automation.Language.VariableExpressionAst] -and $n.Left.VariablePath.UserPath-eq'measuredPopup'},$true))
    Assert ($clicks.Count-eq1-and$measures.Count-eq1) 'Expected the actual final measured-popup physical click.'
    $exitClick=[ScriptBlock]::Create($measures[0].Extent.Text+"`n"+$clicks[0].Extent.Text)
    return @{callback=$exitCallback;click=$exitClick}
}
Add-Type -AssemblyName WindowsBase
$fixtureRoot=Join-Path ([IO.Path]::GetTempPath()) ('fomawrite-readiness-unit-'+[Guid]::NewGuid().ToString('N'))
$null=New-Item -ItemType Directory -Path $fixtureRoot
$outputRoot=$fixtureRoot;$script:NativeFilenameSequence=0
$sample='C:\public-fixture\document.md';$openPickerHandle=[IntPtr]42
function Reset-Native([string]$mode){[NormalLaunchInput]::Reset(0,0);[NormalLaunchInput]::ResetInput();[NativeFilenameWitness]::Reset($mode)}
function Latest-NativeProof {
    Get-Content -LiteralPath (Join-Path $outputRoot ('native-filename-readiness-'+$script:NativeFilenameSequence+'.json')) -Raw|ConvertFrom-Json
}
function Assert-NoNativeEnter([int]$types,[string]$reason){
    Assert ([NormalLaunchInput]::TypeCalls-eq$types-and[NormalLaunchInput]::EnterCalls-eq0) $reason
    $proof=Latest-NativeProof
    Assert (-not$proof.matched-and-not[string]::IsNullOrWhiteSpace($proof.failure)) 'Failed filename readiness must retain failure proof.'
}
$script:ExitQueue=New-Object Collections.Queue
function Read-ExitMenuTarget($owned) {
    Assert ([object]::ReferenceEquals($owned,$script:FixturePopup)) 'Callback changed the retained popup identity.'
    Assert ($script:ExitQueue.Count-gt0) 'Actual Exit callback consumed unprovided observations.'
    $frame=$script:ExitQueue.Dequeue()
    $script:ExitMenuLastCrop=[ordered]@{key=$frame.key;bounds='same-owned-popup';captureTicks=[Diagnostics.Stopwatch]::GetTimestamp()}
    if($frame.present){return [ordered]@{contentSha256=$frame.key;x=40;y=60;popup=@{left=10;top=20;width=100;height=100};captureTicks=[Diagnostics.Stopwatch]::GetTimestamp()}}
    return $null
}
function Add-ExitFrame([string]$key,[bool]$present){$script:ExitQueue.Enqueue([ordered]@{key=$key;present=$present})}
function Reset-Exit {
    [NormalLaunchInput]::ResetInput();$script:ExitQueue.Clear()
    $script:FixturePopup=New-Object object;[NormalLaunchInput]::FixturePopup=$script:FixturePopup;$anchor=$script:FixturePopup
    # The real callback looks these up in its caller scope; return the anchor.
    $script:ExitMenuStableKey=$null;$script:ExitMenuStable=@();$script:ExitMenuStableClock=[Diagnostics.Stopwatch]::StartNew()
    $script:ExitMenuMissingKey=$null;$script:ExitMenuMissingCount=0;$script:ExitMenuMissingClock=[Diagnostics.Stopwatch]::StartNew();$script:ExitMenuGestures=1
    $script:ExitMenuDeadline=[Diagnostics.Stopwatch]::GetTimestamp()+20*[Diagnostics.Stopwatch]::Frequency
    return $anchor
}
function Tick-Exit([string]$key,[bool]$present){Add-ExitFrame $key $present; & $exitCallback}
try {
    Reset-Native 'prefix-then-exact';$clock=[Diagnostics.Stopwatch]::StartNew();& $filenameCaller
    Assert ([NormalLaunchInput]::TypeCalls-eq1-and[NormalLaunchInput]::SelectAllCalls-eq1-and[NormalLaunchInput]::EnterCalls-eq1) 'Completed filename must be typed once, selected once and submitted once.'
    Assert ([NativeFilenameWitness]::AfterReads-ge5-and$clock.ElapsedMilliseconds-ge500) 'Enter preceded actual full-value completion.'
    $proof=Latest-NativeProof
    Assert ($proof.typedOnce-and$proof.matched-and$proof.samples[-1].value-ceq$sample) 'Completed native path proof is missing or inexact.'
    $checks.Add('native-path-prefix-settles-before-one-enter-no-retyping')

    Reset-Native 'never-complete';$clock.Restart();Reject { & $filenameCaller } 'did not settle to the exact full path'
    Assert-NoNativeEnter 1 'Incomplete path replayed typing or submitted Enter.'
    Assert ($clock.ElapsedMilliseconds-ge3900-and$clock.ElapsedMilliseconds-lt6500) 'Completion mismatch changed the real four-second bound.'
    $checks.Add('native-path-never-completes-fails-within-original-budget-no-enter')

    Reset-Native 'unavailable-before-type';Reject { & $filenameCaller } '^Timed out:'
    Assert-NoNativeEnter 0 'Unavailable filename focus permitted typing or Enter.'
    $checks.Add('native-path-unavailable-focus-prevents-all-input')

    Reset-Native 'lost-owner-after-type';Reject { & $filenameCaller } 'fixture owner lost'
    Assert-NoNativeEnter 1 'Lost native owner permitted Enter or typing replay.'
    $checks.Add('native-path-owner-loss-after-single-type-fails-closed')

    Reset-Native 'input-rejected';Reject { & $filenameCaller } 'fixture rejected input'
    Assert-NoNativeEnter 1 'Rejected typing was replayed or submitted.'
    $checks.Add('native-path-rejected-type-is-never-replayed')

    foreach($exitDriver in @($DriverPath,$PublishingDriverPath)) {
        $actual=Load-ExitDriver $exitDriver;$exitCallback=$actual.callback;$exitClick=$actual.click
        $driverLabel=[IO.Path]::GetFileNameWithoutExtension($exitDriver)
        $anchor=Reset-Exit
        $null=Tick-Exit 'missing-A' $false;Start-Sleep -Milliseconds 160
        $null=Tick-Exit 'missing-A' $false
        $null=Tick-Exit 'visible-Exit' $true;Start-Sleep -Milliseconds 160
        $null=Tick-Exit 'missing-A' $false
        Assert ([NormalLaunchInput]::WheelCalls-eq0-and$script:ExitMenuMissingCount-eq1) 'Missing-crop stability improperly spanned a visible Exit frame.'
        Start-Sleep -Milliseconds 160;$null=Tick-Exit 'missing-A' $false
        Start-Sleep -Milliseconds 160;$null=Tick-Exit 'missing-A' $false
        Assert ([NormalLaunchInput]::WheelCalls-eq1-and$script:ExitMenuGestures-eq2) 'Three new consecutive stable missing crops did not permit exactly one wheel.'
        $checks.Add('exit-present-frame-resets-missing-stability-before-next-wheel-'+$driverLabel)

        $anchor=Reset-Exit
        $null=Tick-Exit 'A' $false;Start-Sleep -Milliseconds 160;$null=Tick-Exit 'B' $false;Start-Sleep -Milliseconds 160;$null=Tick-Exit 'A' $false
        Assert ([NormalLaunchInput]::WheelCalls-eq0) 'Changing crop pixels permitted a blind wheel.'
        $checks.Add('exit-changing-missing-crops-cannot-authorize-wheel-'+$driverLabel)

        $anchor=Reset-Exit;$script:ExitMenuGestures=12
        $null=Tick-Exit 'A' $false;Start-Sleep -Milliseconds 160;$null=Tick-Exit 'A' $false;Start-Sleep -Milliseconds 160
        Reject { $null=Tick-Exit 'A' $false } 'twelve owned wheel gestures'
        Assert ([NormalLaunchInput]::WheelCalls-eq0-and[NormalLaunchInput]::ClickCalls-eq0) 'Gesture exhaustion sent more input.'
        $checks.Add('exit-twelve-gesture-limit-rejects-further-input-'+$driverLabel)

        $anchor=Reset-Exit;$script:ExitMenuDeadline=[Diagnostics.Stopwatch]::GetTimestamp()-1
        Reject { $null=Tick-Exit 'A' $false } 'twenty seconds'
        Assert ([NormalLaunchInput]::WheelCalls-eq0-and[NormalLaunchInput]::ClickCalls-eq0) 'Expired Exit deadline sent input.'
        $checks.Add('exit-expired-navigation-deadline-rejects-input-'+$driverLabel)

        $anchor=Reset-Exit
        $null=Tick-Exit 'exact-Exit' $true;Start-Sleep -Milliseconds 160;$null=Tick-Exit 'exact-Exit' $true;Start-Sleep -Milliseconds 160
        Add-ExitFrame 'exact-Exit' $true;Add-ExitFrame 'exact-Exit' $true;$target=& $exitCallback
        Assert ($null-ne$target-and$script:ExitMenuStable.Count-eq4) 'Three stable Exit samples plus a matching fresh sample were not required.'
        & $exitClick
        Assert ([NormalLaunchInput]::ClickCalls-eq1-and[NormalLaunchInput]::WheelCalls-eq0) 'Actual measured-popup Exit caller did not click exactly once.'
        $checks.Add('exit-three-stable-plus-fresh-target-reaches-one-real-click-call-'+$driverLabel)

        $anchor=Reset-Exit
        $null=Tick-Exit 'exact-Exit' $true;Start-Sleep -Milliseconds 160;$null=Tick-Exit 'exact-Exit' $true;Start-Sleep -Milliseconds 160
        Add-ExitFrame 'exact-Exit' $true;Add-ExitFrame 'moved-Exit' $true;$target=& $exitCallback
        Assert ($null-eq$target-and$script:ExitMenuStable.Count-eq0-and[NormalLaunchInput]::ClickCalls-eq0) 'Changed fresh Exit pixels permitted a click.'
        $checks.Add('exit-fresh-target-mismatch-resets-stability-without-click-'+$driverLabel)
    }
} finally {
    Remove-Item -LiteralPath $fixtureRoot -Recurse -Force
}

[ordered]@{ passed = $true; count = $checks.Count; checks = @($checks.ToArray());
    driver = [IO.Path]::GetFullPath($DriverPath); publishingDriver = [IO.Path]::GetFullPath($PublishingDriverPath); applicationStarted = $false; inputSent = $false } | ConvertTo-Json -Depth 4
