/**
 * @file src/main.c
 * @brief SGDK boot and cooperative foreground loop. Owns controller polling
 * and VBlank flush.
 *
 * Console startup code linked by SGDK; owns boot and main-loop
 * integration.
 * Dispatches public semantic events through internal OS services; no app
 * allocation.
 * PAL/NTSC rates normalize pointer motion and session time on stock 68000 hardware.

 */
#include <genesis.h>
#include "internal.h"
#ifdef MB_SELF_TEST
extern void mb_contract_selftest(void);
#endif
#ifdef MB_LFS_EXPERIMENT
extern void mb_littlefs_target(void);
#endif
#ifdef MB_NATIVE_MUSIC
extern void mb_music_native_test(void);
#endif
static s16 px, py;
static u16 previous, hold, motion, repeat;
static u16 escape_elapsed, mouse_previous;
static u16 restart_requested;
/* Defer desktop confirmation until input callbacks have returned. */
void mb_system_restart_request(void) {
    restart_requested = 1;
}
static u16 semantic(u16 state) {
    u16 b = 0;
    if (state & BUTTON_UP)
        b |= MB_KEY_UP;
    if (state & BUTTON_DOWN)
        b |= MB_KEY_DOWN;
    if (state & BUTTON_LEFT)
        b |= MB_KEY_LEFT;
    if (state & BUTTON_RIGHT)
        b |= MB_KEY_RIGHT;
    if (state & BUTTON_A)
        b |= MB_KEY_A;
    if (state & BUTTON_B)
        b |= MB_KEY_B;
    if (state & BUTTON_C)
        b |= MB_KEY_C;
    if (state & BUTTON_START)
        b |= MB_KEY_START;
    return b;
}
int main(bool hardReset) {
    const u16 hz = SYS_isPAL() ? 50 : 60;
    u32 last_vblank;
    (void)hardReset;
    px = 40;
    py = 48;
    previous = 0;
    restart_requested = 0;
    hold = 0;
    motion = 0;
    repeat = 0;
    mb_runtime_reset();
    mb_shell_reset();
    mb_video_boot_reset();
    escape_elapsed = mouse_previous = 0;
    mb_services_init();
    mb_boot_logo(hz);
    mb_video_init();
    mb_desktop_draw();
    JOY_setSupport(PORT_1, JOY_SUPPORT_3BTN);
    if (JOY_getPortType(PORT_2) == PORT_TYPE_MOUSE)
        JOY_setSupport(PORT_2, JOY_SUPPORT_MOUSE);
    mb_input_mode(MB_INPUT_FOCUS);
#ifdef MB_SELF_TEST
    mb_contract_selftest();
#endif
#ifdef MB_LFS_EXPERIMENT
    mb_littlefs_target();
#endif
#ifdef MB_PREVIEW_APP
    /* Native-emulator visual smoke build: same registered app and public lifecycle,
     * with a deterministic startup selection. This flag is absent from release ROMs. */
    mb_launch_context(0);
    mb_app_launch_id(MB_PREVIEW_APP);
#ifdef MB_PREVIEW_MUSIC
#ifdef MB_NATIVE_MUSIC
    mb_music_native_test();
#else
    mb_music_play(0);
#endif
#endif
#endif
    last_vblank = vtimer;
    while (TRUE) {
        u16 state, pressed, active_before, mouse_moved = 0;
        s16 dx = 0, dy = 0, speed;
        u32 began, ended, now_vblank, frames;
        MB_Input input;
        MB_Event e;
        SYS_doVBlankProcess();
        now_vblank = vtimer;
        /* Count interrupts, including those during costly transitions, to retain uptime. */
        frames = now_vblank - last_vblank;
        last_vblank = now_vblank;
        began = getSubTick();
        mb_video_flush();
        mb_pointer(px, py);
        state = semantic(JOY_readJoypad(JOY_1));
        if (JOY_getJoypadType(JOY_2) == JOY_TYPE_MOUSE) {
            u16 mouse = JOY_readJoypad(JOY_2);
            s16 mx = JOY_readJoypadX(JOY_2), my = JOY_readJoypadY(JOY_2);
            JOY_writeJoypadX(JOY_2, 0);
            JOY_writeJoypadY(JOY_2, 0);
            if (mx || my || (mouse & ~mouse_previous))
                mb_input_mode(MB_INPUT_POINTER);
            px += mx;
            py += my;
            mouse_moved = mx || my;
            if (mouse & BUTTON_LMB)
                state |= MB_KEY_A;
            if (mouse & BUTTON_RMB)
                state |= MB_KEY_B;
            mouse_previous = mouse;
        }
        pressed = state & ~previous;
        previous = state;
        mb_input_cache(state, px, py);
        mb_input_state(&input);
        active_before = mb_diagnostics.app_id;
        if (mb_runtime_active() && mb_escape_step(&escape_elapsed, state, hz, frames)) {
            mb_app_close();
            escape_elapsed = 0;
            continue;
        }
        if (!mb_runtime_active())
            escape_elapsed = 0;
        if (!mb_stack_check()) {
            mb_audio_stop();
            continue;
        }
        if (state & (MB_KEY_UP | MB_KEY_DOWN | MB_KEY_LEFT | MB_KEY_RIGHT)) {
            if (hold < hz)
                ++hold;
            ++repeat;
        } else {
            hold = 0;
            motion = 0;
            repeat = 0;
        }
        if (state & MB_KEY_LEFT)
            dx = -1;
        if (state & MB_KEY_RIGHT)
            dx = 1;
        if (state & MB_KEY_UP)
            dy = -1;
        if (state & MB_KEY_DOWN)
            dy = 1;
        if (input.mode == MB_INPUT_POINTER) {
            u16 level = mb_input_speed_level();
            /* Fractional pixels survive across frames so PAL and NTSC move at equal rates. */
            motion += hold > hz / 3 ? (level + 1) * 60 : (level + 1) * 20;
            speed = motion / hz;
            motion %= hz;
            px += dx * speed;
            py += dy * speed;
            if (px < 0)
                px = 0;
            if (px > 319)
                px = 319;
            if (py < 0)
                py = 0;
            if (py > 223)
                py = 223;
            mb_input_cache(state, px, py);
            e.x = px;
            e.y = py;
            if (dx || dy || mouse_moved) {
                e.action = MB_MOVE;
                if (!mb_runtime_active() || !mb_window_chrome(&e))
                    mb_runtime_event(&e);
            }
        } else if ((pressed & 15) ||
                   (input.mode == MB_INPUT_FOCUS && hold > hz / 4 && repeat >= hz / 8)) {
            e.action = MB_NAVIGATE;
            /* Game turns follow new presses even if the previous direction is
             * still held. Focus navigation retains held-key repeat semantics. */
            if (input.mode == MB_INPUT_GAME) {
                dx = (pressed & MB_KEY_RIGHT) ? 1 : (pressed & MB_KEY_LEFT) ? -1 : 0;
                dy = (pressed & MB_KEY_DOWN) ? 1 : (pressed & MB_KEY_UP) ? -1 : 0;
            }
            e.x = dx;
            e.y = dy;
            mb_runtime_event(&e);
            repeat = 0;
        }
        e.x = px;
        e.y = py;
        if ((pressed & MB_KEY_B) && mb_window_mode() != MB_WINDOW_FULLSCREEN) {
            if (mb_runtime_active())
                mb_runtime_back();
            else {
                e.action = MB_BACK;
                mb_runtime_event(&e);
            }
        }
        if (pressed & MB_KEY_A) {
            e.action = MB_ACTIVATE;
            if (mb_runtime_active()) {
                if (!mb_window_chrome(&e))
                    mb_runtime_event(&e);
            } else
                mb_desktop_activate();
        }
        if (pressed & MB_KEY_START) {
            e.action = MB_MENU;
            if (mb_runtime_active() && (state & MB_KEY_C))
                mb_window_set_mode((mb_window_mode() + 1) % 3);
            else if (mb_runtime_active())
                mb_runtime_event(&e);
            else if (!mb_shell_modal()) {
                mb_launch_context(0);
                mb_app_launch_id(MB_APP_SYSTEM);
            }
        }
        if (pressed & MB_KEY_C) {
            e.action = MB_CONTEXT;
            if (!(state & MB_KEY_START) && input.mode != MB_INPUT_GAME)
                mb_input_mode(input.mode == MB_INPUT_POINTER ? MB_INPUT_FOCUS : MB_INPUT_POINTER);
            if (!(state & MB_KEY_START))
                mb_runtime_event(&e);
        }
        mb_runtime_elapsed(hz, frames);
        mb_runtime_pending();
        if (restart_requested) {
            mb_audio_stop();
            SYS_hardReset();
        }
        ended = getSubTick();
        mb_profile(ended >= began ? ended - began : 0, active_before != mb_diagnostics.app_id);
    }
    return 0;
}
