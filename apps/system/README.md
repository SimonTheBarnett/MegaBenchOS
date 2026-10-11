# System

[MegaBenchOS](../../README.md) / [Apps](../README.md) / System

Tune the desktop and see what's going on under the hood. System brings together appearance, pointer speed, live input, memory, storage and console information.

## Explore

Open a tool from the System folder, or press Start on the desktop for the System menu. **Settings** offers three pointer speeds and five themes, with preview and Apply. **Input Test** shows controller buttons, pointer coordinates and interaction mode. **Memory**, **Storage** and **System Info** show the current session's figures.

Use D-pad and A, or pointer controls, to choose options. Back returns to the tool menu; B closes the window. Fullscreen supports Exit and held Start+B.

Themes and pointer settings last for the session. RAM files are temporary; storage information also describes the bundled ROM.

## Development

[system.c](system.c) reads public service APIs and uses launch context to open the requested page. Theme previews are cancelled when the app closes. See the [SDK](../../sdk/README.md), [storage capabilities](../../docs/VFS.md), [window layout](../../docs/WINDOW_LAYOUT.md) and [testing](../../docs/testing.md).
