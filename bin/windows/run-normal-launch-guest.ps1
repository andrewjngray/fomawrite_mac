# Windows PowerShell 5.1, interactive desktop; ordinary deployed app startup.
# UI Automation reads control bounds/state/text. Only SendInput clicks and keys
# interact with the application; no diagnostic branch or command invocation.
[CmdletBinding()]
param([Parameter(Mandatory = $true)][string]$Package,
      [Parameter(Mandatory = $true)][string]$Output,
      [string]$DriverDirectory = $PSScriptRoot)
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
if ($env:OS -ne 'Windows_NT') { throw 'Run in an unlocked Windows interactive desktop.' }
if ([string]::IsNullOrWhiteSpace($DriverDirectory)) { throw 'ScriptBlock invocation requires -DriverDirectory.' }
$driverResolution = Resolve-Path -LiteralPath $DriverDirectory
if ($driverResolution.Provider.Name -ne 'FileSystem') { throw 'DriverDirectory must use the filesystem provider.' }
$driverRoot = [IO.Path]::GetFullPath($driverResolution.ProviderPath).TrimEnd('\')
$driverPath = Join-Path $driverRoot 'run-normal-launch-guest.ps1'
$inputHelperPath = Join-Path $driverRoot 'normal-launch-input.cs'
foreach ($requiredDriverFile in @($driverPath, $inputHelperPath)) {
    if (-not (Test-Path -LiteralPath $requiredDriverFile -PathType Leaf)) { throw "Required driver file is missing: $requiredDriverFile" }
}
Add-Type -AssemblyName UIAutomationClient, UIAutomationTypes, WindowsBase, System.Drawing, System.Windows.Forms
Add-Type -Path $inputHelperPath -ReferencedAssemblies @([Windows.Automation.AutomationElement].Assembly.Location, [Windows.Automation.ControlType].Assembly.Location, [Windows.Rect].Assembly.Location)
[NormalLaunchInput]::Initialize()
$resolvedPackage = Resolve-Path -LiteralPath $Package
if ($resolvedPackage.Provider.Name -ne 'FileSystem') { throw 'Package must use the Windows filesystem provider.' }
$packageRoot = [IO.Path]::GetFullPath($resolvedPackage.ProviderPath).TrimEnd('\')
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
$viewObservations = $null
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
function Wait-WinRt($Operation, [Type]$ResultType) {
    $method = [System.WindowsRuntimeSystemExtensions].GetMethods() | Where-Object {
        $_.Name -eq 'AsTask' -and $_.IsGenericMethod -and $_.GetGenericArguments().Count -eq 1 -and
        $_.GetParameters().Count -eq 1 -and $_.GetParameters()[0].ParameterType.IsGenericType -and
        $_.GetParameters()[0].ParameterType.GetGenericTypeDefinition().FullName -eq 'Windows.Foundation.IAsyncOperation`1'
    } | Select-Object -First 1
    $task = $method.MakeGenericMethod($ResultType).Invoke($null, @($Operation))
    if (-not $task.Wait(6000)) { throw 'Windows OCR operation timed out.' }
    return $task.Result
}
function Read-LiveRenderedText($SourceBounds) {
    # The Source text rectangle is inside the writing pane. Reusing it after
    # Live is selected excludes Library excerpts and the read-only publishing
    # pane, so their copies of the fixture cannot satisfy this render check.
    $windowBounds = $script:main.Current.BoundingRectangle
    $left = [int][Math]::Ceiling([Math]::Max($SourceBounds.Left, $windowBounds.Left))
    $top = [int][Math]::Ceiling([Math]::Max($SourceBounds.Top, $windowBounds.Top))
    $right = [int][Math]::Floor([Math]::Min($SourceBounds.Right, $windowBounds.Right))
    $bottom = [int][Math]::Floor([Math]::Min($SourceBounds.Bottom, $windowBounds.Bottom))
    if ($right - $left -lt 100 -or $bottom - $top -lt 60) { throw 'Source writing rectangle cannot provide a visible Live render region.' }
    $imagePath = Join-Path $outputRoot 'live-editor-content.png'
    $bitmap = New-Object Drawing.Bitmap(($right - $left), ($bottom - $top))
    $graphics = [Drawing.Graphics]::FromImage($bitmap)
    try {
        $graphics.CopyFromScreen($left, $top, 0, 0, $bitmap.Size)
        $bitmap.Save($imagePath, [Drawing.Imaging.ImageFormat]::Png)
    } finally { $graphics.Dispose(); $bitmap.Dispose() }
    Add-Type -AssemblyName System.Runtime.WindowsRuntime
    $null = [Windows.Storage.StorageFile, Windows.Storage, ContentType=WindowsRuntime]
    $null = [Windows.Storage.FileAccessMode, Windows.Storage, ContentType=WindowsRuntime]
    $null = [Windows.Storage.Streams.IRandomAccessStream, Windows.Storage.Streams, ContentType=WindowsRuntime]
    $null = [Windows.Graphics.Imaging.BitmapDecoder, Windows.Graphics.Imaging, ContentType=WindowsRuntime]
    $null = [Windows.Graphics.Imaging.SoftwareBitmap, Windows.Graphics.Imaging, ContentType=WindowsRuntime]
    $null = [Windows.Media.Ocr.OcrEngine, Windows.Foundation, ContentType=WindowsRuntime]
    $null = [Windows.Media.Ocr.OcrResult, Windows.Foundation, ContentType=WindowsRuntime]
    $null = [Windows.Globalization.Language, Windows.Globalization, ContentType=WindowsRuntime]
    $file = Wait-WinRt ([Windows.Storage.StorageFile]::GetFileFromPathAsync($imagePath)) ([Windows.Storage.StorageFile])
    $stream = Wait-WinRt ($file.OpenAsync([Windows.Storage.FileAccessMode]::Read)) ([Windows.Storage.Streams.IRandomAccessStream])
    $software = $null
    try {
        $decoder = Wait-WinRt ([Windows.Graphics.Imaging.BitmapDecoder]::CreateAsync($stream)) ([Windows.Graphics.Imaging.BitmapDecoder])
        $software = Wait-WinRt ($decoder.GetSoftwareBitmapAsync()) ([Windows.Graphics.Imaging.SoftwareBitmap])
        $engine = [Windows.Media.Ocr.OcrEngine]::TryCreateFromLanguage((New-Object Windows.Globalization.Language('en-US')))
        if (-not $engine) { throw 'English Windows OCR is required to verify actual Live sample rendering.' }
        $result = Wait-WinRt ($engine.RecognizeAsync($software)) ([Windows.Media.Ocr.OcrResult])
        $text = $result.Text
    } finally {
        if ($software) { $software.Dispose() }
        $stream.Dispose()
    }
    $text | Set-Content -LiteralPath (Join-Path $outputRoot 'live-editor-ocr.txt') -Encoding UTF8
    return [ordered]@{ text = $text; left = $left; top = $top; width = $right - $left; height = $bottom - $top }
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
}
function Activate-Application {
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
    $receiptRoot = [IO.Path]::GetFullPath($isolation.profileRoot.Replace('/', '\')).TrimEnd('\')
    if ($receiptRoot -ne $profile.TrimEnd('\') -or $isolation.normalStartup -ne $true) { throw 'Invalid ordinary-startup profile receipt.' }
    $digest = [Security.Cryptography.SHA256]::Create()
    try { $namespaceHash = [BitConverter]::ToString($digest.ComputeHash([Text.Encoding]::UTF8.GetBytes($isolation.profileRoot))).Replace('-', '').ToLowerInvariant().Substring(0, 20) }
    finally { $digest.Dispose() }
    $namespace = 'FomawriteAcceptance-' + $namespaceHash
    if ($isolation.applicationName -cne $namespace) { throw 'Profile application name is not derived from its canonical root.' }
    foreach ($key in @('settingsPath', 'dataPath', 'cachePath')) {
        if (($isolation.$key -split '[\\/]') -cnotcontains $namespace) { throw "Profile $key escapes its isolated namespace." }
    }
    if ($isolation.identity -cnotmatch '^[0-9a-f]{20}$') { throw 'Invalid isolated workspace/IPC identity.' }
    $receiptWorkspace = [IO.Path]::GetFullPath($isolation.workspacePath.Replace('/', '\'))
    $expectedWorkspace = [IO.Path]::GetFullPath((Join-Path $isolation.dataPath ('workspace-' + $isolation.identity + '.json')))
    $receiptExecutable = [IO.Path]::GetFullPath($isolation.executable.Replace('/', '\'))
    if ($receiptWorkspace -ne $expectedWorkspace -or $receiptExecutable -ne $exe) { throw 'Isolated receipt workspace or executable differs.' }
    Activate-Application
    Passed $stage 'Deployed executable launched without command-line arguments and acknowledged isolated profile.'

    $stage = 'folder-picker-and-navigation'
    # Fresh defaults may hide Organizer. Use its actual menu control if needed.
    if (-not (Find-Control '^Add library location$')) { Open-Menu 'View' '^Organizer$' }
    Click-Control '^Add library location$'
    $dialog = Native-Picker '^Add writing location$'
    [NormalLaunchInput]::RequireInputForeground()
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
    [NormalLaunchInput]::RequireInputForeground()
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
    [NormalLaunchInput]::TypeSource($expected)
    # Deliberately exercise the menu Save action, rather than substituting Ctrl+S.
    Open-Menu 'File' '^Save$'
    Wait-For { [IO.File]::ReadAllText($sample).Replace("`r`n", "`n") -eq $expected } 'saved exact edited Markdown on disk' | Out-Null
    Passed $stage 'Windows text input and Windows Save menu action changed the disposable file exactly.'

    $stage = 'view-buttons'
    $sourceBounds = $editor.Current.BoundingRectangle
    $sourceBeforeVisible = $null -ne (Find-Control '^Markdown editor$')
    if (-not $sourceBeforeVisible) { throw 'Source editor must be visible before testing the mode buttons.' }
    # Qt's ModeButton exposes Invoke/Value but no Toggle/Selection pattern.
    # Verify actual visible editor state and rendered pixels instead.
    $rect = $script:main.Current.BoundingRectangle
    [NormalLaunchInput]::RequireInputForeground()
    [NormalLaunchInput]::MovePhysical([int]($rect.Left + $rect.Width / 2), [int]($rect.Bottom - 8))
    Click-Control ('^Live ' + [char]0x2014 + ' edit with Markdown rendered in place$')
    Wait-For { -not (Find-Control '^Markdown editor$') } 'Source editor hidden after Live button click' | Out-Null
    $liveLabel = Wait-For { Find-Control '^Live follows the output style:' } 'appearance control reflects Live editor mode'
    $liveModeLabel = $liveLabel.Current.Name
    $render = Wait-For {
        $observation = Read-LiveRenderedText $sourceBounds
        if ($observation.text -match '(?i)Normal\s+launch\s+acceptance' -and $observation.text -match '(?i)Saved\s+with\s+Windows\s+input\b') { $observation }
    } 'Live writing region visibly renders the exact sample heading and saved sentence' 30
    Capture 'live-mode-rendered'
    Click-Control '^Edit Markdown source$'
    $editor = Wait-For { Find-Control '^Markdown editor$' } 'Source editor visible after Source button click'
    if ([NormalLaunchInput]::Text($editor).Replace("`r`n", "`n") -ne $expected) { throw 'View switching changed document source.' }
    $viewObservations = [ordered]@{
        sourceBeforeVisible = $true; liveSourceAbsent = $true; sourceAfterVisible = $true
        liveModeLabel = $liveModeLabel; liveHeadingRendered = $true; liveBodyRendered = $true
        renderRegion = [ordered]@{ left = $render.left; top = $render.top; width = $render.width; height = $render.height }
    }
    Passed $stage 'Windows Live/Source buttons switched visible editors; OCR of the writing region proved Live rendered the sample, then Source preserved exact Markdown.'

    $stage = 'quit-and-restore'
    Quit
    Launch
    Activate-Application
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
        driverSha256 = Hash $driverPath; inputHelperSha256 = Hash $inputHelperPath
        profileRoot = $profile; outputRoot = $outputRoot
        profileReceiptSha256 = if (Test-Path -LiteralPath (Join-Path $profile 'profile.json')) { Hash (Join-Path $profile 'profile.json') } else { $null }
        launches = @($launches.ToArray()); checks = @($checks.ToArray())
        viewObservations = $viewObservations
        sampleSha256 = Hash $sample; evidenceSha256 = $evidence
    }
    $report | ConvertTo-Json -Depth 10 | Set-Content -LiteralPath (Join-Path $outputRoot 'normal-launch-report.json') -Encoding UTF8
}
if ($failure) { throw "Normal-launch acceptance failed at ${stage}: $failure" }
Write-Output "Normal-launch Windows input gate passed; independently verify $outputRoot/normal-launch-report.json."
