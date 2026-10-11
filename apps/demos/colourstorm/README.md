# ColourStorm

[MegaBenchOS](../../../README.md) / [Apps](../../README.md) / [Demos](../README.md) / ColourStorm

A fullscreen colour show: aurora, plasma waves, rotating bands and a sunrise sweep, all built from the Mega Drive's palette.

## Controls

Left/right selects a look, A cycles normal, fast and reduced-motion modes, and Start pauses. C exits; held Start+B returns to the desktop. Reduced motion freezes palette and rotation updates.

## Development

[colourstorm.c](../colourstorm.c) uses generated maps and the shared palette and retained-plane APIs. Updates run through the normal video flush.

Artwork generation is documented in [build](../../../docs/BUILD.md); graphics ownership in the [SDK](../../../docs/API.md). See [testing](../../../docs/testing.md) for PAL/NTSC checks.

## Licensing

Original colour tables and geometry follow the [project licence](../../../README.md#licensing).
