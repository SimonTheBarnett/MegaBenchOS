/**
 * @file tools/native/checks.c
 * @brief Runs Unity checks for generated contracts, source boundaries, ROMs and music.
 *
 * Host-only C99 tooling; excluded from the console ROM.
 * Invalid inputs are isolated with a local trap; no OS state is mocked.
 */
#include "host.h"
#include <ctype.h>
static int checks;
static void check(bool value, const char *message) {
    checks++;
    REQUIRE(value, "%s", message);
}
#define REJECT(expression, message)                                                                \
    do {                                                                                           \
        jmp_buf trap;                                                                              \
        mb_trap = &trap;                                                                           \
        bool rejected = false;                                                                     \
        if (setjmp(trap) == 0) {                                                                   \
            expression;                                                                            \
        } else {                                                                                   \
            char *error = mb_copy(mb_error);                                                       \
            mb_trap = NULL;                                                                        \
            REQUIRE(strstr(error, message), "Unexpected rejection: %s", error);                    \
            free(error);                                                                           \
            rejected = true;                                                                       \
        }                                                                                          \
        mb_trap = NULL;                                                                            \
        check(rejected, "Invalid input accepted: " message);                                       \
    } while (0)
static bool source_file(const char *path) {
    const char *dot = strrchr(path, '.');
    return dot && (!strcmp(dot, ".c") || !strcmp(dot, ".h"));
}
static unsigned char *vgm_source(const unsigned char *commands, size_t count, size_t *size) {
    *size = 64 + count;
    unsigned char *b = mb_alloc(*size);
    memcpy(b, "Vgm ", 4);
    mb_put_le32(b + 4, (uint32_t)*size - 4);
    mb_put_le32(b + 8, 0x160);
    mb_put_le32(b + 44, 7670454);
    mb_put_le32(b + 52, 12);
    memcpy(b + 64, commands, count);
    return b;
}
static void test_audio_provenance_and_analysis(void) {
    music_check();
    audio_check();
    checks += 2;
    const char *base = "third_party/music/beatscribe";
    cJSON *p = json_read(mb_path(mb_fmt("%s/provenance.json", base))),
          *track = cJSON_GetArrayItem(json_get(p, "tracks"), 0);
    const char *path = mb_path(mb_fmt("%s/%s", base, json_s(track, "filename")));
    size_t n;
    unsigned char *bytes = mb_read(path, &n);
    check(!strcmp(mb_hash_file(path).text, json_s(track, "sha256")) &&
              !strcmp(mb_hash_file(
                          mb_path(mb_fmt("%s/%s", base, json_s(p, "license_evidence_filename"))))
                          .text,
                      json_s(p, "license_evidence_sha256")) &&
              vgm_validate(bytes, n) == (uint64_t)json_number(json_get(track, "samples")),
          "Licensed Charge fixture changed");
    free(bytes);
    cJSON_Delete(p);
    unsigned char commands[] = {0x90, 0, 2, 0, 0x2a, 0x94, 0, 0x66};
    bytes = vgm_source(commands, sizeof commands, &n);
    check(vgm_validate(bytes, n) == 0, "Bounded DAC setup failed");
    bytes[66] = 3;
    REJECT(vgm_validate(bytes, n), "Only YM2612 DAC");
    bytes[66] = 2;
    bytes[68] = 0x2b;
    REJECT(vgm_validate(bytes, n), "Only YM2612 DAC");
    free(bytes);
    unsigned char short_command[] = {0x95, 0};
    bytes = vgm_source(short_command, sizeof short_command, &n);
    REJECT(vgm_validate(bytes, n), "Truncated command");
    free(bytes);
    bytes = vgm_source(commands, 5, &n);
    REJECT(vgm_validate(bytes, n), "no end command");
    free(bytes);
}
static void test_storage_vendor_and_rom(void) {
    vendor_check("unity");
    vendor_check("littlefs");
    vendor_check("cjson");
    checks += 3;
    cJSON *rom = rom_info(mb_path("dist/MegaBenchOS.bin")),
          *report = json_read(mb_path("dist/build-report.json"));
    check(!strcmp(json_s(report, "source_sha256"), source_hash(false, false, false).text) &&
              !strcmp(json_s(rom, "sha256"), json_s(report, "sha256")),
          "Stale ROM: build before tests");
    size_t n;
    unsigned char *bytes = mb_read(mb_path("dist/MegaBenchOS.bin"), &n);
    bytes[1024] ^= 1;
    REJECT(rom_bytes(bytes, n), "checksum");
    bytes[1024] ^= 1;
    memset(bytes + 4, 0, 4);
    REJECT(rom_bytes(bytes, n), "reset vectors");
    free(bytes);
    unsigned char tiny[] = {0, 1, 2};
    REJECT(rom_bytes(tiny, sizeof tiny), "full header");
    cJSON_Delete(rom);
    cJSON_Delete(report);
}
/** Scan identifiers rather than matching substrings inside longer names. */
static bool token_prefix(const char *text, const char *prefix, bool call) {
    size_t length = strlen(prefix);
    for (const char *p = text; *p; p++) {
        if (isalnum((unsigned char)*p) || *p == '_') {
            const char *start = p;
            while (isalnum((unsigned char)*p) || *p == '_')
                p++;
            if ((size_t)(p - start) > length && !strncmp(start, prefix, length)) {
                const char *end = p;
                while (isspace((unsigned char)*end))
                    end++;
                if (!call || *end == '(')
                    return true;
            }
            if (!*p)
                break;
        }
    }
    return false;
}
static void includes(const char *file, const char *text, bool app) {
    for (const char *p = text; *p; p++) {
        if (*p != '#')
            continue;
        const char *at = p + 1;
        while (isspace((unsigned char)*at))
            at++;
        if (strncmp(at, "include", 7))
            continue;
        at += 7;
        while (isspace((unsigned char)*at))
            at++;
        if (*at != '<' && *at != '"')
            continue;
        char end = *at == '<' ? '>' : '"';
        const char *start = ++at;
        while (*at && *at != end && *at != '\n')
            at++;
        REQUIRE(*at == end, "Malformed include in %s", file);
        char name[512];
        size_t n = (size_t)(at - start);
        REQUIRE(n < sizeof name, "Include name exceeds bound");
        memcpy(name, start, n);
        name[n] = 0;
        if (app) {
            if (!strcmp(name, "megabenchos.h"))
                continue;
            const char *slash = strrchr(file, '/');
            REQUIRE(slash, "App path has no directory");
            char directory[4096];
            size_t length = (size_t)(slash - file);
            memcpy(directory, file, length);
            directory[length] = 0;
            REQUIRE(!strstr(name, "..") && name[0] != '/' && !strchr(name, ':') &&
                        mb_exists(mb_path(mb_fmt("%s/%s", directory, name))),
                    "Non-public/nonlocal app include: %s in %s", name, file);
        } else
            REQUIRE(!strstr(name, "ramdisk.h") && !strstr(name, "backend.h") &&
                        !(strstr(name, "lfs") && strstr(name, ".h")),
                    "Private filesystem include outside storage: %s", file);
    }
}
static void test_filesystem_service_boundary(void) {
    const char *folders[] = {"src", "kernel", "desktop", "apps"};
    for (int i = 0; i < 4; i++) {
        Paths paths = {0};
        mb_scan(&paths, folders[i], true);
        for (size_t j = 0; j < paths.count; j++) {
            const char *file = paths.items[j];
            if (source_file(file) && strncmp(file, "kernel/storage/", 15)) {
                char *text = mb_text(mb_path(file));
                REQUIRE(!token_prefix(text, "lfs_", false) && !token_prefix(text, "mb_ram_", false),
                        "Private filesystem access outside storage: %s", file);
                includes(file, text, false);
                free(text);
            }
            free(paths.items[j]);
        }
        free(paths.items);
    }
    checks++;
}
static void test_sdk_application_compilation(void) {
    cJSON *cfg = mb_config();
    const char *sgdk = json_s(cfg, "sgdk"), *gcc = mb_copy(mb_fmt("%s/bin/gcc.exe", sgdk)),
               *bin = mb_copy(mb_fmt("-B%s/bin/", sgdk));
    for (char *p = (char *)bin; *p; p++)
        if (*p == '\\')
            *p = '/';
    Paths paths = {0};
    mb_scan(&paths, "apps", true);
    for (size_t i = 0; i < paths.count; i++) {
        const char *file = paths.items[i];
        if (source_file(file)) {
            char *text = mb_text(mb_path(file));
            includes(file, text, true);
            const char *prefixes[] = {"VDP_", "SYS_",  "JOY_",    "PSG_", "MEM_", "DMA_",
                                      "SPR_", "XGM2_", "YM2612_", "Z80_", "SND_"};
            for (int n = 0; n < 11; n++)
                REQUIRE(!token_prefix(text, prefixes[n], true), "App violates SDK boundary: %s",
                        file);
            free(text);
            if (strstr(file, ".c") == file + strlen(file) - 2) {
                const char *args[] = {bin,
                                      "-m68000",
                                      "-DSGDK_GCC",
                                      "-std=c99",
                                      "-Wall",
                                      "-Wextra",
                                      "-Werror",
                                      "-Wno-misleading-indentation",
                                      "-Wno-sign-compare",
                                      "-fsyntax-only",
                                      mb_fmt("-I%s/sdk/include", mb_root),
                                      mb_path(file)};
                check(mb_process(gcc, args, 12, 120, NULL) == 0,
                      mb_fmt("Public SDK compilation failed: %s", file));
            }
        }
        free(paths.items[i]);
    }
    free(paths.items);
    free((char *)bin);
    free((char *)gcc);
    cJSON_Delete(cfg);
    char *header = mb_text(mb_path("sdk/include/megabenchos.h"));
    int briefs = 0, since = 0;
    for (char *p = header; (p = strstr(p, "@brief")); p += 6)
        briefs++;
    for (char *p = header; (p = strstr(p, "@since 1")); p += 8)
        since++;
    cJSON *spec = json_read(mb_path("api/core.json"));
    int count = cJSON_GetArraySize(json_get(spec, "functions"));
    check(briefs == count && since == count, "Undocumented SDK function");
    free(header);
    cJSON_Delete(spec);
}
static void test_graphics_generated_contracts(void) {
    sdk_generate(NULL, NULL, true);
    demos_generate(true);
    char *header = mb_text(mb_path("inc/ui_assets.h")),
         *palette = strstr(header, "ui_palette[16] = {");
    REQUIRE(palette, "Missing UI palette");
    palette = strchr(palette, '{') + 1;
    int count = 0;
    while (*palette && *palette != '}') {
        char *end;
        unsigned long value = strtoul(palette, &end, 16);
        REQUIRE(end != palette && !(value & ~0xeeeUL), "Illegal palette");
        count++;
        palette = end;
        if (*palette == ',')
            palette++;
    }
    check(count == 16, "Palette must contain 16 colors");
    free(header);
    const char *kinds[] = {"incomplete", "duplicate", "palette", "geometry", "tile", "signature"};
    for (int n = 0; n < 6; n++) {
        bool api = n < 2 || n == 5;
        cJSON *input = json_read(mb_path(api ? "api/core.json" : "assets/source/ui.json"));
        if (n == 0)
            json_str(cJSON_GetArrayItem(json_get(input, "functions"), 0), "ownership", "");
        if (n == 1)
            cJSON_AddItemToArray(
                json_get(input, "functions"),
                cJSON_Duplicate(cJSON_GetArrayItem(json_get(input, "functions"), 0), true));
        if (n == 2) {
            cJSON *colors = cJSON_CreateObject();
            for (int i = 0; i < 15; i++)
                json_str(colors, mb_fmt("c%d", i), "000000");
            json_set(input, "colors", colors);
        }
        if (n == 3)
            cJSON_DeleteItemFromArray(json_get(json_get(input, "icons"), "app"), 15);
        if (n == 4)
            cJSON_ReplaceItemInArray(json_get(input, "pointer"), 0, cJSON_CreateString("90000000"));
        if (n == 5)
            json_str(cJSON_GetArrayItem(json_get(input, "functions"), 0), "signature",
                     "void mb_Bad(void)");
        const char *path = mb_path(mb_fmt("out/test-inputs/%s.json", kinds[n]));
        json_write(path, input);
        jmp_buf trap;
        mb_trap = &trap;
        bool rejected = false;
        if (setjmp(trap) == 0)
            sdk_generate(api ? path : NULL, api ? NULL : path, true);
        else
            rejected = true;
        mb_trap = NULL;
        check(rejected, mb_fmt("Invalid generator input accepted: %s", kinds[n]));
        /* Verify the naming guard itself, rather than an unrelated generated-file mismatch. */
        if (n == 5)
            check(strstr(mb_error, "Invalid signature") != NULL,
                  "Malformed function name was not rejected by signature validation");
        cJSON_Delete(input);
    }
}
void host_checks(void) {
    checks = 0;
    UNITY_BEGIN();
    Unity.TestFile = "tools/native/checks.c";
    RUN_TEST(test_filesystem_service_boundary);
    RUN_TEST(test_sdk_application_compilation);
    RUN_TEST(test_graphics_generated_contracts);
    RUN_TEST(test_audio_provenance_and_analysis);
    RUN_TEST(test_storage_vendor_and_rom);
    int failures = UNITY_END();
    Unity.CurrentTestName = NULL;
    cJSON *report = cJSON_CreateObject();
    json_str(report, "status", failures ? "FAIL" : "PASS");
    json_num(report, "host_checks", checks);
    json_num(report, "unity_tests", 5);
    json_num(report, "failures", failures);
    char timestamp[32];
    mb_timestamp(timestamp);
    json_str(report, "timestamp_utc", timestamp);
    json_str(report, "rom_sha256", mb_hash_file(mb_path("dist/MegaBenchOS.bin")).text);
    json_write(mb_path("reports/host-test-results.json"), report);
    cJSON_Delete(report);
    REQUIRE(!failures, "Host Unity checks failed");
    printf("PASS %d host validation checks\n", checks);
}
void littlefs_check(const char *path) {
    vendor_check("littlefs");
    cJSON *rows = json_read(path);
    REQUIRE(cJSON_GetArraySize(rows) == 4, "Expected four littlefs candidates");
    const char *fields[] = {"bounds_checked",          "flash_checked",
                            "old_preserved",           "io_failure_preserved",
                            "close_failure_preserved", "interrupted_preserved",
                            "corrupt_rejected"};
    cJSON *row;
    cJSON_ArrayForEach(row, rows) for (int i = 0; i < 7; i++)
        REQUIRE(json_i(row, fields[i]) != 0, "littlefs integrity failure: %s", fields[i]);
    json_write(mb_path("reports/emulator/littlefs/host.json"), rows);
    cJSON_Delete(rows);
    puts("PASS four littlefs host candidates");
}
