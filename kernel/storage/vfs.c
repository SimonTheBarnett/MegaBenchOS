/**
 * @file kernel/storage/vfs.c
 * @brief One bounded SDK VFS: prefix mount dispatch, opaque global handles and compatibility
 *
 * OS service implementation behind the public SDK; apps use megabenchos.h.
 * adapters. Filesystem drivers alone own storage representations and raw resources.
 */
#include "backend.h"
#include "ramdisk.h"
typedef struct {
    const char *name, *backend;
    uint16_t capabilities;
    const MB_Backend *ops;
} Mount;
static const Mount mounts[] = {
    {"RAM:", "Compact RAM",
     MB_FS_MOUNTED | MB_FS_WRITABLE | MB_FS_VOLATILE | MB_FS_DIRECTORIES | MB_FS_OFFSET_READ |
         MB_FS_RENAME | MB_FS_SPACE_EXACT | MB_FS_ATOMIC_REPLACE,
     &mb_ram_backend},
    {"ROM:", "ROM resources",
     MB_FS_MOUNTED | MB_FS_READ_ONLY | MB_FS_BUNDLED | MB_FS_DIRECTORIES | MB_FS_OFFSET_READ,
     &mb_rom_backend},
    {"SAVE:", "Unavailable", 0, 0},
    {"SD:", "Unavailable", 0, 0}};
typedef struct {
    uint16_t token, local, mount, flags;
} OpenFile;
static OpenFile handles[6];
static uint16_t serial;
static uint8_t lower(uint8_t c) {
    return c >= 'A' && c <= 'Z' ? c + 32 : c;
}
uint16_t mb_path_equal(const char *a, const char *b) {
    while (*a && lower(*a) == lower(*b)) {
        ++a;
        ++b;
    }
    return !*a && !*b;
}
void mb_backend_directory(MB_FileInfo *out, const char *name, uint16_t id, uint16_t flags) {
    uint16_t i = 0;
    out->version = 1;
    out->directory = id;
    out->size = 0;
    out->flags = MB_FILE_DIRECTORY | flags;
    do {
        out->name[i] = name[i];
    } while (name[i++]);
}
static MB_Result drive(const char *name, uint16_t *id) {
    uint16_t i;
    if (!name)
        return MB_INVALID;
    for (i = 0; i < 4; i++)
        if (mb_path_equal(name, mounts[i].name)) {
            *id = i;
            return mounts[i].ops ? MB_OK : MB_NOT_SUPPORTED;
        }
    return MB_NOT_FOUND;
}
/* Normalize only drive spelling and optional directory trailing slash. Deliberately
 * reject traversal/double separators rather than resolving them into another file. */
static MB_Result path(const char *input, char *out, uint16_t *mount, uint16_t directory) {
    char prefix[6];
    uint16_t i = 0, j, n = 0, component = 0;
    MB_Result r;
    if (!input)
        return MB_INVALID;
    while (input[i] && input[i] != ':' && i < 4) {
        prefix[i] = input[i];
        ++i;
    }
    if (!i || input[i] != ':')
        return MB_INVALID;
    prefix[i++] = ':';
    prefix[i] = 0;
    if ((r = drive(prefix, mount)) != MB_OK)
        return r;
    if (input[i] != '/')
        return MB_INVALID;
    for (j = 0; j < i; j++)
        out[j] = mounts[*mount].name[j];
    out[i++] = '/';
    while (input[i]) {
        uint8_t c = input[i];
        if (i >= 63 || c < 32 || c > 126 || c == '\\')
            return MB_INVALID;
        if (c == '/') {
            if (!component)
                return MB_INVALID;
            component = 0;
        } else {
            if ((c == '.' && (input[i + 1] == '/' || !input[i + 1])) ||
                (c == '.' && input[i + 1] == '.' && (input[i + 2] == '/' || !input[i + 2])))
                return MB_INVALID;
            ++component;
        }
        out[i] = c;
        ++i;
        ++n;
    }
    if (n && !component) {
        if (!directory)
            return MB_INVALID;
        --i;
    }
    out[i] = 0;
    return MB_OK;
}
MB_Result mb_fs_drive(uint16_t index, uint16_t version, MB_DriveInfo *out) {
    const Mount *m;
    MB_Result r = MB_OK;
    if (!out)
        return MB_INVALID;
    if (version != 1)
        return MB_VERSION;
    if (index >= 4)
        return MB_NOT_FOUND;
    m = &mounts[index];
    out->version = 1;
    out->id = index;
    out->capabilities = m->capabilities;
    out->name = m->name;
    out->backend = m->backend;
    out->reserved_bytes = out->capacity = out->used = out->free = 0;
    out->max_files = out->max_handles = 0;
    if (m->ops)
        r = m->ops->capacity(out);
    return r;
}
MB_Result mb_fs_capacity(const char *name, uint16_t version, MB_DriveInfo *out) {
    uint16_t id;
    MB_Result r;
    if (!out)
        return MB_INVALID;
    if (version != 1)
        return MB_VERSION;
    if ((r = drive(name, &id)) != MB_OK)
        return r;
    return mb_fs_drive(id, version, out);
}
MB_Result mb_fs_info(MB_FSInfo *out) {
    MB_DriveInfo d;
    MB_Result r;
    if (!out)
        return MB_INVALID;
    if ((r = mb_fs_capacity("RAM:", 1, &d)) != MB_OK)
        return r;
    /* Compatibility preserves total file count in the original RAM-only descriptor. */
    return mb_ram_info(out);
}
MB_Result mb_dir_list_path(const char *p, uint16_t index, MB_FileInfo *out) {
    char normalized[64];
    uint16_t id;
    MB_Result r;
    if (!out)
        return MB_INVALID;
    if ((r = path(p, normalized, &id, 1)) != MB_OK)
        return r;
    return mounts[id].ops->list(normalized, index, out);
}
MB_Result mb_dir_list(uint16_t dir, uint16_t index, MB_FileInfo *out) {
    if (dir > 1)
        return MB_INVALID;
    return mb_dir_list_path(dir ? "RAM:/Pictures/" : "RAM:/Documents/", index, out);
}
MB_Result mb_file_stat(const char *p, MB_FileInfo *out) {
    char normalized[64];
    uint16_t id;
    MB_Result r;
    if (!out)
        return MB_INVALID;
    if ((r = path(p, normalized, &id, 0)) != MB_OK)
        return r;
    return mounts[id].ops->stat(normalized, out);
}
static OpenFile *get(uint16_t token) {
    uint16_t i;
    if (!token)
        return 0;
    for (i = 0; i < 6; i++)
        if (handles[i].token == token)
            return &handles[i];
    return 0;
}
MB_Result mb_file_open(const char *p, uint16_t flags, MB_FileHandle *out) {
    char normalized[64];
    uint16_t id, i, local;
    MB_Result r;
    if (!out || !(flags & 3) || (flags & ~7) || ((flags & 4) && !(flags & 2)))
        return MB_INVALID;
    if ((r = path(p, normalized, &id, 0)) != MB_OK)
        return r;
    if ((flags & MB_FILE_WRITE) && !(mounts[id].capabilities & MB_FS_WRITABLE))
        return MB_READ_ONLY;
    for (i = 0; i < 6 && handles[i].token;
         i++) { /* Find a free global handle before creating a file. */
    }
    if (i == 6 || serial == 65535)
        return MB_LIMIT;
    if ((r = mounts[id].ops->open(normalized, flags, &local)) != MB_OK)
        return r;
    handles[i] = (OpenFile){++serial, local, id, flags};
    *out = serial;
    return MB_OK;
}
MB_Result mb_file_read(MB_FileHandle token, uint16_t offset, uint8_t *data, uint16_t size,
                       uint16_t *actual) {
    OpenFile *h = get(token);
    if (!h || !(h->flags & MB_FILE_READ) || !actual || (!data && size))
        return MB_INVALID;
    return mounts[h->mount].ops->read(h->local, offset, data, size, actual);
}
MB_Result mb_file_write(MB_FileHandle token, const uint8_t *data, uint16_t size) {
    OpenFile *h = get(token);
    if (!h || (!data && size))
        return MB_INVALID;
    if (!(mounts[h->mount].capabilities & MB_FS_WRITABLE))
        return MB_READ_ONLY;
    if (!(h->flags & MB_FILE_WRITE))
        return MB_INVALID;
    return mounts[h->mount].ops->write(h->local, data, size);
}
MB_Result mb_file_close(MB_FileHandle token) {
    OpenFile *h = get(token);
    MB_Result r;
    if (!h)
        return MB_INVALID;
    r = mounts[h->mount].ops->close(h->local);
    h->token = 0;
    return r;
}
void mb_fs_close_handles(void) {
    uint16_t i;
    for (i = 0; i < 6; i++)
        if (handles[i].token)
            mb_file_close(handles[i].token);
    mb_ram_close_handles();
}
void mb_fs_reset(void) {
    mb_fs_close_handles();
    mb_ram_reset();
}
MB_Result mb_file_delete(const char *p) {
    char normalized[64];
    uint16_t id;
    MB_Result r;
    if ((r = path(p, normalized, &id, 0)) != MB_OK)
        return r;
    if (!(mounts[id].capabilities & MB_FS_WRITABLE))
        return MB_READ_ONLY;
    return mounts[id].ops->remove(normalized);
}
MB_Result mb_file_rename(const char *p, const char *newp) {
    char from[64], to[64];
    uint16_t a, b;
    MB_Result r;
    if ((r = path(p, from, &a, 0)) != MB_OK || (r = path(newp, to, &b, 0)) != MB_OK)
        return r;
    if (a != b)
        return MB_NOT_SUPPORTED;
    if (!(mounts[a].capabilities & MB_FS_RENAME))
        return MB_READ_ONLY;
    return mounts[a].ops->rename(from, to);
}
