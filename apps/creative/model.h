/* SDK-only bounded editor and MBP1 models; also compiled by native host tests. */
#ifndef MB_CREATIVE_MODEL_H
#define MB_CREATIVE_MODEL_H
#include "megabenchos.h"
#define MB_PAD_LIMIT 1024
#define MB_PAINT_BYTES 1040
typedef struct {
    char text[1025];
    uint16_t length, cursor, dirty, scroll;
} Pad;
void pad_reset(Pad *p);
MB_Result pad_edit(Pad *p, const MB_TextInput *input);
void pad_move(Pad *p, int16_t x, int16_t y, uint16_t columns);
uint16_t pad_line(const Pad *p, uint16_t start, uint16_t columns);
void paint_new(uint8_t *doc);
uint8_t paint_get(const uint8_t *doc, uint16_t x, uint16_t y);
void paint_set(uint8_t *doc, uint16_t x, uint16_t y, uint16_t color);
void paint_encode(uint8_t *doc);
MB_Result paint_validate(const uint8_t *doc, uint16_t length);
#endif
