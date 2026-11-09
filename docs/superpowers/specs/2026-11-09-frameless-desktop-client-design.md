# 无边框独立桌面客户端规范 (Schedule & WorkLog - Frameless Native Client)

- **日期**：2026-11-09
- **状态**：待用户确认 (Pending Approval)
- **文档路径**：`docs/superpowers/specs/2026-11-09-frameless-desktop-client-design.md`
- **工程目录**：`D:/schedule-worklog-c`

---

## 1. 意图与初衷 (Intent & The Why)

用户明确指出：“其实我想要的是一个本地客户端”，“无边框独立桌面窗体”，“点击关闭时收起到右下角系统托盘”。
上一版本虽然实现了完整的 C 核心业务、SQLite 存储与 Apifox 前端，但运行模式是借助外部默认浏览器打开标签页（如 Chrome/Edge）。
本次设计的核心目标是将系统升级为**真正的现代化独立桌面应用客户端（Desktop Client App）**：
- 拥有属于自己的独立无边框桌面窗口，界面与 Apifox / 微信桌面端高度一致。
- 窗口原生支持顶部栏鼠标平滑拖拽移动、双击最大化/还原、最小化与关闭。
- 点击“关闭”按钮时不杀进程，而是平滑最小化收起到**Windows 右下角系统托盘（System Tray）**。
- 用户双击或单击托盘图标可秒级唤醒还原窗口；右键托盘弹出菜单可选择“打开工作台”或“彻底退出”。
- 核心继续保持纯 C/C++ 高性能底层与轻量 WebView2 宿主结合，无臃肿 Electron 依赖，启动毫秒级。

---

## 2. 目标与范围 (Goals, Non-Goals & Anti-Goals)

### 2.1 目标 (Goals)
1. **无边框独立原生窗口 (Frameless Native Window)**：
   - 创建 Win32 无边框原生窗口，带 Windows 10/11 亚克力/原生窗口阴影。
   - 顶部导航栏继承 Apifox 质感深色 Header，并集成自定义窗口控制三件套：最小化 `—`、最大化/还原 `□`、关闭 `✕`。
   - 顶部空白区域支持鼠标长按拖拽窗口；双击标题栏触发最大化/还原。
2. **系统托盘后台驻留与唤醒 (System Tray Integration)**：
   - 采用 Win32 `Shell_NotifyIcon` 创建独立托盘图标。
   - 点击顶部右上角 `✕` 或最小化时，窗口自动隐藏并常驻托盘。
   - 鼠标左键单击或双击托盘图标，秒级呼出窗口并置于前台。
   - 鼠标右键托盘图标呼出原生上下文菜单：「打开工作台」、「退出程序」。
3. **宿主与通讯 (WebView2 Bridge)**：
   - 宿主程序直接加载本地运行的嵌入式服务界面（`http://127.0.0.1:<port>/`）。
   - 前端通过 `window.chrome.webview.postMessage` 与 Win32 宿主高效通讯（处理窗口拖拽、最小化、最大化、关闭等指令）。
4. **编译与单机交付 (Packaging)**：
   - 将 `WebView2Loader.dll` 及 WebView2 SDK 纳管入工程。
   - 更新 `build.bat`，一键编译生成 `schedule_worklog.exe`。双击该 exe 直接拉起独立桌面窗口。

### 2.2 非目标 (Non-Goals)
- **全局快捷键**：用户已确认日常不需要全局热键唤醒，遵循极简设计原则不注册后台全局 Hook。
- **引入庞大 Electron/Qt 框架**：坚决不引入动辄 150MB+ 的 Node.js/Chromium/Electron 捆绑，保持轻量二进制。

### 2.3 反向目标 (Anti-Goals)
- **避免打开外部浏览器**：严禁再调用 `ShellExecute(..., "http://...")` 跳转到外部浏览器。
- **避免卡死或僵尸进程**：右键托盘点击“退出”时，严格终止 HTTP 线程、释放 SQLite 句柄并彻底安全退出。

---

## 3. 架构设计与技术实现 (Architecture & Implementation)

```
+-------------------------------------------------------------+
|        Win32 原生无边框桌面窗口 (Native HWND Frame)          |
|  +-------------------------------------------------------+  |
|  | Apifox 深色顶部栏: [项目标]  [成员切换]  [—] [□] [✕]  |  |
|  +-------------------------------------------------------+  |
|  |                                                       |  |
|  |     WebView2 Client Area (Apifox 风格工作台)          |  |
|  |     - 双栏流水 (待办池 / 今日已完成打卡)              |  |
|  |     - 周报自动化聚合导出 (Markdown 一键复制)          |  |
|  |                                                       |  |
|  +-------------------------------------------------------+  |
+-------------------------------------------------------------+
                              |
                     window.chrome.webview
                              |
                              v
+-------------------------------------------------------------+
|              C 宿主进程 (schedule_worklog.exe)              |
| - Win32 消息循环 (WndProc: 托盘事件、拖拽、大小变化)        |
| - 系统托盘模块 (Shell_NotifyIcon: 单双击唤醒 / 右键菜单)     |
| - 嵌入式 HTTP/JSON API 服务 (server.c)                      |
| - 业务引擎 & 原生 SQLite3 数据库 (db.c, item.c, project.c)   |
+-------------------------------------------------------------+
```

### 3.1 窗口与无边框拖拽细节
- Win32 窗口类设置 `WS_POPUP | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX`，并在 `WM_NCCALCSIZE` 中控制客户端区域占满整个窗体，消除传统白框/原生标题栏。
- 窗口拖拽由前端 `.top-nav` 触发 `drag` 消息，Win32 宿主调用 `ReleaseCapture()` + `SendMessage(hwnd, WM_NCLBUTTONDOWN, HTCAPTION, 0)` 实现原生且丝滑的窗口拖拽。

### 3.2 托盘事件与生命周期管理
- 注册自定义消息 `WM_TRAYICON = WM_USER + 1`。
- 点击窗体关闭按钮 `✕` 时，调用 `ShowWindow(hwnd, SW_HIDE)`，并显示系统气泡通知：“已最小化到系统托盘，双击图标可随时呼出”。
- 托盘右键菜单响应 `IDM_TRAY_OPEN` 与 `IDM_TRAY_EXIT`，点击退出时执行完整清理逻辑并退出。
- **防止托盘残留伪图标**：在窗体销毁 `WM_DESTROY` 及程序退出时，严格调用 `Shell_NotifyIcon(NIM_DELETE, &nid)` 移除托盘图标。

### 3.3 健壮性与生产级防御设计 (Production-Grade Safeguards)
1. **单实例锁保护 (Single-Instance Mutex)**：
   - 使用命名互斥体 `CreateMutex(NULL, TRUE, "Global\\ScheduleWorkLog_SingleInstance")`。
   - 若检测到实例已存在，自动通过广播或遍历查找已有窗口并发送恢复指令（`ShowWindow(hwnd, SW_RESTORE)` + `SetForegroundWindow(hwnd)`），当前重复进程立即安全退出，避免托盘驻留时用户重复点击 exe 导致端口冲突。
2. **高分屏高清适配 (Per-Monitor High-DPI Awareness)**：
   - 启动时调用 `SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2)`，保证在 125%/150%/200% 缩放下无边框窗体与字体绝对锐利清晰。
3. **WebView2 运行库缺失友好引导**：
   - 首次初始化时若系统缺少 WebView2 运行时，弹窗友好提示并提供官方下载指引，不发生默默崩溃。

---

## 4. 交付文件与变动范围

1. **新增与更新 C/C++ 模块**：
   - `src/gui.cpp`（或 `src/gui.c`）：Win32 原生无边框窗口创建、WebView2 挂载、消息循环与托盘管理。
   - `include/gui.h`：桌面窗口与托盘接口定义。
   - `src/main.c`：更新入口逻辑，后台启动 HTTP 服务的同构线程后，主线程启动原生 GUI 窗口与消息循环，不再打开系统默认浏览器。
2. **WebView2 SDK 依赖**：
   - `include/WebView2.h`, `include/WebView2EnvironmentOptions.h`
   - `lib/WebView2Loader.dll`
3. **前端控制条与样式适配**：
   - `web/index.html`：在顶部导航栏右侧增加 `—` (最小化)、`□` (最大化/还原)、`✕` (收起到托盘) 窗口按钮。
   - `web/css/apifox-theme.css`：优化窗口控制按钮悬停动画（红色关闭、深色最小化）与顶部栏可拖拽交互体验。
   - `web/js/app.js`：添加窗口控制事件绑定（与宿主 WebView2 双向 postMessage）。
4. **编译脚本**：
   - `build.bat`：编译 GUI 模块并自动将 `WebView2Loader.dll` 拷贝至输出目录。
