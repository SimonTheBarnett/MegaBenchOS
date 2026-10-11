/**
 * @file kernel/app_runtime/runtime.c
 * @brief Single foreground app lifecycle, ROM registry
 * and nominal session time.
 *
 * OS service implementation behind the public SDK; apps use
 * megabenchos.h.
 * Public SDK descriptors are borrowed; internal drawing/audio services restore
 * desktop
 * ownership on exit. Deferred transitions prevent callback recursion; no multitasking.

 */
#include "internal.h"
static const MB_App *active;
static uint32_t seconds, ticks;
static uint16_t subticks, dispatch, lifecycle, pending;
static uint16_t notification_count, launch_context;
static uint8_t notifications[12];
static MB_Result failure;
static uint16_t close_guard;
static uint16_t back_intercept;
void mb_app_back_intercept(uint16_t enabled) {
    if (active)
        back_intercept = enabled != 0;
}
void mb_runtime_back(void) {
    if (active && back_intercept) {
        MB_Event e = {MB_BACK, 0, 0};
        mb_runtime_event(&e);
    } else
        mb_runtime_request_close();
}
static char document[44], queued_document[44];
void mb_launch_document(const char *path) {
    uint16_t i = 0;
    if (path)
        while (i < 43 && path[i]) {
            queued_document[i] = path[i];
            ++i;
        }
    queued_document[i] = 0;
}
MB_Result mb_app_document(char *path, uint16_t capacity) {
    uint16_t i = 0;
    if (!path)
        return MB_INVALID;
    if (!document[0])
        return MB_NOT_FOUND;
    while (document[i])
        ++i;
    if (capacity <= i)
        return MB_OVERFLOW;
    do {
        path[i] = document[i];
    } while (i--);
    return MB_OK;
}
void mb_app_close_guard(uint16_t required) {
    if (active)
        close_guard = required != 0;
}
void mb_runtime_request_close(void) {
    if (active && close_guard) {
        MB_Event e = {MB_CLOSE_REQUEST, 0, 0};
        mb_runtime_event(&e);
    } else
        mb_app_close();
}
void mb_launch_context(uint16_t context) {
    launch_context = context;
}
uint16_t mb_app_context(void) {
    return launch_context;
}
void mb_runtime_notify(MB_Action action) {
    uint16_t i;
    if (!active)
        return;
    for (i = 0; i < notification_count; i++)
        if (notifications[i] == action)
            return;
    if (notification_count < 12)
        notifications[notification_count++] = (uint8_t)action;
}
static const MB_App *const apps[MB_APP_COUNT] = {0,
                                                 &system_app,
                                                 &about_app,
                                                 &calculator_app,
                                                 &clock_app,
                                                 &sound_app,
                                                 &snake_app,
                                                 &megabounce_app,
                                                 &colourstorm_app,
                                                 &ocean_app,
                                                 &stardrive_app,
                                                 &megapad_app,
                                                 &megapaint_app,
                                                 &megaplayer_app};
void mb_runtime_reset(void) {
    active = 0;
    seconds = 0;
    subticks = 0;
    ticks = 0;
    dispatch = 0;
    lifecycle = 0;
    pending = 0xffff;
    notification_count = 0;
    failure = MB_OK;
    close_guard = 0;
    back_intercept = 0;
    document[0] = 0;
    queued_document[0] = 0;
    mb_fs_reset();
}
MB_Result mb_app_get(uint16_t index, MB_AppInfo *info) {
    if (!info)
        return MB_INVALID;
    if (index >= MB_APP_COUNT || !apps[index])
        return MB_NOT_FOUND;
    info->id = index;
    info->name = apps[index]->name;
    return MB_OK;
}
MB_Result mb_app_launch(const MB_App *app) {
    MB_Result result;
    uint16_t i;
    if (lifecycle || dispatch)
        return MB_BUSY;
    if (!app || !app->name || !app->init || !app->activate || !app->event || !app->tick ||
        !app->deactivate || !app->shutdown)
        return MB_INVALID;
    if (app->version != MB_API_VERSION)
        return MB_VERSION;
    if (active)
        return MB_BUSY;
    if (!mb_stack_check())
        return MB_UNAVAILABLE;
    lifecycle = 1;
    result = app->init();
    lifecycle = 0;
    if (result != MB_OK) {
        mb_audio_stop();
        mb_window_reset();
        mb_desktop_draw();
        return result;
    }
    active = app;
    close_guard = 0;
    back_intercept = 0;
    for (i = 0; i < 44; i++) {
        document[i] = queued_document[i];
        if (!document[i])
            break;
    }
    queued_document[0] = 0;
    mb_window_reset();
    mb_diagnostics.app_id = 0xffff;
    for (i = 0; i < MB_APP_COUNT; i++)
        if (app == apps[i])
            mb_diagnostics.app_id = i;
    mb_input_mode(MB_INPUT_FOCUS);
    lifecycle = 1;
    app->activate();
    lifecycle = 0;
    mb_runtime_notify(MB_OPEN);
    mb_runtime_notify(MB_FOCUS);
    ++mb_diagnostics.launch_count;
    mb_services_sample();
    mb_desktop_status();
    return MB_OK;
}
void mb_app_close(void) {
    const MB_App *app = active;
    if (lifecycle)
        return;
    if (dispatch) {
        pending = MB_APP_COUNT;
        return;
    }
    if (!app)
        return;
    mb_video_cancel_upload();
    mb_text_reset();
    mb_fs_close_handles();
    close_guard = 0;
    back_intercept = 0;
    lifecycle = 1;
    {
        MB_Event e = {MB_CLOSE, 0, 0};
        app->event(&e);
    }
    app->deactivate();
    app->shutdown();
    lifecycle = 0;
    mb_audio_stop();
    if (mb_window_mode() == MB_WINDOW_FULLSCREEN)
        mb_display_recover();
    active = 0;
    notification_count = 0;
    ++mb_diagnostics.close_count;
    mb_diagnostics.app_id = 0xffff;
    mb_window_reset();
    mb_input_mode(MB_INPUT_FOCUS);
    mb_services_sample();
    mb_desktop_draw();
}
MB_Result mb_app_launch_id(uint16_t id) {
    if (id >= MB_APP_COUNT || !apps[id])
        return MB_NOT_FOUND;
    if (lifecycle || pending != 0xffff)
        return MB_BUSY;
    if (dispatch) {
        pending = id;
        return MB_OK;
    }
    mb_app_close();
    return mb_app_launch(apps[id]);
}
void mb_runtime_pending(void) {
    uint16_t id = pending;
    uint16_t events, i;
    uint8_t queue[12];
    if (dispatch || lifecycle)
        return;
    if (failure != MB_OK) {
        MB_Result error = failure;
        failure = MB_OK;
        pending = 0xffff;
        mb_app_close();
        mb_shell_error(error);
        return;
    }
    events = notification_count;
    for (i = 0; i < events; i++)
        queue[i] = notifications[i];
    notification_count = 0;
    for (i = 0; i < events; i++) {
        MB_Event e = {(MB_Action)queue[i], queue[i] == MB_FOCUS ? 1 : 0, 0};
        mb_runtime_event(&e);
    }
    if (id == 0xffff)
        return;
    pending = 0xffff;
    if (id == MB_APP_COUNT)
        mb_app_close();
    else
        mb_app_launch_id(id);
}
MB_Result mb_app_fail(MB_Result error) {
    if (error >= MB_OK)
        return MB_INVALID;
    if (!active)
        return MB_UNAVAILABLE;
    failure = error;
    return MB_OK;
}
void mb_runtime_event(const MB_Event *event) {
    if (active && event) {
        ++dispatch;
        active->event(event);
        --dispatch;
    } else if (event)
        mb_shell_event(event);
}
void mb_runtime_elapsed(uint16_t hz, uint32_t frames) {
    uint32_t accumulated = (uint32_t)subticks + frames;
    ticks += frames * (300 / hz);
    if (accumulated >= hz) {
        seconds += accumulated / hz;
        mb_services_sample();
        mb_desktop_status();
    }
    subticks = accumulated % hz;
    if (active) {
        ++dispatch;
        active->tick();
        --dispatch;
    }
    mb_ui_tick();
}
void mb_runtime_tick(uint16_t hz) {
    mb_runtime_elapsed(hz, 1);
}
uint16_t mb_runtime_active(void) {
    return active != 0;
}
uint16_t mb_system_capabilities(void) {
    return MB_CAP_CONTROLLER | MB_CAP_ROM_LINKED | MB_CAP_PSG | MB_CAP_ROM_VOLUME;
}
uint32_t mb_time_seconds(void) {
    return seconds;
}
uint32_t mb_time_ticks(void) {
    return ticks;
}
uint32_t mb_time_milliseconds(void) {
    return (ticks / 3) * 10 + (ticks % 3) * 10 / 3;
}
