/**
 * @file tests/unit/models.c
 * @brief Native tests call the production models directly; no duplicated implementation.
 *
 * Host Unity fixture linked against production C code; no emulator or hardware dependency.
 */
#include "unity.h"
#include "megabenchos.h"
#include "../../apps/calculator/model.h"
#include "../../apps/snake/model.h"
#include <limits.h>
#include <string.h>
#include <stdlib.h>
uint16_t mb_grid_capacity(uint16_t, uint16_t);
uint16_t mb_escape_step(uint16_t *, uint16_t, uint16_t, uint32_t);
void test_applications_calculator(void) {
    const int64_t cases[][5] = {{12, 7, 10, 0, 19},           {12, 7, 11, 0, 5},
                                {-12, 7, 12, 0, -84},         {-12, 7, 13, 0, -1},
                                {1, 0, 13, -7, 123},          {INT32_MIN, -1, 13, -6, 123},
                                {INT32_MAX, 1, 10, -6, 123},  {INT32_MIN, 1, 11, -6, 123},
                                {INT32_MAX, 2, 12, -6, 123},  {INT32_MIN, 1, 12, 0, INT32_MIN},
                                {INT32_MIN, -1, 12, -6, 123}, {INT32_MIN, INT32_MIN, 11, 0, 0}};
    MB_Calculator c;
    unsigned i;
    int32_t value;
    for (i = 0; i < sizeof cases / sizeof cases[0]; i++) {
        value = 123;
        TEST_ASSERT_EQUAL_INT(cases[i][3], mb_calc_math((int32_t)cases[i][0], (int32_t)cases[i][1],
                                                        (uint16_t)cases[i][2], &value));
        TEST_ASSERT_EQUAL_INT32(cases[i][4], value);
    }
    mb_calc_reset(&c);
    {
        const uint16_t keys[] = {1, 2, 10, 3, 12, 2, 14};
        for (i = 0; i < 7; i++)
            mb_calc_press(&c, keys[i]);
    }
    TEST_ASSERT_TRUE(c.value == 30 && c.error == 0);
    mb_calc_press(&c, 15);
    for (i = 0; i < 10; i++)
        mb_calc_press(&c, (uint16_t)("2147483648"[i] - '0'));
    TEST_ASSERT_EQUAL_INT(-6, c.error);
    mb_calc_press(&c, 15);
    TEST_ASSERT_TRUE(c.value == 0 && c.error == 0);
    mb_calc_press(&c, 8);
    mb_calc_press(&c, 16);
    mb_calc_press(&c, 17);
    TEST_ASSERT_EQUAL_INT(0, c.value);
}
void test_sdk_formatters(void) {
    const uint32_t values[] = {0, 59, 60, 3599, 3600, 86400, UINT32_MAX};
    const char *expected[] = {"00:00:00", "00:00:59", "00:01:00",     "00:59:59",
                              "01:00:00", "24:00:00", "1193046:28:15"};
    char buffer[16];
    unsigned i;
    for (i = 0; i < 7; i++) {
        TEST_ASSERT_EQUAL_INT(MB_OK, mb_time_format(values[i], buffer, 14));
        TEST_ASSERT_EQUAL_STRING(expected[i], buffer);
    }
    TEST_ASSERT_TRUE(mb_time_format(0, buffer, 8) == MB_INVALID && buffer[0] == 0);
    TEST_ASSERT_EQUAL_INT(MB_INVALID, mb_time_format(0, 0, 14));
    TEST_ASSERT_EQUAL_INT(MB_OK, mb_format_i32(INT32_MIN, buffer, 12));
    TEST_ASSERT_EQUAL_STRING("-2147483648", buffer);
    TEST_ASSERT_EQUAL_INT(MB_OK, mb_format_u32(UINT32_MAX, buffer, 11));
    TEST_ASSERT_EQUAL_STRING("4294967295", buffer);
}
static int contains(const uint16_t *items, uint16_t count, uint16_t value) {
    uint16_t i;
    for (i = 0; i < count; i++)
        if (items[i] == value)
            return 1;
    return 0;
}
void test_input_snake(void) {
    MB_Snake s, t;
    uint16_t head, i;
    mb_snake_reset(&s);
    mb_snake_reset(&t);
    TEST_ASSERT_TRUE(s.food == t.food && s.seed == t.seed && s.body[0] == t.body[0]);
    mb_snake_turn(&s, 2);
    TEST_ASSERT_EQUAL_INT(0, s.turned);
    mb_snake_turn(&s, 3);
    mb_snake_turn(&s, 2);
    mb_snake_turn(&s, 1);
    TEST_ASSERT_TRUE(s.turned == 2 && s.queued == 3 && s.buffered == 2);
    mb_snake_step(&s);
    TEST_ASSERT_TRUE(s.body[0] == 85 && s.direction == 3 && s.turned == 1);
    mb_snake_step(&s);
    TEST_ASSERT_TRUE(s.body[0] == 84 && s.direction == 2 && s.turned == 0);
    mb_snake_reset(&s);
    mb_snake_turn(&s, 0);
    mb_snake_turn(&s, 3);
    mb_snake_turn(&s, 3);
    mb_snake_turn(&s, 1);
    mb_snake_turn(&s, 2);
    TEST_ASSERT_TRUE(s.turned == 2 && s.queued == 3 && s.buffered == 2);
    mb_snake_pause(&s);
    mb_snake_turn(&s, 1);
    mb_snake_step(&s);
    TEST_ASSERT_TRUE(s.body[0] == 105 && s.turned == 2);
    mb_snake_pause(&s);
    mb_snake_step(&s);
    mb_snake_step(&s);
    TEST_ASSERT_TRUE(s.body[0] == 84 && s.direction == 2);
    mb_snake_reset(&s);
    TEST_ASSERT_TRUE(s.turned == 0 && s.queued == 0 && s.buffered == 0);
    mb_snake_turn(&s, 3);
    mb_snake_step(&s);
    mb_snake_pause(&s);
    head = s.body[0];
    mb_snake_step(&s);
    TEST_ASSERT_EQUAL_INT(head, s.body[0]);
    mb_snake_pause(&s);
    for (i = 0; i < 6; i++)
        mb_snake_step(&s);
    TEST_ASSERT_EQUAL_INT(2, s.phase);
    mb_snake_reset(&s);
    s.food = 106;
    mb_snake_step(&s);
    TEST_ASSERT_TRUE(s.score == 1 && s.length == 4 && !contains(s.body, s.length, s.food));
    s.length = 4;
    s.body[0] = 21;
    s.body[1] = 1;
    s.body[2] = 0;
    s.body[3] = 20;
    s.direction = 2;
    s.queued = 2;
    s.food = 199;
    mb_snake_step(&s);
    TEST_ASSERT_TRUE(s.phase == 0 && s.body[0] == 20);
    s.body[0] = 21;
    s.body[1] = 20;
    s.body[2] = 0;
    s.body[3] = 1;
    s.direction = 2;
    s.queued = 2;
    mb_snake_step(&s);
    TEST_ASSERT_EQUAL_INT(2, s.phase);
    mb_snake_reset(&s);
    s.length = 199;
    for (i = 0; i < 199; i++)
        s.body[i] = i;
    s.body[0] = 198;
    s.body[198] = 0;
    s.food = 199;
    s.direction = 0;
    s.queued = 0;
    mb_snake_step(&s);
    TEST_ASSERT_TRUE(s.phase == 3 && s.length == 200 && s.score == 1);
    mb_snake_size(&s, 20, 13);
    TEST_ASSERT_TRUE(s.width == 20 && s.height == 13 && s.length == 3);
    mb_snake_size(&s, 40, 28);
    TEST_ASSERT_TRUE(s.width == 20 && s.height == 13);
    mb_snake_reset(&s);
    TEST_ASSERT_TRUE(mb_snake_level(&s) == 1 && mb_snake_interval(&s) == 60 && s.rock_count == 0);
    for (i = 0; i < 4; i++) {
        s.food = (uint16_t)(s.body[0] + 1);
        mb_snake_step(&s);
    }
    TEST_ASSERT_TRUE(s.score == 4 && s.rock_count == 1 && mb_snake_level(&s) == 2 &&
                     mb_snake_interval(&s) == 52);
    TEST_ASSERT_TRUE(!contains(s.body, s.length, s.rocks[0]) && s.food != s.rocks[0] &&
                     abs(s.rocks[0] % s.width - s.body[0] % s.width) +
                             abs(s.rocks[0] / s.width - s.body[0] / s.width) >
                         2);
    s.score = 100;
    TEST_ASSERT_TRUE(mb_snake_level(&s) == 5 && mb_snake_interval(&s) == 28);
    mb_snake_reset(&s);
    s.rock_count = 1;
    s.rocks[0] = (uint16_t)(s.body[0] + 1);
    mb_snake_step(&s);
    TEST_ASSERT_TRUE(s.phase == 2 && s.moves == 0);
    mb_snake_reset(&s);
    TEST_ASSERT_TRUE(s.rock_count == 0 && s.score == 0);
    s.score = 19;
    s.rock_count = 4;
    for (i = 0; i < 4; i++)
        s.rocks[i] = i;
    s.food = (uint16_t)(s.body[0] + 1);
    mb_snake_step(&s);
    TEST_ASSERT_TRUE(s.score == 20 && s.rock_count == 4 && mb_snake_interval(&s) == 28 &&
                     !contains(s.rocks, 4, s.food));
}
void test_desktop_layout_and_exit(void) {
    uint16_t elapsed = 0;
    TEST_ASSERT_TRUE(mb_grid_capacity(30, 14) == 6 && mb_grid_capacity(40, 26) == 16 &&
                     mb_grid_capacity(9, 2) == 0);
    TEST_ASSERT_EQUAL_INT(3, 23 / mb_grid_capacity(30, 14));
    TEST_ASSERT_TRUE(mb_escape_step(&elapsed, 160, 50, 49) == 0 && elapsed == 294);
    TEST_ASSERT_EQUAL_INT(1, mb_escape_step(&elapsed, 160, 50, 1));
    TEST_ASSERT_TRUE(mb_escape_step(&elapsed, 128, 60, 60) == 0 && elapsed == 0);
    TEST_ASSERT_TRUE(mb_escape_step(&elapsed, 160, 60, 59) == 0 &&
                     mb_escape_step(&elapsed, 160, 60, 1) == 1);
}
