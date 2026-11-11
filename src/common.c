#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#ifdef _WIN32
#include <windows.h>
#include <wincrypt.h>
#else
#include <sys/time.h>
#include <unistd.h>
#endif

void generate_uuid_v4(char *out_uuid) {
    if (!out_uuid) return;

    unsigned char bytes[16];
#ifdef _WIN32
    HCRYPTPROV hCryptProv = 0;
    int acquired = CryptAcquireContext(&hCryptProv, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT);
    if (acquired) {
        CryptGenRandom(hCryptProv, 16, bytes);
        CryptReleaseContext(hCryptProv, 0);
    } else {
        // Fallback to rand() with entropy
        srand((unsigned int)(time(NULL) ^ (intptr_t)out_uuid));
        for (int i = 0; i < 16; i++) {
            bytes[i] = (unsigned char)(rand() % 256);
        }
    }
#else
    arc4random_buf(bytes, 16);
#endif

    // Set version to 0100 (v4)
    bytes[6] = (bytes[6] & 0x0F) | 0x40;
    // Set variant to 10xx (RFC 4122)
    bytes[8] = (bytes[8] & 0x3F) | 0x80;

    snprintf(out_uuid, UUID_STR_LEN,
             "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
             bytes[0], bytes[1], bytes[2], bytes[3],
             bytes[4], bytes[5],
             bytes[6], bytes[7],
             bytes[8], bytes[9],
             bytes[10], bytes[11], bytes[12], bytes[13], bytes[14], bytes[15]);
}

int64_t get_current_timestamp_ms(void) {
#ifdef _WIN32
    FILETIME ft;
    GetSystemTimeAsFileTime(&ft);
    ULARGE_INTEGER uli;
    uli.LowPart = ft.dwLowDateTime;
    uli.HighPart = ft.dwHighDateTime;

    // Convert from Windows Epoch (1601-01-01) to Unix Epoch (1970-01-01):
    // 116444736000000000 100-nanosecond intervals
    const uint64_t EPOCH_DIFF = 116444736000000000ULL;
    if (uli.QuadPart < EPOCH_DIFF) return 0;
    uint64_t unix_100ns = uli.QuadPart - EPOCH_DIFF;
    return (int64_t)(unix_100ns / 10000ULL);
#else
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return ((int64_t)tv.tv_sec * 1000LL) + ((int64_t)tv.tv_usec / 1000LL);
#endif
}

void format_iso8601(int64_t ts_ms, char *out_str, size_t max_len) {
    if (!out_str || max_len < 21) return;

    time_t sec = (time_t)(ts_ms / 1000LL);
    struct tm gm_tm;
#ifdef _WIN32
    gmtime_s(&gm_tm, &sec);
#else
    gmtime_r(&sec, &gm_tm);
#endif

    snprintf(out_str, max_len, "%04d-%02d-%02dT%02d:%02d:%02dZ",
             gm_tm.tm_year + 1900, gm_tm.tm_mon + 1, gm_tm.tm_mday,
             gm_tm.tm_hour, gm_tm.tm_min, gm_tm.tm_sec);
}

void extract_date_str(const char *iso_str, char *out_date, size_t max_len) {
    if (!iso_str || !out_date || max_len < 11) return;
    snprintf(out_date, max_len, "%.10s", iso_str);
}
