/**
 * @file apps/demos/stardrive.c
 * @brief Fixed-point perspective stars reuse four points and directional tapered trails.
 * Twenty-four stars
 *
 * ROM-linked application code. Shared hardware and resource ownership remain in OS services.
 * plus a tiled background planet leave room for conservative scanline enforcement.
 */
#include "demo_common.h"
#include "stardrive_assets.h"
typedef struct {
    int16_t x, y;
    int32_t z;
} Star;
static Demo d;
static Star stars[24];
static uint16_t random_state, speed, warp, warp_blend, note;
volatile uint16_t mb_star_observation[4];
static uint16_t random(void) {
    random_state = random_state * 25173U + 13849U;
    return random_state;
}
static void reset(uint16_t index) {
    stars[index].x = (int16_t)(random() % 255) - 127;
    stars[index].y = (int16_t)(random() % 179) - 89;
    stars[index].z = (180 + random() % 77) * 256L;
}
static MB_Result init(void) {
    uint16_t i;
    speed = 56;
    warp = 0;
    warp_blend = 0;
    note = 0xffff;
    random_state = 0x4d42;
    for (i = 0; i < 24; i++) {
        reset(i);
        stars[i].z = (24 + random() % 233) * 256L;
    }
    return MB_OK;
}
static void activate(void) {
    uint16_t p[16] = {0, 0x444, 0x888, 0xccc, 0xeee}, bank;
    if (demo_begin(&d, "StarDrive", star_tiles, 93) != MB_OK)
        return;
    p[15] = 0xeee;
    for (bank = 0; bank < 4; bank++)
        mb_palette_set(&d.lease, bank, p);
    mb_palette_set(&d.lease, 1, planet_palette);
    mb_input_mode(MB_INPUT_GAME);
}
static void event(const MB_Event *e) {
    if (demo_event(&d, e)) {
        note = 0xffff;
        return;
    }
    if (e->action == MB_ACTIVATE)
        warp = !warp;
    if (e->action == MB_NAVIGATE) {
        if (e->y < 0 && speed < 160)
            speed += 8;
        if (e->y > 0 && speed > 24)
            speed -= 8;
    }
}
static void tick(void) {
    uint16_t delta = demo_step(&d), i, x, y, next_note;
    static const uint16_t melody[] = {220, 330, 440, 330, 220, 440, 660, 330};
    if (!delta)
        return;
    /* Nominal-time ramp keeps acceleration identical at PAL and NTSC rates. */
    if (warp)
        warp_blend = warp_blend + delta * 2 > 256 ? 256 : warp_blend + delta * 2;
    else
        warp_blend = warp_blend > delta * 2 ? warp_blend - delta * 2 : 0;
    if (!d.ready) {
        mb_graphics_fill(&d.lease, 0, 0, 0, 40, 28, d.lease.first_tile, 0);
        mb_graphics_fill(&d.lease, 1, 0, 0, 40, 28, d.lease.first_tile, 0);
        for (y = 0; y < 8; y++)
            for (x = 0; x < 8; x++)
                mb_graphics_fill(&d.lease, 1, 26 + x, 5 + y, 1, 1,
                                 d.lease.first_tile + 29 + y * 8 + x, 1);
        for (i = 0; i < 24; i++)
            mb_graphics_sprite_box(&d.lease, i, -32, -32, 1, 1, d.lease.first_tile + 1, 0);
        d.ready = 1;
    }
    for (i = 0; i < 24; i++) {
        int16_t px, py;
        uint16_t brightness, scale, tile, direction, length;
        int16_t ax, ay;
        MB_Result result;
        stars[i].z -= (uint32_t)delta * ((speed * (256UL + warp_blend * 2)) >> 8);
        if (stars[i].z < 16 * 256L)
            reset(i);
        /* Quantized reciprocal uses native DIVU instead of two costly 32-bit divisions. */
        scale = 25600U / (uint16_t)(stars[i].z >> 8);
        px = 160 + ((int32_t)stars[i].x * scale >> 8);
        py = 112 + ((int32_t)stars[i].y * scale >> 8);
        if (px < 4 || px > 316 || py < 4 || py > 220) {
            reset(i);
            mb_graphics_hide(&d.lease, i);
            continue;
        }
        brightness = stars[i].z > 160 * 256L  ? 0
                     : stars[i].z > 90 * 256L ? 1
                     : stars[i].z > 45 * 256L ? 2
                                              : 3;
        tile = 1 + brightness;
        if (warp_blend >= 64) {
            ax = stars[i].x < 0 ? -stars[i].x : stars[i].x;
            ay = stars[i].y < 0 ? -stars[i].y : stars[i].y;
            if (ax > ay * 2)
                direction = stars[i].x < 0 ? 4 : 0;
            else if (ay > ax * 2)
                direction = stars[i].y < 0 ? 6 : 2;
            else
                direction = stars[i].y < 0 ? (stars[i].x < 0 ? 5 : 7) : (stars[i].x < 0 ? 3 : 1);
            length = brightness > 2 ? 2 : brightness;
            if (warp_blend < 160 && length)
                --length;
            tile = 5 + direction * 3 + length;
        }
        result =
            mb_graphics_sprite_box(&d.lease, i, px - 4, py - 4, 1, 1, d.lease.first_tile + tile, 0);
        if (result == MB_BUSY)
            mb_graphics_hide(&d.lease, i);
    }
    next_note = (d.elapsed / 150) & 7;
    if (note != next_note) {
        note = next_note;
        mb_audio_tone(melody[note], 13);
    }
    mb_audio_volume(d.elapsed % 150 < 45 ? 13 : 14);
    demo_help(&d, "StarDrive / ringworld passage", "Up/down speed / A warp / Start pause");
    mb_star_observation[0] = speed;
    mb_star_observation[1] = warp;
    mb_star_observation[2] = d.paused;
    mb_star_observation[3] = note;
}
static void stop(void) {
    demo_end(&d);
}
const MB_App stardrive_app = {MB_API_VERSION, "StarDrive", init, activate, event, tick, stop, stop};
