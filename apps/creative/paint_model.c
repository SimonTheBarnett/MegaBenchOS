/**
 * @file apps/creative/paint_model.c
 * @brief Portable sparse 4bpp tiles and MBP2/MBP1 decoding; production code used by host tests.
 *
 * ROM-linked application code. Shared hardware and resource ownership remain in OS services.
 * MBP2 uses byte RLE over row-major packed pixels. Validate twice before replacement:
 * a first pass checks CRC/length/pool demand without altering the current picture.
 */
#include "paint_model.h"
static uint16_t word(const uint8_t *b) {
    return ((uint16_t)b[0] << 8) | b[1];
}
static void put(uint8_t *b, uint16_t v) {
    b[0] = v >> 8;
    b[1] = v;
}
static uint16_t crc_byte(uint16_t v, uint8_t c) {
    static const uint16_t table[16] = {0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50a5,
                                       0x60c6, 0x70e7, 0x8108, 0x9129, 0xa14a, 0xb16b,
                                       0xc18c, 0xd1ad, 0xe1ce, 0xf1ef};
    v ^= (uint16_t)c << 8;
    v = (v << 4) ^ table[v >> 12];
    v = (v << 4) ^ table[v >> 12];
    return v;
}
static uint16_t count(const Picture *p) {
    return (p->width / 8) * (p->height / 8);
}
static uint16_t tile_at(const Picture *p, uint16_t x, uint16_t y) {
    return (y / 8) * (p->width / 8) + x / 8;
}
static void release(Picture *p, uint16_t id) {
    if (!(id & PAINT_UNIFORM)) {
        p->allocated[id - 1] = 0;
        --p->used;
    }
}
static uint16_t allocate(Picture *p, uint16_t color) {
    uint16_t i, j;
    for (i = 0; i < PAINT_POOL; ++i)
        if (!p->allocated[i]) {
            p->allocated[i] = 1;
            ++p->used;
            for (j = 0; j < 32; ++j)
                p->tiles[i][j] = color * 17;
            return i + 1;
        }
    return 0;
}
static void collapse(Picture *p, uint16_t t) {
    uint16_t id = p->map[t], i;
    uint8_t c;
    if (id & PAINT_UNIFORM)
        return;
    c = p->tiles[id - 1][0];
    if ((c >> 4) != (c & 15))
        return;
    for (i = 1; i < 32; ++i)
        if (p->tiles[id - 1][i] != c)
            return;
    release(p, id);
    p->map[t] = PAINT_UNIFORM | (c & 15);
}
void picture_clear(Picture *p, uint16_t color) {
    uint16_t i;
    for (i = 0; i < PAINT_POOL; ++i)
        p->allocated[i] = 0;
    for (i = 0; i < PAINT_MAP; ++i)
        p->map[i] = PAINT_UNIFORM | (color & 15);
    p->used = 0;
}
MB_Result picture_new(Picture *p, uint16_t w, uint16_t h) {
    static const uint16_t colors[16] = {0xeee, 0x000, 0x222, 0x666, 0xaaa, 0x00e, 0x06e, 0x0ee,
                                        0x0e0, 0x0a0, 0xee0, 0xe00, 0xe0e, 0x80a, 0xe8e, 0x88e};
    uint16_t i;
    if (!p || !w || !h || w > PAINT_MAX_W || h > PAINT_MAX_H || (w & 7) || (h & 7))
        return MB_INVALID;
    p->width = w;
    p->height = h;
    for (i = 0; i < 16; ++i)
        p->palette[i] = colors[i];
    picture_clear(p, 0);
    return MB_OK;
}
uint8_t picture_pixel(const void *context, uint16_t x, uint16_t y) {
    const Picture *p = context;
    uint16_t id;
    uint8_t b;
    if (x >= p->width || y >= p->height)
        return 0;
    id = p->map[tile_at(p, x, y)] & ~PAINT_VISITED;
    if (id & PAINT_UNIFORM)
        return id & 15;
    b = p->tiles[id - 1][(y & 7) * 4 + (x & 7) / 2];
    return (x & 1) ? b & 15 : b >> 4;
}
void picture_tile(const void *context, uint16_t x, uint16_t y, uint32_t *rows) {
    const Picture *p = context;
    uint16_t id = p->map[y * (p->width >> 3) + x] & ~PAINT_VISITED, i;
    if (id & PAINT_UNIFORM) {
        uint32_t row = (uint32_t)(id & 15) * 0x11111111;
        for (i = 0; i < 8; ++i)
            rows[i] = row;
    } else {
        const uint8_t *b = p->tiles[id - 1];
        for (i = 0; i < 8; ++i, b += 4)
            rows[i] =
                ((uint32_t)b[0] << 24) | ((uint32_t)b[1] << 16) | ((uint16_t)b[2] << 8) | b[3];
    }
}
static MB_Result brush_rect(Picture *p, uint16_t x, uint16_t y, uint16_t w, uint16_t h,
                            uint16_t color) {
    uint16_t xx, yy, tx, ty, endx, endy, need = 0;
    if (!p || x >= p->width || y >= p->height || !w || w > 8 || !h || h > 8 || color > 15)
        return MB_INVALID;
    endx = x + w;
    if (endx > p->width)
        endx = p->width;
    endy = y + h;
    if (endy > p->height)
        endy = p->height;
    /* Reserve the entire stamp before changing any pixel. A failed brush is atomic. */
    for (ty = y / 8; ty <= (endy - 1) / 8; ++ty)
        for (tx = x / 8; tx <= (endx - 1) / 8; ++tx) {
            uint16_t id = p->map[ty * (p->width / 8) + tx];
            if ((id & PAINT_UNIFORM) && (id & 15) != color &&
                !(x <= tx * 8 && y <= ty * 8 && endx >= tx * 8 + 8 && endy >= ty * 8 + 8))
                ++need;
        }
    if (need > PAINT_POOL - p->used)
        return MB_NO_SPACE;
    for (ty = y / 8; ty <= (endy - 1) / 8; ++ty)
        for (tx = x / 8; tx <= (endx - 1) / 8; ++tx) {
            uint16_t t = ty * (p->width / 8) + tx, id = p->map[t];
            if (x <= tx * 8 && y <= ty * 8 && endx >= tx * 8 + 8 && endy >= ty * 8 + 8) {
                release(p, id);
                p->map[t] = PAINT_UNIFORM | color;
                continue;
            }
            if (id & PAINT_UNIFORM) {
                if ((id & 15) == color)
                    continue;
                id = p->map[t] = allocate(p, id & 15);
            }
            for (yy = y; yy < endy; ++yy)
                for (xx = x; xx < endx; ++xx)
                    if (xx / 8 == tx && yy / 8 == ty) {
                        uint16_t i = (yy & 7) * 4 + (xx & 7) / 2;
                        uint8_t b = p->tiles[id - 1][i];
                        p->tiles[id - 1][i] =
                            (xx & 1) ? (b & 0xf0) | color : (b & 15) | (color << 4);
                    }
            collapse(p, t);
        }
    return MB_OK;
}
MB_Result picture_brush(Picture *p, uint16_t x, uint16_t y, uint16_t size, uint16_t color) {
    return brush_rect(p, x, y, size, size, color);
}
MB_Result picture_stamp(Picture *p, uint16_t cx, uint16_t cy, uint16_t size, uint16_t color,
                        MB_Rect *damage) {
    int16_t x, y, endx, endy;
    MB_Result result;
    if (!p || !damage || cx >= p->width || cy >= p->height || !size || size > 8 || color > 15)
        return MB_INVALID;
    /* Even widths straddle the cursor pixel: 4px covers offsets -2..+1.
     * Clip the original footprint, never shift a stroke inward at an edge. */
    x = (int16_t)cx - size / 2;
    y = (int16_t)cy - size / 2;
    endx = x + size;
    endy = y + size;
    if (x < 0)
        x = 0;
    if (y < 0)
        y = 0;
    if (endx > p->width)
        endx = p->width;
    if (endy > p->height)
        endy = p->height;
    result = brush_rect(p, x, y, endx - x, endy - y, color);
    if (result == MB_OK)
        *damage = (MB_Rect){x, y, endx - x, endy - y};
    return result;
}
MB_Result picture_encode(const Picture *p, uint8_t *out, uint16_t cap, uint16_t *length) {
    uint16_t i, y, tx, j, cols, n = 48, run = 0, crc = 65535;
    uint8_t last = 0;
    if (!p || !out || !length || cap < 48)
        return MB_INVALID;
    cols = p->width >> 3;
    /* Walk tile rows, avoiding per-byte 32-bit quotient/remainder on the 68000. */
    for (y = 0; y < p->height; ++y)
        for (tx = 0; tx < cols; ++tx) {
            uint16_t id = p->map[(y >> 3) * cols + tx] & ~PAINT_VISITED;
            for (j = 0; j < 4; ++j) {
                uint8_t b =
                    (id & PAINT_UNIFORM) ? (id & 15) * 17 : p->tiles[id - 1][(y & 7) * 4 + j];
                crc = crc_byte(crc, b);
                if (run && (b != last || run == 255)) {
                    if (n + 2 > cap)
                        return MB_NO_SPACE;
                    out[n++] = run;
                    out[n++] = last;
                    run = 0;
                }
                last = b;
                ++run;
            }
        }
    if (n + 2 > cap)
        return MB_NO_SPACE;
    out[n++] = run;
    out[n++] = last;
    out[0] = 'M';
    out[1] = 'B';
    out[2] = 'P';
    out[3] = '2';
    put(out + 4, p->width);
    put(out + 6, p->height);
    out[8] = 4;
    out[9] = 1;
    put(out + 10, n - 48);
    out[14] = out[15] = 0;
    for (i = 0; i < 16; ++i)
        put(out + 16 + i * 2, p->palette[i]);
    /* Also protect dimensions, codec, lengths and palette. CRC fields themselves
     * are excluded; metadata follows the expanded pixel stream in CRC order. */
    for (i = 4; i < 12; ++i)
        crc = crc_byte(crc, out[i]);
    for (i = 14; i < 48; ++i)
        crc = crc_byte(crc, out[i]);
    put(out + 12, crc);
    *length = n;
    return MB_OK;
}
typedef struct {
    const uint8_t *b;
    uint16_t n, at, run;
    uint8_t value;
} Reader;
static uint8_t next(Reader *r) {
    if (!r->run) {
        r->run = r->b[r->at++];
        r->value = r->b[r->at++];
    }
    --r->run;
    return r->value;
}
static MB_Result inspect(const uint8_t *b, uint16_t n, uint16_t *w, uint16_t *h) {
    uint32_t expanded = 0;
    uint16_t i, crc = 65535;
    if (!b || n < 16 || b[0] != 'M' || b[1] != 'B' || b[2] != 'P')
        return MB_CORRUPT;
    if (b[3] == '1') {
        if (n != 1040 || word(b + 4) != 64 || word(b + 6) != 64 || b[8] != 2 || b[9] ||
            word(b + 10) != 1024 || b[14] || b[15])
            return MB_CORRUPT;
        for (i = 16; i < n; ++i)
            crc = crc_byte(crc, b[i]);
        *w = *h = 64;
        return crc == word(b + 12) ? MB_OK : MB_CORRUPT;
    }
    if (b[3] != '2' || b[8] != 4 || b[9] != 1)
        return MB_VERSION;
    *w = word(b + 4);
    *h = word(b + 6);
    if (!*w || !*h || *w > PAINT_MAX_W || *h > PAINT_MAX_H || (*w & 7) || (*h & 7))
        return MB_VERSION;
    if (n < 50 || n > PAINT_FILE_LIMIT || word(b + 10) != n - 48 || ((n - 48) & 1) || b[14] ||
        b[15])
        return MB_CORRUPT;
    for (i = 0; i < 16; ++i)
        if (word(b + 16 + i * 2) & ~0xeee)
            return MB_CORRUPT;
    for (i = 48; i < n; i += 2) {
        uint16_t j;
        if (!b[i])
            return MB_CORRUPT;
        expanded += b[i];
        if (expanded > (uint32_t)*w * *h / 2)
            return MB_CORRUPT;
        for (j = 0; j < b[i]; ++j)
            crc = crc_byte(crc, b[i + 1]);
    }
    for (i = 4; i < 12; ++i)
        crc = crc_byte(crc, b[i]);
    for (i = 14; i < 48; ++i)
        crc = crc_byte(crc, b[i]);
    return expanded == (uint32_t)*w * *h / 2 && crc == word(b + 12) ? MB_OK : MB_CORRUPT;
}
/* Borrow a 2048-byte band from the inactive fill workspace, preserving the 2560-byte
 * OS stack. Identify uniform tiles before committing any replacement document. */
MB_Result picture_decode(Picture *p, const uint8_t *b, uint16_t n, uint8_t *band) {
    uint16_t w, h, pass, pools = 0, by, tx, y, j;
    if (!p || !band)
        return MB_INVALID;
    MB_Result r = inspect(b, n, &w, &h);
    if (r != MB_OK)
        return r;
    for (pass = 0; pass < 2; ++pass) {
        Reader reader = {b, n, 48, 0, 0};
        pools = 0;
        if (pass) {
            picture_new(p, w, h);
            if (b[3] == '2')
                for (j = 0; j < 16; ++j)
                    p->palette[j] = word(b + 16 + j * 2);
            /* MBP1 stored logical theme roles rather than RGB values. Import the
             * original default Blue theme's face/ink/accent/light colours. */
            else {
                p->palette[0] = 0xccc;
                p->palette[1] = 0x222;
                p->palette[2] = 0xaa6;
                p->palette[3] = 0xeee;
            }
        }
        for (by = 0; by < h; by += 8) {
            for (y = 0; y < 8; ++y)
                for (j = 0; j < w / 2; ++j) {
                    if (b[3] == '2')
                        band[y * (w / 2) + j] = next(&reader);
                    else {
                        uint16_t x = j * 2, shift = 6 - (x & 3) * 2;
                        uint8_t old = b[16 + (by + y) * 16 + x / 4];
                        band[y * (w / 2) + j] =
                            (((old >> shift) & 3) << 4) | ((old >> (shift - 2)) & 3);
                    }
                }
            for (tx = 0; tx < w / 8; ++tx) {
                uint8_t first = band[tx * 4], uniform = (first >> 4) == (first & 15);
                for (y = 0; y < 8; ++y)
                    for (j = 0; j < 4; ++j)
                        if (band[y * (w / 2) + tx * 4 + j] != first)
                            uniform = 0;
                if (!uniform && ++pools > PAINT_POOL)
                    return MB_NO_SPACE;
                if (pass) {
                    uint16_t t = (by / 8) * (w / 8) + tx;
                    if (uniform)
                        p->map[t] = PAINT_UNIFORM | (first & 15);
                    else {
                        uint16_t id = allocate(p, 0);
                        p->map[t] = id;
                        for (y = 0; y < 8; ++y)
                            for (j = 0; j < 4; ++j)
                                p->tiles[id - 1][y * 4 + j] = band[y * (w / 2) + tx * 4 + j];
                    }
                }
            }
        }
    }
    return MB_OK;
}
static void pending(PaintFill *f, uint16_t t) {
    uint8_t bit = 1 << (t & 7);
    if (!(f->pending[t / 8] & bit)) {
        f->pending[t / 8] |= bit;
        ++f->remaining;
    }
}
static uint16_t visited(const Picture *p, const PaintFill *f, int16_t x, int16_t y) {
    uint16_t id;
    if (x < 0 || y < 0 || x >= p->width || y >= p->height)
        return 0;
    id = p->map[tile_at(p, x, y)];
    return (id & PAINT_UNIFORM) ? (id & PAINT_VISITED) != 0
                                : (f->masks[id - 1][y & 7] & (1 << (x & 7))) != 0;
}
static void visit(Picture *p, PaintFill *f, uint16_t x, uint16_t y) {
    uint16_t t = tile_at(p, x, y), id = p->map[t];
    if (id & PAINT_UNIFORM)
        p->map[t] |= PAINT_VISITED;
    else
        f->masks[id - 1][y & 7] |= 1 << (x & 7);
}
MB_Result picture_fill_begin(Picture *p, PaintFill *f, uint16_t x, uint16_t y, uint16_t color) {
    uint16_t i, j;
    if (x >= p->width || y >= p->height || color > 15)
        return MB_INVALID;
    f->old_color = picture_pixel(p, x, y);
    f->new_color = color;
    f->scan = f->remaining = f->commit = 0;
    f->phase = 0;
    if (f->old_color == color)
        return MB_OK;
    for (i = 0; i < PAINT_POOL; ++i)
        for (j = 0; j < 8; ++j)
            f->masks[i][j] = 0;
    for (i = 0; i < sizeof f->pending; ++i)
        f->pending[i] = 0;
    visit(p, f, x, y);
    f->seed = tile_at(p, x, y);
    pending(f, f->seed);
    f->phase = 1;
    return MB_OK;
}
void picture_fill_cancel(Picture *p, PaintFill *f) {
    uint16_t i;
    for (i = 0; i < count(p); ++i)
        p->map[i] &= ~PAINT_VISITED;
    f->phase = 0;
}
void picture_fill_step(Picture *p, PaintFill *f, uint16_t budget) {
    uint16_t total = count(p), cols = p->width / 8;
    while (budget && f->phase == 1) {
        uint16_t scanned = 0, t, ox, oy, x, y, change = 0;
        if (!f->remaining) {
            f->phase = 2;
            f->commit = 0;
            break;
        }
        while (scanned++ < total && !(f->pending[f->scan / 8] & (1 << (f->scan & 7))))
            f->scan = (f->scan + 1) % total;
        t = f->scan;
        f->scan = (f->scan + 1) % total;
        f->pending[t / 8] &= ~(1 << (t & 7));
        --f->remaining;
        --budget;
        ox = (t % cols) * 8;
        oy = (t / cols) * 8;
        if (t == f->seed) {
            change = 1;
            f->seed = 65535;
        }
        /* Local closure is at most 64 pixels; uniform tiles become one visited flag. */
        do {
            uint16_t again = 0;
            for (y = oy; y < oy + 8; ++y)
                for (x = ox; x < ox + 8; ++x)
                    if (!visited(p, f, x, y) && picture_pixel(p, x, y) == f->old_color &&
                        (visited(p, f, x - 1, y) || visited(p, f, x + 1, y) ||
                         visited(p, f, x, y - 1) || visited(p, f, x, y + 1))) {
                        visit(p, f, x, y);
                        again = change = 1;
                    }
            if (!again)
                break;
        } while (1);
        /* Only new reachability propagates, so visited tiles cannot requeue forever. */
        if (change) {
            if (t % cols)
                pending(f, t - 1);
            if (t % cols + 1 < cols)
                pending(f, t + 1);
            if (t >= cols)
                pending(f, t - cols);
            if (t + cols < total)
                pending(f, t + cols);
        }
    }
    while (budget && f->phase == 2) {
        uint16_t t = f->commit++, id = p->map[t], i;
        --budget;
        if (id & PAINT_UNIFORM) {
            if (id & PAINT_VISITED)
                p->map[t] = PAINT_UNIFORM | f->new_color;
        } else {
            for (i = 0; i < 64; ++i)
                if (f->masks[id - 1][i / 8] & (1 << (i & 7))) {
                    uint8_t b = p->tiles[id - 1][i / 2];
                    p->tiles[id - 1][i / 2] =
                        (i & 1) ? (b & 0xf0) | f->new_color : (b & 15) | (f->new_color << 4);
                }
            collapse(p, t);
        }
        if (f->commit == total)
            f->phase = 0;
    }
}
