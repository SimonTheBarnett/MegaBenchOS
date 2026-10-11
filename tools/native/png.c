/**
 * @file tools/native/png.c
 * @brief Writes captured RGB frames as PNG evidence using lossless stored DEFLATE blocks.
 *
 * Host-only C99 tooling; excluded from the console ROM.
 */
#include "core.h"
static uint32_t crc(const unsigned char *data, size_t size) {
    uint32_t c = UINT32_MAX;
    for (size_t n = 0; n < size; n++) {
        c ^= data[n];
        for (int bit = 0; bit < 8; bit++)
            c = (c >> 1) ^ ((c & 1) ? 0xedb88320u : 0);
    }
    return c ^ UINT32_MAX;
}
static void chunk(unsigned char *out, size_t *at, const char *type, const void *bytes,
                  size_t size) {
    REQUIRE(size <= UINT32_MAX, "PNG chunk too large");
    mb_put_be32(out + *at, (uint32_t)size);
    memcpy(out + *at + 4, type, 4);
    memcpy(out + *at + 8, bytes, size);
    mb_put_be32(out + *at + 8 + size, crc(out + *at + 4, size + 4));
    *at += size + 12;
}
void png_write(const char *path, int width, int height, const unsigned char *rgb) {
    REQUIRE(width > 0 && height > 0 && width <= 4096 && height <= 4096,
            "PNG dimensions out of bounds");
    size_t size = (size_t)height * (width * 3 + 1);
    unsigned char *rows = mb_alloc(size);
    for (int y = 0; y < height; y++)
        memcpy(rows + (size_t)y * (width * 3 + 1) + 1, rgb + (size_t)y * width * 3,
               (size_t)width * 3);
    /* PNG's zlib stream uses lossless stored DEFLATE blocks: no compression dependency. */
    size_t capacity = size + (size / 65535 + 1) * 5 + 6;
    unsigned char *z = mb_alloc(capacity);
    z[0] = 0x78;
    z[1] = 0x01;
    size_t at = 2;
    for (size_t n = 0; n < size;) {
        size_t length = size - n;
        if (length > 65535)
            length = 65535;
        z[at++] = (unsigned char)(n + length == size);
        z[at++] = (unsigned char)length;
        z[at++] = (unsigned char)(length >> 8);
        z[at++] = (unsigned char)~length;
        z[at++] = (unsigned char)(~length >> 8);
        memcpy(z + at, rows + n, length);
        at += length;
        n += length;
    }
    uint32_t a = 1, b = 0;
    for (size_t n = 0; n < size; n++) {
        a = (a + rows[n]) % 65521;
        b = (b + a) % 65521;
    }
    mb_put_be32(z + at, b << 16 | a);
    at += 4;
    unsigned char *png = mb_alloc(at + 57), header[13] = {0};
    memcpy(png, "\x89PNG\r\n\x1a\n", 8);
    mb_put_be32(header, (uint32_t)width);
    mb_put_be32(header + 4, (uint32_t)height);
    header[8] = 8;
    header[9] = 2;
    size_t used = 8;
    chunk(png, &used, "IHDR", header, 13);
    chunk(png, &used, "IDAT", z, at);
    chunk(png, &used, "IEND", z, 0);
    mb_write(path, png, used);
    free(rows);
    free(z);
    free(png);
}
