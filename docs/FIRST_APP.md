# Creating your first MegaBench app

Build once using [BUILD.md](BUILD.md). Apps use C99 and only the public
`sdk/include/megabenchos.h` plus their own headers. Hardware and allocation belong
to the OS. The [Clock](../apps/clock/clock.c) and
[Calculator](../apps/calculator/calculator.c) are maintained reference apps.

## 1. Create the app

Create `apps/myapp/myapp.c` with this ROM-linked descriptor:

```c
#include "megabenchos.h"
static MB_Result init(void) { return MB_OK; }
static void draw(void) {
    MB_Rect r;
    if (mb_window_get_client_rect(&r) != MB_OK) return;
    mb_window_clear();
    mb_draw_field(0, 0, r.width, "My first app");
    if (r.height > 1) mb_draw_field(0, 1, r.width, "A closes this app");
}
static void activate(void) { mb_window_open("My App"); draw(); }
static void event(const MB_Event *e) {
    if (e->action == MB_ACTIVATE) mb_app_close();
    else if (e->action == MB_RESIZE || e->action == MB_MODE_CHANGED ||
             e->action == MB_THEME_CHANGED) draw();
}
static void tick(void) { }
static void stop(void) { }
const MB_App myapp_app = {
    MB_API_VERSION, "My App", init, activate, event, tick, stop, stop
};
```

## 2. Register it

Declare the descriptor in `inc/internal.h`. Add a new explicit app ID in
`api/core.json` immediately before `MB_APP_COUNT` and increment that upper bound.
Append `&myapp_app` to the matching slot in the runtime's `apps` array in
`kernel/app_runtime/runtime.c`; keep retired slot zero and existing IDs stable.
Add an Apps-folder entry with a unique entry ID in `desktop/shell.c`, following
Calculator's `E(...)` entry. Registry configuration is private integration code;
the application's source still uses only the public SDK. Ordinary C sources are
staged automatically by the build.

## 3. Build and try it

After loading the [build environment](BUILD.md), run `make generate`, `make build`, then
`make test`. Launch BlastEm, open Apps, select My App and press
A. Check its label, resize/theme redraw and close; also check PAL through the
existing emulator workflow. Tests compile all apps against public headers and
exercise lifecycle, file and graphics/audio restoration. Extend an existing
scenario if the new app needs behavior beyond those checks.

## Ownership

Keep the descriptor in ROM and return promptly from every callback. Event pointers are valid only for that call. Undo partial setup if `init` fails. App close is deferred during events/ticks; normal teardown calls `deactivate`, then `shutdown`.

Static buffers remain reserved while an app is closed. Keep them bounded and query geometry after resize. The [SDK guide](API.md) and [function reference](api/core.md) specify buffer sizes, return codes and lifetimes. Source packages are for ROM relinking, not runtime installation.
