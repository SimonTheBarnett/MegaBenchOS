# MegaPaint canvas reference

For tools and controls, use the [app guide](../apps/creative/megapaint/README.md). This page documents image limits and canvas integration.

## Dimensions and storage

The default canvas is native 320x208 beneath the maximised header and menu. Supported dimensions are nonzero multiples of eight, up to 512x256. Larger images pan; integer zoom preserves pixel shapes. Fit may use a half-size overview that samples pixels without altering image data.

The sparse model has one descriptor per 8x8 tile. Solid tiles use only a descriptor; mixed tiles share a fixed pool of **384 patterns**, representing at most 24576 detailed pixels. Repainting a tile solid releases its pattern. Detailed images can exhaust the pool even below the maximum dimensions.

Brush operations validate required tiles before modifying pixels. Imports validate format, checksum and tile demand before replacing the document. Capacity failures are explicit and preserve the current image; there is no automatic reduction in resolution.

Save/Open use `RAM:/Pictures/`. MBP2 stores dimensions, all sixteen RGB333 palette entries and lossless RLE pixels. MBP1 remains readable. Serialized files must fit the **3592-byte shared RAM Disk payload** and available free space. Save replaces the current file atomically; Save As requires an unused name. See [file formats](MBP_FORMAT.md) and [VFS](VFS.md).

## Shared canvas service

MegaPaint queries the client/content rectangles instead of hardcoding screen positions. [Window layout](WINDOW_LAYOUT.md) defines coordinate conversion. The image source/context stays valid while attached; close the canvas before reusing its storage.

The OS owns PAL2 for the sixteen image colours and restores desktop palettes on close. Legal colours use RGB333 channels 0..7. Shared swatches retain their colours during hover/focus. [Cursor styles](CURSORS.md) use the separate OS pointer palette.

Only dirty visible patterns are uploaded. The native tile callback supports aligned 1:1 views. Updates are bounded to sixteen tiles per refresh for aligned native views, eight for scaled/unaligned views and one for pixel-only sources. Full view changes settle progressively; use `mb_canvas_ready` to check pending work.

Fill processes four tiles per app tick and can be cancelled during discovery. File encoding and validation are synchronous and can temporarily pause interaction. Temporary fill and file/import workspaces overlap; these operations must not run simultaneously. There is no undo.

Pencil/Eraser presets are 1, 2, 4 and 8 pixels, centred on the cursor. Even sizes straddle its pixel; a 4-pixel brush uses offsets -2..+1. Image-edge clipping never shifts the stroke inward, and dirty regions use the same clipped footprint.

See [public SDK contracts](api/core.md), [picture model](../apps/creative/paint_model.h) and [testing](testing.md) for integration and verification.
