/**
 * @file apps/system/system.c
 * @brief System folder: SDK app links, measured diagnostics, storage metadata and session settings.
 *
 * ROM-linked application code. Shared hardware and resource ownership remain in OS services.
 * Uses only megabenchos.h and app-local interfaces; static state/ROM, no heap or hardware.
 */
#include "megabenchos.h"
static uint16_t page;
static MB_ThemeId preview;
static void number(uint16_t y, const char *label, uint32_t value) {
    char b[11];
    mb_draw_text(0, y, label);
    mb_format_u32(value, b, sizeof b);
    mb_draw_field(20, y, 10, b);
}
static void show(uint16_t p) {
    MB_MemoryStats m;
    MB_SystemInfo s;
    MB_Volume v;
    MB_Rect r;
    mb_window_get_client_rect(&r);
    page = p;
    mb_window_clear();
    if (!p) {
        mb_draw_text(0, 0, "System tools");
        mb_ui_button(1, 1, 2, 27, "About", 1);
        mb_ui_button(2, 1, 5, 27, "System Info", 1);
        mb_ui_button(3, 1, 8, 27, "Storage", 1);
        mb_ui_button(4, 1, 11, 27, "Settings", 1);
        mb_ui_exit();
        return;
    }
    if (p == 2) {
        mb_system_info(&s);
        mb_draw_text(0, 0, s.pal ? "PAL / nominal 50 Hz" : "NTSC / nominal 60 Hz");
        if (mb_memory_stats(&m) == MB_OK) {
            number(1, "SGDK heap total", m.total_tracked);
            number(2, "Heap allocated", m.used);
            number(3, "Heap free", m.free);
            number(4, "Heap high water", m.high_water);
            number(5, "Static RAM", m.static_bytes);
            number(6, "Reserved stack", m.stack_reserved);
            number(7, "Stack watermark", m.stack_used);
            mb_draw_text(0, 8, m.stack_guard_ok ? "Stack guard OK" : "Stack guard FAILED");
        } else
            mb_draw_text(0, 2, "RAM --");
        mb_draw_text(0, 9, "Controller / PSG / ROM");
        mb_draw_text(0, 10, "RAM files; no RTC or SRAM");
    }
    if (p == 5) {
        if (mb_memory_stats(&m) == MB_OK) {
            number(1, "Heap total", m.total_tracked);
            number(3, "Heap free", m.free);
            number(5, "High water", m.high_water);
            number(7, "Static RAM", m.static_bytes);
            number(9, "Stack watermark", m.stack_used);
        }
    }
    if (p == 6) {
        mb_draw_text(0, 0, "Live semantic input");
        mb_draw_text(0, 8, "Port 2 Sega Mouse supported");
        mb_draw_text(0, 10, "Hardware verification pending");
    }
    if (p == 3) {
        if (mb_storage_volume(0, &v) == MB_OK) {
            mb_draw_text(0, 0, v.name);
            mb_draw_text(0, 2, "Read only ROM metadata");
            number(4, "Capacity bytes", v.capacity_bytes);
        }
        {
            MB_DriveInfo fs;
            if (mb_fs_capacity("RAM:", 1, &fs) == MB_OK) {
                number(6, "RAM reserved", fs.reserved_bytes);
                number(7, "RAM file capacity", fs.capacity);
                number(8, "RAM file free", fs.free);
            }
        }
        mb_draw_text(0, 10, "Reset loses RAM files");
    }
    if (p == 4) {
        mb_draw_text(0, 0, "Cursor speed (session only)");
        mb_ui_button(11, 0, 3, 9, "Slow", 1);
        mb_ui_button(12, 10, 3, 9, "Normal", 1);
        mb_ui_button(13, 20, 3, 9, "Fast", 1);
        number(6, "Current level", mb_input_speed_level());
        mb_draw_text(0, 8, "No persistent settings");
        mb_ui_button(20, 0, 10, 20, "Appearance", 1);
    }
    if (p == 7) {
        static const char *const names[] = {"Workbench Blue", "Graphite", "Forest Terminal",
                                            "Amber Studio", "Ocean Teal"};
        uint16_t i;
        mb_draw_text(0, 0, "Appearance (session only)");
        for (i = 0; i < 5; i++)
            mb_ui_button(30 + i, 0, 1 + i * 2, 19, names[i], 1);
        mb_ui_button(40, 20, 3, 10, "Apply", 1);
        mb_ui_button(41, 20, 6, 10, "Cancel", 1);
        mb_ui_focus(30 + preview);
    }
    mb_ui_button(10, 0, r.height - 2, 12, "Back", 1);
    mb_ui_exit();
}
static MB_Result init(void) {
    page = mb_app_context();
    preview = mb_theme_current();
    return MB_OK;
}
static void activate(void) {
    mb_window_open("System");
    show(page);
}
static void event(const MB_Event *e) {
    uint16_t id;
    if (e->action == MB_RESIZE || e->action == MB_THEME_CHANGED) {
        activate();
        return;
    }
    if (mb_ui_event(e, &id) != MB_OK)
        return;
    if (id == MB_UI_EXIT) {
        mb_app_close();
        return;
    }
    if (id == 1)
        mb_app_launch_id(MB_APP_ABOUT);
    else if (id == 10) {
        mb_theme_cancel_preview();
        show(0);
    } else if (id == 20)
        show(7);
    else if (id >= 30 && id < 35) {
        preview = (MB_ThemeId)(id - 30);
        mb_theme_preview(preview);
    } else if (id == 40) {
        mb_theme_apply(preview);
        show(4);
    } else if (id == 41) {
        mb_theme_cancel_preview();
        preview = mb_theme_current();
        show(4);
    } else if (id >= 11 && id <= 13) {
        mb_input_speed(id - 10);
        number(6, "Current level", mb_input_speed_level());
    } else if (id >= 2 && id <= 4)
        show(id);
}
static void tick(void) {
    if (page == 6) {
        MB_Input in;
        mb_input_state(&in);
        number(2, "Buttons", in.buttons);
        number(4, "Pointer X", in.x);
        number(5, "Pointer Y", in.y);
        number(6, "Mode", in.mode);
    }
}
static void stop(void) {
    mb_theme_cancel_preview();
}
const MB_App system_app = {MB_API_VERSION, "System", init, activate, event, tick, stop, stop};
