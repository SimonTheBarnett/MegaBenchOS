/**
 * @file tools/native/runner.c
 * @brief Runs Unity emulator fixtures, observes ROM state and records regional test evidence.
 *
 * Host-only C99 tooling; excluded from the console ROM.
 */
#include "runner.h"
int Hz, Cycles, Assertions, PressFrames, SettleFrames, PointerTolerance, PointerSteps,
    PointerSettle, RootAttempts;
char Output[4096];
const char *Scenario;
cJSON *Report;
typedef struct {
    char name[192];
    int address;
} SymbolEntry;
static SymbolEntry symbols[8192];
static size_t symbol_count;
static const char *diagnostics[] = {
    "version",      "heap_total",     "heap_used",   "heap_free",      "heap_high_water",
    "static_bytes", "stack_reserved", "stack_used",  "stack_guard_ok", "work_last",
    "work_max",     "transition_max", "dirty_cells", "tiles",          "refresh_hz",
    "app_id",       "launch_count",   "close_count", "audio_active"};
int Symbol(const char *name) {
    for (size_t i = 0; i < symbol_count; i++)
        if (!strcmp(name, symbols[i].name))
            return symbols[i].address;
    REQUIRE(false, "Missing ROM symbol %s", name);
    return 0;
}
Words(W)(const char *name, int count, int offset) {
    Words result = {{0}};
    REQUIRE(count >= 0 && count <= 512, "Word snapshot exceeds 512 words");
    core_words(Symbol(name) + offset, count, result.v);
    return result;
}
uint32_t(U)(const char *name, int offset) {
    return core_u32(Symbol(name) + offset);
}
int D(const char *name) {
    for (size_t i = 0; i < sizeof diagnostics / sizeof *diagnostics; i++)
        if (!strcmp(name, diagnostics[i]))
            return W("mb_diagnostics", 19, 0).v[i];
    REQUIRE(false, "Unknown diagnostic %s", name);
    return 0;
}
void A(bool value, const char *message) {
    Assertions++;
    REQUIRE(value, "%s", message);
}
void(Advance)(int frames, int button) {
    core_advance(frames, button < 0 ? 0 : 1u << button);
}
void Press(int button) {
    Advance(PressFrames, button);
    Advance(SettleFrames, -1);
}
void Chord(int frames, int one, int two) {
    core_advance(frames, (1u << one) | (1u << two));
    Advance(18, -1);
}
Hash Capture(const char *name) {
    return core_capture(mb_fmt("%s/%s.png", Output, name));
}
Words Shell(void) {
    return W("mb_shell_observation", 4, 0);
}
Words Ui(void) {
    return W("mb_ui_observation", 7, 0);
}
Words Position(void) {
    return W("mb_input_observation", 4, 0);
}
Hash WordsHash(Words words, int count) {
    return mb_hash(words.v, (size_t)count * sizeof(int));
}
void Focus(void) {
    if (Position().v[1] == 1)
        Press(8);
}
void SelectEntry(int index) {
    Focus();
    for (int n = 0; n < 30 && Shell().v[1] != index; n++)
        Press(7);
    A(Shell().v[1] == index, mb_fmt("Shell selection failed: %d", index));
}
void Root(void) {
    for (int n = 0; n < RootAttempts && Shell().v[0] != 0; n++)
        Press(0);
    A(Shell().v[0] == 0, "Root return failed");
}
void MovePointer(int x, int y) {
    if (!strcmp(Scenario, "creative") && Position().v[1] == 2)
        Press(3);
    if (Position().v[1] != 1)
        Press(8);
    A(Position().v[1] == 1, "Pointer mode failed");
    for (int n = 0; n < PointerSteps; n++) {
        Words p = Position();
        if (abs(p.v[2] - x) <= PointerTolerance && abs(p.v[3] - y) <= PointerTolerance) {
            Advance(PointerSettle, -1);
            return;
        }
        int b = abs(p.v[2] - x) > PointerTolerance ? (p.v[2] < x ? 7 : 6) : (p.v[3] < y ? 5 : 4);
        Advance(1, b);
    }
    REQUIRE(false, "Pointer cannot reach %d,%d", x, y);
}
void Click(int x, int y) {
    Words r = Ui();
    MovePointer((r.v[1] + x) * 8 + 4, (r.v[2] + y) * 8 + 4);
    Press(1);
}
unsigned char *ReadBytes(int address, int count) {
    REQUIRE(count >= 0 && count <= 65536 && address >= 0 && address + count <= 65536,
            "RAM byte read exceeds bounds");
    unsigned char *result = mb_alloc((size_t)count + 1);
    for (int i = 0; i < count; i++) {
        int pair[1];
        core_words((address + i) & 65534, 1, pair);
        result[i] = (unsigned char)((address + i) & 1 ? pair[0] : pair[0] >> 8);
    }
    return result;
}
const char *Text(const char *symbol, int count) {
    static char slots[8][4096];
    static unsigned slot;
    REQUIRE(count >= 0 && count < 4096, "Text read exceeds bounds");
    unsigned char *bytes = ReadBytes(Symbol(symbol), count);
    char *result = slots[slot++ % 8];
    memcpy(result, bytes, count);
    result[count] = 0;
    free(bytes);
    return result;
}
void MouseTo(int x, int y, int steps) {
    for (int n = 0; n < steps; n++) {
        Words p = Position();
        int dx = x - p.v[2], dy = y - p.v[3];
        if (abs(dx) <= 2 && abs(dy) <= 2)
            return;
        core_mouse(1, MAX(-60, MIN(60, dx)), MAX(-60, MIN(60, dy)), 0);
        Advance(12, -1);
    }
    REQUIRE(false, "Mouse target failed");
}
void int_add(IntSet *set, int value) {
    for (size_t i = 0; i < set->count; i++)
        if (set->values[i] == value)
            return;
    REQUIRE(set->count < 2048, "Integer set exceeds bounds");
    set->values[set->count++] = value;
}
void hash_add(HashSet *set, Hash value) {
    for (size_t i = 0; i < set->count; i++)
        if (hash_equal(set->values[i], value))
            return;
    REQUIRE(set->count < 2048, "Hash set exceeds bounds");
    set->values[set->count++] = value;
}
static void boot(void) {
    for (int f = 0; f < Hz * 2 && !W("mb_boot_observation", 1, 0).v[0]; f++)
        Advance(1, -1);
    Advance(mb_round(Hz / 4.0), -1);
    A(W("mb_boot_observation", 1, 0).v[0] == 1, "Boot fade-in missing");
    Capture("boot-fade-in");
    Advance(mb_round(Hz * 1.25), -1);
    A(W("mb_boot_observation", 1, 0).v[0] == 2, "Boot logo hold missing");
    Capture("boot-logo");
    Advance(mb_round(Hz * 1.25), -1);
    A(W("mb_boot_observation", 1, 0).v[0] == 3, "Boot fade-out missing");
    Capture("boot-fade-out");
    Advance(mb_round(Hz * 1.25), -1);
    A(W("mb_boot_observation", 1, 0).v[0] == 4, "Boot desktop transition missing");
    Capture("boot");
}
static void contracts(void) {
    int value = W("mb_contract_result", 1, 0).v[0];
    for (int i = 0; i < 100 && value == 1; i++) {
        Advance(120, -1);
        value = W("mb_contract_result", 1, 0).v[0];
    }
    A(value == 0x600d, mb_fmt("Target contract failure code %d", value));
    json_str(Report, "mailbox", "0x600d");
    json_str(Report, "contract_failure_codes", "2..153");
}
static void littlefs(void) {
    int value = W("mb_lfs_target_result", 1, 0).v[0];
    for (int i = 0; i < 1000 && value == 1; i++) {
        Advance(120, -1);
        value = W("mb_lfs_target_result", 1, 0).v[0];
    }
    A(value == 0x600d, mb_fmt("littlefs target failure code %d", value));
    const char *names[] = {"block",
                           "cache",
                           "medium_bytes",
                           "runtime_bytes",
                           "empty_blocks",
                           "directory_blocks",
                           "format",
                           "picture",
                           "text_and_picture",
                           "maximum_payload",
                           "file_count",
                           "replacement",
                           "old_preserved",
                           "io_failure_preserved",
                           "close_failure_preserved",
                           "interrupted_preserved",
                           "corrupt_rejected",
                           "bounds_checked",
                           "flash_checked",
                           "reads",
                           "programs",
                           "erases",
                           "syncs"};
    cJSON *rows = cJSON_CreateArray();
    for (int n = 0; n < 4; n++) {
        cJSON *row = cJSON_CreateObject();
        for (int j = 0; j < 23; j++)
            json_num(row, names[j], (int32_t)U("mb_lfs_target_rows", (n * 23 + j) * 4));
        json_num(row, "frames", U("mb_lfs_target_frames", n * 4));
        cJSON_AddItemToArray(rows, row);
    }
    A(D("stack_guard_ok") == 1, "littlefs stack guard failed");
    json_set(Report, "candidates", rows);
    json_str(Report, "mailbox", "0x600d");
    Capture("completed");
}
static void finish(bool pass) {
    json_str(Report, "status", pass ? "PASS" : "FAIL");
    json_num(Report, "assertions", Assertions);
    char timestamp[32];
    mb_timestamp(timestamp);
    json_str(Report, "timestamp_utc", timestamp);
    cJSON *d = cJSON_CreateObject();
    for (size_t i = 0; i < 19; i++)
        json_num(d, diagnostics[i], D(diagnostics[i]));
    json_set(Report, "diagnostics", d);
    if (!pass)
        json_str(Report, "error", mb_error);
    cJSON *limits = json_read(mb_path("config/budgets.json")), *warnings = cJSON_CreateArray();
    const char *keys[] = {"heap_high_water", "stack_used", "work_max"};
    for (int i = 0; i < 3; i++)
        if (D(keys[i]) > json_i(limits, i == 2 ? "work_max_subticks" : keys[i]))
            cJSON_AddItemToArray(
                warnings,
                cJSON_CreateString(mb_fmt("%s: %d exceeds advisory budget", keys[i], D(keys[i]))));
    json_set(Report, "budget_warnings", warnings);
    json_write(mb_fmt("%s/report.json", Output), Report);
    printf("%s GX %s %s (%d assertions)\n", pass ? "PASS" : "FAIL", Hz == 50 ? "pal" : "ntsc",
           Scenario, Assertions);
    cJSON_Delete(limits);
}
static void run_scenario(void) {
    boot();
    if (!strncmp(Scenario, "contracts", 9) || !strcmp(Scenario, "legacy-paint"))
        contracts();
    if (!strcmp(Scenario, "littlefs"))
        littlefs();
    else if (!strcmp(Scenario, "ui"))
        scenario_ui();
    else if (!strcmp(Scenario, "creative"))
        scenario_creative();
    else if (!strcmp(Scenario, "paint"))
        scenario_paint();
    else if (!strcmp(Scenario, "legacy-paint"))
        scenario_legacy_paint();
    else if (!strcmp(Scenario, "music") || !strcmp(Scenario, "contracts-music"))
        scenario_music();
    else if (!strncmp(Scenario, "demo", 4))
        scenario_demo(atoi(Scenario + 4));
}
void setUp(void) {}
void tearDown(void) {}
static int single(const char *scenario, const char *region, int cycles) {
    Scenario = scenario;
    Cycles = cycles;
    Hz = !strcmp(region, "pal") ? 50 : 60;
    Assertions = 0;
    PressFrames = 2;
    SettleFrames = 18;
    PointerTolerance = 3;
    PointerSteps = 900;
    PointerSettle = 6;
    RootAttempts = 5;
    symbol_count = 0;
    bool con = !strncmp(scenario, "contracts", 9) || !strcmp(scenario, "legacy-paint"),
         lfs = !strcmp(scenario, "littlefs");
    cJSON *cfg = mb_config();
    const char *dll = json_s(cfg, "genesis_plus_gx_core");
    REQUIRE(mb_exists(dll), "Configure MB_GENPLUS_CORE for automated GX checks");
    cJSON *lock = json_read(mb_path("config/toolchain.lock.json"));
    bool locked = false;
    cJSON *entry;
    Hash corehash = mb_hash_file(dll);
    cJSON_ArrayForEach(entry, lock) if (!strcmp(json_s(entry, "name"), "genesis_plus_gx_core"))
        locked = !strcmp(corehash.text, json_s(entry, "runtime_sha256"));
    REQUIRE(locked, "GX core differs from dependency lock");
    cJSON *built = json_read(mb_path(mb_fmt("dist/%sbuild-report.json", con   ? "contracts-"
                                                                        : lfs ? "littlefs-"
                                                                              : "")));
    REQUIRE(!strcmp(json_s(built, "source_sha256"), source_hash(con, lfs, false).text),
            "Stale ROM source stamp; build before emulation");
    const char *rom = mb_path(mb_fmt("dist/MegaBenchOS%s.bin", con   ? "-contracts"
                                                               : lfs ? "-littlefs"
                                                                     : ""));
    cJSON *info = rom_info(rom);
    REQUIRE(!strcmp(json_s(info, "sha256"), json_s(built, "sha256")),
            "ROM differs from build report");
    snprintf(Output, sizeof Output, "%s/reports/emulator/%s/%s", mb_root, scenario, region);
    mb_mkdir(Output);
    char *source = mb_text(mb_path(con   ? "out/contracts/out/symbol.txt"
                                   : lfs ? "out/littlefs-project/out/symbol.txt"
                                         : "out/project/out/symbol.txt"));
    for (char *line = strtok(source, "\n"); line; line = strtok(NULL, "\n")) {
        unsigned address;
        char kind, name[192];
        if (sscanf(line, "%x %c %191s", &address, &kind, name) == 3) {
            REQUIRE(symbol_count < 8192, "Too many ROM symbols");
            snprintf(symbols[symbol_count].name, 192, "%s", name);
            symbols[symbol_count++].address = (int)(address & 65535);
        }
    }
    free(source);
    core_open(dll, rom, Hz == 50);
    Report = cJSON_CreateObject();
    json_str(Report, "region", region);
    json_str(Report, "scenario", scenario);
    json_num(Report, "cycles", cycles);
    json_str(Report, "emulator", core.emulator);
    json_str(Report, "core_sha256", corehash.text);
    json_str(Report, "rom_sha256", json_s(built, "sha256"));
    json_str(Report, "source_sha256", json_s(built, "source_sha256"));
    json_str(Report, "hardware", "MANUAL");
    json_str(Report, "blastem", "MANUAL: no input/state automation supplied");
    UNITY_BEGIN();
    char test_name[128];
    snprintf(test_name, sizeof test_name, "%s/%s", scenario, region);
    UnityDefaultTestRun(run_scenario, test_name, __LINE__);
    bool pass = Unity.TestFailures == 0;
    if (!pass)
        Capture("failure");
    finish(pass);
    int failures = UNITY_END();
    Unity.CurrentTestName = NULL;
    core_close();
    cJSON_Delete(cfg);
    cJSON_Delete(lock);
    cJSON_Delete(built);
    cJSON_Delete(info);
    cJSON_Delete(Report);
    Report = NULL;
    return failures ? 1 : 0;
}
int emulator_main(const char *scenario, const char *region, int cycles) {
    const char *scenarios[] = {"ui",    "creative", "paint",  "music",           "demo7",
                               "demo8", "demo9",    "demo10", "contracts-music", "legacy-paint"};
    bool known =
        !strcmp(scenario, "all") || !strcmp(scenario, "contracts") || !strcmp(scenario, "littlefs");
    for (int i = 0; i < 10; i++)
        known |= !strcmp(scenario, scenarios[i]);
    REQUIRE(known, "Unknown scenario %s", scenario);
    REQUIRE(!strcmp(region, "both") || !strcmp(region, "pal") || !strcmp(region, "ntsc"),
            "Invalid region");
    REQUIRE(cycles >= 1 && cycles <= 100, "Cycles must be 1..100");
    if (strcmp(scenario, "all") && strcmp(region, "both"))
        return single(scenario, region, cycles);
    cJSON *cfg = mb_config(), *summary = cJSON_CreateObject();
    if (!mb_exists(json_s(cfg, "genesis_plus_gx_core"))) {
        json_str(summary, "status", "SKIPPED");
        json_str(summary, "reason", "Configure MB_GENPLUS_CORE; no emulator pass claimed");
        json_write(mb_path("reports/emulator/summary.json"), summary);
        puts("SKIPPED GX: configure MB_GENPLUS_CORE");
        cJSON_Delete(cfg);
        cJSON_Delete(summary);
        return 0;
    }
    cJSON *results = cJSON_CreateArray();
    bool pass = true;
    char exe[4096];
    GetModuleFileNameA(NULL, exe, sizeof exe);
    const char *regions[] = {"ntsc", "pal"};
    for (int i = 0; i < (!strcmp(scenario, "all") ? 10 : 1); i++)
        for (int r = 0; r < (!strcmp(region, "both") ? 2 : 1); r++) {
            const char *item = !strcmp(scenario, "all") ? scenarios[i] : scenario,
                       *timing = !strcmp(region, "both") ? regions[r] : region;
            const char *args[] = {"emulator", "--root", mb_root,    "--scenario",        item,
                                  "--region", timing,   "--cycles", mb_fmt("%d", cycles)};
            bool ok;
            jmp_buf timeout;
            mb_trap = &timeout;
            const char *log = mb_path(mb_fmt("reports/emulator/%s/%s/process.log", item, timing));
            if (setjmp(timeout) == 0)
                ok = mb_process(exe, args, 9, 240, log) == 0;
            else {
                fprintf(stderr, "%s\n", mb_error);
                ok = false;
            }
            mb_trap = NULL;
            cJSON *row = cJSON_CreateObject();
            json_str(row, "scenario", item);
            json_str(row, "region", timing);
            json_str(row, "status", ok ? "PASS" : "FAIL");
            printf("%s GX %s %s\n", ok ? "PASS" : "FAIL", timing, item);
            if (!ok && mb_exists(log)) {
                char *evidence = mb_text(log);
                fprintf(stderr, "%s", evidence);
                free(evidence);
            }
            cJSON_AddItemToArray(results, row);
            pass &= ok;
        }
    json_str(summary, "status", pass ? "PASS" : "FAIL");
    json_str(summary, "blastem", "MANUAL");
    json_str(summary, "hardware", "MANUAL");
    json_set(summary, "results", results);
    json_write(mb_path("reports/emulator/summary.json"), summary);
    cJSON_Delete(cfg);
    cJSON_Delete(summary);
    return pass ? 0 : 1;
}
