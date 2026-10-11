/**
 * @file kernel/media/music.c
 * @brief Foreground audio ownership. Only pinned, rescomp-aligned ROM music reaches XGM2.
 *
 * OS service implementation behind the public SDK; apps use megabenchos.h.
 * No DMA is issued by MegaPlayer; SGDK owns Z80 bus protection and VBlank sync.
 * External bytes are rejected: header checks do not validate packed commands or
 * prevent Z80/68000 RAM contention, mutation races and arbitrary banked reads.
 */
#include <genesis.h>
#include "internal.h"
#include "music.h"
#include "format.h"
typedef struct {
    const u8 *data;
    u32 size;
    const char *title;
} Track;
static const Track tracks[] = {{music_battle_7, sizeof music_battle_7, "Battle 7"},
                               {music_mad_bossa, sizeof music_mad_bossa, "Mad Bossa"},
                               {music_i_wondered_what_i_could_do_with_it,
                                sizeof music_i_wondered_what_i_could_do_with_it,
                                "I Wondered What I Could Do With It"},
                               {music_questions, sizeof music_questions, "Questions"},
                               {music_sad_polka, sizeof music_sad_polka, "Sad Polka"}};
static const char credit[] = "Safety Stoat Studios (CC0)";
static u16 state;
static u32 started;
MB_Result mb_music_track(uint16_t index, uint16_t version, MB_MusicTrack *out) {
    if (!out)
        return MB_INVALID;
    if (version != 1)
        return MB_VERSION;
    if (index >= sizeof tracks / sizeof tracks[0])
        return MB_NOT_FOUND;
    *out = (MB_MusicTrack){1, index, tracks[index].size, tracks[index].title,
                           index == 1 ? "Spring / Safety Stoat Studios (CC0)" : credit};
    return MB_OK;
}
uint16_t mb_music_capabilities(void) {
    return MB_MUSIC_BUNDLED;
}
void mb_music_stop(void) {
    if (Z80_getLoadedDriver() == Z80_DRIVER_XGM2) {
        /* Stop is asynchronous. Unload immediately installs SGDK's NULL driver,
         * resetting both chips before a PSG app can acquire them. */
        XGM2_stop();
        Z80_unloadDriver();
    }
    state = MB_MUSIC_STOPPED;
    PSG_reset();
    mb_diagnostics.audio_active = 0;
}
MB_Result mb_music_play(uint16_t index) {
    mb_audio_stop();
    if (!mb_runtime_active()) {
        state = MB_MUSIC_ERROR;
        return MB_UNAVAILABLE;
    }
    if (index >= sizeof tracks / sizeof tracks[0]) {
        state = MB_MUSIC_ERROR;
        return MB_NOT_FOUND;
    }
    if ((u32)tracks[index].data & 255) {
        state = MB_MUSIC_ERROR;
        return MB_CORRUPT;
    }
    XGM2_play(tracks[index].data);
    XGM2_setLoopNumber(0); /* Selected source VGMs have no programmed loop points. */
    started = vtimer;
    state = MB_MUSIC_PLAYING;
    mb_diagnostics.audio_active = 3;
    return MB_OK;
}
uint16_t mb_music_state(void) {
    if (state == MB_MUSIC_PLAYING && vtimer - started > 2 && !XGM2_isPlaying())
        mb_music_stop();
    return state;
}
static u8 lower(u8 c) {
    return c >= 'A' && c <= 'Z' ? c + 32 : c;
}
static u16 extension(const char *p, const char *ext) {
    u16 i = 0, n = 0;
    while (p[n])
        ++n;
    if (n < 4)
        return 0;
    while (ext[i]) {
        if (lower(p[n - 4 + i]) != ext[i])
            return 0;
        ++i;
    }
    return 1;
}
MB_Result mb_music_play_file(const char *path) {
    MB_FileInfo info;
    MB_FileHandle file;
    MB_Result r, closed;
    u8 header[8];
    u16 count;
    mb_audio_stop();
    state = MB_MUSIC_ERROR;
    if (!mb_runtime_active())
        return MB_UNAVAILABLE;
    if ((r = mb_file_stat(path, &info)) != MB_OK)
        return r;
    if (extension(path, ".vgm"))
        return MB_NOT_SUPPORTED;
    if (!extension(path, ".xgc"))
        return MB_NOT_SUPPORTED;
    if ((r = mb_file_open(path, MB_FILE_READ, &file)) != MB_OK)
        return r;
    r = mb_file_read(file, 0, header, sizeof header, &count);
    closed = mb_file_close(file);
    if (r != MB_OK)
        return r;
    if (closed != MB_OK)
        return closed;
    if (count != sizeof header)
        return MB_CORRUPT;
    r = mb_music_header(header, info.size);
    return r == MB_OK ? MB_NOT_SUPPORTED : r;
}
