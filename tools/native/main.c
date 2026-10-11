/**
 * @file tools/native/main.c
 * @brief One native host executable serves Make, Windows helpers and emulator tests.
 *
 * Host-only C99 tooling; excluded from the console ROM.
 * Arguments are passed separately to child tools; shell text is not assembled
 * from filenames. Every failed check returns a nonzero process exit status.
 */
#include "host.h"
typedef struct {
    const char *key, *value;
} Option;
static Option options[32];
static int option_count;
static const char *option(const char *name, const char *fallback) {
    for (int i = 0; i < option_count; i++)
        if (!strcmp(name, options[i].key))
            return options[i].value;
    return fallback;
}
static const char *required(const char *name) {
    const char *value = option(name, NULL);
    REQUIRE(value && *value, "Missing --%s", name);
    return value;
}
static bool flag(const char *name) {
    const char *value = option(name, "0");
    REQUIRE(!strcmp(value, "0") || !strcmp(value, "1"), "--%s must be 0 or 1", name);
    return !strcmp(value, "1");
}
static void allowed(const char *keys) {
    for (int i = 0; i < option_count; i++) {
        if (!strcmp(options[i].key, "root"))
            continue;
        const char *p = keys;
        bool found = false;
        while (*p) {
            while (*p == ' ')
                p++;
            const char *end = strchr(p, ' ');
            size_t n = end ? (size_t)(end - p) : strlen(p);
            if (strlen(options[i].key) == n && !strncmp(p, options[i].key, n))
                found = true;
            p += n;
        }
        REQUIRE(found, "Unknown --%s", options[i].key);
    }
}
void format_sources(bool check);
void clean_workflow(bool skip_emulator);
int main(int argc, char **argv) {
    REQUIRE(GetCurrentDirectoryA(sizeof mb_root, mb_root), "Cannot determine repository path");
    if (argc < 2 || !strcmp(argv[1], "--help")) {
        puts("MegaBenchOS native C host tools\n  check | music-check | generate | "
             "check-generated\n  emulator --scenario "
             "all|ui|creative|paint|music|demo7..demo10|contracts|contracts-music|legacy-paint|"
             "littlefs --region both|ntsc|pal --cycles 1\n  rom-info --input path | publish-rom "
             "--input path --output path\n  source-hash --contracts 0|1 --littlefs 0|1 "
             "--music-smoke 0|1\n  vgm-info --input path | littlefs-check --input path\n  "
             "native-audio --region ntsc|pal --raw path --frames n,n,n,n,n\n  format | lint | "
             "clean-workflow --skip-emulator 0|1\nAll commands accept --root path. Nonzero exit "
             "means failure.");
        return 0;
    }
    REQUIRE(argc % 2 == 0, "Expected --option value pairs");
    for (int i = 2; i < argc; i += 2) {
        REQUIRE(i + 1 < argc && !strncmp(argv[i], "--", 2) && strlen(argv[i]) > 2 &&
                    option_count < 32,
                "Expected --option value pairs");
        REQUIRE(!option(argv[i] + 2, NULL), "Duplicate option %s", argv[i]);
        options[option_count++] = (Option){argv[i] + 2, argv[i + 1]};
    }
    const char *root = option("root", NULL);
    if (root) {
        REQUIRE(GetFullPathNameA(root, sizeof mb_root, mb_root, NULL) > 0, "Invalid root path");
        REQUIRE(SetCurrentDirectoryA(mb_root), "Cannot enter repository");
    }
    const char *command = argv[1];
    if (!strcmp(command, "check")) {
        allowed("");
        host_checks();
    } else if (!strcmp(command, "music-check")) {
        allowed("");
        music_check();
        puts("PASS five pinned CC0 sources and XGM2 assets");
    } else if (!strcmp(command, "generate") || !strcmp(command, "check-generated")) {
        allowed("api assets");
        bool check = !strcmp(command, "check-generated");
        sdk_generate(option("api", NULL), option("assets", NULL), check);
        demos_generate(check);
        puts(check ? "PASS generated SDK and artwork" : "Generated SDK and artwork");
    } else if (!strcmp(command, "emulator")) {
        allowed("scenario region cycles");
        const char *cycles = option("cycles", "1");
        char *end;
        long count = strtol(cycles, &end, 10);
        REQUIRE(!*end && count >= 1 && count <= 100, "Cycles must be 1..100");
        return emulator_main(option("scenario", "all"), option("region", "both"), (int)count);
    } else if (!strcmp(command, "rom-info")) {
        allowed("input");
        cJSON *json = rom_info(required("input"));
        char *text = cJSON_PrintUnformatted(json);
        REQUIRE(text, "Cannot serialize ROM information");
        puts(text);
        cJSON_free(text);
        cJSON_Delete(json);
    } else if (!strcmp(command, "publish-rom")) {
        allowed("input output");
        rom_publish(required("input"), required("output"));
    } else if (!strcmp(command, "source-hash")) {
        allowed("contracts littlefs music-smoke");
        puts(source_hash(flag("contracts"), flag("littlefs"), flag("music-smoke")).text);
    } else if (!strcmp(command, "vgm-info")) {
        allowed("input");
        size_t size;
        unsigned char *bytes = mb_read(required("input"), &size);
        REQUIRE(size <= 16 * 1024 * 1024, "VGM input exceeds 16 MiB");
        printf("%llu\n", (unsigned long long)vgm_validate(bytes, size));
        free(bytes);
    } else if (!strcmp(command, "littlefs-check")) {
        allowed("input");
        littlefs_check(required("input"));
    } else if (!strcmp(command, "native-audio")) {
        allowed("region raw frames");
        audio_report(required("region"), required("raw"), required("frames"));
    } else if (!strcmp(command, "format") || !strcmp(command, "lint")) {
        allowed("");
        format_sources(!strcmp(command, "lint"));
    } else if (!strcmp(command, "clean-workflow")) {
        allowed("skip-emulator");
        clean_workflow(flag("skip-emulator"));
    } else
        REQUIRE(false, "Unknown command %s", command);
    return 0;
}
