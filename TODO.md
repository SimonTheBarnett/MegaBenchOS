# What's next for MegaBenchOS

A working list for future development. These are ideas and research tasks, not features already available. Keep the desktop fun, build on the public SDK and measure each addition against the console's RAM, VRAM and timing limits.

## Bring your own files

- [ ] Let users choose a list of files or a directory when starting a session, without editing C source or the app registry.
- [ ] Define a small file manifest mapping selected files to visible drive paths and app associations, with clear size and format checks.
- [ ] Investigate a supported emulator file bridge or storage-device backend so selected files are actually accessible at runtime. The ROM cannot directly read arbitrary Windows paths; this needs a real transport and VFS adapter.
- [ ] Add PowerShell import/export tools for text, MBP pictures and preconverted XGM2 music. Keep ROM bundling as a separate option for files that should travel with the cartridge.
- [ ] Make missing files, unsupported formats and full storage understandable from the desktop, while preserving open documents.

## Drives and persistent storage

- [ ] Choose and document a first real storage target: supported cartridge SRAM, an accessible SD interface or a small external storage adapter. Verify its actual protocol; a flash cart's SD menu alone does not provide OS file access.
- [ ] Add the backend behind the existing VFS, with read-only access first and checked writes once reliable.
- [ ] Evaluate a filesystem suited to the chosen medium and measured capacity. Revisit littlefs for suitable storage; retain the compact backend for the current 4 KiB RAM Disk.
- [ ] Introduce **Drives** when multiple devices are supported, retaining RAM Disk and the existing file associations.
- [ ] Support copying files between drives, free-space reporting and safe removal where applicable. Test interrupted writes, damaged media and file replacement.
- [ ] Once persistent storage works, remember themes, pointer settings and Snake's best score between boots.

## PS/2 mouse and keyboard

- [ ] Research simple PS/2-to-joystick-port adapters or small external modifications, preferably without altering the console itself. Compare direct software-driven interfaces with a small active adapter where needed.
- [ ] Document a tested parts list, connector orientation, wiring, electrical requirements and the required software protocol before publishing a build guide. Aim for inexpensive, easy-to-build adapters.
- [ ] Extend the existing mouse abstraction with PS/2 movement and buttons, keeping controller navigation and Sega Mouse support working.
- [ ] Add a shared keyboard API for key press/release, modifiers, text input and bounded repeat. Start with ordinary ASCII typing and navigation keys.
- [ ] Use physical typing in MegaPad and filename dialogs, then add useful keyboard shortcuts in MegaPaint and the desktop.
- [ ] Decide how mouse, keyboard and controller share the two ports. Handle absent or disconnected devices without stalling drawing, input or music.
- [ ] Test adapters on actual PAL and NTSC consoles, including input latency while XGM2 music plays.

Research starting points: [SMDT's PS/2 and mouse documentation](https://github.com/SweMonkey/smdt#devices) and [SGDK 2.11's peripheral interface](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/joy.h). These are references to investigate, not verified MegaBenchOS adapter designs.

## A few worthwhile app upgrades

- [ ] **MegaPad:** selection, copy/paste and a small undo history, sized to fit the available memory.
- [ ] **MegaPaint:** bounded undo, line/rectangle tools and reusable palettes. Keep native pixel editing and explicit capacity errors.
- [ ] **MegaPlayer:** playlists and replay controls; later, validated external preconverted XGM2 files once storage and buffer lifetime support them. Keep the existing engine and offline conversion workflow.
- [ ] **Desktop:** shared file pickers that can select any available drive, with consistent controller, mouse and keyboard controls.

## Make it easier to share and run

- [ ] Produce a release ZIP with the built ROM and a short first-launch guide, so players can start without installing a compiler.
- [ ] Offer a useful import/export path for session files, so RAM Disk work can be taken back to the PC when a transport exists.
- [ ] Expand real-hardware checks for CRT overscan, controller and adapter compatibility, audio balance and longer sessions.
- [ ] Investigate PAL's extra display height only if the shared window API, canvas memory and transfer budgets can support it reliably.
- [ ] Keep installation and focused tests working in Windows 11's built-in Windows PowerShell 5.1, with no Python dependency.

## Boundaries

Continue using C, SGDK and the shared SDK. External data files come before executable app loading; dynamic apps, general multitasking and new playback engines would need separate decisions. Choose hardware and filesystem designs after prototypes and measurements, rather than committing to an untested interface here.

## Licensing

Release packages and imported asset manifests should preserve their notices and source credits. See [project licensing](README.md#licensing), [music credits](music/CREDITS.md) and [dependency notices](docs/dependencies.md).
