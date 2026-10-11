# Clock

[MegaBenchOS](../../README.md) / [Apps](../README.md) / Clock

How long has MegaBenchOS been running? Clock keeps the time since boot ticking in the centre of its window.

Restarting the OS starts it over. This is runtime, rather than the time of day.

## Controls

B closes the window. In fullscreen, use Exit or hold Start+B.

## Development

[clock.c](clock.c) uses the shared time and window APIs, updates once per displayed second and redraws on resize or theme changes. PAL and NTSC use the OS's nominal regional timebase. See the [SDK](../../sdk/README.md), [build guide](../../docs/BUILD.md) and [testing](../../docs/testing.md).
