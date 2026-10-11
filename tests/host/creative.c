/**
 * @file tests/host/creative.c
 * @brief Tests production VFS, RAM Disk, text editing and legacy picture handling with Unity.
 *
 * Host Unity fixture linked against production C code; no emulator or hardware dependency.
 */
#include "unity.h"
/* Host-only assertions against the production filesystem and document models. */
#include "megabenchos.h"
#include "../../kernel/storage/ramdisk.h"
#include "../../kernel/storage/vfs.h"
#include "../../apps/creative/model.h"
#include "../../kernel/media/format.h"
#include <stddef.h>
static uint8_t bytes[3592], readback[3592];
static Pad pad;
static uint8_t picture[1040];
#define CHECK(x)                                                                                   \
    do {                                                                                           \
        if (!(x))                                                                                  \
            UnityFail(#x, __LINE__);                                                               \
        ++checks;                                                                                  \
    } while (0)
int mb_creative_tests(void) {
    int checks = 0;
    uint16_t i, n;
    MB_FileHandle h, a[6];
    MB_FSInfo fs;
    MB_FileInfo f;
    char path[] = "RAM:/Documents/f00.txt";
    mb_fs_reset();
    {
        MB_DriveInfo d;
        MB_FileHandle rom, stale;
        CHECK(mb_fs_capacity("ram:", 1, &d) == MB_OK && d.capacity == 3592 &&
              (d.capabilities & (MB_FS_VOLATILE | MB_FS_ATOMIC_REPLACE)) ==
                  (MB_FS_VOLATILE | MB_FS_ATOMIC_REPLACE));
        CHECK(mb_fs_capacity("RAM:", 2, &d) == MB_VERSION);
        CHECK(mb_fs_capacity("RAM:", 1, 0) == MB_INVALID);
        CHECK(mb_fs_drive(2, 1, &d) == MB_OK && d.capabilities == 0);
        CHECK(mb_fs_drive(4, 1, &d) == MB_NOT_FOUND);
        CHECK(mb_dir_list_path("RAM:/", 0, &f) == MB_OK && f.directory == MB_DIR_DOCUMENTS &&
              (f.flags & MB_FILE_DIRECTORY));
        CHECK(mb_dir_list_path("RAM:/", 1, &f) == MB_OK && f.directory == MB_DIR_PICTURES);
        CHECK(mb_dir_list_path("RAM:/", 2, &f) == MB_OK && f.directory == MB_DIR_MUSIC);
        CHECK(mb_dir_list_path("RAM:/", 3, &f) == MB_NOT_FOUND);
        CHECK(mb_dir_list_path("RAM:/Documents/", 0, &f) == MB_NOT_FOUND);
        CHECK(mb_fs_capacity("ROM:", 1, &d) == MB_OK && d.free == 0 &&
              (d.capabilities & MB_FS_READ_ONLY));
        CHECK(mb_dir_list_path("rom:/Help/", 0, &f) == MB_OK && (f.flags & MB_FILE_READ_ONLY));
        CHECK(mb_file_open("ROM:/Help/readme.txt", MB_FILE_READ, &rom) == MB_OK);
        CHECK(mb_file_read(rom, 0, readback, 24, &n) == MB_OK && n == 24 && readback[0] == 'M');
        CHECK(mb_file_read(rom, f.size, readback, 24, &n) == MB_OK && n == 0);
        CHECK(mb_file_write(rom, bytes, 1) == MB_READ_ONLY);
        CHECK(mb_file_close(rom) == MB_OK);
        CHECK(mb_file_close(rom) == MB_INVALID);
        CHECK(mb_file_open("ROM:/Help/readme.txt", MB_FILE_WRITE, &rom) == MB_READ_ONLY);
        CHECK(mb_file_delete("ROM:/Help/readme.txt") == MB_READ_ONLY);
        CHECK(mb_file_rename("ROM:/Help/readme.txt", "ROM:/Help/new.txt") == MB_READ_ONLY);
        CHECK(mb_file_rename("RAM:/Documents/a.txt", "ROM:/Help/a.txt") == MB_NOT_SUPPORTED);
        CHECK(mb_file_stat("SAVE:/a.txt", &f) == MB_NOT_SUPPORTED);
        CHECK(mb_file_stat("SD:/a.txt", &f) == MB_NOT_SUPPORTED);
        CHECK(mb_file_stat("NO:/a.txt", &f) == MB_NOT_FOUND);
        CHECK(mb_file_stat("RAM://Documents/a.txt", &f) == MB_INVALID);
        CHECK(mb_file_stat("RAM:/Documents/./a.txt", &f) == MB_INVALID);
        CHECK(mb_file_stat("RAM:\\Documents\\a.txt", &f) == MB_INVALID);
        for (i = 0; i < 6; i++)
            CHECK(mb_file_open("ROM:/Help/readme.txt", MB_FILE_READ, &a[i]) == MB_OK);
        CHECK(mb_file_open("ROM:/Help/readme.txt", MB_FILE_READ, &rom) == MB_LIMIT);
        stale = a[0];
        mb_fs_reset();
        CHECK(mb_file_read(stale, 0, readback, 1, &n) == MB_INVALID);
        CHECK(mb_file_open("ROM:/Help/readme.txt", MB_FILE_READ, &rom) == MB_OK && rom != stale);
        CHECK(mb_file_close(rom) == MB_OK);
    }
    CHECK(mb_fs_info(&fs) == 0 && fs.reserved_bytes == 4096 && fs.capacity == 3592 && fs.used == 0);
    CHECK(mb_file_open("RAM:/Documents/../bad.txt", 6, &h) == MB_INVALID);
    CHECK(mb_file_open("RAM:/Pictures/test.txt", 6, &h) == MB_INVALID);
    CHECK(mb_file_open("RAM:/Documents/.txt", 6, &h) == MB_INVALID);
    CHECK(mb_file_open("RAM:/Documents/Test.txt", 7, &h) == 0);
    for (i = 0; i < 3592; i++)
        bytes[i] = (uint8_t)i;
    CHECK(mb_file_write(h, bytes, 3592) == 0);
    CHECK(mb_file_write(h, bytes, 3593) == MB_NO_SPACE);
    CHECK(mb_file_read(h, 0, readback, 3592, &n) == 0 && n == 3592);
    for (i = 0; i < n; i++)
        CHECK(readback[i] == bytes[i]);
    CHECK(mb_file_write(h, 0, 1) == MB_INVALID);
    CHECK(mb_file_read(h, 3592, 0, 0, &n) == 0 && n == 0);
    CHECK(mb_file_delete("RAM:/Documents/Test.txt") == MB_BUSY);
    CHECK(mb_file_close(h) == 0);
    CHECK(mb_file_close(h) == MB_INVALID);
    CHECK(mb_file_stat("ram:/documents/test.TXT", &f) == 0 && f.size == 3592);
    CHECK(mb_file_open("RAM:/Documents/Test.txt", 1, &a[0]) == 0);
    for (i = 1; i < 6; i++)
        CHECK(mb_file_open("RAM:/Documents/Test.txt", 1, &a[i]) == 0);
    CHECK(mb_file_open("RAM:/Documents/Test.txt", 1, &h) == MB_LIMIT);
    CHECK(mb_file_open("RAM:/Documents/Test.txt", 2, &h) == MB_LIMIT);
    CHECK(mb_file_close(a[5]) == MB_OK);
    CHECK(mb_file_open("RAM:/Documents/Test.txt", 2, &h) == MB_BUSY);
    mb_fs_close_handles();
    CHECK(mb_file_close(a[0]) == MB_INVALID);
    CHECK(mb_file_open("RAM:/Documents/Other.txt", 7, &h) == 0);
    CHECK(mb_file_write(h, bytes, 1) == MB_NO_SPACE);
    CHECK(mb_file_close(h) == 0);
    CHECK(mb_file_rename("RAM:/Documents/Test.txt", "RAM:/Documents/Other.txt") == MB_EXISTS);
    CHECK(mb_file_rename("RAM:/Documents/Test.txt", "RAM:/Pictures/Test.mbp") == MB_INVALID);
    CHECK(mb_file_rename("RAM:/Documents/Test.txt", "RAM:/Documents/Renamed.txt") == 0);
    CHECK(mb_file_delete("RAM:/Documents/Other.txt") == 0);
    CHECK(mb_file_delete("RAM:/Documents/Renamed.txt") == 0);
    CHECK(mb_fs_info(&fs) == 0 && fs.used == 0);
    for (i = 0; i < 16; i++) {
        path[16] = '0' + i / 10;
        path[17] = '0' + i % 10;
        CHECK(mb_file_open(path, 6, &h) == 0);
        CHECK(mb_file_close(h) == 0);
    }
    CHECK(mb_file_open("RAM:/Documents/full.txt", 6, &h) == MB_LIMIT);
    CHECK(mb_dir_list(0, 15, &f) == 0);
    CHECK(mb_dir_list(0, 16, &f) == MB_NOT_FOUND);
    mb_fs_test_corrupt();
    CHECK(mb_fs_info(&fs) == MB_CORRUPT);
    CHECK(mb_file_open(path, 1, &h) == MB_CORRUPT);
    mb_fs_reset();
    CHECK(mb_fs_info(&fs) == 0 && fs.files == 0);
    CHECK(mb_file_open("RAM:/Documents/a.txt", 7, &h) == 0);
    CHECK(mb_file_write(h, bytes, 100) == 0);
    CHECK(mb_file_close(h) == 0);
    CHECK(mb_file_open("RAM:/Documents/b.txt", 7, &h) == 0);
    CHECK(mb_file_write(h, bytes + 100, 100) == 0);
    CHECK(mb_file_close(h) == 0);
    CHECK(mb_file_open("RAM:/Documents/a.txt", 7, &h) == 0);
    CHECK(mb_file_write(h, bytes, 3000) == 0);
    CHECK(mb_file_close(h) == 0);
    CHECK(mb_file_open("RAM:/Documents/b.txt", 1, &h) == 0);
    CHECK(mb_file_read(h, 0, readback, 100, &n) == 0 && n == 100);
    for (i = 0; i < 100; i++)
        CHECK(readback[i] == bytes[i + 100]);
    CHECK(mb_file_close(h) == 0);
    CHECK(mb_file_open("RAM:/Documents/a.txt", 3, &h) == 0);
    CHECK(mb_file_write(h, bytes, 3592) == MB_NO_SPACE);
    CHECK(mb_file_read(h, 0, readback, 3592, &n) == 0 && n == 3000);
    for (i = 0; i < n; i++)
        CHECK(readback[i] == bytes[i]);
    CHECK(mb_file_write(h, 0, 0) == 0);
    CHECK(mb_file_close(h) == 0);
    CHECK(mb_fs_info(&fs) == 0 && fs.used == 100);
    pad_reset(&pad);
    {
        MB_TextInput t = {MB_TEXT_CHAR, 'a'};
        for (i = 0; i < 1024; i++)
            CHECK(pad_edit(&pad, &t) == 0);
        CHECK(pad.length == 1024 && pad.cursor == 1024 && pad.text[1024] == 0);
        CHECK(pad_edit(&pad, &t) == MB_OVERFLOW && pad.length == 1024);
        t.action = MB_TEXT_BACKSPACE;
        CHECK(pad_edit(&pad, &t) == 0 && pad.length == 1023);
        pad.cursor = 0;
        CHECK(pad_edit(&pad, &t) == 0 && pad.length == 1023);
        t.action = MB_TEXT_DELETE;
        CHECK(pad_edit(&pad, &t) == 0 && pad.length == 1022);
        t.action = MB_TEXT_ENTER;
        CHECK(pad_edit(&pad, &t) == 0 && pad.text[0] == '\n');
        t.action = MB_TEXT_CHAR;
        t.character = 1;
        CHECK(pad_edit(&pad, &t) == MB_INVALID);
        pad.cursor = pad.length;
        pad_move(&pad, 1, 0, 38);
        CHECK(pad.cursor == pad.length);
        pad.cursor = 0;
        pad_move(&pad, -1, -1, 38);
        CHECK(pad.cursor == 0);
    }
    paint_new(picture);
    CHECK(paint_validate(picture, 1040) == 0);
    for (i = 0; i < 4096; i++)
        paint_set(picture, i % 64, i / 64, i % 4);
    paint_encode(picture);
    CHECK(paint_validate(picture, 1040) == 0);
    for (i = 0; i < 4096; i++)
        CHECK(paint_get(picture, i % 64, i / 64) == i % 4);
    CHECK(mb_file_open("RAM:/Pictures/image.mbp", 7, &h) == 0);
    CHECK(mb_file_write(h, picture, 1040) == 0);
    CHECK(mb_file_read(h, 0, readback, 1040, &n) == 0 && n == 1040);
    for (i = 0; i < 1040; i++)
        CHECK(readback[i] == picture[i]);
    CHECK(mb_file_close(h) == 0);
    CHECK(paint_validate(picture, 1039) == MB_CORRUPT);
    picture[16] ^= 1;
    CHECK(paint_validate(picture, 1040) == MB_CORRUPT);
    picture[16] ^= 1;
    picture[3] = '2';
    CHECK(paint_validate(picture, 1040) == MB_VERSION);
    picture[3] = '1';
    picture[5] = 32;
    CHECK(paint_validate(picture, 1040) == MB_VERSION);
    mb_fs_reset();
    CHECK(mb_fs_info(&fs) == 0 && fs.files == 0 && fs.used == 0);
    {
        uint8_t header[8] = {0x10, 8, 0, 0, 1, 0, 1, 0};
        CHECK(mb_music_header(header, 768) == MB_OK);
        CHECK(mb_music_header(header, 767) == MB_CORRUPT);
        CHECK(mb_music_header(0, 768) == MB_CORRUPT);
        CHECK(mb_music_header(header, 7) == MB_CORRUPT);
        header[0] = 0x11;
        CHECK(mb_music_header(header, 768) == MB_VERSION);
        header[0] = 0x10;
        header[1] = 10;
        CHECK(mb_music_header(header, 768) == MB_NOT_SUPPORTED);
        header[1] = 8;
        header[2] = header[3] = 255;
        CHECK(mb_music_header(header, 768) == MB_CORRUPT);
        CHECK(mb_file_open("RAM:/Music/probe.xgc", 7, &h) == MB_OK);
        CHECK(mb_file_write(h, bytes, 512) == MB_OK && mb_file_close(h) == MB_OK);
        CHECK(mb_dir_list_path("RAM:/Music/", 0, &f) == MB_OK && f.size == 512);
        CHECK(mb_file_rename("RAM:/Music/probe.xgc", "RAM:/Music/renamed.xgc") == MB_OK);
        CHECK(mb_file_open("RAM:/Music/raw.vgm", 6, &h) == MB_OK && mb_file_close(h) == MB_OK);
        CHECK(mb_file_open("RAM:/Music/bad.txt", 6, &h) == MB_INVALID);
        CHECK(mb_file_open("RAM:/Documents/bad.xgc", 6, &h) == MB_INVALID);
        CHECK(mb_file_delete("RAM:/Music/renamed.xgc") == MB_OK);
        mb_fs_reset();
    }
    return checks;
}
