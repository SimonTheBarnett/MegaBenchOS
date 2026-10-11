# MegaBenchOS

A desktop for the Sega Mega Drive / Genesis. Draw pixel art, write notes, play music, chase a high score and explore fullscreen demos from one cartridge ROM.

![MegaBenchOS v1.0 tour: boot splash, desktop, apps and demos](docs/images/MegaBenchOS-v1-tour.gif)

## Download and play

Download the ROM from the [latest release](https://github.com/SimonTheBarnett/MegaBenchOS/releases/latest) and open it in BlastEm or Genesis Plus GX. The same ROM supports PAL and NTSC.

For emulator setup or building from source, follow the [Windows 11 quick start](docs/INSTALL.md). Physical console and cartridge compatibility still needs verification; see [testing](docs/testing.md).

## Features

- **MegaPaint:** native 320x208 pixel editing, 16 colours, brush presets, fill, zoom, pan and lossless picture files.
- **MegaPad:** notes with an on-screen keyboard and RAM Disk saving.
- **MegaPlayer:** five bundled Mega Drive songs, Play/Stop and composer credits.
- **Snake:** a full-window board, countdown, increasing speed, obstacles and sound cues.
- **Demos:** Ocean Dreams, StarDrive, MegaBounce and ColourStorm.
- **Utilities:** Calculator, Sound Test, runtime Clock and System settings.

The desktop has six folders, five themes, compact windows and controller or Sega Mouse navigation. **RAM Disk** supports browsing, opening, renaming and deleting session files within its 4 KiB capacity. Files disappear on restart; external storage and music-file playback are not yet supported.

[App guides](apps/README.md) cover individual controls and limits. [Release notes](docs/VERSIONS.md) describe v1.0.

## Controls

| Control | Action |
| --- | --- |
| D-pad / A | Select / activate |
| B | Back or request app exit |
| C | Switch pointer/focus mode; apps may use it differently |
| Start | Open System from the desktop; app-specific controls elsewhere |
| C+Start | Cycle supported window modes |
| Hold Start+B for one second | Exit the active app |

BlastEm defaults are arrows, **A/S/D** and **Enter** for D-pad, A/B/C and Start. Normal windows can be dragged; title bars have a close button. Desktop **Restart** begins a fresh session and replays the logo.

## Build and develop

Use Windows PowerShell 5.1 after [setup](docs/INSTALL.md):

```powershell
. ./scripts/env.ps1
make build
make run
```

The ROM is `dist/MegaBenchOS.bin`. VS Code provides **Build**, **Test** and **Run** tasks. Development uses C99 and SGDK 2.11. Start with [Creating Your First MegaBench App](docs/FIRST_APP.md), the [SDK guide](docs/API.md) and [contributing](docs/CONTRIBUTING.md).

## Project guides

| Guide | Contents |
| --- | --- |
| [Documentation](docs/README.md) | Build, architecture, testing and technical references |
| [Apps](apps/README.md) | App, game and demo controls |
| [SDK](sdk/README.md) | Public services and complete API reference |
| [OS services](kernel/README.md) | Implementation source map |
| [Music](music/README.md) | Credits, listening links and conversion |
| [Future ideas](TODO.md) | Planned features |

## Licensing

Original code and artwork use [MIT](LICENSE). Music has separate [CC0 credits](music/CREDITS.md). See [third-party notices](THIRD_PARTY_NOTICES.md) for component licences.
