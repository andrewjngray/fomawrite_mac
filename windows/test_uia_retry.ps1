# Non-UI contract fixture. Execute actual production Find/retry/root methods
# against a disconnected-provider stub in a fresh Windows PowerShell process.
[CmdletBinding()]
param([string]$HelperPath='', [string]$DriverPath='')
Set-StrictMode -Version Latest
$ErrorActionPreference='Stop'
if ([string]::IsNullOrWhiteSpace($HelperPath)) { $HelperPath=Join-Path $PSScriptRoot '..\bin\windows\normal-launch-input.cs' }
if ([string]::IsNullOrWhiteSpace($DriverPath)) { $DriverPath=Join-Path $PSScriptRoot '..\bin\windows\run-normal-launch-guest.ps1' }
$source=[IO.File]::ReadAllText($HelperPath)
function Method([string]$Name,[bool]$Required=$false) {
    $pattern='(?m)^    (?:public )?static [^\r\n]+\b'+[Regex]::Escape($Name)+'(?:<T>)?\('
    $match=[Regex]::Match($source,$pattern)
    if(-not $match.Success) { if($Required){throw "Missing method $Name"}; return '' }
    $start=$source.IndexOf('{',$match.Index)
    $depth=1; $at=$start+1
    while($depth -gt 0 -and $at -lt $source.Length) {
        if($source[$at] -eq '{'){$depth++}; if($source[$at] -eq '}'){$depth--}; $at++
    }
    if($depth -ne 0){throw "Unclosed method $Name"}
    return $source.Substring($match.Index,$at-$match.Index)
}
$methods=(Method 'RetryDisconnected')+"`n"+(Method 'ReacquireOwnedRoot')+"`n"+(Method 'Find' $true)+"`n"+(Method 'OpenedDocument')
$prefix=@'
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Runtime.InteropServices;
using System.Threading;
public sealed class AutomationElement {
    public sealed class Info { public string Name="normal-launch-sample.md - Fomawrite"; public int ProcessId=7; public int NativeWindowHandle=42; }
    public Info Current=new Info();
    public static AutomationElement FromHandle(IntPtr handle) {
        Probe.Reacquisitions++;
        if(Probe.Mode=="reacquire-fails" && Probe.Calls>0) throw new InvalidOperationException("reacquire failed");
        var e=new AutomationElement();
        if(Probe.Mode=="wrong-title") e.Current.Name="Untitled.md - Fomawrite";
        if(Probe.Mode=="uia-owner-changes" && Probe.Calls>0) e.Current.ProcessId=8;
        if(Probe.Mode=="handle-changes" && Probe.Calls>0) e.Current.NativeWindowHandle=99;
        return e;
    }
}
public static class Probe {
    public static string Mode;
    public static int Calls, Reacquisitions;
    public static uint expectedProcessId=7;
    static bool IsWindow(IntPtr handle) { return Mode=="modal-open"; }
    static uint GetWindowThreadProcessId(IntPtr handle,out uint pid) {
        pid=Mode=="native-owner-changes" && Calls>0 ? 8u : 7u;
        return 1;
    }
    static T Bounded<T>(Func<T> action,string operation) { return action(); }
    static AutomationElement Walk(AutomationElement root,string pattern,List<string> description,Stopwatch budget=null) {
        Calls++;
        if(Mode=="always-fail" || (Mode=="fail-once" && Calls==1)) throw new COMException("provider E_FAIL",unchecked((int)0x80004005));
        if(Mode=="other-com") throw new COMException("access denied",unchecked((int)0x80070005));
        if(Mode=="incomplete") throw new InvalidOperationException("UIA search incomplete");
        if(Mode=="always-disconnect" || Calls==1) throw new COMException("disconnected",unchecked((int)0x80010108));
        return new AutomationElement();
    }
'@
$suffix=@'
    public static string Budget() {
        int searched=0, reacquired=0;
        var method=typeof(Probe).GetMethod("RetryDisconnected",System.Reflection.BindingFlags.NonPublic|System.Reflection.BindingFlags.Static);
        if(method==null) return "missing retry method";
        var budget=Stopwatch.StartNew();
        try {
            method.MakeGenericMethod(typeof(AutomationElement)).Invoke(null,new object[] {
                new Func<AutomationElement>(delegate() { searched++; Thread.Sleep(30); return new AutomationElement(); }),
                new Action(delegate() { reacquired++; }),budget,10 });
            return "accepted late result";
        } catch(System.Reflection.TargetInvocationException error) {
            return error.InnerException.GetType().Name+":"+searched+":"+reacquired;
        }
    }
    public static string Run(string mode) {
        Mode=mode; Calls=0; Reacquisitions=0;
        var root=new AutomationElement();
        if(mode=="wrong-owner") root.Current.ProcessId=8;
        try { var found=Find(root,"fixture"); return found==null ? "null" : "found"; }
        catch(Exception error) { return error.GetType().Name+":"+error.Message; }
    }
}
'@
Add-Type -TypeDefinition ($prefix+$methods+$suffix)
function Assert([bool]$Condition,[string]$Message) { if(-not $Condition){throw $Message} }
$checks=New-Object 'System.Collections.Generic.List[string]'
$result=[Probe]::Run('disconnect-once')
Assert ($result -eq 'found' -and [Probe]::Calls -eq 2) ("Disconnected provider did not complete a fresh search: "+$result)
$checks.Add('disconnected-provider-restarts-complete-traversal')
$result=[Probe]::Run('always-disconnect')
Assert ($result -match '^COMException:' -and [Probe]::Calls -eq 3) 'Repeated disconnection did not fail within two retries.'
$checks.Add('repeated-disconnect-is-fatal')
$result=[Probe]::Run('fail-once')
Assert ($result -eq 'found' -and [Probe]::Calls -eq 2) ('Provider E_FAIL did not complete a fresh search: '+$result)
$checks.Add('provider-efail-restarts-complete-traversal')
$result=[Probe]::Run('always-fail')
Assert ($result -match '^COMException:' -and [Probe]::Calls -eq 3) 'Repeated E_FAIL did not fail within two retries.'
$checks.Add('repeated-efail-is-fatal')

foreach($mode in @('other-com','incomplete')) {
    $result=[Probe]::Run($mode)
    Assert ($result -notin @('found','null') -and [Probe]::Calls -eq 1) ("Unexpected error was retried or became absence: "+$mode)
    $checks.Add($mode+'-remains-fatal')
}
foreach($mode in @('wrong-owner','reacquire-fails','native-owner-changes','uia-owner-changes','handle-changes')) {
    $result=[Probe]::Run($mode)
    Assert ($result -match '^InvalidOperationException:' -and [Probe]::Calls -le 1) ("Root identity/reacquisition failure was accepted: "+$mode+' '+$result)
    $checks.Add($mode+'-is-rejected')
}
Assert ([Probe]::Budget() -eq 'TimeoutException:1:0') 'Search completion escaped the shared retry deadline.'
$checks.Add('late-result-rejected-by-shared-deadline')
# Execute the actual driver condition, so the former Library-row condition is red.
$tokens=$null; $errors=$null
$ast=[Management.Automation.Language.Parser]::ParseFile([IO.Path]::GetFullPath($DriverPath),[ref]$tokens,[ref]$errors)
if($errors.Count){throw 'Driver parse failed.'}
$commands=@($ast.FindAll({param($node)
    $node -is [Management.Automation.Language.CommandAst] -and
    $node.GetCommandName() -eq 'Wait-For' -and $node.Extent.Text -match "'opened document title'"
},$true))
Assert ($commands.Count -eq 1) 'Expected one actual opened-document wait.'
$conditionText=$commands[0].CommandElements[1].ScriptBlock.Extent.Text
$condition=[ScriptBlock]::Create(($conditionText.Substring(1,$conditionText.Length-2)).Replace('[NormalLaunchInput]','[Probe]'))
$script:main=New-Object AutomationElement
$openPickerHandle=[IntPtr]43
function Find-Control([string]$Pattern) { return New-Object object } # existing sidebar row
[Probe]::Mode='modal-open'
Assert (-not (& $condition)) 'Sidebar row/native modal falsely established document open.'
[Probe]::Mode='wrong-title'
Assert (-not (& $condition)) 'Wrong owned document title falsely established document open.'
[Probe]::Mode='ready'
Assert ([bool](& $condition)) 'Dismissed picker plus exact owned title was rejected.'
$checks.Add('opened-document-requires-dismissed-picker-and-owned-title')
[ordered]@{ passed=$true; count=$checks.Count; checks=@($checks.ToArray());
    helper=[IO.Path]::GetFullPath($HelperPath); applicationStarted=$false; inputSent=$false } | ConvertTo-Json -Depth 4
