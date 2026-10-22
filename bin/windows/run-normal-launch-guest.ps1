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
$script:NativeFilenameSequence=0
function Type-NativeFilePath([IntPtr]$dialog,[string]$path) {
    $script:NativeFilenameSequence++
    $proof=[ordered]@{dialogHwnd=$dialog.ToInt64();expectedPublicPath=$path;typedOnce=$false;matched=$false;before=$null;focusSamples=@();samples=@();failure=$null}
    try {
        $proof.before=Wait-For {$w=[NativeFilenameWitness]::Read($dialog,$script:process.Id);$proof.focusSamples+=,$w;if($w.ready){$w}} 'actual owned native FileName Edit focus before typing' 5
        [NormalLaunchInput]::Chord(0x11,0x41)
        [NormalLaunchInput]::Type($path);$proof.typedOnce=$true
        $samples=New-Object 'Collections.Generic.List[object]';$deadline=[DateTime]::UtcNow.AddSeconds(4)
        do {
            $w=[NativeFilenameWitness]::Read($dialog,$script:process.Id);$samples.Add($w)
            if($w.ready-and$w.value-ceq$path){$proof.matched=$true;break}
            Start-Sleep -Milliseconds 150
        }while([DateTime]::UtcNow-lt$deadline)
        $proof.samples=@($samples.ToArray())
        if(-not$proof.matched){throw 'One typed native filename did not settle to the exact full path; refusing Enter or input replay'}
    } catch {$proof.failure=$_.Exception.ToString();throw}
    finally {$proof|ConvertTo-Json -Depth 8|Set-Content (Join-Path $outputRoot ('native-filename-readiness-'+$script:NativeFilenameSequence+'.json')) -Encoding UTF8}
}
function Find-Control([string]$pattern) { [NormalLaunchInput]::FindOwnedWindow($script:process.Id, $pattern) }
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
function Read-ExitMenuTarget($anchor) {
    $popup = [NormalLaunchInput]::OwnedMenuBounds($anchor, $script:process.Id)
    $window = $script:main.Current.BoundingRectangle
    if (-not $window.Contains($popup)) { throw 'File popup crop escaped owned application window.' }
    [NormalLaunchInput]::RequireInputForeground()
    $left=[int][Math]::Ceiling($popup.Left); $top=[int][Math]::Ceiling($popup.Top)
    $width=[int][Math]::Floor($popup.Right)-$left; $height=[int][Math]::Floor($popup.Bottom)-$top
    $script:ExitMenuOcrAttempt++
    $prefix='exit-menu-' + $script:process.Id + '-sample-' + $script:ExitMenuOcrAttempt
    $path=Join-Path $outputRoot ($prefix + '-content.png')
    $bitmap=New-Object Drawing.Bitmap($width,$height); $graphics=[Drawing.Graphics]::FromImage($bitmap)
    try { $graphics.CopyFromScreen($left,$top,0,0,$bitmap.Size); $capturedAt=[Diagnostics.Stopwatch]::GetTimestamp(); $bitmap.Save($path,[Drawing.Imaging.ImageFormat]::Png) }
    finally { $graphics.Dispose(); $bitmap.Dispose() }
    $file=Wait-WinRt ([Windows.Storage.StorageFile]::GetFileFromPathAsync($path)) ([Windows.Storage.StorageFile])
    $stream=Wait-WinRt ($file.OpenAsync([Windows.Storage.FileAccessMode]::Read)) ([Windows.Storage.Streams.IRandomAccessStream])
    $software=$null
    try {
        $decoder=Wait-WinRt ([Windows.Graphics.Imaging.BitmapDecoder]::CreateAsync($stream)) ([Windows.Graphics.Imaging.BitmapDecoder])
        $software=Wait-WinRt ($decoder.GetSoftwareBitmapAsync()) ([Windows.Graphics.Imaging.SoftwareBitmap])
        $engine=[Windows.Media.Ocr.OcrEngine]::TryCreateFromLanguage((New-Object Windows.Globalization.Language('en-US')))
        if (-not $engine) { throw 'English Windows OCR is required for the clipped File menu.' }
        $result=Wait-WinRt ($engine.RecognizeAsync($software)) ([Windows.Media.Ocr.OcrResult])
        $text=$result.Text
        $words=@($result.Lines | ForEach-Object { $_.Words | Where-Object { $_.Text -ceq 'Exit' } | ForEach-Object {
            [ordered]@{text=$_.Text;left=[double]$_.BoundingRect.X;top=[double]$_.BoundingRect.Y;width=[double]$_.BoundingRect.Width;height=[double]$_.BoundingRect.Height}
        } })
    } finally { if($software){$software.Dispose()}; $stream.Dispose() }
    $text | Set-Content -LiteralPath (Join-Path $outputRoot ($prefix + '-ocr.txt')) -Encoding UTF8
    $script:ExitMenuLastCrop=[ordered]@{key=((Hash $path)+':'+$popup.ToString());bounds=$popup;image=[IO.Path]::GetFileName($path);captureTicks=$capturedAt}
    if ($words.Count -eq 0) { return $null }
    if ($words.Count -ne 1) { throw 'Exit OCR match is not unique inside the owned File popup.' }
    $word=$words[0]; $x=[int][Math]::Round($left+$word.left+$word.width/2); $y=[int][Math]::Round($top+$word.top+$word.height/2)
    if (-not $popup.Contains($x,$y)) { throw 'Exit OCR target escaped File popup.' }
    $observation=[ordered]@{processId=$script:process.Id;popup=@{left=$left;top=$top;width=$width;height=$height};word=$word;x=$x;y=$y;contentSha256=Hash $path;contentFile=[IO.Path]::GetFileName($path);captureTicks=$capturedAt;uiaExitAbsent=$script:ExitMenuUiaAbsent;wheelDelta=-360;observedUtc=[DateTime]::UtcNow.ToString('o')}
    $observation | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath (Join-Path $outputRoot ($prefix + '-target.json')) -Encoding UTF8
    return $observation
}
function Open-Menu([string]$menu, [string]$item) {
    Click-Control ('^' + $menu + '$')
    if ($menu -ceq 'File' -and $item -ceq '^Exit$') {
        # Exit can be below the viewport in a short window. Scroll the actual
        # owned File popup with bounded guarded wheel gestures, then click its exact item.
        $first = Wait-For { Find-Control '^(?:New Document|Save)$' } 'visible owned File menu'
        $proof = [NormalLaunchInput]::RequireOwnedMenuPopup($first, $script:process.Id)
        $proof | Add-Content -LiteralPath (Join-Path $outputRoot 'exit-menu-scroll.txt') -Encoding UTF8
        $anchor = [NormalLaunchInput]::RetainOwnedMenuPopup($first, $script:process.Id)
        $anchor.ToString() | Add-Content -LiteralPath (Join-Path $outputRoot 'exit-menu-scroll.txt') -Encoding UTF8
        [NormalLaunchInput]::ScrollOwnedMenuDown($first, $script:process.Id) | Add-Content -LiteralPath (Join-Path $outputRoot 'exit-menu-scroll.txt') -Encoding UTF8
        $exit = Find-Control '^Exit$'
        $script:ExitMenuUiaAbsent=($null -eq $exit)
        $uiaObservation=if($exit){[ordered]@{name=$exit.Current.Name;bounds=$exit.Current.BoundingRectangle.ToString();processId=$exit.Current.ProcessId}}else{$null}
        $uiaObservation | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $outputRoot ('exit-menu-uia-target-' + $script:process.Id + '.json')) -Encoding UTF8
        # Even exposed UIA bounds can describe an intermediate wheel frame.
        # Always measure the settled pixels before clicking the actual Exit.
        [NormalLaunchInput]::Describe($script:main) | Set-Content -LiteralPath (Join-Path $outputRoot ('exit-menu-uia-after-scroll-' + $script:process.Id + '.txt')) -Encoding UTF8
        # Revalidate the proved popup itself; scrolled rows need not remain in UIA.
        [NormalLaunchInput]::OwnedMenuBounds($anchor, $script:process.Id) | Out-Null
        Capture ('exit-menu-visible-' + $script:process.Id)
        $script:ExitMenuOcrAttempt=0
        $script:ExitMenuStableKey=$null; $script:ExitMenuStable=@(); $script:ExitMenuStableClock=[Diagnostics.Stopwatch]::StartNew()
        $script:ExitMenuMissingKey=$null;$script:ExitMenuMissingCount=0;$script:ExitMenuMissingClock=[Diagnostics.Stopwatch]::StartNew();$script:ExitMenuGestures=1
        $script:ExitMenuDeadline=[Diagnostics.Stopwatch]::GetTimestamp()+20*[Diagnostics.Stopwatch]::Frequency
        $target = Wait-For {
            $candidate=Read-ExitMenuTarget $anchor
            if ([Diagnostics.Stopwatch]::GetTimestamp()-ge$script:ExitMenuDeadline){throw 'Owned Exit navigation exceeded twenty seconds'}
            if (-not $candidate) {
                $script:ExitMenuStableKey=$null;$script:ExitMenuStable=@();$script:ExitMenuStableClock.Restart()
                $crop=$script:ExitMenuLastCrop
                if($crop.key-ceq$script:ExitMenuMissingKey){$script:ExitMenuMissingCount++}else{$script:ExitMenuMissingKey=$crop.key;$script:ExitMenuMissingCount=1;$script:ExitMenuMissingClock.Restart()}
                if($script:ExitMenuMissingCount-ge3-and$script:ExitMenuMissingClock.ElapsedMilliseconds-ge300){
                    if($script:ExitMenuGestures-ge12){throw 'Exit absent after twelve owned wheel gestures'}
                    [NormalLaunchInput]::ScrollOwnedMenuDown($anchor,$script:process.Id,$crop.bounds,$script:ExitMenuDeadline)|Add-Content -LiteralPath (Join-Path $outputRoot 'exit-menu-scroll.txt') -Encoding UTF8
                    $script:ExitMenuGestures++;$script:ExitMenuMissingKey=$null;$script:ExitMenuMissingCount=0;$script:ExitMenuMissingClock.Restart()
                }
                return $null
            }

            $script:ExitMenuMissingKey=$null;$script:ExitMenuMissingCount=0;$script:ExitMenuMissingClock.Restart()
            $key=$candidate.contentSha256 + ':' + $candidate.x + ':' + $candidate.y
            if ($key -ceq $script:ExitMenuStableKey) { $script:ExitMenuStable+=,$candidate }
            else { $script:ExitMenuStableKey=$key; $script:ExitMenuStable=@($candidate); $script:ExitMenuStableClock.Restart() }
            if ($script:ExitMenuStable.Count -ge 3 -and $script:ExitMenuStableClock.ElapsedMilliseconds -ge 300) {
                $fresh=Read-ExitMenuTarget $anchor
                if ($fresh -and $fresh.contentSha256 -ceq $candidate.contentSha256 -and $fresh.x -eq $candidate.x -and $fresh.y -eq $candidate.y) {
                    $script:ExitMenuStable+=,$fresh
                    $fresh
                } else { $script:ExitMenuStableKey=$null; $script:ExitMenuStable=@(); $script:ExitMenuStableClock.Restart() }
            }
        } 'three identical popup crops/Exit coordinates spanning 300ms, then matching fresh owned crop'
        @($script:ExitMenuStable) | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath (Join-Path $outputRoot ('exit-menu-stable-' + $script:process.Id + '.json')) -Encoding UTF8
        $measuredPopup=New-Object Windows.Rect($target.popup.left,$target.popup.top,$target.popup.width,$target.popup.height)
        [NormalLaunchInput]::ClickOwnedMenuPoint($anchor, $script:process.Id, $target.x, $target.y, $measuredPopup, $target.captureTicks)
        return
    }
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
    # Shell navigation temporarily disables the confirmation button. Wait for
    # its actual enabled state; a fixed delay races slower/emulated desktops.
    $select = Wait-For { [NormalLaunchInput]::Find($dialog, '^Select Folder$') } 'enabled native Select Folder button'
    [NormalLaunchInput]::Click($select)
    Click-Control '^writing-normal-launch$'
    Wait-For { Find-Control '^normal-launch-sample\.md$' } 'sample file visible after Windows folder navigation' | Out-Null
    Passed $stage 'Organizer button opened native folder picker; selected location exposed sample in Library.'

    $stage = 'file-menu-and-native-picker'
    Open-Menu 'File' ('^Open File' + [char]0x2026 + '$')
    $dialog = Native-Picker 'Open'
    $openPickerHandle = [IntPtr]$dialog.Current.NativeWindowHandle
    [NormalLaunchInput]::RequireInputForeground()
    [NormalLaunchInput]::Chord(0x12, 0x4E) # Alt+N: native File name
    Type-NativeFilePath $openPickerHandle $sample
    [NormalLaunchInput]::Press(0x0D)
    Wait-For { [NormalLaunchInput]::OpenedDocument($script:main, $openPickerHandle, 'normal-launch-sample.md - Fomawrite') } 'opened document title' | Out-Null
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
    $caught = $_
    try {
        $caught.Exception.ToString() | Set-Content -LiteralPath (Join-Path $outputRoot 'failure-exception.txt') -Encoding UTF8
        $chain = New-Object 'System.Collections.Generic.List[object]'
        $exception = $caught.Exception
        while ($null -ne $exception) {
            $chain.Add([ordered]@{ type = $exception.GetType().FullName;
                hresult = ('0x{0:X8}' -f [int]$exception.HResult); message = $exception.Message })
            $exception = $exception.InnerException
        }
        [ordered]@{ scriptStackTrace = $caught.ScriptStackTrace; exceptions = @($chain.ToArray()) } |
            ConvertTo-Json -Depth 6 | Set-Content -LiteralPath (Join-Path $outputRoot 'failure-context.json') -Encoding UTF8
    } catch { Write-Warning "Could not retain failure exception diagnostics: $_" }
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
