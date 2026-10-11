# Testing

Native tools and tests require the configured SGDK toolchain, LLVM clang/lld and Windows C runtime headers/libraries. Automated emulator tests also require the pinned Genesis Plus GX x64 libretro core; RetroArch is not required. See [installation](INSTALL.md) for setup.

## Test levels

```powershell
. ./scripts/env.ps1
```

| Level | Command | Scope |
| --- | --- | --- |
| Unity unit/integration fixtures | `make test-unit` | Portable production models: Calculator, Snake, layout, formatting, filesystem and editors; no emulator |
| Standard validation | `make test` | Builds the ROM, runs Unity fixtures, then checks generated assets, music provenance, ROM integrity and public-header-only app compilation |
| Emulator compatibility | `make test-emulator` | Builds release/contract ROMs and runs all GX scenarios in PAL and NTSC, one cycle |
| Combined regression | `make compatibility` | Standard validation plus full emulator compatibility |
| Interactive/hardware | `make run` or `make run REGION=pal` | Launches BlastEm for the checklist below; physical consoles require a person and a cartridge |

Unit fixtures cover malformed inputs, capacity failures and save/reopen equality. Emulator suites drive controller and emulated Sega Mouse input, checking desktop navigation, app lifecycle, window modes, themes, file operations, drawing, music and graphics/audio restoration. Contract ROMs exercise API errors, ownership and hardware-state restoration.

## GitHub Actions and local checks

[GitHub Actions](../.github/workflows/check.yml) runs `make build`, `make test`, `make lint`, `make test-emulator SCENARIO=contracts TEST_REGION=both`, and `make package`. The smoke test runs the actual API-contract ROM headlessly in both regions using the existing hash-verified GX core. CI requires two PASS results; a missing core cannot count as a pass. Reports and ROMs are uploaded as artifacts.

Run `make compatibility` locally for all application scenarios, including full-song playback. Optional storage fixtures, interactive BlastEm checks and physical hardware checks also run locally. The CI smoke test does not establish visual quality, subjective audio quality or real hardware compatibility. Core downloads use the existing lock: if upstream changes the archive, setup fails rather than accepting an unverified binary.

## Test sections

| Section | Production code and coverage |
| --- | --- |
| Filesystem | VFS/RAM Disk, handles, paths, atomic replacement, file associations; `tests/host/creative.c` and creative emulator fixture |
| Desktop | Geometry, menus, focus, themes, window modes and Restart; model and UI/creative emulator fixtures |
| Applications | Calculator errors, Snake rules, editors and lifecycle; model and app emulator fixtures |
| Input | Queued Snake turns, controller focus/pointer and emulated Sega Mouse; model and UI/paint/demo fixtures |
| Graphics | Sparse tiles, MBP1/2, brush/palette/zoom/pan, captures and restoration; paint and demo fixtures |
| Audio | VGM validation, provenance, bounded capture analysis, five full songs, Stop and PSG cleanup; host and music/UI/demo fixtures |
| SDK | Public-header compilation, complete generated contracts, negative API checks; native host checks and target contract ROM |
| Storage | Volume capacity, corrupt-input rejection, ROM validation and optional littlefs faults; host and target storage fixtures |

`tests/unit/` and `tests/host/` link actual portable production C. `tests/emulator/` drives the actual ROM through the GX C API. The privileged target harness uses its existing bounded mailbox checks rather than linking a host framework into the ROM.

## Focused emulator checks

Use a focused Make command after a change to one app or service:

```powershell
make test-emulator SCENARIO=ui TEST_REGION=ntsc
make test-emulator SCENARIO=creative TEST_REGION=both
make test-emulator SCENARIO=paint TEST_REGION=both
make test-emulator SCENARIO=music TEST_REGION=both
make test-emulator SCENARIO=demo7 TEST_REGION=both
make test-emulator SCENARIO=contracts TEST_REGION=both
```

The default is `SCENARIO=all TEST_REGION=both CYCLES=1`. `make compatibility` runs host checks and all 20 release/contract scenarios. Demo IDs are 7 MegaBounce, 8 ColourStorm, 9 Ocean Dreams and 10 StarDrive. `contracts-music` checks malformed music files and associations; `legacy-paint` checks MBP1 import. Paint tests cover palettes, centred brushes, zoom/pan and lossless saves. Music tests play all five tracks to completion, then verify restart, Stop, credits and exit cleanup. Repeat affected checks after failures; extra cycles are optional.

Make compiles `out/host/megabench-tools.exe`. For an already-built, source-matched ROM, run a single scenario directly:

```powershell
./out/host/megabench-tools.exe emulator --scenario paint --region ntsc --cycles 1
./out/host/megabench-tools.exe --help
```

Each emulator scenario runs in a separate process with a four-minute timeout. The runner drives controller and Sega Mouse input through the pinned GX core, inspects read-only observations, and saves actual frames/audio. It does not inject game state or automate BlastEm input.

The optional storage fixture is excluded from release builds:

```powershell
make test-littlefs
make littlefs
./out/host/megabench-tools.exe emulator --scenario littlefs --region both
```

It tests four host and four target configurations without enabling persistent storage in the OS.

## Results

Reports, screenshots and audio captures stay in ignored `reports/`. Unity prints file/line diagnostics and returns nonzero on failure. Host results are in `reports/host-test-results.json`; emulator results are in `reports/emulator/<scenario>/<region>/report.json`, with a full-run `summary.json`. Build reports are in `dist/`. Tests reject ROMs whose hash or source stamp is stale; rebuild rather than bypassing validation.

- **PASS:** the named automated or manual check completed successfully.
- **FAIL:** a check failed; resolve it before merging.
- **MANUAL:** interaction or hardware checks still require a person.
- **SKIPPED:** the check was not run, with its reason recorded.

Contract completion uses mailbox value `0x600D`; a failure code identifies the failed gate in [tests/target/contracts.c](../tests/target/contracts.c). Work counters are coarse observations, not CPU-cycle profiles or guarantees of hardware deadlines. A successful build or emulator launch alone does not prove correct playback or interaction.

## Interactive and hardware checks

Run `./scripts/run-blastem.ps1 -Region ntsc` or `-Region pal`. Use one audible emulator at a time. Check labels and clipping, menu dismissal, brush position, save/reopen, app exit and clean audio stops.

For a native music smoke test, build with `make build PREVIEW=megaplayer MUSIC_SMOKE=1`, then run the corresponding preview ROM. The harness displays completion and regional frame counts. Optional audio analysis uses the host utility: retain its completion screenshot as `reports/emulator/megaplayer/native/<region>/completed.jpg` and pass the captured raw audio path and five completed frame counts. Capture configuration must use SDL F32 stereo at 48000 Hz.

```powershell
./out/host/megabench-tools.exe native-audio --region ntsc --raw out/capture.raw --frames 1,2,3,4,5
```

Replace the example counts with the five values on the observed PASS screen. Without that native capture and observation, this check remains MANUAL.

BlastEm and Genesis Plus GX are the emulator targets. Physical PAL/NTSC consoles, flash cartridges, CRT overscan and a physical Sega Mouse still need hardware verification. Record device models, firmware, region and ROM hash, then check:

- Cold boot and Restart, readable desktop and windows, and controller navigation.
- Every app's controls, universal exit and restoration after fullscreen.
- MegaPad/MegaPaint create, save, reopen and capacity-error handling; Restart empties RAM Disk.
- Snake countdown, queued turns, difficulty, collisions, pause and retry.
- Music and PSG effects, Stop and exit, with no stuck or overlapping audio.
- Themes, window dragging, port-2 Sega Mouse movement and buttons.
- Continuous runtime against a stopwatch; oscillator drift is not calibrated.

The current writable filesystem is volatile RAM only. Cartridge SD menus do not provide in-app SD access. Keyboard, RTC, networking, H32 and PAL240 are unsupported.

## Adding tests

Add portable C cases to `tests/unit/`, register them in `main.c`, and use Unity assertions against production functions. Existing filesystem and editor fixtures are in `tests/host/`; hardware-dependent API contracts remain in `tests/target/`. Make discovers `tests/unit/*.c` automatically; add any production dependencies to `UNIT_SOURCES` in the root Makefile. No generated test runner is required.

Add emulator assertions to the appropriate scenario in `tests/emulator/`. Shared core/process/report handling lives in `tools/native/`. Register a new scenario in `runner.c` when needed. Use controller or mouse input and read-only observations; test both PAL and NTSC. All native and emulator fixtures use Unity; no project C#/.NET dependency is present.
Use descriptive `test_<section>_<behaviour>` names. State each file's purpose and test layer in a short `@file` comment. Explain unusual fixtures and assertions without narrating every statement. Reset mutable state in `setUp` or explicitly in each fixture; reserve `tearDown` for resources acquired by that fixture. Existing model cases reset their production models directly. The emulator runner always closes the core after the Unity case and records a failure frame when possible.

```c
/** Verify the production formatter includes the terminator in capacity checks. */
void test_sdk_formatter_rejects_short_buffer(void) {
    char text[1] = {'x'};
    TEST_ASSERT_EQUAL_INT(MB_INVALID, mb_format_u32(12, text, sizeof text));
}
```

Follow the real function signature in the [SDK reference](api/core.md); register a fixture with `RUN_TEST` in its native runner. A failed assertion must fail the command, not merely print a warning. Emulator fixtures use `A(condition, message)` for the same Unity failure behaviour and assertion evidence.

Run `make format` after C edits and `make lint` to check the repository's 100-column style and run clang-tidy on the native tools and emulator fixtures. SGDK and host compilation also treat warnings as errors. Vendored files and generated headers are excluded. `make verify-clean` builds/tests/packages a source copy under an ignored path containing spaces, with no local config or Python on its process PATH. To include the full emulator suite in that copy, run `./out/host/megabench-tools.exe clean-workflow --skip-emulator 0`.
