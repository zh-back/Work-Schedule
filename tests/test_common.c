#include "test_framework.h"
#include "common.h"
#include <string.h>

int test_uuid_v4_format(void) {
    char uuid1[UUID_STR_LEN];
    char uuid2[UUID_STR_LEN];
    generate_uuid_v4(uuid1);
    generate_uuid_v4(uuid2);

    ASSERT_EQ_INT(36, strlen(uuid1));
    ASSERT_EQ_INT(36, strlen(uuid2));

    // Hyphens at positions 8, 13, 18, 23
    ASSERT_TRUE(uuid1[8] == '-');
    ASSERT_TRUE(uuid1[13] == '-');
    ASSERT_TRUE(uuid1[18] == '-');
    ASSERT_TRUE(uuid1[23] == '-');

    // Version 4 check: 14th character is '4'
    ASSERT_TRUE(uuid1[14] == '4');

    // Uniqueness
    ASSERT_TRUE(strcmp(uuid1, uuid2) != 0);
    return 0;
}

int test_timestamp_and_iso_format(void) {
    int64_t ts = get_current_timestamp_ms();
    ASSERT_TRUE(ts > 1700000000000LL); // Valid modern timestamp

    char iso[64];
    format_iso8601(ts, iso, sizeof(iso));
    ASSERT_TRUE(strlen(iso) >= 19);
    ASSERT_TRUE(iso[4] == '-');
    ASSERT_TRUE(iso[7] == '-');
    ASSERT_TRUE(iso[10] == 'T');

    char date[16];
    extract_date_str(iso, date, sizeof(date));
    ASSERT_EQ_INT(10, strlen(date));
    ASSERT_TRUE(date[4] == '-');
    ASSERT_TRUE(date[7] == '-');

    return 0;
}

int main(void) {
    printf("=== Running Common Utilities Tests ===\n");
    RUN_TEST(test_uuid_v4_format);
    RUN_TEST(test_timestamp_and_iso_format);
    TEST_REPORT();
}
