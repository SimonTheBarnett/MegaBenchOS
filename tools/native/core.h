/**
 * @file tools/native/core.h
 * @brief Private libretro session interface for real GX integration tests.
 *
 * One core is loaded per process. The session owns its ROM, frame and audio
 * buffers until core_close; RAM observations are read-only borrowed core data.
 */
#ifndef MB_NATIVE_CORE_H
#define MB_NATIVE_CORE_H
#include "host.h"
/* Libretro C ABI: https://docs.libretro.com/development/cores/developing-cores/ */
typedef struct {
    HMODULE library;
    void (*run)(void), (*reset)(void), (*unload)(void), (*deinit)(void);
    void (*controller)(unsigned, unsigned);
    bool initialized, loaded, capture_audio;
    unsigned char *rom, *ram, *pixels, *audio;
    size_t pixel_capacity, audio_used, audio_capacity;
    unsigned pixel_format, width, height, pitch, buttons;
    int mouse_x, mouse_y, mouse_buttons;
    int peaks[8192];
    unsigned peak_count;
    double fps, sample_rate;
    char emulator[256], directory[4096];
    bool pal;
} Core;
extern Core core;
/** Load the core/ROM and verify PAL/NTSC timing, ABI and 64 KiB RAM mapping. */
void core_open(const char *dll, const char *rom, bool pal);
/** Unload/deinitialize and free buffers; safe before opening or after closing. */
void core_close(void);
/** Advance 0..120000 frames using libretro joypad bits; does not write RAM. */
void core_advance(int frames, unsigned mask);
/** Supply mouse deltas/buttons for these frames, then release mouse input. */
void core_mouse(int frames, int dx, int dy, int mask);
/** Attach a port-2 Sega Mouse and reset the actual core to detect the device. */
void core_enable_mouse(void);
/** Copy count words into caller storage, decoding GX's word-swapped RAM layout. */
void core_words(int address, int count, int *destination);
uint32_t core_u32(int address);
int core_peak(int frames);
/** Start a bounded 16 MiB audio capture; discard previous PCM and peak history. */
void core_start_audio(void);
/** Write captured PCM16 stereo WAV and stop capture; peak monitoring continues. */
void core_save_audio(const char *path);
/** Return RGB888 from the current frame; fail if coordinates are outside it. */
uint32_t core_pixel(int x, int y);
/** Write the actual frame as PNG and return its canonical RGB byte SHA256. */
Hash core_capture(const char *path);
void png_write(const char *path, int width, int height, const unsigned char *rgb);
#endif
