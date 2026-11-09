#include "test_framework.h"
#include "db.h"
#include "project.h"
#include "item.h"

int test_schedule_item_flow(void) {
    ASSERT_EQ_INT(0, db_init(":memory:"));

    User current_user;
    ASSERT_EQ_INT(0, user_get_current(&current_user));

    ProjectList plist;
    ASSERT_EQ_INT(0, project_list(&plist));
    ASSERT_TRUE(plist.count > 0);
    const char *proj_id = plist.items[0].id; // Use default "个人工作"

    // 1. Tag extraction check
    char tags[128];
    extract_tags_from_content("完成登录接口联调 #后端开发 #接口对接", tags, sizeof(tags));
    ASSERT_TRUE(strstr(tags, "后端开发") != NULL);
    ASSERT_TRUE(strstr(tags, "接口对接") != NULL);

    // 2. Quickly record a done item (with private flag)
    char done_id[UUID_STR_LEN];
    int rc = item_create_done(proj_id, current_user.id, "优化首屏加载性能 #性能优化", 1, done_id);
    ASSERT_EQ_INT(0, rc);
    ASSERT_EQ_INT(36, strlen(done_id));

    // 3. Create a todo item
    char todo_id[UUID_STR_LEN];
    rc = item_create_todo(proj_id, current_user.id, "准备下午技术方案评审", 0, todo_id);
    ASSERT_EQ_INT(0, rc);
    ASSERT_EQ_INT(36, strlen(todo_id));

    // 4. Verify todo list contains 1 item
    ItemList todos;
    memset(&todos, 0, sizeof(todos));
    rc = item_list_active_todos(proj_id, current_user.id, &todos);
    ASSERT_EQ_INT(0, rc);
    ASSERT_EQ_INT(1, todos.count);
    ASSERT_EQ_STR("准备下午技术方案评审", todos.items[0].content);
    ASSERT_EQ_STR("todo", todos.items[0].status);
    item_list_free(&todos);

    // 5. Mark todo as completed (transitions to done with timestamp)
    rc = item_mark_done(todo_id);
    ASSERT_EQ_INT(0, rc);

    // 6. Verify active todos is now 0
    rc = item_list_active_todos(proj_id, current_user.id, &todos);
    ASSERT_EQ_INT(0, rc);
    ASSERT_EQ_INT(0, todos.count);
    item_list_free(&todos);

    // 7. Verify today dones now has 2 items
    ItemList dones;
    memset(&dones, 0, sizeof(dones));
    rc = item_list_today_dones(proj_id, current_user.id, &dones);
    ASSERT_EQ_INT(0, rc);
    ASSERT_EQ_INT(2, dones.count);

    // Verify private visibility flag is preserved
    int found_private = 0;
    for (int i = 0; i < dones.count; i++) {
        if (strcmp(dones.items[i].id, done_id) == 0) {
            ASSERT_EQ_STR(VISIBILITY_PRIVATE, dones.items[i].visibility);
            ASSERT_TRUE(dones.items[i].completed_at > 0);
            found_private = 1;
        }
    }
    ASSERT_TRUE(found_private);
    item_list_free(&dones);

    // 8. Test deletion of an item
    rc = item_delete(done_id);
    ASSERT_EQ_INT(0, rc);

    // 9. Verify today dones now has only 1 item remaining
    memset(&dones, 0, sizeof(dones));
    rc = item_list_today_dones(proj_id, current_user.id, &dones);
    ASSERT_EQ_INT(0, rc);
    ASSERT_EQ_INT(1, dones.count);
    ASSERT_EQ_STR(todo_id, dones.items[0].id);
    item_list_free(&dones);

    project_list_free(&plist);
    db_close();
    return 0;
}

int main(void) {
    printf("=== Running Schedule Item & Flow Tests ===\n");
    RUN_TEST(test_schedule_item_flow);
    TEST_REPORT();
}
