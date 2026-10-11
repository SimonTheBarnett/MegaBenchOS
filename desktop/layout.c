/**
 * @file desktop/layout.c
 * @brief Pure bounded shell geometry and elapsed-time escape policy; no SGDK dependency.
 *
 * Shared desktop/window implementation; apps reach it through the public SDK.
 * Shared by controller polling and privileged target diagnostics. Time uses 1/300 s.
 */
#include "internal.h"
uint16_t mb_grid_capacity(uint16_t width, uint16_t height) {
    return (width / 10) * ((height > 2 ? height - 2 : 0) / 6);
}
uint16_t mb_escape_step(uint16_t *elapsed, uint16_t buttons, uint16_t hz, uint32_t frames) {
    uint32_t next;
    if ((buttons & (MB_KEY_START | MB_KEY_B)) != (MB_KEY_START | MB_KEY_B)) {
        *elapsed = 0;
        return 0;
    }
    next = *elapsed + frames * (300 / hz);
    *elapsed = next > 300 ? 300 : (uint16_t)next;
    return next >= 300;
}
