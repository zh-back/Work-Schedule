@echo off
set ZIG=D:\tools\zig-windows-x86_64-0.13.0\zig.exe
if not exist "%ZIG%" (
    echo [ERROR] Zig compiler not found at %ZIG%
    exit /b 1
)

if "%1"=="test" (
    echo [BUILD] Compiling and running tests...
    "%ZIG%" cc -Iinclude -Itests tests/test_common.c src/common.c -ladvapi32 -o tests/test_common.exe
    if errorlevel 1 exit /b 1
    tests\test_common.exe
    exit /b %errorlevel%
)

echo [BUILD] Use 'build.bat test' to run tests.
