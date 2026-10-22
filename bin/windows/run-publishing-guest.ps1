# Ordinary startup and physical Windows input only. See docs/windows/PUBLISHING-ACCEPTANCE.md.
# Run in a fresh hidden PowerShell 5.1 process on an unlocked English desktop.
[CmdletBinding()]
param([Parameter(Mandatory = $true)][string]$Package,
      [Parameter(Mandatory = $true)][string]$Output,
      [Parameter(Mandatory = $true)][string]$HelperDirectory,
      [string]$DriverPath = $PSCommandPath,
      [switch]$PrepareOnly)
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
if ($env:OS -ne 'Windows_NT') { throw 'This driver requires an unlocked Windows desktop.' }
function Resolve-Folder([string]$value) {
    $resolved = Resolve-Path -LiteralPath $value
    if ($resolved.Provider.Name -ne 'FileSystem' -or -not (Test-Path -LiteralPath $resolved.ProviderPath -PathType Container)) { throw "Not a filesystem directory: $value" }
    [IO.Path]::GetFullPath($resolved.ProviderPath).TrimEnd('\')
}
function Hash([string]$path) { (Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash.ToLowerInvariant() }
$packageRoot = Resolve-Folder $Package
$helperRoot = Resolve-Folder $HelperDirectory
$inputHelperPath = Join-Path $helperRoot 'normal-launch-input.cs'
if ([string]::IsNullOrWhiteSpace($DriverPath)) { throw 'ScriptBlock invocation requires the actual -DriverPath.' }
$driverResolved = Resolve-Path -LiteralPath $DriverPath
if ($driverResolved.Provider.Name -ne 'FileSystem') { throw 'DriverPath must be a filesystem file.' }
$driverPath = $driverResolved.ProviderPath
foreach ($required in @($driverPath, $inputHelperPath, (Join-Path $packageRoot 'manifest.json'))) {
    if (-not (Test-Path -LiteralPath $required -PathType Leaf)) { throw "Missing required file: $required" }
}
$exe = Join-Path $packageRoot 'Fomawrite.exe'
$manifestPath = Join-Path $packageRoot 'manifest.json'
$manifest = Get-Content -LiteralPath $manifestPath -Raw | ConvertFrom-Json
foreach ($entry in $manifest.files.PSObject.Properties) {
    $path = [IO.Path]::GetFullPath((Join-Path $packageRoot $entry.Name))
    if ([IO.Path]::IsPathRooted($entry.Name) -or -not $path.StartsWith($packageRoot + '\', [StringComparison]::OrdinalIgnoreCase)) { throw "Invalid manifest path: $($entry.Name)" }
    if ((Hash $path) -ne $entry.Value) { throw "Package hash mismatch: $($entry.Name)" }
}
if ((Hash $exe) -ne $manifest.executableSha256) { throw 'Executable differs from the package manifest.' }
$outputRoot = [IO.Path]::GetFullPath($Output)
if (Test-Path -LiteralPath $outputRoot) { throw 'Use a fresh output directory.' }
New-Item -ItemType Directory -Path $outputRoot | Out-Null
$profile = Join-Path $outputRoot 'profile'
$writing = Join-Path $outputRoot 'publishing-fixtures'
New-Item -ItemType Directory -Path $profile, $writing | Out-Null
$documents = @(
    [ordered]@{ id = 'A'; filename = 'publishing-A.md'; title = 'Amber orchard proof'; body = 'Amber foxes gather beside the orchard.' },
    [ordered]@{ id = 'B'; filename = 'publishing-B.md'; title = 'Cobalt harbor proof'; body = 'Cobalt boats rest beside the harbor.' }
)
foreach ($document in $documents) {
    $document['path'] = Join-Path $writing $document.filename
    $document['source'] = '# ' + $document.title + "`n`n" + $document.body + "`n"
    [IO.File]::WriteAllText($document.path, $document.source, (New-Object Text.UTF8Encoding($false)))
    $document['sha256'] = Hash $document.path
}
$plan = [ordered]@{
    schemaVersion = 1; gate = 'windows-publishing-supplement'; status = 'prepared'; passed = $false
    qualificationPending = $true; executable = $exe; executableSha256 = Hash $exe
    manifestSha256 = Hash $manifestPath; driverSha256 = Hash $driverPath; inputHelperSha256 = Hash $inputHelperPath
    profileRoot = $profile; outputRoot = $outputRoot; documents = $documents
    stages = @('ordinary-startup', 'library-A-split-web', 'A-pdf', 'library-B-pdf-web', 'style-switch-and-reload', 'export-html', 'export-pdf', 'library-A-restored-web-pdf', 'normal-exit')
}
$plan | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath (Join-Path $outputRoot 'publishing-plan.json') -Encoding UTF8
if ($PrepareOnly) { Write-Output 'Prepared only; no application was launched and no acceptance was established.'; return }
if ('NormalLaunchInput' -as [type]) { throw 'Use a fresh PowerShell process; an input helper is already loaded.' }
Add-Type -AssemblyName UIAutomationClient, UIAutomationTypes, WindowsBase, System.Drawing, System.Windows.Forms, System.Runtime.WindowsRuntime
Add-Type -Path $inputHelperPath -ReferencedAssemblies @([Windows.Automation.AutomationElement].Assembly.Location, [Windows.Automation.ControlType].Assembly.Location, [Windows.Rect].Assembly.Location)
[NormalLaunchInput]::Initialize()
$null = [Windows.Storage.StorageFile, Windows.Storage, ContentType=WindowsRuntime]
$null = [Windows.Storage.FileAccessMode, Windows.Storage, ContentType=WindowsRuntime]
$null = [Windows.Storage.Streams.IRandomAccessStream, Windows.Storage.Streams, ContentType=WindowsRuntime]
$null = [Windows.Storage.Streams.InMemoryRandomAccessStream, Windows.Storage.Streams, ContentType=WindowsRuntime]
$null = [Windows.Graphics.Imaging.BitmapDecoder, Windows.Graphics.Imaging, ContentType=WindowsRuntime]
$null = [Windows.Graphics.Imaging.SoftwareBitmap, Windows.Graphics.Imaging, ContentType=WindowsRuntime]
$null = [Windows.Media.Ocr.OcrEngine, Windows.Foundation, ContentType=WindowsRuntime]
$null = [Windows.Media.Ocr.OcrResult, Windows.Foundation, ContentType=WindowsRuntime]
$null = [Windows.Globalization.Language, Windows.Globalization, ContentType=WindowsRuntime]
$null = [Windows.Data.Pdf.PdfDocument, Windows.Data.Pdf, ContentType=WindowsRuntime]
$null = [Windows.Data.Pdf.PdfPageRenderOptions, Windows.Data.Pdf, ContentType=WindowsRuntime]
$checks = New-Object 'System.Collections.Generic.List[object]'
$observations = New-Object 'System.Collections.Generic.List[object]'
$saved = @{}
foreach ($entry in Get-ChildItem Env:) {
    if ($entry.Name -match '^(QT_|QTWEBENGINE|QML_|FOMAWRITE_)' -or $entry.Name -eq 'PATH') { $saved[$entry.Name] = $entry.Value }
}
$script:process = $null; $script:main = $null; $script:previewBounds = $null
$failure = $null; $stage = 'ordinary-startup'; $started = [DateTime]::UtcNow
function Wait-For([scriptblock]$condition, [string]$reason, [int]$seconds = 30) {
    $deadline = [DateTime]::UtcNow.AddSeconds($seconds)
    do {
        if ($script:process.HasExited) { throw "Application exited before $reason ($($script:process.ExitCode))." }
        $result = & $condition
        if ($result) { return $result }
        Start-Sleep -Milliseconds 150
    } while ([DateTime]::UtcNow -lt $deadline)
    throw "Timed out: $reason"
}
function Find-Control([string]$pattern) { [NormalLaunchInput]::Find($script:main, $pattern) }
function Click-Control([string]$pattern) {
    $element = Wait-For { Find-Control $pattern } "control $pattern"
    if ($element.Current.ProcessId -ne $script:process.Id) { throw 'UIA click target belongs to another process.' }
    [NormalLaunchInput]::Click($element)
}
function Require-MainForeground {
    [NormalLaunchInput]::RequireInputForeground()
    if ([NormalLaunchInput]::GetForegroundWindow() -ne [IntPtr]$script:main.Current.NativeWindowHandle) { throw 'Main application window is not foreground; refusing capture or input.' }
}
function Reveal-Footer {
    Require-MainForeground
    $bounds = $script:main.Current.BoundingRectangle
    [NormalLaunchInput]::MovePhysical([int]($bounds.Left + $bounds.Width / 2), [int]($bounds.Bottom - 8))
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
    $file=Await-WinRt ([Windows.Storage.StorageFile]::GetFileFromPathAsync($path)) ([Windows.Storage.StorageFile])
    $stream=Await-WinRt ($file.OpenAsync([Windows.Storage.FileAccessMode]::Read)) ([Windows.Storage.Streams.IRandomAccessStream])
    $software=$null
    try {
        $decoder=Await-WinRt ([Windows.Graphics.Imaging.BitmapDecoder]::CreateAsync($stream)) ([Windows.Graphics.Imaging.BitmapDecoder])
        $software=Await-WinRt ($decoder.GetSoftwareBitmapAsync()) ([Windows.Graphics.Imaging.SoftwareBitmap])
        $engine=[Windows.Media.Ocr.OcrEngine]::TryCreateFromLanguage((New-Object Windows.Globalization.Language('en-US')))
        if (-not $engine) { throw 'English Windows OCR is required for the clipped File menu.' }
        $result=Await-WinRt ($engine.RecognizeAsync($software)) ([Windows.Media.Ocr.OcrResult])
        $text=$result.Text
        $words=@($result.Lines | ForEach-Object { $_.Words | Where-Object { $_.Text -ceq 'Exit' } | ForEach-Object {
            [ordered]@{text=$_.Text;left=[double]$_.BoundingRect.X;top=[double]$_.BoundingRect.Y;width=[double]$_.BoundingRect.Width;height=[double]$_.BoundingRect.Height}
        } })
    } finally { if($software){$software.Dispose()}; $stream.Dispose() }
    $text | Set-Content -LiteralPath (Join-Path $outputRoot ($prefix + '-ocr.txt')) -Encoding UTF8
    if ($words.Count -eq 0) { return $null }
    if ($words.Count -ne 1) { throw 'Exit OCR match is not unique inside the owned File popup.' }
    $word=$words[0]; $x=[int][Math]::Round($left+$word.left+$word.width/2); $y=[int][Math]::Round($top+$word.top+$word.height/2)
    if (-not $popup.Contains($x,$y)) { throw 'Exit OCR target escaped File popup.' }
    $observation=[ordered]@{processId=$script:process.Id;popup=@{left=$left;top=$top;width=$width;height=$height};word=$word;x=$x;y=$y;contentSha256=Hash $path;contentFile=[IO.Path]::GetFileName($path);captureTicks=$capturedAt;uiaExitAbsent=$script:ExitMenuUiaAbsent;wheelDelta=-360;observedUtc=[DateTime]::UtcNow.ToString('o')}
    $observation | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath (Join-Path $outputRoot ($prefix + '-target.json')) -Encoding UTF8
    return $observation
}
function Open-Menu([string]$menu, [string]$item) {
    Click-Control ('^' + [regex]::Escape($menu) + '$')
    if ($menu -ceq 'File' -and $item -ceq '^Exit$') {
        # Exit can be below the viewport in a short window. Scroll the actual
        # owned File popup with one guarded physical wheel gesture, then click its exact item.
        $first = Wait-For { Find-Control '^New Document$' } 'visible owned File menu'
        $proof = [NormalLaunchInput]::RequireOwnedMenuPopup($first, $script:process.Id)
        $proof | Add-Content -LiteralPath (Join-Path $outputRoot 'exit-menu-scroll.txt') -Encoding UTF8
        [NormalLaunchInput]::ScrollOwnedMenuDown($first, $script:process.Id) | Add-Content -LiteralPath (Join-Path $outputRoot 'exit-menu-scroll.txt') -Encoding UTF8
        $exit = Find-Control '^Exit$'
        $script:ExitMenuUiaAbsent=($null -eq $exit)
        $uiaObservation=if($exit){[ordered]@{name=$exit.Current.Name;bounds=$exit.Current.BoundingRectangle.ToString();processId=$exit.Current.ProcessId}}else{$null}
        $uiaObservation | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $outputRoot ('exit-menu-uia-target-' + $script:process.Id + '.json')) -Encoding UTF8
        # Even exposed UIA bounds can describe an intermediate wheel frame.
        # Always measure the settled pixels before clicking the actual Exit.
        [NormalLaunchInput]::Describe($script:main) | Set-Content -LiteralPath (Join-Path $outputRoot ('exit-menu-uia-after-scroll-' + $script:process.Id + '.txt')) -Encoding UTF8
        $anchor = Wait-For { Find-Control ('^Page Setup' + [char]0x2026 + '$') } 'visible owned File row after wheel scroll'
        Capture-Diagnostic ('exit-menu-visible-' + $script:process.Id)
        $script:ExitMenuOcrAttempt=0
        $script:ExitMenuStableKey=$null; $script:ExitMenuStable=@(); $script:ExitMenuStableClock=[Diagnostics.Stopwatch]::StartNew()
        $target = Wait-For {
            $candidate=Read-ExitMenuTarget $anchor
            if (-not $candidate) { $script:ExitMenuStableKey=$null; $script:ExitMenuStable=@(); $script:ExitMenuStableClock=[Diagnostics.Stopwatch]::StartNew(); return $null }
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
function Picker([string]$title) {
    $dialog = Wait-For {
        $element = [NormalLaunchInput]::ForegroundRoot($script:process.Id)
        if ($element -and $element.Current.ClassName -eq '#32770' -and $element.Current.Name -match $title) { $element }
    } "native picker $title"
    [NormalLaunchInput]::Describe($dialog) | Set-Content -LiteralPath (Join-Path $outputRoot ($stage + '-native-picker-uia.txt')) -Encoding UTF8
    return $dialog
}
function Capture-Diagnostic([string]$name) {
    [NormalLaunchInput]::Describe($script:main) | Set-Content -LiteralPath (Join-Path $outputRoot ($name + '-uia.txt')) -Encoding UTF8
    $foreground = [NormalLaunchInput]::ForegroundRoot($script:process.Id)
    if (-not $foreground) { throw 'Owned application is not foreground; diagnostic capture refused.' }
    if ($foreground.Current.NativeWindowHandle -ne $script:main.Current.NativeWindowHandle) {
        [NormalLaunchInput]::Describe($foreground) | Set-Content -LiteralPath (Join-Path $outputRoot ($name + '-foreground-uia.txt')) -Encoding UTF8
        Save-Crop $foreground.Current.BoundingRectangle (Join-Path $outputRoot ($name + '-foreground-window.png')) -OwnedForegroundDialog
    } else { Save-Crop $script:main.Current.BoundingRectangle (Join-Path $outputRoot ($name + '-window.png')) }
}
function Save-Crop($bounds, [string]$path, [switch]$OwnedForegroundDialog) {
    [NormalLaunchInput]::RequireInputForeground()
    if (-not $OwnedForegroundDialog) { Require-MainForeground }
    $desktop = [Windows.Forms.SystemInformation]::VirtualScreen
    $left = [int][Math]::Ceiling([Math]::Max($bounds.Left, $desktop.Left))
    $top = [int][Math]::Ceiling([Math]::Max($bounds.Top, $desktop.Top))
    $right = [int][Math]::Floor([Math]::Min($bounds.Right, $desktop.Right))
    $bottom = [int][Math]::Floor([Math]::Min($bounds.Bottom, $desktop.Bottom))
    if ($right - $left -lt 100 -or $bottom - $top -lt 60) { throw 'Capture region is not visibly large enough.' }
    $bitmap = New-Object Drawing.Bitmap(($right - $left), ($bottom - $top))
    $graphics = [Drawing.Graphics]::FromImage($bitmap)
    try { [NormalLaunchInput]::RequireInputForeground(); if (-not $OwnedForegroundDialog) { Require-MainForeground }; $graphics.CopyFromScreen($left, $top, 0, 0, $bitmap.Size); $bitmap.Save($path, [Drawing.Imaging.ImageFormat]::Png) }
    finally { $graphics.Dispose(); $bitmap.Dispose() }
    return [ordered]@{ left = $left; top = $top; width = $right - $left; height = $bottom - $top }
}
function Await-WinRt($operation, [Type]$resultType) {
    $method = [System.WindowsRuntimeSystemExtensions].GetMethods() | Where-Object {
        $_.Name -eq 'AsTask' -and $_.IsGenericMethod -and $_.GetGenericArguments().Count -eq 1 -and $_.GetParameters().Count -eq 1 -and
        $_.GetParameters()[0].ParameterType.IsGenericType -and $_.GetParameters()[0].ParameterType.GetGenericTypeDefinition().FullName -eq 'Windows.Foundation.IAsyncOperation`1'
    } | Select-Object -First 1
    $task = $method.MakeGenericMethod($resultType).Invoke($null, @($operation))
    if (-not $task.Wait(10000)) { throw 'WinRT operation timed out.' }
    return $task.Result
}
function Await-WinRtAction($operation) {
    $method = [System.WindowsRuntimeSystemExtensions].GetMethods() | Where-Object {
        $_.Name -eq 'AsTask' -and -not $_.IsGenericMethod -and $_.GetParameters().Count -eq 1 -and $_.GetParameters()[0].ParameterType.FullName -eq 'Windows.Foundation.IAsyncAction'
    } | Select-Object -First 1
    $task = $method.Invoke($null, @($operation))
    if (-not $task.Wait(10000)) { throw 'WinRT action timed out.' }
}
function Ocr-Stream($stream) {
    $decoder = Await-WinRt ([Windows.Graphics.Imaging.BitmapDecoder]::CreateAsync($stream)) ([Windows.Graphics.Imaging.BitmapDecoder])
    $bitmap = Await-WinRt ($decoder.GetSoftwareBitmapAsync()) ([Windows.Graphics.Imaging.SoftwareBitmap])
    try {
        $engine = [Windows.Media.Ocr.OcrEngine]::TryCreateFromLanguage((New-Object Windows.Globalization.Language('en-US')))
        if (-not $engine) { throw 'English Windows OCR is required; a missing OCR engine is not a pass.' }
        $result = Await-WinRt ($engine.RecognizeAsync($bitmap)) ([Windows.Media.Ocr.OcrResult])
        return $result.Text
    } finally { $bitmap.Dispose() }
}
function Ocr-File([string]$path) {
    $file = Await-WinRt ([Windows.Storage.StorageFile]::GetFileFromPathAsync($path)) ([Windows.Storage.StorageFile])
    $stream = Await-WinRt ($file.OpenAsync([Windows.Storage.FileAccessMode]::Read)) ([Windows.Storage.Streams.IRandomAccessStream])
    try { Ocr-Stream $stream } finally { $stream.Dispose() }
}
function Has-Document([string]$text, $document, $other) {
    $normalized = [regex]::Replace($text, '\s+', ' ')
    return $normalized.IndexOf($document.title, [StringComparison]::OrdinalIgnoreCase) -ge 0 -and
        $normalized.IndexOf($document.body.TrimEnd('.'), [StringComparison]::OrdinalIgnoreCase) -ge 0 -and
        $normalized.IndexOf($other.title, [StringComparison]::OrdinalIgnoreCase) -lt 0 -and
        $normalized.IndexOf($other.body.TrimEnd('.'), [StringComparison]::OrdinalIgnoreCase) -lt 0
}
function Assert-Source($document) {
    $editor = Wait-For { Find-Control '^Markdown editor$' } 'visible Source editor'
    Wait-For { [NormalLaunchInput]::Text($editor).Replace("`r`n", "`n") -eq $document.source } "exact $($document.id) source" | Out-Null
    return $editor
}
function Select-LibraryDocument($document) {
    $source = Wait-For { Find-Control '^Markdown editor$' } 'Source before library click'
    $file = Wait-For { Find-Control ('^' + [regex]::Escape($document.filename) + '$') } "Library row $($document.filename)"
    if ($file.Current.BoundingRectangle.Right -gt $source.Current.BoundingRectangle.Left + 4) { throw 'Filename target is not inside Library to the left of Source.' }
    [NormalLaunchInput]::Click($file)
    Assert-Source $document | Out-Null
}
function Observe-FormatButtons([string]$name) {
    $states = @()
    foreach ($formatName in @('Web', 'PDF')) {
        $button = Find-Control ('^' + $formatName + ' ' + [char]0x2014)
        $patterns = @($button.GetSupportedPatterns() | ForEach-Object { $_.ProgrammaticName })
        $state = [ordered]@{ format = $formatName; name = $button.Current.Name; patterns = $patterns; checked = $null; checkedError = $null; value = $null; legacyState = $null }
        try { $state.checked = [NormalLaunchInput]::Checked($button) } catch { $state.checkedError = $_.Exception.Message }
        try { $state.value = [NormalLaunchInput]::Text($button) } catch { }
        $states += $state
    }
    $states | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $outputRoot ($name + '-format-state.json')) -Encoding UTF8
    return $states
}
function Publishing-Bounds($document) {
    $source = Assert-Source $document
    $sourceBounds = $source.Current.BoundingRectangle
    $header = Wait-For {
        $candidate = Find-Control ('^' + [regex]::Escape($document.filename) + '$')
        if ($candidate -and $candidate.Current.ControlType -eq [Windows.Automation.ControlType]::Text -and $candidate.Current.BoundingRectangle.Left -gt $sourceBounds.Right) { $candidate }
    } 'publishing document header to the right of Source'
    $style = Wait-For { Find-Control '^Choose output style:' } 'publishing style footer'
    $close = Wait-For { Find-Control '^Hide preview$' } 'publishing header close control'
    $zoom = Wait-For { Find-Control '^Decrease preview zoom$' } 'publishing header zoom control'
    $headerBounds = $header.Current.BoundingRectangle; $styleBounds = $style.Current.BoundingRectangle
    $closeBounds = $close.Current.BoundingRectangle; $zoomBounds = $zoom.Current.BoundingRectangle
    $left = [Math]::Max($headerBounds.Left, $styleBounds.Left)
    $top = [Math]::Max($headerBounds.Bottom, $zoomBounds.Bottom)
    $right = $closeBounds.Right; $bottom = $styleBounds.Top
    if ([Math]::Abs($headerBounds.Left - $styleBounds.Left) -gt 4 -or $left -le $sourceBounds.Right -or $right -le $left -or $bottom -le $top) { throw 'Publishing header/footer bounds do not establish a distinct output region.' }
    $controls = [ordered]@{ method = 'UIA-publishing-header-footer'; header = $headerBounds.ToString(); styleFooter = $styleBounds.ToString(); close = $closeBounds.ToString(); previewZoom = $zoomBounds.ToString(); source = $sourceBounds.ToString() }
    $controls | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $outputRoot ($stage + '-crop-controls.json')) -Encoding UTF8
    return New-Object Windows.Rect($left, $top, ($right - $left), ($bottom - $top))
}
function Assert-Publishing($document, $other, [string]$format, [string]$name) {
    Reveal-Footer
    if ($format -eq 'web') {
        Click-Control ('^Web ' + [char]0x2014 + ' read-only HTML publishing output$')
        $script:previewBounds = Publishing-Bounds $document
    } else {
        Click-Control ('^PDF ' + [char]0x2014 + ' read-only paginated publishing output$')
        if (-not $script:previewBounds) { throw 'Web publishing bounds must be established before PDF capture.' }
    }
    $formatStates = @(Observe-FormatButtons $name)
    # Qt exposes neither publishing surfaces nor Toggle/SelectionItem here.
    # Retain the actual absence; final format qualification is manual review.
    $pageAvailable = $null -ne (Find-Control '^Publishing PDF page 1$')
    $source = Assert-Source $document
    $sourceBounds = $source.Current.BoundingRectangle
    $windowBounds = $script:main.Current.BoundingRectangle
    $bounds = $script:previewBounds
    if ($bounds.Left -lt $sourceBounds.Right - 2 -or $bounds.Right -gt $windowBounds.Right + 2 -or $bounds.Top -lt $windowBounds.Top -or $bounds.Bottom -gt $windowBounds.Bottom) { throw 'Publishing OCR region overlaps Source or escapes the application window.' }
    $path = Join-Path $outputRoot ($name + '-publishing.png')
    $seen = Wait-For {
        $region = Save-Crop $bounds $path
        $text = Ocr-File $path
        $text | Set-Content -LiteralPath (Join-Path $outputRoot ($name + '-ocr.txt')) -Encoding UTF8
        if (Has-Document $text $document $other) { [ordered]@{ document = $document.id; format = $format; text = $text; crop = $region; sourceBounds = [ordered]@{ left = $sourceBounds.Left; right = $sourceBounds.Right; top = $sourceBounds.Top; bottom = $sourceBounds.Bottom }; image = [IO.Path]::GetFileName($path); imageSha256 = Hash $path; observedUtc = [DateTime]::UtcNow.ToString('o') } }
    } "$format publishing crop renders $($document.id) title/body and excludes $($other.id)" 60
    $webButton = Find-Control ('^Web ' + [char]0x2014)
    $pdfButton = Find-Control ('^PDF ' + [char]0x2014)
    $webBounds = $webButton.Current.BoundingRectangle; $pdfBounds = $pdfButton.Current.BoundingRectangle
    $reviewBottom = [Math]::Max($webBounds.Bottom, $pdfBounds.Bottom) + 1
    if ($reviewBottom -gt $windowBounds.Bottom -or $webBounds.Left -lt $bounds.Left -or $pdfBounds.Right -gt $bounds.Right) { throw 'Format controls escape the publishing-only review image.' }
    $reviewBounds = New-Object Windows.Rect($bounds.Left, $bounds.Top, $bounds.Width, ($reviewBottom - $bounds.Top))
    $reviewPath = Join-Path $outputRoot ($name + '-format-review.png')
    $reviewRegion = Save-Crop $reviewBounds $reviewPath
    $seen['formatReview'] = [ordered]@{ required = $true; qualified = $false; image = [IO.Path]::GetFileName($reviewPath); imageSha256 = Hash $reviewPath; crop = $reviewRegion; webButtonBounds = $webBounds.ToString(); pdfButtonBounds = $pdfBounds.ToString(); uiaStates = $formatStates; pdfPageUiaAvailable = $pageAvailable }
    $observations.Add($seen)
}
function Passed([string]$detail) { $checks.Add([ordered]@{ stage = $stage; passed = $true; detail = $detail; observedUtc = [DateTime]::UtcNow.ToString('o') }); Capture-Diagnostic $stage | Out-Null }
function Native-FileNameField($dialog) {
    if ($dialog.Current.ClassName -ne '#32770' -or $dialog.Current.ProcessId -ne $script:process.Id) { throw 'Not the owned native file picker.' }
    $pending = New-Object 'System.Collections.Generic.Queue[Windows.Automation.AutomationElement]'
    $pending.Enqueue($dialog); $visited = 0
    $deadline = [DateTime]::UtcNow.AddSeconds(4)
    while ($pending.Count -and $visited -lt 250 -and [DateTime]::UtcNow -lt $deadline) {
        $element = $pending.Dequeue(); $visited++
        $current = $element.Current
        if ($current.ControlType -eq [Windows.Automation.ControlType]::Edit -and $current.Name -eq 'File name:' -and $current.IsEnabled -and -not $current.IsOffscreen) { return $element }
        if ($current.ControlType -eq [Windows.Automation.ControlType]::Tree -or $current.ControlType -eq [Windows.Automation.ControlType]::List) { continue }
        foreach ($child in $element.FindAll([Windows.Automation.TreeScope]::Children, [Windows.Automation.Condition]::TrueCondition)) { $pending.Enqueue($child) }
    }
    throw 'Native File name edit field was not found within the bounded non-tree search.'
}
function Export-File([string]$format, [string]$destination) {
    if (Test-Path -LiteralPath $destination) { throw 'Export destination must not exist.' }
    Open-Menu 'File' ('^Export ' + $format.ToUpperInvariant() + [char]0x2026 + '$')
    Click-Control ('^Save ' + $format.ToUpperInvariant() + [char]0x2026 + '$')
    $dialog = Picker '^Export Document$'
    if ([NormalLaunchInput]::GetForegroundWindow() -ne [IntPtr]$dialog.Current.NativeWindowHandle) { throw 'Native export picker lost foreground.' }
    $field = Native-FileNameField $dialog
    [NormalLaunchInput]::Click($field); [NormalLaunchInput]::Chord(0x11, 0x41)
    [NormalLaunchInput]::Type($destination)
    Wait-For { [NormalLaunchInput]::Text($field) -eq $destination } 'native export filename matches exact destination' | Out-Null
    [NormalLaunchInput]::Text($field) | Set-Content -LiteralPath (Join-Path $outputRoot ($stage + '-typed-destination.txt')) -Encoding UTF8
    if ([NormalLaunchInput]::GetForegroundWindow() -ne [IntPtr]$dialog.Current.NativeWindowHandle) { throw 'Native export picker changed during filename input.' }
    $save = Wait-For { [NormalLaunchInput]::Find($dialog, '^Save$') } 'enabled native Save button'
    [NormalLaunchInput]::Click($save)
    Wait-For {
        if (Test-Path -LiteralPath $destination -PathType Leaf) {
            $probe = $null
            try {
                $probe = [IO.File]::Open($destination, [IO.FileMode]::Open, [IO.FileAccess]::Read, [IO.FileShare]::None)
                if ($probe.Length -gt 200) { $true }
            } catch [IO.IOException] { } # Still being written, not accepted yet.
            finally { if ($probe) { $probe.Dispose() } }
        }
    } "closed nonempty exported $format file" 60 | Out-Null
    Wait-For { [NormalLaunchInput]::GetForegroundWindow() -eq [IntPtr]$script:main.Current.NativeWindowHandle } 'export picker closes to the application' | Out-Null
}
try {
    foreach ($name in $saved.Keys) { [Environment]::SetEnvironmentVariable($name, $null, 'Process') }
    $env:PATH = $packageRoot + ';' + (Join-Path $env:SystemRoot 'System32') + ';' + $env:SystemRoot
    $env:QT_FORCE_STDERR_LOGGING = '1'; $env:FOMAWRITE_PROFILE_ROOT = $profile
    $script:process = Start-Process -FilePath $exe -WorkingDirectory $packageRoot -PassThru -RedirectStandardOutput (Join-Path $outputRoot 'stdout.log') -RedirectStandardError (Join-Path $outputRoot 'stderr.log')
    $script:process.Handle | Out-Null
    $script:main = Wait-For { [NormalLaunchInput]::Root($script:process.Id) } 'ordinary application window' 40
    $receiptPath = Join-Path $profile 'profile.json'
    if (-not (Test-Path -LiteralPath $receiptPath)) { throw 'Missing isolated normal-startup profile receipt; no UI input sent.' }
    $receipt = Get-Content -LiteralPath $receiptPath -Raw | ConvertFrom-Json
    if ($receipt.normalStartup -ne $true -or [IO.Path]::GetFullPath($receipt.profileRoot.Replace('/', '\')).TrimEnd('\') -ne $profile.TrimEnd('\')) { throw 'Invalid normal-startup profile receipt.' }
    $digest = [Security.Cryptography.SHA256]::Create()
    try { $namespaceHash = [BitConverter]::ToString($digest.ComputeHash([Text.Encoding]::UTF8.GetBytes($receipt.profileRoot))).Replace('-', '').ToLowerInvariant().Substring(0, 20) } finally { $digest.Dispose() }
    $namespace = 'FomawriteAcceptance-' + $namespaceHash
    if ($receipt.applicationName -cne $namespace -or $receipt.identity -cnotmatch '^[0-9a-f]{20}$') { throw 'Invalid isolated application/IPC namespace.' }
    foreach ($key in @('settingsPath', 'dataPath', 'cachePath')) { if (($receipt.$key -split '[\\/]') -cnotcontains $namespace) { throw "Profile $key escapes the namespace." } }
    if ([IO.Path]::GetFullPath($receipt.executable.Replace('/', '\')) -ne $exe -or [IO.Path]::GetFullPath($receipt.workspacePath.Replace('/', '\')) -ne [IO.Path]::GetFullPath((Join-Path $receipt.dataPath ('workspace-' + $receipt.identity + '.json')))) { throw 'Receipt executable or workspace differs.' }
    [NormalLaunchInput]::Foreground([IntPtr]$script:main.Current.NativeWindowHandle)
    # Standard Windows system-menu maximize input provides enough room for Split.
    [NormalLaunchInput]::Chord(0x12, 0x20); [NormalLaunchInput]::Press(0x58)
    Passed 'Argument-free ordinary startup; manifest and native settings/workspace namespace verified.'

    $stage = 'library-A-split-web'
    if (-not (Find-Control '^Add library location$')) { Open-Menu 'View' '^Organizer$' }
    Click-Control '^Add library location$'
    $dialog = Picker '^Add writing location$'
    [NormalLaunchInput]::Chord(0x12, 0x44); [NormalLaunchInput]::Type($writing); [NormalLaunchInput]::Press(0x0D)
    $select = Wait-For { [NormalLaunchInput]::Find($dialog, '^Select Folder$') } 'enabled Select Folder'
    [NormalLaunchInput]::Click($select)
    Click-Control '^publishing-fixtures$'
    Open-Menu 'View' '^Source$'
    Select-LibraryDocument $documents[0]
    Reveal-Footer; Click-Control ('^Split ' + [char]0x2014 + ' show the chosen editor beside output Preview$')
    Assert-Publishing $documents[0] $documents[1] 'web' 'A-web'
    Passed 'Actual Library A click, Split and Web controls; publishing-only OCR matches A, not B.'

    $stage = 'A-pdf'
    Assert-Publishing $documents[0] $documents[1] 'pdf' 'A-pdf'
    Passed 'Actual PDF control; rendered PDF page in publishing-only crop contains A.'
    $stage = 'library-B-pdf-web'
    Select-LibraryDocument $documents[1]
    Assert-Publishing $documents[1] $documents[0] 'pdf' 'B-pdf'
    Assert-Publishing $documents[1] $documents[0] 'web' 'B-web'
    Passed 'Actual Library switch A to B replaces both PDF and Web output, excluding stale A.'

    $stage = 'style-switch-and-reload'
    Reveal-Footer; Click-Control '^Choose output style:'; Click-Control '^Modern \(Sans\)$'
    Wait-For { Find-Control '^Choose output style: Modern \(Sans\)$' } 'baseline output style readback' | Out-Null
    Assert-Publishing $documents[1] $documents[0] 'web' 'B-web-modern'
    Reveal-Footer; Click-Control '^Choose output style:'; Click-Control '^Classic \(Serif\)$'
    Wait-For { Find-Control '^Choose output style: Classic \(Serif\)$' } 'selected output style readback' | Out-Null
    Reveal-Footer; Click-Control '^Choose output style:'; Click-Control '^Reload Themes$'
    Open-Menu 'View' '^Reload Preview$'
    Assert-Publishing $documents[1] $documents[0] 'web' 'B-web-classic-reloaded'
    if ((Hash (Join-Path $outputRoot 'B-web-modern-publishing.png')) -eq (Hash (Join-Path $outputRoot 'B-web-classic-reloaded-publishing.png'))) { throw 'Switching distinct output styles did not change publishing pixels.' }
    Passed 'Physical output-style selection, Reload Themes and Reload Preview retained current B rendering.'

    $stage = 'export-html'
    $htmlPath = Join-Path $outputRoot 'B-classic.html'
    Export-File 'html' $htmlPath
    $html = [IO.File]::ReadAllText($htmlPath)
    $htmlText = [Net.WebUtility]::HtmlDecode([regex]::Replace($html, '<[^>]*>', ' '))
    if (-not (Has-Document $htmlText $documents[1] $documents[0]) -or $html -notmatch '(?is)font-family\s*:[^;{}]*Georgia') { throw 'HTML file does not contain B with the selected Classic font.' }
    Passed 'File Export HTML, export hub Save HTML and native destination picker produced B HTML with selected Georgia CSS.'

    $stage = 'export-pdf'
    $pdfPath = Join-Path $outputRoot 'B-classic.pdf'
    Export-File 'pdf' $pdfPath
    $pdfBytes = [IO.File]::ReadAllBytes($pdfPath)
    if ([Text.Encoding]::ASCII.GetString($pdfBytes, 0, 5) -ne '%PDF-' -or [Text.Encoding]::ASCII.GetString($pdfBytes).LastIndexOf('%%EOF') -lt 0) { throw 'Exported file is not a complete PDF.' }
    $file = Await-WinRt ([Windows.Storage.StorageFile]::GetFileFromPathAsync($pdfPath)) ([Windows.Storage.StorageFile])
    $pdf = Await-WinRt ([Windows.Data.Pdf.PdfDocument]::LoadFromFileAsync($file)) ([Windows.Data.Pdf.PdfDocument])
    if ($pdf.PageCount -ne 1) { throw 'Expected exactly one short fixture PDF page.' }
    $page = $pdf.GetPage(0); $stream = New-Object Windows.Storage.Streams.InMemoryRandomAccessStream
    try {
        $options = New-Object Windows.Data.Pdf.PdfPageRenderOptions; $options.DestinationWidth = 1600
        Await-WinRtAction ($page.RenderToStreamAsync($stream, $options)); $stream.Seek(0)
        $pdfText = Ocr-Stream $stream
        $pdfText | Set-Content -LiteralPath (Join-Path $outputRoot 'B-exported-pdf-ocr.txt') -Encoding UTF8
        if (-not (Has-Document $pdfText $documents[1] $documents[0])) { throw 'Windows rendering of the exported PDF does not contain B title/body or contains stale A.' }
    } finally { $stream.Dispose(); $page.Dispose() }
    Passed 'File Export PDF and native picker produced a complete one-page PDF; Windows rendered-file OCR proves B content.'

    $stage = 'library-A-restored-web-pdf'
    Select-LibraryDocument $documents[0]
    Assert-Publishing $documents[0] $documents[1] 'web' 'A-restored-web'
    Assert-Publishing $documents[0] $documents[1] 'pdf' 'A-restored-pdf'
    foreach ($document in $documents) { if ((Hash $document.path) -ne $document.sha256) { throw 'Publishing changed Markdown bytes.' } }
    Passed 'Physical Library switch B to A refreshes both output formats; both Markdown files remain byte-exact.'

    $stage = 'normal-exit'
    Open-Menu 'File' '^Exit$'
    if (-not $script:process.WaitForExit(15000)) { throw 'Ordinary File Exit did not terminate the isolated application.' }
    $script:process.WaitForExit()
    if ($script:process.ExitCode -ne 0) { throw "Application exited with $($script:process.ExitCode)." }
    $stderr = [IO.File]::ReadAllText((Join-Path $outputRoot 'stderr.log'))
    if ($stderr -match '(?i)Live editor page failed to load|fomawrite editor failed to start|The Live editor (?:could not load|stopped unexpectedly)|(?:renderer|render process|QtWebEngineProcess)[^\r\n]{0,160}(?:crash|terminated|failed|fatal)|(?:crash|terminated|failed|fatal)[^\r\n]{0,160}(?:renderer|QtWebEngineProcess)|RenderProcessGone|STATUS_ACCESS_VIOLATION|\bFATAL[:\]]|Uncaught (?:TypeError|Error)|web preview stopped unexpectedly') { throw 'Renderer/page-load failure in application stderr.' }
    $checks.Add([ordered]@{ stage = $stage; passed = $true; detail = 'File Exit terminated normally with zero status and no detected renderer failures.'; observedUtc = [DateTime]::UtcNow.ToString('o') })
} catch {
    $failure = $_.Exception.Message
    $checks.Add([ordered]@{ stage = $stage; passed = $false; detail = $failure; observedUtc = [DateTime]::UtcNow.ToString('o') })
    try { Capture-Diagnostic 'failure' | Out-Null } catch { Write-Warning "Failure capture unavailable: $_" }
} finally {
    if ($script:process -and -not $script:process.HasExited) { $script:process.Kill(); $script:process.WaitForExit() }
    foreach ($entry in Get-ChildItem Env:) { if ($entry.Name -match '^(QT_|QTWEBENGINE|QML_|FOMAWRITE_)' -or $entry.Name -eq 'PATH') { [Environment]::SetEnvironmentVariable($entry.Name, $null, 'Process') } }
    foreach ($name in $saved.Keys) { [Environment]::SetEnvironmentVariable($name, $saved[$name], 'Process') }
    $evidence = [ordered]@{}
    foreach ($file in Get-ChildItem -LiteralPath $outputRoot -File) { $evidence[$file.Name] = Hash $file.FullName }
    $report = [ordered]@{
        schemaVersion = 1; gate = 'windows-publishing-supplement'; passed = $false; workflowPassed = ($null -eq $failure); qualified = $false; qualificationPending = $true; manualFormatReview = 'required'
        stage = $stage; failure = $failure; executable = $exe; executableSha256 = Hash $exe; manifestSha256 = Hash $manifestPath
        driverSha256 = Hash $driverPath; inputHelperSha256 = Hash $inputHelperPath
        startedUtc = $started.ToString('o'); finishedUtc = [DateTime]::UtcNow.ToString('o')
        processId = if ($script:process) { $script:process.Id } else { $null }; arguments = @(); exitCode = if ($script:process -and $script:process.HasExited) { $script:process.ExitCode } else { $null }
        inputMethod = 'Win32-SendInput'; discoveryMethod = 'read-only-UIAutomation'; profileRoot = $profile; outputRoot = $outputRoot
        profileReceiptSha256 = if (Test-Path -LiteralPath (Join-Path $profile 'profile.json')) { Hash (Join-Path $profile 'profile.json') } else { $null }
        checks = @($checks.ToArray()); publishingObservations = @($observations.ToArray()); documents = $documents; evidenceSha256 = $evidence
    }
    $report | ConvertTo-Json -Depth 12 | Set-Content -LiteralPath (Join-Path $outputRoot 'publishing-supplement-report.json') -Encoding UTF8
}
if ($failure) { throw "Publishing supplement failed at ${stage}: $failure" }
Write-Output 'Publishing workflow completed; qualification remains PENDING root review of hashed Web/PDF publishing and selected-format images.'
