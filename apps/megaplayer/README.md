# MegaPlayer

[MegaBenchOS](../../README.md) / [Apps](../README.md) / MegaPlayer

Five full Mega Drive tracks, ready to play from the desktop. Pick a song, hit Play and let the FM and PSG do their thing.

## Controls

Choose **Bundled**, select a track, then use **Play** or **Stop** with controller or pointer controls. **Credits** opens the selected song's attribution; Back returns to the library while the music keeps playing. B, the window's close button or held Start+B exits and stops the audio.

Songs play through once; Play starts them again. Switching between Bundled and RAM Disk stops the current track.

The **RAM Disk** library browses `RAM:/Music/`. External `.vgm` and `.xgc` files can be inspected, but playback from files is still future work.

## Development

[megaplayer.c](megaplayer.c) uses the public media and VFS APIs. Preconverted XGM2 resources stay in ROM, and the OS owns playback and audio cleanup. The app holds no song buffer. See [media contracts](../../docs/API.md#files-text-and-audio), [conversion](../../docs/MUSIC_CONVERSION.md) and the [SDK](../../sdk/README.md).

Source packages require ROM relinking and the host's media service; they do not include music assets. See [building and packaging](../../docs/BUILD.md).

## Licensing

Music by Safety Stoat Studios and Spring uses CC0. [Track credits and listening links](../../music/CREDITS.md) record each source and creator. Original app code follows the [project licence](../../README.md#licensing).
