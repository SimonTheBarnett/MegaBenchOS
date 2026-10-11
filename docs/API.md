# Public SDK

Apps include [megabenchos.h](../sdk/include/megabenchos.h) and app-local headers. Start with [Creating Your First MegaBench App](FIRST_APP.md), then use the [complete function reference](api/core.md) for exact signatures, parameters, return values, errors and ownership.

[api/core.json](../api/core.json) defines the contracts. `make generate` generates the header, reference and metadata. Source API additions through 1.7 retain the v1 `MB_App` descriptor; there is no external binary ABI or process isolation.

## Services

| Service | Use |
| --- | --- |
| App lifecycle | Launch, close, dirty-close guards and associated document paths |
| Windows and widgets | Client geometry, themes, buttons, menus and notifications |
| Input | Controller focus, pointer events, game direction presses and Sega Mouse |
| Graphics | Exclusive palette/pattern/sprite leases and bounded tile uploads |
| Canvas and cursors | Borrowed 16-colour image sources, zoom, pan, damage and copied custom cursors |
| Files and text | Session RAM files, ROM resources and semantic keyboard commands |
| Audio and music | PSG effects and bundled XGM2 playback |
| Time and system | Session runtime, capabilities and diagnostics |

## Lifecycle and events

Implement every `MB_App` callback and return promptly. The descriptor is borrowed ROM data until shutdown; event pointers are valid only during their callback. `init` returns `MB_OK` or an error and must unwind partial setup on failure. Normal close calls `deactivate`, then `shutdown`, and releases shared resources.

Request launch or close from event/tick callbacks; the runtime defers the transition until dispatch finishes. Do not initiate lifecycle changes from init, activate, deactivate or shutdown. Dirty editors use `mb_app_close_guard` and handle `MB_CLOSE_REQUEST` before explicitly closing.

Redraw on `MB_RESIZE` and `MB_THEME_CHANGED`. Mode transitions notify display lost, mode changed, resize and display restored in order. Focus notifications use x=1 for ownership. Notifications may coalesce; queries return current state.

## Coordinates and input

Drawing and widgets use client-relative tiles. Window/client rectangle origins are screen-relative tiles; pointer events use screen pixels. Query client/content geometry after mode changes. See [window and menu rules](WINDOW_LAYOUT.md).

In `MB_INPUT_GAME`, `MB_NAVIGATE` reports newly pressed D-pad directions without automatic repeat. Both axes may be nonzero. `mb_input_state` reports held buttons; games decide simultaneous-axis priority and movement buffering.

B normally requests exit; apps can intercept it for menu cancellation. Held Start+B remains the universal one-second exit shortcut. C+Start cycles supported window modes. Unsupported modes return `MB_UNAVAILABLE`; active graphics leases can return `MB_BUSY`.

## Buffers and ownership

The full reference specifies every function's input bounds and lifetime. In particular:

| API/data | Required storage and lifetime |
| --- | --- |
| `mb_format_u32` / `mb_format_i32` | Caller-owned output; 11 / 12 bytes cover all values, including NUL |
| `mb_time_format` | Caller-owned output; 14 bytes cover all uint32 second values |
| `mb_app_document` | Caller-owned output; 44 bytes cover accepted association paths |
| `mb_file_read` | Caller-owned `capacity` bytes and byte-count output; data copied synchronously |
| `mb_graphics_load` | Immutable borrowed pattern rows until ready or cancellation; eight uint32 rows per tile |
| `mb_graphics_map` | 1120 uint16 cells, copied during the call |
| `mb_graphics_map_region` | Contiguous `width * height` uint16 cells, copied during the call |
| `mb_palette_set` / `mb_canvas_palette` | Sixteen legal RGB333 words, read synchronously |
| `MB_CanvasSource` | Keep image context and callbacks valid while attached; close before reusing source storage |
| `mb_canvas_update` | Borrowed 1024-byte packed 2bpp image until canvas/app close |
| `mb_cursor_set` | Descriptor and sixteen bitmap rows copied synchronously; stack-local input is safe |

Use bounded static app workspaces; they remain reserved while the app is closed. There is no public app allocator. Never call foreground SDK services from interrupts. Check result codes before using outputs, and query capabilities before optional operations.

## Graphics and UI

Windowed apps use semantic theme colours. One fullscreen app may lease all four palette lines, available patterns and up to 79 sprites. Lease tokens expire on release/close and block mode/theme changes. H40 sprite limits also apply: 20 sprites and 320 sprite pixels per scanline; the service checks conservative eight-line bands.

Borrowed ROM uploads stream at most 24 tiles per flush. The OS owns retained maps and queued palette/sprite commits. Close cancels uploads before shutdown. Ordinary apps must not access SGDK or hardware directly.

Buttons and menu headings share a 24-entry pool and copy at most 11 label characters. Disabled controls do not activate. Menus support heading-toggle and outside-click dismissal; see [layout](WINDOW_LAYOUT.md). Custom cursors and hotspots are documented in [cursors](CURSORS.md).

Canvas sources provide pixel and optional native tile callbacks, integer zoom/pan and dirty rectangles. Callbacks run in bounded foreground video work. PAL2 and canvas patterns belong to the OS; close restores the desktop palette. `mb_canvas_ready` reports when pending transfers settle. See [canvas limits](MEGAPAINT.md).

## Files, text and audio

Use public [VFS](VFS.md) hooks for RAM and ROM. File metadata is copied; handles are opaque and foreground-owned. Open never truncates. Whole-file replacement preserves existing files on failure. Check read, write and close errors, and preserve unsaved edits. Versioned queries currently require version 1. Persistent SAVE/SD operations return `MB_NOT_SUPPORTED`.

Shared text input returns commands; the app owns the document buffer and its bounds. Picture serialization is specified in [MBP formats](MBP_FORMAT.md).

Audio has one foreground owner. PSG requests accept 8..15: audible values 8..14 currently use maximum hardware volume; 15 mutes. Bundled music uses the XGM2 service and separate mix. Stop and app close release audio. External music files are inspectable through the VFS but playback is unsupported; see [music conversion](MUSIC_CONVERSION.md).

Session time uses a nominal PAL/NTSC VBlank timebase and includes transitions. It is runtime since boot, not wall-clock time. SRAM/SD persistence, RTC, networking and external app loading are unavailable.
