#ifndef PROJECT_H
#define PROJECT_H

#include "common.h"
#include <stdint.h>

typedef struct {
    char id[UUID_STR_LEN];
    char name[128];
    char description[512];
    int64_t created_at;
    int64_t updated_at;
    int member_count;
} Project;

typedef struct {
    Project *items;
    int count;
    int capacity;
} ProjectList;

typedef struct {
    char id[UUID_STR_LEN];
    char name[128];
    char avatar[128];
    char role[32]; // 'owner', 'admin', 'member'
    int is_current;
    int64_t joined_at;
} Member;

typedef struct {
    Member *items;
    int count;
    int capacity;
} MemberList;

typedef struct {
    char id[UUID_STR_LEN];
    char name[128];
    char avatar[128];
} User;

// Create a new project
int project_create(const char *name, const char *desc, char *out_project_id);

// Delete (soft-delete) a project
int project_delete(const char *project_id);

// List all active (non-deleted) projects
int project_list(ProjectList *out_list);
void project_list_free(ProjectList *list);

// Add or bind a member to a project
int member_add(const char *project_id, const char *user_name, const char *role, char *out_user_id);

// List all members of a project
int member_list_by_project(const char *project_id, MemberList *out_list);
void member_list_free(MemberList *list);

// Set the currently active local user (single-machine persona switch)
int user_set_current(const char *user_id);

// Get currently active user
int user_get_current(User *out_user);

#endif // PROJECT_H
