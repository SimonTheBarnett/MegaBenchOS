/**
 * @file apps/demos/ocean.c
 * @brief One deduplicated original coast resource set at a time. Palette fades conceal
 *
 * ROM-linked application code. Shared hardware and resource ownership remain in OS services.
 * bounded ROM streaming; PSG white noise envelope creates surf without PCM/XGM.
 */
#include "demo_common.h"
#include "ocean_assets.h"
static Demo d;
static uint16_t scene, target, ambience, label, fade, stage, last_wave, last_envelope, last_label;
static uint16_t motion_row;
static uint32_t fade_clock;
static const uint32_t *const tiles[] = {ocean_tiles_0, ocean_tiles_1, ocean_tiles_2};
static const uint16_t *const maps[] = {ocean_map_0, ocean_map_1, ocean_map_2};
static const uint16_t *const palettes[] = {ocean_palette_0, ocean_palette_1, ocean_palette_2};
static const uint16_t counts[] = {OCEAN_0_TILES, OCEAN_1_TILES, OCEAN_2_TILES};
static const char *const names[] = {"Ocean Dreams / tropical midday", "Ocean Dreams / amber sunset",
                                    "Ocean Dreams / moonlit coast"};
volatile uint16_t mb_ocean_observation[6];
static void palette(void) {
    uint16_t p[16], bank, c;
    for (bank = 0; bank < 4; bank++) {
        for (c = 0; c < 16; c++) {
            uint16_t raw = palettes[scene][bank * 16 + c];
            p[c] = ((((raw >> 1) & 7) * fade / 8) << 1) | ((((raw >> 5) & 7) * fade / 8) << 5) |
                   ((((raw >> 9) & 7) * fade / 8) << 9);
        }
        mb_palette_set(&d.lease, bank, p);
    }
}
static MB_Result init(void) {
    scene = target = stage = fade = 0;
    ambience = label = 1;
    last_label = last_wave = last_envelope = 0xffff;
    fade_clock = 0;
    motion_row = 22;
    return MB_OK;
}
static void activate(void) {
    if (demo_begin(&d, "Ocean Dreams", tiles[0], counts[0]) != MB_OK)
        return;
    palette();
    mb_input_mode(MB_INPUT_GAME);
}
static void event(const MB_Event *e) {
    if (e->action == MB_CONTEXT) {
        mb_app_close();
        return;
    }
    if (e->action == MB_MENU) {
        label = !label;
        last_label = 0xffff;
        mb_graphics_fill(&d.lease, 0, 0, 0, 40, 6, d.lease.first_tile, 0);
        return;
    }
    if (e->action == MB_ACTIVATE) {
        ambience = !ambience;
        last_envelope = 0xffff;
        if (!ambience)
            mb_audio_stop();
    }
    if (e->action == MB_NAVIGATE && e->x) {
        target = (target + 3 + e->x) % 3;
        if (!stage && target != scene) {
            stage = 1;
            fade_clock = 0;
        }
    }
}
static void tick(void) {
    uint16_t delta = demo_step(&d), wave, envelope;
    if (!delta)
        return;
    if (!d.ready) {
        mb_graphics_fill(&d.lease, 0, 0, 0, 40, 28, d.lease.first_tile, 0);
        mb_graphics_map(&d.lease, 1, maps[scene]);
        d.ready = 1;
        stage = 3;
        fade_clock = 0;
        last_label = last_wave = 0xffff;
    }
    fade_clock += delta;
    if (stage == 1 && fade_clock >= 45) {
        fade_clock = 0;
        if (fade)
            fade--;
        palette();
        if (!fade) {
            MB_Result r;
            scene = target;
            r = mb_graphics_load(&d.lease, d.lease.first_tile, tiles[scene], counts[scene]);
            if (r != MB_OK) {
                mb_app_fail(r);
                return;
            }
            d.ready = 0;
            stage = 2;
            return;
        }
    } else if (stage == 3 && fade_clock >= 45) {
        fade_clock = 0;
        if (fade < 8)
            fade++;
        palette();
        if (fade == 8) {
            stage = target != scene ? 1 : 0;
        }
    }
    wave = (d.elapsed / 60) & 7;
    if (wave != last_wave) {
        last_wave = wave;
        motion_row = 11;
    }
    if (motion_row < 22) {
        uint16_t end = motion_row + 2;
        if (end > 22)
            end = 22;
        /* Two rows per refresh keep damage below the shared flush budget.
         * Only sea and shore move; the generated phases preserve the palm. */
        mb_graphics_map_region(&d.lease, 1, 0, motion_row, 40, end - motion_row,
                               maps[scene] + last_wave * 40 * 28 + motion_row * 40);
        motion_row = end;
    }
    if (label) {
        if (last_label != scene)
            mb_draw_text(2, 1, names[scene]);
        if (d.elapsed < 1200) {
            if (last_label != scene) {
                mb_draw_text(2, 3, "Left/right scene / A ambience");
                mb_draw_text(2, 4, "Start label / C exit / hold Start+B");
            }
        } else if (d.overlay != 2) {
            mb_graphics_fill(&d.lease, 0, 0, 3, 40, 3, d.lease.first_tile, 0);
            d.overlay = 2;
        }
        last_label = scene;
    }
    /* Continuous PSG noise; envelope changes preserve the noise generator phase. */
    envelope = 13 + (((d.elapsed / 100) % 12 < 3 || (d.elapsed / 100) % 12 > 8) ? 1 : 0);
    if (ambience && envelope != last_envelope) {
        if (last_envelope == 0xffff)
            mb_audio_noise(envelope);
        else
            mb_audio_volume(envelope);
        last_envelope = envelope;
    }
    mb_ocean_observation[0] = scene;
    mb_ocean_observation[1] = target;
    mb_ocean_observation[2] = fade;
    mb_ocean_observation[3] = stage;
    mb_ocean_observation[4] = ambience;
    mb_ocean_observation[5] = wave;
}
static void stop(void) {
    demo_end(&d);
}
const MB_App ocean_app = {MB_API_VERSION, "Ocean Dreams", init, activate, event, tick, stop, stop};
