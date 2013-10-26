@echo off
"%~dp0\..\pythonportable\python.exe" -B "%~dp0\..\scripts\gendiff.py" %*
echo.
pause
