/**
 * @file apps/demos/demo_common.c
 * @brief Exclusive demos borrow one bounded ROM upload and PSG voice; no heap allocation.
 *
 * ROM-linked application code. Shared hardware and resource ownership remain in OS services.
 * The OS owns shortcut polling and reconstructs the desktop on every teardown.
 */
#include "demo_common.h"
volatile uint32_t mb_demo_timing[3];
MB_Result demo_begin(Demo *d, const char *name, const uint32_t *tiles, uint16_t count) {
    MB_PaletteRequest request = {15};
    MB_Result r;
    *d = (Demo){0};
    mb_demo_timing[0] = mb_demo_timing[1] = mb_demo_timing[2] = 0;
    d->last = mb_time_ticks();
    mb_window_open(name);
    r = mb_window_set_mode(MB_WINDOW_FULLSCREEN);
    if (r == MB_OK)
        r = mb_palette_request(&request, &d->lease);
    if (r == MB_OK && count > d->lease.tile_count)
        r = MB_UNAVAILABLE;
    if (r == MB_OK)
        r = mb_graphics_load(&d->lease, d->lease.first_tile, tiles, count);
    if (r != MB_OK)
        mb_app_fail(r);
    return r;
}
uint16_t demo_step(Demo *d) {
    uint32_t now = mb_time_ticks(), delta = now - d->last;
    d->last = now;
    if (!d->lease.token || mb_graphics_ready(&d->lease) != MB_OK)
        return 0;
    /* Cap recovery after a stall; nominal time uses the same 300 Hz units in both regions. */
    if (d->paused)
        return 0;
    mb_demo_timing[0]++;
    mb_demo_timing[1] += delta;
    if (delta > mb_demo_timing[2])
        mb_demo_timing[2] = delta;
    if (delta > 30)
        delta = 30;
    d->elapsed += delta;
    return delta;
}
void demo_end(Demo *d) {
    mb_audio_stop();
    d->lease.token = 0; /* OS cancels queued uploads before reconstructing its resources. */
}
void demo_help(Demo *d, const char *title, const char *controls) {
    if (!d->overlay) {
        mb_draw_text(2, 1, title);
        mb_draw_text(2, 3, controls);
        mb_draw_text(2, 4, "C exit / hold Start+B one second");
        d->overlay = 1;
    }
    if (d->elapsed > 1200 && d->overlay == 1) {
        mb_graphics_fill(&d->lease, 0, 0, 0, 40, 6, d->lease.first_tile, 0);
        d->overlay = 2;
    }
}
uint16_t demo_event(Demo *d, const MB_Event *e) {
    if (e->action == MB_CONTEXT) {
        mb_app_close();
        return 1;
    }
    if (e->action == MB_MENU) {
        d->paused = !d->paused;
        if (d->paused)
            mb_audio_stop();
        return 1;
    }
    return 0;
}
