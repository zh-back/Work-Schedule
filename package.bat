@echo off

echo ===================================================
echo [PACKAGE] Packaging Schedule and WorkLog Setup...
echo ===================================================

cd /d "%~dp0"

call build.bat
if errorlevel 1 goto :error

set "ISCC_EXE=C:\Users\Administrator\AppData\Local\Programs\Inno Setup 6\ISCC.exe"
if not exist "%ISCC_EXE%" set "ISCC_EXE=C:\Program Files (x86)\Inno Setup 6\ISCC.exe"
if not exist "%ISCC_EXE%" set "ISCC_EXE=C:\Program Files\Inno Setup 6\ISCC.exe"

if not exist "%ISCC_EXE%" (
    echo [ERROR] Inno Setup compiler ISCC.exe not found.
    exit /b 1
)

if not exist dist mkdir dist

echo [PACKAGE] Running Inno Setup Compiler...
"%ISCC_EXE%" installer.iss
if errorlevel 1 goto :error

echo.
echo ===================================================
echo [SUCCESS] Windows Installer created successfully!
echo Path: dist\ScheduleWorklog-Setup-v1.0.0.exe
echo ===================================================
exit /b 0

:error
echo [ERROR] Packaging failed.
exit /b 1
