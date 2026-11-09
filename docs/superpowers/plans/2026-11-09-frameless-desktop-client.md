# Frameless Native Desktop Client Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Transform the Schedule & WorkLog application into an Apifox-like standalone frameless native desktop client window with top-bar dragging, window controls, and system tray background residency.

**Architecture:** A native Win32 frameless window (`WS_POPUP | WS_THICKFRAME`) hosts an embedded WebView2 controller displaying the local Apifox UI. The C backend runs the embedded HTTP API and SQLite engine in a background thread. Custom top-nav window controls and drag interactions communicate with the Win32 host via WebView2 WebMessages. A system tray icon (`Shell_NotifyIcon`) handles background hiding, one-click restoration, and context menu exit. A named mutex guarantees single-instance execution.

**Tech Stack:** C (C99/C11), C++ (for WebView2 COM callbacks via Zig C++ / Clang 18), Win32 API (`User32`, `Shell32`, `Gdi32`, `DwmApi`), WebView2 SDK (`WebView2.h`, `WebView2Loader.dll`).

**Spec:** `D:/schedule-worklog-c/docs/superpowers/specs/2026-11-09-frameless-desktop-client-design.md`

## Global Constraints
- Preserve all existing SQLite database schema, business rules, and unit tests (`test_common`, `test_db`, `test_project`, `test_item`, `test_summary`, `test_server`).
- The application must run completely offline without external browser dependencies.
- Single-instance protection via named mutex `Global\ScheduleWorkLog_SingleInstance`.
- No lingering ghost icons in the system tray on application termination.

---

### Task 1: WebView2 SDK and Dependency Staging

- [ ] **Step 1.1**: Copy `WebView2.h` and `WebView2EnvironmentOptions.h` from `D:\tools\webview2\build\native\include` into `D:\schedule-worklog-c\include\`.
- [ ] **Step 1.2**: Copy `WebView2Loader.dll` from `D:\tools\webview2\build\native\x64\` into `D:\schedule-worklog-c\lib\` and root `D:\schedule-worklog-c\`.
- [ ] **Step 1.3**: Write test `tests/test_wv_env.cpp` verifying `WebView2.h` compilation and `CreateCoreWebView2EnvironmentWithOptions` dynamic symbol resolution via `LoadLibraryA`.
- [ ] **Step 1.4**: Compile and execute `tests/test_wv_env.exe` to verify successful symbol loading.
- [ ] **Step 1.5**: Git commit staged SDK headers, library, and test harness (`feat(sdk): stage WebView2 SDK and loader verification`).

---

### Task 2: System Tray & Single-Instance Mutex Subsystem

- [ ] **Step 2.1**: Define tray and window interfaces in `include/tray.h`:
  - `int check_single_instance(void)`: returns 0 if first instance, -1 if another instance already exists (and restores the existing instance).
  - `void tray_init(HWND hwnd, HINSTANCE hInstance)`: registers `Shell_NotifyIcon(NIM_ADD, &nid)`.
  - `void tray_remove(void)`: calls `Shell_NotifyIcon(NIM_DELETE, &nid)`.
  - `void tray_show_menu(HWND hwnd)`: displays popup menu with "打开工作台" and "退出程序".
- [ ] **Step 2.2**: Implement `src/tray.c` implementing single-instance named mutex and Win32 tray notification icon management.
- [ ] **Step 2.3**: Write test `tests/test_tray.c` asserting mutex creation, second instance detection, and cleanup logic.
- [ ] **Step 2.4**: Run `test_tray.exe` and confirm PASS.
- [ ] **Step 2.5**: Git commit (`feat(tray): implement single-instance mutex and system tray management`).

---

### Task 3: Frameless Window & WebView2 Host Engine

- [ ] **Step 3.1**: Define GUI host interface in `include/gui.h`:
  - `int gui_window_create_and_run(int http_port)`: creates frameless HWND, attaches WebView2 controller pointing to `http://127.0.0.1:<port>/`, handles message loop.
  - Handles WebMessage actions: `"drag"`, `"minimize"`, `"maximize"`, `"close"`.
- [ ] **Step 3.2**: Implement `src/gui.cpp` implementing:
  - Frameless Win32 window class with `WS_POPUP | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX`.
  - `WM_NCCALCSIZE` to extend client area over the entire frame.
  - DWM drop shadow via `DwmExtendFrameIntoClientArea`.
  - WebView2 COM event handler receiving WebMessage (`ICoreWebView2WebMessageReceivedEventHandler`).
  - Handling `"drag"` via `ReleaseCapture()` + `SendMessage(hwnd, WM_NCLBUTTONDOWN, HTCAPTION, 0)`.
  - Handling `"close"` by hiding window (`ShowWindow(hwnd, SW_HIDE)`) to system tray.
  - Tray callback handling: Left-click/Double-click restores window (`SW_RESTORE` + `SetForegroundWindow`), right-click opens tray menu.
- [ ] **Step 3.3**: Write a headless/mock test `tests/test_gui_message.cpp` verifying message parsing and action routing.
- [ ] **Step 3.4**: Compile and run `tests/test_gui_message.exe` to verify PASS.
- [ ] **Step 3.5**: Git commit (`feat(gui): implement native frameless window and WebView2 host`).

---

### Task 4: Apifox Header Window Controls & JS Bridge

- [ ] **Step 4.1**: Update `web/index.html` to add window control buttons (`btn-win-min`, `btn-win-max`, `btn-win-close`) to `.top-nav`.
- [ ] **Step 4.2**: Update `web/css/apifox-theme.css`:
  - Style window control buttons (subtle hover state, red hover for close).
  - Style `.top-nav` as the window drag region (`user-select: none; cursor: default`).
- [ ] **Step 4.3**: Update `web/js/app.js`:
  - Add mousedown drag trigger on `.top-nav` for WebMessage `drag`.
  - Bind click on `btn-win-min` -> `window.chrome.webview.postMessage({action: "minimize"})`.
  - Bind click on `btn-win-max` -> `window.chrome.webview.postMessage({action: "maximize"})`.
  - Bind click on `btn-win-close` -> `window.chrome.webview.postMessage({action: "close"})`.
- [ ] **Step 4.4**: Verify front-end assets render cleanly.
- [ ] **Step 4.5**: Git commit (`feat(ui): add native window controls and WebView2 bridge in top nav`).

---

### Task 5: Application Launcher Integration & Packaging

- [ ] **Step 5.1**: Update `src/main.c`:
  - Check single instance: if another instance is running, notify it and exit.
  - Enable Per-Monitor High-DPI awareness (`SetProcessDpiAwarenessContext`).
  - Start embedded HTTP server in background thread (`server_start_background(0)`).
  - Launch GUI window (`gui_window_create_and_run(port)`), blocking until user clicks "退出程序" in tray.
  - On exit, stop HTTP server, close DB, remove tray icon.
- [ ] **Step 5.2**: Update `build.bat`:
  - Compile `schedule_worklog.exe` linking C files and `gui.cpp`, with `-ldwmapi -lole32 -lshell32 -luser32 -ladvapi32 -lws2_32`.
  - Ensure `WebView2Loader.dll` is present in the output folder.
  - Support `build.bat test` (running all 6 test suites) and `build.bat run`.
- [ ] **Step 5.3**: Run `build.bat test` to verify all test suites PASS.
- [ ] **Step 5.4**: Run `build.bat` to build the standalone binary `schedule_worklog.exe`.
- [ ] **Step 5.5**: Git commit (`feat(app): integrate native desktop client launcher and build pipeline`).
