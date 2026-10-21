[CmdletBinding()]
param([ValidateSet('arm64', 'x64')][string]$Architecture = 'arm64', [string]$QtRoot)
. "$PSScriptRoot/common.ps1"
Assert-Windows
$QtRoot = Get-QtRoot $QtRoot
Enter-MsvcEnvironment $Architecture
$env:PATH = (Join-Path $QtRoot 'bin') + ';' + $env:PATH
$build = Join-Path $RepoRoot "build-tests/windows-$Architecture"
New-Item -ItemType Directory -Force -Path $build | Out-Null
Push-Location $build
try {
    Invoke-Checked (Join-Path $QtRoot 'bin/qmake.exe') @((Join-Path $RepoRoot 'tests/tests.pro'), 'CONFIG+=release', 'CONFIG-=debug_and_release', 'CONFIG-=debug')
    Invoke-Checked nmake @('/NOLOGO')
    # The offscreen plugin intentionally has no system colour-scheme override.
    # Require the appearance transition check against the actual Windows QPA as
    # well as the full offscreen suite; neither failure suppresses the other.
    $env:QT_QPA_PLATFORM = 'windows'
    & (Join-Path $build 'tst_fomawrite.exe') windowsAppearanceTracksQtColorScheme -o 'native-appearance.xml,junitxml' -o 'native-appearance.log,txt'
    $appearanceStatus = $LASTEXITCODE
    $env:QT_QPA_PLATFORM = 'offscreen'
    & (Join-Path $build 'tst_fomawrite.exe') -o 'test.xml,junitxml' -o 'test.log,txt'
    $regressionStatus = $LASTEXITCODE
    if ($appearanceStatus -ne 0 -or $regressionStatus -ne 0) {
        throw "Qt tests failed: native appearance status $appearanceStatus, full offscreen suite status $regressionStatus"
    }
} finally { Pop-Location; Remove-Item Env:QT_QPA_PLATFORM -ErrorAction SilentlyContinue }
