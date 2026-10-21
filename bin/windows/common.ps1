Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$script:RepoRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))

function Assert-Windows {
    if (-not $IsWindows) { throw 'Run this entry point in PowerShell 7 on Windows.' }
}

function Invoke-Checked {
    param([string]$Program, [string[]]$Arguments = @())
    & $Program @Arguments
    if ($LASTEXITCODE -ne 0) { throw "$Program exited with status $LASTEXITCODE" }
}

function Get-QtRoot {
    param([string]$Path)
    if (-not $Path) { $Path = $env:QT_ROOT_DIR }
    if (-not $Path) { throw 'Pass -QtRoot or set QT_ROOT_DIR to the pinned Qt kit.' }
    $resolved = (Resolve-Path -LiteralPath $Path).Path
    if (-not (Test-Path -LiteralPath (Join-Path $resolved 'bin/qmake.exe'))) {
        throw "qmake.exe missing in $resolved"
    }
    $version = & (Join-Path $resolved 'bin/qmake.exe') -query QT_VERSION
    if ($LASTEXITCODE -ne 0 -or $version.Trim() -ne '6.11.3') {
        throw "Expected Qt 6.11.3, found $version"
    }
    return $resolved
}

function Enter-MsvcEnvironment {
    param([ValidateSet('arm64', 'x64')][string]$Architecture)
    $vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio/Installer/vswhere.exe'
    if (-not (Test-Path -LiteralPath $vswhere)) { throw 'Visual Studio Installer/vswhere.exe missing.' }
    # ARM hosted images now have VS2026. Select the installed v143 (MSVC2022)
    # compiler explicitly; never silently take VS2026's default v145 toolset.
    $installs = & $vswhere -products '*' -all -format json | ConvertFrom-Json
    if ($LASTEXITCODE -ne 0) { throw 'vswhere failed.' }
    $candidates = @(foreach ($install in $installs) {
        $tools = Join-Path $install.installationPath 'VC/Tools/MSVC'
        if (Test-Path -LiteralPath $tools) {
            foreach ($tool in Get-ChildItem -LiteralPath $tools -Directory) {
                $version = [version]$tool.Name
                if ($version.Major -eq 14 -and $version.Minor -ge 30 -and $version.Minor -lt 50) {
                    [PSCustomObject]@{ Install = $install.installationPath; Version = $version }
                }
            }
        }
    })
    $selected = $candidates | Sort-Object Version -Descending | Select-Object -First 1
    if (-not $selected) { throw 'MSVC2022 v143 toolset missing; install it instead of using the default compiler.' }
    $hostArchitecture = if ([Runtime.InteropServices.RuntimeInformation]::OSArchitecture -eq 'Arm64') { 'arm64' } else { 'x64' }
    $devcmd = Join-Path $selected.Install 'Common7/Tools/VsDevCmd.bat'
    $toolVersion = '{0}.{1}' -f $selected.Version.Major, $selected.Version.Minor
    $command = '""{0}" -no_logo -arch={1} -host_arch={2} -vcvars_ver={3} && set"' -f $devcmd, $Architecture, $hostArchitecture, $toolVersion
    $environment = & $env:ComSpec /d /s /c $command
    if ($LASTEXITCODE -ne 0) { throw 'VsDevCmd failed.' }
    foreach ($line in $environment) {
        if ($line -match '^([^=]+)=(.*)$') { [Environment]::SetEnvironmentVariable($Matches[1], $Matches[2], 'Process') }
    }
    if ($env:VSCMD_ARG_TGT_ARCH -ne $Architecture) { throw 'Compiler target architecture mismatch.' }
    if (-not $env:VCToolsVersion.StartsWith($toolVersion + '.')) { throw 'VsDevCmd selected an unexpected toolset.' }
    Write-Host "MSVC2022 $($env:VCToolsVersion), host $hostArchitecture, target $Architecture"
}
