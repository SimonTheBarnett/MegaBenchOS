# App cursors

[MegaBenchOS](../README.md) / [Documentation](README.md) / App cursors

The shared SDK supports brush-sized canvas markers and app-defined cursor bitmaps. Apps provide data; the OS handles patterns, sprites, palettes, hotspots and cleanup. These are additive source API 1.7 services; the app descriptor stays v1.

## Canvas brush markers

Set the logical footprint with `mb_canvas_brush(size)` (1–8 pixels), then select the style with `mb_canvas_cursor(style)`:

| Style | Appearance |
| --- | --- |
| 0 | Legacy outlined brush cursor. |
| 1 | Fixed three-screen-pixel dot. |
| 2 | Single-colour marker matching the brush footprint, without a white centre. |

MegaPaint uses style 2. Its marker grows with brush size and integer zoom, centred on the actual stroke. A 4-pixel brush covers offsets −2 through +1 from the cursor pixel; at canvas edges the stroke clips rather than shifting inward. A 1-pixel brush shows a single pixel. Fit's half-size overview rounds the footprint up to at least one screen pixel.

The marker has a transparent interior. Footprints above 16 screen pixels use four corner markers at the footprint's boundaries rather than allocating patterns for a large full outline. The maximum 8-pixel brush at 8× zoom spans 64 screen pixels. Move the cursor aside to inspect covered pixels. Menus retain the ordinary pointer.

## Custom bitmaps

`mb_cursor_set(&cursor, scope)` copies a bitmap up to **16×16 screen pixels**, with a hotspot inside it. Each `uint32_t` row holds sixteen left-aligned 2-bit pixels: 0 transparent, 1 dark, 2 light and 3 accent, using the shared theme palette. Pixels outside the declared dimensions are ignored. Shapes keep their screen size when zoom changes; use the canvas marker above for a brush that scales with the image.

```c
/* A three-pixel dot, with its hotspot in the centre. */
MB_Cursor cursor = {
    3, 3, 1, 1,
    {0x10000000, 0x54000000, 0x10000000}
};
MB_Result result = mb_cursor_set(&cursor, MB_CURSOR_CLIENT);
if (result != MB_OK) {
    /* Keep using the existing OS cursor. */
}
```

| Scope | Behaviour |
| --- | --- |
| `MB_CURSOR_CLIENT` | Custom pointer throughout the client, including menus, in pointer mode; normal pointer over title-bar chrome. |
| `MB_CURSOR_CANVAS` | Custom cursor at the logical drawing point of an attached source canvas; normal pointer over menus. Works with controller drawing or pointer input. |

The hotspot remains at the input coordinate, independently of bitmap dimensions. The descriptor and all sixteen rows are copied synchronously; stack-local source data is safe and can be changed immediately after the call. Failure preserves the installed cursor.

Call `mb_cursor_reset()` to restore the ordinary pointer or selected canvas marker. It also cancels an upload that has not happened yet. App exit, window reset and OS restart reset custom cursors automatically; closing a canvas resets its canvas-scoped custom cursor. Theme changes use the current semantic colours without borrowing an app palette.

## Limits and errors

- `MB_INVALID`: null descriptor, dimensions outside 1–16, hotspot outside the bitmap or unknown scope.
- `MB_UNAVAILABLE`: no active app/window, or canvas scope without an attached source canvas.
- `MB_BUSY`: the app holds an exclusive graphics lease. The lease retains all its palette and sprite ownership; OS cursors stay hidden while leased.

The OS uses one fixed copied descriptor, no heap allocation, four reserved patterns (128 VRAM bytes) and at most four pointer sprites. Custom patterns occupy the existing UI reservation at `TILE_USER_INDEX+60..63`, below the canvas and exclusive lease ranges. Uploads run in the normal video flush; movement only updates sprite positions. Repeated sets before a flush replace the pending bitmap.

See the [complete API contracts](api/core.md#cursorset), [implementation](../desktop/video.c) and [SDK guide](API.md).

## Licensing

Original cursor code follows the [project licence](../README.md#licensing).
