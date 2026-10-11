/**
 * @file tools/native/host.h
 * @brief Shared contracts for the native Windows command-line tools.
 *
 * Paths and text are UTF-8. A failed REQUIRE records mb_error and fails the
 * active Unity fixture, jumps to an explicitly installed rejection trap, or
 * exits with status 1. These interfaces are private host tooling, not the SDK.
 */
#ifndef MB_HOST_TOOLS_H
#define MB_HOST_TOOLS_H
/* Host-only C99 utilities. None of these files are linked into the ROM. */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <math.h>
#include <setjmp.h>
#include "cJSON.h"
#include "unity.h"
typedef struct {
    char text[65];
} Hash;
typedef struct {
    char **items;
    size_t count, capacity;
} Paths;
extern char mb_root[4096];
extern char mb_error[2048];
extern jmp_buf *mb_trap;
__declspec(noreturn) void mb_fail(const char *file, int line, const char *format, ...);
#define REQUIRE(ok, ...)                                                                           \
    do {                                                                                           \
        if (!(ok))                                                                                 \
            mb_fail(__FILE__, __LINE__, __VA_ARGS__);                                              \
    } while (0)
/** Return zeroed caller-owned memory; free it. Reject allocations above 128 MiB. */
void *mb_alloc(size_t bytes);
/** Copy a NUL-terminated string into caller-owned memory. */
char *mb_copy(const char *s);
/** Borrow one of 64 temporary 4096-byte buffers; copy strings kept across calls. */
const char *mb_fmt(const char *format, ...);
/** Open a UTF-8 path using the Windows wide API; caller closes; NULL on failure. */
FILE *mb_fopen(const char *path, const char *mode);
bool mb_exists(const char *path);
void mb_mkdir(const char *path);
const char *mb_path(const char *relative);
/** Read at most 128 MiB. Return caller-owned bytes plus a trailing NUL; set size. */
unsigned char *mb_read(const char *path, size_t *size);
/** Write borrowed bytes, creating parent directories; report all I/O failures. */
void mb_write(const char *path, const void *data, size_t size);
/** Return a caller-owned, NUL-terminated file buffer with the mb_read bound. */
char *mb_text(const char *path);
/** Append caller-owned relative paths; reject directory reparse points. */
void mb_scan(Paths *paths, const char *relative, bool recursive);
void mb_paths_add(Paths *paths, const char *relative);
void mb_paths_sort(Paths *paths);
Hash mb_hash(const void *data, size_t size);
/** Hash a stream with a fixed 64 KiB buffer, including large raw audio captures. */
Hash mb_hash_file(const char *path);
bool hash_equal(Hash a, Hash b);
/** Parse at most 16 MiB of JSON. Caller releases the tree with cJSON_Delete. */
cJSON *json_read(const char *path);
/** Borrow a required field; fail for missing keys. Typed getters validate types. */
cJSON *json_get(const cJSON *object, const char *key);
const char *json_string(const cJSON *value);
const char *json_s(const cJSON *object, const char *key);
double json_number(const cJSON *value);
int json_i(const cJSON *object, const char *key);
void json_write(const char *path, const cJSON *value);
/** Transfer ownership of value to object, replacing and deleting any old field. */
void json_set(cJSON *object, const char *key, cJSON *value);
void json_str(cJSON *object, const char *key, const char *value);
void json_num(cJSON *object, const char *key, double value);
/** Read tool configuration with process environment overrides; caller deletes. */
cJSON *mb_config(void);
uint32_t mb_be32(const unsigned char *p);
uint32_t mb_le32(const unsigned char *p);
uint16_t mb_le16(const unsigned char *p);
void mb_put_be32(unsigned char *p, uint32_t v);
void mb_put_le32(unsigned char *p, uint32_t v);
/** Return caller-owned ROM metadata after checking bounds, vectors and checksum. */
cJSON *rom_info(const char *path);
/** Validate borrowed bytes without mutation; return caller-owned JSON metadata. */
cJSON *rom_bytes(const unsigned char *bytes, size_t size);
/** Patch header checksum/end in a private copy; validate before writing output. */
void rom_publish(const char *source, const char *destination);
/** Fingerprint build inputs for the selected variant; no input files are modified. */
Hash source_hash(bool contracts, bool littlefs, bool music_smoke);
/** Validate borrowed Mega Drive VGM bytes; return duration in 44100 Hz samples. */
uint64_t vgm_validate(const unsigned char *bytes, size_t size);
void music_check(void);
void vendor_check(const char *name);
/** Validate/generate SDK and UI data. NULL paths select project defaults.
 * Check mode compares outputs without writing; malformed input fails the command.
 */
void sdk_generate(const char *api, const char *assets, bool check);
void demos_generate(bool check);
void host_checks(void);
void littlefs_check(const char *path);
void audio_check(void);
void audio_report(const char *region, const char *raw, const char *frames);
/** Run selected real-core fixtures. Return 0 for success or explicit missing-core
 * SKIPPED, 1 for failure. Both-region runs isolate each case with a 240 s timeout.
 */
int emulator_main(const char *scenario, const char *region, int cycles);
/** Execute borrowed argv without a shell. Return the exit code; a timeout fails
 * and kills the job's process tree. Optional log captures stdout/stderr to a file.
 */
int mb_process(const char *exe, const char *const *args, size_t argc, unsigned timeout,
               const char *log);
void mb_timestamp(char output[32]);
/** Compare/publish borrowed generated text at a project-relative path, normalizing
 * CRLF. Check mode fails on drift; matching outputs retain their timestamps.
 */
void mb_publish(const char *path, const char *text, bool check);
int mb_round(double number);
#endif
