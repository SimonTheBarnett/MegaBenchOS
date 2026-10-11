/* Minimal shared helpers for ordinary MB_App demos; no alternate lifecycle. */
#ifndef MB_DEMO_COMMON_H
#define MB_DEMO_COMMON_H
#include "megabenchos.h"
typedef struct {
    MB_PaletteLease lease;
    uint32_t last, elapsed;
    uint16_t paused, ready, overlay;
} Demo;
MB_Result demo_begin(Demo *d, const char *name, const uint32_t *tiles, uint16_t count);
uint16_t demo_step(Demo *d);
void demo_end(Demo *d);
void demo_help(Demo *d, const char *title, const char *controls);
uint16_t demo_event(Demo *d, const MB_Event *e);
#endif
