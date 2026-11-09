#ifndef TRAY_H
#define TRAY_H

#include <windows.h>

#define WM_TRAYICON (WM_USER + 1)
#define WM_RESTORE_EXISTING (WM_USER + 2)

#define IDM_TRAY_OPEN 1001
#define IDM_TRAY_EXIT 1002

#ifdef __cplusplus
extern "C" {
#endif

// Check single-instance mutex.
// Returns 0 if this is the primary instance.
// Returns -1 if an instance is already running (attempts to bring existing window to foreground).
int check_single_instance(const char *mutex_name);

// Release and close single-instance mutex handle.
void single_instance_cleanup(void);

// Initialize system tray icon
void tray_init(HWND hwnd, HINSTANCE hInstance, const wchar_t *tip);

// Remove system tray icon (prevents ghost icons)
void tray_remove(void);

// Show context menu when right-clicking the tray icon
void tray_show_menu(HWND hwnd);

#ifdef __cplusplus
}
#endif

#endif // TRAY_H
