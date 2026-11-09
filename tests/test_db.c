#include "test_framework.h"
#include "db.h"

static int check_table_exists(const char *table_name) {
    sqlite3 *db = db_get_handle();
    const char *sql = "SELECT count(*) FROM sqlite_master WHERE type='table' AND name=?;";
    sqlite3_stmt *stmt = NULL;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) return 0;

    sqlite3_bind_text(stmt, 1, table_name, -1, SQLITE_STATIC);
    int exists = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        exists = sqlite3_column_int(stmt, 0);
    }
    sqlite3_finalize(stmt);
    return exists;
}

int test_db_init_and_tables(void) {
    int rc = db_init(":memory:");
    ASSERT_EQ_INT(0, rc);
    ASSERT_TRUE(db_get_handle() != NULL);

    // Verify all four core domain tables exist
    ASSERT_TRUE(check_table_exists("users"));
    ASSERT_TRUE(check_table_exists("projects"));
    ASSERT_TRUE(check_table_exists("project_members"));
    ASSERT_TRUE(check_table_exists("schedule_items"));

    // Verify default project is auto-initialized
    sqlite3 *db = db_get_handle();
    sqlite3_stmt *stmt = NULL;
    rc = sqlite3_prepare_v2(db, "SELECT count(*) FROM projects WHERE name='个人工作';", -1, &stmt, NULL);
    ASSERT_EQ_INT(SQLITE_OK, rc);
    ASSERT_EQ_INT(SQLITE_ROW, sqlite3_step(stmt));
    ASSERT_EQ_INT(1, sqlite3_column_int(stmt, 0));
    sqlite3_finalize(stmt);

    db_close();
    ASSERT_TRUE(db_get_handle() == NULL);
    return 0;
}

int main(void) {
    printf("=== Running Database Storage Tests ===\n");
    RUN_TEST(test_db_init_and_tables);
    TEST_REPORT();
}
