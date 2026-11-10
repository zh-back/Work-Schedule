@echo off
setlocal
cd /d "%~dp0"
set ZIG=D:\tools\zig-windows-x86_64-0.13.0\zig.exe
if not exist "%ZIG%" (
    echo [ERROR] Zig compiler not found at: %ZIG%
    exit /b 1
)

set CFLAGS=-Iinclude -Itests -O2 -ladvapi32 -lws2_32 -lshell32
set CXXFLAGS=-Iinclude -Itests -O2 -ldwmapi -lole32 -lshell32 -luser32 -lgdi32 -ladvapi32 -lws2_32
set ALL_SRC=src/common.c src/db.c src/project.c src/item.c src/summary.c src/server.c src/sqlite3.c src/tray.c src/gui.cpp

if not exist "WebView2Loader.dll" (
    if exist "lib\WebView2Loader.dll" (
        copy /y "lib\WebView2Loader.dll" "WebView2Loader.dll" >nul
    )
)

if "%1"=="clean" (
    echo [CLEAN] Cleaning test and temporary binaries...
    del /q /f tests\*.exe tests\*.pdb tests\*.lib 2>nul
    rmdir /s /q tests\diag_wv.exe.WebView2 2>nul
    del /q /f *.pdb *.lib check_*.* dump_*.* test_api_*.* test_item_api.* restore_*.* sync_*.* delete_*.* inspect_*.* schedule_worklog.db.bak 2>nul
    echo [CLEAN] Done.
    exit /b 0
)

if "%1"=="test" (
    echo ===================================================
    echo [TEST] Compiling and running unit tests...
    echo ===================================================
    "%ZIG%" cc -Iinclude -Itests tests/test_common.c src/common.c -ladvapi32 -o tests/test_common.exe
    if errorlevel 1 exit /b 1
    tests\test_common.exe
    if errorlevel 1 exit /b 1

    "%ZIG%" cc -Iinclude -Itests tests/test_db.c src/db.c src/common.c src/sqlite3.c -ladvapi32 -o tests/test_db.exe
    if errorlevel 1 exit /b 1
    tests\test_db.exe
    if errorlevel 1 exit /b 1

    "%ZIG%" cc -Iinclude -Itests tests/test_project.c src/project.c src/db.c src/common.c src/sqlite3.c -ladvapi32 -o tests/test_project.exe
    if errorlevel 1 exit /b 1
    tests\test_project.exe
    if errorlevel 1 exit /b 1

    "%ZIG%" cc -Iinclude -Itests tests/test_item.c src/item.c src/project.c src/db.c src/common.c src/sqlite3.c -ladvapi32 -o tests/test_item.exe
    if errorlevel 1 exit /b 1
    tests\test_item.exe
    if errorlevel 1 exit /b 1

    "%ZIG%" cc -Iinclude -Itests tests/test_summary.c src/summary.c src/item.c src/project.c src/db.c src/common.c src/sqlite3.c -ladvapi32 -o tests/test_summary.exe
    if errorlevel 1 exit /b 1
    tests\test_summary.exe
    if errorlevel 1 exit /b 1

    "%ZIG%" cc -Iinclude -Itests tests/test_server.c src/server.c src/summary.c src/item.c src/project.c src/db.c src/common.c src/sqlite3.c -ladvapi32 -lws2_32 -o tests/test_server.exe
    if errorlevel 1 exit /b 1
    tests\test_server.exe
    if errorlevel 1 exit /b 1

    "%ZIG%" cc -Iinclude -Itests tests/test_tray.c src/tray.c -lshell32 -luser32 -o tests/test_tray.exe
    if errorlevel 1 exit /b 1
    tests\test_tray.exe
    if errorlevel 1 exit /b 1

    "%ZIG%" c++ -Iinclude -Itests tests/test_wv_env.cpp -o tests/test_wv_env.exe
    if errorlevel 1 exit /b 1
    tests\test_wv_env.exe
    if errorlevel 1 exit /b 1

    "%ZIG%" c++ -Iinclude -Itests tests/test_gui_message.cpp src/gui.cpp src/tray.c -ldwmapi -lole32 -lshell32 -luser32 -lgdi32 -o tests/test_gui_message.exe
    if errorlevel 1 exit /b 1
    tests\test_gui_message.exe
    if errorlevel 1 exit /b 1

    echo ===================================================
    echo [PASS] All 9 test suites passed!
    echo ===================================================
    exit /b 0
)

echo [BUILD] Compiling Schedule and WorkLog desktop client...
"%ZIG%" c++ %CXXFLAGS% -Wl,/subsystem:windows src/main.c %ALL_SRC% -o schedule_worklog.exe
if errorlevel 1 (
    echo [ERROR] Build failed!
    exit /b 1
)

echo [SUCCESS] Binary created: schedule_worklog.exe
if "%1"=="run" (
    echo [RUN] Launching schedule_worklog.exe...
    schedule_worklog.exe
)
