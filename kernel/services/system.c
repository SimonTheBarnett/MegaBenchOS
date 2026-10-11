/**
 * @file kernel/services/system.c
 * @brief System/input/PSG services and read-only diagnostics behind the public SDK.
 *
 * OS service implementation behind the public SDK; apps use megabenchos.h.
 * SGDK owns hardware and allocator bookkeeping. Stack painting measures only unused
 * reserved RAM after init; no SRAM, RTC, mouse or external filesystem is advertised.
 */
#include <genesis.h>
#include "internal.h"
#include "version.h"
extern u32 _bend;
volatile MB_Diagnostics __attribute__((externally_visible)) mb_diagnostics;
static u16 high_water, cursor_level;
MB_Input __attribute__((externally_visible)) mb_input_observation;
#define input mb_input_observation
#define STACK_PAINT 0xa55a
static u32 painted_end;
uint16_t mb_stack_check(void) {
    volatile u16 *p = (volatile u16 *)MEMORY_HIGH;
    u16 i;
    if (!painted_end)
        return 1;
    for (i = 0; i < 16; i++)
        if (p[i] != STACK_PAINT) {
            mb_diagnostics.stack_guard_ok = 0;
            return 0;
        }
    return 1;
}
void mb_services_init(void) {
    u32 sp;
    volatile u16 *p;
    __asm__ volatile("move.l %%sp,%0" : "=r"(sp));
    /* Leave the live stack and interrupt/call headroom untouched; align 68000 words. */
    painted_end = (sp - 256) & ~1UL;
    if (painted_end < MEMORY_HIGH + 32)
        painted_end = MEMORY_HIGH + 32;
    for (p = (volatile u16 *)MEMORY_HIGH; (u32)p < painted_end; p++)
        *p = STACK_PAINT;
    high_water = 0;
    cursor_level = 2;
    input.buttons = 0;
    input.mode = MB_INPUT_POINTER;
    input.x = 40;
    input.y = 48;
    mb_diagnostics = (MB_Diagnostics){0};
    mb_diagnostics.version = 1;
    mb_diagnostics.refresh_hz = SYS_isPAL() ? 50 : 60;
    mb_diagnostics.app_id = 0xffff;
    mb_diagnostics.tiles = 26; /* Generated atlas count; replaced by video init. */
    mb_audio_stop();
    mb_services_sample();
}
MB_Result mb_memory_stats(MB_MemoryStats *stats) {
    u32 total;
    u16 free_bytes, used_bytes;
    volatile u16 *p;
    if (!stats)
        return MB_INVALID;
    *stats = (MB_MemoryStats){0};
    stats->version = 1;
    stats->scope = MB_MEMORY_SGDK_HEAP;
    stats->static_bytes = (u32)&_bend - RAM;
    stats->stack_reserved = STACK_SIZE;
    if (!mb_stack_check())
        return MB_UNAVAILABLE;
    free_bytes = MEM_getFree();
    used_bytes = MEM_getAllocated();
    total = (u32)free_bytes + used_bytes;
    /* SGDK heap starts at aligned BSS end, ends before the stack, and has a 2-byte sentinel. */
    if (total != MEMORY_HIGH - (((u32)&_bend + 1) & ~1UL) - 2 || total > 65535)
        return MB_UNAVAILABLE;
    if (used_bytes > high_water)
        high_water = used_bytes;
    for (p = (volatile u16 *)MEMORY_HIGH; (u32)p < painted_end && *p == STACK_PAINT; p++) {
    }
    stats->total_tracked = total;
    stats->used = used_bytes;
    stats->free = free_bytes;
    stats->high_water = high_water;
    /* SGDK maps RAM at 0xE0FF0000; one-past-top is 0xE1000000 (24-bit bus mirror).
     * Include the unpainted live headroom: this is a conservative post-init watermark. */
    stats->stack_used = 0xE1000000UL - (u32)p;
    stats->stack_guard_ok = 1;
    return MB_OK;
}
void mb_services_sample(void) {
    MB_MemoryStats m;
    if (mb_memory_stats(&m) != MB_OK) {
        mb_diagnostics.stack_guard_ok = 0;
        return;
    }
    mb_diagnostics.heap_total = m.total_tracked;
    mb_diagnostics.heap_used = m.used;
    mb_diagnostics.heap_free = m.free;
    mb_diagnostics.heap_high_water = m.high_water;
    mb_diagnostics.static_bytes = m.static_bytes;
    mb_diagnostics.stack_reserved = m.stack_reserved;
    mb_diagnostics.stack_used = m.stack_used;
    mb_diagnostics.stack_guard_ok = m.stack_guard_ok;
}
void mb_profile(uint32_t duration, uint16_t transition) {
    if (duration > 65535)
        duration = 65535;
    mb_diagnostics.work_last = duration;
    if (duration > mb_diagnostics.work_max)
        mb_diagnostics.work_max = duration;
    if (transition && duration > mb_diagnostics.transition_max)
        mb_diagnostics.transition_max = duration;
}
MB_Result mb_system_diagnostics(MB_Diagnostics *out) {
    if (!out)
        return MB_INVALID;
    *out = mb_diagnostics;
    return MB_OK;
}
MB_Result mb_system_info(MB_SystemInfo *info) {
    if (!info)
        return MB_INVALID;
    info->api_version = MB_API_VERSION;
    info->refresh_hz = SYS_isPAL() ? 50 : 60;
    info->pal = SYS_isPAL();
    info->version = MB_OS_VERSION;
    info->credits = "Made by \"Simon The Barnett\"";
    info->license = "MIT; SGDK MIT runtime";
    return MB_OK;
}
MB_Result mb_storage_volume(uint16_t index, MB_Volume *v) {
    MB_DriveInfo drive;
    MB_Result result;
    if (!v)
        return MB_INVALID;
    if (index)
        return MB_NOT_FOUND;
    result = mb_fs_capacity("ROM:", 1, &drive);
    if (result != MB_OK)
        return result;
    v->name = drive.name;
    v->flags = MB_VOLUME_READ_ONLY;
    v->capacity_bytes = drive.reserved_bytes;
    return MB_OK;
}
void mb_input_cache(uint16_t buttons, int16_t x, int16_t y) {
    input.buttons = buttons;
    input.x = x;
    input.y = y;
}
MB_Result mb_input_state(MB_Input *out) {
    if (!out)
        return MB_INVALID;
    *out = input;
    return MB_OK;
}
MB_Result mb_input_mode(uint16_t mode) {
    if (mode > MB_INPUT_GAME)
        return MB_INVALID;
    input.mode = mode;
    return MB_OK;
}
MB_Result mb_input_speed(uint16_t level) {
    if (level < 1 || level > 3)
        return MB_INVALID;
    cursor_level = level;
    return MB_OK;
}
uint16_t mb_input_speed_level(void) {
    return cursor_level;
}
void mb_audio_stop(void) {
    mb_music_stop();
    PSG_reset();
    mb_diagnostics.audio_active = 0;
}
/* All audible API effects use maximum PSG volume. Keep the established
 * request range and explicit mute; XGM2 music owns its own mix. */
static uint16_t effect_envelope(uint16_t attenuation) {
    return attenuation == 15 ? 15 : 0;
}
MB_Result mb_audio_volume(uint16_t attenuation) {
    if (attenuation < 8 || attenuation > 15)
        return MB_INVALID;
    if (!mb_runtime_active() || !mb_diagnostics.audio_active)
        return MB_UNAVAILABLE;
    if (mb_diagnostics.audio_active == 3)
        return MB_BUSY;
    PSG_setEnvelope(mb_diagnostics.audio_active == 2 ? 3 : 0, effect_envelope(attenuation));
    return MB_OK;
}
MB_Result mb_audio_tone(uint16_t frequency, uint16_t attenuation) {
    if (frequency < 110 || frequency > 1760 || attenuation < 8 || attenuation > 15)
        return MB_INVALID;
    if (!mb_runtime_active())
        return MB_UNAVAILABLE;
    mb_audio_stop();
    PSG_setFrequency(0, frequency);
    PSG_setEnvelope(0, effect_envelope(attenuation));
    mb_diagnostics.audio_active = attenuation < 15;
    return MB_OK;
}
MB_Result mb_audio_noise(uint16_t attenuation) {
    if (attenuation < 8 || attenuation > 15)
        return MB_INVALID;
    if (!mb_runtime_active())
        return MB_UNAVAILABLE;
    mb_audio_stop();
    PSG_setNoise(PSG_NOISE_TYPE_WHITE, PSG_NOISE_FREQ_CLOCK8);
    PSG_setEnvelope(3, effect_envelope(attenuation));
    mb_diagnostics.audio_active = attenuation < 15 ? 2 : 0;
    return MB_OK;
}
