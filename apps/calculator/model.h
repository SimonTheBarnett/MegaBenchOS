/* Calculator model: bounded signed 32-bit arithmetic, entry, overflow and division errors.
 * Uses only megabenchos.h and app-local interfaces; static state/ROM, no heap or hardware.
 * Callbacks return promptly; drawing and resource restoration belong to the OS. */
#ifndef MB_CALC_MODEL_H
#define MB_CALC_MODEL_H
#include "megabenchos.h"
#ifdef MB_HOST_TEST
#define MB_MODEL_EXPORT __declspec(dllexport)
#else
#define MB_MODEL_EXPORT
#endif
typedef struct {
    int32_t value, left;
    uint16_t operation, entering;
    MB_Result error;
} MB_Calculator;
enum {
    CALC_ADD = 10,
    CALC_SUB = 11,
    CALC_MUL = 12,
    CALC_DIV = 13,
    CALC_EQUAL = 14,
    CALC_CLEAR = 15,
    CALC_NEG = 16,
    CALC_DELETE = 17
};
MB_MODEL_EXPORT MB_Result mb_calc_math(int32_t a, int32_t b, uint16_t op, int32_t *out);
MB_MODEL_EXPORT void mb_calc_reset(MB_Calculator *c);
MB_MODEL_EXPORT void mb_calc_press(MB_Calculator *c, uint16_t key);
#endif
