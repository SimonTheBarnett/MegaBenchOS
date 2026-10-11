/* Isolated 4 KiB littlefs experiment; never an application or release backend. */
#ifndef MB_LFS_EXPERIMENT_H
#define MB_LFS_EXPERIMENT_H
#include "../../third_party/littlefs/lfs.h"
typedef struct {
    uint32_t block, cache, medium_bytes, runtime_bytes, empty_blocks, directory_blocks;
    int32_t format, picture, text_and_picture, maximum_payload, file_count;
    int32_t replacement, old_preserved, io_failure_preserved, close_failure_preserved;
    int32_t interrupted_preserved, corrupt_rejected, bounds_checked, flash_checked;
    uint32_t reads, programs, erases, syncs;
} MB_LfsExperiment;
int mb_lfs_experiment(uint32_t block, uint32_t cache, MB_LfsExperiment *result);
#endif
