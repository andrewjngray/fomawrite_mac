@echo off
rem Compatibility alias: use the same current build as Launch.cmd.
call "%~dp0Launch.cmd" %*
exit /b %errorlevel%
