# StarDrive

[MegaBenchOS](../../../README.md) / [Apps](../../README.md) / [Demos](../README.md) / StarDrive

Fly through a field of depth stars past a ringed planet, then kick into warp. Trails stretch outward as the speed builds, with a synth pulse along for the ride.

## Controls

Up/down changes speed, A toggles warp and Start pauses. C exits, or hold Start+B to return to the desktop.

## Development

[stardrive.c](../stardrive.c) projects fixed-point stars using the shared sprite and palette APIs. Warp trails ease between speeds.

Artwork generation is documented in [build](../../../docs/BUILD.md); graphics ownership in the [SDK](../../../docs/API.md). See [testing](../../../docs/testing.md) for PAL/NTSC checks.

## Licensing

Original planet artwork, star effects and synth pulse follow the [project licence](../../../README.md#licensing).
