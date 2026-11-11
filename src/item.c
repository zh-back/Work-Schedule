#include "item.h"
#include "db.h"
#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

void extract_tags_from_content(const char *content, char *out_tags, size_t max_len) {
    if (!out_tags || max_len == 0) return;
    out_tags[0] = '\0';
    if (!content) return;

    const char *p = content;
    size_t current_len = 0;

    while (*p) {
        if (*p == '#') {
            p++;
            const char *start = p;
            while (*p && !isspace((unsigned char)*p) && *p != '#' && *p != ',' && *p != ';') {
                p++;
            }
            size_t tag_len = p - start;
            if (tag_len > 0) {
                if (current_len > 0 && current_len + 1 < max_len) {
                    out_tags[current_len++] = ',';
                    out_tags[current_len] = '\0';
                }
                if (current_len + tag_len < max_len) {
                    strncat(out_tags, start, tag_len);
                    current_len += tag_len;
                }
            }
        } else {
            p++;
        }
    }
}

int item_create_done_ext(const char *project_id, const char *user_id, const char *content, int is_private, int64_t created_at, int64_t completed_at, char *out_item_id) {
    if (!project_id || !user_id || !content || strlen(content) == 0) return -1;
    sqlite3 *db = db_get_handle();
    if (!db) return -1;

    char item_id[UUID_STR_LEN];
    generate_uuid_v4(item_id);
    int64_t now = get_current_timestamp_ms();
    int64_t c_at = (created_at > 0) ? created_at : now;
    int64_t comp_at = (completed_at > 0) ? completed_at : now;

    char tags[256];
    extract_tags_from_content(content, tags, sizeof(tags));

    const char *vis = is_private ? VISIBILITY_PRIVATE : VISIBILITY_PROJECT_PUBLIC;

    const char *sql = 
        "INSERT INTO schedule_items (id, project_id, creator_id, content, status, completed_at, visibility, tags, created_at, updated_at) "
        "VALUES (?, ?, ?, ?, 'done', ?, ?, ?, ?, ?);";

    sqlite3_stmt *stmt = NULL;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) return rc;

    sqlite3_bind_text(stmt, 1, item_id, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, project_id, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, user_id, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, content, -1, SQLITE_STATIC);
    sqlite3_bind_int64(stmt, 5, comp_at); // completed_at
    sqlite3_bind_text(stmt, 6, vis, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 7, tags, -1, SQLITE_STATIC);
    sqlite3_bind_int64(stmt, 8, c_at);
    sqlite3_bind_int64(stmt, 9, now);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    if (rc != SQLITE_DONE) return rc;

    if (out_item_id) {
        strncpy(out_item_id, item_id, UUID_STR_LEN);
    }
    return 0;
}

int item_create_done(const char *project_id, const char *user_id, const char *content, int is_private, char *out_item_id) {
    return item_create_done_ext(project_id, user_id, content, is_private, 0, 0, out_item_id);
}

int item_create_todo_ext(const char *project_id, const char *user_id, const char *content, int is_private, int64_t created_at, char *out_item_id) {
    if (!project_id || !user_id || !content || strlen(content) == 0) return -1;
    sqlite3 *db = db_get_handle();
    if (!db) return -1;

    char item_id[UUID_STR_LEN];
    generate_uuid_v4(item_id);
    int64_t now = get_current_timestamp_ms();
    int64_t c_at = (created_at > 0) ? created_at : now;

    char tags[256];
    extract_tags_from_content(content, tags, sizeof(tags));

    const char *vis = is_private ? VISIBILITY_PRIVATE : VISIBILITY_PROJECT_PUBLIC;

    const char *sql = 
        "INSERT INTO schedule_items (id, project_id, creator_id, content, status, completed_at, visibility, tags, created_at, updated_at) "
        "VALUES (?, ?, ?, ?, 'todo', NULL, ?, ?, ?, ?);";

    sqlite3_stmt *stmt = NULL;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) return rc;

    sqlite3_bind_text(stmt, 1, item_id, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, project_id, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, user_id, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, content, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 5, vis, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 6, tags, -1, SQLITE_STATIC);
    sqlite3_bind_int64(stmt, 7, c_at);
    sqlite3_bind_int64(stmt, 8, now);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    if (rc != SQLITE_DONE) return rc;

    if (out_item_id) {
        strncpy(out_item_id, item_id, UUID_STR_LEN);
    }
    return 0;
}

int item_create_todo(const char *project_id, const char *user_id, const char *content, int is_private, char *out_item_id) {
    return item_create_todo_ext(project_id, user_id, content, is_private, 0, out_item_id);
}

int item_mark_done(const char *item_id) {
    if (!item_id) return -1;
    sqlite3 *db = db_get_handle();
    if (!db) return -1;

    int64_t now = get_current_timestamp_ms();
    const char *sql = "UPDATE schedule_items SET status = 'done', completed_at = ?, updated_at = ? WHERE id = ?;";

    sqlite3_stmt *stmt = NULL;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) return rc;

    sqlite3_bind_int64(stmt, 1, now);
    sqlite3_bind_int64(stmt, 2, now);
    sqlite3_bind_text(stmt, 3, item_id, -1, SQLITE_STATIC);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return (rc == SQLITE_DONE) ? 0 : -1;
}

int item_mark_todo(const char *item_id) {
    if (!item_id) return -1;
    sqlite3 *db = db_get_handle();
    if (!db) return -1;

    int64_t now = get_current_timestamp_ms();
    const char *sql = "UPDATE schedule_items SET status = 'todo', completed_at = NULL, updated_at = ? WHERE id = ?;";

    sqlite3_stmt *stmt = NULL;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) return rc;

    sqlite3_bind_int64(stmt, 1, now);
    sqlite3_bind_text(stmt, 2, item_id, -1, SQLITE_STATIC);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return (rc == SQLITE_DONE) ? 0 : -1;
}

int item_delete(const char *item_id) {
    if (!item_id) return -1;
    sqlite3 *db = db_get_handle();
    if (!db) return -1;

    int64_t now = get_current_timestamp_ms();
    const char *sql = "UPDATE schedule_items SET deleted_at = ?, updated_at = ? WHERE id = ?;";

    sqlite3_stmt *stmt = NULL;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) return rc;

    sqlite3_bind_int64(stmt, 1, now);
    sqlite3_bind_int64(stmt, 2, now);
    sqlite3_bind_text(stmt, 3, item_id, -1, SQLITE_STATIC);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return (rc == SQLITE_DONE) ? 0 : -1;
}

static int parse_items_from_stmt(sqlite3_stmt *stmt, ItemList *out_list) {
    out_list->items = NULL;
    out_list->count = 0;
    out_list->capacity = 16;
    out_list->items = (ScheduleItem*)malloc(sizeof(ScheduleItem) * out_list->capacity);
    if (!out_list->items) return -1;

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        if (out_list->count >= out_list->capacity) {
            out_list->capacity *= 2;
            ScheduleItem *new_items = (ScheduleItem*)realloc(out_list->items, sizeof(ScheduleItem) * out_list->capacity);
            if (!new_items) break;
            out_list->items = new_items;
        }

        ScheduleItem *item = &out_list->items[out_list->count];
        memset(item, 0, sizeof(ScheduleItem));

        const char *id = (const char*)sqlite3_column_text(stmt, 0);
        const char *proj_id = (const char*)sqlite3_column_text(stmt, 1);
        const char *creator_id = (const char*)sqlite3_column_text(stmt, 2);
        const char *creator_name = (const char*)sqlite3_column_text(stmt, 3);
        const char *content = (const char*)sqlite3_column_text(stmt, 4);
        const char *status = (const char*)sqlite3_column_text(stmt, 5);
        const char *vis = (const char*)sqlite3_column_text(stmt, 7);
        const char *tags = (const char*)sqlite3_column_text(stmt, 8);

        if (id) strncpy(item->id, id, sizeof(item->id) - 1);
        if (proj_id) strncpy(item->project_id, proj_id, sizeof(item->project_id) - 1);
        if (creator_id) strncpy(item->creator_id, creator_id, sizeof(item->creator_id) - 1);
        if (creator_name) strncpy(item->creator_name, creator_name, sizeof(item->creator_name) - 1);
        if (content) strncpy(item->content, content, sizeof(item->content) - 1);
        if (status) strncpy(item->status, status, sizeof(item->status) - 1);
        if (vis) strncpy(item->visibility, vis, sizeof(item->visibility) - 1);
        if (tags) strncpy(item->tags, tags, sizeof(item->tags) - 1);

        item->completed_at = sqlite3_column_int64(stmt, 6);
        item->created_at = sqlite3_column_int64(stmt, 9);
        item->updated_at = sqlite3_column_int64(stmt, 10);

        out_list->count++;
    }

    return 0;
}

int item_list_active_todos(const char *project_id, const char *user_id, ItemList *out_list) {
    if (!out_list) return -1;
    sqlite3 *db = db_get_handle();
    if (!db) return -1;

    char sql[512];
    snprintf(sql, sizeof(sql),
        "SELECT si.id, si.project_id, si.creator_id, u.name, si.content, si.status, si.completed_at, si.visibility, si.tags, si.created_at, si.updated_at "
        "FROM schedule_items si "
        "JOIN users u ON si.creator_id = u.id "
        "WHERE si.status = 'todo' AND si.deleted_at IS NULL "
        "%s%s "
        "%s%s "
        "ORDER BY si.created_at DESC;",
        project_id ? "AND si.project_id = '" : "", project_id ? project_id : "",
        user_id ? "AND si.creator_id = '" : "", user_id ? user_id : "");

    // Clean query builder
    sqlite3_stmt *stmt = NULL;
    int rc = sqlite3_prepare_v2(db, 
        "SELECT si.id, si.project_id, si.creator_id, u.name, si.content, si.status, si.completed_at, si.visibility, si.tags, si.created_at, si.updated_at "
        "FROM schedule_items si "
        "JOIN users u ON si.creator_id = u.id "
        "WHERE si.status = 'todo' AND si.deleted_at IS NULL "
        "AND (? IS NULL OR si.project_id = ?) "
        "AND (? IS NULL OR si.creator_id = ?) "
        "ORDER BY si.created_at DESC;", -1, &stmt, NULL);

    if (rc != SQLITE_OK) return rc;

    if (project_id) {
        sqlite3_bind_text(stmt, 1, project_id, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, project_id, -1, SQLITE_STATIC);
    } else {
        sqlite3_bind_null(stmt, 1);
        sqlite3_bind_null(stmt, 2);
    }

    if (user_id) {
        sqlite3_bind_text(stmt, 3, user_id, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 4, user_id, -1, SQLITE_STATIC);
    } else {
        sqlite3_bind_null(stmt, 3);
        sqlite3_bind_null(stmt, 4);
    }

    rc = parse_items_from_stmt(stmt, out_list);
    sqlite3_finalize(stmt);
    return rc;
}

int item_list_dones_by_date_range(const char *project_id, const char *user_id, int64_t start_ms, int64_t end_ms, ItemList *out_list) {
    if (!out_list) return -1;
    sqlite3 *db = db_get_handle();
    if (!db) return -1;

    sqlite3_stmt *stmt = NULL;
    int rc = sqlite3_prepare_v2(db, 
        "SELECT si.id, si.project_id, si.creator_id, u.name, si.content, si.status, si.completed_at, si.visibility, si.tags, si.created_at, si.updated_at "
        "FROM schedule_items si "
        "JOIN users u ON si.creator_id = u.id "
        "WHERE si.status = 'done' AND si.deleted_at IS NULL "
        "AND (? <= 0 OR si.completed_at >= ?) "
        "AND (? <= 0 OR si.completed_at <= ?) "
        "AND (? IS NULL OR si.project_id = ?) "
        "AND (? IS NULL OR si.creator_id = ?) "
        "ORDER BY si.completed_at DESC;", -1, &stmt, NULL);

    if (rc != SQLITE_OK) return rc;

    sqlite3_bind_int64(stmt, 1, start_ms);
    sqlite3_bind_int64(stmt, 2, start_ms);
    sqlite3_bind_int64(stmt, 3, end_ms);
    sqlite3_bind_int64(stmt, 4, end_ms);

    if (project_id) {
        sqlite3_bind_text(stmt, 5, project_id, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 6, project_id, -1, SQLITE_STATIC);
    } else {
        sqlite3_bind_null(stmt, 5);
        sqlite3_bind_null(stmt, 6);
    }

    if (user_id) {
        sqlite3_bind_text(stmt, 7, user_id, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 8, user_id, -1, SQLITE_STATIC);
    } else {
        sqlite3_bind_null(stmt, 7);
        sqlite3_bind_null(stmt, 8);
    }

    rc = parse_items_from_stmt(stmt, out_list);
    sqlite3_finalize(stmt);
    return rc;
}

int item_list_today_dones(const char *project_id, const char *user_id, ItemList *out_list) {
    int64_t now = get_current_timestamp_ms();
    time_t sec = (time_t)(now / 1000LL);
    struct tm l_tm;
    PORTABLE_LOCALTIME(&l_tm, &sec);
    l_tm.tm_hour = 0;
    l_tm.tm_min = 0;
    l_tm.tm_sec = 0;
    int64_t today_start_ms = (int64_t)mktime(&l_tm) * 1000LL;

    return item_list_dones_by_date_range(project_id, user_id, today_start_ms, 0, out_list);
}

void item_list_free(ItemList *list) {
    if (list && list->items) {
        free(list->items);
        list->items = NULL;
        list->count = 0;
        list->capacity = 0;
    }
}
