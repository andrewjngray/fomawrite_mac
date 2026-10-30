@echo off
setlocal DisableDelayedExpansion
set "FOMAWRITE_REPOSITORY_ROOT=%~dp0"
set "FOMAWRITE_DOCUMENT_PATH=%~1"
echo Preparing the current downloaded Fomawrite build. No sign-in or network connection is needed.
powershell.exe -NoProfile -Command "try { & ([ScriptBlock]::Create([IO.File]::ReadAllText((Join-Path $env:FOMAWRITE_REPOSITORY_ROOT 'bin\windows\launch-current.ps1')))) -RepositoryRoot $env:FOMAWRITE_REPOSITORY_ROOT -DocumentPath $env:FOMAWRITE_DOCUMENT_PATH } catch { Write-Host $_.Exception.Message -ForegroundColor Red; exit 1 }"
if errorlevel 1 (
  echo.
  pause
  exit /b 1
)
endlocal
