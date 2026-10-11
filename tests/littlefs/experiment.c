/**
 * @file tests/littlefs/experiment.c
 * @brief Realistic RAM flash adapter and bounded littlefs capacity/fault experiment.
 *
 * Optional test-only code, excluded from the release ROM.
 * The volume is exactly 4096 bytes; library state/caches are reported separately.
 * SDK apps never include this test-only module or littlefs headers.
 */
#include "experiment.h"
typedef struct {
    uint8_t medium[4096];
    lfs_t fs;
    lfs_file_t file;
    struct lfs_config config;
    struct lfs_file_config file_config;
    uint8_t read_cache[32], prog_cache[32], file_cache[32], lookahead[8];
    int fault_program, fault_sync;
    uint32_t reads, programs, erases, syncs;
} Experiment;
static Experiment device;
/* Benchmark input/readback and power-loss snapshots are harness-only buffers. */
static uint8_t data[3592], readback[3592], snapshot[4096];
static int range(const struct lfs_config *c, lfs_block_t b, lfs_off_t o, lfs_size_t n) {
    return b < c->block_count && o <= c->block_size && n <= c->block_size - o;
}
static int read_block(const struct lfs_config *c, lfs_block_t b, lfs_off_t o, void *p,
                      lfs_size_t n) {
    Experiment *d = c->context;
    if (!p || !range(c, b, o, n) || o % c->read_size || n % c->read_size)
        return LFS_ERR_INVAL;
    memcpy(p, d->medium + b * c->block_size + o, n);
    ++d->reads;
    return 0;
}
static int prog_block(const struct lfs_config *c, lfs_block_t b, lfs_off_t o, const void *p,
                      lfs_size_t n) {
    Experiment *d = c->context;
    const uint8_t *s = p;
    uint8_t *t;
    uint32_t i;
    if (!p || !range(c, b, o, n) || o % c->prog_size || n % c->prog_size)
        return LFS_ERR_INVAL;
    if (d->fault_program == 0)
        return LFS_ERR_IO;
    if (d->fault_program > 0)
        --d->fault_program;
    t = d->medium + b * c->block_size + o;
    for (i = 0; i < n; i++)
        if ((t[i] & s[i]) != s[i])
            return LFS_ERR_CORRUPT;
    for (i = 0; i < n; i++)
        t[i] &= s[i];
    ++d->programs;
    return 0;
}
static int erase_block(const struct lfs_config *c, lfs_block_t b) {
    Experiment *d = c->context;
    if (!range(c, b, 0, c->block_size))
        return LFS_ERR_INVAL;
    memset(d->medium + b * c->block_size, 255, c->block_size);
    ++d->erases;
    return 0;
}
static int sync_block(const struct lfs_config *c) {
    Experiment *d = c->context;
    ++d->syncs;
    return d->fault_sync ? LFS_ERR_IO : 0;
}
static void configure(uint32_t block, uint32_t cache) {
    memset(&device, 0, sizeof device);
    memset(device.medium, 255, 4096);
    device.fault_program = -1;
    device.config.context = &device;
    device.config.read = read_block;
    device.config.prog = prog_block;
    device.config.erase = erase_block;
    device.config.sync = sync_block;
    device.config.read_size = 16;
    device.config.prog_size = 16;
    device.config.block_size = block;
    device.config.block_count = 4096 / block;
    device.config.block_cycles = -1;
    device.config.cache_size = cache;
    device.config.lookahead_size = 8;
    device.config.read_buffer = device.read_cache;
    device.config.prog_buffer = device.prog_cache;
    device.config.lookahead_buffer = device.lookahead;
    device.config.name_max = 19;
    device.config.file_max = 3592;
    device.config.attr_max = 32;
    device.file_config.buffer = device.file_cache;
}
static int fresh(void) {
    int r;
    memset(device.medium, 255, 4096);
    memset(&device.fs, 0, sizeof device.fs);
    device.fault_program = -1;
    device.fault_sync = 0;
    r = lfs_format(&device.fs, &device.config);
    if (r)
        return r;
    r = lfs_mount(&device.fs, &device.config);
    if (r)
        return r;
    if ((r = lfs_mkdir(&device.fs, "Documents")))
        return r;
    return lfs_mkdir(&device.fs, "Pictures");
}
static int put(const char *path, uint32_t size, uint8_t value) {
    int r, close;
    uint32_t i;
    for (i = 0; i < size; i++)
        data[i] = value;
    r = lfs_file_opencfg(&device.fs, &device.file, path, LFS_O_WRONLY | LFS_O_CREAT | LFS_O_TRUNC,
                         &device.file_config);
    if (r)
        return r;
    r = lfs_file_write(&device.fs, &device.file, data, size);
    close = lfs_file_close(&device.fs, &device.file);
    return r == (int)size ? close : r < 0 ? r : LFS_ERR_IO;
}
static int intact(const char *path, uint32_t size, uint8_t value) {
    int r, close;
    uint32_t i;
    r = lfs_file_opencfg(&device.fs, &device.file, path, LFS_O_RDONLY, &device.file_config);
    if (r)
        return 0;
    r = lfs_file_read(&device.fs, &device.file, readback, sizeof readback);
    close = lfs_file_close(&device.fs, &device.file);
    if (r != (int)size || close)
        return 0;
    for (i = 0; i < size; i++)
        if (readback[i] != value)
            return 0;
    return 1;
}
/* Do not truncate the original. A failed staged save leaves it committed and readable. */
static int replace(uint32_t size, uint8_t value) {
    int r = put("Pictures/pending.mbp", size, value);
    if (!r)
        r = lfs_rename(&device.fs, "Pictures/pending.mbp", "Pictures/image.mbp");
    if (r)
        lfs_remove(&device.fs, "Pictures/pending.mbp");
    return r;
}
static int remount(void) {
    memset(&device.fs, 0, sizeof device.fs);
    memset(&device.file, 0, sizeof device.file);
    device.fault_program = -1;
    device.fault_sync = 0;
    return lfs_mount(&device.fs, &device.config);
}
int mb_lfs_experiment(uint32_t block, uint32_t cache, MB_LfsExperiment *out) {
    uint32_t i, max = 0;
    int r;
    uint8_t zero[16] = {0}, ones[16];
    char name[] = "Documents/f00.txt";
    if (!out || (block != 128 && block != 256) || (cache != 16 && cache != 32))
        return -1;
    memset(out, 0, sizeof *out);
    configure(block, cache);
    out->block = block;
    out->cache = cache;
    out->medium_bytes = sizeof device.medium;
    /* Report all allocated bytes, including the maximum-size static cache buffers
     * and adapter instrumentation, even when the selected cache uses fewer bytes. */
    out->runtime_bytes = sizeof device - sizeof device.medium;
    out->bounds_checked =
        read_block(&device.config, device.config.block_count, 0, readback, 16) == LFS_ERR_INVAL &&
        prog_block(&device.config, 0, block, zero, 16) == LFS_ERR_INVAL &&
        erase_block(&device.config, device.config.block_count) == LFS_ERR_INVAL;
    memset(ones, 255, 16);
    erase_block(&device.config, 0);
    r = prog_block(&device.config, 0, 0, zero, 16);
    out->flash_checked = !r && prog_block(&device.config, 0, 0, ones, 16) == LFS_ERR_CORRUPT;
    memset(device.medium, 255, 4096);
    r = lfs_format(&device.fs, &device.config);
    out->format = r;
    if (r)
        return 0;
    r = lfs_mount(&device.fs, &device.config);
    if (r) {
        out->format = r;
        return 0;
    }
    out->empty_blocks = lfs_fs_size(&device.fs);
    r = lfs_mkdir(&device.fs, "Documents");
    if (!r)
        r = lfs_mkdir(&device.fs, "Pictures");
    if (r) {
        out->format = r;
        return 0;
    }
    out->directory_blocks = lfs_fs_size(&device.fs);
    out->picture = put("Pictures/image.mbp", 1040, 0x55);
    if (!out->picture && !intact("Pictures/image.mbp", 1040, 0x55))
        return -2;
    out->text_and_picture = out->picture ? out->picture : put("Documents/test.txt", 1024, 0x61);
    if (!out->text_and_picture &&
        (!intact("Pictures/image.mbp", 1040, 0x55) || !intact("Documents/test.txt", 1024, 0x61)))
        return -3;
    out->replacement = replace(1040, 0xaa);
    out->old_preserved = out->replacement ? intact("Pictures/image.mbp", 1040, 0x55)
                                          : intact("Pictures/image.mbp", 1040, 0xaa);
    {
        uint32_t low = 0, high = 3592;
        while (low <= high) {
            uint32_t middle = low + (high - low) / 2;
            if (fresh())
                return -4;
            if (!put("Documents/test.txt", middle, 0x61)) {
                max = middle;
                low = middle + 1;
            } else {
                if (!middle)
                    break;
                high = middle - 1;
            }
        }
    }
    out->maximum_payload = max;
    if (fresh())
        return -5;
    for (i = 0; i < 64; i++) {
        name[11] = '0' + i / 10;
        name[12] = '0' + i % 10;
        if (put(name, 0, 0))
            break;
    }
    out->file_count = i;
    if (fresh() || put("Pictures/image.mbp", 1040, 0x55))
        return -6;
    memcpy(snapshot, device.medium, 4096);
    device.fault_program = 1;
    r = replace(1040, 0xaa);
    device.fault_program = -1;
    out->io_failure_preserved =
        r == LFS_ERR_IO && !remount() && intact("Pictures/image.mbp", 1040, 0x55);
    memcpy(device.medium, snapshot, 4096);
    if (remount())
        return -7;
    r = lfs_file_opencfg(&device.fs, &device.file, "Pictures/pending.mbp",
                         LFS_O_WRONLY | LFS_O_CREAT, &device.file_config);
    if (r)
        return -9;
    memset(data, 0xaa, 1040);
    r = lfs_file_write(&device.fs, &device.file, data, 1040);
    if (r != 1040)
        return -10;
    /* Inject only after open and the complete write: this must fail close itself. */
    device.fault_sync = 1;
    r = lfs_file_close(&device.fs, &device.file);
    device.fault_sync = 0;
    out->close_failure_preserved =
        r == LFS_ERR_IO && !remount() && intact("Pictures/image.mbp", 1040, 0x55);
    memcpy(device.medium, snapshot, 4096);
    if (remount())
        return -8;
    r = lfs_file_opencfg(&device.fs, &device.file, "Pictures/pending.mbp",
                         LFS_O_WRONLY | LFS_O_CREAT, &device.file_config);
    if (!r) {
        memset(data, 0xaa, 1040);
        r = lfs_file_write(&device.fs, &device.file, data, 513);
    }
    out->interrupted_preserved = r >= 0 && !remount() && intact("Pictures/image.mbp", 1040, 0x55);
    memset(device.medium, 0, 4096);
    out->corrupt_rejected = remount() < 0;
    out->reads = device.reads;
    out->programs = device.programs;
    out->erases = device.erases;
    out->syncs = device.syncs;
    return 0;
}
