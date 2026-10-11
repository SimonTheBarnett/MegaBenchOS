/**
 * @file tools/native/common.c
 * @brief Provides bounded file, JSON, path and SHA256 helpers for Windows host tools.
 *
 * Host-only C99 tooling; excluded from the console ROM.
 */
#include "host.h"
#include <bcrypt.h>
#include <direct.h>
#include <wchar.h>
char mb_root[4096], mb_error[2048];
jmp_buf *mb_trap;
void mb_fail(const char *file, int line, const char *format, ...) {
    va_list args;
    va_start(args, format);
    vsnprintf(mb_error, sizeof mb_error, format, args);
    va_end(args);
    if (mb_trap)
        longjmp(*mb_trap, 1);
    if (Unity.CurrentTestName) {
        Unity.TestFile = file;
        UnityFail(mb_error, (UNITY_LINE_TYPE)line);
    }
    fprintf(stderr, "FAIL %s:%d: %s\n", file, line, mb_error);
    exit(1);
}
void *mb_alloc(size_t bytes) {
    REQUIRE(bytes <= 128 * 1024 * 1024, "Host allocation exceeds 128 MiB");
    void *p = calloc(bytes ? bytes : 1, 1);
    REQUIRE(p, "Host allocation failed");
    return p;
}
char *mb_copy(const char *s) {
    size_t n = strlen(s) + 1;
    char *p = mb_alloc(n);
    memcpy(p, s, n);
    return p;
}
const char *mb_fmt(const char *format, ...) {
    static char buffers[64][4096];
    static unsigned index;
    char *out = buffers[index++ % 64];
    va_list args;
    va_start(args, format);
    int n = vsnprintf(out, 4096, format, args);
    va_end(args);
    REQUIRE(n >= 0 && n < 4096, "Formatted string exceeds limit");
    return out;
}
static wchar_t *wide(const char *s) {
    int n = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, s, -1, NULL, 0);
    REQUIRE(n > 0, "Invalid UTF-8 path");
    wchar_t *p = mb_alloc((size_t)n * sizeof *p);
    REQUIRE(MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, s, -1, p, n),
            "Path conversion failed");
    return p;
}
FILE *mb_fopen(const char *path, const char *mode) {
    wchar_t *p = wide(path), *m = wide(mode);
    FILE *f = _wfopen(p, m);
    free(p);
    free(m);
    return f;
}
bool mb_exists(const char *path) {
    wchar_t *p = wide(path);
    DWORD a = GetFileAttributesW(p);
    free(p);
    return a != INVALID_FILE_ATTRIBUTES;
}
const char *mb_path(const char *relative) {
    return mb_fmt("%s/%s", mb_root, relative);
}
void mb_mkdir(const char *path) {
    wchar_t *p = wide(path);
    for (wchar_t *c = p + 3; *c; c++)
        if (*c == L'/' || *c == L'\\') {
            wchar_t saved = *c;
            *c = 0;
            _wmkdir(p);
            *c = saved;
        }
    _wmkdir(p);
    free(p);
}
unsigned char *mb_read(const char *path, size_t *size) {
    FILE *f = mb_fopen(path, "rb");
    REQUIRE(f, "Cannot read %s", path);
    REQUIRE(fseek(f, 0, SEEK_END) == 0, "Cannot seek %s", path);
    long n = ftell(f);
    REQUIRE(n >= 0 && n <= 128 * 1024 * 1024, "File exceeds host input limit: %s", path);
    rewind(f);
    unsigned char *b = mb_alloc((size_t)n + 1);
    REQUIRE(fread(b, 1, (size_t)n, f) == (size_t)n, "Cannot read all of %s", path);
    REQUIRE(fclose(f) == 0, "Read close failed");
    *size = (size_t)n;
    return b;
}
void mb_write(const char *path, const void *data, size_t size) {
    char *parent = mb_copy(path);
    char *slash = strrchr(parent, '/'), *back = strrchr(parent, '\\');
    if (!slash || (back && back > slash))
        slash = back;
    if (slash) {
        *slash = 0;
        mb_mkdir(parent);
    }
    free(parent);
    FILE *f = mb_fopen(path, "wb");
    REQUIRE(f, "Cannot write %s", path);
    REQUIRE(fwrite(data, 1, size, f) == size && fclose(f) == 0, "Cannot write all of %s", path);
}
char *mb_text(const char *path) {
    size_t n;
    return (char *)mb_read(path, &n);
}
void mb_paths_add(Paths *p, const char *relative) {
    if (p->count == p->capacity) {
        p->capacity = p->capacity ? p->capacity * 2 : 128;
        p->items = realloc(p->items, p->capacity * sizeof *p->items);
        REQUIRE(p->items, "Path list allocation failed");
    }
    p->items[p->count++] = mb_copy(relative);
}
void mb_scan(Paths *p, const char *relative, bool recursive) {
    wchar_t *pattern = wide(mb_fmt("%s/*", mb_path(relative)));
    WIN32_FIND_DATAW entry;
    HANDLE h = FindFirstFileW(pattern, &entry);
    free(pattern);
    REQUIRE(h != INVALID_HANDLE_VALUE, "Cannot scan %s", relative);
    do {
        if (entry.cFileName[0] == L'.' &&
            (entry.cFileName[1] == 0 || (entry.cFileName[1] == L'.' && entry.cFileName[2] == 0)))
            continue;
        char name[1024];
        REQUIRE(
            WideCharToMultiByte(CP_UTF8, 0, entry.cFileName, -1, name, sizeof name, NULL, NULL) > 0,
            "Filename exceeds limit");
        char *next = mb_copy(mb_fmt("%s/%s", relative, name));
        if (entry.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            REQUIRE(!(entry.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT),
                    "Source scan refuses reparse points: %s", next);
            if (recursive)
                mb_scan(p, next, true);
        } else
            mb_paths_add(p, next);
        free(next);
    } while (FindNextFileW(h, &entry));
    DWORD error = GetLastError();
    FindClose(h);
    REQUIRE(error == ERROR_NO_MORE_FILES, "Directory enumeration failed");
}
static int path_compare(const void *a, const void *b) {
    return strcmp(*(char *const *)a, *(char *const *)b);
}
void mb_paths_sort(Paths *p) {
    qsort(p->items, p->count, sizeof *p->items, path_compare);
}
Hash mb_hash(const void *data, size_t size) {
    BCRYPT_ALG_HANDLE algorithm;
    BCRYPT_HASH_HANDLE hash;
    unsigned char bytes[32] = {0};
    Hash out;
    REQUIRE(size <= UINT32_MAX, "Hash input too large");
    REQUIRE(BCryptOpenAlgorithmProvider(&algorithm, BCRYPT_SHA256_ALGORITHM, NULL, 0) == 0,
            "SHA256 provider failed");
    REQUIRE(BCryptCreateHash(algorithm, &hash, NULL, 0, NULL, 0, 0) == 0, "SHA256 setup failed");
    REQUIRE(BCryptHashData(hash, (PUCHAR)data, (ULONG)size, 0) == 0 &&
                BCryptFinishHash(hash, bytes, 32, 0) == 0,
            "SHA256 calculation failed");
    BCryptDestroyHash(hash);
    BCryptCloseAlgorithmProvider(algorithm, 0);
    for (int i = 0; i < 32; i++)
        sprintf(out.text + i * 2, "%02x", bytes[i]);
    return out;
}
Hash mb_hash_file(const char *path) {
    /* Raw audio captures can exceed a gigabyte; hash a bounded stream. */
    FILE *file = mb_fopen(path, "rb");
    REQUIRE(file, "Cannot read %s", path);
    BCRYPT_ALG_HANDLE algorithm;
    BCRYPT_HASH_HANDLE hash;
    unsigned char buffer[65536], digest[32] = {0};
    REQUIRE(BCryptOpenAlgorithmProvider(&algorithm, BCRYPT_SHA256_ALGORITHM, NULL, 0) == 0,
            "SHA256 provider failed");
    REQUIRE(BCryptCreateHash(algorithm, &hash, NULL, 0, NULL, 0, 0) == 0, "SHA256 setup failed");
    size_t size;
    while ((size = fread(buffer, 1, sizeof buffer, file)) != 0)
        REQUIRE(BCryptHashData(hash, buffer, (ULONG)size, 0) == 0, "SHA256 stream failed");
    bool good = !ferror(file);
    good &= fclose(file) == 0;
    REQUIRE(good && BCryptFinishHash(hash, digest, 32, 0) == 0, "SHA256 file read failed");
    BCryptDestroyHash(hash);
    BCryptCloseAlgorithmProvider(algorithm, 0);
    Hash result;
    for (int i = 0; i < 32; i++)
        sprintf(result.text + i * 2, "%02x", digest[i]);
    return result;
}
bool hash_equal(Hash a, Hash b) {
    return strcmp(a.text, b.text) == 0;
}
cJSON *json_read(const char *path) {
    size_t n;
    char *text = (char *)mb_read(path, &n);
    REQUIRE(n <= 16 * 1024 * 1024, "JSON exceeds 16 MiB");
    char *start = text;
    if (n >= 3 && (unsigned char)text[0] == 239 && (unsigned char)text[1] == 187 &&
        (unsigned char)text[2] == 191)
        start += 3;
    cJSON *json = cJSON_ParseWithOpts(start, NULL, true);
    free(text);
    REQUIRE(json, "Invalid JSON: %s", path);
    return json;
}
cJSON *json_get(const cJSON *o, const char *key) {
    cJSON *v = cJSON_GetObjectItemCaseSensitive(o, key);
    REQUIRE(v, "Missing JSON field %s", key);
    return v;
}
const char *json_string(const cJSON *v) {
    REQUIRE(cJSON_IsString(v), "Expected JSON string");
    return v->valuestring;
}
const char *json_s(const cJSON *o, const char *key) {
    return json_string(json_get(o, key));
}
double json_number(const cJSON *v) {
    REQUIRE(cJSON_IsNumber(v), "Expected JSON number");
    return v->valuedouble;
}
int json_i(const cJSON *o, const char *key) {
    double n = json_number(json_get(o, key));
    REQUIRE(n >= INT32_MIN && n <= INT32_MAX && floor(n) == n, "Expected bounded integer: %s", key);
    return (int)n;
}
void json_write(const char *path, const cJSON *v) {
    char *text = cJSON_PrintUnformatted(v);
    REQUIRE(text, "JSON serialization failed");
    size_t n = strlen(text);
    char *out = mb_alloc(n + 2);
    memcpy(out, text, n);
    out[n] = '\n';
    mb_write(path, out, n + 1);
    free(out);
    cJSON_free(text);
}
void json_set(cJSON *o, const char *key, cJSON *v) {
    REQUIRE(v, "JSON allocation failed");
    if (cJSON_HasObjectItem(o, key))
        cJSON_ReplaceItemInObjectCaseSensitive(o, key, v);
    else
        cJSON_AddItemToObject(o, key, v);
}
void json_str(cJSON *o, const char *key, const char *v) {
    json_set(o, key, cJSON_CreateString(v));
}
void json_num(cJSON *o, const char *key, double v) {
    json_set(o, key, cJSON_CreateNumber(v));
}
cJSON *mb_config(void) {
    cJSON *c = json_read(mb_path(mb_exists(mb_path("config/toolchain.local.json"))
                                     ? "config/toolchain.local.json"
                                     : "config/toolchain.example.json"));
    const char *keys[] = {"sgdk", "blastem", "retroarch", "genesis_plus_gx_core"},
               *vars[] = {"MB_SGDK", "MB_BLASTEM", "MB_RETROARCH", "MB_GENPLUS_CORE"};
    for (int i = 0; i < 4; i++) {
        const char *v = getenv(vars[i]);
        if (v && *v)
            json_str(c, keys[i], v);
    }
    return c;
}
uint32_t mb_be32(const unsigned char *p) {
    return (uint32_t)p[0] << 24 | (uint32_t)p[1] << 16 | (uint32_t)p[2] << 8 | p[3];
}
uint32_t mb_le32(const unsigned char *p) {
    return (uint32_t)p[3] << 24 | (uint32_t)p[2] << 16 | (uint32_t)p[1] << 8 | p[0];
}
uint16_t mb_le16(const unsigned char *p) {
    return (uint16_t)(p[0] | p[1] << 8);
}
void mb_put_be32(unsigned char *p, uint32_t v) {
    for (int i = 0; i < 4; i++)
        p[i] = (unsigned char)(v >> (24 - i * 8));
}
void mb_put_le32(unsigned char *p, uint32_t v) {
    for (int i = 0; i < 4; i++)
        p[i] = (unsigned char)(v >> (i * 8));
}
void mb_timestamp(char out[32]) {
    SYSTEMTIME time;
    GetSystemTime(&time);
    snprintf(out, 32, "%04u-%02u-%02uT%02u:%02u:%02uZ", time.wYear, time.wMonth, time.wDay,
             time.wHour, time.wMinute, time.wSecond);
}
void mb_publish(const char *relative, const char *text, bool check) {
    /* Embedded API type declarations may contain Windows line endings too. */
    char *normalized = mb_copy(text);
    size_t length = 0;
    for (size_t i = 0; text[i]; i++)
        if (text[i] != '\r')
            normalized[length++] = text[i];
    normalized[length] = 0;
    const char *path = mb_path(relative);
    bool exists = mb_exists(path), equal = false;
    if (exists) {
        size_t n;
        unsigned char *old = mb_read(path, &n); /* Git may check text out with CRLF. */
        size_t j = 0;
        for (size_t i = 0; i < n; i++)
            if (old[i] != '\r')
                old[j++] = old[i];
        old[j] = 0;
        equal = strcmp((char *)old, normalized) == 0;
        free(old);
    }
    if (check)
        REQUIRE(equal, "Generated drift: %s", relative);
    else if (!equal)
        mb_write(path, normalized, length);
    free(normalized);
}
int mb_round(double v) {
    return (int)nearbyint(v);
}
