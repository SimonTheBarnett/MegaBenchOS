/**
 * @file apps/creative/megapaint.c
 * @brief MegaPaint: SDK-only variable-size sparse picture editor. Shared OS widgets form
 *
 * ROM-linked application code. Shared hardware and resource ownership remain in OS services.
 * temporary menus; PAL2/VDP ownership and bounded dirty transfers remain in the OS.
 */
#include "paint_model.h"
enum {
    DRAW,
    FILE_MENU,
    TOOLS_MENU,
    COLOURS_MENU,
    VIEW_MENU,
    NEW_MENU,
    PALETTE_MENU,
    NAME_VIEW,
    OPEN_VIEW,
    CONFIRM_VIEW,
    ERROR_VIEW
};
enum { NEW_IMAGE = 1, OPEN_IMAGE, SAVE_IMAGE, SAVE_AS_IMAGE, CLOSE_IMAGE, CLEAR_IMAGE };
static Picture paint_picture;
/* File callbacks are synchronous and rejected during fill. Import needs both
 * compressed bytes and one 8-row band; neither overlaps the persistent picture. */
static union {
    PaintFill fill;
    struct {
        uint8_t bytes[PAINT_FILE_LIMIT], band[PAINT_MAX_W * 4];
    } io;
} paint_work;
#define paint_fill paint_work.fill
#define paint_file paint_work.io.bytes
static uint16_t paint_menu, paint_dirty, paint_tool, paint_brush = 1, paint_color = 1;
static uint16_t paint_x, paint_y, paint_ox, paint_oy, paint_zoom = 1, paint_pan;
static uint16_t paint_action, paint_page, paint_name_length, paint_pointer, paint_vw, paint_vh;
static uint16_t paint_canvas_y;
static uint32_t paint_move_tick;
static int16_t paint_drag_x, paint_drag_y;
static uint16_t paint_dragging;
static uint16_t paint_wait_release;
static char paint_path[44], paint_name[16], paint_message[39];
volatile uint16_t mb_paint_observation[12] __attribute__((externally_visible));
volatile uint16_t mb_paint_events[5] __attribute__((externally_visible));
static void draw(void);
static void observe(void) {
    mb_paint_observation[0] = paint_picture.width;
    mb_paint_observation[1] = paint_picture.height;
    mb_paint_observation[2] = paint_x;
    mb_paint_observation[3] = paint_y;
    mb_paint_observation[4] = paint_zoom;
    mb_paint_observation[5] = paint_menu;
    mb_paint_observation[6] = paint_color;
    mb_paint_observation[7] = paint_brush;
    mb_paint_observation[8] = paint_picture.used;
    mb_paint_observation[9] = paint_dirty;
    mb_paint_observation[10] = paint_fill.phase;
    mb_paint_observation[11] = paint_tool;
    mb_app_close_guard(paint_dirty || paint_fill.phase);
}
static void make_path(char *out, const char *name, uint16_t extension) {
    const char *prefix = "RAM:/Pictures/";
    uint16_t i = 0, j = 0;
    while (prefix[j])
        out[i++] = prefix[j++];
    j = 0;
    while (name[j] && i < 39)
        out[i++] = name[j++];
    if (extension) {
        out[i++] = '.';
        out[i++] = 'm';
        out[i++] = 'b';
        out[i++] = 'p';
    }
    out[i] = 0;
}
static void copy_path(const char *p) {
    uint16_t i = 0;
    do {
        paint_path[i] = p[i];
    } while (p[i] && ++i < 43);
    paint_path[43] = 0;
}
static void message(const char *text) {
    uint16_t i = 0;
    while (text[i] && i < 38) {
        paint_message[i] = text[i];
        ++i;
    }
    paint_message[i] = 0;
    paint_menu = ERROR_VIEW;
    draw();
}
static void error(MB_Result r) {
    message(r == MB_NO_SPACE  ? "Image exceeds free RAM Disk space."
            : r == MB_LIMIT   ? "Image detail exceeds tile memory."
            : r == MB_EXISTS  ? "Filename already exists."
            : r == MB_VERSION ? "Unsupported picture dimensions/format."
            : r == MB_CORRUPT ? "Invalid picture; current edits kept."
                              : "File operation failed; edits kept.");
}
static MB_Result load_file(const char *path) {
    MB_FileInfo info;
    MB_FileHandle h;
    MB_Result r, close;
    uint16_t n;
    if ((r = mb_file_stat(path, &info)) != MB_OK)
        return r;
    if (info.directory != MB_DIR_PICTURES)
        return MB_INVALID;
    if (info.size > sizeof paint_file)
        return MB_NO_SPACE;
    if ((r = mb_file_open(path, MB_FILE_READ, &h)) != MB_OK)
        return r;
    r = mb_file_read(h, 0, paint_file, sizeof paint_file, &n);
    close = mb_file_close(h);
    if (r != MB_OK)
        return r;
    if (close != MB_OK)
        return close;
    if (n != info.size)
        return MB_CORRUPT;
    r = picture_decode(&paint_picture, paint_file, n, paint_work.io.band);
    if (r == MB_NO_SPACE)
        return MB_LIMIT;
    if (r != MB_OK)
        return r;
    mb_canvas_close();
    copy_path(path);
    paint_dirty = paint_x = paint_y = paint_ox = paint_oy = 0;
    paint_zoom = 1;
    return MB_OK;
}
static MB_Result save_file(const char *path, uint16_t fresh) {
    MB_DriveInfo d;
    MB_FileInfo info;
    MB_FileHandle h;
    MB_Result r, close;
    uint16_t n, exists;
    if ((r = mb_fs_capacity("RAM:", 1, &d)) != MB_OK)
        return r;
    if (!(d.capabilities & MB_FS_WRITABLE))
        return MB_READ_ONLY;
    if (!(d.capabilities & MB_FS_ATOMIC_REPLACE))
        return MB_NOT_SUPPORTED;
    r = mb_file_stat(path, &info);
    exists = r == MB_OK;
    if (r != MB_OK && r != MB_NOT_FOUND)
        return r;
    if (fresh && exists)
        return MB_EXISTS;
    if ((r = picture_encode(&paint_picture, paint_file, sizeof paint_file, &n)) != MB_OK)
        return r;
    if ((uint32_t)n > d.free + (exists ? info.size : 0))
        return MB_NO_SPACE;
    if ((r = mb_file_open(path, MB_FILE_WRITE | MB_FILE_CREATE, &h)) != MB_OK)
        return r;
    r = mb_file_write(h, paint_file, n);
    close = mb_file_close(h);
    if (r == MB_OK)
        r = close;
    if (r != MB_OK) {
        if (!exists)
            mb_file_delete(path);
        return r;
    }
    copy_path(path);
    paint_dirty = 0;
    return MB_OK;
}
/* Every success/failure leaves the union in inactive-fill state. A later fill
 * initializes its masks/queue before use; app callbacks cannot reenter this I/O. */
static MB_Result load(const char *path) {
    MB_Result r;
    if (paint_fill.phase)
        return MB_BUSY;
    r = load_file(path);
    paint_fill.phase = 0;
    return r;
}
static MB_Result save(const char *path, uint16_t fresh) {
    MB_Result r;
    if (paint_fill.phase)
        return MB_BUSY;
    r = save_file(path, fresh);
    paint_fill.phase = 0;
    return r;
}
static void perform(uint16_t action) {
    MB_Result r;
    paint_action = action;
    if (action == NEW_IMAGE)
        paint_menu = NEW_MENU;
    else if (action == OPEN_IMAGE) {
        paint_page = 0;
        paint_menu = OPEN_VIEW;
    } else if (action == SAVE_IMAGE && paint_path[0]) {
        r = save(paint_path, 0);
        if (r != MB_OK) {
            error(r);
            return;
        }
        paint_menu = DRAW;
    } else if (action == SAVE_IMAGE || action == SAVE_AS_IMAGE) {
        paint_name_length = 0;
        paint_name[0] = 0;
        mb_text_reset();
        paint_menu = NAME_VIEW;
    } else if (action == CLEAR_IMAGE) {
        picture_clear(&paint_picture, 0);
        paint_dirty = 1;
        paint_menu = DRAW;
        mb_canvas_damage(0, 0, paint_picture.width, paint_picture.height);
    } else if (action == CLOSE_IMAGE) {
        mb_app_close_guard(0);
        mb_app_close();
        return;
    }
    draw();
}
static void request(uint16_t action) {
    if (action == CLEAR_IMAGE ||
        (paint_dirty && (action == NEW_IMAGE || action == OPEN_IMAGE || action == CLOSE_IMAGE))) {
        paint_action = action;
        paint_menu = CONFIRM_VIEW;
        draw();
    } else
        perform(action);
}
static uint16_t visible(uint16_t pixels) {
    return paint_zoom ? pixels / paint_zoom : pixels * 2;
}
static void clamp_view(uint16_t follow) {
    uint16_t w = visible(paint_vw * 8), h = visible(paint_vh * 8);
    if (follow) {
        if (paint_x < paint_ox)
            paint_ox = paint_x;
        if (paint_y < paint_oy)
            paint_oy = paint_y;
        if (paint_x >= paint_ox + w)
            paint_ox = paint_x - w + 1;
        if (paint_y >= paint_oy + h)
            paint_oy = paint_y - h + 1;
    }
    if (w >= paint_picture.width)
        paint_ox = 0;
    else if (paint_ox > paint_picture.width - w)
        paint_ox = paint_picture.width - w;
    if (h >= paint_picture.height)
        paint_oy = 0;
    else if (paint_oy > paint_picture.height - h)
        paint_oy = paint_picture.height - h;
    mb_canvas_view(paint_ox, paint_oy, paint_zoom, paint_x, paint_y);
    observe();
}
static void bar(void) {
    uint16_t id;
    static const char *const labels[] = {"File", "Tools", "Colours", "View"};
    mb_ui_reset();
    for (id = 1; id <= 4; ++id)
        mb_ui_menu_button(id, (id - 1) * 10, 10, labels[id - 1],
                          MB_UI_ENABLED | (paint_menu == id ? MB_UI_MENU_OPEN : 0));
}
static void overlay(uint16_t x, uint16_t w, uint16_t h) {
    mb_draw_rect(x, 2, w, h, MB_STYLE_SURFACE);
}
static void button(uint16_t id, uint16_t x, uint16_t y, uint16_t w, const char *s) {
    mb_ui_button(id, x, y, w, s, MB_UI_ENABLED);
}
static void draw(void) {
    MB_Rect r, content;
    MB_CanvasSource source = {
        1, paint_picture.width, paint_picture.height, &paint_picture, picture_pixel, picture_tile};
    uint16_t i;
    char b[12];
    MB_Result result;
    mb_window_get_client_rect(&r);
    mb_window_get_content_rect(1, &content);
    paint_vw = content.width;
    paint_vh = content.height;
    paint_canvas_y = content.y - r.y;
    result =
        mb_canvas_attach(&source, 0, paint_canvas_y, paint_vw, paint_vh, paint_picture.palette);
    if (result != MB_OK) {
        mb_app_fail(result);
        return;
    }
    mb_canvas_brush(paint_tool == 2 ? 1 : paint_brush);
    mb_canvas_cursor(2);
    clamp_view(0);
    bar();
    if (paint_menu == FILE_MENU) {
        overlay(0, 18, 10);
        button(100, 1, 3, 16, "New");
        button(101, 1, 5, 16, "Open");
        button(102, 1, 7, 16, "Save");
        button(103, 1, 9, 16, "Save As");
        mb_ui_focus(100);
    } else if (paint_menu == TOOLS_MENU) {
        const char *names[] = {"Pencil",     "Eraser",     "Fill",       "Clear",
                               "Brush 1 px", "Brush 2 px", "Brush 4 px", "Brush 8 px"};
        overlay(10, 20, 18);
        for (i = 0; i < 8; ++i)
            button(300 + i, 11, 3 + i * 2, 18, names[i]);
        mb_ui_focus(300 + paint_tool);
    } else if (paint_menu == COLOURS_MENU) {
        overlay(0, 40, 12);
        for (i = 0; i < 16; ++i) {
            /* Two rows of 16px squares; empty shared widgets retain the swatch
             * across hover/focus and include the entire square in their hit area. */
            button(400 + i, 8 + (i % 8) * 3, 3 + (i / 8) * 3, 3, "");
            mb_canvas_swatch(10 + (i % 8) * 3, 3 + (i / 8) * 3, i);
        }
        button(416, 8, 9, 24, "Edit RGB");
        mb_ui_focus(400 + paint_color);
    } else if (paint_menu == VIEW_MENU) {
        const char *names[] = {"Fit", "1:1", "Zoom In", "Zoom Out", "Pan"};
        overlay(22, 18, 12);
        for (i = 0; i < 5; ++i)
            button(500 + i, 23, 3 + i * 2, 16, names[i]);
        mb_ui_focus(500);
    } else if (paint_menu == NEW_MENU) {
        overlay(0, 26, 12);
        mb_draw_text(1, 3, "New picture size");
        button(200, 1, 5, 24, "Window size");
        button(201, 1, 7, 24, "512 x 256");
        button(202, 1, 9, 24, "256 x 256");
        button(203, 1, 11, 24, "64 x 64");
        mb_ui_focus(200);
    } else if (paint_menu == PALETTE_MENU) {
        uint16_t c = paint_picture.palette[paint_color];
        overlay(0, 30, 14);
        mb_draw_text(1, 3, "Edit RGB (0..7)");
        mb_format_u32((c >> 1) & 7, b, sizeof b);
        mb_draw_field(1, 5, 8, b);
        mb_format_u32((c >> 5) & 7, b, sizeof b);
        mb_draw_field(11, 5, 8, b);
        mb_format_u32((c >> 9) & 7, b, sizeof b);
        mb_draw_field(21, 5, 8, b);
        button(600, 1, 7, 8, "R-");
        button(601, 11, 7, 8, "R+");
        button(602, 1, 9, 8, "G-");
        button(603, 11, 9, 8, "G+");
        button(604, 1, 11, 8, "B-");
        button(605, 11, 11, 8, "B+");
        button(606, 1, 13, 18, "Done");
        mb_canvas_swatch(25, 7, paint_color);
    } else if (paint_menu == NAME_VIEW) {
        overlay(0, 40, 22);
        mb_draw_text(1, 4, "New filename (letters/digits/_/-)");
        mb_draw_field(1, 7, 20, paint_name);
        mb_text_keyboard(0, 16);
    } else if (paint_menu == OPEN_VIEW) {
        MB_FileInfo f;
        overlay(0, 40, 22);
        mb_draw_text(1, 3, "Pictures (*.mbp)");
        for (i = 0; i < 6; ++i)
            if (mb_dir_list_path("RAM:/Pictures/", paint_page * 6 + i, &f) == MB_OK)
                button(700 + i, 1, 5 + i * 2, 30, f.name);
            else
                break;
        if (!i)
            mb_draw_text(1, 5, "No pictures saved.");
        button(706, 1, 20, 10, "Prev");
        button(707, 13, 20, 10, "Next");
        button(708, 25, 20, 12, "Cancel");
    } else if (paint_menu == CONFIRM_VIEW) {
        overlay(0, 40, 10);
        mb_draw_text(1, 4,
                     paint_action == CLEAR_IMAGE ? "Clear picture?" : "Discard unsaved changes?");
        button(800, 3, 8, 12, "Yes");
        button(801, 20, 8, 12, "Cancel");
        mb_ui_focus(801);
    } else if (paint_menu == ERROR_VIEW) {
        overlay(0, 40, 10);
        mb_draw_text(1, 4, paint_message);
        button(900, 3, 8, 12, "OK");
        mb_ui_focus(900);
    }
    if (!paint_menu) {
        MB_Input in;
        mb_input_state(&in);
        if (in.buttons & MB_KEY_A)
            paint_wait_release = 1;
    }
    if (paint_menu) {
        mb_canvas_view(paint_ox, paint_oy, paint_zoom, 65535, 65535);
        mb_input_mode(paint_pointer ? MB_INPUT_POINTER : MB_INPUT_FOCUS);
    } else
        mb_input_mode(paint_pointer ? MB_INPUT_POINTER : MB_INPUT_GAME);
    observe();
}
static void stamp(void) {
    MB_Result r;
    ++mb_paint_events[3];
    if (paint_pan || paint_fill.phase)
        return;
    if (paint_tool == 2) {
        r = picture_fill_begin(&paint_picture, &paint_fill, paint_x, paint_y, paint_color);
        if (r == MB_OK && paint_fill.phase)
            paint_dirty = 1;
    } else {
        MB_Rect damage;
        r = picture_stamp(&paint_picture, paint_x, paint_y, paint_brush,
                          paint_tool == 1 ? 0 : paint_color, &damage);
        if (r == MB_OK) {
            paint_dirty = 1;
            mb_canvas_damage(damage.x, damage.y, damage.width, damage.height);
        }
    }
    mb_paint_events[4] = r;
    if (r != MB_OK) {
        message("Detail tile limit; stroke not applied.");
        return;
    }
    observe();
}
static void point(const MB_Event *e) {
    MB_Input in;
    MB_Rect r;
    int16_t x = paint_x, y = paint_y;
    mb_input_state(&in);
    mb_window_get_client_rect(&r);
    if (in.mode == MB_INPUT_POINTER) {
        int16_t sx = e->x - r.x * 8, sy = e->y - (r.y + paint_canvas_y) * 8;
        if (sx < 0 || sy < 0 || sx >= paint_vw * 8 || sy >= paint_vh * 8)
            return;
        if (paint_pan) {
            if (in.buttons & MB_KEY_A) {
                if (paint_dragging) {
                    int16_t dx = paint_drag_x - e->x, dy = paint_drag_y - e->y;
                    int16_t ox = paint_ox + (paint_zoom ? dx / (int16_t)paint_zoom : dx * 2),
                            oy = paint_oy + (paint_zoom ? dy / (int16_t)paint_zoom : dy * 2);
                    paint_ox = ox < 0 ? 0 : ox;
                    paint_oy = oy < 0 ? 0 : oy;
                    clamp_view(0);
                }
                paint_drag_x = e->x;
                paint_drag_y = e->y;
                paint_dragging = 1;
            } else
                paint_dragging = 0;
            return;
        }
        x = paint_ox + (paint_zoom ? sx / paint_zoom : sx * 2);
        y = paint_oy + (paint_zoom ? sy / paint_zoom : sy * 2);
    } else if (e->action == MB_NAVIGATE) {
        if (paint_pan) {
            int16_t ox = paint_ox + e->x * 8, oy = paint_oy + e->y * 8;
            paint_ox = ox < 0 ? 0 : ox;
            paint_oy = oy < 0 ? 0 : oy;
            clamp_view(0);
            return;
        }
        x += e->x;
        y += e->y;
    }
    if (x < 0)
        x = 0;
    if (y < 0)
        y = 0;
    if (x >= paint_picture.width)
        x = paint_picture.width - 1;
    if (y >= paint_picture.height)
        y = paint_picture.height - 1;
    paint_x = x;
    paint_y = y;
    clamp_view(1);
    paint_move_tick = mb_time_ticks();
    if (!paint_wait_release && (e->action == MB_ACTIVATE || (in.buttons & MB_KEY_A)))
        stamp();
}
static void choice(uint16_t id) {
    uint16_t i;
    if (id == MB_UI_MENU_DISMISS) {
        paint_menu = DRAW;
        paint_wait_release = 1;
        draw();
        return;
    }
    if (id >= 1 && id <= 4) {
        paint_menu = paint_menu == id ? DRAW : id;
        draw();
        return;
    }
    if (id >= 100 && id <= 103) {
        request(id - 99);
        return;
    }
    if (id >= 200 && id <= 203) {
        uint16_t w = id == 200 ? paint_vw * 8 : id == 201 ? 512 : id == 202 ? 256 : 64;
        uint16_t h = id == 200 ? paint_vh * 8 : id == 203 ? 64 : 256;
        mb_canvas_close();
        picture_new(&paint_picture, w, h);
        paint_fill.phase = 0;
        paint_dirty = paint_x = paint_y = paint_ox = paint_oy = paint_pan = 0;
        paint_path[0] = 0;
        paint_zoom = 1;
        paint_menu = DRAW;
        draw();
        return;
    }
    if (id >= 300 && id <= 302) {
        paint_tool = id - 300;
        paint_pan = 0;
        paint_menu = DRAW;
        draw();
        return;
    }
    if (id == 303) {
        request(CLEAR_IMAGE);
        return;
    }
    if (id >= 304 && id <= 307) {
        paint_brush = 1 << (id - 304);
        paint_menu = DRAW;
        draw();
        return;
    }
    if (id >= 400 && id <= 415) {
        paint_color = id - 400;
        paint_menu = DRAW;
        draw();
        return;
    }
    if (id == 416) {
        paint_menu = PALETTE_MENU;
        draw();
        return;
    }
    if (id >= 500 && id <= 504) {
        paint_pan = id == 504;
        if (id == 500) {
            paint_zoom = 1;
            paint_ox = paint_oy = 0;
            while ((uint32_t)paint_picture.width * paint_zoom <= paint_vw * 4 &&
                   (uint32_t)paint_picture.height * paint_zoom <= paint_vh * 4 && paint_zoom < 8)
                paint_zoom *= 2;
            if (paint_picture.width > paint_vw * 8 || paint_picture.height > paint_vh * 8)
                paint_zoom = 0;
        } else if (id == 501)
            paint_zoom = 1;
        else if (id == 502)
            paint_zoom = paint_zoom ? (paint_zoom < 8 ? paint_zoom * 2 : 8) : 1;
        else if (id == 503)
            paint_zoom = paint_zoom > 1 ? paint_zoom / 2 : 0;
        paint_menu = DRAW;
        clamp_view(id != 500 && id != 504);
        draw();
        return;
    }
    if (id >= 600 && id <= 605) {
        uint16_t shift = 1 + ((id - 600) / 2) * 4, c = paint_picture.palette[paint_color],
                 v = (c >> shift) & 7;
        v = (id & 1) ? (v < 7 ? v + 1 : 7) : (v ? v - 1 : 0);
        paint_picture.palette[paint_color] = (c & ~(7 << shift)) | (v << shift);
        paint_dirty = 1;
        draw();
        mb_ui_focus(id);
        return;
    }
    if (id == 606 || id == 708 || id == 801 || id == 900) {
        paint_menu = DRAW;
        draw();
        return;
    }
    if (id >= 700 && id <= 705) {
        MB_FileInfo f;
        char path[44];
        MB_Result r;
        if (mb_dir_list_path("RAM:/Pictures/", paint_page * 6 + id - 700, &f) == MB_OK) {
            make_path(path, f.name, 0);
            r = load(path);
            if (r != MB_OK) {
                error(r);
                return;
            }
        }
        paint_menu = DRAW;
        draw();
        return;
    }
    if (id == 706 || id == 707) {
        MB_FileInfo f;
        if (id == 706) {
            if (paint_page)
                --paint_page;
        } else if (mb_dir_list_path("RAM:/Pictures/", (paint_page + 1) * 6, &f) == MB_OK)
            ++paint_page;
        draw();
        return;
    }
    if (id == 800) {
        i = paint_action;
        perform(i);
    }
}
static void event(const MB_Event *e) {
    MB_Input in;
    uint16_t id;
    mb_input_state(&in);
    ++mb_paint_events[0];
    mb_paint_events[1] = e->action;
    mb_paint_events[2] = in.mode;
    if (e->action == MB_CLOSE)
        return;
    if (e->action == MB_RESIZE || e->action == MB_THEME_CHANGED) {
        draw();
        return;
    }
    if (e->action == MB_CLOSE_REQUEST) {
        request(CLOSE_IMAGE);
        return;
    }
    if (e->action == MB_BACK) {
        if (paint_fill.phase == 1) {
            picture_fill_cancel(&paint_picture, &paint_fill);
            observe();
        }
        if (!paint_fill.phase) {
            paint_menu = DRAW;
            draw();
        }
        return;
    }
    if (paint_fill.phase)
        return;
    if (e->action == MB_CONTEXT) {
        paint_pointer = in.mode == MB_INPUT_GAME || in.mode == MB_INPUT_POINTER;
        draw();
        return;
    }
    if (e->action == MB_MENU) {
        paint_pointer = in.mode == MB_INPUT_POINTER;
        paint_menu = paint_menu ? DRAW : FILE_MENU;
        draw();
        return;
    }
    if (paint_menu == NAME_VIEW) {
        MB_TextInput t;
        mb_text_event(e, &t);
        if (t.action == MB_TEXT_CANCEL) {
            paint_menu = DRAW;
            draw();
            return;
        }
        if (t.action == MB_TEXT_DONE) {
            char path[44];
            MB_Result r;
            if (!paint_name_length) {
                error(MB_INVALID);
                return;
            }
            make_path(path, paint_name, 1);
            r = save(path, 1);
            if (r != MB_OK) {
                error(r);
                return;
            }
            paint_menu = DRAW;
            draw();
            return;
        }
        if (t.action == MB_TEXT_BACKSPACE && paint_name_length)
            paint_name[--paint_name_length] = 0;
        if (t.action == MB_TEXT_CHAR && paint_name_length < 15) {
            uint8_t c = t.character;
            if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') ||
                c == '_' || c == '-') {
                paint_name[paint_name_length++] = c;
                paint_name[paint_name_length] = 0;
            }
        }
        mb_draw_field(1, 7, 20, paint_name);
        return;
    }
    if ((paint_menu || in.mode == MB_INPUT_POINTER) && mb_ui_event(e, &id) == MB_OK) {
        choice(id);
        return;
    }
    if (!paint_menu &&
        (e->action == MB_MOVE || e->action == MB_NAVIGATE || e->action == MB_ACTIVATE))
        point(e);
}
static MB_Result init(void) {
    return MB_OK;
}
static void activate(void) {
    char path[44];
    MB_Result r;
    paint_menu = paint_dirty = paint_tool = paint_x = paint_y = paint_ox = paint_oy = paint_pan =
        paint_pointer = paint_fill.phase = 0;
    paint_color = paint_brush = paint_zoom = 1;
    /* One pointer crosses the canvas/menu boundary without changing input mode.
     * C still offers the precise controller-only drawing alternative. */
    paint_pointer = 1;
    paint_dragging = paint_wait_release = 0;
    paint_path[0] = 0;
    paint_move_tick = mb_time_ticks();
    mb_window_open("MegaPaint");
    mb_window_set_mode(MB_WINDOW_MAXIMIZED);
    {
        MB_Rect content;
        r = mb_window_get_content_rect(1, &content);
        if (r == MB_OK)
            r = picture_new(&paint_picture, content.width * 8, content.height * 8);
        if (r != MB_OK) {
            mb_app_fail(r);
            return;
        }
    }
    mb_app_back_intercept(1);
    r = mb_app_document(path, sizeof path);
    if (r == MB_OK && (r = load(path)) != MB_OK) {
        error(r);
        return;
    }
    draw();
}
static void tick(void) {
    MB_Input in;
    uint32_t now = mb_time_ticks();
    mb_input_state(&in);
    if (paint_wait_release) {
        if (!(in.buttons & MB_KEY_A))
            paint_wait_release = 0;
        else
            return;
    }
    if (paint_fill.phase) {
        uint16_t before = paint_fill.phase == 2 ? paint_fill.commit : 0, i;
        picture_fill_step(&paint_picture, &paint_fill, 4);
        if (paint_fill.phase != 1)
            for (i = before; i < paint_fill.commit; ++i)
                mb_canvas_damage((i % (paint_picture.width / 8)) * 8,
                                 (i / (paint_picture.width / 8)) * 8, 8, 8);
        observe();
        return;
    }
    if (paint_menu || now - paint_move_tick < 10)
        return;
    mb_input_state(&in);
    if (in.mode == MB_INPUT_GAME && (in.buttons & 15)) {
        MB_Event e = {MB_NAVIGATE, ((in.buttons & 8) != 0) - ((in.buttons & 4) != 0),
                      ((in.buttons & 2) != 0) - ((in.buttons & 1) != 0)};
        point(&e);
    } else if (in.mode == MB_INPUT_POINTER && (in.buttons & MB_KEY_A)) {
        MB_Event e = {MB_ACTIVATE, in.x, in.y};
        point(&e);
    }
}
static void deactivate(void) {
    picture_fill_cancel(&paint_picture, &paint_fill);
    mb_canvas_close();
    mb_text_reset();
}
static void shutdown(void) {}
const MB_App megapaint_app = {MB_API_VERSION, "MegaPaint", init,       activate,
                              event,          tick,        deactivate, shutdown};
