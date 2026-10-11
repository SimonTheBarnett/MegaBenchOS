/**
 * @file apps/clock/clock.c
 * @brief Runtime Clock: continuously shows elapsed time since boot through the public SDK.
 *
 * ROM-linked application code. Shared hardware and resource ownership remain in OS services.
 * Uses only megabenchos.h and app-local interfaces; static state/ROM, no heap or hardware.
 */
#include "megabenchos.h"
static uint32_t shown;
static void display(void) {
    char time[14];
    uint16_t length = 0;
    MB_Rect r;
    mb_window_get_client_rect(&r);
    mb_time_format(shown, time, sizeof time);
    while (time[length])
        ++length;
    mb_draw_field((r.width - length) / 2, r.height / 2, length, time);
}
static MB_Result init(void) {
    shown = mb_time_seconds();
    return MB_OK;
}
static void activate(void) {
    mb_window_open("Clock");
    mb_draw_text(0, 0, "Runtime since boot");
    shown = mb_time_seconds();
    display();
    mb_ui_exit();
}
static void event(const MB_Event *e) {
    if (e->action == MB_RESIZE || e->action == MB_THEME_CHANGED) {
        activate();
        return;
    }
    uint16_t id;
    MB_Result result = mb_ui_event(e, &id);
    if (result == MB_OK && id == MB_UI_EXIT) {
        mb_app_close();
        return;
    }
}
static void tick(void) {
    uint32_t now = mb_time_seconds();
    if (now != shown) {
        shown = now;
        display();
    }
}
static void stop(void) {}
const MB_App clock_app = {MB_API_VERSION, "Clock", init, activate, event, tick, stop, stop};
