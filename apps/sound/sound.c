/**
 * @file apps/sound/sound.c
 * @brief PSG Sound Test: public SDK bounded tones/noise; deactivate and shutdown mute audio.
 *
 * ROM-linked application code. Shared hardware and resource ownership remain in OS services.
 * Uses only megabenchos.h and app-local interfaces; static state/ROM, no heap or hardware.
 */
#include "megabenchos.h"
static uint16_t playing;
static void status(void) {
    MB_Rect r;
    mb_window_get_client_rect(&r);
    mb_draw_field(0, r.height - 2, r.width,
                  !playing       ? "Stopped"
                  : playing == 4 ? "White noise"
                                 : "PSG tone playing");
}
static MB_Result init(void) {
    playing = 0;
    return MB_OK;
}
static void activate(void) {
    mb_window_open("Sound Test / PSG");
    mb_draw_text(0, 0, "PSG sound effects / one voice");
    mb_ui_button(1, 0, 3, 14, "220 Hz", 1);
    mb_ui_button(2, 15, 3, 14, "440 Hz", 1);
    mb_ui_button(3, 0, 6, 14, "880 Hz", 1);
    mb_ui_button(4, 15, 6, 14, "Noise", 1);
    mb_ui_button(5, 7, 9, 15, "Stop / Mute", 1);
    status();
    mb_ui_exit();
}
static void event(const MB_Event *e) {
    if (e->action == MB_RESIZE || e->action == MB_THEME_CHANGED) {
        activate();
        return;
    }
    uint16_t id;
    MB_Result r = MB_OK;
    if (mb_ui_event(e, &id) != MB_OK)
        return;
    if (id == MB_UI_EXIT) {
        mb_app_close();
        return;
    }
    if (id >= 1 && id <= 3)
        r = mb_audio_tone(id == 1 ? 220 : id == 2 ? 440 : 880, 12);
    else if (id == 4)
        r = mb_audio_noise(12);
    else
        mb_audio_stop();
    playing = r == MB_OK && id != 5 ? id : 0;
    status();
}
static void tick(void) {}
static void stop(void) {
    mb_audio_stop();
}
const MB_App sound_app = {MB_API_VERSION, "Sound", init, activate, event, tick, stop, stop};
