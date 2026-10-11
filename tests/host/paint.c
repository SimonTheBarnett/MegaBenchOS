/**
 * @file tests/host/paint.c
 * @brief Tests sparse pictures, bounded fill, brush footprints and lossless MBP saves with Unity.
 *
 * Host Unity fixture linked against production C code; no emulator or hardware dependency.
 */
#include "unity.h"
/* Exact production sparse picture tests and representation benchmark metrics. */
#include "../../apps/creative/paint_model.h"
#include "../../apps/creative/model.h"
static Picture a, b;
static PaintFill fill;
static uint8_t encoded[PAINT_FILE_LIMIT], legacy[1040];
static uint32_t metrics[8];
uint32_t mb_paint_metric(uint16_t index) {
    return index < 8 ? metrics[index] : 0;
}
#define CHECK(x)                                                                                   \
    do {                                                                                           \
        if (!(x))                                                                                  \
            UnityFail(#x, __LINE__);                                                               \
        ++checks;                                                                                  \
    } while (0)
int mb_paint_tests(void) {
    int checks = 0;
    uint16_t x, y, n, i;
    uint32_t steps;
    {
        MB_Rect damage;
        uint16_t size, edge;
        for (size = 1; size <= 8; size *= 2)
            for (edge = 0; edge < 4; ++edge) {
                uint16_t cx = edge == 1   ? 0
                              : edge == 2 ? 15
                                          : 8,
                         cy = edge == 1   ? 0
                              : edge == 2 ? 15
                              : edge == 3 ? 0
                                          : 8;
                int16_t left = (int16_t)cx - size / 2, top = (int16_t)cy - size / 2;
                CHECK(picture_new(&a, 16, 16) == MB_OK);
                CHECK(picture_stamp(&a, cx, cy, size, 7, &damage) == MB_OK);
                for (y = 0; y < 16; ++y)
                    for (x = 0; x < 16; ++x)
                        CHECK(picture_pixel(&a, x, y) ==
                              (x >= left && x < left + size && y >= top && y < top + size ? 7 : 0));
                CHECK(damage.x == (left < 0 ? 0 : left) && damage.y == (top < 0 ? 0 : top) &&
                      damage.width == (left + size > 16 ? 16 : left + size) - damage.x &&
                      damage.height == (top + size > 16 ? 16 : top + size) - damage.y);
                CHECK(picture_stamp(&a, cx, cy, size, 0, &damage) == MB_OK && a.used == 0);
            }
        CHECK(picture_stamp(&a, 0, 0, 0, 1, &damage) == MB_INVALID &&
              picture_stamp(&a, 0, 0, 9, 1, &damage) == MB_INVALID &&
              picture_stamp(&a, 0, 0, 1, 1, 0) == MB_INVALID);
    }
    metrics[0] = sizeof(Picture);
    metrics[1] = sizeof(PaintFill);
    CHECK(picture_new(&a, 320, 208) == MB_OK && a.used == 0);
    CHECK(picture_new(&b, 319, 208) == MB_INVALID);
    CHECK(picture_encode(&a, encoded, sizeof encoded, &n) == MB_OK);
    metrics[2] = n;
    CHECK(picture_brush(&a, 319, 207, 8, 15) == MB_OK && picture_pixel(&a, 319, 207) == 15);
    CHECK(picture_brush(&a, 200, 140, 4, 7) == MB_OK);
    for (y = 140; y < 144; ++y)
        for (x = 200; x < 204; ++x)
            CHECK(picture_pixel(&a, x, y) == 7);
    CHECK(picture_brush(&a, 0, 0, 2, 3) == MB_OK);
    CHECK(picture_encode(&a, encoded, sizeof encoded, &n) == MB_OK);
    metrics[3] = n;
    CHECK(picture_decode(&b, encoded, n, fill.masks[0]) == MB_OK && b.width == 320 &&
          b.height == 208);
    for (y = 0; y < a.height; ++y)
        for (x = 0; x < a.width; ++x)
            CHECK(picture_pixel(&a, x, y) == picture_pixel(&b, x, y));
    encoded[12] ^= 1;
    CHECK(picture_decode(&b, encoded, n, fill.masks[0]) == MB_CORRUPT &&
          picture_pixel(&b, 319, 207) == 15);
    encoded[12] ^= 1;
    CHECK(picture_decode(&b, encoded, n - 1, fill.masks[0]) == MB_CORRUPT);
    encoded[17] ^= 2;
    CHECK(picture_decode(&b, encoded, n, fill.masks[0]) ==
          MB_CORRUPT); /* legal palette corruption */
    encoded[17] ^= 2;
    encoded[16] = 1;
    CHECK(picture_decode(&b, encoded, n, fill.masks[0]) == MB_CORRUPT);
    encoded[16] = 0x0e;
    paint_new(legacy);
    paint_set(legacy, 63, 63, 3);
    paint_set(legacy, 0, 0, 1);
    paint_encode(legacy);
    CHECK(picture_decode(&b, legacy, sizeof legacy, fill.masks[0]) == MB_OK && b.width == 64 &&
          picture_pixel(&b, 63, 63) == 3 && picture_pixel(&b, 0, 0) == 1);
    CHECK(picture_new(&a, 512, 256) == MB_OK);
    CHECK(picture_brush(&a, 511, 255, 1, 12) == MB_OK);
    CHECK(picture_encode(&a, encoded, sizeof encoded, &n) == MB_OK);
    metrics[4] = n;
    CHECK(picture_decode(&b, encoded, n, fill.masks[0]) == MB_OK &&
          picture_pixel(&b, 511, 255) == 12);
    /* A compact valid file can still require more mixed tiles than the runtime pool.
     * Build a read-only repeated-tile fixture; decoder must refuse it before commit. */
    CHECK(picture_new(&a, 512, 64) == MB_OK);
    CHECK(picture_brush(&a, 0, 0, 1, 1) == MB_OK);
    for (i = 1; i < 512; ++i)
        a.map[i] = a.map[0];
    CHECK(picture_encode(&a, encoded, sizeof encoded, &n) == MB_OK);
    CHECK(picture_decode(&b, encoded, n, fill.masks[0]) == MB_NO_SPACE && b.height == 256 &&
          picture_pixel(&b, 511, 255) == 12);
    CHECK(picture_new(&a, 512, 256) == MB_OK);
    /* 384 unrelated mixed tiles: the next stamp fails without modifying either tile. */
    picture_clear(&a, 0);
    for (i = 0; i < PAINT_POOL; ++i)
        CHECK(picture_brush(&a, (i % 64) * 8, (i / 64) * 8, 1, 1) == MB_OK);
    CHECK(a.used == PAINT_POOL);
    metrics[5] = a.used;
    CHECK(picture_brush(&a, 0, 48, 8, 2) == MB_OK &&
          a.used == PAINT_POOL); /* uniform needs no pool */
    CHECK(picture_brush(&a, 8, 48, 2, 2) == MB_NO_SPACE && picture_pixel(&a, 8, 48) == 0);
    CHECK(picture_brush(&a, 0, 0, 8, 0) == MB_OK && a.used == PAINT_POOL - 1);
    CHECK(picture_brush(&a, 8, 48, 2, 2) == MB_OK);
    CHECK(picture_encode(&a, encoded, 50, &n) == MB_NO_SPACE);
    /* Fill an enclosed nonuniform region, leaving its other component untouched. */
    picture_new(&a, 320, 176);
    for (x = 0; x < 320; ++x)
        CHECK(picture_brush(&a, x, 80, 1, 1) == MB_OK);
    CHECK(picture_fill_begin(&a, &fill, 0, 0, 6) == MB_OK);
    for (steps = 0; fill.phase && steps < 10000; ++steps)
        picture_fill_step(&a, &fill, 8);
    CHECK(!fill.phase);
    metrics[6] = steps;
    for (y = 0; y < 176; ++y)
        for (x = 0; x < 320; ++x)
            CHECK(picture_pixel(&a, x, y) == (y < 80 ? 6 : y == 80 ? 1 : 0));
    CHECK(picture_fill_begin(&a, &fill, 0, 81, 4) == MB_OK);
    picture_fill_step(&a, &fill, 2);
    picture_fill_cancel(&a, &fill);
    CHECK(!fill.phase && picture_pixel(&a, 0, 81) == 0);
    CHECK(picture_fill_begin(&a, &fill, 0, 81, 4) == MB_OK);
    for (steps = 0; fill.phase && steps < 10000; ++steps)
        picture_fill_step(&a, &fill, 8);
    CHECK(!fill.phase && picture_pixel(&a, 319, 175) == 4);
    CHECK(picture_encode(&a, encoded, sizeof encoded, &n) == MB_OK);
    metrics[7] = n;
    return checks;
}
