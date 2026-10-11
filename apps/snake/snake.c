/**
 * @file apps/snake/snake.c
 * @brief SDK-only Snake: maximised native tile canvas, incremental cell damage and
 *
 * ROM-linked application code. Shared hardware and resource ownership remain in OS services.
 * short PSG cues. PAL2, VRAM transfers and restoration remain OS-owned.
 */
#include "megabenchos.h"
#include "model.h"
MB_Snake mb_snake_observation __attribute__((externally_visible));
static uint32_t last, sound_until;
static uint16_t cell_size, board_x, board_y, best, sound_next;
/* Zero waits for the bounded canvas upload; 900 nominal ticks are three seconds. */
volatile uint16_t mb_snake_countdown __attribute__((externally_visible));
static uint32_t countdown_last;
static uint16_t countdown_label;
static uint8_t field[SNAKE_CELLS];
static const uint16_t palette[16] = {0x000, 0x020, 0x040, 0x0a0, 0x0e0, 0x00e, 0x046, 0x08e,
                                     0x000, 0xeee, 0,     0,     0,     0,     0,     0};
static void quiet(void) {
    sound_until = sound_next = 0;
    mb_audio_stop();
}
static void cue(uint16_t frequency, uint16_t duration, uint16_t next) {
    if (mb_audio_tone(frequency, 11) == MB_OK) {
        sound_until = mb_time_ticks() + duration;
        sound_next = next;
    }
}
static void layout(void) {
    MB_Rect r;
    uint16_t width, height;
    mb_window_get_client_rect(&r);
    cell_size = r.width >= 40 ? 2 : 1;
    width = r.width / cell_size;
    height = (r.height - 1) / cell_size;
    while ((uint32_t)width * height > SNAKE_CELLS)
        height--;
    board_x = (r.width - width * cell_size) / 2;
    board_y = 1 + (r.height - 1 - height * cell_size) / 2;
    if (width != mb_snake_observation.width || height != mb_snake_observation.height)
        mb_snake_size(&mb_snake_observation, width, height);
}
/* Centre overlay is generated into existing canvas tiles, without new patterns
 * or a framebuffer. Only its small rectangle is damaged on each digit change. */
static void countdown_damage(void) {
    MB_Snake *s = &mb_snake_observation;
    mb_canvas_damage((s->width * cell_size * 8 - 40) / 2, (s->height * cell_size * 8 - 56) / 2, 40,
                     56);
}
static uint16_t countdown_pixel(uint16_t x, uint16_t y, uint16_t digit) {
    static const uint16_t segments[] = {0, 0x06, 0x5b, 0x4f};
    uint16_t mask = segments[digit];
    if (x >= 8 && x < 32 && y >= 8 && y < 48) {
        x -= 8;
        y -= 8;
        /* A lone vertical 1 should be visually centred, rather than sitting
         * at the right edge of the shared seven-segment digit box. */
        if (digit == 1)
            x += 10;
        if (((mask & 1) && y < 4 && x >= 4 && x < 20) ||
            ((mask & 2) && x >= 20 && x < 24 && y >= 4 && y < 18) ||
            ((mask & 4) && x >= 20 && x < 24 && y >= 22 && y < 36) ||
            ((mask & 8) && y >= 36 && x >= 4 && x < 20) ||
            ((mask & 16) && x < 4 && y >= 22 && y < 36) ||
            ((mask & 32) && x < 4 && y >= 4 && y < 18) ||
            ((mask & 64) && y >= 18 && y < 22 && x >= 4 && x < 20))
            return 9;
    }
    return 8;
}
/* Small occupancy cache is one byte per game cell, not a pixel framebuffer. */
static void tile(const void *context, uint16_t tx, uint16_t ty, uint32_t *rows) {
    const MB_Snake *s = context;
    uint16_t cx = tx / cell_size, cy = ty / cell_size;
    uint16_t value = field[cy * s->width + cx], size = cell_size * 8;
    uint16_t bx = (tx % cell_size) * 8, by = (ty % cell_size) * 8, x, y;
    uint16_t background = 1 + ((cx + cy) & 1);
    uint16_t overlay = mb_snake_countdown && s->phase == SNAKE_PLAYING;
    uint16_t ox = (s->width * size - 40) / 2, oy = (s->height * size - 56) / 2;
    overlay = overlay && tx * 8 < ox + 40 && tx * 8 + 8 > ox && ty * 8 < oy + 56 && ty * 8 + 8 > oy;
    if (!value && !overlay) {
        uint32_t row = (uint32_t)background * 0x11111111;
        for (y = 0; y < 8; ++y)
            rows[y] = row;
        return;
    }
    for (y = 0; y < 8; ++y) {
        uint32_t row = 0;
        for (x = 0; x < 8; ++x) {
            uint16_t px = bx + x, py = by + y, color = background;
            if ((value == 3 || value == 4) && px && py && px < size - 1 && py < size - 1) {
                color = value;
                if (value == 4) {
                    uint16_t a = size / 4, b = size - a - 1;
                    if ((s->direction == SNAKE_RIGHT && px == b && (py == a || py == b)) ||
                        (s->direction == SNAKE_LEFT && px == a && (py == a || py == b)) ||
                        (s->direction == SNAKE_UP && py == a && (px == a || px == b)) ||
                        (s->direction == SNAKE_DOWN && py == b && (px == a || px == b)))
                        color = 8;
                }
            } else if (value == 5) {
                int16_t dx = px - size / 2, dy = py - size / 2;
                if (dx < 0)
                    dx = -dx;
                if (dy < 0)
                    dy = -dy;
                if (dx + dy <= size / 3)
                    color = 5;
                if (px == size / 2 - 1 && py == size / 2 - 1)
                    color = 9;
            } else if (value == 6 && px > 1 && py > 1 && px < size - 2 && py < size - 2)
                color = ((px + py) & 3) < 2 ? 6 : 7;
            if (overlay && tx * 8 + x >= ox && tx * 8 + x < ox + 40 && ty * 8 + y >= oy &&
                ty * 8 + y < oy + 56)
                color = countdown_pixel(tx * 8 + x - ox, ty * 8 + y - oy, countdown_label);
            row = (row << 4) | color;
        }
        rows[y] = row;
    }
}
static void damage(uint16_t cell) {
    MB_Snake *s = &mb_snake_observation;
    mb_canvas_damage((cell % s->width) * cell_size * 8, (cell / s->width) * cell_size * 8,
                     cell_size * 8, cell_size * 8);
}
static void status(void) {
    char b[11];
    MB_Rect r;
    MB_Snake *s = &mb_snake_observation;
    mb_window_get_client_rect(&r);
    mb_draw_field(0, 0, r.width, "");
    mb_draw_text(0, 0, "Score:");
    mb_format_u32(s->score, b, sizeof(b));
    mb_draw_field(6, 0, 4, b);
    mb_draw_text(11, 0, "Lv:");
    mb_format_u32(mb_snake_level(s), b, sizeof(b));
    mb_draw_field(15, 0, 2, b);
    if (r.width >= 40) {
        mb_draw_text(18, 0, "Best:");
        mb_format_u32(best, b, sizeof(b));
        mb_draw_field(23, 0, 4, b);
    }
    if (s->phase == SNAKE_PLAYING && mb_snake_countdown) {
        mb_draw_field(r.width - 12, 0, 12, "Get ready");
        return;
    }
    mb_draw_field(r.width - 12, 0, 12,
                  s->phase == SNAKE_OVER     ? "Over:A retry"
                  : s->phase == SNAKE_WON    ? "Won:A retry"
                  : s->phase == SNAKE_PAUSED ? "Pause:A exit"
                                             : "Playing");
}
static void paint(void) {
    MB_Snake *s = &mb_snake_observation;
    MB_CanvasSource source = {1, s->width * cell_size * 8, s->height * cell_size * 8, s, 0, tile};
    uint16_t i;
    MB_Result result;
    for (i = 0; i < SNAKE_CELLS; ++i)
        field[i] = 0;
    if (s->phase != SNAKE_OVER)
        for (i = 0; i < s->length; ++i)
            field[s->body[i]] = i ? 3 : 4;
    for (i = 0; i < s->rock_count; ++i)
        field[s->rocks[i]] = 6;
    if (s->phase != SNAKE_WON)
        field[s->food] = 5;
    mb_window_clear();
    result = mb_canvas_attach(&source, board_x, board_y, s->width * cell_size,
                              s->height * cell_size, palette);
    if (result != MB_OK) {
        mb_app_fail(result);
        return;
    }
    mb_canvas_view(0, 0, 1, 65535, 65535);
    /* Reattaching the same source retains patterns; retry/theme repaint replaces
     * occupancy in place and must explicitly invalidate the complete image. */
    mb_canvas_damage(0, 0, source.width, source.height);
    if (s->phase == SNAKE_PLAYING) {
        mb_snake_countdown = 900;
        countdown_last = 0;
        countdown_label = 3;
    }
    status();
}
static MB_Result init(void) {
    mb_snake_reset(&mb_snake_observation);
    last = mb_time_ticks();
    return MB_OK;
}
static void activate(void) {
    mb_window_open("Snake");
    mb_window_set_mode(MB_WINDOW_MAXIMIZED);
    mb_input_mode(MB_INPUT_GAME);
    layout();
    last = mb_time_ticks();
    paint();
}
static void event(const MB_Event *e) {
    MB_Snake *s = &mb_snake_observation;
    if (e->action == MB_RESIZE || e->action == MB_THEME_CHANGED) {
        quiet();
        layout();
        last = mb_time_ticks();
        paint();
        return;
    }
    if (e->action == MB_NAVIGATE) {
        if (e->x > 0)
            mb_snake_turn(s, SNAKE_RIGHT);
        else if (e->x < 0)
            mb_snake_turn(s, SNAKE_LEFT);
        else if (e->y > 0)
            mb_snake_turn(s, SNAKE_DOWN);
        else if (e->y < 0)
            mb_snake_turn(s, SNAKE_UP);
    } else if (e->action == MB_MENU || e->action == MB_CONTEXT) {
        mb_snake_pause(s);
        if (mb_snake_countdown)
            countdown_damage();
        quiet();
        last = mb_time_ticks();
        status();
    } else if (e->action == MB_ACTIVATE && s->phase == SNAKE_PAUSED)
        mb_app_close();
    else if (e->action == MB_ACTIVATE && (s->phase == SNAKE_OVER || s->phase == SNAKE_WON)) {
        quiet();
        mb_snake_size(s, s->width, s->height);
        last = mb_time_ticks();
        paint();
    }
}
static void tick(void) {
    uint32_t now = mb_time_ticks();
    MB_Snake *s = &mb_snake_observation;
    uint16_t tail, head, old_food, length, rocks, steps = 0, interval;
    if (sound_until && (int32_t)(now - sound_until) >= 0) {
        if (sound_next) {
            uint16_t next = sound_next;
            cue(next, 45, 0);
        } else
            quiet();
    }
    if (s->phase != SNAKE_PLAYING) {
        if (mb_snake_countdown && countdown_last)
            countdown_last = now;
        last = now;
        return;
    }
    if (mb_snake_countdown) {
        uint16_t label;
        last = now;
        if (!countdown_last) {
            if (mb_canvas_ready() == MB_OK) {
                countdown_last = now;
                cue(440, 45, 0);
            }
            return;
        }
        {
            uint32_t elapsed = now - countdown_last;
            countdown_last = now;
            mb_snake_countdown = elapsed >= mb_snake_countdown ? 0 : mb_snake_countdown - elapsed;
        }
        label = (mb_snake_countdown + 299) / 300;
        if (label != countdown_label) {
            countdown_label = label;
            countdown_damage();
            status();
            if (label)
                cue(label == 2 ? 660 : 880, label == 1 ? 120 : 45, 0);
        }
        return;
    }
    while (s->phase == SNAKE_PLAYING &&
           (uint32_t)(now - last) >= (interval = mb_snake_interval(s)) && steps++ < 3) {
        last += interval;
        tail = s->body[s->length - 1];
        head = s->body[0];
        old_food = s->food;
        length = s->length;
        rocks = s->rock_count;
        mb_snake_step(s);
        if (s->phase == SNAKE_OVER) {
            uint16_t i;
            for (i = 0; i < s->length; ++i) {
                field[s->body[i]] = 0;
                damage(s->body[i]);
            }
            cue(220, 60, 110);
            status();
            break;
        }
        if (length == s->length) {
            field[tail] = 0;
            damage(tail);
        }
        field[head] = 3;
        damage(head);
        field[s->body[0]] = 4;
        damage(s->body[0]);
        if (length != s->length) {
            if (s->score > best)
                best = s->score;
            damage(old_food);
            if (s->phase != SNAKE_WON) {
                field[s->food] = 5;
                damage(s->food);
            }
            if (rocks != s->rock_count) {
                field[s->rocks[rocks]] = 6;
                damage(s->rocks[rocks]);
            }
            cue(s->score % 4 ? 880 : 660, 30, s->score % 4 ? 0 : 1320);
            status();
        }
    }
}
const MB_App snake_app = {MB_API_VERSION, "Snake", init, activate, event, tick, quiet, quiet};
