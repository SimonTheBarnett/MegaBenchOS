# Ocean Dreams

[MegaBenchOS](../../../README.md) / [Apps](../../README.md) / [Demos](../README.md) / Ocean Dreams

Three coastal scenes for a change of pace: tropical midday, amber sunset and moonlight. Sea bands ripple, reflections drift and the foam gently washes up and down the shore, with fades between scenes.

## Controls

Left/right changes the scene, A toggles surf sound and Start shows or hides the scene label. C exits; held Start+B returns to the desktop too. A short controls overlay appears when the demo opens.

## Development

[ocean.c](../ocean.c) animates generated scene maps through the shared region API. It loads one scene at a time and uses public palette and PSG services.

Artwork generation is documented in [build](../../../docs/BUILD.md); graphics ownership in the [SDK](../../../docs/API.md). See [testing](../../../docs/testing.md) for PAL/NTSC checks.

## Licensing

Original artwork and surf effects follow the [project licence](../../../README.md#licensing).
