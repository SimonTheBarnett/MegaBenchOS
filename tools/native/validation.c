/**
 * @file tools/native/validation.c
 * @brief Validates ROM headers, source stamps, VGM commands and pinned music attribution.
 *
 * Host-only C99 tooling; excluded from the console ROM.
 */
#include "host.h"
static bool extension(const char *p, const char *ext) {
    size_t a = strlen(p), b = strlen(ext);
    return a >= b && strcmp(p + a - b, ext) == 0;
}
cJSON *rom_bytes(const unsigned char *b, size_t size) {
    REQUIRE(size >= 512 && size % 2 == 0, "ROM requires an even size and full header");
    REQUIRE(size <= 4194304, "ROM exceeds 4 MiB unbanked limit");
    REQUIRE(memcmp(b + 256, "SEGA MEGA DRIVE ", 16) == 0, "Invalid console signature");
    uint32_t sum = 0;
    for (size_t i = 512; i < size; i += 2)
        sum += b[i] * 256 + b[i + 1];
    sum &= 65535;
    REQUIRE(sum == (uint32_t)(b[398] * 256 + b[399]), "ROM additive checksum mismatch");
    uint32_t sp = mb_be32(b), pc = mb_be32(b + 4), ramsp = sp & 0xffffff;
    REQUIRE((ramsp == 0 || ramsp >= 0xff0000) && pc >= 512 && pc < size && pc % 2 == 0,
            "Invalid reset vectors");
    REQUIRE(memcmp(b + 432, "RA", 2) != 0, "Unimplemented SRAM advertised");
    char brand[97];
    memcpy(brand, b + 288, 96);
    brand[96] = 0;
    REQUIRE(strstr(brand, "MEGABENCHOS"), "Missing MegaBenchOS branding");
    REQUIRE(mb_be32(b + 420) == size - 1, "ROM end address mismatch");
    cJSON *info = cJSON_CreateObject();
    json_num(info, "rom_bytes", (double)size);
    json_str(info, "sha256", mb_hash(b, size).text);
    json_num(info, "checksum", sum);
    json_num(info, "reset_pc", pc);
    json_num(info, "initial_sp", sp);
    return info;
}
cJSON *rom_info(const char *path) {
    size_t n;
    unsigned char *b = mb_read(path, &n);
    cJSON *info = rom_bytes(b, n);
    free(b);
    return info;
}
void rom_publish(const char *source, const char *destination) {
    size_t n;
    unsigned char *b = mb_read(source, &n);
    REQUIRE(n >= 512 && n % 2 == 0 && n <= 4194304, "Invalid ROM size");
    uint32_t sum = 0;
    for (size_t i = 512; i < n; i += 2)
        sum += b[i] * 256 + b[i + 1];
    b[398] = (unsigned char)(sum >> 8);
    b[399] = (unsigned char)sum;
    mb_put_be32(b + 420, (uint32_t)n - 1);
    cJSON *info = rom_bytes(b, n);
    cJSON_Delete(info);
    mb_write(destination, b, n);
    free(b);
}
Hash source_hash(bool contracts, bool littlefs, bool smoke) {
    Paths all = {0}, files = {0};
    const char *folders[] = {"src", "kernel", "desktop", "apps", "inc", "sdk/include"};
    for (size_t i = 0; i < sizeof folders / sizeof folders[0]; i++)
        mb_scan(&all, folders[i], true);
    if (contracts)
        mb_scan(&all, "tests/target", true);
    if (littlefs) {
        mb_scan(&all, "tests/littlefs", true);
        mb_scan(&all, "third_party/littlefs", true);
    }
    for (size_t i = 0; i < all.count; i++) {
        if (extension(all.items[i], ".c") || extension(all.items[i], ".h"))
            mb_paths_add(&files, all.items[i]);
        free(all.items[i]);
    }
    free(all.items);
    if (smoke && !contracts)
        mb_paths_add(&files, "tests/target/music_native.c");
    const char *inputs[] = {"Makefile",
                            ".clang-format",
                            ".clang-tidy",
                            "scripts/build-support.ps1",
                            "scripts/common.ps1",
                            "api/core.json",
                            "api/schema.json",
                            "assets/source/ui.json",
                            "assets/generated/manifest.json",
                            "assets/generated/demos-manifest.json",
                            "config/toolchain.lock.json",
                            "config/budgets.json",
                            "assets/music/music.res"};
    for (size_t i = 0; i < sizeof inputs / sizeof inputs[0]; i++)
        mb_paths_add(&files, inputs[i]);
    mb_scan(&files, "tools/native", true);
    mb_scan(&files, "third_party/cjson", false);
    mb_scan(&files, "third_party/music/safetystoat", false);
    all = (Paths){0};
    mb_scan(&all, "assets/music/generated", false);
    for (size_t i = 0; i < all.count; i++) {
        const char *p = all.items[i];
        if (extension(p, ".xgc") || extension(p, ".json") || extension(p, ".md") ||
            extension(p, ".txt"))
            mb_paths_add(&files, p);
        free(all.items[i]);
    }
    free(all.items);
    mb_paths_sort(&files);
    /* Canonical relative names keep the stamp independent of checkout directory. */
    size_t capacity = 1048576, used = 0;
    unsigned char *data = mb_alloc(capacity);
    for (size_t i = 0; i < files.count; i++) {
        size_t size;
        unsigned char *bytes = mb_read(mb_path(files.items[i]), &size);
        size_t name = strlen(files.items[i]);
        while (used + name + size + 2 > capacity) {
            capacity *= 2;
            REQUIRE(capacity <= 128 * 1024 * 1024, "Source stamp exceeds limit");
            data = realloc(data, capacity);
            REQUIRE(data, "Source stamp allocation failed");
        }
        memcpy(data + used, files.items[i], name + 1);
        used += name + 1;
        memcpy(data + used, bytes, size);
        used += size;
        data[used++] = 0;
        free(bytes);
        free(files.items[i]);
    }
    free(files.items);
    Hash hash = mb_hash(data, used);
    free(data);
    return hash;
}
uint64_t vgm_validate(const unsigned char *b, size_t size) {
    REQUIRE(size >= 64 && memcmp(b, "Vgm ", 4) == 0, "Expected uncompressed Sega Mega Drive VGM");
    REQUIRE((uint64_t)mb_le32(b + 4) + 4 == size, "VGM EOF offset mismatch");
    uint32_t version = mb_le32(b + 8);
    REQUIRE(version >= 0x150 && version <= 0x171, "VGM versions 1.50 through 1.71 only");
    uint32_t relative = mb_le32(b + 52);
    uint64_t p = relative ? 52ULL + relative : 64;
    REQUIRE(p >= 64 && p < size, "Invalid VGM data offset");
    uint32_t ym = mb_le32(b + 44), psg = mb_le32(b + 12);
    REQUIRE(ym || psg, "No YM2612 or SN76489 clock");
    REQUIRE(ym < 0x40000000 && psg < 0x40000000, "Dual/variant chip unsupported");
    const unsigned offsets[] = {0x10, 0x30, 0x38, 0x40, 0x44, 0x48, 0x4c, 0x50, 0x54, 0x58,
                                0x5c, 0x60, 0x64, 0x68, 0x6c, 0x70, 0x74, 0x80, 0x84, 0x88,
                                0x8c, 0x90, 0x98, 0x9c, 0xa0, 0xa4, 0xa8, 0xac, 0xb0, 0xb4,
                                0xb8, 0xc0, 0xc4, 0xc8, 0xcc, 0xd0, 0xd8, 0xdc, 0xe0};
    for (size_t i = 0; i < sizeof offsets / sizeof offsets[0]; i++)
        REQUIRE(offsets[i] + 4 > p || mb_le32(b + offsets[i]) == 0,
                "Additional/unsupported VGM chip field");
    uint32_t gd3 = mb_le32(b + 20);
    if (gd3) {
        uint64_t g = 20ULL + gd3;
        REQUIRE(g + 12 <= size && memcmp(b + g, "Gd3 ", 4) == 0 &&
                    g + 12 + mb_le32(b + g + 8) <= size,
                "Invalid/truncated GD3 tags");
    }
    bool ended = false;
    uint64_t samples = 0;
    while (p < size) {
        unsigned op = b[p++];
        uint64_t skip = 0;
        switch (op) {
        case 0x4f:
        case 0x50:
            skip = 1;
            break;
        case 0x52:
        case 0x53:
            skip = 2;
            break;
        case 0x61:
            REQUIRE(p + 2 <= size, "Truncated wait");
            samples += mb_le16(b + p);
            skip = 2;
            break;
        case 0x62:
            samples += 735;
            break;
        case 0x63:
            samples += 882;
            break;
        case 0x66:
            ended = true;
            break;
        case 0x67:
            REQUIRE(p + 6 <= size && b[p] == 0x66 && b[p + 1] == 0,
                    "Only bounded YM2612 PCM data blocks");
            skip = 6ULL + mb_le32(b + p + 2);
            break;
        case 0x90:
            REQUIRE(p + 4 <= size && b[p + 1] == 2 && b[p + 2] == 0 && b[p + 3] == 0x2a,
                    "Only YM2612 DAC streams supported");
            skip = 4;
            break;
        case 0x91:
            skip = 4;
            break;
        case 0x92:
            skip = 5;
            break;
        case 0x93:
            skip = 10;
            break;
        case 0x94:
            skip = 1;
            break;
        case 0x95:
            skip = 4;
            break;
        case 0xe0:
            skip = 4;
            break;
        default:
            if (op >= 0x70 && op <= 0x7f)
                samples += (op & 15) + 1;
            else if (op >= 0x80 && op <= 0x8f)
                samples += op & 15;
            else
                REQUIRE(false, "Unsupported VGM opcode 0x%02x", op);
            break;
        }
        if (ended)
            break;
        p += skip;
        REQUIRE(p <= size, "Truncated command/data block");
    }
    REQUIRE(ended, "VGM has no end command");
    REQUIRE(samples == mb_le32(b + 24), "VGM declared duration disagrees with commands");
    return samples;
}
static void file_hash_matches(const char *path, const char *expected) {
    REQUIRE(strcmp(mb_hash_file(path).text, expected) == 0, "Hash mismatch: %s", path);
}
void vendor_check(const char *name) {
    cJSON *manifest = json_read(mb_path(mb_fmt("third_party/%s/upstream.json", name))), *file;
    cJSON_ArrayForEach(file, json_get(manifest, "files")) file_hash_matches(
        mb_path(mb_fmt("third_party/%s/%s", name, json_s(file, "name"))), json_s(file, "sha256"));
    cJSON_Delete(manifest);
}
void music_check(void) {
    const char *originals = "third_party/music/safetystoat", *assets = "assets/music/generated";
    cJSON *m = json_read(mb_path(mb_fmt("%s/provenance.json", originals))),
          *out = json_read(mb_path(mb_fmt("%s/manifest.json", assets)));
    file_hash_matches(mb_path(mb_fmt("%s/%s", originals, json_s(m, "license_evidence_filename"))),
                      json_s(m, "license_evidence_sha256"));
    file_hash_matches(mb_path(mb_fmt("%s/%s", originals, json_s(m, "composer_evidence_filename"))),
                      json_s(m, "composer_evidence_sha256"));
    file_hash_matches(mb_path(mb_fmt("%s/%s", assets, json_s(out, "license_evidence_file"))),
                      json_s(out, "license_evidence_sha256"));
    file_hash_matches(mb_path(mb_fmt("%s/%s", assets, json_s(out, "composer_evidence_file"))),
                      json_s(out, "composer_evidence_sha256"));
    cJSON *tracks = json_get(m, "tracks"), *outputs = json_get(out, "tracks");
    REQUIRE(cJSON_GetArraySize(tracks) == 5 && cJSON_GetArraySize(outputs) == 5,
            "Expected five attributed music resources");
    cJSON *track;
    cJSON_ArrayForEach(track, tracks) {
        const char *filename = json_s(track, "filename");
        const char *path = mb_path(mb_fmt("%s/%s", originals, filename));
        file_hash_matches(path, json_s(track, "sha256"));
        size_t n;
        unsigned char *bytes = mb_read(path, &n);
        REQUIRE(vgm_validate(bytes, n) == (uint64_t)json_number(json_get(track, "samples")) &&
                    mb_le32(bytes + 28) == (uint32_t)json_number(json_get(track, "loop_offset")) &&
                    mb_le32(bytes + 32) == (uint32_t)json_number(json_get(track, "loop_samples")),
                "Music timing/loop metadata mismatch");
        free(bytes);
        cJSON *candidate, *match = NULL;
        int count = 0;
        cJSON_ArrayForEach(
            candidate, outputs) if (strcmp(json_s(candidate, "source_filename"), filename) == 0) {
            match = candidate;
            count++;
        }
        REQUIRE(count == 1, "Missing/ambiguous converted music record");
        REQUIRE(strcmp(json_s(match, "status"), "PASS") == 0 &&
                    strcmp(json_s(match, "license"), "CC0-1.0") == 0 &&
                    strcmp(json_s(match, "source_sha256"), json_s(track, "sha256")) == 0 &&
                    strcmp(json_s(match, "output_filename"), json_s(track, "converted_asset")) ==
                        0 &&
                    strcmp(json_s(match, "upstream_commit"), json_s(m, "upstream_commit")) == 0,
                "Converted music metadata mismatch");
        const char *fields[] = {"title", "author", "source_url"};
        for (size_t j = 0; j < 3; j++)
            REQUIRE(strcmp(json_s(match, fields[j]), json_s(track, fields[j])) == 0,
                    "Music attribution mismatch");
        path = mb_path(mb_fmt("%s/%s", assets, json_s(track, "converted_asset")));
        bytes = mb_read(path, &n);
        REQUIRE((double)n == json_number(json_get(match, "output_bytes")) &&
                    strcmp(mb_hash(bytes, n).text, json_s(match, "output_sha256")) == 0,
                "Converted music hash/size mismatch");
        free(bytes);
    }
    cJSON_Delete(out);
    cJSON_Delete(m);
}
