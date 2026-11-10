#include "test_framework.h"
#include "gui.h"
#include <windows.h>
#include <stdio.h>

int test_gui_action_router(void) {
    // Test unrecognized action
    ASSERT_EQ_INT(-1, gui_handle_action(NULL, "unknown_command"));

    // Test recognized actions (NULL hwnd is handled safely without crashing)
    ASSERT_EQ_INT(0, gui_handle_action(NULL, "drag"));
    ASSERT_EQ_INT(0, gui_handle_action(NULL, "minimize"));
    ASSERT_EQ_INT(0, gui_handle_action(NULL, "maximize"));
    ASSERT_EQ_INT(0, gui_handle_action(NULL, "close"));
    ASSERT_EQ_INT(0, gui_handle_action(NULL, "resize_left"));
    ASSERT_EQ_INT(0, gui_handle_action(NULL, "resize_right"));
    ASSERT_EQ_INT(0, gui_handle_action(NULL, "resize_top"));
    ASSERT_EQ_INT(0, gui_handle_action(NULL, "resize_bottom"));
    ASSERT_EQ_INT(0, gui_handle_action(NULL, "resize_topleft"));
    ASSERT_EQ_INT(0, gui_handle_action(NULL, "resize_topright"));
    ASSERT_EQ_INT(0, gui_handle_action(NULL, "resize_bottomleft"));
    ASSERT_EQ_INT(0, gui_handle_action(NULL, "resize_bottomright"));

    return 0;
}

int main(void) {
    printf("=== Running GUI Action Router Tests ===\n");
    RUN_TEST(test_gui_action_router);
    TEST_REPORT();
}
