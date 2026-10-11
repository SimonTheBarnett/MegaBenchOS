/**
 * @file tools/native/core.c
 * @brief Loads the Genesis Plus GX libretro core and supplies video, audio and input callbacks.
 *
 * Host-only C99 tooling; excluded from the console ROM.
 */
#include "core.h"
Core core;
typedef struct {
    const char *path;
    const void *data;
    size_t size;
    const char *meta;
} RetroGame;
typedef struct {
    const char *name, *version, *extensions;
    bool fullpath, block;
} RetroInfo;
typedef struct {
    unsigned width, height, maxwidth, maxheight;
    float aspect;
} RetroGeometry;
typedef struct {
    RetroGeometry geometry;
    double fps, sample_rate;
} RetroAV;
typedef struct {
    const char *key, *value;
} RetroVariable;
#define LOAD(type, name, symbol)                                                                   \
    type name = (type)GetProcAddress(core.library, symbol);                                        \
    REQUIRE(name, "Missing libretro export %s", symbol)
typedef void (*VoidFn)(void);
typedef void (*EnvironmentSetter)(bool (*)(unsigned, void *));
typedef void (*VideoSetter)(void (*)(const void *, unsigned, unsigned, size_t));
typedef void (*AudioSetter)(void (*)(int16_t, int16_t));
typedef void (*BatchSetter)(size_t (*)(const int16_t *, size_t));
typedef void (*PollSetter)(void (*)(void));
typedef void (*InputSetter)(int16_t (*)(unsigned, unsigned, unsigned, unsigned));
static bool environment(unsigned command, void *data) {
    if (command == 10) {
        core.pixel_format = *(unsigned *)data;
        return core.pixel_format <= 2;
    }
    if (command == 9 || command == 30 || command == 31) {
        *(const char **)data = core.directory;
        return true;
    }
    if (command == 15) {
        RetroVariable *variable = data;
        variable->value = NULL;
        if (strcmp(variable->key, "genesis_plus_gx_region_detect") == 0) {
            variable->value = core.pal ? "pal" : "ntsc-u";
            return true;
        }
        return false;
    }
    if (command == 17 || command == 3) {
        *(bool *)data = false;
        return true;
    }
    return command == 18 || command == 35;
}
static void video(const void *pixels, unsigned width, unsigned height, size_t pitch) {
    if (!pixels || !width || !height)
        return;
    REQUIRE(width <= 4096 && height <= 4096 && pitch <= 16384 &&
                pitch >= (size_t)width * (core.pixel_format == 1 ? 4 : 2),
            "Core video exceeds bounds");
    size_t n = pitch * height;
    if (core.pixel_capacity < n) {
        core.pixels = realloc(core.pixels, n);
        REQUIRE(core.pixels, "Video allocation failed");
        core.pixel_capacity = n;
    }
    memcpy(core.pixels, pixels, n);
    core.width = width;
    core.height = height;
    core.pitch = (unsigned)pitch;
}
static void sample(int16_t left, int16_t right) {
    (void)left;
    (void)right;
}
static size_t batch(const int16_t *samples, size_t frames) {
    REQUIRE(frames <= 65536, "Audio batch exceeds bounds");
    int peak = 0;
    for (size_t i = 0; i < frames * 2; i++) {
        int v = abs(samples[i]);
        if (v > peak)
            peak = v;
    }
    core.peaks[core.peak_count++ % 8192] = peak;
    if (core.capture_audio) {
        size_t bytes = frames * 4;
        REQUIRE(core.audio_used + bytes <= 16 * 1024 * 1024, "Audio evidence exceeds 16 MiB");
        if (core.audio_used + bytes > core.audio_capacity) {
            size_t capacity = core.audio_capacity ? core.audio_capacity * 2 : 65536;
            while (capacity < core.audio_used + bytes)
                capacity *= 2;
            core.audio = realloc(core.audio, capacity);
            REQUIRE(core.audio, "Audio allocation failed");
            core.audio_capacity = capacity;
        }
        memcpy(core.audio + core.audio_used, samples, bytes);
        core.audio_used += bytes;
    }
    return frames;
}
static void poll(void) {}
static int16_t input(unsigned port, unsigned device, unsigned index, unsigned id) {
    (void)index;
    if (port == 1 && device == 2) {
        if (id == 0)
            return (int16_t)core.mouse_x;
        if (id == 1)
            return (int16_t)core.mouse_y;
        if (id == 2)
            return (int16_t)(core.mouse_buttons & 1);
        if (id == 3)
            return (int16_t)((core.mouse_buttons >> 1) & 1);
        return 0;
    }
    if (port != 0 || device != 1)
        return 0;
    return id == 256 ? (int16_t)core.buttons
           : id < 16 ? (int16_t)((core.buttons & (1u << id)) != 0)
                     : 0;
}
void core_open(const char *dll, const char *rom, bool pal) {
    REQUIRE(!core.library, "Core is already open");
    memset(&core, 0, sizeof core);
    core.pal = pal;
    snprintf(core.directory, sizeof core.directory, "%s", dll);
    char *slash = strrchr(core.directory, '/'), *back = strrchr(core.directory, '\\');
    if (!slash || (back && back > slash))
        slash = back;
    REQUIRE(slash, "Core path needs directory");
    *slash = 0;
    wchar_t path[4096];
    REQUIRE(MultiByteToWideChar(CP_UTF8, 0, dll, -1, path, 4096), "Core path exceeds limit");
    core.library = LoadLibraryW(path);
    REQUIRE(core.library, "Cannot load GX core: Windows error %lu", (unsigned long)GetLastError());
    LOAD(EnvironmentSetter, set_environment, "retro_set_environment");
    LOAD(VideoSetter, set_video, "retro_set_video_refresh");
    LOAD(AudioSetter, set_sample, "retro_set_audio_sample");
    LOAD(BatchSetter, set_batch, "retro_set_audio_sample_batch");
    LOAD(PollSetter, set_poll, "retro_set_input_poll");
    LOAD(InputSetter, set_input, "retro_set_input_state");
    set_environment(environment);
    set_video(video);
    set_sample(sample);
    set_batch(batch);
    set_poll(poll);
    set_input(input);
    core.run = (VoidFn)GetProcAddress(core.library, "retro_run");
    core.reset = (VoidFn)GetProcAddress(core.library, "retro_reset");
    core.unload = (VoidFn)GetProcAddress(core.library, "retro_unload_game");
    core.deinit = (VoidFn)GetProcAddress(core.library, "retro_deinit");
    core.controller = (void (*)(unsigned, unsigned))GetProcAddress(
        core.library, "retro_set_controller_port_device");
    REQUIRE(core.run && core.reset && core.unload && core.deinit && core.controller,
            "Missing core lifecycle exports");
    LOAD(VoidFn, init, "retro_init");
    init();
    core.initialized = true;
    typedef void (*InfoFn)(RetroInfo *);
    typedef bool (*GameFn)(const RetroGame *);
    typedef void (*AVFn)(RetroAV *);
    typedef unsigned (*RegionFn)(void);
    typedef void *(*MemoryFn)(unsigned);
    typedef size_t (*SizeFn)(unsigned);
    typedef unsigned (*VersionFn)(void);
    LOAD(VersionFn, version, "retro_api_version");
    REQUIRE(version() == 1, "Unsupported libretro ABI");
    LOAD(InfoFn, get_info, "retro_get_system_info");
    RetroInfo info = {0};
    get_info(&info);
    snprintf(core.emulator, sizeof core.emulator, "%s %s", info.name, info.version);
    core.controller(0, 257);
    size_t size;
    core.rom = mb_read(rom, &size);
    RetroGame game = {rom, core.rom, size, NULL};
    LOAD(GameFn, load, "retro_load_game");
    REQUIRE(load(&game), "Core rejected ROM");
    core.loaded = true;
    LOAD(AVFn, get_av, "retro_get_system_av_info");
    RetroAV av = {0};
    get_av(&av);
    core.fps = av.fps;
    core.sample_rate = av.sample_rate;
    LOAD(RegionFn, region, "retro_get_region");
    REQUIRE(region() == (pal ? 1u : 0u) && core.fps > (pal ? 49 : 59) && core.fps < (pal ? 51 : 61),
            "Core region mismatch");
    LOAD(MemoryFn, memory, "retro_get_memory_data");
    LOAD(SizeFn, memory_size, "retro_get_memory_size");
    core.ram = memory(2);
    REQUIRE(core.ram && memory_size(2) == 65536, "Invalid core RAM mapping");
}
void core_close(void) {
    if (core.loaded)
        core.unload();
    if (core.initialized)
        core.deinit();
    if (core.library)
        FreeLibrary(core.library);
    free(core.rom);
    free(core.pixels);
    free(core.audio);
    memset(&core, 0, sizeof core);
}
void core_advance(int frames, unsigned mask) {
    REQUIRE(core.run && frames >= 0 && frames <= 120000, "Invalid frame advance");
    core.buttons = mask;
    for (int n = 0; n < frames; n++)
        core.run();
}
void core_enable_mouse(void) {
    core.controller(1, 2);
    core.buttons = 0;
    core.reset();
}
void core_mouse(int frames, int dx, int dy, int mask) {
    core.mouse_x = dx;
    core.mouse_y = dy;
    core.mouse_buttons = mask;
    core_advance(frames, 0);
    core.mouse_x = core.mouse_y = core.mouse_buttons = 0;
}
void core_words(int address, int count, int *words) {
    REQUIRE(core.ram && address >= 0 && count >= 0 && (size_t)address + (size_t)count * 2 <= 65536,
            "RAM bounds");
    for (int n = 0; n < count; n++)
        words[n] = mb_le16(core.ram + address + n * 2);
}
uint32_t core_u32(int address) {
    int words[2];
    core_words(address, 2, words);
    return (uint32_t)words[0] << 16 | (uint32_t)words[1];
}
int core_peak(int frames) {
    REQUIRE(frames >= 0 && frames <= 8192, "Audio peak window exceeds ring");
    int maximum = 0;
    unsigned count = core.peak_count < (unsigned)frames ? core.peak_count : (unsigned)frames;
    for (unsigned n = 0; n < count; n++) {
        int value = core.peaks[(core.peak_count - 1 - n) % 8192];
        if (value > maximum)
            maximum = value;
    }
    return maximum;
}
void core_start_audio(void) {
    core.audio_used = 0;
    core.peak_count = 0;
    core.capture_audio = true;
}
void core_save_audio(const char *path) {
    core.capture_audio = false;
    unsigned char header[44] = {0};
    memcpy(header, "RIFF", 4);
    mb_put_le32(header + 4, (uint32_t)core.audio_used + 36);
    memcpy(header + 8, "WAVEfmt ", 8);
    mb_put_le32(header + 16, 16);
    header[20] = 1;
    header[22] = 2;
    mb_put_le32(header + 24, (uint32_t)mb_round(core.sample_rate));
    mb_put_le32(header + 28, (uint32_t)mb_round(core.sample_rate) * 4);
    header[32] = 4;
    header[34] = 16;
    memcpy(header + 36, "data", 4);
    mb_put_le32(header + 40, (uint32_t)core.audio_used);
    size_t size = 44 + core.audio_used;
    unsigned char *all = mb_alloc(size);
    memcpy(all, header, 44);
    memcpy(all + 44, core.audio, core.audio_used);
    mb_write(path, all, size);
    free(all);
}
uint32_t core_pixel(int x, int y) {
    REQUIRE(core.pixels && x >= 0 && y >= 0 && (unsigned)x < core.width &&
                (unsigned)y < core.height,
            "Pixel bounds");
    const unsigned char *p = core.pixels + y * core.pitch + x * (core.pixel_format == 1 ? 4 : 2);
    uint32_t value = core.pixel_format == 1 ? mb_le32(p) : mb_le16(p);
    if (core.pixel_format == 1)
        return value & 0xffffff;
    unsigned r = ((value >> (core.pixel_format == 2 ? 11 : 10)) & 31) * 255 / 31,
             g = ((value >> 5) & (core.pixel_format == 2 ? 63 : 31)) * 255 /
                 (core.pixel_format == 2 ? 63 : 31),
             b = (value & 31) * 255 / 31;
    return r << 16 | g << 8 | b;
}
Hash core_capture(const char *path) {
    REQUIRE(core.pixels, "No video frames");
    size_t size = (size_t)core.width * core.height * 3;
    unsigned char *rgb = mb_alloc(size);
    for (unsigned y = 0; y < core.height; y++)
        for (unsigned x = 0; x < core.width; x++) {
            uint32_t color = core_pixel((int)x, (int)y);
            size_t at = ((size_t)y * core.width + x) * 3;
            rgb[at] = (unsigned char)(color >> 16);
            rgb[at + 1] = (unsigned char)(color >> 8);
            rgb[at + 2] = (unsigned char)color;
        }
    png_write(path, (int)core.width, (int)core.height, rgb);
    Hash hash = mb_hash(rgb, size);
    free(rgb);
    return hash;
}
