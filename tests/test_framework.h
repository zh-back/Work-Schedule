#ifndef TEST_FRAMEWORK_H
#define TEST_FRAMEWORK_H

#include <stdio.h>
#include <string.h>

static int g_tests_run = 0;
static int g_tests_failed = 0;

#define ASSERT_TRUE(cond) do { \
    if (!(cond)) { \
        printf("  [FAIL] %s:%d: Assertion failed: %s\n", __FILE__, __LINE__, #cond); \
        return 1; \
    } \
} while(0)

#define ASSERT_EQ_INT(expected, actual) do { \
    long long exp = (long long)(expected); \
    long long act = (long long)(actual); \
    if (exp != act) { \
        printf("  [FAIL] %s:%d: Expected %lld, got %lld (%s != %s)\n", __FILE__, __LINE__, exp, act, #expected, #actual); \
        return 1; \
    } \
} while(0)

#define ASSERT_EQ_STR(expected, actual) do { \
    const char *exp = (expected); \
    const char *act = (actual); \
    if (!exp && !act) break; \
    if (!exp || !act || strcmp(exp, act) != 0) { \
        printf("  [FAIL] %s:%d: Expected \"%s\", got \"%s\"\n", __FILE__, __LINE__, exp ? exp : "null", act ? act : "null"); \
        return 1; \
    } \
} while(0)

#define RUN_TEST(test_func) do { \
    g_tests_run++; \
    printf("Running %s ...\n", #test_func); \
    if (test_func() != 0) { \
        g_tests_failed++; \
    } else { \
        printf("  [PASS] %s\n", #test_func); \
    } \
} while(0)

#define TEST_REPORT() do { \
    printf("\n===================================\n"); \
    printf("Tests run: %d, Passed: %d, Failed: %d\n", g_tests_run, g_tests_run - g_tests_failed, g_tests_failed); \
    printf("===================================\n"); \
    return g_tests_failed > 0 ? 1 : 0; \
} while(0)

#endif // TEST_FRAMEWORK_H
