/* SGDK-only test configuration. Utility helpers below derived from littlefs
 * v2.11.3 lfs_util.h; BSD-3-Clause, copyright 2022 littlefs authors,
 * copyright 2017 Arm Limited. See third_party/littlefs/LICENSE.md.
 * Uses upstream LFS_CONFIG extension; vendored sources remain unmodified. */
#ifndef MB_LFS_SGDK_H
#define MB_LFS_SGDK_H
#include <genesis.h>
#define LFS_TRACE(...)
#define LFS_DEBUG(...)
#define LFS_WARN(...)
#define LFS_ERROR(...)
#define LFS_ASSERT(...)
static inline int mb_lfs_compare(const void *a, const void *b, size_t n) {
    const u8 *x = a, *y = b;
    while (n--) {
        if (*x != *y)
            return *x - *y;
        ++x;
        ++y;
    }
    return 0;
}
static inline char *mb_lfs_chr(const char *s, int c) {
    do {
        if (*s == c)
            return (char *)s;
    } while (*s++);
    return NULL;
}
static inline size_t mb_lfs_span(const char *s, const char *reject) {
    size_t n = 0;
    while (s[n] && !mb_lfs_chr(reject, s[n]))
        ++n;
    return n;
}
#define memcmp mb_lfs_compare
#define strchr mb_lfs_chr
#define strcspn mb_lfs_span
static inline size_t mb_lfs_accept(const char *s, const char *accept) {
    size_t n = 0;
    while (s[n] && mb_lfs_chr(accept, s[n]))
        ++n;
    return n;
}
#define strspn mb_lfs_accept
// Builtin functions, these may be replaced by more efficient
// toolchain-specific implementations. LFS_NO_INTRINSICS falls back to a more
// expensive basic C implementation for debugging purposes

// Min/max functions for unsigned 32-bit numbers
static inline uint32_t lfs_max(uint32_t a, uint32_t b) {
    return (a > b) ? a : b;
}

static inline uint32_t lfs_min(uint32_t a, uint32_t b) {
    return (a < b) ? a : b;
}

// Align to nearest multiple of a size
static inline uint32_t lfs_aligndown(uint32_t a, uint32_t alignment) {
    return a - (a % alignment);
}

static inline uint32_t lfs_alignup(uint32_t a, uint32_t alignment) {
    return lfs_aligndown(a + alignment - 1, alignment);
}

// Find the smallest power of 2 greater than or equal to a
static inline uint32_t lfs_npw2(uint32_t a) {
#if !defined(LFS_NO_INTRINSICS) && (defined(__GNUC__) || defined(__CC_ARM))
    return 32 - __builtin_clz(a - 1);
#else
    uint32_t r = 0;
    uint32_t s;
    a -= 1;
    s = (a > 0xffff) << 4;
    a >>= s;
    r |= s;
    s = (a > 0xff) << 3;
    a >>= s;
    r |= s;
    s = (a > 0xf) << 2;
    a >>= s;
    r |= s;
    s = (a > 0x3) << 1;
    a >>= s;
    r |= s;
    return (r | (a >> 1)) + 1;
#endif
}

// Count the number of trailing binary zeros in a
// lfs_ctz(0) may be undefined
static inline uint32_t lfs_ctz(uint32_t a) {
#if !defined(LFS_NO_INTRINSICS) && defined(__GNUC__)
    return __builtin_ctz(a);
#else
    return lfs_npw2((a & -a) + 1) - 1;
#endif
}

// Count the number of binary ones in a
static inline uint32_t lfs_popc(uint32_t a) {
#if !defined(LFS_NO_INTRINSICS) && (defined(__GNUC__) || defined(__CC_ARM))
    return __builtin_popcount(a);
#else
    a = a - ((a >> 1) & 0x55555555);
    a = (a & 0x33333333) + ((a >> 2) & 0x33333333);
    return (((a + (a >> 4)) & 0xf0f0f0f) * 0x1010101) >> 24;
#endif
}

// Find the sequence comparison of a and b, this is the distance
// between a and b ignoring overflow
static inline int lfs_scmp(uint32_t a, uint32_t b) {
    return (int)(unsigned)(a - b);
}

// Convert between 32-bit little-endian and native order
static inline uint32_t lfs_fromle32(uint32_t a) {
#if (defined(BYTE_ORDER) && defined(ORDER_LITTLE_ENDIAN) && BYTE_ORDER == ORDER_LITTLE_ENDIAN) ||  \
    (defined(__BYTE_ORDER) && defined(__ORDER_LITTLE_ENDIAN) &&                                    \
     __BYTE_ORDER == __ORDER_LITTLE_ENDIAN) ||                                                     \
    (defined(__BYTE_ORDER__) && defined(__ORDER_LITTLE_ENDIAN__) &&                                \
     __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
    return a;
#elif !defined(LFS_NO_INTRINSICS) &&                                                               \
    ((defined(BYTE_ORDER) && defined(ORDER_BIG_ENDIAN) && BYTE_ORDER == ORDER_BIG_ENDIAN) ||       \
     (defined(__BYTE_ORDER) && defined(__ORDER_BIG_ENDIAN) &&                                      \
      __BYTE_ORDER == __ORDER_BIG_ENDIAN) ||                                                       \
     (defined(__BYTE_ORDER__) && defined(__ORDER_BIG_ENDIAN__) &&                                  \
      __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__))
    return __builtin_bswap32(a);
#else
    return ((uint32_t)((uint8_t *)&a)[0] << 0) | ((uint32_t)((uint8_t *)&a)[1] << 8) |
           ((uint32_t)((uint8_t *)&a)[2] << 16) | ((uint32_t)((uint8_t *)&a)[3] << 24);
#endif
}

static inline uint32_t lfs_tole32(uint32_t a) {
    return lfs_fromle32(a);
}

// Convert between 32-bit big-endian and native order
static inline uint32_t lfs_frombe32(uint32_t a) {
#if !defined(LFS_NO_INTRINSICS) &&                                                                 \
    ((defined(BYTE_ORDER) && defined(ORDER_LITTLE_ENDIAN) && BYTE_ORDER == ORDER_LITTLE_ENDIAN) || \
     (defined(__BYTE_ORDER) && defined(__ORDER_LITTLE_ENDIAN) &&                                   \
      __BYTE_ORDER == __ORDER_LITTLE_ENDIAN) ||                                                    \
     (defined(__BYTE_ORDER__) && defined(__ORDER_LITTLE_ENDIAN__) &&                               \
      __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__))
    return __builtin_bswap32(a);
#elif (defined(BYTE_ORDER) && defined(ORDER_BIG_ENDIAN) && BYTE_ORDER == ORDER_BIG_ENDIAN) ||      \
    (defined(__BYTE_ORDER) && defined(__ORDER_BIG_ENDIAN) &&                                       \
     __BYTE_ORDER == __ORDER_BIG_ENDIAN) ||                                                        \
    (defined(__BYTE_ORDER__) && defined(__ORDER_BIG_ENDIAN__) &&                                   \
     __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
    return a;
#else
    return ((uint32_t)((uint8_t *)&a)[0] << 24) | ((uint32_t)((uint8_t *)&a)[1] << 16) |
           ((uint32_t)((uint8_t *)&a)[2] << 8) | ((uint32_t)((uint8_t *)&a)[3] << 0);
#endif
}

static inline uint32_t lfs_tobe32(uint32_t a) {
    return lfs_frombe32(a);
}

// Calculate CRC-32 with polynomial = 0x04c11db7
#ifdef LFS_CRC
static inline uint32_t lfs_crc(uint32_t crc, const void *buffer, size_t size) {
    return LFS_CRC(crc, buffer, size);
}
#else
uint32_t lfs_crc(uint32_t crc, const void *buffer, size_t size);
#endif

// Allocate memory, only used if buffers are not provided to littlefs
//
// littlefs current has no alignment requirements, as it only allocates
// byte-level buffers.
static inline void *lfs_malloc(size_t size) {
#if defined(LFS_MALLOC)
    return LFS_MALLOC(size);
#elif !defined(LFS_NO_MALLOC)
    return malloc(size);
#else
    (void)size;
    return NULL;
#endif
}

// Deallocate memory, only used if buffers are not provided to littlefs
static inline void lfs_free(void *p) {
#if defined(LFS_FREE)
    LFS_FREE(p);
#elif !defined(LFS_NO_MALLOC)
    free(p);
#else
    (void)p;
#endif
}

#endif
