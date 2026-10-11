/**
 * @file tests/littlefs/host.c
 * @brief Separate native C executable; stdout is machine-readable JSON for PowerShell.
 *
 * Optional test-only code, excluded from the release ROM.
 */
#include "experiment.h"
#include <stdio.h>
int main(void) {
    uint32_t b, c;
    MB_LfsExperiment r;
    int first = 1;
    puts("[");
    for (b = 128; b <= 256; b *= 2)
        for (c = 16; c <= 32; c *= 2) {
            if (mb_lfs_experiment(b, c, &r))
                return 2;
            printf("%s{\"block\":%u,\"cache\":%u,\"medium_bytes\":%u,\"runtime_bytes\":%u,\"empty_"
                   "blocks\":%u,\"directory_blocks\":%u,\"format\":%d,\"picture\":%d,\"text_and_"
                   "picture\":%d,\"maximum_payload\":%d,\"file_count\":%d,\"replacement\":%d,\"old_"
                   "preserved\":%d,\"io_failure_preserved\":%d,\"close_failure_preserved\":%d,"
                   "\"interrupted_preserved\":%d,\"corrupt_rejected\":%d,\"bounds_checked\":%d,"
                   "\"flash_checked\":%d,\"reads\":%u,\"programs\":%u,\"erases\":%u,\"syncs\":%u}",
                   first ? "" : ",\n", r.block, r.cache, r.medium_bytes, r.runtime_bytes,
                   r.empty_blocks, r.directory_blocks, r.format, r.picture, r.text_and_picture,
                   r.maximum_payload, r.file_count, r.replacement, r.old_preserved,
                   r.io_failure_preserved, r.close_failure_preserved, r.interrupted_preserved,
                   r.corrupt_rejected, r.bounds_checked, r.flash_checked, r.reads, r.programs,
                   r.erases, r.syncs);
            first = 0;
        }
    puts("\n]");
    return 0;
}
