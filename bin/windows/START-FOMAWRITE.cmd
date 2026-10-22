@echo off
setlocal
echo Preparing Fomawrite. The first launch may take a few minutes while files are checked and copied.
set "FOMAWRITE_LAUNCHER_DIR=%~dp0"
powershell.exe -NoProfile -WindowStyle Hidden -Command "& ([ScriptBlock]::Create([IO.File]::ReadAllText((Join-Path $env:FOMAWRITE_LAUNCHER_DIR 'launch-local.ps1')))) -Package $env:FOMAWRITE_LAUNCHER_DIR"
if errorlevel 1 exit /b 1
endlocal
