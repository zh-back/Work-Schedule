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

    return 0;
}

int main(void) {
    printf("=== Running GUI Action Router Tests ===\n");
    RUN_TEST(test_gui_action_router);
    TEST_REPORT();
}
