# Ordinary physical Help setup; require actual unchecked state before scheduled update.
$script:DesktopSupplementUpdaterSetup=[ordered]@{completed=$false;initialChecked=$null;reopenedChecked=$null;offElapsedMs=$null;input='ordinary Help menu physical click';failure=$null}
try {
    $top=Wait-For {Find-Control '^Help$'} 'Help menu before scheduled updater' 5
    $handle=[IntPtr]$script:process.MainWindowHandle
    $null=[OwnedMenuNavigation]::HoverTop($top,$script:process.Id,$handle,'Help')
    $item=$null;$clock=[Diagnostics.Stopwatch]::StartNew()
    do {$item=Find-Control '^Check for Updates Automatically$';if($item){break};Start-Sleep -Milliseconds 100}while($clock.ElapsedMilliseconds-lt500)
    if(-not$item){[NormalLaunchInput]::Click($top);$item=Wait-For {Find-Control '^Check for Updates Automatically$'} 'automatic update menu item' 5}
    $null=[NormalLaunchInput]::RequireOwnedMenuPopup($item,$script:process.Id)
    $script:DesktopSupplementUpdaterSetup.initialChecked=[NormalLaunchInput]::Checked($item)
    if(-not$script:DesktopSupplementUpdaterSetup.initialChecked){throw 'Fresh isolated default automatic update item was not checked'}
    Capture 'desktop-automatic-before'
    [NormalLaunchInput]::Click($item)
    $top=Wait-For {Find-Control '^Help$'} 'reopened Help menu' 5
    $null=[OwnedMenuNavigation]::HoverTop($top,$script:process.Id,$handle,'Help')
    $item=$null;$clock=[Diagnostics.Stopwatch]::StartNew()
    do {$item=Find-Control '^Check for Updates Automatically$';if($item){break};Start-Sleep -Milliseconds 100}while($clock.ElapsedMilliseconds-lt500)
    if(-not$item){[NormalLaunchInput]::Click($top);$item=Wait-For {Find-Control '^Check for Updates Automatically$'} 'reopened automatic update item' 5}
    $null=[NormalLaunchInput]::RequireOwnedMenuPopup($item,$script:process.Id)
    $script:DesktopSupplementUpdaterSetup.reopenedChecked=[NormalLaunchInput]::Checked($item)
    if($script:DesktopSupplementUpdaterSetup.reopenedChecked){throw 'Automatic update remained checked after actual toggle'}
    $script:DesktopSupplementUpdaterSetup.offElapsedMs=([DateTime]::UtcNow-$script:process.StartTime.ToUniversalTime()).TotalMilliseconds
    if($script:DesktopSupplementUpdaterSetup.offElapsedMs-ge9000){throw 'Automatic update was not proved OFF before scheduled check race'}
    Capture 'desktop-automatic-off-reopened'
    [NormalLaunchInput]::Press(0x1b)
    $script:DesktopSupplementUpdaterSetup.completed=$true
} catch {$script:DesktopSupplementUpdaterSetup.failure=$_.Exception.ToString();throw}
finally {$script:DesktopSupplementUpdaterSetup|ConvertTo-Json -Depth 6|Set-Content (Join-Path $outputRoot 'desktop-supplement-updater-setup.json') -Encoding UTF8}

# DESKTOP-SUPPLEMENT-WORKFLOW
# Hooked after the maintained normal gate has saved/rendered its public file.
# No clipboard, UIA Invoke/SetFocus, app API, page script or print-accept input.
$stage='desktop-supplement-workflow'
$script:DesktopSupplementObservation=[ordered]@{schemaVersion=1;updaterSetup=$script:DesktopSupplementUpdaterSetup;completed=$false;qualified=$false;nativeVisualReviewPending=$true;clipboardTouched=$false;originalClipboardRestoration='unknown-r3-unresolved-by-this-run';checks=@();failure=$null;binding=$null}
$script:DesktopSupplementObservation['visualReviewRequired']=$true
$script:DesktopSupplementObservation['visualReviewCriteria']=@('Actual opened Review shows Harper 2.10.0 built-in and three public spelling cards','Same physical chord visibly closes Review without changing document or selection')
$script:CenterDiagnostic=$null
$desktopChecks=New-Object 'Collections.Generic.List[object]'
$desktopWindow=[IntPtr]$script:main.Current.NativeWindowHandle
$desktopOriginalHash=Hash $sample
$desktopReview=Join-Path $writing 'desktop-public-review.md'
$desktopText="<!-- Unicode witness: caf$([char]0xe9) $([char]::ConvertFromUtf32(0x1f600)) -->`n`nA flomptoid waits.`n`nWe recieve the mail.`n`nA quorbleflat waits.`n"
[IO.File]::WriteAllText($desktopReview,$desktopText,(New-Object Text.UTF8Encoding($false)))
$desktopReviewHash=Hash $desktopReview
function Desktop-Source([string]$wanted) {
    $source=Wait-For {Find-Control '^Markdown editor$'} 'owned visible Source editor'
    Wait-For {[NormalLaunchInput]::Text($source).Replace("`r`n","`n") -ceq $wanted} 'exact public Source text' | Out-Null
    return $source
}
function Desktop-Unchanged {
    if((Hash $sample)-cne$desktopOriginalHash-or(Hash $desktopReview)-cne$desktopReviewHash){throw 'Public document disk bytes changed'}
}
function Desktop-Record([string]$id,$detail) {
    Desktop-Unchanged;Capture ('desktop-'+$id)
    $desktopChecks.Add([ordered]@{id=$id;passed=$true;detail=$detail;capture='desktop-'+$id+'.png';observedUtc=[DateTime]::UtcNow.ToString('o')})
    $script:DesktopSupplementObservation.checks=@($desktopChecks.ToArray())
}
function Desktop-Chord([UInt16[]]$modifiers,[UInt16]$key,[bool]$punctuation=$false) {
    [DesktopSupplementInput]::Chord($desktopWindow,$script:process.Id,$modifiers,$key,$punctuation)
}
function Desktop-Open([string]$path,[string]$title) {
    [NormalLaunchInput]::Chord(0x11,0x4f)
    $picker=Native-Picker 'Open';$pickerHandle=[IntPtr]$picker.Current.NativeWindowHandle
    [NormalLaunchInput]::Chord(0x12,0x4e);Type-NativeFilePath $pickerHandle $path;[NormalLaunchInput]::Press(0x0d)
    Wait-For {[NormalLaunchInput]::OpenedDocument($script:main,$pickerHandle,$title)} 'native Open completed with exact title' | Out-Null
}
function Desktop-OpenByPath([string]$path,[string]$title) {
    Desktop-Chord @(0x11,0x10) 0x4f
    $field=Wait-For {$e=Find-Control '^File or folder path$';if($e){$c=$e.Current;if($c.ProcessId-eq$script:process.Id-and$c.ControlType-eq[Windows.Automation.ControlType]::Edit-and$c.IsEnabled-and-not$c.IsOffscreen-and$c.HasKeyboardFocus){$e}}} 'actual owned focused Open by Path Edit'
    [NormalLaunchInput]::Chord(0x11,0x41)
    [NormalLaunchInput]::Type($path)
    Wait-For {$e=Find-Control '^File or folder path$';if($e-and$e.Current.HasKeyboardFocus-and[NormalLaunchInput]::Text($e)-ceq$path){$e}} 'one typed exact full public path in Open by Path' 5 | Out-Null
    Capture 'desktop-review-open-path-complete'
    [NormalLaunchInput]::Press(0x0d)
    Wait-For {-not(Find-Control '^File or folder path$')-and[NormalLaunchInput]::OwnedWindowTitle($script:process.Id)-ceq$title} 'Open by Path completed with exact document title' | Out-Null
}
function Desktop-Menu([string]$top,[string]$first) {
    $entry=Wait-For {Find-Control ('^'+[regex]::Escape($top)+'$')} ('owned top menu '+$top)
    $null=[OwnedMenuNavigation]::HoverTop($entry,$script:process.Id,$desktopWindow,$top)
    $pattern='^'+[regex]::Escape($first)+'$';$opened=$false;$clock=[Diagnostics.Stopwatch]::StartNew()
    do {if(Find-Control $pattern){$opened=$true;break};Start-Sleep -Milliseconds 100}while($clock.ElapsedMilliseconds-lt500)
    if(-not$opened){[NormalLaunchInput]::Click($entry)}
    $item=Wait-For {Find-Control $pattern} ('actual owned '+$top+' popup')
    $null=[NormalLaunchInput]::RequireOwnedMenuPopup($item,$script:process.Id)
    return $item
}
try {
    Copy-Item -LiteralPath (Join-Path $driverRoot 'driver-composition.json') -Destination (Join-Path $outputRoot 'driver-composition.json')
    $script:DesktopSupplementObservation.binding=Get-Content (Join-Path $driverRoot 'driver-composition.json') -Raw | ConvertFrom-Json
    $receiptName=$script:DesktopSupplementObservation.binding.localStageReceipt
    if($receiptName-cne'local-stage-receipt.json'){throw 'Unexpected copied StageOnly receipt name'}
    $receiptSource=Join-Path $driverRoot $receiptName
    $receiptEvidence=Join-Path $outputRoot $receiptName
    Copy-Item -LiteralPath $receiptSource -Destination $receiptEvidence
    if((Hash $receiptEvidence)-cne$script:DesktopSupplementObservation.binding.localStageReceiptSha256){throw 'Copied StageOnly receipt binding differs'}
    Click-Control '^Edit Markdown source$';$source=Desktop-Source $expected
    Desktop-Chord @(0x11,0x10) 0x4f
    $null=Wait-For {Find-Control '^File or folder path$'} 'physical CtrlShiftO opened Open by Path input'
    Capture 'desktop-open-path-open';[NormalLaunchInput]::Press(0x1b)
    Wait-For {-not(Find-Control '^File or folder path$')} 'Open by Path canceled' | Out-Null
    $source=Desktop-Source $expected
    Desktop-Record 'open-path' ([ordered]@{physicalKeys='Ctrl+Shift+O';opened=$true;canceled=$true})

    Desktop-Chord @(0x11,0x10) 0x50
    $desktopPalette=Wait-For {$state=[DesktopSupplementInput]::Palette($desktopWindow,$script:process.Id);if($state.present-and$state.ready){$state}} 'physical CtrlShiftP opened actual owned focused Command Palette input'
    Capture 'desktop-palette-open';[NormalLaunchInput]::Press(0x1b)
    Wait-For {-not([DesktopSupplementInput]::Palette($desktopWindow,$script:process.Id)).present} 'Command Palette canceled' | Out-Null
    $source=Desktop-Source $expected
    Desktop-Record 'palette' ([ordered]@{physicalKeys='Ctrl+Shift+P';opened=$true;canceled=$true;ownedQuery=$desktopPalette})

    Desktop-OpenByPath $desktopReview 'desktop-public-review.md - Fomawrite';$source=Desktop-Source $desktopText
    [NormalLaunchInput]::Click($source);[NormalLaunchInput]::Chord(0x11,0x24)
    if(Find-Control '^Close Spelling and Grammar$'){throw 'Expected isolated default Review pane hidden before punctuation input'}
    Desktop-Chord @(0x11,0x10) 0xba $true
    # Separate manual-review supplement: the R6 UIA banner failure is preserved.
    # Exact banner/card visibility is judged from captures, never synthesized.
    $engineName='UNQUALIFIED: independent visual review required'
    $source=Desktop-Source $desktopText
    # Bounded settling observations only; these never assert a card count.
    Capture 'desktop-review-open-initial'
    Start-Sleep -Milliseconds 750
    Capture 'desktop-review-open-middle'
    Start-Sleep -Milliseconds 750
    $selection=[DesktopSupplementInput]::Selection($source,$script:process.Id)
    if($selection.text-cne''){throw 'CtrlShiftOEM1 navigated/selected an issue instead of toggling Review'}
    Capture 'desktop-review-open'
    Desktop-Chord @(0x11,0x10) 0xba $true
    $source=Desktop-Source $desktopText
    $closedSelection=[DesktopSupplementInput]::Selection($source,$script:process.Id)
    if($closedSelection.text-cne$selection.text){throw 'Review toggle changed the Source selection'}
    Desktop-Record 'review-toggle' ([ordered]@{physicalKeys='Ctrl+Shift+OEM_1';engine=$engineName;visibleCardCount=$null;engineObserved=$null;settleObservationMs=1500;selectedText=$selection.text;closedSelectedText=$closedSelection.text;openingChordSent=$true;closingChordSent=$true;visualReviewPending=$true;openCapture='desktop-review-open.png';closeCapture='desktop-review-toggle.png'})

    [NormalLaunchInput]::Click($source);[NormalLaunchInput]::Chord(0x11,0x24)
    Desktop-Chord @(0x11) 0xba $true
    $next=Wait-For {$s=[DesktopSupplementInput]::Selection($source,$script:process.Id);if($s.keyboardFocus-and$s.text-ceq'flomptoid'){$s}} 'CtrlOEM1 selected first actual misspelling'
    Desktop-Record 'next-issue' ([ordered]@{physicalKeys='Ctrl+OEM_1';selection=$next})
    [NormalLaunchInput]::Chord(0x11,0x24)
    Desktop-Chord @(0x11,0x12) 0xba $true
    $previous=Wait-For {$s=[DesktopSupplementInput]::Selection($source,$script:process.Id);if($s.keyboardFocus-and$s.text-ceq'quorbleflat'){$s}} 'CtrlAltOEM1 wrapped to last actual misspelling'
    Desktop-Record 'previous-issue' ([ordered]@{physicalKeys='Ctrl+Alt+OEM_1';selection=$previous})

    $script:CenterDiagnostic=[ordered]@{criterion='successful nonempty DWM visible frame center vs GetMonitorInfo.work <=3 physical pixels both axes';initial=$null;before=$null;afterSamples=@();assertionPassed=$false;printUnrun=$true;failure=$null}
    function Save-CenterDiagnostic {$script:CenterDiagnostic|ConvertTo-Json -Depth 10|Set-Content (Join-Path $outputRoot 'center-native-observations.json') -Encoding UTF8}
    $initialWindow=[DesktopSupplementInput]::Window($desktopWindow,$script:process.Id)
    $script:CenterDiagnostic.initial=$initialWindow;Save-CenterDiagnostic
    [DesktopSupplementInput]::OffsetByTitlebarDrag($desktopWindow,$script:process.Id)
    $before=Wait-For {$w=[DesktopSupplementInput]::Window($desktopWindow,$script:process.Id);if($w.left-ne$initialWindow.left-or$w.top-ne$initialWindow.top){$w}} 'real titlebar drag moved the owned ordinary window'
    $script:CenterDiagnostic.before=$before;Save-CenterDiagnostic
    if($before.minimized-or$before.maximized-or$before.monitor-ne$initialWindow.monitor-or$before.visibleFrameHresult-ne0-or$before.visibleWidth-le0-or$before.visibleHeight-le0){throw 'Center preparation left ordinary window or own monitor'}
    $center=Desktop-Menu 'Window' 'Center';[NormalLaunchInput]::Click($center)
    $script:CenterStableKey=$null;$script:CenterStableCount=0;$script:CenterStableSince=$null
    $after=Wait-For {
        $w=[DesktopSupplementInput]::Window($desktopWindow,$script:process.Id)
        $script:CenterDiagnostic.afterSamples+=,$w;Save-CenterDiagnostic
        if($w.visibleFrameHresult-ne0-or$w.visibleWidth-le0-or$w.visibleHeight-le0-or$w.workWidth-le0-or$w.workHeight-le0-or$w.width-le0-or$w.height-le0-or$w.dpi-le0){throw 'DWM visible frame, native size, DPI or monitor workarea unavailable'}
        if($w.processId-ne$script:process.Id-or$w.hwnd-ne$desktopWindow.ToInt64()-or$w.monitor-ne$before.monitor-or$w.minimized-or$w.maximized){throw 'Center observation lost owned ordinary window or monitor'}
        $key=@($w.left,$w.top,$w.width,$w.height,$w.visibleLeft,$w.visibleTop,$w.visibleWidth,$w.visibleHeight,$w.workLeft,$w.workTop,$w.workWidth,$w.workHeight,$w.monitor,$w.dpi)-join ':'
        $centered=([Math]::Abs(($w.visibleLeft+$w.visibleWidth/2.0)-($w.workLeft+$w.workWidth/2.0))-le3-and[Math]::Abs(($w.visibleTop+$w.visibleHeight/2.0)-($w.workTop+$w.workHeight/2.0))-le3)
        if(-not$centered){$script:CenterStableKey=$null;$script:CenterStableCount=0;$script:CenterStableSince=$null;return}
        if($key-ceq$script:CenterStableKey){$script:CenterStableCount++}else{$script:CenterStableKey=$key;$script:CenterStableCount=1;$script:CenterStableSince=[Diagnostics.Stopwatch]::GetTimestamp()}
        $elapsed=([Diagnostics.Stopwatch]::GetTimestamp()-$script:CenterStableSince)*1000.0/[Diagnostics.Stopwatch]::Frequency
        if($script:CenterStableCount-ge3-and$elapsed-ge300){$w}
    } 'three stable DWM visible-frame centers over at least 300ms' 20
    if($after.minimized-or$after.maximized-or$after.width-ne$before.width-or$after.height-ne$before.height-or$after.visibleWidth-ne$before.visibleWidth-or$after.visibleHeight-ne$before.visibleHeight-or$after.monitor-ne$before.monitor){throw 'Center changed size or monitor'}
    if($before.left-eq$after.left-and$before.top-eq$after.top){throw 'No movement by actual Center action'}
    $source=Desktop-Source $desktopText
    $afterSelection=[DesktopSupplementInput]::Selection($source,$script:process.Id)
    if($afterSelection.text-cne$previous.text){throw 'Center changed the Source selection'}
    Desktop-Record 'center' ([ordered]@{initial=$initialWindow;before=$before;after=$after;selection=$afterSelection;movementObserved=$true})

    $script:CenterDiagnostic['stableSamples']=$script:CenterStableCount
    $script:CenterDiagnostic['stableIntervalMs']=([Diagnostics.Stopwatch]::GetTimestamp()-$script:CenterStableSince)*1000.0/[Diagnostics.Stopwatch]::Frequency
    $script:CenterDiagnostic.assertionPassed=$true;Save-CenterDiagnostic
    Desktop-Open $sample 'normal-launch-sample.md - Fomawrite';$source=Desktop-Source $expected
    $sourceBounds=$source.Current.BoundingRectangle
    Click-Control ('^Live '+[char]0x2014+' edit with Markdown rendered in place$')
    Wait-For {-not(Find-Control '^Markdown editor$')} 'original sample returned to maintained Live hook' | Out-Null
    Wait-For {$r=Read-LiveRenderedText $sourceBounds;if($r.text-match'(?i)Saved\s+with\s+Windows\s+input\b'){$r}} 'original Live sample renders again' | Out-Null
    Desktop-Unchanged
    if($desktopChecks.Count-ne6){throw 'Require all six independent native workflows'}
    $script:DesktopSupplementObservation.completed=$true
    Passed $stage 'Six desktop OS-input workflows completed; independent capture/identity review still required. Clipboard untouched; prior unknown restoration unresolved.'
}catch{$script:DesktopSupplementObservation.failure=$_.Exception.Message;if($script:CenterDiagnostic){$script:CenterDiagnostic.failure=$_.Exception.Message;Save-CenterDiagnostic};throw}
finally{$script:DesktopSupplementObservation.checks=@($desktopChecks.ToArray());$script:DesktopSupplementObservation|ConvertTo-Json -Depth 12|Set-Content (Join-Path $outputRoot 'desktop-supplement-observations.json') -Encoding UTF8}
$stage='view-buttons' # Resume the unchanged maintained hook's stage identity.
