@echo off
setlocal DisableDelayedExpansion
set "FOMAWRITE_TEST_LAUNCHER=%~dp0dist\windows-testing\TEST-B056-ARM64.cmd"
if not exist "%FOMAWRITE_TEST_LAUNCHER%" (
    echo Fomawrite ARM64 test launcher is missing:
    echo "%FOMAWRITE_TEST_LAUNCHER%"
    echo Restore the b056 internal test package in dist\windows-testing.
    pause
    exit /b 1
)
call "%FOMAWRITE_TEST_LAUNCHER%"
exit /b %errorlevel%
