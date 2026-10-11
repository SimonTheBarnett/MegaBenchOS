# Window layout and menus

The renderer uses an 8x8 tile grid in a 320x224 display, at NTSC 60 Hz or PAL 50 Hz.

| Mode | Header | Client |
| --- | --- | --- |
| Desktop | One 8-pixel row with MegaBenchOS and available heap | Origin (0,1), 40x27 tiles |
| Normal | System row and 8-pixel window title | 30x14 tiles, within a bordered 32x16 frame |
| Maximised | One combined row with app name, heap and close control | Origin (0,1), 40x27 tiles |
| Fullscreen | No system or title rows | Origin (0,0), 40x28 tiles |

Normal titles are draggable; their position survives maximise/restore. Headers expose a close control. C+Start cycles permitted window modes; held Start+B exits the app, including exclusive fullscreen.

## Geometry

Query `mb_window_get_client_rect` after resize/mode changes. Rectangle origins are absolute screen tiles; drawing/widget coordinates are client-relative. `mb_window_get_content_rect(menu_rows, &rect)` excludes the requested menu rows. Subtract the client origin before passing content coordinates to canvas/drawing APIs.

An app with a one-row menu has a maximised 40x26-tile content area, or 320x208 pixels. Apps without menus use the whole client. PAL240 and H32 are unsupported.

## Menus

`mb_ui_menu_button` places one-row headings at client row zero. Mark the open heading with `MB_UI_ENABLED | MB_UI_MENU_OPEN` on redraw. `mb_ui_event` returns `MB_UI_MENU_DISMISS` when that heading is selected again or a pointer clicks outside registered items. Selecting a different heading returns its usual ID so the app can switch menus.

Remove the overlay on dismissal and consume A until release, preventing the dismissing click from drawing into underlying content. Back interception can cancel menus while preserving the universal exit shortcut. Menu headings and ordinary two-row buttons share the same widget pool.

## Desktop restart

Restart appears at the bottom right of the root desktop. Down from the last folder row focuses it; a direction returns to folder selection. The confirmation initially selects No. B cancels. Confirming Yes resets the OS, stops audio, clears session files/state and replays the boot logo.

See [API](API.md) for event ordering, ownership and error handling.
