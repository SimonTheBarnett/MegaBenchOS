/**
 * @file apps/snake/model.c
 * @brief Snake model: bounded geometry, growth, speed levels, safe obstacles and deterministic
 * food.
 *
 * ROM-linked application code. Shared hardware and resource ownership remain in OS services.
 * Uses only megabenchos.h and app-local interfaces; static state/ROM, no heap or hardware.
 */
#include "model.h"
uint16_t mb_snake_level(const MB_Snake *s) {
    return s->score / 4 < 4 ? 1 + s->score / 4 : 5;
}
uint16_t mb_snake_interval(const MB_Snake *s) {
    /* Nominal 300-Hz ticks: start at 5 moves/s, then gradually reach 10.7. */
    return 60 - (mb_snake_level(s) - 1) * 8;
}
static uint16_t occupied(const MB_Snake *s, uint16_t cell) {
    uint16_t i;
    for (i = 0; i < s->length; ++i)
        if (s->body[i] == cell)
            return 1;
    for (i = 0; i < s->rock_count; ++i)
        if (s->rocks[i] == cell)
            return 1;
    return 0;
}
/* Add one bounded obstacle on level-up, never on food/body or within two
 * Manhattan steps of the head. Skip it if no safe position exists. */
static void rock(MB_Snake *s) {
    uint16_t i, cells = s->width * s->height, cell;
    int16_t hx = s->body[0] % s->width, hy = s->body[0] / s->width;
    if (s->rock_count == SNAKE_ROCKS)
        return;
    s->seed = (uint16_t)(s->seed * 25173u + 13849u);
    cell = s->seed % cells;
    for (i = 0; i < cells; ++i, cell = (cell + 1) % cells) {
        int16_t dx = cell % s->width - hx, dy = cell / s->width - hy;
        if (dx < 0)
            dx = -dx;
        if (dy < 0)
            dy = -dy;
        if (dx + dy > 2 && cell != s->food && !occupied(s, cell)) {
            s->rocks[s->rock_count++] = cell;
            return;
        }
    }
}
static void food(MB_Snake *s) {
    uint16_t i, cell;
    s->seed = (uint16_t)(s->seed * 25173u + 13849u);
    uint16_t cells = s->width * s->height;
    cell = s->seed % cells;
    for (i = 0; i < cells; i++) {
        if (!occupied(s, cell)) {
            s->food = cell;
            return;
        }
        cell = (cell + 1) % cells;
    }
    s->phase = SNAKE_WON;
}
void mb_snake_reset(MB_Snake *s) {
    mb_snake_size(s, SNAKE_WIDTH, SNAKE_HEIGHT);
}
void mb_snake_size(MB_Snake *s, uint16_t width, uint16_t height) {
    uint16_t i;
    if (width < 6 || height < 4 || (uint32_t)width * height > SNAKE_CELLS)
        return;
    s->width = width;
    s->height = height;
    s->rock_count = 0;
    for (i = 0; i < SNAKE_ROCKS; ++i)
        s->rocks[i] = 0;
    for (i = 0; i < SNAKE_CELLS; i++)
        s->body[i] = 0;
    s->body[0] = (height / 2) * width + width / 4;
    s->body[1] = s->body[0] - 1;
    s->body[2] = s->body[0] - 2;
    s->length = 3;
    s->score = 0;
    s->direction = SNAKE_RIGHT;
    s->queued = SNAKE_RIGHT;
    s->turned = 0;
    s->buffered = SNAKE_RIGHT;
    s->phase = SNAKE_PLAYING;
    s->seed = 1;
    s->moves = 0;
    food(s);
}
void mb_snake_turn(MB_Snake *s, uint16_t dir) {
    uint16_t previous = s->turned == 2 ? s->buffered : s->turned ? s->queued : s->direction;
    /* Validate against the last accepted turn, not the still-moving head.
     * Ignore duplicate/reverse inputs without consuming either queue slot. */
    if (dir > 3 || s->phase != SNAKE_PLAYING || s->turned == 2 || dir == previous ||
        dir == ((previous + 2) % 4))
        return;
    if (s->turned)
        s->buffered = dir;
    else
        s->queued = dir;
    s->turned++;
}
void mb_snake_pause(MB_Snake *s) {
    if (s->phase == SNAKE_PLAYING)
        s->phase = SNAKE_PAUSED;
    else if (s->phase == SNAKE_PAUSED)
        s->phase = SNAKE_PLAYING;
}
void mb_snake_step(MB_Snake *s) {
    uint16_t x, y, next, grow, i;
    if (s->phase != SNAKE_PLAYING)
        return;
    s->direction = s->queued;
    if (s->turned == 2) {
        s->queued = s->buffered;
        s->turned = 1;
    } else {
        s->turned = 0;
    }
    x = s->body[0] % s->width;
    y = s->body[0] / s->width;
    if ((s->direction == SNAKE_RIGHT && x == s->width - 1) || (s->direction == SNAKE_LEFT && !x) ||
        (s->direction == SNAKE_DOWN && y == s->height - 1) || (s->direction == SNAKE_UP && !y)) {
        s->phase = SNAKE_OVER;
        return;
    }
    next = s->body[0] + (s->direction == SNAKE_RIGHT  ? 1
                         : s->direction == SNAKE_LEFT ? -1
                         : s->direction == SNAKE_DOWN ? s->width
                                                      : -s->width);
    grow = next == s->food;
    for (i = 0; i < s->rock_count; ++i)
        if (s->rocks[i] == next) {
            s->phase = SNAKE_OVER;
            return;
        }
    for (i = 0; i < s->length - (grow ? 0 : 1); i++)
        if (s->body[i] == next) {
            s->phase = SNAKE_OVER;
            return;
        }
    if (grow && s->length < s->width * s->height) {
        s->length++;
        s->score++;
    }
    for (i = s->length - 1; i; i--)
        s->body[i] = s->body[i - 1];
    s->body[0] = next;
    s->moves++;
    if (grow) {
        if (!(s->score % 4))
            rock(s);
        food(s);
    }
}
