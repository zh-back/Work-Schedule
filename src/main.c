#include "db.h"
#include "server.h"
#include "project.h"
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <shellapi.h>

static BOOL WINAPI console_ctrl_handler(DWORD ctrl_type) {
    if (ctrl_type == CTRL_C_EVENT || ctrl_type == CTRL_CLOSE_EVENT) {
        printf("\n[Schedule & WorkLog] 正在平稳关闭服务与数据库...\n");
        server_stop();
        db_close();
        ExitProcess(0);
    }
    return TRUE;
}

int main(int argc, char *argv[]) {
    // Set console UTF-8 code page
    SetConsoleOutputCP(65001);
    SetConsoleCP(65001);

    printf("===================================================\n");
    printf("  🚀 Schedule & WorkLog (Apifox Edition - C Core)  \n");
    printf("  本地优先 (Local-First) 多项目日程与周总结软件     \n");
    printf("===================================================\n\n");

    SetConsoleCtrlHandler(console_ctrl_handler, TRUE);

    // 1. Initialize local SQLite database
    const char *db_path = "schedule_worklog.db";
    printf("[1/3] 初始化本地数据库 (%s)...\n", db_path);
    if (db_init(db_path) != 0) {
        fprintf(stderr, "[ERROR] 数据库初始化失败!\n");
        return 1;
    }
    printf("      数据库初始化成功 (Schema 就绪，内置个人空间已就绪)\n");

    // 2. Start local C embedded HTTP server
    printf("[2/3] 启动本地 C 语言嵌入式 API 服务...\n");
    int port = 48920;
    if (server_start_background(port) != 0) {
        // Fallback to ephemeral port if 48920 is occupied
        if (server_start_background(0) != 0) {
            fprintf(stderr, "[ERROR] 本地服务启动失败!\n");
            db_close();
            return 1;
        }
    }
    int bound_port = server_get_bound_port();
    char app_url[128];
    snprintf(app_url, sizeof(app_url), "http://127.0.0.1:%d/", bound_port);
    printf("      服务已就绪，监听地址: %s\n", app_url);

    // 3. Launch UI in browser
    printf("[3/3] 正在调起桌面界面窗口...\n");
    ShellExecuteA(NULL, "open", app_url, NULL, NULL, SW_SHOWNORMAL);

    printf("\n✅ 软件已正常启动并运行！\n");
    printf("---------------------------------------------------\n");
    printf("提示：在浏览器中使用软件；在控制台按回车键或 Ctrl+C 退出程序。\n");
    printf("---------------------------------------------------\n");

    // Wait for user input to exit
    getchar();

    printf("[Schedule & WorkLog] 正在停止服务并保存数据...\n");
    server_stop();
    db_close();
    printf("服务已安全退出。\n");
    return 0;
}
