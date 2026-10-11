/**
 * @file tests/emulator/ui.c
 * @brief Checks desktop navigation, windows, themes, Snake input, clock timing and PSG restoration.
 *
 * Unity integration fixture. Drives controller/mouse input and reads observations; no RAM
 * injection.
 */
#include "../../tools/native/runner.h"
static void MouseJump(int x, int y);
static bool int_new(IntSet *s, int v) {
    size_t n = s->count;
    int_add(s, v);
    return s->count != n;
}

static int heap;
static cJSON *metrics;

static void Record(const char *phase) {
    A(D("stack_guard_ok") == 1 && D("heap_used") == heap,
      mb_fmt("Heap/stack regression: %s", phase));
    cJSON *row = cJSON_CreateObject();
    const char *keys[] = {"version",         "heap_total",   "heap_used",      "heap_free",
                          "heap_high_water", "static_bytes", "stack_reserved", "stack_used",
                          "stack_guard_ok",  "work_last",    "work_max",       "transition_max",
                          "dirty_cells",     "tiles",        "refresh_hz",     "app_id",
                          "launch_count",    "close_count",  "audio_active"};
    for (int i = 0; i < 19; i++)
        json_num(row, keys[i], D(keys[i]));
    json_str(row, "phase", phase);
    cJSON_AddItemToArray(metrics, row);
}
static void Current(int app) {
    Record(mb_fmt("app-%d", app));
    A(D("app_id") == app, mb_fmt("Expected app %d, got %d", app, D("app_id")));
}
static void OpenFolder(int folder) {
    if (Shell().v[0] != 0)
        Press(0);
    SelectEntry(folder == 6 ? 4 : folder == 7 ? 5 : folder - 1);
    Press(1);
    A(Shell().v[0] == folder, mb_fmt("Folder did not open %d", folder));
}
static void OpenApp(int app) {
    OpenFolder(app == 1 || app == 2 ? 4 : app == 6 ? 2 : 1);
    SelectEntry(app == 2 ? 5 : app == 3 ? 3 : app == 4 ? 5 : app == 5 ? 4 : 0);
    Press(1);
    Current(app);
}
static void Close(void) {
    Press(0);
    Current(65535);
    A(D("audio_active") == 0, "Audio remained owned");
}
static Words Snake(void) {
    return W("mb_snake_observation", 416);
}
/** Countdown must finish with three audible cues before any Snake movement. */
static void Countdown(void) {
    int moves = W("mb_snake_observation", 1, 816).v[0];
    IntSet labels = {0};
    int peaks[4] = {0};
    for (int n = 0; n < Hz * 6; n++) {
        int remaining = W("mb_snake_countdown", 1).v[0];
        if (remaining == 0)
            break;
        int label = (int)ceil(remaining / 300.0);
        if (remaining < 900 && int_new(&labels, label)) {
            Advance(Round(Hz / 3.0));
            Capture(mb_fmt("snake-countdown-%d", label));
        }
        if (label >= 1 && label <= 3)
            peaks[label] = MAX(peaks[label], core_peak(20));
        Advance(1);
        A(W("mb_snake_observation", 1, 816).v[0] == moves, "Snake moved during countdown");
    }
    A(W("mb_snake_countdown", 1).v[0] == 0 && labels.count == 3,
      "Three-second countdown did not finish");
    for (int n = 1; n <= 3; n++)
        A(peaks[n] > 0, mb_fmt("Countdown did not beep %d", n));
    Advance(2);
    A(D("audio_active") == 0, "Countdown beep did not stop");
}
static void WaitMove(int moves) {
    for (int n = 0; n < 20 && W("mb_snake_observation", 1, 816).v[0] == moves; n++)
        Advance(1);
}
/** Route with controller presses using read-only observations, never state writes. */
static int RouteFood(void) {
    core_start_audio();
    int peak = 0;
    for (int route = 0; route < 200 && Snake().v[401] < 4; route++) {
        Words s = Snake();
        int width = s.v[409], cells = width * s.v[410], head = s.v[0], food = s.v[405];
        A(s.v[406] == 0, "Snake died during food route");
        A(cells > 0 && cells <= 400, "Snake dimensions exceed route workspace");
        bool blocked[400] = {false};
        int prev[400], queue[400];
        for (int i = 0; i < cells; i++)
            prev[i] = -1;
        for (int i = 1; i < s.v[400] - 1; i++)
            blocked[s.v[i]] = true;
        for (int i = 0; i < s.v[415]; i++)
            blocked[s.v[411 + i]] = true;
        int front = 0, end = 0;
        queue[end++] = head;
        prev[head] = head;
        while (front < end && prev[food] < 0) {
            int at = queue[front++], x = at % width, y = at / width, nextCells[4], count = 0;
            if (x < width - 1)
                nextCells[count++] = at + 1;
            if (y < s.v[410] - 1)
                nextCells[count++] = at + width;
            if (x > 0)
                nextCells[count++] = at - 1;
            if (y > 0)
                nextCells[count++] = at - width;
            for (int n = 0; n < count; n++) {
                int nextCell = nextCells[n];
                if (!blocked[nextCell] && prev[nextCell] < 0) {
                    prev[nextCell] = at;
                    queue[end++] = nextCell;
                }
            }
        }
        A(prev[food] >= 0, "No safe food path");
        int next = food;
        while (prev[next] != head)
            next = prev[next];
        Advance(2, next == head + 1 ? 7 : next == head - 1 ? 6 : next == head + width ? 5 : 4);
        for (int n = 0; n < 20 && Snake().v[408] == s.v[408] && Snake().v[406] == 0; n++)
            Advance(1);
        s = Snake();
        A(s.v[0] == next && s.v[406] == 0, "Snake did not follow controller turn");
        peak = MAX(peak, core_peak(10));
    }
    Words final = Snake();
    A(final.v[401] == 4 && final.v[415] == 1 && peak > 0, "Snake growth, obstacle or audio failed");
    Advance(4, 3);
    Advance(20);
    Capture("snake-level-two");
    A(Snake().v[406] == 1 && D("audio_active") == 0, "Pause did not silence cue");
    cJSON *snake = cJSON_CreateObject();
    json_num(snake, "score", final.v[401]);
    json_num(snake, "level", 2);
    json_num(snake, "obstacles", final.v[415]);
    json_str(snake, "route", "Controller-driven four-food path; no RAM writes");
    json_set(Report, "snake", snake);
    return peak;
}
void scenario_ui(void) {
    PointerSteps = 800;
    metrics = cJSON_CreateArray();
    heap = D("heap_used");
    Record("boot");
    int folders[] = {1, 2, 3, 4, 7, 6};
    for (int f = 0; f < 6; f++) {
        int folder = folders[f];
        OpenFolder(folder);
        Capture(mb_fmt("folder-%d", folder));
    }
    Press(1);
    Advance(12);
    Capture("help-getting-started");
    Press(0);
    Press(0);
    OpenApp(3);
    int add[][2] = {{1, 7}, {8, 7}, {22, 9}, {15, 7}, {15, 9}};
    for (int i = 0; i < 5; i++)
        Click(add[i][0], add[i][1]);
    A(U("calculator") == 15, "Calculator 12+3 failed");
    Capture("calculator-15");
    int divide[][2] = {{1, 11}, {1, 7}, {22, 3}, {1, 9}, {15, 9}};
    for (int i = 0; i < 5; i++)
        Click(divide[i][0], divide[i][1]);
    A(W("calculator", 1, 12).v[0] == 65529, "Divide-by-zero failed");
    Words normal = Ui();
    Chord(2, 8, 3);
    A(Ui().v[0] == 1 && Ui().v[2] == 1 && Ui().v[3] == 40 && Ui().v[4] == 27,
      "Maximised client failed");
    Capture("calculator-maximized");
    MovePointer(36 * 8 + 4, 4);
    Press(1);
    A(Ui().v[0] == 1, "Removed maximise control changes mode");
    Chord(2, 8, 3);
    Chord(2, 8, 3);
    A(hash_equal(WordsHash(Ui(), 5), WordsHash(normal, 5)), "Normal geometry not restored");
    Chord(2, 8, 3);
    Chord(2, 8, 3);
    A(Ui().v[0] == 2 && Ui().v[4] == 28, "Fullscreen client failed");
    Capture("calculator-fullscreen");
    Chord(Round(Hz / 2.0), 3, 0);
    Current(3);
    Chord(Hz, 3, 0);
    Current(65535);
    A(Shell().v[0] == 1 && Shell().v[1] == 3, "Folder focus lost");
    OpenApp(1);
    Click(1, 10);
    Capture("appearance");
    for (int theme = 0; theme <= 4; theme++) {
        Click(1, 1 + theme * 2);
        A(Ui().v[5] == theme, "Theme preview failed");
        Capture(mb_fmt("theme-%d-preview", theme));
    }
    Click(21, 6);
    A(Ui().v[5] == 0, "Theme Cancel failed");
    Click(1, 10);
    Click(1, 3);
    Click(21, 3);
    A(Ui().v[5] == 1, "Theme Apply failed");
    Close();
    Capture("graphite-desktop");
    Hash palettes = WordsHash(W("palette_words", 64), 64);
    OpenApp(6);
    A(Ui().v[0] == 1, "Snake did not maximise");
    Capture("snake-maximized");
    Countdown();
    Words start = Snake();
    int head = start.v[0], moves = start.v[408];
    Advance(2, 7);
    core_advance(2, (1 << 7) | (1 << 4));
    Advance(2, 6);
    Words state = Snake();
    A(state.v[408] == moves && state.v[403] == 3 && state.v[404] == 2,
      "Rapid Up/Left turns not queued");
    WaitMove(moves);
    state = Snake();
    A(state.v[0] == head - state.v[409] && state.v[402] == 3, "Buffered Up failed");
    WaitMove(moves + 1);
    state = Snake();
    A(state.v[0] == head - state.v[409] - 1 && state.v[402] == 2, "Buffered Left failed");
    Advance(2, 6);
    core_advance(2, (1 << 6) | (1 << 4));
    A(W("mb_snake_observation", 1, 806).v[0] == 3, "Held Left swallowed new Up");
    WaitMove(moves + 2);
    state = Snake();
    A(state.v[0] == head - 2 * state.v[409] - 1 && state.v[402] == 3, "Overlapping Up failed");
    moves = W("mb_snake_observation", 1, 816).v[0];
    Advance(Round(Hz / 2.0), 4);
    Advance(Hz - Round(Hz / 2.0), 6);
    A(W("mb_snake_observation", 1, 816).v[0] - moves == 5, "Snake initial speed failed");
    Press(3);
    A(W("mb_snake_observation", 1, 812).v[0] == 1, "Start pause failed");
    Hash paused = WordsHash(W("mb_snake_observation", 411), 411);
    Advance(Hz);
    A(hash_equal(WordsHash(W("mb_snake_observation", 411), 411), paused), "Paused snake changed");
    Chord(2, 8, 3);
    Capture("snake-fullscreen");
    A(W("mb_snake_observation", 2, 818).v[0] == 20 && W("mb_snake_observation", 2, 818).v[1] == 13,
      "Snake fullscreen dimensions failed");
    Press(3);
    Advance(Hz * 3);
    A(W("mb_snake_observation", 1, 812).v[0] == 2, "Wall collision failed");
    Words dead = Snake();
    for (int i = 0; i < dead.v[400]; i++) {
        int at = dead.v[i], word = W("field", 1, at & 65534).v[0];
        A(((at & 1) != 0 ? word & 255 : word >> 8) == 0, "Dead body remained in occupancy");
    }
    Advance(Hz);
    Capture("snake-dead-cleared");
    Press(1);
    A(W("mb_snake_observation", 1, 812).v[0] == 0, "Snake restart failed");
    Countdown();
    int snakePeak = RouteFood();
    Chord(Hz, 3, 0);
    Current(65535);
    A(Ui().v[5] == 1 && hash_equal(WordsHash(W("palette_words", 64), 64), palettes),
      "Snake theme/palette restoration failed");
    OpenApp(5);
    Focus();
    core_start_audio();
    Press(1);
    Advance(Round(Hz / 2.0));
    int tonePeak = core_peak(10);
    A(tonePeak > 0 && D("audio_active") == 1, "PSG tone failed");
    core_save_audio(mb_fmt("%s/%s", Output, "psg-tone.wav"));
    Click(8, 9);
    Advance(Round(Hz / 2.0));
    A(core_peak(10) <= 2 && D("audio_active") == 0, "Sound Stop failed");
    Click(16, 6);
    Advance(Round(Hz / 2.0));
    A(core_peak(10) > 0 && D("audio_active") == 2, "Noise failed");
    Close();
    Advance(Round(Hz / 2.0));
    int mutePeak = core_peak(10);
    A(mutePeak <= 2, "Exit did not mute PSG");
    OpenApp(4);
    uint32_t seconds = U("seconds");
    Advance(Hz);
    A(U("seconds") - seconds == 1, "Region timebase failed");
    Press(1);
    Press(3);
    Current(4);
    Hash runtime = Capture("clock-runtime");
    Advance(Hz + 4);
    A(!hash_equal(Capture("clock-runtime-after"), runtime), "Clock runtime paused");
    Close();
    OpenApp(4);
    Chord(2, 8, 3);
    Chord(2, 8, 3);
    Click(33, 26);
    Current(65535);
    for (int app = 1; app <= 6; app++) {
        OpenApp(app);
        Capture(mb_fmt("app-%d", app));
        Close();
    }
    for (int n = 0; n < Cycles; n++) {
        OpenApp(6);
        Chord(2, 8, 3);
        Chord(2, 8, 3);
        Chord(Hz, 3, 0);
        Current(65535);
    }
    Record("after");
    Capture("restored");
    core_enable_mouse();
    Advance(240);
    Words before = Position();
    core_mouse(2, 5, 4, 0);
    Advance(18);
    Words after = Position();
    A(after.v[1] == 1 && after.v[2] > before.v[2] && after.v[3] > before.v[3],
      "Sega Mouse movement failed");
    MouseJump(40, 42);
    core_mouse(2, 0, 0, 1);
    Advance(18);
    A(Shell().v[0] == 1, "Mouse left Apps failed");
    core_mouse(2, 0, 0, 2);
    Advance(18);
    A(Shell().v[0] == 0, "Mouse right Back failed");
    Capture("mouse-desktop");
    MouseJump(40, 42);
    core_mouse(2, 0, 0, 1);
    Advance(18);
    MouseJump(40, 110);
    core_mouse(2, 0, 0, 1);
    Advance(18);
    Current(3);
    Words rect = Ui();
    MouseJump(100, (rect.v[2] - 1) * 8 + 4);
    Words origin = Ui();
    core_mouse(2, 0, 0, 1);
    core_mouse(1, 16, 16, 1);
    Advance(18);
    A(Ui().v[1] > origin.v[1] && Ui().v[2] > origin.v[2], "Mouse title drag failed");
    Capture("mouse-window-drag");
    Close();
    OpenApp(3);
    Chord(2, 8, 3);
    MovePointer(38 * 8 + 4, 4);
    Press(1);
    Current(65535);
    json_set(Report, "metrics", metrics);
    cJSON *audio = cJSON_CreateObject();
    json_num(audio, "tone_peak", tonePeak);
    json_num(audio, "mute_peak", mutePeak);
    json_num(audio, "snake_cue_peak", snakePeak);
    json_set(Report, "audio", audio);
    cJSON *limits = cJSON_CreateArray();
    cJSON_AddItemToArray(limits, cJSON_CreateString("Physical Sega Mouse/hardware remain manual."));
    cJSON_AddItemToArray(
        limits,
        cJSON_CreateString("Work subticks are coarse; no CPU-cycle or VBlank deadline claim."));
    cJSON_AddItemToArray(limits,
                         cJSON_CreateString("SAVE, H32, PAL240 and raster effects unavailable."));
    json_set(Report, "limitations", limits);
}
static void MouseJump(int x, int y) {
    Words p = Position();
    core_mouse(1, x - p.v[2], y - p.v[3], 0);
    Advance(18);
}
