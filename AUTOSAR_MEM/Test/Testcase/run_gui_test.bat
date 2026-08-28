@echo off
chcp 65001 >nul
title AUTOSAR MEM GUI and Generator Test Suite
cls

echo ==================================================================================
echo   DANG CHAY BO TEST TU DONG GUI VA CODE GENERATOR...
echo ==================================================================================

set "PY_EXE=%LOCALAPPDATA%\Programs\Python\Python310\python.exe"

if exist "%PY_EXE%" (
    "%PY_EXE%" -u "%~dp0test_gui_generator.py"
) else (
    python -u "%~dp0test_gui_generator.py"
)

echo.
echo ==================================================================================
echo   HOAN TAT KIEM THU. Bam phim bat ky de thoat...
echo ==================================================================================
pause >nul
