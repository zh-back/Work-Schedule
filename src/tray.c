#include "tray.h"
#include <shellapi.h>
#include <stdio.h>

static HANDLE g_hMutex = NULL;
static NOTIFYICONDATAW g_nid;
static int g_tray_active = 0;

int check_single_instance(const char *mutex_name) {
    const char *name = mutex_name ? mutex_name : "Global\\ScheduleWorkLog_SingleInstance";
    g_hMutex = CreateMutexA(NULL, TRUE, name);
    if (!g_hMutex) {
        return 0; // Fallback if mutex creation fails
    }

    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        HWND hExisting = FindWindowW(L"ScheduleWorklogWindow", NULL);
        if (hExisting) {
            ShowWindow(hExisting, SW_RESTORE);
            SetForegroundWindow(hExisting);
        }
        CloseHandle(g_hMutex);
        g_hMutex = NULL;
        return -1;
    }

    return 0;
}

void single_instance_cleanup(void) {
    if (g_hMutex) {
        ReleaseMutex(g_hMutex);
        CloseHandle(g_hMutex);
        g_hMutex = NULL;
    }
}

void tray_init(HWND hwnd, HINSTANCE hInstance, const wchar_t *tip) {
    memset(&g_nid, 0, sizeof(g_nid));
    g_nid.cbSize = sizeof(NOTIFYICONDATAW);
    g_nid.hWnd = hwnd;
    g_nid.uID = 1;
    g_nid.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
    g_nid.uCallbackMessage = WM_TRAYICON;
    g_nid.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wcsncpy(g_nid.szTip, tip ? tip : L"Schedule & WorkLog - 日程与工作记录", 127);

    Shell_NotifyIconW(NIM_ADD, &g_nid);
    g_tray_active = 1;
}

void tray_remove(void) {
    if (g_tray_active) {
        Shell_NotifyIconW(NIM_DELETE, &g_nid);
        g_tray_active = 0;
    }
}

void tray_show_menu(HWND hwnd) {
    HMENU hMenu = CreatePopupMenu();
    if (!hMenu) return;

    AppendMenuW(hMenu, MF_STRING, IDM_TRAY_OPEN, L"打开工作台");
    AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);
    AppendMenuW(hMenu, MF_STRING, IDM_TRAY_EXIT, L"退出程序");

    POINT pt;
    GetCursorPos(&pt);
    SetForegroundWindow(hwnd);
    TrackPopupMenu(hMenu, TPM_RIGHTBUTTON, pt.x, pt.y, 0, hwnd, NULL);
    DestroyMenu(hMenu);
}
