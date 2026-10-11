/**
 * @file apps/demos/colourstorm.c
 * @brief Four deterministic palette looks. Entire CRAM updates are queued for VBlank;
 *
 * ROM-linked application code. Shared hardware and resource ownership remain in OS services.
 * no raster interrupt, live decompression or software pixel framebuffer.
 */
#include "demo_common.h"
#include "colourstorm_assets.h"
static Demo d;
static uint16_t look, speed, phase, refresh;
volatile uint16_t mb_colour_observation[4];
static MB_Result init(void) {
    look = speed = phase = 0;
    refresh = 1;
    return MB_OK;
}
static void activate(void) {
    if (demo_begin(&d, "ColourStorm", colour_tiles, 57) != MB_OK)
        return;
    mb_input_mode(MB_INPUT_GAME);
}
static void event(const MB_Event *e) {
    if (demo_event(&d, e))
        return;
    if (e->action == MB_NAVIGATE && e->x) {
        look = (look + 4 + e->x) % 4;
        refresh = 1;
    }
    if (e->action == MB_ACTIVATE) {
        speed = (speed + 1) % 3;
        refresh = 1;
    }
}
static void tick(void) {
    uint16_t delta = demo_step(&d), next, bank;
    static const char *const names[] = {"ColourStorm / slow aurora", "ColourStorm / plasma waves",
                                        "ColourStorm / turning bands",
                                        "ColourStorm / sunrise sweep"};
    if (!delta)
        return;
    if (!d.ready || refresh) {
        mb_graphics_fill(&d.lease, 0, 0, 0, 40, 28, d.lease.first_tile, 0);
        mb_graphics_map(&d.lease, 1, colour_maps + look * 1120);
        d.ready = 1;
        if (d.overlay == 1)
            d.overlay = 0;
    }
    next = speed == 2 ? phase : (d.elapsed / (speed == 1 ? 45 : 90)) & 15;
    if (next != phase || refresh || d.elapsed <= delta) {
        phase = next;
        for (bank = 0; bank < 4; bank++)
            mb_palette_set(&d.lease, bank, colour_palettes + look * 1024 + phase * 64 + bank * 16);
        if (look == 2)
            mb_graphics_map(&d.lease, 1, colour_rotating_maps + phase * 1120);
    }
    refresh = 0;
    demo_help(&d, names[look], "L/R look / A motion / Start pause");
    mb_colour_observation[0] = look;
    mb_colour_observation[1] = speed;
    mb_colour_observation[2] = phase;
    mb_colour_observation[3] = d.paused;
}
static void stop(void) {
    demo_end(&d);
}
const MB_App colourstorm_app = {MB_API_VERSION, "ColourStorm", init, activate,
                                event,          tick,          stop, stop};
