#include "test_framework.h"
#include "db.h"
#include "project.h"
#include "item.h"
#include "summary.h"
#include <string.h>

int test_weekly_summary_and_privacy_filter(void) {
    ASSERT_EQ_INT(0, db_init(":memory:"));

    int year, week;
    summary_get_current_week(&year, &week);
    int64_t start_ms, end_ms;
    ASSERT_EQ_INT(0, summary_get_week_range(year, week, &start_ms, &end_ms));
    ASSERT_TRUE(start_ms < end_ms);
    ASSERT_TRUE((end_ms - start_ms) >= 6 * 24 * 3600 * 1000LL);

    // Create a project
    char proj_id[UUID_STR_LEN];
    ASSERT_EQ_INT(0, project_create("核心中台 2.0", "业务中台", proj_id));

    // Create 2 users
    char user_a[UUID_STR_LEN];
    char user_b[UUID_STR_LEN];
    ASSERT_EQ_INT(0, member_add(proj_id, "张三", "owner", user_a));
    ASSERT_EQ_INT(0, member_add(proj_id, "李四", "member", user_b));

    // 1. User A adds a public task
    char item_a1[UUID_STR_LEN];
    ASSERT_EQ_INT(0, item_create_done(proj_id, user_a, "完成了中台认证接口联调 #后端", 0, item_a1));

    // 2. User A adds a PRIVATE task
    char item_a2[UUID_STR_LEN];
    ASSERT_EQ_INT(0, item_create_done(proj_id, user_a, "准备下周个人晋升答辩材料 #个人", 1, item_a2));

    // 3. User B adds a public task
    char item_b1[UUID_STR_LEN];
    ASSERT_EQ_INT(0, item_create_done(proj_id, user_b, "搭建前端基础组件库 #前端", 0, item_b1));

    // 4. Test Personal Summary for User A
    char *personal_md = NULL;
    ASSERT_EQ_INT(0, summary_generate_personal(user_a, start_ms, end_ms, &personal_md));
    ASSERT_TRUE(personal_md != NULL);
    // User A sees both public AND private items in their personal summary
    ASSERT_TRUE(strstr(personal_md, "完成了中台认证接口联调") != NULL);
    ASSERT_TRUE(strstr(personal_md, "准备下周个人晋升答辩材料") != NULL);
    // User A does not see User B's items in their personal summary
    ASSERT_TRUE(strstr(personal_md, "搭建前端基础组件库") == NULL);
    summary_free(personal_md);

    // 5. Test Project Summary for Project 1 (CRUCIAL PRIVACY FILTER TEST)
    char *project_md = NULL;
    ASSERT_EQ_INT(0, summary_generate_project(proj_id, start_ms, end_ms, &project_md));
    ASSERT_TRUE(project_md != NULL);
    // Project summary contains User A's public task
    ASSERT_TRUE(strstr(project_md, "完成了中台认证接口联调") != NULL);
    // Project summary contains User B's public task
    ASSERT_TRUE(strstr(project_md, "搭建前端基础组件库") != NULL);
    // Project summary MUST NOT leak User A's private task!
    ASSERT_TRUE(strstr(project_md, "准备下周个人晋升答辩材料") == NULL);
    summary_free(project_md);

    db_close();
    return 0;
}

int main(void) {
    printf("=== Running Weekly Summary & Privacy Tests ===\n");
    RUN_TEST(test_weekly_summary_and_privacy_filter);
    TEST_REPORT();
}
