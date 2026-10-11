/**
 * @file kernel/storage/ramdisk.c
 * @brief Compact session-only RAM volume. No heap/SGDK dependency; exact same C is host tested.
 *
 * OS service implementation behind the public SDK; apps use megabenchos.h.
 * Replacement preflights every failure before moving bytes, so old files survive errors.
 */
#include "ramdisk.h"
#define FILES 16
#define HANDLES 6
#define PAYLOAD 3592
typedef struct {
    char name[20];
    uint16_t offset, length, generation;
    uint8_t dir, used;
} File;
typedef struct {
    uint16_t token, file, generation, flags;
} Handle;
typedef struct {
    uint16_t magic, used, serial, generation;
    File files[FILES];
    Handle handles[HANDLES];
    uint8_t data[PAYLOAD];
} Disk;
static Disk disk;
typedef char disk_is_four_kib[(sizeof(Disk) == 4096) ? 1 : -1];
static uint8_t lower(uint8_t c) {
    return c >= 'A' && c <= 'Z' ? c + 32 : c;
}
static uint16_t equal(const char *a, const char *b) {
    while (*a && lower(*a) == lower(*b)) {
        ++a;
        ++b;
    }
    return !*a && !*b;
}
static void copy_name(char *to, const char *from) {
    uint16_t i;
    for (i = 0; i < 20; i++) {
        to[i] = from[i];
        if (!from[i])
            break;
    }
}
static uint16_t valid_extension(uint16_t dir, const char *ext) {
    if (dir == 2)
        return equal(ext, ".vgm") || equal(ext, ".xgc");
    return equal(ext, dir ? ".mbp" : ".txt");
}
static MB_Result path_parse(const char *p, uint16_t *dir, char *name) {
    const char *prefix;
    uint16_t i, n = 0;
    if (!p)
        return MB_INVALID;
    prefix = "RAM:/";
    for (i = 0; prefix[i]; i++)
        if (lower(p[i]) != lower(prefix[i]))
            return MB_INVALID;
    p += i;
    if (lower(*p) == 'd') {
        prefix = "Documents/";
        *dir = 0;
    } else if (lower(*p) == 'm') {
        prefix = "Music/";
        *dir = 2;
    } else {
        prefix = "Pictures/";
        *dir = 1;
    }
    for (i = 0; prefix[i]; i++)
        if (lower(p[i]) != lower(prefix[i]))
            return MB_INVALID;
    p += i;
    while (p[n] && p[n] != '.') {
        uint8_t c = p[n];
        if (n == 15 || !((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
                         (c >= '0' && c <= '9') || c == '_' || c == '-'))
            return MB_INVALID;
        name[n] = c;
        ++n;
    }
    if (!n || !valid_extension(*dir, p + n))
        return MB_INVALID;
    for (i = 0; i < 5; i++) {
        name[n + i] = p[n + i];
        if (!p[n + i])
            break;
    }
    return MB_OK;
}
static MB_Result validate(void) {
    uint16_t i, j, sum = 0;
    if (disk.magic != 0x4d42 || disk.used > PAYLOAD)
        return MB_CORRUPT;
    for (i = 0; i < FILES; i++)
        if (disk.files[i].used) {
            File *f = &disk.files[i];
            uint16_t end = f->offset + f->length;
            if (f->used != 1 || f->dir > 2 || f->length > PAYLOAD || f->offset > disk.used ||
                end > disk.used || end < f->offset || !f->name[0] || f->name[19])
                return MB_CORRUPT;
            {
                uint16_t k = 0;
                while (f->name[k] && f->name[k] != '.') {
                    uint8_t c = f->name[k];
                    if (k == 15 || !((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
                                     (c >= '0' && c <= '9') || c == '_' || c == '-'))
                        return MB_CORRUPT;
                    ++k;
                }
                if (!k || !valid_extension(f->dir, f->name + k))
                    return MB_CORRUPT;
            }
            sum += f->length;
            for (j = 0; j < i; j++)
                if (disk.files[j].used) {
                    File *g = &disk.files[j];
                    if ((f->dir == g->dir && equal(f->name, g->name)) ||
                        (f->length && g->length && f->offset < g->offset + g->length &&
                         g->offset < end))
                        return MB_CORRUPT;
                }
        }
    return sum == disk.used ? MB_OK : MB_CORRUPT;
}
void mb_ram_close_handles(void) {
    uint16_t i;
    for (i = 0; i < HANDLES; i++)
        disk.handles[i].token = 0;
}
void mb_ram_reset(void) {
    uint16_t i, serial = disk.magic == 0x4d42 ? disk.serial : 0;
    disk.magic = 0x4d42;
    disk.used = 0;
    disk.serial = serial;
    disk.generation = 0;
    for (i = 0; i < FILES; i++)
        disk.files[i].used = 0;
    mb_ram_close_handles();
}
static int16_t find(uint16_t dir, const char *name) {
    uint16_t i;
    for (i = 0; i < FILES; i++)
        if (disk.files[i].used && disk.files[i].dir == dir && equal(disk.files[i].name, name))
            return i;
    return -1;
}
static void info_copy(File *f, MB_FileInfo *out) {
    out->version = 1;
    out->directory = f->dir;
    out->size = f->length;
    out->flags = MB_FILE_REGULAR;
    copy_name(out->name, f->name);
}
MB_Result mb_ram_info(MB_FSInfo *out) {
    uint16_t i, n = 0;
    MB_Result r;
    if (!out)
        return MB_INVALID;
    if ((r = validate()) != MB_OK)
        return r;
    for (i = 0; i < FILES; i++)
        n += disk.files[i].used != 0;
    out->version = 1;
    out->reserved_bytes = sizeof disk;
    out->capacity = PAYLOAD;
    out->used = disk.used;
    out->free = PAYLOAD - disk.used;
    out->files = n;
    out->max_files = FILES;
    out->max_handles = HANDLES;
    return MB_OK;
}
MB_Result mb_ram_list(uint16_t dir, uint16_t index, MB_FileInfo *out) {
    uint16_t i;
    MB_Result r;
    if (dir > 2 || !out)
        return MB_INVALID;
    if ((r = validate()) != MB_OK)
        return r;
    for (i = 0; i < FILES; i++)
        if (disk.files[i].used && disk.files[i].dir == dir && !index--) {
            info_copy(&disk.files[i], out);
            return MB_OK;
        }
    return MB_NOT_FOUND;
}
static MB_Result locate(const char *p, int16_t *index) {
    char name[20] = {0};
    uint16_t dir;
    MB_Result r;
    if ((r = validate()) != MB_OK || (r = path_parse(p, &dir, name)) != MB_OK)
        return r;
    *index = find(dir, name);
    return *index < 0 ? MB_NOT_FOUND : MB_OK;
}
MB_Result mb_ram_file_stat(const char *p, MB_FileInfo *out) {
    int16_t i;
    MB_Result r;
    if (!out)
        return MB_INVALID;
    if ((r = locate(p, &i)) == MB_OK)
        info_copy(&disk.files[i], out);
    return r;
}
MB_Result mb_ram_file_open(const char *p, uint16_t flags, MB_FileHandle *out) {
    char name[20] = {0};
    uint16_t dir, i, h = HANDLES;
    int16_t f;
    MB_Result r;
    if (!out || !(flags & 3) || (flags & ~7) || ((flags & 4) && !(flags & 2)))
        return MB_INVALID;
    if ((r = validate()) != MB_OK || (r = path_parse(p, &dir, name)) != MB_OK)
        return r;
    f = find(dir, name);
    for (i = 0; i < HANDLES; i++) {
        Handle *a = &disk.handles[i];
        if (!a->token)
            h = i;
        else if (f >= 0 && a->file == f && ((flags | a->flags) & MB_FILE_WRITE))
            return MB_BUSY;
    }
    if (h == HANDLES || disk.serial == 65535)
        return MB_LIMIT;
    if (f < 0) {
        if (!(flags & MB_FILE_CREATE))
            return MB_NOT_FOUND;
        for (i = 0; i < FILES; i++)
            if (!disk.files[i].used)
                break;
        if (i == FILES)
            return MB_LIMIT;
        f = i;
        copy_name(disk.files[f].name, name);
        disk.files[f].dir = dir;
        disk.files[f].offset = disk.used;
        disk.files[f].length = 0;
        disk.files[f].generation = ++disk.generation;
        disk.files[f].used = 1;
    }
    disk.handles[h].token = ++disk.serial;
    disk.handles[h].file = f;
    disk.handles[h].generation = disk.files[f].generation;
    disk.handles[h].flags = flags;
    *out = disk.handles[h].token;
    return MB_OK;
}
static Handle *handle_get(uint16_t token) {
    uint16_t i;
    if (!token)
        return 0;
    for (i = 0; i < HANDLES; i++) {
        Handle *h = &disk.handles[i];
        if (h->token == token && h->file < FILES && disk.files[h->file].used &&
            h->generation == disk.files[h->file].generation)
            return h;
    }
    return 0;
}
MB_Result mb_ram_file_read(MB_FileHandle token, uint16_t offset, uint8_t *data, uint16_t capacity,
                           uint16_t *actual) {
    Handle *h;
    File *f;
    uint16_t i, n;
    MB_Result r;
    if (!actual || (!data && capacity))
        return MB_INVALID;
    if ((r = validate()) != MB_OK)
        return r;
    h = handle_get(token);
    if (!h || !(h->flags & MB_FILE_READ))
        return MB_INVALID;
    f = &disk.files[h->file];
    if (offset > f->length)
        return MB_INVALID;
    n = f->length - offset;
    if (n > capacity)
        n = capacity;
    for (i = 0; i < n; i++)
        data[i] = disk.data[f->offset + offset + i];
    *actual = n;
    return MB_OK;
}
/* Remove a payload range, including zero-byte files at the end, without allocation. */
static void remove_data(File *f) {
    uint16_t i, end = f->offset + f->length, n = f->length;
    for (i = end; i < disk.used; i++)
        disk.data[i - n] = disk.data[i];
    for (i = 0; i < FILES; i++)
        if (&disk.files[i] != f && disk.files[i].used && disk.files[i].offset >= end)
            disk.files[i].offset -= n;
    disk.used -= n;
}
MB_Result mb_ram_file_write(MB_FileHandle token, const uint8_t *data, uint16_t length) {
    Handle *h;
    File *f;
    uint16_t i;
    MB_Result r;
    if (!data && length)
        return MB_INVALID;
    if ((r = validate()) != MB_OK)
        return r;
    h = handle_get(token);
    if (!h || !(h->flags & MB_FILE_WRITE))
        return MB_INVALID;
    f = &disk.files[h->file];
    if (length > PAYLOAD || length > PAYLOAD - (disk.used - f->length))
        return MB_NO_SPACE;
    remove_data(f);
    f->offset = disk.used;
    for (i = 0; i < length; i++)
        disk.data[disk.used + i] = data[i];
    f->length = length;
    disk.used += length;
    return MB_OK;
}
MB_Result mb_ram_file_close(MB_FileHandle token) {
    Handle *h = handle_get(token);
    if (!h)
        return MB_INVALID;
    h->token = 0;
    return MB_OK;
}
static uint16_t opened(uint16_t f) {
    uint16_t i;
    for (i = 0; i < HANDLES; i++)
        if (disk.handles[i].token && disk.handles[i].file == f)
            return 1;
    return 0;
}
MB_Result mb_ram_file_delete(const char *p) {
    int16_t i;
    MB_Result r;
    if ((r = locate(p, &i)) != MB_OK)
        return r;
    if (opened(i))
        return MB_BUSY;
    remove_data(&disk.files[i]);
    disk.files[i].used = 0;
    return MB_OK;
}
MB_Result mb_ram_file_rename(const char *p, const char *newp) {
    char name[20] = {0};
    uint16_t dir;
    int16_t i, j;
    MB_Result r;
    if ((r = locate(p, &i)) != MB_OK || (r = path_parse(newp, &dir, name)) != MB_OK)
        return r;
    if (dir != disk.files[i].dir)
        return MB_INVALID;
    if (opened(i))
        return MB_BUSY;
    j = find(dir, name);
    if (j >= 0 && j != i)
        return MB_EXISTS;
    copy_name(disk.files[i].name, name);
    return MB_OK;
}
#ifdef MB_HOST_TEST
void mb_fs_test_corrupt(void) {
    disk.used = PAYLOAD + 1;
}
#endif
