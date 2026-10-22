# Windows PowerShell 5.1, interactive desktop; ordinary deployed app startup.
# UI Automation reads control bounds/state/text. Only SendInput clicks and keys
# interact with the application; no diagnostic branch or command invocation.
[CmdletBinding()]
param([Parameter(Mandatory = $true)][string]$Package,
      [Parameter(Mandatory = $true)][string]$Output)
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
if ($env:OS -ne 'Windows_NT') { throw 'Run in an unlocked Windows interactive desktop.' }
Add-Type -AssemblyName UIAutomationClient, UIAutomationTypes, WindowsBase, System.Drawing, System.Windows.Forms
Add-Type -Path (Join-Path $PSScriptRoot 'normal-launch-input.cs') -ReferencedAssemblies @([Windows.Automation.AutomationElement].Assembly.Location, [Windows.Automation.ControlType].Assembly.Location, [Windows.Rect].Assembly.Location)
[NormalLaunchInput]::Initialize()
$packageRoot = [IO.Path]::GetFullPath((Resolve-Path -LiteralPath $Package).Path).TrimEnd('\')
$exe = Join-Path $packageRoot 'Fomawrite.exe'
$manifestPath = Join-Path $packageRoot 'manifest.json'
$manifest = Get-Content -LiteralPath $manifestPath -Raw | ConvertFrom-Json
function Hash([string]$path) { (Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash.ToLowerInvariant() }
foreach ($entry in $manifest.files.PSObject.Properties) {
    $path = [IO.Path]::GetFullPath((Join-Path $packageRoot $entry.Name))
    if ([IO.Path]::IsPathRooted($entry.Name) -or -not $path.StartsWith($packageRoot + '\', [StringComparison]::OrdinalIgnoreCase)) { throw "Invalid manifest path: $($entry.Name)" }
    if ((Hash $path) -ne $entry.Value) { throw "Deployed file hash mismatch: $($entry.Name)" }
}
if ((Hash $exe) -ne $manifest.executableSha256) { throw 'Executable differs from package manifest.' }
$outputRoot = [IO.Path]::GetFullPath($Output)
if (Test-Path -LiteralPath $outputRoot) { throw 'Use a new output directory; old evidence cannot satisfy acceptance.' }
New-Item -ItemType Directory -Path $outputRoot | Out-Null
$profile = Join-Path $outputRoot 'profile'
$writing = Join-Path $outputRoot 'writing-normal-launch'
New-Item -ItemType Directory -Path $profile, $writing | Out-Null
$sample = Join-Path $writing 'normal-launch-sample.md'
$initial = "# Normal launch acceptance`n`nUntouched sample.`n"
$expected = "# Normal launch acceptance`n`nSaved with Windows input.`n"
[IO.File]::WriteAllText($sample, $initial, (New-Object Text.UTF8Encoding($false)))
$checks = New-Object 'System.Collections.Generic.List[object]'
$launches = New-Object 'System.Collections.Generic.List[object]'
$saved = @{}
foreach ($entry in Get-ChildItem Env:) {
    if ($entry.Name -match '^(QT_|QTWEBENGINE|QML_|FOMAWRITE_)' -or $entry.Name -eq 'PATH') { $saved[$entry.Name] = $entry.Value }
}
$script:process = $null
$script:main = $null
$stage = 'ordinary-startup'
$started = [DateTime]::UtcNow
$failure = $null
function Wait-For([scriptblock]$condition, [string]$reason, [int]$seconds = 20) {
    $deadline = [DateTime]::UtcNow.AddSeconds($seconds)
    do {
        if ($script:process.HasExited) { throw "Application exited before $reason (status $($script:process.ExitCode))." }
        $result = & $condition
        if ($result) { return $result }
        Start-Sleep -Milliseconds 150
    } while ([DateTime]::UtcNow -lt $deadline)
    throw "Timed out: $reason"
}
function Find-Control([string]$pattern) { [NormalLaunchInput]::Find($script:main, $pattern) }
function Click-Control([string]$pattern) {
    $element = Wait-For { Find-Control $pattern } "visible control $pattern"
    [NormalLaunchInput]::Click($element)
}
function Capture([string]$name) {
    [NormalLaunchInput]::Describe($script:main) | Set-Content -LiteralPath (Join-Path $outputRoot ($name + '-uia.txt')) -Encoding UTF8
    $bounds = [Windows.Forms.SystemInformation]::VirtualScreen
    $bitmap = New-Object Drawing.Bitmap($bounds.Width, $bounds.Height)
    $graphics = [Drawing.Graphics]::FromImage($bitmap)
    try {
        $graphics.CopyFromScreen($bounds.Left, $bounds.Top, 0, 0, $bitmap.Size)
        $bitmap.Save((Join-Path $outputRoot ($name + '.png')), [Drawing.Imaging.ImageFormat]::Png)
    } finally { $graphics.Dispose(); $bitmap.Dispose() }
}
function Passed([string]$name, [string]$detail) {
    Capture $name
    $checks.Add([ordered]@{ name = $name; passed = $true; detail = $detail; observedUtc = [DateTime]::UtcNow.ToString('o') })
}
function Launch {
    $index = $launches.Count + 1
    # Intentionally no application arguments: this must reach ordinary startup.
    $script:process = Start-Process -FilePath $exe -WorkingDirectory $packageRoot -PassThru `
        -RedirectStandardOutput (Join-Path $outputRoot "launch-$index-stdout.log") `
        -RedirectStandardError (Join-Path $outputRoot "launch-$index-stderr.log")
    $script:process.Handle | Out-Null
    $launches.Add([ordered]@{ processId = $script:process.Id; arguments = @(); startedUtc = [DateTime]::UtcNow.ToString('o'); exitCode = $null })
    $script:main = Wait-For { [NormalLaunchInput]::Root($script:process.Id) } 'ordinary application window' 40
    [NormalLaunchInput]::Foreground([IntPtr]$script:main.Current.NativeWindowHandle)
}
function Open-Menu([string]$menu, [string]$item) {
    Click-Control ('^' + $menu + '$')
    Click-Control $item
}
function Native-Picker([string]$title) {
    $dialog = Wait-For {
        $element = [NormalLaunchInput]::ForegroundRoot($script:process.Id)
        if ($element -and $element.Current.ClassName -eq '#32770' -and $element.Current.Name -match $title) { $element }
    } "native $title picker"
    [NormalLaunchInput]::Describe($dialog) | Set-Content -LiteralPath (Join-Path $outputRoot ($stage + '-picker-uia.txt')) -Encoding UTF8
    return $dialog
}
function Quit {
    Open-Menu 'File' '^Exit$'
    if (-not $script:process.WaitForExit(15000)) { throw 'Exit menu did not terminate the ordinary application.' }
    $script:process.WaitForExit()
    $launches[$launches.Count - 1].exitCode = $script:process.ExitCode
    if ($script:process.ExitCode -ne 0) { throw "Application exited with $($script:process.ExitCode)." }
}
try {
    foreach ($name in $saved.Keys) { [Environment]::SetEnvironmentVariable($name, $null, 'Process') }
    $env:PATH = $packageRoot + ';' + (Join-Path $env:SystemRoot 'System32') + ';' + $env:SystemRoot
    $env:QT_FORCE_STDERR_LOGGING = '1'
    $env:FOMAWRITE_PROFILE_ROOT = $profile
    Launch
    # App-written isolation receipt must exist before any editing or picker input.
    $profileReceipt = Join-Path $profile 'profile.json'
    if (-not (Test-Path -LiteralPath $profileReceipt)) { throw 'App did not acknowledge isolated ordinary-startup profile; refusing UI input.' }
    $isolation = Get-Content -LiteralPath $profileReceipt -Raw | ConvertFrom-Json
    if ($isolation.profileRoot -ne $profile -or $isolation.normalStartup -ne $true) { throw 'Invalid ordinary-startup profile receipt.' }
    Passed $stage 'Deployed executable launched without command-line arguments and acknowledged isolated profile.'

    $stage = 'folder-picker-and-navigation'
    # Fresh defaults may hide Organizer. Use its actual menu control if needed.
    if (-not (Find-Control '^Add library location$')) { Open-Menu 'View' '^Organizer$' }
    Click-Control '^Add library location$'
    $dialog = Native-Picker '^Add writing location$'
    [NormalLaunchInput]::Chord(0x12, 0x44) # Alt+D: native address bar
    [NormalLaunchInput]::Type($writing)
    [NormalLaunchInput]::Press(0x0D)
    Start-Sleep -Milliseconds 500
    $select = [NormalLaunchInput]::Find($dialog, '^Select Folder$')
    if (-not $select) { throw 'Native folder picker has no Select Folder button.' }
    [NormalLaunchInput]::Click($select)
    Click-Control '^writing-normal-launch$'
    Wait-For { Find-Control '^normal-launch-sample\.md$' } 'sample file visible after Windows folder navigation' | Out-Null
    Passed $stage 'Organizer button opened native folder picker; selected location exposed sample in Library.'

    $stage = 'file-menu-and-native-picker'
    Open-Menu 'File' ('^Open File' + [char]0x2026 + '$')
    $dialog = Native-Picker 'Open'
    [NormalLaunchInput]::Chord(0x12, 0x4E) # Alt+N: native File name
    [NormalLaunchInput]::Type($sample)
    [NormalLaunchInput]::Press(0x0D)
    Wait-For { Find-Control '^normal-launch-sample\.md$' } 'opened document title' | Out-Null
    # Require a readable editor with exact source before altering the fixture.
    Open-Menu 'View' '^Source$'
    $editor = Wait-For { Find-Control '^Markdown editor$' } 'Source editor'
    Wait-For { [NormalLaunchInput]::Text($editor).Replace("`r`n", "`n") -eq $initial } 'opened sample source' | Out-Null
    Passed $stage 'Windows File menu action opened native file picker and the selected file source matched exactly.'

    $stage = 'edit-and-save'
    [NormalLaunchInput]::Click($editor)
    [NormalLaunchInput]::Chord(0x11, 0x41)
    [NormalLaunchInput]::Type($expected)
    # Deliberately exercise the menu Save action, rather than substituting Ctrl+S.
    Open-Menu 'File' '^Save$'
    Wait-For { [IO.File]::ReadAllText($sample).Replace("`r`n", "`n") -eq $expected } 'saved exact edited Markdown on disk' | Out-Null
    Passed $stage 'Windows text input and Windows Save menu action changed the disposable file exactly.'

    $stage = 'view-buttons'
    # Hover the window bottom edge to reveal the production fading footer.
    $rect = $script:main.Current.BoundingRectangle
    [Windows.Forms.Cursor]::Position = New-Object Drawing.Point([int]($rect.Left + $rect.Width / 2), [int]($rect.Bottom - 8))
    Click-Control ('^Live ' + [char]0x2014 + ' edit with Markdown rendered in place$')
    Wait-For { $button = Find-Control ('^Live ' + [char]0x2014 + ' edit with Markdown rendered in place$'); $button -and [NormalLaunchInput]::Checked($button) } 'Live button checked after Windows click' | Out-Null
    Click-Control '^Edit Markdown source$'
    Wait-For { $button = Find-Control '^Edit Markdown source$'; $button -and [NormalLaunchInput]::Checked($button) } 'Source button checked after Windows click' | Out-Null
    $editor = Wait-For { Find-Control '^Markdown editor$' } 'Source editor after mode switch'
    if ([NormalLaunchInput]::Text($editor).Replace("`r`n", "`n") -ne $expected) { throw 'View switching changed document source.' }
    Passed $stage 'Windows Live/Source footer buttons changed checked state and preserved exact Markdown.'

    $stage = 'quit-and-restore'
    Quit
    Launch
    $editor = Wait-For { Find-Control '^Markdown editor$' } 'restored Source editor' 40
    Wait-For { [NormalLaunchInput]::Text($editor).Replace("`r`n", "`n") -eq $expected } 'normal relaunch restored saved sample' | Out-Null
    Passed $stage 'Exit terminated with zero status; a second argument-free launch restored the edited sample.'
    Quit
} catch {
    $failure = $_.Exception.Message
    $checks.Add([ordered]@{ name = $stage; passed = $false; detail = $failure; observedUtc = [DateTime]::UtcNow.ToString('o') })
    try { Capture 'failure' } catch { Write-Warning "Could not capture failure: $_" }
} finally {
    if ($script:process -and -not $script:process.HasExited) {
        # Only this run's isolated process may be terminated after failed acceptance.
        $script:process.Kill(); $script:process.WaitForExit()
        $launches[$launches.Count - 1].exitCode = $script:process.ExitCode
    }
    foreach ($entry in Get-ChildItem Env:) {
        if ($entry.Name -match '^(QT_|QTWEBENGINE|QML_|FOMAWRITE_)' -or $entry.Name -eq 'PATH') { [Environment]::SetEnvironmentVariable($entry.Name, $null, 'Process') }
    }
    foreach ($name in $saved.Keys) { [Environment]::SetEnvironmentVariable($name, $saved[$name], 'Process') }
    $evidence = [ordered]@{}
    foreach ($file in Get-ChildItem -LiteralPath $outputRoot -File) { $evidence[$file.Name] = Hash $file.FullName }
    $report = [ordered]@{
        schemaVersion = 1; gate = 'windows-normal-launch'; passed = ($null -eq $failure)
        executable = $exe; executableSha256 = Hash $exe; manifestSha256 = Hash $manifestPath
        startedUtc = $started.ToString('o'); finishedUtc = [DateTime]::UtcNow.ToString('o')
        inputMethod = 'Win32-SendInput'; discoveryMethod = 'read-only-UIAutomation'
        driverSha256 = Hash $PSCommandPath; inputHelperSha256 = Hash (Join-Path $PSScriptRoot 'normal-launch-input.cs')
        profileRoot = $profile; outputRoot = $outputRoot
        profileReceiptSha256 = if (Test-Path -LiteralPath (Join-Path $profile 'profile.json')) { Hash (Join-Path $profile 'profile.json') } else { $null }
        launches = @($launches.ToArray()); checks = @($checks.ToArray())
        sampleSha256 = Hash $sample; evidenceSha256 = $evidence
    }
    $report | ConvertTo-Json -Depth 10 | Set-Content -LiteralPath (Join-Path $outputRoot 'normal-launch-report.json') -Encoding UTF8
}
if ($failure) { throw "Normal-launch acceptance failed at ${stage}: $failure" }
Write-Output "Normal-launch Windows input gate passed; independently verify $outputRoot/normal-launch-report.json."
