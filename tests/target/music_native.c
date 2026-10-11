/**
 * @file tests/target/music_native.c
 * @brief Native BlastEm playback harness. Release ROMs exclude this module.
 *
 * Optional test-only code, excluded from the release ROM.
 * Exercises real MegaPlayer UI callbacks and the unchanged public music engine;
 * no emulated RAM injection. Record native YM waveforms with BlastEm -y.
 */
#ifdef MB_NATIVE_MUSIC
#include <genesis.h>
#include "internal.h"
volatile uint16_t mb_native_music_result;
volatile uint32_t mb_native_music_frames[5];
static uint16_t hz;
static void number(uint16_t x, uint16_t y, uint32_t value) {
    char text[12];
    mb_format_u32(value, text, sizeof text);
    mb_draw_text(x, y, text);
}
static void wait_frames(uint16_t count) {
    while (count--) {
        SYS_doVBlankProcess();
        mb_video_flush();
        mb_runtime_tick(hz);
    }
}
static void choose(uint16_t id) {
    MB_Event e = {MB_ACTIVATE, 0, 0};
    mb_ui_focus(id);
    mb_runtime_event(&e);
}
static void check(bool success, uint16_t code) {
    if (success)
        return;
    mb_music_stop();
    mb_native_music_result = code;
    mb_window_open("Music test FAILED");
    number(1, 2, code);
    while (TRUE)
        SYS_doVBlankProcess();
}
void mb_music_native_test(void) {
    static const uint32_t tenths[5] = {896, 1152, 1056, 960, 768};
    uint16_t i;
    uint32_t began, elapsed, expected, tolerance;
    hz = SYS_isPAL() ? 50 : 60;
    mb_native_music_result = 1;
    for (i = 0; i < 5; i++) {
        choose(10 + i);
        choose(5);
        wait_frames(hz * 2);
        check(mb_music_state() == MB_MUSIC_PLAYING, 10 + i);
        choose(6);
        wait_frames(hz);
        check(mb_music_state() == MB_MUSIC_STOPPED && !mb_diagnostics.audio_active, 20 + i);
        choose(5);
        began = vtimer;
        expected = tenths[i] * hz / 10;
        do {
            wait_frames(1);
            elapsed = vtimer - began;
        } while (mb_music_state() == MB_MUSIC_PLAYING && elapsed < expected + hz * 2);
        mb_native_music_frames[i] = elapsed;
        tolerance = hz / 10 + expected / 200;
        check(mb_music_state() == MB_MUSIC_STOPPED && elapsed + tolerance >= expected &&
                  elapsed <= expected + tolerance,
              30 + i);
        wait_frames(hz);
    }
    choose(5);
    wait_frames(hz);
    mb_app_close();
    wait_frames(hz);
    check(!mb_runtime_active() && !mb_diagnostics.audio_active && mb_stack_check(), 40);
    mb_native_music_result = 0x600d;
    mb_window_open("Native music PASS");
    mb_window_set_mode(MB_WINDOW_MAXIMIZED);
    mb_draw_text(1, 1, "5 full songs / Stop / exit passed");
    mb_draw_text(1, 3, "Elapsed frames (no source loops):");
    for (i = 0; i < 5; i++)
        number(2, 5 + i * 2, mb_native_music_frames[i]);
    mb_draw_text(1, 17, hz == 50 ? "PAL 50 Hz" : "NTSC 60 Hz");
    while (TRUE) {
        SYS_doVBlankProcess();
        mb_video_flush();
    }
}
#endif
