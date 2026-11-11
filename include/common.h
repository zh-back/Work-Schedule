#ifndef COMMON_H
#define COMMON_H

#include <stdint.h>
#include <stddef.h>
#include <time.h>

#ifdef _WIN32
#define PORTABLE_LOCALTIME(tm_ptr, time_t_ptr) localtime_s((tm_ptr), (time_t_ptr))
#else
#define PORTABLE_LOCALTIME(tm_ptr, time_t_ptr) localtime_r((time_t_ptr), (tm_ptr))
#endif

#define UUID_STR_LEN 37 // 36 chars + null terminator

// Generate standard UUID v4 string (e.g. "f47ac10b-58cc-4372-a567-0e02b2c3d479")
void generate_uuid_v4(char *out_uuid);

// Get current Unix timestamp in milliseconds
int64_t get_current_timestamp_ms(void);

// Format millisecond timestamp to ISO-8601 string (e.g. "2026-11-09T10:15:00Z")
void format_iso8601(int64_t ts_ms, char *out_str, size_t max_len);

// Extract date part (YYYY-MM-DD) from ISO-8601 string
void extract_date_str(const char *iso_str, char *out_date, size_t max_len);

#endif // COMMON_H
