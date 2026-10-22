# Existing test_uia_retry.ps1 extraction/provider-stub pattern; no app or input.
[CmdletBinding()]
param([Parameter(Mandatory=$true)][string]$HelperPath)
Set-StrictMode -Version Latest
$ErrorActionPreference='Stop'
$source=[IO.File]::ReadAllText([IO.Path]::GetFullPath($HelperPath))
function Method([string]$name,[bool]$required=$false) {
    $match=[Regex]::Match($source,'(?m)^    (?:public )?static [^\r\n]+\b'+[Regex]::Escape($name)+'\(')
    if(-not$match.Success){if($required){throw ('Missing actual helper method '+$name)};return ''}
    $start=$source.IndexOf('{',$match.Index);$depth=1;$at=$start+1
    while($depth-gt0-and$at-lt$source.Length){if($source[$at]-eq'{'){$depth++};if($source[$at]-eq'}'){$depth--};$at++}
    if($depth-ne0){throw ('Unclosed actual helper method '+$name)}
    return $source.Substring($match.Index,$at-$match.Index)
}
# The optional pure guard intentionally remains absent for the old helper:
# old Click must compile and fail the clipped-menu case by moving/sending.
$methods=(Method 'RequireMenuClickBounds')+"`n"+(Method 'Click' $true)
$prefix=@'
using System;
using System.Threading;
public enum ControlType {MenuItem,Button}
public sealed class AutomationElement {
 public sealed class AutomationElementInformation {
  public bool IsEnabled=true,IsOffscreen=false;public string Name="Export HTML";
  public System.Windows.Rect BoundingRectangle;public int ProcessId=7;
  public ControlType ControlType=global::ControlType.MenuItem;
 }
 public readonly AutomationElementInformation Current=new AutomationElementInformation();
 public AutomationElement Parent;public string ClassName="";
}
public static class Probe {
 struct Mouse {public uint dwFlags;}
 struct Union {public Mouse mi;}
 struct INPUT {public uint type;public Union u;}
 public static int Moves,Downs,Ups,PopupReads;public static int LastX,LastY;
 static bool Foreground=true;
 static T Bounded<T>(Func<T> read,string operation){return read();}
 static void RequireInputForeground(){if(!Foreground)throw new InvalidOperationException("fixture foreground lost");}
 static void MovePhysical(int x,int y){Moves++;LastX=x;LastY=y;}
 static void Send(INPUT input){if(input.type!=0)throw new InvalidOperationException("Unexpected fixture input");if(input.u.mi.dwFlags==2)Downs++;else if(input.u.mi.dwFlags==4)Ups++;else throw new InvalidOperationException("Unexpected fixture mouse flags");}
 // Mock only provider identity/bounds. Center containment must be performed by
 // the actual extracted production Click/RequireMenuClickBounds, not this stub.
 static System.Windows.Rect OwnedMenuBounds(AutomationElement item,int pid){
  PopupReads++;
  if(pid!=7||item.Current.ProcessId!=7||item.Parent==null||item.Parent.Current.ProcessId!=7||item.Parent.ClassName!="QQuickPopupItem")throw new InvalidOperationException("fixture owned popup changed");
  return item.Parent.Current.BoundingRectangle;
 }
'@
$suffix=@'
 public static string Run(string mode){
  Moves=Downs=Ups=PopupReads=LastX=LastY=0;Foreground=true;
  var popup=new AutomationElement();popup.ClassName="QQuickPopupItem";popup.Current.BoundingRectangle=new System.Windows.Rect(536,500,400,1370);
  var item=new AutomationElement();item.Parent=popup;item.Current.BoundingRectangle=new System.Windows.Rect(536,1000,400,96);
  if(mode=="clipped-menu")item.Current.BoundingRectangle=new System.Windows.Rect(536,1870,400,96);
  if(mode=="wrong-popup-owner")popup.Current.ProcessId=8;
  if(mode=="wrong-popup-class")popup.ClassName="another-popup";
  if(mode=="offscreen-menu")item.Current.IsOffscreen=true;
  if(mode=="nonmenu-button"){item.Parent=null;item.Current.ControlType=ControlType.Button;item.Current.BoundingRectangle=new System.Windows.Rect(20,30,40,40);}
  try{Click(item);return "clicked";}catch(Exception error){return error.GetType().Name+":"+error.Message;}
 }
}
'@
Add-Type -AssemblyName WindowsBase
Add-Type -TypeDefinition ($prefix+$methods+$suffix) -ReferencedAssemblies @([Windows.Rect].Assembly.Location)
$checks=New-Object 'Collections.Generic.List[string]'
function Assert([bool]$condition,[string]$reason){if(-not$condition){throw $reason}}
$result=[Probe]::Run('clipped-menu')
Assert ($result-match'^InvalidOperationException:'-and[Probe]::Moves-eq0-and[Probe]::Downs-eq0-and[Probe]::Ups-eq0) ('Clipped row center1918 beyond owned-popup bottom1870 must be rejected BEFORE moving/sending: '+$result+' moves='+[Probe]::Moves+' downs='+[Probe]::Downs)
$checks.Add('visible-uia-menu-center-outside-owned-popup-rejects-before-all-input')
foreach($mode in @('wrong-popup-owner','wrong-popup-class')){
 $result=[Probe]::Run($mode)
 Assert ($result-match'^InvalidOperationException:'-and[Probe]::Moves-eq0-and[Probe]::Downs-eq0-and[Probe]::Ups-eq0) ('Changed owned popup allowed input: '+$mode+' '+$result)
 $checks.Add($mode+'-rejects-before-all-input')
}
$result=[Probe]::Run('contained-menu')
Assert ($result-eq'clicked'-and[Probe]::PopupReads-eq1-and[Probe]::Moves-eq1-and[Probe]::Downs-eq1-and[Probe]::Ups-eq1-and[Probe]::LastX-eq736-and[Probe]::LastY-eq1048) 'Contained actual menu target must produce exactly one center click through its owned popup.'
$checks.Add('contained-menu-center-permits-one-recorded-click')
$result=[Probe]::Run('offscreen-menu')
Assert ($result-match'^InvalidOperationException:'-and[Probe]::Moves-eq0-and[Probe]::Downs-eq0-and[Probe]::Ups-eq0) 'Existing offscreen-control rejection was lost.'
$checks.Add('offscreen-menu-remains-rejected-before-input')
$result=[Probe]::Run('nonmenu-button')
Assert ($result-eq'clicked'-and[Probe]::PopupReads-eq0-and[Probe]::Moves-eq1-and[Probe]::Downs-eq1-and[Probe]::Ups-eq1-and[Probe]::LastX-eq40-and[Probe]::LastY-eq50) 'Non-menu button click behavior changed.'
$checks.Add('nonmenu-button-keeps-one-center-click-without-menu-proof')
[ordered]@{passed=$true;count=$checks.Count;checks=@($checks.ToArray());helper=[IO.Path]::GetFullPath($HelperPath);helperSha256=(Get-FileHash -LiteralPath $HelperPath -Algorithm SHA256).Hash.ToLowerInvariant();applicationStarted=$false;inputSent=$false;inputImplementation='local recorder stubs'}|ConvertTo-Json -Depth 4
