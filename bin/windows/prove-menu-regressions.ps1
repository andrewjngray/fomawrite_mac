[CmdletBinding()]
param(
    [Parameter(Mandatory)][ValidatePattern('^[0-9a-fA-F]{40}$')][string]$BaselineRevision,
    [ValidateSet('arm64', 'x64')][string]$Architecture = 'arm64',
    [string]$QtRoot
)
. "$PSScriptRoot/common.ps1"
Assert-Windows
# The red process intentionally exits nonzero; inspect it explicitly.
$PSNativeCommandUseErrorActionPreference = $false
$QtRoot = Get-QtRoot $QtRoot
Enter-MsvcEnvironment $Architecture
$build = Join-Path $RepoRoot "build-tests/windows-$Architecture"
$executable = Join-Path $build 'tst_fomawrite.exe'
if (-not (Test-Path -LiteralPath $executable -PathType Leaf)) { throw "Native test executable missing: $executable" }
$proofRoot = Join-Path $build 'menu-regression-proof'
New-Item -ItemType Directory -Force -Path $proofRoot | Out-Null
foreach ($name in @('red.xml', 'red.log', 'green.xml', 'green.log', 'proof.json')) {
    Remove-Item -LiteralPath (Join-Path $proofRoot $name) -Force -ErrorAction SilentlyContinue
}
$menuPath = Join-Path $RepoRoot 'src/WindowsMenuBar.qml'
$testPath = Join-Path $RepoRoot 'tests/tst_fomawrite.cpp'
$expected = [ordered]@{
    focusMenuKeepsModesExclusiveAndTypewriterIndependent = 'focusMenu && sentence && paragraph && typewriter && review'
    customStyleCheckPersistsAndRefreshesWithoutEditing = 'styleMenu && custom'
    fillerStyleCheckTogglesIndependentlyWithoutEditing = 'fillers && custom'
    statisticsCountRenderedUnicodeTasksAndAuthorship = 'statsOnlyAction && defaultAction'
    chromePresentationModesPersistAndKeepCommandsReachable = 'titleBarMenu'
}
$testNames = @($expected.Keys)
$originalBytes = [IO.File]::ReadAllBytes($menuPath)
$originalHash = (Get-FileHash -LiteralPath $menuPath -Algorithm SHA256).Hash.ToLowerInvariant()
$report = [ordered]@{
    startedUtc = [DateTime]::UtcNow.ToString('o'); missingNodeAssertions = $expected
    passed = $false; architecture = $Architecture; baselineRevision = $BaselineRevision.ToLowerInvariant()
    executableSha256 = (Get-FileHash -LiteralPath $executable -Algorithm SHA256).Hash.ToLowerInvariant()
    testsSha256 = (Get-FileHash -LiteralPath $testPath -Algorithm SHA256).Hash.ToLowerInvariant()
    helpersSha256 = (Get-FileHash -LiteralPath (Join-Path $RepoRoot 'tests/platform-menu-helpers.inc') -Algorithm SHA256).Hash.ToLowerInvariant()
    commandsSha256 = (Get-FileHash -LiteralPath (Join-Path $RepoRoot 'src/WorkspaceCommands.qml') -Algorithm SHA256).Hash.ToLowerInvariant()
    mainSha256 = (Get-FileHash -LiteralPath (Join-Path $RepoRoot 'src/Main.qml') -Algorithm SHA256).Hash.ToLowerInvariant()
    originalMenuSha256 = $originalHash; baselineMenuSha256 = $null; restoredMenuSha256 = $null
    restored = $false; red = $null; green = $null; error = $null
}

# Read git's stdout as bytes. PowerShell pipelines/redirection can transcode
# native stdout or change newlines; neither is acceptable for a baseline blob.
function Read-BaselineMenu {
    $start = [Diagnostics.ProcessStartInfo]::new('git')
    $start.WorkingDirectory = $RepoRoot
    $start.UseShellExecute = $false
    $start.RedirectStandardOutput = $true
    $start.RedirectStandardError = $true
    foreach ($argument in @('cat-file', 'blob', "${BaselineRevision}:src/WindowsMenuBar.qml")) { $start.ArgumentList.Add($argument) }
    $process = [Diagnostics.Process]::new()
    $process.StartInfo = $start
    $buffer = [IO.MemoryStream]::new()
    try {
        if (-not $process.Start()) { throw 'Could not start git cat-file.' }
        $errorText = $process.StandardError.ReadToEndAsync()
        $process.StandardOutput.BaseStream.CopyTo($buffer)
        $process.WaitForExit()
        if ($process.ExitCode -ne 0) { throw "Baseline menu blob missing: $($errorText.GetAwaiter().GetResult())" }
        if ($buffer.Length -eq 0) { throw 'Baseline menu blob is empty.' }
        return ,$buffer.ToArray()
    } finally { $buffer.Dispose(); $process.Dispose() }
}

function Invoke-MenuProofPhase {
    param([ValidateSet('red', 'green')][string]$Phase)
    $xmlPath = Join-Path $proofRoot "$Phase.xml"
    $logPath = Join-Path $proofRoot "$Phase.log"
    Remove-Item -LiteralPath $xmlPath, $logPath -Force -ErrorAction SilentlyContinue
    & $executable @testNames -o "$xmlPath,junitxml" -o "$logPath,txt"
    $status = $LASTEXITCODE
    if (-not (Test-Path -LiteralPath $xmlPath) -or -not (Test-Path -LiteralPath $logPath)) { throw "$Phase did not produce both Qt logs." }
    [xml]$junit = [IO.File]::ReadAllText($xmlPath)
    $cases = @($junit.SelectNodes('//testcase'))
    $failures = @($junit.SelectNodes('//failure'))
    $skips = @($junit.SelectNodes('//skipped'))
    $errors = @($junit.SelectNodes('//error'))
    $report[$Phase] = [ordered]@{ exitCode = $status; cases = $cases.Count; failures = $failures.Count; skips = $skips.Count; errors = $errors.Count }
    if ($cases.Count -ne 7 -or $skips.Count -ne 0 -or $errors.Count -ne 0) { throw "$Phase must contain exactly five cases plus init/cleanup, with no skips/errors." }
    foreach ($name in @('initTestCase', 'cleanupTestCase') + $testNames) {
        $matching = @($cases | Where-Object { $_.GetAttribute('name') -eq $name })
        if ($matching.Count -ne 1) { throw "$Phase is missing or duplicates $name." }
        $caseFailures = @($matching[0].SelectNodes('failure'))
        $shouldFail = $Phase -eq 'red' -and $expected.Contains($name)
        if ($caseFailures.Count -ne [int]$shouldFail) { throw "$Phase has an unexpected result for $name." }
        if ($shouldFail) {
            $expression = $expected[$name]
            if (-not $caseFailures[0].GetAttribute('message').Contains($expression)) { throw "Red failed outside the missing menu assertion for $name." }
            # Require the current test source location as well as its expression;
            # an earlier setup, parse or platform-boundary failure is not red.
            $source = [IO.File]::ReadAllLines($testPath)
            $functionLine = [Array]::IndexOf($source, "    void $name() {")
            if ($functionLine -lt 0) { throw "Test function source missing: $name" }
            $assertionLine = $null
            for ($index = $functionLine + 1; $index -lt $source.Length; $index++) {
                if ($source[$index] -match '^    void ') { break }
                if ($source[$index].Trim() -eq "QVERIFY($expression);") { $assertionLine = $index + 1; break }
            }
            if ($null -eq $assertionLine) { throw "Missing-node assertion not found: $name" }
            $pattern = 'FAIL!\s+: FomawriteTest::' + [regex]::Escape($name) + '\(\)[^\r\n]*' + [regex]::Escape($expression) + '[^\r\n]*\r?\n(?:\s*Loc: \[)?[^\r\n]*[\\/]tests[\\/]tst_fomawrite\.cpp\(' + $assertionLine + '\)(?:\]|\s*: failure location)'
            if ([IO.File]::ReadAllText($logPath) -notmatch $pattern) { throw "Red log does not identify the expected missing-node source location for $name." }
        }
    }
    $expectedFailures = if ($Phase -eq 'red') { 5 } else { 0 }
    if ($status -ne $expectedFailures -or $failures.Count -ne $expectedFailures) { throw "$Phase has unexpected process status or failure count." }
}

$savedEnvironment = @{}
foreach ($name in @('PATH', 'QT_QPA_PLATFORM', 'QML_DISABLE_DISK_CACHE')) { $savedEnvironment[$name] = [Environment]::GetEnvironmentVariable($name, 'Process') }
Push-Location $build
try {
    $env:PATH = (Join-Path $QtRoot 'bin') + ';' + $env:PATH
    $env:QT_QPA_PLATFORM = 'offscreen'
    $env:QML_DISABLE_DISK_CACHE = '1'
    $commit = & git -C $RepoRoot rev-parse --verify "${BaselineRevision}^{commit}"
    if ($LASTEXITCODE -ne 0 -or $commit.Trim() -ne $BaselineRevision) { throw 'Baseline must resolve to the exact supplied commit.' }
    $baselineBytes = Read-BaselineMenu
    try {
        [IO.File]::WriteAllBytes($menuPath, $baselineBytes)
        $report.baselineMenuSha256 = (Get-FileHash -LiteralPath $menuPath -Algorithm SHA256).Hash.ToLowerInvariant()
        if ($report.baselineMenuSha256 -eq $originalHash) { throw 'Baseline and current menu are identical; no regression can be proved.' }
        Invoke-MenuProofPhase red
    } finally {
        [IO.File]::WriteAllBytes($menuPath, $originalBytes)
        $report.restoredMenuSha256 = (Get-FileHash -LiteralPath $menuPath -Algorithm SHA256).Hash.ToLowerInvariant()
        $report.restored = $report.restoredMenuSha256 -eq $originalHash
        if (-not $report.restored) { throw 'Original WindowsMenuBar.qml bytes were not restored.' }
    }
    Invoke-MenuProofPhase green
    $report.passed = $true
} catch {
    $report.error = $_.Exception.Message
    throw
} finally {
    Pop-Location
    foreach ($name in $savedEnvironment.Keys) { [Environment]::SetEnvironmentVariable($name, $savedEnvironment[$name], 'Process') }
    [IO.File]::WriteAllText((Join-Path $proofRoot 'proof.json'), ($report | ConvertTo-Json -Depth 8) + "`n", [Text.UTF8Encoding]::new($false))
}
