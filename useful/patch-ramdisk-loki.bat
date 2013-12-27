@echo off
"%~dp0\..\pythonportable\python.exe" -B "%~dp0\..\scripts\advancedpatch.py" --askramdisk --loki %*
echo.
pause
