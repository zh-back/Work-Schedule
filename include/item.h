#ifndef ITEM_H
#define ITEM_H

#include "common.h"
#include <stdint.h>

#define VISIBILITY_PRIVATE "private"
#define VISIBILITY_PROJECT_PUBLIC "project_public"

#define STATUS_TODO "todo"
#define STATUS_DONE "done"

typedef struct {
    char id[UUID_STR_LEN];
    char project_id[UUID_STR_LEN];
    char creator_id[UUID_STR_LEN];
    char creator_name[128];
    char content[512];
    char status[16]; // 'todo', 'done'
    int64_t completed_at;
    char visibility[32]; // 'private', 'project_public'
    char tags[256];
    int64_t created_at;
    int64_t updated_at;
} ScheduleItem;

typedef struct {
    ScheduleItem *items;
    int count;
    int capacity;
} ItemList;

// Quickly create a completed item (auto timestamped with current time)
int item_create_done(const char *project_id, const char *user_id, const char *content, int is_private, char *out_item_id);

// Create a todo item
int item_create_todo(const char *project_id, const char *user_id, const char *content, int is_private, char *out_item_id);

// Mark a todo item as completed (records current timestamp as completed_at)
int item_mark_done(const char *item_id);

// Soft delete a schedule item (sets deleted_at to current timestamp)
int item_delete(const char *item_id);

// List active todo items for project and user (pass NULL for project_id/user_id to list all)
int item_list_active_todos(const char *project_id, const char *user_id, ItemList *out_list);

// List completed items for today for project and user
int item_list_today_dones(const char *project_id, const char *user_id, ItemList *out_list);

// Free an item list
void item_list_free(ItemList *list);

// Extract tags like "#tag" from content string into out_tags (comma separated)
void extract_tags_from_content(const char *content, char *out_tags, size_t max_len);

#endif // ITEM_H
