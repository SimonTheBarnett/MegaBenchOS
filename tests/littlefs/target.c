/**
 * @file tests/littlefs/target.c
 * @brief Privileged SGDK experiment ROM: exact host adapter/library, region frame timings.
 *
 * Optional test-only code, excluded from the release ROM.
 * Separate from the release VFS and existing apps. Exposes read-only test mailboxes.
 */
#include <genesis.h>
#include "experiment.h"
#include "internal.h"
volatile uint16_t mb_lfs_target_result __attribute__((externally_visible));
volatile MB_LfsExperiment mb_lfs_target_rows[4] __attribute__((externally_visible));
volatile uint32_t mb_lfs_target_frames[4] __attribute__((externally_visible));
void mb_littlefs_target(void) {
    MB_LfsExperiment row;
    uint32_t start;
    uint16_t b, c, n = 0;
    int r;
    mb_lfs_target_result = 1;
    for (b = 128; b <= 256; b *= 2)
        for (c = 16; c <= 32; c *= 2) {
            start = vtimer;
            r = mb_lfs_experiment(b, c, &row);
            mb_lfs_target_frames[n] = vtimer - start;
            mb_lfs_target_rows[n] = row;
            if (r || !row.bounds_checked || !row.flash_checked || !row.old_preserved ||
                !row.io_failure_preserved || !row.close_failure_preserved ||
                !row.interrupted_preserved || !row.corrupt_rejected) {
                mb_lfs_target_result = 2 + n;
                return;
            }
            ++n;
        }
    mb_services_sample();
    mb_desktop_draw();
    mb_lfs_target_result = 0x600d;
}
