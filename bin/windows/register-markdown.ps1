# Register the verified portable executable for this Windows user.
[CmdletBinding()]
param([Parameter(Mandatory)][string]$Executable, [Parameter(Mandatory)][string]$IconSource)
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
if (-not (Test-Path -LiteralPath $Executable -PathType Leaf) -or
    [IO.Path]::GetFileName($Executable) -ine 'Fomawrite.exe') {
    throw 'Markdown registration requires a local Fomawrite.exe.'
}

# Keep Explorer's icon local even when the launcher runs from a shared folder.
$iconFolder = Join-Path $env:LOCALAPPDATA 'Fomawrite\Shell'
[IO.Directory]::CreateDirectory($iconFolder) | Out-Null
$iconFile = Join-Path $iconFolder 'Fomawrite.ico'
$changed = -not (Test-Path -LiteralPath $iconFile -PathType Leaf) -or
    (Get-FileHash -LiteralPath $iconFile).Hash -ne (Get-FileHash -LiteralPath $IconSource).Hash
if ($changed) { Copy-Item -LiteralPath $IconSource -Destination $iconFile -Force }
$icon = '"' + $iconFile + '",0'

$progId = 'Fomawrite.Markdown.Portable'
$base = 'Software\Classes\'
$definitions = @(
    @($progId, '', 'Fomawrite Markdown Document'),
    @("$progId\Application", 'ApplicationName', 'Fomawrite (Windows)'),
    @("$progId\Application", 'ApplicationIcon', $icon),
    @("$progId\DefaultIcon", '', $icon),
    @("$progId\shell\open\command", '', ('"' + $Executable + '" "%1"')),
    @('.md\OpenWithProgids', $progId, '')
)

foreach ($definition in $definitions) {
    $key = [Microsoft.Win32.Registry]::CurrentUser.CreateSubKey($base + $definition[0])
    try {
        if ($key.GetValue($definition[1]) -cne $definition[2]) {
            $key.SetValue($definition[1], $definition[2], [Microsoft.Win32.RegistryValueKind]::String)
            $changed = $true
        }
    } finally { $key.Dispose() }
}
if ($changed) {
    if (-not ('Fomawrite.FomawriteAssociationChange' -as [type])) {
        Add-Type -Name FomawriteAssociationChange -MemberDefinition @'
[System.Runtime.InteropServices.DllImport("shell32.dll")]
public static extern void SHChangeNotify(uint eventId, uint flags, System.IntPtr item1, System.IntPtr item2);
'@ -Namespace Fomawrite
    }
    # SHCNE_ASSOCCHANGED asks Explorer to refresh its cached associations.
    [Fomawrite.FomawriteAssociationChange]::SHChangeNotify(0x08000000, 0, [IntPtr]::Zero, [IntPtr]::Zero)
}
