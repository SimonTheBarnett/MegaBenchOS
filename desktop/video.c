/**
 * @file desktop/video.c
 * @brief Shared Slate tile renderer, desktop hit testing and bounded SDK widgets.
 *
 * Shared desktop/window implementation; apps reach it through the public SDK.
 * Owns SGDK VDP/font/palette resources and one pointer sprite. Retained A/B tile maps
 * are main-RAM metadata, not pixel framebuffers; dirty spans flush only after VBlank.
 */
#include <genesis.h>
#include "internal.h"
#include "ui_assets.h"
#define TILE_BASE TILE_USER_INDEX
#define CELLS (40 * 28)
/* Four reserved patterns below the canvas/lease range; never overwrite UI art. */
#define CURSOR_TILE (TILE_BASE + 60)
#if UI_TILE_COUNT > 60
#error Custom cursor reservation overlaps UI artwork
#endif
/* Bound transfers per plane; transitions settle across refreshes instead of a huge redraw. */
#define FLUSH_BUDGET 192
static u16 plane_a[CELLS], plane_b[CELLS], dirty_count, flush_cursor, window_open;
static u8 dirty[(CELLS + 7) / 8];
static u16 heap_label = 0xffff;
static void palette_apply(void);
static u16 lease_token, lease_serial;
static u16 leased_sprites, sprites_pending;
static u16 palette_pending, palette_words[4][16];
/* ROM uploads are borrowed and canceled before an owner can disappear. */
static const u32 *upload_rows;
static u16 upload_tile, upload_left;
static const uint8_t *canvas_pixels;
static u8 canvas_dirty[140];
static u16 canvas_opened, canvas_x, canvas_y, canvas_cx = 65535, canvas_cy = 65535, canvas_scan;
static MB_CanvasSource canvas_source;
static u16 canvas_width, canvas_height, canvas_ox, canvas_oy, canvas_scale;
static u16 canvas_swatches_pending;
static u16 canvas_brush_size = 1, canvas_cursor_extent, canvas_cursor_pending, canvas_cursor_ready;
static u16 canvas_cursor_dot;
/* Copied 2bpp rows: no borrowed app bitmap survives a callback. */
MB_Cursor mb_cursor_observation __attribute__((externally_visible));
static u16 cursor_scope, cursor_pending, cursor_ready;
volatile uint16_t mb_brush_cursor_observation[4] __attribute__((externally_visible));
volatile uint16_t mb_canvas_observation[8] __attribute__((externally_visible));
static u16 canvas_flush(void);
MB_Result mb_canvas_ready(void) {
    u16 i, total = canvas_width * canvas_height;
    if (canvas_opened != 2)
        return MB_UNAVAILABLE;
    if (dirty_count || palette_pending || canvas_swatches_pending || canvas_cursor_pending ||
        cursor_pending)
        return MB_BUSY;
    for (i = 0; i < total; i += 8) {
        u16 remaining = total - i;
        u16 mask = remaining >= 8 ? 255 : (1U << remaining) - 1;
        if (canvas_dirty[i / 8] & mask)
            return MB_BUSY;
    }
    return MB_OK;
}
void mb_video_cancel_upload(void) {
    upload_left = 0;
    upload_rows = 0;
    mb_canvas_close();
}
static u8 sprite_visible[79], band_count[28];
static u16 band_pixels[28];
volatile MB_GraphicsStats mb_graphics_observation __attribute__((externally_visible));
static u16 dragging, drag_dx, drag_dy;
static void queue_palette(u16 line, const u16 *colors) {
    u16 i;
    for (i = 0; i < 16; i++)
        palette_words[line][i] = colors[i];
    palette_pending |= 1 << line;
}
static void flush_palettes(void) {
    u16 i;
    for (i = 0; i < 4; i++)
        if (palette_pending & (1 << i))
            PAL_setPalette(i, palette_words[i], CPU);
    palette_pending = 0;
}
static MB_ThemeId applied_theme, visible_theme;
static MB_Rect client = {4, 5, 30, 14};
static MB_WindowMode mode;
volatile uint16_t mb_ui_observation[7] __attribute__((externally_visible));
static void observe(void) {
    mb_ui_observation[0] = mode;
    mb_ui_observation[1] = client.x;
    mb_ui_observation[2] = client.y;
    mb_ui_observation[3] = client.width;
    mb_ui_observation[4] = client.height;
    mb_ui_observation[5] = visible_theme;
    mb_ui_observation[6] = lease_token;
}
static char window_title[26];
static u16 normal_x = 3, normal_y = 4;
void mb_video_boot_reset(void) {
    mb_cursor_reset();
    applied_theme = visible_theme = MB_THEME_BLUE;
    mode = MB_WINDOW_NORMAL;
    lease_token = leased_sprites = palette_pending = dragging = 0;
    normal_x = 3;
    normal_y = 4;
}
static const u16 style_tiles[] = {1, 2, 4, 3, 0};
typedef struct {
    u16 id, x, y, w, flags;
    char label[12];
} Button;
#define BUTTON_MENU_ROW 0x4000
#define BUTTON_MENU_OPEN 0x8000
static Button buttons[24];
static u16 button_count, focus = 0xffff, pressed_button = 0xffff, pressed_ticks;
static void mark(u16 cell) {
    u8 mask = 1 << (cell & 7);
    if (!(dirty[cell >> 3] & mask)) {
        dirty[cell >> 3] |= mask;
        ++dirty_count;
    }
}
static u16 is_dirty(u16 cell) {
    return dirty[cell >> 3] & (1 << (cell & 7));
}
static void cell(u16 x, u16 y, u16 a, u16 b) {
    u16 i = y * 40 + x;
    if (plane_a[i] != a || plane_b[i] != b) {
        plane_a[i] = a;
        plane_b[i] = b;
        mark(i);
    }
}
void mb_canvas_close(void) {
    if (cursor_scope == MB_CURSOR_CANVAS)
        mb_cursor_reset();
    if (canvas_opened == 2) {
        palette_apply();
        VDP_setBackgroundColor(1);
    }
    canvas_pixels = 0;
    canvas_opened = 0;
    canvas_cursor_ready = 0;
    canvas_brush_size = 1;
    canvas_cursor_dot = 0;
    canvas_cx = canvas_cy = 65535;
}
MB_Result mb_canvas_open(uint16_t x, uint16_t y) {
    u16 i, j;
    if (!mb_runtime_active() || lease_token)
        return MB_UNAVAILABLE;
    if (x > client.width || y > client.height || 16 > client.width - x || 16 > client.height - y)
        return MB_INVALID;
    mb_canvas_close();
    canvas_opened = 1;
    canvas_x = x;
    canvas_y = y;
    canvas_pixels = 0;
    canvas_cx = canvas_cy = 65535;
    canvas_scan = 0;
    for (i = 0; i < 32; i++)
        canvas_dirty[i] = 255;
    for (j = 0; j < 16; j++)
        for (i = 0; i < 16; i++) {
            u16 tile = TILE_ATTR_FULL(PAL0, 0, 0, 0, TILE_BASE + 64 + j * 16 + i);
            cell(client.x + x + i, client.y + y + j, tile, tile);
        }
    return MB_OK;
}
static void canvas_mark(u16 x, u16 y) {
    if (x < 64 && y < 64) {
        u16 t = (y / 4) * 16 + x / 4;
        canvas_dirty[t >> 3] |= 1 << (t & 7);
    }
}
MB_Result mb_canvas_update(const uint8_t *pixels, uint16_t x, uint16_t y, uint16_t w, uint16_t h,
                           uint16_t cx, uint16_t cy) {
    u16 i, j;
    if (canvas_opened != 1 || !pixels || x >= 64 || y >= 64 || !w || !h || w > 64 - x ||
        h > 64 - y || ((cx != 65535 || cy != 65535) && (cx >= 64 || cy >= 64)))
        return MB_INVALID;
    canvas_pixels = pixels;
    canvas_mark(canvas_cx, canvas_cy);
    canvas_mark(cx, cy);
    canvas_cx = cx;
    canvas_cy = cy;
    for (j = y / 4; j <= (y + h - 1) / 4; j++)
        for (i = x / 4; i <= (x + w - 1) / 4; i++) {
            u16 t = j * 16 + i;
            canvas_dirty[t >> 3] |= 1 << (t & 7);
        }
    return MB_OK;
}
/* Convert only dirty 4x4 logical blocks into scaled 8x8 VDP patterns after VBlank.
 * Two planes reference the same pattern; no CRAM writes or second pixel buffer. */
static u16 canvas_legacy_flush(void) {
    u16 scanned = 0, count = 0;
    u32 rows[8];
    static const u16 colors[] = {2, 3, 5, 6};
    if (!canvas_opened || !canvas_pixels)
        return 0;
    while (scanned < 256 && count < 8) {
        u16 t = canvas_scan++, mask = 1 << (t & 7), x, y;
        canvas_scan &= 255;
        ++scanned;
        if (!(canvas_dirty[t >> 3] & mask))
            continue;
        canvas_dirty[t >> 3] &= ~mask;
        for (y = 0; y < 8; y++) {
            u32 row = 0;
            u16 py = (t / 16) * 4 + y / 2;
            for (x = 0; x < 8; x++) {
                u16 px = (t % 16) * 4 + x / 2,
                    c = (canvas_pixels[py * 16 + px / 4] >> (6 - (px % 4) * 2)) & 3;
                if (px == canvas_cx && py == canvas_cy)
                    c ^= 3;
                row = (row << 4) | colors[c];
            }
            rows[y] = row;
        }
        VDP_loadTileData(rows, TILE_BASE + 64 + t, 1, CPU);
        ++count;
    }
    return count * 32;
}
static u16 canvas_colors_valid(const u16 *colors) {
    u16 i;
    if (!colors)
        return 0;
    for (i = 0; i < 16; ++i)
        if (colors[i] & ~0xeee)
            return 0;
    return 1;
}
MB_Result mb_canvas_palette(const uint16_t *colors) {
    if (canvas_opened != 2 || !canvas_colors_valid(colors))
        return MB_INVALID;
    queue_palette(PAL2, colors);
    return MB_OK;
}
MB_Result mb_canvas_attach(const MB_CanvasSource *source, uint16_t x, uint16_t y, uint16_t w,
                           uint16_t h, const uint16_t *colors) {
    u16 i, j, same;
    if (!source || (!source->pixel && !source->tile) || !source->context || !source->width ||
        !source->height || source->width > 512 || source->height > 256 || (source->width & 7) ||
        (source->height & 7) || !w || !h || w > 40 || h > 28 || x >= client.width ||
        y >= client.height || w > client.width - x || h > client.height - y ||
        TILE_BASE + 64 + (u32)w * h > TILE_FONT_INDEX || TILE_BASE + 64 + 1140 > TILE_FONT_INDEX ||
        !canvas_colors_valid(colors))
        return MB_INVALID;
    if (source->version != 1)
        return MB_VERSION;
    if (!mb_runtime_active() || lease_token)
        return MB_UNAVAILABLE;
    same = canvas_opened == 2 && canvas_source.context == source->context &&
           canvas_source.width == source->width && canvas_source.height == source->height &&
           canvas_width == w && canvas_height == h;
    if (!same) {
        mb_canvas_close();
        canvas_ox = canvas_oy = canvas_scan = 0;
        canvas_scale = 1;
        for (i = 0; i < sizeof canvas_dirty; ++i)
            canvas_dirty[i] = 255;
        canvas_swatches_pending = 1;
    }
    canvas_source = *source;
    canvas_opened = 2;
    canvas_x = x;
    canvas_y = y;
    canvas_width = w;
    canvas_height = h;
    queue_palette(PAL2, colors);
    VDP_setBackgroundColor(32);
    for (j = 0; j < h; ++j)
        for (i = 0; i < w; ++i) {
            u16 tile = TILE_ATTR_FULL(PAL2, 0, 0, 0, TILE_BASE + 64 + j * w + i);
            cell(client.x + x + i, client.y + y + j, tile, tile);
        }
    return MB_OK;
}
static void canvas_cursor_prepare(void) {
    u16 pixels = canvas_scale ? canvas_brush_size * canvas_scale : (canvas_brush_size + 1) / 2;
    u16 extent = canvas_cursor_dot == 1 ? 3 : canvas_cursor_dot == 2 ? pixels : pixels + 4;
    if (canvas_cursor_extent != extent || !canvas_cursor_ready) {
        canvas_cursor_extent = extent;
        canvas_cursor_pending = 1;
        canvas_cursor_ready = 0;
    }
}
MB_Result mb_canvas_brush(uint16_t size) {
    if (!size || size > 8)
        return MB_INVALID;
    if (canvas_opened != 2)
        return MB_UNAVAILABLE;
    canvas_brush_size = size;
    /* Dot geometry does not change with the brush, so no upload is needed. */
    mb_brush_cursor_observation[0] = size;
    canvas_cursor_prepare();
    return MB_OK;
}
MB_Result mb_canvas_cursor(uint16_t dot) {
    if (dot > 2)
        return MB_INVALID;
    if (canvas_opened != 2)
        return MB_UNAVAILABLE;
    if (canvas_cursor_dot != dot) {
        canvas_cursor_dot = dot;
        canvas_cursor_ready = 0;
        canvas_cursor_prepare();
    }
    return MB_OK;
}
void mb_cursor_reset(void) {
    mb_cursor_observation.width = mb_cursor_observation.height = 0;
    cursor_pending = cursor_ready = 0;
}
MB_Result mb_cursor_set(const MB_Cursor *cursor, uint16_t scope) {
    if (!cursor || !cursor->width || cursor->width > 16 || !cursor->height || cursor->height > 16 ||
        cursor->hotspot_x >= cursor->width || cursor->hotspot_y >= cursor->height ||
        scope > MB_CURSOR_CANVAS)
        return MB_INVALID;
    if (!mb_runtime_active() || !window_open)
        return MB_UNAVAILABLE;
    if (lease_token)
        return MB_BUSY;
    if (scope == MB_CURSOR_CANVAS && canvas_opened != 2)
        return MB_UNAVAILABLE;
    mb_cursor_observation = *cursor;
    cursor_scope = scope;
    cursor_pending = 1;
    cursor_ready = 0;
    return MB_OK;
}
static u16 cursor_flush(void) {
    u16 k, x, y;
    u32 rows[8];
    if (!cursor_pending || lease_token)
        return 0;
    for (k = 0; k < 4; ++k) {
        for (y = 0; y < 8; ++y) {
            u32 row = 0;
            u16 py = (k / 2) * 8 + y;
            for (x = 0; x < 8; ++x) {
                u16 px = (k % 2) * 8 + x, c = 0;
                if (px < mb_cursor_observation.width && py < mb_cursor_observation.height)
                    c = (mb_cursor_observation.rows[py] >> (30 - px * 2)) & 3;
                row = (row << 4) | c;
            }
            rows[y] = row;
        }
        VDP_loadTileData(rows, CURSOR_TILE + k, 1, CPU);
    }
    cursor_pending = 0;
    cursor_ready = 1;
    return 128;
}
static void cursor_show(int16_t x, int16_t y) {
    u16 k;
    for (k = 0; k < 4; ++k)
        VDP_setSpriteFull(k, x - mb_cursor_observation.hotspot_x + (k % 2) * 8,
                          y - mb_cursor_observation.hotspot_y + (k / 2) * 8, SPRITE_SIZE(1, 1),
                          TILE_ATTR_FULL(PAL3, 1, 0, 0, CURSOR_TILE + k), k == 3 ? 0 : k + 1);
    VDP_updateSprites(4, CPU);
}
MB_Result mb_canvas_swatch(uint16_t x, uint16_t y, uint16_t color) {
    u16 tile, i;
    if (canvas_opened != 2 || x >= client.width || y >= client.height || color > 15)
        return MB_INVALID;
    tile = TILE_ATTR_FULL(PAL2, 0, 0, 0, TILE_BASE + 64 + 1120 + color);
    cell(client.x + x, client.y + y, tile, tile);
    /* A swatch on a button's final cell belongs to that widget's redraw.
     * Retain it in private flag bits so hover/focus/press never erase it. */
    for (i = 0; i < button_count; ++i)
        if (y == buttons[i].y && x == buttons[i].x + buttons[i].w - 1) {
            buttons[i].flags =
                (buttons[i].flags & (MB_UI_ENABLED | BUTTON_MENU_ROW)) | 0x100 | (color << 9);
            if (!buttons[i].label[0] && !(buttons[i].flags & BUTTON_MENU_ROW)) {
                u16 dx, dy;
                buttons[i].flags |= 0x2000;
                for (dy = 0; dy < 2; ++dy)
                    for (dx = 0; dx < 2; ++dx)
                        cell(client.x + x - dx, client.y + y + dy, tile, tile);
            }
        }
    return MB_OK;
}
static void canvas_cursor_mark(u16 x, u16 y, u16 priority) {
    u32 sx, sy;
    u16 t;
    if (x == 65535 || y == 65535 || x < canvas_ox || y < canvas_oy)
        return;
    sx = x - canvas_ox;
    sy = y - canvas_oy;
    if (canvas_scale) {
        sx *= canvas_scale;
        sy *= canvas_scale;
    } else {
        sx /= 2;
        sy /= 2;
    }
    if (sx >= canvas_width * 8 || sy >= canvas_height * 8)
        return;
    t = (sy / 8) * canvas_width + sx / 8;
    if (priority)
        canvas_scan = t;
}
MB_Result mb_canvas_view(uint16_t x, uint16_t y, uint16_t scale, uint16_t cx, uint16_t cy) {
    u16 i;
    if (canvas_opened != 2 || x >= canvas_source.width || y >= canvas_source.height ||
        (scale != 0 && scale != 1 && scale != 2 && scale != 4 && scale != 8) ||
        ((cx != 65535 || cy != 65535) && (cx >= canvas_source.width || cy >= canvas_source.height)))
        return MB_INVALID;
    if (x != canvas_ox || y != canvas_oy || scale != canvas_scale) {
        for (i = 0; i < sizeof canvas_dirty; ++i)
            canvas_dirty[i] = 255;
    }
    canvas_cursor_mark(canvas_cx, canvas_cy, 0);
    canvas_ox = x;
    canvas_oy = y;
    canvas_scale = scale;
    canvas_cursor_prepare();
    canvas_cx = cx;
    canvas_cy = cy;
    canvas_cursor_mark(cx, cy, 1);
    return MB_OK;
}
MB_Result mb_canvas_damage(uint16_t x, uint16_t y, uint16_t w, uint16_t h) {
    int32_t sx, sy, ex, ey;
    u16 i, j;
    if (canvas_opened != 2 || !w || !h || x >= canvas_source.width || y >= canvas_source.height ||
        w > canvas_source.width - x || h > canvas_source.height - y)
        return MB_INVALID;
    sx = (int32_t)x - canvas_ox;
    sy = (int32_t)y - canvas_oy;
    ex = sx + w - 1;
    ey = sy + h - 1;
    if (canvas_scale) {
        sx *= canvas_scale;
        sy *= canvas_scale;
        ex = (ex + 1) * canvas_scale - 1;
        ey = (ey + 1) * canvas_scale - 1;
    } else {
        sx = sx < 0 ? -((-sx + 1) / 2) : sx / 2;
        sy = sy < 0 ? -((-sy + 1) / 2) : sy / 2;
        ex = ex < 0 ? -((-ex + 1) / 2) : ex / 2;
        ey = ey < 0 ? -((-ey + 1) / 2) : ey / 2;
    }
    if (ex < 0 || ey < 0 || sx >= canvas_width * 8 || sy >= canvas_height * 8)
        return MB_OK;
    if (sx < 0)
        sx = 0;
    if (sy < 0)
        sy = 0;
    if (ex >= canvas_width * 8)
        ex = canvas_width * 8 - 1;
    if (ey >= canvas_height * 8)
        ey = canvas_height * 8 - 1;
    for (j = sy / 8; j <= ey / 8; ++j)
        for (i = sx / 8; i <= ex / 8; ++i) {
            u16 t = j * canvas_width + i;
            canvas_dirty[t / 8] |= 1 << (t & 7);
        }
    return MB_OK;
}
/* Stream only dirty visible tiles through a 32-byte stack pattern. PAL2 index zero
 * is transparent on both planes, so background CRAM is explicitly PAL2 colour zero. */
static u16 canvas_flush(void) {
    u16 total, limit, scanned = 0, n = 0,
                      shift = canvas_scale == 8   ? 3
                              : canvas_scale == 4 ? 2
                              : canvas_scale == 2 ? 1
                                                  : 0;
    u32 rows[8], cache[4][8];
    u16 cache_x[4], cache_y[4], used, next;
    if (canvas_opened != 2)
        return canvas_legacy_flush();
    if (canvas_swatches_pending == 1) {
        u16 i, j;
        for (i = 0; i < 16; ++i) {
            for (j = 0; j < 8; ++j)
                rows[j] = (u32)i * 0x11111111;
            VDP_loadTileData(rows, TILE_BASE + 64 + 1120 + i, 1, CPU);
        }
        canvas_swatches_pending = 0;
        return 512;
    }
    if (canvas_cursor_pending) {
        u16 k, x, y, count = canvas_cursor_extent <= 8 ? 1 : 4;
        for (k = 0; k < count; ++k) {
            u16 ox = (k & 1) ? canvas_cursor_extent - 8 : 0,
                oy = (k & 2) ? canvas_cursor_extent - 8 : 0;
            for (y = 0; y < 8; ++y) {
                u32 row = 0;
                for (x = 0; x < 8; ++x) {
                    u16 px = ox + x, py = oy + y, c = 0, edge = canvas_cursor_extent - 1;
                    if (canvas_cursor_dot == 1) {
                        if (px < 3 && py < 3 && (px == 1 || py == 1))
                            c = 1;
                    } else if (canvas_cursor_dot == 2) {
                        if (px <= edge && py <= edge && (!px || !py || px == edge || py == edge))
                            c = 1;
                    } else if (px <= edge && py <= edge) {
                        if (!px || !py || px == edge || py == edge)
                            c = 1;
                        else if (px == 1 || py == 1 || px == edge - 1 || py == edge - 1)
                            c = 2;
                    }
                    row = (row << 4) | c;
                }
                rows[y] = row;
            }
            VDP_loadTileData(rows, TILE_BASE + 64 + 1136 + k, 1, CPU);
        }
        canvas_cursor_pending = 0;
        canvas_cursor_ready = 1;
        mb_brush_cursor_observation[0] = canvas_brush_size;
        mb_brush_cursor_observation[1] = canvas_cursor_extent;
        mb_brush_cursor_observation[2] = mb_brush_cursor_observation[3] = count;
        return count * 32;
    }
    total = canvas_width * canvas_height;
    /* Scaled/unaligned sampling costs more than native pattern copies. Keep its
     * per-refresh work smaller so NTSC input polling remains responsive. */
    limit = !canvas_source.tile                                         ? 1
            : canvas_scale == 1 && !(canvas_ox & 7) && !(canvas_oy & 7) ? 16
                                                                        : 8;
    while (scanned++ < total && n < limit) {
        u16 t = canvas_scan, x, y, sx0, sy0, px0, py0;
        /* Skip clean bitmap bytes before any coordinate division. A complete idle
         * viewport must not do 880 software divides on a 68000 every refresh. */
        if (!(t & 7) && !canvas_dirty[t / 8] && t + 8 <= total) {
            canvas_scan += 8;
            scanned += 7;
            if (canvas_scan == total)
                canvas_scan = 0;
            continue;
        }
        if (++canvas_scan == total)
            canvas_scan = 0;
        if (!(canvas_dirty[t / 8] & (1 << (t & 7))))
            continue;
        canvas_dirty[t / 8] &= ~(1 << (t & 7));
        sx0 = (t % canvas_width) * 8;
        sy0 = (t / canvas_width) * 8;
        px0 = canvas_ox + sx0;
        py0 = canvas_oy + sy0;
        if (canvas_scale == 1 && !(canvas_ox & 7) && !(canvas_oy & 7) && canvas_source.tile &&
            px0 < canvas_source.width && py0 < canvas_source.height) {
            canvas_source.tile(canvas_source.context, px0 >> 3, py0 >> 3, rows);
        } else {
            used = next = 0;
            for (y = 0; y < 8; ++y) {
                u32 row = 0;
                for (x = 0; x < 8; ++x) {
                    u16 sx = sx0 + x, sy = sy0 + y;
                    u16 px = canvas_ox + (canvas_scale ? (sx >> shift) : (sx << 1)),
                        py = canvas_oy + (canvas_scale ? (sy >> shift) : (sy << 1));
                    u8 c = 0;
                    if (px < canvas_source.width && py < canvas_source.height) {
                        if (canvas_source.tile) {
                            u16 k, tx = px >> 3, ty = py >> 3;
                            for (k = 0; k < used; ++k)
                                if (cache_x[k] == tx && cache_y[k] == ty)
                                    break;
                            if (k == used) {
                                k = next;
                                next = (next + 1) & 3;
                                if (used < 4)
                                    ++used;
                                cache_x[k] = tx;
                                cache_y[k] = ty;
                                canvas_source.tile(canvas_source.context, tx, ty, cache[k]);
                            }
                            c = (cache[k][py & 7] >> ((7 - (px & 7)) * 4)) & 15;
                        } else
                            c = canvas_source.pixel(canvas_source.context, px, py) & 15;
                    }
                    row = (row << 4) | c;
                }
                rows[y] = row;
            }
        }
        VDP_loadTileData(rows, TILE_BASE + 64 + t, 1, CPU);
        ++n;
    }
    mb_canvas_observation[0] = canvas_width * 8;
    mb_canvas_observation[1] = canvas_height * 8;
    mb_canvas_observation[2] = canvas_ox;
    mb_canvas_observation[3] = canvas_oy;
    mb_canvas_observation[4] = canvas_scale;
    mb_canvas_observation[5] = n;
    mb_canvas_observation[6] = n * 32;
    mb_canvas_observation[7] = total;
    return n * 32;
}
static void fill(u16 tile, u16 x, u16 y, u16 w, u16 h) {
    u16 cx, cy, a = TILE_ATTR_FULL(PAL0, 0, 0, 0, TILE_BASE + tile);
    for (cy = y; cy < y + h; cy++)
        for (cx = x; cx < x + w; cx++)
            cell(cx, cy, a, a);
}
static void text(u16 pal, const char *s, u16 x, u16 y, u16 max) {
    u16 i;
    for (i = 0; i < max && s[i]; i++) {
        u8 c = s[i];
        u16 index = (y * 40) + x + i;
        if (c < 32 || c > 127)
            c = '?';
        cell(x + i, y, TILE_ATTR_FULL(pal, 1, 0, 0, TILE_FONT_INDEX + c - 32), plane_b[index]);
    }
}
void mb_video_init(void) {
    u16 p[16], i;
    VDP_setScreenWidth320();
    VDP_setScreenHeight224();
    VDP_setPlaneSize(64, 32, TRUE);
    VDP_setEnable(FALSE);
    VDP_loadTileData((const u32 *)ui_tiles, TILE_BASE, UI_TILE_COUNT, CPU);
    for (i = 0; i < 16; i++)
        p[i] = ui_palette[i];
    p[15] = ui_palette[6];
    PAL_setPalette(PAL0, p, CPU);
    p[15] = ui_palette[3];
    PAL_setPalette(PAL1, p, CPU);
    p[1] = ui_palette[3];
    p[2] = ui_palette[2];
    p[3] = ui_palette[5];
    PAL_setPalette(PAL2, p, CPU);
    p[2] = ui_palette[6];
    PAL_setPalette(PAL3, p, CPU);
    palette_apply();
    flush_palettes();
    VDP_setBackgroundColor(1);
    VDP_resetSprites();
    for (i = 0; i < CELLS; i++) {
        plane_a[i] = 0;
        plane_b[i] = 0;
    }
    for (i = 0; i < sizeof(dirty); i++)
        dirty[i] = 0;
    dirty_count = 0;
    flush_cursor = 0;
    heap_label = 0xffff;
    mb_ui_reset();
    for (i = 0; i < CELLS; i++)
        mark(i);
    mb_diagnostics.tiles = UI_TILE_COUNT;
    VDP_setEnable(TRUE);
}
void mb_video_flush(void) {
    u16 budget = FLUSH_BUDGET, scanned = 0;
    u16 bytes = 0, p;
    for (p = 0; p < 4; p++)
        if (palette_pending & (1 << p))
            bytes += 32;
    flush_palettes();
    if (upload_left && lease_token) {
        u16 count = upload_left > 24 ? 24 : upload_left;
        VDP_loadTileData(upload_rows, upload_tile, count, CPU);
        upload_rows += count * 8;
        upload_tile += count;
        upload_left -= count;
        mb_graphics_observation.loaded_tiles += count;
        bytes += count * 32;
    }
    bytes += canvas_flush();
    bytes += cursor_flush();
    if (sprites_pending) {
        VDP_updateSprites(leased_sprites + 1, CPU);
        bytes += (leased_sprites + 1) * 8;
        sprites_pending = 0;
    }
    while (dirty_count && budget && scanned < CELLS) {
        u16 start, len = 0, row, x;
        while (scanned < CELLS && !is_dirty(flush_cursor)) {
            u16 skip = (!(flush_cursor & 7) && !dirty[flush_cursor >> 3]) ? 8 : 1;
            flush_cursor += skip;
            scanned += skip;
            if (flush_cursor == CELLS)
                flush_cursor = 0;
        }
        if (scanned == CELLS)
            break;
        start = flush_cursor;
        row = start / 40;
        x = start % 40;
        while (len < budget && x + len < 40 && is_dirty(start + len)) {
            dirty[(start + len) >> 3] &= ~(1 << ((start + len) & 7));
            ++len;
            --dirty_count;
        }
        VDP_setTileMapDataRow(BG_A, plane_a + start, row, x, len, CPU);
        VDP_setTileMapDataRow(BG_B, plane_b + start, row, x, len, CPU);
        budget -= len;
        bytes += len * 4;
        scanned += len;
        flush_cursor = (start + len) % CELLS;
    }
    mb_diagnostics.dirty_cells = dirty_count;
    if (lease_token) {
        mb_graphics_observation.queue_tiles = upload_left;
        mb_graphics_observation.sprite_count = leased_sprites;
        mb_graphics_observation.transfer_last = bytes;
        if (bytes > mb_graphics_observation.transfer_peak)
            mb_graphics_observation.transfer_peak = bytes;
    }
}
void mb_desktop_status(void) {
    MB_MemoryStats stats;
    char n[12];
    u16 available = 0xffff;
    if (mb_memory_stats(&stats) == MB_OK)
        available = stats.free / 1024;
    if (mode == MB_WINDOW_FULLSCREEN || available == heap_label)
        return;
    heap_label = available;
    fill(3, mode == MB_WINDOW_MAXIMIZED ? 22 : 21, 0, mode == MB_WINDOW_MAXIMIZED ? 15 : 19, 1);
    if (available == 0xffff)
        text(PAL0, "RAM --", 23, 0, 7);
    else {
        mb_format_u32(available, n, sizeof(n));
        text(PAL0, n, 23, 0, 2);
        text(PAL0, "K free", 25, 0, 6);
    }
}
void mb_desktop_draw(void) {
    window_open = 0;
    mb_ui_reset();
    fill(0, 0, 0, 40, 28);
    fill(3, 0, 0, 40, 1);
    text(PAL0, "MegaBenchOS", 1, 0, 11);
    heap_label = 0xffff;
    mb_desktop_status();
    mb_shell_draw();
}
void mb_desktop_activate(void) {
    MB_Input in;
    MB_Event e = {MB_ACTIVATE, 0, 0};
    mb_input_state(&in);
    e.x = in.x;
    e.y = in.y;
    mb_shell_event(&e);
}
static void chrome(void) {
    u16 x = mode == MB_WINDOW_NORMAL ? normal_x : 0;
    u16 y = mode == MB_WINDOW_NORMAL ? normal_y : 0;
    u16 w = mode == MB_WINDOW_NORMAL ? 32 : 40;
    mb_ui_reset();
    if (mode == MB_WINDOW_FULLSCREEN) {
        client = (MB_Rect){0, 0, 40, 28};
        fill(1, 0, 0, 40, 28);
        observe();
        return;
    }
    client = mode == MB_WINDOW_NORMAL ? (MB_Rect){x + 1, y + 1, 30, 14} : (MB_Rect){0, 1, 40, 27};
    fill(2, x, y, w, mode == MB_WINDOW_NORMAL ? 16 : 28);
    fill(3, x, y, w, 1);
    if (mode == MB_WINDOW_MAXIMIZED) {
        text(PAL0, window_title, 1, 0, 20);
        text(PAL0, "[X]", 37, 0, 3);
        heap_label = 0xffff;
        mb_desktop_status();
    } else {
        text(PAL0, "[X]", x + 1, y, 3);
        text(PAL0, window_title, x + 5, y, w - 6);
    }
    fill(1, client.x, client.y, client.width, client.height);
    observe();
}
MB_Result mb_window_open(const char *title) {
    u16 i;
    if (!title)
        return MB_INVALID;
    for (i = 0; i < 25 && title[i]; i++)
        window_title[i] = title[i];
    window_title[i] = 0;
    window_open = 1;
    chrome();
    return MB_OK;
}
MB_Result mb_draw_text(uint16_t x, uint16_t y, const char *s) {
    if (!window_open || !s || x >= client.width || y >= client.height)
        return MB_INVALID;
    text(PAL1, s, client.x + x, client.y + y, client.width - x);
    return MB_OK;
}
MB_Result mb_draw_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t style) {
    if (!window_open || !w || !h || x >= client.width || y >= client.height ||
        w > client.width - x || h > client.height - y || style > MB_STYLE_DESKTOP)
        return MB_INVALID;
    fill(style_tiles[style], client.x + x, client.y + y, w, h);
    return MB_OK;
}
MB_Result mb_draw_field(uint16_t x, uint16_t y, uint16_t w, const char *s) {
    if (!s)
        return MB_INVALID;
    if (mb_draw_rect(x, y, w, 1, MB_STYLE_SURFACE) != MB_OK)
        return MB_INVALID;
    text(PAL1, s, client.x + x, client.y + y, w);
    return MB_OK;
}
MB_Result mb_window_clear(void) {
    if (!window_open)
        return MB_UNAVAILABLE;
    mb_ui_reset();
    fill(1, client.x, client.y, client.width, client.height);
    return MB_OK;
}
void mb_window_reset(void) {
    mb_cursor_reset();
    window_open = 0;
    mode = MB_WINDOW_NORMAL;
    dragging = 0;
    mb_ui_reset();
}
void mb_pointer(int16_t x, int16_t y) {
    MB_Input input;
    u16 tile = TILE_BASE + 5, show;
    mb_input_state(&input);
    if (mode == MB_WINDOW_FULLSCREEN && lease_token)
        return;
    show = input.mode == MB_INPUT_POINTER && mode != MB_WINDOW_FULLSCREEN;
    if (cursor_ready && cursor_scope == MB_CURSOR_CLIENT && input.mode == MB_INPUT_POINTER &&
        x >= client.x * 8 && y >= client.y * 8 && x < (client.x + client.width) * 8 &&
        y < (client.y + client.height) * 8) {
        cursor_show(x, y);
        return;
    }
    if (canvas_opened == 2 && canvas_cursor_ready && canvas_cx != 65535 && canvas_cy != 65535) {
        int32_t sx = (int32_t)canvas_cx - canvas_ox, sy = (int32_t)canvas_cy - canvas_oy;
        sx = canvas_scale ? sx * canvas_scale : sx / 2;
        sy = canvas_scale ? sy * canvas_scale : sy / 2;
        if (input.mode == MB_INPUT_POINTER) {
            int16_t mx = x - (client.x + canvas_x) * 8, my = y - (client.y + canvas_y) * 8;
            if (mx < 0 || my < 0 || mx >= canvas_width * 8 || my >= canvas_height * 8)
                sx = -1;
        }
        if (sx >= 0 && sy >= 0 && sx < canvas_width * 8 && sy < canvas_height * 8) {
            u16 k, count = canvas_cursor_extent <= 8 ? 1 : 4;
            if (cursor_ready && cursor_scope == MB_CURSOR_CANVAS) {
                cursor_show((client.x + canvas_x) * 8 + sx, (client.y + canvas_y) * 8 + sy);
                return;
            }
            {
                u16 offset = canvas_cursor_dot == 1 ? 1
                             : canvas_cursor_dot == 2
                                 ? (canvas_scale ? (canvas_brush_size / 2) * canvas_scale
                                                 : canvas_brush_size / 4)
                                 : 2;
                x = (client.x + canvas_x) * 8 + sx - offset;
                y = (client.y + canvas_y) * 8 + sy - offset;
            }
            for (k = 0; k < count; ++k)
                VDP_setSpriteFull(k, x + ((k & 1) ? canvas_cursor_extent - 8 : 0),
                                  y + ((k & 2) ? canvas_cursor_extent - 8 : 0), SPRITE_SIZE(1, 1),
                                  TILE_ATTR_FULL(PAL3, 1, 0, 0, TILE_BASE + 64 + 1136 + k),
                                  k + 1 == count ? 0 : k + 1);
            VDP_updateSprites(count, CPU);
            return;
        }
    }
    if (!show)
        VDP_updateSprites(0, CPU);
    else {
        VDP_setSpriteFull(0, x, y, SPRITE_SIZE(1, 1), TILE_ATTR_FULL(PAL3, 1, 0, 0, tile), 0);
        VDP_updateSprites(1, CPU);
    }
}
static void button_draw(u16 index) {
    Button *b = &buttons[index];
    u16 chosen = (index == focus), pal = PAL1, i, max = b->w >= 3 && b->w <= 40 ? b->w - 2 : 0;
    char label[41];
    fill(chosen ? UI_TILE_SELECTION : 1, client.x + b->x, client.y + b->y, b->w,
         (b->flags & BUTTON_MENU_ROW) ? 1 : 2);
    label[0] = chosen ? '[' : ((b->flags & MB_UI_ENABLED) ? ' ' : '-');
    for (i = 0; i < max && i < 38; i++)
        label[i + 1] = (i < 11 && b->label[i]) ? b->label[i] : ' ';
    if (b->w < 3 || b->w > 40)
        return;
    label[b->w - 1] = chosen ? ']' : ((b->flags & MB_UI_ENABLED) ? ' ' : '-');
    label[b->w] = 0;
    if (index == pressed_button && pressed_ticks)
        label[0] = '*';
    text(pal, label, client.x + b->x, client.y + b->y, b->w);
    if (canvas_opened == 2 && (b->flags & 0x100)) {
        u16 tile = TILE_ATTR_FULL(PAL2, 0, 0, 0, TILE_BASE + 64 + 1120 + ((b->flags >> 9) & 15));
        cell(client.x + b->x + b->w - 1, client.y + b->y, tile, tile);
        if (b->flags & 0x2000) {
            u16 dx, dy;
            for (dy = 0; dy < 2; ++dy)
                for (dx = 0; dx < 2; ++dx)
                    cell(client.x + b->x + b->w - 1 - dx, client.y + b->y + dy, tile, tile);
        }
    }
}
void mb_ui_reset(void) {
    button_count = 0;
    focus = 0xffff;
    pressed_button = 0xffff;
    pressed_ticks = 0;
}
static MB_Result button_add(uint16_t id, uint16_t x, uint16_t y, uint16_t w, const char *label,
                            uint16_t flags, uint16_t rows) {
    u16 i, index;
    Button *b;
    if (id == 0xffff || id == MB_UI_MENU_DISMISS || !window_open || !label || x >= client.width ||
        y >= client.height || rows > client.height - y || w < 3 || w > client.width - x ||
        flags > (rows == 1 ? 3 : MB_UI_ENABLED) ||
        ((flags & MB_UI_MENU_OPEN) && !(flags & MB_UI_ENABLED)))
        return MB_INVALID;
    for (i = 0; i < button_count; i++)
        if (buttons[i].id == id)
            return MB_INVALID;
    if (button_count == 24)
        return MB_BUSY;
    index = button_count++;
    b = &buttons[index];
    b->id = id;
    b->x = x;
    b->y = y;
    b->w = w;
    b->flags = (flags & MB_UI_ENABLED) | (rows == 1 ? BUTTON_MENU_ROW : 0) |
               ((flags & MB_UI_MENU_OPEN) ? BUTTON_MENU_OPEN : 0);
    for (i = 0; i < 12; i++)
        b->label[i] = 0;
    for (i = 0; i < 11 && i < w - 2 && label[i]; i++)
        b->label[i] = label[i];
    if (label[i])
        b->label[i ? i - 1 : 0] = '>';
    if (focus == 0xffff && (flags & MB_UI_ENABLED))
        focus = index;
    button_draw(index);
    return MB_OK;
}
MB_Result mb_ui_button(uint16_t id, uint16_t x, uint16_t y, uint16_t w, const char *label,
                       uint16_t flags) {
    return button_add(id, x, y, w, label, flags, 2);
}
MB_Result mb_ui_menu_button(uint16_t id, uint16_t x, uint16_t w, const char *label,
                            uint16_t flags) {
    return button_add(id, x, 0, w, label, flags, 1);
}
MB_Result mb_ui_focus(uint16_t id) {
    u16 i, old = focus;
    for (i = 0; i < button_count; i++)
        if (buttons[i].id == id && (buttons[i].flags & MB_UI_ENABLED)) {
            focus = i;
            if (old < button_count)
                button_draw(old);
            button_draw(i);
            return MB_OK;
        }
    return MB_NOT_FOUND;
}
MB_Result mb_ui_exit(void) {
    if (mode != MB_WINDOW_FULLSCREEN)
        return MB_UNAVAILABLE;
    return mb_ui_button(MB_UI_EXIT, client.width - 8, client.height - 2, 8, "Exit", MB_UI_ENABLED);
}
static u16 hit(int16_t px, int16_t py) {
    u16 i;
    for (i = 0; i < button_count; i++) {
        Button *b = &buttons[i];
        if ((b->flags & MB_UI_ENABLED) && px >= (client.x + b->x) * 8 &&
            px < (client.x + b->x + b->w) * 8 && py >= (client.y + b->y) * 8 &&
            py < (client.y + ((b->flags & BUTTON_MENU_ROW) ? 1 : 2) + b->y) * 8)
            return i;
    }
    return 0xffff;
}
static u16 abs16(s16 n) {
    return n < 0 ? -n : n;
}
MB_Result mb_ui_event(const MB_Event *e, uint16_t *activated) {
    MB_Input input;
    u16 next = 0xffff, i, best = 0xffff;
    if (!e || !activated)
        return MB_INVALID;
    mb_input_state(&input);
    if (e->action == MB_MOVE && input.mode == MB_INPUT_POINTER)
        next = hit(e->x, e->y);
    else if (e->action == MB_NAVIGATE && focus < button_count) {
        for (i = 0; i < button_count; i++)
            if (i != focus && (buttons[i].flags & MB_UI_ENABLED)) {
                s16 dx = (s16)buttons[i].x - buttons[focus].x,
                    dy = (s16)buttons[i].y - buttons[focus].y;
                s16 primary = e->x ? dx * e->x : dy * e->y;
                u16 score = abs16(primary) * 4 + (e->x ? abs16(dy) : abs16(dx)) * 8;
                if (primary > 0 && score < best) {
                    best = score;
                    next = i;
                }
            }
        if (next == 0xffff)
            for (i = 1; i <= button_count; i++) {
                u16 n = (focus + ((e->x < 0 || e->y < 0) ? button_count - i : i)) % button_count;
                if (buttons[n].flags & MB_UI_ENABLED) {
                    next = n;
                    break;
                }
            }
    } else if (e->action == MB_ACTIVATE) {
        next = input.mode == MB_INPUT_POINTER ? hit(e->x, e->y) : focus;
        if (next < button_count && (buttons[next].flags & MB_UI_ENABLED)) {
            mb_ui_focus(buttons[next].id);
            pressed_button = next;
            pressed_ticks = 2;
            button_draw(next);
            *activated =
                (buttons[next].flags & BUTTON_MENU_OPEN) ? MB_UI_MENU_DISMISS : buttons[next].id;
            return MB_OK;
        }
        /* Open headings declare their state on redraw. Consume outside clicks
         * so the underlying canvas does not receive a stray drawing action. */
        if (input.mode == MB_INPUT_POINTER)
            for (i = 0; i < button_count; ++i)
                if (buttons[i].flags & BUTTON_MENU_OPEN) {
                    *activated = MB_UI_MENU_DISMISS;
                    return MB_OK;
                }
    }
    if (next < button_count)
        mb_ui_focus(buttons[next].id);
    return MB_NOT_FOUND;
}
void mb_ui_tick(void) {
    if (pressed_ticks && --pressed_ticks == 0 && pressed_button < button_count)
        button_draw(pressed_button);
}
/* Role palettes retain icon indices and font slot 15 across every theme. */
static const u16 themes[5][7] = {{234, 666, 233, 111, 777, 355, 466},
                                 {222, 555, 111, 0, 777, 456, 567},
                                 {132, 565, 121, 10, 777, 364, 475},
                                 {322, 665, 321, 110, 777, 653, 764},
                                 {134, 566, 123, 11, 777, 257, 367}};
static u16 rgb333(u16 n) {
    return RGB3_3_3_TO_VDPCOLOR(n / 100, (n / 10) % 10, n % 10);
}
static void palette_apply(void) {
    u16 p[16] = {0};
    const u16 *t = themes[visible_theme];
    p[1] = rgb333(t[0]);
    p[2] = rgb333(t[1]);
    p[3] = rgb333(t[3]);
    p[4] = rgb333(t[2]);
    p[5] = rgb333(t[5]);
    p[6] = rgb333(t[4]);
    p[7] = rgb333(t[6]);
    p[15] = p[6];
    queue_palette(PAL0, p);
    p[15] = p[3];
    queue_palette(PAL1, p);
    p[1] = rgb333(t[3]);
    p[2] = rgb333(t[1]);
    p[3] = rgb333(t[5]);
    queue_palette(PAL2, p);
    p[2] = rgb333(t[4]);
    queue_palette(PAL3, p);
}
void mb_display_recover(void) {
    VDP_setEnable(FALSE);
    lease_token = 0;
    leased_sprites = 0;
    sprites_pending = 0;
    upload_left = 0;
    upload_rows = 0;
    visible_theme = applied_theme;
    VDP_setHInterrupt(FALSE);
    SYS_setHIntCallback(0);
    VDP_setHilightShadow(FALSE);
    VDP_setScrollingMode(HSCROLL_PLANE, VSCROLL_PLANE);
    VDP_setHorizontalScroll(BG_A, 0);
    VDP_setHorizontalScroll(BG_B, 0);
    VDP_setVerticalScroll(BG_A, 0);
    VDP_setVerticalScroll(BG_B, 0);
    VDP_loadFont(&font_default, CPU);
    mb_video_init();
}
MB_Result mb_window_get_client_rect(MB_Rect *out) {
    if (!out)
        return MB_INVALID;
    if (!window_open)
        return MB_UNAVAILABLE;
    *out = client;
    return MB_OK;
}
/* One shared menu row may be reserved at the top of the queried client. */
MB_Result mb_window_get_content_rect(uint16_t menu_rows, MB_Rect *out) {
    MB_Result result;
    if (menu_rows > 1)
        return MB_INVALID;
    result = mb_window_get_client_rect(out);
    if (result != MB_OK)
        return result;
    out->y += menu_rows;
    out->height -= menu_rows;
    return MB_OK;
}
MB_WindowMode mb_window_mode(void) {
    return mode;
}
MB_Result mb_window_set_mode(MB_WindowMode next) {
    if (next > MB_WINDOW_FULLSCREEN)
        return MB_INVALID;
    if (!window_open)
        return MB_UNAVAILABLE;
    if (lease_token)
        return MB_BUSY;
    if (!mb_registry_mode_allowed(next, next == MB_WINDOW_NORMAL ? 30 : 40,
                                  next == MB_WINDOW_NORMAL      ? 14
                                  : next == MB_WINDOW_MAXIMIZED ? 27
                                                                : 28))
        return MB_UNAVAILABLE;
    if (next == mode)
        return MB_OK;
    mb_canvas_close();
    mb_runtime_notify(MB_DISPLAY_LOST);
    if (mode == MB_WINDOW_FULLSCREEN)
        mb_display_recover();
    mode = next;
    /* Repaint uncovered desktop before new chrome, without altering shell selection. */
    fill(0, 0, 0, 40, 28);
    if (mode != MB_WINDOW_FULLSCREEN) {
        fill(3, 0, 0, 40, 1);
        text(PAL0, "MegaBenchOS", 1, 0, 11);
        heap_label = 0xffff;
        mb_desktop_status();
    }
    chrome();
    mb_runtime_notify(MB_MODE_CHANGED);
    mb_runtime_notify(MB_RESIZE);
    mb_runtime_notify(MB_DISPLAY_RESTORED);
    return MB_OK;
}
MB_Result mb_window_restore(void) {
    return mb_window_set_mode(MB_WINDOW_NORMAL);
}
void mb_shell_surface(uint16_t root, const char *title) {
    mode = MB_WINDOW_NORMAL;
    if (root) {
        window_open = 1;
        client = (MB_Rect){0, 1, 40, 27};
        mb_ui_reset();
    } else
        mb_window_open(title);
    observe();
}
void mb_shell_icon(uint16_t x, uint16_t y, uint16_t icon) {
    u16 q;
    for (q = 0; q < 4; q++)
        cell(client.x + x + (q & 1), client.y + y + (q >> 1),
             TILE_ATTR_FULL(PAL2, 0, 0, 0, TILE_BASE + icon + q),
             plane_b[(client.y + y + (q >> 1)) * 40 + client.x + x + (q & 1)]);
}
void mb_shell_label(uint16_t x, uint16_t y, const char *label, uint16_t chosen, uint16_t root) {
    fill(chosen ? UI_TILE_SELECTION : root ? 0 : 1, client.x + x, client.y + y, 10, 1);
    text(chosen || !root ? PAL1 : PAL0, label, client.x + x, client.y + y, 10);
}
uint16_t mb_window_chrome(const MB_Event *e) {
    MB_Input in;
    u16 x = mode == MB_WINDOW_NORMAL ? normal_x : 0, y = mode == MB_WINDOW_NORMAL ? normal_y : 0;
    mb_input_state(&in);
    if (mode == MB_WINDOW_FULLSCREEN || in.mode != MB_INPUT_POINTER)
        return 0;
    if (e->action == MB_MOVE && dragging) {
        s16 nx = e->x / 8 - drag_dx, ny = e->y / 8 - drag_dy;
        if (!(in.buttons & MB_KEY_A)) {
            dragging = 0;
            return 0;
        }
        if (nx < 0)
            nx = 0;
        if (nx > 40 - 32)
            nx = 40 - 32;
        if (ny < 1)
            ny = 1;
        if (ny > 28 - 16)
            ny = 28 - 16;
        if (nx != normal_x || ny != normal_y) {
            normal_x = nx;
            normal_y = ny;
            fill(0, 0, 1, 40, 27);
            chrome();
            mb_runtime_notify(MB_RESIZE);
        }
        return 1;
    }
    if (e->action == MB_ACTIVATE && e->y >= y * 8 && e->y < (y + 1) * 8) {
        u16 close_x = mode == MB_WINDOW_NORMAL ? x + 1 : 37;
        if (e->x >= close_x * 8 && e->x < (close_x + 3) * 8) {
            mb_runtime_request_close();
            return 1;
        }
        if (mode == MB_WINDOW_NORMAL) {
            dragging = 1;
            drag_dx = e->x / 8 - x;
            drag_dy = e->y / 8 - y;
            return 1;
        }
    }
    return 0;
}
MB_Result mb_theme_preview(MB_ThemeId id) {
    if (id >= MB_THEME_COUNT)
        return MB_INVALID;
    if (lease_token)
        return MB_BUSY;
    visible_theme = id;
    palette_apply();
    observe();
    mb_runtime_notify(MB_THEME_CHANGED);
    return MB_OK;
}
MB_Result mb_theme_apply(MB_ThemeId id) {
    MB_Result r = mb_theme_preview(id);
    if (r == MB_OK)
        applied_theme = id;
    return r;
}
void mb_theme_cancel_preview(void) {
    if (!lease_token)
        mb_theme_preview(applied_theme);
}
MB_ThemeId mb_theme_current(void) {
    return applied_theme;
}
static u16 lease_valid(const MB_PaletteLease *l) {
    return l && lease_token && l->token == lease_token && mb_runtime_active() &&
           mode == MB_WINDOW_FULLSCREEN;
}
MB_Result mb_palette_request(const MB_PaletteRequest *req, MB_PaletteLease *out) {
    u16 i;
    if (!req || !out || req->lines != 15)
        return MB_INVALID;
    if (lease_token)
        return MB_BUSY;
    if (!mb_runtime_active() || mode != MB_WINDOW_FULLSCREEN)
        return MB_UNAVAILABLE;
    mb_canvas_close();
    lease_serial++;
    if (!lease_serial)
        lease_serial++;
    lease_token = lease_serial;
    leased_sprites = 0;
    upload_left = 0;
    for (i = 0; i < 79; i++)
        sprite_visible[i] = 0;
    for (i = 0; i < 28; i++)
        band_count[i] = band_pixels[i] = 0;
    mb_graphics_observation = (MB_GraphicsStats){0};
    VDP_setBackgroundColor(0);
    observe();
    *out =
        (MB_PaletteLease){lease_token, 15, TILE_BASE + 64, TILE_FONT_INDEX - (TILE_BASE + 64), 79};
    mb_runtime_notify(MB_DISPLAY_LOST);
    return MB_OK;
}
MB_Result mb_palette_set(const MB_PaletteLease *l, uint16_t line, const uint16_t *colors) {
    u16 i;
    if (!lease_valid(l) || line > 3 || !colors)
        return MB_INVALID;
    for (i = 0; i < 16; i++)
        if (colors[i] & ~0xeee)
            return MB_INVALID;
    queue_palette(line, colors);
    return MB_OK;
}
MB_Result mb_palette_release(MB_PaletteLease *l) {
    if (!lease_valid(l))
        return MB_INVALID;
    mb_display_recover();
    l->token = 0;
    chrome();
    mb_runtime_notify(MB_DISPLAY_RESTORED);
    mb_runtime_notify(MB_RESIZE);
    return MB_OK;
}
MB_Result mb_graphics_pattern(const MB_PaletteLease *l, uint16_t tile, const uint32_t *rows) {
    if (!lease_valid(l) || !rows || tile < TILE_BASE + 64 || tile >= TILE_FONT_INDEX)
        return MB_INVALID;
    VDP_loadTileData((const u32 *)rows, tile, 1, CPU);
    return MB_OK;
}
MB_Result mb_graphics_tile(const MB_PaletteLease *l, uint16_t x, uint16_t y, uint16_t tile,
                           uint16_t line) {
    if (!lease_valid(l) || x >= client.width || y >= client.height || line > 3 ||
        tile < TILE_BASE + 64 || tile >= TILE_FONT_INDEX)
        return MB_INVALID;
    cell(client.x + x, client.y + y, TILE_ATTR_FULL(line, 0, 0, 0, tile),
         plane_b[(client.y + y) * 40 + client.x + x]);
    return MB_OK;
}
MB_Result mb_graphics_sprite(const MB_PaletteLease *l, uint16_t index, int16_t x, int16_t y,
                             uint16_t tile, uint16_t line) {
    if (x < 0 || x > 312 || y < 0 || y > 216)
        return MB_INVALID;
    return mb_graphics_sprite_box(l, index, x, y, 1, 1, tile, line);
}
static u16 intersects(s16 y, u16 height, u16 band) {
    return y < (band + 1) * 8 && y + (s16)(height * 8) > band * 8;
}
MB_Result mb_graphics_sprite_box(const MB_PaletteLease *l, uint16_t index, int16_t x, int16_t y,
                                 uint16_t width, uint16_t height, uint16_t tile, uint16_t line) {
    u16 b, oldw = 0, oldh = 0;
    s16 first, last;
    s16 oldy = 0;
    if (!lease_valid(l) || index > leased_sprites || index >= 79 || x < -32 || x > 319 || y < -32 ||
        y > 223 || !width || width > 4 || !height || height > 4 || line > 3 ||
        tile < TILE_BASE + 64 || tile >= TILE_FONT_INDEX || width * height > TILE_FONT_INDEX - tile)
        return MB_INVALID;
    if (sprite_visible[index]) {
        /* Quantized animation often repeats a sprite's complete state. The
         * cache already owns it, including its chain link: avoid revalidating
         * scanline bands and scheduling an identical SAT transfer. Pending
         * changes from other sprites remain queued. Hidden slots still rebuild. */
        if (vdpSpriteCache[index + 1].x == x + 128 && vdpSpriteCache[index + 1].y == y + 128 &&
            vdpSpriteCache[index + 1].size == SPRITE_SIZE(width, height) &&
            vdpSpriteCache[index + 1].attribut == TILE_ATTR_FULL(line, 1, 0, 0, tile))
            return MB_OK;
        oldw = (vdpSpriteCache[index + 1].size >> 2) + 1;
        oldh = (vdpSpriteCache[index + 1].size & 3) + 1;
        oldy = vdpSpriteCache[index + 1].y - 128;
    }
    first = oldh && oldy < y ? oldy : y;
    last = oldh && oldy + oldh * 8 > y + height * 8 ? oldy + oldh * 8 : y + height * 8;
    first = first < 0 ? 0 : first / 8;
    last = last > 224 ? 27 : (last - 1) / 8;
    /* Only touched scanline bands need validation; full-screen scans per moving star
     * would waste the 68000 budget. Distant old/new positions still validate their union. */
    for (b = first; b <= last; b++) {
        u16 old = oldh && intersects(oldy, oldh, b), next = intersects(y, height, b);
        u16 count = band_count[b] - old + next;
        u16 pixels = band_pixels[b] - old * oldw * 8 + next * width * 8;
        if (count > 20 || pixels > 320)
            return MB_BUSY;
    }
    for (b = first; b <= last; b++) {
        u16 old = oldh && intersects(oldy, oldh, b), next = intersects(y, height, b);
        band_count[b] += next - old;
        band_pixels[b] += next * width * 8 - old * oldw * 8;
        if (band_count[b] > mb_graphics_observation.band_peak)
            mb_graphics_observation.band_peak = band_count[b];
    }
    if (index == leased_sprites) {
        if (!index)
            VDP_setSpriteFull(0, 0, -8, SPRITE_SIZE(1, 1), 0, 1);
        else
            VDP_setSpriteLink(index, index + 1);
        leased_sprites++;
    }
    VDP_setSpriteFull(index + 1, x, y, SPRITE_SIZE(width, height),
                      TILE_ATTR_FULL(line, 1, 0, 0, tile),
                      index + 1 < leased_sprites ? index + 2 : 0);
    sprites_pending = 1;
    sprite_visible[index] = 1;
    return MB_OK;
}
MB_Result mb_graphics_hide(const MB_PaletteLease *l, uint16_t index) {
    u16 b, w, h;
    s16 y;
    if (!lease_valid(l) || index >= leased_sprites)
        return MB_INVALID;
    if (sprite_visible[index]) {
        w = (vdpSpriteCache[index + 1].size >> 2) + 1;
        h = (vdpSpriteCache[index + 1].size & 3) + 1;
        y = vdpSpriteCache[index + 1].y - 128;
        for (b = 0; b < 28; b++)
            if (intersects(y, h, b)) {
                band_count[b]--;
                band_pixels[b] -= w * 8;
            }
    }
    sprite_visible[index] = 0;
    VDP_setSpritePosition(index + 1, -32, -32);
    sprites_pending = 1;
    return MB_OK;
}
MB_Result mb_graphics_load(const MB_PaletteLease *l, uint16_t tile, const uint32_t *rows,
                           uint16_t count) {
    if (!lease_valid(l) || !rows || !count || tile < TILE_BASE + 64 || tile >= TILE_FONT_INDEX ||
        count > TILE_FONT_INDEX - tile)
        return MB_INVALID;
    if (upload_left)
        return MB_BUSY;
    upload_rows = (const u32 *)rows;
    upload_tile = tile;
    upload_left = count;
    return MB_OK;
}
MB_Result mb_graphics_ready(const MB_PaletteLease *l) {
    if (!lease_valid(l))
        return MB_INVALID;
    return upload_left ? MB_BUSY : MB_OK;
}
MB_Result mb_graphics_fill(const MB_PaletteLease *l, uint16_t plane, uint16_t x, uint16_t y,
                           uint16_t w, uint16_t h, uint16_t tile, uint16_t line) {
    u16 cx, cy, attr;
    if (!lease_valid(l) || plane > 1 || !w || !h || x >= 40 || y >= 28 || w > 40 - x ||
        h > 28 - y || line > 3 || tile < TILE_BASE + 64 || tile >= TILE_FONT_INDEX)
        return MB_INVALID;
    attr = TILE_ATTR_FULL(line, 0, 0, 0, tile);
    for (cy = y; cy < y + h; cy++)
        for (cx = x; cx < x + w; cx++) {
            u16 n = cy * 40 + cx;
            cell(cx, cy, plane ? plane_a[n] : attr, plane ? attr : plane_b[n]);
        }
    return MB_OK;
}
MB_Result mb_graphics_map(const MB_PaletteLease *l, uint16_t plane, const uint16_t *cells) {
    u16 n;
    if (!lease_valid(l) || plane > 1 || !cells)
        return MB_INVALID;
    for (n = 0; n < CELLS; n++)
        if ((cells[n] & 2047) >= TILE_FONT_INDEX - (TILE_BASE + 64))
            return MB_INVALID;
    for (n = 0; n < CELLS; n++) {
        u16 attr = (cells[n] & 0xf800) | ((cells[n] & 2047) + TILE_BASE + 64);
        cell(n % 40, n / 40, plane ? plane_a[n] : attr, plane ? attr : plane_b[n]);
    }
    return MB_OK;
}
MB_Result mb_graphics_map_region(const MB_PaletteLease *l, uint16_t plane, uint16_t x, uint16_t y,
                                 uint16_t width, uint16_t height, const uint16_t *cells) {
    u16 n, cx, cy;
    if (!lease_valid(l) || !cells || plane > 1 || !width || !height || x >= 40 || y >= 28 ||
        width > 40 - x || height > 28 - y)
        return MB_INVALID;
    /* Validate the whole region before replacing any retained cell. */
    for (n = 0; n < width * height; ++n)
        if ((cells[n] & 2047) >= TILE_FONT_INDEX - (TILE_BASE + 64))
            return MB_INVALID;
    n = 0;
    for (cy = y; cy < y + height; ++cy)
        for (cx = x; cx < x + width; ++cx) {
            u16 value = cells[n++], attr = (value & 0xf800) | ((value & 2047) + TILE_BASE + 64),
                index = cy * 40 + cx;
            cell(cx, cy, plane ? plane_a[index] : attr, plane ? attr : plane_b[index]);
        }
    return MB_OK;
}
MB_Result mb_graphics_stats(MB_GraphicsStats *out) {
    if (!out)
        return MB_INVALID;
    if (!lease_token)
        return MB_UNAVAILABLE;
    *out = mb_graphics_observation;
    out->queue_tiles = upload_left;
    return MB_OK;
}
