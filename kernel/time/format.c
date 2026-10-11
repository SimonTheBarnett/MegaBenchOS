/**
 * @file kernel/time/format.c
 * @brief Bounded integer/session-time formatters implementing the
 * public SDK.
 *
 * OS service implementation behind the public SDK; apps use megabenchos.h.
 *
 * Freestanding C without heap, RTC or libc; identical code runs in target and host tests.
 * Caller
 * owns output buffers and must provide capacity including the terminating NUL.
 */
#include "megabenchos.h"
MB_Result mb_format_u32(uint32_t value, char *out, uint16_t capacity) {
    char reversed[10];
    uint16_t n = 0, i;
    if (!out || !capacity)
        return MB_INVALID;
    do {
        reversed[n++] = (char)('0' + value % 10);
        value /= 10;
    } while (value);
    if (capacity <= n) {
        out[0] = 0;
        return MB_INVALID;
    }
    for (i = 0; i < n; i++)
        out[i] = reversed[n - i - 1];
    out[n] = 0;
    return MB_OK;
}
MB_Result mb_format_i32(int32_t value, char *out, uint16_t capacity) {
    uint32_t magnitude = (uint32_t)value;
    if (!out || !capacity)
        return MB_INVALID;
    if (value < 0) {
        if (capacity < 2) {
            out[0] = 0;
            return MB_INVALID;
        }
        out[0] = '-';
        magnitude = 0U - magnitude;
        if (mb_format_u32(magnitude, out + 1, capacity - 1) != MB_OK) {
            out[0] = 0;
            return MB_INVALID;
        }
        return MB_OK;
    }
    return mb_format_u32(magnitude, out, capacity);
}
MB_Result mb_time_format(uint32_t seconds, char *out, uint16_t capacity) {
    char hours[11];
    uint16_t len = 0, i, minutes = (seconds / 60) % 60, secs = seconds % 60;
    if (!out || !capacity)
        return MB_INVALID;
    mb_format_u32(seconds / 3600, hours, sizeof(hours));
    while (hours[len])
        ++len;
    if (capacity < ((len < 2 ? 2 : len) + 7)) {
        out[0] = 0;
        return MB_INVALID;
    }
    i = 0;
    if (len == 1)
        out[i++] = '0';
    {
        uint16_t j;
        for (j = 0; j < len; j++)
            out[i++] = hours[j];
    }
    out[i++] = ':';
    out[i++] = (char)('0' + minutes / 10);
    out[i++] = (char)('0' + minutes % 10);
    out[i++] = ':';
    out[i++] = (char)('0' + secs / 10);
    out[i++] = (char)('0' + secs % 10);
    out[i] = 0;
    return MB_OK;
}
