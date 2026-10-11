/**
 * @file apps/calculator/model.c
 * @brief Calculator model: bounded signed 32-bit arithmetic, entry, overflow and division errors.
 *
 * ROM-linked application code. Shared hardware and resource ownership remain in OS services.
 * Uses only megabenchos.h and app-local interfaces; static state/ROM, no heap or hardware.
 */
#include "model.h"
#define MAX_I32 ((int32_t)2147483647)
#define MIN_I32 (-MAX_I32 - 1)
static uint32_t magnitude(int32_t v) {
    return v < 0 ? 0u - (uint32_t)v : (uint32_t)v;
}
MB_Result mb_calc_math(int32_t a, int32_t b, uint16_t op, int32_t *out) {
    uint32_t x, y, z, limit;
    uint16_t negative;
    if (!out)
        return MB_INVALID;
    if (op == CALC_ADD) {
        if ((b > 0 && a > MAX_I32 - b) || (b < 0 && a < MIN_I32 - b))
            return MB_OVERFLOW;
        *out = a + b;
    } else if (op == CALC_SUB) {
        if ((b < 0 && a > MAX_I32 + b) || (b > 0 && a < MIN_I32 + b))
            return MB_OVERFLOW;
        *out = a - b;
    } else if (op == CALC_MUL) {
        negative = (a < 0) != (b < 0);
        x = magnitude(a);
        y = magnitude(b);
        limit = negative ? 2147483648u : 2147483647u;
        if (y && x > limit / y)
            return MB_OVERFLOW;
        z = x * y;
        *out = negative ? (int32_t)(0u - z) : (int32_t)z;
    } else if (op == CALC_DIV) {
        if (!b)
            return MB_DIV_ZERO;
        if (a == MIN_I32 && b == -1)
            return MB_OVERFLOW;
        *out = a / b;
    } else
        return MB_INVALID;
    return MB_OK;
}
void mb_calc_reset(MB_Calculator *c) {
    c->value = 0;
    c->left = 0;
    c->operation = 0;
    c->entering = 0;
    c->error = MB_OK;
}
void mb_calc_press(MB_Calculator *c, uint16_t key) {
    uint32_t n, limit;
    int32_t result;
    if (key == CALC_CLEAR) {
        mb_calc_reset(c);
        return;
    }
    if (c->error != MB_OK)
        return;
    if (key <= 9) {
        if (!c->entering) {
            c->value = 0;
            c->entering = 1;
        }
        n = magnitude(c->value);
        limit = c->value < 0 ? 2147483648u : 2147483647u;
        if (n > (limit - key) / 10u) {
            c->error = MB_OVERFLOW;
            return;
        }
        n = n * 10u + key;
        c->value = c->value < 0 ? (int32_t)(0u - n) : (int32_t)n;
        return;
    }
    if (key == CALC_NEG) {
        if (c->value == MIN_I32)
            c->error = MB_OVERFLOW;
        else
            c->value = -c->value;
        c->entering = 1;
        return;
    }
    if (key == CALC_DELETE) {
        c->value /= 10;
        c->entering = 1;
        return;
    }
    if (key >= CALC_ADD && key <= CALC_EQUAL) {
        if (c->operation && c->entering) {
            c->error = mb_calc_math(c->left, c->value, c->operation, &result);
            if (c->error != MB_OK)
                return;
            c->value = result;
        }
        c->left = c->value;
        c->operation = key == CALC_EQUAL ? 0 : key;
        c->entering = 0;
    }
}
