/**
 * @file kernel/media/format.c
 * @brief SGDK compiled XGM2 omits the four-byte XGM2 magic. Never infer trust from extension.
 *
 * OS service implementation behind the public SDK; apps use megabenchos.h.
 * Only v0x10 packed single-track lengths are inspected; commands are not interpreted.
 */
#include "format.h"
MB_Result mb_music_header(const uint8_t *h, uint16_t size) {
    uint32_t blocks;
    if (!h || size < 256)
        return MB_CORRUPT;
    if (h[0] != 0x10)
        return MB_VERSION;
    if ((h[1] & ~0x0d) || !(h[1] & 8))
        return MB_NOT_SUPPORTED;
    blocks = (uint32_t)(h[2] | ((uint16_t)h[3] << 8)) + (h[4] | ((uint16_t)h[5] << 8)) +
             (h[6] | ((uint16_t)h[7] << 8));
    if (!blocks || blocks * 256UL + 256 > size)
        return MB_CORRUPT;
    return MB_OK;
}
