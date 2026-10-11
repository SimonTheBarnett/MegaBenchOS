# MegaPaint

[MegaBenchOS](../../../README.md) / [Apps](../../README.md) / [Creative apps](../README.md) / MegaPaint

Pixel art with room to draw. MegaPaint opens onto a native **320x208 canvas**, with 16 colours, a centred brush-sized cursor and compact menus that get out of the way.

## Tools and colours

**File** handles New, Open, Save and Save As. **Tools** offers Pencil, Eraser, Fill and Clear, with 1, 2, 4 and 8-pixel square brushes for Pencil and Eraser, centred on the cursor and clipped at canvas edges. **Colours** brings up the 16 swatches; Edit RGB adjusts each channel through the Mega Drive's eight hardware levels.

**View** offers Fit, 1:1, Zoom In, Zoom Out and Pan. Zoom uses 2x, 4x and 8x steps. Larger images can be panned; Fit can show a half-size overview without changing the picture. Choose 1:1 for exact pixel editing.

New also offers 512x256, 256x256 and 64x64 pictures. The default canvas uses one image pixel per screen pixel.

## Controls

Start opens File, B closes an open menu, and A draws or selects. D-pad moves the pointer freely between canvas and menus, with no mode switch needed. C optionally switches to precise controller drawing and back. Mouse input uses the shared Sega Mouse support. Select an open menu heading again to close it, select another to switch, or click the canvas to dismiss it before drawing.

Pan uses D-pad or pointer dragging. Hold Start+B to exit, or use the window's close button. Unsaved changes ask before closing or replacing the picture. There is no undo yet.

## Files and limits

Pictures save to `RAM:/Pictures/` as lossless MBP2 files containing their dimensions, pixels and palette. Earlier MBP1 pictures still open. Save updates the current file; Save As chooses a new name. RAM Disk files last for the current session.

Large, mostly solid pictures are practical, but very detailed images can reach the editor's tile limit or fill RAM Disk. Both cases report an error and preserve your work; the editor never silently shrinks a picture or truncates a save.

## Development

See [canvas limits](../../../docs/MEGAPAINT.md), [picture formats](../../../docs/MBP_FORMAT.md), the [SDK](../../../sdk/README.md) and [building and packaging](../../../docs/BUILD.md).

For crisp pixel inspection in BlastEm, use nearest filtering and integer scaling.

## Licensing

See [project licensing](../../../README.md#licensing).
