/* Private session-storage lifecycle; public file operations live in the SDK. */
#ifndef MB_RAMDISK_H
#define MB_RAMDISK_H
#include "megabenchos.h"
void mb_ram_reset(void);
void mb_ram_close_handles(void);
MB_Result mb_ram_info(MB_FSInfo *info);
MB_Result mb_ram_list(uint16_t directory, uint16_t index, MB_FileInfo *info);
MB_Result mb_ram_file_stat(const char *path, MB_FileInfo *info);
MB_Result mb_ram_file_open(const char *path, uint16_t flags, MB_FileHandle *handle);
MB_Result mb_ram_file_read(MB_FileHandle handle, uint16_t offset, uint8_t *data, uint16_t capacity,
                           uint16_t *actual);
MB_Result mb_ram_file_write(MB_FileHandle handle, const uint8_t *data, uint16_t length);
MB_Result mb_ram_file_close(MB_FileHandle handle);
MB_Result mb_ram_file_rename(const char *path, const char *new_path);
MB_Result mb_ram_file_delete(const char *path);
#ifdef MB_HOST_TEST
void mb_fs_test_corrupt(void);
#endif
#endif
