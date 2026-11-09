#include "db.h"
#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static sqlite3 *g_db = NULL;

sqlite3* db_get_handle(void) {
    return g_db;
}

void db_close(void) {
    if (g_db) {
        sqlite3_close(g_db);
        g_db = NULL;
    }
}

int db_exec_migration(void) {
    if (!g_db) return -1;

    const char *ddl =
        // 1. Users table
        "CREATE TABLE IF NOT EXISTS users ("
        "  id TEXT PRIMARY KEY,"
        "  name TEXT NOT NULL,"
        "  avatar TEXT,"
        "  is_current INTEGER DEFAULT 0,"
        "  created_at INTEGER NOT NULL"
        ");"

        // 2. Projects table
        "CREATE TABLE IF NOT EXISTS projects ("
        "  id TEXT PRIMARY KEY,"
        "  name TEXT NOT NULL,"
        "  description TEXT,"
        "  created_at INTEGER NOT NULL,"
        "  updated_at INTEGER NOT NULL,"
        "  deleted_at INTEGER DEFAULT NULL"
        ");"

        // 3. Project Members table
        "CREATE TABLE IF NOT EXISTS project_members ("
        "  project_id TEXT NOT NULL,"
        "  user_id TEXT NOT NULL,"
        "  role TEXT NOT NULL,"
        "  joined_at INTEGER NOT NULL,"
        "  PRIMARY KEY (project_id, user_id)"
        ");"

        // 4. Schedule Items table
        "CREATE TABLE IF NOT EXISTS schedule_items ("
        "  id TEXT PRIMARY KEY,"
        "  project_id TEXT NOT NULL,"
        "  creator_id TEXT NOT NULL,"
        "  content TEXT NOT NULL,"
        "  status TEXT NOT NULL," // 'todo' or 'done'
        "  completed_at INTEGER DEFAULT NULL,"
        "  visibility TEXT NOT NULL DEFAULT 'project_public'," // 'private' or 'project_public'
        "  tags TEXT," // JSON string or comma-separated tags
        "  created_at INTEGER NOT NULL,"
        "  updated_at INTEGER NOT NULL,"
        "  deleted_at INTEGER DEFAULT NULL"
        ");";

    char *err_msg = NULL;
    int rc = sqlite3_exec(g_db, ddl, NULL, NULL, &err_msg);
    if (rc != SQLITE_OK) {
        if (err_msg) {
            fprintf(stderr, "SQLite migration error: %s\n", err_msg);
            sqlite3_free(err_msg);
        }
        return rc;
    }

    // Auto-seed default user if none exists
    sqlite3_stmt *stmt = NULL;
    rc = sqlite3_prepare_v2(g_db, "SELECT count(*) FROM users;", -1, &stmt, NULL);
    if (rc == SQLITE_OK && sqlite3_step(stmt) == SQLITE_ROW) {
        int count = sqlite3_column_int(stmt, 0);
        sqlite3_finalize(stmt);
        if (count == 0) {
            char user_id[UUID_STR_LEN];
            generate_uuid_v4(user_id);
            int64_t now = get_current_timestamp_ms();

            const char *insert_user = "INSERT INTO users (id, name, avatar, is_current, created_at) VALUES (?, ?, ?, 1, ?);";
            if (sqlite3_prepare_v2(g_db, insert_user, -1, &stmt, NULL) == SQLITE_OK) {
                sqlite3_bind_text(stmt, 1, user_id, -1, SQLITE_STATIC);
                sqlite3_bind_text(stmt, 2, "默认用户", -1, SQLITE_STATIC);
                sqlite3_bind_text(stmt, 3, "default_avatar", -1, SQLITE_STATIC);
                sqlite3_bind_int64(stmt, 4, now);
                sqlite3_step(stmt);
                sqlite3_finalize(stmt);
            }

            // Auto-seed default "个人工作" project
            char proj_id[UUID_STR_LEN];
            generate_uuid_v4(proj_id);
            const char *insert_proj = "INSERT INTO projects (id, name, description, created_at, updated_at) VALUES (?, ?, ?, ?, ?);";
            if (sqlite3_prepare_v2(g_db, insert_proj, -1, &stmt, NULL) == SQLITE_OK) {
                sqlite3_bind_text(stmt, 1, proj_id, -1, SQLITE_STATIC);
                sqlite3_bind_text(stmt, 2, "个人工作", -1, SQLITE_STATIC);
                sqlite3_bind_text(stmt, 3, "默认个人空间，用于记录个人独立日常事务", -1, SQLITE_STATIC);
                sqlite3_bind_int64(stmt, 4, now);
                sqlite3_bind_int64(stmt, 5, now);
                sqlite3_step(stmt);
                sqlite3_finalize(stmt);
            }

            // Bind user to default project
            const char *insert_member = "INSERT INTO project_members (project_id, user_id, role, joined_at) VALUES (?, ?, 'owner', ?);";
            if (sqlite3_prepare_v2(g_db, insert_member, -1, &stmt, NULL) == SQLITE_OK) {
                sqlite3_bind_text(stmt, 1, proj_id, -1, SQLITE_STATIC);
                sqlite3_bind_text(stmt, 2, user_id, -1, SQLITE_STATIC);
                sqlite3_bind_int64(stmt, 3, now);
                sqlite3_step(stmt);
                sqlite3_finalize(stmt);
            }
        }
    } else if (stmt) {
        sqlite3_finalize(stmt);
    }

    return 0;
}

int db_init(const char *db_path) {
    db_close();

    const char *path = db_path ? db_path : "schedule_worklog.db";
    int rc = sqlite3_open(path, &g_db);
    if (rc != SQLITE_OK) {
        if (g_db) {
            fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(g_db));
            sqlite3_close(g_db);
            g_db = NULL;
        }
        return rc;
    }

    // Enable WAL mode and foreign keys for high performance
    sqlite3_exec(g_db, "PRAGMA foreign_keys = ON;", NULL, NULL, NULL);
    if (strcmp(path, ":memory:") != 0) {
        sqlite3_exec(g_db, "PRAGMA journal_mode = WAL;", NULL, NULL, NULL);
    }

    return db_exec_migration();
}
