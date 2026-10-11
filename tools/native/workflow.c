/**
 * @file tools/native/workflow.c
 * @brief Windows filesystem/process operations for formatting and isolated builds.
 *
 * Host-only C99 tooling; excluded from the console ROM.
 * Copies and evidence remain below out/ and reports/ in the repository.
 */
#include "host.h"
static bool handwritten(const char *path) {
    const char *dot = strrchr(path, '.');
    if (!dot || (strcmp(dot, ".c") && strcmp(dot, ".h")))
        return false;
    return !strstr(path, "_assets.h") && strcmp(path, "inc/ui_assets.h") &&
           strcmp(path, "sdk/include/megabenchos.h");
}
static const char *formatter(void) {
    const char *cc = getenv("MB_HOST_CC");
    REQUIRE(cc && *cc, "Dot-source scripts/env.ps1 to configure LLVM");
    char *directory = mb_copy(cc), *slash = strrchr(directory, '/'),
         *back = strrchr(directory, '\\');
    if (!slash || (back && back > slash))
        slash = back;
    REQUIRE(slash, "Host compiler path must be absolute");
    *slash = 0;
    const char *path = mb_fmt("%s/clang-format.exe", directory);
    free(directory);
    REQUIRE(mb_exists(path), "clang-format is missing from the LLVM installation");
    return path;
}
/** Keep analyzer output in evidence files, avoiding PowerShell stderr semantics. */
static void analyze_sources(void) {
    char *tool = mb_copy(formatter());
    char *name = strrchr(tool, '/');
    if (!name)
        name = strrchr(tool, '\\');
    REQUIRE(name, "Invalid LLVM directory");
    strcpy(name + 1, "clang-tidy.exe");
    REQUIRE(mb_exists(tool), "clang-tidy is missing from LLVM");
    Paths files = {0};
    mb_scan(&files, "tools/native", false);
    mb_scan(&files, "tests/emulator", false);
    const char *arguments[128];
    size_t count = 0;
    for (size_t i = 0; i < files.count; i++) {
        const char *dot = strrchr(files.items[i], '.');
        if (dot && !strcmp(dot, ".c")) {
            REQUIRE(count < 120, "Too many analyzer inputs");
            arguments[count++] = files.items[i];
        }
    }
    size_t source_count = count;
    arguments[count++] = "--";
    arguments[count++] = "-std=c99";
    arguments[count++] = "-D_CRT_SECURE_NO_WARNINGS";
    arguments[count++] = "-Itools/native";
    arguments[count++] = "-Ithird_party/cjson";
    arguments[count++] = "-Ithird_party/unity/src";
    const char *log = mb_path("reports/lint-static-analysis.log");
    int result = mb_process(tool, arguments, count, 120, log);
    if (result) {
        char *text = mb_text(log);
        fputs(text, stdout);
        free(text);
    }
    free(tool);
    for (size_t i = 0; i < files.count; i++)
        free(files.items[i]);
    free(files.items);
    REQUIRE(result == 0, "clang-tidy failed; see reports/lint-static-analysis.log");
    printf("PASS static analysis of %zu native C files\n", source_count);
}
void format_sources(bool check) {
    const char *folders[] = {"src", "kernel", "desktop", "apps",
                             "inc", "sdk",    "tests",   "tools/native"};
    Paths paths = {0};
    for (int i = 0; i < 8; i++)
        mb_scan(&paths, folders[i], true);
    const char *tool = mb_copy(formatter());
    int count = 0;
    for (size_t i = 0; i < paths.count; i++) {
        if (handwritten(paths.items[i])) {
            const char *args[4] = {check ? "--dry-run" : "-i",
                                   check ? "--Werror" : mb_path(paths.items[i]),
                                   check ? mb_path(paths.items[i]) : NULL, NULL};
            REQUIRE(mb_process(tool, args, check ? 3 : 2, 120, NULL) == 0,
                    "Formatting check failed: %s", paths.items[i]);
            count++;
        }
        free(paths.items[i]);
    }
    free(paths.items);
    free((char *)tool);
    printf("PASS %s %d handwritten C files\n", check ? "formatting of" : "formatted", count);
    if (check)
        analyze_sources();
}
static void copy_file(const char *source, const char *destination) {
    size_t size;
    unsigned char *bytes = mb_read(source, &size);
    mb_write(destination, bytes, size);
    free(bytes);
}
void clean_workflow(bool skip_emulator) {
    cJSON *cfg = mb_config();
    const char *copy = mb_copy(mb_path(mb_fmt("out/workflow-check/MegaBenchOS clean %lu-%llu",
                                              (unsigned long)GetCurrentProcessId(),
                                              (unsigned long long)GetTickCount64()))),
               *list = mb_path("reports/clean-source-files.txt");
    const char *args[] = {"-C", mb_root, "ls-files", "--cached", "--others", "--exclude-standard"};
    REQUIRE(mb_process("git.exe", args, 6, 120, list) == 0, "Cannot enumerate source files");
    char *files = mb_text(list);
    for (char *line = strtok(files, "\r\n"); line; line = strtok(NULL, "\r\n")) {
        REQUIRE(!strstr(line, "..") && line[0] != '/' && !strchr(line, ':'),
                "Unsafe source-copy path");
        const char *source = mb_path(line);
        if (mb_exists(source))
            copy_file(source, mb_fmt("%s/%s", copy, line));
    }
    free(files);
    REQUIRE(!mb_exists(mb_fmt("%s/config/toolchain.local.json", copy)),
            "Local configuration unexpectedly copied");
    const char *cc = getenv("MB_HOST_CC");
    REQUIRE(cc && *cc, "Configure MB_HOST_CC");
    SetEnvironmentVariableA("MB_SGDK", json_s(cfg, "sgdk"));
    SetEnvironmentVariableA(
        "MB_GENPLUS_CORE",
        json_s(cfg, "genesis_plus_gx_core")); /* Process-only PATH: Java and Windows utilities,
                                                 without language environments. */
    const char *java_list = mb_path("reports/clean-java-path.txt"), *java_args[] = {"java"};
    REQUIRE(mb_process("where.exe", java_args, 1, 30, java_list) == 0, "Java is unavailable");
    char *java = mb_text(java_list), *end = strpbrk(java, "\r\n");
    if (end)
        *end = 0;
    char *slash = strrchr(java, '\\');
    REQUIRE(slash, "Java path has no directory");
    *slash = 0;
    const char *windows = getenv("WINDIR");
    REQUIRE(windows, "Windows directory is unavailable");
    SetEnvironmentVariableA("PATH", mb_fmt("%s;%s/System32;%s/System32/WindowsPowerShell/v1.0",
                                           java, windows, windows));
    free(java);
    char saved_root[4096];
    snprintf(saved_root, sizeof saved_root, "%s", mb_root);
    snprintf(mb_root, sizeof mb_root, "%s", copy);
    const char *make = mb_fmt("%s/bin/make.exe", json_s(cfg, "sgdk"));
    const char *commands[] = {"build", skip_emulator ? "test" : "compatibility", "package-app"};
    for (int i = 0; i < 3; i++) {
        const char *make_args[] = {"-C", copy, commands[i], "APP=calculator"};
        REQUIRE(mb_process(make, make_args, 4, 600,
                           mb_fmt("%s/reports/clean-%s.log", copy, commands[i])) == 0,
                "Isolated make %s failed; see retained logs", commands[i]);
    }
    snprintf(mb_root, sizeof mb_root, "%s", saved_root);
    cJSON *built = json_read(mb_fmt("%s/dist/build-report.json", copy)),
          *original = rom_info(mb_path("dist/MegaBenchOS.bin"));
    REQUIRE(!strcmp(json_s(built, "sha256"), json_s(original, "sha256")), "Isolated ROM differs");
    cJSON *report = cJSON_CreateObject();
    json_str(report, "status", "PASS");
    json_str(report, "source_copy", copy);
    json_set(report, "local_config", cJSON_CreateBool(false));
    json_set(report, "python_on_path", cJSON_CreateBool(false));
    json_str(report, "tool_configuration", "Explicit process-only tool paths");
    json_str(report, "rom_sha256", json_s(built, "sha256"));
    json_write(mb_path("reports/clean-workflow.json"), report);
    printf("PASS isolated workflow; retained source copy: %s\n", copy);
    free((char *)copy);
    cJSON_Delete(cfg);
    cJSON_Delete(built);
    cJSON_Delete(original);
    cJSON_Delete(report);
}
