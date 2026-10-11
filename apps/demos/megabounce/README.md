# MegaBounce

[MegaBenchOS](../../../README.md) / [Apps](../../README.md) / [Demos](../README.md) / MegaBounce

A rotating checker sphere that roams across the screen, rebounds from the sides and varies its bounce heights and direction. Squash, shadow and impact sounds give it weight.

## Controls

A changes speed, Start pauses and C exits. Hold Start+B to return to the desktop at any time.

## Development

[megabounce.c](../megabounce.c) uses generated spin/squash frames and the shared sprite, palette and audio APIs. Movement uses fixed-point positions and the PAL/NTSC timebase.

Artwork generation is documented in [build](../../../docs/BUILD.md); graphics ownership in the [SDK](../../../docs/API.md). See [testing](../../../docs/testing.md) for PAL/NTSC checks.

## Licensing

Original sphere artwork and sound effects follow the [project licence](../../../README.md#licensing).
