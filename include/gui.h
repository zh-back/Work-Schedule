#ifndef GUI_H
#define GUI_H

#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif

// Parse and execute window actions ("drag", "minimize", "maximize", "close")
// Returns 0 on recognized action, -1 on unknown action.
int gui_handle_action(HWND hwnd, const char *action);

// Create the frameless native desktop window, attach WebView2 controller, and run message loop.
// Blocks until application exit.
int gui_window_create_and_run(int http_port);

#ifdef __cplusplus
}
#endif

#endif // GUI_H
