[CmdletBinding()]
param([ValidateSet('arm64', 'x64')][string]$Architecture = 'arm64')
# Builds qpdf (dynamic, Apache-2.0) with vcpkg at the commit pinned in
# windows/vcpkg-baseline.txt and prints QPDF_PREFIX. Under GitHub Actions it
# also exports QPDF_PREFIX so later steps (qmake, tests, packaging) find it.
# Built packages are kept in a vcpkg binary cache (build/vcpkg-cache, restored
# by actions/cache), so a later run with the same pin and toolchain only
# restores them.
. "$PSScriptRoot/common.ps1"
Assert-Windows
$commit = (Get-Content -LiteralPath (Join-Path $RepoRoot 'windows/vcpkg-baseline.txt') -TotalCount 1).Trim()
if ($commit -notmatch '^[0-9a-f]{40}$') { throw 'windows/vcpkg-baseline.txt must hold one full vcpkg commit SHA.' }
$vcpkg = Join-Path $RepoRoot "build/vcpkg-$($commit.Substring(0, 12))"
if (-not (Test-Path -LiteralPath (Join-Path $vcpkg 'vcpkg.exe'))) {
    New-Item -ItemType Directory -Force -Path $vcpkg | Out-Null
    Push-Location $vcpkg
    try {
        Invoke-Checked git @('init', '--quiet')
        Invoke-Checked git @('fetch', '--quiet', '--depth', '1', 'https://github.com/microsoft/vcpkg.git', $commit)
        Invoke-Checked git @('checkout', '--quiet', 'FETCH_HEAD')
    } finally { Pop-Location }
    Invoke-Checked (Join-Path $vcpkg 'bootstrap-vcpkg.bat') @('-disableMetrics')
}
$cache = Join-Path $RepoRoot 'build/vcpkg-cache'
New-Item -ItemType Directory -Force -Path $cache | Out-Null
$triplet = "$Architecture-windows"
$install = Join-Path $RepoRoot "build/vcpkg-installed-$Architecture"
Invoke-Checked (Join-Path $vcpkg 'vcpkg.exe') @('install', "qpdf:$triplet", '--disable-metrics',
    '--x-install-root', $install, '--binarysource', "clear;files,$cache,readwrite")
$prefix = Join-Path $install $triplet
foreach ($need in 'include/qpdf/QPDF.hh', 'lib/qpdf.lib') {
    if (-not (Test-Path -LiteralPath (Join-Path $prefix $need))) { throw "vcpkg qpdf prefix lacks $need" }
}
if (-not (Get-ChildItem -LiteralPath (Join-Path $prefix 'bin') -Filter 'qpdf*.dll')) { throw 'vcpkg qpdf prefix has no qpdf DLL' }
& (Join-Path $vcpkg 'vcpkg.exe') list --x-install-root $install
Write-Host "QPDF_PREFIX=$prefix (vcpkg $commit)"
if ($env:GITHUB_ENV) { "QPDF_PREFIX=$prefix" | Out-File -Append -Encoding utf8 $env:GITHUB_ENV }
