#include "test_framework.h"
#include "db.h"
#include "server.h"
#include <winsock2.h>
#include <ws2tcpip.h>

static int http_get(int port, const char *path, char *out_buf, size_t max_len) {
    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) return -1;

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons((u_short)port);
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) != 0) {
        closesocket(sock);
        return -1;
    }

    char req[512];
    snprintf(req, sizeof(req), "GET %s HTTP/1.1\r\nHost: 127.0.0.1\r\nConnection: close\r\n\r\n", path);
    send(sock, req, (int)strlen(req), 0);

    int total = 0;
    while (total < (int)max_len - 1) {
        int bytes = recv(sock, out_buf + total, (int)max_len - 1 - total, 0);
        if (bytes <= 0) break;
        total += bytes;
    }
    out_buf[total] = '\0';
    closesocket(sock);
    return total;
}

int test_embedded_server_api(void) {
    ASSERT_EQ_INT(0, db_init(":memory:"));

    ASSERT_EQ_INT(0, server_start_background(0));
    int port = server_get_bound_port();
    ASSERT_TRUE(port > 1024);
    ASSERT_TRUE(server_is_running());

    // Give socket brief moment to listen
    Sleep(50);

    char resp[4096];
    int bytes = http_get(port, "/api/projects", resp, sizeof(resp));
    ASSERT_TRUE(bytes > 0);
    ASSERT_TRUE(strstr(resp, "200 OK") != NULL);
    ASSERT_TRUE(strstr(resp, "个人工作") != NULL);

    server_stop();
    ASSERT_TRUE(!server_is_running());

    db_close();
    return 0;
}

int main(void) {
    printf("=== Running Embedded HTTP Server Tests ===\n");
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);

    RUN_TEST(test_embedded_server_api);

    WSACleanup();
    TEST_REPORT();
}
