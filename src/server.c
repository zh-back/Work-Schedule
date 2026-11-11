#include "server.h"
#include "project.h"
#include "item.h"
#include "summary.h"
#include "db.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
typedef int SOCKET;
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#define closesocket close
typedef pthread_t HANDLE;
#endif

static HANDLE g_thread = 0;
static SOCKET g_listen_socket = INVALID_SOCKET;
static int g_bound_port = 0;
static volatile int g_running = 0;

int server_is_running(void) {
    return g_running;
}

int server_get_bound_port(void) {
    return g_bound_port;
}

// Minimal JSON escape
static void json_escape(const char *in, char *out, size_t max_len) {
    if (!in || !out || max_len == 0) return;
    size_t o = 0;
    for (size_t i = 0; in[i] && o + 2 < max_len; i++) {
        if (in[i] == '"' || in[i] == '\\') {
            out[o++] = '\\';
            out[o++] = in[i];
        } else if (in[i] == '\n') {
            out[o++] = '\\';
            out[o++] = 'n';
        } else if (in[i] == '\r') {
            out[o++] = '\\';
            out[o++] = 'r';
        } else if (in[i] == '\t') {
            out[o++] = '\\';
            out[o++] = 't';
        } else {
            out[o++] = in[i];
        }
    }
    out[o] = '\0';
}

// Simple query parameter extractor
static int get_query_param(const char *query, const char *key, char *out_val, size_t max_len) {
    if (!query || !key || !out_val || max_len == 0) return 0;
    out_val[0] = '\0';
    size_t klen = strlen(key);
    const char *p = query;
    while (*p) {
        if (strncmp(p, key, klen) == 0 && p[klen] == '=') {
            p += klen + 1;
            size_t o = 0;
            while (*p && *p != '&' && *p != ' ' && o + 1 < max_len) {
                out_val[o++] = *p++;
            }
            out_val[o] = '\0';
            return 1;
        }
        while (*p && *p != '&' && *p != ' ') p++;
        if (*p == '&') p++;
    }
    return 0;
}

// Simple JSON body field extractor
static int get_json_str(const char *body, const char *key, char *out_val, size_t max_len) {
    if (!body || !key || !out_val || max_len == 0) return 0;
    out_val[0] = '\0';
    char pattern[128];
    snprintf(pattern, sizeof(pattern), "\"%s\"", key);
    const char *p = strstr(body, pattern);
    if (!p) return 0;
    p += strlen(pattern);
    while (*p && (*p == ' ' || *p == ':')) p++;
    if (*p == '"') {
        p++;
        size_t o = 0;
        while (*p && o + 1 < max_len) {
            if (*p == '\\' && *(p + 1) != '\0') {
                p++; // skip backslash escape
                out_val[o++] = *p++;
            } else if (*p == '"') {
                break;
            } else {
                out_val[o++] = *p++;
            }
        }
        out_val[o] = '\0';
        return 1;
    }
    return 0;
}

static int get_json_int(const char *body, const char *key, int default_val) {
    if (!body || !key) return default_val;
    char pattern[128];
    snprintf(pattern, sizeof(pattern), "\"%s\"", key);
    const char *p = strstr(body, pattern);
    if (!p) return default_val;
    p += strlen(pattern);
    while (*p && (*p == ' ' || *p == ':')) p++;
    return atoi(p);
}

static int64_t get_json_int64(const char *body, const char *key, int64_t default_val) {
    if (!body || !key) return default_val;
    char pattern[128];
    snprintf(pattern, sizeof(pattern), "\"%s\"", key);
    const char *p = strstr(body, pattern);
    if (!p) return default_val;
    p += strlen(pattern);
    while (*p && (*p == ' ' || *p == ':')) p++;
#ifdef _WIN32
    return _strtoi64(p, NULL, 10);
#else
    return strtoll(p, NULL, 10);
#endif
}

static void send_http_response(SOCKET sock, int status_code, const char *content_type, const char *body) {
    char header[512];
    size_t body_len = body ? strlen(body) : 0;
    snprintf(header, sizeof(header),
             "HTTP/1.1 %d %s\r\n"
             "Content-Type: %s\r\n"
             "Content-Length: %zu\r\n"
             "Access-Control-Allow-Origin: *\r\n"
             "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n"
             "Access-Control-Allow-Headers: Content-Type\r\n"
             "Connection: close\r\n\r\n",
             status_code, (status_code == 200 ? "OK" : (status_code == 404 ? "Not Found" : "Internal Error")),
             content_type, body_len);
    send(sock, header, (int)strlen(header), 0);
    if (body_len > 0) {
        send(sock, body, (int)body_len, 0);
    }
}

static void handle_client(SOCKET client) {
    char req[8192];
    int received = recv(client, req, sizeof(req) - 1, 0);
    if (received <= 0) {
        closesocket(client);
        return;
    }
    req[received] = '\0';

    char method[16] = {0};
    char full_path[1024] = {0};
    sscanf(req, "%15s %1023s", method, full_path);

    // OPTIONS preflight
    if (strcmp(method, "OPTIONS") == 0) {
        send_http_response(client, 200, "text/plain", "");
        closesocket(client);
        return;
    }

    char path[512] = {0};
    char query[512] = {0};
    char *qmark = strchr(full_path, '?');
    if (qmark) {
        size_t plen = qmark - full_path;
        strncpy(path, full_path, plen < sizeof(path) - 1 ? plen : sizeof(path) - 1);
        strncpy(query, qmark + 1, sizeof(query) - 1);
    } else {
        strncpy(path, full_path, sizeof(path) - 1);
    }

    const char *body = strstr(req, "\r\n\r\n");
    if (body) body += 4; else body = "";

    // API Routes
    if (strcmp(path, "/api/projects") == 0 && strcmp(method, "GET") == 0) {
        ProjectList list;
        project_list(&list);
        char *resp = (char*)malloc(16384);
        strcpy(resp, "[");
        for (int i = 0; i < list.count; i++) {
            char item[512];
            char escaped_desc[512];
            json_escape(list.items[i].description, escaped_desc, sizeof(escaped_desc));
            snprintf(item, sizeof(item), "%s{\"id\":\"%s\",\"name\":\"%s\",\"description\":\"%s\",\"member_count\":%d}",
                     i > 0 ? "," : "", list.items[i].id, list.items[i].name, escaped_desc, list.items[i].member_count);
            strcat(resp, item);
        }
        strcat(resp, "]");
        send_http_response(client, 200, "application/json; charset=utf-8", resp);
        free(resp);
        project_list_free(&list);
    }
    else if (strcmp(path, "/api/projects") == 0 && strcmp(method, "POST") == 0) {
        char name[128] = {0};
        char desc[512] = {0};
        get_json_str(body, "name", name, sizeof(name));
        get_json_str(body, "description", desc, sizeof(desc));
        char new_id[UUID_STR_LEN];
        int rc = project_create(name, desc, new_id);
        char resp[256];
        snprintf(resp, sizeof(resp), "{\"success\":%s,\"id\":\"%s\"}", rc == 0 ? "true" : "false", new_id);
        send_http_response(client, rc == 0 ? 200 : 400, "application/json; charset=utf-8", resp);
    }
    else if ((strcmp(path, "/api/projects/delete") == 0 || (strcmp(path, "/api/projects") == 0 && strcmp(method, "DELETE") == 0)) && (strcmp(method, "POST") == 0 || strcmp(method, "DELETE") == 0)) {
        char proj_id[UUID_STR_LEN] = {0};
        get_json_str(body, "id", proj_id, sizeof(proj_id));
        if (strlen(proj_id) == 0) {
            get_json_str(body, "project_id", proj_id, sizeof(proj_id));
        }
        if (strlen(proj_id) == 0) {
            get_query_param(query, "id", proj_id, sizeof(proj_id));
        }
        int rc = project_delete(proj_id);
        char resp[128];
        snprintf(resp, sizeof(resp), "{\"success\":%s}", rc == 0 ? "true" : "false");
        send_http_response(client, rc == 0 ? 200 : 400, "application/json; charset=utf-8", resp);
    }
    else if (strcmp(path, "/api/members") == 0 && strcmp(method, "GET") == 0) {
        char proj_id[UUID_STR_LEN] = {0};
        get_query_param(query, "project_id", proj_id, sizeof(proj_id));
        MemberList list;
        member_list_by_project(proj_id, &list);
        char *resp = (char*)malloc(16384);
        strcpy(resp, "[");
        for (int i = 0; i < list.count; i++) {
            char item[512];
            snprintf(item, sizeof(item), "%s{\"id\":\"%s\",\"name\":\"%s\",\"role\":\"%s\",\"is_current\":%d}",
                     i > 0 ? "," : "", list.items[i].id, list.items[i].name, list.items[i].role, list.items[i].is_current);
            strcat(resp, item);
        }
        strcat(resp, "]");
        send_http_response(client, 200, "application/json; charset=utf-8", resp);
        free(resp);
        member_list_free(&list);
    }
    else if (strcmp(path, "/api/members") == 0 && strcmp(method, "POST") == 0) {
        char proj_id[UUID_STR_LEN] = {0};
        char name[128] = {0};
        char role[32] = "member";
        get_json_str(body, "project_id", proj_id, sizeof(proj_id));
        get_json_str(body, "name", name, sizeof(name));
        get_json_str(body, "role", role, sizeof(role));
        char user_id[UUID_STR_LEN];
        int rc = member_add(proj_id, name, role, user_id);
        char resp[256];
        snprintf(resp, sizeof(resp), "{\"success\":%s,\"id\":\"%s\"}", rc == 0 ? "true" : "false", user_id);
        send_http_response(client, rc == 0 ? 200 : 400, "application/json; charset=utf-8", resp);
    }
    else if (strcmp(path, "/api/user/current") == 0 && strcmp(method, "GET") == 0) {
        User u;
        if (user_get_current(&u) == 0) {
            char resp[512];
            snprintf(resp, sizeof(resp), "{\"id\":\"%s\",\"name\":\"%s\",\"avatar\":\"%s\"}", u.id, u.name, u.avatar);
            send_http_response(client, 200, "application/json; charset=utf-8", resp);
        } else {
            send_http_response(client, 404, "application/json", "{\"error\":\"No active user\"}");
        }
    }
    else if (strcmp(path, "/api/user/switch") == 0 && strcmp(method, "POST") == 0) {
        char user_id[UUID_STR_LEN] = {0};
        get_json_str(body, "user_id", user_id, sizeof(user_id));
        int rc = user_set_current(user_id);
        send_http_response(client, rc == 0 ? 200 : 400, "application/json", rc == 0 ? "{\"success\":true}" : "{\"error\":\"Failed\"}");
    }
    else if (strcmp(path, "/api/items") == 0 && strcmp(method, "GET") == 0) {
        char proj_id[UUID_STR_LEN] = {0};
        char user_id[UUID_STR_LEN] = {0};
        char date_param[64] = {0};
        get_query_param(query, "project_id", proj_id, sizeof(proj_id));
        get_query_param(query, "user_id", user_id, sizeof(user_id));
        get_query_param(query, "date", date_param, sizeof(date_param));

        const char *p_id = strlen(proj_id) > 0 ? proj_id : NULL;
        const char *u_id = strlen(user_id) > 0 ? user_id : NULL;

        int64_t start_ms = 0;
        int64_t end_ms = 0;

        int64_t now_ms = get_current_timestamp_ms();
        time_t sec = (time_t)(now_ms / 1000LL);
        struct tm l_tm;
        PORTABLE_LOCALTIME(&l_tm, &sec);

        if (strcmp(date_param, "all") == 0) {
            start_ms = 0;
            end_ms = 0;
        } else if (strcmp(date_param, "yesterday") == 0) {
            l_tm.tm_hour = 0;
            l_tm.tm_min = 0;
            l_tm.tm_sec = 0;
            l_tm.tm_mday -= 1;
            time_t y_sec = mktime(&l_tm);
            start_ms = (int64_t)y_sec * 1000LL;
            end_ms = start_ms + 86400000LL - 1LL;
        } else if (strlen(date_param) >= 10 && strchr(date_param, '-')) {
            int y = 0, m = 0, d = 0;
            if (sscanf(date_param, "%d-%d-%d", &y, &m, &d) == 3) {
                struct tm t = {0};
                t.tm_year = y - 1900;
                t.tm_mon = m - 1;
                t.tm_mday = d;
                t.tm_hour = 0;
                t.tm_min = 0;
                t.tm_sec = 0;
                t.tm_isdst = -1;
                time_t target_sec = mktime(&t);
                if (target_sec != (time_t)-1) {
                    start_ms = (int64_t)target_sec * 1000LL;
                    end_ms = start_ms + 86400000LL - 1LL;
                }
            }
        } else {
            // Default: today
            l_tm.tm_hour = 0;
            l_tm.tm_min = 0;
            l_tm.tm_sec = 0;
            time_t t_sec = mktime(&l_tm);
            start_ms = (int64_t)t_sec * 1000LL;
            end_ms = 0;
        }

        ItemList todos;
        ItemList dones;
        item_list_active_todos(p_id, u_id, &todos);
        item_list_dones_by_date_range(p_id, u_id, start_ms, end_ms, &dones);

        size_t buf_cap = 65536;
        char *resp = (char*)malloc(buf_cap);
        if (!resp) {
            send_http_response(client, 500, "application/json", "{\"error\":\"Out of memory\"}");
            closesocket(client);
            item_list_free(&todos);
            item_list_free(&dones);
            return;
        }

        size_t resp_len = snprintf(resp, buf_cap, "{\"todos\":[");
        for (int i = 0; i < todos.count; i++) {
            char escaped_c[1024];
            json_escape(todos.items[i].content, escaped_c, sizeof(escaped_c));
            char itm[2048];
            int n = snprintf(itm, sizeof(itm), "%s{\"id\":\"%s\",\"content\":\"%s\",\"creator_name\":\"%s\",\"visibility\":\"%s\",\"tags\":\"%s\",\"created_at\":%lld}",
                             i > 0 ? "," : "", todos.items[i].id, escaped_c, todos.items[i].creator_name, todos.items[i].visibility, todos.items[i].tags, (long long)todos.items[i].created_at);
            if (resp_len + (size_t)n + 2048 > buf_cap) {
                buf_cap *= 2;
                char *new_resp = (char*)realloc(resp, buf_cap);
                if (new_resp) resp = new_resp;
            }
            strcpy(resp + resp_len, itm);
            resp_len += n;
        }
        resp_len += snprintf(resp + resp_len, buf_cap - resp_len, "],\"dones\":[");
        for (int i = 0; i < dones.count; i++) {
            char escaped_c[1024];
            json_escape(dones.items[i].content, escaped_c, sizeof(escaped_c));
            char itm[2048];
            int n = snprintf(itm, sizeof(itm), "%s{\"id\":\"%s\",\"content\":\"%s\",\"creator_name\":\"%s\",\"completed_at\":%lld,\"created_at\":%lld,\"visibility\":\"%s\",\"tags\":\"%s\"}",
                             i > 0 ? "," : "", dones.items[i].id, escaped_c, dones.items[i].creator_name, (long long)dones.items[i].completed_at, (long long)dones.items[i].created_at, dones.items[i].visibility, dones.items[i].tags);
            if (resp_len + (size_t)n + 2048 > buf_cap) {
                buf_cap *= 2;
                char *new_resp = (char*)realloc(resp, buf_cap);
                if (new_resp) resp = new_resp;
            }
            strcpy(resp + resp_len, itm);
            resp_len += n;
        }
        snprintf(resp + resp_len, buf_cap - resp_len, "]}");

        send_http_response(client, 200, "application/json; charset=utf-8", resp);
        free(resp);
        item_list_free(&todos);
        item_list_free(&dones);
    }
    else if (strcmp(path, "/api/items") == 0 && strcmp(method, "POST") == 0) {
        char proj_id[UUID_STR_LEN] = {0};
        char user_id[UUID_STR_LEN] = {0};
        char content[512] = {0};
        get_json_str(body, "project_id", proj_id, sizeof(proj_id));
        get_json_str(body, "user_id", user_id, sizeof(user_id));
        get_json_str(body, "content", content, sizeof(content));
        int is_done = get_json_int(body, "is_done", 0);
        int is_private = get_json_int(body, "is_private", 0);
        int64_t created_at = get_json_int64(body, "created_at", 0);

        char new_id[UUID_STR_LEN];
        int rc;
        if (is_done) {
            rc = item_create_done_ext(proj_id, user_id, content, is_private, created_at, created_at, new_id);
        } else {
            rc = item_create_todo_ext(proj_id, user_id, content, is_private, created_at, new_id);
        }
        char resp[256];
        snprintf(resp, sizeof(resp), "{\"success\":%s,\"id\":\"%s\"}", rc == 0 ? "true" : "false", new_id);
        send_http_response(client, rc == 0 ? 200 : 400, "application/json; charset=utf-8", resp);
    }
    else if (strcmp(path, "/api/items/done") == 0 && strcmp(method, "POST") == 0) {
        char item_id[UUID_STR_LEN] = {0};
        get_json_str(body, "item_id", item_id, sizeof(item_id));
        int rc = item_mark_done(item_id);
        send_http_response(client, rc == 0 ? 200 : 400, "application/json", rc == 0 ? "{\"success\":true}" : "{\"error\":\"Failed\"}");
    }
    else if (strcmp(path, "/api/items/todo") == 0 && strcmp(method, "POST") == 0) {
        char item_id[UUID_STR_LEN] = {0};
        get_json_str(body, "item_id", item_id, sizeof(item_id));
        int rc = item_mark_todo(item_id);
        send_http_response(client, rc == 0 ? 200 : 400, "application/json", rc == 0 ? "{\"success\":true}" : "{\"error\":\"Failed\"}");
    }
    else if ((strcmp(path, "/api/items/delete") == 0 || (strcmp(path, "/api/items") == 0 && strcmp(method, "DELETE") == 0)) && (strcmp(method, "POST") == 0 || strcmp(method, "DELETE") == 0)) {
        char item_id[UUID_STR_LEN] = {0};
        get_json_str(body, "item_id", item_id, sizeof(item_id));
        if (strlen(item_id) == 0) {
            get_query_param(query, "item_id", item_id, sizeof(item_id));
        }
        int rc = item_delete(item_id);
        send_http_response(client, rc == 0 ? 200 : 400, "application/json", rc == 0 ? "{\"success\":true}" : "{\"error\":\"Failed\"}");
    }
    else if (strcmp(path, "/api/summary") == 0 && strcmp(method, "GET") == 0) {
        char type[32] = "personal";
        char id[UUID_STR_LEN] = {0};
        get_query_param(query, "type", type, sizeof(type));
        get_query_param(query, "id", id, sizeof(id));

        int year, week;
        summary_get_current_week(&year, &week);
        int64_t start_ms, end_ms;
        summary_get_week_range(year, week, &start_ms, &end_ms);

        char *md = NULL;
        int rc;
        if (strcmp(type, "project") == 0) {
            rc = summary_generate_project(id, start_ms, end_ms, &md);
        } else {
            rc = summary_generate_personal(id, start_ms, end_ms, &md);
        }

        if (rc == 0 && md) {
            size_t md_len = strlen(md);
            char *escaped_md = (char*)malloc(md_len * 2 + 128);
            json_escape(md, escaped_md, md_len * 2 + 128);
            char *resp = (char*)malloc(strlen(escaped_md) + 64);
            snprintf(resp, strlen(escaped_md) + 64, "{\"markdown\":\"%s\"}", escaped_md);
            send_http_response(client, 200, "application/json; charset=utf-8", resp);
            free(resp);
            free(escaped_md);
            summary_free(md);
        } else {
            send_http_response(client, 400, "application/json", "{\"error\":\"Failed to generate summary\"}");
        }
    }
    // Static HTML/CSS/JS files serving
    else {
        // Prevent path traversal
        if (strstr(path, "..") != NULL) {
            send_http_response(client, 403, "text/plain", "Forbidden: Invalid path");
            closesocket(client);
            return;
        }

        char file_path[1024];
        if (strcmp(path, "/") == 0) {
            snprintf(file_path, sizeof(file_path), "web/index.html");
        } else {
            snprintf(file_path, sizeof(file_path), "web%s", path);
        }

        FILE *f = fopen(file_path, "rb");
        if (f) {
            fseek(f, 0, SEEK_END);
            long fsize = ftell(f);
            fseek(f, 0, SEEK_SET);

            char *fdata = (char*)malloc(fsize + 1);
            fread(fdata, 1, fsize, f);
            fdata[fsize] = '\0';
            fclose(f);

            const char *ctype = "text/html; charset=utf-8";
            if (strstr(file_path, ".css")) ctype = "text/css; charset=utf-8";
            else if (strstr(file_path, ".js")) ctype = "application/javascript; charset=utf-8";

            send_http_response(client, 200, ctype, fdata);
            free(fdata);
        } else {
            send_http_response(client, 404, "text/plain", "Not Found");
        }
    }

    closesocket(client);
}

#ifdef _WIN32
static DWORD WINAPI server_thread_func(LPVOID lpParam) {
    (void)lpParam;
#else
static void* server_thread_func(void *lpParam) {
    (void)lpParam;
#endif
    while (g_running) {
        SOCKET client = accept(g_listen_socket, NULL, NULL);
        if (client == INVALID_SOCKET) {
            if (!g_running) break;
            continue;
        }
        handle_client(client);
    }
#ifdef _WIN32
    return 0;
#else
    return NULL;
#endif
}

int server_start_background(int port) {
    if (g_running) return 0;

#ifdef _WIN32
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);
#endif

    g_listen_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (g_listen_socket == INVALID_SOCKET) return -1;

#ifndef _WIN32
    int opt = 1;
    setsockopt(g_listen_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
#endif

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    addr.sin_port = htons((unsigned short)port);

    if (bind(g_listen_socket, (struct sockaddr*)&addr, sizeof(addr)) != 0) {
        closesocket(g_listen_socket);
        g_listen_socket = INVALID_SOCKET;
        return -1;
    }

    // Determine actual bound port
#ifdef _WIN32
    int addrlen = sizeof(addr);
#else
    socklen_t addrlen = sizeof(addr);
#endif
    getsockname(g_listen_socket, (struct sockaddr*)&addr, &addrlen);
    g_bound_port = ntohs(addr.sin_port);

    if (listen(g_listen_socket, 16) != 0) {
        closesocket(g_listen_socket);
        g_listen_socket = INVALID_SOCKET;
        return -1;
    }

    g_running = 1;
#ifdef _WIN32
    g_thread = CreateThread(NULL, 0, server_thread_func, NULL, 0, NULL);
    if (!g_thread) {
        g_running = 0;
        closesocket(g_listen_socket);
        g_listen_socket = INVALID_SOCKET;
        return -1;
    }
#else
    if (pthread_create(&g_thread, NULL, server_thread_func, NULL) != 0) {
        g_running = 0;
        closesocket(g_listen_socket);
        g_listen_socket = INVALID_SOCKET;
        return -1;
    }
#endif

    return 0;
}

void server_stop(void) {
    if (!g_running) return;
    g_running = 0;
    if (g_listen_socket != INVALID_SOCKET) {
        closesocket(g_listen_socket);
        g_listen_socket = INVALID_SOCKET;
    }
#ifdef _WIN32
    if (g_thread) {
        WaitForSingleObject(g_thread, 1000);
        CloseHandle(g_thread);
        g_thread = NULL;
    }
#else
    if (g_thread) {
        pthread_join(g_thread, NULL);
        g_thread = 0;
    }
#endif
}
