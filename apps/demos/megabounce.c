/**
 * @file apps/demos/megabounce.c
 * @brief Pre-rendered original shaded checker sphere: nine 32x32 pieces, three shadow pieces.
 *
 * ROM-linked application code. Shared hardware and resource ownership remain in OS services.
 * Nine frames fit the leased tile bank. Integer time, no decompression or heap per frame.
 */
#include "demo_common.h"
#include "megabounce_assets.h"
static Demo d;
static uint16_t phase, fast, impact;
static uint16_t period, arc_height, motion_remainder, bounces, walls;
static int32_t position;
static int16_t velocity;
static uint32_t random_state, spin;
volatile uint16_t mb_bounce_observation[10];
static uint16_t random_value(void) {
    random_state ^= random_state << 13;
    random_state ^= random_state >> 17;
    random_state ^= random_state << 5;
    return (uint16_t)random_state;
}
static void next_arc(void) {
    period = 330 + random_value() % 151;
    arc_height = 30 + random_value() % 71;
    if (!(random_value() & 3))
        velocity = -velocity;
    velocity = velocity < 0 ? -(36 + random_value() % 60) : 36 + random_value() % 60;
}
static MB_Result init(void) {
    phase = fast = impact = 0;
    bounces = walls = motion_remainder = 0;
    position = 112L * 256;
    velocity = 72;
    random_state = mb_time_ticks() ^ 0x6d2b79f5UL;
    if (!random_state)
        random_state = 1;
    spin = 0;
    next_arc();
    return MB_OK;
}
static void activate(void) {
    uint16_t p[16] = {0}, i;
    if (demo_begin(&d, "MegaBounce", bounce_tiles, BOUNCE_TILE_COUNT) != MB_OK)
        return;
    mb_palette_set(&d.lease, 0, bounce_palette);
    for (i = 0; i < 16; i++)
        p[i] = bounce_palette[i];
    p[0] = 0;
    p[1] = 0x422;
    p[2] = 0x444;
    p[3] = 0x666;
    p[15] = 0xeee;
    mb_palette_set(&d.lease, 1, p);
    p[1] = 0x222;
    mb_palette_set(&d.lease, 2, p);
    mb_input_mode(MB_INPUT_GAME);
}
static void event(const MB_Event *e) {
    if (demo_event(&d, e))
        return;
    if (e->action == MB_ACTIVATE)
        fast = !fast;
}
static void tick(void) {
    uint16_t delta = demo_step(&d), step, frame, q, base = d.lease.first_tile;
    int16_t y, x;
    uint32_t height;
    if (!delta)
        return;
    if (!d.ready) {
        mb_graphics_fill(&d.lease, 0, 0, 0, 40, 28, base, 0);
        mb_graphics_fill(&d.lease, 1, 0, 0, 40, 24, base + 1, 1);
        mb_graphics_fill(&d.lease, 1, 0, 24, 40, 4, base + 2, 1);
        d.ready = 1;
    }
    motion_remainder += delta * (fast ? 3 : 2);
    step = motion_remainder / 2;
    motion_remainder %= 2;
    spin += delta;
    position += (int32_t)velocity * step;
    if (position < 0) {
        position = -position;
        velocity = 36 + random_value() % 60;
        ++walls;
    } else if (position > 224L * 256) {
        position = 448L * 256 - position;
        velocity = -(36 + random_value() % 60);
        ++walls;
    }
    phase += step;
    if (phase >= period) {
        phase -= period;
        ++bounces;
        next_arc();
        impact = 45;
        mb_audio_tone(170, 11);
    }
    if (impact) {
        impact = impact > delta ? impact - delta : 0;
        if (!impact)
            mb_audio_stop();
        else
            mb_audio_volume(impact > 25 ? 11 : 13);
    }
    /* Variable arcs land on the same floor; the full 96px sphere stays on screen.
     * Fixed-point horizontal reflection allows rebounds across the whole width. */
    height = (uint32_t)phase * (period - phase) * (arc_height * 4) / ((uint32_t)period * period);
    y = 104 - height;
    x = position / 256;
    frame = phase < 10 || phase > period - 8 ? 8 : (spin / (fast ? 60 : 90)) & 7;
    /* Earlier hardware indices win overlaps: ball must precede its cast shadow. */
    for (q = 0; q < 9; q++)
        mb_graphics_sprite_box(&d.lease, q, x + (q % 3) * 32, y + (q / 3) * 32, 4, 4,
                               base + 3 + frame * 144 + q * 16, 0);
    for (q = 0; q < 3; q++) {
        uint16_t small = height > 45, shadow = base + 3 + 1296 + (small ? 24 : 0);
        if (small && q == 2) {
            if (d.ready == 1)
                mb_graphics_sprite_box(&d.lease, q + 9, -32, -32, 4, 2, shadow, 2);
            mb_graphics_hide(&d.lease, q + 9);
        } else
            mb_graphics_sprite_box(&d.lease, q + 9, x + (small ? 16 : 0) + q * 32, 194, 4, 2,
                                   shadow + q * 8, 2);
    }
    d.ready = 2;
    demo_help(&d, "MegaBounce / original checker study", "A speed / Start pause");
    mb_bounce_observation[0] = phase;
    mb_bounce_observation[1] = height;
    mb_bounce_observation[2] = frame;
    mb_bounce_observation[3] = d.paused;
    mb_bounce_observation[4] = fast;
    mb_bounce_observation[5] = x;
    mb_bounce_observation[6] = y;
    mb_bounce_observation[7] = bounces;
    mb_bounce_observation[8] = walls;
    mb_bounce_observation[9] = arc_height;
}
static void stop(void) {
    demo_end(&d);
}
const MB_App megabounce_app = {MB_API_VERSION, "MegaBounce", init, activate,
                               event,          tick,         stop, stop};
