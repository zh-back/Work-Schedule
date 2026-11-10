#include "db.h"
#include "server.h"
#include "project.h"
#include "tray.h"
#include "gui.h"
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

static BOOL WINAPI console_ctrl_handler(DWORD ctrl_type) {
    if (ctrl_type == CTRL_C_EVENT || ctrl_type == CTRL_CLOSE_EVENT) {
        printf("\n[Schedule & WorkLog] 正在平稳关闭服务与数据库...\n");
        tray_remove();
        server_stop();
        db_close();
        single_instance_cleanup();
        ExitProcess(0);
    }
    return TRUE;
}

int main(int argc, char *argv[]) {
    // Hide console window if any exists
    HWND hConsole = GetConsoleWindow();
    if (hConsole) {
        ShowWindow(hConsole, SW_HIDE);
    }

    // 1. Single-instance protection: restore existing window if already running
    if (check_single_instance("Global\\ScheduleWorkLog_SingleInstance") != 0) {
        // Existing instance was brought to foreground; safely exit duplicate process
        return 0;
    }

    // 2. High-DPI Awareness for crisp rendering on 4K / scaled displays
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    // Set console UTF-8 code page
    SetConsoleOutputCP(65001);
    SetConsoleCP(65001);

    printf("===================================================\n");
    printf("  🚀 Schedule & WorkLog (Apifox Frameless Client)  \n");
    printf("  无边框独立桌面应用 | 毫秒级打卡 | 托盘常驻守护    \n");
    printf("===================================================\n\n");

    SetConsoleCtrlHandler(console_ctrl_handler, TRUE);

    // 3. Initialize local SQLite database
    const char *db_path = "schedule_worklog.db";
    printf("[1/3] 初始化本地数据库 (%s)...\n", db_path);
    if (db_init(db_path) != 0) {
        fprintf(stderr, "[ERROR] 数据库初始化失败!\n");
        single_instance_cleanup();
        return 1;
    }
    printf("      数据库初始化成功 (Schema 就绪，内置个人空间已就绪)\n");

    // 4. Start local C embedded HTTP server
    printf("[2/3] 启动本地 C 语言嵌入式 API 服务...\n");
    int port = 48920;
    if (server_start_background(port) != 0) {
        if (server_start_background(0) != 0) {
            fprintf(stderr, "[ERROR] 本地服务启动失败!\n");
            db_close();
            single_instance_cleanup();
            return 1;
        }
    }
    int bound_port = server_get_bound_port();
    printf("      服务已就绪，端口: %d\n", bound_port);

    // 5. Create native frameless desktop client window (runs message loop until quit)
    printf("[3/3] 创建无边框原生桌面窗体与 WebView2 宿主...\n");
    printf("      提示：点击窗口右上角 ✕ 将最小化收起到右下角系统托盘。\n");

    gui_window_create_and_run(bound_port);

    // 6. Graceful application exit
    printf("[Exit] 正在退出应用，清理资源...\n");
    tray_remove();
    server_stop();
    db_close();
    single_instance_cleanup();

    return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    (void)hInstance;
    (void)hPrevInstance;
    (void)lpCmdLine;
    (void)nCmdShow;
    return main(__argc, __argv);
}
