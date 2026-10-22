# Inserted only in a disposable derivative after the standard Live render proof.
# The unchanged standard driver supplies isolation, process guards and UI discovery.
$stage='live-edit-workflow'
$liveOriginal=$expected
$liveSuffix='Live writing with Windows keys.'
$liveEdited=$liveOriginal+$liveSuffix
$liveCheckpoints=New-Object 'System.Collections.Generic.List[object]'
$liveObservation=[ordered]@{ schemaVersion=1; case=$script:LiveEditCase; requestedGapMs=$script:LiveEditGapMs;
    completed=$false; originalBase64=[Convert]::ToBase64String([Text.Encoding]::UTF8.GetBytes($liveOriginal));
    editedBase64=[Convert]::ToBase64String([Text.Encoding]::UTF8.GetBytes($liveEdited));
    target=$null; focus=$null; rapidTiming=$null; afterRapid=$null; failureState=$null; stableSamples=3; checkpoints=@(); failure=$null }
Copy-Item -LiteralPath (Join-Path $driverRoot 'driver-composition.json') -Destination (Join-Path $outputRoot 'driver-composition.json')
function Live-ExactBytes([string]$text) {
    return [Convert]::ToBase64String([IO.File]::ReadAllBytes($sample)) -ceq [Convert]::ToBase64String([Text.Encoding]::UTF8.GetBytes($text))
}
function Live-State {
    $title=[LiveEditInput]::Title($script:process.Id)
    if ($title -notmatch '^\*? ?normal-launch-sample\.md - Fomawrite$') { throw "Unexpected owned window title: $title" }
    return [ordered]@{ title=$title; dirty=$title.StartsWith('* '); observedUtc=[DateTime]::UtcNow.ToString('o');
        diskBase64=[Convert]::ToBase64String([IO.File]::ReadAllBytes($sample)) }
}
function Live-StableSaved([string]$text, [string]$reason) {
    $stability=New-Object 'System.Collections.Generic.List[object]'
    Wait-For {
        $state=Live-State
        if (-not $state.dirty -and (Live-ExactBytes $text)) { $stability.Add($state) } else { $stability.Clear() }
        if ($stability.Count -ge 3) { $state }
    } $reason | Out-Null
    $script:LiveStableSamples=@($stability.ToArray())
}
function Live-Checkpoint([string]$name, [string]$wanted) {
    $state=Live-State
    if ($state.dirty -or -not (Live-ExactBytes $wanted)) { throw "Unstable saved state at $name" }
    $snapshot=$name+'.md'
    Copy-Item -LiteralPath $sample -Destination (Join-Path $outputRoot $snapshot)
    $liveCheckpoints.Add([ordered]@{ name=$name; state=$state; stableStates=$script:LiveStableSamples; snapshot=$snapshot; sha256=Hash (Join-Path $outputRoot $snapshot) })
    Capture $name
}
function Live-Visible([bool]$withSuffix, [string]$reason) {
    Wait-For {
        $visible=Read-LiveRenderedText $sourceBounds
        $body=$visible.text -match '(?i)Saved\s+with\s+Windows\s+input'
        $suffixVisible=$visible.text -match '(?i)Live\s+writing\s+with\s+Windows\s+keys'
        if ($body -and $suffixVisible -eq $withSuffix) { $visible }
    } $reason 15
}
function Live-WordTarget {
    # Re-read the existing bounded writing crop; OCR only that screenshot.
    $visible=Read-LiveRenderedText $sourceBounds
    $file=Wait-WinRt ([Windows.Storage.StorageFile]::GetFileFromPathAsync((Join-Path $outputRoot 'live-editor-content.png'))) ([Windows.Storage.StorageFile])
    $stream=Wait-WinRt ($file.OpenAsync([Windows.Storage.FileAccessMode]::Read)) ([Windows.Storage.Streams.IRandomAccessStream])
    $bitmap=$null
    try {
        $decoder=Wait-WinRt ([Windows.Graphics.Imaging.BitmapDecoder]::CreateAsync($stream)) ([Windows.Graphics.Imaging.BitmapDecoder])
        $bitmap=Wait-WinRt ($decoder.GetSoftwareBitmapAsync()) ([Windows.Graphics.Imaging.SoftwareBitmap])
        $ocr=[Windows.Media.Ocr.OcrEngine]::TryCreateFromLanguage((New-Object Windows.Globalization.Language('en-US')))
        if (-not $ocr) { throw 'English Windows OCR is required for a precise Live text target.' }
        $result=Wait-WinRt ($ocr.RecognizeAsync($bitmap)) ([Windows.Media.Ocr.OcrResult])
        $words=@($result.Lines | ForEach-Object { $_.Words | ForEach-Object {
            $rect=$_.BoundingRect
            [ordered]@{ text=$_.Text; left=[double]$rect.X; top=[double]$rect.Y; width=[double]$rect.Width; height=[double]$rect.Height }
        } })
    } finally { if($bitmap){$bitmap.Dispose()}; $stream.Dispose() }
    $body=@($words | Where-Object { $_.text -ceq 'Saved' })
    if ($body.Count -ne 1) { throw 'Require exactly one rendered Saved body word in the exclusive writing pane.' }
    $word=$body[0]
    return [ordered]@{ word=$word; region=$visible;
        x=[int][Math]::Round($visible.left+$word.left+$word.width/2);
        y=[int][Math]::Round($visible.top+$word.top+$word.height/2) }
}
try {
    $target=Live-WordTarget
    $liveObservation.target=$target
    [LiveEditInput]::ClickWord($script:process.Id,$sourceBounds,$target.x,$target.y)
    $focus=[LiveEditInput]::ObserveFocus()
    $liveObservation.focus=$focus
    if ($focus.processId -ne $script:process.Id -or -not $focus.keyboardFocus -or $focus.offscreen -or $focus.controlType -ne 'ControlType.Edit') {
        throw 'Read-only UIA did not establish owned, focused Live text input.'
    }
    Capture 'live-text-focus'
    [NormalLaunchInput]::Chord(0x11,0x23)
    [NormalLaunchInput]::Type($liveSuffix)
    Live-Visible $true 'Actual typed suffix rendered in the Live writing pane' | Out-Null
    Capture 'live-text-entered'
    [NormalLaunchInput]::Chord(0x11,0x53)
    Live-StableSaved $liveEdited 'Live typed text saved as stable exact UTF-8 bytes with a clean title'
    Live-Checkpoint 'live-typed-saved' $liveEdited
    if ($script:LiveEditGapMs -lt 0) {
        [NormalLaunchInput]::Chord(0x11,0x5A)
        Live-Visible $false 'Settled baseline: Undo visibly completed before Save' | Out-Null
        [NormalLaunchInput]::Chord(0x11,0x53)
    } else {
        # Only the requested millisecond gap occurs between these shortcuts.
        # No OCR, UIA readback, document poll or completion wait is inserted.
        $liveObservation.rapidTiming=[LiveEditInput]::UndoThenSave($script:LiveEditGapMs)
        $liveObservation.afterRapid=Live-State
    }
    Live-StableSaved $liveOriginal 'Undo then Save persisted exact original bytes and cleared dirty state'
    Live-Visible $false 'Undo restored the rendered original body' | Out-Null
    Live-Checkpoint 'live-undone-saved' $liveOriginal
    [NormalLaunchInput]::Chord(0x11,0x59)
    Live-Visible $true 'Redo visibly restored the typed suffix before Save' | Out-Null
    [NormalLaunchInput]::Chord(0x11,0x53)
    Live-StableSaved $liveEdited 'Redo then Save persisted stable exact edited bytes and cleared dirty state'
    Live-Checkpoint 'live-redone-saved' $liveEdited
    $expected=$liveEdited
    $liveObservation.completed=$true
    Passed $stage 'Owned OCR-targeted Live input, exact stable save/undo/redo bytes and clean dirty state; rapid cases wait only their declared gap before Save.'
    $stage='view-buttons'
} catch {
    $liveObservation.failure=$_.Exception.Message
    try { $liveObservation.failureState=Live-State } catch { $liveObservation.failureState=[ordered]@{ observationError=$_.Exception.Message } }
    throw
} finally {
    $liveObservation.checkpoints=@($liveCheckpoints.ToArray())
    $liveObservation | ConvertTo-Json -Depth 12 | Set-Content -LiteralPath (Join-Path $outputRoot 'live-edit-observations.json') -Encoding UTF8
}
