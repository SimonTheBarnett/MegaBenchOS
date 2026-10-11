/**
 * @file apps/megaplayer/megaplayer.c
 * @brief SDK-only music UI. Five visible catalog/file entries, bounded paging and no song
 *
 * ROM-linked application code. Shared hardware and resource ownership remain in OS services.
 * buffer. File paths are borrowed only for synchronous public VFS/media calls.
 */
#include "megabenchos.h"
static uint16_t source, selected, page, count, displayed_state, credits;
static MB_Result result;
static char opened[64];
volatile uint16_t mb_player_observation[5] __attribute__((externally_visible));
static void snapshot(void) {
    mb_player_observation[0] = source;
    mb_player_observation[1] = selected;
    mb_player_observation[2] = page;
    mb_player_observation[3] = displayed_state;
    mb_player_observation[4] = result;
}
static void status(void) {
    const char *message;
    displayed_state = mb_music_state();
    message = result == MB_CORRUPT                  ? "Invalid compiled music file"
              : result == MB_VERSION                ? "Unsupported music version"
              : result == MB_NOT_SUPPORTED          ? "External playback unavailable"
              : result != MB_OK                     ? "Music unavailable / file error"
              : displayed_state == MB_MUSIC_PLAYING ? "Playing"
                                                    : "Stopped";
    mb_draw_field(1, 21, 38, message);
    snapshot();
}
static void file_path(char *path, const char *name) {
    const char *prefix = "RAM:/Music/";
    uint16_t i = 0, j = 0;
    while (prefix[j])
        path[i++] = prefix[j++];
    j = 0;
    do {
        path[i] = name[j];
        ++i;
    } while (name[j++]);
}
/* Two bounded lines keep complete long titles visible on a 40-column screen. */
static void title_lines(uint16_t x, uint16_t y, uint16_t width, const char *title) {
    char line[40];
    uint16_t i = 0, n, space = 0;
    for (n = 0; n < width && title[i]; n++)
        line[n] = title[i++];
    if (title[i]) {
        for (n = 0; n < i; n++)
            if (line[n] == ' ')
                space = n;
        if (space) {
            i = space + 1;
            n = space;
        } else
            n = i;
    }
    line[n] = 0;
    mb_draw_field(x, y, width, line);
    mb_draw_field(x, y + 1, width, title + i);
}
static void draw(void) {
    MB_MusicTrack track;
    MB_FileInfo file;
    MB_DriveInfo drive;
    uint16_t i;
    char number[12];
    mb_window_open("MegaPlayer");
    if (credits) {
        mb_draw_text(1, 1, "Music credits - CC0 1.0");
        if (!source && mb_music_track(selected, 1, &track) == MB_OK)
            title_lines(1, 3, 38, track.title);
        else
            mb_draw_text(1, 3, "Bundled music collection credits");
        mb_draw_text(1, 6, "Safety Stoat Studios");
        if (source || selected == 1) {
            mb_draw_text(1, 8, "Mad Bossa composed by Spring");
            mb_draw_text(1, 9, "VGM transcription by Zachary");
            mb_draw_text(1, 10, "at Safety Stoat Studios");
        }
        mb_draw_text(1, 12, "github.com/jerellsworth/free_vgms");
        mb_draw_text(1, 14, "safetystoatstudios.itch.io/free-vgms");
        mb_draw_text(1, 16, "Sources and licence: music/CREDITS.md");
        mb_ui_button(7, 2, 18, 12, "Back", MB_UI_ENABLED);
        mb_ui_button(6, 17, 18, 12, "Stop", MB_UI_ENABLED);
        mb_ui_focus(7);
        status();
        return;
    }
    mb_ui_button(1, 1, 0, 18, "Bundled", MB_UI_ENABLED);
    mb_ui_button(2, 21, 0, 18, "RAM Disk", MB_UI_ENABLED);
    count = 0;
    if (!source) {
        while (mb_music_track(count, 1, &track) == MB_OK)
            ++count;
    } else if (mb_fs_capacity("RAM:", 1, &drive) == MB_OK && (drive.capabilities & MB_FS_MOUNTED) &&
               (drive.capabilities & MB_FS_OFFSET_READ)) {
        while (mb_dir_list_path("RAM:/Music/", count, &file) == MB_OK)
            ++count;
    }
    if (selected >= count)
        selected = 0;
    if (page >= count)
        page = 0;
    for (i = 0; i < 5 && page + i < count; i++) {
        const char *title;
        if (!source) {
            mb_music_track(page + i, 1, &track);
            title = track.title;
        } else {
            mb_dir_list_path("RAM:/Music/", page + i, &file);
            title = file.name;
        }
        number[0] = selected == page + i ? '*' : ' ';
        mb_format_u32(page + i + 1, number + 1, sizeof number - 1);
        mb_ui_button(10 + i, 1, 2 + i * 2, 8, number, MB_UI_ENABLED);
        title_lines(10, 2 + i * 2, 29, title);
    }
    if (!count)
        mb_draw_text(1, 3, "No music files in RAM:/Music");
    if (count > 5) {
        mb_ui_button(3, 1, 12, 12, "Previous", MB_UI_ENABLED);
        mb_ui_button(4, 15, 12, 12, "Next", MB_UI_ENABLED);
    }
    if (!source && mb_music_track(selected, 1, &track) == MB_OK) {
        mb_draw_text(1, 15, track.credit);
        mb_draw_text(1, 16, "Verified ROM XGM2 / play once");
    } else {
        mb_draw_text(1, 15, "RAM files last this session");
        mb_draw_text(1, 16, ".vgm/.xgc external play unavailable");
    }
    mb_ui_button(5, 3, 18, 12, "Play", count ? MB_UI_ENABLED : 0);
    mb_ui_button(6, 17, 18, 12, "Stop", MB_UI_ENABLED);
    mb_ui_button(7, 31, 18, 9, "Credits", MB_UI_ENABLED);
    mb_ui_focus(count ? 10 + selected % 5 : source ? 2 : 1);
    status();
}
static MB_Result init(void) {
    source = selected = page = count = credits = 0;
    result = MB_OK;
    displayed_state = MB_MUSIC_STOPPED;
    opened[0] = 0;
    return MB_OK;
}
static void activate(void) {
    MB_FileInfo file;
    uint16_t i;
    mb_input_mode(MB_INPUT_FOCUS);
    mb_window_open("MegaPlayer");
    mb_window_set_mode(MB_WINDOW_MAXIMIZED);
    if (mb_app_document(opened, sizeof opened) == MB_OK)
        source = 1;
    if (opened[0]) {
        for (i = 0; mb_dir_list_path("RAM:/Music/", i, &file) == MB_OK; i++) {
            char path[64];
            uint16_t n = 0;
            file_path(path, file.name);
            while (path[n] && path[n] == opened[n])
                ++n;
            if (!path[n] && !opened[n]) {
                selected = i;
                page = (i / 5) * 5;
                break;
            }
        }
        result = mb_music_play_file(opened);
        opened[0] = 0;
    }
    draw();
}
static void event(const MB_Event *e) {
    uint16_t id;
    if (e->action == MB_RESIZE || e->action == MB_THEME_CHANGED) {
        draw();
        return;
    }
    if (mb_ui_event(e, &id) != MB_OK)
        return;
    if (id == 7) {
        credits = !credits;
        draw();
    } else if (id == 1 || id == 2) {
        mb_music_stop();
        source = id - 1;
        selected = page = 0;
        result = MB_OK;
        draw();
    } else if (id == 3 || id == 4) {
        uint16_t pages = (count + 4) / 5;
        page = ((page / 5 + (id == 3 ? pages - 1 : 1)) % pages) * 5;
        selected = page;
        draw();
    } else if (id >= 10 && id <= 14) {
        selected = page + id - 10;
        draw();
    } else if (id == 5) {
        if (source) {
            MB_FileInfo file;
            char path[64];
            result = mb_dir_list_path("RAM:/Music/", selected, &file);
            if (result == MB_OK) {
                file_path(path, file.name);
                result = mb_music_play_file(path);
            }
        } else
            result = mb_music_play(selected);
        status();
    } else if (id == 6) {
        mb_music_stop();
        result = MB_OK;
        status();
    }
}
static void tick(void) {
    if (displayed_state != mb_music_state())
        status();
}
static void stop(void) {
    mb_music_stop();
}
const MB_App megaplayer_app = {MB_API_VERSION, "MegaPlayer", init, activate,
                               event,          tick,         stop, stop};
