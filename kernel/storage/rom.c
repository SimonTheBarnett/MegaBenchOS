/**
 * @file kernel/storage/rom.c
 * @brief Immutable ROM resource table, separate from cartridge capacity metadata.
 *
 * OS service implementation behind the public SDK; apps use megabenchos.h.
 * No writes, heap, flash-cart assumptions or exposed ROM pointers.
 */
#include "backend.h"
#ifndef MB_HOST_TEST
#include <genesis.h>
#endif
typedef struct {
    const char *path, *name;
    const uint8_t *data;
    uint16_t size, directory;
} Resource;
static const uint8_t readme[] =
    "MegaBenchOS ROM resource\nRead-only bundled files.\nRAM files reset on boot.\n";
static const uint8_t palette[] =
    "Logical colors: face, dark, accent, light.\nShared themes own the hardware palette.\n";
static const Resource resources[] = {
    {"ROM:/Help/readme.txt", "readme.txt", readme, sizeof readme - 1, 2},
    {"ROM:/Resources/palette.txt", "palette.txt", palette, sizeof palette - 1, 3}};
static void file_info(uint16_t index, MB_FileInfo *out) {
    const Resource *r = &resources[index];
    uint16_t i = 0;
    out->version = 1;
    out->directory = r->directory;
    out->size = r->size;
    out->flags = MB_FILE_REGULAR | MB_FILE_READ_ONLY;
    do {
        out->name[i] = r->name[i];
    } while (r->name[i++]);
}
static MB_Result capacity(MB_DriveInfo *out) {
    uint16_t i;
    out->used = 0;
    for (i = 0; i < 2; i++)
        out->used += resources[i].size;
    out->capacity = out->used;
    out->free = 0;
    out->max_files = 2;
    out->max_handles = 6;
#ifdef MB_HOST_TEST
    out->reserved_bytes = 393216; /* Host fixture only; target reads SGDK's linked ROM_SIZE. */
#else
    out->reserved_bytes = ROM_SIZE;
#endif
    return MB_OK;
}
static MB_Result list(const char *path, uint16_t index, MB_FileInfo *out) {
    uint16_t dir, i;
    if (mb_path_equal(path, "ROM:/")) {
        if (index > 1)
            return MB_NOT_FOUND;
        mb_backend_directory(out, index ? "Resources" : "Help", index ? 3 : 2, MB_FILE_READ_ONLY);
        return MB_OK;
    }
    if (mb_path_equal(path, "ROM:/Help"))
        dir = 2;
    else if (mb_path_equal(path, "ROM:/Resources"))
        dir = 3;
    else
        return MB_NOT_FOUND;
    for (i = 0; i < 2; i++)
        if (resources[i].directory == dir && !index--) {
            file_info(i, out);
            return MB_OK;
        }
    return MB_NOT_FOUND;
}
static MB_Result stat(const char *path, MB_FileInfo *out) {
    uint16_t i;
    for (i = 0; i < 2; i++)
        if (mb_path_equal(path, resources[i].path)) {
            file_info(i, out);
            return MB_OK;
        }
    return MB_NOT_FOUND;
}
static MB_Result open(const char *path, uint16_t flags, uint16_t *handle) {
    uint16_t i;
    if (flags != MB_FILE_READ)
        return MB_READ_ONLY;
    for (i = 0; i < 2; i++)
        if (mb_path_equal(path, resources[i].path)) {
            *handle = i + 1;
            return MB_OK;
        }
    return MB_NOT_FOUND;
}
static MB_Result read(uint16_t handle, uint16_t offset, uint8_t *data, uint16_t size,
                      uint16_t *actual) {
    const Resource *r;
    uint16_t i, n;
    if (!handle || handle > 2 || !actual || (!data && size))
        return MB_INVALID;
    r = &resources[handle - 1];
    if (offset > r->size)
        return MB_INVALID;
    n = r->size - offset;
    if (n > size)
        n = size;
    for (i = 0; i < n; i++)
        data[i] = r->data[offset + i];
    *actual = n;
    return MB_OK;
}
static MB_Result close(uint16_t handle) {
    return handle && handle <= 2 ? MB_OK : MB_INVALID;
}
static MB_Result write(uint16_t handle, const uint8_t *data, uint16_t size) {
    (void)handle;
    (void)data;
    (void)size;
    return MB_READ_ONLY;
}
static MB_Result rename(const char *path, const char *new_path) {
    (void)path;
    (void)new_path;
    return MB_READ_ONLY;
}
static MB_Result remove(const char *path) {
    (void)path;
    return MB_READ_ONLY;
}
const MB_Backend mb_rom_backend = {capacity, list, stat, open, read, write, close, rename, remove};
