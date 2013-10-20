@echo off
cd %~dp0
..\pythonportable\python.exe ..\scripts\replaceramdisk.py --list
echo.
pause
