/**
 * @file kernel/storage/ram_adapter.c
 * @brief Existing compact disk behind the private VFS interface; no on-volume change.
 *
 * OS service implementation behind the public SDK; apps use megabenchos.h.
 */
#include "backend.h"
#include "ramdisk.h"
static MB_Result capacity(MB_DriveInfo *out) {
    MB_FSInfo f;
    MB_Result r = mb_ram_info(&f);
    if (r != MB_OK)
        return r;
    out->reserved_bytes = f.reserved_bytes;
    out->capacity = f.capacity;
    out->used = f.used;
    out->free = f.free;
    out->max_files = f.max_files;
    out->max_handles = f.max_handles;
    return MB_OK;
}
static MB_Result list(const char *path, uint16_t index, MB_FileInfo *out) {
    if (mb_path_equal(path, "RAM:/")) {
        if (index > 2)
            return MB_NOT_FOUND;
        mb_backend_directory(out,
                             index == 2 ? "Music"
                             : index    ? "Pictures"
                                        : "Documents",
                             index, 0);
        return MB_OK;
    }
    if (mb_path_equal(path, "RAM:/Documents"))
        return mb_ram_list(0, index, out);
    if (mb_path_equal(path, "RAM:/Pictures"))
        return mb_ram_list(1, index, out);
    if (mb_path_equal(path, "RAM:/Music"))
        return mb_ram_list(2, index, out);
    return MB_NOT_FOUND;
}
const MB_Backend mb_ram_backend = {capacity,          list,
                                   mb_ram_file_stat,  mb_ram_file_open,
                                   mb_ram_file_read,  mb_ram_file_write,
                                   mb_ram_file_close, mb_ram_file_rename,
                                   mb_ram_file_delete};
