[CmdletBinding()]
param([ValidateSet('arm64', 'x64')][string]$Architecture = 'x64')
# Builds qpdf (vcpkg) and the Phase 0 PDF spike against the pinned Qt, then
# runs every spike. Results: build/pdf-spike/<arch>/*.json
. "$PSScriptRoot/../../bin/windows/common.ps1"
Assert-Windows
$QtRoot = Get-QtRoot $env:QT_ROOT_DIR
$out = Join-Path $RepoRoot "build/pdf-spike/$Architecture"
New-Item -ItemType Directory -Force -Path $out | Out-Null

# qpdf from vcpkg (Apache-2.0), dynamic, for this architecture.
$vcpkg = if ($env:VCPKG_INSTALLATION_ROOT -and (Test-Path (Join-Path $env:VCPKG_INSTALLATION_ROOT 'vcpkg.exe'))) { $env:VCPKG_INSTALLATION_ROOT } else {
    $clone = Join-Path $RepoRoot 'build/vcpkg'
    if (-not (Test-Path (Join-Path $clone 'vcpkg.exe'))) {
        Invoke-Checked git @('clone', '--depth', '1', 'https://github.com/microsoft/vcpkg.git', $clone)
        Invoke-Checked (Join-Path $clone 'bootstrap-vcpkg.bat') @('-disableMetrics')
    }
    $clone
}
$triplet = "$Architecture-windows"
$timer = [Diagnostics.Stopwatch]::StartNew()
Invoke-Checked (Join-Path $vcpkg 'vcpkg.exe') @('install', "qpdf:$triplet", '--disable-metrics')
$qpdfMinutes = [math]::Round($timer.Elapsed.TotalMinutes, 1)
$env:QPDF_ROOT = Join-Path $vcpkg "installed/$triplet"
$lib = Get-ChildItem (Join-Path $env:QPDF_ROOT 'lib') -Filter 'qpdf*.lib' | Select-Object -First 1
if (-not $lib) { throw "No qpdf import library in $env:QPDF_ROOT/lib" }
$env:QPDF_LIB = $lib.FullName
& (Join-Path $vcpkg 'vcpkg.exe') list qpdf | Out-File (Join-Path $out 'qpdf-vcpkg.txt')
"qpdf vcpkg build minutes: $qpdfMinutes; import library: $($lib.Name)" | Out-File -Append (Join-Path $out 'qpdf-vcpkg.txt')

Enter-MsvcEnvironment $Architecture
$env:PATH = (Join-Path $QtRoot 'bin') + ';' + (Join-Path $env:QPDF_ROOT 'bin') + ';' + $env:PATH
$build = Join-Path $RepoRoot "build/pdf-spike-build-$Architecture"
New-Item -ItemType Directory -Force -Path $build | Out-Null
Push-Location $build
try {
    Invoke-Checked (Join-Path $QtRoot 'bin/qmake.exe') @((Join-Path $PSScriptRoot 'pdfspike.pro'), 'CONFIG+=release', 'CONFIG-=debug_and_release')
    Invoke-Checked nmake @('/NOLOGO')
} finally { Pop-Location }
$exe = Get-ChildItem $build -Recurse -Filter 'pdfspike.exe' | Select-Object -First 1
if (-not $exe) { throw 'pdfspike.exe was not built' }

$fixtures = Join-Path $RepoRoot "build/pdf-spike-fixtures-$Architecture"
Invoke-Checked python @((Join-Path $PSScriptRoot 'make_fixtures.py'), $fixtures)
Copy-Item (Join-Path $PSScriptRoot 'fixtures/annots-pdfkit.pdf') $fixtures

$env:QT_QPA_PLATFORM = 'offscreen'
Invoke-Checked $exe.FullName @($fixtures, (Join-Path $out 'results-offscreen.json'))
foreach ($platform in 'offscreen', 'windows') {
    $env:QT_QPA_PLATFORM = $platform
    foreach ($variant in 'pdfPageImage_1800px_cacheOff', 'provider_1800px_cacheOff', 'provider_reuseItems', 'paintedPageItem') {
        $env:SPIKE_SCROLL = $variant
        Invoke-Checked $exe.FullName @($fixtures, (Join-Path $out "scroll-$platform-$variant.json"))
    }
}
Remove-Item Env:SPIKE_SCROLL
Write-Host "Spike results in $out"
