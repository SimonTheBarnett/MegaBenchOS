/* Sparse, lossless picture model. Uniform 8x8 tiles cost only a map word.
 * Mixed tiles use a bounded pool; no full pixel framebuffer or heap allocation. */
#ifndef MB_PAINT_MODEL_H
#define MB_PAINT_MODEL_H
#include "megabenchos.h"
#define PAINT_MAX_W 512
#define PAINT_MAX_H 256
#define PAINT_MAP 2048
#define PAINT_POOL 384
#define PAINT_UNIFORM 0x4000
#define PAINT_VISITED 0x8000
#define PAINT_FILE_LIMIT 3592
typedef struct {
    uint16_t width, height, used, palette[16];
    uint16_t map[PAINT_MAP];
    uint8_t tiles[PAINT_POOL][32], allocated[PAINT_POOL];
} Picture;
typedef struct {
    uint8_t masks[PAINT_POOL][8], pending[PAINT_MAP / 8];
    uint16_t old_color, new_color, scan, remaining, phase, commit, seed;
} PaintFill;
MB_Result picture_new(Picture *p, uint16_t width, uint16_t height);
uint8_t picture_pixel(const void *p, uint16_t x, uint16_t y);
void picture_tile(const void *p, uint16_t x, uint16_t y, uint32_t *rows);
MB_Result picture_brush(Picture *p, uint16_t x, uint16_t y, uint16_t size, uint16_t color);
/* Centred square stamp, clipped at edges. On success copies pixel damage bounds.
 * Failure preserves picture and output; caller owns both, no allocation. */
MB_Result picture_stamp(Picture *p, uint16_t cx, uint16_t cy, uint16_t size, uint16_t color,
                        MB_Rect *damage);
void picture_clear(Picture *p, uint16_t color);
MB_Result picture_encode(const Picture *p, uint8_t *out, uint16_t capacity, uint16_t *length);
MB_Result picture_decode(Picture *p, const uint8_t *bytes, uint16_t length, uint8_t *band);
MB_Result picture_fill_begin(Picture *p, PaintFill *f, uint16_t x, uint16_t y, uint16_t color);
/* Process at most budget tiles; phase 0 means finished. No recursive flood-fill stack. */
void picture_fill_step(Picture *p, PaintFill *f, uint16_t budget);
void picture_fill_cancel(Picture *p, PaintFill *f);
#endif
