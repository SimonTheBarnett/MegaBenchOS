/**
 * @file apps/calculator/calculator.c
 * @brief Calculator UI: public SDK widgets and bounded signed integer app-local model.
 *
 * ROM-linked application code. Shared hardware and resource ownership remain in OS services.
 * Uses only megabenchos.h and app-local interfaces; static state/ROM, no heap or hardware.
 */
#include "megabenchos.h"
#include "model.h"
static MB_Calculator calculator;
static const uint16_t keys[20] = {
    7, 8,        9, CALC_DIV, 4,          5,        6,          CALC_MUL,    1,  2,
    3, CALC_SUB, 0, CALC_NEG, CALC_EQUAL, CALC_ADD, CALC_CLEAR, CALC_DELETE, 18, 19};
static const char *const labels[20] = {"7", "8", "9", "/",   "4", "5", "6",   "*",   "1", "2",
                                       "3", "-", "0", "Neg", "=", "+", "Clr", "Del", "",  ""};
static void display(void) {
    char b[12];
    MB_Rect r;
    mb_window_get_client_rect(&r);
    mb_format_i32(calculator.value, b, sizeof b);
    mb_draw_field(0, 0, r.width, b);
    mb_draw_field(0, 1, r.width,
                  calculator.error == MB_DIV_ZERO    ? "Divide by zero - use Clr"
                  : calculator.error == MB_OVERFLOW  ? "Overflow - use Clr"
                  : calculator.operation == CALC_ADD ? "+"
                  : calculator.operation == CALC_SUB ? "-"
                  : calculator.operation == CALC_MUL ? "*"
                  : calculator.operation == CALC_DIV ? "/ (integer division)"
                                                     : "Signed integer calculator");
}
static MB_Result init(void) {
    mb_calc_reset(&calculator);
    return MB_OK;
}
static void activate(void) {
    uint16_t i;
    MB_Rect r;
    mb_window_open("Calculator");
    mb_window_get_client_rect(&r);
    for (i = 0; i < 18; i++)
        mb_ui_button(keys[i], 1 + (i % 4) * ((r.width - 2) / 4), 3 + (i / 4) * 2,
                     (r.width - 2) / 4 - 1, labels[i], 1);
    display();
    mb_ui_exit();
}
static void event(const MB_Event *e) {
    if (e->action == MB_RESIZE || e->action == MB_THEME_CHANGED) {
        activate();
        return;
    }
    uint16_t key;
    if (mb_ui_event(e, &key) == MB_OK) {
        if (key == MB_UI_EXIT) {
            mb_app_close();
            return;
        }
        mb_calc_press(&calculator, key);
        display();
    }
}
static void tick(void) {}
static void stop(void) {}
const MB_App calculator_app = {MB_API_VERSION, "Calculator", init, activate,
                               event,          tick,         stop, stop};
