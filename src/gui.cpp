#include "gui.h"
#include "tray.h"
#include "WebView2.h"
#include <dwmapi.h>
#include <windowsx.h>
#include <stdio.h>
#include <string.h>

static ICoreWebView2Controller *g_controller = NULL;
static ICoreWebView2 *g_webview = NULL;
static HWND g_hwnd = NULL;
static int g_port = 8080;

extern "C" int gui_handle_action(HWND hwnd, const char *action) {
    if (!action) return -1;

    if (strcmp(action, "drag") == 0) {
        if (hwnd) {
            ReleaseCapture();
            SendMessageW(hwnd, WM_NCLBUTTONDOWN, HTCAPTION, 0);
        }
        return 0;
    } else if (strcmp(action, "minimize") == 0) {
        if (hwnd) {
            ShowWindow(hwnd, SW_MINIMIZE);
        }
        return 0;
    } else if (strcmp(action, "maximize") == 0) {
        if (hwnd) {
            WINDOWPLACEMENT wp;
            memset(&wp, 0, sizeof(wp));
            wp.length = sizeof(wp);
            GetWindowPlacement(hwnd, &wp);
            ShowWindow(hwnd, wp.showCmd == SW_SHOWMAXIMIZED ? SW_RESTORE : SW_MAXIMIZE);
        }
        return 0;
    } else if (strcmp(action, "close") == 0) {
        if (hwnd) {
            ShowWindow(hwnd, SW_HIDE);
        }
        return 0;
    }

    return -1;
}

// Explicit COM Callback implementations for WebView2
class EnvCompletedHandler : public ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler {
    LONG m_refCount;
    HWND m_hwnd;
    int m_port;
public:
    EnvCompletedHandler(HWND hwnd, int port) : m_refCount(1), m_hwnd(hwnd), m_port(port) {}
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppv) override {
        if (riid == IID_IUnknown || riid == IID_ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler) {
            *ppv = static_cast<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler*>(this);
            AddRef();
            return S_OK;
        }
        *ppv = NULL;
        return E_NOINTERFACE;
    }
    ULONG STDMETHODCALLTYPE AddRef() override { return InterlockedIncrement(&m_refCount); }
    ULONG STDMETHODCALLTYPE Release() override {
        ULONG u = InterlockedDecrement(&m_refCount);
        if (u == 0) delete this;
        return u;
    }
    HRESULT STDMETHODCALLTYPE Invoke(HRESULT result, ICoreWebView2Environment *env) override;
};

class ControllerCompletedHandler : public ICoreWebView2CreateCoreWebView2ControllerCompletedHandler {
    LONG m_refCount;
    HWND m_hwnd;
    int m_port;
public:
    ControllerCompletedHandler(HWND hwnd, int port) : m_refCount(1), m_hwnd(hwnd), m_port(port) {}
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppv) override {
        if (riid == IID_IUnknown || riid == IID_ICoreWebView2CreateCoreWebView2ControllerCompletedHandler) {
            *ppv = static_cast<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler*>(this);
            AddRef();
            return S_OK;
        }
        *ppv = NULL;
        return E_NOINTERFACE;
    }
    ULONG STDMETHODCALLTYPE AddRef() override { return InterlockedIncrement(&m_refCount); }
    ULONG STDMETHODCALLTYPE Release() override {
        ULONG u = InterlockedDecrement(&m_refCount);
        if (u == 0) delete this;
        return u;
    }
    HRESULT STDMETHODCALLTYPE Invoke(HRESULT result, ICoreWebView2Controller *controller) override;
};

class WebMessageReceivedHandler : public ICoreWebView2WebMessageReceivedEventHandler {
    LONG m_refCount;
    HWND m_hwnd;
public:
    WebMessageReceivedHandler(HWND hwnd) : m_refCount(1), m_hwnd(hwnd) {}
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppv) override {
        if (riid == IID_IUnknown || riid == IID_ICoreWebView2WebMessageReceivedEventHandler) {
            *ppv = static_cast<ICoreWebView2WebMessageReceivedEventHandler*>(this);
            AddRef();
            return S_OK;
        }
        *ppv = NULL;
        return E_NOINTERFACE;
    }
    ULONG STDMETHODCALLTYPE AddRef() override { return InterlockedIncrement(&m_refCount); }
    ULONG STDMETHODCALLTYPE Release() override {
        ULONG u = InterlockedDecrement(&m_refCount);
        if (u == 0) delete this;
        return u;
    }
    HRESULT STDMETHODCALLTYPE Invoke(ICoreWebView2 *sender, ICoreWebView2WebMessageReceivedEventArgs *args) override {
        LPWSTR rawMsg = NULL;
        if (SUCCEEDED(args->TryGetWebMessageAsString(&rawMsg)) && rawMsg) {
            char actionBuf[128] = {0};
            WideCharToMultiByte(CP_UTF8, 0, rawMsg, -1, actionBuf, sizeof(actionBuf), NULL, NULL);
            gui_handle_action(m_hwnd, actionBuf);
            CoTaskMemFree(rawMsg);
        }
        return S_OK;
    }
};

HRESULT STDMETHODCALLTYPE ControllerCompletedHandler::Invoke(HRESULT hr, ICoreWebView2Controller *controller) {
    if (FAILED(hr) || !controller) return hr;

    g_controller = controller;
    g_controller->AddRef();

    RECT bounds;
    GetClientRect(m_hwnd, &bounds);
    g_controller->put_Bounds(bounds);

    g_controller->get_CoreWebView2(&g_webview);
    if (g_webview) {
        EventRegistrationToken token;
        g_webview->add_WebMessageReceived(new WebMessageReceivedHandler(m_hwnd), &token);

        wchar_t url[256];
        swprintf_s(url, 256, L"http://127.0.0.1:%d/", m_port);
        g_webview->Navigate(url);
    }
    return S_OK;
}

HRESULT STDMETHODCALLTYPE EnvCompletedHandler::Invoke(HRESULT hr, ICoreWebView2Environment *env) {
    if (FAILED(hr) || !env) {
        wchar_t errBuf[512];
        swprintf_s(errBuf, 512, L"初始化 WebView2 运行环境失败 (错误代码: 0x%08lX)。\n请确认系统已安装 Microsoft Edge WebView2 Runtime。", (unsigned long)hr);
        MessageBoxW(m_hwnd, errBuf, L"环境错误", MB_OK | MB_ICONERROR);
        return hr;
    }

    env->CreateCoreWebView2Controller(m_hwnd, new ControllerCompletedHandler(m_hwnd, m_port));
    return S_OK;
}

static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_NCCALCSIZE: {
            if (wParam) {
                // Remove standard window borders, filling the whole client area
                return 0;
            }
            break;
        }

        case WM_NCHITTEST: {
            // Enable resize borders on the frameless window edges
            RECT rc;
            GetWindowRect(hwnd, &rc);
            int x = GET_X_LPARAM(lParam);
            int y = GET_Y_LPARAM(lParam);
            const int border = 8;

            int left = (x >= rc.left && x < rc.left + border);
            int right = (x < rc.right && x >= rc.right - border);
            int top = (y >= rc.top && y < rc.top + border);
            int bottom = (y < rc.bottom && y >= rc.bottom - border);

            if (top && left) return HTTOPLEFT;
            if (top && right) return HTTOPRIGHT;
            if (bottom && left) return HTBOTTOMLEFT;
            if (bottom && right) return HTBOTTOMRIGHT;
            if (left) return HTLEFT;
            if (right) return HTRIGHT;
            if (top) return HTTOP;
            if (bottom) return HTBOTTOM;

            return HTCLIENT;
        }

        case WM_SIZE: {
            if (g_controller) {
                RECT bounds;
                GetClientRect(hwnd, &bounds);
                g_controller->put_Bounds(bounds);
            }
            break;
        }

        case WM_TRAYICON: {
            if (lParam == WM_LBUTTONUP || lParam == WM_LBUTTONDBLCLK) {
                ShowWindow(hwnd, SW_RESTORE);
                SetForegroundWindow(hwnd);
            } else if (lParam == WM_RBUTTONUP) {
                tray_show_menu(hwnd);
            }
            break;
        }

        case WM_COMMAND: {
            WORD cmd = LOWORD(wParam);
            if (cmd == IDM_TRAY_OPEN) {
                ShowWindow(hwnd, SW_RESTORE);
                SetForegroundWindow(hwnd);
            } else if (cmd == IDM_TRAY_EXIT) {
                DestroyWindow(hwnd);
            }
            break;
        }

        case WM_CLOSE: {
            // Minimize to system tray on close
            ShowWindow(hwnd, SW_HIDE);
            return 0;
        }

        case WM_DESTROY: {
            tray_remove();
            if (g_controller) {
                g_controller->Close();
                g_controller->Release();
                g_controller = NULL;
            }
            if (g_webview) {
                g_webview->Release();
                g_webview = NULL;
            }
            PostQuitMessage(0);
            return 0;
        }
    }

    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

static void InitWebView(HWND hwnd, int port) {
    HMODULE hLoader = LoadLibraryA("WebView2Loader.dll");
    if (!hLoader) {
        hLoader = LoadLibraryA("lib\\WebView2Loader.dll");
    }
    if (!hLoader) {
        MessageBoxW(hwnd, L"无法加载 WebView2Loader.dll，请确保该文件位于应用目录。", L"启动错误", MB_OK | MB_ICONERROR);
        return;
    }

    auto pfnCreateEnv = (decltype(&CreateCoreWebView2EnvironmentWithOptions))GetProcAddress(hLoader, "CreateCoreWebView2EnvironmentWithOptions");
    if (!pfnCreateEnv) {
        MessageBoxW(hwnd, L"无法定位 WebView2 接口函数。", L"启动错误", MB_OK | MB_ICONERROR);
        return;
    }

    wchar_t userDir[MAX_PATH] = {0};
    DWORD len = GetEnvironmentVariableW(L"LOCALAPPDATA", userDir, MAX_PATH);
    if (len > 0) {
        wcscat_s(userDir, MAX_PATH, L"\\ScheduleWorklog");
        CreateDirectoryW(userDir, NULL);
        wcscat_s(userDir, MAX_PATH, L"\\WebView2_Data");
        CreateDirectoryW(userDir, NULL);
    }

    pfnCreateEnv(NULL, userDir[0] ? userDir : NULL, NULL, new EnvCompletedHandler(hwnd, port));
}

extern "C" int gui_window_create_and_run(int http_port) {
    // Initialize OLE / STA COM apartment for WebView2
    OleInitialize(NULL);

    g_port = http_port;
    HINSTANCE hInstance = GetModuleHandleW(NULL);

    WNDCLASSEXW wc;
    memset(&wc, 0, sizeof(wc));
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.lpszClassName = L"ScheduleWorklogWindow";

    RegisterClassExW(&wc);

    // Initial window size
    int width = 1200;
    int height = 800;
    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);
    int x = (screenW - width) / 2;
    int y = (screenH - height) / 2;

    HWND hwnd = CreateWindowExW(
        0,
        L"ScheduleWorklogWindow",
        L"Schedule & WorkLog",
        WS_POPUP | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX,
        x, y, width, height,
        NULL, NULL, hInstance, NULL
    );

    if (!hwnd) return -1;
    g_hwnd = hwnd;

    // Enable native window shadow via DWM
    MARGINS margins = {1, 1, 1, 1};
    DwmExtendFrameIntoClientArea(hwnd, &margins);

    // Initialize System Tray
    tray_init(hwnd, hInstance, L"Schedule & WorkLog - 日程与工作记录");

    // Initialize WebView2
    InitWebView(hwnd, http_port);

    // Display window
    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);
    SetForegroundWindow(hwnd);

    // Win32 Message Loop
    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    OleUninitialize();
    return 0;
}
