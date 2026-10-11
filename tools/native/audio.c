/**
 * @file tools/native/audio.c
 * @brief Analyzes F32LE stereo captures and writes native playback evidence.
 *
 * Host-only C99 tooling; excluded from the console ROM.
 * Input must be F32LE stereo at 48 kHz; this is an analyzer, not an audio converter.
 */
#include "host.h"
#define RATE 48000
#define BLOCK 9600
typedef struct {
    double time, peak, rms, dc, ac;
} AudioRow;
static float sample_float(const unsigned char bytes[4]) {
    uint32_t bits = mb_le32(bytes);
    float value;
    memcpy(&value, &bits, 4);
    return value;
}
static void analyze(const char *raw, const char *csv, const char *wave) {
    FILE *source = mb_fopen(raw, "rb");
    REQUIRE(source, "Cannot read native audio capture");
    mb_write(csv, "", 0);
    FILE *output = mb_fopen(csv, "wb");
    REQUIRE(output, "Cannot write native audio CSV");
    fputs("seconds,peak,rms,dc,ac_rms\n", output);
    unsigned char bytes[BLOCK * 4];
    uint64_t samples = 0;
    double first = -1;
    while (fread(bytes, 1, sizeof bytes, source) == sizeof bytes) {
        double peak = 0, sum = 0, total = 0;
        for (int i = 0; i < BLOCK; i++) {
            float value = sample_float(bytes + i * 4);
            if (!isfinite(value) || fabs(value) > 1.01) {
                fclose(source);
                fclose(output);
                REQUIRE(false, "Expected bounded F32LE stereo SDL audio");
            }
            if (fabs(value) > peak)
                peak = fabs(value);
            /* Match the original float multiplication before accumulation. */
            sum += (double)(value * value);
            total += value;
        }
        double time = samples / 96000.0, dc = total / BLOCK,
               ac = sqrt(fmax(0, sum / BLOCK - dc * dc));
        if (first < 0 && ac > .002)
            first = time;
        REQUIRE(fprintf(output, "%.1f,%.7f,%.7f,%.7f,%.7f\n", time, peak, sqrt(sum / BLOCK), dc,
                        ac) > 0,
                "Audio CSV write failed");
        samples += BLOCK;
    }
    bool read_ok = !ferror(source), write_ok = fclose(output) == 0;
    REQUIRE(read_ok && write_ok, "Audio capture read/report write failed");
    REQUIRE(first >= 0, "No varying native audio");
    const size_t pcm_bytes = RATE * 2 * 3 * 2;
    uint64_t start = (uint64_t)((first + 4) * RATE) * 8;
    REQUIRE(_fseeki64(source, (int64_t)start, SEEK_SET) == 0, "Cannot seek listening excerpt");
    unsigned char *pcm = mb_alloc(pcm_bytes + 44);
    memcpy(pcm, "RIFF", 4);
    mb_put_le32(pcm + 4, (uint32_t)pcm_bytes + 36);
    memcpy(pcm + 8, "WAVEfmt ", 8);
    mb_put_le32(pcm + 16, 16);
    pcm[20] = 1;
    pcm[22] = 2;
    mb_put_le32(pcm + 24, RATE);
    mb_put_le32(pcm + 28, RATE * 4);
    pcm[32] = 4;
    pcm[34] = 16;
    memcpy(pcm + 36, "data", 4);
    mb_put_le32(pcm + 40, (uint32_t)pcm_bytes);
    for (size_t i = 0; i < pcm_bytes / 2; i++) {
        unsigned char value[4];
        REQUIRE(fread(value, 1, 4, source) == 4, "Native capture lacks three-second excerpt");
        float input = sample_float(value);
        if (!isfinite(input) || fabs(input) > 1.01) {
            fclose(source);
            free(pcm);
            REQUIRE(false, "Expected bounded F32LE stereo SDL audio");
        }
        int sample = mb_round(input * 32767.0);
        pcm[44 + i * 2] = (unsigned char)sample;
        pcm[45 + i * 2] = (unsigned char)(sample >> 8);
    }
    REQUIRE(fclose(source) == 0, "Audio capture close failed");
    mb_write(wave, pcm, pcm_bytes + 44);
    free(pcm);
}
void audio_check(void) {
    const char *raw = mb_path("out/test-inputs/audio.raw"),
               *csv = mb_path("out/test-inputs/audio.csv"),
               *wave = mb_path("out/test-inputs/audio.wav");
    size_t size = RATE * 8 * 8;
    unsigned char *bytes = mb_alloc(size);
    for (int n = 0; n < RATE * 8; n++) {
        float value = (float)(.1 * sin(n * 3.14159265358979323846 / 48));
        uint32_t bits;
        memcpy(&bits, &value, 4);
        mb_put_le32(bytes + n * 8, bits);
        mb_put_le32(bytes + n * 8 + 4, bits);
    }
    mb_write(raw, bytes, size);
    analyze(raw, csv, wave);
    char *text = mb_text(csv);
    int rows = 0;
    for (char *p = text; *p; p++)
        rows += *p == '\n';
    size_t wave_size;
    unsigned char *pcm = mb_read(wave, &wave_size);
    REQUIRE(rows == 81 && wave_size == 576044, "Native audio analyzer report/excerpt regression");
    free(pcm);
    free(text);
    mb_put_le32(bytes, 0x7fc00000);
    mb_write(raw, bytes, size);
    free(bytes);
    jmp_buf trap;
    mb_trap = &trap;
    bool rejected = false;
    if (setjmp(trap) == 0)
        analyze(raw, csv, wave);
    else
        rejected = strstr(mb_error, "bounded F32LE") != NULL;
    mb_trap = NULL;
    REQUIRE(rejected, "Native analyzer accepted NaN");
}
void audio_report(const char *region, const char *raw, const char *frames) {
    REQUIRE(!strcmp(region, "pal") || !strcmp(region, "ntsc"), "Region must be ntsc or pal");
    uint32_t completed[5];
    char tail;
    REQUIRE(sscanf(frames, "%u,%u,%u,%u,%u%c", completed, completed + 1, completed + 2,
                   completed + 3, completed + 4, &tail) == 5,
            "Supply five completed frame counts from native PASS screen");
    const char *output = mb_path(mb_fmt("reports/emulator/megaplayer/native/%s", region));
    REQUIRE(mb_exists(mb_fmt("%s/completed.jpg", output)),
            "Retain observed completion screen first");
    analyze(raw, mb_fmt("%s/audio.csv", output), mb_fmt("%s/excerpt.wav", output));
    char *text = mb_text(mb_fmt("%s/audio.csv", output));
    AudioRow *rows = mb_alloc(20000 * sizeof *rows);
    double gaps[1024][3], begin = -1;
    int count = 0, gap_count = 0;
    strtok(text, "\n"); /* Skip the CSV column names. */
    char *line;
    while ((line = strtok(NULL, "\n"))) {
        REQUIRE(count < 20000, "Audio report exceeds 2000 seconds");
        AudioRow *row = &rows[count++];
        REQUIRE(sscanf(line, "%lf,%lf,%lf,%lf,%lf", &row->time, &row->peak, &row->rms, &row->dc,
                       &row->ac) == 5,
                "Invalid audio CSV row");
        if (row->ac < .000001) {
            if (begin < 0)
                begin = row->time;
        } else if (begin >= 0) {
            if (row->time - begin >= .7) {
                REQUIRE(gap_count < 1024, "Too many capture silence gaps");
                gaps[gap_count][0] = begin;
                gaps[gap_count][1] = row->time;
                gaps[gap_count++][2] = row->time - begin;
            }
            begin = -1;
        }
    }
    REQUIRE(gap_count >= 12, "Expected five early Stop gaps and five complete-song endings");
    cJSON *provenance = json_read(mb_path("third_party/music/safetystoat/provenance.json")),
          *source = json_get(provenance, "tracks"), *tracks = cJSON_CreateArray();
    int hz = !strcmp(region, "pal") ? 50 : 60;
    for (int n = 0; n < 5; n++) {
        cJSON *song = cJSON_GetArrayItem(source, n);
        double expected = json_number(json_get(song, "samples")) / 44100,
               seconds = completed[n] / (double)hz, *stop = gaps[2 + n * 2], *end = gaps[3 + n * 2],
               peak = 0, ac = 0, audio_seconds = end[0] - stop[1];
        bool found = false;
        REQUIRE(fabs(seconds - expected) <= .1 + expected * .005,
                "Native frame duration outside harness gate");
        for (int i = 0; i < count; i++)
            if (rows[i].time >= stop[1] && rows[i].time < end[0]) {
                found = true;
                peak = fmax(peak, rows[i].peak);
                ac = fmax(ac, rows[i].ac);
            }
        REQUIRE(found && ac >= .002 && fabs(audio_seconds - expected) <= .5 + expected * .02,
                "Missing/incomplete varying native audio");
        cJSON *track = cJSON_CreateObject();
        json_str(track, "title", json_s(song, "title"));
        json_num(track, "frames", completed[n]);
        json_num(track, "nominal_seconds", seconds);
        json_num(track, "source_seconds", expected);
        json_num(track, "native_capture_seconds", nearbyint(audio_seconds * 10) / 10);
        json_num(track, "peak", peak);
        json_num(track, "maximum_ac_rms", ac);
        json_num(track, "early_stop_silence_seconds", nearbyint(stop[2] * 10) / 10);
        json_num(track, "natural_end_silence_seconds", nearbyint(end[2] * 10) / 10);
        json_str(track, "loop", "none in source; ends once");
        json_str(track, "status", "PASS");
        cJSON_AddItemToArray(tracks, track);
    }
    for (int i = count > 100 ? count - 100 : 0; i < count; i++)
        REQUIRE(rows[i].ac <= .000001, "Native output did not settle to silence after exit");
    cJSON *built = json_read(mb_path("dist/preview-megaplayer-smoke-build-report.json")),
          *cfg = mb_config(), *report = cJSON_CreateObject(),
          *rom = rom_info(mb_path("dist/MegaBenchOS.bin"));
    json_str(report, "status", "PASS");
    json_str(report, "region", region);
    json_str(report, "emulator", "BlastEm 1.0.0 native Windows");
    json_str(report, "emulator_sha256", mb_hash_file(json_s(cfg, "blastem")).text);
    json_str(report, "harness_rom_sha256", json_s(built, "sha256"));
    json_str(report, "harness_source_sha256",
             mb_hash_file(mb_path("tests/target/music_native.c")).text);
    json_str(report, "release_rom_sha256", json_s(rom, "sha256"));
    json_str(report, "capture_backend", "Bundled SDL2 disk / F32LE stereo 48000 Hz");
    json_set(report, "tracks", tracks);
    json_num(report, "exit_silence_ac_rms", 0);
    json_str(report, "silence_note", "AC RMS removes the inaudible constant chip DC offset");
    json_str(report, "raw_capture_sha256", mb_hash_file(raw).text);
    json_str(report, "subjective_listening",
             "MANUAL: source-versus-conversion listening comparison");
    char timestamp[32];
    mb_timestamp(timestamp);
    json_str(report, "timestamp_utc", timestamp);
    json_write(mb_fmt("%s/report.json", output), report);
    printf("PASS native BlastEm %s: five complete songs, Stop and exit silence\n", region);
    free(text);
    free(rows);
    cJSON_Delete(provenance);
    cJSON_Delete(built);
    cJSON_Delete(cfg);
    cJSON_Delete(report);
    cJSON_Delete(rom);
}
