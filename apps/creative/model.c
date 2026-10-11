/**
 * @file apps/creative/model.c
 * @brief Pure text-edit and portable MBP1 packing/CRC logic; no UI, heap or hardware calls.
 *
 * ROM-linked application code. Shared hardware and resource ownership remain in OS services.
 */
#include "model.h"
void pad_reset(Pad *p) {
    p->text[0] = 0;
    p->length = p->cursor = p->dirty = p->scroll = 0;
}
MB_Result pad_edit(Pad *p, const MB_TextInput *in) {
    uint16_t i;
    uint8_t c = in->character;
    if (in->action == MB_TEXT_LEFT) {
        if (p->cursor)
            --p->cursor;
        return MB_OK;
    }
    if (in->action == MB_TEXT_RIGHT) {
        if (p->cursor < p->length)
            ++p->cursor;
        return MB_OK;
    }
    if (in->action == MB_TEXT_BACKSPACE) {
        if (!p->cursor)
            return MB_OK;
        --p->cursor;
    } else if (in->action == MB_TEXT_DELETE) {
        if (p->cursor == p->length)
            return MB_OK;
    } else if (in->action == MB_TEXT_CHAR || in->action == MB_TEXT_ENTER) {
        if (in->action == MB_TEXT_ENTER)
            c = '\n';
        if (c != '\n' && (c < 32 || c > 126))
            return MB_INVALID;
        if (p->length == 1024)
            return MB_OVERFLOW;
        for (i = p->length + 1; i > p->cursor; i--)
            p->text[i] = p->text[i - 1];
        p->text[p->cursor++] = c;
        ++p->length;
        p->dirty = 1;
        return MB_OK;
    } else
        return MB_OK;
    for (i = p->cursor; i < p->length; i++)
        p->text[i] = p->text[i + 1];
    --p->length;
    p->dirty = 1;
    return MB_OK;
}
uint16_t pad_line(const Pad *p, uint16_t start, uint16_t columns) {
    uint16_t n = 0;
    while (start < p->length && n < columns) {
        ++n;
        if (p->text[start++] == '\n')
            break;
    }
    return start;
}
void pad_move(Pad *p, int16_t x, int16_t y, uint16_t columns) {
    uint16_t start = 0, prev = 0, col, end, next;
    if (x < 0 && p->cursor)
        --p->cursor;
    else if (x > 0 && p->cursor < p->length)
        ++p->cursor;
    if (!y)
        return;
    while (start < p->cursor) {
        next = pad_line(p, start, columns);
        if (next > p->cursor || next == start)
            break;
        prev = start;
        start = next;
    }
    col = p->cursor - start;
    if (y < 0) {
        if (!start) {
            p->cursor = 0;
            return;
        }
        end = start;
        start = prev;
    } else {
        start = pad_line(p, start, columns);
        end = pad_line(p, start, columns);
    }
    p->cursor = start + col;
    if (p->cursor > end)
        p->cursor = end;
    if (p->cursor > p->length)
        p->cursor = p->length;
}
static uint16_t crc(const uint8_t *p) {
    uint16_t v = 65535, i, b;
    for (i = 0; i < 1024; i++) {
        v ^= (uint16_t)p[i] << 8;
        for (b = 0; b < 8; b++)
            v = (v & 0x8000) ? (v << 1) ^ 0x1021 : v << 1;
    }
    return v;
}
void paint_encode(uint8_t *p) {
    uint16_t c;
    p[0] = 'M';
    p[1] = 'B';
    p[2] = 'P';
    p[3] = '1';
    p[4] = p[6] = 0;
    p[5] = p[7] = 64;
    p[8] = 2;
    p[9] = 0;
    p[10] = 4;
    p[11] = 0;
    c = crc(p + 16);
    p[12] = c >> 8;
    p[13] = c;
    p[14] = p[15] = 0;
}
void paint_new(uint8_t *p) {
    uint16_t i;
    for (i = 0; i < 1040; i++)
        p[i] = 0;
    paint_encode(p);
}
uint8_t paint_get(const uint8_t *p, uint16_t x, uint16_t y) {
    if (x >= 64 || y >= 64)
        return 0;
    return (p[16 + y * 16 + x / 4] >> (6 - (x % 4) * 2)) & 3;
}
void paint_set(uint8_t *p, uint16_t x, uint16_t y, uint16_t c) {
    uint16_t i, s;
    if (x >= 64 || y >= 64 || c > 3)
        return;
    i = 16 + y * 16 + x / 4;
    s = 6 - (x % 4) * 2;
    p[i] = (p[i] & ~(3 << s)) | (c << s);
}
MB_Result paint_validate(const uint8_t *p, uint16_t n) {
    if (!p || n != 1040)
        return MB_CORRUPT;
    if (p[0] != 'M' || p[1] != 'B' || p[2] != 'P')
        return MB_CORRUPT;
    if (p[3] != '1' || p[4] || p[5] != 64 || p[6] || p[7] != 64 || p[8] != 2 || p[9])
        return MB_VERSION;
    if (p[10] != 4 || p[11] || p[14] || p[15] || crc(p + 16) != ((uint16_t)p[12] << 8 | p[13]))
        return MB_CORRUPT;
    return MB_OK;
}
