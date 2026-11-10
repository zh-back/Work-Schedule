#include "test_framework.h"
#include "db.h"
#include "project.h"

int test_project_and_member_lifecycle(void) {
    ASSERT_EQ_INT(0, db_init(":memory:"));

    // 1. Create project
    char proj_id[UUID_STR_LEN];
    int rc = project_create("核心中台 2.0", "业务中台研发", proj_id);
    ASSERT_EQ_INT(0, rc);
    ASSERT_EQ_INT(36, strlen(proj_id));

    // 2. List projects
    ProjectList plist;
    memset(&plist, 0, sizeof(plist));
    rc = project_list(&plist);
    ASSERT_EQ_INT(0, rc);
    ASSERT_TRUE(plist.count >= 2); // Default "个人工作" + "核心中台 2.0"

    int found = 0;
    for (int i = 0; i < plist.count; i++) {
        if (strcmp(plist.items[i].name, "核心中台 2.0") == 0) {
            found = 1;
            ASSERT_EQ_STR("业务中台研发", plist.items[i].description);
        }
    }
    ASSERT_TRUE(found);
    project_list_free(&plist);

    // 3. Add member
    char user_id[UUID_STR_LEN];
    rc = member_add(proj_id, "李四", "member", user_id);
    ASSERT_EQ_INT(0, rc);
    ASSERT_EQ_INT(36, strlen(user_id));

    // 4. List members
    MemberList mlist;
    memset(&mlist, 0, sizeof(mlist));
    rc = member_list_by_project(proj_id, &mlist);
    ASSERT_EQ_INT(0, rc);
    ASSERT_TRUE(mlist.count >= 1);

    int member_found = 0;
    for (int i = 0; i < mlist.count; i++) {
        if (strcmp(mlist.items[i].name, "李四") == 0) {
            member_found = 1;
            ASSERT_EQ_STR("member", mlist.items[i].role);
        }
    }
    ASSERT_TRUE(member_found);
    member_list_free(&mlist);

    // 5. User switch persona
    rc = user_set_current(user_id);
    ASSERT_EQ_INT(0, rc);

    User current_user;
    rc = user_get_current(&current_user);
    ASSERT_EQ_INT(0, rc);
    ASSERT_EQ_STR("李四", current_user.name);

    // 6. Delete project
    rc = project_delete(proj_id);
    ASSERT_EQ_INT(0, rc);
    memset(&plist, 0, sizeof(plist));
    rc = project_list(&plist);
    ASSERT_EQ_INT(0, rc);
    int deleted_found = 0;
    for (int i = 0; i < plist.count; i++) {
        if (strcmp(plist.items[i].id, proj_id) == 0) {
            deleted_found = 1;
        }
    }
    ASSERT_TRUE(!deleted_found);
    project_list_free(&plist);

    db_close();
    return 0;
}

int main(void) {
    printf("=== Running Project & Member Management Tests ===\n");
    RUN_TEST(test_project_and_member_lifecycle);
    TEST_REPORT();
}
