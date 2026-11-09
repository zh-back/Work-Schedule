#include "test_framework.h"
#include "tray.h"
#include <windows.h>
#include <stdio.h>

int test_single_instance_guard(void) {
    const char *test_mutex = "Local\\Test_ScheduleWorklog_Mutex";

    // 1. First instance should acquire successfully
    int rc1 = check_single_instance(test_mutex);
    ASSERT_EQ_INT(0, rc1);

    // 2. Second instance attempting to acquire the same mutex should detect collision (-1)
    HANDLE hDup = CreateMutexA(NULL, TRUE, test_mutex);
    ASSERT_TRUE(hDup != NULL);
    ASSERT_EQ_INT(ERROR_ALREADY_EXISTS, GetLastError());
    CloseHandle(hDup);

    // 3. Cleanup primary instance
    single_instance_cleanup();

    // 4. After cleanup, acquiring again should succeed
    int rc2 = check_single_instance(test_mutex);
    ASSERT_EQ_INT(0, rc2);
    single_instance_cleanup();

    return 0;
}

int main(void) {
    printf("=== Running System Tray & Mutex Tests ===\n");
    RUN_TEST(test_single_instance_guard);
    TEST_REPORT();
}
