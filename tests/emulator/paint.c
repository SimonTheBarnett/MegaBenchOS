/**
 * @file tests/emulator/paint.c
 * @brief Checks native pixel editing, colours, menus, zoom, pan, saves and legacy import in GX.
 *
 * Unity integration fixture. Drives controller/mouse input and reads observations; no RAM
 * injection.
 */
#include "../../tools/native/runner.h"
static bool CursorIs(int a, int b, int c, int d) {
    Words w = W("mb_brush_cursor_observation", 4);
    return w.v[0] == a && w.v[1] == b && w.v[2] == c && w.v[3] == d;
}
// Native canvas, menus, palette, storage capacity and mouse regression.

static Words Paint(void) {
    return W("mb_paint_observation", 12);
}
static Words Canvas(void) {
    return W("mb_canvas_observation", 8);
}

/** Compare pixels and palette independently of sparse tile allocation IDs. */
static Hash PictureHash(void) {
    Words p = Paint();
    int n = (p.v[0] / 8) * (p.v[1] / 8);
    int map[2048];
    core_words(Symbol("paint_picture") + 38, n, map);
    unsigned char *tiles = ReadBytes(Symbol("paint_picture") + 4134, 12288),
                  *header = ReadBytes(Symbol("paint_picture"), 38),
                  *canonical = mb_alloc(36 + (size_t)n * 32);
    memcpy(canonical, header, 4);
    memcpy(canonical + 4, header + 6, 32);
    for (int t = 0; t < n; t++) {
        int id = map[t] & 32767;
        if (id & 16384)
            memset(canonical + 36 + t * 32, (id & 15) * 17, 32);
        else {
            REQUIRE(id >= 1 && id <= 384, "Paint tile reference exceeds detail storage");
            memcpy(canonical + 36 + t * 32, tiles + (id - 1) * 32, 32);
        }
    }
    Hash hash = mb_hash(canonical, 36 + (size_t)n * 32);
    free(tiles);
    free(header);
    free(canonical);
    return hash;
}
static Hash DiskHash(void) {
    unsigned char *bytes = ReadBytes(Symbol("disk"), 4096);
    Hash hash = mb_hash(bytes, 4096);
    free(bytes);
    return hash;
}
static int Pixel(int x, int y) {
    Words p = Paint();
    int t = y / 8 * (p.v[0] / 8) + x / 8, id = W("paint_picture", 1, 38 + t * 2).v[0] & 32767;
    if ((id & 16384) != 0)
        return id & 15;
    unsigned char *bytes =
        ReadBytes(Symbol("paint_picture") + 4134 + (id - 1) * 32 + y % 8 * 4 + x % 8 / 2, 1);
    unsigned char b = bytes[0];
    free(bytes);
    return x % 2 != 0 ? b & 15 : b >> 4;
}
static void OpenPaint(void) {
    Root();
    SelectEntry(0);
    Press(1);
    SelectEntry(0);
    Press(1);
    Advance(100);
    A(D("app_id") == 12, "MegaPaint launch failed");
}
static void Point(int x, int y) {
    Words r = Ui();
    MovePointer(r.v[1] * 8 + x, (r.v[2] + 1) * 8 + y);
    Press(1);
    Advance(50);
}
static void Name(char letter) {
    int k = (int)(strchr("abcdefghijklmnopqrstuvwxyz0123456789.,?!-_:/+*=() ", letter) -
                  "abcdefghijklmnopqrstuvwxyz0123456789.,?!-_:/+*=() ");
    Click(k % 10 * 4, 16 + k / 10);
    Click(24, 21);
    for (int wait = 0; wait < 500 && Paint().v[5] == 7; wait++)
        Advance(4);
    Advance(20);
}
static void FileAction(int action) {
    Click(1, 0);
    Click(2, 3 + action * 2);
}
static void SaveAs(char letter) {
    FileAction(3);
    Name(letter);
}
/** Edit both supported canvas sizes through actual UI input, then reopen them. */
void scenario_paint(void) {
    int heap = D("heap_used");
    Hash desktopPalettes = WordsHash(W("palette_words", 64), 64);
    PressFrames = 4;
    SettleFrames = 20;
    PointerTolerance = 1;
    PointerSteps = 1400;
    PointerSettle = 2;
    RootAttempts = 4;
    cJSON *checks = cJSON_CreateArray();
    OpenPaint();
    A(Position().v[1] == 1, "MegaPaint must start with direct canvas-to-menu pointer navigation");
    for (int preset = 0; preset < 4; preset++) {
        Click(11, 0);
        Click(12, 11 + preset * 2);
        Advance(40);
        int size = 1 << preset;
        A(CursorIs(size, size, 1, 1), mb_fmt("Cursor does not match brush preset %d", size));
    }
    Click(11, 0);
    Click(12, 11);
    int idleWork = D("work_last");
    A(Ui().v[2] == 1 && Ui().v[4] == 27 && Paint().v[0] == 320 && Paint().v[1] == 208 &&
          Canvas().v[0] == 320 && Canvas().v[1] == 208 && Canvas().v[4] == 1,
      "Default is not the recovered full native canvas");
    Point(200, 2);
    Words first = Paint(), r = Ui();
    MovePointer(r.v[1] * 8 + 220, (r.v[2] + 1) * 8 + 12);
    Advance(20);
    uint32_t firstRgb = core_pixel(r.v[1] * 8 + first.v[2], (r.v[2] + 1) * 8 + first.v[3]);
    MovePointer((r.v[1] + 21) * 8 + 4, r.v[2] * 8 + 4);
    Advance(20);
    A(core_pixel(r.v[1] * 8 + first.v[2], (r.v[2] + 1) * 8 + first.v[3]) == firstRgb,
      "Menu hover overwrote the first canvas row");
    Hash menuHash = PictureHash();
    for (int id = 1; id <= 4; id++) {
        int x = (id - 1) * 10 + 1;
        Click(x, 0);
        A(Paint().v[5] == id, mb_fmt("Menu did not open: %d", id));
        Click(x, 0);
        A(Paint().v[5] == 0, mb_fmt("Menu did not toggle closed: %d", id));
    }
    A(hash_equal(PictureHash(), menuHash), "Menu toggling changed the picture");
    Click(1, 0);
    Click(21, 0);
    A(Paint().v[5] == 3, "Selecting another heading did not switch menus");
    Point(200, 196);
    A(Paint().v[5] == 0 && hash_equal(PictureHash(), menuHash),
      "Outside click did not dismiss without drawing");
    cJSON_AddItemToArray(
        checks, cJSON_CreateString(
                    "Menu hover, heading toggle/switch, outside dismissal and unchanged picture"));
    uint32_t rendered[16] = {0};
    for (int colour = 0; colour < 16; colour++) {
        Click(21, 0);
        Advance(80);
        r = Ui();
        int col = colour % 8, row = colour / 8;
        uint32_t swatch =
            core_pixel((r.v[1] + 9 + col * 3) * 8 + 3, (r.v[2] + 3 + row * 3) * 8 + 3);
        MovePointer((r.v[1] + 8 + col * 3) * 8 + 3, (r.v[2] + 3 + row * 3) * 8 + 3);
        Advance(10);
        A(core_pixel((r.v[1] + 9 + col * 3) * 8 + 3, (r.v[2] + 3 + row * 3) * 8 + 3) == swatch,
          mb_fmt("Hover erased colour square %d", colour));
        if (colour == 15)
            Capture("colours-grid");
        Click(10 + col * 3, 3 + row * 3);
        A(Paint().v[6] == colour, mb_fmt("Colour square selection failed: %d", colour));
        Point(80 + colour * 10, 80);
        Words p = Paint();
        r = Ui();
        MovePointer((r.v[1] + 21) * 8 + 4, r.v[2] * 8 + 4);
        Advance(20);
        uint32_t drawn = core_pixel(r.v[1] * 8 + p.v[2], (r.v[2] + 1) * 8 + p.v[3]);
        A(drawn == swatch,
          mb_fmt("Canvas RGB differs from swatch %d: %u vs %u", colour, drawn, swatch));
        rendered[colour] = drawn;
    }
    A((rendered[5] & 65535) == 0 && rendered[5] > 0, "Default red is not red");
    A((rendered[8] & 16711935) == 0 && rendered[8] > 0, "Default green is not green");
    cJSON_AddItemToArray(
        checks,
        cJSON_CreateString("All 16 swatches, hover preservation and rendered RGB equality"));
    Click(11, 0);
    Click(12, 9);
    Click(4, 8);
    Click(21, 0);
    Click(12, 3);
    Press(8);
    Press(1);
    Press(7);
    Press(1);
    A(Paint().v[9] == 1 && Paint().v[8] > 0, "Controller pencil did not draw");
    Click(11, 0);
    Click(12, 15);
    A(Paint().v[7] == 4, "Four-pixel brush preset failed");
    Click(21, 0);
    Click(28, 3);
    A(Paint().v[6] == 6, "Sixteen-colour selection failed");
    Point(200, 140);
    Words at = Paint();
    A(Pixel(at.v[2] - 2, at.v[3] - 2) == 6 && Pixel(at.v[2] + 1, at.v[3] + 1) == 6 &&
          Pixel(at.v[2] + 2, at.v[3] + 2) != 6,
      "Four-pixel brush is not centred");
    A(CursorIs(4, 4, 1, 1), "Cursor does not match centred footprint");
    Capture("brush-cursor");
    Click(21, 0);
    Click(9, 9);
    A(Paint().v[5] == 6, "Palette editor failed");
    int old = W("paint_picture", 1, 18).v[0];
    Click(2, 7);
    Click(12, 11);
    Click(2, 13);
    A(W("paint_picture", 1, 18).v[0] != old, "RGB333 palette edit did not change palette");
    Point(200, 196);
    at = Paint();
    A(Pixel(at.v[2], at.v[3]) == 6, "Recovered canvas rows are not editable");
    Hash smallHash = PictureHash();
    Capture("native-320x208");
    FileAction(2);
    Name('p');
    int smallBytes = W("disk", 1, 2).v[0];
    A(smallBytes > 270 && smallBytes < 3592 && Paint().v[9] == 0, "Compressed picture save failed");
    Click(31, 0);
    Click(24, 7);
    A(Paint().v[4] == 2, "Zoom In failed");
    Advance(100);
    Capture("zoom-two");
    A(CursorIs(4, 8, 1, 1), "Brush cursor did not scale with zoom");
    Click(11, 0);
    Click(12, 17);
    Click(31, 0);
    Click(24, 7);
    Click(31, 0);
    Click(24, 7);
    Advance(100);
    A(CursorIs(8, 64, 4, 4), "Large zoom footprint did not use bounded corner markers");
    Capture("brush-cursor-zoom-eight");
    Click(31, 0);
    Click(24, 9);
    Click(31, 0);
    Click(24, 9);
    Click(11, 0);
    Click(12, 15);
    A(hash_equal(PictureHash(), smallHash), "Zoom changed pixels/palette");
    Click(31, 0);
    Click(24, 9);
    A(Paint().v[4] == 1, "Zoom Out failed");
    Click(31, 0);
    Click(24, 3);
    A(Paint().v[4] == 1, "Native-size Fit failed");
    FileAction(1);
    Click(2, 5);
    for (int wait = 0; wait < 500 && Paint().v[5] == 8; wait++)
        Advance(4);
    Advance(60);
    A(hash_equal(PictureHash(), smallHash), "Native image did not reopen pixel-perfectly");
    Click(11, 0);
    Click(12, 9);
    Press(0);
    A(hash_equal(PictureHash(), smallHash) && Paint().v[5] == 0,
      "B did not cancel Clear and return to drawing");
    FileAction(0);
    Click(2, 7);
    A(Paint().v[0] == 512 && Paint().v[1] == 256, "Larger logical image creation failed");
    Click(31, 0);
    Click(24, 11);
    Press(8);
    A(Position().v[1] == 2, "Controller pan mode failed");
    Advance(100, 7);
    Advance(2);
    Advance(80, 5);
    Advance(2);
    A(Canvas().v[2] == 192 && Canvas().v[3] == 48, "Pan did not reach image edge");
    Click(11, 0);
    Click(12, 3);
    Point(320, 208);
    A(Paint().v[2] == 511 && Paint().v[3] == 255 && Pixel(511, 255) == Paint().v[6],
      "Largest image corner is not editable");
    Hash largeHash = PictureHash();
    Capture("panned-512x256-corner");
    Click(31, 0);
    Click(24, 3);
    A(Paint().v[4] == 0, "Larger-image Fit overview failed");
    Advance(100);
    Capture("fit-large");
    Click(31, 0);
    A(Paint().v[5] == 4, "View did not reopen after Fit");
    Click(24, 5);
    A(Paint().v[4] == 1, "1:1 view failed");
    for (const char *letters = "qrstu"; *letters; letters++) {
        char letter = *letters;
        SaveAs(letter);
        A(Paint().v[5] == 0 && Paint().v[9] == 0, mb_fmt("Save As failed: %d", letter));
    }
    Point(12, 12);
    Hash unsaved = PictureHash(), diskBefore = DiskHash();
    SaveAs('v');
    A(Paint().v[5] == 10 && Paint().v[9] == 1,
      "Full disk did not report error and retain dirty state");
    A(hash_equal(PictureHash(), unsaved) && hash_equal(DiskHash(), diskBefore),
      "Failed save changed document or existing files");
    Capture("insufficient-space");
    Press(0);
    A(Paint().v[5] == 0 && D("app_id") == 12, "B error cancellation closed app");
    FileAction(1);
    Click(4, 8);
    Click(2, 7);
    for (int wait = 0; wait < 500 && Paint().v[5] == 8; wait++)
        Advance(4);
    Advance(60);
    A(hash_equal(PictureHash(), largeHash), "512x256 image did not reopen pixel-perfectly");
    cJSON_AddItemToArray(
        checks, cJSON_CreateString(
                    "Native 320x208 and 512x256 edge editing; centred brush; RGB palette; integer "
                    "zoom/Fit/pan; lossless saves and failure atomicity"));
    Click(11, 0);
    Click(12, 7);
    Point(12, 12);
    for (int n = 0; n < 2000 && Paint().v[10] != 0; n++)
        Advance(4);
    A(Paint().v[10] == 0 && Pixel(0, 0) == Paint().v[6], "Bounded Fill failed");
    Click(11, 0);
    Click(12, 5);
    Point(12, 12);
    at = Paint();
    A(Pixel(at.v[2], at.v[3]) == 0, "Eraser failed");
    cJSON_AddItemToArray(checks, cJSON_CreateString("Bounded sparse flood fill and eraser"));
    Click(11, 0);
    Click(12, 9);
    Click(4, 8);
    A(Paint().v[8] == 0 && Pixel(0, 0) == 0 && Paint().v[0] == 512,
      "Clear did not retain dimensions and free detail tiles");
    int mask = (1 << 3) | (1 << 0);
    core_advance(120, mask);
    Advance(100);
    A(D("app_id") == 65535, "Universal exit did not close maximised MegaPaint");
    A(hash_equal(WordsHash(W("palette_words", 64), 64), desktopPalettes),
      "Desktop palettes were not restored");
    Capture("desktop-restored");
    for (int cycle = 1; cycle < Cycles; cycle++) {
        OpenPaint();
        core_advance(120, mask);
        Advance(50);
        A(D("app_id") == 65535, "Repeated exit failed");
    }
    A(D("heap_used") == heap && D("stack_guard_ok") == 1, "MegaPaint heap/stack cleanup failed");
    core_enable_mouse();
    Advance(240);
    MouseTo(44, 42, 30);
    core_mouse(2, 0, 0, 1);
    Advance(30);
    A(Shell().v[0] == 1, "Mouse Apps folder failed");
    MouseTo(44, 60, 30);
    core_mouse(2, 0, 0, 1);
    Advance(120);
    A(D("app_id") == 12, "Mouse MegaPaint launch failed");
    MouseTo(200, 180, 30);
    core_mouse(2, 0, 0, 1);
    Advance(30);
    at = Paint();
    A(at.v[2] > 64 && at.v[3] > 64 && Pixel(at.v[2], at.v[3]) == 1,
      "Sega Mouse did not draw a native pixel");
    Capture("sega-mouse-drawing");
    core_advance(120, mask);
    Advance(60);
    A(D("app_id") == 65535 && D("stack_guard_ok") == 1, "Mouse exit/stack failed");
    json_num(Report, "small_saved_bytes", smallBytes);
    json_str(Report, "small_picture_sha256", smallHash.text);
    json_str(Report, "large_picture_sha256", largeHash.text);
    json_set(Report, "checks", checks);
    json_num(Report, "idle_work_subticks", idleWork);
    json_str(Report, "mouse", "PASS emulated port-2 Sega Mouse");
}

void scenario_legacy_paint(void) {
    SettleFrames = 30;
    SelectEntry(5);
    Press(1);
    SelectEntry(1);
    Press(1);
    SelectEntry(0);
    Press(1);
    for (int i = 0; i < 500 && W("mb_paint_observation", 12).v[0] != 64; i++)
        Advance(4);
    Advance(120);
    Words p = W("mb_paint_observation", 12);
    A(D("app_id") == 12 && p.v[0] == 64 && p.v[1] == 64 && p.v[8] == 1,
      "MBP1 association/import failed");
    int id = W("paint_picture", 1, 38 + 63 * 2).v[0],
        word = W("paint_picture", 1, 4134 + (id - 1) * 32 + 30).v[0];
    A((word & 15) == 3, "Legacy corner pixel changed");
    Capture("legacy-mbp1-import");
    core_advance(120, (1 << 3) | (1 << 0));
    Advance(60);
    A(D("app_id") == 65535 && D("stack_guard_ok") == 1, "Legacy import exit/stack failed");
    json_str(Report, "legacy_mbp1",
             "PASS original 64x64 corner pixel, palette import and desktop association");
}
