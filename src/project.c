#include "project.h"
#include "db.h"
#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int project_create(const char *name, const char *desc, char *out_project_id) {
    if (!name || strlen(name) == 0) return -1;
    sqlite3 *db = db_get_handle();
    if (!db) return -1;

    char proj_id[UUID_STR_LEN];
    generate_uuid_v4(proj_id);
    int64_t now = get_current_timestamp_ms();

    const char *sql = "INSERT INTO projects (id, name, description, created_at, updated_at) VALUES (?, ?, ?, ?, ?);";
    sqlite3_stmt *stmt = NULL;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) return rc;

    sqlite3_bind_text(stmt, 1, proj_id, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, name, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, desc ? desc : "", -1, SQLITE_STATIC);
    sqlite3_bind_int64(stmt, 4, now);
    sqlite3_bind_int64(stmt, 5, now);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    if (rc != SQLITE_DONE) return rc;

    // Automatically bind the currently active user as 'owner'
    User current_user;
    if (user_get_current(&current_user) == 0) {
        const char *member_sql = "INSERT INTO project_members (project_id, user_id, role, joined_at) VALUES (?, ?, 'owner', ?);";
        if (sqlite3_prepare_v2(db, member_sql, -1, &stmt, NULL) == SQLITE_OK) {
            sqlite3_bind_text(stmt, 1, proj_id, -1, SQLITE_STATIC);
            sqlite3_bind_text(stmt, 2, current_user.id, -1, SQLITE_STATIC);
            sqlite3_bind_int64(stmt, 3, now);
            sqlite3_step(stmt);
            sqlite3_finalize(stmt);
        }
    }

    if (out_project_id) {
        strncpy(out_project_id, proj_id, UUID_STR_LEN);
    }
    return 0;
}

int project_list(ProjectList *out_list) {
    if (!out_list) return -1;
    sqlite3 *db = db_get_handle();
    if (!db) return -1;

    out_list->items = NULL;
    out_list->count = 0;
    out_list->capacity = 8;
    out_list->items = (Project*)malloc(sizeof(Project) * out_list->capacity);
    if (!out_list->items) return -1;

    const char *sql = 
        "SELECT p.id, p.name, p.description, p.created_at, p.updated_at, count(pm.user_id) "
        "FROM projects p "
        "LEFT JOIN project_members pm ON p.id = pm.project_id "
        "WHERE p.deleted_at IS NULL "
        "GROUP BY p.id "
        "ORDER BY p.created_at ASC;";

    sqlite3_stmt *stmt = NULL;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        free(out_list->items);
        out_list->items = NULL;
        return rc;
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        if (out_list->count >= out_list->capacity) {
            out_list->capacity *= 2;
            Project *new_items = (Project*)realloc(out_list->items, sizeof(Project) * out_list->capacity);
            if (!new_items) break;
            out_list->items = new_items;
        }

        Project *item = &out_list->items[out_list->count];
        memset(item, 0, sizeof(Project));

        const char *id = (const char*)sqlite3_column_text(stmt, 0);
        const char *name = (const char*)sqlite3_column_text(stmt, 1);
        const char *desc = (const char*)sqlite3_column_text(stmt, 2);

        if (id) strncpy(item->id, id, sizeof(item->id) - 1);
        if (name) strncpy(item->name, name, sizeof(item->name) - 1);
        if (desc) strncpy(item->description, desc, sizeof(item->description) - 1);

        item->created_at = sqlite3_column_int64(stmt, 3);
        item->updated_at = sqlite3_column_int64(stmt, 4);
        item->member_count = sqlite3_column_int(stmt, 5);

        out_list->count++;
    }

    sqlite3_finalize(stmt);
    return 0;
}

void project_list_free(ProjectList *list) {
    if (list && list->items) {
        free(list->items);
        list->items = NULL;
        list->count = 0;
        list->capacity = 0;
    }
}

int member_add(const char *project_id, const char *user_name, const char *role, char *out_user_id) {
    if (!project_id || !user_name || strlen(user_name) == 0) return -1;
    sqlite3 *db = db_get_handle();
    if (!db) return -1;

    int64_t now = get_current_timestamp_ms();
    char user_id[UUID_STR_LEN] = {0};

    // Check if user with user_name already exists in users table
    sqlite3_stmt *stmt = NULL;
    const char *find_user_sql = "SELECT id FROM users WHERE name = ? LIMIT 1;";
    if (sqlite3_prepare_v2(db, find_user_sql, -1, &stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, user_name, -1, SQLITE_STATIC);
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            const char *existing_id = (const char*)sqlite3_column_text(stmt, 0);
            if (existing_id) strncpy(user_id, existing_id, sizeof(user_id) - 1);
        }
        sqlite3_finalize(stmt);
    }

    // If user does not exist, create new user
    if (strlen(user_id) == 0) {
        generate_uuid_v4(user_id);
        const char *insert_user = "INSERT INTO users (id, name, avatar, is_current, created_at) VALUES (?, ?, 'default_avatar', 0, ?);";
        if (sqlite3_prepare_v2(db, insert_user, -1, &stmt, NULL) == SQLITE_OK) {
            sqlite3_bind_text(stmt, 1, user_id, -1, SQLITE_STATIC);
            sqlite3_bind_text(stmt, 2, user_name, -1, SQLITE_STATIC);
            sqlite3_bind_int64(stmt, 3, now);
            sqlite3_step(stmt);
            sqlite3_finalize(stmt);
        }
    }

    // Bind to project
    const char *bind_sql = "INSERT OR REPLACE INTO project_members (project_id, user_id, role, joined_at) VALUES (?, ?, ?, ?);";
    if (sqlite3_prepare_v2(db, bind_sql, -1, &stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, project_id, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, user_id, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 3, role ? role : "member", -1, SQLITE_STATIC);
        sqlite3_bind_int64(stmt, 4, now);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
    }

    if (out_user_id) {
        strncpy(out_user_id, user_id, UUID_STR_LEN);
    }
    return 0;
}

int member_list_by_project(const char *project_id, MemberList *out_list) {
    if (!project_id || !out_list) return -1;
    sqlite3 *db = db_get_handle();
    if (!db) return -1;

    out_list->items = NULL;
    out_list->count = 0;
    out_list->capacity = 8;
    out_list->items = (Member*)malloc(sizeof(Member) * out_list->capacity);
    if (!out_list->items) return -1;

    const char *sql = 
        "SELECT u.id, u.name, u.avatar, u.is_current, pm.role, pm.joined_at "
        "FROM project_members pm "
        "JOIN users u ON pm.user_id = u.id "
        "WHERE pm.project_id = ? "
        "ORDER BY pm.joined_at ASC;";

    sqlite3_stmt *stmt = NULL;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        free(out_list->items);
        out_list->items = NULL;
        return rc;
    }

    sqlite3_bind_text(stmt, 1, project_id, -1, SQLITE_STATIC);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        if (out_list->count >= out_list->capacity) {
            out_list->capacity *= 2;
            Member *new_items = (Member*)realloc(out_list->items, sizeof(Member) * out_list->capacity);
            if (!new_items) break;
            out_list->items = new_items;
        }

        Member *m = &out_list->items[out_list->count];
        memset(m, 0, sizeof(Member));

        const char *id = (const char*)sqlite3_column_text(stmt, 0);
        const char *name = (const char*)sqlite3_column_text(stmt, 1);
        const char *avatar = (const char*)sqlite3_column_text(stmt, 2);
        const char *role = (const char*)sqlite3_column_text(stmt, 4);

        if (id) strncpy(m->id, id, sizeof(m->id) - 1);
        if (name) strncpy(m->name, name, sizeof(m->name) - 1);
        if (avatar) strncpy(m->avatar, avatar, sizeof(m->avatar) - 1);
        if (role) strncpy(m->role, role, sizeof(m->role) - 1);

        m->is_current = sqlite3_column_int(stmt, 3);
        m->joined_at = sqlite3_column_int64(stmt, 5);

        out_list->count++;
    }

    sqlite3_finalize(stmt);
    return 0;
}

void member_list_free(MemberList *list) {
    if (list && list->items) {
        free(list->items);
        list->items = NULL;
        list->count = 0;
        list->capacity = 0;
    }
}

int user_set_current(const char *user_id) {
    if (!user_id) return -1;
    sqlite3 *db = db_get_handle();
    if (!db) return -1;

    sqlite3_exec(db, "UPDATE users SET is_current = 0;", NULL, NULL, NULL);

    sqlite3_stmt *stmt = NULL;
    const char *sql = "UPDATE users SET is_current = 1 WHERE id = ?;";
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, user_id, -1, SQLITE_STATIC);
        rc = sqlite3_step(stmt);
        sqlite3_finalize(stmt);
    }
    return (rc == SQLITE_DONE) ? 0 : -1;
}

int user_get_current(User *out_user) {
    if (!out_user) return -1;
    sqlite3 *db = db_get_handle();
    if (!db) return -1;

    memset(out_user, 0, sizeof(User));
    const char *sql = "SELECT id, name, avatar FROM users WHERE is_current = 1 LIMIT 1;";
    sqlite3_stmt *stmt = NULL;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc == SQLITE_OK && sqlite3_step(stmt) == SQLITE_ROW) {
        const char *id = (const char*)sqlite3_column_text(stmt, 0);
        const char *name = (const char*)sqlite3_column_text(stmt, 1);
        const char *avatar = (const char*)sqlite3_column_text(stmt, 2);

        if (id) strncpy(out_user->id, id, sizeof(out_user->id) - 1);
        if (name) strncpy(out_user->name, name, sizeof(out_user->name) - 1);
        if (avatar) strncpy(out_user->avatar, avatar, sizeof(out_user->avatar) - 1);

        sqlite3_finalize(stmt);
        return 0;
    }

    if (stmt) sqlite3_finalize(stmt);
    return -1;
}
