# Non-UI contract fixture. Execute actual production Find/retry/root methods
# against a disconnected-provider stub in a fresh Windows PowerShell process.
[CmdletBinding()]
param([string]$HelperPath='', [string]$DriverPath='', [switch]$PrimitiveOnly, [switch]$InitialOnly)
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
$methods=(Method 'FindOwnedWindow')+"`n"+(Method 'ReadOwnedWindow')+"`n"+(Method 'OwnedWindowBounds')+"`n"+(Method 'OwnedWindowTitle')+"`n"+(Method 'RetryDisconnected')+"`n"+(Method 'ReacquireOwnedRoot')+"`n"+(Method 'Find' $true)+"`n"+(Method 'OpenedDocument')
$prefix=@'
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Runtime.InteropServices;
using System.Threading;
namespace System.Windows.Automation {
 public sealed class ElementNotAvailableException : Exception {
  public ElementNotAvailableException():this(unchecked((int)0x80040201)){}
  public ElementNotAvailableException(string text):this(unchecked((int)0x80040201)){}
  public ElementNotAvailableException(int code):base("typed provider unavailable"){HResult=code;}
 }
}
public sealed class AutomationElement {
    public sealed class AutomationElementInformation { public string Name="normal-launch-sample.md - Fomawrite"; public int ProcessId=7; public int NativeWindowHandle=42; public System.Windows.Rect BoundingRectangle=new System.Windows.Rect(80,60,600,400); }
    public bool Stale;
    AutomationElementInformation info=new AutomationElementInformation();
    public AutomationElementInformation Current { get { if(Stale) throw new System.Windows.Automation.ElementNotAvailableException(); return info; } }
    public static AutomationElement FromHandle(IntPtr handle) {
        Probe.Reacquisitions++;
        if(Probe.Mode=="initial-unavailable-once" && Probe.Reacquisitions==1) return new AutomationElement(){Stale=true};
        if(Probe.Mode=="initial-unavailable-always") return new AutomationElement(){Stale=true};
        if(Probe.Mode=="primitive-null-once" && Probe.Reacquisitions==1) return null;
        if(Probe.Mode=="primitive-always-null") return null;
        if(Probe.Mode=="primitive-window-changes") Probe.expectedInputWindow=new IntPtr(99);
        if(Probe.Mode=="reacquire-fails" && Probe.Calls>0) throw new InvalidOperationException("reacquire failed");
        var e=new AutomationElement();
        if(Probe.Mode=="wrong-title") e.Current.Name="Untitled.md - Fomawrite";
        if(Probe.Mode=="uia-owner-changes" && Probe.Calls>0) e.Current.ProcessId=8;
        if(Probe.Mode=="handle-changes" && Probe.Calls>0) e.Current.NativeWindowHandle=99;
        if(Probe.Mode=="primitive-uia-owner-changes") e.Current.ProcessId=8;
        return e;
    }
}
public static class Probe {
    public static string Mode;
    public static int Calls, Reacquisitions;
    public static uint expectedProcessId=7; public static IntPtr expectedInputWindow=new IntPtr(42);
    static bool IsWindow(IntPtr handle) { return Mode=="modal-open"; }
    static uint GetWindowThreadProcessId(IntPtr handle,out uint pid) {
        pid=((Mode=="native-owner-changes" && Calls>0) || Mode=="primitive-native-owner-changes") ? 8u : 7u;
        return 1;
    }
    static T Bounded<T>(Func<T> action,string operation) { return action(); }
    static AutomationElement Walk(AutomationElement root,string pattern,List<string> description,Stopwatch budget=null) {
        Calls++;
        if(Mode=="typed-always" || (Mode=="typed-once" && Calls==1)) throw new System.Windows.Automation.ElementNotAvailableException();
        if(Mode=="typed-wrong-hresult") throw new System.Windows.Automation.ElementNotAvailableException(unchecked((int)0x80070005));
        if(Mode=="com-unavailable") throw new COMException("wrong exception class",unchecked((int)0x80040201));
        if(Mode=="always-fail" || (Mode=="fail-once" && Calls==1)) throw new COMException("provider E_FAIL",unchecked((int)0x80004005));
        if(Mode=="other-com") throw new COMException("access denied",unchecked((int)0x80070005));
        if(Mode=="incomplete") throw new InvalidOperationException("UIA search incomplete");
        if(Mode=="always-disconnect" || (Calls==1 && !Mode.StartsWith("initial-"))) throw new COMException("disconnected",unchecked((int)0x80010108));
        return new AutomationElement();
    }
'@
$suffix=@'
    public static string InitialRead(string mode) {
        Mode=mode;Calls=0;Reacquisitions=0;expectedProcessId=7;expectedInputWindow=new IntPtr(42);
        var method=typeof(Probe).GetMethod("FindOwnedWindow");
        try { if(method==null) return Find(new AutomationElement(){Stale=true},"fixture")==null?"null":"found";
              return method.Invoke(null,new object[]{7,"fixture"})==null?"null":"found"; }
        catch(System.Reflection.TargetInvocationException e){return e.InnerException.GetType().Name+":"+e.InnerException.Message;}
        catch(Exception e){return e.GetType().Name+":"+e.Message;}
    }
    public static string Primitive(string mode) {
        Mode=mode; Calls=0; Reacquisitions=0; expectedProcessId=7;expectedInputWindow=new IntPtr(42);
        var title=typeof(Probe).GetMethod("OwnedWindowTitle");var bounds=typeof(Probe).GetMethod("OwnedWindowBounds");
        if(title==null || bounds==null)return "missing owned-window primitive";
        if(mode=="primitive-no-window")expectedInputWindow=IntPtr.Zero;
        int pid=mode=="primitive-wrong-pid"?8:7;
        try { var name=(string)title.Invoke(null,new object[]{pid});var rect=(System.Windows.Rect)bounds.Invoke(null,new object[]{pid});return name+"|"+rect.ToString(System.Globalization.CultureInfo.InvariantCulture); }
        catch(System.Reflection.TargetInvocationException error){return error.InnerException.GetType().Name+":"+error.InnerException.Message;}
    }
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
Add-Type -AssemblyName WindowsBase
Add-Type -TypeDefinition ($prefix+$methods+$suffix) -ReferencedAssemblies @([Windows.Rect].Assembly.Location)
function Assert([bool]$Condition,[string]$Message) { if(-not $Condition){throw $Message} }
$checks=New-Object 'System.Collections.Generic.List[string]'
if(-not$PrimitiveOnly -and -not$InitialOnly){
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
$result=[Probe]::Run('typed-once')
Assert ($result -eq 'found' -and [Probe]::Calls -eq 2) ('Typed80040201 did not restart a complete same-owner read: '+$result)
$checks.Add('typed-unavailable-restarts-complete-traversal')
$result=[Probe]::Run('typed-always')
Assert ($result -match '^ElementNotAvailableException:' -and [Probe]::Calls -eq 3) 'Typed unavailable escaped the same two retries.'
$checks.Add('repeated-typed-unavailable-is-fatal')
foreach($mode in @('typed-wrong-hresult','com-unavailable')){
 $result=[Probe]::Run($mode);Assert ($result -notin @('found','null') -and [Probe]::Calls -eq 1) ('Typed class/HRESULT allowlist broadened: '+$mode)
 $checks.Add($mode+'-remains-fatal')
}


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
}
if(-not$PrimitiveOnly){
$result=[Probe]::InitialRead('initial-unavailable-once')
Assert ($result -eq 'found' -and [Probe]::Reacquisitions -eq 3 -and [Probe]::Calls -eq 1) ('Initial owned provider read did not restart safely: '+$result)
$checks.Add('initial-owned-provider-unavailable-restarts-before-traversal')
$result=[Probe]::InitialRead('initial-unavailable-always')
Assert ($result -match '^ElementNotAvailableException:' -and [Probe]::Calls -eq 0) 'Initial persistent unavailable became absence or traversed stale root.'
$checks.Add('initial-owned-provider-persistent-failure-is-fatal')
}

if(-not$InitialOnly){
foreach($mode in @('primitive-ready','primitive-null-once')){
 $result=[Probe]::Primitive($mode)
 Assert ($result -eq 'normal-launch-sample.md - Fomawrite|80,60,600,400') ('Known owned HWND primitive failed: '+$mode+' '+$result)
 $checks.Add($mode+'-returns-only-current-title-and-bounds')
}
foreach($mode in @('primitive-no-window','primitive-wrong-pid','primitive-window-changes','primitive-native-owner-changes','primitive-uia-owner-changes','primitive-always-null')){
 $result=[Probe]::Primitive($mode)
 Assert ($result -match '^(InvalidOperationException|ElementNotAvailableException):') ('Changed/missing primitive provider was accepted: '+$mode+' '+$result)
 $checks.Add($mode+'-rejected')
}
}
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
