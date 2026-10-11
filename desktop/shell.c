/**
 * @file desktop/shell.c
 * @brief ROM entry registry and paged shell. Uses shared retained tiles and no heap.
 *
 * Shared desktop/window implementation; apps reach it through the public SDK.
 * Folder/page/selection survive app transitions; unavailable entries never launch.
 * A single client-derived grid handles both desktop roots and folder contents.
 */
#include <genesis.h>
#include "internal.h"
#include "ui_assets.h"
enum { ROOT, APPS, GAMES, DEMOS, SYSTEM, FILES, HELP, RAMDISK, DOCUMENTS, PICTURES, MUSIC };
typedef struct {
    uint16_t id, category, icon, app, context, capabilities;
    MB_WindowMode preferred;
    uint16_t min_width, min_height, modes;
    const char *label, *help;
    MB_Result (*launch)(uint16_t);
} Entry;
static MB_Result launch(uint16_t app) {
    return mb_app_launch_id(app);
}
#define E(id, cat, label, icon, app, ctx, help)                                                    \
    {id, cat, icon, app, ctx, MB_CAP_ROM_LINKED, MB_WINDOW_NORMAL, 30, 14, 7, label, help, launch}
#define D(id, label, icon, app, help)                                                              \
    {id, DEMOS, icon,  app,  0,     MB_CAP_ROM_LINKED | MB_CAP_PSG, MB_WINDOW_FULLSCREEN, 40,      \
     28, 4,     label, help, launch}
#define P(id, cat, label)                                                                          \
    {id,                                                                                           \
     cat,                                                                                          \
     (cat == ROOT || cat == FILES ? UI_ICON_SYSTEM : UI_ICON_APP),                                 \
     0xffff,                                                                                       \
     0,                                                                                            \
     0,                                                                                            \
     MB_WINDOW_NORMAL,                                                                             \
     30,                                                                                           \
     14,                                                                                           \
     7,                                                                                            \
     label,                                                                                        \
     "Coming later",                                                                               \
     0}
static const Entry entries[] = {
    P(APPS, ROOT, "Apps"),
    P(GAMES, ROOT, "Games"),
    P(DEMOS, ROOT, "Demos"),
    P(SYSTEM, ROOT, "System"),
    P(HELP, ROOT, "Help"),
    P(RAMDISK, ROOT, "RAM Disk"),
    P(DOCUMENTS, RAMDISK, "Documents"),
    P(PICTURES, RAMDISK, "Pictures"),
    P(MUSIC, RAMDISK, "Music"),
    {107, APPS, UI_ICON_APP, MB_APP_MEGAPAINT, 0, MB_CAP_ROM_LINKED, MB_WINDOW_MAXIMIZED, 40, 24, 2,
     "MegaPaint", "16-colour pixel editor", launch},
    {108, APPS, UI_ICON_SOUND, MB_APP_MEGAPLAYER, 0, MB_CAP_ROM_LINKED, MB_WINDOW_MAXIMIZED, 40, 24,
     2, "MegaPlayer", "Bundled FM/PSG music", launch},
    {106, APPS, UI_ICON_APP, MB_APP_MEGAPAD, 0, MB_CAP_ROM_LINKED, MB_WINDOW_MAXIMIZED, 40, 24, 2,
     "MegaPad", "Session text editor", launch},
    E(102, APPS, "Calculator", UI_ICON_CALCULATOR, MB_APP_CALCULATOR, 0, "Integer arithmetic"),
    E(104, APPS, "Sound Test", UI_ICON_SOUND, MB_APP_SOUND, 0, "PSG audio"),
    E(103, APPS, "Clock", UI_ICON_CLOCK, MB_APP_CLOCK, 0, "Session timer"),
    {200, GAMES, UI_ICON_SNAKE, MB_APP_SNAKE, 0, MB_CAP_ROM_LINKED, MB_WINDOW_MAXIMIZED, 30, 14, 7,
     "Snake", "Eat, speed up, avoid rocks; Start pauses", launch},
    D(301, "Ocean Dreams", UI_ICON_OCEAN, MB_APP_OCEAN, "1.0 / Three original coasts"),
    D(303, "StarDrive", UI_ICON_STARDRIVE, MB_APP_STARDRIVE, "1.0 / Depth and warp flight"),
    D(302, "MegaBounce", UI_ICON_MEGABOUNCE, MB_APP_MEGABOUNCE, "1.0 / Shaded checker sphere"),
    D(300, "ColourStorm", UI_ICON_COLOURSTORM, MB_APP_COLOURSTORM,
      "1.0 / Four gentle colour looks"),
    E(400, SYSTEM, "Settings", UI_ICON_SYSTEM, MB_APP_SYSTEM, 4, "Session settings"),
    E(404, SYSTEM, "Input Test", UI_ICON_SYSTEM, MB_APP_SYSTEM, 6, "Controller and pointer"),
    E(402, SYSTEM, "Memory", UI_ICON_SYSTEM, MB_APP_SYSTEM, 5, "Measured RAM"),
    E(403, SYSTEM, "Storage", UI_ICON_SYSTEM, MB_APP_SYSTEM, 3, "ROM and session RAM"),
    E(401, SYSTEM, "System Info", UI_ICON_SYSTEM, MB_APP_SYSTEM, 2, "Hardware and version"),
    E(405, SYSTEM, "About", UI_ICON_SYSTEM, MB_APP_ABOUT, 0, "Credits and license"),
    P(500, FILES, "ROM:"),
    P(600, HELP, "Getting Started"),
    P(601, HELP, "Controls"),
    P(602, HELP, "Navigation"),
    P(603, HELP, "App Guide"),
    P(604, HELP, "Troubleshooting"),
    P(605, HELP, "RAM Disk"),
    P(606, HELP, "MegaPlayer")
#ifdef MB_SELF_TEST
        ,
    P(1000, DEMOS, "Fixture 00"),
    P(1001, DEMOS, "Fixture 01"),
    P(1002, DEMOS, "Fixture 02"),
    P(1003, DEMOS, "Fixture 03"),
    P(1004, DEMOS, "Fixture 04"),
    P(1005, DEMOS, "Fixture 05"),
    P(1006, DEMOS, "Fixture 06"),
    P(1007, DEMOS, "Fixture 07"),
    P(1008, DEMOS, "Fixture 08"),
    P(1009, DEMOS, "Fixture 09"),
    P(1010, DEMOS, "Fixture 10"),
    P(1011, DEMOS, "Fixture 11"),
    P(1012, DEMOS, "Fixture 12"),
    P(1013, DEMOS, "Fixture 13"),
    P(1014, DEMOS, "Fixture 14"),
    P(1015, DEMOS, "Fixture 15"),
    P(1016, DEMOS, "Fixture 16"),
    P(1017, DEMOS, "Fixture 17"),
    P(1018, DEMOS, "Fixture 18"),
    P(1019, DEMOS, "Fixture 19"),
    P(10, ROOT, "Root 00"),
    P(11, ROOT, "Root 01"),
    P(12, ROOT, "Root 02"),
    P(13, ROOT, "Root 03"),
    P(14, ROOT, "Root 04"),
    P(15, ROOT, "Root 05"),
    P(16, ROOT, "Root 06"),
    P(17, ROOT, "Root 07"),
    P(18, ROOT, "Root 08"),
    P(19, ROOT, "Root 09"),
    P(20, ROOT, "Root 10"),
    P(21, ROOT, "Root 11"),
    P(22, ROOT, "Root 12"),
    P(23, ROOT, "Root 13"),
    P(24, ROOT, "Root 14"),
    P(25, ROOT, "Root 15"),
    P(26, ROOT, "Root 16"),
    P(27, ROOT, "Root 17"),
    P(28, ROOT, "Root 18"),
    P(29, ROOT, "Root 19")
#endif
};
static const char *const folders[] = {"MegaBenchOS", "Apps",     "Games", "Demos",
                                      "System",      "Files",    "Help",  "RAM Disk",
                                      "Documents",   "Pictures", "Music"};
static uint16_t folder, selected[32], detail;
static uint16_t restart_focus, restart_confirm;
uint16_t mb_shell_modal(void) {
    return restart_confirm;
}
static MB_FileInfo browser_file;
static Entry browser_entry;
static char browser_path[44], rename_name[16], browser_message[39];
static uint16_t rename_length;
static uint16_t ram_directory(void) {
    return folder == DOCUMENTS || folder == PICTURES || folder == MUSIC;
}
static const char *directory_path(void) {
    return folder == MUSIC       ? "RAM:/Music/"
           : folder == DOCUMENTS ? "RAM:/Documents/"
                                 : "RAM:/Pictures/";
}
static void file_path(char *out, const char *name) {
    const char *p = directory_path();
    uint16_t i = 0, j = 0;
    while (p[j])
        out[i++] = p[j++];
    j = 0;
    while (name[j] && i < 43)
        out[i++] = name[j++];
    out[i] = 0;
}
static MB_Result shell_error;
void mb_shell_error(MB_Result error) {
    char code[12];
    shell_error = error;
    mb_window_open("Demo unavailable");
    mb_format_i32(error, code, sizeof code);
    mb_draw_text(1, 2, "Display/audio unavailable.");
    mb_draw_text(1, 4, code);
    mb_draw_text(1, 7, "A or B returns to folder");
}
volatile uint16_t mb_shell_observation[4] __attribute__((externally_visible));
void mb_shell_reset(void) {
    restart_focus = restart_confirm = 0;
    uint16_t i;
    folder = ROOT;
    detail = 0;
    shell_error = MB_OK;
    for (i = 0; i < 32; i++)
        selected[i] = 0;
}
static const char *folder_name(void) {
    uint16_t i;
    if (folder < 11)
        return folders[folder];
    for (i = 0; i < sizeof(entries) / sizeof(entries[0]); i++)
        if (entries[i].category == ROOT && entries[i].id == folder)
            return entries[i].label;
    return "Folder";
}
static const Entry *item(uint16_t index) {
    uint16_t i;
    if (ram_directory()) {
        if (mb_dir_list_path(directory_path(), index, &browser_file) != MB_OK)
            return 0;
        browser_entry.id = 700 + index;
        browser_entry.category = folder;
        browser_entry.icon = UI_ICON_APP;
        browser_entry.app = folder == MUSIC       ? MB_APP_MEGAPLAYER
                            : folder == DOCUMENTS ? MB_APP_MEGAPAD
                                                  : MB_APP_MEGAPAINT;
        browser_entry.context = 0;
        browser_entry.capabilities = MB_CAP_ROM_LINKED;
        browser_entry.preferred = MB_WINDOW_MAXIMIZED;
        browser_entry.min_width = 40;
        browser_entry.min_height = 24;
        browser_entry.modes = 2;
        browser_entry.label = browser_file.name;
        browser_entry.help =
            folder == MUSIC ? "External playback unavailable" : "RAM session document";
        browser_entry.launch = launch;
        return &browser_entry;
    }
    for (i = 0; i < sizeof(entries) / sizeof(entries[0]); i++)
        if (entries[i].category == folder && !index--)
            return &entries[i];
    return 0;
}
static uint16_t count(void) {
    uint16_t n = 0;
    while (item(n))
        n++;
    return n;
}
uint16_t mb_registry_mode_allowed(MB_WindowMode mode, uint16_t width, uint16_t height) {
    uint16_t i;
    for (i = 0; i < sizeof(entries) / sizeof(entries[0]); i++) {
        const Entry *e = &entries[i];
        if (e->launch && e->app == mb_diagnostics.app_id && e->context == mb_app_context())
            return (e->modes & (1 << mode)) && width >= e->min_width && height >= e->min_height;
    }
    return width >= 30 && height >= 14;
}
static void help_page(uint16_t id) {
    mb_draw_text(0, 0, item(selected[folder])->label);
    if (folder == FILES) {
        MB_Volume v;
        char b[11];
        mb_storage_volume(0, &v);
        mb_draw_text(0, 2, "Read only ROM metadata");
        mb_format_u32(v.capacity_bytes, b, sizeof b);
        mb_draw_text(0, 4, b);
        mb_draw_text(0, 6, "User files live in RAM Disk");
        return;
    }
    switch (id) {
    case 606:
        mb_draw_text(0, 2, "MegaPlayer: select, Play, Stop.");
        mb_draw_text(0, 4, "Music by Beatscribe (CC0).");
        mb_draw_text(0, 6, "Bundled FM/PSG tracks play.");
        mb_draw_text(0, 8, "RAM Music: .vgm/.xgc listed.");
        mb_draw_text(0, 10, "External playback unavailable.");
        break;
    case 605:
        mb_draw_text(0, 2, "RAM files last this session.");
        mb_draw_text(0, 4, "Reset/power-off erases files.");
        mb_draw_text(0, 6, "Documents: MegaPad .txt");
        mb_draw_text(0, 8, "Pictures: MegaPaint .mbp");
        mb_draw_text(0, 10, "C opens file actions.");
        mb_draw_text(0, 12, "Music: MegaPlayer .vgm/.xgc");
        break;
    case 600:
        mb_draw_text(0, 2, "Open Apps, Games or System.");
        mb_draw_text(0, 4, "B returns to the prior folder.");
        break;
    case 601:
        mb_draw_text(0, 2, "D-pad selects; A opens.");
        mb_draw_text(0, 4, "B back; C pointer/focus.");
        mb_draw_text(0, 6, "Start pauses Snake.");
        mb_draw_text(0, 8, "C+Start cycles window mode.");
        mb_draw_text(0, 10, "App exit: hold Start+B 1s.");
        break;
    case 602:
        mb_draw_text(0, 2, "Arrows move between pages.");
        mb_draw_text(0, 4, "Mouse left clicks open.");
        mb_draw_text(0, 6, "Mouse right clicks go back.");
        mb_draw_text(0, 8, "Click title to move window.");
        mb_draw_text(0, 10, "C+Start changes window mode.");
        break;
    case 603:
        mb_draw_text(0, 2, "Clock measures session time.");
        mb_draw_text(0, 4, "Calculator uses integers.");
        mb_draw_text(0, 6, "Snake: paused A exits.");
        mb_draw_text(0, 8, "MegaPaint / MegaPad create files.");
        break;
    default:
        mb_draw_text(0, 2, "No SAVE: or clock hardware.");
        mb_draw_text(0, 4, "Themes last for this session.");
        mb_draw_text(0, 6, "Port 2: Sega Mouse driver.");
        mb_draw_text(0, 8, "A hard lock needs reset.");
        break;
    }
}
static void browser_draw(void) {
    char b[12];
    MB_FileInfo f;
    mb_ui_reset();
    /* Enumeration reuses one metadata slot; recover the selected file for modal titles. */
    if (mb_file_stat(browser_path, &browser_file) != MB_OK) {
        mb_draw_text(0, 0, "File unavailable");
        return;
    }
    mb_draw_text(0, 0, browser_file.name);
    if (detail == 2) {
        mb_ui_button(1, 1, 3, 12, "Open", MB_UI_ENABLED);
        mb_ui_button(2, 16, 3, 12, "Rename", MB_UI_ENABLED);
        mb_ui_button(3, 1, 6, 12, "Delete", MB_UI_ENABLED);
        mb_ui_button(4, 16, 6, 12, "Properties", MB_UI_ENABLED);
        mb_ui_focus(1);
    } else if (detail == 3) {
        if (mb_file_stat(browser_path, &f) == MB_OK) {
            mb_draw_text(1, 3, "Size (bytes)");
            mb_format_u32(f.size, b, sizeof b);
            mb_draw_text(1, 5, b);
            mb_draw_text(1, 7, "Temporary: lost on reset.");
        }
        mb_ui_button(5, 1, 10, 12, "Back", MB_UI_ENABLED);
    } else if (detail == 4) {
        mb_draw_text(1, 3, "Delete this file?");
        mb_ui_button(6, 1, 6, 12, "Delete", MB_UI_ENABLED);
        mb_ui_button(5, 16, 6, 12, "Cancel", MB_UI_ENABLED);
        mb_ui_focus(5);
    } else if (detail == 5) {
        /* A maximized shared keyboard gives filenames the same input path as editors. */
        mb_window_set_mode(MB_WINDOW_MAXIMIZED);
        mb_window_clear();
        mb_draw_text(1, 1, "Rename (basename only)");
        mb_draw_field(1, 3, 20, rename_name);
        mb_text_keyboard(0, 10);
    }
    if (browser_message[0])
        mb_draw_text(1, 9, browser_message);
    mb_input_mode(MB_INPUT_FOCUS);
}
static void associated_open(void) {
    uint16_t app = folder == MUSIC       ? MB_APP_MEGAPLAYER
                   : folder == DOCUMENTS ? MB_APP_MEGAPAD
                                         : MB_APP_MEGAPAINT;
    MB_Result r;
    detail = 0;
    mb_launch_context(0);
    mb_launch_document(browser_path);
    r = mb_app_launch_id(app);
    if (r != MB_OK)
        mb_shell_error(r);
}
static void browser_event(const MB_Event *e) {
    uint16_t id;
    MB_Result result = MB_OK;
    if (detail == 5) {
        MB_TextInput t;
        mb_text_event(e, &t);
        if (t.action == MB_TEXT_CANCEL) {
            detail = 2;
            mb_text_reset();
            mb_desktop_draw();
            return;
        }
        if (t.action == MB_TEXT_BACKSPACE && rename_length)
            rename_name[--rename_length] = 0;
        if (t.action == MB_TEXT_CHAR) {
            uint8_t c = t.character;
            if (rename_length < 15 && ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
                                       (c >= '0' && c <= '9') || c == '_' || c == '-')) {
                rename_name[rename_length++] = c;
                rename_name[rename_length] = 0;
            }
        }
        if (t.action == MB_TEXT_DONE) {
            char full[20], target[44];
            uint16_t i = 0, j = 0;
            const char *ext = folder == DOCUMENTS ? ".txt" : ".mbp";
            uint16_t length = 0;
            if (folder == MUSIC) {
                while (browser_path[length])
                    ++length;
                ext = browser_path + length - 4;
            }
            while (i < rename_length) {
                full[i] = rename_name[i];
                ++i;
            }
            while (ext[j])
                full[i++] = ext[j++];
            full[i] = 0;
            file_path(target, full);
            result = mb_file_rename(browser_path, target);
            if (result == MB_OK) {
                detail = 0;
                mb_text_reset();
                mb_desktop_draw();
                return;
            }
        }
        mb_draw_field(1, 3, 20, rename_name);
    } else if (mb_ui_event(e, &id) == MB_OK) {
        if (id == 1) {
            associated_open();
            return;
        }
        if (id == 2) {
            detail = 5;
            rename_length = 0;
            rename_name[0] = 0;
            mb_text_reset();
        } else if (id == 3)
            detail = 4;
        else if (id == 4)
            detail = 3;
        else if (id == 5)
            detail = 2;
        else if (id == 6) {
            result = mb_file_delete(browser_path);
            if (result == MB_OK)
                detail = 0;
        }
        mb_desktop_draw();
    }
    if (result != MB_OK) {
        const char *s = result == MB_EXISTS ? "Name already exists." : "Operation failed.";
        uint16_t i = 0;
        while (s[i]) {
            browser_message[i] = s[i];
            ++i;
        }
        browser_message[i] = 0;
        mb_draw_text(1, 9, browser_message);
    }
}
void mb_shell_draw(void) {
    MB_Rect r;
    uint16_t capacity, cols, n, start, i;
    char b[11];
    mb_shell_surface(folder == ROOT, folder_name());
    mb_window_get_client_rect(&r);
    capacity = mb_grid_capacity(r.width, r.height);
    cols = r.width / 10;
    n = count();
    if (n && selected[folder] >= n)
        selected[folder] = n - 1;
    if (!n)
        selected[folder] = 0;
    mb_shell_observation[0] = folder;
    mb_shell_observation[1] = restart_focus && folder == ROOT ? n : selected[folder];
    mb_shell_observation[2] = capacity ? selected[folder] / capacity : 0;
    mb_shell_observation[3] = restart_confirm ? 6 : detail;
    if (restart_confirm) {
        mb_window_open("Restart MegaBenchOS");
        mb_draw_text(1, 2, "Are you sure?");
        mb_draw_text(1, 4, "RAM Disk files will be lost.");
        /* Register No first so every fresh confirmation defaults to safety. */
        mb_ui_button(1, 5, 7, 8, "No", MB_UI_ENABLED);
        mb_ui_button(2, 17, 7, 8, "Yes", MB_UI_ENABLED);
        return;
    }
    if (detail) {
        if (ram_directory() && detail >= 2) {
            browser_draw();
            return;
        }
        help_page(item(selected[folder])->id);
        return;
    }
    if (!n) {
        mb_draw_text(0, 0, ram_directory() ? "No files here yet." : "Empty folder");
    }
    if (!capacity)
        return;
    start = (selected[folder] / capacity) * capacity;
    for (i = 0; i < capacity && start + i < n; i++) {
        const Entry *e = item(start + i);
        uint16_t x = (i % cols) * 10, y = (i / cols) * 6;
        uint16_t chosen = start + i == selected[folder] && !restart_focus;
        mb_draw_rect(x, y, 10, 5,
                     chosen           ? MB_STYLE_ACCENT
                     : folder == ROOT ? MB_STYLE_DESKTOP
                                      : MB_STYLE_SURFACE);
        mb_shell_icon(x + 4, y + 1, e->icon);
        /* Clip labels to the cell, rather than allowing text into adjacent icons. */
        {
            char label[11];
            uint16_t j;
            for (j = 0; j < 9 && e->label[j]; j++)
                label[j] = e->label[j];
            if (e->label[j] && j)
                label[j - 1] = '>';
            label[j] = 0;
            mb_shell_label(x, y + 3, label, chosen, folder == ROOT);
        }
        if (ram_directory()) {
            mb_format_u32(browser_file.size, b, sizeof b);
            mb_draw_text(x, y + 4, b);
        }
        if (!e->launch && folder != ROOT && folder != HELP && folder != FILES && folder != RAMDISK)
            mb_draw_text(x, y + 4, "[later]");
    }
    if (n > capacity) {
        mb_format_u32(start / capacity + 1, b, sizeof b);
        mb_draw_text(0, r.height - 1, "Page");
        mb_draw_text(5, r.height - 1, b);
        mb_draw_text(8, r.height - 1, "< prev    next >");
    }
    if (folder == RAMDISK || ram_directory()) {
        MB_DriveInfo fs;
        if (mb_fs_capacity("RAM:", 1, &fs) == MB_OK) {
            mb_format_u32(fs.free, b, sizeof b);
            mb_draw_text(0, r.height - 2, "Free bytes:");
            mb_draw_text(12, r.height - 2, b);
        }
    }
    if (folder == ROOT) {
        mb_draw_rect(r.width - 10, r.height - 2, 10, 2,
                     restart_focus ? MB_STYLE_ACCENT : MB_STYLE_SURFACE);
        mb_draw_text(r.width - 9, r.height - 2, "Restart");
    }
}
void mb_shell_event(const MB_Event *e) {
    MB_Input in;
    MB_Rect r;
    uint16_t capacity, cols, n, next = selected[folder];
    if (restart_confirm) {
        uint16_t id;
        mb_input_state(&in);
        mb_window_get_client_rect(&r);
        if (e->action == MB_BACK ||
            (e->action == MB_ACTIVATE && in.mode == MB_INPUT_POINTER && e->x >= r.x * 8 &&
             e->x < (r.x + 3) * 8 && e->y >= (r.y - 1) * 8 && e->y < r.y * 8)) {
            restart_confirm = 0;
            mb_desktop_draw();
        } else if (mb_ui_event(e, &id) == MB_OK) {
            if (id == 2)
                mb_system_restart_request();
            else {
                restart_confirm = 0;
                mb_desktop_draw();
            }
        }
        return;
    }
    if (shell_error) {
        if (e->action == MB_BACK || e->action == MB_ACTIVATE) {
            shell_error = MB_OK;
            mb_desktop_draw();
        }
        return;
    }
    mb_input_state(&in);
    mb_window_get_client_rect(&r);
    cols = r.width / 10;
    capacity = mb_grid_capacity(r.width, r.height);
    n = count();
    if (e->action == MB_BACK) {
        restart_focus = 0;
        if (detail)
            detail = 0;
        else if (folder)
            folder = ram_directory() ? RAMDISK : ROOT;
        mb_text_reset();
        mb_desktop_draw();
        return;
    }
    if (detail && ram_directory()) {
        browser_event(e);
        return;
    }
    if (detail)
        return;
    if (!n || !capacity)
        return;
    if (ram_directory() && e->action == MB_CONTEXT) {
        item(selected[folder]);
        file_path(browser_path, browser_file.name);
        browser_message[0] = 0;
        detail = 2;
        mb_desktop_draw();
        return;
    }
    if (e->action == MB_NAVIGATE) {
        if (folder == ROOT && (restart_focus || (e->y > 0 && next >= cols))) {
            restart_focus = !restart_focus;
            mb_desktop_draw();
            return;
        }
        int16_t delta = e->x ? e->x : e->y * cols;
        next = (next + n + delta) % n;
    } else if ((e->action == MB_MOVE || e->action == MB_ACTIVATE) && in.mode == MB_INPUT_POINTER) {
        if (e->x < r.x * 8 || e->y < r.y * 8 || e->x >= (r.x + r.width) * 8 ||
            e->y >= (r.y + r.height) * 8)
            return;
        if (folder == ROOT && e->y / 8 - r.y >= r.height - 2 && e->x / 8 - r.x >= r.width - 10) {
            restart_focus = 1;
            if (e->action == MB_ACTIVATE)
                restart_confirm = 1;
            mb_desktop_draw();
            return;
        }
        if (e->y / 8 - r.y >= r.height - 2) {
            /* Footer hover is not a third row of icons. Only a click pages. */
            if (n > capacity && e->action == MB_ACTIVATE) {
                uint16_t page = selected[folder] / capacity, pages = (n + capacity - 1) / capacity;
                selected[folder] =
                    ((page + (e->x / 8 - r.x < 16 ? pages - 1 : 1)) % pages) * capacity;
                mb_desktop_draw();
            }
            return;
        }
        next = (selected[folder] / capacity) * capacity + ((e->y / 8 - r.y) / 6) * cols +
               (e->x / 8 - r.x) / 10;
        if (next >= n)
            return;
        if (restart_focus) {
            restart_focus = 0;
            mb_desktop_draw();
        }
    }
    if (next != selected[folder]) {
        selected[folder] = next;
        mb_desktop_draw();
    }
    if (e->action == MB_ACTIVATE) {
        if (folder == ROOT && restart_focus) {
            restart_confirm = 1;
            mb_desktop_draw();
            return;
        }
        const Entry *entry = item(selected[folder]);
        if ((!folder || folder == RAMDISK) && entry->id < 32) {
            folder = entry->id;
            mb_desktop_draw();
        } else if (folder == HELP || folder == FILES) {
            detail = 1;
            mb_desktop_draw();
        } else if (entry->launch &&
                   (mb_system_capabilities() & entry->capabilities) == entry->capabilities) {
            mb_launch_context(entry->context);
            if (ram_directory()) {
                file_path(browser_path, browser_file.name);
                mb_launch_document(browser_path);
            } else
                mb_launch_document(0);
            MB_Result result = entry->launch(entry->app);
            if (result == MB_OK)
                mb_window_set_mode(entry->preferred);
            else
                mb_shell_error(result);
        }
    }
}
#ifdef MB_SELF_TEST
uint16_t mb_shell_test_paging(void) {
    uint16_t i, ok;
    MB_Event e = {MB_NAVIGATE, 1, 0};
    folder = DEMOS;
    selected[DEMOS] = 0;
    detail = 0;
    mb_input_mode(MB_INPUT_FOCUS);
    mb_desktop_draw();
    for (i = 0; i < 23; i++)
        mb_shell_event(&e);
    ok = selected[DEMOS] == 23 && mb_shell_observation[2] == 3;
    mb_app_launch_id(MB_APP_CLOCK);
    mb_app_close();
    ok = ok && folder == DEMOS && selected[DEMOS] == 23 && mb_shell_observation[2] == 3;
    folder = ROOT;
    selected[DEMOS] = 0;
    selected[ROOT] = 23;
    mb_desktop_draw();
    ok = ok && mb_shell_observation[2] == 1;
    e.action = MB_ACTIVATE;
    mb_shell_event(&e);
    e.action = MB_BACK;
    mb_shell_event(&e);
    ok = ok && !folder && selected[ROOT] == 23 && mb_shell_observation[2] == 1;
    selected[ROOT] = 0;
    mb_desktop_draw();
    return ok;
}
#endif
