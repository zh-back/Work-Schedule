#include "summary.h"
#include "db.h"
#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

void summary_get_current_week(int *out_year, int *out_week) {
    time_t now = time(NULL);
    struct tm t;
    PORTABLE_LOCALTIME(&t, &now);

    char buf[16];
    strftime(buf, sizeof(buf), "%Y", &t);
    if (out_year) *out_year = atoi(buf);

    strftime(buf, sizeof(buf), "%W", &t);
    if (out_week) *out_week = atoi(buf) + 1; // 1-based week
}

int summary_get_week_range(int year, int week_num, int64_t *out_start_ms, int64_t *out_end_ms) {
    if (week_num < 1 || week_num > 53) return -1;

    struct tm tm_first;
    memset(&tm_first, 0, sizeof(tm_first));
    tm_first.tm_year = year - 1900;
    tm_first.tm_mon = 0;
    tm_first.tm_mday = 4; // Jan 4 is always in week 1 of ISO 8601
    time_t t_jan4 = mktime(&tm_first);

    struct tm tm_jan4;
    PORTABLE_LOCALTIME(&tm_jan4, &t_jan4);

    // Days since Monday (0 for Mon, 6 for Sun)
    int day_of_week = (tm_jan4.tm_wday + 6) % 7;
    time_t t_week1_mon = t_jan4 - (day_of_week * 86400);

    time_t t_target_mon = t_week1_mon + ((week_num - 1) * 7 * 86400);
    time_t t_target_sun = t_target_mon + (7 * 86400) - 1;

    if (out_start_ms) *out_start_ms = (int64_t)t_target_mon * 1000LL;
    if (out_end_ms) *out_end_ms = ((int64_t)t_target_sun * 1000LL) + 999LL;
    return 0;
}

typedef struct {
    char *data;
    size_t length;
    size_t capacity;
} StrBuf;

static void buf_init(StrBuf *buf) {
    buf->capacity = 1024;
    buf->length = 0;
    buf->data = (char*)malloc(buf->capacity);
    if (buf->data) buf->data[0] = '\0';
}

static void buf_append(StrBuf *buf, const char *str) {
    if (!str || !buf->data) return;
    size_t str_len = strlen(str);
    while (buf->length + str_len + 1 >= buf->capacity) {
        buf->capacity *= 2;
        char *new_data = (char*)realloc(buf->data, buf->capacity);
        if (!new_data) return;
        buf->data = new_data;
    }
    memcpy(buf->data + buf->length, str, str_len);
    buf->length += str_len;
    buf->data[buf->length] = '\0';
}

int summary_generate_personal(const char *user_id, int64_t start_ms, int64_t end_ms, char **out_markdown) {
    if (!user_id || !out_markdown) return -1;
    sqlite3 *db = db_get_handle();
    if (!db) return -1;

    // Fetch user name
    char user_name[128] = "我";
    sqlite3_stmt *stmt = NULL;
    if (sqlite3_prepare_v2(db, "SELECT name FROM users WHERE id = ?;", -1, &stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, user_id, -1, SQLITE_STATIC);
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            const char *n = (const char*)sqlite3_column_text(stmt, 0);
            if (n) strncpy(user_name, n, sizeof(user_name) - 1);
        }
        sqlite3_finalize(stmt);
    }

    StrBuf buf;
    buf_init(&buf);

    char header[256];
    snprintf(header, sizeof(header), "# 个人工作周报总结 - %s\n\n> 统计区间：已完成事项汇总\n\n", user_name);
    buf_append(&buf, header);

    // Query all items for this user within date range, grouped by project
    const char *sql = 
        "SELECT p.name, si.content, si.completed_at, si.visibility, si.tags "
        "FROM schedule_items si "
        "JOIN projects p ON si.project_id = p.id "
        "WHERE si.creator_id = ? "
        "AND si.status = 'done' "
        "AND si.completed_at >= ? AND si.completed_at <= ? "
        "AND si.deleted_at IS NULL "
        "ORDER BY p.name ASC, si.completed_at ASC;";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        free(buf.data);
        return -1;
    }

    sqlite3_bind_text(stmt, 1, user_id, -1, SQLITE_STATIC);
    sqlite3_bind_int64(stmt, 2, start_ms);
    sqlite3_bind_int64(stmt, 3, end_ms);

    char current_project[128] = {0};
    int item_count = 0;

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const char *proj_name = (const char*)sqlite3_column_text(stmt, 0);
        const char *content = (const char*)sqlite3_column_text(stmt, 1);
        int64_t completed_at = sqlite3_column_int64(stmt, 2);
        const char *vis = (const char*)sqlite3_column_text(stmt, 3);
        const char *tags = (const char*)sqlite3_column_text(stmt, 4);

        if (strcmp(current_project, proj_name ? proj_name : "") != 0) {
            strncpy(current_project, proj_name ? proj_name : "默认项目", sizeof(current_project) - 1);
            char proj_header[256];
            snprintf(proj_header, sizeof(proj_header), "### 📁 项目: %s\n", current_project);
            buf_append(&buf, proj_header);
        }

        char time_str[32];
        format_iso8601(completed_at, time_str, sizeof(time_str));

        char item_line[1024];
        int is_priv = (vis && strcmp(vis, "private") == 0);
        snprintf(item_line, sizeof(item_line), "- [x] **%.10s** %s %s%s\n",
                 time_str,
                 content ? content : "",
                 (tags && strlen(tags) > 0) ? tags : "",
                 is_priv ? " *(🔒私密)*" : "");
        buf_append(&buf, item_line);
        item_count++;
    }

    sqlite3_finalize(stmt);

    if (item_count == 0) {
        buf_append(&buf, "*本周暂无已完成的事项记录。*\n");
    }

    *out_markdown = buf.data;
    return 0;
}

int summary_generate_project(const char *project_id, int64_t start_ms, int64_t end_ms, char **out_markdown) {
    if (!project_id || !out_markdown) return -1;
    sqlite3 *db = db_get_handle();
    if (!db) return -1;

    // Fetch project name
    char proj_name[128] = "项目";
    sqlite3_stmt *stmt = NULL;
    if (sqlite3_prepare_v2(db, "SELECT name FROM projects WHERE id = ?;", -1, &stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, project_id, -1, SQLITE_STATIC);
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            const char *n = (const char*)sqlite3_column_text(stmt, 0);
            if (n) strncpy(proj_name, n, sizeof(proj_name) - 1);
        }
        sqlite3_finalize(stmt);
    }

    StrBuf buf;
    buf_init(&buf);

    char header[256];
    snprintf(header, sizeof(header), "# 项目团队周报: %s\n\n> 团队全员公开进展汇总 (已自动排除私密条目)\n\n", proj_name);
    buf_append(&buf, header);

    // CRUCIAL: Must filter visibility != 'private'
    const char *sql = 
        "SELECT u.name, si.content, si.completed_at, si.tags "
        "FROM schedule_items si "
        "JOIN users u ON si.creator_id = u.id "
        "WHERE si.project_id = ? "
        "AND si.status = 'done' "
        "AND si.visibility != 'private' "
        "AND si.completed_at >= ? AND si.completed_at <= ? "
        "AND si.deleted_at IS NULL "
        "ORDER BY u.name ASC, si.completed_at ASC;";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        free(buf.data);
        return -1;
    }

    sqlite3_bind_text(stmt, 1, project_id, -1, SQLITE_STATIC);
    sqlite3_bind_int64(stmt, 2, start_ms);
    sqlite3_bind_int64(stmt, 3, end_ms);

    char current_member[128] = {0};
    int item_count = 0;

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const char *member_name = (const char*)sqlite3_column_text(stmt, 0);
        const char *content = (const char*)sqlite3_column_text(stmt, 1);
        int64_t completed_at = sqlite3_column_int64(stmt, 2);
        const char *tags = (const char*)sqlite3_column_text(stmt, 3);

        if (strcmp(current_member, member_name ? member_name : "") != 0) {
            strncpy(current_member, member_name ? member_name : "成员", sizeof(current_member) - 1);
            char member_header[256];
            snprintf(member_header, sizeof(member_header), "\n### 👤 成员: %s\n", current_member);
            buf_append(&buf, member_header);
        }

        char time_str[32];
        format_iso8601(completed_at, time_str, sizeof(time_str));

        char item_line[1024];
        snprintf(item_line, sizeof(item_line), "- [x] **%.10s** %s %s\n",
                 time_str,
                 content ? content : "",
                 (tags && strlen(tags) > 0) ? tags : "");
        buf_append(&buf, item_line);
        item_count++;
    }

    sqlite3_finalize(stmt);

    if (item_count == 0) {
        buf_append(&buf, "\n*本周该项目暂无公开完成事项记录。*\n");
    }

    *out_markdown = buf.data;
    return 0;
}

void summary_free(char *markdown) {
    if (markdown) {
        free(markdown);
    }
}
