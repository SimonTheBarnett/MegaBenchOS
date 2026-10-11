/**
 * @file apps/creative/editors.c
 * @brief MegaPad uses shared SDK file dialogs and the OS ASCII keyboard.
 *
 * ROM-linked application code. Shared hardware and resource ownership remain in OS services.
 * Workspaces are bounded/static; all files and graphics flow through public services.
 */
#include "model.h"
enum { VIEW_DOCUMENT, VIEW_NAME, VIEW_OPEN, VIEW_CONFIRM };
enum { DO_NEW = 1, DO_OPEN, DO_SAVE, DO_SAVE_AS, DO_CLOSE };
static Pad pad;
static uint8_t scratch[1024];
static uint16_t view, operation, tools, page, name_length;

static char path[44], name[16], message[39];
static void draw(void);
static const char *prefix(void) {
    return "RAM:/Documents/";
}
static void make_path(char *out, const char *basename, uint16_t extension) {
    const char *a = prefix();
    uint16_t i = 0, j = 0;
    while (a[j])
        out[i++] = a[j++];
    j = 0;
    while (basename[j] && i < 39)
        out[i++] = basename[j++];
    if (extension) {
        a = ".txt";
        j = 0;
        while (a[j])
            out[i++] = a[j++];
    }
    out[i] = 0;
}
static uint16_t changed(void) {
    return pad.dirty;
}
static void guard(void) {
    mb_app_close_guard(changed() || view != VIEW_DOCUMENT);
}
static void status(MB_Result r) {
    const char *s = r == MB_NO_SPACE   ? "RAM Disk full; edits kept."
                    : r == MB_OVERFLOW ? "Document limit reached."
                    : r == MB_EXISTS   ? "Name already exists."
                    : r == MB_VERSION  ? "Unsupported image format."
                    : r == MB_CORRUPT  ? "Invalid document; edits kept."
                                       : "File operation failed.";
    uint16_t i = 0;
    while (s[i] && i < 38) {
        message[i] = s[i];
        ++i;
    }
    message[i] = 0;
}
static MB_Result load(const char *p) {
    MB_FileInfo f;
    MB_FileHandle h;
    uint16_t n, i;
    MB_Result r;
    r = mb_file_stat(p, &f);
    if (r != MB_OK)
        return r;
    if (f.directory != MB_DIR_DOCUMENTS)
        return MB_INVALID;
    if (f.size > 1024)
        return MB_OVERFLOW;
    if ((r = mb_file_open(p, MB_FILE_READ, &h)) != MB_OK)
        return r;
    r = mb_file_read(h, 0, scratch, sizeof scratch, &n);
    {
        MB_Result close = mb_file_close(h);
        if (r == MB_OK)
            r = close;
    }
    if (r != MB_OK)
        return r;
    for (i = 0; i < n; i++)
        if (scratch[i] != '\n' && (scratch[i] < 32 || scratch[i] > 126))
            return MB_CORRUPT;
    pad_reset(&pad);
    for (i = 0; i < n; i++)
        pad.text[i] = scratch[i];
    pad.text[n] = 0;
    pad.length = n;
    for (i = 0; p[i] && i < 43; i++)
        path[i] = p[i];
    path[i] = 0;
    message[0] = 0;
    return MB_OK;
}
static MB_Result save(const char *p, uint16_t fresh) {
    MB_DriveInfo drive;
    MB_FileInfo f;
    MB_FileHandle h;
    MB_Result r;
    uint16_t existed, i;
    r = mb_fs_capacity("RAM:", 1, &drive);
    if (r != MB_OK)
        return r;
    if (!(drive.capabilities & MB_FS_WRITABLE))
        return MB_READ_ONLY;
    if (!(drive.capabilities & MB_FS_ATOMIC_REPLACE))
        return MB_NOT_SUPPORTED;
    r = mb_file_stat(p, &f);
    existed = r == MB_OK;
    if (r != MB_OK && r != MB_NOT_FOUND)
        return r;
    if (fresh && existed)
        return MB_EXISTS;
    if ((r = mb_file_open(p, MB_FILE_WRITE | MB_FILE_CREATE, &h)) != MB_OK)
        return r;
    r = mb_file_write(h, (const uint8_t *)pad.text, pad.length);
    {
        MB_Result close = mb_file_close(h);
        if (r == MB_OK)
            r = close;
    }
    if (r != MB_OK) {
        if (!existed)
            mb_file_delete(p);
        return r;
    }
    for (i = 0; p[i] && i < 43; i++)
        path[i] = p[i];
    path[i] = 0;
    pad.dirty = 0;
    message[0] = 0;
    return MB_OK;
}
static void perform(uint16_t action) {
    operation = action;
    message[0] = 0;
    if (action == DO_NEW) {
        path[0] = 0;

        pad_reset(&pad);

        view = VIEW_DOCUMENT;
    } else if (action == DO_OPEN) {
        view = VIEW_OPEN;
        page = 0;
        tools = 1;
    } else if (action == DO_SAVE && path[0]) {
        MB_Result r = save(path, 0);
        if (r != MB_OK)
            status(r);
        view = VIEW_DOCUMENT;
    } else if (action == DO_SAVE || action == DO_SAVE_AS) {
        view = VIEW_NAME;
        name[0] = 0;
        name_length = 0;
        mb_text_reset();
    } else if (action == DO_CLOSE) {
        mb_app_close_guard(0);
        mb_app_close();
        return;
    }
    guard();
    draw();
}
static void request(uint16_t action) {
    if (changed() && (action == DO_NEW || action == DO_OPEN || action == DO_CLOSE)) {
        operation = action;
        view = VIEW_CONFIRM;
        guard();
        draw();
    } else
        perform(action);
}
static void draw_pad(void) {
    uint16_t at = 0, line = 0, cursor_line = 0, cursor_col = 0, i, row;
    char b[39];
    /* Locate cursor in the same wrapped line model used for vertical movement. */
    while (at < pad.cursor) {
        uint16_t next = pad_line(&pad, at, 38);
        if (next > pad.cursor || next == at)
            break;
        at = next;
        ++line;
    }
    cursor_line = line;
    cursor_col = pad.cursor - at;
    if (cursor_line < pad.scroll)
        pad.scroll = cursor_line;
    if (cursor_line >= pad.scroll + 14)
        pad.scroll = cursor_line - 13;
    at = 0;
    for (i = 0; i < pad.scroll && at < pad.length; i++)
        at = pad_line(&pad, at, 38);
    for (row = 0; row < 14; row++) {
        uint16_t start = at, n = 0, next = pad_line(&pad, at, 38);
        while (at < next && pad.text[at] != '\n' && n < 38)
            b[n++] = pad.text[at++];
        b[n] = 0;
        at = next;
        mb_draw_field(1, 2 + row, 38, b);
        if (cursor_line == pad.scroll + row && cursor_col < 38) {
            char c[2] = {pad.cursor < pad.length && pad.text[pad.cursor] != '\n'
                             ? pad.text[pad.cursor]
                             : '_',
                         0};
            mb_draw_rect(1 + cursor_col, 2 + row, 1, 1, MB_STYLE_ACCENT);
            mb_draw_text(1 + cursor_col, 2 + row, c);
        }
        (void)start;
    }
    mb_draw_field(0, 16, 40,
                  tools == 2 ? "Cursor mode; Start changes mode"
                  : tools    ? "Toolbar; Start returns to keys"
                             : "Keys; Start for toolbar/cursor");
}
static void draw(void) {
    uint16_t i;
    char title[26], b[12];

    mb_canvas_close();
    mb_window_clear();
    mb_text_reset();
    {
        const char *base = path;
        uint16_t n = 0, j = 0;
        while (path[j]) {
            if (path[j] == '/')
                base = path + j + 1;
            ++j;
        }
        if (!*base)
            base = "Untitled";
        while (base[n] && n < 22) {
            title[n] = base[n];
            ++n;
        }
        if (changed())
            title[n++] = '*';
        title[n] = 0;
    }
    mb_draw_field(0, 1, 40, title);
    mb_ui_menu_button(1, 0, 7, "New", MB_UI_ENABLED);
    mb_ui_menu_button(2, 8, 7, "Open", MB_UI_ENABLED);
    mb_ui_menu_button(3, 16, 7, "Save", MB_UI_ENABLED);
    mb_ui_menu_button(4, 23, 9, "Save As", MB_UI_ENABLED);
    mb_ui_menu_button(5, 32, 8, "Mode", MB_UI_ENABLED);
    if (view == VIEW_CONFIRM) {
        mb_draw_text(1, 5, "Discard unsaved changes?");
        mb_ui_reset();
        mb_ui_button(100, 3, 8, 12, "Yes", MB_UI_ENABLED);
        mb_ui_button(101, 20, 8, 12, "Cancel", MB_UI_ENABLED);
        mb_ui_focus(101);
        mb_input_mode(MB_INPUT_FOCUS);
    } else if (view == VIEW_NAME) {
        mb_draw_text(1, 5, "New filename (letters/digits/_/-)");
        mb_draw_field(1, 7, 20, name);
        mb_text_keyboard(0, 16);
        mb_input_mode(MB_INPUT_FOCUS);
    } else if (view == VIEW_OPEN) {
        MB_FileInfo f;
        mb_ui_reset();
        mb_draw_text(1, 4, "Documents (*.txt)");
        for (i = 0; i < 6; i++)
            if (mb_dir_list_path("RAM:/Documents/", page * 6 + i, &f) == MB_OK) {
                mb_ui_button(20 + i, 1, 6 + i * 2, 22, f.name, MB_UI_ENABLED);
                mb_format_u32(f.size, b, sizeof b);
                mb_draw_text(25, 6 + i * 2, b);
            } else
                break;
        if (!i)
            mb_draw_text(1, 6, "No files here yet.");
        mb_ui_button(26, 1, 20, 10, "Prev", MB_UI_ENABLED);
        mb_ui_button(27, 13, 20, 10, "Next", MB_UI_ENABLED);
        mb_ui_button(28, 25, 20, 12, "Cancel", MB_UI_ENABLED);
        mb_input_mode(MB_INPUT_FOCUS);
    } else {
        draw_pad();
        mb_text_keyboard(0, 18);
        mb_input_mode(MB_INPUT_FOCUS);
    }
    if (message[0])
        mb_draw_field(0, view == VIEW_DOCUMENT ? 17 : 11, 40, message);
    guard();
}
static void name_event(const MB_Event *e) {
    MB_TextInput t;
    MB_Result r;
    mb_text_event(e, &t);
    if (t.action == MB_TEXT_CANCEL) {
        view = VIEW_DOCUMENT;
        draw();
        return;
    }
    if (t.action == MB_TEXT_DONE) {
        char target[44];
        if (!name_length) {
            status(MB_INVALID);
            mb_draw_field(0, 11, 40, message);
            return;
        }
        make_path(target, name, 1);
        r = save(target, 1);
        if (r != MB_OK) {
            status(r);
            mb_draw_field(0, 11, 40, message);
            return;
        }
        view = VIEW_DOCUMENT;
        draw();
        return;
    }
    if (t.action == MB_TEXT_BACKSPACE && name_length)
        name[--name_length] = 0;
    if (t.action == MB_TEXT_CHAR) {
        uint8_t c = t.character;
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') ||
            c == '_' || c == '-') {
            if (name_length < 15) {
                name[name_length++] = c;
                name[name_length] = 0;
            } else
                status(MB_OVERFLOW);
        }
    }
    mb_draw_field(1, 7, 20, name);
}

static void event(const MB_Event *e) {
    uint16_t id;
    MB_Input in;
    if (e->action == MB_RESIZE || e->action == MB_THEME_CHANGED) {
        draw();
        return;
    }
    if (e->action == MB_CLOSE_REQUEST) {
        if (view != VIEW_DOCUMENT) {
            view = VIEW_DOCUMENT;
            draw();
        } else
            request(DO_CLOSE);
        return;
    }
    if (e->action == MB_CLOSE)
        return;
    if (view == VIEW_NAME) {
        name_event(e);
        return;
    }
    if (e->action == MB_MENU && view == VIEW_DOCUMENT) {
        tools = (tools + 1) % 3;
        draw();
        return;
    }
    mb_input_state(&in);
    if ((view != VIEW_DOCUMENT || tools == 1 || in.mode == MB_INPUT_POINTER) &&
        mb_ui_event(e, &id) == MB_OK) {
        if (view == VIEW_CONFIRM) {
            if (id == 100)
                perform(operation);
            else if (id == 101) {
                view = VIEW_DOCUMENT;
                draw();
            }
            return;
        }
        if (view == VIEW_OPEN) {
            if (id == 26) {
                if (page)
                    --page;
                draw();
            } else if (id == 27) {
                MB_FileInfo f;
                if (mb_dir_list_path("RAM:/Documents/", (page + 1) * 6, &f) == MB_OK)
                    ++page;
                draw();
            } else if (id == 28) {
                view = VIEW_DOCUMENT;
                draw();
            } else if (id >= 20 && id < 26) {
                MB_FileInfo f;
                char target[44];
                MB_Result r;
                if (mb_dir_list_path("RAM:/Documents/", page * 6 + id - 20, &f) == MB_OK) {
                    make_path(target, f.name, 0);
                    r = load(target);
                    if (r != MB_OK)
                        status(r);
                    view = VIEW_DOCUMENT;
                    draw();
                }
            }
            return;
        }
        if (id >= 1 && id <= 4) {
            request(id);
            return;
        }
        if (id == 5) {
            tools = 0;
            draw();
            return;
        }
    }
    if (view != VIEW_DOCUMENT)
        return;
    if (tools == 2 && e->action == MB_NAVIGATE) {
        pad_move(&pad, e->x, e->y, 38);
        draw_pad();
    } else if (!tools || in.mode == MB_INPUT_POINTER) {
        MB_TextInput t;
        MB_Result r;
        mb_text_event(e, &t);
        if (t.action == MB_TEXT_NONE)
            return;
        if (t.action == MB_TEXT_DONE) {
            tools = 1;
            draw();
        } else if (t.action == MB_TEXT_CANCEL)
            request(DO_CLOSE);
        else {
            r = pad_edit(&pad, &t);
            if (r != MB_OK)
                status(r);
            guard();
            draw_pad();
            if (message[0])
                mb_draw_field(0, 17, 40, message);
            if (pad.dirty)
                mb_draw_text(23, 2, "*");
        }
    }
}
static MB_Result init_pad(void) {

    pad_reset(&pad);
    return MB_OK;
}

static void activate(void) {
    char incoming[44];
    MB_Result r;
    view = operation = tools = page = name_length = 0;

    path[0] = message[0] = 0;

    mb_window_open("MegaPad");
    mb_window_set_mode(MB_WINDOW_MAXIMIZED);
    r = mb_app_document(incoming, sizeof incoming);
    if (r == MB_OK && (r = load(incoming)) != MB_OK)
        status(r);
    draw();
}
static void tick(void) {}
static void deactivate(void) {
    mb_canvas_close();
    mb_text_reset();
}
static void shutdown(void) {}
const MB_App megapad_app = {MB_API_VERSION, "MegaPad", init_pad,   activate,
                            event,          tick,      deactivate, shutdown};
