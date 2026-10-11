# Sound Test

[MegaBenchOS](../../README.md) / [Apps](../README.md) / Sound Test

Give the Mega Drive's PSG a spin: three tones, a noise generator and a Stop button.

## Controls

D-pad and A select **220 Hz**, **440 Hz**, **880 Hz**, **Noise** or **Stop**. Pointer controls work too. B closes the app and stops the sound; fullscreen also supports Exit and held Start+B.

## Development

[sound.c](sound.c) uses the public audio and widget APIs. All audible API effects currently use maximum PSG volume; attenuation 15 mutes. Music has separate ownership through MegaPlayer, and app exit releases audio. See [audio contracts](../../docs/API.md), the [SDK](../../sdk/README.md) and [testing](../../docs/testing.md).
