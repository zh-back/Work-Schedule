@echo off
setlocal
set ZIG=D:\tools\zig-windows-x86_64-0.13.0\zig.exe
if not exist "%ZIG%" (
    echo [ERROR] Zig compiler not found at: %ZIG%
    exit /b 1
)

set CFLAGS=-Iinclude -Itests -O2 -ladvapi32 -lws2_32 -lshell32
set SRC_FILES=src/common.c src/db.c src/project.c src/item.c src/summary.c src/server.c src/sqlite3.c

if "%1"=="test" (
    echo ===================================================
    echo [TEST] Compiling and running unit tests...
    echo ===================================================
    "%ZIG%" cc -Iinclude -Itests tests/test_common.c src/common.c -ladvapi32 -o tests/test_common.exe
    if errorlevel 1 exit /b 1
    tests\test_common.exe

    "%ZIG%" cc -Iinclude -Itests tests/test_project.c src/project.c src/db.c src/common.c src/sqlite3.c -ladvapi32 -o tests/test_project.exe
    if errorlevel 1 exit /b 1
    tests\test_project.exe

    "%ZIG%" cc -Iinclude -Itests tests/test_item.c src/item.c src/project.c src/db.c src/common.c src/sqlite3.c -ladvapi32 -o tests/test_item.exe
    if errorlevel 1 exit /b 1
    tests\test_item.exe

    "%ZIG%" cc -Iinclude -Itests tests/test_summary.c src/summary.c src/item.c src/project.c src/db.c src/common.c src/sqlite3.c -ladvapi32 -o tests/test_summary.exe
    if errorlevel 1 exit /b 1
    tests\test_summary.exe

    "%ZIG%" cc -Iinclude -Itests tests/test_server.c src/server.c src/summary.c src/item.c src/project.c src/db.c src/common.c src/sqlite3.c -ladvapi32 -lws2_32 -o tests/test_server.exe
    if errorlevel 1 exit /b 1
    tests\test_server.exe

    echo ===================================================
    echo [PASS] All 5 test suites passed!
    echo ===================================================
    exit /b 0
)

echo [BUILD] Compiling Schedule and WorkLog desktop application...
"%ZIG%" cc %CFLAGS% src/main.c %SRC_FILES% -o schedule_worklog.exe
if errorlevel 1 (
    echo [ERROR] Build failed!
    exit /b 1
)

echo [SUCCESS] Binary created: schedule_worklog.exe
if "%1"=="run" (
    echo [RUN] Launching schedule_worklog.exe...
    schedule_worklog.exe
)
