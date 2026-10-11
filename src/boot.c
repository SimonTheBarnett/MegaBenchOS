/**
 * @file src/boot.c
 * @brief Original ROM-only boot mark. Owns VDP briefly before the desktop initializes;
 *
 * Console startup code linked by SGDK; owns boot and main-loop integration.
 * synchronous PAL/NTSC VBlank fades use stack palettes, no heap or app services.
 */
#include <genesis.h>
#include "internal.h"
#include "boot_assets.h"
#include "version.h"
volatile uint16_t mb_boot_observation __attribute__((externally_visible));
static void brightness(u16 level) {
    u16 p[16], i;
    for (i = 0; i < 16; ++i) {
        u16 c = boot_palette[i];
        p[i] = ((((c >> 1) & 7) * level / 7) << 1) | ((((c >> 5) & 7) * level / 7) << 5) |
               ((((c >> 9) & 7) * level / 7) << 9);
    }
    PAL_setPalette(PAL0, p, CPU);
}
void mb_boot_logo(uint16_t hz) {
    u16 x, y, frame, fade = hz * 3 / 5;
    VDP_setEnable(FALSE);
    VDP_setScreenWidth320();
    VDP_setScreenHeight224();
    VDP_setPlaneSize(64, 32, TRUE);
    VDP_clearPlane(BG_A, TRUE);
    VDP_clearPlane(BG_B, TRUE);
    VDP_resetSprites();
    VDP_setBackgroundColor(0);
    VDP_loadTileData(boot_tiles, TILE_USER_INDEX, BOOT_TILE_COUNT, CPU);
    for (y = 0; y < 28; ++y)
        for (x = 0; x < 40; ++x)
            VDP_setTileMapXY(
                BG_A, TILE_ATTR_FULL(PAL0, 0, 0, 0, TILE_USER_INDEX + boot_map[y * 40 + x]), x, y);
    VDP_setTextPalette(PAL0);
    VDP_drawText("NATIVE MEGA DRIVE DESKTOP", 8, 18);
    VDP_drawText("v" MB_OS_VERSION, 18, 20);
    brightness(0);
    VDP_setEnable(TRUE);
    for (frame = 0; frame < hz * 3; ++frame) {
        u16 level = frame < fade             ? frame * 7 / fade
                    : frame >= hz * 3 - fade ? (hz * 3 - frame - 1) * 7 / fade
                                             : 7;
        SYS_doVBlankProcess();
        mb_boot_observation = frame < fade ? 1 : frame >= hz * 3 - fade ? 3 : 2;
        brightness(level);
    }
    VDP_setEnable(FALSE);
    VDP_clearPlane(BG_A, TRUE);
    VDP_clearPlane(BG_B, TRUE);
    mb_boot_observation = 4;
}
