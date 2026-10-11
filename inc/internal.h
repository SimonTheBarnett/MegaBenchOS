/* Private OS module interfaces and ROM app descriptors. Depends on the public SDK.
 * Only kernel/desktop/boot and the privileged test harness may include this header;
 * apps cannot acquire SGDK/VDP ownership or interrupt context through it. */
#ifndef MB_INTERNAL_H
#define MB_INTERNAL_H
#include "megabenchos.h"
void mb_runtime_reset(void);
void mb_fs_reset(void);
void mb_fs_close_handles(void);
void mb_runtime_request_close(void);
void mb_runtime_back(void);
void mb_launch_document(const char *path);
void mb_runtime_event(const MB_Event *event);
void mb_runtime_tick(uint16_t hz);
void mb_runtime_elapsed(uint16_t hz, uint32_t frames);
void mb_runtime_pending(void);
uint16_t mb_runtime_active(void);
void mb_services_init(void);
void mb_system_restart_request(void);
void mb_services_sample(void);
uint16_t mb_stack_check(void);
void mb_profile(uint32_t duration, uint16_t transition);
void mb_input_cache(uint16_t buttons, int16_t x, int16_t y);
void mb_desktop_draw(void);
void mb_desktop_status(void);
void mb_desktop_activate(void);
void mb_video_init(void);
void mb_boot_logo(uint16_t hz);
void mb_video_flush(void);
void mb_video_cancel_upload(void);
void mb_pointer(int16_t x, int16_t y);
void mb_window_reset(void);
void mb_ui_tick(void);
void mb_shell_draw(void);
void mb_shell_reset(void);
uint16_t mb_shell_modal(void);
void mb_video_boot_reset(void);
void mb_shell_event(const MB_Event *event);
void mb_shell_error(MB_Result error);
void mb_shell_surface(uint16_t root, const char *title);
void mb_shell_icon(uint16_t x, uint16_t y, uint16_t icon);
void mb_shell_label(uint16_t x, uint16_t y, const char *label, uint16_t selected, uint16_t root);
uint16_t mb_window_chrome(const MB_Event *event);
void mb_display_recover(void);
void mb_runtime_notify(MB_Action action);
void mb_launch_context(uint16_t context);
uint16_t mb_registry_mode_allowed(MB_WindowMode mode, uint16_t width, uint16_t height);
uint16_t mb_grid_capacity(uint16_t width, uint16_t height);
uint16_t mb_escape_step(uint16_t *elapsed, uint16_t buttons, uint16_t hz, uint32_t frames);
#ifdef MB_SELF_TEST
uint16_t mb_shell_test_paging(void);
#endif
extern volatile MB_Diagnostics mb_diagnostics;
extern const MB_App system_app, about_app, calculator_app, clock_app, sound_app, snake_app,
    megabounce_app, colourstorm_app, ocean_app, stardrive_app, megapad_app, megapaint_app,
    megaplayer_app;
#endif
