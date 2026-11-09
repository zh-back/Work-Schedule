#ifndef DB_H
#define DB_H

#include "sqlite3.h"

// Initialize database connection and execute automatic schema migrations
// Passing ":memory:" or NULL creates in-memory database for testing
int db_init(const char *db_path);

// Close database connection
void db_close(void);

// Get raw sqlite3 database handle
sqlite3* db_get_handle(void);

// Execute schema migrations (create users, projects, project_members, schedule_items tables)
int db_exec_migration(void);

#endif // DB_H
