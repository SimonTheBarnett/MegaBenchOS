/**
 * @file tests/unit/main.c
 * @brief Registers production-model Unity tests and writes sparse-canvas fixture metrics.
 *
 * Host Unity fixture linked against production C code; no emulator or hardware dependency.
 */
#include "unity.h"
#include <stdio.h>
#include <stdint.h>
void test_applications_calculator(void), test_sdk_formatters(void), test_input_snake(void),
    test_desktop_layout_and_exit(void);
int mb_creative_tests(void), mb_paint_tests(void);
uint32_t mb_paint_metric(uint16_t);
static int creative_assertions, paint_assertions;
void setUp(void) {}
void tearDown(void) {}
static void test_filesystem_and_pad_roundtrip(void) {
    Unity.TestFile = "tests/host/creative.c";
    creative_assertions = mb_creative_tests();
}
static void test_graphics_sparse_paint(void) {
    Unity.TestFile = "tests/host/paint.c";
    paint_assertions = mb_paint_tests();
}
int main(void) {
    int failures;
    UNITY_BEGIN();
    Unity.TestFile = "tests/unit/models.c";
    RUN_TEST(test_applications_calculator);
    RUN_TEST(test_sdk_formatters);
    RUN_TEST(test_input_snake);
    RUN_TEST(test_desktop_layout_and_exit);
    RUN_TEST(test_graphics_sparse_paint);
    RUN_TEST(test_filesystem_and_pad_roundtrip);
    failures = UNITY_END();
    printf("Creative fixture assertions: %d\n", creative_assertions + paint_assertions);
    if (!failures) {
        FILE *report = fopen("reports/paint-model-metrics.json", "w");
        const char *names[] = {"sparse_document_bytes",
                               "fill_workspace_bytes",
                               "blank_320x208_file_bytes",
                               "brush_fixture_file_bytes",
                               "maximum_512x256_fixture_file_bytes",
                               "mixed_tile_capacity",
                               "fill_eight_tile_steps",
                               "filled_fixture_file_bytes"};
        int i;
        if (!report) {
            perror("paint metrics report");
            return 1;
        }
        fputs("{", report);
        for (i = 0; i < 8; i++)
            fprintf(report, "%s\"%s\":%d", i ? "," : "", names[i],
                    (int)mb_paint_metric((uint16_t)i));
        fputs("}\n", report);
        if (fclose(report))
            return 1;
    }
    return failures ? 1 : 0;
}
