# Architecture

MegaBenchOS is a C99/SGDK desktop for the Sega Mega Drive / Genesis. Apps are compiled into one ROM and share OS-managed input, windows, graphics, audio and files.

## Source layout

| Location | Responsibility |
| --- | --- |
| [src](../src/) | Boot logo, main loop, controller/mouse polling and VBlank integration |
| [desktop](../desktop/) | Folder registry, navigation, windows, themes and rendering |
| [kernel/app_runtime](../kernel/app_runtime/) | Foreground app lifecycle and resource cleanup |
| [kernel/services](../kernel/services/) | Shared system services |
| [kernel/storage](../kernel/storage/) | VFS, session RAM Disk and read-only ROM files |
| [kernel/media](../kernel/media/) | Bundled XGM2 music playback |
| [kernel/time](../kernel/time/) | Session timebase |
| [apps](../apps/) | ROM-linked apps, games and demos |
| [api/core.json](../api/core.json) | Public contracts that generate SDK headers and reference |
| [scripts](../scripts/) | Windows setup, SGDK staging, emulator launch and release packaging |
| [tools/native](../tools/native/) | C host validation, generators and emulator runner |

## App lifecycle

One cooperative foreground app runs at a time. The runtime validates its ROM descriptor, calls `init`, then activates it. Events and ticks must return promptly. Launch and close requests made during event/tick dispatch are deferred to avoid lifecycle recursion.

Closing deactivates and shuts down the app, closes its file handles, cancels pending graphics uploads, resets widgets and cursors, stops audio and restores the desktop's folder and selection. Dirty editors use a close guard to confirm unsaved changes. Apps have no process isolation or public heap allocator.

## Display and input

The display is 320x224 in both NTSC 60 Hz and PAL 50 Hz. The renderer retains tile maps and transfers dirty regions during bounded video flushes. Windowed apps use shared semantic colours; fullscreen apps may lease exclusive palettes, patterns and sprites. Mode and theme changes are blocked while a lease is active.

The shell offers controller focus and pointer navigation, plus Sega Mouse input on port 2. Games receive new directional presses and manage their own movement buffering. Held Start+B exits the active app; this cooperative shortcut cannot recover software that stops polling.

See [window layout](WINDOW_LAYOUT.md), [cursors](CURSORS.md) and [API ownership rules](API.md).

## Files, canvas and audio

The [VFS](VFS.md) provides session RAM files and immutable ROM resources. RAM Disk has a fixed 4 KiB medium and atomic whole-file replacement. Restart clears it. Persistent SRAM/SD storage and runtime file import are not implemented.

MegaPad uses shared text input. MegaPaint uses a bounded sparse tile model and the OS's 16-colour canvas service. The renderer owns the picture palette and dirty visible-tile uploads; app source data remains borrowed while attached. See [canvas limits](MEGAPAINT.md) and [picture formats](MBP_FORMAT.md).

Audio has one foreground owner. The OS mediates PSG effects and SGDK XGM2 playback, including Stop and exit cleanup. Music is converted ahead of time and bundled in ROM; external playback, streaming and seeking are unsupported. See [music conversion](MUSIC_CONVERSION.md).

## Timing and compatibility

Session time uses VBlank interrupts with a nominal 300 Hz timebase for both regions. It includes display transitions and is not a real-time clock. H32, PAL240, keyboard input, networking and dynamic app loading are unsupported. Emulator checks do not establish physical hardware compatibility; see [testing](testing.md).
