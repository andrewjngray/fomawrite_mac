# Non-UI regression: execute the real driver wait and picker selection with a stub.
# Run in a fresh Windows PowerShell 5.1 process; no application is started.
[CmdletBinding()]
param([string]$DriverPath = '')
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
if ([string]::IsNullOrWhiteSpace($DriverPath)) {
    $DriverPath = Join-Path $PSScriptRoot '..\bin\windows\run-normal-launch-guest.ps1'
}
$tokens = $null
$parseErrors = $null
$ast = [Management.Automation.Language.Parser]::ParseFile(
    [IO.Path]::GetFullPath($DriverPath), [ref]$tokens, [ref]$parseErrors)
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
public sealed class NormalLaunchFixtureProcess {
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

[ordered]@{ passed = $true; count = $checks.Count; checks = @($checks.ToArray());
    driver = [IO.Path]::GetFullPath($DriverPath); applicationStarted = $false } | ConvertTo-Json -Depth 4
