/* Snake model: bounded geometry, growth, speed levels, safe obstacles and deterministic food.
 * Uses only megabenchos.h and app-local interfaces; static state/ROM, no heap or hardware.
 * Callbacks return promptly; drawing and resource restoration belong to the OS. */
#ifndef MB_SNAKE_MODEL_H
#define MB_SNAKE_MODEL_H
#include "megabenchos.h"
#ifdef MB_HOST_TEST
#define MB_SNAKE_EXPORT __declspec(dllexport)
#else
#define MB_SNAKE_EXPORT
#endif
#define SNAKE_WIDTH 20
#define SNAKE_HEIGHT 10
#define SNAKE_CELLS 400
#define SNAKE_ROCKS 4
enum { SNAKE_RIGHT = 0, SNAKE_DOWN = 1, SNAKE_LEFT = 2, SNAKE_UP = 3 };
enum { SNAKE_PLAYING = 0, SNAKE_PAUSED = 1, SNAKE_OVER = 2, SNAKE_WON = 3 };
typedef struct {
    uint16_t body[SNAKE_CELLS];
    uint16_t length, score, direction, queued, turned, food, phase, seed, moves;
    uint16_t width, height;
    uint16_t rocks[SNAKE_ROCKS], rock_count;
    /* Second pending turn; turned counts the bounded queue (0..2). */
    uint16_t buffered;
} MB_Snake;
MB_SNAKE_EXPORT void mb_snake_reset(MB_Snake *s);
MB_SNAKE_EXPORT void mb_snake_size(MB_Snake *s, uint16_t width, uint16_t height);
MB_SNAKE_EXPORT void mb_snake_turn(MB_Snake *s, uint16_t direction);
MB_SNAKE_EXPORT void mb_snake_step(MB_Snake *s);
MB_SNAKE_EXPORT void mb_snake_pause(MB_Snake *s);
MB_SNAKE_EXPORT uint16_t mb_snake_level(const MB_Snake *s);
MB_SNAKE_EXPORT uint16_t mb_snake_interval(const MB_Snake *s);
#endif
