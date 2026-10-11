/**
 * @file tests/target/contracts.c
 * @brief Privileged API contract harness: uses SGDK allocator probes and internal dispatch.
 *
 * Optional test-only code, excluded from the release ROM.
 * Exposes only a read-only result mailbox to host tests. Not part of the release ROM.
 */
#include <genesis.h> /* Privileged target harness only: real allocator probe. */
#include "internal.h"
volatile uint16_t __attribute__((externally_visible)) mb_contract_result;
extern MB_Cursor mb_cursor_observation; /* Privileged inspection of the copied OS bitmap. */
static uint16_t cursor_vram_word(uint16_t address) {
    uint16_t value;
    SYS_disableInts();
    *(volatile uint32_t *)VDP_CTRL_PORT = VDP_READ_VRAM_ADDR(address);
    value = *(volatile uint16_t *)VDP_DATA_PORT;
    SYS_enableInts();
    return value;
}
static const uint8_t legacy_picture[1040] = {77, 66, 80, 49,  0,  64, 0, 64,        2,
                                             0,  4,  0,  135, 12, 0,  0, [1039] = 3};
static void canvas_test_tile(const void *context, uint16_t x, uint16_t y, uint32_t *rows) {
    uint16_t i;
    (void)context;
    (void)x;
    (void)y;
    for (i = 0; i < 8; ++i)
        rows[i] = 0;
}
static uint16_t init_count, activate_count, event_count, tick_count, stop_count, shutdown_count;
static MB_Result good_init(void) {
    ++init_count;
    return MB_OK;
}
static MB_Result bad_init(void) {
    ++init_count;
    return -99;
}
static void activate(void) {
    ++activate_count;
}
static void event(const MB_Event *e) {
    if (e && e->action == MB_ACTIVATE)
        ++event_count;
}
static void tick(void) {
    ++tick_count;
}
static void stop(void) {
    ++stop_count;
}
static void shutdown(void) {
    ++shutdown_count;
}
static void close_event(const MB_Event *e) {
    (void)e;
    mb_app_close();
}
static void failed_activate(void) {
    MB_PaletteRequest req = {15};
    MB_PaletteLease lease;
    mb_window_open("Failure fixture");
    mb_window_set_mode(MB_WINDOW_FULLSCREEN);
    if (mb_palette_request(&req, &lease) == MB_OK)
        mb_app_fail(MB_UNAVAILABLE);
    else
        mb_app_fail(MB_INVALID);
}
#define CHECK(condition, code)                                                                     \
    do {                                                                                           \
        if (!(condition)) {                                                                        \
            mb_contract_result = (code);                                                           \
            return;                                                                                \
        }                                                                                          \
    } while (0)
void mb_contract_selftest(void) {
    MB_App app = {MB_API_VERSION, "Test", good_init, activate, event, tick, stop, shutdown};
    MB_Event e = {MB_ACTIVATE, 1, 2};
    uint16_t i;
    mb_contract_result = 1;
    mb_runtime_reset();
    CHECK(mb_app_launch(0) == MB_INVALID, 2);
    app.tick = 0;
    CHECK(mb_app_launch(&app) == MB_INVALID, 3);
    app.tick = tick;
    app.version = 2;
    CHECK(mb_app_launch(&app) == MB_VERSION, 4);
    app.version = 1;
    app.init = bad_init;
    CHECK(mb_app_launch(&app) == -99 && !mb_runtime_active(), 5);
    CHECK(init_count == 1 && shutdown_count == 0 && activate_count == 0, 6);
    app.init = good_init;
    CHECK(mb_app_launch(&app) == MB_OK && mb_runtime_active(), 7);
    CHECK(mb_app_launch(&app) == MB_BUSY, 8);
    mb_runtime_event(&e);
    mb_runtime_tick(60);
    CHECK(init_count == 2 && activate_count == 1 && event_count == 1 && tick_count == 1, 9);
    mb_app_close();
    mb_app_close();
    CHECK(!mb_runtime_active() && stop_count == 1 && shutdown_count == 1, 10);
    mb_window_reset();
    CHECK(mb_window_open(0) == MB_INVALID, 11);
    CHECK(mb_draw_text(0, 0, "bad") == MB_INVALID, 12);
    CHECK(mb_window_open("Test") == MB_OK, 13);
    CHECK(mb_draw_text(30, 0, "bad") == MB_INVALID, 14);
    CHECK(mb_draw_text(0, 14, "bad") == MB_INVALID, 15);
    CHECK(mb_draw_text(0, 0, 0) == MB_INVALID, 16);
    CHECK(mb_draw_text(29, 13, "right edge clips") == MB_OK, 17);
    mb_runtime_reset();
    for (i = 0; i < 50; i++)
        mb_runtime_tick(50);
    CHECK(mb_time_seconds() == 1, 18);
    mb_runtime_reset();
    for (i = 0; i < 60; i++)
        mb_runtime_tick(60);
    CHECK(mb_time_seconds() == 1, 19);
    CHECK(mb_system_capabilities() ==
              (MB_CAP_CONTROLLER | MB_CAP_ROM_LINKED | MB_CAP_PSG | MB_CAP_ROM_VOLUME),
          20);
    {
        MB_MemoryStats before, after;
        uint16_t j;
        CHECK(mb_memory_stats(&before) == MB_OK && before.scope == MB_MEMORY_SGDK_HEAP, 21);
        CHECK((uint32_t)before.static_bytes + before.total_tracked + before.stack_reserved + 2 ==
                  65536,
              22);
        CHECK(before.used + before.free == before.total_tracked && before.stack_guard_ok, 23);
        CHECK(mb_memory_stats(0) == MB_INVALID, 24);
        for (j = 0; j < 3; j++)
            for (i = 1; i < MB_APP_COUNT; i++) {
                CHECK(mb_app_launch_id(i) == MB_OK, 25);
                mb_app_close();
                CHECK(!mb_runtime_active() && !mb_diagnostics.audio_active, 26);
            }
        CHECK(mb_memory_stats(&after) == MB_OK && before.used == after.used &&
                  before.free == after.free,
              27);
        CHECK(mb_app_launch_id(MB_APP_COUNT) == MB_NOT_FOUND, 28);
    }
    {
        MB_Volume v;
        MB_SystemInfo info;
        MB_Input input;
        MB_AppInfo appinfo;
        MB_Diagnostics d;
        char b[14];
        CHECK(mb_system_info(&info) == MB_OK && info.api_version == 1 &&
                  (info.refresh_hz == 50 || info.refresh_hz == 60),
              29);
        CHECK(mb_system_info(0) == MB_INVALID && mb_input_state(0) == MB_INVALID &&
                  mb_system_diagnostics(0) == MB_INVALID,
              30);
        CHECK(mb_storage_volume(0, &v) == MB_OK && v.flags == MB_VOLUME_READ_ONLY &&
                  v.capacity_bytes >= 131072,
              31);
        CHECK(mb_storage_volume(1, &v) == MB_NOT_FOUND && mb_storage_volume(0, 0) == MB_INVALID,
              32);
        CHECK(mb_app_get(MB_APP_COUNT, &appinfo) == MB_NOT_FOUND &&
                  mb_app_get(0, 0) == MB_INVALID && mb_app_get(0, &appinfo) == MB_NOT_FOUND &&
                  mb_app_get(MB_APP_CLOCK, &appinfo) == MB_OK,
              33);
        CHECK(mb_input_speed(0) == MB_INVALID && mb_input_speed(4) == MB_INVALID &&
                  mb_input_mode(3) == MB_INVALID,
              34);
        CHECK(mb_input_state(&input) == MB_OK && mb_system_diagnostics(&d) == MB_OK, 35);
        CHECK(mb_audio_tone(440, 12) == MB_UNAVAILABLE && mb_audio_noise(12) == MB_UNAVAILABLE, 36);
        CHECK(mb_app_launch_id(MB_APP_SOUND) == MB_OK && mb_audio_tone(440, 12) == MB_OK &&
                  mb_diagnostics.audio_active == 1,
              37);
        CHECK(mb_audio_tone(1, 12) == MB_INVALID && mb_audio_noise(7) == MB_INVALID, 38);
        mb_app_close();
        CHECK(!mb_diagnostics.audio_active, 39);
        CHECK(mb_window_open("Widgets") == MB_OK && mb_draw_rect(29, 13, 2, 1, 0) == MB_INVALID,
              40);
        CHECK(mb_ui_button(1, 0, 0, 6, "Active", 1) == MB_OK &&
                  mb_ui_button(2, 8, 0, 6, "Disabled", 0) == MB_OK,
              41);
        CHECK(mb_ui_focus(2) == MB_NOT_FOUND &&
                  mb_ui_button(1, 0, 2, 6, "Duplicate", 1) == MB_INVALID,
              42);
        mb_ui_reset();
        for (i = 0; i < 24; i++)
            CHECK(mb_ui_button(i, 0, 0, 6, "Pool", 1) == MB_OK, 43);
        CHECK(mb_ui_button(24, 0, 0, 6, "Full", 1) == MB_BUSY, 44);
        mb_runtime_reset();
        mb_runtime_elapsed(50, 150);
        CHECK(mb_time_seconds() == 3 && mb_time_ticks() == 900 && mb_time_milliseconds() == 3000,
              45);
        CHECK(mb_time_format(86400, b, sizeof b) == MB_OK && b[0] == '2' && b[1] == '4', 46);
    }
    {
        MB_MemoryStats before, allocated, after;
        void *probe;
        mb_memory_stats(&before);
        probe = MEM_alloc(512);
        CHECK(probe && mb_memory_stats(&allocated) == MB_OK &&
                  allocated.used >= before.used + 512 && allocated.free < before.free,
              47);
        MEM_free(probe);
        CHECK(mb_memory_stats(&after) == MB_OK && after.used == before.used &&
                  after.free == before.free && after.high_water >= allocated.used,
              48);
        app.event = close_event;
        CHECK(mb_app_launch(&app) == MB_OK, 49);
        mb_runtime_event(&e);
        CHECK(mb_runtime_active(), 50);
        mb_runtime_pending();
        CHECK(!mb_runtime_active(), 51);
    }
    {
        MB_Rect r;
        MB_PaletteRequest req = {15};
        MB_PaletteLease lease, forged;
        uint16_t colors[16] = {0}, elapsed = 0;
        const uint32_t pattern[8] = {0x11111111, 0x11111111, 0x11111111, 0x11111111,
                                     0x11111111, 0x11111111, 0x11111111, 0x11111111};
        CHECK(mb_grid_capacity(30, 14) == 6 && mb_grid_capacity(40, 26) == 16 &&
                  !mb_grid_capacity(9, 2),
              52);
        CHECK(!mb_escape_step(&elapsed, MB_KEY_START | MB_KEY_B, 50, 49) && elapsed == 294, 53);
        CHECK(mb_escape_step(&elapsed, MB_KEY_START | MB_KEY_B, 50, 1), 54);
        CHECK(!mb_escape_step(&elapsed, MB_KEY_START, 60, 60) && !elapsed, 55);
        CHECK(!mb_escape_step(&elapsed, MB_KEY_START | MB_KEY_B, 60, 59), 56);
        CHECK(mb_escape_step(&elapsed, MB_KEY_START | MB_KEY_B, 60, 1), 57);
        CHECK(mb_app_launch_id(MB_APP_CLOCK) == MB_OK, 58);
        CHECK(mb_palette_request(&req, &lease) == MB_UNAVAILABLE, 59);
        CHECK(mb_window_set_mode(MB_WINDOW_MAXIMIZED) == MB_OK &&
                  mb_window_get_client_rect(&r) == MB_OK && r.x == 0 && r.y == 1 && r.width == 40 &&
                  r.height == 27,
              60);
        CHECK(mb_window_get_content_rect(1, &r) == MB_OK && r.x == 0 && r.y == 2 && r.width == 40 &&
                  r.height == 26,
              130);
        CHECK(mb_window_get_content_rect(2, &r) == MB_INVALID &&
                  mb_window_get_content_rect(0, 0) == MB_INVALID,
              131);
        mb_ui_reset();
        CHECK(mb_ui_menu_button(1000, 0, 10, "File", MB_UI_ENABLED) == MB_OK &&
                  mb_ui_menu_button(1000, 10, 10, "Tools", MB_UI_ENABLED) == MB_INVALID &&
                  mb_ui_menu_button(1001, 39, 3, "Bad", MB_UI_ENABLED) == MB_INVALID &&
                  mb_ui_menu_button(1001, 10, 10, 0, MB_UI_ENABLED) == MB_INVALID &&
                  mb_ui_menu_button(1001, 10, 10, "Bad", 2) == MB_INVALID,
              132);
        {
            MB_Event click = {MB_ACTIVATE, 160, 80};
            uint16_t activated;
            mb_ui_reset();
            CHECK(mb_ui_menu_button(1000, 0, 10, "File", MB_UI_ENABLED | MB_UI_MENU_OPEN) ==
                          MB_OK &&
                      mb_ui_menu_button(1001, 10, 10, "Tools", MB_UI_ENABLED) == MB_OK,
                  133);
            mb_input_mode(MB_INPUT_FOCUS);
            CHECK(mb_ui_event(&click, &activated) == MB_OK && activated == MB_UI_MENU_DISMISS, 134);
            mb_input_mode(MB_INPUT_POINTER);
            CHECK(mb_ui_event(&click, &activated) == MB_OK && activated == MB_UI_MENU_DISMISS, 135);
            click.x = 100;
            click.y = 12;
            CHECK(mb_ui_event(&click, &activated) == MB_OK && activated == 1001, 136);
        }
        CHECK(mb_window_set_mode(MB_WINDOW_FULLSCREEN) == MB_OK &&
                  mb_window_get_client_rect(&r) == MB_OK && r.x == 0 && r.y == 0 && r.width == 40 &&
                  r.height == 28,
              61);
        CHECK(mb_palette_request(&req, &lease) == MB_OK && lease.lines == 15 &&
                  lease.tile_count > 0,
              62);
        {
            MB_Cursor cursor = {3, 3, 1, 1, {0x10000000, 0x54000000, 0x10000000}};
            CHECK(mb_cursor_set(&cursor, MB_CURSOR_CLIENT) == MB_BUSY, 137);
        }
        forged = lease;
        forged.token++;
        CHECK(mb_palette_set(&forged, 0, colors) == MB_INVALID &&
                  mb_palette_request(&req, &forged) == MB_BUSY,
              63);
        CHECK(mb_theme_preview(MB_THEME_AMBER) == MB_BUSY && mb_window_restore() == MB_BUSY, 64);
        colors[1] = 0xffff;
        CHECK(mb_palette_set(&lease, 0, colors) == MB_INVALID, 65);
        colors[1] = 0xeee;
        CHECK(mb_palette_set(&lease, 3, colors) == MB_OK &&
                  mb_palette_set(&lease, 4, colors) == MB_INVALID,
              66);
        CHECK(mb_graphics_pattern(&lease, lease.first_tile, pattern) == MB_OK &&
                  mb_graphics_pattern(&lease, 0, pattern) == MB_INVALID,
              67);
        CHECK(mb_graphics_tile(&lease, 39, 27, lease.first_tile, 3) == MB_OK &&
                  mb_graphics_tile(&lease, 40, 0, lease.first_tile, 3) == MB_INVALID,
              68);
        CHECK(mb_graphics_sprite(&lease, 0, 0, 0, lease.first_tile, 3) == MB_OK &&
                  mb_graphics_sprite(&lease, 2, 0, 0, lease.first_tile, 3) == MB_INVALID,
              68);
        SYS_doVBlankProcess();
        mb_video_flush();
        CHECK(PAL_getColor(49) == 0xeee, 66);
        CHECK(mb_palette_release(&lease) == MB_OK && !lease.token &&
                  mb_palette_release(&lease) == MB_INVALID,
              69);
        CHECK(PAL_getColor(1) == 0x864, 69);
        CHECK(mb_window_restore() == MB_OK && mb_window_get_client_rect(&r) == MB_OK &&
                  r.width == 30 && r.height == 14,
              70);
        for (i = 0; i < MB_THEME_COUNT; i++) {
            static const uint16_t expected[] = {0x864, 0x444, 0x462, 0x446, 0x862};
            CHECK(mb_theme_preview((MB_ThemeId)i) == MB_OK && mb_theme_current() == MB_THEME_BLUE,
                  71);
            SYS_doVBlankProcess();
            mb_video_flush();
            CHECK(PAL_getColor(1) == expected[i], 71);
            mb_theme_cancel_preview();
            SYS_doVBlankProcess();
            mb_video_flush();
            CHECK(PAL_getColor(1) == 0x864, 72);
            CHECK(mb_theme_current() == MB_THEME_BLUE, 72);
            CHECK(mb_theme_apply((MB_ThemeId)i) == MB_OK && mb_theme_current() == i, 73);
            CHECK(mb_theme_apply(MB_THEME_BLUE) == MB_OK, 74);
        }
        CHECK(mb_theme_preview(MB_THEME_COUNT) == MB_INVALID &&
                  mb_window_set_mode((MB_WindowMode)3) == MB_INVALID,
              75);
        mb_app_close();
        mb_runtime_pending();
        CHECK(mb_shell_test_paging(), 76);
        CHECK(mb_app_launch_id(MB_APP_SNAKE) == MB_OK &&
                  mb_window_set_mode(MB_WINDOW_FULLSCREEN) == MB_OK,
              77);
        CHECK(mb_palette_request(&req, &lease) == MB_OK, 78);
        mb_app_close();
        CHECK(mb_palette_set(&lease, 0, colors) == MB_INVALID &&
                  mb_window_mode() == MB_WINDOW_NORMAL,
              79);
    }
    {
        MB_PaletteRequest request = {15};
        MB_PaletteLease lease, stale;
        MB_GraphicsStats stats;
        static const uint32_t pattern[8] = {0x11111111};
        static const uint16_t bad_map[1] = {0xffff};
        uint16_t colors[64], n, theme, id;
        uint16_t region[2] = {0x2001, 2};
        MB_MemoryStats before, after;
        mb_launch_context(0);
        CHECK(mb_app_launch_id(MB_APP_CLOCK) == MB_OK &&
                  mb_window_set_mode(MB_WINDOW_FULLSCREEN) == MB_OK &&
                  mb_palette_request(&request, &lease) == MB_OK,
              80);
        CHECK(mb_graphics_load(&lease, lease.first_tile, pattern, 1) == MB_OK &&
                  mb_graphics_ready(&lease) == MB_BUSY &&
                  mb_graphics_load(&lease, lease.first_tile, pattern, 1) == MB_BUSY,
              81);
        CHECK(mb_graphics_load(&lease, lease.first_tile, pattern, lease.tile_count + 1) ==
                      MB_INVALID &&
                  mb_graphics_load(&lease, lease.first_tile, 0, 1) == MB_INVALID,
              82);
        SYS_doVBlankProcess();
        mb_video_flush();
        CHECK(mb_graphics_ready(&lease) == MB_OK && mb_graphics_stats(&stats) == MB_OK &&
                  stats.loaded_tiles == 1 && stats.queue_tiles == 0,
              83);
        CHECK(mb_graphics_fill(&lease, 1, 39, 27, 1, 1, lease.first_tile, 3) == MB_OK &&
                  mb_graphics_fill(&lease, 1, 39, 27, 2, 1, lease.first_tile, 3) == MB_INVALID &&
                  mb_graphics_map(&lease, 0, bad_map) == MB_INVALID &&
                  mb_graphics_map(&lease, 0, 0) == MB_INVALID,
              84);
        CHECK(mb_graphics_map_region(&lease, 1, 38, 27, 2, 1, region) == MB_OK &&
                  mb_graphics_map_region(&lease, 2, 0, 0, 1, 1, region) == MB_INVALID &&
                  mb_graphics_map_region(&lease, 1, 39, 27, 2, 1, region) == MB_INVALID &&
                  mb_graphics_map_region(&lease, 1, 0, 0, 0, 1, region) == MB_INVALID &&
                  mb_graphics_map_region(&lease, 1, 0, 0, 1, 1, 0) == MB_INVALID,
              151);
        region[0] = 3;
        region[1] = 2047;
        CHECK(mb_graphics_map_region(&lease, 1, 38, 27, 2, 1, region) == MB_INVALID, 152);
        for (n = 0; n < 20; n++)
            CHECK(mb_graphics_sprite_box(&lease, n, 8 * n, 0, 1, 1, lease.first_tile, 0) == MB_OK,
                  85);
        /* Drain transition work before checking an unchanged sprite submission. */
        for (n = 0; n < 8; n++) {
            SYS_doVBlankProcess();
            mb_video_flush();
        }
        CHECK(cursor_vram_word(VDP_getBGBAddress() + (27 * 64 + 38) * 2) ==
                      (0x2000 | (lease.first_tile + 1)) &&
                  cursor_vram_word(VDP_getBGBAddress() + (27 * 64 + 39) * 2) ==
                      lease.first_tile + 2,
              153);
        /* Repeated cached sprites must not inflate band occupancy or queue SAT. */
        for (n = 0; n < 20; n++)
            CHECK(mb_graphics_sprite_box(&lease, n, 8 * n, 0, 1, 1, lease.first_tile, 0) == MB_OK,
                  85);
        SYS_doVBlankProcess();
        mb_video_flush();
        CHECK(mb_graphics_stats(&stats) == MB_OK && stats.transfer_last == 0, 85);
        CHECK(mb_graphics_sprite_box(&lease, 20, 160, 0, 1, 1, lease.first_tile, 0) == MB_BUSY &&
                  mb_graphics_hide(&lease, 0) == MB_OK &&
                  mb_graphics_sprite_box(&lease, 20, 160, 0, 1, 1, lease.first_tile, 0) == MB_OK,
              86);
        CHECK(mb_graphics_sprite_box(&lease, 0, 0, 0, 4, 4, lease.first_tile + lease.tile_count - 1,
                                     0) == MB_INVALID &&
                  mb_graphics_sprite_box(&lease, 0, 0, 0, 5, 1, lease.first_tile, 0) ==
                      MB_INVALID &&
                  mb_graphics_hide(&lease, 21) == MB_INVALID,
              87);
        CHECK(mb_audio_noise(13) == MB_OK && mb_audio_volume(14) == MB_OK &&
                  mb_audio_volume(7) == MB_INVALID,
              88);
        mb_audio_stop();
        CHECK(mb_audio_volume(13) == MB_UNAVAILABLE, 89);
        CHECK(mb_graphics_load(&lease, lease.first_tile, pattern, 1) == MB_OK, 90);
        stale = lease;
        mb_app_close();
        CHECK(mb_graphics_ready(&stale) == MB_INVALID &&
                  mb_graphics_stats(&stats) == MB_UNAVAILABLE,
              91);
        mb_memory_stats(&before);
        for (theme = 0; theme < MB_THEME_COUNT; theme++) {
            CHECK(mb_theme_apply((MB_ThemeId)theme) == MB_OK, 92);
            SYS_doVBlankProcess();
            mb_video_flush();
            for (n = 0; n < 64; n++)
                colors[n] = PAL_getColor(n);
            for (id = MB_APP_MEGABOUNCE; id < MB_APP_COUNT; id++) {
                CHECK(mb_app_launch_id(id) == MB_OK, 93);
                for (n = 0; n < 75; n++) {
                    SYS_doVBlankProcess();
                    mb_video_flush();
                    mb_runtime_tick(SYS_isPAL() ? 50 : 60);
                    mb_runtime_pending();
                }
                mb_app_close();
                /* Palette restoration is queued for the next foreground VBlank flush. */
                SYS_doVBlankProcess();
                mb_video_flush();
                for (n = 0; n < 64; n++)
                    CHECK(PAL_getColor(n) == colors[n], 94);
                mb_memory_stats(&after);
                CHECK(before.used == after.used && after.stack_guard_ok &&
                          !mb_diagnostics.audio_active,
                      95);
            }
        }
        app.activate = failed_activate;
        CHECK(mb_app_launch(&app) == MB_OK && mb_runtime_active(), 96);
        mb_runtime_pending();
        CHECK(!mb_runtime_active() && mb_window_mode() == MB_WINDOW_NORMAL &&
                  mb_graphics_stats(&stats) == MB_UNAVAILABLE && !mb_diagnostics.audio_active,
              97);
        e.action = MB_ACTIVATE;
        mb_runtime_event(&e);
        CHECK(mb_app_fail(MB_OK) == MB_INVALID && mb_app_fail(MB_UNAVAILABLE) == MB_UNAVAILABLE,
              98);
        mb_theme_apply(MB_THEME_BLUE);
    }
    {
        MB_FileHandle h;
        MB_FSInfo fs;
        MB_FileInfo f;
        MB_TextInput t;
        MB_Event e = {MB_ACTIVATE, 0, 0};
        char p[44];
        static const uint8_t pixels[1024] = {0};
        mb_fs_reset();
        CHECK(mb_fs_info(&fs) == MB_OK && fs.reserved_bytes == 4096 && fs.capacity == 3592 &&
                  !fs.used,
              99);
        CHECK(mb_file_open("RAM:/Documents/test.txt", 7, &h) == MB_OK &&
                  mb_file_write(h, (const uint8_t *)"ab", 2) == MB_OK,
              100);
        CHECK(mb_file_close(h) == MB_OK && mb_file_stat("RAM:/Documents/test.txt", &f) == MB_OK &&
                  f.size == 2,
              101);
        mb_launch_document("RAM:/Documents/test.txt");
        CHECK(mb_app_launch_id(MB_APP_MEGAPAD) == MB_OK, 102);
        mb_runtime_pending();
        CHECK(mb_app_document(p, sizeof p) == MB_OK && mb_window_mode() == MB_WINDOW_MAXIMIZED,
              103);
        CHECK(mb_text_keyboard(65535, 0) == MB_INVALID && mb_text_keyboard(0, 18) == MB_OK &&
                  mb_text_event(0, &t) == MB_INVALID,
              104);
        mb_input_mode(MB_INPUT_FOCUS);
        mb_runtime_event(&e);
        mb_runtime_request_close();
        CHECK(mb_runtime_active(), 105);
        CHECK(mb_file_open("RAM:/Documents/test.txt", 1, &h) == MB_OK, 106);
        mb_app_close();
        CHECK(mb_file_close(h) == MB_INVALID &&
                  mb_file_stat("RAM:/Documents/test.txt", &f) == MB_OK && f.size == 2,
              107);
        CHECK(mb_app_launch_id(MB_APP_MEGAPAINT) == MB_OK &&
                  mb_app_document(p, sizeof p) == MB_NOT_FOUND,
              108);
        CHECK(mb_canvas_open(65535, 0) == MB_INVALID && mb_canvas_open(0, 0) == MB_OK &&
                  mb_canvas_update(0, 0, 0, 64, 64, 0, 0) == MB_INVALID,
              109);
        CHECK(mb_canvas_update(pixels, 0, 0, 64, 64, 63, 63) == MB_OK &&
                  mb_canvas_update(pixels, 63, 0, 2, 1, 0, 0) == MB_INVALID,
              110);
        SYS_doVBlankProcess();
        mb_video_flush();
        mb_app_close();
        CHECK(mb_fs_info(&fs) == MB_OK && fs.used == 2, 111);
    }
    {
        MB_MusicTrack track;
        MB_FileHandle h;
        static const uint8_t header[512] = {0x10, 8, 0, 0, 1, 0, 0, 0};
        static const uint8_t overflow[512] = {0x10, 8, 255, 255, 2, 0, 0, 0};
        CHECK(mb_music_capabilities() == MB_MUSIC_BUNDLED &&
                  mb_music_track(0, 1, &track) == MB_OK && track.bytes > 0,
              112);
        CHECK(mb_music_track(5, 1, &track) == MB_NOT_FOUND &&
                  mb_music_track(0, 2, &track) == MB_VERSION &&
                  mb_music_track(0, 1, 0) == MB_INVALID,
              113);
        CHECK(mb_music_play(0) == MB_UNAVAILABLE, 114);
        mb_fs_reset();
        CHECK(mb_file_open("RAM:/Music/test.xgc", 6, &h) == MB_OK &&
                  mb_file_write(h, header, sizeof header) == MB_OK && mb_file_close(h) == MB_OK &&
                  mb_app_launch_id(MB_APP_MEGAPLAYER) == MB_OK,
              115);
        CHECK(mb_music_play_file("RAM:/Music/test.xgc") == MB_NOT_SUPPORTED &&
                  mb_music_state() == MB_MUSIC_ERROR && !mb_diagnostics.audio_active,
              116);
        CHECK(mb_file_open("RAM:/Music/test.xgc", 2, &h) == MB_OK &&
                  mb_file_write(h, header, 7) == MB_OK && mb_file_close(h) == MB_OK &&
                  mb_music_play_file("RAM:/Music/test.xgc") == MB_CORRUPT,
              117);
        CHECK(mb_file_open("RAM:/Music/test.xgc", 2, &h) == MB_OK &&
                  mb_file_write(h, overflow, sizeof overflow) == MB_OK &&
                  mb_file_close(h) == MB_OK &&
                  mb_music_play_file("RAM:/Music/test.xgc") == MB_CORRUPT,
              118);
        CHECK(mb_file_open("RAM:/Music/raw.vgm", 6, &h) == MB_OK && mb_file_close(h) == MB_OK &&
                  mb_music_play_file("RAM:/Music/raw.vgm") == MB_NOT_SUPPORTED &&
                  mb_music_play_file("SAVE:/Music/test.xgc") == MB_NOT_SUPPORTED,
              119);
        CHECK(mb_music_play(50) == MB_NOT_FOUND && !mb_diagnostics.audio_active, 120);
        mb_app_close();
        CHECK(mb_music_state() == MB_MUSIC_STOPPED && !mb_diagnostics.audio_active, 121);
        mb_fs_reset();
    }
    {
        MB_CanvasSource source = {1, 320, 176, legacy_picture, 0, canvas_test_tile};
        uint16_t colors[16] = {0};
        CHECK(mb_app_launch_id(MB_APP_MEGAPAINT) == MB_OK, 123);
        CHECK(mb_canvas_attach(0, 0, 2, 40, 22, colors) == MB_INVALID, 124);
        source.version = 2;
        CHECK(mb_canvas_attach(&source, 0, 2, 40, 22, colors) == MB_VERSION, 125);
        source.version = 1;
        CHECK(mb_canvas_attach(&source, 0, 2, 40, 22, colors) == MB_OK &&
                  mb_canvas_view(0, 0, 3, 0, 0) == MB_INVALID &&
                  mb_canvas_view(511, 0, 1, 0, 0) == MB_INVALID &&
                  mb_canvas_view(0, 0, 1, 65535, 65535) == MB_OK &&
                  mb_canvas_damage(319, 175, 1, 1) == MB_OK &&
                  mb_canvas_damage(319, 175, 2, 1) == MB_INVALID,
              126);
        colors[4] = 1;
        CHECK(mb_canvas_palette(colors) == MB_INVALID && mb_canvas_swatch(0, 0, 16) == MB_INVALID,
              127);
        CHECK(mb_canvas_brush(0) == MB_INVALID && mb_canvas_brush(9) == MB_INVALID &&
                  mb_canvas_brush(8) == MB_OK && mb_canvas_cursor(3) == MB_INVALID &&
                  mb_canvas_cursor(2) == MB_OK && mb_canvas_cursor(1) == MB_OK &&
                  mb_canvas_cursor(0) == MB_OK,
              129);
        {
            MB_Cursor cursor = {3, 3, 1, 1, {0x10000000, 0x54000000, 0x10000000}};
            MB_Rect rect;
            {
                MB_Cursor maximum = {16, 16, 15, 15, {0}};
                maximum.rows[15] = 3;
                CHECK(mb_cursor_set(&maximum, MB_CURSOR_CLIENT) == MB_OK, 138);
                SYS_doVBlankProcess();
                mb_video_flush();
                CHECK(cursor_vram_word((TILE_USER_INDEX + 63) * 32 + 30) == 3, 143);
            }
            CHECK(mb_cursor_set(&cursor, MB_CURSOR_CLIENT) == MB_OK, 138);
            cursor.rows[0] = 0;
            CHECK(mb_cursor_observation.rows[0] == 0x10000000, 139);
            cursor.width = 17;
            CHECK(mb_cursor_set(&cursor, MB_CURSOR_CLIENT) == MB_INVALID &&
                      mb_cursor_observation.width == 3,
                  140);
            cursor.width = 3;
            cursor.hotspot_x = 3;
            CHECK(mb_cursor_set(&cursor, MB_CURSOR_CLIENT) == MB_INVALID, 141);
            cursor.hotspot_x = 1;
            CHECK(mb_cursor_set(0, MB_CURSOR_CLIENT) == MB_INVALID &&
                      mb_cursor_set(&cursor, 2) == MB_INVALID,
                  142);
            SYS_doVBlankProcess();
            mb_video_flush();
            CHECK(cursor_vram_word((TILE_USER_INDEX + 60) * 32) == 0x0100, 143);
            mb_input_mode(MB_INPUT_POINTER);
            mb_pointer(80, 80);
            CHECK(vdpSpriteCache[0].x == 207 && vdpSpriteCache[0].y == 207 &&
                      VDP_getSpriteTile(0) == TILE_USER_INDEX + 60,
                  144);
            mb_pointer(80, 4);
            CHECK(VDP_getSpriteTile(0) == TILE_USER_INDEX + 5, 144);
            mb_cursor_reset();
            CHECK(mb_cursor_observation.width == 0, 145);
            CHECK(mb_cursor_set(&cursor, MB_CURSOR_CANVAS) == MB_OK &&
                      mb_canvas_view(0, 0, 1, 20, 20) == MB_OK &&
                      mb_window_get_client_rect(&rect) == MB_OK,
                  146);
            SYS_doVBlankProcess();
            mb_video_flush();
            mb_pointer(80, 80);
            CHECK(vdpSpriteCache[0].x == rect.x * 8 + 20 - 1 + 128 &&
                      vdpSpriteCache[0].y == (rect.y + 2) * 8 + 20 - 1 + 128,
                  147);
            mb_canvas_close();
            CHECK(!mb_cursor_observation.width &&
                      mb_cursor_set(&cursor, MB_CURSOR_CANVAS) == MB_UNAVAILABLE,
                  148);
            CHECK(mb_cursor_set(&cursor, MB_CURSOR_CLIENT) == MB_OK, 149);
            mb_cursor_reset(); /* Cancellation cannot leave a custom sprite visible. */
            SYS_doVBlankProcess();
            mb_video_flush();
            mb_pointer(80, 80);
            CHECK(VDP_getSpriteTile(0) == TILE_USER_INDEX + 5, 149);
            CHECK(mb_cursor_set(&cursor, MB_CURSOR_CLIENT) == MB_OK, 149);
        }
        mb_app_close();
        CHECK(mb_canvas_cursor(1) == MB_UNAVAILABLE && !mb_cursor_observation.width, 129);
        {
            MB_Cursor cursor = {16, 16, 15, 15, {0xffffffff}};
            CHECK(mb_cursor_set(&cursor, MB_CURSOR_CLIENT) == MB_UNAVAILABLE, 150);
        }
    }
    mb_runtime_reset();
    mb_shell_reset();
    mb_input_mode(MB_INPUT_POINTER);
    mb_desktop_draw();
    {
        /* Only this privileged test ROM imports fixtures; the release cold disk is empty. */
        MB_FileHandle h;
        static const uint8_t bad[7] = {0x10, 8};
        CHECK(mb_file_open("RAM:/Music/bad.xgc", 6, &h) == MB_OK &&
                  mb_file_write(h, bad, sizeof bad) == MB_OK && mb_file_close(h) == MB_OK &&
                  mb_file_open("RAM:/Music/raw.vgm", 6, &h) == MB_OK && mb_file_close(h) == MB_OK,
              122);
    }
    {
        MB_FileHandle h;
        CHECK(mb_file_open("RAM:/Pictures/legacy.mbp", 6, &h) == MB_OK &&
                  mb_file_write(h, legacy_picture, sizeof legacy_picture) == MB_OK &&
                  mb_file_close(h) == MB_OK,
              128);
    }
    mb_contract_result = 0x600d;
}
