/**
 * @file kernel/services/text.c
 * @brief Shared bounded ASCII keyboard; renders through the existing client/theme API.
 *
 * OS service implementation behind the public SDK; apps use megabenchos.h.
 * No editor storage or device pointers: semantic edit commands are copied to callers.
 */
#include "megabenchos.h"
static uint16_t key, shifted, visible, origin_x, origin_y;
static const char chars[] = "abcdefghijklmnopqrstuvwxyz0123456789.,?!-_:/+*=() ";
static const char *const commands[] = {"BS",   "Del",  "Ent", "<", ">",
                                       "Case", "Done", "Esc", "'", ";"};
static void draw_key(uint16_t i) {
    char b[2] = {0, 0};
    const char *label;
    if (i < 50) {
        b[0] = chars[i];
        if (shifted && b[0] >= 'a' && b[0] <= 'z')
            b[0] -= 32;
        label = i == 49 ? "Spc" : b;
    } else
        label = commands[i - 50];
    mb_draw_rect(origin_x + (i % 10) * 4, origin_y + i / 10, 4, 1,
                 i == key ? MB_STYLE_ACCENT : MB_STYLE_SURFACE);
    mb_draw_text(origin_x + (i % 10) * 4, origin_y + i / 10, label);
}
void mb_text_reset(void) {
    key = shifted = visible = 0;
}
MB_Result mb_text_keyboard(uint16_t x, uint16_t y) {
    MB_Rect r;
    uint16_t i;
    if (mb_window_get_client_rect(&r) != MB_OK)
        return MB_UNAVAILABLE;
    if (x > r.width || y > r.height || 40 > r.width - x || 6 > r.height - y)
        return MB_INVALID;
    origin_x = x;
    origin_y = y;
    visible = 1;
    for (i = 0; i < 60; i++)
        draw_key(i);
    return MB_OK;
}
MB_Result mb_text_event(const MB_Event *e, MB_TextInput *out) {
    MB_Input in;
    MB_Rect r;
    int16_t x, y;
    uint16_t old = key;
    if (!e || !out)
        return MB_INVALID;
    out->action = MB_TEXT_NONE;
    out->character = 0;
    if (!visible)
        return MB_UNAVAILABLE;
    mb_input_state(&in);
    if (e->action == MB_NAVIGATE && in.mode != MB_INPUT_POINTER) {
        key = (key + 60 + e->x + e->y * 10) % 60;
        draw_key(old);
        draw_key(key);
        return MB_OK;
    }
    if (e->action != MB_ACTIVATE)
        return MB_OK;
    if (in.mode == MB_INPUT_POINTER) {
        mb_window_get_client_rect(&r);
        x = e->x / 8 - r.x - origin_x;
        y = e->y / 8 - r.y - origin_y;
        if (e->x < 0 || e->y < 0 || x < 0 || x >= 40 || y < 0 || y >= 6)
            return MB_OK;
        key = y * 10 + x / 4;
    }
    if (key < 50) {
        out->action = MB_TEXT_CHAR;
        out->character = chars[key];
        if (shifted && out->character >= 'a' && out->character <= 'z')
            out->character -= 32;
    } else if (key == 55) {
        shifted = !shifted;
        mb_text_keyboard(origin_x, origin_y);
    } else if (key >= 58) {
        out->action = MB_TEXT_CHAR;
        out->character = key == 58 ? '\'' : ';';
    } else {
        static const uint16_t action[] = {MB_TEXT_BACKSPACE, MB_TEXT_DELETE, MB_TEXT_ENTER,
                                          MB_TEXT_LEFT,      MB_TEXT_RIGHT,  0,
                                          MB_TEXT_DONE,      MB_TEXT_CANCEL};
        out->action = action[key - 50];
    }
    draw_key(old);
    draw_key(key);
    return MB_OK;
}
