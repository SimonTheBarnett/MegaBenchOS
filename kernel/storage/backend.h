/* Private mount interface. Whole-file write success means committed content;
 * all reported errors must preserve an existing file. No app may call these hooks. */
#ifndef MB_STORAGE_BACKEND_H
#define MB_STORAGE_BACKEND_H
#include "megabenchos.h"
typedef struct {
    MB_Result (*capacity)(MB_DriveInfo *info);
    MB_Result (*list)(const char *directory, uint16_t index, MB_FileInfo *info);
    MB_Result (*stat)(const char *path, MB_FileInfo *info);
    MB_Result (*open)(const char *path, uint16_t flags, uint16_t *handle);
    MB_Result (*read)(uint16_t handle, uint16_t offset, uint8_t *data, uint16_t capacity,
                      uint16_t *actual);
    MB_Result (*write)(uint16_t handle, const uint8_t *data, uint16_t length);
    MB_Result (*close)(uint16_t handle);
    MB_Result (*rename)(const char *path, const char *new_path);
    MB_Result (*remove)(const char *path);
} MB_Backend;
extern const MB_Backend mb_ram_backend, mb_rom_backend;
uint16_t mb_path_equal(const char *a, const char *b);
void mb_backend_directory(MB_FileInfo *info, const char *name, uint16_t id, uint16_t flags);
#endif
