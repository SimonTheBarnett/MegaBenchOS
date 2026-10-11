/**
 * @file apps/about/about.c
 * @brief About page: release version, desktop description and creator credit.
 *
 * ROM-linked application code. Shared hardware and resource ownership remain in OS services.
 * Uses only megabenchos.h and app-local interfaces; static state/ROM, no heap or hardware.
 */
#include "megabenchos.h"
static MB_Result init(void) {
    return MB_OK;
}
static void activate(void) {
    MB_SystemInfo info;
    mb_window_open("About MegaBenchOS");
    mb_system_info(&info);
    mb_draw_text(1, 1, "A desktop for the Sega");
    mb_draw_text(1, 2, "Mega Drive / Genesis.");
    mb_draw_text(1, 4, "Version");
    mb_draw_text(9, 4, "v");
    mb_draw_text(10, 4, info.version);
    mb_draw_text(1, 6, info.credits);
    mb_draw_text(1, 8, "\"We come from the future,");
    mb_draw_text(1, 9, "to relive the past\"");
    mb_draw_text(1, 11, "Code/art + SGDK: MIT");
    mb_draw_text(1, 12, "Music: Safety Stoat Studios");
    mb_draw_text(1, 13, "CC0 - credits in MegaPlayer");
    mb_ui_exit();
}
static void event(const MB_Event *e) {
    if (e->action == MB_RESIZE || e->action == MB_THEME_CHANGED) {
        activate();
        return;
    }
    {
        uint16_t id;
        if (mb_ui_event(e, &id) == MB_OK && id == MB_UI_EXIT)
            mb_app_close();
    }
}
static void tick(void) {}
static void deactivate(void) {}
static void shutdown(void) {}
const MB_App about_app = {MB_API_VERSION, "About", init,       activate,
                          event,          tick,    deactivate, shutdown};
